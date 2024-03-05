#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/poll.h>

#include "vpss_debug.h"
#include "vpss_common.h"
#include "scaler.h"

#include "vpss_core.h"
#include "cvi_vip_img.h"
#include "cvi_vip_sc.h"
#include "vpss.h"


static const char *const CLK_SC_NAME[] = {"clk_sc_d", "clk_sc_v1", "clk_sc_v2", "clk_sc_v3"};

int cvi_sc_buf_queue(struct cvi_sc_vdev *vdev, struct vpss_sc_buffer *list_buf)
{
	unsigned long flags;
	struct vpss_sc_buffer *b = NULL;
	u8 grp_id = list_buf->index;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d), grp_id(%d), num_rdy=%d\n",
			vdev->dev_idx, grp_id, vdev->num_rdy[grp_id]);

	b = kmalloc(sizeof(struct vpss_sc_buffer), GFP_ATOMIC);
	if (b == NULL) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "QBUF kmalloc fail\n");
		return -1;
	}
	memcpy(b, list_buf, sizeof(struct vpss_sc_buffer));

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue[grp_id]);
	++vdev->num_rdy[grp_id];
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
	return 0;
}

struct vpss_sc_buffer *cvi_sc_next_buf(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct vpss_sc_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue[grp_id]))
		b = list_first_entry(&vdev->rdy_queue[grp_id], struct vpss_sc_buffer, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

int cvi_sc_buf_num(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	int num;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	num = vdev->num_rdy[grp_id];
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return num;
}

int cvi_sc_buf_remove(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct vpss_sc_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (vdev->num_rdy[grp_id] == 0) {
		spin_unlock_irqrestore(&vdev->rdy_lock, flags);
		return -1;
	}
	if (!list_empty(&vdev->rdy_queue[grp_id])) {
		b = list_first_entry(&vdev->rdy_queue[grp_id],
			struct vpss_sc_buffer, list);
		list_del_init(&b->list);
		kfree(b);
		--vdev->num_rdy[grp_id];
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return 0;
}

void cvi_sc_buf_remove_all(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct vpss_sc_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	while (!list_empty(&vdev->rdy_queue[grp_id])) {
		b = list_first_entry(&vdev->rdy_queue[grp_id],
			struct vpss_sc_buffer, list);
		list_del_init(&b->list);
		kfree(b);
		--vdev->num_rdy[grp_id];
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

void cvi_sc_device_run(void *priv, bool is_tile, bool is_work_on_r_tile, u8 grp_id)
{
	struct cvi_sc_vdev *sdev = priv;
	struct vpss_sc_buffer *b, sb_buf;
	struct vpss_sc_buffer *list_buf = NULL;
	int i;

	if (sdev->sb_enabled[grp_id] && sdev->sb_phy_addr[grp_id][0]) {
		b = &sb_buf;
		for (i = 0; i < 3; i++)
			b->planes[i].addr = sdev->sb_phy_addr[grp_id][i];
	} else {
		list_buf = cvi_sc_next_buf(sdev, grp_id);
		if (list_buf == NULL) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d) grp(%d) no buf\n", sdev->dev_idx, grp_id);
			return;
		}
		b = list_buf;
	}

	if (!(debug & BIT(2)) && sdev->clk)
		clk_enable(sdev->clk);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "update sc%d-buf for grp(%d): 0x%llx-0x%llx-0x%llx\n",
		sdev->dev_idx, grp_id, b->planes[0].addr, b->planes[1].addr, b->planes[2].addr);

	sclr_odma_set_addr(sdev->dev_idx, b->planes[0].addr, b->planes[1].addr, b->planes[2].addr);

	if (!is_tile || !is_work_on_r_tile)
		cvi_sc_update(sdev, &sdev->vpss_chn_cfg[grp_id]);
}

/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
int sc_set_src_to_imgv(struct cvi_sc_vdev *sdev, u8 enable)
{
	struct sclr_top_cfg *cfg = NULL;
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) enable=%d\n",
			sdev->dev_idx, enable);

	if (sdev->dev_idx != 0) {
		CVI_TRACE_VPSS(CVI_DBG_ERR,
			"sc-(%d) invalid for SC-D set img_src.\n",
			sdev->dev_idx);
		return -EINVAL;
	}

	if (enable >= 2) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid parameter(%d).\n", enable);
		return -EINVAL;
	}

	cfg = sclr_top_get_cfg();
	cfg->sclr_d_src = enable;
	sclr_top_set_cfg(cfg);

	sdev->img_src = enable;
	if (enable == 0) {
		bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding = CVI_VIP_IMG_2_SC_D;
		bdev->img_vdev[CVI_VIP_IMG_V].sc_bounding = CVI_VIP_IMG_2_SC_V;
	} else {
		bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding = CVI_VIP_IMG_2_SC_NONE;
		bdev->img_vdev[CVI_VIP_IMG_V].sc_bounding = CVI_VIP_IMG_2_SC_ALL;
	}

	return 0;
}

