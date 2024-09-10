#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <linux/cvi_vip.h>
#include <linux/cvi_base_ctx.h>

#include <base_cb.h>
#include <base_ctx.h>
#include <dwa_cb.h>
#include <vi_cb.h>

#include "vpss_common.h"
#include <vip_common.h>
#include "scaler.h"
#include "dsi_phy.h"
#include "vpss.h"
#include <rgn_cb.h>

#include "vpss_debug.h"
#include "vpss_core.h"
#include "cvi_vip_sc.h"
#include "cvi_vip_img.h"
#include "vo_cb.h"
#include "vpss_ioctl.h"
#include "vpss_rgn_ctrl.h"
#include "cvi_vip_vpss_proc.h"
#include "sclr_test.h"


//u32 vpss_log_lv = CVI_DBG_WARN;
u32 vpss_log_lv = 0;
int single_vb;
int debug;
int vip_clk_freq;
static bool vpss_vo_cb_status;

static atomic_t open_count = ATOMIC_INIT(0);

const struct cvi_vip_fmt cvi_vip_formats[] = {
	{
	.fourcc      = PIXEL_FORMAT_YUV_PLANAR_420,
	.fmt         = SCL_FMT_YUV420,
	.bit_depth   = { 8, 4, 4 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc      = PIXEL_FORMAT_YUV_PLANAR_422,
	.fmt         = SCL_FMT_YUV422,
	.bit_depth   = { 8, 4, 4 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_YUV_PLANAR_444,
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_NV12,
	.fmt         = SCL_FMT_NV12,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc      = PIXEL_FORMAT_NV21,
	.fmt         = SCL_FMT_NV21,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc      = PIXEL_FORMAT_NV16,
	.fmt         = SCL_FMT_YUV422SP1,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_NV61,
	.fmt         = SCL_FMT_YUV422SP2,
	.bit_depth   = { 8, 8, 0 },
	.buffers     = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_YUYV,
	.fmt         = SCL_FMT_YUYV,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_YVYU,
	.fmt         = SCL_FMT_YVYU,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_UYVY,
	.fmt         = SCL_FMT_UYVY,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_VYUY,
	.fmt         = SCL_FMT_VYUY,
	.bit_depth   = { 16 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_RGB_888_PLANAR, /* rgb */
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_BGR_888_PLANAR, /* rgb */
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_RGB_888, /* rgb */
	.fmt         = SCL_FMT_RGB_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_BGR_888, /* bgr */
	.fmt         = SCL_FMT_BGR_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_YUV_400, /* Y-Only */
	.fmt         = SCL_FMT_Y_ONLY,
	.bit_depth   = { 8 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_HSV_888, /* hsv */
	.fmt         = SCL_FMT_RGB_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = PIXEL_FORMAT_HSV_888_PLANAR, /* hsv */
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

module_param(vpss_log_lv, int, 0644);
/* debug: for tile mode debug
 * - bit[0]: if true, sc force tile mode
 * - bit[1]: if true, sc only do left tile.
 * - bit[2]: if true, disable ccf on sc_top/vip_sys1/sc/img/disp/ldc
 * - bit[3]: if true, print vb info
 */
module_param(debug, int, 0644);

/* switch for single vb in online
 * 1:only one vb for vpss online
 * 0:normal vpss online
 */
module_param(single_vb, int, 0644);

module_param(vip_clk_freq, int, 0644);

/*************************************************************************
 *	General functions
 *************************************************************************/
const struct cvi_vip_fmt *cvi_vip_get_format(u32 pixelformat)
{
	const struct cvi_vip_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(cvi_vip_formats); k++) {
		fmt = &cvi_vip_formats[k];
		if (fmt->fourcc == pixelformat) {
			//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "pixelformat=%d, cvi_vip_formats[%d], fmt=%d\n",
			//		pixelformat, k, fmt->fmt);
			return fmt;
		}
	}

	return NULL;
}

void cvi_vip_buf_queue(struct cvi_base_vdev *vdev, struct vpss_img_buffer *b)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue);
	++vdev->num_rdy;
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	vpss_img_sb_qbuf((struct cvi_img_vdev *)vdev, NULL, NULL);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "num_rdy=%d\n", vdev->num_rdy);
}

struct vpss_img_buffer *cvi_vip_next_buf(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct vpss_img_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue))
		b = list_first_entry(&vdev->rdy_queue,
			struct vpss_img_buffer, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

struct vpss_img_buffer *cvi_vip_buf_remove(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct vpss_img_buffer *b = NULL;

	if (vdev->num_rdy == 0)
		return b;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue)) {
		b = list_first_entry(&vdev->rdy_queue,
			struct vpss_img_buffer, list);
		list_del_init(&b->list);
		--vdev->num_rdy;
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

void cvi_vip_buf_cancel(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct vpss_img_buffer *b = NULL;

	if (vdev->num_rdy == 0)
		return;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	while (!list_empty(&vdev->rdy_queue)) {
		b = list_first_entry(&vdev->rdy_queue,
			struct vpss_img_buffer, list);
		list_del_init(&b->list);
		--vdev->num_rdy;
		kfree(b);
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

CVI_VOID vpss_img_sb_qbuf(struct cvi_img_vdev *idev, struct cvi_buffer *buf, struct vpss_grp_sbm_cfg *sbm_cfg)
{
	int i;

	if (buf && sbm_cfg) {
		idev->img_sb_cfg.sb_mode = sbm_cfg->sb_mode;
		idev->img_sb_cfg.sb_size = sbm_cfg->sb_size;
		idev->img_sb_cfg.sb_nb = sbm_cfg->sb_nb;

		for (i = 0; i < 3; i++)
			idev->sb_phy_addr[i] = buf->phy_addr[i];
	} else {
		memset(&idev->img_sb_cfg, 0, sizeof(idev->img_sb_cfg));
		memset(idev->sb_phy_addr, 0, sizeof(idev->sb_phy_addr));
	}
}

#if defined(CONFIG_TILE_MODE)
static void img_left_tile_cfg(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	u8 i;
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(idev->img_type);
	struct sclr_mem mem = cfg->mem;
	struct cvi_vip_dev *bdev = NULL;

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img-%d: tile on left.\n", idev->dev_idx);

#ifdef TILE_ON_IMG
	if (idev->is_online_from_isp)
		mem.width = idev->post_para.ol_tile_cfg.l_in.end -
			idev->post_para.ol_tile_cfg.l_in.start + 1;
	else
		mem.width = (mem.width >> 1) + TILE_GUARD_PIXEL;
	sclr_img_set_mem(idev->img_type, &mem, false);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img-%d start_x(%d) width(%d).\n", idev->dev_idx,
		mem.start_x, mem.width);
#endif
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (!sclr_left_tile(i, mem.width))
			atomic_set(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE);
	}
}

static void img_right_tile_cfg(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	u8 i;
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(idev->img_type);
	struct sclr_mem mem = cfg->mem;
	struct cvi_vip_dev *bdev = NULL;
#ifdef TILE_ON_IMG
	u32 sc_offset;
#endif

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img-%d: tile on right.\n", idev->dev_idx);

#ifdef TILE_ON_IMG
	if (idev->is_online_from_isp) {
		mem.width = idev->post_para.ol_tile_cfg.r_in.end -
			idev->post_para.ol_tile_cfg.r_in.start + 1;
		sc_offset = idev->post_para.ol_tile_cfg.r_in.start;
	} else {
		sc_offset = (mem.width >> 1) - TILE_GUARD_PIXEL;
		mem.start_x += sc_offset;
		mem.width = mem.width - sc_offset;
	}
	sclr_img_set_mem(idev->img_type, &mem, false);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img-%d start_x(%d) width(%d).\n", idev->dev_idx,
		mem.start_x, mem.width);
#endif
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (!sclr_right_tile(i, sc_offset))
			atomic_set(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE);
	}
}
#endif

