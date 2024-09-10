#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/div64.h>

#include <linux/cvi_base.h>
#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>
#include <linux/delay.h>

#include <base_cb.h>
#include <vi_cb.h>
#include <vpss_cb.h>
#include <dwa_cb.h>
#include <vcodec_cb.h>
#include <vb.h>
#include <vip_common.h>
#include "vpss.h"
#include "vpss_common.h"
#include "vpss_core.h"
#include "cvi_vip_img.h"
#include "cvi_vip_sc.h"
#include "cvi_vip_vpss_proc.h"
#include "sys.h"

/*******************************************************
 *  MACRO definition
 ******************************************************/
#define IDLE_TIMEOUT_MS      10000
#define EOF_WAIT_TIMEOUT_MS  500

#define YRATIO_SCALE         100

#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_VI_ERR     0x0004

enum handler_state {
	HANDLER_STATE_STOP = 0,
	HANDLER_STATE_RUN,
	HANDLER_STATE_SUSPEND,
	HANDLER_STATE_RESUME,
	HANDLER_STATE_MAX,
};

enum vpss_grp_state {
	GRP_STATE_IDLE = 0x0,
	GRP_STATE_BUF_FILLED = 0x1,
	GRP_STATE_HW_STARTED = 0x2,
	GPP_MAX_STATE
};

struct _vpss_gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};

struct vpss_ext_ctx {
	struct cvi_csc_cfg csc_cfg;
	CVI_S32 proc_amp[PROC_AMP_MAX];
	struct vpss_grp_sbm_cfg sbm_cfg;
	CVI_U32 grp_state;
	CVI_U8 scene;
};

static struct cvi_vip_dev *vip_dev;

static struct cvi_vpss_ctx *vpssCtx[VPSS_MAX_GRP_NUM] = { [0 ... VPSS_MAX_GRP_NUM - 1] = NULL };

// cvi_vpss_ctx in uapi, internal extension version.
static struct vpss_ext_ctx vpssExtCtx[VPSS_MAX_GRP_NUM];

// Motion level for vcodec
static struct mlv_i_s g_mlv_i[VI_MAX_DEV_NUM];

static CVI_U8 isp_bypass_frm[VI_MAX_CHN_NUM];

struct vpss_handler_ctx {
	CVI_U8 u8VpssDev;	// index of handler_ctx
	CVI_BOOL online_from_isp;
	CVI_U8 online_chkMask;
	//vpss_rgnex_jobs_t rgnex_jobs;
	enum handler_state enHdlState;
	wait_queue_head_t wait;
	wait_queue_head_t vi_reset_wait;
	bool reset_done;
	struct task_struct *thread;

	VPSS_GRP workingGrp;
	CVI_U8 workingMask;
	atomic_t events;
	CVI_U8 img_idx;
	struct timespec64 time;
	spinlock_t lock;
	CVI_U8 IntMask[VPSS_ONLINE_NUM];
	CVI_U8 isr_evt[VPSS_ONLINE_NUM];
	struct cvi_vpss_ctx vpssCtxWork[VPSS_ONLINE_NUM];
} handler_ctx[VPSS_IP_NUM];

static struct cvi_gdc_mesh mesh[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];

static PROC_AMP_CTRL_S procamp_ctrls[PROC_AMP_MAX] = {
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
};

VI_VPSS_MODE_S stVIVPSSMode;
VPSS_MODE_S stVPSSMode;
static bool fb_on_vpss;

static inline CVI_S32 CHECK_VPSS_GRP_CREATED(VPSS_GRP grp)
{
	if (!vpssCtx[grp] || !vpssCtx[grp]->isCreated) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return CVI_ERR_VPSS_UNEXIST;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VPSS_CHN_VALID(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	if (vpss_get_mode() == VPSS_MODE_SINGLE) {
		if ((VpssChn >= VPSS_MAX_CHN_NUM) || (VpssChn < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid for VPSS-Single.\n",
				       VpssGrp, VpssChn);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	} else {
		if ((VpssChn >= vpssCtx[VpssGrp]->chnNum) || (VpssChn < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid for VPSS-Dual(%d).\n"
			, VpssGrp, VpssChn, vpssCtx[VpssGrp]->stGrpAttr.u8VpssDev);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 check_vpss_id(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;
	return ret;
}

CVI_VOID vpss_notify_wkup_evt(CVI_U8 u8VpssDev)
{
	if (u8VpssDev >= VPSS_IP_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid dev(%d)\n", u8VpssDev);
		return;
	}

	atomic_fetch_or(CTX_EVENT_WKUP, &handler_ctx[u8VpssDev].events);
	wake_up_interruptible(&handler_ctx[u8VpssDev].wait);
}

CVI_VOID vpss_notify_isr_evt(CVI_U8 img_idx)
{
	struct cvi_img_vdev *idev = &vip_dev->img_vdev[img_idx];
	unsigned long flags_job;
	CVI_U8 i;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), vpss isr, wakeup\n",
			idev->dev_idx, idev->img_type, idev->input_type);

	for (i = 0; i < VPSS_IP_NUM; i++) {
		if (handler_ctx[i].enHdlState == HANDLER_STATE_RUN &&
			handler_ctx[i].img_idx == img_idx) {

			atomic_fetch_or(CTX_EVENT_EOF, &handler_ctx[i].events);
			spin_lock_irqsave(&handler_ctx[i].lock, flags_job);
			if (handler_ctx[i].online_from_isp) {
				handler_ctx[i].workingGrp = idev->job_grp;
				handler_ctx[i].IntMask[idev->job_grp] = idev->IntMask;
				handler_ctx[i].isr_evt[idev->job_grp]++;
			}
			spin_unlock_irqrestore(&handler_ctx[i].lock, flags_job);

			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "handler_ctx[%d] state=%d, img_idx=%d, event=0x%x\n",
					i, handler_ctx[i].enHdlState,
					handler_ctx[i].img_idx,
					handler_ctx[i].events.counter);
			wake_up_interruptible(&handler_ctx[i].wait);
			break;
		} else
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "handler_ctx[%d] state=%d, img_idx=%d, event=0x%x\n",
					i, handler_ctx[i].enHdlState,
					handler_ctx[i].img_idx,
					handler_ctx[i].events.counter);
	}
}

CVI_VOID vpss_notify_vi_err_evt(CVI_U8 img_idx)
{
	struct cvi_img_vdev *idev = &vip_dev->img_vdev[img_idx];
	struct vpss_handler_ctx *ctx = NULL;
	CVI_U8 i;
	int ret;

	CVI_TRACE_VPSS(CVI_DBG_WARN, "img(%d:%d:%d), vi err, wakeup\n",
			idev->dev_idx, idev->img_type, idev->input_type);

	for (i = 0; i < VPSS_IP_NUM; i++) {
		if (handler_ctx[i].enHdlState == HANDLER_STATE_RUN &&
			handler_ctx[i].img_idx == img_idx) {
			ctx = &handler_ctx[i];
		}
	}
	if (ctx) {
		atomic_set(&ctx->events, CTX_EVENT_VI_ERR);
		ctx->workingGrp = idev->job_grp;
	} else
		return;

	wake_up_interruptible(&ctx->wait);

	// blocking wait
	ctx->reset_done = false;
	ret = wait_event_interruptible(ctx->vi_reset_wait, ctx->reset_done);
	CVI_TRACE_VPSS(CVI_DBG_WARN, "wait reset done, ret=%d\n", ret);

}

CVI_U8 sc_index_to_chn_id(CVI_U8 sc_idx)
{
	if ((vpss_get_mode() != VPSS_MODE_SINGLE) && sc_idx)
		return sc_idx - 1;

	return sc_idx;
}

CVI_VOID vpss_print_vb_info(CVI_U8 grp_id, CVI_U8 sc_idx)
{
	CVI_U8 chn_id;
	CVI_U32 pool_id;

	chn_id = sc_index_to_chn_id(sc_idx);

	if (vpssCtx[grp_id] == NULL) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", grp_id);
		return;
	}
	pool_id = vpssCtx[grp_id]->stChnCfgs[chn_id].VbPool;
	if (pool_id == VB_INVALID_POOLID) {
		pool_id = find_vb_pool(vpssCtx[grp_id]->stChnCfgs[chn_id].blk_size);
		if (pool_id == VB_INVALID_POOLID) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Not find vb pool.\n");
			return;
		}
	}
	vb_print_pool(pool_id);

#if 0
	CVI_U32 i = 0;
	struct vb_jobs_t *jobs = NULL;
	struct vb_s *vb_tmp = NULL;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = grp_id, .s32ChnId = 0};

	chn.s32ChnId = chn_id;
	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_OUT);

	if (!jobs || !jobs->inited)
		return;

	mutex_lock(&jobs->lock);
	if (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_FOREACH(vb_tmp, &jobs->workq, i) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "workq vb paddr(%#llx).\n", vb_tmp->phy_addr);
		}
	}

	if (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_FOREACH(vb_tmp, &jobs->doneq, i) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "doneq vb paddr(%#llx).\n", vb_tmp->phy_addr);
		}
	}
	mutex_unlock(&jobs->lock);
#endif
}

struct cvi_vpss_ctx **vpss_get_shdw_ctx(void)
{
	return vpssCtx;
}

CVI_S32 get_dev_info_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	if (chn.enModId != CVI_ID_VPSS)
		return 0;
	if (vpssCtx[chn.s32DevId] == NULL) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", chn.s32DevId);
		return 0;
	}
	/* single mode: img_dev[1], img_in_v + (sc_d -> sc_v1 -> sc_v2 -> sc_v3)  */
	if (vpss_get_mode() == VPSS_MODE_SINGLE)
		return (chn_type == CHN_TYPE_OUT) ? chn.s32ChnId : 1;

	// for VPSS_DUAL
	// [0] img_in_d + sc_d
	// [1] img_in_v + (sc_v1 -> sc_v2 -> sc_v3)
	if (chn_type == CHN_TYPE_OUT) {
		if (vpssCtx[chn.s32DevId]->u8DevId == 0)
			return 0;
		if ((chn.s32ChnId + 1) < VPSS_MAX_CHN_NUM)
			return chn.s32ChnId + 1;
		return 0;
	}
	return vpssCtx[chn.s32DevId]->u8DevId;

	return 0;
}

static CVI_S32 _mesh_gdc_do_op_cb(enum GDC_USAGE usage, const CVI_VOID *pUsageParam,
				struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat, CVI_U64 mesh_addr,
				CVI_BOOL sync_io, CVI_VOID *pcbParam, CVI_U32 cbParamSize,
				MOD_ID_E enModId, ROTATION_E enRotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "push jobs(%d) for gdc\n", usage);

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
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = DWA_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out)
{
	RECT_S rect;
	CVI_U32 scale = in.u32Height * in.u32Width;
	CVI_U32 ratio_int = MIN2(out.u32Width * in.u32Height, out.u32Height * in.u32Width);
	CVI_U64 height, width;

	//float ratio = MIN2((float)out.u32Width / in.u32Width, (float)out.u32Height / in.u32Height);
	//rect.u32Height = (float)in.u32Height * ratio + 0.5;
	//rect.u32Width = (float)in.u32Width * ratio + 0.5;
	//rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	//rect.s32Y = (out.u32Height - rect.u32Height) >> 1;

	height = (CVI_U64)in.u32Height * ratio_int + scale/2;
	do_div(height, scale);
	rect.u32Height = (CVI_U32)height;

	width = (CVI_U64)in.u32Width * ratio_int + scale/2;
	do_div(width, scale);
	rect.u32Width = (CVI_U32)width;

	rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	rect.s32Y = (out.u32Height - rect.u32Height) >> 1;

	return rect;
}

static CVI_U32 vpss_get_wrap_buffer_size(CVI_U32 u32Width, CVI_U32 u32Height,
	PIXEL_FORMAT_E enPixelFormat, CVI_U32 u32BufLine, CVI_U32 u32BufDepth)
{
	CVI_U32 u32BufSize;
	VB_CAL_CONFIG_S stCalConfig;

	if (u32Width < 64 || u32Height < 64) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "width(%d) or height(%d) too small\n", u32Width, u32Height);
		return 0;
	}
	if (u32BufLine != 64 && u32BufLine != 128) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "u32BufLine(%d) invalid, only 64 or 128 lines\n",
				u32BufLine);
		return 0;
	}

	// 1 only for interal use
	if (!u32BufDepth || u32BufDepth > 32) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "u32BufDepth(%d) invalid, only 1 ~ 31\n",
				u32BufDepth);
		return 0;
	}

	COMMON_GetPicBufferConfig(u32Width, u32Height, enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stCalConfig);

	u32BufSize = stCalConfig.u32VBSize / u32Height;
	u32BufSize *= u32BufLine * u32BufDepth;
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "width(%d), height(%d), u32BufSize=%d\n",
		   u32Width, u32Height, u32BufSize);

	return u32BufSize;
}

/**************************************************************************
 *   Job related APIs.
 **************************************************************************/
CVI_VOID vpss_gdc_callback(CVI_VOID *pParam, VB_BLK blk)
{
	struct _vpss_gdc_cb_param *_pParam = pParam;

	if (!pParam)
		return;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) usage(%d)\n", _pParam->chn.s32DevId,
		       _pParam->chn.s32ChnId, _pParam->usage);
	mutex_unlock(&mesh[_pParam->chn.s32DevId][_pParam->chn.s32ChnId].lock);
	if (blk != VB_INVALID_HANDLE)
		vb_done_handler(_pParam->chn, CHN_TYPE_OUT, blk);
	vfree(pParam);
}

static VPSS_GRP findNextEnGrp(VPSS_GRP workingGrp, CVI_U8 u8VpssDev)
{
	VPSS_GRP i = workingGrp;
	CVI_U8 count = 0;
	struct vb_jobs_t *jobs;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS};

	do {
		++i;
		if (i >= VPSS_MAX_GRP_NUM)
			i = 0;

		if (vpssCtx[i] && vpssCtx[i]->isCreated && vpssCtx[i]->isStarted
			&& (vpssCtx[i]->u8DevId == u8VpssDev)) {
			chn.s32DevId = i;
			jobs = base_get_jobs_by_chn(chn, CHN_TYPE_IN);
			if (!jobs) {
				CVI_TRACE_VPSS(CVI_DBG_INFO, "get jobs failed\n");
				continue;
			}
			if (!down_trylock(&jobs->sem))
				return i;
		}
	} while (++count < VPSS_MAX_GRP_NUM);

	return VPSS_MAX_GRP_NUM;
}

static CVI_U8 getWorkMask(struct cvi_vpss_ctx *ctx)
{
	CVI_U8 mask = 0;
	VPSS_CHN VpssChn;

	if (!ctx->isCreated || !ctx->isStarted)
		return 0;

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		if (!ctx->stChnCfgs[VpssChn].isEnabled)
			continue;
		mask |= BIT(VpssChn);
	}
	if (mask == 0)
		return 0;

	// img's mask
	mask |= BIT(7);

	return mask;
}

static CVI_U8 _vpss_get_chn_mask_by_dev(VPSS_GRP VpssGrp, CVI_U8 devMask)
{
	CVI_U8 chnMask = 0;

	if (stVPSSMode.enMode == VPSS_MODE_SINGLE)
		chnMask = devMask & (BIT(VPSS_MAX_CHN_NUM) - 1);
	else {
		if (vpssCtx[VpssGrp]->u8DevId == 0)
			chnMask = devMask & BIT(0);
		else
			chnMask = (devMask >> 1) & (BIT(VPSS_MAX_CHN_NUM - 1) - 1);
	}
	return chnMask;
}


static CVI_BOOL vpss_enable_handler_ctx(struct vpss_handler_ctx *ctx)
{
	CVI_U8 u8VpssDev = ctx->u8VpssDev;
	int i;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (vpssCtx[i] && vpssCtx[i]->isCreated && vpssCtx[i]->isStarted &&
		    vpssCtx[i]->u8DevId == u8VpssDev)
			return CVI_TRUE;

	return CVI_FALSE;
}

static CVI_VOID _vpss_fill_cvi_buffer(MMF_CHN_S chn, struct vb_s *grp_vb_in,
		uint64_t phy_addr, struct cvi_buffer *buf, struct cvi_vpss_ctx *ctx)
{
	SIZE_S size;
	CVI_BOOL ldc_wa = CVI_FALSE;

	//workaround for ldc 64-align for width/height.
	if (ctx->stChnCfgs[chn.s32ChnId].enRotation != ROTATION_0
		|| ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.bEnable)
		ldc_wa = CVI_TRUE;

	if (ldc_wa) {
		size.u32Width = ALIGN(ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Width, 64);
		size.u32Height = ALIGN(ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Height, 64);
	} else {
		size.u32Width = ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Width;
		size.u32Height = ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Height;
	}
	base_get_frame_info(ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
			   , size
			   , buf
			   , phy_addr
			   , ctx->stChnCfgs[chn.s32ChnId].align);
	buf->s16OffsetTop = 0;
	buf->s16OffsetBottom =
		size.u32Height - ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Height;
	buf->s16OffsetLeft = 0;
	buf->s16OffsetRight =
		size.u32Width - ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Width;

	if (grp_vb_in) {
		buf->u64PTS = grp_vb_in->buf.u64PTS;
		buf->frm_num = grp_vb_in->buf.frm_num;
		buf->motion_lv = grp_vb_in->buf.motion_lv;
		memcpy(buf->motion_table, grp_vb_in->buf.motion_table, MO_TBL_SIZE);
	}
}

static CVI_VOID _vpss_fill_sbm_buffer(CVI_U32 width, PIXEL_FORMAT_E fmt,
	CVI_U32 align, CVI_U32 line, CVI_U32 depth, uint64_t phy_addr,
	struct cvi_buffer *buf)
{
	SIZE_S size;

	memset(buf, 0, sizeof(*buf));
	size.u32Width = width;
	size.u32Height = line * depth;
	base_get_frame_info(fmt, size, buf, phy_addr, align);
}

static CVI_S32 vpss_qbuf(MMF_CHN_S chn, struct vb_s *grp_vb_in,
	VB_BLK chn_vb_blk, struct cvi_vpss_ctx *ctx)
{
	VB_BLK blk = chn_vb_blk;
	struct vb_s *vb = (struct vb_s *)blk;
	struct cvi_buffer *buf = &vb->buf;
	CVI_U64 addr[3];

	memcpy(addr, buf->phy_addr, sizeof(addr));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "save buf-->temp_addr chn(s32DevId=%d, s32ChnId=%d), 0x%llx-0x%llx-0x%llx\n",
		chn.s32DevId, chn.s32ChnId, addr[0], addr[1], addr[2]);

	_vpss_fill_cvi_buffer(chn, grp_vb_in, vb_handle2PhysAddr(blk), buf, ctx);

	if (vb->external) {
		memcpy(buf->phy_addr, addr, sizeof(addr));
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "temp_addr-->buf chn(s32DevId=%d, s32ChnId=%d), 0x%llx-0x%llx-0x%llx\n",
			chn.s32DevId, chn.s32ChnId,
			buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
	}

	if (vb_qbuf(chn, CHN_TYPE_OUT, blk) == CVI_SUCCESS) {
		// Implement cvi_qbuf in user space
		chn.s32ChnId = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
		vpss_sc_qbuf(vip_dev, buf, chn);

	} else
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.s32DevId, chn.s32ChnId);

	vb_release_block(blk);

	return CVI_SUCCESS;
}

static CVI_S32 vpss_online_qbuf(MMF_CHN_S chn)
{
	struct cvi_vpss_ctx *ctx;
	CVI_U8 u8VpssDev;
	VB_BLK blk;

	if (CHECK_VPSS_GRP_VALID(chn.s32DevId))
		return CVI_SUCCESS;
	if (!vpssCtx[chn.s32DevId])
		return CVI_SUCCESS;
	if (!vpssCtx[chn.s32DevId]->stChnCfgs[chn.s32ChnId].isEnabled)
		return CVI_SUCCESS;

	u8VpssDev = vpssCtx[chn.s32DevId]->u8DevId;

	ctx = handler_ctx[u8VpssDev].online_from_isp ? &handler_ctx[u8VpssDev].vpssCtxWork[chn.s32DevId] :
			&handler_ctx[u8VpssDev].vpssCtxWork[0];
	blk = vb_get_block_with_id(ctx->stChnCfgs[chn.s32ChnId].VbPool,
		ctx->stChnCfgs[chn.s32ChnId].blk_size, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
			, chn.s32DevId, chn.s32ChnId);
		return CVI_FAILURE;
	}

	CVI_TRACE_VPSS(CVI_DBG_NOTICE, "Grp(%d) Chn(%d) acquire VB BLK\n", chn.s32DevId, chn.s32ChnId);
	return vpss_qbuf(chn, NULL, blk, ctx);
}

int _vpss_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static CVI_S32 mod_jobs_enque_img_work(CVI_S32 dev_idx, struct cvi_buffer *buf)
{
	struct vpss_img_buffer *cvi_vb2;
	CVI_U8 i;
	struct cvi_img_vdev *idev;

	if (dev_idx >= ARRAY_SIZE(vip_dev->img_vdev)) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "invalid dev_idx(%d)\n", dev_idx);
		return CVI_FAILURE;
	}

	idev = &vip_dev->img_vdev[dev_idx];

	cvi_vb2 = kmalloc(sizeof(*cvi_vb2), GFP_ATOMIC);
	if (!cvi_vb2) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "buf alloc fail\n");
		return CVI_FAILURE;
	}

	INIT_LIST_HEAD(&cvi_vb2->list);

	for (i = 0; i < 3; ++i) {
		cvi_vb2->phy_addr[i] = buf->phy_addr[i];
	}

	if (buf->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
		cvi_vb2->phy_addr[0] = buf->phy_addr[2];
		cvi_vb2->phy_addr[2] = buf->phy_addr[0];
	}

	cvi_vip_buf_queue((struct cvi_base_vdev *)idev, cvi_vb2);

	return CVI_SUCCESS;
}

