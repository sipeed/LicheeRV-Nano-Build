#include <vi.h>
#include <linux/cvi_base_ctx.h>
#include <linux/of_gpio.h>
#include <proc/vi_dbg_proc.h>
#include <proc/vi_proc.h>
#include <proc/vi_isp_proc.h>
#include <vi_ext.h>
#include <base_cb.h>
#include <base_ctx.h>
#include <cif_cb.h>
#include <vpss_cb.h>
#include <vi_cb.h>
#include <dwa_cb.h>
#include <vb.h>
#include <vip/vi_perf_chk.h>
#include <vcodec_cb.h>
#include <vi_raw_dump.h>

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define DMA_SETUP_2(id, raw_num)					\
	do {								\
		bufaddr = _mempool_get_addr();				\
		ispblk_dma_setaddr(ictx, id, bufaddr);			\
		bufsize = ispblk_dma_buf_get_size(ictx, id, raw_num);	\
		_mempool_pop(bufsize);					\
	} while (0)

#define DMA_SETUP(id, raw_num)						\
	do {							\
		bufaddr = _mempool_get_addr();			\
		bufsize = ispblk_dma_config(ictx, id, raw_num, bufaddr); \
		_mempool_pop(bufsize);				\
	} while (0)

#define VI_SHARE_MEM_SIZE	(0x2000)
#define VI_PROFILE
#define VI_MAX_SNS_CFG_NUM	(0x10)

/* In practical application, it is necessary to drop the frame of AE convergence process.
 * But in vi-vpss online & vpss-vc sbm scenario, there is no way to drop the frame.
 * Use cover with black to avoid this problem.
 */
#ifndef PORTING_TEST
#define COVER_WITH_BLACK
#endif
/*******************************************************
 *  Global variables
 ******************************************************/
//u32 vi_log_lv = VI_ERR | VI_WARN | VI_NOTICE | VI_INFO | VI_DBG;
u32 vi_log_lv = 0;
module_param(vi_log_lv, int, 0644);

#ifdef PORTING_TEST //test only
int stop_stream_en;
module_param(stop_stream_en, int, 0644);
#endif

bool ctrl_flow = false;
module_param(ctrl_flow, bool, 0644);

struct cvi_vi_ctx *gViCtx;
struct cvi_overflow_info *gOverflowInfo;
struct _vi_gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};
struct cvi_gdc_mesh g_vi_mesh[VI_MAX_CHN_NUM];

/*******************************************************
 *  Internal APIs
 ******************************************************/

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);

	lt->function(lt->data);
}
#endif //(KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)

/**
 * _mempool_reset - reset the byteused and assigned buffer for each dma
 *
 */
static void _vi_mempool_reset(void)
{
	u8 i = 0;

	isp_mempool.byteused = 0;

	memset(isp_bufpool, 0x0, (sizeof(struct _membuf) * ISP_PRERAW_VIRT_MAX));

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		spin_lock_init(&isp_bufpool[i].pre_fe_sts_lock);
		spin_lock_init(&isp_bufpool[i].pre_be_sts_lock);
		spin_lock_init(&isp_bufpool[i].post_sts_lock);
	}
}

/**
 * _mempool_get_addr - get mempool's latest address.
 *
 * @return: the latest address of the mempool.
 */
static uint64_t _mempool_get_addr(void)
{
	return isp_mempool.base + isp_mempool.byteused;
}

/**
 * _mempool_pop - acquire a buffer-space from mempool.
 *
 * @param size: the space acquired.
 * @return: negative if no enough space; o/w, the address of the buffer needed.
 */
static int64_t _mempool_pop(uint32_t size)
{
	int64_t addr;

	size = VI_ALIGN(size);

	if ((isp_mempool.byteused + size) > isp_mempool.size) {
		vi_pr(VI_ERR, "reserved_memory(0x%x) is not enough. byteused(0x%x) alloc_size(0x%x)\n",
				isp_mempool.size, isp_mempool.byteused, size);
		return -EINVAL;
	}

	addr = isp_mempool.base + isp_mempool.byteused;
	isp_mempool.byteused += size;

	return addr;
}

static CVI_VOID vi_gdc_callback(CVI_VOID *pParam, VB_BLK blk)
{
	struct _vi_gdc_cb_param *_pParam = pParam;

	if (!pParam)
		return;

	vi_pr(VI_DBG, "ViChn(%d) usage(%d)\n", _pParam->chn.s32ChnId, _pParam->usage);
	mutex_unlock(&g_vi_mesh[_pParam->chn.s32ChnId].lock);
	if (blk != VB_INVALID_HANDLE)
		vb_done_handler(_pParam->chn, CHN_TYPE_OUT, blk);
	vfree(pParam);
}

static CVI_S32 _mesh_gdc_do_op_cb(enum GDC_USAGE usage, const CVI_VOID *pUsageParam,
				struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat, CVI_U64 mesh_addr,
				CVI_BOOL sync_io, CVI_VOID *pcbParam, CVI_U32 cbParamSize,
				MOD_ID_E enModId, ROTATION_E enRotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.pUsageParam = pUsageParam;
	cfg.vb_in = vb_in;
	cfg.enPixFormat = enPixFormat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.pcbParam = pcbParam;
	cfg.cbParamSize = cbParamSize;
	cfg.enRotation = enRotation;

	exe_cb.callee = E_MODULE_DWA;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = DWA_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

void _isp_snr_cfg_enq(struct cvi_isp_snr_update *snr_node, const enum cvi_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_snr_i2c_node *n, *q;
	struct _isp_crop_node  *c_n;

	if (snr_node == NULL)
		return;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);

	if (snr_node->snr_cfg_node.isp.need_update) {
		c_n = kmalloc(sizeof(*c_n), GFP_ATOMIC);
		if (c_n == NULL) {
			vi_pr(VI_ERR, "SNR cfg node alloc size(%zu) fail\n", sizeof(*n));
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
			return;
		}
		memcpy(&c_n->n, &snr_node->snr_cfg_node.isp, sizeof(struct snsr_isp_s));
		list_add_tail(&c_n->list, &isp_crop_queue[raw_num].list);
	}

	if (snr_node->snr_cfg_node.snsr.need_update) {
		n = kmalloc(sizeof(*n), GFP_ATOMIC);
		if (n == NULL) {
			vi_pr(VI_ERR, "SNR cfg node alloc size(%zu) fail\n", sizeof(*n));
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
			return;
		}
		memcpy(&n->n, &snr_node->snr_cfg_node.snsr, sizeof(struct snsr_regs_s));

		while (!list_empty(&isp_snr_i2c_queue[raw_num].list)
			&& (isp_snr_i2c_queue[raw_num].num_rdy >= (VI_MAX_SNS_CFG_NUM - 1))) {
			q = list_first_entry(&isp_snr_i2c_queue[raw_num].list, struct _isp_snr_i2c_node, list);
			list_del_init(&q->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			kfree(q);
		}
		list_add_tail(&n->list, &isp_snr_i2c_queue[raw_num].list);
		++isp_snr_i2c_queue[raw_num].num_rdy;
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
}

void pre_raw_num_enq(struct _isp_sof_raw_num_q *q, struct _isp_raw_num_n *n)
{
	unsigned long flags;

	spin_lock_irqsave(&raw_num_lock, flags);
	list_add_tail(&n->list, &q->list);
	spin_unlock_irqrestore(&raw_num_lock, flags);
}

void cvi_isp_buf_queue(struct cvi_vi_dev *vdev, struct cvi_isp_buf *b)
{
	unsigned long flags;

	vi_pr(VI_DBG, "buf_queue chn_id=%d\n", b->buf.index);

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	list_add_tail(&b->list, &vdev->qbuf_list[b->buf.index]);
	++vdev->qbuf_num[b->buf.index];
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
}

void cvi_isp_buf_queue_wrap(struct cvi_vi_dev *vdev, struct cvi_isp_buf *b)
{
	s8 chn_id = b->buf.index;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	u8 pre_trig = false, post_trig = false;

	vi_pr(VI_DBG, "buf_queue chn_id=%d\n", b->buf.index);

	if (_cvi_isp_next_buf(vdev, chn_id) == NULL) {
		//check raw, chn
		for (; raw_num < gViCtx->total_dev_num; raw_num++) {
			chn_id -= gViCtx->devAttr[raw_num].chn_num;
			if (chn_id < 0) {
				chn_id += gViCtx->devAttr[raw_num].chn_num;
				break;
			}
		}

		//for yuv fe->dram, if no buffer we need retrriger preraw
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (vdev->pre_fe_frm_num[raw_num][chn_id] > 0) {
				vdev->pre_fe_trig_cnt[raw_num][chn_id]++;
				vdev->isp_triggered[raw_num][chn_id] = true;
				vdev->is_yuv_trigger = true;
				pre_trig = true;
			}
		} else { //rgb sensor
			if (_is_all_online(ctx) && vdev->pre_fe_frm_num[raw_num][chn_id] > 0) {
				pre_trig = true;
			} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
				if (vdev->postraw_frame_number[raw_num] > 0) {
					post_trig = true;
				}
			}
		}
	}

	cvi_isp_buf_queue(vdev, b);

	if (pre_trig || post_trig) {
		vi_pr(VI_DBG, "raw_%d chn_%d buf empty, trigger post\n", raw_num, chn_id);
		tasklet_hi_schedule(&vdev->job_work);
	}
}

struct cvi_isp_buf *_cvi_isp_next_buf(struct cvi_vi_dev *vdev, const u8 chn_num)
{
	unsigned long flags;
	struct cvi_isp_buf *b = NULL;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	if (!list_empty(&vdev->qbuf_list[chn_num]))
		b = list_first_entry(&vdev->qbuf_list[chn_num], struct cvi_isp_buf, list);
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);

	return b;
}

int cvi_isp_rdy_buf_empty(struct cvi_vi_dev *vdev, const u8 chn_num)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	empty = (vdev->qbuf_num[chn_num] == 0);
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);

	return empty;
}

void cvi_isp_rdy_buf_pop(struct cvi_vi_dev *vdev, const u8 chn_num)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	vdev->qbuf_num[chn_num]--;
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
}

void cvi_isp_rdy_buf_remove(struct cvi_vi_dev *vdev, const u8 chn_num)
{
	unsigned long flags;
	struct cvi_isp_buf *b = NULL;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	if (!list_empty(&vdev->qbuf_list[chn_num])) {
		b = list_first_entry(&vdev->qbuf_list[chn_num], struct cvi_isp_buf, list);
		list_del_init(&b->list);
		kfree(b);
	}
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
}

/**
 * when rgbmap use frame buffer mode and seglen is not equal to the stride,
 * mmap will read garbage data on the right side, so fill 0x80 instead of the garbage.
 */
void isp_fill_rgbmap(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t ba;
	uint32_t dmaid;
	uint32_t seglen, stride, max_size;
	uint8_t i = 0;
	void *vaddr = NULL;

	if (ctx->is_rgbmap_sbm_on)
		return;

	if (raw_num == ISP_PRERAW_A) {
		if (chn_num == ISP_FE_CH0)
			dmaid = ISP_BLK_ID_DMA_CTL10;
		else if (chn_num == ISP_FE_CH1)
			dmaid = ISP_BLK_ID_DMA_CTL11;
	} else if (raw_num == ISP_PRERAW_B) {
		if (chn_num == ISP_FE_CH0)
			dmaid = ISP_BLK_ID_DMA_CTL16;
		else if (chn_num == ISP_FE_CH1)
			dmaid = ISP_BLK_ID_DMA_CTL17;
	} else if (raw_num == ISP_PRERAW_C) {
		if (chn_num == ISP_FE_CH0)
			dmaid = ISP_BLK_ID_DMA_CTL20;
	}

	ba = ctx->phys_regs[dmaid];
	seglen = ISP_RD_BITS(ba, REG_ISP_DMA_CTL_T, DMA_SEGLEN, SEGLEN);
	stride = ISP_RD_BITS(ba, REG_ISP_DMA_CTL_T, DMA_STRIDE, STRIDE);
	max_size = ((((UPPER(ctx->isp_pipe_cfg[raw_num].crop.w, 3)) * 6 + 15) / 16) * 16)
			* UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, 3);

	vi_pr(VI_DBG, "raw_num=%d, chn_num=%d, seglen=%d, stride=%d, max_size=%d\n",
			raw_num, chn_num, seglen, stride, max_size);

	if ((seglen != stride) && (max_size > 0)) {
		for (i = 0; i < RGBMAP_BUF_IDX; i++) {
			if (chn_num == ISP_FE_CH0)
				vaddr = phys_to_virt(isp_bufpool[raw_num].rgbmap_le[i]);
			else if (chn_num == ISP_FE_CH1)
				vaddr = phys_to_virt(isp_bufpool[raw_num].rgbmap_se[i]);

			memset(vaddr, 0x80, max_size);
		}
	}
}

void _vi_yuv_dma_setup(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	struct isp_buffer *b;
	uint32_t bufsize_yuyv = 0;
	uint8_t  i = 0;

	struct _membuf *pool = &isp_bufpool[raw_num];

	u8 total_chn = (raw_num == ISP_PRERAW_A) ?
			ctx->rawb_chnstr_num :
			ctx->total_chn_num;
	u8 chn_str = (raw_num == ISP_PRERAW_A) ? 0 : ctx->rawb_chnstr_num;

	for (; chn_str < total_chn; chn_str++) {
		enum ISP_BLK_ID_T dma;

		if (raw_num == ISP_PRERAW_C) {
			dma = (chn_str == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19;
		} else {
			dma = (chn_str == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13;
		}

		for (i = 0; i < OFFLINE_YUV_BUF_NUM; i++) {
			b = vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "yuv_buf isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
				return;
			}
			memset(b, 0, sizeof(*b));
			b->chn_num = chn_str;
			b->is_yuv_frm = true;
			bufsize_yuyv = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			pool->yuv_yuyv[b->chn_num][i] = b->addr = _mempool_pop(bufsize_yuyv);

			if (i == 0)
				ispblk_dma_setaddr(ctx, dma, b->addr);

			isp_buf_queue(&pre_out_queue[b->chn_num], b);
		}
	}
}

static void _isp_preraw_fe_dma_dump(struct isp_ctx *ictx, enum cvi_isp_raw  raw_num)
{
	u8 i = 0;
	char str[64] = "PRERAW_FE";

	vi_pr(VI_INFO, "***************%s_%d************************\n", str, raw_num);
	for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
		vi_pr(VI_INFO, "bayer_le(0x%llx)\n", isp_bufpool[raw_num].bayer_le[i]);

	for (i = 0; i < RGBMAP_BUF_IDX; i++)
		vi_pr(VI_INFO, "rgbmap_le(0x%llx)\n", isp_bufpool[raw_num].rgbmap_le[i]);

	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
			vi_pr(VI_INFO, "bayer_se(0x%llx)\n", isp_bufpool[raw_num].bayer_se[i]);

		for (i = 0; i < RGBMAP_BUF_IDX; i++)
			vi_pr(VI_INFO, "rgbmap_se(0x%llx)\n", isp_bufpool[raw_num].rgbmap_se[i]);
	}

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path &&
		!ictx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		for (i = 0; i < ISP_CHN_MAX; i++) {
			vi_pr(VI_INFO, "yuyv_yuv(0x%llx), yuyv_yuv(0x%llx)\n",
				isp_bufpool[raw_num].yuv_yuyv[i][0], isp_bufpool[raw_num].yuv_yuyv[i][1]);
		}
	}
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_preraw_be_dma_dump(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	u8 i = 0;
	char str[64] = "PRERAW_BE";
	enum cvi_isp_raw raw = ISP_PRERAW_A;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	vi_pr(VI_INFO, "be_rdma_le(0x%llx)\n", isp_bufpool[raw].bayer_le[0]);
	for (i = 0; i < OFFLINE_PRE_BE_BUF_NUM; i++)
		vi_pr(VI_INFO, "be_wdma_le(0x%llx)\n", isp_bufpool[raw].prebe_le[i]);

	if (ictx->is_hdr_on) {
		vi_pr(VI_INFO, "be_rdma_se(0x%llx)\n", isp_bufpool[raw].bayer_se[0]);
		for (i = 0; i < OFFLINE_PRE_BE_BUF_NUM; i++)
			vi_pr(VI_INFO, "be_wdma_se(0x%llx)\n", isp_bufpool[raw].prebe_se[i]);
	}

	for (; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;

		vi_pr(VI_INFO, "***********************raw_%d*****************", raw);
		vi_pr(VI_INFO, "af(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].af.phy_addr,
				isp_bufpool[raw].sts_mem[1].af.phy_addr);
	}
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_rawtop_dma_dump(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	char str[64] = "RAW_TOP";
	enum cvi_isp_raw raw = ISP_PRERAW_A;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	vi_pr(VI_INFO, "rawtop_rdma_le(0x%llx)\n", isp_bufpool[raw].prebe_le[0]);

	if (ictx->is_hdr_on)
		vi_pr(VI_INFO, "rawtop_rdma_se(0x%llx)\n", isp_bufpool[raw].prebe_se[0]);

	for (; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;

		vi_pr(VI_INFO, "***********************raw_%d*****************", raw);
		vi_pr(VI_INFO, "lsc(0x%llx)\n", isp_bufpool[raw].lsc);
		vi_pr(VI_INFO, "ae_le(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].ae_le.phy_addr,
				isp_bufpool[raw].sts_mem[1].ae_le.phy_addr);
		vi_pr(VI_INFO, "gms(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].gms.phy_addr,
				isp_bufpool[raw].sts_mem[1].gms.phy_addr);
		vi_pr(VI_INFO, "awb(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].awb.phy_addr,
				isp_bufpool[raw].sts_mem[1].awb.phy_addr);
		vi_pr(VI_INFO, "lmap_le(0x%llx)\n", isp_bufpool[raw].lmap_le);

		if (ictx->isp_pipe_cfg[raw].is_hdr_on) {
			vi_pr(VI_INFO, "ae_se(0x%llx, 0x%llx)\n",
					isp_bufpool[raw].sts_mem[0].ae_se.phy_addr,
					isp_bufpool[raw].sts_mem[1].ae_se.phy_addr);
			vi_pr(VI_INFO, "lmap_se(0x%llx)\n", isp_bufpool[raw].lmap_se);
		}
	}

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_rgbtop_dma_dump(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	char str[64] = "RGB_TOP";
	enum cvi_isp_raw raw = ISP_PRERAW_A;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	for (; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;

		vi_pr(VI_INFO, "***********************raw_%d*****************", raw);
		vi_pr(VI_INFO, "hist_edge_v(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].hist_edge_v.phy_addr,
				isp_bufpool[raw].sts_mem[1].hist_edge_v.phy_addr);
		vi_pr(VI_INFO, "manr(0x%llx), manr_rtile(0x%llx)\n",
				isp_bufpool[raw].manr,
				isp_bufpool[raw].manr_rtile);
		vi_pr(VI_INFO, "tdnr(0x%llx, 0x%llx), tdnr_rtile(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].tdnr[0],
				isp_bufpool[raw].tdnr[1],
				isp_bufpool[raw].tdnr_rtile[0],
				isp_bufpool[raw].tdnr_rtile[1]);
	}

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_yuvtop_dma_dump(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	char str[64] = "YUV_TOP";
	enum cvi_isp_raw raw = ISP_PRERAW_A;

	uint64_t dci_bufaddr = 0;
	uint64_t ldci_bufaddr = 0;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	for (; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;

		vi_pr(VI_INFO, "***********************raw_%d*****************", raw);
		vi_pr(VI_INFO, "dci(0x%llx, 0x%llx)\n",
				isp_bufpool[raw].sts_mem[0].dci.phy_addr,
				isp_bufpool[raw].sts_mem[1].dci.phy_addr);
		vi_pr(VI_INFO, "ldci(0x%llx)\n", isp_bufpool[raw].ldci);

		// show wasted buf size for 256B-aligned ldci bufaddr
		dci_bufaddr = isp_bufpool[raw].sts_mem[1].dci.phy_addr;
		ldci_bufaddr = isp_bufpool[raw].ldci;
		vi_pr(VI_INFO, "ldci wasted_bufsize_for_alignment(%d)\n",
			(uint32_t)(ldci_bufaddr - (dci_bufaddr + 0x200)));
	}
	vi_pr(VI_INFO, "*************************************************\n");
	vi_pr(VI_INFO, "VI total reserved memory(0x%x)\n", isp_mempool.byteused);
	vi_pr(VI_INFO, "*************************************************\n");
}

void _isp_preraw_fe_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_num)
{
	uint64_t bufaddr = 0;
	uint32_t bufsize = 0;
	uint8_t  i = 0;
	struct isp_buffer *b;
	enum cvi_isp_raw ac_raw;

	u32 raw_le, raw_se;
	u32 rgbmap_le, rgbmap_se;

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		if (!ictx->isp_pipe_cfg[raw_num].is_offline_scaler) //Online mode to scaler
			_vi_yuv_dma_setup(ictx, raw_num);

		goto EXIT;
	}

	ac_raw = find_hw_raw_num(raw_num);

	if (ac_raw == ISP_PRERAW_A) {
		raw_le = ISP_BLK_ID_DMA_CTL6;
		raw_se = ISP_BLK_ID_DMA_CTL7;
		rgbmap_le = ISP_BLK_ID_DMA_CTL10;
		rgbmap_se = ISP_BLK_ID_DMA_CTL11;
	} else if (ac_raw == ISP_PRERAW_B) {
		raw_le = ISP_BLK_ID_DMA_CTL12;
		raw_se = ISP_BLK_ID_DMA_CTL13;
		rgbmap_le = ISP_BLK_ID_DMA_CTL16;
		rgbmap_se = ISP_BLK_ID_DMA_CTL17;
	} else {
		raw_le = ISP_BLK_ID_DMA_CTL18;
		raw_se = ISP_BLK_ID_DMA_CTL19;
		rgbmap_le = ISP_BLK_ID_DMA_CTL20;
		//No se in ISP_PRERAW_C
	}

	if (_is_be_post_online(ictx) && !ictx->isp_pipe_cfg[raw_num].is_offline_preraw) { //fe->dram->be->post
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
			//muxdev only use one buffer
			if ((ictx->isp_pipe_cfg[raw_num].is_mux || (raw_num > ISP_PRERAW_MAX - 1)) && i != 0)
				continue;
			DMA_SETUP_2(raw_le, raw_num);
			b = vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "raw_le isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
				return;
			}
			memset(b, 0, sizeof(*b));
			b->addr = bufaddr;
			b->raw_num = raw_num;
			b->ir_idx = i;
			isp_bufpool[raw_num].bayer_le[i] = b->addr;
			isp_buf_queue(&pre_out_queue[b->raw_num], b);
		}
		ispblk_dma_config(ictx, raw_le, raw_num, isp_bufpool[ac_raw].bayer_le[0]);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
				//muxdev only use one buffer
				if ((ictx->isp_pipe_cfg[raw_num].is_mux || (raw_num > ISP_PRERAW_MAX - 1)) && i != 0)
					continue;
				DMA_SETUP_2(raw_se, raw_num);
				b = vmalloc(sizeof(*b));
				if (b == NULL) {
					vi_pr(VI_ERR, "raw_se isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
					return;
				}
				memset(b, 0, sizeof(*b));
				b->addr = bufaddr;
				b->raw_num = raw_num;
				b->ir_idx = i;
				isp_bufpool[raw_num].bayer_se[i] = b->addr;
				isp_buf_queue(&pre_out_se_queue[b->raw_num], b);
			}
			ispblk_dma_config(ictx, raw_se, raw_num, isp_bufpool[ac_raw].bayer_se[0]);
		}
	}

	if (ictx->is_3dnr_on) {
		// rgbmap_le
		DMA_SETUP_2(rgbmap_le, raw_num);
		isp_bufpool[raw_num].rgbmap_le[0] = bufaddr;
		ispblk_rgbmap_dma_config(ictx, ac_raw, rgbmap_le);
		ispblk_dma_setaddr(ictx, rgbmap_le, isp_bufpool[ac_raw].rgbmap_le[0]);

		//Slice buffer mode use ring buffer
		if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
			if (!_is_all_online(ictx)) {
				for (i = 1; i < RGBMAP_BUF_IDX; i++)
					isp_bufpool[raw_num].rgbmap_le[i] = _mempool_pop(bufsize);
			}
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// rgbmap se
			DMA_SETUP_2(rgbmap_se, raw_num);
			isp_bufpool[raw_num].rgbmap_se[0] = bufaddr;
			ispblk_rgbmap_dma_config(ictx, ac_raw, rgbmap_se);
			ispblk_dma_setaddr(ictx, rgbmap_se, isp_bufpool[ac_raw].rgbmap_se[0]);

			//Slice buffer mode use ring buffer
			if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
				for (i = 1; i < RGBMAP_BUF_IDX; i++)
					isp_bufpool[raw_num].rgbmap_se[i] = _mempool_pop(bufsize);
			}
		}
	}

EXIT:
	_isp_preraw_fe_dma_dump(ictx, raw_num);
}

void _isp_preraw_be_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint64_t bufaddr = 0;
	uint32_t bufsize = 0;
	uint8_t  buf_num = 0;
	struct isp_buffer *b;

	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false)
		goto EXIT;

	if (ictx->is_offline_be && !ictx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
		//apply pre_fe_le buffer
		ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL4, isp_bufpool[ISP_PRERAW_A].bayer_le[0]);

		if (ictx->is_hdr_on) {
			//apply pre_fe_se buffer
			ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL5, isp_bufpool[ISP_PRERAW_A].bayer_se[0]);
		}
	}

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		if (ictx->is_slice_buf_on) {
			DMA_SETUP_2(ISP_BLK_ID_DMA_CTL22, raw_num);
			isp_bufpool[raw_num].prebe_le[0] = bufaddr;

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
				DMA_SETUP_2(ISP_BLK_ID_DMA_CTL23, raw_num);
				isp_bufpool[raw_num].prebe_se[0] = bufaddr;
			}
		} else {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				DMA_SETUP_2(ISP_BLK_ID_DMA_CTL22, raw_num);
				b = vmalloc(sizeof(*b));
				if (b == NULL) {
					vi_pr(VI_ERR, "be_wdma_le isp_buf_%d vmalloc size(%zu) fail\n",
						buf_num, sizeof(*b));
					return;
				}
				memset(b, 0, sizeof(*b));
				b->addr = bufaddr;
				b->raw_num = raw_num;
				b->ir_idx = buf_num;
				isp_bufpool[raw_num].prebe_le[buf_num] = b->addr;
				isp_buf_queue(&pre_be_out_q, b);
			}

			ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL22, isp_bufpool[raw_num].prebe_le[0]);

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					DMA_SETUP_2(ISP_BLK_ID_DMA_CTL23, raw_num);
					b = vmalloc(sizeof(*b));
					if (b == NULL) {
						vi_pr(VI_ERR, "be_wdma_se isp_buf_%d vmalloc size(%zu) fail\n",
								buf_num, sizeof(*b));
						return;
					}
					memset(b, 0, sizeof(*b));
					b->addr = bufaddr;
					b->raw_num = raw_num;
					b->ir_idx = buf_num;
					isp_bufpool[raw_num].prebe_se[buf_num] = b->addr;
					isp_buf_queue(&pre_be_out_se_q, b);
				}

				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL23, isp_bufpool[raw_num].prebe_se[0]);
			}
		}
	}

	for (; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		//Be out dma
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path)
			continue;

		// af
		DMA_SETUP(ISP_BLK_ID_DMA_CTL21, raw_num);
		isp_bufpool[raw_num].sts_mem[0].af.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].af.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].af.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].af.size = bufsize;
	}
EXIT:
	_isp_preraw_be_dma_dump(ictx, raw_max);
}

void _isp_rawtop_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint64_t bufaddr = 0;
	uint32_t bufsize = 0;

	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		//apply pre_be le buffer from DMA_CTL22
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL28, raw_num, isp_bufpool[ISP_PRERAW_A].prebe_le[0]);

		if (ictx->is_hdr_on) {
			//apply pre_be se buffer from DMA_CTL23
			ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL29, raw_num, isp_bufpool[ISP_PRERAW_A].prebe_se[0]);
		}
	}

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor
			continue;

		// lsc
		DMA_SETUP(ISP_BLK_ID_DMA_CTL24, raw_num);
		isp_bufpool[raw_num].lsc = bufaddr;

		// gms
		bufaddr = _mempool_get_addr();
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL25, raw_num, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL25, raw_num);
		_mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[0].gms.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].gms.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].gms.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].gms.size = bufsize;

		// lmap_le
		DMA_SETUP_2(ISP_BLK_ID_DMA_CTL30, raw_num);
		isp_bufpool[raw_num].lmap_le = bufaddr;

		// ae le
		DMA_SETUP(ISP_BLK_ID_DMA_CTL26, raw_num);
		isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].ae_le.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].ae_le.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].ae_le.size = bufsize;

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// lmap_se
			DMA_SETUP_2(ISP_BLK_ID_DMA_CTL31, raw_num);
			isp_bufpool[raw_num].lmap_se = bufaddr;

			// ae se
			DMA_SETUP(ISP_BLK_ID_DMA_CTL27, raw_num);
			isp_bufpool[raw_num].sts_mem[0].ae_se.phy_addr = bufaddr;
			isp_bufpool[raw_num].sts_mem[0].ae_se.size = bufsize;
			isp_bufpool[raw_num].sts_mem[1].ae_se.phy_addr = _mempool_pop(bufsize);
			isp_bufpool[raw_num].sts_mem[1].ae_se.size = bufsize;
		}
	}

EXIT:
	_isp_rawtop_dma_dump(ictx, raw_max);
}

void _isp_rgbtop_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint64_t bufaddr = 0;
	uint32_t bufsize = 0;

	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor
			continue;

		// hist_edge_v
		DMA_SETUP_2(ISP_BLK_ID_DMA_CTL38, raw_num);
		isp_bufpool[raw_num].sts_mem[0].hist_edge_v.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].hist_edge_v.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].hist_edge_v.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].hist_edge_v.size = bufsize;

		// manr
		if (ictx->is_3dnr_on) {
			DMA_SETUP_2(ISP_BLK_ID_DMA_CTL36, raw_num);
			isp_bufpool[raw_num].manr = bufaddr;
			ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL37, isp_bufpool[raw_num].manr);

			isp_bufpool[raw_num].sts_mem[0].mmap.phy_addr =
				isp_bufpool[raw_num].sts_mem[1].mmap.phy_addr = bufaddr;
			isp_bufpool[raw_num].sts_mem[0].mmap.size =
				isp_bufpool[raw_num].sts_mem[1].mmap.size = bufsize;

			if (_is_all_online(ictx)) {
				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL32, isp_bufpool[raw_num].rgbmap_le[0]);
			} else {
				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL34, isp_bufpool[raw_num].rgbmap_le[0]);
				if (_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)
					ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL32, isp_bufpool[raw_num].rgbmap_le[0]);
				else
					ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL32, isp_bufpool[raw_num].rgbmap_le[1]);
			}

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL35, isp_bufpool[raw_num].rgbmap_se[0]);
				if (_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)
					ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL33, isp_bufpool[raw_num].rgbmap_se[0]);
				else
					ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL33, isp_bufpool[raw_num].rgbmap_se[1]);
			}

			// TNR
			if (ictx->is_fbc_on) {
				u64 bufaddr_tmp = 0;

				bufaddr_tmp = _mempool_get_addr();
				//ring buffer constraint. reg_base is 256 byte-align
				bufaddr = VI_256_ALIGN(bufaddr_tmp);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL44, raw_num, bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL44, raw_num);
				_mempool_pop(bufsize + (u32)(bufaddr - bufaddr_tmp));

				ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL42, raw_num, bufaddr);
				isp_bufpool[raw_num].tdnr[0] = bufaddr;

				bufaddr_tmp = _mempool_get_addr();
				//ring buffer constraint. reg_base is 256 byte-align
				bufaddr = VI_256_ALIGN(bufaddr_tmp);
				ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL43, raw_num, bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL43, raw_num);
				_mempool_pop(bufsize + (u32)(bufaddr - bufaddr_tmp));

				ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL41, raw_num, bufaddr);
				isp_bufpool[raw_num].tdnr[1] = bufaddr;
			} else {
				DMA_SETUP_2(ISP_BLK_ID_DMA_CTL44, raw_num);
				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL42, bufaddr);
				isp_bufpool[raw_num].tdnr[0] = bufaddr;

				DMA_SETUP_2(ISP_BLK_ID_DMA_CTL43, raw_num);
				ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL41, bufaddr);
				isp_bufpool[raw_num].tdnr[1] = bufaddr;
			}
		}

		// ltm dma
		ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL39, isp_bufpool[raw_num].lmap_le);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on && !ictx->isp_pipe_cfg[raw_num].is_hdr_detail_en)
			ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL40, isp_bufpool[raw_num].lmap_se);
		else
			ispblk_dma_setaddr(ictx, ISP_BLK_ID_DMA_CTL40, isp_bufpool[raw_num].lmap_le);

	}