static void cvi_img_device_run(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	struct cvi_vip_dev *bdev = NULL;
	u8 i;
	struct sclr_top_cfg *top_cfg = sclr_top_get_cfg();
	struct sclr_img_in_sb_cfg *img_sb_cfg = &idev->img_sb_cfg;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), is_online_from_isp(%d), job_grp(%d)\n",
		idev->dev_idx, idev->img_type, idev->input_type, idev->is_online_from_isp,
		idev->job_grp);

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	// only update hw if not-tile or at left-tile
	if (!(debug & BIT(2)) && idev->clk)
		clk_enable(idev->clk);

	cvi_img_update(idev, &idev->vpss_grp_cfg[idev->job_grp]);
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (sc_need_check[i]) {
			cvi_sc_device_run(&bdev->sc_vdev[i], idev->is_tile, idev->is_work_on_r_tile,
				idev->job_grp);
			top_cfg->sclr_enable[i] = true;
		} else {
			if (bdev->sc_vdev[i].img_src == idev->dev_idx)
				top_cfg->sclr_enable[i] = false;
		}
	}

	sclr_img_set_dwa_to_sclr_sb(idev->img_type, img_sb_cfg->sb_mode, img_sb_cfg->sb_size, img_sb_cfg->sb_nb);

	if (img_sb_cfg->sb_mode) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "update img-sbm-buf: 0x%llx-0x%llx-0x%llx\n",
				(unsigned long long)idev->sb_phy_addr[0],
				(unsigned long long)idev->sb_phy_addr[1],
				(unsigned long long)idev->sb_phy_addr[2]);

		sclr_img_set_addr(idev->img_type, idev->sb_phy_addr[0], idev->sb_phy_addr[1], idev->sb_phy_addr[2]);
	} else if (!idev->is_online_from_isp) {
		struct vpss_img_buffer *b = cvi_vip_next_buf((struct cvi_base_vdev *)idev);

		if (!b) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "img%d no buffer\n", idev->dev_idx);
			return;
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "update img-buf: 0x%llx-0x%llx-0x%llx\n",
				(unsigned long long)b->phy_addr[0], (unsigned long long)b->phy_addr[1],
				(unsigned long long)b->phy_addr[2]);

		sclr_img_set_addr(idev->img_type, b->phy_addr[0], b->phy_addr[1], b->phy_addr[2]);
	}

	//spin_lock_irqsave(&dev->job_lock, flags);
	idev->job_flags |= TRANS_RUNNING;
	//spin_unlock_irqrestore(&dev->job_lock, flags);

	sclr_img_checksum_en(idev->img_type, true);

	sclr_top_set_cfg(top_cfg);
	ktime_get_ts64(&idev->ts_start);

	if (!bdev->disp_online)
		sclr_img_start(idev->img_type);

	CVI_TRACE_VPSS(CVI_DBG_WARN, "****img(%d) grp(%d) start****\n", idev->dev_idx, idev->job_grp);
}