static int _sc_ext_set_quant(struct cvi_sc_vdev *sdev, const struct cvi_sc_quant_param *param)
{
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(sdev->dev_idx);

	if (!param->enable)
		return 0;

	memcpy(odma_cfg->csc_cfg.quant_form.sc_frac, param->sc_frac, sizeof(param->sc_frac));
	memcpy(odma_cfg->csc_cfg.quant_form.sub, param->sub, sizeof(param->sub));
	memcpy(odma_cfg->csc_cfg.quant_form.sub_frac, param->sub_frac, sizeof(param->sub_frac));

	odma_cfg->csc_cfg.mode = SCL_OUT_QUANT;
	odma_cfg->csc_cfg.quant_round = (enum sclr_quant_rounding)param->rounding;
	odma_cfg->csc_cfg.work_on_border = false;

	sclr_ctrl_set_output(sdev->dev_idx, &odma_cfg->csc_cfg, sdev->fmt->fmt);

	// if fmt is yuv, try use img'csc to convert rgb to yuv.
	if (IS_YUV_FMT(odma_cfg->fmt)) {
		struct cvi_img_vdev *idev = &bdev->img_vdev[sdev->img_src];
		struct sclr_img_cfg *img_cfg = sclr_img_get_cfg(idev->img_type);
		enum sclr_input input;

		img_cfg->csc = (IS_YUV_FMT(img_cfg->fmt))
			     ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;

		if (idev->is_online_from_isp) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "quant for yuv not work in online.\n");
			return -EINVAL;
		}
		cvi_img_get_input(idev->img_type, idev->input_type, &input);
		sclr_ctrl_set_input(idev->img_type, input, img_cfg->fmt, img_cfg->csc,
			idev->input_type == CVI_VIP_INPUT_ISP);
	}

	return 0;
}

static int _sc_ext_set_border(struct cvi_sc_vdev *sdev, const struct cvi_sc_border_param *param)
{
	struct sclr_border_cfg cfg;
	struct sclr_odma_cfg *odma_cfg;

	if (param->enable) {
		// full-size odma for border
		odma_cfg = sclr_odma_get_cfg(sdev->dev_idx);
		odma_cfg->mem.start_x = odma_cfg->mem.start_y = 0;
		odma_cfg->mem.width = odma_cfg->frame_size.w;
		odma_cfg->mem.height = odma_cfg->frame_size.h;
		sclr_odma_set_mem(sdev->dev_idx, &odma_cfg->mem);
	}

	cfg.cfg.b.enable = param->enable;
	cfg.cfg.b.bd_color_r = param->bg_color[0];
	cfg.cfg.b.bd_color_g = param->bg_color[1];
	cfg.cfg.b.bd_color_b = param->bg_color[2];
	cfg.start.x = param->offset_x;
	cfg.start.y = param->offset_y;
	sclr_border_set_cfg(sdev->dev_idx, &cfg);

	return 0;
}

static void _sc_ext_set_coverex(struct cvi_sc_vdev *sdev, const struct cvi_rgn_coverex_cfg *cfg)
{
	int i;
	struct sclr_cover_cfg sc_cover_cfg;

	for (i = 0; i < RGN_COVEREX_MAX_NUM; i++) {
		if (cfg->rgn_coverex_param[i].enable) {
			sc_cover_cfg.start.raw = 0;
			sc_cover_cfg.color.raw = 0;
			sc_cover_cfg.start.b.enable = 1;
			sc_cover_cfg.start.b.x = cfg->rgn_coverex_param[i].rect.left;
			sc_cover_cfg.start.b.y = cfg->rgn_coverex_param[i].rect.top;
			sc_cover_cfg.img_size.w = cfg->rgn_coverex_param[i].rect.width;
			sc_cover_cfg.img_size.h = cfg->rgn_coverex_param[i].rect.height;
			sc_cover_cfg.color.b.cover_color_r = (cfg->rgn_coverex_param[i].color >> 16) & 0xff;
			sc_cover_cfg.color.b.cover_color_g = (cfg->rgn_coverex_param[i].color >> 8) & 0xff;
			sc_cover_cfg.color.b.cover_color_b = cfg->rgn_coverex_param[i].color & 0xff;
		} else {
			sc_cover_cfg.start.raw = 0;
		}
		sclr_cover_set_cfg(sdev->dev_idx, i, &sc_cover_cfg);
	}
}