static CVI_S32 vpss_sb_qbuf(struct cvi_vpss_ctx *ctx, MMF_CHN_S chn, CVI_BOOL sb_vc_ready)
{
	struct VPSS_CHN_CFG *pstChnCfg;
	struct cvi_buffer buf;

	pstChnCfg = &ctx->stChnCfgs[chn.s32ChnId];

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) enabled=%d, sb_enabled=%d, sb_addr=0x%llx\n",
			chn.s32DevId, chn.s32ChnId, pstChnCfg->isEnabled,
			pstChnCfg->stBufWrap.bEnable,
			(unsigned long long)pstChnCfg->bufWrapPhyAddr);

	if (!pstChnCfg->isEnabled)
		return CVI_FAILURE;

	if (!pstChnCfg->stBufWrap.bEnable || !pstChnCfg->bufWrapPhyAddr)
		return CVI_FAILURE;

	_vpss_fill_sbm_buffer(pstChnCfg->stChnAttr.u32Width,
			      pstChnCfg->stChnAttr.enPixelFormat,
			      pstChnCfg->align,
			      pstChnCfg->stBufWrap.u32BufLine,
			      pstChnCfg->u32BufWrapDepth,
			      pstChnCfg->bufWrapPhyAddr,
			      &buf);

	chn.s32ChnId = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
	vpss_sc_sb_qbuf(vip_dev, &buf, chn);
	vpss_sc_set_vc_sbm(vip_dev, chn, sb_vc_ready);

	return CVI_SUCCESS;
}

static CVI_VOID fill_sb_buffers(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx, CVI_BOOL sb_vc_ready)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VPSS_CHN VpssChn;
	struct VPSS_CHN_CFG *stChnCfg;

	// get buffers.
	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		if (!stChnCfg->isEnabled)
			continue;

		chn.s32ChnId = VpssChn;

		// chn buffer from ion
		if (stChnCfg->stBufWrap.bEnable && stChnCfg->bufWrapPhyAddr) {
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) chn buffer from ion.\n", VpssGrp, VpssChn);
			vpss_sb_qbuf(ctx, chn, sb_vc_ready);
			continue;
		}
	}
}

static CVI_S32 fill_buffers(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx, bool online_from_isp)
{
	VB_BLK blk[VPSS_MAX_CHN_NUM] = { [0 ... VPSS_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	VB_BLK blk_grp;
	//struct vdev *d;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	CVI_S32 ret = CVI_SUCCESS;
	struct vb_s *vb_in = NULL;
	VPSS_CHN VpssChn = 0;
	struct VPSS_CHN_CFG *stChnCfg;
	CVI_S32 dev_idx;
	struct cvi_buffer *buf;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), online_from_isp=%d, grp_sb_mode=%d\n",
			VpssGrp, online_from_isp, vpssExtCtx[VpssGrp].sbm_cfg.sb_mode);

	if (!online_from_isp && !vpssExtCtx[VpssGrp].sbm_cfg.sb_mode && base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN))
		return CVI_ERR_VPSS_BUF_EMPTY;

	// get buffers.
	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		if (!stChnCfg->isEnabled)
			continue;
		if (stChnCfg->stBufWrap.bEnable && stChnCfg->bufWrapPhyAddr)
			continue;

		chn.s32ChnId = VpssChn;

		// chn buffer from user
		if (!base_mod_jobs_waitq_empty(chn, CHN_TYPE_OUT)) {
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) chn buffer from user.\n", VpssGrp, VpssChn);

			buf = base_mod_jobs_enque_work(chn, CHN_TYPE_OUT);
			if (!buf) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", VpssGrp, VpssChn);
				ret = CVI_ERR_VPSS_NOTREADY;
				break;
			}

			// Implement cvi_qbuf in user space
			chn.s32ChnId = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
			vpss_sc_qbuf(vip_dev, buf, chn);
			continue;
		}

		// chn buffer from pool
		blk[VpssChn] = vb_get_block_with_id(ctx->stChnCfgs[VpssChn].VbPool,
						ctx->stChnCfgs[VpssChn].blk_size, CVI_ID_VPSS);
		if (blk[VpssChn] == VB_INVALID_HANDLE) {
			if (online_from_isp) {
				vb_acquire_block(vpss_online_qbuf, chn,
					ctx->stChnCfgs[VpssChn].blk_size, ctx->stChnCfgs[VpssChn].VbPool);
				CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) acquire VB BLK later\n"
					, VpssGrp, VpssChn);
			} else {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
					, VpssGrp, VpssChn);
				ret = CVI_ERR_VPSS_NOBUF;
				break;
			}
		}
	}
	if (ret != CVI_SUCCESS)
		goto ERR_FILL_BUF;

	if (!online_from_isp && !vpssExtCtx[VpssGrp].sbm_cfg.sb_mode) {
		struct cvi_buffer *buf =
			base_mod_jobs_enque_work(chn, CHN_TYPE_IN);
		if (buf == NULL) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) qbuf failed.\n", VpssGrp);

			ret = CVI_ERR_VPSS_NOTREADY;
			goto ERR_FILL_BUF;
		}

		CVI_TRACE_VPSS(CVI_DBG_INFO, "buf: 0x%lx-0x%lx-0x%lx\n",
			(unsigned long)buf->phy_addr[0], (unsigned long)buf->phy_addr[1],
			(unsigned long)buf->phy_addr[2]);

		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);
		if (mod_jobs_enque_img_work(dev_idx, buf) != CVI_SUCCESS) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) qbuf failed.\n", VpssGrp);
			ret = CVI_ERR_VPSS_NOTREADY;
			goto ERR_FILL_BUF;
		}

		// FIXME: assume only one in work queue
		vb_in = container_of(buf, struct vb_s, buf);
	} else if (!online_from_isp && vpssExtCtx[VpssGrp].sbm_cfg.sb_mode) {
		struct cvi_buffer buf;
		struct vpss_grp_sbm_cfg *sbm_cfg = &vpssExtCtx[VpssGrp].sbm_cfg;
		SIZE_S size = {
			.u32Width = ctx->stGrpAttr.u32MaxW,
			.u32Height = ctx->stGrpAttr.u32MaxH};

		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);
		if (dev_idx >= ARRAY_SIZE(vip_dev->img_vdev)) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "invalid img dev_idx(%d)\n", dev_idx);
			goto ERR_FILL_BUF;
		}

		size.u32Height = ((sbm_cfg->sb_size + 1) * 64) * (sbm_cfg->sb_nb + 1);
		base_get_frame_info(ctx->stGrpAttr.enPixelFormat, size, &buf,
				    vpssExtCtx[VpssGrp].sbm_cfg.ion_paddr, 64);
		vpss_img_sb_qbuf(&vip_dev->img_vdev[dev_idx], &buf, &vpssExtCtx[VpssGrp].sbm_cfg);
	}

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		if (blk[VpssChn] == VB_INVALID_HANDLE)
			continue;
		chn.s32ChnId = VpssChn;

		vpss_qbuf(chn, vb_in, blk[VpssChn], ctx);
	}

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		if (!stChnCfg->isEnabled)
			continue;
		if (!stChnCfg->stBufWrap.bEnable || !stChnCfg->bufWrapPhyAddr)
			continue;

	}

	return ret;
ERR_FILL_BUF:
	while ((VpssChn > 0) && (--VpssChn < ctx->chnNum)) {
		if (blk[VpssChn] != VB_INVALID_HANDLE)
			vb_release_block(blk[VpssChn]);
	}
	blk_grp = base_mod_jobs_waitq_pop(chn, CHN_TYPE_IN);
	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	return ret;
}

static CVI_S32 _vb_dqbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK *blk)
{
	struct vb_s *p;
	*blk = VB_INVALID_HANDLE;

	// get vb from workq which is done.
	if (base_mod_jobs_workq_empty(chn, chn_type)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) ChnId(%d) No more vb for dequeue.\n",
			       chn.enModId, chn.s32ChnId);
		return CVI_FAILURE;
	}
	p = (struct vb_s *)base_mod_jobs_workq_pop(chn, chn_type);
	if (!p)
		return CVI_FAILURE;

	*blk = (VB_BLK)p;
	p->mod_ids &= ~BIT(chn.enModId);

	return CVI_SUCCESS;
}

static void release_online_sc_buffers(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	VPSS_CHN VpssChn;
	CVI_U8 dev_idx;
	struct VPSS_CHN_CFG *pstChnCfg;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		pstChnCfg = &ctx->stChnCfgs[VpssChn];
		if (!pstChnCfg->isEnabled)
			continue;
		if (pstChnCfg->stBufWrap.bEnable)
			continue;
		chn.s32ChnId = VpssChn;
		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
		cvi_sc_buf_remove_all(&vip_dev->sc_vdev[dev_idx], VpssGrp);
	}
}

static void release_buffers(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VB_BLK blk;
	VPSS_CHN VpssChn;

	if (!handler_ctx[ctx->u8DevId].online_from_isp &&
	    !vpssExtCtx[VpssGrp].sbm_cfg.sb_mode) {
		chn.s32ChnId = 0;
		vb_dqbuf(chn, CHN_TYPE_IN, &blk);
		if (blk != VB_INVALID_HANDLE)
			vb_release_block(blk);
	}

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		struct VPSS_CHN_CFG *stChnCfg = &ctx->stChnCfgs[VpssChn];

		if (!stChnCfg->isEnabled)
			continue;
		if (stChnCfg->stBufWrap.bEnable)
			continue;

		chn.s32ChnId = VpssChn;
		vb_cancel_block(chn, ctx->stChnCfgs[VpssChn].blk_size, ctx->stChnCfgs[VpssChn].VbPool);
		while (!base_mod_jobs_workq_empty(chn, CHN_TYPE_OUT)) {
			_vb_dqbuf(chn, CHN_TYPE_OUT, &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}
}

static CVI_S32 hw_start(VPSS_GRP VpssGrp, CVI_BOOL onoff, CVI_BOOL isForce, struct cvi_vpss_ctx *ctx)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	CVI_S32 dev_idx;
	CVI_S32 s32Ret = CVI_SUCCESS;
	VPSS_CHN VpssChn;

	if (onoff) {
		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);

		CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d), img(%d:%d:%d)\n",
			VpssGrp, dev_idx,
			vip_dev->img_vdev[dev_idx].img_type,
			vip_dev->img_vdev[dev_idx].input_type);

		for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
			chn.s32ChnId = VpssChn;
			dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
			s32Ret = cvi_sc_streamon(&vip_dev->sc_vdev[dev_idx]);
			if (s32Ret != CVI_SUCCESS)
				return s32Ret;
		}

		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);
		cvi_img_streamon(&vip_dev->img_vdev[dev_idx]);
	} else {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), hw stop, Caller is %pS\n",
			VpssGrp, __builtin_return_address(0));

		for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
			chn.s32ChnId = VpssChn;
			dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
			s32Ret = cvi_sc_streamoff(&vip_dev->sc_vdev[dev_idx], isForce);
			if (s32Ret != CVI_SUCCESS)
				return s32Ret;
		}

		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);
		s32Ret = cvi_img_streamoff(&vip_dev->img_vdev[dev_idx], isForce);
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	return s32Ret;
}

static void hw_reset(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx, CVI_BOOL bToggleReset)
{
	union vip_sys_reset val;

	release_buffers(VpssGrp, ctx);

	if (bToggleReset) {
		val.raw = 0;
		val.b.img_d = 1;
		val.b.img_v = 1;
		val.b.sc_top = 1;
		val.b.sc_d = 1;
		val.b.sc_v1 = 1;
		val.b.sc_v2 = 1;
		val.b.sc_v3 = 1;
		vip_toggle_reset(val);
	}

	hw_start(VpssGrp, CVI_FALSE, bToggleReset, ctx);
}

static CVI_VOID _vpss_online_set_mlv_info(struct vb_s *blk)
{
	CVI_U8 snr_num = blk->buf.dev_num;

	blk->buf.motion_lv = g_mlv_i[snr_num].mlv_i_level;
	memcpy(blk->buf.motion_table, g_mlv_i[snr_num].mlv_i_table, sizeof(blk->buf.motion_table));
}

static CVI_S32 _vpss_online_get_dpcm_wr_crop(CVI_U8 snr_num,
	RECT_S *dpcm_wr_crop, SIZE_S src_size)
{
#if 0
	struct crop_size crop = g_dpcm_wr_i.dpcm_wr_i_crop[snr_num];
	CVI_S32 bRet = CVI_SUCCESS;

	if (g_dpcm_wr_i.dpcm_wr_i_dpcmon) {
		// check if dpcm_wr crop valid
		if (crop.end_x <= crop.start_x ||
			crop.end_y <= crop.start_y ||
			crop.end_x > src_size.u32Width ||
			crop.end_y > src_size.u32Height ||
			((CVI_U32)(crop.end_x - crop.start_x) == src_size.u32Width &&
			(CVI_U32)(crop.end_y - crop.start_y) == src_size.u32Height))
			bRet = CVI_ERR_VPSS_ILLEGAL_PARAM;
		else {
			dpcm_wr_crop->s32X = (CVI_S32)crop.start_x;
			dpcm_wr_crop->s32Y = (CVI_S32)crop.start_y;
			dpcm_wr_crop->u32Width = (CVI_U32)(crop.end_x - crop.start_x);
			dpcm_wr_crop->u32Height = (CVI_U32)(crop.end_y - crop.start_y);
		}
	} else
		bRet = CVI_ERR_VPSS_NOT_PERM;

	return bRet;
#else
	if (snr_num >= VPSS_MAX_GRP_NUM || !dpcm_wr_crop || !src_size.u32Width)
		return CVI_FAILURE;

	return CVI_ERR_VPSS_NOT_PERM;
#endif
}

/*
 * _vpss_get_union_crop() - get union crop area of cropA & cropB.
 * If two crop area has no union, return cropA
 */
static RECT_S _vpss_get_union_crop(RECT_S cropA, RECT_S cropB)
{
	RECT_S union_crop;

	// check if no union
	if ((cropA.s32X >= cropB.s32X + (CVI_S32)cropB.u32Width) ||
		(cropA.s32Y >= cropB.s32Y + (CVI_S32)cropB.u32Height) ||
		(cropB.s32X >= cropA.s32X + (CVI_S32)cropA.u32Width) ||
		(cropB.s32Y >= cropA.s32Y + (CVI_S32)cropA.u32Height))
		return cropA;

	union_crop.s32X = (cropA.s32X > cropB.s32X) ? cropA.s32X : cropB.s32X;
	union_crop.s32Y = (cropA.s32Y > cropB.s32Y) ? cropA.s32Y : cropB.s32Y;
	union_crop.u32Width =
		((cropA.s32X + (CVI_S32)cropA.u32Width) < (cropB.s32X + (CVI_S32)cropB.u32Width)) ?
		(CVI_U32)(cropA.s32X + (CVI_S32)cropA.u32Width - union_crop.s32X) :
		(CVI_U32)(cropB.s32X + (CVI_S32)cropB.u32Width - union_crop.s32X);
	union_crop.u32Height =
		((cropA.s32Y + (CVI_S32)cropA.u32Height) < (cropB.s32Y + (CVI_S32)cropB.u32Height)) ?
		(CVI_U32)(cropA.s32Y + (CVI_S32)cropA.u32Height - union_crop.s32Y) :
		(CVI_U32)(cropB.s32Y + (CVI_S32)cropB.u32Height - union_crop.s32Y);

	return union_crop;
}

/* _is_frame_crop_changed() - to see if frame's crop info changed
 */
static CVI_BOOL _is_frame_crop_changed(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	struct vb_s *vb_in = NULL;
	CVI_BOOL ret = CVI_FALSE;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	struct vb_jobs_t *jobs;

	//if (mod_jobs_waitq_empty(&vpssCtx[VpssGrp]->vb_jobs))
	//	return false;
	//FIFO_GET_TAIL(&vpssCtx[VpssGrp]->vb_jobs.waitq, &vb_in);
	if (base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN))
		return CVI_FALSE;

	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_IN);
	FIFO_GET_TAIL(&jobs->waitq, &vb_in);

	if (vb_in == NULL) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) unexpected empty waitq\n", VpssGrp);
		return CVI_FALSE;
	}

	if ((ctx->s16OffsetLeft != vb_in->buf.s16OffsetLeft) ||
		(ctx->s16OffsetTop != vb_in->buf.s16OffsetTop) ||
		(ctx->s16OffsetRight != vb_in->buf.s16OffsetRight) ||
		(ctx->s16OffsetBottom != vb_in->buf.s16OffsetBottom)) {
		vpssCtx[VpssGrp]->s16OffsetLeft = ctx->s16OffsetLeft = vb_in->buf.s16OffsetLeft;
		vpssCtx[VpssGrp]->s16OffsetTop = ctx->s16OffsetTop = vb_in->buf.s16OffsetTop;
		vpssCtx[VpssGrp]->s16OffsetRight = ctx->s16OffsetRight = vb_in->buf.s16OffsetRight;
		vpssCtx[VpssGrp]->s16OffsetBottom = ctx->s16OffsetBottom = vb_in->buf.s16OffsetBottom;
		ret = CVI_TRUE;
	}

	if (memcmp(&vb_in->buf.frame_crop, &ctx->frame_crop, sizeof(vb_in->buf.frame_crop))) {
		CVI_BOOL chk_width_even = IS_FMT_YUV420(ctx->stGrpAttr.enPixelFormat) ||
				      IS_FMT_YUV422(ctx->stGrpAttr.enPixelFormat);
		CVI_BOOL chk_height_even = IS_FMT_YUV420(ctx->stGrpAttr.enPixelFormat);

		if (chk_width_even && ((vb_in->buf.frame_crop.end_x - vb_in->buf.frame_crop.start_x) & 0x01)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VpssGrp(%d) frame-crop invalid - start_x(%d) end_x(%d)\n",
				       VpssGrp, vb_in->buf.frame_crop.start_x, vb_in->buf.frame_crop.end_x);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "frame-crop's width should be even for yuv format\n");
			return ret;
		}
		if (chk_height_even && ((vb_in->buf.frame_crop.end_y - vb_in->buf.frame_crop.start_y) & 0x01)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VpssGrp(%d) frame-crop invalid - start_y(%d) end_y(%d)\n",
				       VpssGrp, vb_in->buf.frame_crop.start_y, vb_in->buf.frame_crop.end_y);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "frame-crop's height should be even for yuv format\n");
			return ret;
		}

		ctx->frame_crop = vb_in->buf.frame_crop;
		vpssCtx[VpssGrp]->frame_crop = ctx->frame_crop;
		ret = CVI_TRUE;
	}
	return ret;
}

/* _is_frame_crop_valid() - to see if frame's crop info valid/enabled
 */
static bool _is_frame_crop_valid(struct cvi_vpss_ctx *ctx)
{
	return (ctx->frame_crop.end_x > ctx->frame_crop.start_x &&
		ctx->frame_crop.end_y > ctx->frame_crop.start_y &&
		ctx->frame_crop.end_x <= ctx->stGrpAttr.u32MaxW &&
		ctx->frame_crop.end_y <= ctx->stGrpAttr.u32MaxH &&
		!((CVI_U32)(ctx->frame_crop.end_x - ctx->frame_crop.start_x)
			== ctx->stGrpAttr.u32MaxW &&
		  (CVI_U32)(ctx->frame_crop.end_y - ctx->frame_crop.start_y)
			== ctx->stGrpAttr.u32MaxH));
}