u8 _gop_get_bpp(enum sclr_gop_format fmt)
{
	return (fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		(fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;
}

int cvi_vip_set_rgn_cfg(const u8 inst, u8 layer, const struct cvi_rgn_cfg *rgn_cfg, const struct sclr_size *size)
{
	struct sclr_gop_cfg *gop_cfg = sclr_gop_get_cfg(inst, layer);
	struct sclr_gop_odec_cfg *odec_cfg = &gop_cfg->odec_cfg;
	struct sclr_gop_ow_cfg *ow_cfg;
	u8 bpp, ow_idx;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = rgn_cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = rgn_cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = rgn_cfg->colorkey_en;
	gop_cfg->colorkey = rgn_cfg->colorkey;

	if (rgn_cfg->odec.enable) { // odec enable
		ow_idx = rgn_cfg->odec.attached_ow;
		ow_cfg = &gop_cfg->ow_cfg[ow_idx];
		gop_cfg->gop_ctrl.raw |= BIT(ow_idx);

		if (rgn_cfg->param[ow_idx].fmt > CVI_RGN_FMT_MAX) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "RGN FMT Error\n");
		} else {
			ow_cfg->fmt = (enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt;
		}
		ow_cfg->addr = rgn_cfg->param[ow_idx].phy_addr;
		ow_cfg->pitch = rgn_cfg->param[ow_idx].stride;
		ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
		ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
		ow_cfg->img_size.w = rgn_cfg->param[ow_idx].rect.width;
		ow_cfg->img_size.h = rgn_cfg->param[ow_idx].rect.height;
		ow_cfg->end.x = ow_cfg->start.x + ow_cfg->img_size.w;
		ow_cfg->end.y = ow_cfg->start.y + ow_cfg->img_size.h;

		//while reg_odec_en=1, odec_stream_size = {reg_ow_dram_vsize,reg_ow_dram_hsize}
		ow_cfg->mem_size.w = ALIGN(rgn_cfg->odec.bso_sz, 16) & 0x3fff;
		ow_cfg->mem_size.h = ALIGN(rgn_cfg->odec.bso_sz, 16) >> 14;

		// set odec cfg
		odec_cfg->odec_ctrl.b.odec_en = true;
		odec_cfg->odec_ctrl.b.odec_int_en = true;
		odec_cfg->odec_ctrl.b.odec_int_clr = true;
		odec_cfg->odec_ctrl.b.odec_wdt_en = true;
		odec_cfg->odec_ctrl.b.odec_attached_idx = ow_idx;
#if 0
		CVI_TRACE_VPSS(CVI_DBG_INFO, "inst(%d) gop(%d) fmt(%d) rect(%d %d %d %d) addr(0x%llx) pitch(%d).\n",
				inst, layer, ow_cfg->fmt,
				ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h,
				ow_cfg->addr, ow_cfg->pitch);
		CVI_TRACE_VPSS(CVI_DBG_INFO, "odec:enable(%d) attached_ow(%d) size(%d).\n",
				odec_cfg->odec_ctrl.b.odec_en, odec_cfg->odec_ctrl.b.odec_attached_idx,
				rgn_cfg->odec.bso_sz);
#endif

		sclr_gop_ow_set_cfg(inst, layer, ow_idx, ow_cfg, true);
	} else { //normal rgn w/o odec enabled
		for (ow_idx = 0; ow_idx < rgn_cfg->num_of_rgn; ++ow_idx) {
			ow_cfg = &gop_cfg->ow_cfg[ow_idx];
			gop_cfg->gop_ctrl.raw |= BIT(ow_idx);

			if (rgn_cfg->param[ow_idx].fmt > CVI_RGN_FMT_MAX) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "RGN FMT Error\n");
			} else {
				bpp = _gop_get_bpp((enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt);
				ow_cfg->fmt = (enum sclr_gop_format)rgn_cfg->param[ow_idx].fmt;
			}
			ow_cfg->addr = rgn_cfg->param[ow_idx].phy_addr;
			ow_cfg->pitch = rgn_cfg->param[ow_idx].stride;
			if (rgn_cfg->param[ow_idx].rect.left < 0) {
				ow_cfg->start.x = 0;
				ow_cfg->addr -= bpp * rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w
					= rgn_cfg->param[ow_idx].rect.width + rgn_cfg->param[ow_idx].rect.left;
			} else if ((rgn_cfg->param[ow_idx].rect.left + rgn_cfg->param[ow_idx].rect.width) > size->w) {
				ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w = size->w - rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->mem_size.w = rgn_cfg->param[ow_idx].stride;
			} else {
				ow_cfg->start.x = rgn_cfg->param[ow_idx].rect.left;
				ow_cfg->img_size.w = rgn_cfg->param[ow_idx].rect.width;
				ow_cfg->mem_size.w = rgn_cfg->param[ow_idx].stride;
			}

			if (rgn_cfg->param[ow_idx].rect.top < 0) {
				ow_cfg->start.y = 0;
				ow_cfg->addr -= ow_cfg->pitch * rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h
					= rgn_cfg->param[ow_idx].rect.height + rgn_cfg->param[ow_idx].rect.top;
			} else if ((rgn_cfg->param[ow_idx].rect.top + rgn_cfg->param[ow_idx].rect.height) > size->h) {
				ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h = size->h - rgn_cfg->param[ow_idx].rect.top;
			} else {
				ow_cfg->start.y = rgn_cfg->param[ow_idx].rect.top;
				ow_cfg->img_size.h = rgn_cfg->param[ow_idx].rect.height;
			}

			ow_cfg->end.x = ow_cfg->start.x +
					(ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en) -
					gop_cfg->gop_ctrl.b.hscl_en;
			ow_cfg->end.y = ow_cfg->start.y +
					(ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en) -
					gop_cfg->gop_ctrl.b.vscl_en;
			ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
			ow_cfg->mem_size.h = ow_cfg->img_size.h;

#if 0
			CVI_TRACE_VPSS(CVI_DBG_INFO, "inst(%d) gop(%d) fmt(%d)\n", inst, layer, ow_cfg->fmt);
			CVI_TRACE_VPSS(CVI_DBG_INFO, "rect(%d %d %d %d) addr(0x%llx) pitch(%d).\n",
				ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h,
				ow_cfg->addr, ow_cfg->pitch);
#endif

			sclr_gop_ow_set_cfg(inst, layer, ow_idx, ow_cfg, true);
		}

		// set odec enable to false
		odec_cfg->odec_ctrl.b.odec_en = false;
	}
	sclr_gop_set_cfg(inst, layer, gop_cfg, true);

	return 0;
}