static void _sc_ext_set_mask(struct cvi_sc_vdev *sdev, const struct cvi_rgn_mosaic_cfg *cfg)
{
	struct sclr_privacy_cfg mask_cfg = {0};
	//struct sclr_img_cfg *img_cfg;
	//struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	//struct cvi_img_vdev *idev = &bdev->img_vdev[sdev->img_src];

	if (cfg->enable) {
		mask_cfg.cfg.raw = 0;
		mask_cfg.cfg.b.enable = 1;
		mask_cfg.cfg.b.mode = 0;
		mask_cfg.cfg.b.grid_size = cfg->blk_size;
		mask_cfg.cfg.b.fit_picture = 0;
		mask_cfg.cfg.b.force_alpha = 0;
		mask_cfg.cfg.b.mask_rgb332 = 0;
		//mask_cfg.cfg.b.blend_y = 1;
		//mask_cfg.cfg.b.y2r_enable = 1;
		mask_cfg.map_cfg.alpha_factor = 256;
		mask_cfg.map_cfg.no_mask_idx = 0;
		mask_cfg.map_cfg.base = cfg->phy_addr;
		mask_cfg.map_cfg.axi_burst = 7;
		mask_cfg.start.x = cfg->start_x;
		mask_cfg.end.x = cfg->end_x - 1;
		mask_cfg.start.y = cfg->start_y;
		mask_cfg.end.y = cfg->end_y - 1;

		//img_cfg= sclr_img_get_cfg(idev->img_type);
		//img_cfg->csc = SCL_CSC_NONE;
		//sclr_img_set_cfg(idev->img_type, img_cfg);
	} else {
		mask_cfg.cfg.raw = 0;
	}
	sclr_pri_set_cfg(sdev->dev_idx, &mask_cfg);
}

int cvi_sc_streamon(struct cvi_sc_vdev *sdev)
{
	int rc = 0;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d)\n", sdev->dev_idx);

	if (atomic_cmpxchg(&sdev->is_streaming, 0, 1) != 0) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) is running\n", sdev->dev_idx);
		return rc;
	}

	//if (!(debug & BIT(2)) && sdev->clk)
	//	clk_prepare(sdev->clk);

	return rc;
}

int cvi_sc_streamoff(struct cvi_sc_vdev *sdev, bool isForce)
{
	int rc = 0, count = 200, i;
	struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d)\n", sdev->dev_idx);

	//It's right between be-done and cvi_vip_try_schedule
	if (bdev->img_vdev[sdev->img_src].isp_triggered)
		usleep_range(5000, 6000);

	if (atomic_cmpxchg(&sdev->is_streaming, 1, 0) != 1) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) is off\n", sdev->dev_idx);
		return rc;
	}

	while (--count > 0 && (!isForce)) {
		if (atomic_read(&sdev->job_state) == CVI_VIP_IDLE)
			break;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 6 * 1000);
	}

	if (atomic_read(&sdev->job_state) != CVI_VIP_IDLE) {
		struct sclr_status sts = sclr_get_status(sdev->dev_idx);

		CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d) isn't idle. crop(%d) hscale(%d) vscale(%d) gop(%d) dma(%d)\n",
			sdev->dev_idx, sts.crop_idle, sts.hscale_idle, sts.vscale_idle, sts.gop_idle, sts.wdma_idle);

		sclr_img_reset(sdev->img_src);
	}
	if (!(debug & BIT(2)) && sdev->clk)
		while (__clk_is_enabled(sdev->clk))
			clk_disable(sdev->clk);

	//if (!(debug & BIT(2)) && sdev->clk)
	//	clk_unprepare(sdev->clk);

	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		cvi_sc_buf_remove_all(sdev, i);
		sdev->sb_enabled[i] = false;
		sdev->sb_vc_ready[i] = false;
		sclr_odma_clear_sb(sdev->dev_idx);
	}

	atomic_set(&sdev->job_state, CVI_VIP_IDLE);

	return rc;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