static void _vpss_over_crop_resize
	(struct VPSS_GRP_HW_CFG *pstGrpHwCfg, RECT_S crop_rect, RECT_S *resize_rect)
{
	CVI_U32 scale = crop_rect.u32Width * crop_rect.u32Height;
	CVI_U32 ratio;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "rect_crop (l=%d, t=%d, w=%d, h=%d)\n",
			pstGrpHwCfg->rect_crop.left,
			pstGrpHwCfg->rect_crop.top,
			pstGrpHwCfg->rect_crop.width,
			pstGrpHwCfg->rect_crop.height);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "rect before resize(%d, %d, %d, %d)\n"
				, resize_rect->s32X, resize_rect->s32Y
				, resize_rect->u32Width, resize_rect->u32Height);

	if (crop_rect.s32X < 0) {
		//ratio = (float)ABS(crop_rect.s32X) / crop_rect.u32Width;
		//resize_rect->s32X += (CVI_S32)(resize_rect->u32Width * ratio + 0.5);
		//resize_rect->u32Width -= (CVI_U32)(resize_rect->u32Width * ratio + 0.5);
		ratio = ABS(crop_rect.s32X) * crop_rect.u32Height;
		resize_rect->s32X += (CVI_S32)(resize_rect->u32Width * ratio + scale / 2) / scale;
		resize_rect->u32Width -= (CVI_U32)(resize_rect->u32Width * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32X + crop_rect.u32Width > pstGrpHwCfg->rect_crop.width) {
		//ratio = (float)(crop_rect.s32X + crop_rect.u32Width - pstGrpHwCfg->rect_crop.width)
		//	/ (crop_rect.u32Width);
		//resize_rect->u32Width -= (CVI_U32)(resize_rect->u32Width * ratio + 0.5);
		ratio = (crop_rect.s32X + crop_rect.u32Width - pstGrpHwCfg->rect_crop.width)
			* (crop_rect.u32Height);
		resize_rect->u32Width -= (CVI_U32)(resize_rect->u32Width * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32Y < 0) {
		//ratio = (float)ABS(crop_rect.s32Y) / crop_rect.u32Height;
		//resize_rect->s32Y += (CVI_S32)(resize_rect->u32Height * ratio + 0.5);
		//resize_rect->u32Height -= (CVI_U32)(resize_rect->u32Height * ratio + 0.5);
		ratio = ABS(crop_rect.s32Y) * crop_rect.u32Width;
		resize_rect->s32Y += (CVI_S32)(resize_rect->u32Height * ratio + scale / 2) / scale;
		resize_rect->u32Height -= (CVI_U32)(resize_rect->u32Height * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32Y + crop_rect.u32Height > pstGrpHwCfg->rect_crop.height) {
		//ratio = (float)(crop_rect.s32Y + crop_rect.u32Height - pstGrpHwCfg->rect_crop.height)
		//	/ (crop_rect.u32Height);
		//resize_rect->u32Height -= (CVI_U32)(resize_rect->u32Height * ratio + 0.5);
		ratio = (crop_rect.s32Y + crop_rect.u32Height - pstGrpHwCfg->rect_crop.height)
			* (crop_rect.u32Width);
		resize_rect->u32Height -= (CVI_U32)(resize_rect->u32Height * ratio + scale / 2) / scale;
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "rect after resize(%d, %d, %d, %d)\n"
			, resize_rect->s32X, resize_rect->s32Y
			, resize_rect->u32Width, resize_rect->u32Height);
}

/*
 * @param VpssGrp: VPSS Grp to update cfg
 * @param VpssChn: VPSS Chn to update cfg
 * @param ctx: VPSS ctx which records settings of this grp
 * @param pstGrpHwCfg: cfg to be updated
 */
void _vpss_chn_hw_cfg_update(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_vpss_ctx *ctx,
	struct VPSS_GRP_HW_CFG *pstGrpHwCfg, bool online_from_isp)
{
	struct VPSS_CHN_CFG *stChnCfg = &ctx->stChnCfgs[VpssChn];
	struct VPSS_CHN_HW_CFG *pstHwCfg = &pstGrpHwCfg->stChnCfgs[VpssChn];
	VB_CAL_CONFIG_S stVbCalConfig;
	RECT_S chnCrop = stChnCfg->stCropInfo.stCropRect;
	CVI_BOOL bCropOverSrcRange = CVI_FALSE;
	struct sclr_csc_matrix *mtrx;
	int i;

	if (!ctx->is_cfg_changed && !stChnCfg->is_cfg_changed)
		return;
	stChnCfg->is_cfg_changed = CVI_FALSE;

	COMMON_GetPicBufferConfig(stChnCfg->stChnAttr.u32Width, stChnCfg->stChnAttr.u32Height,
		stChnCfg->stChnAttr.enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, stChnCfg->align, &stVbCalConfig);
	pstHwCfg->bytesperline[0] = stVbCalConfig.u32MainStride;
	pstHwCfg->bytesperline[1] = stVbCalConfig.u32CStride;

	if (stChnCfg->stCropInfo.bEnable) {
		//CVI_FLOAT h_ratio = 1.0f, v_ratio = 1.0f;
		CVI_U32 scale = ctx->stGrpAttr.u32MaxW * ctx->stGrpAttr.u32MaxH;
		CVI_U32 h_ratio = scale, v_ratio = scale;
		CVI_U64 u64Temp;

		if (!online_from_isp) {
			// use ratio-coordinate if dis enabled.
			if (_is_frame_crop_valid(ctx)) {
				//h_ratio = (CVI_FLOAT)pstGrpHwCfg->rect_crop.width / ctx->stGrpAttr.u32MaxW;
				//v_ratio = (CVI_FLOAT)pstGrpHwCfg->rect_crop.height / ctx->stGrpAttr.u32MaxH;
				h_ratio = pstGrpHwCfg->rect_crop.width * ctx->stGrpAttr.u32MaxH;
				v_ratio = pstGrpHwCfg->rect_crop.height * ctx->stGrpAttr.u32MaxW;
			}
		} else {
			RECT_S dpcm_wr_crop;
			SIZE_S chn_src_size;
			CVI_S32 bRet;

			chn_src_size.u32Width = pstGrpHwCfg->rect_crop.width;
			chn_src_size.u32Height = pstGrpHwCfg->rect_crop.height;
			bRet = _vpss_online_get_dpcm_wr_crop(VpssGrp, &dpcm_wr_crop, chn_src_size);
			if (bRet == CVI_SUCCESS)
				chnCrop = _vpss_get_union_crop(dpcm_wr_crop, chnCrop);
		}
		//pstHwCfg->rect_crop.left = chnCrop.s32X = chnCrop.s32X * h_ratio;
		//pstHwCfg->rect_crop.top = chnCrop.s32Y = chnCrop.s32Y * v_ratio;
		//pstHwCfg->rect_crop.width = chnCrop.u32Width = chnCrop.u32Width * h_ratio;
		//pstHwCfg->rect_crop.height = chnCrop.u32Height = chnCrop.u32Height * v_ratio;
		u64Temp = chnCrop.s32X * (CVI_S64)h_ratio;
		do_div(u64Temp, scale);
		pstHwCfg->rect_crop.left = chnCrop.s32X = (CVI_S32)u64Temp;

		u64Temp = chnCrop.s32Y * (CVI_S64)v_ratio;
		do_div(u64Temp, scale);
		pstHwCfg->rect_crop.top = chnCrop.s32Y = (CVI_S32)u64Temp;

		u64Temp = chnCrop.u32Width * (CVI_S64)h_ratio;
		do_div(u64Temp, scale);
		pstHwCfg->rect_crop.width = chnCrop.u32Width = (CVI_U32)u64Temp;

		u64Temp = chnCrop.u32Height * (CVI_S64)v_ratio;
		do_div(u64Temp, scale);
		pstHwCfg->rect_crop.height = chnCrop.u32Height = (CVI_U32)u64Temp;

		// check if crop rect contains the region outside input src
		if (chnCrop.s32X < 0) {
			pstHwCfg->rect_crop.left = 0;
			pstHwCfg->rect_crop.width = (chnCrop.u32Width - ABS(chnCrop.s32X));
			bCropOverSrcRange = CVI_TRUE;
		}
		if (chnCrop.s32X + chnCrop.u32Width > pstGrpHwCfg->rect_crop.width) {
			pstHwCfg->rect_crop.width = pstGrpHwCfg->rect_crop.width - pstHwCfg->rect_crop.left;
			bCropOverSrcRange = CVI_TRUE;
		}

		if (chnCrop.s32Y < 0) {
			pstHwCfg->rect_crop.top = 0;
			pstHwCfg->rect_crop.height = (chnCrop.u32Height - ABS(chnCrop.s32Y));
			bCropOverSrcRange = CVI_TRUE;
		}
		if (chnCrop.s32Y + chnCrop.u32Height > pstGrpHwCfg->rect_crop.height) {
			pstHwCfg->rect_crop.height = pstGrpHwCfg->rect_crop.height - pstHwCfg->rect_crop.top;
			bCropOverSrcRange = CVI_TRUE;
		}
	} else {
		pstHwCfg->rect_crop.left = pstHwCfg->rect_crop.top
			= chnCrop.s32X = chnCrop.s32Y = 0;
		pstHwCfg->rect_crop.width = chnCrop.u32Width = pstGrpHwCfg->rect_crop.width;
		pstHwCfg->rect_crop.height = chnCrop.u32Height = pstGrpHwCfg->rect_crop.height;
		if (online_from_isp) {
			RECT_S dpcm_wr_crop;
			SIZE_S chn_src_size;
			CVI_S32 bRet;

			chn_src_size.u32Width = pstGrpHwCfg->rect_crop.width;
			chn_src_size.u32Height = pstGrpHwCfg->rect_crop.height;
			bRet = _vpss_online_get_dpcm_wr_crop(VpssGrp, &dpcm_wr_crop, chn_src_size);
			if (bRet == CVI_SUCCESS) {
				pstHwCfg->rect_crop.left = chnCrop.s32X = dpcm_wr_crop.s32X;
				pstHwCfg->rect_crop.top = chnCrop.s32Y = dpcm_wr_crop.s32Y;
				pstHwCfg->rect_crop.width = chnCrop.u32Width = dpcm_wr_crop.u32Width;
				pstHwCfg->rect_crop.height = chnCrop.u32Height = dpcm_wr_crop.u32Height;
			}
		}
	}
	CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) chn(%d) rect(%d %d %d %d)\n", VpssGrp, VpssChn
			, pstHwCfg->rect_crop.left, pstHwCfg->rect_crop.top
			, pstHwCfg->rect_crop.width, pstHwCfg->rect_crop.height);

	if (stChnCfg->stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_AUTO) {
		SIZE_S in, out;
		RECT_S rect;
		CVI_BOOL is_border_enabled = CVI_FALSE;

		in.u32Width = chnCrop.u32Width;
		in.u32Height = chnCrop.u32Height;
		out.u32Width = stChnCfg->stChnAttr.u32Width;
		out.u32Height = stChnCfg->stChnAttr.u32Height;
		rect = aspect_ratio_resize(in, out);

		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		is_border_enabled = stChnCfg->stChnAttr.stAspectRatio.bEnableBgColor
			&& ((rect.u32Width != stChnCfg->stChnAttr.u32Width)
			 || (rect.u32Height != stChnCfg->stChnAttr.u32Height));

		CVI_TRACE_VPSS(CVI_DBG_INFO, "input(%d %d) output(%d %d)\n"
				, in.u32Width, in.u32Height, out.u32Width, out.u32Height);
		CVI_TRACE_VPSS(CVI_DBG_INFO, "ratio (%d %d %d %d) border_enabled(%d)\n"
				, rect.s32X, rect.s32Y, rect.u32Width, rect.u32Height, is_border_enabled);

		pstHwCfg->border_param.enable = is_border_enabled;
		pstHwCfg->border_param.offset_x = rect.s32X;
		pstHwCfg->border_param.offset_y = rect.s32Y;
		pstHwCfg->border_param.bg_color[2] = stChnCfg->stChnAttr.stAspectRatio.u32BgColor & 0xff;
		pstHwCfg->border_param.bg_color[1] = (stChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 8) & 0xff;
		pstHwCfg->border_param.bg_color[0] = (stChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 16) & 0xff;

		if (is_border_enabled) {
			pstHwCfg->rect_output.left = pstHwCfg->rect_output.top = 0;
		} else {
			pstHwCfg->rect_output.left = rect.s32X;
			pstHwCfg->rect_output.top = rect.s32Y;
		}


		if (IS_FMT_YUV420(stChnCfg->stChnAttr.enPixelFormat)
			|| IS_FMT_YUV422(stChnCfg->stChnAttr.enPixelFormat)) {
			pstHwCfg->rect_output.width = rect.u32Width & ~0x01;
			pstHwCfg->rect_output.left &= ~0x01;
		} else
			pstHwCfg->rect_output.width = rect.u32Width;

		if (IS_FMT_YUV420(stChnCfg->stChnAttr.enPixelFormat))
			pstHwCfg->rect_output.height = rect.u32Height & ~0x01;
		else
			pstHwCfg->rect_output.height = rect.u32Height;
	} else if (stChnCfg->stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
		RECT_S rect = stChnCfg->stChnAttr.stAspectRatio.stVideoRect;
		CVI_BOOL is_border_enabled = CVI_FALSE;

		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		is_border_enabled = stChnCfg->stChnAttr.stAspectRatio.bEnableBgColor
			&& ((rect.u32Width != stChnCfg->stChnAttr.u32Width)
			 || (rect.u32Height != stChnCfg->stChnAttr.u32Height));

		CVI_TRACE_VPSS(CVI_DBG_INFO, "rect(%d %d %d %d) border_enabled(%d)\n"
				, rect.s32X, rect.s32Y, rect.u32Width, rect.u32Height, is_border_enabled);

		if (is_border_enabled) {
			pstHwCfg->rect_output.left = pstHwCfg->rect_output.top = 0;
		} else {
			pstHwCfg->rect_output.left = rect.s32X;
			pstHwCfg->rect_output.top = rect.s32Y;
		}
		pstHwCfg->rect_output.width = rect.u32Width;
		pstHwCfg->rect_output.height = rect.u32Height;

		pstHwCfg->border_param.enable = is_border_enabled;
		pstHwCfg->border_param.offset_x = rect.s32X;
		pstHwCfg->border_param.offset_y = rect.s32Y;
		pstHwCfg->border_param.bg_color[2] = stChnCfg->stChnAttr.stAspectRatio.u32BgColor & 0xff;
		pstHwCfg->border_param.bg_color[1] = (stChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 8) & 0xff;
		pstHwCfg->border_param.bg_color[0] = (stChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 16) & 0xff;
	} else {
		RECT_S rect;

		rect.s32X = rect.s32Y = 0;
		rect.u32Width = stChnCfg->stChnAttr.u32Width;
		rect.u32Height = stChnCfg->stChnAttr.u32Height;
		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		pstHwCfg->rect_output.left = pstHwCfg->rect_output.top = 0;
		pstHwCfg->rect_output.width = rect.u32Width;
		pstHwCfg->rect_output.height = rect.u32Height;
		if (bCropOverSrcRange) {
			pstHwCfg->border_param.enable = CVI_TRUE;
			pstHwCfg->border_param.offset_x = rect.s32X;
			pstHwCfg->border_param.offset_y = rect.s32Y;
			pstHwCfg->border_param.bg_color[2] = 0;
			pstHwCfg->border_param.bg_color[1] = 0;
			pstHwCfg->border_param.bg_color[0] = 0;
		} else {
			pstHwCfg->border_param.enable = CVI_FALSE;
		}
	}

	if (pstHwCfg->rect_output.width * VPSS_MAX_ZOOMOUT  < pstHwCfg->rect_crop.width
		|| pstHwCfg->rect_output.height * VPSS_MAX_ZOOMOUT  < pstHwCfg->rect_crop.height) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "zoom out over %d times, sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, VPSS_MAX_ZOOMOUT, pstHwCfg->rect_crop.width, pstHwCfg->rect_crop.height
			, pstHwCfg->rect_output.width, pstHwCfg->rect_output.height);

		pstHwCfg->rect_crop.width = pstHwCfg->rect_output.width * VPSS_MAX_ZOOMOUT;
		pstHwCfg->rect_crop.height = pstHwCfg->rect_output.height * VPSS_MAX_ZOOMOUT;
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Modify to sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, pstHwCfg->rect_crop.width, pstHwCfg->rect_crop.height
			, pstHwCfg->rect_output.width, pstHwCfg->rect_output.height);
	}

	pstHwCfg->quantCfg.enable = stChnCfg->stChnAttr.stNormalize.bEnable;
	if (stChnCfg->stChnAttr.stNormalize.bEnable) {
		CVI_U8 i;
		struct vpss_int_normalize *int_norm =
			(struct vpss_int_normalize *)&ctx->stChnCfgs[VpssChn].stChnAttr.stNormalize;

		for (i = 0; i < 3; i++) {
			pstHwCfg->quantCfg.sc_frac[i] = int_norm->sc_frac[i];
			pstHwCfg->quantCfg.sub[i] = int_norm->sub[i];
			pstHwCfg->quantCfg.sub_frac[i] = int_norm->sub_frac[i];
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_frac(0x%x, 0x%x, 0x%x)\n",
			pstHwCfg->quantCfg.sc_frac[0],
			pstHwCfg->quantCfg.sc_frac[1],
			pstHwCfg->quantCfg.sc_frac[2]);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub(0x%x, 0x%x, 0x%x), sub_frac(0x%x, 0x%x, 0x%x)\n",
			pstHwCfg->quantCfg.sub[0],
			pstHwCfg->quantCfg.sub[1],
			pstHwCfg->quantCfg.sub[2],
			pstHwCfg->quantCfg.sub_frac[0],
			pstHwCfg->quantCfg.sub_frac[1],
			pstHwCfg->quantCfg.sub_frac[2]);

		pstHwCfg->quantCfg.rounding = (enum cvi_sc_quant_rounding)stChnCfg->stChnAttr.stNormalize.rounding;
	} else {
#if 0
		static CVI_FLOAT csc[3][3] = {
			{ 0.299,  0.587,  0.114},
			{-0.169, -0.331,  0.500},
			{ 0.500, -0.419, -0.081},
		};

		pstHwCfg->csc_cfg.sub[0] = 0;
		pstHwCfg->csc_cfg.sub[1] = 0;
		pstHwCfg->csc_cfg.sub[2] = 0;
		pstHwCfg->csc_cfg.add[0] = 0;
		pstHwCfg->csc_cfg.add[1] = 128;
		pstHwCfg->csc_cfg.add[2] = 128;

		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j) {
				CVI_FLOAT tmpf = ABS(csc[i][j]) * BIT(10) + 0.5;
				CVI_U16 tmp;

				if (i == 0)
					tmp = (CVI_U16)MIN2(tmpf * ctx->stChnCfgs[VpssChn].YRatio, BIT(12));
				else
					tmp = (CVI_U16)tmpf;
				pstHwCfg->csc_cfg.coef[i][j] =
					(csc[i][j] >= 0) ? tmp : (tmp | BIT(13));
			}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[0][0]
			, pstHwCfg->csc_cfg.coef[0][1]
			, pstHwCfg->csc_cfg.coef[0][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[1][0]
			, pstHwCfg->csc_cfg.coef[1][1]
			, pstHwCfg->csc_cfg.coef[1][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[2][0]
			, pstHwCfg->csc_cfg.coef[2][1]
			, pstHwCfg->csc_cfg.coef[2][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
			, pstHwCfg->csc_cfg.sub[0]
			, pstHwCfg->csc_cfg.sub[1]
			, pstHwCfg->csc_cfg.sub[2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
			, pstHwCfg->csc_cfg.add[0]
			, pstHwCfg->csc_cfg.add[1]
			, pstHwCfg->csc_cfg.add[2]);
#endif

		// use designer provided table
		mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_RGB2YUV);
		for (i = 0; i < 3; i++)
			pstHwCfg->csc_cfg.coef[0][i] =
				(mtrx->coef[0][i] * ctx->stChnCfgs[VpssChn].YRatio) / YRATIO_SCALE;

		//pstHwCfg->csc_cfg.coef[0][0] = mtrx->coef[0][0];
		//pstHwCfg->csc_cfg.coef[0][1] = mtrx->coef[0][1];
		//pstHwCfg->csc_cfg.coef[0][2] = mtrx->coef[0][2];
		pstHwCfg->csc_cfg.coef[1][0] = mtrx->coef[1][0];
		pstHwCfg->csc_cfg.coef[1][1] = mtrx->coef[1][1];
		pstHwCfg->csc_cfg.coef[1][2] = mtrx->coef[1][2];
		pstHwCfg->csc_cfg.coef[2][0] = mtrx->coef[2][0];
		pstHwCfg->csc_cfg.coef[2][1] = mtrx->coef[2][1];
		pstHwCfg->csc_cfg.coef[2][2] = mtrx->coef[2][2];
		pstHwCfg->csc_cfg.add[0] = mtrx->add[0];
		pstHwCfg->csc_cfg.add[1] = mtrx->add[1];
		pstHwCfg->csc_cfg.add[2] = mtrx->add[2];
		pstHwCfg->csc_cfg.sub[0] = mtrx->sub[0];
		pstHwCfg->csc_cfg.sub[1] = mtrx->sub[1];
		pstHwCfg->csc_cfg.sub[2] = mtrx->sub[2];
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[0][0]
			, pstHwCfg->csc_cfg.coef[0][1]
			, pstHwCfg->csc_cfg.coef[0][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[1][0]
			, pstHwCfg->csc_cfg.coef[1][1]
			, pstHwCfg->csc_cfg.coef[1][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
			, pstHwCfg->csc_cfg.coef[2][0]
			, pstHwCfg->csc_cfg.coef[2][1]
			, pstHwCfg->csc_cfg.coef[2][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
			, pstHwCfg->csc_cfg.sub[0]
			, pstHwCfg->csc_cfg.sub[1]
			, pstHwCfg->csc_cfg.sub[2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
			, pstHwCfg->csc_cfg.add[0]
			, pstHwCfg->csc_cfg.add[1]
			, pstHwCfg->csc_cfg.add[2]);
	}

	switch (stChnCfg->enCoef) {
	default:
	case VPSS_SCALE_COEF_BICUBIC:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_BICUBIC;
		break;
	case VPSS_SCALE_COEF_BILINEAR:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_BILINEAR;
		break;
	case VPSS_SCALE_COEF_NEAREST:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_NEAREST;
		break;
	case VPSS_SCALE_COEF_Z2:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_Z2;
		break;
	case VPSS_SCALE_COEF_Z3:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_Z3;
		break;
	case VPSS_SCALE_COEF_DOWNSCALE_SMOOTH:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_DOWNSCALE_SMOOTH;
		break;
	case VPSS_SCALE_COEF_OPENCV_BILINEAR:
		pstHwCfg->coef = CVI_SC_SCALING_COEF_OPENCV_BILINEAR;
		break;
	}
}

/*
 * @param VpssGrp: VPSS Grp to update cfg
 * @param ctx: VPSS ctx which records settings of this grp
 * @param pstGrpHwCfg: cfg to be updated
 */
void _vpss_grp_hw_cfg_update(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx, struct VPSS_GRP_HW_CFG *pstGrpHwCfg)
{
	VB_CAL_CONFIG_S stVbCalConfig;
	//struct sclr_csc_matrix *mtrx;

	if (!ctx->is_cfg_changed)
		return;

	COMMON_GetPicBufferConfig(ctx->stGrpAttr.u32MaxW, ctx->stGrpAttr.u32MaxH,
		ctx->stGrpAttr.enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);
	pstGrpHwCfg->bytesperline[0] = stVbCalConfig.u32MainStride;
	pstGrpHwCfg->bytesperline[1] = stVbCalConfig.u32CStride;

	// frame_crop applied if valid
	if (_is_frame_crop_valid(ctx)) {
		// for frame crop.
		RECT_S grp_crop;

		grp_crop.s32X = ctx->frame_crop.start_x;
		grp_crop.s32Y = ctx->frame_crop.start_y;
		grp_crop.u32Width = ctx->frame_crop.end_x - ctx->frame_crop.start_x;
		grp_crop.u32Height = ctx->frame_crop.end_y - ctx->frame_crop.start_y;
		if (ctx->stGrpCropInfo.bEnable)
			grp_crop = _vpss_get_union_crop(grp_crop, ctx->stGrpCropInfo.stCropRect);

		pstGrpHwCfg->rect_crop.left = grp_crop.s32X + ctx->s16OffsetLeft;
		pstGrpHwCfg->rect_crop.top = grp_crop.s32Y + ctx->s16OffsetTop;
		pstGrpHwCfg->rect_crop.width = grp_crop.u32Width;
		pstGrpHwCfg->rect_crop.height = grp_crop.u32Height;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) use frame crop.\n", VpssGrp);
	} else {
		// for grp crop.
		if (ctx->stGrpCropInfo.bEnable) {
			pstGrpHwCfg->rect_crop.width = ctx->stGrpCropInfo.stCropRect.u32Width;
			pstGrpHwCfg->rect_crop.height = ctx->stGrpCropInfo.stCropRect.u32Height;
			pstGrpHwCfg->rect_crop.left = ctx->stGrpCropInfo.stCropRect.s32X +
				ctx->s16OffsetLeft;
			pstGrpHwCfg->rect_crop.top = ctx->stGrpCropInfo.stCropRect.s32Y +
				ctx->s16OffsetTop;
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) use GrpCrop.\n", VpssGrp);
		} else {
			pstGrpHwCfg->rect_crop.left = ctx->s16OffsetLeft;
			pstGrpHwCfg->rect_crop.top = ctx->s16OffsetTop;
			pstGrpHwCfg->rect_crop.width = ctx->stGrpAttr.u32MaxW;
			pstGrpHwCfg->rect_crop.height = ctx->stGrpAttr.u32MaxH;
		}
	}
	CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) Offset(left:%d top:%d right:%d bottom:%d) rect(%d %d %d %d)\n"
			, VpssGrp, ctx->s16OffsetLeft, ctx->s16OffsetTop, ctx->s16OffsetRight, ctx->s16OffsetBottom
			, pstGrpHwCfg->rect_crop.left, pstGrpHwCfg->rect_crop.top
			, pstGrpHwCfg->rect_crop.width, pstGrpHwCfg->rect_crop.height);

		memcpy(&pstGrpHwCfg->csc_cfg, &vpssExtCtx[VpssGrp].csc_cfg, sizeof(pstGrpHwCfg->csc_cfg));

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
			, pstGrpHwCfg->csc_cfg.coef[0][0]
			, pstGrpHwCfg->csc_cfg.coef[0][1]
			, pstGrpHwCfg->csc_cfg.coef[0][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
			, pstGrpHwCfg->csc_cfg.coef[1][0]
			, pstGrpHwCfg->csc_cfg.coef[1][1]
			, pstGrpHwCfg->csc_cfg.coef[1][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
			, pstGrpHwCfg->csc_cfg.coef[2][0]
			, pstGrpHwCfg->csc_cfg.coef[2][1]
			, pstGrpHwCfg->csc_cfg.coef[2][2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
			, pstGrpHwCfg->csc_cfg.sub[0]
			, pstGrpHwCfg->csc_cfg.sub[1]
			, pstGrpHwCfg->csc_cfg.sub[2]);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
			, pstGrpHwCfg->csc_cfg.add[0]
			, pstGrpHwCfg->csc_cfg.add[1]
			, pstGrpHwCfg->csc_cfg.add[2]);

}