bool cvi_vip_online_check_sc_rdy(struct cvi_img_vdev *idev, u8 grp_id)
{
	struct cvi_vip_dev *bdev = NULL;
	unsigned long flags_out[4], flags_job;
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	bool sc_locked[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	u8 i, workq_num;
	bool sc_enable = false;
	bool sc_ready = false;
	struct cvi_img_vdev tmp_idev;

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	tmp_idev = *idev;
	tmp_idev.job_grp = grp_id;	// snr_num
	cvi_img_get_sc_bound(&tmp_idev, sc_need_check);

	//check stream on
	if (!atomic_read(&idev->is_streaming)) {
		return false;
	}
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (!atomic_read(&bdev->sc_vdev[i].is_streaming))
			return false;
	}

	spin_lock_irqsave(&idev->job_lock, flags_job);
	if (!cvi_vip_job_is_queued(idev)) {
		sc_ready = true;
		goto job_unlock;
	}

	// check sc's queue if bounding
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;

		sc_enable = true;
		spin_lock_irqsave(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
		sc_locked[i] = true;
		if (bdev->sc_vdev[i].sb_enabled[grp_id] && bdev->sc_vdev[i].sb_phy_addr[grp_id][0]) {
			if (bdev->sc_vdev[i].sb_vc_ready[grp_id]) {
				sc_locked[i] = false;
				spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
				continue;
			}
			CVI_TRACE_VPSS(CVI_DBG_WARN, "sc-%d vc sbm not ready.\n", i);
			vpss_notify_wkup_evt(idev->dev_idx);
			goto sc_unlock;
		}

		workq_num = (atomic_read(&bdev->sc_vdev[i].job_state) == CVI_VIP_RUNNING) ? 2 : 1;

		if (bdev->sc_vdev[i].num_rdy[grp_id] < workq_num) {
			//if (idev->is_online_from_isp) {
			//	atomic_cmpxchg(&bdev->sc_vdev[i].buf_empty[grp_id], 0, 1);
				//vpss_notify_vi_reqbuf_evt(idev->dev_idx);
			//}
			CVI_TRACE_VPSS(CVI_DBG_WARN, "No sc-%d buffer available. workq_num:%d\n", i, workq_num);
			goto sc_unlock;
		}
		sc_locked[i] = false;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
	}
	if (!sc_enable) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "no sc_enable\n");
		goto job_unlock;
	}

	if (idev->is_online_from_isp) {
		idev->isp_triggered = true;
		idev->next_job_grp = grp_id;
	}
	spin_unlock_irqrestore(&idev->job_lock, flags_job);
	return sc_ready;

sc_unlock:
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_locked[i])
			continue;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
	}
job_unlock:
	spin_unlock_irqrestore(&idev->job_lock, flags_job);

	return sc_ready;
}

/**
 * cvi_vip_try_schedule - check if img is ready for a job
 *
 * @param idev: img-dev who to run.
 * @param grp_id: vpss grp settings to run.
 *                0 for offline or online-snr0; 1 for online-srn1.
 * @return: 0 if ready
 */
int cvi_vip_try_schedule(struct cvi_img_vdev *idev, u8 grp_id)
{
	struct cvi_vip_dev *bdev = NULL;
	unsigned long flags_out[CVI_VIP_SC_MAX], flags_img, flags_job;
	u8 i;
	bool check_img_buffer = (idev->input_type == CVI_VIP_INPUT_MEM);
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	bool sc_locked[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	bool sc_enable = false;

	//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), grp_id=%d, check_img_buffer=%d\n",
	//		idev->dev_idx, idev->img_type, idev->input_type, grp_id, check_img_buffer);

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	spin_lock_irqsave(&idev->job_lock, flags_job);
	if (cvi_vip_job_is_queued(idev)) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Caller is %pS, On job queue already\n",
			__builtin_return_address(0));
		goto job_unlock;
	}

	spin_lock_irqsave(&idev->rdy_lock, flags_img);

	// check if instances is on
	if (!atomic_read(&idev->is_streaming)) {
		//CVI_TRACE_VPSS(CVI_DBG_WARN, "Caller is %pS, img-%d needs to be on.\n",
		//	__builtin_return_address(0), idev->dev_idx);
		goto img_unlock;
	}

	// if img_in online, then buffer no needed
	if (check_img_buffer) {
		if (!idev->img_sb_cfg.sb_mode && list_empty(&idev->rdy_queue)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Caller is %pS, No input buffers available.\n",
				__builtin_return_address(0));
			goto img_unlock;
		}
	}
	idev->job_grp = grp_id;	// snr_num
	cvi_img_get_sc_bound(idev, sc_need_check);

	// check sc's queue if bounding
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;

		sc_enable = true;
		spin_lock_irqsave(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
		sc_locked[i] = true;
		if (!atomic_read(&bdev->sc_vdev[i].is_streaming)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "sc-%d needs to be on.\n", i);
			goto sc_unlock;
		}
		if (atomic_read(&bdev->sc_vdev[i].job_state) == CVI_VIP_RUNNING) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "sc-%d busy.\n", i);
			goto sc_unlock;
		}
		if (bdev->sc_vdev[i].sb_enabled[grp_id] && bdev->sc_vdev[i].sb_phy_addr[grp_id][0]) {
			if (bdev->sc_vdev[i].sb_vc_ready[grp_id])
				continue;

			CVI_TRACE_VPSS(CVI_DBG_WARN, "sc-%d vc sbm not ready.\n", i);
			vpss_notify_wkup_evt(idev->dev_idx);
			goto sc_unlock;
		}
		if (bdev->sc_vdev[i].num_rdy[grp_id] == 0) {
			if (idev->is_online_from_isp) {
				atomic_cmpxchg(&bdev->sc_vdev[i].buf_empty[grp_id], 0, 1);
				if (debug & BIT(3))
					vpss_print_vb_info(grp_id, i);
			}
			CVI_TRACE_VPSS(CVI_DBG_WARN, "No sc-%d buffer available.\n", i);
			goto sc_unlock;
		}
	}
	if (!sc_enable) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "no sc_enable\n");
		goto img_unlock;
	}

	// job status update
	idev->is_tile = false;

	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_locked[i])
			continue;

		if (atomic_cmpxchg(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE, CVI_VIP_RUNNING)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, " sc(%d) still busy. Reject\n", i);
			goto sc_unlock;
		}

		bdev->sc_vdev[i].job_grp = grp_id;	// snr_num
		sc_locked[i] = false;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
	}

	idev->job_flags |= TRANS_QUEUED;
	idev->IntMask = 0;
	idev->isp_triggered = false;
	spin_unlock_irqrestore(&idev->rdy_lock, flags_img);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "cvi_img_device_run\n");

	//notify venc
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (bdev->sc_vdev[i].sb_enabled[grp_id])
			vpss_sbm_notify_venc(grp_id, i);
	}

	// hw operations
	cvi_img_device_run(idev, sc_need_check);

	spin_unlock_irqrestore(&idev->job_lock, flags_job);

	return 0;

sc_unlock:
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_locked[i])
			continue;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
	}
img_unlock:
	spin_unlock_irqrestore(&idev->rdy_lock, flags_img);
job_unlock:
	spin_unlock_irqrestore(&idev->job_lock, flags_job);

	return -1;
}

/**
 * cvi_vip_job_finish - bottom-half of img/sc-isr to check
 *                      if current job is done and ready for next one.
 *
 * @param idev: img-dev who is running.
 */