int sc_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct cvi_sc_vdev *sdev;
	u8 i = 0;
	u8 j = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_SC_MAX; ++i) {
		sdev = &bdev->sc_vdev[i];

		sdev->clk = devm_clk_get(&pdev->dev, CLK_SC_NAME[i]);
		if (IS_ERR(sdev->clk)) {
			pr_err("Cannot get clk for sc-%d\n", i);
			sdev->clk = NULL;
		}

		sdev->dev_idx = i;
		sdev->fmt = cvi_vip_get_format(PIXEL_FORMAT_RGB_888_PLANAR);
		sdev->img_src = (i == 0) ? CVI_VIP_IMG_D : CVI_VIP_IMG_V;
		sdev->sc_coef = CVI_SC_SCALING_COEF_BICUBIC;
		sdev->tile_mode = 0;
		sdev->is_cmdq = false;
		spin_lock_init(&sdev->rdy_lock);
		atomic_set(&sdev->job_state, CVI_VIP_IDLE);
		atomic_set(&sdev->is_streaming, 0);
		memset(&sdev->crop_rect, 0, sizeof(sdev->crop_rect));
		memset(&sdev->compose_out, 0, sizeof(sdev->compose_out));
		memset(&sdev->sink_rect, 0, sizeof(sdev->sink_rect));

		for (j = 0; j < VPSS_ONLINE_NUM; j++) {
			sdev->num_rdy[j] = 0;
			INIT_LIST_HEAD(&sdev->rdy_queue[j]);
			atomic_set(&sdev->buf_empty[j], 0);
		}
	}

	return rc;
}

int sc_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	return 0;
}

void sc_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev)
{
	u8 i = 0;
	struct cvi_sc_vdev *sdev = NULL;
	//struct sclr_core_checksum_status chk_sts;

	for (i = 0; i < CVI_VIP_SC_MAX; ++i) {

		if ((i == 0 &&
			(intr_status.b.scl0_frame_end == 0 || bdev->sc_vdev[i].is_cmdq == true)) ||
		    (i == 1 && intr_status.b.scl1_frame_end == 0) ||
		    (i == 2 && intr_status.b.scl2_frame_end == 0) ||
		    (i == 3 && intr_status.b.scl3_frame_end == 0))
			continue;

		sdev = &bdev->sc_vdev[i];
		CVI_TRACE_VPSS(CVI_DBG_INFO/*CVI_DBG_DEBUG*/, "sc-%d: grp(%d) frame_end\n", i, sdev->job_grp);

		//sclr_core_get_checksum_status(sdev->dev_idx, &chk_sts);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d): checksum (raw=0x%x), enable=%d, in=0x%x, out=0x%x\n",
		//		sdev->dev_idx,
		//		chk_sts.checksum_base.raw,
		//		chk_sts.checksum_base.b.enable,
		//		chk_sts.checksum_base.b.data_in_from_img_in,
		//		chk_sts.checksum_base.b.data_out);

		atomic_set(&sdev->job_state, CVI_VIP_IDLE);
		// in tile mode, only step forward if right-tile is done.
		if (bdev->img_vdev[sdev->img_src].is_tile) {
			if (!bdev->img_vdev[sdev->img_src].is_work_on_r_tile)
				sdev->tile_mode &= ~(SCL_TILE_LEFT);
			else
				sdev->tile_mode &= ~(SCL_TILE_RIGHT);

			if (sdev->tile_mode != 0)
				goto sc_job_finish;

			sclr_set_scale_phase(i, 0, 0);
		}
		bdev->img_vdev[sdev->img_src].IntMask |= (1 << i);

		if ((i == CVI_VIP_SC_D) && bdev->disp_online)
			goto sc_job_finish;

		// One dedicated slice buffer
		if (sdev->sb_enabled[sdev->job_grp])
			goto sc_job_finish;

		if (cvi_sc_buf_remove(sdev, sdev->job_grp))
			CVI_TRACE_VPSS(CVI_DBG_ERR, "no sc%d buf, intr-status(%#x)\n", i, intr_status.raw);

sc_job_finish:
		cvi_vip_job_finish(&bdev->img_vdev[sdev->img_src]);
	}
}