static void _notify_dwa_sbm_cfg_done(VPSS_GRP VpssGrp)
{
	struct vpss_grp_sbm_cfg *sbm_cfg = &vpssExtCtx[VpssGrp].sbm_cfg;
	struct base_exe_m_cb exe_cb;

	if (!sbm_cfg->sb_mode)
		return;

	exe_cb.callee = E_MODULE_DWA;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = DWA_CB_VPSS_SBM_DONE;
	exe_cb.data = NULL;
	base_exe_module_cb(&exe_cb);
}

static CVI_S32 commitHWSettings(VPSS_GRP VpssGrp, bool online_from_isp, struct cvi_vpss_ctx *ctx)
{
	CVI_S32 dev_idx;
	VPSS_GRP_ATTR_S *stGrpAttr;
	struct VPSS_CHN_CFG *stChnCfg;
	struct VPSS_GRP_HW_CFG *vpss_hw_cfgs = (struct VPSS_GRP_HW_CFG *)ctx->hw_cfgs;
	struct VPSS_CHN_HW_CFG *pstHwCfg;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	MMF_BIND_DEST_S stBindDest;
	struct cvi_vpss_chn_cfg chn_cfg;
	struct cvi_vpss_grp_cfg grp_cfg;
	struct vb_s *vb_in = NULL;
	VPSS_CHN VpssChn;
	CVI_S32 i;
	CVI_U8 sb_wr_ctrl_idx = 0;
	struct vb_jobs_t *jobs;
	CVI_BOOL bind_fb = CVI_FALSE;

	stGrpAttr = &ctx->stGrpAttr;

	if (_is_frame_crop_changed(VpssGrp, ctx))
		ctx->is_cfg_changed = CVI_TRUE;

	_vpss_grp_hw_cfg_update(VpssGrp, ctx, vpss_hw_cfgs);

	dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_IN);

	if (!online_from_isp) {
		//FIFO_GET_TAIL(&vpssCtx[VpssGrp]->vb_jobs.waitq, &vb_in);
		if (!base_mod_jobs_waitq_empty(chn, CHN_TYPE_IN)) {
			jobs = base_get_jobs_by_chn(chn, CHN_TYPE_IN);
			FIFO_GET_TAIL(&jobs->waitq, &vb_in);
		}
	}

	memset(&grp_cfg, 0, sizeof(grp_cfg));
	grp_cfg.grp_id = online_from_isp ? VpssGrp : 0;
	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		grp_cfg.chn_enable[VpssChn] = stChnCfg->isEnabled;
	}
	grp_cfg.src_size.width = stGrpAttr->u32MaxW;
	grp_cfg.src_size.height = stGrpAttr->u32MaxH;
	grp_cfg.pixelformat = stGrpAttr->enPixelFormat;
	grp_cfg.bytesperline[0] = (vb_in && (vb_in->buf.stride[0] > vpss_hw_cfgs->bytesperline[0]))
				? vb_in->buf.stride[0] : vpss_hw_cfgs->bytesperline[0];
	grp_cfg.bytesperline[1] = (vb_in && (vb_in->buf.stride[1] > vpss_hw_cfgs->bytesperline[1]))
				? vb_in->buf.stride[1] : vpss_hw_cfgs->bytesperline[1];
	grp_cfg.crop = vpss_hw_cfgs->rect_crop;
	grp_cfg.csc_cfg = vpss_hw_cfgs->csc_cfg;
	if (img_set_vpss_grp_cfg(&vip_dev->img_vdev[dev_idx], &grp_cfg) != 0) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "img_set_vpss_grp_cfg NG\n");
	}

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		pstHwCfg = &vpss_hw_cfgs->stChnCfgs[VpssChn];

		if (!stChnCfg->isEnabled)
			continue;

		_vpss_chn_hw_cfg_update(VpssGrp, VpssChn, ctx, vpss_hw_cfgs, online_from_isp);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) size(%d %d) rect(%d %d %d %d)\n", VpssGrp, VpssChn
				, stChnCfg->stChnAttr.u32Width, stChnCfg->stChnAttr.u32Height
				, pstHwCfg->rect_crop.left, pstHwCfg->rect_crop.top
				, pstHwCfg->rect_crop.width, pstHwCfg->rect_crop.height);
		chn.s32ChnId = VpssChn;
		dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_OUT);

		memset(&chn_cfg, 0, sizeof(chn_cfg));
		chn_cfg.grp_id = online_from_isp ? VpssGrp : 0;
		chn_cfg.src_size.width = vpss_hw_cfgs->rect_crop.width;
		chn_cfg.src_size.height = vpss_hw_cfgs->rect_crop.height;
		chn_cfg.pixelformat = stChnCfg->stChnAttr.enPixelFormat;
		chn_cfg.bytesperline[0] = pstHwCfg->bytesperline[0];
		chn_cfg.bytesperline[1] = pstHwCfg->bytesperline[1];
		chn_cfg.crop = pstHwCfg->rect_crop;
		chn_cfg.dst_rect = pstHwCfg->rect_output;
		chn_cfg.dst_size.width = stChnCfg->stChnAttr.u32Width;
		chn_cfg.dst_size.height = stChnCfg->stChnAttr.u32Height;

		if (stChnCfg->stChnAttr.bFlip && stChnCfg->stChnAttr.bMirror)
			chn_cfg.flip = CVI_SC_FLIP_HVFLIP;
		else if (stChnCfg->stChnAttr.bFlip)
			chn_cfg.flip = CVI_SC_FLIP_VFLIP;
		else if (stChnCfg->stChnAttr.bMirror)
			chn_cfg.flip = CVI_SC_FLIP_HFLIP;
		else
			chn_cfg.flip = CVI_SC_FLIP_NO;
		chn_cfg.border_cfg = pstHwCfg->border_param;
		chn_cfg.quant_cfg = pstHwCfg->quantCfg;
		for (i = 0; i < RGN_MAX_LAYER_VPSS; i++)
			chn_cfg.rgn_cfg[i] = pstHwCfg->rgn_cfg[i];
		chn_cfg.rgn_coverex_cfg = pstHwCfg->rgn_coverex_cfg;
		chn_cfg.rgn_mosaic_cfg = pstHwCfg->rgn_mosaic_cfg;
		chn_cfg.sc_coef = pstHwCfg->coef;
		chn_cfg.csc_cfg = pstHwCfg->csc_cfg;
		chn_cfg.mute_cfg.enable = stChnCfg->isMuted;
		chn_cfg.mute_cfg.color[0] = 0;
		chn_cfg.mute_cfg.color[1] = 0;
		chn_cfg.mute_cfg.color[2] = 0;
		chn_cfg.sb_cfg.sb_mode = stChnCfg->stBufWrap.bEnable ? SCLR_SB_FREE_RUN : SCLR_SB_DISABLE;
		chn_cfg.sb_cfg.sb_size = (stChnCfg->stBufWrap.u32BufLine / 64) - 1; // 0: 64 line, 1: 128
		chn_cfg.sb_cfg.sb_nb = stChnCfg->u32BufWrapDepth - 1; // start from 0
		//chn_cfg.sb_cfg.sb_full_nb = ;
		chn_cfg.sb_cfg.sb_wr_ctrl_idx = sb_wr_ctrl_idx;
		if (stChnCfg->stBufWrap.bEnable)
			sb_wr_ctrl_idx++;
		sc_set_vpss_chn_cfg(&vip_dev->sc_vdev[dev_idx], &chn_cfg);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "chn[%d] sb_mode=%d, sb_size=%d, sb_nb=%d\n",
				VpssChn, chn_cfg.sb_cfg.sb_mode, chn_cfg.sb_cfg.sb_size,
				chn_cfg.sb_cfg.sb_nb);

		if (fb_on_vpss)
			if (sys_get_bindbysrc(&chn, &stBindDest) == CVI_SUCCESS) {
				for (i = 0; i < stBindDest.u32Num; ++i)
					if (stBindDest.astMmfChn[i].enModId == CVI_ID_VO) {
						bind_fb = CVI_TRUE;
						CVI_TRACE_VPSS(CVI_DBG_INFO,
							"grp(%d) chn(%d) use fb's gop. fb_on_vpss(%d)\n",
							VpssGrp, VpssChn, bind_fb);
						break;
					}
			}
		sc_set_vpss_chn_bind_fb(&vip_dev->sc_vdev[dev_idx], bind_fb);

	}
	return CVI_SUCCESS;
}

static CVI_VOID _update_vpss_chnRealFrameRate(void)
{
	int i, j;
	CVI_U64 duration, curTimeUs;
	struct timespec64 curTime;

	ktime_get_ts64(&curTime);
	curTimeUs = (u64)curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC;

	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpssCtx[i] && vpssCtx[i]->isCreated) {
			for (j = 0; j < vpssCtx[i]->chnNum; ++j) {
				if (vpssCtx[i]->stChnCfgs[j].isEnabled) {
					duration = curTimeUs - vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u64PrevTime;
					if (duration >= 1000000) {
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32RealFrameRate
							= vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32FrameNum;
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32FrameNum = 0;
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u64PrevTime = curTimeUs;
					}
				}
			}
		}
	}
}

/* _vpss_chl_frame_rate_ctrl: dynamically disabled chn per frame-rate-ctrl
 *
 * @param proc_ctx: the frame statics for reference
 * @param ctx: the working settings
 */
static CVI_S32 simplify_rate(CVI_U32 dst_in, CVI_U32 src_in, CVI_U32 *dst_out, CVI_U32 *src_out)
{
	CVI_U32 i = 1;
	CVI_U32 a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}
	*dst_out = dst_in;
	*src_out = src_in;
	return CVI_SUCCESS;
}

static CVI_BOOL vpss_frame_ctrl(CVI_U32 u32FrameIndex, FRAME_RATE_CTRL_S *pstFrameRate)
{
	CVI_U32 src_simp;
	CVI_U32 dst_simp;
	CVI_U32 u32Index;
	CVI_U32 srcDur, dstDur;
	CVI_U32 curIndx, nextIndx;

	simplify_rate(pstFrameRate->s32DstFrameRate, pstFrameRate->s32SrcFrameRate,
		&dst_simp, &src_simp);

	u32Index = u32FrameIndex % src_simp;
	if (u32Index == 0) {
		return CVI_TRUE;
	}
	srcDur = 100;
	dstDur = (srcDur * src_simp) / dst_simp;
	curIndx = (u32Index - 1) * srcDur / dstDur;
	nextIndx = u32Index * srcDur / dstDur;

	if (nextIndx == curIndx)
		return CVI_FALSE;

	return CVI_TRUE;
}

static CVI_VOID _vpss_chl_frame_rate_ctrl(struct cvi_vpss_ctx *ctx)
{
	VPSS_CHN VpssChn;

	if (!ctx)
		return;
	if (!ctx->isCreated || !ctx->isStarted)
		return;

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		if (!ctx->stChnCfgs[VpssChn].isEnabled || !ctx->stChnCfgs[VpssChn].stChnWorkStatus.u32SendOk)
			continue;
		if (FRC_INVALID(ctx, VpssChn))
			continue;
		if (!vpss_frame_ctrl(ctx->stGrpWorkStatus.u32RecvCnt - 1,
			&ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate)) {
			ctx->stChnCfgs[VpssChn].isEnabled = false;
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "chn[%d] frame index(%d) drop\n", VpssChn,
				ctx->stGrpWorkStatus.u32RecvCnt);
		}
	}
}

static CVI_VOID _update_vpss_grp_proc(VPSS_GRP VpssGrp, CVI_U32 duration, CVI_U32 HwDuration)
{
	if (!vpssCtx[VpssGrp])
		return;

	vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime = duration;
	if (vpssCtx[VpssGrp]->stGrpWorkStatus.u32MaxCostTime <
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime) {
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32MaxCostTime
			= vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime;
	}
	vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime = HwDuration;
	if (vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwMaxCostTime <
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime) {
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwMaxCostTime
			= vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime;
	}
}

static CVI_VOID _update_vpss_chn_proc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	struct VPSS_CHN_WORK_STATUS_S *pstChnStatus;

	if (!vpssCtx[VpssGrp])
		return;

	pstChnStatus = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnWorkStatus;
	pstChnStatus->u32SendOk++;
	pstChnStatus->u32FrameNum++;
}

static CVI_VOID _release_vpss_waitq(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	VB_BLK blk_grp = base_mod_jobs_waitq_pop(chn, chn_type);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);
}

static CVI_VOID _release_vpss_doneq(MMF_CHN_S chn)
{
	struct vb_jobs_t *jobs = NULL;
	struct vb_s *vb;

	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_OUT);

	if (!jobs || !jobs->inited)
		return;

	while (1) {
		mutex_lock(&jobs->dlock);
		if (FIFO_EMPTY(&jobs->doneq)) {
			mutex_unlock(&jobs->dlock);
			break;
		}
		FIFO_POP(&jobs->doneq, &vb);
		vb->mod_ids &= ~BIT(chn.enModId);
		mutex_unlock(&jobs->dlock);
		vb_release_block((VB_BLK)vb);
	}
}

static CVI_VOID _clean_vpss_workq(MMF_CHN_S chn)
{
	CVI_U32 i = 0;
	struct vb_jobs_t *jobs = NULL;
	struct vb_s *vb = NULL;

	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_OUT);

	if (!jobs || !jobs->inited)
		return;

	mutex_lock(&jobs->lock);
	if (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_FOREACH(vb, &jobs->workq, i) {
			vb->buf.flags = 1;
		}
	}
	mutex_unlock(&jobs->lock);
}

static CVI_BOOL _vpss_check_gdc_job(MMF_CHN_S chn, VB_BLK blk, struct cvi_vpss_ctx *vpss_ctx)
{
	struct cvi_gdc_mesh *pmesh;

	pmesh = &mesh[chn.s32DevId][chn.s32ChnId];
	if (mutex_trylock(&pmesh->lock)) {
		if (vpss_ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.bEnable) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

			if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC
				, &vpss_ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.stAttr
				, vb
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
				, pmesh->paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VPSS
				, vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pmesh->lock);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "gdc LDC failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return CVI_FALSE;
			}
			return CVI_TRUE;
		} else if (vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation != ROTATION_0) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn,
				.usage = GDC_USAGE_ROTATION };

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
				, pmesh->paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VPSS
				, vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pmesh->lock);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "gdc rotation failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return CVI_FALSE;
			}
			return CVI_TRUE;
		}
		mutex_unlock(&pmesh->lock);
	} else {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "grp(%d) chn(%d) drop frame due to gdc op blocked.\n",
			chn.s32DevId, chn.s32ChnId);
		// release blk if gdc not done yet
		vb_release_block(blk);
		return CVI_TRUE;
	}

	return CVI_FALSE;
}

static CVI_BOOL _is_vc_sbm_enabled(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	VPSS_CHN VpssChn;

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		if (ctx->stChnCfgs[VpssChn].isEnabled &&
		    ctx->stChnCfgs[VpssChn].stBufWrap.bEnable)
			return CVI_TRUE;
	}

	return CVI_FALSE;
}

static CVI_U32 _get_enabled_channels(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	VPSS_CHN VpssChn;
	CVI_U32 num = 0;

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn)
		if (ctx->stChnCfgs[VpssChn].isEnabled)
			num++;

	return num;
}

static CVI_S32 IS_VENC_BIND_VPSS(MMF_CHN_S *pchn)
{
	MMF_BIND_DEST_S stBindDest;

	if (sys_get_bindbysrc(pchn, &stBindDest) == CVI_SUCCESS) {
		if (stBindDest.astMmfChn[0].enModId != CVI_ID_VENC) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "Disable SB for next Mod(%d)\n",
					stBindDest.astMmfChn[0].enModId);
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}

CVI_BOOL is_online_hw_run(CVI_U8 img_idx)
{
	CVI_U8 i;
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	struct cvi_img_vdev *idev = &vip_dev->img_vdev[img_idx];

	if (cvi_vip_job_is_queued(idev))
		return CVI_TRUE;

	cvi_img_get_sc_bound(idev, sc_need_check);

	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (atomic_read(&vip_dev->sc_vdev[i].job_state) == CVI_VIP_RUNNING)
			return CVI_TRUE;
	}
	return CVI_FALSE;
}

static CVI_S32 _notify_vc_sb_mode(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx,
		CVI_U32 cmd_id, CVI_BOOL online_from_isp)
{
	CVI_S32 ret = CVI_SUCCESS;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	MMF_CHN_S chn1 = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 1};
	MMF_CHN_S *pchn = NULL;
	CVI_U32 sbChCnt = 0;
	VPSS_CHN VpssChn;
	VPSS_CHN_BUF_WRAP_S *pstBufWrap;
	struct VPSS_CHN_CFG *stChnCfg;
	struct base_exe_m_cb exe_cb;
	CVI_BOOL bEnable = CVI_FALSE;
	struct venc_send_frm_info send_frm;
	struct cvi_buffer *buf = &send_frm.stInFrmBuf;
	struct cvi_buffer *buf1 = &send_frm.stInFrmBuf1;

	for (VpssChn = 0; VpssChn < ctx->chnNum; ++VpssChn) {
		stChnCfg = &ctx->stChnCfgs[VpssChn];
		pstBufWrap = &stChnCfg->stBufWrap;

		if (!pstBufWrap->bEnable)
			continue;

		if (sbChCnt == 0) {
			chn.s32ChnId = VpssChn;
			pchn = &chn;
		} else if (sbChCnt == 1) {
			chn1.s32ChnId = VpssChn;
			pchn = &chn1;
		} else
			continue;

		if (IS_VENC_BIND_VPSS(pchn) != CVI_SUCCESS) {
			pstBufWrap->bEnable = CVI_FALSE;
			continue;
		}

		if (sbChCnt == 0) {
			_vpss_fill_sbm_buffer(stChnCfg->stChnAttr.u32Width,
					      stChnCfg->stChnAttr.enPixelFormat,
					      stChnCfg->align,
					      pstBufWrap->u32BufLine,
					      stChnCfg->u32BufWrapDepth,
					      stChnCfg->bufWrapPhyAddr,
					      buf);
			if (VpssGrp < ARRAY_SIZE(g_mlv_i))
				memcpy(buf->motion_table, g_mlv_i[VpssGrp].mlv_i_table,
				       sizeof(buf->motion_table));
		} else if (sbChCnt == 1) {
			_vpss_fill_sbm_buffer(stChnCfg->stChnAttr.u32Width,
					      stChnCfg->stChnAttr.enPixelFormat,
					      stChnCfg->align,
					      pstBufWrap->u32BufLine,
					      stChnCfg->u32BufWrapDepth,
					      stChnCfg->bufWrapPhyAddr,
					      buf1);
			if (VpssGrp < ARRAY_SIZE(g_mlv_i))
				memcpy(buf1->motion_table, g_mlv_i[VpssGrp].mlv_i_table,
				       sizeof(buf1->motion_table));
		}
		bEnable = CVI_TRUE;

		sbChCnt++;
	}

	if (bEnable) {

		if (cmd_id >= VCODEC_CB_MAX)
			return CVI_FAILURE;

		buf->size.u32Height = ctx->stChnCfgs[chn.s32ChnId].stChnAttr.u32Height;

		if (sbChCnt == 2) {
			buf1->size.u32Height = ctx->stChnCfgs[chn1.s32ChnId].stChnAttr.u32Height;
		}

		send_frm.vpss_grp = VpssGrp;
		send_frm.vpss_chn = chn.s32ChnId;
		if (sbChCnt == 2)
			send_frm.vpss_chn1 = chn1.s32ChnId;
		else
			send_frm.vpss_chn1 = 0xFFFFFFFF;

		send_frm.sb_nb = ctx->stChnCfgs[chn.s32ChnId].u32BufWrapDepth;
		send_frm.isOnline = online_from_isp;
		exe_cb.callee = E_MODULE_VCODEC;
		exe_cb.caller = E_MODULE_VPSS;
		exe_cb.cmd_id = cmd_id;
		exe_cb.data    = (void *)&send_frm;

		ret = base_exe_module_cb(&exe_cb);
	}

	return ret;
}