EXIT:
	_isp_rgbtop_dma_dump(ictx, raw_max);
}

void _isp_yuvtop_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint64_t bufaddr = 0;
	uint64_t tmp_bufaddr = 0;
	uint32_t bufsize = 0;

	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor
			continue;

		// dci
		//DMA_SETUP(ISP_BLK_ID_WDMA27);
		DMA_SETUP(ISP_BLK_ID_DMA_CTL45, raw_num);
		isp_bufpool[raw_num].sts_mem[0].dci.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].dci.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].dci.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].dci.size = bufsize;

		// ldci
		//DMA_SETUP(ISP_BLK_ID_DMA_CTL48);
		//addr 256B alignment workaround
		tmp_bufaddr = _mempool_get_addr();
		bufaddr = VI_256_ALIGN(tmp_bufaddr);
		bufsize = ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL48, raw_num, bufaddr);
		_mempool_pop(bufsize + (uint32_t)(bufaddr - tmp_bufaddr));

		isp_bufpool[raw_num].ldci = bufaddr;
		ispblk_dma_config(ictx, ISP_BLK_ID_DMA_CTL49, raw_num, isp_bufpool[raw_num].ldci);
	}

	if (cfg_dma) {
		if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler ||
			(ictx->is_multi_sensor && ictx->isp_pipe_cfg[ISP_PRERAW_B].is_offline_scaler)) {
			//SW workaround. Need to set y/uv dma_disable = 1 before csibdg enable
			if (_is_be_post_online(ictx) && !ictx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL46, 1, 1);
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL47, 1, 1);
			} else {
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL46, 1, 0);
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL47, 1, 0);
			}
		} else {
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL46, 0, 0);
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL47, 0, 0);
		}
	}

EXIT:
	_isp_yuvtop_dma_dump(ictx, raw_max);
}

void _vi_dma_setup(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		_isp_preraw_fe_dma_setup(ictx, raw_num);
	}

	_isp_preraw_be_dma_setup(ictx, raw_max);
	_isp_rawtop_dma_setup(ictx, raw_max);
	_isp_rgbtop_dma_setup(ictx, raw_max);
	_isp_yuvtop_dma_setup(ictx, raw_max);
}

void _vi_dma_set_sw_mode(struct isp_ctx *ctx)
{
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL4, false); //be_le_rdma_ctl
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL5, false); //be_se_rdma_ctl
	//ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL10, false); //fe0 rgbmap LE
	//ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL11, false); //fe0 rgbmap SE
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL10); //fe0 rgbmap LE
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL11); //fe0 rgbmap SE
	//ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL16, false); //fe1 rgbmap LE
	//ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL17, false); //fe1 rgbmap SE
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL16); //fe1 rgbmap LE
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL17); //fe1 rgbmap SE
	//ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL20, false); //fe2 rgbmap LE
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL20); //fe2 rgbmap LE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL22, false); //be_le_wdma_ctl
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL23, false); //be_se_wdma_ctl
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL30, false); //lmap LE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL31, false); //lmap SE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL6, false); //fe0 csi0
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL7, false); //fe0 csi1
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL8, false); //fe0 csi2/fe1 ch0
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL9, false); //fe0 csi3/fe1 ch1
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL12, false); //fe1 ch0
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL13, false); //fe1 ch1
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL18, false); //fe2 ch0
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL19, false); //fe2 ch1
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL21, true); //af
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL24, true); //lsc
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL25, true); //gms
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL26, true); //aehist0
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL27, true); //aehist1
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL28, ctx->is_slice_buf_on ? false : true); //raw crop LE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL29, ctx->is_slice_buf_on ? false : true); //raw crop SE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL38, false); //hist_v
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL46, false); //yuv crop y
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL47, false); //yuv crop uv
	// ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL32, false); //MANR_P_LE
	// ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL33, false); //MANR_P_SE
	// ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL34, false); //MANR_C_LE
	// ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL35, false); //MANR_C_SE
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL32); //MANR_P_LE
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL33); //MANR_P_SE
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL34); //MANR_C_LE
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL35); //MANR_C_SE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL36, false); //MANR_IIR_R
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL37, false); //MANR_IIR_W
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL41, false); //TNR_Y_R
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL42, false); //TNR_C_R

	if (ctx->is_fbc_on) {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL43, true); //TNR_Y_W
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL44, true); //TNR_C_W
	} else {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL43, false); //TNR_Y_W
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL44, false); //TNR_C_W
	}
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL45, true); //dci
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL39, false); //LTM LE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL40, false); //LTM SE
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL48, true); //ldci_iir_w
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL49, true); //ldci_iir_r
}

void _vi_yuv_get_dma_size(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uint32_t bufsize_yuyv = 0;
	uint8_t  i = 0;

	u8 total_chn = (raw_num == ISP_PRERAW_A) ?
			ctx->rawb_chnstr_num :
			ctx->total_chn_num;
	u8 chn_str = (raw_num == ISP_PRERAW_A) ? 0 : ctx->rawb_chnstr_num;

	for (; chn_str < total_chn; chn_str++) {
		enum ISP_BLK_ID_T dma;

		if (raw_num == ISP_PRERAW_C) {
			dma = (chn_str == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19;
		} else {
			dma = (chn_str == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13;
		}

		for (i = 0; i < OFFLINE_YUV_BUF_NUM; i++) {
			bufsize_yuyv = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			_mempool_pop(bufsize_yuyv);
		}
	}
}

void _vi_pre_fe_get_dma_size(struct isp_ctx *ictx, enum cvi_isp_raw  raw_num)
{
	uint32_t bufsize = 0;
	uint8_t  i = 0;
	u32 raw_le, raw_se;
	u32 rgbmap_le, rgbmap_se;

	if (raw_num == ISP_PRERAW_A) {
		raw_le = ISP_BLK_ID_DMA_CTL6;
		raw_se = ISP_BLK_ID_DMA_CTL7;
		rgbmap_le = ISP_BLK_ID_DMA_CTL10;
		rgbmap_se = ISP_BLK_ID_DMA_CTL11;
	} else if (raw_num == ISP_PRERAW_B) {
		raw_le = ISP_BLK_ID_DMA_CTL12;
		raw_se = ISP_BLK_ID_DMA_CTL13;
		rgbmap_le = ISP_BLK_ID_DMA_CTL16;
		rgbmap_se = ISP_BLK_ID_DMA_CTL17;
	} else {
		raw_le = ISP_BLK_ID_DMA_CTL18;
		raw_se = ISP_BLK_ID_DMA_CTL19;
		rgbmap_le = ISP_BLK_ID_DMA_CTL20;
		//No se in ISP_PRERAW_C
	}

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		if (!ictx->isp_pipe_cfg[raw_num].is_offline_scaler) //Online mode to scaler
			_vi_yuv_get_dma_size(ictx, raw_num);

		goto EXIT;
	}

	if (_is_be_post_online(ictx)) { //fe->dram->be->post
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
			//muxdev only use one buffer
			if ((ictx->isp_pipe_cfg[raw_num].is_mux || (raw_num > ISP_PRERAW_MAX - 1)) && i != 0)
				continue;
			bufsize = ispblk_dma_buf_get_size(ictx, raw_le, raw_num);
			_mempool_pop(bufsize);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
				//muxdev only use one buffer
				if ((ictx->isp_pipe_cfg[raw_num].is_mux || (raw_num > ISP_PRERAW_MAX - 1)) && i != 0)
					continue;
				bufsize = ispblk_dma_buf_get_size(ictx, raw_se, raw_num);
				_mempool_pop(bufsize);
			}
		}
	}

	// rgbmap le
	bufsize = ispblk_dma_buf_get_size(ictx, rgbmap_le, raw_num);
	_mempool_pop(bufsize);

	//Slice buffer mode use ring buffer
	if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
		if (!_is_all_online(ictx)) {
			for (i = 1; i < RGBMAP_BUF_IDX; i++)
				_mempool_pop(bufsize);
		}
	}

	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		// rgbmap se
		bufsize = ispblk_dma_buf_get_size(ictx, rgbmap_se, raw_num);
		_mempool_pop(bufsize);

		//Slice buffer mode use ring buffer
		if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
			for (i = 1; i < RGBMAP_BUF_IDX; i++)
				_mempool_pop(bufsize);
		}
	}
EXIT:
	return;
}

void _vi_pre_be_get_dma_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint32_t bufsize = 0;
	uint8_t  buf_num = 0;

	enum cvi_isp_raw raw = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false)
		goto EXIT;

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		if (ictx->is_slice_buf_on) {
			bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL22, raw);
			_mempool_pop(bufsize);

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL23, raw);
				_mempool_pop(bufsize);
			}
		} else {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL22, raw);
				_mempool_pop(bufsize);
			}

			if (ictx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL23, raw);
					_mempool_pop(bufsize);
				}
			}
		}
	}

	for (; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		//Be out dma
		if (!ictx->isp_pipe_enable[raw])
			continue;
		if (ictx->isp_pipe_cfg[raw].is_yuv_bypass_path)
			continue;

		// af
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL21, raw);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);
	}
EXIT:
	return;
}

void _vi_rawtop_get_dma_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint32_t bufsize = 0;

	enum cvi_isp_raw raw = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw = ISP_PRERAW_A; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;
		if (ictx->isp_pipe_cfg[raw].is_yuv_bypass_path) //YUV sensor
			continue;

		// lsc
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL24, raw);
		_mempool_pop(bufsize);

		// gms
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL25, raw);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// lmap_le
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL30, raw);
		_mempool_pop(bufsize);

		// ae le
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL26, raw);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw].is_hdr_on) {
			// lmap_se
			bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL31, raw);
			_mempool_pop(bufsize);

			// ae se
			bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL27, raw);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);
		}
	}
EXIT:
	return;
}

void _vi_rgbtop_get_dma_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint32_t bufsize = 0;

	enum cvi_isp_raw raw = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw = ISP_PRERAW_A; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;
		if (ictx->isp_pipe_cfg[raw].is_yuv_bypass_path) //YUV sensor
			continue;

		// hist_edge_v
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL38, raw);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// manr
		if (ictx->is_3dnr_on) {
			uint64_t bufaddr = 0;
			uint64_t tmp_bufaddr = 0;

			// MANR M + H
			bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL36, raw);
			_mempool_pop(bufsize);

			if (ictx->is_fbc_on) {
				// TNR UV
				tmp_bufaddr = _mempool_get_addr();
				//ring buffer constraint. reg_base is 256 byte-align
				bufaddr = VI_256_ALIGN(tmp_bufaddr);

				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL44, raw);
				_mempool_pop(bufsize + (uint32_t)(bufaddr - tmp_bufaddr));

				// TNR Y
				tmp_bufaddr = _mempool_get_addr();
				//ring buffer constraint. reg_base is 256 byte-align
				bufaddr = VI_256_ALIGN(tmp_bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL43, raw);
				_mempool_pop(bufsize + (uint32_t)(bufaddr - tmp_bufaddr));
			} else {
				// TNR UV
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL44, raw);
				_mempool_pop(bufsize);

				// TNR Y
				bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL43, raw);
				_mempool_pop(bufsize);
			}
		}
	}
EXIT:
	return;
}

void _vi_yuvtop_get_dma_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	uint32_t bufsize = 0;
	uint64_t bufaddr = 0;
	uint64_t tmp_bufaddr = 0;

	enum cvi_isp_raw raw = ISP_PRERAW_A;

	u8 cfg_dma = false;

	//RGB path
	if (!ictx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_dma = true;
	} else if (ictx->is_multi_sensor && !ictx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_dma = true;
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw = ISP_PRERAW_A; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;
		if (ictx->isp_pipe_cfg[raw].is_yuv_bypass_path) //YUV sensor
			continue;

		// dci
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL45, raw);
		_mempool_pop(bufsize);
		tmp_bufaddr = _mempool_pop(bufsize);

		// ldci
		bufaddr = VI_256_ALIGN(tmp_bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, ISP_BLK_ID_DMA_CTL48, raw);
		_mempool_pop(bufsize + (uint32_t)(bufaddr - tmp_bufaddr));

		vi_pr(VI_INFO, "ldci bufsize: total(%d), used(%d), wasted_for_alignment(%d)\n",
			bufsize + (uint32_t)(bufaddr - tmp_bufaddr),
			bufsize,
			(uint32_t)(bufaddr - tmp_bufaddr));
	}
EXIT:
	return;
}

void _vi_get_dma_buf_size(struct isp_ctx *ictx, enum cvi_isp_raw raw_max)
{
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		_vi_pre_fe_get_dma_size(ictx, raw_num);
	}

	_vi_pre_be_get_dma_size(ictx, raw_max);
	_vi_rawtop_get_dma_size(ictx, raw_max);
	_vi_rgbtop_get_dma_size(ictx, raw_max);
	_vi_yuvtop_get_dma_size(ictx, raw_max);
}

static void _vi_preraw_be_init(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint32_t bps[40] = {((0 << 12) | 0), ((0 << 12) | 20), ((0 << 12) | 40), ((0 << 12) | 60)};
	u8 cfg_be = false;

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_be = true;
	} else if (ctx->is_multi_sensor && !ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_be = true;
	}

	if (cfg_be) { //RGB sensor
		// preraw_vi_sel
		ispblk_preraw_vi_sel_config(ctx);
		// preraw_be_top
		ispblk_preraw_be_config(ctx, ISP_PRERAW_A);
		// preraw_be wdma ctrl
		ispblk_pre_wdma_ctrl_config(ctx, ISP_PRERAW_A);

		//ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_LE, 511, 511, 511, 511);
		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x40f, 0x419, 0x419, 0x405);
		//ispblk_blc_set_2ndoffset(ctx, ISP_BLC_ID_BE_LE, 511, 511, 511, 511);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, false, false);

		if (ctx->is_hdr_on) {
			ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_SE, 511, 511, 511, 511);
			ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_SE, 0x800, 0x800, 0x800, 0x800);
			ispblk_blc_set_2ndoffset(ctx, ISP_BLC_ID_BE_SE, 511, 511, 511, 511);
			ispblk_blc_enable(ctx, ISP_BLC_ID_BE_SE, false, false);
		}

		ispblk_dpc_set_static(ctx, ISP_RAW_PATH_LE, 0, bps, 4);
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, false, 0);

		if (ctx->is_hdr_on)
			ispblk_dpc_config(ctx, ISP_RAW_PATH_SE, true, 0);
		else
			ispblk_dpc_config(ctx, ISP_RAW_PATH_SE, false, 0);

		ispblk_af_config(ctx, true);

		if (_is_fe_be_online(ctx))
			ispblk_slice_buf_config(&vdev->ctx, ISP_PRERAW_A, vdev->ctx.is_slice_buf_on);
		else
			ispblk_slice_buf_config(&vdev->ctx, ISP_PRERAW_A, false);
	}
}

static void _isp_rawtop_init(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;

	// raw_top
	ispblk_rawtop_config(ictx, ISP_PRERAW_A);
	// raw_rdma ctrl
	ispblk_raw_rdma_ctrl_config(ictx, ISP_PRERAW_A);

	ispblk_bnr_config(ictx, ISP_BNR_OUT_B_DELAY, false, 0, 0);

	ispblk_lsc_config(ictx, false);

	ispblk_cfa_config(ictx);
#ifndef PORTING_TEST
	ispblk_rgbcac_config(ictx, true, 0);
#else
	ispblk_rgbcac_config(ictx, false, 0);
#endif
	ispblk_lcac_config(ictx, false, 0);
	ispblk_gms_config(ictx, true);

	ispblk_wbg_config(ictx, ISP_WBG_ID_RAW_TOP_LE, 0x400, 0x400, 0x400);
	ispblk_wbg_enable(ictx, ISP_WBG_ID_RAW_TOP_LE, false, false);

	ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP0, true);

	ispblk_aehist_config(ictx, ISP_BLK_ID_AEHIST0, true);

	if (ictx->is_hdr_on) {
		ispblk_wbg_config(ictx, ISP_WBG_ID_RAW_TOP_SE, 0x400, 0x400, 0x400);
		ispblk_wbg_enable(ictx, ISP_WBG_ID_RAW_TOP_SE, false, false);
		ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP1, true);
		ispblk_aehist_config(ictx, ISP_BLK_ID_AEHIST1, true);
	} else {
		ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP1, false);
		ispblk_aehist_config(ictx, ISP_BLK_ID_AEHIST1, false);
	}
}

static void _isp_rgbtop_init(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;

	ispblk_rgbtop_config(ictx, ISP_PRERAW_A);

	ispblk_hist_v_config(ictx, true, 0);

	//ispblk_awb_config(ictx, ISP_BLK_ID_AWB2, true, ISP_AWB_LE);

	ispblk_ccm_config(ictx, ISP_BLK_ID_CCM0, false, &ccm_hw_cfg);
	ispblk_dhz_config(ictx, false);

	ispblk_ygamma_config(ictx, false, ictx->gamma_tbl_idx, ygamma_data, 0, 0);
	ispblk_ygamma_enable(ictx, false);

	ispblk_gamma_config(ictx, false, ictx->gamma_tbl_idx, gamma_data, 0);
	ispblk_gamma_enable(ictx, false);

	//ispblk_clut_config(ictx, false, c_lut_r_lut, c_lut_g_lut, c_lut_b_lut);
	ispblk_rgbdither_config(ictx, false, false, false, false);
	ispblk_csc_config(ictx);

	ispblk_manr_config(ictx, ictx->is_3dnr_on);

	if (ictx->is_hdr_on) {
		ispblk_fusion_config(ictx, true, true, ISP_FS_OUT_FS);

		ispblk_ltm_b_lut(ictx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ictx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ictx, 0, ltm_g_lut);
		ispblk_ltm_config(ictx, true, true, true, true);
	} else {
		ispblk_fusion_config(ictx, false, false, ISP_FS_OUT_LONG);
		ispblk_ltm_config(ictx, false, false, false, false);
	}
}

static void _isp_yuvtop_init(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;

	ispblk_yuvtop_config(ictx, ISP_PRERAW_A);

	ispblk_yuvdither_config(ictx, 0, false, true, true, true);
	ispblk_yuvdither_config(ictx, 1, false, true, true, true);

	ispblk_tnr_config(ictx, ictx->is_3dnr_on, 0);
	if (ictx->is_3dnr_on && ictx->is_fbc_on) {
		ispblk_fbce_config(ictx, true);
		ispblk_fbcd_config(ictx, true);
		ispblk_fbc_ring_buf_config(ictx, true);
	} else {
		ispblk_fbce_config(ictx, false);
		ispblk_fbcd_config(ictx, false);
		ispblk_fbc_ring_buf_config(ictx, false);
	}

	ispblk_ynr_config(ictx, ISP_YNR_OUT_Y_DELAY, 128);
	ispblk_cnr_config(ictx, false, false, 255, 0);
	ispblk_pre_ee_config(ictx, true);
	ispblk_ee_config(ictx, false);
#ifdef COVER_WITH_BLACK
	memset(ycur_data, 0, sizeof(ycur_data));
	ispblk_ycur_config(ictx, false, 0, ycur_data);
	ispblk_ycur_enable(ictx, true, 0);
#else
	ispblk_ycur_config(ictx, false, 0, ycur_data);
	ispblk_ycur_enable(ictx, false, 0);
#endif
	ispblk_dci_config(ictx, true, ictx->gamma_tbl_idx, dci_map_lut_50, 0);
	ispblk_ldci_config(ictx, false, 0);

	ispblk_ca_config(ictx, false, 1);
	ispblk_ca_lite_config(ictx, false);

	ispblk_crop_enable(ictx, ISP_BLK_ID_CROP4, false);
	ispblk_crop_enable(ictx, ISP_BLK_ID_CROP5, false);
}

static u32 _is_drop_next_frame(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const enum cvi_isp_pre_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint32_t start_drop_num = ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num;
	uint32_t end_drop_num = start_drop_num + ctx->isp_pipe_cfg[raw_num].drop_frm_cnt;
	u32 frm_num = 0;

	if (ctx->isp_pipe_cfg[raw_num].is_drop_next_frame) {
		//for tuning_dis, shoudn't trigger preraw;
		if ((ctx->is_multi_sensor) && (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path)) {
			if ((tuning_dis[0] > 0) && ((tuning_dis[0] - 1) != raw_num)) {
				vi_pr(VI_DBG, "input buf is not equal to current tuning number\n");
				return 1;
			}
		}

		//if sof_num in [start_sof, end_sof), shoudn't trigger preraw;
		frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		if ((start_drop_num != 0) && (frm_num >= start_drop_num) && (frm_num < end_drop_num))
			return 1;
	}

	return 0;
}

static void _set_drop_frm_info(
	const struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	struct isp_i2c_data *i2c_data)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	ctx->isp_pipe_cfg[raw_num].drop_frm_cnt = i2c_data->drop_frame_cnt;

	ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
	ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

	vi_pr(VI_DBG, "raw_%d, drop_ref_frm_num=%d, drop frame=%d\n", raw_num,
				ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num,
				i2c_data->drop_frame_cnt);
}

static void _clear_drop_frm_info(
	const struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	ctx->isp_pipe_cfg[raw_num].drop_frm_cnt = 0;
	ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num = 0;
	ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = false;
}

static void _isp_crop_update_chk(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_crop_node *node)
{
	u16 i = 0, del_node = true;
	unsigned long flags;

	if (node->n.dly_frm_num == 0) {

		vdev->ctx.isp_pipe_cfg[raw_num].crop.x = node->n.wdr.img_size[0].start_x;
		vdev->ctx.isp_pipe_cfg[raw_num].crop.y = node->n.wdr.img_size[0].start_y;
		vdev->ctx.isp_pipe_cfg[raw_num].crop.w = node->n.wdr.img_size[0].active_w;
		vdev->ctx.isp_pipe_cfg[raw_num].crop.h = node->n.wdr.img_size[0].active_h;

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on) {
			vdev->ctx.isp_pipe_cfg[raw_num].crop_se.x = node->n.wdr.img_size[1].start_x;
			vdev->ctx.isp_pipe_cfg[raw_num].crop_se.y = node->n.wdr.img_size[1].start_y;
			vdev->ctx.isp_pipe_cfg[raw_num].crop_se.w = node->n.wdr.img_size[1].active_w;
			vdev->ctx.isp_pipe_cfg[raw_num].crop_se.h = node->n.wdr.img_size[1].active_h;
		}

		vdev->ctx.isp_pipe_cfg[raw_num].csibdg_width =
					node->n.wdr.img_size[0].width;
		vdev->ctx.isp_pipe_cfg[raw_num].csibdg_height =
					node->n.wdr.img_size[0].height;

		vi_pr(VI_DBG, "Preraw_%d, %d crop_x:y:w:h=%d:%d:%d:%d\n", raw_num, i,
						vdev->ctx.isp_pipe_cfg[raw_num].crop.x,
						vdev->ctx.isp_pipe_cfg[raw_num].crop.y,
						vdev->ctx.isp_pipe_cfg[raw_num].crop.w,
						vdev->ctx.isp_pipe_cfg[raw_num].crop.h);

		ispblk_csibdg_crop_update(&vdev->ctx, raw_num, true);
		ispblk_csibdg_update_size(&vdev->ctx, raw_num);
	} else {
		node->n.dly_frm_num--;
		del_node = false;
	}

	if (del_node) {
		vi_pr(VI_DBG, "crop del node and free\n");
		spin_lock_irqsave(&snr_node_lock[raw_num], flags);
		list_del_init(&node->list);
		kfree(node);
		spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
	}
}

static void _isp_crop_update(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_crop_node **_crop_n,
	const u16 _crop_num)
{
	struct _isp_crop_node *node;
	u16 i = 0;

	if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw || vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en)
		return;

	for (i = 0; i < _crop_num; i++) {
		node = _crop_n[i];

		_isp_crop_update_chk(vdev, raw_num, node);
	}
}

static void _snr_i2c_update(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	struct _isp_snr_i2c_node **_i2c_n,
	const u16 _i2c_num,
	int is_vblank_update)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);
	struct _isp_snr_i2c_node *node;
	struct _isp_snr_i2c_node *next_node;
	struct isp_i2c_data *i2c_data;
	struct isp_i2c_data *next_i2c_data;
	unsigned long flags;
	u16 i = 0, j = 0;
	// 0: Delete Node, 1: Postpone Node, 2: Do nothing
	u16 del_node = 0;
	uint32_t dev_mask = 0;
	uint32_t cmd = burst_i2c_en ? CVI_SNS_I2C_BURST_QUEUE : CVI_SNS_I2C_WRITE;
	CVI_BOOL no_update = CVI_TRUE;
	CVI_BOOL fe_frm_equal = CVI_FALSE;
	u32 fe_frm_num = 0;

	if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw || vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en)
		return;

	//dual sensor case, fe/be frm_num wouldn't be the same when dump frame of rotation.
	if (_is_be_post_online(&vdev->ctx)) {
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
			fe_frm_equal = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
					vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]) ? CVI_TRUE : CVI_FALSE;
		fe_frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] - vdev->dump_frame_number[raw_num];
	} else {
		fe_frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
	}

	vi_pr(VI_DBG, "raw_num=%d, fe_frm_num=%d, is_vblank_update=%d, fe_frm_equal=%d\n",
			raw_num, fe_frm_num, is_vblank_update, fe_frm_equal);

	for (j = 0; j < _i2c_num; j++) {
		node = _i2c_n[j];
		no_update = CVI_TRUE;

		vi_pr(VI_DBG, "raw_num=%d, i2c_num=%d, j=%d, magic_num=%d, magic_num_vblank=%d\n",
				raw_num, _i2c_num, j, node->n.magic_num, node->n.magic_num_vblank);

		//magic num set by ISP team. fire i2c when magic num same as last fe frm num.
		if (((node->n.magic_num == fe_frm_num || (node->n.magic_num < fe_frm_num && (j + 1) >= _i2c_num)) &&
			(!is_vblank_update)) || ((node->n.magic_num_vblank  == fe_frm_num ||
			(node->n.magic_num_vblank < fe_frm_num && (j + 1) >= _i2c_num)) && (is_vblank_update))) {

		}
		if (((ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on) &&
		     ((node->n.magic_num == fe_frm_num && fe_frm_equal == CVI_TRUE) ||
		      (node->n.magic_num_dly == fe_frm_num))) ||
		    (!(ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on) &&
		     (((!is_vblank_update) && ((node->n.magic_num == fe_frm_num) ||
		       (node->n.magic_num < fe_frm_num && (j + 1) >= _i2c_num))) ||
		      ((is_vblank_update) && ((node->n.magic_num_vblank == fe_frm_num) ||
		       (node->n.magic_num_vblank < fe_frm_num && (j + 1) >= _i2c_num)))))) {

			if ((node->n.magic_num != fe_frm_num && !is_vblank_update) ||
				(node->n.magic_num_vblank != fe_frm_num && is_vblank_update)) {
				vi_pr(VI_WARN, "exception handle, send delayed i2c data.\n");
			}

			for (i = 0; i < node->n.regs_num; i++) {
				i2c_data = &node->n.i2c_data[i];

				vi_pr(VI_DBG,
					"i2cdev[%d] i2cdata[%d]:addr=0x%x write:0x%x update:%d update_v:%d dly_frm:%d\n",
					i2c_data->i2c_dev, i, i2c_data->reg_addr, i2c_data->data, i2c_data->update,
					i2c_data->vblank_update, i2c_data->dly_frm_num);

				if (i2c_data->update && (i2c_data->dly_frm_num == 0)) {
					if ((i2c_data->vblank_update && is_vblank_update)
					|| (!i2c_data->vblank_update && !is_vblank_update)) {
						vip_sys_cmm_cb_i2c(cmd, (void *)i2c_data);
						i2c_data->update = 0;
						if (burst_i2c_en)
							dev_mask |= BIT(i2c_data->i2c_dev);
						if (i2c_data->drop_frame)
							_set_drop_frm_info(vdev, raw_num, i2c_data);
					} else {
						no_update = CVI_FALSE;
					}
				} else if (i2c_data->update && !(i2c_data->dly_frm_num == 0)) {
					i2c_data->dly_frm_num--;
					del_node = 1;
				}
			}

		} else if ((node->n.magic_num < fe_frm_num && !is_vblank_update) ||
			   (node->n.magic_num_vblank < fe_frm_num && is_vblank_update)) {

			if ((j + 1) < _i2c_num) {
				next_node = _i2c_n[j + 1];

				for (i = 0; i < next_node->n.regs_num; i++) {
					next_i2c_data = &next_node->n.i2c_data[i];
					i2c_data = &node->n.i2c_data[i];

					if (i2c_data->update && next_i2c_data->update == 0) {
						next_i2c_data->update = i2c_data->update;
						vi_pr(VI_WARN, "exception handle, i2c node merge, addr: 0x%x\n",
							i2c_data->reg_addr);
					}
				}

				del_node = 0;
			} else {
				// impossible case
			}
		} else {
			del_node = 2;
		}

		if (del_node == 0 && no_update) {
			vi_pr(VI_DBG, "i2c node %d del node and free\n", j);
			spin_lock_irqsave(&snr_node_lock[raw_num], flags);
			list_del_init(&node->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			kfree(node);
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
		} else if (del_node == 1) {
			if (is_vblank_update)
				node->n.magic_num_vblank++;
			else
				node->n.magic_num++;

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
				node->n.magic_num_dly = node->n.magic_num;

			vi_pr(VI_DBG, "postpone i2c node\n");
		}
	}

	while (dev_mask) {
		uint32_t tmp = ffs(dev_mask) - 1;

		vip_sys_cmm_cb_i2c(CVI_SNS_I2C_BURST_FIRE, (void *)&tmp);
		dev_mask &= ~BIT(tmp);
	}
}

static void _isp_snr_cfg_deq_and_fire(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	int needvblank)
{
	struct list_head *pos, *temp;
	struct _isp_snr_i2c_node *i2c_n[VI_MAX_SNS_CFG_NUM], *i2c_n_temp[0];
	struct _isp_crop_node *crop_n[VI_MAX_SNS_CFG_NUM];
	unsigned long flags;
	u16 i2c_num = 0, crop_num = 0;
	int i;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);
	if (needvblank == 0) {
		list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n[i2c_num] = list_entry(pos, struct _isp_snr_i2c_node, list);
			i2c_num++;
		}

		list_for_each_safe(pos, temp, &isp_crop_queue[raw_num].list) {
			if (crop_num < i2c_num) {
				crop_n[crop_num] = list_entry(pos, struct _isp_crop_node, list);
				crop_num++;
			}
		}

	} else {
		list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n_temp[0] = list_entry(pos, struct _isp_snr_i2c_node, list);
			for (i = 0; i < i2c_n_temp[0]->n.regs_num; i++) {
				if (i2c_n_temp[0]->n.i2c_data[i].vblank_update && i2c_n_temp[0]->n.i2c_data[i].update) {
					i2c_n[i2c_num] = i2c_n_temp[0];
					i2c_num++;
					break;
				}
			}
		}
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);

	if (i2c_num > 0)
		_snr_i2c_update(vdev, raw_num, i2c_n, i2c_num, needvblank);
	if (crop_num > 0)
		_isp_crop_update(vdev, raw_num, crop_n, crop_num);
}

static inline void _vi_clear_mmap_fbc_ring_base(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	//Clear mmap previous ring base to start addr after first frame done.
	if (ctx->is_3dnr_on && (ctx->isp_pipe_cfg[raw_num].first_frm_cnt == 1)) {
		manr_clear_prv_ring_base(ctx, raw_num);

		if (ctx->is_fbc_on) {
			ispblk_fbc_chg_to_sw_mode(&vdev->ctx, raw_num);
			ispblk_fbc_clear_fbcd_ring_base(&vdev->ctx, raw_num);
		}
	}
}

static void _usr_pic_timer_handler(unsigned long data)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)usr_pic_timer.data;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
		mod_timer(&usr_pic_timer.t, jiffies + vdev->usr_pic_delay);