void cvi_sc_update(struct cvi_sc_vdev *sdev, const struct cvi_vpss_chn_cfg *chn_cfg)
{
	u8 i, sbm_num = 0;
	const struct cvi_vip_fmt *fmt;
	struct sclr_odma_cfg *cfg;
	struct sclr_cir_cfg cir_cfg;
	struct sclr_size size;
	struct sclr_rect rect;
	struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "-- sc%d --\n", sdev->dev_idx);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d * %4d)%10s(%4d)\n", "src size", chn_cfg->src_size.width, chn_cfg->src_size.height,
		"sc coef", chn_cfg->sc_coef);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d %4d %4d %4d)\n", "crop rect", chn_cfg->crop.left, chn_cfg->crop.top,
		chn_cfg->crop.width, chn_cfg->crop.height);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d %4d %4d %4d)\n", "dst_rect", chn_cfg->dst_rect.left, chn_cfg->dst_rect.top,
		chn_cfg->dst_rect.width, chn_cfg->dst_rect.height);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d)%10s(%4d)\n", "pitch_y", chn_cfg->bytesperline[0],
		"pitch_c", chn_cfg->bytesperline[1]);

	cfg = sclr_odma_get_cfg(sdev->dev_idx);

	// input
	size.w = chn_cfg->src_size.width;
	size.h = chn_cfg->src_size.height;
	sclr_set_input_size(sdev->dev_idx, size, true);
	rect.x = chn_cfg->crop.left;
	rect.y = chn_cfg->crop.top;
	rect.w = chn_cfg->crop.width;
	rect.h = chn_cfg->crop.height;
	sclr_set_crop(sdev->dev_idx, rect, true);

	// fmt
	fmt = cvi_vip_get_format(chn_cfg->pixelformat);
	cfg->fmt = fmt->fmt;
	cfg->csc_cfg.work_on_border = true;
	if ((chn_cfg->pixelformat == PIXEL_FORMAT_HSV_888) || (chn_cfg->pixelformat == PIXEL_FORMAT_HSV_888_PLANAR))
		cfg->csc_cfg.mode = SCL_OUT_HSV;
	else if (chn_cfg->pixelformat == PIXEL_FORMAT_YUV_PLANAR_444) {
		cfg->csc_cfg.mode = SCL_OUT_CSC;
		cfg->csc_cfg.csc_type = SCL_CSC_601_LIMIT_RGB2YUV;
	} else if (IS_YUV_FMT(cfg->fmt)) {
		cfg->csc_cfg.mode = SCL_OUT_CSC;
		cfg->csc_cfg.csc_type = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;
	} else {
		cfg->csc_cfg.mode = SCL_OUT_DISABLE;
		cfg->csc_cfg.csc_type = SCL_CSC_NONE;
	}
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d)%10s(%4d)%10s(%4d)\n", "fmt", cfg->fmt, "csc mode", cfg->csc_cfg.mode,
		"csc_type", cfg->csc_cfg.csc_type);

	sclr_ctrl_set_output(sdev->dev_idx, &cfg->csc_cfg, cfg->fmt);
	if (cfg->csc_cfg.mode == SCL_OUT_CSC)
		sclr_set_csc(sdev->dev_idx, (struct sclr_csc_matrix *)&chn_cfg->csc_cfg);

	// update sc's output
	size.w = chn_cfg->dst_rect.width;
	size.h = chn_cfg->dst_rect.height;
	sclr_set_output_size(sdev->dev_idx, size);
	sclr_set_scale(sdev->dev_idx);

	if (chn_cfg->mute_cfg.enable) {
		cir_cfg.mode = SCL_CIR_SHAPE;
		cir_cfg.rect.x = 0;
		cir_cfg.rect.y = 0;
		cir_cfg.rect.w = chn_cfg->dst_size.width;
		cir_cfg.rect.h = chn_cfg->dst_size.height;
		cir_cfg.center.x = chn_cfg->dst_size.width >> 1;
		cir_cfg.center.y = chn_cfg->dst_size.height >> 1;
		cir_cfg.radius = MAX(cir_cfg.rect.w, cir_cfg.rect.h);
		cir_cfg.color_r = chn_cfg->mute_cfg.color[0];
		cir_cfg.color_g = chn_cfg->mute_cfg.color[1];
		cir_cfg.color_b = chn_cfg->mute_cfg.color[2];
	} else
		cir_cfg.mode = SCL_CIR_DISABLE;
	sclr_cir_set_cfg(sdev->dev_idx, &cir_cfg);

	// update out-2-mem's pos & size
	cfg->mem.pitch_y = chn_cfg->bytesperline[0];
	cfg->mem.pitch_c = chn_cfg->bytesperline[1];
	cfg->flip = (enum sclr_flip_mode)chn_cfg->flip;
	cfg->frame_size.w = chn_cfg->dst_size.width;
	cfg->frame_size.h = chn_cfg->dst_size.height;
	cfg->mem.width	 = chn_cfg->dst_rect.width;
	cfg->mem.height  = chn_cfg->dst_rect.height;
	if ((cfg->flip == SCL_FLIP_HFLIP) || (cfg->flip == SCL_FLIP_HVFLIP))
		cfg->mem.start_x = cfg->frame_size.w - chn_cfg->dst_rect.width - chn_cfg->dst_rect.left;
	else
		cfg->mem.start_x = chn_cfg->dst_rect.left;
	if ((cfg->flip == SCL_FLIP_VFLIP) || (cfg->flip == SCL_FLIP_HVFLIP))
		cfg->mem.start_y = cfg->frame_size.h - chn_cfg->dst_rect.height - chn_cfg->dst_rect.top;
	else
		cfg->mem.start_y = chn_cfg->dst_rect.top;
	sclr_odma_set_cfg(sdev->dev_idx, cfg);

	_sc_ext_set_border(sdev, &chn_cfg->border_cfg);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d)%10s(%4d %4d)%10s(%4d %4d %4d)\n",
		"border enable", chn_cfg->border_cfg.enable,
		"offset", chn_cfg->border_cfg.offset_x, chn_cfg->border_cfg.offset_y,
		"bgcolor", chn_cfg->border_cfg.bg_color[0], chn_cfg->border_cfg.bg_color[1],
		chn_cfg->border_cfg.bg_color[2]);
	_sc_ext_set_quant(sdev, &chn_cfg->quant_cfg);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%10s(%4d)%10s(%4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)\n",
		"quant enable", chn_cfg->quant_cfg.enable,
		"rounding", chn_cfg->quant_cfg.rounding,
		"factor", chn_cfg->quant_cfg.sc_frac[0], chn_cfg->quant_cfg.sc_frac[1], chn_cfg->quant_cfg.sc_frac[2],
		"sub", chn_cfg->quant_cfg.sub[0], chn_cfg->quant_cfg.sub[1], chn_cfg->quant_cfg.sub[2],
		"sub_frac", chn_cfg->quant_cfg.sub_frac[0], chn_cfg->quant_cfg.sub_frac[1],
		chn_cfg->quant_cfg.sub_frac[2]);

	//cover
	_sc_ext_set_coverex(sdev, &chn_cfg->rgn_coverex_cfg);
	//mosaic
	_sc_ext_set_mask(sdev, &chn_cfg->rgn_mosaic_cfg);

	if (sdev->bind_fb) {
		struct sclr_gop_cfg *disp_cfg = sclr_gop_get_cfg(SCL_GOP_DISP, 0);
		struct sclr_gop_cfg cfg = *disp_cfg;

		sclr_gop_ow_set_cfg(sdev->dev_idx, 0, 0, &disp_cfg->ow_cfg[0], true);
		cfg.gop_ctrl.b.ow0_en = true;
		sclr_gop_set_cfg(sdev->dev_idx, 0, &cfg, true);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "use fb bind\n");
	} else {
		const struct cvi_rgn_cfg *rgn_cfg;
		u8 i, ow_idx;

		for (i = 0; i < RGN_MAX_LAYER_VPSS; ++i) {
			rgn_cfg = &chn_cfg->rgn_cfg[i];
			cvi_vip_set_rgn_cfg(sdev->dev_idx, i, rgn_cfg, &sclr_get_cfg(sdev->dev_idx)->sc.dst);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "gop%d:%10s(%4d)%10s(%4d)%10s(%4d)%10s(%4d)%10s(0x%08x)\n", i,
				"rgn number", rgn_cfg->num_of_rgn,
				"hscale_x2", rgn_cfg->hscale_x2,
				"vscale_x2", rgn_cfg->vscale_x2,
				"colorkey_en", rgn_cfg->colorkey_en,
				"colorkey", rgn_cfg->colorkey);
			if (rgn_cfg->odec.enable) {
				CVI_TRACE_VPSS(CVI_DBG_DEBUG, "gop%d odec:%10s(%4d)%10s(%4d)%10s(%4d)\n", i,
					       "enable", rgn_cfg->odec.enable,
					       "attached_ow", rgn_cfg->odec.attached_ow,
					       "bso_sz", rgn_cfg->odec.bso_sz);

				ow_idx = rgn_cfg->odec.attached_ow;
				CVI_TRACE_VPSS(CVI_DBG_DEBUG,
					"ow%d:%11s(%4d)%10s(%4d %4d %4d %4d)%10s(%6d)%10s(0x%llx)\n", ow_idx,
					"fmt", rgn_cfg->param[ow_idx].fmt,
					"rect", rgn_cfg->param[ow_idx].rect.left, rgn_cfg->param[ow_idx].rect.top,
						rgn_cfg->param[ow_idx].rect.width, rgn_cfg->param[ow_idx].rect.height,
					"stride", rgn_cfg->param[ow_idx].stride,
					"phy_addr", rgn_cfg->param[ow_idx].phy_addr);
			} else {
				for (ow_idx = 0; ow_idx < rgn_cfg->num_of_rgn; ++ow_idx)
					CVI_TRACE_VPSS(CVI_DBG_DEBUG,
						"ow%d:%11s(%4d)%10s(%4d %4d %4d %4d)%10s(%4d)%10s(0x%llx)\n", ow_idx,
						"fmt", rgn_cfg->param[ow_idx].fmt,
						"rect", rgn_cfg->param[ow_idx].rect.left,
							rgn_cfg->param[ow_idx].rect.top,
							rgn_cfg->param[ow_idx].rect.width,
							rgn_cfg->param[ow_idx].rect.height,
						"stride", rgn_cfg->param[ow_idx].stride,
						"phy_addr", rgn_cfg->param[ow_idx].phy_addr);
			}
		}
	}

	if (sclr_check_factor_over4(sdev->dev_idx) && bdev->img_vdev[sdev->img_src].is_tile) {
		sclr_update_coef(sdev->dev_idx, SCL_COEF_DOWNSCALE_SMOOTH, NULL);
	} else if (chn_cfg->sc_coef <= CVI_SC_SCALING_COEF_DOWNSCALE_SMOOTH) {
		sclr_update_coef(sdev->dev_idx, (enum sclr_scaling_coef)chn_cfg->sc_coef, NULL);
	} else if (chn_cfg->sc_coef == CVI_SC_SCALING_COEF_OPENCV_BILINEAR) {
		sclr_set_opencv_scale(sdev->dev_idx);
		sclr_update_coef(sdev->dev_idx, SCL_COEF_BILINEAR, NULL);
	}

	/* odma order : sb_mode -> sb_size -> sb_nb */
	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		if (sdev->sb_enabled[i])
			sbm_num++;

	sclr_set_sclr_to_vc_sb(sdev->dev_idx, (sbm_num > 1) ? 2 : chn_cfg->sb_cfg.sb_mode,
		chn_cfg->sb_cfg.sb_size, chn_cfg->sb_cfg.sb_nb, chn_cfg->sb_cfg.sb_wr_ctrl_idx);

	sclr_core_checksum_en(sdev->dev_idx, true);
}