void cvi_vip_job_finish(struct cvi_img_vdev *idev)
{
	struct cvi_vip_dev *bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	u8 i;

	// check if all dev idle
	if (idev->job_flags & TRANS_RUNNING)
		return;

	cvi_img_get_sc_bound(idev, sc_need_check);

	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;

		if (atomic_read(&bdev->sc_vdev[i].job_state) != CVI_VIP_IDLE)
			return;
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "img(%d:%d:%d) job_grp(%d) %s done\n",
		idev->dev_idx, idev->img_type, idev->input_type, idev->job_grp,
		idev->is_tile ? (idev->is_work_on_r_tile ? "right tile" : "left tile") : "");

	if (!idev->is_tile || !idev->tile_mode) {
		struct sclr_top_cfg *cfg = sclr_top_get_cfg();
		struct cvi_sc_vdev *sdev;

		idev->is_tile = false;
		idev->is_work_on_r_tile = true;
		idev->tile_mode = 0;

		// disable ip & clk
		for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
			if (!sc_need_check[i])
				continue;
			sdev = &bdev->sc_vdev[i];
			cfg->sclr_enable[sdev->dev_idx] = false;
		}
		sclr_top_set_cfg(cfg);

		for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
			if (!sc_need_check[i])
				continue;
			sdev = &bdev->sc_vdev[i];
			if (!(debug & BIT(2)) && sdev->clk && __clk_is_enabled(sdev->clk))
				clk_disable(sdev->clk);
		}
		if (!(debug & BIT(2)) && idev->clk && __clk_is_enabled(idev->clk))
			clk_disable(idev->clk);

		CVI_TRACE_VPSS(CVI_DBG_WARN, "****img(%d) grp(%d) finish****\n", idev->dev_idx, idev->job_grp);
		vpss_notify_isr_evt(idev->dev_idx);
	}

	idev->job_flags &= ~(TRANS_QUEUED);

	if (idev->is_online_from_isp && idev->isp_triggered) {
		tasklet_hi_schedule(&idev->job_work);
	}
}

bool cvi_vip_job_is_queued(struct cvi_img_vdev *idev)
{
	return (idev->job_flags & TRANS_QUEUED);
}

u32 cvi_sc_cfg_cb(struct sc_cfg_cb *post_para, struct cvi_vip_dev *dev)
{
	u32 ret = -1;
	int i;
	u8 grp_id = post_para->snr_num;

	if (!dev)
		return -1;

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i)
		if (dev->img_vdev[i].is_online_from_isp) {
			if (grp_id >= VPSS_ONLINE_NUM) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "post_para->snr_num error\n");
				break;
			}
			CVI_TRACE_VPSS(CVI_DBG_WARN, "online trig for snr-%d. img(%d:%d:%d)\n",
				grp_id, i, dev->img_vdev[i].img_type, dev->img_vdev[i].input_type);
			//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "post_para: is_tile:%d is_left_tile:%d\n",
			//		post_para->is_tile,post_para->is_left_tile);
			//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "left: in(%d %d) out(%d %d)\n",
			//	post_para->ol_tile_cfg.l_in.start, post_para->ol_tile_cfg.l_in.end,
			//	post_para->ol_tile_cfg.l_out.start,
			//	post_para->ol_tile_cfg.l_out.end);
			//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "right: in(%d %d) out(%d %d)\n",
			//	post_para->ol_tile_cfg.r_in.start,
			//	post_para->ol_tile_cfg.r_in.end, post_para->ol_tile_cfg.r_out.start,
			//	post_para->ol_tile_cfg.r_out.end);

			dev->img_vdev[i].isp_trig_cnt[grp_id]++;
			dev->img_vdev[i].post_para = *post_para;

			if (cvi_vip_online_check_sc_rdy(&dev->img_vdev[i], grp_id) == true)
				ret = cvi_vip_try_schedule(&dev->img_vdev[i], grp_id);
			else {
				if (dev->img_vdev[i].isp_triggered)
					ret = 0;
			}
			if (ret)
				dev->img_vdev[i].isp_trig_fail_cnt[grp_id]++;

			break;
		}

	return ret;
}

void cvi_sc_vi_err_cb(struct sc_err_handle_cb *err_cb_para, struct cvi_vip_dev *dev)
{
	int i;
	u8 grp_id = err_cb_para->snr_num;
	struct cvi_img_vdev *idev;

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		idev = &dev->img_vdev[i];
		if (idev->is_online_from_isp) {
			if (grp_id >= VPSS_ONLINE_NUM) {
				CVI_TRACE_VPSS(CVI_DBG_DEBUG, "post_para->snr_num error\n");
				break;
			}
			idev->job_grp = grp_id;
			vpss_notify_vi_err_evt(i);
		}
	}
}

void cvi_sc_trigger_post(void *arg)
{
	struct cvi_vip_dev *dev = (struct cvi_vip_dev *)arg;
	struct base_exe_m_cb exe_cb;

	if (!dev)
		return;

	/* Notify vi to send buffer as soon as possible */
	exe_cb.callee = E_MODULE_VI;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VI_CB_QBUF_TRIGGER;
	exe_cb.data   = NULL;
	base_exe_module_cb(&exe_cb);
}

void cvi_sc_frm_done_cb(struct cvi_vip_dev *dev)
{
	if (!dev)
		return;
}