#else
		mod_timer(&usr_pic_timer, jiffies + vdev->usr_pic_delay);
#endif
		return;
	}

	if (atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_PRE_BE_IDLE &&
		(atomic_read(&vdev->isp_streamoff) == 0) && ctx->is_ctrl_inited) {
		struct _isp_raw_num_n  *n;

		if (_is_fe_be_online(ctx) && ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on) {
			struct isp_buffer *b = NULL;

			b = isp_next_buf(&pre_be_out_se_q);
			if (!b) {
				vi_pr(VI_DBG, "pre_be chn_num_%d outbuf is empty\n", ISP_FE_CH1);
				return;
			}

			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL23, b->addr);
		}

		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1 && _is_fe_be_online(ctx)) {//raw_dump flow
			_isp_fe_be_raw_dump_cfg(vdev, raw_num, 0);
			atomic_set(&vdev->isp_raw_dump_en[raw_num], 3);
		}

		_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

		vi_tuning_gamma_ips_update(ctx, raw_num);
		vi_tuning_clut_update(ctx, raw_num);
		vi_tuning_dci_update(ctx, raw_num);
		vi_tuning_drc_update(ctx, raw_num);

		_post_rgbmap_update(ctx, raw_num, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);

		_pre_hw_enque(vdev, raw_num, ISP_BE_CH0);

		n = kmalloc(sizeof(*n), GFP_ATOMIC);
		if (n == NULL) {
			vi_pr(VI_ERR, "pre_raw_num_q kmalloc size(%zu) fail\n", sizeof(*n));
			return;
		}
		n->raw_num = raw_num;
		pre_raw_num_enq(&pre_raw_num_q, n);

		vdev->vi_th[E_VI_TH_PRERAW].flag = raw_num + 1;

		wake_up(&vdev->vi_th[E_VI_TH_PRERAW].wq);

		//if (!_is_all_online(ctx)) //Not on the fly mode
		//	tasklet_hi_schedule(&vdev->job_work);
	}

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	mod_timer(&usr_pic_timer.t, jiffies + vdev->usr_pic_delay);
#else
	mod_timer(&usr_pic_timer, jiffies + vdev->usr_pic_delay);
#endif
}

void usr_pic_time_remove(void)
{
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	if (timer_pending(&usr_pic_timer.t)) {
		del_timer_sync(&usr_pic_timer.t);
		timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	if (timer_pending(&usr_pic_timer)) {
		del_timer_sync(&usr_pic_timer);
		init_timer(&usr_pic_timer);
#endif
	}
}

int usr_pic_timer_init(struct cvi_vi_dev *vdev)
{
	usr_pic_time_remove();
	usr_pic_timer.function = _usr_pic_timer_handler;
	usr_pic_timer.data = (uintptr_t)vdev;
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	usr_pic_timer.t.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer.t);
#else
	usr_pic_timer.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer);
#endif

	return 0;
}

void vi_event_queue(struct cvi_vi_dev *vdev, const u32 type, const u32 frm_num)
{
	unsigned long flags;
	struct vi_event_k *ev_k;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 ts;
#endif

	if (type >= VI_EVENT_MAX) {
		vi_pr(VI_ERR, "event queue type(%d) error\n", type);
		return;
	}

	ev_k = kzalloc(sizeof(*ev_k), GFP_ATOMIC);
	if (ev_k == NULL) {
		vi_pr(VI_ERR, "event queue kzalloc size(%zu) fail\n", sizeof(*ev_k));
		return;
	}

	spin_lock_irqsave(&event_lock, flags);
	ev_k->ev.type = type;
	ev_k->ev.frame_sequence = frm_num;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	ts = ktime_to_timespec64(ktime_get());
	ev_k->ev.timestamp.tv_sec = ts.tv_sec;
	ev_k->ev.timestamp.tv_nsec = ts.tv_nsec;
#else
	ev_k->ev.timestamp = ktime_to_timeval(ktime_get());
#endif
	list_add_tail(&ev_k->list, &event_q.list);
	spin_unlock_irqrestore(&event_lock, flags);

	wake_up(&vdev->isp_event_wait_q);
}

void cvi_isp_dqbuf_list(struct cvi_vi_dev *vdev, const u32 frm_num, const u8 chn_id)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n;

	n = kzalloc(sizeof(struct _isp_dqbuf_n), GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "DQbuf kmalloc size(%zu) fail\n", sizeof(struct _isp_dqbuf_n));
		return;
	}
	n->chn_id	= chn_id;
	n->frm_num	= frm_num;
	n->timestamp	= ktime_to_timespec64(ktime_get());

	spin_lock_irqsave(&dq_lock, flags);
	list_add_tail(&n->list, &dqbuf_q.list);
	spin_unlock_irqrestore(&dq_lock, flags);
}

int vi_dqbuf(struct _vi_buffer *b)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n = NULL;
	int ret = -1;

	spin_lock_irqsave(&dq_lock, flags);
	if (!list_empty(&dqbuf_q.list)) {
		n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		b->chnId	= n->chn_id;
		b->sequence	= n->frm_num;
		b->timestamp	= n->timestamp;
		list_del_init(&n->list);
		kfree(n);
		ret = 0;
	}
	spin_unlock_irqrestore(&dq_lock, flags);

	return ret;
}

static int _vi_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static void vi_init(void)
{
	int i, j;

	for (i = 0; i < VI_MAX_CHN_NUM; ++i) {
		gViCtx->enRotation[i] = ROTATION_0;
		gViCtx->stLDCAttr[i].bEnable = CVI_FALSE;
		mutex_init(&g_vi_mesh[i].lock);
	}

	for (i = 0; i < VI_MAX_CHN_NUM; ++i)
		for (j = 0; j < VI_MAX_EXTCHN_BIND_PER_CHN; ++j)
			gViCtx->chn_bind[i][j] = VI_INVALID_CHN;
}

static void _isp_yuv_bypass_buf_enq(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 buf_chn)
{
	struct isp_ctx *ictx = &vdev->ctx;
	struct cvi_isp_buf *b = NULL;
	enum ISP_BLK_ID_T dmaid;
	u64 tmp_addr = 0, i = 0;
	u8 hw_chn_num = (raw_num == ISP_PRERAW_A) ? buf_chn : (buf_chn - vdev->ctx.rawb_chnstr_num);

	cvi_isp_rdy_buf_pop(vdev, buf_chn);
	b = _cvi_isp_next_buf(vdev, buf_chn);
	if (b == NULL) {
		vi_pr(VI_WARN, "no buffer\n");
		return;
	}

	vi_pr(VI_DBG, "update yuv bypass outbuf: 0x%llx raw_%d chn_num(%d)\n",
			b->buf.planes[0].addr, raw_num, buf_chn);
	if (raw_num == ISP_PRERAW_A) {
		switch (buf_chn) {
		case 0:
			dmaid = ISP_BLK_ID_DMA_CTL6;
			break;
		case 1:
			dmaid = ISP_BLK_ID_DMA_CTL7;
			break;
		case 2:
			dmaid = ISP_BLK_ID_DMA_CTL8;
			break;
		case 3:
			dmaid = ISP_BLK_ID_DMA_CTL9;
			break;
		default:
			vi_pr(VI_ERR, "PRERAW_A Wrong chn_num(%d)\n", buf_chn);
			return;
		}
	} else if (raw_num == ISP_PRERAW_B) {
		switch (hw_chn_num) {
		case 0:
			dmaid = ISP_BLK_ID_DMA_CTL12;
			break;
		case 1:
			dmaid = ISP_BLK_ID_DMA_CTL13;
			break;
		default:
			vi_pr(VI_ERR, "RAW_%c Wrong chn_num(%d), rawb_chnstr_num(%d)\n",
					(raw_num + 'A'), buf_chn, ictx->rawb_chnstr_num);
			return;
		}
	} else {
		switch (hw_chn_num) {
		case 0:
			dmaid = ISP_BLK_ID_DMA_CTL18;
			break;
		case 1:
			dmaid = ISP_BLK_ID_DMA_CTL19;
			break;
		default:
			vi_pr(VI_ERR, "RAW_%c Wrong chn_num(%d), rawb_chnstr_num(%d)\n",
					(raw_num + 'A'), buf_chn, ictx->rawb_chnstr_num);
			return;
		}
	}

	if (ictx->isp_pipe_cfg[raw_num].is_422_to_420) {
		for (i = 0; i < 2; i++) {
			tmp_addr = b->buf.planes[i].addr;
			if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
				ispblk_dma_yuv_bypass_config(ictx, dmaid + i, tmp_addr, raw_num);
			else
				ispblk_dma_setaddr(ictx, dmaid + i, tmp_addr);
		}
	} else {
		tmp_addr = b->buf.planes[0].addr;

		if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
			ispblk_dma_yuv_bypass_config(ictx, dmaid, tmp_addr, raw_num);
		else
			ispblk_dma_setaddr(ictx, dmaid, tmp_addr);
	}
}

static int _isp_yuv_bypass_trigger(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 buf_chn;

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][hw_chn_num],
					ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
					ISP_PRERAW_RUNNING) {
			vi_pr(VI_DBG, "fe_%d chn_num_%d is running\n", raw_num, hw_chn_num);
			return -1;
		}
		buf_chn = (raw_num == ISP_PRERAW_A) ? hw_chn_num : vdev->ctx.rawb_chnstr_num + hw_chn_num;

		_isp_yuv_bypass_buf_enq(vdev, raw_num, buf_chn);
		isp_pre_trig(ctx, raw_num, hw_chn_num);
	}
	return 0;
}

void _vi_postraw_ctrl_setup(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 cfg_post = false;

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
		cfg_post = true;
	} else if (ctx->is_multi_sensor && !ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) {
		cfg_post = true;
	}

	if (cfg_post) { //RGB sensor
		_isp_rawtop_init(vdev);
		_isp_rgbtop_init(vdev);
		_isp_yuvtop_init(vdev);
	}

	ispblk_isptop_config(ctx);
}

void _vi_pre_fe_ctrl_setup(enum cvi_isp_raw raw_num, struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u32 blc_le_id, blc_se_id, wbg_le_id, wbg_se_id;
	struct cif_yuv_swap_s swap = {0};

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {//YUV sensor
		if (ictx->isp_pipe_cfg[raw_num].is_422_to_420) {//uyvy to yuyv to 420
			swap.devno = raw_num;
			swap.yc_swap = 1;
			swap.uv_swap = 1;
			_vi_call_cb(E_MODULE_CIF, CVI_MIPI_SET_YUV_SWAP, &swap);
		}

		ispblk_csibdg_yuv_bypass_config(ictx, raw_num);

		if (ictx->isp_pipe_cfg[raw_num].is_offline_scaler) { //vi vpss offline mode
			u8 total_chn = (raw_num == ISP_PRERAW_A) ?
					ictx->rawb_chnstr_num :
					ictx->total_chn_num;
			u8 chn_str = (raw_num == ISP_PRERAW_A) ? 0 : ictx->rawb_chnstr_num;

			for (; chn_str < total_chn; chn_str++)
				_isp_yuv_bypass_buf_enq(vdev, raw_num, chn_str);
		}
	} else { //RGB sensor
		if (raw_num == ISP_PRERAW_A) {
			blc_le_id = ISP_BLC_ID_FE0_LE;
			blc_se_id = ISP_BLC_ID_FE0_SE;
			wbg_le_id = ISP_WBG_ID_FE0_RGBMAP_LE;
			wbg_se_id = ISP_WBG_ID_FE0_RGBMAP_SE;
		} else if (raw_num == ISP_PRERAW_B) {
			blc_le_id = ISP_BLC_ID_FE1_LE;
			blc_se_id = ISP_BLC_ID_FE1_SE;
			wbg_le_id = ISP_WBG_ID_FE1_RGBMAP_LE;
			wbg_se_id = ISP_WBG_ID_FE1_RGBMAP_SE;
		} else {
			blc_le_id = ISP_BLC_ID_FE2_LE;
			wbg_le_id = ISP_WBG_ID_FE2_RGBMAP_LE;
		}

		ispblk_preraw_fe_config(ictx, raw_num);
		ispblk_csibdg_config(ictx, raw_num);
		ispblk_csibdg_crop_update(ictx, raw_num, true);

		ispblk_blc_set_gain(ictx, blc_le_id, 0x40f, 0x419, 0x419, 0x405);
		ispblk_blc_enable(ictx, blc_le_id, false, false);

		ispblk_wbg_config(ictx, wbg_le_id, 0x400, 0x400, 0x400);
		ispblk_wbg_enable(ictx, wbg_le_id, false, false);

		ispblk_rgbmap_config(ictx, ISP_BLK_ID_RGBMAP0, ictx->is_3dnr_on, raw_num);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on && !ictx->is_synthetic_hdr_on) {
			ispblk_blc_set_gain(ictx, blc_se_id, 0x40f, 0x419, 0x419, 0x405);
			ispblk_blc_enable(ictx, blc_se_id, false, false);

			ispblk_wbg_config(ictx, wbg_se_id, 0x400, 0x400, 0x400);
			ispblk_wbg_enable(ictx, wbg_se_id, false, false);

			ispblk_rgbmap_config(ictx, ISP_BLK_ID_RGBMAP1, ictx->is_3dnr_on, raw_num);
		} else {
			if (raw_num <= ISP_PRERAW_B) {
				ispblk_rgbmap_config(ictx, ISP_BLK_ID_RGBMAP1, false, raw_num);
			}
		}
	}
}

void _vi_ctrl_init(enum cvi_isp_raw raw_num, struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	enum cvi_isp_raw raw = ISP_PRERAW_A;

	if (ictx->is_ctrl_inited)
		return;

	if (vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w != 0) { //MW config snr_info flow
		ictx->isp_pipe_cfg[raw_num].csibdg_width = vdev->snr_info[raw_num].snr_fmt.img_size[0].width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height = vdev->snr_info[raw_num].snr_fmt.img_size[0].height;
		ictx->isp_pipe_cfg[raw_num].max_width =
						vdev->snr_info[raw_num].snr_fmt.img_size[0].max_width;
		ictx->isp_pipe_cfg[raw_num].max_height =
						vdev->snr_info[raw_num].snr_fmt.img_size[0].max_height;

		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->snr_info[raw_num].snr_fmt.img_size[0].start_x;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->snr_info[raw_num].snr_fmt.img_size[0].start_y;

		if (vdev->snr_info[raw_num].snr_fmt.frm_num > 1) { //HDR
			ictx->isp_pipe_cfg[raw_num].crop_se.w =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].active_w;
			ictx->isp_pipe_cfg[raw_num].crop_se.h =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].active_h;
			ictx->isp_pipe_cfg[raw_num].crop_se.x =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].start_x;
			ictx->isp_pipe_cfg[raw_num].crop_se.y =
							vdev->snr_info[raw_num].snr_fmt.img_size[1].start_y;

			ictx->isp_pipe_cfg[raw_num].is_hdr_on = true;

			vdev->ctx.is_hdr_on = true;
		}

		ictx->rgb_color_mode[raw_num] = vdev->snr_info[raw_num].color_mode;

		if ((ictx->rgb_color_mode[raw_num] == ISP_BAYER_TYPE_BGRGI) ||
			(ictx->rgb_color_mode[raw_num] == ISP_BAYER_TYPE_RGBGI)) {
			ictx->is_rgbir_sensor = true;
			ictx->isp_pipe_cfg[raw_num].is_rgbir_sensor = true;
		}
		vi_pr(VI_INFO, "sensor_%d csibdg_w_h(%d:%d)\n", raw_num,
			ictx->isp_pipe_cfg[raw_num].csibdg_width, ictx->isp_pipe_cfg[raw_num].csibdg_height);
	}

	if (ictx->isp_pipe_cfg[raw_num].is_patgen_en) {
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->usr_crop.top;
		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].csibdg_width	= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height	= vdev->usr_fmt.height;
		ictx->isp_pipe_cfg[raw_num].max_width		= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].max_height		= vdev->usr_fmt.height;

		ictx->rgb_color_mode[raw_num] = vdev->usr_fmt.code;

		vi_pr(VI_INFO, "patgen csibdg_w_h(%d:%d)\n",
			ictx->isp_pipe_cfg[raw_num].csibdg_width, ictx->isp_pipe_cfg[raw_num].csibdg_height);

#if defined( __SOC_PHOBOS__)
/**
 * the hardware limit is clk_mac <= clk_be * 2
 * cv180x's clk_mac is 594M, but clk_be just 198M(ND)/250M(OD)
 * clk_mac need to do frequency division.
 * ratio = (div_val + 1) / 32
 * target = source * ratio
 * div_val = target / source * 32 - 1
 * ex: target = 200, source = 594, div_val = 200 / 594 * 32 - 1 = 10
 */
		vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0,
					VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0_MASK,
					10 << VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0_OFFSET);
		vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0,
					VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0_MASK,
					1 << VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0_OFFSET);
		vip_sys_reg_write_mask(VIP_SYS_REG_UPDATE_SEL_CSI_MAC0,
					VIP_SYS_REG_UPDATE_SEL_CSI_MAC0_MASK,
					1 << VIP_SYS_REG_UPDATE_SEL_CSI_MAC0_OFFSET);
#endif
	} else if (ictx->isp_pipe_cfg[raw_num].is_offline_preraw) {
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->usr_crop.top;
		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].csibdg_width	= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height	= vdev->usr_fmt.height;
		ictx->isp_pipe_cfg[raw_num].max_width		= vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].max_height		= vdev->usr_fmt.height;

		ictx->rgb_color_mode[raw_num] = vdev->usr_fmt.code;

		vi_pr(VI_INFO, "csi_bdg=%d:%d, post_crop=%d:%d:%d:%d\n",
				vdev->usr_fmt.width, vdev->usr_fmt.height,
				vdev->usr_crop.width, vdev->usr_crop.height,
				vdev->usr_crop.left, vdev->usr_crop.top);
	}

	ictx->isp_pipe_cfg[raw_num].post_img_w = ictx->isp_pipe_cfg[raw_num].crop.w;
	ictx->isp_pipe_cfg[raw_num].post_img_h = ictx->isp_pipe_cfg[raw_num].crop.h;

	/* use csibdg crop */
	ictx->crop_x = 0;
	ictx->crop_y = 0;
	ictx->crop_se_x = 0;
	ictx->crop_se_y = 0;

	if (!ictx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
		//Postraw out size
		ictx->img_width = ictx->isp_pipe_cfg[ISP_PRERAW_A].crop.w;
		ictx->img_height = ictx->isp_pipe_cfg[ISP_PRERAW_A].crop.h;
	}

	if (raw_num == ISP_PRERAW_A) {
		if (_is_fe_be_online(ictx) && ictx->is_slice_buf_on)
			vi_calculate_slice_buf_setting(ictx, raw_num);

		isp_init(ictx);
	}

	vi_pr(VI_INFO, "sensor_%d init_done\n", raw_num);
	ictx->isp_pipe_cfg[raw_num].is_ctrl_inited = true;
	for (raw = ISP_PRERAW_A; raw < ISP_PRERAW_VIRT_MAX; raw++) {
		if (!ictx->isp_pipe_enable[raw])
			continue;
		if (!ictx->isp_pipe_cfg[raw].is_ctrl_inited)
			break;
	}

	if (raw == ISP_PRERAW_VIRT_MAX) {
		ictx->is_ctrl_inited = true;
	}
}

void _vi_scene_ctrl(struct cvi_vi_dev *vdev, enum cvi_isp_raw *raw_max)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	if (ctx->is_ctrl_inited) {
		// vi_scene_ctrl had been inited before.
		*raw_max = gViCtx->total_dev_num;
		return;
	}
#ifndef FPGA_PORTING
	if (gViCtx->total_dev_num >= 2) { // multi sensor scenario
		*raw_max = gViCtx->total_dev_num;
		ctx->is_multi_sensor = true;

		if (*raw_max == ISP_PRERAW_MAX) { // three sensor
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
			ctx->is_fbc_on = false;
			RGBMAP_BUF_IDX = 3;

			// if 422to420, chnAttr must be NV21
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) {
				if (ctx->isp_pipe_cfg[ISP_PRERAW_C].is_yuv_bypass_path &&
				ctx->isp_pipe_cfg[ISP_PRERAW_C].muxMode == VI_WORK_MODE_1Multiplex &&
				gViCtx->chnAttr[ctx->rawb_chnstr_num].enPixelFormat == PIXEL_FORMAT_NV21)
					ctx->isp_pipe_cfg[ISP_PRERAW_C].is_422_to_420 = true;
			}
		} else { // two sensor
			if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path &&
			    ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) { //rgb + yuv
				if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) {
					// dual offline
					// rgb: fe->be->dram(sbm)->post->dram
					// yuv: fe->dram
					ctx->is_offline_be = false;
					ctx->is_offline_postraw = true;
					ctx->is_slice_buf_on = true;
					RGBMAP_BUF_IDX = 2;

					// if 422to420, chnAttr must be NV21
					if (ctx->isp_pipe_cfg[ISP_PRERAW_B].muxMode == VI_WORK_MODE_1Multiplex &&
					    gViCtx->chnAttr[ctx->rawb_chnstr_num].enPixelFormat == PIXEL_FORMAT_NV21)
						ctx->isp_pipe_cfg[ISP_PRERAW_B].is_422_to_420 = true;
				} else {
					// dual online
					// rgb: fe->be->dram->post->sc
					// yuv: fe->dram->post->sc
					ctx->is_offline_be = false;
					ctx->is_offline_postraw = true;
					ctx->is_slice_buf_on = false;
					RGBMAP_BUF_IDX = 3;

					//TODO
					// // rgb: fe->dram->be->post->sc
					// // yuv: fe->dram->be->post->sc
					// ctx->is_offline_be = true;
					// ctx->is_offline_postraw = false;
					// ctx->is_slice_buf_on = false;
					// RGBMAP_BUF_IDX = 3;
				}
			} else { // rgb + rgb, fe->dram->be->post
				ctx->is_offline_be = true;
				ctx->is_offline_postraw = false;
				ctx->is_slice_buf_on = false;
				ctx->is_fbc_on = false;
				RGBMAP_BUF_IDX = 3;
			}
		}
	} else { // single sensor scenario
		*raw_max = ISP_PRERAW_B;
		ctx->is_multi_sensor = false;

		if (ctx->is_offline_be || ctx->is_offline_postraw) {
			ctx->is_offline_be = false;
			ctx->is_offline_postraw = true;
		}

		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			RGBMAP_BUF_IDX = 3;
			ctx->rgbmap_prebuf_idx = 0;
			ctx->is_slice_buf_on = false;
		} else {
			//Only single sensor with non-tile can use two rgbmap buf
			RGBMAP_BUF_IDX = 2;
			ctx->is_slice_buf_on = true;
		}

		//Currently don't support single yuv sensor online to scaler or on-the-fly
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) {
			ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler = true;
			ctx->isp_pipe_cfg[ISP_PRERAW_A].is_422_to_420 = false;
			ctx->is_slice_buf_on = false;
		}

		if (ctx->is_synthetic_hdr_on) {
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
			RGBMAP_BUF_IDX = 3;
		}
	}
#endif
	if (!sbm_en)
		ctx->is_slice_buf_on = false;

	if (rgbmap_sbm_en && ctx->is_slice_buf_on)
		ctx->is_rgbmap_sbm_on = true;

	//sbm on, rgbmapsbm off. update rgbmap after postrawdone
	if (ctx->is_slice_buf_on && !ctx->is_rgbmap_sbm_on)
		ctx->rgbmap_prebuf_idx = 0;

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) //RGB sensor
		ctx->rawb_chnstr_num = 1;
	else if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) //YUV sensor
		ctx->rawb_chnstr_num = ctx->isp_pipe_cfg[ISP_PRERAW_A].muxMode + 1;

	if (ctx->is_multi_sensor) {
		for (raw_num = ISP_PRERAW_B; raw_num < gViCtx->total_dev_num - 1; raw_num++) {
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
				ctx->rawb_chnstr_num++;
			else if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor
				ctx->rawb_chnstr_num += ctx->isp_pipe_cfg[raw_num].muxMode + 1;
		}
	}

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
			ctx->total_chn_num++;
		else //YUV sensor
			ctx->total_chn_num += ctx->isp_pipe_cfg[raw_num].muxMode + 1;
	}

	if (ctx->total_chn_num > 4) {
		vi_pr(VI_ERR, "[ERR] Total chn_num(%d) is wrong\n", ctx->total_chn_num);
		vi_pr(VI_ERR, "[ERR] raw_A,infMode(%d),muxMode(%d)\n",
				ctx->isp_pipe_cfg[ISP_PRERAW_A].infMode, ctx->isp_pipe_cfg[ISP_PRERAW_A].muxMode);
		if (ctx->is_multi_sensor) {
			vi_pr(VI_ERR, "[ERR] raw_B,infMode(%d),muxMode(%d)\n",
				ctx->isp_pipe_cfg[ISP_PRERAW_B].infMode, ctx->isp_pipe_cfg[ISP_PRERAW_B].muxMode);
		}
	}

	vi_pr(VI_INFO, "Total_chn_num=%d, rawb_chnstr_num=%d\n",
			ctx->total_chn_num, ctx->rawb_chnstr_num);

	ctx->is_slice_buf_on = false;
}

static void _vi_suspend(struct cvi_vi_dev *vdev)
{
	struct cvi_vi_ctx *pviProcCtx = NULL;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	pviProcCtx = (struct cvi_vi_ctx *)(vdev->shared_mem);

	if (pviProcCtx->vi_stt == VI_SUSPEND) {
		for (raw_num = ISP_PRERAW_A; raw_num < gViCtx->total_dev_num; raw_num++)
			isp_streaming(&vdev->ctx, false, raw_num);
		_vi_sw_init(vdev);
#ifndef FPGA_PORTING
		_vi_clk_ctrl(vdev, false);
#endif
	}
}

static int _vi_resume(struct cvi_vi_dev *vdev)
{
	struct cvi_vi_ctx *pviProcCtx = NULL;

	pviProcCtx = (struct cvi_vi_ctx *)(vdev->shared_mem);

	if (pviProcCtx->vi_stt == VI_SUSPEND) {
		//_vi_mempool_reset();
		//cvi_isp_sw_init(vdev);

		pviProcCtx->vi_stt = VI_RUNNING;
	}

	return 0;
}

void _viBWCalSet(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct cvi_vi_ctx *pviProcCtx = NULL;
	void __iomem *bw_limiter;
	u64 bwladdr[3] = {0x0A074020, 0x0A072020, 0x0A078020}; // rdma, wdma0, wdma1
	u32 bwlwin = 0, data_size[3] = {0}, BW[2][3] = {0}, total_bw = 0, bwltxn = 0, margin = 125, fps = 25;
	u32 def_bwltxn = 4, def_fps = 25;
	u32 width, height;
	u8 i, raw_num, yuv_chn_num;

	pviProcCtx = (struct cvi_vi_ctx *)(vdev->shared_mem);

	raw_num = ISP_PRERAW_A;
	fps = pviProcCtx->devAttr[raw_num].snrFps ?
		pviProcCtx->devAttr[raw_num].snrFps :
		def_fps;
	width = ctx->isp_pipe_cfg[raw_num].crop.w;
	height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
		fps = (fps >> 1);

	if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		data_size[0] = (411 * width * height) / 128 + 50052;
		data_size[1] = (396 * width * height) / 128 + 8160;
		data_size[2] = (391 * width * height) / 128 + 21536;
	} else {
		data_size[0] = (630 * width * height) / 128 + 50052;
		data_size[1] = (792 * width * height) / 128 + 8160;
		data_size[2] = (394 * width * height) / 128 + 38496;
	}

	for (i = 0; i < 3; ++i) {
		BW[0][i] = (fps * data_size[i]) / 1000000 + 1;
	}

	if (ctx->is_multi_sensor) {
		raw_num = ISP_PRERAW_B;
		fps = pviProcCtx->devAttr[raw_num].snrFps ?
			pviProcCtx->devAttr[raw_num].snrFps :
			def_fps;
		width = ctx->isp_pipe_cfg[raw_num].crop.w;
		height = ctx->isp_pipe_cfg[raw_num].crop.h;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
			fps = (fps >> 1);

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {//RGB sensor
			if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				data_size[0] = (411 * width * height) / 128 + 50052;
				data_size[1] = (396 * width * height) / 128 + 8160;
				data_size[2] = (391 * width * height) / 128 + 21536;
			} else {
				data_size[0] = (630 * width * height) / 128 + 50052;
				data_size[1] = (792 * width * height) / 128 + 8160;
				data_size[2] = (394 * width * height) / 128 + 38496;
			}
		} else { //YUV sensor
			yuv_chn_num = ctx->isp_pipe_cfg[raw_num].muxMode + 1;
			data_size[0] = (192 * yuv_chn_num * width * height) / 128;
			data_size[1] = (192 * yuv_chn_num * width * height) / 128;
			data_size[2] = 0;
		}

		for (i = 0; i < 3; ++i) {
			BW[1][i] = (fps * data_size[i]) / 1000000 + 1;
		}
	}

	// TODO
	// just restrain RDMA now, WDMA wait for Brian
	for (i = 0; i < 1; ++i) {
		total_bw = BW[0][i] + BW[1][i];
		for (bwltxn = def_bwltxn; bwltxn > 1; --bwltxn) {
			bwlwin = bwltxn * 256000 / ((((total_bw * 33) / 10) * margin) / 100);
			if (bwlwin <= 1024)
				break;
		}
		bw_limiter = ioremap(bwladdr[i], 0x4);
		iowrite32(((bwltxn << 10) | bwlwin), bw_limiter);
		vi_pr(VI_INFO, "isp %s bw_limiter=0x%x, BW=%d, bwltxn=%d, bwlwin=%d\n",
				(i == 0) ? "rdma" : ((i == 1) ? "wdma0" : "wdma1"),
				ioread32(bw_limiter), total_bw, bwltxn, bwlwin);
		iounmap(bw_limiter);
	}
}

static void _set_init_state(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (ctx->is_multi_sensor) {
		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
				atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_RUNNING);
			} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on)
				atomic_set(&vdev->pre_be_state[chn_num], ISP_PRE_BE_RUNNING);
			else if (_is_be_post_online(ctx))
				atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_RUNNING);
			else if (_is_all_online(ctx))
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_RUNNING);
		} else {
			if (_is_fe_be_online(ctx) || _is_be_post_online(ctx))
				atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_RUNNING);
		}
	}
}