/**
 * @return: 0 if ready
 */
static CVI_S32 vpss_try_schedule_online(struct vpss_handler_ctx *ctx)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VPSS_ONLINE_GRP_0, .s32ChnId = 0};
	CVI_U8 workingMask = ctx->workingMask;
	CVI_U8 u8ChnMask = 0;
	VPSS_GRP workingGrp = 0;
	VPSS_CHN VpssChn;
	struct cvi_vpss_ctx *vpss_ctx = CVI_NULL;
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BOOL vc_sbm_enabled;

	// makr sure all online grp is ready.
	for (workingGrp = 0; workingGrp < VPSS_ONLINE_NUM; workingGrp++) {
		if (!(ctx->online_chkMask & BIT(workingGrp)))
			continue;
		if (workingMask & BIT(workingGrp))
			continue;
		if (!vpssCtx[workingGrp])
			continue;

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d), workingMask(%d), online_chkMask(%d).\n",
			workingGrp, workingMask, ctx->online_chkMask);

		mutex_lock(&vpssCtx[workingGrp]->lock);
		vpss_ctx = &ctx->vpssCtxWork[workingGrp];

		memcpy(vpss_ctx, vpssCtx[workingGrp], sizeof(*vpss_ctx));
		vpssCtx[workingGrp]->is_cfg_changed = CVI_FALSE;
		for (VpssChn = 0; VpssChn < vpssCtx[workingGrp]->chnNum; ++VpssChn)
			vpssCtx[workingGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_FALSE;

		// sc's mask
		u8ChnMask = getWorkMask(vpss_ctx);

		chn.s32DevId = workingGrp;
		vc_sbm_enabled = _is_vc_sbm_enabled(workingGrp, vpss_ctx);

		// commit hw settings of this vpss-grp.
		if (commitHWSettings(workingGrp, ctx->online_from_isp, vpss_ctx) != CVI_SUCCESS) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) apply hw settings NG.\n", workingGrp);
			vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
			mutex_unlock(&vpssCtx[workingGrp]->lock);
			break;
		}

		if (u8ChnMask && !(vpssExtCtx[workingGrp].grp_state & GRP_STATE_BUF_FILLED)) {
			if (fill_buffers(workingGrp, vpss_ctx, ctx->online_from_isp) != CVI_SUCCESS) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);
				vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
				mutex_unlock(&vpssCtx[workingGrp]->lock);
				break;
			}
			vpssExtCtx[workingGrp].grp_state |= GRP_STATE_BUF_FILLED;
		}

		if (!(vpssExtCtx[workingGrp].grp_state & GRP_STATE_HW_STARTED)) {
			// fill second buffer
			if (fill_buffers(workingGrp, vpss_ctx, ctx->online_from_isp) != CVI_SUCCESS) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill 2nd buffer NG.\n", workingGrp);
				vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
				// keep going
				//break;
			}

			if (vc_sbm_enabled) {
				/* Configure vc h/w before vpss h/w */
				if (_notify_vc_sb_mode(workingGrp, vpss_ctx, VCODEC_CB_SEND_FRM,
					CVI_TRUE) != CVI_SUCCESS) {
					mutex_unlock(&vpssCtx[workingGrp]->lock);
					CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) vb sbm not ready\n", workingGrp);
					// Let isp wakeup vpss, not use wakeup event
					fill_sb_buffers(workingGrp, vpss_ctx, false);
					hw_start(workingGrp, CVI_TRUE, CVI_FALSE, vpss_ctx);
					break;
				}
				fill_sb_buffers(workingGrp, vpss_ctx, true);
			}

			if (hw_start(workingGrp, CVI_TRUE, CVI_FALSE, vpss_ctx) != CVI_SUCCESS) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) run NG.\n", workingGrp);
				vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
				mutex_unlock(&vpssCtx[workingGrp]->lock);
				break;
			}

			if (vc_sbm_enabled) {
				/* Configure vpss h/w before dwa h/w */
				_notify_dwa_sbm_cfg_done(workingGrp);
			}

			vpssExtCtx[workingGrp].grp_state |= GRP_STATE_HW_STARTED;
		}

		workingMask |= BIT(workingGrp);
		ctx->workingMask |= BIT(workingGrp);
		mutex_unlock(&vpssCtx[workingGrp]->lock);
	}

	// Not all online grp ready
	for (workingGrp = 0; workingGrp < VPSS_ONLINE_NUM; workingGrp++) {
		if (!(ctx->online_chkMask & BIT(workingGrp)))
			continue;
		if (workingMask & BIT(workingGrp))
			continue;
		if (!vpssCtx[workingGrp])
			continue;

		if (vpssExtCtx[workingGrp].grp_state & GRP_STATE_BUF_FILLED) {
			release_buffers(workingGrp, &ctx->vpssCtxWork[workingGrp]);
			release_online_sc_buffers(workingGrp, vpss_ctx);
		}

		vpssExtCtx[workingGrp].grp_state = GRP_STATE_IDLE;
		workingMask |= BIT(workingGrp);
		ret = CVI_ERR_VPSS_BUSY;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] workingGrp=%d, workingMask=%d, img_idx=%d\n",
			ctx->u8VpssDev, ctx->workingGrp, ctx->workingMask, ctx->img_idx);

	return ret;
}

static CVI_VOID vpss_handle_online_frame_done(struct vpss_handler_ctx *ctx, VPSS_GRP workingGrp)
{
	struct cvi_vpss_ctx *vpss_ctx = &ctx->vpssCtxWork[workingGrp];
	CVI_U8 workingMask = ctx->workingMask;
	VPSS_CHN VpssChn;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};
	VB_BLK blk;
	CVI_U32 duration = 0, HwDuration = 0;
	struct timespec64 time;
	CVI_U64 u64PTS;
	CVI_U8 u8ChnMask;
	struct vb_s *vb;
	CVI_BOOL vc_sbm_enabled = _is_vc_sbm_enabled(workingGrp, vpss_ctx);

	vpssCtx[workingGrp]->stGrpWorkStatus.u32RecvCnt++;
	u8ChnMask = _vpss_get_chn_mask_by_dev(workingGrp, ctx->IntMask[workingGrp]);
	_vpss_chl_frame_rate_ctrl(vpss_ctx);

	VpssChn = 0;
	mutex_lock(&vpssCtx[workingGrp]->lock);

	do {
		if (vpss_ctx->stChnCfgs[VpssChn].stBufWrap.bEnable &&
			vpss_ctx->stChnCfgs[VpssChn].bufWrapPhyAddr) {
			_update_vpss_chn_proc(workingGrp, VpssChn);
			continue;
		}
		if (!vpssCtx[workingGrp]->stChnCfgs[VpssChn].isEnabled)
			continue;
		if (!(u8ChnMask & BIT(VpssChn)))
			continue;

		chn.s32ChnId = VpssChn;
		_vb_dqbuf(chn, CHN_TYPE_OUT, &blk);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Mod(%d) can't get vb-blk.\n"
				     , workingGrp, VpssChn, chn.enModId);
			continue;
		}
		vb = (struct vb_s *)blk;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) dqbuf phy(%#llx)\n",
			workingGrp, VpssChn, ((struct vb_s *)blk)->phy_addr);

		if ((workingGrp <= VI_MAX_CHN_NUM) && !vc_sbm_enabled) {
			if (isp_bypass_frm[workingGrp] >= vpssCtx[workingGrp]->stGrpWorkStatus.u32RecvCnt) {
				CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) drop frame for vi-bypass(%d).\n",
						workingGrp, isp_bypass_frm[workingGrp]);
				vb_release_block(blk);
				_update_vpss_chn_proc(workingGrp, VpssChn);
				continue;
			}
		}

		//if (vi_ctx.bypass_frm[workingGrp] >= vpssPrcCtx[workingGrp].stChnCfgs[VpssChn].u32SendOk) {
		//	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) drop frame for vi-bypass(%d).\n",
		//		       workingGrp, VpssChn, vi_ctx.bypass_frm[workingGrp]);
		//	vb_release_block(blk);
		//	_update_vpss_chn_proc(workingGrp, VpssChn);
		//	continue;
		//}
		if (!vpss_ctx->stChnCfgs[VpssChn].isEnabled) {
			vb_release_block(blk);
			continue;
		}
		if (vb->buf.flags == 1) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) drop frame.\n", workingGrp, VpssChn);
			vb_release_block(blk);
			continue;
		}

		// update pts & frm_num info to vb
		u64PTS = timespec64_to_ns(&vip_dev->img_vdev[ctx->img_idx].ts_end);
		do_div(u64PTS, 1000);
		vb->buf.u64PTS = u64PTS;
		vb->buf.frm_num = vip_dev->img_vdev[ctx->img_idx].frame_number[workingGrp];
		vb->buf.dev_num = workingGrp;
		_vpss_online_set_mlv_info(vb);

		// check if there is rgn_ex need to do on this chn
		//if (vpss_ctx->stChnCfgs[VpssChn].rgn_ex_cfg.num_of_rgn_ex > 0
		//	&& CVI_SYS_GetVPSSMode() == VPSS_MODE_RGNEX && ctx->u8VpssDev == 1) {
		//	struct _vpss_rgnex_job_info job;

		//	job.chn = chn;
		//	job.blk = blk;
		//	job.rgn_ex_cfg = vpss_ctx->stChnCfgs[VpssChn].rgn_ex_cfg;
		//	job.enPixelFormat = vpss_ctx->stChnCfgs[VpssChn].stChnAttr.enPixelFormat;
		//	job.bytesperline[0]
		//		= vpss_hw_cfgs[workingGrp].stChnCfgs[VpssChn].bytesperline[0];
		//	job.bytesperline[1]
		//		= vpss_hw_cfgs[workingGrp].stChnCfgs[VpssChn].bytesperline[1];

		//	mutex_lock(&handler_ctx[0].rgnex_jobs.lock);
		//	if (FIFO_FULL(&handler_ctx[0].rgnex_jobs.jobq))
		//		CVI_TRACE_VPSS(CVI_DBG_ERR, "rgnex jobq full.\n");
		//	else {
		//		*((VB_S *)blk)->mod_ids |= BIT(chn.enModId);
		//		FIFO_PUSH(&handler_ctx[0].rgnex_jobs.jobq, job);
		//		mutex_unlock(&handler_ctx[0].rgnex_jobs.lock);
		//		CVI_VPSS_PostJob(0);
		//		CVI_TRACE_VPSS(CVI_DBG_INFO, "push grp(%d) chn(%d) rgn_ex job\n",
		//			job.chn.s32DevId, job.chn.s32ChnId);
		//		continue;
		//	}
		//	mutex_unlock(&handler_ctx[0].rgnex_jobs.lock);
		//}

		if (_vpss_check_gdc_job(chn, blk, vpss_ctx) != CVI_TRUE)
			vb_done_handler(chn, CHN_TYPE_OUT, blk);

		CVI_TRACE_VPSS(CVI_DBG_WARN, "grp(%d) chn(%d) end\n", workingGrp, VpssChn);
		_update_vpss_chn_proc(workingGrp, VpssChn);
	} while (++VpssChn < vpss_ctx->chnNum);
	mutex_unlock(&vpssCtx[workingGrp]->lock);

	workingMask &= ~BIT(workingGrp);
	ctx->workingMask = workingMask;

	// job done.

	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);
	HwDuration = vip_dev->img_vdev[ctx->img_idx].hw_duration;
	// Update vpss proc info
	_update_vpss_grp_proc(workingGrp, duration, HwDuration);
}

static CVI_VOID vpss_handle_online(struct vpss_handler_ctx *ctx)
{
	VPSS_GRP workingGrp;
	struct cvi_vpss_ctx *vpss_ctx;
	CVI_S32 dev_idx;
	CVI_U64 duration64;
	struct timespec64 time;
	CVI_U32 state;
	CVI_U32 events;
	CVI_BOOL vc_sbm_enabled;
	CVI_U32 enabled_chnls;
	unsigned long flags_job;
	CVI_U32 i;

	_update_vpss_chnRealFrameRate();

	spin_lock_irqsave(&ctx->lock, flags_job);
	events = atomic_read(&ctx->events);
	workingGrp = ctx->workingGrp;
	spin_unlock_irqrestore(&ctx->lock, flags_job);

	if (workingGrp >= VPSS_ONLINE_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) invalid\n", workingGrp);
		return;
	}

	vpss_ctx = &ctx->vpssCtxWork[workingGrp];
	state = vpssExtCtx[workingGrp].grp_state;
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] event=0x%x, Grp(%d) state=0x%x\n",
			ctx->u8VpssDev, events, workingGrp,
			vpssExtCtx[workingGrp].grp_state);

	dev_idx = ctx->img_idx;

	//sbm err handle
	if (events & CTX_EVENT_VI_ERR) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "img(%d:%d:%d) grp(%d) vi err\n",
			dev_idx,
			vip_dev->img_vdev[dev_idx].img_type,
			vip_dev->img_vdev[dev_idx].input_type, workingGrp);

		// sclr_check_register();
		ctx->isr_evt[workingGrp] = 0;

		img_g_input(&vip_dev->img_vdev[dev_idx], &i);
		hw_reset(workingGrp, vpss_ctx, CVI_TRUE);
		_notify_vc_sb_mode(workingGrp, vpss_ctx, VCODEC_CB_SKIP_FRM, CVI_TRUE);
		img_s_input(&vip_dev->img_vdev[dev_idx], i);

		spin_lock_irqsave(&ctx->lock, flags_job);
		atomic_set(&ctx->events, 0);
		ctx->reset_done = true;
		ctx->workingMask = 0;
		spin_unlock_irqrestore(&ctx->lock, flags_job);
		vpssExtCtx[workingGrp].grp_state = GRP_STATE_IDLE;

		hw_start(workingGrp, CVI_TRUE, CVI_FALSE, vpss_ctx);
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss reset done\n");
		wake_up_interruptible(&ctx->vi_reset_wait);
		usleep_range(100, 200);
		//return;
	} else if (events & CTX_EVENT_EOF) {
		if (!vpssCtx[workingGrp] || !vpssCtx[workingGrp]->isStarted) {
			spin_lock_irqsave(&ctx->lock, flags_job);
			atomic_set(&ctx->events, 0);
			ctx->isr_evt[workingGrp] = 0;
			spin_unlock_irqrestore(&ctx->lock, flags_job);
		}
	}

	//frame done
	if (vpssExtCtx[workingGrp].grp_state & (GRP_STATE_HW_STARTED|GRP_STATE_BUF_FILLED)) {
		if (events & CTX_EVENT_EOF) {
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d) grp(%d) eof\n",
				dev_idx,
				vip_dev->img_vdev[dev_idx].img_type,
				vip_dev->img_vdev[dev_idx].input_type, workingGrp);

			spin_lock_irqsave(&ctx->lock, flags_job);
			ctx->isr_evt[workingGrp]--;
			atomic_fetch_and(~CTX_EVENT_EOF, &ctx->events);
			spin_unlock_irqrestore(&ctx->lock, flags_job);
			vpss_handle_online_frame_done(ctx, workingGrp);

			vc_sbm_enabled = _is_vc_sbm_enabled(workingGrp, vpss_ctx);
			enabled_chnls = _get_enabled_channels(workingGrp, vpss_ctx);
			if (vc_sbm_enabled && enabled_chnls == 1) {
				// Keep current state
				spin_lock_irqsave(&ctx->lock, flags_job);
				atomic_set(&ctx->events, 0);
				spin_unlock_irqrestore(&ctx->lock, flags_job);
				ktime_get_ts64(&ctx->time);
			} else {
				// H/W starts once and continues to receive a frame
				vpssExtCtx[workingGrp].grp_state &= ~GRP_STATE_BUF_FILLED;
			}
		}
	}

	//timeout
	ktime_get_ts64(&time);
	duration64 = get_diff_in_us(ctx->time, time);
	do_div(duration64, 1000);
	if ((duration64 > EOF_WAIT_TIMEOUT_MS)
		&& (is_online_hw_run(ctx->img_idx))) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "online timeout.\n");

		sclr_check_register();
		atomic_set(&ctx->events, 0);
		ctx->workingMask = 0;
		for (i = 0; i < VPSS_ONLINE_NUM; i++) {
			if (vpssExtCtx[i].grp_state & (GRP_STATE_HW_STARTED|GRP_STATE_BUF_FILLED)) {
				CVI_TRACE_VPSS(CVI_DBG_NOTICE, "grp(%d) timeout.\n", i);
				vpssExtCtx[i].grp_state = GRP_STATE_IDLE;
				release_buffers(i, &ctx->vpssCtxWork[i]);
				hw_start(i, CVI_FALSE, CVI_TRUE, &ctx->vpssCtxWork[i]);
				_notify_vc_sb_mode(i, &ctx->vpssCtxWork[i], VCODEC_CB_SKIP_FRM, CVI_TRUE);
			}
		}
		i = 0;
		_vpss_call_cb(E_MODULE_VI, VI_CB_RESET_ISP, &i);
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss reset done\n");
	}

	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		if (vpssCtx[i] && vpssCtx[i]->isStarted && !(vpssExtCtx[i].grp_state & GRP_STATE_BUF_FILLED)) {
			vpss_try_schedule_online(ctx);
			ktime_get_ts64(&ctx->time);
			break;
		}
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] evt=0x%x, Grp(%d) state 0x%x->0x%x\n",
			ctx->u8VpssDev, atomic_read(&ctx->events), workingGrp, state,
			vpssExtCtx[workingGrp].grp_state);
}

/**
 * @return: 0 if ready
 */
static CVI_S32 vpss_try_schedule_offline(struct vpss_handler_ctx *ctx)
{
	VPSS_GRP workingGrp = ctx->workingGrp;
	VPSS_CHN VpssChn;
	CVI_U8 workingMask = 0;
	struct cvi_vpss_ctx *vpss_ctx = CVI_NULL;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};

	ktime_get_ts64(&ctx->time);

	mutex_lock(&vpssCtx[workingGrp]->lock);

	vpss_ctx = &ctx->vpssCtxWork[0];
	memcpy(vpss_ctx, vpssCtx[workingGrp], sizeof(*vpss_ctx));

	// sc's mask
	workingMask = getWorkMask(vpss_ctx);
	if (workingMask) {
		_vpss_chl_frame_rate_ctrl(vpss_ctx);
		workingMask = getWorkMask(vpss_ctx);
	}
	if (workingMask == 0) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "grp(%d) workingMask zero.\n", workingGrp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		goto vpss_next_job;
	}
	vpssCtx[workingGrp]->is_cfg_changed = CVI_FALSE;
	for (VpssChn = 0; VpssChn < vpssCtx[workingGrp]->chnNum; ++VpssChn)
		vpssCtx[workingGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_FALSE;

	//ktime_get_ts64(&time[0]);

	if (hw_start(workingGrp, CVI_FALSE, CVI_FALSE, vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) stop before run NG.\n", workingGrp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		hw_reset(workingGrp, vpss_ctx, CVI_FALSE);
		vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
		goto vpss_next_job;
	}

	// commit hw settings of this vpss-grp.
	if (commitHWSettings(workingGrp, ctx->online_from_isp, vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) apply hw settings NG.\n", workingGrp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
		goto vpss_next_job;
	}

	if (fill_buffers(workingGrp, vpss_ctx, ctx->online_from_isp) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);
		vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
		goto vpss_next_job;
	}

	/* Configure vc h/w before vpss h/w */
	_notify_vc_sb_mode(workingGrp, vpss_ctx, VCODEC_CB_SEND_FRM, CVI_FALSE);
	fill_sb_buffers(workingGrp, vpss_ctx, true);

	/* Update state first, isr could occur immediately */
	ctx->workingGrp = workingGrp;
	ctx->workingMask = workingMask;
	vpssExtCtx[workingGrp].grp_state = GRP_STATE_HW_STARTED;

	if (hw_start(workingGrp, CVI_TRUE, CVI_FALSE, vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) run NG.\n", workingGrp);
		hw_reset(workingGrp, vpss_ctx, CVI_FALSE);
		vpssCtx[workingGrp]->stGrpWorkStatus.u32StartFailCnt++;
		goto vpss_next_job;
	}

	/* Configure vpss h/w before dwa h/w */
	_notify_dwa_sbm_cfg_done(workingGrp);

	/* Should use async sbm, the lock region is too big ! */
	mutex_unlock(&vpssCtx[workingGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] workingGrp=%d, img_idx=%d\n",
			ctx->u8VpssDev, ctx->workingGrp, ctx->img_idx);

	// wait for h/w done
	return CVI_SUCCESS;

vpss_next_job:
	// job done.
	ctx->workingMask = 0;
	vpssExtCtx[workingGrp].grp_state = GRP_STATE_IDLE;
	if (hw_start(workingGrp, CVI_FALSE, CVI_FALSE, vpss_ctx) != CVI_SUCCESS)
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) stop at job-done NG.\n", workingGrp);

	mutex_unlock(&vpssCtx[workingGrp]->lock);

	return CVI_FAILURE;
}