int vpss_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_vip_dev *vdev = (struct cvi_vip_dev *)dev;
	int rc = -1;

	switch (cmd) {
	case VPSS_CB_VI_ONLINE_TRIGGER:
	{
		struct sc_cfg_cb *post_para = (struct sc_cfg_cb *)arg;

		vpss_set_mlv_info(post_para->snr_num, &post_para->m_lv_i);
		if (single_vb == 1) {
			post_para->bypass_num = 0;
		}
		vpss_set_isp_bypassfrm(post_para->snr_num, post_para->bypass_num);
		rc = cvi_sc_cfg_cb(post_para, vdev);
		break;
	}

	case VPSS_CB_QBUF_TRIGGER:
	{
		CVI_S32 dev_id;

		dev_id = *((CVI_S32 *)arg);
		vpss_post_job(dev_id);
		rc = 0;
		break;
	}

	case VPSS_CB_SET_VIVPSSMODE:
	{
		const VI_VPSS_MODE_S *pstVIVPSSMode = (const VI_VPSS_MODE_S *)arg;

		CVI_TRACE_VPSS(CVI_DBG_INFO, "VPSS_CB_SET_VIVPSSMODE\n");
		rc = vpss_set_vivpss_mode(pstVIVPSSMode);
		break;
	}

	case VPSS_CB_SET_VPSSMODE:
	{
		CVI_U32 enVPSSMode = *((CVI_U32 *)arg);

		CVI_TRACE_VPSS(CVI_DBG_INFO, "VPSS_CB_SET_VPSSMODE\n");
		rc = vpss_set_mode(vdev, (VPSS_MODE_E)enVPSSMode);
		break;
	}

	case VPSS_CB_SET_VPSSMODE_EX:
	{
		const VPSS_MODE_S *pstVPSSMode = (const VPSS_MODE_S *)arg;

		CVI_TRACE_VPSS(CVI_DBG_INFO, "VPSS_CB_SET_VPSSMODE_EX\n");
		rc = vpss_set_mode_ex(vdev, pstVPSSMode);
		break;
	}

	case VPSS_CB_ONLINE_ERR_HANDLE:
	{
		struct sc_err_handle_cb *err_cb_para = (struct sc_err_handle_cb *)arg;

		cvi_sc_vi_err_cb(err_cb_para, vdev);
		rc = 0;
		break;
	}

	case VPSS_CB_SET_DWA_2_VPSS_SBM:
	{
		struct vpss_grp_sbm_cfg *cfg = (struct vpss_grp_sbm_cfg *)arg;

		rc = vpss_set_grp_sbm(cfg);
		break;
	}

	case VPSS_CB_VC_SET_SBM_FLOW:
	{
		struct vpss_vc_sbm_flow_cfg *cfg = (struct vpss_vc_sbm_flow_cfg *)arg;
		rc = vpss_set_vc_sbm_flow(cfg);
		break;
	}

	case VPSS_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;
		CVI_U32 layer = attr->layer;

		rc = vpss_get_rgn_hdls(VpssGrp, VpssChn, layer, enType, pstHandle);
		break;
	}

	case VPSS_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		RGN_HANDLE *pstHandle = attr->hdls;
		RGN_TYPE_E enType = attr->enType;
		CVI_U32 layer = attr->layer;

		rc = vpss_set_rgn_hdls(VpssGrp, VpssChn, layer, enType, pstHandle);
		break;
	}

	case VPSS_CB_SET_RGN_CFG:
	{
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct cvi_rgn_cfg *pstRgnCfg = &attr->rgn_cfg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;
		CVI_U32 layer = attr->layer;

		rc = vpss_set_rgn_cfg(VpssGrp, VpssChn, layer, pstRgnCfg);
		break;
	}

	case VPSS_CB_SET_RGNEX_CFG:
	{
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Not support rgn_ex\n");
		rc = -1;
		break;
	}

	case VPSS_CB_SET_COVEREX_CFG:
	{
		struct _rgn_coverex_cfg_cb_param *attr = (struct _rgn_coverex_cfg_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_coverex_cfg(VpssGrp, VpssChn, &attr->rgn_coverex_cfg);
		break;
	}

	case VPSS_CB_SET_MOSAIC_CFG:
	{
		struct _rgn_mosaic_cfg_cb_param *attr = (struct _rgn_mosaic_cfg_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_mosaic_cfg(VpssGrp, VpssChn, &attr->rgn_mosaic_cfg);
		break;
	}

	case VPSS_CB_SET_RGN_LUT_CFG:
	{
		struct _rgn_lut_cb_param *attr = (struct _rgn_lut_cb_param *)arg;
		struct cvi_rgn_lut_cfg *pstRgnLutCfg = &attr->lut_cfg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_set_rgn_lut_cfg(VpssGrp, VpssChn, pstRgnLutCfg);
		break;
	}

	case VPSS_CB_GET_RGN_OW_ADDR:
	{
		struct _rgn_get_ow_addr_cb_param *attr = (struct _rgn_get_ow_addr_cb_param *)arg;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_get_rgn_ow_addr(VpssGrp, VpssChn, attr->layer, attr->handle, &attr->addr);
		break;
	}

	case VPSS_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *attr = (struct _rgn_chn_size_cb_param *)arg;
		VPSS_CHN_ATTR_S stChnAttr;
		VPSS_GRP VpssGrp = attr->stChn.s32DevId;
		VPSS_CHN VpssChn = attr->stChn.s32ChnId;

		rc = vpss_get_chn_attr(VpssGrp, VpssChn, &stChnAttr);
		attr->rect.u32Width = stChnAttr.u32Width;
		attr->rect.u32Height = stChnAttr.u32Height;
		break;
	}

	case VPSS_CB_GET_MLV_INFO:
	{
		struct vpss_grp_mlv_info *mlv_info = (struct vpss_grp_mlv_info *)arg;
		VPSS_GRP VpssGrp = mlv_info->vpss_grp;

		vpss_get_mlv_info(VpssGrp, &mlv_info->m_lv_i);
		break;
	}

	case DWA_CB_GDC_OP_DONE:
	{
		struct dwa_op_done_cfg *cfg =
			(struct dwa_op_done_cfg *)arg;

		vpss_gdc_callback(cfg->pParam, cfg->blk);
		rc = 0;
		break;
	}

	case VPSS_CB_OVERFLOW_CHECK:
	{
		struct cvi_overflow_info *info = (struct cvi_overflow_info *)arg;
		VPSS_GRP VpssGrp = info->vpss_info.dev_num;

		rc = vpss_overflow_check(VpssGrp, &info->vpss_info, &info->vc_info);
		break;
	}

	case VPSS_CB_VO_STATUS_SET:
	{
		bool status = *(bool *)arg;

		vpss_vo_cb_status = status;
		rc = 0;
		break;
	}

	case VPSS_CB_SET_FB_ON_VPSS:
	{
		set_fb_on_vpss(*(bool *)arg);
		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

static int vpss_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VPSS);
}