int vi_start_streaming(struct cvi_vi_dev *vdev)
{
	struct cif_attr_s cif_attr;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw dev_num = ISP_PRERAW_A;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_raw raw_max = ISP_PRERAW_MAX - 1;
	int rc = 0;

	vi_pr(VI_DBG, "+\n");

	if (_vi_resume(vdev) != 0) {
		vi_pr(VI_ERR, "vi resume failed\n");
		return -1;
	}

	_vi_mempool_reset();
	vi_tuning_buf_clear();

	_vi_scene_ctrl(vdev, &raw_max);

	//SW workaround to disable csibdg enable first due to csibdg enable is on as default.
	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_MAX; raw_num++)
		isp_streaming(ctx, false, raw_num);

	/* cif lvds reset */
	_vi_call_cb(E_MODULE_CIF, CIF_CB_RESET_LVDS, &dev_num);
	if (ctx->is_multi_sensor) {
		for (raw_num = ISP_PRERAW_B; raw_num < gViCtx->total_dev_num; raw_num++) {
			dev_num = raw_num;
			_vi_call_cb(E_MODULE_CIF, CIF_CB_RESET_LVDS, &dev_num);
		}
	}

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_MAX; raw_num++) {
		/* Get stagger vsync info from cif */
		cif_attr.devno = raw_num;

		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (_vi_call_cb(E_MODULE_CIF, CIF_CB_GET_CIF_ATTR, &cif_attr) == 0)
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync = cif_attr.stagger_vsync;

		ctx->isp_pipe_cfg[raw_num].is_patgen_en = csi_patgen_en[raw_num];

		if (ctx->isp_pipe_cfg[raw_num].is_patgen_en) {
#ifndef PORTING_TEST
			vdev->usr_fmt.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
			vdev->usr_fmt.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
			vdev->usr_crop.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
			vdev->usr_crop.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
#else
			vdev->usr_fmt.width = 1920;
			vdev->usr_fmt.height = 1080;
			vdev->usr_crop.width = 1920;
			vdev->usr_crop.height = 1080;
#endif
			vdev->usr_fmt.code = ISP_BAYER_TYPE_BG;
			vdev->usr_crop.left = 0;
			vdev->usr_crop.top = 0;

			vi_pr(VI_WARN, "patgen enable, w_h(%d:%d), color mode(%d)\n",
					vdev->usr_fmt.width, vdev->usr_fmt.height, vdev->usr_fmt.code);
		}

		_vi_ctrl_init(raw_num, vdev);
		if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
			_vi_pre_fe_ctrl_setup(raw_num, vdev);
		if (!ctx->is_multi_sensor) { //only single sensor maybe break
			if (_is_all_online(ctx) ||
				(_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
				vi_pr(VI_INFO, "on-the-fly mode or slice_buffer is on\n");
				break;
			}
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
				_set_init_state(vdev, raw_num, ISP_FE_CH0);
				isp_pre_trig(ctx, raw_num, ISP_FE_CH0);
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->is_synthetic_hdr_on) {
					_set_init_state(vdev, raw_num, ISP_FE_CH1);
					isp_pre_trig(ctx, raw_num, ISP_FE_CH1);
				}

			} else { //YUV sensor
				u8 total_chn = (raw_num == ISP_PRERAW_A) ?
						ctx->rawb_chnstr_num :
						ctx->total_chn_num - ctx->rawb_chnstr_num;
				u8 chn_str = 0;

				for (; chn_str < total_chn; chn_str++) {
					_set_init_state(vdev, raw_num, chn_str);
					isp_pre_trig(ctx, raw_num, chn_str);
				}
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].is_mux &&
			ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin != -1U) {
			rc = gpio_request(ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin, "switch_gpio");
			if (rc) {
				vi_pr(VI_ERR, "request for switch_gpio_%d failed:%d\n",
					ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin, rc);
				return 0;
			}

			gpio_direction_output(ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin,
					ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPol);

			vi_pr(VI_DBG, "raw_num_%d switch_gpio_%d is_mux\n", raw_num,
				ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin);
		}
	}

	_vi_preraw_be_init(vdev);
	_vi_postraw_ctrl_setup(vdev);
	_vi_dma_setup(ctx, raw_max);
	_vi_dma_set_sw_mode(ctx);

	vi_pr(VI_INFO, "ISP scene path, be_off=%d, post_off=%d, slice_buff_on=%d\n",
			ctx->is_offline_be, ctx->is_offline_postraw, ctx->is_slice_buf_on);

	if (_is_all_online(ctx)) {
		raw_num = ISP_PRERAW_A;

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //offline mode
			if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
				isp_pre_trig(ctx, raw_num, ISP_FE_CH0);

			_postraw_outbuf_enq(vdev, raw_num);
		} else { //online mode
			struct sc_cfg_cb post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = raw_num;
			post_para.is_tile = false;
			post_para.bypass_num = gViCtx->bypass_frm[raw_num];
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_INFO, "sc is not ready. try later\n");
			} else {
				atomic_set(&vdev->ol_sc_frm_done, 0);

				if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
					isp_pre_trig(ctx, raw_num, ISP_FE_CH0);
			}
		}
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
		raw_num = ISP_PRERAW_A;

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //offline mode
			_postraw_outbuf_enq(vdev, raw_num);

			isp_post_trig(ctx, raw_num);

			if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
				if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
					isp_pre_trig(ctx, raw_num, ISP_FE_CH0);
					if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
						isp_pre_trig(ctx, raw_num, ISP_FE_CH1);
				}
			}

			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_RUNNING);
		} else { //online mode
			struct sc_cfg_cb post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = raw_num;
			post_para.is_tile = false;
			post_para.bypass_num = gViCtx->bypass_frm[raw_num];
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_INFO, "sc is not ready. try later\n");
			} else {
				atomic_set(&vdev->ol_sc_frm_done, 0);

				isp_post_trig(ctx, raw_num);

				if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
					isp_pre_trig(ctx, raw_num, ISP_FE_CH0);
					if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
						isp_pre_trig(ctx, raw_num, ISP_FE_CH1);
				}

				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_RUNNING);
			}
		}
	}

	if (bw_en) {
		_viBWCalSet(vdev);
	}

#ifdef PORTING_TEST
	vi_ip_test_cases_init(ctx);
#endif

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		isp_streaming(ctx, true, raw_num);
	}


	return rc;
}

/* abort streaming and wait for last buffer */
int vi_stop_streaming(struct cvi_vi_dev *vdev)
{
	struct cvi_isp_buf *cvi_vb, *tmp;
	struct _isp_dqbuf_n *n = NULL;
	struct vi_event_k   *ev_k = NULL;
	unsigned long flags;
	struct isp_buffer *isp_b;
	struct _isp_snr_i2c_node *i2c_n;
	struct _isp_raw_num_n    *raw_n;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	u8 i = 0, count = 10;
	u8 rc = 0;

	vi_pr(VI_INFO, "+\n");

	atomic_set(&vdev->isp_streamoff, 1);

	// disable load-from-dram at streamoff
	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++)
		vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw = false;

	usr_pic_time_remove();

	// wait to make sure hw stopped.
	while (--count > 0) {
		if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_PRERAW_IDLE &&
			atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_PRERAW_IDLE)
			break;
		vi_pr(VI_WARN, "wait count(%d)\n", count);
#ifdef FPGA_PORTING
		msleep(200);
#else
		msleep(20);
#endif
	}

	if (count == 0) {
		vi_pr(VI_ERR, "isp status fe_0(ch0:%d, ch1:%d) fe_1(ch0:%d, ch1:%d) fe_2(ch0:%d, ch1:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status be(ch0:%d, ch1:%d) postraw(%d)\n",
				atomic_read(&vdev->pre_be_state[ISP_BE_CH0]),
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]),
				atomic_read(&vdev->postraw_state));
	}

#if 0
	for (i = 0; i < 2; i++) {
		/*
		 * Release all the buffers enqueued to driver
		 * when streamoff is issued
		 */
		spin_lock_irqsave(&vdev->rdy_lock, flags);
		list_for_each_entry_safe(cvi_vb2, tmp, &(vdev->rdy_queue[i]), list) {
			vfree(cvi_vb2);
		}
		vdev->num_rdy[i] = 0;
		INIT_LIST_HEAD(&vdev->rdy_queue[i]);
		spin_unlock_irqrestore(&vdev->rdy_lock, flags);
	}
#endif

	for (i = 0; i < ISP_FE_CHN_MAX; i++) {
		/*
		 * Release all the buffers enqueued to driver
		 * when streamoff is issued
		 */
		spin_lock_irqsave(&vdev->qbuf_lock, flags);
		list_for_each_entry_safe(cvi_vb, tmp, &(vdev->qbuf_list[i]), list) {
			kfree(cvi_vb);
		}
		vdev->qbuf_num[i] = 0;
		INIT_LIST_HEAD(&vdev->qbuf_list[i]);
		spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
	}

	spin_lock_irqsave(&dq_lock, flags);
	while (!list_empty(&dqbuf_q.list)) {
		n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		list_del_init(&n->list);
		kfree(n);
	}
	spin_unlock_irqrestore(&dq_lock, flags);

	spin_lock_irqsave(&event_lock, flags);
	while (!list_empty(&event_q.list)) {
		ev_k = list_first_entry(&event_q.list, struct vi_event_k, list);
		list_del_init(&ev_k->list);
		kfree(ev_k);
	}
	spin_unlock_irqrestore(&event_lock, flags);

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		while ((isp_b = isp_buf_remove(&pre_out_queue[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&pre_out_se_queue[i])) != NULL)
			vfree(isp_b);
	}

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		while ((isp_b = isp_buf_remove(&raw_dump_b_dq[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_se_dq[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_q[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_se_q[i])) != NULL)
			vfree(isp_b);

		spin_lock_irqsave(&snr_node_lock[i], flags);
		while (!list_empty(&isp_snr_i2c_queue[i].list)) {
			i2c_n = list_first_entry(&isp_snr_i2c_queue[i].list, struct _isp_snr_i2c_node, list);
			list_del_init(&i2c_n->list);
			kfree(i2c_n);
		}
		isp_snr_i2c_queue[i].num_rdy = 0;
		spin_unlock_irqrestore(&snr_node_lock[i], flags);

		while ((isp_b = isp_buf_remove(&pre_be_in_se_q[i])) != NULL)
			vfree(isp_b);
	}

	while ((isp_b = isp_buf_remove(&pre_be_in_q)) != NULL)
		vfree(isp_b);
	while ((isp_b = isp_buf_remove(&pre_be_out_q)) != NULL)
		vfree(isp_b);
	while ((isp_b = isp_buf_remove(&pre_be_out_se_q)) != NULL)
		vfree(isp_b);

	while ((isp_b = isp_buf_remove(&post_in_queue)) != NULL)
		vfree(isp_b);
	while ((isp_b = isp_buf_remove(&post_in_se_queue)) != NULL)
		vfree(isp_b);

	spin_lock_irqsave(&raw_num_lock, flags);
	while (!list_empty(&pre_raw_num_q.list)) {
		raw_n = list_first_entry(&pre_raw_num_q.list, struct _isp_raw_num_n, list);
		list_del_init(&raw_n->list);
		kfree(raw_n);
	}
	spin_unlock_irqrestore(&raw_num_lock, flags);

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		kfree(isp_bufpool[i].fswdr_rpt);
		isp_bufpool[i].fswdr_rpt = 0;
	}

	// reset at stop for next run.
	isp_reset(&vdev->ctx);
	for (raw_num = ISP_PRERAW_A; raw_num < gViCtx->total_dev_num; raw_num++) {
		isp_streaming(&vdev->ctx, false, raw_num);
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_mux &&
			vdev->ctx.isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin != -1U) {
			gpio_free(vdev->ctx.isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin);
		}
	}
#ifdef PORTING_TEST
	vi_ip_test_cases_uninit(&vdev->ctx);
#endif
	_vi_suspend(vdev);

	return rc;
}

static int _pre_be_outbuf_enque(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
		enum ISP_BLK_ID_T pre_be_dma = (hw_chn_num == ISP_BE_CH0) ?
						ISP_BLK_ID_DMA_CTL22 : ISP_BLK_ID_DMA_CTL23;
		struct isp_queue *be_out_q = (hw_chn_num == ISP_BE_CH0) ?
						&pre_be_out_q : &pre_be_out_se_q;
		struct isp_buffer *b = NULL;

		b = isp_next_buf(be_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_be chn_num_%d outbuf is empty\n", hw_chn_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, pre_be_dma, b->addr);
	} else if (ctx->is_multi_sensor &&
		   ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB+YUV sensor
		u8 buf_chn = vdev->ctx.rawb_chnstr_num + hw_chn_num;
		enum ISP_BLK_ID_T pre_fe_dma;
		struct isp_queue *fe_out_q = &pre_out_queue[buf_chn];
		struct isp_buffer *b = NULL;

		if (raw_num == ISP_PRERAW_C) {
			pre_fe_dma = (buf_chn == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19;
		} else {
			pre_fe_dma = (buf_chn == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13;
		}

		b = isp_next_buf(fe_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_fe_%d buf_chn_num_%d outbuf is empty\n", raw_num, buf_chn);
			return 0;
		}

		ispblk_dma_setaddr(ctx, pre_fe_dma, b->addr);
	}

	return 1;
}

static int _pre_fe_outbuf_enque(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const enum cvi_isp_pre_chn_num fe_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw ac_raw = raw_num;

	ac_raw = find_hw_raw_num(raw_num);

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
		enum ISP_BLK_ID_T pre_fe_dma;
		struct isp_queue *fe_out_q = (fe_chn_num == ISP_FE_CH0) ?
						&pre_out_queue[raw_num] : &pre_out_se_queue[raw_num];
		struct isp_buffer *b = NULL;
		u8 trigger = false;

		if (ac_raw == ISP_PRERAW_A) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
				pre_fe_dma = ISP_BLK_ID_DMA_CTL6;
			else
				pre_fe_dma = (fe_chn_num == ISP_FE_CH0) ?
						ISP_BLK_ID_DMA_CTL6 : ISP_BLK_ID_DMA_CTL7;
		} else if (ac_raw == ISP_PRERAW_B) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
				pre_fe_dma = ISP_BLK_ID_DMA_CTL12;
			else
				pre_fe_dma = (fe_chn_num == ISP_FE_CH0) ?
						 ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13;
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
				pre_fe_dma = ISP_BLK_ID_DMA_CTL18;
			else
				pre_fe_dma = (fe_chn_num == ISP_FE_CH0) ?
						ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19;
		}

		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) {//raw_dump flow
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				trigger = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1];
			} else {
				trigger = true;
			}

			if (trigger) {
				struct isp_queue *fe_out_q = &raw_dump_b_q[raw_num];
				u32 dmaid, dmaid_se;

				if (ac_raw == ISP_PRERAW_A) {
					dmaid		= ISP_BLK_ID_DMA_CTL6;
					dmaid_se	= ISP_BLK_ID_DMA_CTL7;
				} else if (ac_raw == ISP_PRERAW_B) {
					dmaid		= ISP_BLK_ID_DMA_CTL12;
					dmaid_se	= ISP_BLK_ID_DMA_CTL13;
				} else {
					dmaid		= ISP_BLK_ID_DMA_CTL18;
					dmaid_se	= ISP_BLK_ID_DMA_CTL19;
				}

				vi_pr(VI_DBG, "pre_fe raw_dump cfg start\n");

				b = isp_next_buf(fe_out_q);
				if (b == NULL) {
					vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
					return 0;
				}

				ispblk_dma_setaddr(ctx, dmaid, b->addr);

				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->is_synthetic_hdr_on) {
					struct isp_buffer *b_se = NULL;
					struct isp_queue *fe_out_q_se = &raw_dump_b_se_q[raw_num];

					b_se = isp_next_buf(fe_out_q_se);
					if (b_se == NULL) {
						vi_pr(VI_ERR, "Pre_fe_%d SE raw_dump outbuf is empty\n", raw_num);
						return 0;
					}

					ispblk_dma_config(ctx, dmaid_se, raw_num, b_se->addr);
				}

				atomic_set(&vdev->isp_raw_dump_en[raw_num], 2);
			}
		} else if ((ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on) &&
			   (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3)) {
			struct isp_buffer *b_se = NULL;
			struct isp_queue *fe_out_q_se = &raw_dump_b_se_q[raw_num];

			b_se = isp_next_buf(fe_out_q_se);
			if (b_se == NULL) {
				vi_pr(VI_ERR, "Pre_fe_%d SE raw_dump outbuf is empty\n", raw_num);
				return 0;
			}

			ispblk_dma_config(ctx, pre_fe_dma, raw_num, b_se->addr);
		} else {
			b = isp_next_buf(fe_out_q);
			if (!b) {
				vi_pr(VI_DBG, "pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, fe_chn_num);
				return 0;
			}

			ispblk_dma_config(ctx, pre_fe_dma, raw_num, b->addr);
		}
	} else if (ctx->is_multi_sensor &&
		   ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB+YUV sensor
		u8 buf_chn = vdev->ctx.rawb_chnstr_num + fe_chn_num;
		enum ISP_BLK_ID_T pre_fe_dma;
		struct isp_queue *fe_out_q = &pre_out_queue[buf_chn];
		struct isp_buffer *b = NULL;

		if (raw_num == ISP_PRERAW_C) {
			pre_fe_dma = (buf_chn == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19;
		} else {
			pre_fe_dma = (buf_chn == ctx->rawb_chnstr_num) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13;
		}

		b = isp_next_buf(fe_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_fe_%d buf_chn_num_%d outbuf is empty\n", raw_num, buf_chn);
			return 0;
		}

		ispblk_dma_config(ctx, pre_fe_dma, raw_num, b->addr);
	}

	return 1;
}

static void _postraw_clear_inbuf(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL, *b_se = NULL;
	struct isp_queue *in_q = &pre_be_in_q;

	while ((b = isp_next_buf(in_q)) != NULL) {
		if (vdev->offline_raw_num != b->raw_num) {
			break;
		}

		b = isp_buf_remove(&pre_be_in_q);
		isp_buf_queue(&pre_out_queue[b->raw_num], b);
		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
			b_se = isp_buf_remove(&pre_be_in_se_q[b->raw_num]);
			isp_buf_queue(&pre_out_se_queue[b->raw_num], b_se);
		}
		vi_pr(VI_DBG, "remove raw_%d from be_in_q\n", b->raw_num);
	}
}

static int _postraw_inbuf_enq_check(
	struct cvi_vi_dev *vdev,
	enum cvi_isp_raw *raw_num,
	enum cvi_isp_chn_num *chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_queue *in_q = NULL, *in_se_q = NULL;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;

	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		in_q = &post_in_queue;
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		in_q = &pre_be_in_q;
	}

	if (unlikely(ctrl_flow) && ctx->is_multi_sensor) { //snr0/snr1
		_postraw_clear_inbuf(vdev);
	}

	b = isp_next_buf(in_q);
	if (b == NULL) {
		if (_is_fe_be_online(ctx)) //fe->be->dram->post
			vi_pr(VI_DBG, "Postraw input buf is empty\n");
		else if (_is_be_post_online(ctx)) //fe->dram->be->post
			vi_pr(VI_DBG, "Pre_be input buf is empty\n");
		ret = 1;
		return ret;
	}

	*raw_num = b->raw_num;
	*chn_num = (b->is_yuv_frm) ? b->chn_num : b->raw_num;

	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.x = b->crop_le.x;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.y = b->crop_le.y;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.w = vdev->ctx.img_width =
							ctx->isp_pipe_cfg[b->raw_num].post_img_w;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.h = vdev->ctx.img_height =
							ctx->isp_pipe_cfg[b->raw_num].post_img_h;

	//YUV sensor, offline return error, online than config rawtop read dma.
	if (ctx->isp_pipe_cfg[b->raw_num].is_yuv_bypass_path) {
		if (ctx->isp_pipe_cfg[b->raw_num].is_offline_scaler) {
			ret = 1;
		} else {
			ispblk_dma_yuv_bypass_config(ctx, ISP_BLK_ID_DMA_CTL28, b->addr, b->raw_num);
		}

		return ret;
	}

	isp_bufpool[b->raw_num].post_ir_busy_idx = b->ir_idx;

	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		in_se_q = &post_in_se_queue;
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		in_se_q = &pre_be_in_se_q[b->raw_num];
	}

	if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
		b_se = isp_next_buf(in_se_q);
		if (b_se == NULL) {
			if (_is_fe_be_online(ctx)) //fe->be->dram->post
				vi_pr(VI_DBG, "Postraw se input buf is empty\n");
			else if (_is_be_post_online(ctx)) //fe->dram->be->post
				vi_pr(VI_DBG, "Pre_be se input buf is empty\n");
			ret = 1;
			return ret;
		}
	}

	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.w_bit = b->rgbmap_i.w_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.h_bit = b->rgbmap_i.h_bit;

	vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.w_bit = b->lmap_i.w_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.h_bit = b->lmap_i.h_bit;

	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL28, b->raw_num, b->addr);

		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.x = b_se->crop_se.x;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.y = b_se->crop_se.y;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.w = vdev->ctx.img_width;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.h = vdev->ctx.img_height;

			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL29, b->raw_num, b_se->addr);
		}
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL4, b->raw_num, b->addr);
		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on)
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL5, b->raw_num, b_se->addr);
	}

	return ret;
}

static void _postraw_outbuf_enque(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct vi_buffer *vb2_buf;
	struct cvi_isp_buf *b = NULL;
	struct isp_ctx *ctx = &vdev->ctx;
	u64 tmp_addr = 0, i;
	u8 chn_num = raw_num;

	chn_num = find_ac_chn_num(ctx, raw_num);

	//Get the buffer for postraw output buffer
	b = _cvi_isp_next_buf(vdev, chn_num);
	if (b == NULL)
		return;

	vb2_buf = &b->buf;

	if (vb2_buf == NULL) {
		vi_pr(VI_DBG, "fail\n");
		return;
	}
	vi_pr(VI_DBG, "update isp-buf: 0x%llx-0x%llx\n",
		vb2_buf->planes[0].addr, vb2_buf->planes[1].addr);

	for (i = 0; i < 2; i++) {
		tmp_addr = (u64)vb2_buf->planes[i].addr;
		if (i == 0)
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL46, raw_num, tmp_addr);
		else
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL47, raw_num, tmp_addr);
	}
}

static u8 _postraw_outbuf_empty(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	u8 ret = 0;
	u8 chn_num = raw_num;

	chn_num = find_ac_chn_num(&vdev->ctx, raw_num);

	if (cvi_isp_rdy_buf_empty(vdev, chn_num)) {
		vi_pr(VI_DBG, "postraw chn_%d output buffer is empty\n", raw_num);
		ret = 1;
	}

	return ret;
}

void _postraw_outbuf_enq(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	u8 chn_num = raw_num;

	chn_num = find_ac_chn_num(&vdev->ctx, raw_num);

	cvi_isp_rdy_buf_pop(vdev, chn_num);
	_postraw_outbuf_enque(vdev, raw_num);
}

/*
 * for postraw offline only.
 *  trig preraw if there is output buffer in preraw output.
 */
void _pre_hw_enque(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw hw_raw = raw_num;

	// virt_raw and raw use the same state
	hw_raw = find_hw_raw_num(raw_num);

	//ISP frame error handling
	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handling done\n");
		return;
	}

#ifdef PORTING_TEST //test only
	if (stop_stream_en) {
		vi_pr(VI_WARN, "stop_stream_en\n");
		return;
	}
#endif

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		if (_is_drop_next_frame(vdev, raw_num, chn_num)) {
			vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d drop_frame_num %d\n",
					raw_num, chn_num, vdev->drop_frame_number[raw_num]);
			return;
		}

		if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
				if (atomic_cmpxchg(&vdev->pre_be_state[chn_num],
							ISP_PRE_BE_IDLE, ISP_PRE_BE_RUNNING) ==
							ISP_PRE_BE_RUNNING) {
					vi_pr(VI_DBG, "Pre_be chn_num_%d is running\n", chn_num);
					return;
				}
			} else { //YUV sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
							ISP_PRERAW_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return;
				}
			}

			// only if fe->be->dram
			if (_pre_be_outbuf_enque(vdev, raw_num, chn_num)) {
				if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
					_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);
				isp_pre_trig(ctx, raw_num, chn_num);
			} else {
				if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
					atomic_set(&vdev->pre_be_state[chn_num], ISP_PRE_BE_IDLE);
				else  //YUV sensor
					atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_IDLE);
			}
		} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				if (atomic_read(&vdev->isp_streamon) == 0) {
					vi_pr(VI_DBG, "VI not ready\n");
					return;
				}

				if (atomic_cmpxchg(&vdev->postraw_state, ISP_POSTRAW_IDLE, ISP_POSTRAW_RUNNING)
							!= ISP_POSTRAW_IDLE) {
					vi_pr(VI_DBG, "Postraw is running\n");
					return;
				}

				if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler onffline mode
					if (_postraw_outbuf_empty(vdev, raw_num)) {
						atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
						return;
					}

					_postraw_outbuf_enq(vdev, raw_num);
				} else { //Scaler online mode
					struct sc_cfg_cb post_para = {0};

					/* VI Online VPSS sc cb trigger */
					post_para.snr_num = raw_num;
					post_para.is_tile = false;
					post_para.bypass_num = gViCtx->bypass_frm[raw_num];
					vi_fill_mlv_info(NULL, raw_num, &post_para.m_lv_i, false);
					if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
						vi_pr(VI_DBG, "snr_num_%d, SC is running\n", raw_num);
						atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
						atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
						return;
					}

					atomic_set(&vdev->ol_sc_frm_done, 0);
				}

				if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
					_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);

				isp_pre_trig(ctx, raw_num, chn_num);
			} else {
				if (atomic_cmpxchg(&vdev->pre_fe_state[hw_raw][chn_num],
							ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
							ISP_PRERAW_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return;
				}

				// only if fe->dram
				if (_pre_fe_outbuf_enque(vdev, raw_num, chn_num))
					isp_pre_trig(ctx, raw_num, chn_num);
				else
					atomic_set(&vdev->pre_fe_state[hw_raw][chn_num], ISP_PRERAW_IDLE);
			}
		} else if (_is_all_online(ctx)) {
			if (atomic_cmpxchg(&vdev->postraw_state, ISP_POSTRAW_IDLE, ISP_POSTRAW_RUNNING)
						!= ISP_POSTRAW_IDLE) {
				vi_pr(VI_DBG, "Postraw is running\n");
				return;
			}

			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler onffline mode
				if (_postraw_outbuf_empty(vdev, raw_num)) {
					atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
					return;
				}

				_postraw_outbuf_enq(vdev, raw_num);
			}

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
				_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);

			isp_pre_trig(ctx, raw_num, chn_num);
		} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //fe->be->dram->post
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //RGB sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
							ISP_PRERAW_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe chn_num_%d is running\n", chn_num);
					return;
				}
			} else { //YUV sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_PRERAW_IDLE, ISP_PRERAW_RUNNING) ==
							ISP_PRERAW_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return;
				}
			}

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
				_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);
			isp_pre_trig(ctx, raw_num, chn_num);
		}
	}
}

static inline void _swap_post_sts_buf(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	struct _membuf *pool;
	unsigned long flags;
	uint8_t idx;

	pool = &isp_bufpool[raw_num];

	spin_lock_irqsave(&pool->post_sts_lock, flags);
	if (pool->post_sts_in_use == 1) {
		spin_unlock_irqrestore(&pool->post_sts_lock, flags);
		return;
	}
	pool->post_sts_busy_idx ^= 1;
	spin_unlock_irqrestore(&pool->post_sts_lock, flags);

	if (_is_be_post_online(ctx))
		idx = pool->post_sts_busy_idx ^ 1;
	else
		idx = pool->post_sts_busy_idx;

	//gms dma
	ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL25, raw_num, pool->sts_mem[idx].gms.phy_addr);

	//ae le dma
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL26, pool->sts_mem[idx].ae_le.phy_addr);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		//ae se dma
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL27, pool->sts_mem[idx].ae_se.phy_addr);
	}

	//dci dma is fixed size
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL45, pool->sts_mem[idx].dci.phy_addr);
	//hist edge v dma is fixed size
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL38, pool->sts_mem[idx].hist_edge_v.phy_addr);
}

static inline void _post_rgbmap_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const u32 frm_num)
{
	u64 rdma10, rdma11, rdma8, rdma9;
	u8 cur_idx = (frm_num - ctx->rgbmap_prebuf_idx) % RGBMAP_BUF_IDX;
	u8 pre_idx = (frm_num - 1 + RGBMAP_BUF_IDX - ctx->rgbmap_prebuf_idx) % RGBMAP_BUF_IDX;

	rdma10 = isp_bufpool[raw_num].rgbmap_le[cur_idx];
	if (frm_num <= ctx->rgbmap_prebuf_idx)
		rdma8 = isp_bufpool[raw_num].rgbmap_le[0];
	else
		rdma8 = isp_bufpool[raw_num].rgbmap_le[pre_idx];

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL34, rdma10);
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL32, rdma8);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		rdma11 = isp_bufpool[raw_num].rgbmap_se[cur_idx];
		if (frm_num <= ctx->rgbmap_prebuf_idx)
			rdma9 = isp_bufpool[raw_num].rgbmap_se[0];
		else
			rdma9 = isp_bufpool[raw_num].rgbmap_se[pre_idx];

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL35, rdma11);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL33, rdma9);
	}
}

static inline void _post_lmap_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	u64 lmap_le = isp_bufpool[raw_num].lmap_le;
	u64 lmap_se = isp_bufpool[raw_num].lmap_se;

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL39, lmap_le);
	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL30, lmap_le);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->isp_pipe_cfg[raw_num].is_hdr_detail_en) {
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL40, lmap_se);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL31, lmap_se);
	} else {
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL40, lmap_le);
#ifdef  __SOC_MARS__
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL31, lmap_le);
#endif
	}
}

static inline void _post_mlsc_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uint64_t lsc_dma = isp_bufpool[raw_num].lsc;

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL24, lsc_dma);
}

static inline void _post_ldci_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uint64_t ldci_dma = isp_bufpool[raw_num].ldci;

	ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL49, ldci_dma);
}

static inline void _post_dma_update(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint64_t manr_addr = isp_bufpool[raw_num].manr;
	uint64_t r_uv_addr, r_y_addr;
	uint64_t w_uv_addr, w_y_addr;

	r_uv_addr = w_uv_addr = isp_bufpool[raw_num].tdnr[0];
	r_y_addr  = w_y_addr  = isp_bufpool[raw_num].tdnr[1];

	//Update rgbmap dma addr
	_post_rgbmap_update(ctx, raw_num, vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);

	//update lmap dma
	_post_lmap_update(ctx, raw_num);

	//update mlsc dma
	_post_mlsc_update(ctx, raw_num);

	//update ldci dma
	_post_ldci_update(ctx, raw_num);

	if (ctx->is_3dnr_on) {
		if (ctx->is_fbc_on) {
			//3dnr y
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL41, raw_num, r_y_addr);
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL43, raw_num, w_y_addr);

			//3dnr uv
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL42, raw_num, r_uv_addr);
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL44, raw_num, w_uv_addr);
		} else {
			//3dnr y
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL41, r_y_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL43, w_y_addr);

			//3dnr uv
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL42, r_uv_addr);
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL44, w_uv_addr);
		}

		//manr
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL36, manr_addr);
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL37, manr_addr);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, true, false);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, true, false);
	} else {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, false, false);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, false, false);
	}
}

static u32 _is_fisrt_frm_after_drop(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint32_t first_frm_num_after_drop = ctx->isp_pipe_cfg[raw_num].isp_reset_frm;
	u32 frm_num = 0;

	frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];

	if ((first_frm_num_after_drop != 0) && (frm_num == first_frm_num_after_drop)) {
		vi_pr(VI_DBG, "reset isp frm_num[%d]\n", frm_num);
		ctx->isp_pipe_cfg[raw_num].isp_reset_frm = 0;
		return 1;
	} else
		return 0;
}

static inline void _post_ctrl_update(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	ispblk_post_cfg_update(ctx, raw_num);

	ispblk_fusion_hdr_cfg(ctx, raw_num);

	_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

	if (ctx->is_3dnr_on)
		ispblk_tnr_post_chg(ctx, raw_num);

	if (ctx->is_multi_sensor) {
		//To set apply the prev frm or not for manr/3dnr
		if (vdev->preraw_first_frm[raw_num]) {
			vdev->preraw_first_frm[raw_num] = false;
			isp_first_frm_reset(ctx, 1);
		} else {
			isp_first_frm_reset(ctx, 0);
		}
	}

	if (_is_fisrt_frm_after_drop(vdev, raw_num)) {
		isp_first_frm_reset(ctx, 1);
	} else {
		isp_first_frm_reset(ctx, 0);
	}
}

static uint8_t _pre_be_sts_in_use_chk(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 chn_num)
{
	unsigned long flags;
	static u8 be_in_use;

	if (chn_num == ISP_BE_CH0) {
		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		if (isp_bufpool[raw_num].pre_be_sts_in_use == 1) {
			be_in_use = 1;
		} else {
			be_in_use = 0;
			isp_bufpool[raw_num].pre_be_sts_busy_idx ^= 1;
		}
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
	}

	return be_in_use;
}

static inline void _swap_pre_be_sts_buf(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _membuf *pool;
	uint8_t idx;

	if (_pre_be_sts_in_use_chk(vdev, raw_num, chn_num) == 0) {
		pool = &isp_bufpool[raw_num];
		if (_is_be_post_online(ctx))
			idx = isp_bufpool[raw_num].pre_be_sts_busy_idx ^ 1;
		else
			idx = isp_bufpool[raw_num].pre_be_sts_busy_idx;

		if (chn_num == ISP_BE_CH0) {
			//af dma
			ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL21, raw_num, pool->sts_mem[idx].af.phy_addr);
		}
	}
}

static inline void _pre_be_ctrl_update(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	ispblk_pre_be_cfg_update(ctx, raw_num);
}