static CVI_VOID vpss_handle_frame_done(struct vpss_handler_ctx *ctx)
{
	VPSS_GRP workingGrp = ctx->workingGrp;
	CVI_U8 workingMask = ctx->workingMask;
	struct cvi_vpss_ctx *vpss_ctx = &ctx->vpssCtxWork[0];
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};
	VB_BLK blk;
	VPSS_CHN VpssChn;
	CVI_U32 duration = 0, HwDuration = 0;
	struct timespec64 time;
	struct vb_s *vb;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "ctx[%d] grp(%d) eof\n", ctx->u8VpssDev, workingGrp);

	vpssExtCtx[workingGrp].grp_state = GRP_STATE_IDLE;

	vb_dqbuf(chn, CHN_TYPE_IN, &blk);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.enModId);
	} else {
		vb_done_handler(chn, CHN_TYPE_IN, blk);
	}

	VpssChn = 0;
	do {
		if (!(workingMask & BIT(VpssChn)))
			continue;

		//check if early interrupt is valid on this chn
		//if (ev.type == V4L2_EVENT_CVI_VIP_VPSS_EARLY_INT
		//	&& (!(earlyIntMask & BIT(VpssChn))
		//	|| !vpss_ctx->stChnCfgs[VpssChn].stLowDelayInfo.bEnable))
		//	continue;
		workingMask &= ~BIT(VpssChn);

		if (!vpss_ctx->stChnCfgs[VpssChn].isEnabled)
			continue;

		chn.s32ChnId = VpssChn;

		if (vpss_ctx->stChnCfgs[VpssChn].stBufWrap.bEnable &&
			vpss_ctx->stChnCfgs[VpssChn].bufWrapPhyAddr) {
			_update_vpss_chn_proc(workingGrp, VpssChn);
			continue;
		}
		_vb_dqbuf(chn, CHN_TYPE_OUT, &blk);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n"
				     , chn.enModId);
			continue;
		}

		vb = (struct vb_s *)blk;
		if (vb->buf.flags == 1) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) drop frame.\n", workingGrp, VpssChn);
			vb_release_block(blk);
			continue;
		}

		// skip rgn
		if (_vpss_check_gdc_job(chn, blk, vpss_ctx) != CVI_TRUE)
			vb_done_handler(chn, CHN_TYPE_OUT, blk);

		CVI_TRACE_VPSS(CVI_DBG_WARN, "grp(%d) chn(%d) end\n", workingGrp, VpssChn);
		_update_vpss_chn_proc(workingGrp, VpssChn);
	} while (++VpssChn < vpss_ctx->chnNum);

	// Remove img's mask if all sc handled
	if (!(workingMask & ~(BIT(7))))
		workingMask = 0;

	ctx->workingMask = workingMask;

	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);
	HwDuration = get_diff_in_us(vip_dev->img_vdev[ctx->img_idx].ts_start,
				    vip_dev->img_vdev[ctx->img_idx].ts_end);

	// Update vpss grp proc info
	_update_vpss_grp_proc(workingGrp, duration, HwDuration);
}

static void vpss_handle_offline(struct vpss_handler_ctx *ctx)
{
	VPSS_GRP workingGrp;
	CVI_U8 workingMask;
	struct timespec64 time;
	CVI_U64 duration64;
	CVI_U32 state;
	CVI_U32 events;
	unsigned long flags_job;

	_update_vpss_chnRealFrameRate();

	spin_lock_irqsave(&ctx->lock, flags_job);
	events = atomic_read(&ctx->events);
	workingGrp = ctx->workingGrp;
	workingMask = ctx->workingMask;
	spin_unlock_irqrestore(&ctx->lock, flags_job);

	// Find next working group when
	//   1. First group started (may not grp(0))
	//   2. after frame done of current working group
	if (workingMask == 0 && workingGrp == VPSS_MAX_GRP_NUM) {
		// find grp has job todo.
		workingGrp = findNextEnGrp(workingGrp, ctx->u8VpssDev);
		ctx->workingGrp = workingGrp;

		if (workingGrp == VPSS_MAX_GRP_NUM)
			return;
	}

	// Sanity check
	if (workingGrp >= VPSS_MAX_GRP_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) invalid\n", workingGrp);
		return;
	}
	if (!vpssCtx[workingGrp] || !vpssCtx[workingGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) invalid,\n", workingGrp);
		ctx->workingGrp = VPSS_MAX_GRP_NUM;
		ctx->workingMask = 0;
		atomic_set(&ctx->events, 0);
		return;
	}

	state = vpssExtCtx[workingGrp].grp_state;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] event=0x%x, Grp(%d) state=%d, mask=0x%x\n",
			ctx->u8VpssDev, events, workingGrp, vpssExtCtx[workingGrp].grp_state,
			workingMask);

	if (vpssExtCtx[workingGrp].grp_state == GRP_STATE_IDLE) {
		atomic_set(&ctx->events, 0);
		vpss_try_schedule_offline(ctx);
	} else if (vpssExtCtx[workingGrp].grp_state == GRP_STATE_HW_STARTED) {
		ktime_get_ts64(&time);
		duration64 = get_diff_in_us(ctx->time, time);
		do_div(duration64, 1000);

		if (events & CTX_EVENT_EOF) {
			vpss_handle_frame_done(ctx);
			spin_lock_irqsave(&ctx->lock, flags_job);
			atomic_set(&ctx->events, 0);
			spin_unlock_irqrestore(&ctx->lock, flags_job);
		} else {
			if (duration64 > EOF_WAIT_TIMEOUT_MS) {
				/* timeout */
				CVI_TRACE_VPSS(CVI_DBG_ERR, "ctx[%d] event timeout on grp(%d)\n",
						ctx->u8VpssDev, workingGrp);
				sclr_check_register();
				atomic_set(&ctx->events, 0);
				ctx->workingGrp = VPSS_MAX_GRP_NUM;
				ctx->workingMask = 0;
				vpssExtCtx[workingGrp].grp_state = GRP_STATE_IDLE;
				hw_reset(workingGrp, &ctx->vpssCtxWork[0], CVI_FALSE);
			} else {
				// keep waiting
			}
		}
	} else {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "ctx[%d] grp(%d) unexpected state=%d, events=0x%x\n",
				ctx->u8VpssDev, workingGrp, vpssExtCtx[workingGrp].grp_state,
				events);
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] evt=0x%x, mask=0x%x, Grp(%d) state 0x%x->0x%x\n",
			ctx->u8VpssDev, atomic_read(&ctx->events), ctx->workingMask, workingGrp, state,
			vpssExtCtx[workingGrp].grp_state);
}

static CVI_BOOL vpss_handler_is_idle(CVI_U8 u8VpssDev)
{
	int i;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (vpssCtx[i] && vpssCtx[i]->isCreated && vpssCtx[i]->isStarted &&
			vpssCtx[i]->u8DevId == u8VpssDev)
			return CVI_FALSE;

	return CVI_TRUE;
}

static int vpss_event_handler(void *arg)
{
	struct vpss_handler_ctx *ctx = (struct vpss_handler_ctx *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	int ret, i;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(ctx->wait, ctx->events.counter ||
			kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && vpss_handler_is_idle(ctx->u8VpssDev))
			continue;

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "ctx[%d] state=%d, %s, events=0x%x\n",
				ctx->u8VpssDev, ctx->enHdlState,
				ctx->online_from_isp ? "online" : "offline",
				atomic_read(&ctx->events));

		atomic_fetch_and(~CTX_EVENT_WKUP, &ctx->events);

		if (ctx->enHdlState != HANDLER_STATE_RUN) {
			atomic_set(&ctx->events, 0);
			continue;
		}

		if (ctx->online_from_isp) {
			vpss_handle_online(ctx);
			//The vpss failed to wake up once
			for (i = 0; i < VPSS_ONLINE_NUM; i++) {
				if (ctx->isr_evt[i]) {
					if (!vpssCtx[i] || !vpssCtx[i]->isStarted) {
						ctx->isr_evt[i] = 0;
						continue;
					}
					atomic_fetch_or(CTX_EVENT_EOF, &ctx->events);
					ctx->workingGrp = i;
					break;
				}
			}
		} else {
			vpss_handle_offline(ctx);

			// check if there are still unfinished jobs
			if (!ctx->workingMask) {
				ctx->workingGrp
					= findNextEnGrp(ctx->workingGrp, ctx->u8VpssDev);

				// unfinished job found, need to re-trig event handler
				if (ctx->workingGrp != VPSS_MAX_GRP_NUM)
					atomic_fetch_or(CTX_EVENT_WKUP, &ctx->events);
			}
		}

		/* Adjust timeout */
		timeout = vpss_handler_is_idle(ctx->u8VpssDev) ? idle_timeout : eof_timeout;
	}

	return 0;
}


void _vpss_GrpParamInit(VPSS_GRP VpssGrp)
{
	CVI_U8 i, j, k;
	PROC_AMP_CTRL_S ctrl;
	struct sclr_csc_matrix *mtrx;

	memset(&vpssCtx[VpssGrp]->stGrpCropInfo, 0, sizeof(vpssCtx[VpssGrp]->stGrpCropInfo));
	memset(&vpssCtx[VpssGrp]->frame_crop, 0, sizeof(vpssCtx[VpssGrp]->frame_crop));
	memset(&vpssCtx[VpssGrp]->stGrpWorkStatus, 0, sizeof(vpssCtx[VpssGrp]->stGrpWorkStatus));

	for (i = 0; i < vpssCtx[VpssGrp]->chnNum; ++i) {
		memset(&vpssCtx[VpssGrp]->stChnCfgs[i], 0, sizeof(vpssCtx[VpssGrp]->stChnCfgs[i]));
		vpssCtx[VpssGrp]->stChnCfgs[i].enCoef = VPSS_SCALE_COEF_BICUBIC;
		vpssCtx[VpssGrp]->stChnCfgs[i].align = DEFAULT_ALIGN;
		vpssCtx[VpssGrp]->stChnCfgs[i].YRatio = YRATIO_SCALE;
		vpssCtx[VpssGrp]->stChnCfgs[i].VbPool = VB_INVALID_POOLID;
		mutex_init(&mesh[VpssGrp][i].lock);

		for (j = 0; j < RGN_MAX_LAYER_VPSS; ++j)
			for (k = 0; k < RGN_MAX_NUM_VPSS; ++k)
				vpssCtx[VpssGrp]->stChnCfgs[i].rgn_handle[j][k] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_COVEREX_MAX_NUM; ++j)
			vpssCtx[VpssGrp]->stChnCfgs[i].coverEx_handle[j] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_MOSAIC_MAX_NUM; ++j)
			vpssCtx[VpssGrp]->stChnCfgs[i].mosaic_handle[j] = RGN_INVALID_HANDLE;
	}

	vpssExtCtx[VpssGrp].scene = 0xff;
	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i) {
		vpss_get_proc_amp_ctrl(i, &ctrl);
		vpssExtCtx[VpssGrp].proc_amp[i] = ctrl.default_value;
	}

	// use designer provided table
	mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_YUV2RGB);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].csc_cfg.coef[i][j] = mtrx->coef[i][j];

		vpssExtCtx[VpssGrp].csc_cfg.add[i] = mtrx->add[i];
		vpssExtCtx[VpssGrp].csc_cfg.sub[i] = mtrx->sub[i];
	}
}

static CVI_S32 _vpss_update_rotation_mesh(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation)
{
	struct cvi_gdc_mesh *pmesh = &mesh[VpssGrp][VpssChn];

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) rotation(%d).\n",
			VpssGrp, VpssChn, enRotation);

	pmesh->paddr = DEFAULT_MESH_PADDR;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = enRotation;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	return CVI_SUCCESS;
}

CVI_S32 _vpss_update_ldc_mesh(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
	const VPSS_LDC_ATTR_S *pstLDCAttr, ROTATION_E enRotation, CVI_U64 paddr)
{
	CVI_U64 paddr_old;
	struct cvi_gdc_mesh *pmesh = &mesh[VpssGrp][VpssChn];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;
	mutex_unlock(&pmesh->lock);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr = *pstLDCAttr;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = enRotation;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	//mutex_unlock(&pmesh->lock);

	//if (paddr_old)
	//	CVI_SYS_IonFree(paddr_old, vaddr_old);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
		      , VpssGrp, VpssChn, (unsigned long long)paddr);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bEnable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d\n",
			pstLDCAttr->bEnable, pstLDCAttr->stAttr.bAspect,
			pstLDCAttr->stAttr.s32XYRatio, pstLDCAttr->stAttr.s32CenterXOffset,
			pstLDCAttr->stAttr.s32CenterYOffset, pstLDCAttr->stAttr.s32DistortionRatio);
	return CVI_SUCCESS;
}

CVI_VOID _clean_chn_vb_jobs(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
			const VPSS_CHN_ATTR_S *pstChnAttrOld, const VPSS_CHN_ATTR_S *pstChnAttrNew)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};

	if ((pstChnAttrOld->u32Width != pstChnAttrNew->u32Width) ||
		(pstChnAttrOld->u32Height != pstChnAttrNew->u32Height) ||
		(pstChnAttrOld->enVideoFormat != pstChnAttrNew->enVideoFormat) ||
		(pstChnAttrOld->enPixelFormat != pstChnAttrNew->enPixelFormat) ||
		(pstChnAttrOld->bMirror != pstChnAttrNew->bMirror) ||
		(pstChnAttrOld->bFlip != pstChnAttrNew->bFlip) ||
		memcmp(&pstChnAttrOld->stAspectRatio, &pstChnAttrNew->stAspectRatio, sizeof(ASPECT_RATIO_S)) ||
		memcmp(&pstChnAttrOld->stNormalize, &pstChnAttrNew->stNormalize, sizeof(VPSS_NORMALIZE_S))) {
		_clean_vpss_workq(chn);
		_release_vpss_doneq(chn);
	}

	if (pstChnAttrOld->u32Depth || (pstChnAttrNew == 0))
		_release_vpss_doneq(chn);
}

/* vpss_postjob: to add the count of job of the vpss-dev.
 *
 * @param dev_id: the vpss group which has this job.
 */
void vpss_post_job(CVI_S32 VpssGrp)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not started yet.\n", VpssGrp);
		return;
	}

	vpssCtx[VpssGrp]->stGrpWorkStatus.u32RecvCnt++;
	vpss_notify_wkup_evt(vpssCtx[VpssGrp]->u8DevId);
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
CVI_S32 vpss_create_grp(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr)
{
	CVI_U32 u32MinHeight;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	CVI_S32 ret;
	CVI_U8 u8DevUsed;
	VPSS_INPUT_E enInput;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_FMT(VpssGrp, pstGrpAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_YUV_PARAM(pstGrpAttr->enPixelFormat, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstGrpAttr->enPixelFormat) ? 2 : 1;
	if ((pstGrpAttr->u32MaxW < VPSS_MIN_IMAGE_WIDTH) || (pstGrpAttr->u32MaxH < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) u32MaxW(%d) or u32MaxH(%d) too small\n"
			, VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) is occupied\n", VpssGrp);
		return CVI_ERR_VPSS_EXIST;
	}

	u8DevUsed = pstGrpAttr->u8VpssDev;
	if (stVPSSMode.enMode == VPSS_MODE_SINGLE) {
		u8DevUsed = 1;
		enInput = stVPSSMode.aenInput[0];
	} else {
		if (u8DevUsed > 1) {
			u8DevUsed = 1;
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VPSS Dual mode only allow VpssDev 0/1.\n");
		}
		enInput = stVPSSMode.aenInput[u8DevUsed];
	}

	if (VpssGrp < VPSS_ONLINE_NUM) {
		if ((stVIVPSSMode.aenMode[VpssGrp] == VI_OFFLINE_VPSS_ONLINE
			|| stVIVPSSMode.aenMode[VpssGrp] == VI_ONLINE_VPSS_ONLINE)
			&& enInput != VPSS_INPUT_ISP) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) using dev(%d), should use online dev\n",
				VpssGrp, u8DevUsed);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	vpssCtx[VpssGrp] = kzalloc(sizeof(struct cvi_vpss_ctx), GFP_ATOMIC);
	if (!vpssCtx[VpssGrp]) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "kzalloc fail.\n");
		return CVI_ERR_VPSS_NOMEM;
	}
	vpssCtx[VpssGrp]->hw_cfgs = kzalloc(sizeof(struct VPSS_GRP_HW_CFG), GFP_ATOMIC);
	if (!vpssCtx[VpssGrp]->hw_cfgs) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "kzalloc fail.\n");
		kfree(vpssCtx[VpssGrp]);
		vpssCtx[VpssGrp] = NULL;
		return CVI_ERR_VPSS_NOMEM;
	}

	vpssCtx[VpssGrp]->isCreated = CVI_TRUE;
	memcpy(&vpssCtx[VpssGrp]->stGrpAttr, pstGrpAttr, sizeof(vpssCtx[VpssGrp]->stGrpAttr));

	base_mod_jobs_init(chn, CHN_TYPE_IN, 1 + CVI_VI_VPSS_EXTRA_BUF, 1, 0);

	mutex_init(&vpssCtx[VpssGrp]->lock);

	vpssCtx[VpssGrp]->u8DevId = u8DevUsed;
	vpssCtx[VpssGrp]->chnNum = (stVPSSMode.enMode == VPSS_MODE_SINGLE) ?
		VPSS_MAX_CHN_NUM : (u8DevUsed == 0) ? 1 : VPSS_MAX_CHN_NUM - 1;
	//handler_ctx[u8DevUsed].online_from_isp = (enInput == VPSS_INPUT_ISP);

	_vpss_GrpParamInit(VpssGrp);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Use dev=%d, online_from_isp=%d\n",
		u8DevUsed, handler_ctx[u8DevUsed].online_from_isp);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) VpssDev(%d) u32MaxW(%d) u32MaxH(%d) PixelFmt(%d)\n",
		VpssGrp, pstGrpAttr->u8VpssDev, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH, pstGrpAttr->enPixelFormat);
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 vpss_destroy_grp(VPSS_GRP VpssGrp)
{
	CVI_U8 VpssChn;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;
	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;

	// FIXME: free ion until dwa hardware stops
	if (vpssCtx[VpssGrp]->isCreated) {
		mutex_lock(&vpssCtx[VpssGrp]->lock);
		vpssCtx[VpssGrp]->isCreated = CVI_FALSE;
		vpssExtCtx[VpssGrp].sbm_cfg.sb_mode = 0;
		base_mod_jobs_exit(chn, CHN_TYPE_IN);

		for (VpssChn = 0; VpssChn < vpssCtx[VpssGrp]->chnNum; ++VpssChn) {
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = ROTATION_0;
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr.bEnable = CVI_FALSE;

			if (mesh[VpssGrp][VpssChn].paddr) {
				if (mesh[VpssGrp][VpssChn].paddr != DEFAULT_MESH_PADDR) {
					sys_ion_free(mesh[VpssGrp][VpssChn].paddr);
				}
				mesh[VpssGrp][VpssChn].paddr = 0;
				mesh[VpssGrp][VpssChn].vaddr = 0;
			}
		}
		mutex_unlock(&vpssCtx[VpssGrp]->lock);
	}
	kfree(vpssCtx[VpssGrp]->hw_cfgs);
	vpssCtx[VpssGrp]->hw_cfgs = NULL;
	kfree(vpssCtx[VpssGrp]);
	vpssCtx[VpssGrp] = NULL;
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

VPSS_GRP vpss_get_available_grp(void)
{
	VPSS_GRP grp = 0;
	CVI_U8 i;

	for (i = 0; i < VI_MAX_PIPE_NUM; i++)
		if ((stVIVPSSMode.aenMode[i] == VI_ONLINE_VPSS_ONLINE) ||
			(stVIVPSSMode.aenMode[i] == VI_OFFLINE_VPSS_ONLINE)) {
			grp = VPSS_ONLINE_NUM;
			break;
		}


	for (; grp < VPSS_MAX_GRP_NUM; ++grp)
		if (!vpssCtx[grp])
			return grp;
	return VPSS_INVALID_GRP;
}

CVI_S32 vpss_start_grp(VPSS_GRP VpssGrp)
{
	CVI_U8 u8VpssDev;
	CVI_S32 ret;
	CVI_U8 i;
	unsigned long flags_job;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	u8VpssDev = vpssCtx[VpssGrp]->u8DevId;

	if (vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) already started.\n", VpssGrp);
		return CVI_SUCCESS;
	}

	if (handler_ctx[u8VpssDev].online_from_isp) {
		if (VpssGrp >= VPSS_ONLINE_NUM) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "online VpssGrp(%d) should be %d - %d.\n",
				       VpssGrp, VPSS_ONLINE_GRP_0, VPSS_ONLINE_NUM - 1);
			return CVI_ERR_VPSS_BUSY;
		}
		handler_ctx[u8VpssDev].online_chkMask |= BIT(VpssGrp);
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->isStarted = CVI_TRUE;
	vpssExtCtx[VpssGrp].grp_state = GRP_STATE_IDLE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	/* Only change state from stop to run */
	if (handler_ctx[u8VpssDev].enHdlState == HANDLER_STATE_STOP) {
		spin_lock_irqsave(&handler_ctx[u8VpssDev].lock, flags_job);
		if (handler_ctx[u8VpssDev].online_from_isp)
			handler_ctx[u8VpssDev].workingGrp = 0;
		else
			handler_ctx[u8VpssDev].workingGrp = VPSS_MAX_GRP_NUM;
		atomic_set(&handler_ctx[u8VpssDev].events, 0);
		handler_ctx[u8VpssDev].workingMask = 0;
		handler_ctx[u8VpssDev].enHdlState = HANDLER_STATE_RUN;
		ktime_get_ts64(&handler_ctx[u8VpssDev].time);
		for (i = 0; i < VPSS_ONLINE_NUM; i++)
			handler_ctx[u8VpssDev].isr_evt[i] = 0;
		spin_unlock_irqrestore(&handler_ctx[u8VpssDev].lock, flags_job);
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);
	vpss_notify_wkup_evt(u8VpssDev);

	return CVI_SUCCESS;
}