static int vpss_core_register_cb(struct cvi_vip_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VPSS;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vpss_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static irqreturn_t scl_isr(int irq, void *_dev)
{
	union sclr_intr intr_status = sclr_intr_status();
	u8 cmdq_intr_status = 0;

#if defined(CONFIG_SCLR_TEST)
	if (unlikely(sclr_test_enabled)) {
		sclr_test_irq_handler(intr_status.raw);
		return IRQ_HANDLED;
	}
#endif

#if defined(CONFIG_RGN_EX)
	cmdq_intr_status = sclr_cmdq_intr_status();

	if (cmdq_intr_status) {
		sclr_cmdq_intr_clr(cmdq_intr_status);
		CVI_TRACE_VPSS(CVI_DBG_INFO, "cmdq_intr_status(0x%x)\n", cmdq_intr_status);
	}
#endif

	sclr_intr_clr(intr_status);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "status(0x%x)\n", intr_status.raw);
#if defined( __SOC_MARS__)
	if (vpss_vo_cb_status)
		_vpss_call_cb(E_MODULE_VO, VO_CB_IRQ_HANDLER, &intr_status);
#endif
	img_irq_handler(intr_status, cmdq_intr_status, _dev);
	sc_irq_handler(intr_status, _dev);

	return IRQ_HANDLED;
}

#if 0
static int _get_reserved_mem(struct platform_device *pdev,
	uint64_t *addr, uint64_t *size)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;

	if (!pdev) {
		dev_err(&pdev->dev, "[VIP] null pointer\n");
		return -EINVAL;
	}

	target = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!target) {
		dev_err(&pdev->dev, "[VIP] No %s specified\n", "memory-region");
		return -EINVAL;
	}

	prmem = of_reserved_mem_lookup(target);
	of_node_put(target);

	if (!prmem) {
		dev_err(&pdev->dev, "[VIP]: cannot acquire memory-region\n");
		return -EINVAL;
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "pool name = %s, size = 0x%llx, base = 0x%llx\n",
		prmem->name, prmem->size, prmem->base);

	*addr = prmem->base;
	*size = prmem->size;

	return 0;
}
#endif

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	int irq_num[1];
	static const char * const irq_name[] = {"sc"};
	static const char * const clk_sys_name[] = {"clk_sys_0", "clk_sys_1", "clk_sys_2"};
	static const char * const clk_sc_top_name = "clk_sc_top";
	struct resource *res = NULL;
	void *reg_base[2];
	struct cvi_vip_dev *dev;
	int i;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(reg_base); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to get res[%d]\n", i);
			return -EINVAL;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res->start,
						    res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start,
						    res->end - res->start);
#endif
		CVI_TRACE_VPSS(CVI_DBG_INFO, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
			i, res->start, res->end, reg_base[i]);
		if (!reg_base[i]) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to get reg_base[%d]\n", i);
			return -EINVAL;
		}
	}
	sclr_set_base_addr(reg_base[0]);
	dphy_set_base_addr(reg_base[1]);

	/* Interrupt */
	for (i = 0; i < ARRAY_SIZE(irq_name); ++i) {
		irq_num[i] = platform_get_irq_byname(pdev, irq_name[i]);
		if (irq_num[i] < 0) {
			dev_err(&pdev->dev, "No IRQ resource for [%d]%s\n",
				i, irq_name[i]);
			return -ENODEV;
		}
		CVI_TRACE_VPSS(CVI_DBG_INFO, "irq(%d) for %s get from platform driver.\n",
			irq_num[i], irq_name[i]);
	}
	dev->irq_num_scl = irq_num[0];

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		dev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(dev->clk_sys[i])) {
			pr_err("Cannot get clk for clk_sys_%d\n", i);
			dev->clk_sys[i] = NULL;
		}
	}
	dev->clk_sc_top = devm_clk_get(&pdev->dev, clk_sc_top_name);
	if (IS_ERR(dev->clk_sc_top)) {
		pr_err("Cannot get clk for clk_sc_top\n");
		dev->clk_sc_top = NULL;
	}

	if (device_property_read_u32(&pdev->dev, "clock-freq-vip-sys1", &dev->clk_sys1_freq) != 0)
		dev->clk_sys1_freq = 300000000UL;
#endif

	return rc;
}

static int vpss_open(struct inode *inode, struct file *filep)
{
	struct cvi_vip_dev *dev =
		container_of(filep->private_data, struct cvi_vip_dev, miscdev);
	struct cvi_img_vdev *idev;
	struct cvi_sc_vdev *sdev;
	int i, j;

	pr_info("vpss_open\n");

	if (!dev) {
		pr_err("Cannot find vpss private data\n");
		return -ENODEV;
	}

	i = atomic_inc_return(&open_count);
	if (i > 1) {
		pr_info("vpss_open: open %d times\n", i);
		return 0;
	}

	if (dev->clk_sys[1])
		clk_prepare_enable(dev->clk_sys[1]);
	if (dev->clk_sc_top)
		clk_prepare_enable(dev->clk_sc_top);

	for (i = 0; i < ARRAY_SIZE(dev->img_vdev); i++) {
		idev = &dev->img_vdev[i];
		if ((debug & BIT(2)) && idev->clk)
			clk_prepare_enable(idev->clk);

		sclr_img_reg_shadow_sel(idev->img_type, false);

		for (j = 0; j < VPSS_ONLINE_NUM; j++) {
			idev->irq_cnt[j] = 0;
			idev->isp_trig_cnt[j] = 0;
			idev->isp_trig_fail_cnt[j] = 0;
			idev->frame_number[j] = 0;
		}
		idev->user_trig_cnt = 0;
		idev->user_trig_fail_cnt = 0;
	}

	for (i = 0; i < ARRAY_SIZE(dev->sc_vdev); i++) {
		sdev = &dev->sc_vdev[i];

		// temporarily enable clk for init.
		if (sdev->clk)
			clk_prepare_enable(sdev->clk);

		sclr_reg_shadow_sel(sdev->dev_idx, false);
		sclr_init(sdev->dev_idx);
		sclr_set_cfg(sdev->dev_idx, false, false, true, false);
		sclr_reg_force_up(sdev->dev_idx);
		if (!(debug & BIT(2)) && sdev->clk)
			clk_disable(sdev->clk);

		for (j = 0; j < VPSS_ONLINE_NUM; j++)
			atomic_set(&sdev->buf_empty[j], 0);
	}

	return 0;
}