static inline int _isp_clk_dynamic_en(struct cvi_vi_dev *vdev, bool en)
{
#if 0//ToDo
	if (clk_dynamic_en && vdev->isp_clk[5]) {
		struct isp_ctx *ctx = &vdev->ctx;

		if (en && !__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vi_pr(VI_ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				else if (_is_be_post_online(ctx)) {
					atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
					atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				}

				return -1;
			}

			vi_pr(VI_DBG, "enable clk(%s)\n", CLK_ISP_NAME[5]);
		} else if (!en && __clk_is_enabled(vdev->isp_clk[5])) {
			clk_disable(vdev->isp_clk[5]);

			vi_pr(VI_DBG, "disable clk(%s)\n", CLK_ISP_NAME[5]);
		}
	} else { //check isp_top_clk is enabled
		struct isp_ctx *ctx = &vdev->ctx;

		if (!__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vip_pr(CVI_ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				else if (_is_be_post_online(ctx)) {
					atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
					atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				}
			}
		}
	}
#endif
	return 0;
}

/*
 * - postraw offline -
 *  trig postraw if there is in/out buffer for postraw
 * - postraw online -
 *  trig preraw if there is output buffer for postraw
 */
static void _post_hw_enque(
	struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_chn_num chn_num = ISP_CHN0;
	bool is_bypass = false;

	if (atomic_read(&vdev->isp_streamoff) == 1 && !ctx->is_slice_buf_on) {
		vi_pr(VI_DBG, "stop streaming\n");
		return;
	}

	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handing done\n");
		return;
	}

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		if (atomic_cmpxchg(&vdev->postraw_state, ISP_POSTRAW_IDLE, ISP_POSTRAW_RUNNING) != ISP_POSTRAW_IDLE) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (_postraw_inbuf_enq_check(vdev, &raw_num, &chn_num)) {
			atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = raw_num;
			post_para.is_tile = false;
			post_para.bypass_num = gViCtx->bypass_frm[raw_num];
			vi_fill_mlv_info(NULL, raw_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, raw_num)) {
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			_postraw_outbuf_enq(vdev, raw_num);
		}

		if (_isp_clk_dynamic_en(vdev, true) < 0)
			return;

		ispblk_post_yuv_cfg_update(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor online mode
			goto YUV_POSTRAW_TILE;

		postraw_tuning_update(&vdev->ctx, raw_num);

		//Update postraw size/ctrl flow
		_post_ctrl_update(vdev, raw_num);
		//Update postraw dma size/addr
		_post_dma_update(vdev, raw_num);
		//Update postraw stt gms/ae/hist_edge_v dma size/addr
		_swap_post_sts_buf(ctx, raw_num);

YUV_POSTRAW_TILE:
		vdev->offline_raw_num = raw_num;

		ctx->cam_id = raw_num;

		isp_post_trig(ctx, raw_num);

		vi_record_post_trigger(vdev, raw_num);
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		if (atomic_cmpxchg(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE, ISP_PRE_BE_RUNNING)
										!= ISP_PRE_BE_IDLE) {
			vi_pr(VI_DBG, "Pre_be ch_num_%d is running\n", ISP_BE_CH0);
			return;
		}

		if (atomic_cmpxchg(&vdev->postraw_state, ISP_POSTRAW_IDLE, ISP_POSTRAW_RUNNING) != ISP_POSTRAW_IDLE) {
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (_postraw_inbuf_enq_check(vdev, &raw_num, &chn_num)) {
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
			atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = raw_num;
			post_para.is_tile = false;
			post_para.bypass_num = gViCtx->bypass_frm[raw_num];
			vi_fill_mlv_info(NULL, raw_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, SC is running\n", raw_num);
				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, raw_num)) {
				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			_postraw_outbuf_enq(vdev, raw_num);
		}

		if (_isp_clk_dynamic_en(vdev, true) < 0)
			return;

		ispblk_post_yuv_cfg_update(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //YUV sensor online mode
			goto YUV_POSTRAW;

		pre_be_tuning_update(&vdev->ctx, raw_num);

		//Update pre be size/ctrl flow
		_pre_be_ctrl_update(vdev, raw_num);
		//Update pre be sts size/addr
		_swap_pre_be_sts_buf(vdev, raw_num, ISP_BE_CH0);

		postraw_tuning_update(&vdev->ctx, raw_num);

		//Update postraw size/ctrl flow
		_post_ctrl_update(vdev, raw_num);
		//Update postraw dma size/addr
		_post_dma_update(vdev, raw_num);
		//Update postraw sts awb/dci/hist_edge_v dma size/addr
		_swap_post_sts_buf(ctx, raw_num);

YUV_POSTRAW:
		vdev->offline_raw_num = raw_num;

		ctx->cam_id = raw_num;

		isp_post_trig(ctx, raw_num);
	} else if (_is_all_online(ctx)) { //on-the-fly

		if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = raw_num;
			post_para.is_tile = false;
			post_para.bypass_num = gViCtx->bypass_frm[raw_num];
			vi_fill_mlv_info(NULL, raw_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, SC is running\n", raw_num);
				atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		}

		_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

		vi_tuning_gamma_ips_update(ctx, raw_num);
		vi_tuning_clut_update(ctx, raw_num);
		vi_tuning_dci_update(ctx, raw_num);
		vi_tuning_drc_update(ctx, raw_num);

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
			_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
		//Things have to be done in post done
		if (atomic_cmpxchg(&ctx->is_post_done, 1, 0) == 1) { //Change is_post_done flag to 0
			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler offline mode
				if (!_postraw_outbuf_empty(vdev, raw_num))
					_postraw_outbuf_enq(vdev, raw_num);
			}

			_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

			vi_tuning_gamma_ips_update(ctx, raw_num);
			vi_tuning_clut_update(ctx, raw_num);
			vi_tuning_dci_update(ctx, raw_num);
			vi_tuning_drc_update(ctx, raw_num);

			if (!ctx->is_rgbmap_sbm_on) {
				//Update rgbmap dma addr
				_post_rgbmap_update(ctx, raw_num, vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
			}
		} else { //Things have to be done in be done for fps issue
			if (atomic_read(&vdev->isp_streamoff) == 0) {
				if (atomic_cmpxchg(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE, ISP_PRE_BE_RUNNING)
					!= ISP_PRE_BE_IDLE) {
					vi_pr(VI_DBG, "BE is running\n");
					return;
				}

				if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
					struct sc_cfg_cb post_para = {0};

					/* VI Online VPSS sc cb trigger */
					post_para.snr_num = raw_num;
					post_para.is_tile = false;
					post_para.bypass_num = gViCtx->bypass_frm[raw_num];
					vi_fill_mlv_info(NULL, raw_num, &post_para.m_lv_i, false);
					if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
						vi_pr(VI_DBG, "snr_num_%d, SC is not ready\n", raw_num);
						atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
						return;
					}

					atomic_set(&vdev->ol_sc_frm_done, 0);
				}
			}

			vdev->offline_raw_num = raw_num;
			ctx->cam_id = raw_num;

			atomic_set(&vdev->postraw_state, ISP_POSTRAW_RUNNING);

			//bypass first garbage frame when overflow happened
			is_bypass = (ctx->isp_pipe_cfg[raw_num].first_frm_cnt < 1) ? true : false;

			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
				ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, true, is_bypass);
				ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, true, is_bypass);
			} else {
				ISP_WR_BITS(ctx->phys_regs[ISP_BLK_ID_YUVTOP],
						REG_YUV_TOP_T, YUV_CTRL, BYPASS_V, !is_bypass);

				ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, is_bypass, is_bypass);
				ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, is_bypass, is_bypass);
			}

			isp_post_trig(ctx, raw_num);
			vi_record_post_trigger(vdev, raw_num);

			if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
				_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
					_pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
			}
		}
	}
}

static void _pre_fe_rgbmap_update(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const enum cvi_isp_pre_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint64_t rgbmap_buf = 0;
	u8 rgbmap_idx = 0;
	enum cvi_isp_raw raw;

	raw = find_hw_raw_num(raw_num);

	// In synthetic HDR mode, always used ch0's dma ctrl.
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on) {
		if (chn_num == ISP_FE_CH0) {
			rgbmap_idx = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]) % RGBMAP_BUF_IDX;
			rgbmap_buf = isp_bufpool[raw_num].rgbmap_se[rgbmap_idx];
		} else if (chn_num == ISP_FE_CH1) {
			rgbmap_idx = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]) % RGBMAP_BUF_IDX;
			rgbmap_buf = isp_bufpool[raw_num].rgbmap_le[rgbmap_idx];
		}

		if (raw == ISP_PRERAW_A)
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL10, rgbmap_buf);
		else if (raw == ISP_PRERAW_B)
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL16, rgbmap_buf);
		else
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL20, rgbmap_buf);
	} else {
		rgbmap_idx = (vdev->pre_fe_frm_num[raw_num][chn_num]) % RGBMAP_BUF_IDX;

		if (chn_num == ISP_FE_CH0) {
			rgbmap_buf = isp_bufpool[raw_num].rgbmap_le[rgbmap_idx];

			if (raw == ISP_PRERAW_A)
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL10, rgbmap_buf);
			else if (raw == ISP_PRERAW_B)
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL16, rgbmap_buf);
			else
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL20, rgbmap_buf);
		} else if (chn_num == ISP_FE_CH1) {
			rgbmap_buf = isp_bufpool[raw_num].rgbmap_se[rgbmap_idx];

			if (raw == ISP_PRERAW_A)
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL11, rgbmap_buf);
			else if (raw == ISP_PRERAW_B)
				ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL17, rgbmap_buf);
		}
	}
}

void vi_destory_thread(struct cvi_vi_dev *vdev, enum E_VI_TH th_id)
{
	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		pr_err("No such thread_id(%d)\n", th_id);
		return;
	}

	if (vdev->vi_th[th_id].w_thread != NULL) {
		int ret;

		ret = kthread_stop(vdev->vi_th[th_id].w_thread);
		if (!ret) {
			while (atomic_read(&vdev->vi_th[th_id].thread_exit) == 0) {
				pr_info("wait for %s exit\n", vdev->vi_th[th_id].th_name);
				usleep_range(5 * 1000, 10 * 1000);
			}
		}
		vdev->vi_th[th_id].w_thread = NULL;
	}
}

int vi_create_thread(struct cvi_vi_dev *vdev, enum E_VI_TH th_id)
{
	struct sched_param param;
	int rc = 0;

	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		pr_err("No such thread_id(%d)\n", th_id);
		return -1;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;

	if (vdev->vi_th[th_id].w_thread == NULL) {
		switch (th_id) {
		case E_VI_TH_PRERAW:
			memcpy(vdev->vi_th[th_id].th_name, "cvitask_isp_pre", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_preraw_thread;
			break;
		case E_VI_TH_VBLANK_HANDLER:
			memcpy(vdev->vi_th[th_id].th_name, "cvitask_isp_blank", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_vblank_handler_thread;
			break;
		case E_VI_TH_ERR_HANDLER:
			memcpy(vdev->vi_th[th_id].th_name, "cvitask_isp_err", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_err_handler_thread;
			break;
		case E_VI_TH_EVENT_HANDLER:
			memcpy(vdev->vi_th[th_id].th_name, "vi_event_handler", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_event_handler_thread;
			break;
		default:
			pr_err("No such thread(%d)\n", th_id);
			return -1;
		}
		vdev->vi_th[th_id].w_thread = kthread_create(vdev->vi_th[th_id].th_handler,
								(void *)vdev,
								vdev->vi_th[th_id].th_name);
		if (IS_ERR(vdev->vi_th[th_id].w_thread)) {
			pr_err("Unable to start %s.\n", vdev->vi_th[th_id].th_name);
			return -1;
		}

		sched_setscheduler(vdev->vi_th[th_id].w_thread, SCHED_FIFO, &param);

		vdev->vi_th[th_id].flag = 0;
		atomic_set(&vdev->vi_th[th_id].thread_exit, 0);
		init_waitqueue_head(&vdev->vi_th[th_id].wq);
		wake_up_process(vdev->vi_th[th_id].w_thread);
	}

	return rc;
}

static void _vi_sw_init(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct cvi_vi_ctx *pviProcCtx = NULL;
	u8 i = 0, j = 0;

	pviProcCtx = (struct cvi_vi_ctx *)(vdev->shared_mem);

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	init_timer(&usr_pic_timer);
#endif

	vdev->postraw_proc_num		= 0;
	vdev->offline_raw_num		= ISP_PRERAW_MAX;
	ctx->is_offline_be		= false;
	ctx->is_offline_postraw		= true;
	ctx->is_3dnr_on			= true;
	ctx->is_dpcm_on			= false;
	ctx->is_hdr_on			= false;
	ctx->is_multi_sensor		= false;
	ctx->is_yuv_sensor		= false;
	ctx->is_sublvds_path		= false;
	ctx->is_fbc_on			= true;
	ctx->is_rgbir_sensor		= false;
	ctx->is_ctrl_inited		= false;
	ctx->is_slice_buf_on		= true;
	ctx->is_rgbmap_sbm_on		= false;
	ctx->is_synthetic_hdr_on	= false;
	ctx->rgbmap_prebuf_idx		= 1;
	ctx->cam_id			= 0;
	ctx->rawb_chnstr_num		= 1;
	ctx->total_chn_num		= 0;
	ctx->gamma_tbl_idx		= 0;
	vdev->usr_pic_delay		= 0;
	vdev->is_yuv_trigger		= false;
	vdev->isp_source		= CVI_ISP_SOURCE_DEV;

	if (pviProcCtx->vi_stt != VI_SUSPEND)
		memset(vdev->snr_info, 0, sizeof(struct cvi_isp_snr_info) * ISP_PRERAW_MAX);

	for (i = 0; i < ARRAY_SIZE(vdev->usr_pic_phy_addr); i++) {
		vdev->usr_pic_phy_addr[i] = 0;
	}

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		for (j = 0; j < ISP_BE_CHN_MAX; j++) {
			vdev->pre_be_frm_num[i][j]		= 0;
			vdev->isp_triggered[i][j]		= false;
		}
		vdev->preraw_first_frm[i]			= true;
		vdev->postraw_frame_number[i]			= 0;
		vdev->drop_frame_number[i]			= 0;
		vdev->dump_frame_number[i]			= 0;
		vdev->isp_int_flag[i]				= false;
		vdev->ctx.mmap_grid_size[i]			= 3;
		vdev->isp_err_times[i]				= 0;
		vdev->ctx.isp_pipe_enable[i]			= false;

		memset(&ctx->isp_pipe_cfg[i], 0, sizeof(struct _isp_cfg));

		ctx->isp_pipe_cfg[i].is_offline_scaler		= true;

		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			vdev->pre_fe_sof_cnt[i][j]		= 0;
			vdev->pre_fe_frm_num[i][j]		= 0;
			vdev->pre_fe_trig_cnt[i][j]		= 0;

			atomic_set(&vdev->pre_fe_state[i][j], ISP_PRERAW_IDLE);
		}

		spin_lock_init(&snr_node_lock[i]);

		atomic_set(&vdev->isp_raw_dump_en[i], 0);
		atomic_set(&vdev->isp_smooth_raw_dump_en[i], 0);
	}

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		INIT_LIST_HEAD(&pre_out_queue[i].rdy_queue);
		INIT_LIST_HEAD(&pre_out_se_queue[i].rdy_queue);
		pre_out_queue[i].num_rdy       = 0;
		pre_out_queue[i].raw_num       = 0;
		pre_out_se_queue[i].num_rdy    = 0;
		pre_out_se_queue[i].raw_num    = 0;
	}

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		INIT_LIST_HEAD(&raw_dump_b_dq[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_dq[i].rdy_queue);
		raw_dump_b_dq[i].num_rdy          = 0;
		raw_dump_b_dq[i].raw_num          = i;
		raw_dump_b_se_dq[i].num_rdy       = 0;
		raw_dump_b_se_dq[i].raw_num       = i;

		INIT_LIST_HEAD(&raw_dump_b_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_q[i].rdy_queue);
		raw_dump_b_q[i].num_rdy          = 0;
		raw_dump_b_q[i].raw_num          = i;
		raw_dump_b_se_q[i].num_rdy       = 0;
		raw_dump_b_se_q[i].raw_num       = i;

		INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy     = 0;
		INIT_LIST_HEAD(&isp_crop_queue[i].list);
		isp_crop_queue[i].num_rdy     = 0;

		INIT_LIST_HEAD(&pre_be_in_se_q[i].rdy_queue);
		pre_be_in_se_q[i].num_rdy        = 0;
	}

	INIT_LIST_HEAD(&pre_raw_num_q.list);
	INIT_LIST_HEAD(&dqbuf_q.list);
	INIT_LIST_HEAD(&event_q.list);

	INIT_LIST_HEAD(&pre_be_in_q.rdy_queue);
	INIT_LIST_HEAD(&pre_be_out_q.rdy_queue);
	INIT_LIST_HEAD(&pre_be_out_se_q.rdy_queue);
	pre_be_in_q.num_rdy	= 0;
	pre_be_out_q.num_rdy	= 0;
	pre_be_out_se_q.num_rdy	= 0;

	INIT_LIST_HEAD(&post_in_queue.rdy_queue);
	INIT_LIST_HEAD(&post_in_se_queue.rdy_queue);
	post_in_queue.num_rdy	 = 0;
	post_in_se_queue.num_rdy = 0;

	atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRERAW_IDLE);
	atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_PRERAW_IDLE);
	atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
	atomic_set(&vdev->isp_streamoff, 0);
	atomic_set(&vdev->isp_err_handle_flag, 0);
	atomic_set(&vdev->ol_sc_frm_done, 1);
	atomic_set(&vdev->isp_dbg_flag, 0);
	atomic_set(&vdev->ctx.is_post_done, 0);

	spin_lock_init(&buf_lock);
	spin_lock_init(&raw_num_lock);
	spin_lock_init(&dq_lock);
	spin_lock_init(&event_lock);
	spin_lock_init(&vdev->qbuf_lock);

	init_waitqueue_head(&vdev->isp_dq_wait_q);
	init_waitqueue_head(&vdev->isp_event_wait_q);
	init_waitqueue_head(&vdev->isp_dbg_wait_q);

	vi_tuning_sw_init();
}

static void _vi_init_param(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uint8_t i = 0;

	atomic_set(&dev_open_cnt, 0);

	memset(ctx, 0, sizeof(*ctx));

	ctx->phys_regs		= isp_get_phys_reg_bases();
	ctx->cam_id		= 0;

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		ctx->rgb_color_mode[i]			= ISP_BAYER_TYPE_GB;
		ctx->isp_pipe_cfg[i].is_patgen_en	= false;
		ctx->isp_pipe_cfg[i].is_offline_preraw	= false;
		ctx->isp_pipe_cfg[i].is_yuv_bypass_path	= false;
		ctx->isp_pipe_cfg[i].is_drop_next_frame	= false;
		ctx->isp_pipe_cfg[i].isp_reset_frm	= 0;
		ctx->isp_pipe_cfg[i].is_422_to_420	= false;
		ctx->isp_pipe_cfg[i].max_height		= 0;
		ctx->isp_pipe_cfg[i].max_width		= 0;
		ctx->isp_pipe_cfg[i].csibdg_width	= 0;
		ctx->isp_pipe_cfg[i].csibdg_height	= 0;

		INIT_LIST_HEAD(&raw_dump_b_dq[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_dq[i].rdy_queue);
		raw_dump_b_dq[i].num_rdy          = 0;
		raw_dump_b_dq[i].raw_num          = i;
		raw_dump_b_se_dq[i].num_rdy       = 0;
		raw_dump_b_se_dq[i].raw_num       = i;

		INIT_LIST_HEAD(&raw_dump_b_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_q[i].rdy_queue);
		raw_dump_b_q[i].num_rdy          = 0;
		raw_dump_b_q[i].raw_num          = i;
		raw_dump_b_se_q[i].num_rdy       = 0;
		raw_dump_b_se_q[i].raw_num       = i;

		INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy     = 0;

		INIT_LIST_HEAD(&pre_be_in_se_q[i].rdy_queue);
		pre_be_in_se_q[i].num_rdy        = 0;
	}

	INIT_LIST_HEAD(&pre_raw_num_q.list);

	memset(&vdev->usr_crop, 0, sizeof(vdev->usr_crop));

	for (i = 0; i < ISP_FE_CHN_MAX; i++) {
		INIT_LIST_HEAD(&vdev->qbuf_list[i]);
		vdev->qbuf_num[i] = 0;
	}

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		vdev->isp_int_flag[i] = false;
		init_waitqueue_head(&vdev->isp_int_wait_q[i]);
	}

	//ToDo sync_task_ext
	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++)
		sync_task_init(i);

	tasklet_init(&vdev->job_work, isp_post_tasklet, (unsigned long)vdev);

	atomic_set(&vdev->isp_streamon, 0);
}

static int _vi_mempool_setup(void)
{
	int ret = 0;

	_vi_mempool_reset();
	ret = vi_tuning_buf_setup();

	return ret;
}

int vi_mac_clk_ctrl(struct cvi_vi_dev *vdev, u8 mac_num, u8 enable)
{
	int rc = 0;

	if (mac_num >= ARRAY_SIZE(vdev->clk_mac))
		return rc;

	if (vdev->clk_mac[mac_num]) {
		if (enable) {
			if (clk_prepare_enable(vdev->clk_mac[mac_num])) {
				vi_pr(VI_ERR, "Failed to prepare and enable clk_mac(%d)\n", mac_num);
				rc = -EAGAIN;
				goto EXIT;
			}
		} else {
			if (__clk_is_enabled(vdev->clk_mac[mac_num]))
				clk_disable_unprepare(vdev->clk_mac[mac_num]);
			else
				clk_unprepare(vdev->clk_mac[mac_num]);
		}
	} else {
		vi_pr(VI_ERR, "clk_mac(%d) is null\n", mac_num);
		rc = -EAGAIN;
		goto EXIT;
	}
EXIT:
	return rc;
}