CVI_S32 vpss_stop_grp(VPSS_GRP VpssGrp)
{
	CVI_S32 ret;
	CVI_U8 u8VpssDev;
	VPSS_CHN VpssChn = 0;
	CVI_U8 i;
	CVI_BOOL enabled;
	struct VPSS_CHN_WORK_STATUS_S *pstChnStatus;
	enum handler_state state;
	unsigned long flags_job;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;
	if (!vpssCtx[VpssGrp]->isStarted)
		return CVI_SUCCESS;

	u8VpssDev = vpssCtx[VpssGrp]->u8DevId;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->isStarted = CVI_FALSE;
	if (handler_ctx[u8VpssDev].online_from_isp)
		handler_ctx[u8VpssDev].online_chkMask &= ~BIT(VpssGrp);

	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	/* Only change state from run to stop */
	enabled = vpss_enable_handler_ctx(&handler_ctx[u8VpssDev]);
	state = handler_ctx[u8VpssDev].enHdlState;
	if (!enabled && state == HANDLER_STATE_RUN) {
		hw_start(VpssGrp, CVI_FALSE, CVI_FALSE, vpssCtx[VpssGrp]);
		handler_ctx[u8VpssDev].enHdlState = HANDLER_STATE_STOP;
		CVI_TRACE_VPSS(CVI_DBG_WARN, "handler_ctx[%d] stop\n", u8VpssDev);
		spin_lock_irqsave(&handler_ctx[u8VpssDev].lock, flags_job);
		handler_ctx[u8VpssDev].workingMask = 0;
		atomic_set(&handler_ctx[u8VpssDev].events, 0);
		for (i = 0; i < VPSS_ONLINE_NUM; i++)
			handler_ctx[u8VpssDev].isr_evt[i] = 0;
		spin_unlock_irqrestore(&handler_ctx[u8VpssDev].lock, flags_job);
	}
	vpssExtCtx[VpssGrp].grp_state = GRP_STATE_IDLE;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	if (handler_ctx[u8VpssDev].online_from_isp && _is_vc_sbm_enabled(VpssGrp, vpssCtx[VpssGrp])) {
		for (VpssChn = 0; VpssChn < vpssCtx[VpssGrp]->chnNum; ++VpssChn) {
			/*if grp online && sbm ,disable channel should in grp stop*/
			chn.s32ChnId = VpssChn;
			mutex_lock(&vpssCtx[VpssGrp]->lock);
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled = CVI_FALSE;
			if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr) {
				sys_ion_free(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr);
				vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr = 0;
			}
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap.bEnable = CVI_FALSE;
			pstChnStatus = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnWorkStatus;
			pstChnStatus->u32SendOk = 0;
			pstChnStatus->u64PrevTime = 0;
			pstChnStatus->u32FrameNum = 0;
			pstChnStatus->u32RealFrameRate = 0;
			base_mod_jobs_exit(chn, CHN_TYPE_OUT);
			mutex_unlock(&vpssCtx[VpssGrp]->lock);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) Chn(%d) disable\n", VpssGrp, VpssChn);
		}
		return CVI_SUCCESS;
	}

	return CVI_SUCCESS;
}

CVI_S32 vpss_reset_grp(VPSS_GRP VpssGrp)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	vpss_stop_grp(VpssGrp);
	mutex_lock(&vpssCtx[VpssGrp]->lock);
	memset(vpssCtx[VpssGrp]->stChnCfgs, 0, sizeof(struct VPSS_CHN_CFG) * vpssCtx[VpssGrp]->chnNum);
	memset(&vpssExtCtx[VpssGrp], 0, sizeof(vpssExtCtx[VpssGrp]));
	_vpss_GrpParamInit(VpssGrp);
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_grp_attr(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstGrpAttr)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	*pstGrpAttr = vpssCtx[VpssGrp]->stGrpAttr;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_grp_attr(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr)
{
	CVI_U32 u32MinHeight;
	CVI_S32 ret;
	CVI_U8 u8DevUsed;
	VPSS_INPUT_E enInput;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_FMT(VpssGrp, pstGrpAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_YUV_PARAM(pstGrpAttr->enPixelFormat, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstGrpAttr->enPixelFormat) ? 2 : 1;
	if ((pstGrpAttr->u32MaxW < VPSS_MIN_IMAGE_WIDTH) || (pstGrpAttr->u32MaxH < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) u32MaxW(%d) or u32MaxH(%d) too small\n"
			, VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (handler_ctx[vpssCtx[VpssGrp]->u8DevId].online_from_isp && vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) operation not allowed if vi-2-vpss online\n", VpssGrp);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (pstGrpAttr->stFrameRate.s32SrcFrameRate != pstGrpAttr->stFrameRate.s32DstFrameRate)
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) dst(%d), not support yet.\n"
				, VpssGrp, pstGrpAttr->stFrameRate.s32SrcFrameRate
				, pstGrpAttr->stFrameRate.s32DstFrameRate);

	u8DevUsed = pstGrpAttr->u8VpssDev;
	if (stVPSSMode.enMode == VPSS_MODE_SINGLE) {
		u8DevUsed = 1;
		enInput = stVPSSMode.aenInput[0];
	} else {
		if (u8DevUsed > 1) {
			u8DevUsed = 1;
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VPSS Dual mode only allow VpssDev 0/1.\n");
		}
		enInput = stVPSSMode.aenInput[u8DevUsed];
	}

	if (VpssGrp < VPSS_ONLINE_NUM) {
		if ((stVIVPSSMode.aenMode[VpssGrp] == VI_OFFLINE_VPSS_ONLINE
			|| stVIVPSSMode.aenMode[VpssGrp] == VI_ONLINE_VPSS_ONLINE)
			&& enInput != VPSS_INPUT_ISP) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) using dev(%d), should use online dev\n",
				VpssGrp, u8DevUsed);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stGrpAttr = *pstGrpAttr;
	vpssCtx[VpssGrp]->u8DevId = u8DevUsed;
	vpssCtx[VpssGrp]->chnNum = (stVPSSMode.enMode == VPSS_MODE_SINGLE) ?
		VPSS_MAX_CHN_NUM : (u8DevUsed == 0) ? 1 : VPSS_MAX_CHN_NUM - 1;
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) VpssDev(%d) u32MaxW(%d) u32MaxH(%d) PixelFmt(%d)\n",
		VpssGrp, pstGrpAttr->u8VpssDev, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH, pstGrpAttr->enPixelFormat);
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg)
{
	CVI_S32 ret;
	VPSS_GRP VpssGrp = cfg->VpssGrp;
	CVI_U8 i, j;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].csc_cfg.coef[i][j] = cfg->coef[i][j];

		vpssExtCtx[VpssGrp].csc_cfg.add[i] = cfg->add[i];
		vpssExtCtx[VpssGrp].csc_cfg.sub[i] = cfg->sub[i];
	}
	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		vpssExtCtx[VpssGrp].proc_amp[i] = cfg->proc_amp[i];
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	vpssExtCtx[VpssGrp].scene = cfg->scene;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_proc_amp_ctrl(PROC_AMP_E type, PROC_AMP_CTRL_S *ctrl)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VPSS, ctrl);

	if (type >= PROC_AMP_MAX) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "ProcAmp type(%d) invalid.\n", type);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	*ctrl = procamp_ctrls[type];
	return CVI_SUCCESS;
}

CVI_S32 vpss_get_proc_amp(VPSS_GRP VpssGrp, CVI_S32 *proc_amp)
{
	CVI_S32 ret;
	CVI_U8 i;

	MOD_CHECK_NULL_PTR(CVI_ID_VPSS, proc_amp);

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		proc_amp[i] = vpssExtCtx[VpssGrp].proc_amp[i];

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_binscene(struct vpss_scene *cfg)
{
	CVI_S32 ret;
	VPSS_GRP VpssGrp = cfg->VpssGrp;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	cfg->scene = vpssExtCtx[VpssGrp].scene;

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg)
{
	CVI_U8 i, j;

	MOD_CHECK_NULL_PTR(CVI_ID_VPSS, cfg);

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j)
			cfg->proc_amp[i][j] = vpssExtCtx[i].proc_amp[j];

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_ATTR_S *pstChnAttr)
{
	VB_CAL_CONFIG_S stVbCalConfig;
	CVI_U32 u32MinHeight;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_FMT(VpssGrp, VpssChn, pstChnAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_YUV_PARAM(pstChnAttr->enPixelFormat, pstChnAttr->u32Width, pstChnAttr->u32Height);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstChnAttr->enPixelFormat) ? 2 : 1;
	if ((pstChnAttr->u32Width < VPSS_MIN_IMAGE_WIDTH) || (pstChnAttr->u32Height < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) u32Width(%d) or u32Height(%d) too small\n"
			, VpssGrp, VpssChn, pstChnAttr->u32Width, pstChnAttr->u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (!memcmp(&vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, pstChnAttr, sizeof(*pstChnAttr))) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) attr not changed\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}

	if (pstChnAttr->stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
		const RECT_S *rect = &pstChnAttr->stAspectRatio.stVideoRect;

		if (!pstChnAttr->stAspectRatio.bEnableBgColor) {
			ret = CHECK_YUV_PARAM(pstChnAttr->enPixelFormat, rect->u32Width, rect->u32Height);
			if (ret != CVI_SUCCESS)
				return ret;
			if ((IS_FMT_YUV420(pstChnAttr->enPixelFormat) || IS_FMT_YUV422(pstChnAttr->enPixelFormat))
				&& (rect->s32X & 0x01)) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
				CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420/YUV_422 rect s32X(%d) should be even.\n",
					rect->s32X);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		}

		if ((rect->s32X < 0) || (rect->s32Y < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect pos(%d %d) can't be negative.\n"
				, VpssGrp, VpssChn, rect->s32X, rect->s32Y);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->u32Width < 4) || (rect->u32Height < u32MinHeight)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect size(%d %d) can't smaller than 4x%d\n"
				, VpssGrp, VpssChn, rect->u32Width, rect->u32Height, u32MinHeight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->s32X + rect->u32Width > pstChnAttr->u32Width)
		|| (rect->s32Y + rect->u32Height > pstChnAttr->u32Height)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect(%d %d %d %d) output-size(%d %d).\n"
					, VpssGrp, VpssChn
					, rect->s32X, rect->s32Y, rect->u32Width, rect->u32Height
					, pstChnAttr->u32Width, pstChnAttr->u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	if (pstChnAttr->stFrameRate.s32SrcFrameRate < pstChnAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) Chn(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, VpssGrp, VpssChn, pstChnAttr->stFrameRate.s32SrcFrameRate
				, pstChnAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	COMMON_GetPicBufferConfig(pstChnAttr->u32Width, pstChnAttr->u32Height,
		pstChnAttr->enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	_clean_chn_vb_jobs(VpssGrp, VpssChn, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, pstChnAttr);
	memcpy(&vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, pstChnAttr,
		sizeof(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr));
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].blk_size = stVbCalConfig.u32VBSize;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align = DEFAULT_ALIGN;

	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) u32Width(%d), u32Height(%d)\n"
		, VpssGrp, VpssChn, pstChnAttr->u32Width, pstChnAttr->u32Height);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstChnAttr, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, sizeof(*pstChnAttr));
	return CVI_SUCCESS;
}

CVI_S32 vpss_enable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	struct VPSS_CHN_CFG *chn_cfg;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	CVI_S32 ret;
	CVI_U8 u8VpssDev;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	chn_cfg = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn];
	if (chn_cfg->isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) already enabled\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}
	u8VpssDev = vpssCtx[VpssGrp]->u8DevId;
	mutex_lock(&vpssCtx[VpssGrp]->lock);

	if (handler_ctx[u8VpssDev].online_from_isp)
		base_mod_jobs_init(chn, CHN_TYPE_OUT, 1, 2, chn_cfg->stChnAttr.u32Depth);
	else
		base_mod_jobs_init(chn, CHN_TYPE_OUT, 1, 1, chn_cfg->stChnAttr.u32Depth);

	chn_cfg->isEnabled = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	if (handler_ctx[u8VpssDev].online_from_isp
		&& vpssCtx[VpssGrp]->isStarted) {
		vpss_notify_wkup_evt(u8VpssDev);
	}

	return CVI_SUCCESS;
}

CVI_S32 vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	CVI_S32 ret;
	CVI_U8 dev_idx = 0;
	struct VPSS_CHN_WORK_STATUS_S *pstChnStatus;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isCreated) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) not created yet\n", VpssGrp);
		return CVI_SUCCESS;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) not enabled yet\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}

	dev_idx = vpssCtx[VpssGrp]->u8DevId;
	vb_cancel_block(chn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].blk_size,
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].VbPool);

	if (handler_ctx[dev_idx].online_from_isp &&
	    _is_vc_sbm_enabled(VpssGrp, vpssCtx[VpssGrp])) {
		/*if grp online && sbm ,disable channel should in grp stop*/
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) Chn(%d) after stop grp\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled = CVI_FALSE;
	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr) {
		sys_ion_free(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr);
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr = 0;
	}
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap.bEnable = CVI_FALSE;
	pstChnStatus = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnWorkStatus;
	pstChnStatus->u32SendOk = 0;
	pstChnStatus->u64PrevTime = 0;
	pstChnStatus->u32FrameNum = 0;
	pstChnStatus->u32RealFrameRate = 0;
	base_mod_jobs_exit(chn, CHN_TYPE_OUT);
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CROP_INFO_S *pstCropInfo)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstCropInfo->bEnable) {
		if ((pstCropInfo->stCropRect.u32Width < 4) || (pstCropInfo->stCropRect.u32Height < 1)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) crop size(%d %d) can't smaller than 4x1\n"
				, VpssGrp, VpssChn, pstCropInfo->stCropRect.u32Width
				, pstCropInfo->stCropRect.u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if (pstCropInfo->stCropRect.s32X + (CVI_S32)pstCropInfo->stCropRect.u32Width < 4
			|| pstCropInfo->stCropRect.s32Y + (CVI_S32)pstCropInfo->stCropRect.u32Height < 1) {
			CVI_TRACE_VPSS(CVI_DBG_ERR
				, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
				, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
				, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if (vpssCtx[VpssGrp]->stGrpCropInfo.bEnable) {
			if ((CVI_S32)vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height
				- pstCropInfo->stCropRect.s32Y < 1
				|| (CVI_S32)vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width
				- pstCropInfo->stCropRect.s32X < 4) {
				CVI_TRACE_VPSS(CVI_DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
					, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "grp crop size(%d %d)\n"
					, vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width
					, vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		} else {
			if ((CVI_S32)vpssCtx[VpssGrp]->stGrpAttr.u32MaxH - pstCropInfo->stCropRect.s32Y < 1
				|| (CVI_S32)vpssCtx[VpssGrp]->stGrpAttr.u32MaxW - pstCropInfo->stCropRect.s32X < 4) {
				CVI_TRACE_VPSS(CVI_DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
					, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "out of grp size(%d %d)\n"
					, vpssCtx[VpssGrp]->stGrpAttr.u32MaxW, vpssCtx[VpssGrp]->stGrpAttr.u32MaxH);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stCropInfo = *pstCropInfo;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d), bEnable=%d, rect(%d %d %d %d)\n",
		VpssGrp, VpssChn, pstCropInfo->bEnable,
		pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y,
		pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pstCropInfo = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stCropInfo;

	return CVI_SUCCESS;
}

CVI_S32 vpss_show_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isMuted = CVI_FALSE;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

CVI_S32 vpss_hide_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled)
		return CVI_SUCCESS;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isMuted = CVI_TRUE;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

CVI_S32 vpss_get_grp_crop(VPSS_GRP VpssGrp, VPSS_CROP_INFO_S *pstCropInfo)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	*pstCropInfo = vpssCtx[VpssGrp]->stGrpCropInfo;
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_grp_crop(VPSS_GRP VpssGrp, const VPSS_CROP_INFO_S *pstCropInfo)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (handler_ctx[vpssCtx[VpssGrp]->u8DevId].online_from_isp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not support crop if online\n", VpssGrp);
		return CVI_ERR_VPSS_NOT_SUPPORT;
	}

	if (pstCropInfo->bEnable) {
		CVI_U32 u32MinHeight = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)
				     ? 2 : 1;

		if ((pstCropInfo->stCropRect.s32X < 0) || (pstCropInfo->stCropRect.s32Y < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop start-point(%d %d) illegal\n"
				, VpssGrp, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((pstCropInfo->stCropRect.u32Width < 4) || (pstCropInfo->stCropRect.u32Height < u32MinHeight)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop size(%d %d) can't smaller than 4x%d\n"
				, VpssGrp, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height
				, u32MinHeight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((pstCropInfo->stCropRect.s32Y + pstCropInfo->stCropRect.u32Height)
			> vpssCtx[VpssGrp]->stGrpAttr.u32MaxH
		 || (pstCropInfo->stCropRect.s32X + pstCropInfo->stCropRect.u32Width)
			 > vpssCtx[VpssGrp]->stGrpAttr.u32MaxW) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop rect(%d %d %d %d) out of grp size(%d %d)\n"
				, VpssGrp, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
				, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height
				, vpssCtx[VpssGrp]->stGrpAttr.u32MaxW, vpssCtx[VpssGrp]->stGrpAttr.u32MaxH);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stGrpCropInfo = *pstCropInfo;
	if (pstCropInfo->bEnable) {
		bool chk_width_even = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat) ||
				      IS_FMT_YUV422(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat);
		bool chk_height_even = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat);

		if (chk_width_even && (pstCropInfo->stCropRect.u32Width & 0x01)) {
			vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width &= ~(0x0001);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) stCropRect.u32Width(%d) to even(%d) due to YUV\n",
				       VpssGrp, pstCropInfo->stCropRect.u32Width,
				       vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width);
		}
		if (chk_height_even && (pstCropInfo->stCropRect.u32Height & 0x01)) {
			vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height &= ~(0x0001);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) stCropRect.u32Height(%d) to even(%d) due to YUV\n",
				       VpssGrp, pstCropInfo->stCropRect.u32Height,
				       vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height);
		}
	}

	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d), bEnable=%d, rect(%d %d %d %d)\n",
		VpssGrp, pstCropInfo->bEnable,
		pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y,
		pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);

	return CVI_SUCCESS;
}

//TBD
CVI_S32 vpss_get_grp_frame(VPSS_GRP VpssGrp, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 ret;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	return CVI_SUCCESS;
}

CVI_S32 vpss_release_grp_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	CVI_S32 ret;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	return CVI_SUCCESS;
}

CVI_S32 vpss_send_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VB_BLK blk;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (handler_ctx[vpssCtx[VpssGrp]->u8DevId].online_from_isp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not support if online\n", VpssGrp);
		return CVI_ERR_VPSS_NOT_SUPPORT;
	}

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) PixelFormat(%d) mismatch.\n"
			, VpssGrp, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((vpssCtx[VpssGrp]->stGrpAttr.u32MaxW != pstVideoFrame->stVFrame.u32Width)
	 || (vpssCtx[VpssGrp]->stGrpAttr.u32MaxH != pstVideoFrame->stVFrame.u32Height)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Size(%d * %d) mismatch.\n"
			, VpssGrp, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) frame offset (%d %d %d %d) invalid\n",
			VpssGrp, pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight,
			pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid width\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		if ((pstVideoFrame->stVFrame.u32Height - pstVideoFrame->stVFrame.s16OffsetTop -
		     pstVideoFrame->stVFrame.s16OffsetBottom) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid height\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Height(%d) s16OffsetTop(%d) s16OffsetBottom(%d)\n",
				pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.s16OffsetTop,
				pstVideoFrame->stVFrame.s16OffsetBottom);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV422 can't accept odd frame valid width\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	blk = vb_physAddr2Handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
			return CVI_ERR_VPSS_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Invalid parameter\n", VpssGrp);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vb_qbuf(chn, CHN_TYPE_IN, blk) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) qbuf failed\n", VpssGrp);
		return CVI_ERR_VPSS_BUSY;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), phy-address(0x%llx)\n",
			VpssGrp, pstVideoFrame->stVFrame.u64PhyAddr[0]);

	return CVI_SUCCESS;
}

CVI_S32 vpss_send_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn
	, const VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	CVI_S32 ret;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	VB_BLK blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) PixelFormat(%d) mismatch.\n"
			, VpssGrp, VpssChn, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width != pstVideoFrame->stVFrame.u32Width)
	 || (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Height != pstVideoFrame->stVFrame.u32Height)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Size(%d * %d) mismatch.\n"
			, VpssGrp, VpssChn, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	UNUSED(s32MilliSec);

	blk = vb_physAddr2Handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
			return CVI_ERR_VPSS_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	chn.s32ChnId = VpssChn;
	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_OUT);
	if (!jobs) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) get job failed\n",
				VpssGrp, VpssChn);
		return CVI_FAILURE;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", VpssGrp, VpssChn);
		mutex_unlock(&jobs->lock);
		return CVI_FAILURE;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	mutex_unlock(&jobs->lock);
	atomic_fetch_add(1, &vb->usr_cnt);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return ret;
}