int sc_set_vpss_chn_cfg(struct cvi_sc_vdev *sdev, struct cvi_vpss_chn_cfg *cfg)
{
	if (cfg->grp_id >= VPSS_ONLINE_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d) set cfg failed.\n", sdev->dev_idx);
		return CVI_FAILURE;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d)\n", sdev->dev_idx);

	memcpy(&sdev->vpss_chn_cfg[cfg->grp_id], cfg, sizeof(*cfg));

	return 0;
}

CVI_S32 vpss_sc_qbuf(struct cvi_vip_dev *dev, struct cvi_buffer *buf, MMF_CHN_S chn)
{
	CVI_U8 i;
	struct cvi_sc_vdev *sdev = NULL;
	struct cvi_img_vdev *idev = NULL;
	struct vpss_sc_buffer list_buf;

	if (!dev)
		return CVI_FAILURE;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "chn(s32DevId=%d, s32ChnId=%d), 0x%llx-0x%llx-0x%llx\n",
		chn.s32DevId, chn.s32ChnId,
		buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);

	if (chn.s32ChnId >= ARRAY_SIZE(dev->sc_vdev)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d) out of range\n",
				chn.s32ChnId);
		return CVI_FAILURE;
	}

	sdev = &dev->sc_vdev[chn.s32ChnId];

	if (sdev->img_src >= ARRAY_SIZE(dev->img_vdev)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "img(%d) out of range\n",
				sdev->img_src);
		return CVI_FAILURE;
	}

	idev = &dev->img_vdev[sdev->img_src];

	// index only work for vi-2-vpss online
	list_buf.index = chn.s32DevId;
	switch (buf->enPixelFormat) {
	default:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_RGB_888_PLANAR:
	case PIXEL_FORMAT_BGR_888_PLANAR:
		list_buf.length = 3;
		break;
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		list_buf.length = 2;
		break;
	case PIXEL_FORMAT_YUV_400:
	case PIXEL_FORMAT_HSV_888:
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_VYUY:
		list_buf.length = 1;
		break;
	}

	for (i = 0; i < list_buf.length; ++i) {
		list_buf.planes[i].addr = buf->phy_addr[i];
		list_buf.planes[i].length = buf->length[i];
	}

	if (buf->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
		CVI_U64 tmp = list_buf.planes[0].addr;

		list_buf.planes[0].addr = list_buf.planes[2].addr;
		list_buf.planes[2].addr = tmp;
	}

	if (idev->is_online_from_isp) {
		if (list_buf.index >= VPSS_ONLINE_NUM)
			list_buf.index = 0;
		if (cvi_sc_buf_num(sdev, list_buf.index) > 1) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d): online, previous buf_index(%d) isn't finished yet\n",
				sdev->dev_idx, list_buf.index);
			return CVI_FAILURE;
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d), grp_id(%d), num_rdy=%d , phy:(%#llx)\n", sdev->dev_idx,
			list_buf.index, sdev->num_rdy[list_buf.index], list_buf.planes[0].addr);
	} else {
		list_buf.index = 0;
		if (cvi_sc_buf_num(sdev, list_buf.index)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d): previous buf_index(%d) isn't finished yet\n",
					sdev->dev_idx, list_buf.index);
			return CVI_FAILURE;
		}
	}

	if (cvi_sc_buf_queue(sdev, &list_buf))
		return CVI_FAILURE;
	if (idev->is_online_from_isp && (atomic_cmpxchg(&sdev->buf_empty[list_buf.index], 1, 0) == 1))
		cvi_sc_trigger_post(dev);

	//vpss_sc_sb_qbuf(dev, NULL, chn);

	return CVI_SUCCESS;
}