#ifndef FPGA_PORTING
static int _vi_clk_ctrl(struct cvi_vi_dev *vdev, u8 enable)
{
	u8 i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(vdev->clk_sys); ++i) {
		if (vdev->clk_sys[i]) {
			if (enable) {
				if (clk_prepare_enable(vdev->clk_sys[i])) {
					vi_pr(VI_ERR, "Failed to prepare and enable clk_sys(%d)\n", i);
					rc = -EAGAIN;
					goto EXIT;
				}
			} else {
				if (__clk_is_enabled(vdev->clk_sys[i]))
					clk_disable_unprepare(vdev->clk_sys[i]);
				else
					clk_unprepare(vdev->clk_sys[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_sys(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	for (i = 0; i < ARRAY_SIZE(vdev->clk_isp); ++i) {
		if (vdev->clk_isp[i]) {
			if (enable) {
				if (clk_prepare_enable(vdev->clk_isp[i])) {
					vi_pr(VI_ERR, "Failed to enable clk_isp(%d)\n", i);
					rc = -EAGAIN;
					goto EXIT;
				}
			} else {
				if (__clk_is_enabled(vdev->clk_isp[i]))
					clk_disable_unprepare(vdev->clk_isp[i]);
				else
					clk_unprepare(vdev->clk_isp[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_isp(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	//Set axi_isp_top_clk_en 1
	vip_sys_reg_write_mask(0x4, BIT(0), BIT(0));
EXIT:
	return rc;
}
#endif

void _vi_sdk_release(struct cvi_vi_dev *vdev)
{
	u8 i = 0;

	vi_disable_chn(0);

	for (i = 0; i < VI_MAX_CHN_NUM; i++) {
		memset(&gViCtx->chnAttr[i], 0, sizeof(VI_CHN_ATTR_S));
		memset(&gViCtx->chnStatus[i], 0, sizeof(VI_CHN_STATUS_S));
		gViCtx->blk_size[i] = 0;
		gViCtx->bypass_frm[i] = 0;
	}

	for (i = 0; i < VI_MAX_PIPE_NUM; i++)
		gViCtx->isPipeCreated[i] = false;

	for (i = 0; i < VI_MAX_DEV_NUM; i++)
		gViCtx->isDevEnable[i] = false;
}

static void _vi_release_op(struct cvi_vi_dev *vdev)
{
#ifndef FPGA_PORTING
	u8 i = 0;

	_vi_clk_ctrl(vdev, false);

	for (i = 0; i < gViCtx->total_dev_num; i++) {
		vi_mac_clk_ctrl(vdev, i, false);
	}
#endif
}

static int _vi_create_proc(struct cvi_vi_dev *vdev)
{
	int ret = 0;

	/* vi proc setup */
	vdev->shared_mem = kzalloc(VI_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!vdev->shared_mem) {
		pr_err("shared_mem alloc size(%d) failed\n", VI_SHARE_MEM_SIZE);
		return -ENOMEM;
	}

	if (vi_proc_init(vdev, vdev->shared_mem) < 0) {
		pr_err("vi proc init failed\n");
		return -EAGAIN;
	}

	if (vi_dbg_proc_init(vdev) < 0) {
		pr_err("vi_dbg proc init failed\n");
		return -EAGAIN;
	}

	if (isp_proc_init(vdev) < 0) {
		pr_err("isp proc init failed\n");
		return -EAGAIN;
	}

	return ret;
}

static void _vi_destroy_proc(struct cvi_vi_dev *vdev)
{
	vi_proc_remove();
	vi_dbg_proc_remove();
	kfree(vdev->shared_mem);
	vdev->shared_mem = NULL;

	isp_proc_remove();
}

/*******************************************************
 *  File operations for core
 ******************************************************/
static long _vi_s_ctrl(struct cvi_vi_dev *vdev, struct vi_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct isp_ctx *ctx = &vdev->ctx;

	switch (id) {
	case VI_IOCTL_SDK_CTRL:
	{
		rc = vi_sdk_ctrl(vdev, p);
		break;
	}

	case VI_IOCTL_HDR:
	{
#if defined( __SOC_PHOBOS__)
		if (p->value == true) {
			vi_pr(VI_ERR, "only support linear mode.\n");
			break;
		}
#endif
		ctx->is_hdr_on = p->value;
		ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on = p->value;
		vi_pr(VI_INFO, "HDR_ON(%d) for test\n", ctx->is_hdr_on);
		rc = 0;
		break;
	}

	case VI_IOCTL_HDR_DETAIL_EN:
	{
		u32 val = 0, snr_num = 0, enable = 0;

		val = p->value;
		snr_num = val & 0x3; //bit0~1: snr_num
		enable = val & 0x4; //bit2: enable/disable

		if (snr_num < ISP_PRERAW_VIRT_MAX) {
			ctx->isp_pipe_cfg[snr_num].is_hdr_detail_en = enable;
			vi_pr(VI_WARN, "HDR_DETAIL_EN(%d)\n",
				ctx->isp_pipe_cfg[snr_num].is_hdr_detail_en);
			rc = 0;
		}

		break;
	}

	case VI_IOCTL_3DNR:
		ctx->is_3dnr_on = p->value;
		vi_pr(VI_INFO, "is_3dnr_on=%d\n", ctx->is_3dnr_on);
		rc = 0;
		break;

	case VI_IOCTL_TILE:
		rc = 0;
		break;

	case VI_IOCTL_COMPRESS_EN:
		ctx->is_dpcm_on = p->value;
		vi_pr(VI_INFO, "ISP_COMPRESS_ON(%d)\n", ctx->is_dpcm_on);
		rc = 0;
		break;

	case VI_IOCTL_STS_PUT:
	{
		u8 raw_num = 0;
		unsigned long flags;

		raw_num = p->value;

		if (raw_num >= ISP_PRERAW_VIRT_MAX)
			break;

		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		isp_bufpool[raw_num].pre_be_sts_in_use = 0;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_PUT:
	{
		u8 raw_num = 0;
		unsigned long flags;

		raw_num = p->value;

		if (raw_num >= ISP_PRERAW_VIRT_MAX)
			break;

		spin_lock_irqsave(&isp_bufpool[raw_num].post_sts_lock, flags);
		isp_bufpool[raw_num].post_sts_in_use = 0;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].post_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_USR_PIC_CFG:
	{
		struct cvi_isp_usr_pic_cfg cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_isp_usr_pic_cfg)))
			break;

		if ((cfg.crop.width < 32) || (cfg.crop.width > 4096)
			|| (cfg.crop.left > cfg.crop.width) || (cfg.crop.top > cfg.crop.height)) {
			vi_pr(VI_ERR, "USR_PIC_CFG:(Invalid Param) w(%d) h(%d) x(%d) y(%d)",
				cfg.crop.width, cfg.crop.height, cfg.crop.left, cfg.crop.top);
		} else {
			vdev->usr_fmt = cfg.fmt;
			vdev->usr_crop = cfg.crop;

			vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_width	= vdev->usr_fmt.width;
			vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].csibdg_height	= vdev->usr_fmt.height;
			vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].max_width		= vdev->usr_fmt.width;
			vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].max_height 	= vdev->usr_fmt.height;

			rc = 0;
		}

		break;
	}

	case VI_IOCTL_USR_PIC_ONOFF:
	{
		vdev->isp_source = p->value;
		ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw =
			(vdev->isp_source == CVI_ISP_SOURCE_FE);

		vi_pr(VI_INFO, "vdev->isp_source=%d\n", vdev->isp_source);
		vi_pr(VI_INFO, "ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw=%d\n",
			ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw);

		rc = 0;
		break;
	}

	case VI_IOCTL_PUT_PIPE_DUMP:
	{
		u32 raw_num = 0;

		raw_num = p->value;

		if (isp_byr[raw_num]) {
			vfree(isp_byr[raw_num]);
			isp_byr[raw_num] = NULL;
		}

		if (isp_byr_se[raw_num]) {
			vfree(isp_byr_se[raw_num]);
			isp_byr_se[raw_num] = NULL;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_USR_PIC_PUT:
	{
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
#if 1
			u64 phy_addr = p->value64;
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL4, phy_addr);
			vdev->usr_pic_phy_addr[0] = phy_addr;
			vi_pr(VI_INFO, "\nvdev->usr_pic_phy_addr(0x%llx)\n", vdev->usr_pic_phy_addr[0]);
			rc = 0;

			if (vdev->usr_pic_delay)
				usr_pic_timer_init(vdev);
#else //for vip_FPGA test
			uint64_t bufaddr = 0;
			uint32_t bufsize = 0;

			bufaddr = _mempool_get_addr();
			bufsize = ispblk_dma_config(ctx, ISP_BLK_ID_RDMA0, bufaddr);
			_mempool_pop(bufsize);

			vi_pr(VI_WARN, "\nRDMA0 base_addr=0x%x\n", bufaddr);

			vdev->usr_pic_phy_addr = bufaddr;
			rc = 0;
#endif
		}
		break;
	}

	case VI_IOCTL_USR_PIC_TIMING:
	{
		if (p->value > 30)
			vdev->usr_pic_delay = msecs_to_jiffies(33);
		else if (p->value > 0)
			vdev->usr_pic_delay = msecs_to_jiffies(1000 / p->value);
		else
			vdev->usr_pic_delay = 0;

		if (!vdev->usr_pic_delay)
			usr_pic_time_remove();

		rc = 0;
		break;
	}

	case VI_IOCTL_ONLINE:
		ctx->is_offline_postraw = !p->value;
		vi_pr(VI_INFO, "is_offline_postraw=%d\n", ctx->is_offline_postraw);
		rc = 0;
		break;

	case VI_IOCTL_BE_ONLINE:
		ctx->is_offline_be = !p->value;
		vi_pr(VI_INFO, "is_offline_be=%d\n", ctx->is_offline_be);
		rc = 0;
		break;

	case VI_IOCTL_SET_SNR_CFG_NODE:
	{
		struct cvi_isp_snr_update *snr_update;
		u8			  raw_num;

		if (vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
			break;

		snr_update = vmalloc(sizeof(struct cvi_isp_snr_update));
		if (copy_from_user(snr_update, p->ptr, sizeof(struct cvi_isp_snr_update)) != 0) {
			vi_pr(VI_ERR, "SNR_CFG_NODE copy from user fail.\n");
			vfree(snr_update);
			break;
		}
		raw_num = snr_update->raw_num;

		if (raw_num >= ISP_PRERAW_VIRT_MAX) {
			vfree(snr_update);
			break;
		}

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_preraw ||
			vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
			rc = 0;
			vfree(snr_update);
			break;
		}

		vi_pr(VI_DBG, "raw_num=%d, magic_num=%d, regs_num=%d, i2c_update=%d, isp_update=%d\n",
			raw_num,
			snr_update->snr_cfg_node.snsr.magic_num,
			snr_update->snr_cfg_node.snsr.regs_num,
			snr_update->snr_cfg_node.snsr.need_update,
			snr_update->snr_cfg_node.isp.need_update);

		_isp_snr_cfg_enq(snr_update, raw_num);

		vfree(snr_update);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SNR_INFO:
	{
		struct cvi_isp_snr_info snr_info;

		if (copy_from_user(&snr_info, p->ptr, sizeof(struct cvi_isp_snr_info)) != 0)
			break;
#if defined( __SOC_PHOBOS__)
		if (snr_info.raw_num >= ISP_PRERAW_VIRT_MAX) {
			vi_pr(VI_ERR, "only support single sensor.\n");
			break;
		}
#endif
		memcpy(&vdev->snr_info[snr_info.raw_num], &snr_info, sizeof(struct cvi_isp_snr_info));
		vi_pr(VI_WARN, "raw_num=%d, color_mode=%d, frm_num=%d, snr_w:h=%d:%d, active_w:h=%d:%d\n",
			snr_info.raw_num,
			vdev->snr_info[snr_info.raw_num].color_mode,
			vdev->snr_info[snr_info.raw_num].snr_fmt.frm_num,
			vdev->snr_info[snr_info.raw_num].snr_fmt.img_size[0].width,
			vdev->snr_info[snr_info.raw_num].snr_fmt.img_size[0].height,
			vdev->snr_info[snr_info.raw_num].snr_fmt.img_size[0].active_w,
			vdev->snr_info[snr_info.raw_num].snr_fmt.img_size[0].active_h);

		rc = 0;
		break;
	}

	case VI_IOCTL_MMAP_GRID_SIZE:
	{
		struct cvi_isp_mmap_grid_size m_gd_sz;

		if (copy_from_user(&m_gd_sz, p->ptr, sizeof(struct cvi_isp_mmap_grid_size)) != 0)
			break;

		m_gd_sz.grid_size = ctx->mmap_grid_size[m_gd_sz.raw_num];

		if (copy_to_user(p->ptr, &m_gd_sz, sizeof(struct cvi_isp_mmap_grid_size)) != 0)
			break;

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_PROC_CONTENT:
	{
		struct isp_proc_cfg proc_cfg;
		int rval = 0;

		rval = copy_from_user(&proc_cfg, p->ptr, sizeof(struct isp_proc_cfg));
		if ((rval != 0) || (proc_cfg.buffer_size == 0))
			break;
		isp_proc_setProcContent(proc_cfg.buffer, proc_cfg.buffer_size);

		rc = 0;
		break;
	}

	case VI_IOCTL_SC_ONLINE:
	{
		struct cvi_isp_sc_online sc_online;

		if (copy_from_user(&sc_online, p->ptr, sizeof(struct cvi_isp_sc_online)) != 0)
			break;

		//Currently both sensor are needed to be online or offline at same time.
		ctx->isp_pipe_cfg[sc_online.raw_num].is_offline_scaler = !sc_online.is_sc_online;
		vi_pr(VI_WARN, "raw_num_%d set is_offline_scaler:%d\n",
				  sc_online.raw_num, !sc_online.is_sc_online);
		rc = 0;
		break;
	}

	case VI_IOCTL_AWB_STS_PUT:
	{
		rc = 0;
		break;
	}

	case VI_IOCTL_ENQ_BUF:
	{
		struct vi_buffer    buf;
		struct cvi_isp_buf *qbuf;
		u8 pre_trig = false, post_trig = false;

		if (copy_from_user(&buf, p->ptr, sizeof(buf))) {
			vi_pr(VI_ERR, "VI_IOCTL_ENQ_BUF, copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		qbuf = kzalloc(sizeof(struct cvi_isp_buf), GFP_ATOMIC);
		if (qbuf == NULL) {
			vi_pr(VI_ERR, "QBUF kzalloc size(%zu) fail\n", sizeof(struct cvi_isp_buf));
			rc = -ENOMEM;
			break;
		}

		vdev->chn_id = buf.index;
		memcpy(&qbuf->buf, &buf, sizeof(buf));

		if (_is_all_online(ctx) &&
			cvi_isp_rdy_buf_empty(vdev, ISP_PRERAW_A) &&
			vdev->pre_fe_frm_num[ISP_PRERAW_A][ISP_FE_CH0] > 0) {
			pre_trig = true;
		} else if (_is_fe_be_online(ctx)) { //fe->be->dram->post
			if (cvi_isp_rdy_buf_empty(vdev, vdev->chn_id) &&
				vdev->postraw_frame_number[ISP_PRERAW_A] > 0) {
				vi_pr(VI_DBG, "chn_%d buf empty, trigger post\n", vdev->chn_id);
				post_trig = true;
			}
		}

		cvi_isp_buf_queue(vdev, qbuf);

		if (pre_trig || post_trig)
			tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_DMA_BUF_INFO:
	{
		struct cvi_vi_dma_buf_info info;
		int rval = 0;

		rval = copy_from_user(&info, p->ptr, sizeof(struct cvi_vi_dma_buf_info));
		if ((rval != 0) || (info.size == 0) || (info.paddr == 0))
			break;

		isp_mempool.base = info.paddr;
		isp_mempool.size = info.size;

		vi_pr(VI_INFO, "ISP dma buf paddr(0x%llx) size=0x%x\n",
				isp_mempool.base, isp_mempool.size);

		rc = 0;
		break;
	}

	case VI_IOCTL_START_STREAMING:
	{
		if (vi_start_streaming(vdev)) {
			vi_pr(VI_ERR, "Failed to vi start streaming\n");
			break;
		}

		atomic_set(&vdev->isp_streamon, 1);

		rc = 0;
		break;
	}

	case VI_IOCTL_STOP_STREAMING:
	{
		if (vi_stop_streaming(vdev)) {
			vi_pr(VI_ERR, "Failed to vi stop streaming\n");
			break;
		}

		atomic_set(&vdev->isp_streamon, 0);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SLICE_BUF_EN:
	{
		ctx->is_slice_buf_on = p->value;
		vi_pr(VI_INFO, "ISP_SLICE_BUF_ON(%d)\n", ctx->is_slice_buf_on);
		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

static long _vi_g_ctrl(struct cvi_vi_dev *vdev, struct vi_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct isp_ctx *ctx = &vdev->ctx;

	switch (id) {
	case VI_IOCTL_STS_GET:
	{
		u8 raw_num;
		unsigned long flags;

		raw_num = p->value;

		if (raw_num >= ISP_PRERAW_VIRT_MAX)
			break;

		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		isp_bufpool[raw_num].pre_be_sts_in_use = 1;
		p->value = isp_bufpool[raw_num].pre_be_sts_busy_idx ^ 1;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_GET:
	{
		u8 raw_num;
		unsigned long flags;

		raw_num = p->value;

		if (raw_num >= ISP_PRERAW_VIRT_MAX)
			break;

		spin_lock_irqsave(&isp_bufpool[raw_num].post_sts_lock, flags);
		isp_bufpool[raw_num].post_sts_in_use = 1;
		p->value = isp_bufpool[raw_num].post_sts_busy_idx ^ 1;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].post_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_STS_MEM:
	{
		struct cvi_isp_sts_mem sts_mem;
		int rval = 0;
		u8 raw_num = 0;

		if (copy_from_user(&sts_mem, p->ptr, sizeof(struct cvi_isp_sts_mem)) != 0)
			break;

		raw_num = sts_mem.raw_num;
		if (raw_num >= ISP_PRERAW_VIRT_MAX) {
			vi_pr(VI_ERR, "sts_mem wrong raw_num(%d)\n", raw_num);
			break;
		}

#if 0//PORTING_TEST //test only
		isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr = 0x11223344;
		isp_bufpool[raw_num].sts_mem[0].ae_le.size = 44800;
		isp_bufpool[raw_num].sts_mem[0].af.phy_addr = 0xaabbccdd;
		isp_bufpool[raw_num].sts_mem[0].af.size = 16320;
		isp_bufpool[raw_num].sts_mem[0].awb.phy_addr = 0x12345678;
		isp_bufpool[raw_num].sts_mem[0].awb.size = 71808;
#endif
		rval = copy_to_user(p->ptr,
					isp_bufpool[raw_num].sts_mem,
					sizeof(struct cvi_isp_sts_mem) * 2);

		if (rval)
			vi_pr(VI_ERR, "fail copying %d bytes of ISP_STS_MEM info\n", rval);
		else
			rc = 0;
		break;
	}

	case VI_IOCTL_GET_LSC_PHY_BUF:
	{
		struct cvi_vip_memblock *isp_mem;

		isp_mem = vmalloc(sizeof(struct cvi_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		isp_mem->phy_addr = isp_bufpool[isp_mem->raw_num].lsc;
		isp_mem->size = ispblk_dma_config(ctx, ISP_BLK_ID_DMA_CTL24, isp_mem->raw_num, 0);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_PIPE_DUMP:
	{
		struct cvi_vip_isp_raw_blk dump[2];

		if (copy_from_user(&dump[0], p->ptr, sizeof(struct cvi_vip_isp_raw_blk) * 2) != 0)
			break;

#if 0//PORTING_TEST //test only
		dump[0].raw_dump.phy_addr = 0x11223344;
		if (copy_to_user(p->ptr, &dump[0], sizeof(struct cvi_vip_isp_raw_blk) * 2) != 0)
			break;
		rc = 0;
#else
		rc = isp_raw_dump(vdev, &dump[0]);
		if (copy_to_user(p->ptr, &dump[0], sizeof(struct cvi_vip_isp_raw_blk) * 2) != 0)
			break;
#endif
		break;
	}

	case VI_IOCTL_AWB_STS_GET:
	{
		rc = 0;
		break;
	}

	case VI_IOCTL_GET_FSWDR_PHY_BUF:
	{
		struct cvi_vip_memblock *isp_mem;

		isp_mem = vmalloc(sizeof(struct cvi_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		isp_mem->size = sizeof(struct cvi_vip_isp_fswdr_report);
		if (isp_bufpool[isp_mem->raw_num].fswdr_rpt == NULL) {
			isp_bufpool[isp_mem->raw_num].fswdr_rpt = kmalloc(
				isp_mem->size, GFP_DMA | GFP_KERNEL);
			if (isp_bufpool[isp_mem->raw_num].fswdr_rpt == NULL) {
				vi_pr(VI_ERR, "isp_bufpool[%d].fswdr_rpt alloc size(%d) fail\n",
					isp_mem->raw_num, isp_mem->size);
				vfree(isp_mem);
				break;
			}
		}
		isp_mem->vir_addr = isp_bufpool[isp_mem->raw_num].fswdr_rpt;
		isp_mem->phy_addr = virt_to_phys(isp_bufpool[isp_mem->raw_num].fswdr_rpt);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_SCENE_INFO:
	{
		enum ISP_SCENE_INFO info = FE_ON_BE_OFF_POST_ON_SC;

		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) {
			if (_is_fe_be_online(ctx))
				info = FE_ON_BE_OFF_POST_OFF_SC;
			else if (_is_be_post_online(ctx))
				info = FE_OFF_BE_ON_POST_OFF_SC;
			else if (_is_all_online(ctx))
				info = FE_ON_BE_ON_POST_OFF_SC;
		} else {
			if (_is_fe_be_online(ctx))
				info = FE_ON_BE_OFF_POST_ON_SC;
			else if (_is_be_post_online(ctx))
				info = FE_OFF_BE_ON_POST_ON_SC;
			else if (_is_all_online(ctx))
				info = FE_ON_BE_ON_POST_ON_SC;
		}

		p->value = info;

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_BUF_SIZE:
	{
		u32 tmp_size = 0;
		enum cvi_isp_raw raw_max = ISP_PRERAW_MAX - 1;
		u8 raw_num = 0;

		tmp_size = isp_mempool.size;
		isp_mempool.base = 0xabde2000; //tmp addr only for check aligment
		isp_mempool.size = 0x8000000;
		isp_mempool.byteused = 0;

		_vi_scene_ctrl(vdev, &raw_max);

		for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
			if (!vdev->ctx.isp_pipe_enable[raw_num])
				continue;

			vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en = csi_patgen_en[raw_num];

			if (vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
#ifndef PORTING_TEST
				vdev->usr_fmt.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
				vdev->usr_fmt.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
				vdev->usr_crop.width = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_w;
				vdev->usr_crop.height = vdev->snr_info[raw_num].snr_fmt.img_size[0].active_h;
#else
				vdev->usr_fmt.width = 1920;
				vdev->usr_fmt.height = 1080;
				vdev->usr_crop.width = 1920;
				vdev->usr_crop.height = 1080;
#endif
				vdev->usr_fmt.code = ISP_BAYER_TYPE_BG;
				vdev->usr_crop.left = 0;
				vdev->usr_crop.top = 0;

				vi_pr(VI_WARN, "patgen enable, w_h(%d:%d), color mode(%d)\n",
						vdev->usr_fmt.width, vdev->usr_fmt.height, vdev->usr_fmt.code);
			}

			_vi_ctrl_init(raw_num, vdev);
		}

		_vi_get_dma_buf_size(ctx, raw_max);

		p->value = isp_mempool.byteused;

		isp_mempool.base	= 0;
		isp_mempool.size	= tmp_size;
		isp_mempool.byteused	= 0;

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_TUN_ADDR:
	{
		void *tun_addr = NULL;
		u32 size;

		tun_addr = vi_get_tuning_buf_addr(&size);

		if (copy_to_user(p->ptr, tun_addr, size) != 0)
			break;

		rc = 0;
		break;
	}

	case VI_IOCTL_DQEVENT:
	{
		struct vi_event ev_u = {.type = VI_EVENT_MAX};
		struct vi_event_k *ev_k;
		unsigned long flags;

		spin_lock_irqsave(&event_lock, flags);
#if 0//PORTING_TEST //test only
		struct vi_event_k *ev_test;
		static u32 frm_num, type;

		ev_test = kzalloc(sizeof(*ev_test), GFP_ATOMIC);

		ev_test->ev.dev_id = 0;
		ev_test->ev.type = type++ % (VI_EVENT_MAX - 1);
		ev_test->ev.frame_sequence = frm_num++;
		ev_test->ev.timestamp = ktime_to_timeval(ktime_get());
		list_add_tail(&ev_test->list, &event_q.list);
#endif
		if (!list_empty(&event_q.list)) {
			ev_k = list_first_entry(&event_q.list, struct vi_event_k, list);
			ev_u.dev_id		= ev_k->ev.dev_id;
			ev_u.type		= ev_k->ev.type;
			ev_u.frame_sequence	= ev_k->ev.frame_sequence;
			ev_u.timestamp		= ev_k->ev.timestamp;
			list_del_init(&ev_k->list);
			kfree(ev_k);
		}
		spin_unlock_irqrestore(&event_lock, flags);

		if (copy_to_user(p->ptr, &ev_u, sizeof(struct vi_event))) {
			vi_pr(VI_ERR, "Failed to dqevent\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_CLUT_TBL_IDX:
	{
		p->value = vi_tuning_get_clut_tbl_idx();

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_IP_INFO:
	{
		if (copy_to_user(p->ptr, &ip_info_list, sizeof(struct ip_info) * IP_INFO_ID_MAX) != 0) {
			vi_pr(VI_ERR, "Failed to copy ip_info_list\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_RGBMAP_LE_PHY_BUF:
	{
		struct cvi_vip_memblock *isp_mem;

		isp_mem = vmalloc(sizeof(struct cvi_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		isp_mem->phy_addr = isp_bufpool[isp_mem->raw_num].rgbmap_le[0];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, ISP_BLK_ID_DMA_CTL10, isp_mem->raw_num);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_RGBMAP_SE_PHY_BUF:
	{
		struct cvi_vip_memblock *isp_mem;

		isp_mem = vmalloc(sizeof(struct cvi_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		isp_mem->phy_addr = isp_bufpool[isp_mem->raw_num].rgbmap_se[0];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, ISP_BLK_ID_DMA_CTL11, isp_mem->raw_num);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct cvi_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

long vi_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct cvi_vi_dev *vdev = file->private_data;
	long ret = 0;
	struct vi_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vi_ext_control)))
		return -EINVAL;

	switch (cmd) {
	case VI_IOC_S_CTRL:
		ret = _vi_s_ctrl(vdev, &p);
		break;
	case VI_IOC_G_CTRL:
		ret = _vi_g_ctrl(vdev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct vi_ext_control)))
		return -EINVAL;

	return ret;
}

int vi_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct cvi_vi_dev *vdev;

	vdev = container_of(inode->i_cdev, struct cvi_vi_dev, cdev);
	file->private_data = vdev;

	if (!atomic_read(&dev_open_cnt)) {
#ifndef FPGA_PORTING
		_vi_clk_ctrl(vdev, true);
#endif
		vi_init();

		_vi_sw_init(vdev);

		vi_pr(VI_INFO, "-\n");
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

int vi_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	atomic_dec(&dev_open_cnt);

	if (!atomic_read(&dev_open_cnt)) {
		struct cvi_vi_dev *vdev;

		vdev = container_of(inode->i_cdev, struct cvi_vi_dev, cdev);

		_vi_sdk_release(vdev);

		_vi_release_op(vdev);

		vi_pr(VI_INFO, "-\n");
	}

	return ret;
}

int vi_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_vi_dev *vdev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos;

	vdev = file->private_data;
	pos = vdev->shared_mem;

	if ((vm_size + offset) > VI_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
#if defined(__LP64__)
		vi_pr(VI_DBG, "vi proc mmap vir(%p) phys(%#lx)\n", pos, virt_to_phys((void *) pos));
#else
		vi_pr(VI_DBG, "vi proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
#endif
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

unsigned int vi_poll(struct file *file, struct poll_table_struct *wait)
{
	struct cvi_vi_dev *vdev = file->private_data;
	unsigned long req_events = poll_requested_events(wait);
	unsigned int res = 0;
	unsigned long flags;

	if (req_events & POLLPRI) {
		/*
		 * If event buf is not empty, then notify MW to DQ event.
		 * Otherwise poll_wait.
		 */
		spin_lock_irqsave(&event_lock, flags);
		if (!list_empty(&event_q.list))
			res = POLLPRI;
		else
			poll_wait(file, &vdev->isp_event_wait_q, wait);
		spin_unlock_irqrestore(&event_lock, flags);
	}

	if (req_events & POLLIN) {
		if (atomic_read(&vdev->isp_dbg_flag)) {
			res = POLLIN | POLLRDNORM;
			atomic_set(&vdev->isp_dbg_flag, 0);
		} else {
			poll_wait(file, &vdev->isp_dbg_wait_q, wait);
		}
	}

	return res;
}

int vi_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)dev;
	struct isp_ctx *ctx = &vdev->ctx;
	int rc = -1;

	switch (cmd) {
	case VI_CB_QBUF_TRIGGER:
		vi_pr(VI_INFO, "isp_ol_sc_trig_post\n");

		tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	case VI_CB_SC_FRM_DONE:
		vi_pr(VI_DBG, "sc frm done cb\n");

		atomic_set(&vdev->ol_sc_frm_done, 1);
		tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	case VI_CB_SET_VIVPSSMODE:
	{
		VI_VPSS_MODE_S stVIVPSSMode;
		CVI_U8   dev_num = 0;
		CVI_BOOL vi_online = CVI_FALSE;

		memcpy(&stVIVPSSMode, arg, sizeof(VI_VPSS_MODE_S));

		vi_online = (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_ONLINE) ||
			    (stVIVPSSMode.aenMode[0] == VI_ONLINE_VPSS_OFFLINE);

		if (vi_online) {
			ctx->is_offline_postraw = ctx->is_offline_be = !vi_online;

			vi_pr(VI_DBG, "Caller_Mod(%d) set vi_online:%d, is_offline_postraw=%d, is_offline_be=%d\n",
					caller, vi_online, ctx->is_offline_postraw, ctx->is_offline_be);
		}

		for (dev_num = 0; dev_num < VI_MAX_DEV_NUM; dev_num++) {
			CVI_BOOL is_vpss_online = (stVIVPSSMode.aenMode[dev_num] == VI_ONLINE_VPSS_ONLINE) ||
						  (stVIVPSSMode.aenMode[dev_num] == VI_OFFLINE_VPSS_ONLINE);

			ctx->isp_pipe_cfg[dev_num].is_offline_scaler = !is_vpss_online;
			vi_pr(VI_DBG, "raw_num_%d set is_offline_scaler:%d\n",
					dev_num, !is_vpss_online);
		}

		rc = 0;
		break;
	}
	case VI_CB_RESET_ISP:
	{
		enum cvi_isp_raw raw_num = *(enum cvi_isp_raw *)arg;

		atomic_set(&vdev->isp_err_handle_flag, 1);

		if (_is_be_post_online(ctx))
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);

		atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
		vi_err_wake_up_th(vdev, raw_num);
		break;
	}
	case VI_CB_GDC_OP_DONE:
	{
		struct dwa_op_done_cfg *cfg =
			(struct dwa_op_done_cfg *)arg;

		vi_gdc_callback(cfg->pParam, cfg->blk);
		rc = 0;
		break;
	}
	default:
		break;
	}

	return rc;
}

/********************************************************************************
 *  VI event handler related
 *******************************************************************************/
void vi_destory_dbg_thread(struct cvi_vi_dev *vdev)
{
	atomic_set(&vdev->isp_dbg_flag, 1);
	wake_up(&vdev->isp_dbg_wait_q);
}

static void _vi_timeout_chk(struct cvi_vi_dev *vdev)
{
	if (++gViCtx->timeout_cnt >= 2) {
		atomic_set(&vdev->isp_dbg_flag, 1);
		wake_up(&vdev->isp_dbg_wait_q);
		gViCtx->timeout_cnt = 0;
	}
}
#ifdef VI_PROFILE
static void _vi_update_chnRealFrameRate(VI_CHN_STATUS_S *pstViChnStatus)
{
	CVI_U64 duration, curTimeUs;
	struct timespec64 curTime;

	curTime = ktime_to_timespec64(ktime_get());
	curTimeUs = curTime.tv_sec * 1000000L + curTime.tv_nsec / 1000L;
	duration = curTimeUs - pstViChnStatus->u64PrevTime;

	if (duration >= 1000000) {
		pstViChnStatus->u32FrameRate = pstViChnStatus->u32FrameNum;
		pstViChnStatus->u32FrameNum = 0;
		pstViChnStatus->u64PrevTime = curTimeUs;
	}

	vi_pr(VI_DBG, "FrameRate=%d\n", pstViChnStatus->u32FrameRate);
}
#endif
static int _vi_event_handler_thread(void * arg)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)arg;
	u32 timeout = 500;//ms
	int ret = 0, flag = 0, ret2 = CVI_SUCCESS;
	enum E_VI_TH th_id = E_VI_TH_EVENT_HANDLER;
	MMF_CHN_S chn = {.enModId = CVI_ID_VI, .s32DevId = 0, .s32ChnId = 0};
#ifdef VI_PROFILE
	struct timespec64 time[2];
	CVI_U32 sum = 0, duration, duration_max = 0, duration_min = 1000 * 1000;
	CVI_U8 count = 0;
#endif

	while (1) {
#ifdef VI_PROFILE
		_vi_update_chnRealFrameRate(&gViCtx->chnStatus[chn.s32ChnId]);
		time[0] = ktime_to_timespec64(ktime_get());
#endif
		ret = wait_event_timeout(vdev->vi_th[th_id].wq,
					vdev->vi_th[th_id].flag != 0 || kthread_should_stop(),
					msecs_to_jiffies(timeout) - 1);
		flag = vdev->vi_th[th_id].flag;
		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		if (!ret) {
			vi_pr(VI_ERR, "vi_event_handler timeout(%d)ms\n", timeout);
			_vi_timeout_chk(vdev);
			continue;
		} else {
			struct _vi_buffer b;
			VB_BLK blk = 0;
			struct cvi_gdc_mesh *pmesh = NULL;
			struct vb_s *vb = NULL;

			//DQbuf from list.
			if (vi_dqbuf(&b) == CVI_FAILURE) {
				vi_pr(VI_WARN, "illegal wakeup raw_num[%d]\n", flag);
				continue;
			}
			chn.s32ChnId = b.chnId;
			ret2 = vb_dqbuf(chn, CHN_TYPE_OUT, &blk);
			if (ret2 != CVI_SUCCESS) {
				if (blk == VB_INVALID_HANDLE)
					vi_pr(VI_ERR, "chn(%d) can't get vb-blk.\n", chn.s32ChnId);
				continue;
			}

			((struct vb_s *)blk)->buf.dev_num = b.chnId;
			((struct vb_s *)blk)->buf.frm_num = b.sequence;
			((struct vb_s *)blk)->buf.u64PTS =
					(CVI_U64)b.timestamp.tv_sec * 1000000 + b.timestamp.tv_nsec / 1000; //microsec

			gViCtx->chnStatus[chn.s32ChnId].u32IntCnt++;
			gViCtx->chnStatus[chn.s32ChnId].u32FrameNum++;
			gViCtx->chnStatus[chn.s32ChnId].u32RecvPic = b.sequence;

			vi_pr(VI_DBG, "dqbuf chn_id=%d, frm_num=%d\n", b.chnId, b.sequence);

			if (gViCtx->bypass_frm[chn.s32ChnId] >= b.sequence) {
				//Release buffer if bypass_frm is not zero
				vb_release_block(blk);
				goto QBUF;
			}

			if (!gViCtx->pipeAttr[chn.s32ChnId].bYuvBypassPath) {
				vi_fill_mlv_info((struct vb_s *)blk, 0, NULL, true);
				vi_fill_dis_info((struct vb_s *)blk);
			}

			// TODO: extchn only support works on original frame without GDC effect.
			//_vi_handle_extchn(chn.s32ChnId, chn, blk, &bFisheyeOn);
			//if (bFisheyeOn)
				//goto VB_DONE;

			pmesh = &g_vi_mesh[chn.s32ChnId];
			vb = (struct vb_s *)blk;

			if (mutex_trylock(&pmesh->lock)) {
				if (gViCtx->stLDCAttr[chn.s32ChnId].bEnable) {
					struct _vi_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

					if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC, &gViCtx->stLDCAttr[chn.s32ChnId].stAttr
						, vb, gViCtx->chnAttr[chn.s32ChnId].enPixelFormat, pmesh->paddr
						, CVI_FALSE, &cb_param
						, sizeof(cb_param), CVI_ID_VI
						, gViCtx->enRotation[chn.s32ChnId]) != CVI_SUCCESS) {
						mutex_unlock(&pmesh->lock);
						vi_pr(VI_ERR, "gdc LDC failed.\n");
					}
					goto QBUF;
				} else if (gViCtx->enRotation[chn.s32ChnId] != ROTATION_0) {
					struct _vi_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_ROTATION};

					if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION, NULL
						, vb, gViCtx->chnAttr[chn.s32ChnId].enPixelFormat, pmesh->paddr
						, CVI_FALSE, &cb_param
						, sizeof(cb_param), CVI_ID_VI
						, gViCtx->enRotation[chn.s32ChnId]) != CVI_SUCCESS) {
						mutex_unlock(&pmesh->lock);
						vi_pr(VI_ERR, "gdc rotation failed.\n");
					}
					goto QBUF;
				}
				mutex_unlock(&pmesh->lock);
			} else {
				vi_pr(VI_WARN, "chn(%d) drop frame due to gdc op blocked.\n",
					     chn.s32ChnId);
				// release blk if gdc not done yet
				vb_release_block(blk);
				goto QBUF;
			}
// VB_DONE:
			vb_done_handler(chn, CHN_TYPE_OUT, blk);
QBUF:
			// get another vb for next frame
			if (vi_sdk_qbuf(chn) != CVI_SUCCESS)
				vb_acquire_block(vi_sdk_qbuf, chn, gViCtx->blk_size[chn.s32ChnId],
							gViCtx->chnAttr[chn.s32ChnId].u32BindVbPool);
		}
#ifdef VI_PROFILE
		time[1] = ktime_to_timespec64(ktime_get());
		duration = get_diff_in_us(time[0], time[1]);
		duration_max = MAX(duration, duration_max);
		duration_min = MIN(duration, duration_min);
		sum += duration;
		if (++count == 100) {
			vi_pr(VI_INFO, "VI duration(ms): average(%d), max(%d) min(%d)\n"
				, sum / count / 1000, duration_max / 1000, duration_min / 1000);
			count = 0;
			sum = duration_max = 0;
			duration_min = 1000 * 1000;
		}
#endif
	}

	return 0;
}

/*******************************************************
 *  Irq handlers
 ******************************************************/

static void _vi_record_debug_info(struct isp_ctx *ctx)
{
	uint8_t i = 0;
	uintptr_t isptop = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t rdma28 = ctx->phys_regs[ISP_BLK_ID_DMA_CTL28];
	struct cvi_vi_info *vi_info = NULL;

	if (gOverflowInfo != NULL)
		return;

	gOverflowInfo = kzalloc(sizeof(struct cvi_overflow_info), GFP_ATOMIC);
	if (gOverflowInfo == NULL) {
		vi_pr(VI_ERR, "gOverflowInfo kzalloc size(%zu) fail\n", sizeof(struct cvi_overflow_info));
		return;
	}

	vi_info = &gOverflowInfo->vi_info;

	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on &&
	    !ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) { //VPSS online
		gOverflowInfo->vpss_info.dev_num = ISP_PRERAW_A;
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_OVERFLOW_CHECK, gOverflowInfo) != 0) {
			vi_pr(VI_ERR, "VPSS_CB_OVERFLOW_CHECK is failed\n");
		}
	}

	//isp_top
	vi_info->isp_top.blk_idle = ISP_RD_REG(isptop, REG_ISP_TOP_T, BLK_IDLE);
	for (i = 0; i <= 6; i++) {
		//Debug
		ISP_WR_BITS(isptop, REG_ISP_TOP_T, DUMMY, DBUS_SEL, i);
		vi_info->isp_top.dbus_sel[i].r_0 = ISP_RD_REG(isptop, REG_ISP_TOP_T, DBUS0); //0x0A070040
		vi_info->isp_top.dbus_sel[i].r_4 = ISP_RD_REG(isptop, REG_ISP_TOP_T, DBUS1); //0x0A070044
		vi_info->isp_top.dbus_sel[i].r_8 = ISP_RD_REG(isptop, REG_ISP_TOP_T, DBUS2); //0x0A070048
		vi_info->isp_top.dbus_sel[i].r_c = ISP_RD_REG(isptop, REG_ISP_TOP_T, DBUS3); //0x0A07004C
	}

	//pre_raw_fe
	vi_info->preraw_fe.preraw_info = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_INFO);
	vi_info->preraw_fe.fe_idle_info = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, FE_IDLE_INFO);

	//pre_raw_be
	vi_info->preraw_be.preraw_be_info = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_INFO);
	vi_info->preraw_be.be_dma_idle_info = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_DMA_IDLE_INFO);
	vi_info->preraw_be.ip_idle_info = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_IP_IDLE_INFO);
	vi_info->preraw_be.stvalid_status = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TVALID_STATUS);
	vi_info->preraw_be.stready_status = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TREADY_STATUS);

	//rawtop
	vi_info->rawtop.stvalid_status = ISP_RD_REG(rawtop, REG_RAW_TOP_T, STVALID_STATUS);
	vi_info->rawtop.stready_status = ISP_RD_REG(rawtop, REG_RAW_TOP_T, STREADY_STATUS);
	vi_info->rawtop.dma_idle = ISP_RD_REG(rawtop, REG_RAW_TOP_T, DMA_IDLE);

#if 0
	ISP_WR_BITS(isptop, REG_RAW_TOP_T, DEBUG_SELECT, RAW_TOP_DEBUG_SELECT, 0);
	vi_pr(VI_INFO, "RAW_TOP, debug_select(h2c)=0x0, debug(h28)=0x%x\n",
		ISP_RD_REG(rawtop, REG_RAW_TOP_T, DEBUG));

	ISP_WR_BITS(isptop, REG_RAW_TOP_T, DEBUG_SELECT, RAW_TOP_DEBUG_SELECT, 4);
	vi_pr(VI_INFO, "RAW_TOP, debug_select(h2c)=0x4, debug(h28)=0x%x\n",
		ISP_RD_REG(rawtop, REG_RAW_TOP_T, DEBUG));
#endif

	//rgbtop
	vi_info->rgbtop.ip_stvalid_status = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, DBG_IP_S_VLD);
	vi_info->rgbtop.ip_stready_status = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, DBG_IP_S_RDY);
	vi_info->rgbtop.dmi_stvalid_status = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, DBG_DMI_VLD);
	vi_info->rgbtop.dmi_stready_status = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, DBG_DMI_RDY);
	vi_info->rgbtop.xcnt_rpt = ISP_RD_BITS(rgbtop, REG_ISP_RGB_TOP_T, PATGEN4, XCNT_RPT);
	vi_info->rgbtop.ycnt_rpt = ISP_RD_BITS(rgbtop, REG_ISP_RGB_TOP_T, PATGEN4, YCNT_RPT);

	//yuvtop
	vi_info->yuvtop.debug_state = ISP_RD_REG(yuvtop, REG_YUV_TOP_T, YUV_DEBUG_STATE);
	vi_info->yuvtop.stvalid_status = ISP_RD_REG(yuvtop, REG_YUV_TOP_T, STVALID_STATUS);
	vi_info->yuvtop.stready_status = ISP_RD_REG(yuvtop, REG_YUV_TOP_T, STREADY_STATUS);
	vi_info->yuvtop.xcnt_rpt = ISP_RD_BITS(yuvtop, REG_YUV_TOP_T, PATGEN4, XCNT_RPT);
	vi_info->yuvtop.ycnt_rpt = ISP_RD_BITS(yuvtop, REG_YUV_TOP_T, PATGEN4, YCNT_RPT);

	//rdma28
	ISP_WR_BITS(rdma28, REG_ISP_DMA_CTL_T, SYS_CONTROL, DBG_SEL, 0x1);
	vi_info->rdma28[0].dbg_sel = 0x1;
	vi_info->rdma28[0].status = ISP_RD_REG(rdma28, REG_ISP_DMA_CTL_T, DMA_STATUS);
	ISP_WR_BITS(rdma28, REG_ISP_DMA_CTL_T, SYS_CONTROL, DBG_SEL, 0x2);
	vi_info->rdma28[1].dbg_sel = 0x2;
	vi_info->rdma28[1].status = ISP_RD_REG(rdma28, REG_ISP_DMA_CTL_T, DMA_STATUS);
	vi_info->enable = true;
}

static void _vi_show_debug_info(void)
{
	struct cvi_vi_info *vi_info = NULL;
	struct cvi_vpss_info *vpss_info = NULL;
	struct cvi_vc_info *vc_info = NULL;
	uint8_t i = 0;

	if (gOverflowInfo == NULL)
		return;

	vi_info = &gOverflowInfo->vi_info;
	vpss_info = &gOverflowInfo->vpss_info;
	vc_info = &gOverflowInfo->vc_info;

	if (vc_info->enable) {
		vc_info->enable = false;
		pr_info("vc reg_00=0x%x, reg_08=0x%x, reg_88=0x%x, reg_90=0x%x, reg_94=0x%x\n",
			vc_info->reg_00, vc_info->reg_08, vc_info->reg_88, vc_info->reg_90, vc_info->reg_94);
	}

	if (vpss_info->enable) {
		vpss_info->enable = false;
		pr_info("sc_top         valid     ready\n");
		pr_info("isp2ip_y in       %d        %d\n",
			vpss_info->sc_top.isp2ip_y_in[0], vpss_info->sc_top.isp2ip_y_in[1]);
		pr_info("isp2ip_u in       %d        %d\n",
			vpss_info->sc_top.isp2ip_u_in[0], vpss_info->sc_top.isp2ip_u_in[1]);
		pr_info("isp2ip_v in       %d        %d\n",
			vpss_info->sc_top.isp2ip_v_in[0], vpss_info->sc_top.isp2ip_v_in[1]);
		pr_info("img_d out         %d        %d\n",
			vpss_info->sc_top.img_d_out[0], vpss_info->sc_top.img_d_out[1]);
		pr_info("img_v out         %d        %d\n",
			vpss_info->sc_top.img_d_out[0], vpss_info->sc_top.img_d_out[1]);
		pr_info("bld_sa(img_v)     %d        %d\n",
			vpss_info->sc_top.bld_sa[0], vpss_info->sc_top.bld_sa[1]);
		pr_info("bld_sb(img_d)     %d        %d\n",
			vpss_info->sc_top.bld_sb[0], vpss_info->sc_top.bld_sb[1]);
		pr_info("bld_m             %d        %d\n",
			vpss_info->sc_top.bld_m[0], vpss_info->sc_top.bld_m[1]);
		pr_info("pri_sp            %d        %d\n",
			vpss_info->sc_top.pri_sp[0], vpss_info->sc_top.pri_sp[1]);
		pr_info("pri_m             %d        %d\n",
			vpss_info->sc_top.pri_m[0], vpss_info->sc_top.pri_m[1]);
		pr_info("sc_d              %d        %d\n",
			vpss_info->sc_top.sc_d[0], vpss_info->sc_top.sc_d[1]);
		pr_info("sc_v1             %d        %d\n",
			vpss_info->sc_top.sc_v1[0], vpss_info->sc_top.sc_v1[1]);
		pr_info("sc_v2             %d        %d\n",
			vpss_info->sc_top.sc_v2[0], vpss_info->sc_top.sc_v2[1]);
		pr_info("sc_v3             %d        %d\n",
			vpss_info->sc_top.sc_v3[0], vpss_info->sc_top.sc_v3[1]);
		pr_info("sc_d_out          %d        %d\n",
			vpss_info->sc_top.sc_d_out[0], vpss_info->sc_top.sc_d_out[1]);

		pr_info("sc(%d) odma\n", vpss_info->sc);
		pr_info("    sc_odma_axi_cmd_cs [0]=%d, [1]=%d, [2]=%d, [3]=%d\n",
			vpss_info->odma.sc_odma_axi_cmd_cs[0], vpss_info->odma.sc_odma_axi_cmd_cs[1],
			vpss_info->odma.sc_odma_axi_cmd_cs[2], vpss_info->odma.sc_odma_axi_cmd_cs[3]);
		pr_info("    sc_odma_v_buf_empty=%d, sc_odma_v_buf_full=%d\n",
			vpss_info->odma.sc_odma_v_buf_empty, vpss_info->odma.sc_odma_v_buf_full);
		pr_info("    sc_odma_u_buf_empty=%d, sc_odma_u_buf_full=%d\n",
			vpss_info->odma.sc_odma_u_buf_empty, vpss_info->odma.sc_odma_u_buf_full);
		pr_info("    sc_odma_y_buf_empty=%d, sc_odma_y_buf_full=%d\n",
			vpss_info->odma.sc_odma_y_buf_empty, vpss_info->odma.sc_odma_y_buf_full);
		pr_info("    sc_odma_axi_v_active=%d, sc_odma_axi_u_active=%d\n",
			vpss_info->odma.sc_odma_axi_v_active, vpss_info->odma.sc_odma_axi_u_active);
		pr_info("    sc_odma_axi_y_active=%d, sc_odma_axi_active=%d\n",
			vpss_info->odma.sc_odma_axi_y_active, vpss_info->odma.sc_odma_axi_active);
		pr_info("    reg_v_sb_empty=%d, reg_v_sb_full=%d\n",
			vpss_info->odma.reg_v_sb_empty, vpss_info->odma.reg_v_sb_full);
		pr_info("    reg_u_sb_empty=%d, reg_u_sb_full=%d\n",
			vpss_info->odma.reg_u_sb_empty, vpss_info->odma.reg_u_sb_full);
		pr_info("    reg_y_sb_empty=%d, reg_y_sb_full=%d\n",
			vpss_info->odma.reg_y_sb_empty, vpss_info->odma.reg_y_sb_full);
		pr_info("    reg_sb_full=%d\n",
			vpss_info->odma.reg_sb_full);
		pr_info("    sb_mode=%d, sb_size=%d, sb_nb=%d, sb_full_nb=%d, sb_sw_wptr=%d\n",
			vpss_info->sb_ctrl.sb_mode,
			vpss_info->sb_ctrl.sb_size,
			vpss_info->sb_ctrl.sb_nb,
			vpss_info->sb_ctrl.sb_full_nb,
			vpss_info->sb_ctrl.sb_sw_wptr);
		pr_info("    u_sb_wptr_ro=%d, u_sb_full=%d, u_sb_empty=%d, u_sb_dptr_ro=%d\n",
			vpss_info->sb_stat.u_sb_wptr_ro,
			vpss_info->sb_stat.u_sb_full,
			vpss_info->sb_stat.u_sb_empty,
			vpss_info->sb_stat.u_sb_dptr_ro);
		pr_info("    v_sb_wptr_ro=%d, v_sb_full=%d, v_sb_empty=%d, v_sb_dptr_ro=%d\n",
			vpss_info->sb_stat.v_sb_wptr_ro,
			vpss_info->sb_stat.v_sb_full,
			vpss_info->sb_stat.v_sb_empty,
			vpss_info->sb_stat.v_sb_dptr_ro);
		pr_info("    y_sb_wptr_ro=%d, y_sb_full=%d, y_sb_empty=%d, y_sb_dptr_ro=%d, sb_full=%d\n",
			vpss_info->sb_stat.y_sb_wptr_ro,
			vpss_info->sb_stat.y_sb_full,
			vpss_info->sb_stat.y_sb_empty,
			vpss_info->sb_stat.y_sb_dptr_ro,
			vpss_info->sb_stat.sb_full);
		pr_info("    latched_line_cnt=%d\n", vpss_info->latched_line_cnt);
	}

	if (vi_info->enable) {
		vi_info->enable = false;
		pr_info("ISP_TOP, blk_idle(h38)=0x%x\n", vi_info->isp_top.blk_idle);
		for (i = 0; i <= 6; i++) {
			pr_info("dbus_sel=%d, r_0=0x%x, r_4=0x%x, r_8=0x%x, r_c=0x%x\n",
				i,
				vi_info->isp_top.dbus_sel[i].r_0,
				vi_info->isp_top.dbus_sel[i].r_4,
				vi_info->isp_top.dbus_sel[i].r_8,
				vi_info->isp_top.dbus_sel[i].r_c);
		}
		pr_info("PRE_RAW_FE0, preraw_info(h34)=0x%x, fe_idle_info(h50)=0x%x\n",
			vi_info->preraw_fe.preraw_info,
			vi_info->preraw_fe.fe_idle_info);
		pr_info("PRE_RAW_BE, preraw_be_info(h14)=0x%x, be_dma_idle_info(h18)=0x%x, ip_idle_info(h1c)=0x%x\n",
			vi_info->preraw_be.preraw_be_info,
			vi_info->preraw_be.be_dma_idle_info,
			vi_info->preraw_be.ip_idle_info);
		pr_info("PRE_RAW_BE, stvalid_status(h28)=0x%x, stready_status(h2c)=0x%x\n",
			vi_info->preraw_be.stvalid_status,
			vi_info->preraw_be.stready_status);
		pr_info("RAW_TOP, stvalid_status(h40)=0x%x, stready_status(h44)=0x%x, dma_idle(h60)=0x%x\n",
			vi_info->rawtop.stvalid_status,
			vi_info->rawtop.stready_status,
			vi_info->rawtop.dma_idle);
		pr_info("RGB_TOP, ip_stvalid_status(h50)=0x%x, ip_stready_status(h54)=0x%x\n",
			vi_info->rgbtop.ip_stvalid_status,
			vi_info->rgbtop.ip_stready_status);
		pr_info("RGB_TOP, dmi_stvalid_status(h58)=0x%x, dmi_stready_status(h5c)=0x%x\n",
			vi_info->rgbtop.dmi_stvalid_status,
			vi_info->rgbtop.dmi_stready_status);
		pr_info("RGB_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->rgbtop.xcnt_rpt,
			vi_info->rgbtop.ycnt_rpt);
		pr_info("YUV_TOP debug_state(h18)=0x%x, stvalid_status(h6c)=0x%x, stready_status(h70)=0x%x\n",
			vi_info->yuvtop.debug_state,
			vi_info->yuvtop.stvalid_status,
			vi_info->yuvtop.stready_status);
		pr_info("YUV_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->yuvtop.xcnt_rpt,
			vi_info->yuvtop.ycnt_rpt);
		pr_info("rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[0].dbg_sel,
			vi_info->rdma28[0].status);
		pr_info("rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[1].dbg_sel,
			vi_info->rdma28[1].status);
	}

	kfree(gOverflowInfo);
	gOverflowInfo = NULL;
}

static void _vi_err_retrig_preraw(struct cvi_vi_dev *vdev, const enum cvi_isp_raw err_raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 wait_fe = ISP_PRERAW_A;
	enum cvi_isp_pre_chn_num fe_max, fe_chn;

	for (wait_fe = ISP_PRERAW_A; wait_fe < ISP_PRERAW_MAX; wait_fe++) {
		if (vdev->isp_err_times[wait_fe]++ > 30) {
			vi_pr(VI_ERR, "raw_%d too much errors happened\n", wait_fe);
			continue;
		}

		vi_pr(VI_WARN, "fe_%d isp_pre_trig retry %d times\n", wait_fe, vdev->isp_err_times[wait_fe]);

		// yuv sensor offline to sc
		if (ctx->isp_pipe_cfg[wait_fe].is_yuv_bypass_path &&
			ctx->isp_pipe_cfg[wait_fe].is_offline_scaler) {
			fe_max = ctx->isp_pipe_cfg[wait_fe].muxMode + 1;
			for (fe_chn = ISP_FE_CH0; fe_chn < fe_max; fe_chn++) {
				u8 buf_chn = (wait_fe == ISP_PRERAW_A) ? fe_chn : vdev->ctx.rawb_chnstr_num + fe_chn;

				if (wait_fe == err_raw_num) //if yuv sensor err need push, that's right qbuf num.
					vdev->qbuf_num[buf_chn]++;

				if (cvi_isp_rdy_buf_empty(vdev, buf_chn)) {
					vi_pr(VI_INFO, "fe_%d chn_%d yuv bypass outbuf is empty\n", wait_fe, buf_chn);
					continue;
				}

				_isp_yuv_bypass_trigger(vdev, wait_fe, fe_chn);
			}
		} else { //rgb sensor
			wait_fe = (ctx->isp_pipe_cfg[wait_fe].is_mux
					&& (vdev->pre_fe_frm_num[wait_fe][ISP_FE_CH0] !=
						vdev->pre_fe_frm_num[wait_fe + ISP_PRERAW_MAX][ISP_FE_CH0]))
					? wait_fe + ISP_PRERAW_MAX
					: wait_fe;
			_pre_hw_enque(vdev, wait_fe, ISP_FE_CH0);
			if (ctx->isp_pipe_cfg[wait_fe].is_hdr_on) {
				_pre_hw_enque(vdev, wait_fe, ISP_FE_CH1);
			}
		}
	}
}

void _vi_err_handler(struct cvi_vi_dev *vdev, const enum cvi_isp_raw err_raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	u8 wait_fe = ISP_PRERAW_A, count = 10;

	//Stop pre/postraw trigger go
	atomic_set(&vdev->isp_err_handle_flag, 1);

	//step 1 : set frm vld = 0
	isp_frm_err_handler(ctx, err_raw_num, 1);

	atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH0], ISP_PRERAW_IDLE);
	atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH1], ISP_PRERAW_IDLE);

	//make sure disable mono for isp reset
	if (ISP_RD_BITS(tnr, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE) == 1) {
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE, 0);
	}

	//step 2 : wait to make sure post and the other fe is done.
	while (--count > 0) {
		if (_is_be_post_online(ctx)) {
			if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH1]) == ISP_PRERAW_IDLE &&
				atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_PRE_BE_IDLE &&
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_PRE_BE_IDLE)
				break;
			vi_pr(VI_WARN, "wait fe, be and post idle count(%d) for be_post online\n", count);
		} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
			if (ctx->is_multi_sensor) {
				if (err_raw_num == ISP_PRERAW_A) {
					wait_fe = ISP_PRERAW_B;

					if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
					atomic_read(&vdev->pre_fe_state[wait_fe][ISP_FE_CH0]) == ISP_PRERAW_IDLE &&
					atomic_read(&vdev->pre_fe_state[wait_fe][ISP_FE_CH1]) == ISP_PRERAW_IDLE)
						break;
					vi_pr(VI_WARN, "wait fe_%d and post idle count(%d) for fe_be online dual\n",
							wait_fe, count);
				} else {
					if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE &&
						atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_PRE_BE_IDLE &&
						atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_PRE_BE_IDLE)
						break;
					vi_pr(VI_WARN, "wait be and post idle count(%d) for fe_be online dual\n",
							count);
				}
			} else {
				if (atomic_read(&vdev->postraw_state) == ISP_POSTRAW_IDLE)
					break;
				vi_pr(VI_WARN, "wait post idle(%d) count(%d) for fe_be online single\n",
						atomic_read(&vdev->postraw_state), count);
			}
		} else {
			break;
		}

		usleep_range(5 * 1000, 10 * 1000);
	}

	//If fe/be/post not done;
	if (count == 0) {
		if (ctx->is_multi_sensor) {
			vi_pr(VI_ERR, "isp status fe_0(ch0:%d, ch1:%d) fe_1(ch0:%d, ch1:%d) fe_2(ch0:%d, ch1:%d)\n",
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH0]),
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_A][ISP_FE_CH1]),
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH0]),
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_B][ISP_FE_CH1]),
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH0]),
					atomic_read(&vdev->pre_fe_state[ISP_PRERAW_C][ISP_FE_CH1]));
			vi_pr(VI_ERR, "isp status be(ch0:%d, ch1:%d) postraw(%d)\n",
					atomic_read(&vdev->pre_be_state[ISP_BE_CH0]),
					atomic_read(&vdev->pre_be_state[ISP_BE_CH1]),
					atomic_read(&vdev->postraw_state));
		} else {
			vi_pr(VI_ERR, "isp status post(%d)\n", atomic_read(&vdev->postraw_state));
		}
		return;
	}

	//step 3 : set csibdg sw abort and wait abort done
	if (isp_frm_err_handler(ctx, err_raw_num, 3) < 0)
		return;

	//step 4 : isp sw reset and vip reset pull up
	isp_frm_err_handler(ctx, err_raw_num, 4);

	//send err cb to vpss if vpss online
	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on &&
		!ctx->isp_pipe_cfg[err_raw_num].is_offline_scaler) { //VPSS online
		struct sc_err_handle_cb err_cb = {0};

		/* VPSS Online error handle */
		err_cb.snr_num = err_raw_num;
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_ONLINE_ERR_HANDLE, &err_cb) != 0) {
			vi_pr(VI_ERR, "VPSS_CB_ONLINE_ERR_HANDLE is failed\n");
		}
	}

	//step 5 : isp sw reset and vip reset pull down
	isp_frm_err_handler(ctx, err_raw_num, 5);

	//step 6 : wait ISP idle
	if (isp_frm_err_handler(ctx, err_raw_num, 6) < 0)
		return;

	//step 7 : reset sw state to idle
	if (_is_be_post_online(ctx)) {
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH0], ISP_PRERAW_IDLE);
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH1], ISP_PRERAW_IDLE);
	} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
		if (ctx->is_multi_sensor) {
			if (err_raw_num == ISP_PRERAW_A) {
				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
				atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_PRE_BE_IDLE);
			} else {
				atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH0], ISP_PRERAW_IDLE);
				atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH1], ISP_PRERAW_IDLE);
			}
		} else {
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
			atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_PRE_BE_IDLE);
		}
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //slice buffer on
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH0], ISP_PRERAW_IDLE);
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH1], ISP_PRERAW_IDLE);
		atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);
		atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_PRE_BE_IDLE);
		atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
	}

	//step 8 : set fbcd dma to hw mode if fbc is on
	if (ctx->is_fbc_on) {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL41, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL42, false);
	}

	//step 9 : reset first frame count
	vdev->ctx.isp_pipe_cfg[err_raw_num].first_frm_cnt = 0;
	if (ctx->is_multi_sensor)
		vdev->ctx.isp_pipe_cfg[wait_fe].first_frm_cnt = 0;

	//step 10 : show overflow info
	_vi_show_debug_info();

	//Let postraw trigger go
	atomic_set(&vdev->isp_err_handle_flag, 0);

	if (unlikely(ctrl_flow) && ctx->is_multi_sensor) { //snr0/snr1
		_postraw_clear_inbuf(vdev);
	}

	//Let preraw trgger, err_handler we stop all dev
	_vi_err_retrig_preraw(vdev, err_raw_num);
}