CVI_S32 vpss_get_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
			   CVI_S32 s32MilliSec)
{
	CVI_S32 ret, i;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstFrameInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_NOTREADY;
	}

	memset(pstFrameInfo, 0, sizeof(*pstFrameInfo));
	ret = base_get_chn_buffer(chn, &blk, s32MilliSec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn frame fail, s32MilliSec=%d, ret=%d\n",
				VpssGrp, VpssChn, s32MilliSec, ret);
		return CVI_ERR_VPSS_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.u32Width) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "buf already released\n");
		return CVI_ERR_VPSS_BUF_EMPTY;
	}

	pstFrameInfo->stVFrame.enPixelFormat = vb->buf.enPixelFormat;
	pstFrameInfo->stVFrame.u32Width = vb->buf.size.u32Width;
	pstFrameInfo->stVFrame.u32Height = vb->buf.size.u32Height;
	pstFrameInfo->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstFrameInfo->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstFrameInfo->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstFrameInfo->stVFrame.u32Length[i] = vb->buf.length[i];
		pstFrameInfo->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstFrameInfo->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstFrameInfo->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstFrameInfo->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstFrameInfo->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstFrameInfo->stVFrame.pPrivateData = vb;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) end to set pstFrameInfo width:%d height:%d buf:0x%llx\n"
			, VpssGrp, VpssChn, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height,
			pstFrameInfo->stVFrame.u64PhyAddr[0]);
	return CVI_SUCCESS;
}

CVI_S32 vpss_release_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VB_BLK blk;
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	blk = vb_physAddr2Handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (pstVideoFrame->stVFrame.pPrivateData == 0) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , VpssGrp, VpssChn, (unsigned long long)pstVideoFrame->stVFrame.u64PhyAddr[0]);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (VB_BLK)pstVideoFrame->stVFrame.pPrivateData;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) buf:0x%llx\n",
			VpssGrp, VpssChn, pstVideoFrame->stVFrame.u64PhyAddr[0]);
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GDC_FMT(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	return _vpss_update_rotation_mesh(VpssGrp, VpssChn, enRotation);
}

CVI_S32 vpss_get_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E *penRotation)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, penRotation);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*penRotation = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation;
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 u32Align)
{
	CVI_S32 ret;
	VB_CAL_CONFIG_S stVbCalConfig;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (u32Align < DEFAULT_ALIGN) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) alignment(%d) must be bigger than %d\n",
			       VpssGrp, VpssChn, u32Align, DEFAULT_ALIGN);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	COMMON_GetPicBufferConfig(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width,
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Height,
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat,
		DATA_BITWIDTH_8, COMPRESS_MODE_NONE, u32Align, &stVbCalConfig);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].blk_size = stVbCalConfig.u32VBSize;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align = u32Align;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) u32Align:%d\n", VpssGrp, VpssChn, u32Align);
	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 *pu32Align)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, pu32Align);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pu32Align = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align;
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E enCoef)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (enCoef >= VPSS_SCALE_COEF_MAX) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) undefined scale_coef type(%d)\n"
			, VpssGrp, VpssChn, enCoef);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enCoef = enCoef;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E *penCoef)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, penCoef);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*penCoef = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enCoef;
	return CVI_SUCCESS;
}

/* CVI_VPSS_SetChnYRatio: Modify the y ratio of chn output. Only work for yuv format.
 *
 * @param VpssGrp: The Vpss Grp to work.
 * @param VpssChn: The Vpss Chn to work.
 * @param YRatio: Output's Y will be sacled by this ratio.
 * @return: CVI_SUCCESS if OK.
 */
CVI_S32 vpss_set_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 YRatio)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!IS_FMT_YUV(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) isn't YUV format. Can't apply this setting.\n"
			, VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.stNormalize.bEnable) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Y-ratio adjustment can't work with normalize.\n"
			, VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].YRatio = YRatio;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 *pYRatio)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pYRatio = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].YRatio;
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation,
				const VPSS_LDC_ATTR_S *pstLDCAttr, CVI_U64 mesh_addr)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GDC_FMT(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	return _vpss_update_ldc_mesh(VpssGrp, VpssChn, pstLDCAttr, enRotation, mesh_addr);
}

CVI_S32 vpss_get_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstLDCAttr, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr, sizeof(*pstLDCAttr));

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bEnable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d\n",
			pstLDCAttr->bEnable, pstLDCAttr->stAttr.bAspect,
			pstLDCAttr->stAttr.s32XYRatio, pstLDCAttr->stAttr.s32CenterXOffset,
			pstLDCAttr->stAttr.s32CenterYOffset, pstLDCAttr->stAttr.s32DistortionRatio);

	return CVI_SUCCESS;
}

CVI_S32 vpss_attach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VB_POOL hVbPool)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].VbPool = hVbPool;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) attach vb pool(%d)\n",
		VpssGrp, VpssChn, hVbPool);
	return CVI_SUCCESS;
}

CVI_S32 vpss_detach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].VbPool = VB_INVALID_POOLID;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_chn_bufwrap_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap)
{
	int ret;
	uint32_t *ion_vaddr = NULL;
	uint64_t ion_paddr = 0;
	CVI_U32 u32WrapBufferSize, u32BufWrapDepth, val;
	char ion_name[64];

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Channel already enabled\n");
		return CVI_ERR_VPSS_NOT_SUPPORT;
	}

	if (pstVpssChnBufWrap->u32BufLine != 64 && pstVpssChnBufWrap->u32BufLine != 128) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Only support 64 or 128 lines, u32BufLine(%d)\n",
				pstVpssChnBufWrap->u32BufLine);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if (pstVpssChnBufWrap->u32WrapBufferSize < 2) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "u32WrapBufferSize(%d) too small\n",
				pstVpssChnBufWrap->u32WrapBufferSize);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	u32BufWrapDepth = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].u32BufWrapDepth;
	u32WrapBufferSize = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap.u32WrapBufferSize;

	if (pstVpssChnBufWrap->bEnable) {
		ion_paddr = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr;
		if (!ion_paddr) {
			sprintf(ion_name, "VpssGrp%dChn%dWrapBuf", VpssGrp, VpssChn);

			u32WrapBufferSize = pstVpssChnBufWrap->u32WrapBufferSize;
			if (u32WrapBufferSize <= 32) {
				// Legacy usage
				u32BufWrapDepth = u32WrapBufferSize;
				u32WrapBufferSize = vpss_get_wrap_buffer_size(
					vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width,
					vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Height,
					vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat,
					pstVpssChnBufWrap->u32BufLine,
					u32BufWrapDepth);
			} else {
				val = vpss_get_wrap_buffer_size(
					vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width,
					pstVpssChnBufWrap->u32BufLine,
					vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat,
					pstVpssChnBufWrap->u32BufLine,
					1);
				u32BufWrapDepth = u32WrapBufferSize / val;
				if (u32BufWrapDepth < 2 || u32BufWrapDepth > 32) {
					CVI_TRACE_VPSS(CVI_DBG_ERR, "u32BufWrapDepth(%d) invalid, only 2 ~ 32\n",
							u32BufWrapDepth);
					return CVI_FAILURE;
				}
			}

			ret = sys_ion_alloc(&ion_paddr, (void *)&ion_vaddr, ion_name, u32WrapBufferSize, true);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) WrapBuf Size(0x%x) Addr(0x%llx)\n",
					VpssGrp, VpssChn, u32WrapBufferSize, (unsigned long long)ion_paddr);
		}
		if (!ion_paddr) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "allocate wrap buffer failed\n");
			return CVI_FAILURE;
		}
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) u32BufWrapDepth=%d/%d, u32WrapBufferSize=0x%x\n",
			VpssGrp, VpssChn,
			u32BufWrapDepth, pstVpssChnBufWrap->u32WrapBufferSize,
			u32WrapBufferSize);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].bufWrapPhyAddr = ion_paddr;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].u32BufWrapDepth = u32BufWrapDepth;
	memcpy(&vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap, pstVpssChnBufWrap,
		sizeof(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap));
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap.u32WrapBufferSize = u32WrapBufferSize;
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_chn_bufwrap_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap)
{
	int ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstVpssChnBufWrap, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap,
		sizeof(*pstVpssChnBufWrap));

	return CVI_SUCCESS;
}

CVI_S32 vpss_trigger_snap_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 frame_cnt)
{
	CVI_S32 ret;
	MMF_BIND_DEST_S stBindDest;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	struct base_exe_m_cb exe_cb;
	struct venc_snap_frm_info sanp_info;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (sys_get_bindbysrc(&chn, &stBindDest) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "sys_get_bindbysrc fails\n");
		return CVI_ERR_VPSS_NOT_PERM;
	}
	if (stBindDest.astMmfChn[0].enModId != CVI_ID_VENC) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "next Mod(%d) is not vcodec\n",
				stBindDest.astMmfChn[0].enModId);
		return CVI_ERR_VPSS_NOT_PERM;
	}
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	sanp_info.vpss_grp = VpssGrp;
	sanp_info.vpss_chn = VpssChn;
	sanp_info.skip_frm_cnt = frame_cnt;

	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VCODEC_CB_SNAP_JPG_FRM;
	exe_cb.data   = &sanp_info;
	ret = base_exe_module_cb(&exe_cb);

	return ret;
}

static void vpss_start_handler(void)
{
	int ret;
	u8 u8VpssDev;
	struct sched_param tsk;
	char thread_name[32];

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	for (u8VpssDev = 0; u8VpssDev < VPSS_IP_NUM; u8VpssDev++) {
		handler_ctx[u8VpssDev].u8VpssDev = u8VpssDev;
		handler_ctx[u8VpssDev].img_idx = u8VpssDev;
		handler_ctx[u8VpssDev].enHdlState = HANDLER_STATE_STOP;
		handler_ctx[u8VpssDev].workingGrp = VPSS_MAX_GRP_NUM;
		handler_ctx[u8VpssDev].workingMask = 0;
		handler_ctx[u8VpssDev].reset_done = false;
		atomic_set(&handler_ctx[u8VpssDev].events, 0);
		//FIFO_INIT(&handler_ctx[u8VpssDev].rgnex_jobs.jobq, 16);
		spin_lock_init(&handler_ctx[u8VpssDev].lock);

		init_waitqueue_head(&handler_ctx[u8VpssDev].wait);
		init_waitqueue_head(&handler_ctx[u8VpssDev].vi_reset_wait);

		snprintf(thread_name, 31, "cvitask_vpss_%d", u8VpssDev);
		handler_ctx[u8VpssDev].thread = kthread_run(vpss_event_handler,
			&handler_ctx[u8VpssDev], thread_name);
		if (IS_ERR(handler_ctx[u8VpssDev].thread)) {
			pr_err("failed to create vpss kthread, u8VpssDev=%d\n", u8VpssDev);
		}

		ret = sched_setscheduler(handler_ctx[u8VpssDev].thread, SCHED_FIFO, &tsk);
		if (ret)
			pr_warn("vpss thread priority update failed: %d\n", ret);
	}
}

static void vpss_stop_handler(void)
{
	int ret;
	u8 u8VpssDev;

	for (u8VpssDev = 0; u8VpssDev < VPSS_IP_NUM; u8VpssDev++) {
		if (!handler_ctx[u8VpssDev].thread) {
			pr_err("vpss thread not initialized yet\n");
			continue;
		}

		ret = kthread_stop(handler_ctx[u8VpssDev].thread);
		if (ret)
			pr_err("fail to stop vpss thread, err=%d\n", ret);

		handler_ctx[u8VpssDev].thread = NULL;
	}
}

CVI_S32 vpss_set_vivpss_mode(const VI_VPSS_MODE_S *pstVIVPSSMode)
{
	if (&stVIVPSSMode != pstVIVPSSMode)
		memcpy(&stVIVPSSMode, pstVIVPSSMode, sizeof(stVIVPSSMode));

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_mode_ex(void *arg, const VPSS_MODE_S *pstVPSSMode)
{
	struct cvi_vip_dev *vdev;
	int rc, i;
	CVI_U8 dev_num = (pstVPSSMode->enMode == VPSS_MODE_SINGLE) ? 1 : VPSS_IP_NUM;
	CVI_S32 input;
	CVI_U8 dev_idx;

	if (!arg)
		return CVI_FAILURE;

	vdev = (struct cvi_vip_dev *)arg;

	rc = sc_set_src_to_imgv(&vdev->sc_vdev[0], pstVPSSMode->enMode == VPSS_MODE_SINGLE);
	if (pstVPSSMode->enMode == VPSS_MODE_SINGLE)
		vdev->img_vdev[0].is_online_from_isp = false;

	for (i = 0; i < dev_num; ++i) {
		dev_idx = (dev_num == 1) ? 1 : i;

		switch (pstVPSSMode->aenInput[i]) {
		default:
		case VPSS_INPUT_MEM:
			input = CVI_VIP_INPUT_MEM;
			handler_ctx[dev_idx].online_from_isp = CVI_FALSE;
			break;
		case VPSS_INPUT_ISP:
			// Support vi offline, vpss online now
			//input = vi_online ? CVI_VIP_INPUT_ISP : CVI_VIP_INPUT_ISP_POST;
			input = CVI_VIP_INPUT_ISP_POST;
			handler_ctx[dev_idx].online_from_isp = CVI_TRUE;
			break;
		}

		img_s_input(&vdev->img_vdev[dev_idx], input);
	}

	if (&stVPSSMode != pstVPSSMode)
		memcpy(&stVPSSMode, pstVPSSMode, sizeof(stVPSSMode));

	return rc;
}

CVI_S32 vpss_get_mode_ex(VPSS_MODE_S *pstVPSSMode)
{
	memcpy(pstVPSSMode, &stVPSSMode, sizeof(*pstVPSSMode));
	return CVI_SUCCESS;
}

CVI_S32 vpss_set_mode(void *arg, VPSS_MODE_E enVPSSMode)
{
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "enVPSSMode=%d\n", enVPSSMode);

	stVPSSMode.enMode = enVPSSMode;
	return vpss_set_mode_ex(arg, &stVPSSMode);
}

VPSS_MODE_E vpss_get_mode(void)
{
	return stVPSSMode.enMode;
}

CVI_VOID vpss_set_mlv_info(CVI_U8 snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "snr_num(%d) out of range\n", snr_num);
		return;
	}

	g_mlv_i[snr_num].mlv_i_level = p_m_lv_i->mlv_i_level;
	memcpy(g_mlv_i[snr_num].mlv_i_table, p_m_lv_i->mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

CVI_VOID vpss_get_mlv_info(CVI_U8 snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "snr_num(%d) out of range\n", snr_num);
		return;
	}
	p_m_lv_i->mlv_i_level = g_mlv_i[snr_num].mlv_i_level;
	memcpy(p_m_lv_i->mlv_i_table, g_mlv_i[snr_num].mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

CVI_VOID vpss_set_isp_bypassfrm(CVI_U8 snr_num, CVI_U8 bypass_frm)
{
	isp_bypass_frm[snr_num] = bypass_frm;
}

void vpss_mode_init(void *arg)
{
	CVI_U8 i, j;
	PROC_AMP_CTRL_S ctrl;

	if (!arg)
		return;

	vip_dev = (struct cvi_vip_dev *)arg;

	for (i = 0; i < VPSS_IP_NUM; ++i)
		stVIVPSSMode.aenMode[i] = VI_OFFLINE_VPSS_OFFLINE;
	stVPSSMode.enMode = VPSS_MODE_SINGLE;
	for (i = 0; i < VPSS_IP_NUM; ++i)
		stVPSSMode.aenInput[i] = VPSS_INPUT_MEM;
	vpss_set_mode_ex(arg, &stVPSSMode);

	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		vpssExtCtx[i].scene = 0xff;
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j) {
			vpss_get_proc_amp_ctrl(j, &ctrl);
			vpssExtCtx[i].proc_amp[j] = ctrl.default_value;
		}
	}
}

void vpss_init(void *arg)
{
	if (!arg)
		return;

	vip_dev = (struct cvi_vip_dev *)arg;

	// CVI_SYS_Init()
	vpss_mode_init(arg);

	vpss_start_handler();
}

void vpss_deinit(void)
{
	if (!vip_dev) {
		pr_err("vpss device not initialized yet\n");
		return;
	}
	vpss_stop_handler();
}

int vpss_set_grp_sbm(struct vpss_grp_sbm_cfg *cb_cfg)
{
	VPSS_GRP VpssGrp = cb_cfg->grp;
	u8 sb_mode = cb_cfg->sb_mode;
	u8 sb_size = cb_cfg->sb_size;
	u8 sb_nb = cb_cfg->sb_nb;
	CVI_U32 ion_size = cb_cfg->ion_size;
	struct vpss_grp_sbm_cfg *sbm_cfg;
	VPSS_GRP_ATTR_S *pstGrpAttr;
	CVI_S32 ret;
	uint32_t *ion_vaddr = NULL;
	uint64_t ion_paddr = 0;
	char ion_name[64];
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	struct vb_jobs_t *jobs;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (sb_mode > 2 || sb_size > 3)
		return CVI_FAILURE;

	sbm_cfg = &vpssExtCtx[VpssGrp].sbm_cfg;
	if (sb_mode == sbm_cfg->sb_mode)
		return CVI_SUCCESS;

	if (!sb_mode) {
		if (sbm_cfg->ion_paddr)
			sys_ion_free(sbm_cfg->ion_paddr);

		mutex_lock(&vpssCtx[VpssGrp]->lock);
		sbm_cfg->ion_paddr = 0;
		sbm_cfg->sb_mode = 0;
		mutex_unlock(&vpssCtx[VpssGrp]->lock);

		return CVI_SUCCESS;
	}

	jobs = base_get_jobs_by_chn(chn, CHN_TYPE_IN);
	if (!jobs) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "get in jobs failed\n");
		return CVI_FAILURE;
	}

	pstGrpAttr = &vpssCtx[VpssGrp]->stGrpAttr;

	sprintf(ion_name, "VpssGrp%dWrapBuf", VpssGrp);
	ret = sys_ion_alloc(&ion_paddr, (void *)&ion_vaddr, ion_name, ion_size, true);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) sb_mode=%d, sb_size=%d, sb_nb=%d\n",
			VpssGrp, sb_mode, sb_size, sb_nb);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) WrapBuf Size(0x%x) Addr(0x%llx)\n",
			VpssGrp, ion_size, (unsigned long long)ion_paddr);

	if (!ion_paddr) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "allocate wrap buffer failed\n");
		return CVI_FAILURE;
	}

	cb_cfg->ion_paddr = ion_paddr;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssExtCtx[VpssGrp].sbm_cfg.sb_mode = sb_mode;
	vpssExtCtx[VpssGrp].sbm_cfg.sb_size = sb_size;
	vpssExtCtx[VpssGrp].sbm_cfg.sb_nb = sb_nb;
	vpssExtCtx[VpssGrp].sbm_cfg.ion_paddr = ion_paddr;
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	up(&jobs->sem);
	vpss_post_job(VpssGrp);

	return CVI_SUCCESS;
}

int vpss_set_vc_sbm_flow(struct vpss_vc_sbm_flow_cfg *cfg)
{
	VPSS_GRP VpssGrp = (VPSS_GRP)cfg->vpss_grp;
	VPSS_CHN VpssChn = (VPSS_CHN)cfg->vpss_chn;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp};
	bool sb_vc_ready = cfg->ready ? true : false;
	CVI_S32 ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stBufWrap.bEnable) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) sbm not enabled\n",
				VpssGrp, VpssChn);
		return CVI_FAILURE;
	}

	chn.s32ChnId = get_dev_info_by_chn(chn, CHN_TYPE_OUT);

	// Let VC module modify sbm status
	vpss_sc_set_vc_sbm(vip_dev, chn, sb_vc_ready);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) vc sbm ready %d\n",
			VpssGrp, VpssChn, cfg->ready);

	return ret;
}

CVI_S32 vpss_set_rgn_lut_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_lut_cfg *cfg)
{
	CVI_S32 ret, dev_idx;
	MMF_CHN_S chn;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_CHN_VALID(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	chn.enModId = CVI_ID_VPSS;
	chn.s32DevId = VpssGrp;
	chn.s32ChnId = VpssChn;
	dev_idx = get_dev_info_by_chn(chn, CHN_TYPE_OUT);
	clk_enable(vip_dev->sc_vdev[dev_idx].clk);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	if (cfg->rgnex_en)
		sclr_gop_setup_256LUT(0, cfg->lut_layer, cfg->lut_length, cfg->lut_addr);
	else
		sclr_gop_setup_256LUT(dev_idx, cfg->lut_layer, cfg->lut_length, cfg->lut_addr);
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	clk_disable(vip_dev->sc_vdev[dev_idx].clk);

	return CVI_SUCCESS;
}

CVI_S32 vpss_overflow_check(VPSS_GRP VpssGrp, struct cvi_vpss_info *vpss_info, struct cvi_vc_info *vc_info)
{
	CVI_S32 ret;
	CVI_U8 dev_idx = 0;
	struct base_exe_m_cb exe_cb;

	ret = CHECK_VPSS_GRP_VALID(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VPSS_GRP_CREATED(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	dev_idx = vpssCtx[VpssGrp]->u8DevId;

	if (handler_ctx[dev_idx].online_from_isp &&
	    _is_vc_sbm_enabled(VpssGrp, vpssCtx[VpssGrp])) {
		exe_cb.callee = E_MODULE_VCODEC;
		exe_cb.caller = E_MODULE_VPSS;
		exe_cb.cmd_id = VCODEC_CB_OVERFLOW_CHECK;
		exe_cb.data = (void *)vc_info;
		base_exe_module_cb(&exe_cb);
	}

	if (handler_ctx[dev_idx].online_from_isp) {
		sclr_check_overflow_reg(vpss_info);
	}

	return CVI_SUCCESS;
}

CVI_S32 vpss_sbm_notify_venc(VPSS_GRP VpssGrp, CVI_U8 sc_idx)
{
	CVI_S32 ret;
	struct base_exe_m_cb exe_cb;
	struct venc_switch_chn cfg;
	CVI_U8 chn_id = sc_index_to_chn_id(sc_idx);

	cfg.vpss_grp = VpssGrp;
	cfg.vpss_chn = chn_id;

	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VCODEC_CB_SWITCH_CHN;
	exe_cb.data   = &cfg;
	ret = base_exe_module_cb(&exe_cb);

	return ret;
}

CVI_VOID set_fb_on_vpss(CVI_BOOL is_fb_on_vpss)
{
	fb_on_vpss = is_fb_on_vpss;
}