CVI_VOID vpss_sc_sb_qbuf(struct cvi_vip_dev *dev, struct cvi_buffer *buf, MMF_CHN_S chn)
{
	int i;
	struct cvi_sc_vdev *sdev = NULL;
	int grp_id = (chn.s32DevId >= VPSS_ONLINE_NUM) ? 0 : chn.s32DevId;

	if (!dev)
		return;

	if (chn.s32ChnId >= ARRAY_SIZE(dev->sc_vdev))
		return;

	sdev = &dev->sc_vdev[chn.s32ChnId];

	if (buf) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d), sb enabled 0x%llx-0x%llx-0x%llx\n",
				sdev->dev_idx,
				buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);

		sdev->sb_enabled[grp_id] = true;

		for (i = 0; i < 3; i++)
			sdev->sb_phy_addr[grp_id][i] = buf->phy_addr[i];
	} else {
		sdev->sb_enabled[grp_id] = false;
		sdev->sb_vc_ready[grp_id] = false;
		for (i = 0; i < 3; i++)
			sdev->sb_phy_addr[grp_id][i] = 0;
	}
}

CVI_VOID vpss_sc_set_vc_sbm(struct cvi_vip_dev *dev, MMF_CHN_S chn, bool sb_vc_ready)
{
	struct cvi_sc_vdev *sdev = NULL;
	int grp_id = (chn.s32DevId >= VPSS_ONLINE_NUM) ? 0 : chn.s32DevId;

	if (!dev)
		return;

	if (chn.s32ChnId >= ARRAY_SIZE(dev->sc_vdev))
		return;

	sdev = &dev->sc_vdev[chn.s32ChnId];

	sdev->sb_vc_ready[grp_id] = sb_vc_ready;
}

CVI_VOID sc_set_vpss_chn_bind_fb(struct cvi_sc_vdev *sdev, bool bind_fb)
{
	sdev->bind_fb = bind_fb;
}