static int _vi_err_handler_thread(void *arg)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)arg;
	enum cvi_isp_raw err_raw_num;
	enum E_VI_TH th_id = E_VI_TH_ERR_HANDLER;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());

		if (vdev->vi_th[th_id].flag == 1)
			err_raw_num = ISP_PRERAW_A;
		else if (vdev->vi_th[th_id].flag == 2)
			err_raw_num = ISP_PRERAW_B;
		else if (vdev->vi_th[th_id].flag == 3)
			err_raw_num = ISP_PRERAW_C;

		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		_vi_err_handler(vdev, err_raw_num);
	}

	return 0;
}

static inline void vi_err_wake_up_th(struct cvi_vi_dev *vdev, enum cvi_isp_raw err_raw)
{
	vdev->vi_th[E_VI_TH_ERR_HANDLER].flag = err_raw + 1;

	wake_up(&vdev->vi_th[E_VI_TH_ERR_HANDLER].wq);
}

u32 isp_err_chk(
	struct cvi_vi_dev *vdev,
	struct isp_ctx *ctx,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 cbdg_0_sts,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 cbdg_1_sts,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 cbdg_0_sts_b,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 cbdg_1_sts_b,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 cbdg_0_sts_c,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 cbdg_1_sts_c)
{
	u32 ret = 0;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_pre_chn_num fe_chn = ISP_FE_CH0;

	if (cbdg_1_sts.bits.FIFO_OVERFLOW_INT) {
		vi_pr(VI_ERR, "CSIBDG_A fifo overflow\n");
		_vi_record_debug_info(ctx);
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt++;
		vi_err_wake_up_th(vdev, raw_num);
		ret = -1;
	}

	if (cbdg_1_sts.bits.FRAME_RESOLUTION_OVER_MAX_INT) {
		vi_pr(VI_ERR, "CSIBDG_A frm size over max\n");
		ret = -1;
	}

	if (cbdg_1_sts.bits.DMA_ERROR_INT) {
		u32 wdma_0_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_err_sts;
		u32 wdma_1_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_err_sts;
		u32 rdma_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_err_sts;
		u32 wdma_0_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_idle;
		u32 wdma_1_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_idle;
		u32 rdma_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_idle;

		if ((wdma_0_err & 0x10) || (wdma_1_err & 0x10) || (rdma_err & 0x10)) {
			vi_pr(VI_ERR, "DMA axi error wdma0_status(0x%x) wdma1_status(0x%x) rdma_status(0x%x)\n",
					wdma_0_err, wdma_1_err, rdma_err);
			ret = -1;
		} else if ((wdma_0_err & 0x20) || (wdma_1_err & 0x20)) {
			vi_pr(VI_ERR, "DMA size mismatch\n wdma0_status(0x%x) wdma1_status(0x%x) rdma_status(0x%x)\n",
					wdma_0_err, wdma_1_err, rdma_err);
			vi_pr(VI_ERR, "wdma0_idle(0x%x) wdma1_idle(0x%x) rdma_idle(0x%x)\n",
					wdma_0_idle, wdma_1_idle, rdma_idle);
			ret = -1;
		} else if ((wdma_0_err & 0x40) || (wdma_1_err & 0x40)) {
			vi_pr(VI_WARN, "WDMA buffer full\n");
		}
	}