static int vpss_release(struct inode *inode, struct file *filep)
{
	struct cvi_vip_dev *dev =
		container_of(filep->private_data, struct cvi_vip_dev, miscdev);
	int i;

	pr_info("vpss_release\n");

	if (!dev) {
		pr_err("Cannot find vpss private data\n");
		return -ENODEV;
	}

	i = atomic_dec_return(&open_count);
	if (i) {
		pr_info("vpss_close: open %d times\n", i);
		return 0;
	}

	return 0;
}

static const struct file_operations vpss_fops = {
	.owner = THIS_MODULE,
	.open = vpss_open,
	.release = vpss_release,
	.unlocked_ioctl = vpss_ioctl,
};

static int register_vpss_dev(struct device *dev, struct cvi_vip_dev *bdev)
{
	int rc;

	bdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	bdev->miscdev.name =
		devm_kasprintf(dev, GFP_KERNEL, "cvi-vpss");
	bdev->miscdev.fops = &vpss_fops;

	rc = misc_register(&bdev->miscdev);
	if (rc)
		pr_err("vpss_dev: failed to register misc device.\n");

	return rc;
}

static int cvi_vpss_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *dev;

	/* allocate main structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to allocate resource\n");
		return -ENOMEM;
	}

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);

	// get hw-resources
	rc = _init_resources(pdev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to init resource\n");
		goto err_irq;
	}

	rc = register_vpss_dev(&pdev->dev, dev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to register dev\n");
		goto err_dev;
	}

	rc = vpss_proc_init(dev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to init proc\n");
		goto err_proc;
	}

	// for img(2) - cap
	rc = img_create_instance(pdev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to create img instance\n");
		goto err_img_reg;
	}

	// for sc(4) - out
	rc = sc_create_instance(pdev);
	if (rc) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Failed to create sc instance\n");
		goto err_sc_reg;
	}

	if (vip_clk_freq)
		dev->clk_sys1_freq = vip_clk_freq;
	//if (dev->clk_sys[1])
	//	clk_set_rate(dev->clk_sys[1], dev->clk_sys1_freq);

	sclr_ctrl_init(false);
#if 0
	if (smooth) {
		sclr_disp_cfg_setup_from_reg();
	} else {
		if (!(debug & BIT(2)) && dev->clk_sc_top && __clk_is_enabled(dev->clk_sc_top))
			clk_disable_unprepare(dev->clk_sc_top);
	}
#endif

	//VIP_CLK_RATIO_CONFIG(LDC, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_V, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V1, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V2, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V3, 0x10);

	dev->disp_online = false;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "hw init done\n");

	if (devm_request_irq(&pdev->dev, dev->irq_num_scl, scl_isr, IRQF_SHARED,
		 "CVI_VIP_SCL", dev)) {
		dev_err(&pdev->dev, "Unable to request scl IRQ(%d)\n",
				dev->irq_num_scl);
		return -EINVAL;
	}

#if defined(CONFIG_SCLR_TEST)
	/* scaler self test */
	sclr_test_proc_init(dev);
#endif

	/* scaler register cb */
	if (vpss_core_register_cb(dev)) {
		dev_err(&pdev->dev, "Failed to register vpss cb, err %d\n", rc);
		return -EINVAL;
	}

	vpss_init(dev);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "done with rc(%d).\n", rc);
	return rc;

err_sc_reg:
err_img_reg:
	vpss_proc_remove(dev);
err_proc:
	misc_deregister(&dev->miscdev);
err_dev:
err_irq:
	dev_set_drvdata(&pdev->dev, NULL);

	CVI_TRACE_VPSS(CVI_DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * bmd_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int cvi_vpss_remove(struct platform_device *pdev)
{
	struct cvi_vip_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	/* scaler rm cb */
	if (vpss_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm vpss cb\n");
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vpss drvdata");
		return -EINVAL;
	}

#if defined(CONFIG_SCLR_TEST)
	/* scaler self test */
	sclr_test_proc_deinit();
#endif
	img_destroy_instance(pdev);
	sc_destroy_instance(pdev);
	vpss_proc_remove(dev);

	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int vpss_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int vpss_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	sclr_ctrl_init(true);
	//VIP_CLK_RATIO_CONFIG(ISP_TOP, 0x10);
	//VIP_CLK_RATIO_CONFIG(LDC, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(IMG_V, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_D, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V1, 0x10);
	//VIP_CLK_RATIO_CONFIG(SC_V2, 0x10);

	vip_sys_set_offline(VIP_SYS_AXI_BUS_SC_TOP, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_RAW, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_YUV, true);

	return 0;
}
#endif

static const struct of_device_id cvi_vpss_dt_match[] = {
	{.compatible = "cvitek,vpss"},
	{}
};

static SIMPLE_DEV_PM_OPS(vpss_pm_ops, vpss_suspend,
				vpss_resume);

#if (!DEVICE_FROM_DTS)
static void cvi_vpss_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_vpss_pdev = {
	.name		= "vpss",
	.dev.release	= cvi_vpss_pdev_release,
};
#endif

static struct platform_driver cvi_vpss_pdrv = {
	.probe      = cvi_vpss_probe,
	.remove     = cvi_vpss_remove,
	.driver     = {
		.name		= "vpss",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cvi_vpss_dt_match,
#endif
		.pm		= &vpss_pm_ops,
	},
};

static int __init cvi_vpss_init(void)
{
	int rc;

	CVI_TRACE_VPSS(CVI_DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_vpss_pdrv);
	#else
	rc = platform_device_register(&cvi_vpss_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_vpss_pdrv);
	if (rc)
		platform_device_unregister(&cvi_vpss_pdev);
	#endif

	return rc;
}

static void __exit cvi_vpss_exit(void)
{
	CVI_TRACE_VPSS(CVI_DBG_INFO, " +\n");
	vpss_deinit();
	platform_driver_unregister(&cvi_vpss_pdrv);
	#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_vpss_pdev);
	#endif
}

MODULE_DESCRIPTION("Cvitek Video Driver");
MODULE_AUTHOR("Jammy Huang");
MODULE_LICENSE("GPL");
module_init(cvi_vpss_init);
module_exit(cvi_vpss_exit);