	if (cbdg_0_sts.bits.CH0_FRAME_WIDTH_GT_INT) {
		vi_pr(VI_ERR, "CSIBDG_A CH%d frm width greater than setting(%d)\n",
				fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
		vi_err_wake_up_th(vdev, raw_num);
		ret = ctx->is_multi_sensor ? 0 : -1;
	}

	if (cbdg_0_sts.bits.CH0_FRAME_WIDTH_LS_INT) {
		vi_pr(VI_ERR, "CSIBDG_A CH%d frm width less than setting(%d)\n",
				fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
		vi_err_wake_up_th(vdev, raw_num);
		ret = ctx->is_multi_sensor ? 0 : -1;
	}

	if (cbdg_0_sts.bits.CH0_FRAME_HEIGHT_GT_INT) {
		vi_pr(VI_ERR, "CSIBDG_A CH%d frm height greater than setting(%d)\n",
				fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
		vi_err_wake_up_th(vdev, raw_num);
		ret = ctx->is_multi_sensor ? 0 : -1;
	}

	if (cbdg_0_sts.bits.CH0_FRAME_HEIGHT_LS_INT) {
		vi_pr(VI_ERR, "CSIBDG_A CH%d frm height less than setting(%d)\n",
				fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
		vi_err_wake_up_th(vdev, raw_num);
		ret = ctx->is_multi_sensor ? 0 : -1;
	}

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
			ctx->isp_pipe_cfg[raw_num].muxMode == VI_WORK_MODE_2Multiplex) {
		fe_chn = ISP_FE_CH1;

		if (cbdg_0_sts.bits.CH1_FRAME_WIDTH_GT_INT) {
			vi_pr(VI_ERR, "CSIBDG_A CH%d frm width greater than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts.bits.CH1_FRAME_WIDTH_LS_INT) {
			vi_pr(VI_ERR, "CSIBDG_A CH%d frm width less than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts.bits.CH1_FRAME_HEIGHT_GT_INT) {
			vi_pr(VI_ERR, "CSIBDG_A CH%d frm height greater than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts.bits.CH1_FRAME_HEIGHT_LS_INT) {
			vi_pr(VI_ERR, "CSIBDG_A CH%d frm height less than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}
	}

	if (ctx->is_multi_sensor) {
		raw_num = ISP_PRERAW_B;
		fe_chn = ISP_FE_CH0;

		if (cbdg_1_sts_b.bits.FIFO_OVERFLOW_INT) {
			vi_pr(VI_ERR, "CSIBDG_B fifo overflow\n");
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_1_sts_b.bits.FRAME_RESOLUTION_OVER_MAX_INT) {
			vi_pr(VI_ERR, "CSIBDG_B frm size over max\n");
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts_b.bits.CH0_FRAME_WIDTH_GT_INT) {
			vi_pr(VI_ERR, "CSIBDG_B CH%d frm width greater than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts_b.bits.CH0_FRAME_WIDTH_LS_INT) {
			vi_pr(VI_ERR, "CSIBDG_B CH%d frm width less than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts_b.bits.CH0_FRAME_HEIGHT_GT_INT) {
			vi_pr(VI_ERR, "CSIBDG_B CH%d frm height greater than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (cbdg_0_sts_b.bits.CH0_FRAME_HEIGHT_LS_INT) {
			vi_pr(VI_ERR, "CSIBDG_B CH%d frm height less than setting(%d)\n",
					fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = ctx->is_multi_sensor ? 0 : -1;
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			fe_chn = ISP_FE_CH1;

			if (cbdg_0_sts_b.bits.CH1_FRAME_WIDTH_GT_INT) {
				vi_pr(VI_ERR, "CSIBDG_B CH%d frm width greater than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = ctx->is_multi_sensor ? 0 : -1;
			}

			if (cbdg_0_sts_b.bits.CH1_FRAME_WIDTH_LS_INT) {
				vi_pr(VI_ERR, "CSIBDG_B CH%d frm width less than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = ctx->is_multi_sensor ? 0 : -1;
			}

			if (cbdg_0_sts_b.bits.CH1_FRAME_HEIGHT_GT_INT) {
				vi_pr(VI_ERR, "CSIBDG_B CH%d frm height greater than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = ctx->is_multi_sensor ? 0 : -1;
			}

			if (cbdg_0_sts_b.bits.CH1_FRAME_HEIGHT_LS_INT) {
				vi_pr(VI_ERR, "CSIBDG_B CH%d frm height less than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = ctx->is_multi_sensor ? 0 : -1;
			}
		}
		if (gViCtx->total_dev_num == ISP_PRERAW_MAX) {
			raw_num = ISP_PRERAW_C;
			fe_chn = ISP_FE_CH0;

			if (cbdg_1_sts_c.bits.FIFO_OVERFLOW_INT) {
				vi_pr(VI_ERR, "CSIBDG_C fifo overflow\n");
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_1_sts_c.bits.FRAME_RESOLUTION_OVER_MAX_INT) {
				vi_pr(VI_ERR, "CSIBDG_C frm size over max\n");
				ret = -1;
			}

			if (cbdg_0_sts_c.bits.CH0_FRAME_WIDTH_GT_INT) {
				vi_pr(VI_ERR, "CSIBDG_C CH%d frm width greater than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts_c.bits.CH0_FRAME_WIDTH_LS_INT) {
				vi_pr(VI_ERR, "CSIBDG_C CH%d frm width less than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts_c.bits.CH0_FRAME_HEIGHT_GT_INT) {
				vi_pr(VI_ERR, "CSIBDG_C CH%d frm height greater than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts_c.bits.CH0_FRAME_HEIGHT_LS_INT) {
				vi_pr(VI_ERR, "CSIBDG_C CH%d frm height less than setting(%d)\n",
						fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}
	}

	return ret;
}

void isp_post_tasklet(unsigned long data)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)data;
	u8 chn_num = 0, raw_num = 0;

	if (unlikely(vdev->is_yuv_trigger)) {
		for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_MAX; raw_num++) {
			for (chn_num = ISP_FE_CH0; chn_num < ISP_FE_CHN_MAX; chn_num++) {
				if (vdev->isp_triggered[raw_num][chn_num] &&
					!_isp_yuv_bypass_trigger(vdev, raw_num, chn_num)) {
					vdev->isp_triggered[raw_num][chn_num] = false;
					vdev->is_yuv_trigger = false;
					vi_pr(VI_DBG, "raw_%d, chn_%d yuv trigger\n", raw_num, chn_num);
				}
			}
		}
	}

	_post_hw_enque(vdev);
}

static int _vi_preraw_thread(void *arg)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)arg;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	struct isp_ctx *ctx = &vdev->ctx;

	struct list_head *pos, *temp;
	struct _isp_raw_num_n  *n[10];
	unsigned long flags;
	u32 enq_num = 0, i = 0;
	enum E_VI_TH th_id = E_VI_TH_PRERAW;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());
		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		spin_lock_irqsave(&raw_num_lock, flags);
		list_for_each_safe(pos, temp, &pre_raw_num_q.list) {
			n[enq_num] = list_entry(pos, struct _isp_raw_num_n, list);
			if (++enq_num == 10) {
				vi_pr(VI_WARN, "wakeup too later, might loss frame\n");
				break;
			}
		}
		spin_unlock_irqrestore(&raw_num_lock, flags);

		for (i = 0; i < enq_num; i++) {
			raw_num = n[i]->raw_num;

			spin_lock_irqsave(&raw_num_lock, flags);
			list_del_init(&n[i]->list);
			kfree(n[i]);
			spin_unlock_irqrestore(&raw_num_lock, flags);

			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				pre_be_tuning_update(&vdev->ctx, raw_num);
				//Update pre be sts size/addr
				_swap_pre_be_sts_buf(vdev, raw_num, ISP_BE_CH0);

				postraw_tuning_update(&vdev->ctx, raw_num);
				//Update postraw sts awb/dci/hist_edge_v dma size/addr
				_swap_post_sts_buf(ctx, raw_num);
			} else {
				_isp_snr_cfg_deq_and_fire(vdev, raw_num, 0);

				pre_fe_tuning_update(&vdev->ctx, raw_num);

				//fe->be->dram->post or on the fly
				if (_is_fe_be_online(ctx) || _is_all_online(ctx)) {
					pre_be_tuning_update(&vdev->ctx, raw_num);

					//on the fly or slice buffer mode on
					if (_is_all_online(ctx) || ctx->is_slice_buf_on) {
						postraw_tuning_update(&vdev->ctx, raw_num);
					}
				}
			}

			if ((ctx->is_multi_sensor) && (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path)) {
				if ((tuning_dis[0] > 0) && ((tuning_dis[0] - 1) != raw_num)) {
					vi_pr(VI_DBG, "raw_%d start drop\n", raw_num);
					ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
				}
			}

			if (ctx->isp_pipe_cfg[raw_num].is_drop_next_frame) {
				//if !is_drop_next_frame, set is_drop_next_frame flags false;
				if (_is_drop_next_frame(vdev, raw_num, ISP_FE_CH0))
					++vdev->drop_frame_number[raw_num];
				else {
					vi_pr(VI_DBG, "raw_%d stop drop\n", raw_num);
					ctx->isp_pipe_cfg[raw_num].isp_reset_frm =
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] + 1;
					_clear_drop_frm_info(vdev, raw_num);
				}

				//vi onthefly and vpss online will trigger preraw in post_hw_enque
				if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
					continue;

				if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
					_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
					if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
						_pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
				}
			}
		}

		enq_num = 0;
	}

	return 0;
}

static int _vi_vblank_handler_thread(void *arg)
{
	struct cvi_vi_dev *vdev = (struct cvi_vi_dev *)arg;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;

	enum E_VI_TH th_id = E_VI_TH_VBLANK_HANDLER;

	raw_num = (vdev->vi_th[th_id].flag = 1) ? ISP_PRERAW_A : ISP_PRERAW_B;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());
		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		_isp_snr_cfg_deq_and_fire(vdev, raw_num, 1);

	}

	return 0;
}

static void _isp_yuv_online_handler(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	u8 buf_chn = (raw_num == ISP_PRERAW_A) ? hw_chn_num : vdev->ctx.rawb_chnstr_num + hw_chn_num;

	atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_PRERAW_IDLE);

	b = isp_buf_remove(&pre_out_queue[buf_chn]);
	if (b == NULL) {
		vi_pr(VI_INFO, "YUV_chn_%d done outbuf is empty\n", buf_chn);
		return;
	}

	b->crop_le.x = 0;
	b->crop_le.y = 0;
	b->crop_le.w = ctx->isp_pipe_cfg[raw_num].post_img_w;
	b->crop_le.h = ctx->isp_pipe_cfg[raw_num].post_img_h;
	b->is_yuv_frm = 1;
	b->raw_num = raw_num;
	b->chn_num = buf_chn;

	if (_is_be_post_online(ctx))
		isp_buf_queue(&pre_be_in_q, b);
	else if (_is_fe_be_online(ctx))
		isp_buf_queue(&post_in_queue, b);

	// if preraw offline, let usr_pic_timer_handler do it.
	if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
		_pre_hw_enque(vdev, raw_num, hw_chn_num);

	if (!vdev->ctx.isp_pipe_cfg[raw_num].is_offline_scaler) { //YUV sensor online mode
		tasklet_hi_schedule(&vdev->job_work);
	}
}

static void _isp_yuv_bypass_handler(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num, const u8 hw_chn_num)
{
	u8 buf_chn = (raw_num == ISP_PRERAW_A) ? hw_chn_num : vdev->ctx.rawb_chnstr_num + hw_chn_num;

	atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_PRERAW_IDLE);

	cvi_isp_rdy_buf_remove(vdev, buf_chn);

	cvi_isp_dqbuf_list(vdev, vdev->pre_fe_frm_num[raw_num][hw_chn_num], buf_chn);

	vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = raw_num + 1;

	wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);

	if (cvi_isp_rdy_buf_empty(vdev, buf_chn))
		vi_pr(VI_INFO, "fe_%d chn_num_%d yuv bypass outbuf is empty\n", raw_num, buf_chn);
	else
		_isp_yuv_bypass_trigger(vdev, raw_num, hw_chn_num);
}

static inline void _vi_wake_up_preraw_th(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct _isp_raw_num_n  *n;

	n = kmalloc(sizeof(*n), GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "pre_raw_num_q kmalloc size(%zu) fail\n", sizeof(*n));
		return;
	}
	n->raw_num = raw_num;
	pre_raw_num_enq(&pre_raw_num_q, n);

	vdev->vi_th[E_VI_TH_PRERAW].flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
	wake_up(&vdev->vi_th[E_VI_TH_PRERAW].wq);
}

static inline void _vi_wake_up_vblank_th(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	vdev->vi_th[E_VI_TH_VBLANK_HANDLER].flag = (raw_num == ISP_PRERAW_A) ? 1 : 2;
	wake_up(&vdev->vi_th[E_VI_TH_VBLANK_HANDLER].wq);
}

static void _isp_sof_handler(struct cvi_vi_dev *vdev, const enum cvi_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum VI_EVENT type = VI_EVENT_PRE0_SOF + raw_num;
	struct _isp_dqbuf_n *n = NULL;
	unsigned long flags;

	if (atomic_read(&vdev->isp_streamoff) == 1)
		return;

	if (!(_is_fe_be_online(ctx) && ctx->is_slice_buf_on) || ctx->isp_pipe_cfg[raw_num].is_drop_next_frame)
		_vi_wake_up_preraw_th(vdev, raw_num);

	if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 2) //raw_dump flow
		atomic_set(&vdev->isp_raw_dump_en[raw_num], 3);

	tasklet_hi_schedule(&vdev->job_work);

	vi_event_queue(vdev, type, vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);

	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		spin_lock_irqsave(&dq_lock, flags);
		if (!list_empty(&dqbuf_q.list)) {
			n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
			vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = n->chn_id + 1;
			wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);
		}
		spin_unlock_irqrestore(&dq_lock, flags);
	}

	isp_sync_task_process(raw_num);
}

static inline void _isp_pre_fe_done_handler(
	struct cvi_vi_dev *vdev,
	const enum cvi_isp_raw raw_num,
	const enum cvi_isp_pre_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 trigger = false;
	enum cvi_isp_raw cur_raw = raw_num;
	enum cvi_isp_raw next_raw = raw_num;

	/*
	 * raw_num in fe_done means hw_raw
	 * only enable is mux dev, cur_raw/next_raw will be not equal raw_num
	 * first frame idx 0 1 0 1
	 * next fe_out_buf idx 1 0 1 0
	 */
	if (ctx->isp_pipe_cfg[raw_num].is_mux) {
		if (vdev->pre_fe_frm_num[raw_num][chn_num] != vdev->pre_fe_frm_num[raw_num + ISP_PRERAW_MAX][chn_num])
			cur_raw = raw_num + ISP_PRERAW_MAX;
		else
			next_raw = raw_num + ISP_PRERAW_MAX;
	}

	++vdev->pre_fe_frm_num[cur_raw][chn_num];

	//reset error times when fe_done
	if (unlikely(vdev->isp_err_times[raw_num]))
		vdev->isp_err_times[raw_num] = 0;

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //offline mode
			vi_pr(VI_DBG, "pre_fe_%d yuv offline done chn_num=%d frm_num=%d\n",
					raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			_isp_yuv_bypass_handler(vdev, raw_num, chn_num);
		} else { //YUV sensor online mode
			vi_pr(VI_DBG, "pre_fe_%d yuv online done chn_num=%d frm_num=%d\n",
					raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			_isp_yuv_online_handler(vdev, raw_num, chn_num);
		}
		return;
	}

	vi_pr(VI_DBG, "%d pre_fe_%d frm_done chn_num=%d frm_num=%d\n",
			ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPol,
			cur_raw, chn_num, vdev->pre_fe_frm_num[cur_raw][chn_num]);

	// No changed in onthefly mode or slice buffer on
	if (!_is_all_online(ctx) && !(_is_fe_be_online(ctx) && ctx->is_rgbmap_sbm_on)) {
		ispblk_tnr_rgbmap_chg(ctx, next_raw, chn_num);
		_pre_fe_rgbmap_update(vdev, next_raw, chn_num);
	}

	if (_is_fe_be_online(ctx) || _is_all_online(ctx)) { //fe->be->dram->post or on the fly mode
		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3) { //raw_dump flow
			struct isp_buffer *b = NULL;
			struct isp_queue *fe_out_q = (chn_num == ISP_FE_CH0) ?
							&raw_dump_b_q[raw_num] :
							&raw_dump_b_se_q[raw_num];
			struct isp_queue *raw_d_q = (chn_num == ISP_FE_CH0) ?
							&raw_dump_b_dq[raw_num] :
							&raw_dump_b_se_dq[raw_num];
			u32 x, y, w, h, dmaid;

			if (ctx->isp_pipe_cfg[raw_num].rawdump_crop.w &&
				ctx->isp_pipe_cfg[raw_num].rawdump_crop.h) {
				x = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.x :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.x;
				y = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.y :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.y;
				w = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.w :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.w;
				h = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.h :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.h;
			} else {
				x = 0;
				y = 0;
				w = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].crop.w :
					ctx->isp_pipe_cfg[raw_num].crop_se.w;
				h = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].crop.h :
					ctx->isp_pipe_cfg[raw_num].crop_se.h;
			}

			if (raw_num == ISP_PRERAW_A) {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL6 : ISP_BLK_ID_DMA_CTL7);
			} else if (raw_num == ISP_PRERAW_B) {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13);
			} else {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19);
			}

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[raw_num];

			ispblk_csidbg_dma_wr_en(ctx, raw_num, chn_num, 0);

			b = isp_buf_remove(fe_out_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
				return;
			}

			b->crop_le.x = b->crop_se.x = x;
			b->crop_le.y = b->crop_se.y = y;
			b->crop_le.w = b->crop_se.w = w;
			b->crop_le.h = b->crop_se.h = h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[raw_num][chn_num];

			isp_buf_queue(raw_d_q, b);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				trigger = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
			} else
				trigger = true;

			if (trigger) {
				_isp_raw_dump_chk(vdev, raw_num, b->frm_num);
			}
		}

		atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_IDLE);

		if (_is_all_online(ctx)) {
			struct isp_grid_s_info m_info;

			m_info = ispblk_rgbmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = m_info.h_bit;

			m_info = ispblk_lmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].lmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].lmap_i.h_bit = m_info.h_bit;
		}
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		struct isp_buffer *b = NULL;
		struct isp_grid_s_info m_info;
		struct isp_queue *fe_out_q = (chn_num == ISP_FE_CH0) ?
						&pre_out_queue[cur_raw] : &pre_out_se_queue[cur_raw];
		struct isp_queue *be_in_q = (chn_num == ISP_FE_CH0) ? &pre_be_in_q : &pre_be_in_se_q[cur_raw];
		struct isp_queue *raw_d_q = (chn_num == ISP_FE_CH0) ?
					    &raw_dump_b_dq[cur_raw] :
					    &raw_dump_b_se_dq[cur_raw];

		if (atomic_read(&vdev->isp_raw_dump_en[cur_raw]) == 3) //raw dump enable
			fe_out_q = (chn_num == ISP_FE_CH0) ? &raw_dump_b_q[cur_raw] : &raw_dump_b_se_q[cur_raw];

		b = isp_buf_remove(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return;
		}

		if (atomic_read(&vdev->isp_raw_dump_en[cur_raw]) == 3) { //raw dump enable
			u32 w = (chn_num == ISP_FE_CH0) ?
				ctx->isp_pipe_cfg[cur_raw].crop.w :
				ctx->isp_pipe_cfg[cur_raw].crop_se.w;
			u32 h = (chn_num == ISP_FE_CH0) ?
				ctx->isp_pipe_cfg[cur_raw].crop.h :
				ctx->isp_pipe_cfg[cur_raw].crop_se.h;
			u32 dmaid;

			if (raw_num == ISP_PRERAW_A) {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL6 : ISP_BLK_ID_DMA_CTL7);
			} else if (raw_num == ISP_PRERAW_B) {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL12 : ISP_BLK_ID_DMA_CTL13);
			} else {
				dmaid = ((chn_num == ISP_FE_CH0) ? ISP_BLK_ID_DMA_CTL18 : ISP_BLK_ID_DMA_CTL19);
			}

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[cur_raw];

			b->crop_le.x = b->crop_se.x = 0;
			b->crop_le.y = b->crop_se.y = 0;
			b->crop_le.w = b->crop_se.w = ctx->isp_pipe_cfg[cur_raw].crop.w;
			b->crop_le.h = b->crop_se.h = ctx->isp_pipe_cfg[cur_raw].crop.h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[cur_raw][chn_num];

			isp_buf_queue(raw_d_q, b);
		} else {
			m_info = ispblk_rgbmap_info(ctx, raw_num);
			b->rgbmap_i.w_bit = m_info.w_bit;
			b->rgbmap_i.h_bit = m_info.h_bit;

			m_info = ispblk_lmap_info(ctx, raw_num);
			b->lmap_i.w_bit = m_info.w_bit;
			b->lmap_i.h_bit = m_info.h_bit;

			b->is_yuv_frm	= 0;
			b->chn_num	= 0;

			isp_buf_queue(be_in_q, b);
		}

		atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_PRERAW_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
					vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			//vi_pr(VI_DBG, "fe->dram->be->post trigger raw_num=%d\n", raw_num);
			if (atomic_read(&vdev->isp_raw_dump_en[cur_raw]) == 3) { //raw dump flow
				_isp_raw_dump_chk(vdev, cur_raw, b->frm_num);
			} else {
				tasklet_hi_schedule(&vdev->job_work);
			}
			_vi_wake_up_vblank_th(vdev, raw_num);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_mux &&
			ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin != -1U) {
			ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPol ^= 1;
			gpio_set_value(ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPin,
				ctx->isp_pipe_cfg[raw_num].muxSwitchGpio.switchGpioPol);
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
				_pre_hw_enque(vdev, raw_num, (chn_num == ISP_FE_CH0) ? ISP_FE_CH1 : ISP_FE_CH0);
			else
				_pre_hw_enque(vdev, next_raw, chn_num);
		}
	}
}

static inline void _isp_pre_be_done_handler(
	struct cvi_vi_dev *vdev,
	const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	u32 type;
	u32 trigger = false;

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { // fe->be->dram->post
		struct isp_buffer *b = NULL;
		struct isp_grid_s_info m_info;
		struct isp_queue *be_out_q = (chn_num == ISP_FE_CH0) ?
						&pre_be_out_q : &pre_be_out_se_q;
		struct isp_queue *post_in_q = (chn_num == ISP_FE_CH0) ?
						&post_in_queue : &post_in_se_queue;

		++vdev->pre_be_frm_num[raw_num][chn_num];
		type = (raw_num == ISP_PRERAW_A) ? VI_EVENT_PRE0_EOF : VI_EVENT_PRE1_EOF;

		vi_pr(VI_DBG, "pre_be frm_done chn_num=%d frm_num=%d\n",
				chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		b = isp_buf_remove(be_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_be chn_num_%d outbuf is empty\n", chn_num);
			return;
		}

		m_info = ispblk_rgbmap_info(ctx, raw_num);
		b->rgbmap_i.w_bit = m_info.w_bit;
		b->rgbmap_i.h_bit = m_info.h_bit;

		m_info = ispblk_lmap_info(ctx, raw_num);
		b->lmap_i.w_bit = m_info.w_bit;
		b->lmap_i.h_bit = m_info.h_bit;

		isp_buf_queue(post_in_q, b);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		atomic_set(&vdev->pre_be_state[chn_num], ISP_PRE_BE_IDLE);

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw)
			_pre_hw_enque(vdev, raw_num, chn_num);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			vi_event_queue(vdev, type, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);

			tasklet_hi_schedule(&vdev->job_work);
			_vi_wake_up_vblank_th(vdev, raw_num);
		}
	} else if (_is_be_post_online(ctx)) { // fe->dram->be->post
		struct isp_buffer *b = NULL;
		struct isp_queue *be_in_q = (chn_num == ISP_BE_CH0) ?
						&pre_be_in_q : &pre_be_in_se_q[vdev->ctx.cam_id];
		struct isp_queue *pre_out_q = NULL;

		if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			b = isp_buf_remove(be_in_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "Pre_be chn_num_%d input buf is empty\n", chn_num);
				return;
			}
			if (b->raw_num >= ISP_PRERAW_VIRT_MAX) {
				vi_pr(VI_ERR, "buf raw_num_%d is wrong\n", b->raw_num);
				return;
			}
			raw_num = b->raw_num;
		}

		++vdev->pre_be_frm_num[raw_num][chn_num];
		type = VI_EVENT_PRE0_EOF + raw_num;

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			pre_out_q = (chn_num == ISP_BE_CH0) ?
				&pre_out_queue[raw_num] : &pre_out_se_queue[raw_num];
			isp_buf_queue(pre_out_q, b);
		}

		atomic_set(&vdev->pre_be_state[chn_num], ISP_PRE_BE_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger)
			vi_event_queue(vdev, type, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
	} else if (_is_all_online(ctx)) { // fly-mode
		++vdev->pre_be_frm_num[raw_num][chn_num];
		type = (raw_num == ISP_PRERAW_A) ? VI_EVENT_PRE0_EOF : VI_EVENT_PRE1_EOF;

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		vi_event_queue(vdev, type, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { // fe->be->dram->post
		++vdev->pre_be_frm_num[raw_num][chn_num];
		type = (raw_num == ISP_PRERAW_A) ? VI_EVENT_PRE0_EOF : VI_EVENT_PRE1_EOF;

		vi_pr(VI_DBG, "pre_be frm_done chn_num=%d frm_num=%d\n",
				chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		atomic_set(&vdev->pre_be_state[chn_num], ISP_PRE_BE_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			tasklet_hi_schedule(&vdev->job_work);
			_vi_wake_up_vblank_th(vdev, raw_num);

			vi_event_queue(vdev, type, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
		}
	}
}

static void _isp_postraw_shaw_done_handler(struct cvi_vi_dev *vdev)
{
	if (_is_fe_be_online(&vdev->ctx) && vdev->ctx.is_slice_buf_on) {
		vi_pr(VI_INFO, "postraw shaw done\n");
		_vi_wake_up_preraw_th(vdev, ISP_PRERAW_A);
	}
}

static void _isp_postraw_done_handler(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW_A;
	enum cvi_isp_raw next_raw = ISP_PRERAW_A;
	enum cvi_isp_raw ac_raw = ISP_PRERAW_A;
	u32 type = VI_EVENT_POST_EOF;

	if (_is_fe_be_online(ctx))
		raw_num = ctx->cam_id;

	if (_isp_clk_dynamic_en(vdev, false) < 0)
		return;

	++ctx->isp_pipe_cfg[raw_num].first_frm_cnt;

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		struct isp_buffer *ispb, *ispb_se;

		ispb = isp_buf_remove(&post_in_queue);
		if (ispb == NULL) {
			vi_pr(VI_ERR, "post_in_q is empty\n");
			return;
		}
		if (ispb->raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "buf raw_num_%d is wrong\n", ispb->raw_num);
			return;
		}
		raw_num = ispb->raw_num;

		if (ispb->is_yuv_frm) {
			isp_buf_queue(&pre_out_queue[ispb->chn_num], ispb);
		} else {
			isp_buf_queue(&pre_be_out_q, ispb);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				ispb_se = isp_buf_remove(&post_in_se_queue);
				if (ispb_se == NULL) {
					vi_pr(VI_ERR, "post_in_se_q is empty\n");
					return;
				}
				isp_buf_queue(&pre_be_out_se_q, ispb_se);
			}
		}
	} else if (_is_be_post_online(ctx)) {
		raw_num = ctx->cam_id;

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			struct isp_buffer *b = NULL;
			struct isp_queue *be_in_q = &pre_be_in_q;
			struct isp_queue *pre_out_q = NULL;
			u8 chn_num = 0;

			b = isp_buf_remove(be_in_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "pre_be_in_q is empty\n");
				return;
			}
			if (b->chn_num >= ISP_CHN_MAX) {
				vi_pr(VI_ERR, "buf chn_num_%d is wrong\n", b->chn_num);
				return;
			}
			chn_num = b->chn_num;

			pre_out_q = &pre_out_queue[chn_num];
			isp_buf_queue(pre_out_q, b);
		}
	} else if (_is_all_online(ctx) ||
		(_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
		//Update postraw stt gms/ae/hist_edge_v dma size/addr
		_swap_post_sts_buf(ctx, raw_num);

		//Change post done flag to be true
		if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)
			atomic_set(&vdev->ctx.is_post_done, 1);
	}

	atomic_set(&vdev->postraw_state, ISP_POSTRAW_IDLE);
	if (_is_be_post_online(ctx) && ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path)
		atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_PRE_BE_IDLE);

	++vdev->postraw_frame_number[raw_num];

	vi_pr(VI_DBG, "Postraw_%d frm_done frm_num=%d\n", raw_num, vdev->postraw_frame_number[raw_num]);

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //ISP team no need yuv post done
		if (isp_bufpool[raw_num].fswdr_rpt)
			ispblk_fswdr_update_rpt(ctx, isp_bufpool[raw_num].fswdr_rpt);

		type = VI_EVENT_POST_EOF + raw_num;

		ctx->mmap_grid_size[raw_num] = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;

		vi_event_queue(vdev, type, vdev->postraw_frame_number[raw_num]);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		u8 chn_num = raw_num;

		chn_num = find_ac_chn_num(ctx, raw_num);

		cvi_isp_rdy_buf_remove(vdev, chn_num);

		cvi_isp_dqbuf_list(vdev, vdev->postraw_frame_number[raw_num], chn_num);

		vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = raw_num + 1;

		wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);
	}

	if (!ctx->isp_pipe_cfg[raw_num].is_offline_preraw) {
		tasklet_hi_schedule(&vdev->job_work);

		if (!ctx->is_slice_buf_on &&
		    !(ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)) {
			ac_raw = find_hw_raw_num(raw_num);
			if (vdev->ctx.isp_pipe_cfg[ac_raw].is_mux &&
					(vdev->ctx.isp_pipe_cfg[ac_raw].muxSwitchGpio.switchGpioPol !=
					vdev->ctx.isp_pipe_cfg[ac_raw].muxSwitchGpio.switchGpioInit))
				next_raw = raw_num + ISP_PRERAW_MAX;
			else
				next_raw = raw_num;
			_pre_hw_enque(vdev, next_raw, ISP_FE_CH0);
			if (ctx->isp_pipe_cfg[ac_raw].is_hdr_on)
				_pre_hw_enque(vdev, next_raw, ISP_FE_CH1);
		}
	}
}

void vi_irq_handler(struct cvi_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 cbdg_0_sts[ISP_PRERAW_MAX] = { 0 };
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 cbdg_1_sts[ISP_PRERAW_MAX] = { 0 };
	union REG_ISP_TOP_INT_EVENT0 top_sts;
	union REG_ISP_TOP_INT_EVENT1 top_sts_1;
	union REG_ISP_TOP_INT_EVENT2 top_sts_2;
	u8 i = 0, raw_num = ISP_PRERAW_A;

	isp_intr_status(ctx, &top_sts, &top_sts_1, &top_sts_2);

	if (!atomic_read(&vdev->isp_streamon))
		return;

	vi_perf_record_dump();

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		isp_csi_intr_status(ctx, raw_num, &cbdg_0_sts[raw_num], &cbdg_1_sts[raw_num]);

		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_0 = cbdg_0_sts[raw_num].raw;
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_1 = cbdg_1_sts[raw_num].raw;

		ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts = ispblk_fe_dbg_info(ctx, raw_num);
		if (raw_num == ISP_PRERAW_A) {
			ctx->isp_pipe_cfg[raw_num].dg_info.be_sts = ispblk_be_dbg_info(ctx);
			ctx->isp_pipe_cfg[raw_num].dg_info.post_sts = ispblk_post_dbg_info(ctx);
			ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts = ispblk_dma_dbg_info(ctx);
		}

		for (i = 0; i < ISP_FE_CHN_MAX; i++)
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[i] = ispblk_csibdg_chn_dbg(ctx, raw_num, i);
	}

	if (isp_err_chk(vdev, ctx, cbdg_0_sts[0], cbdg_1_sts[0], cbdg_0_sts[1], cbdg_1_sts[1],
		cbdg_0_sts[2], cbdg_1_sts[2]) == -1)
		return;

	//if (top_sts.bits.INT_DMA_ERR)
	//	vi_pr(VI_ERR, "DMA error\n");

	/* pre_fe0 ch0 frame start */
	if (top_sts_2.bits.FRAME_START_FE0 & 0x1) {
		vi_record_sof_perf(vdev, ISP_PRERAW_A, ISP_FE_CH0);

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
			++vdev->pre_fe_sof_cnt[ISP_PRERAW_A][ISP_FE_CH0];

		vi_pr(VI_INFO, "pre_fe_%d sof chn_num=%d frm_num=%d\n",
				ISP_PRERAW_A, ISP_FE_CH0, vdev->pre_fe_sof_cnt[ISP_PRERAW_A][ISP_FE_CH0]);

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) { //RGB sensor
			if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				if (vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_mux &&
					(vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].muxSwitchGpio.switchGpioPol !=
					vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].muxSwitchGpio.switchGpioInit))
					_isp_sof_handler(vdev, ISP_PRERAW_A + ISP_PRERAW_MAX);
				else
					_isp_sof_handler(vdev, ISP_PRERAW_A);
			}
		} else if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) { //YUV sensor online mode
			//ISP team no need sof event by yuv sensor
			//_post_hw_enque(vdev);
			tasklet_hi_schedule(&vdev->job_work);
		}
	}

	/* pre_fe0 ch1 frame start */
	if (top_sts_2.bits.FRAME_START_FE0 & 0x2) {
		++vdev->pre_fe_sof_cnt[ISP_PRERAW_A][ISP_FE_CH1];

		//_isp_sof_handler(ISP_PRERAW_A);
	}

	/* pre_fe1 ch0 frame start */
	if (top_sts_2.bits.FRAME_START_FE1 & 0x1) {
		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_offline_preraw)
			++vdev->pre_fe_sof_cnt[ISP_PRERAW_B][ISP_FE_CH0];

		vi_pr(VI_INFO, "pre_fe_%d sof chn_num=%d frm_num=%d\n",
				ISP_PRERAW_B, ISP_FE_CH0, vdev->pre_fe_sof_cnt[ISP_PRERAW_B][ISP_FE_CH0]);

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) { //RGB sensor
			_isp_sof_handler(vdev, ISP_PRERAW_B);
		} else if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_B].is_offline_scaler) { //YUV sensor online mode
			//ISP team no need sof event by yuv sensor
			//_post_hw_enque(vdev);
			tasklet_hi_schedule(&vdev->job_work);
		}
	}

	/* pre_fe1 ch1 frame start */
	if (top_sts_2.bits.FRAME_START_FE1 & 0x2) {
		++vdev->pre_fe_sof_cnt[ISP_PRERAW_B][ISP_FE_CH1];

		//_isp_sof_handler(ISP_PRERAW_B);
	}

	/* pre_fe2 ch0 frame start */
	if (top_sts_2.bits.FRAME_START_FE2 & 0x1) {
		++vdev->pre_fe_sof_cnt[ISP_PRERAW_C][ISP_FE_CH0];
		 vi_pr(VI_INFO, "pre_fe_%d sof chn_num=%d frm_num=%d\n",
				ISP_PRERAW_C, ISP_FE_CH0, vdev->pre_fe_sof_cnt[ISP_PRERAW_C][ISP_FE_CH0]);

		if (!vdev->ctx.isp_pipe_cfg[ISP_PRERAW_C].is_offline_scaler) { //YUV sensor online mode
			//ISP team no need sof event by yuv sensor
			tasklet_hi_schedule(&vdev->job_work);
		}
	}

	/* pre_fe2 ch1 frame start */
	if (top_sts_2.bits.FRAME_START_FE1 & 0x2) {
		++vdev->pre_fe_sof_cnt[ISP_PRERAW_C][ISP_FE_CH1];

		//_isp_sof_handler(ISP_PRERAW_C);
	}

	if (!ctx->is_synthetic_hdr_on) {
		//HW limit
		//On hdr and stagger vsync mode, need to re-trigger pq vld after ch0 pq done.
		/* pre_fe0 ch0 pq done */
		if (top_sts_1.bits.PQ_DONE_FE0 & 0x1) {
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on &&
			    ctx->isp_pipe_cfg[ISP_PRERAW_A].is_stagger_vsync) {
				ISP_WR_BITS(ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0],
					REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH1, 1);
			}
		}

		/* pre_fe1 ch0 pq done */
		if (top_sts_1.bits.PQ_DONE_FE1 & 0x1) {
			if (ctx->isp_pipe_cfg[ISP_PRERAW_B].is_hdr_on &&
			    ctx->isp_pipe_cfg[ISP_PRERAW_B].is_stagger_vsync) {
				ISP_WR_BITS(ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1],
					REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH1, 1);
			}
		}
	}

	/* pre_fe0 ch0 frm_done */
	if (top_sts.bits.FRAME_DONE_FE0 & 0x1) {
		vi_record_fe_perf(vdev, ISP_PRERAW_A, ISP_FE_CH0);

		// In synthetic HDR mode, we assume that the first SOF is long exposure frames,
		// and the second SOF is short exposure frames.
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on && ctx->is_synthetic_hdr_on) {
			if (vdev->pre_fe_frm_num[ISP_PRERAW_A][ISP_FE_CH0] ==
			    vdev->pre_fe_frm_num[ISP_PRERAW_A][ISP_FE_CH1]) { // LE
				_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH0);
			} else { // SE
				_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH1);
			}
		} else {
			_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH0);
		}
	}

	/* pre_fe0 ch1 frm_done */
	if (top_sts.bits.FRAME_DONE_FE0 & 0x2) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH1);
	}

	/* pre_fe0 ch2 frm_done */
	if (top_sts.bits.FRAME_DONE_FE0 & 0x4) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH2);
	}

	/* pre_fe0 ch3 frm_done */
	if (top_sts.bits.FRAME_DONE_FE0 & 0x8) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_A, ISP_FE_CH3);
	}

	/* pre_fe1 ch0 frm_done */
	if (top_sts.bits.FRAME_DONE_FE1 & 0x1) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_B, ISP_FE_CH0);
	}

	/* pre_fe1 ch1 frm_done */
	if (top_sts.bits.FRAME_DONE_FE1 & 0x2) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_B, ISP_FE_CH1);
	}

	/* pre_fe2 ch0 frm_done */
	if (top_sts.bits.FRAME_DONE_FE2 & 0x1) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_C, ISP_FE_CH0);
	}

	/* pre_fe2 ch1 frm_done */
	if (top_sts.bits.FRAME_DONE_FE2 & 0x2) {
		_isp_pre_fe_done_handler(vdev, ISP_PRERAW_C, ISP_FE_CH1);
	}

	/* pre_be ch0 frm done */
	if (top_sts.bits.FRAME_DONE_BE & 0x1) {
		vi_record_be_perf(vdev, ISP_PRERAW_A, ISP_BE_CH0);

		_isp_pre_be_done_handler(vdev, ISP_BE_CH0);
	}

	/* pre_be ch1 frm done */
	if (top_sts.bits.FRAME_DONE_BE & 0x2) {
		_isp_pre_be_done_handler(vdev, ISP_BE_CH1);
	}

	/* post shadow up done */
	if (top_sts.bits.SHAW_DONE_POST) {
		if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
			_isp_postraw_shaw_done_handler(vdev);
		}
	}

	/* post frm done */
	if (top_sts.bits.FRAME_DONE_POST) {
		vi_record_post_end(vdev, ISP_PRERAW_A);

		_isp_postraw_done_handler(vdev);
	}
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vi_create_instance(struct platform_device *pdev)
{
	int ret = 0;
	struct cvi_vi_dev *vdev;
	struct mod_ctx_s  ctx_s;

	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return -EINVAL;
	}

	vi_set_base_addr(vdev->reg_base);

	ret = _vi_mempool_setup();
	if (ret) {
		vi_pr(VI_ERR, "Failed to setup isp memory\n");
		goto err;
	}

	ret = _vi_create_proc(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create proc\n");
		goto err;
	}

	gViCtx = (struct cvi_vi_ctx *)vdev->shared_mem;

	_vi_init_param(vdev);

	ret = vi_create_thread(vdev, E_VI_TH_PRERAW);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create preraw thread\n");
		goto err;
	}

	ret = vi_create_thread(vdev, E_VI_TH_VBLANK_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create vblank_update thread\n");
		goto err;
	}

	ret = vi_create_thread(vdev, E_VI_TH_ERR_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create err_handler thread\n");
		goto err;
	}

	ctx_s.modID = CVI_ID_VI;
	ctx_s.ctx_num = 0;
	ctx_s.ctx_info = (void *)gViCtx;

	ret = base_set_mod_ctx(&ctx_s);
	if (ret) {
		vi_pr(VI_ERR, "Failed to set mod ctx\n");
		goto err;
	}

err:
	return ret;
}

int vi_destroy_instance(struct platform_device *pdev)
{
	int ret = 0, i = 0;
	struct cvi_vi_dev *vdev;

	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return -EINVAL;
	}

	_vi_destroy_proc(vdev);

	for (i = 0; i < E_VI_TH_MAX; i++)
		vi_destory_thread(vdev, i);

	vi_tuning_buf_release();

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		sync_task_exit(i);
		kfree(isp_bufpool[i].fswdr_rpt);
		isp_bufpool[i].fswdr_rpt = 0;
	}

	tasklet_kill(&vdev->job_work);

	return ret;
}
