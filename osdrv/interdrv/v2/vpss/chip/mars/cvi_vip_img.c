#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/uaccess.h>

#include "vpss_debug.h"
#include "vpss_common.h"
#include "scaler.h"

#include "vpss_core.h"
#include "cvi_vip_img.h"
#include "cvi_vip_sc.h"
#include "vpss.h"

static const char *const CLK_IMG_NAME[] = {"clk_img_d", "clk_img_v"};

/* abort streaming and wait for last buffer */
void cvi_img_stop_streaming(struct cvi_img_vdev *idev)
{
	struct vpss_img_buffer *cvi_vb2;
	unsigned long flags;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d)\n",
			idev->dev_idx, idev->img_type, idev->input_type);

	/*
	 * Release all the buffers enqueued to driver
	 * when streamoff is issued
	 */
	spin_lock_irqsave(&idev->rdy_lock, flags);
	while (!list_empty(&idev->rdy_queue)) {
		cvi_vb2 = list_first_entry(&idev->rdy_queue,
			struct vpss_img_buffer, list);
		list_del_init(&cvi_vb2->list);
		kfree(cvi_vb2);
	}
	idev->num_rdy = 0;
	INIT_LIST_HEAD(&idev->rdy_queue);
	spin_unlock_irqrestore(&idev->rdy_lock, flags);
}

int cvi_img_get_input(enum sclr_img_in img_type,
	enum cvi_input_type input_type, enum sclr_input *input)
{
	if (img_type == SCL_IMG_D) {
		if (input_type == CVI_VIP_INPUT_DWA) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "img_d doesn't have dwa input.\n");
			return -EINVAL;
		}

		*input = (input_type == CVI_VIP_INPUT_ISP || input_type == CVI_VIP_INPUT_ISP_POST) ?
			SCL_INPUT_ISP : SCL_INPUT_MEM;
	} else {
		*input = (input_type == CVI_VIP_INPUT_ISP || input_type == CVI_VIP_INPUT_ISP_POST) ?
			SCL_INPUT_ISP : (input_type == CVI_VIP_INPUT_DWA) ?
			SCL_INPUT_DWA : SCL_INPUT_MEM;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img_type=%d, input_type=%d, input=%d\n",
			img_type, input_type, *input);

	return 0;
}

/**
 * cvi_img_get_sc_bound() - to know which sc is bound with this img
 * @idev: the img-dev to check
 * @sc_bound: true if the sc-dev bound with idev
 */
void cvi_img_get_sc_bound(struct cvi_img_vdev *idev, bool sc_bound[])
{
	u8 i, sc_start_idx, chn_num;

	switch (idev->sc_bounding) {
	default:
	case CVI_VIP_IMG_2_SC_NONE:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = 0;
	break;

	case CVI_VIP_IMG_2_SC_D:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = SCL_D_MAX_INST;
	break;

	case CVI_VIP_IMG_2_SC_V:
		sc_start_idx = CVI_VIP_SC_V1;
		chn_num = SCL_V_MAX_INST;
	break;

	case CVI_VIP_IMG_2_SC_ALL:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = SCL_MAX_INST;
	break;
	}

	memset(sc_bound, false, sizeof(sc_bound[0]) * CVI_VIP_SC_MAX);
	for (i = 0; i < chn_num; ++i) {
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), idev->vpss_grp_cfg[%d].chn_enable[%d]=%d\n",
		//	idev->dev_idx,
		//	idev->img_type,
		//	idev->input_type,
		//	idev->job_grp, i,
		//	idev->vpss_grp_cfg[idev->job_grp].chn_enable[i]);

		if (idev->vpss_grp_cfg[idev->job_grp].chn_enable[i])
			sc_bound[i + sc_start_idx] = true;
	}
}

/**
 * cvi_img_device_run_work() - run pending jobs for the context
 * @data: data used for scheduling the execution of this function.
 */
static void cvi_img_device_run_work(unsigned long data)
{
	struct cvi_img_vdev *idev = (struct cvi_img_vdev *)data;

	cvi_vip_try_schedule(idev, idev->next_job_grp);
}

int img_s_input(struct cvi_img_vdev *idev, CVI_U32 i)
{
	int rc = 0;
	enum sclr_input input;
	struct sclr_img_cfg *cfg;

	if (i >= CVI_VIP_INPUT_MAX)
		return -EINVAL;

	idev->input_type = i;

	// update hw
	cfg = sclr_img_get_cfg(idev->img_type);
	rc = cvi_img_get_input(idev->img_type, i, &input);
	if (rc == 0) {
		idev->is_online_from_isp = (i == CVI_VIP_INPUT_ISP) || (i == CVI_VIP_INPUT_ISP_POST);
		sclr_ctrl_set_input(idev->img_type, input, cfg->fmt, cfg->csc, i == CVI_VIP_INPUT_ISP);
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), input=%d, fmt=%d, csc=%d, is_online_from_isp=%d\n",
			idev->dev_idx, idev->img_type, idev->input_type,
			cfg->fmt, cfg->csc, input, idev->is_online_from_isp);

	return rc;
}

int img_g_input(struct cvi_img_vdev *idev, CVI_U32 *i)
{
	int rc = 0;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d)\n",
			idev->dev_idx, idev->img_type, idev->input_type);

	*i = idev->input_type;
	return rc;
}

int cvi_img_streamon(struct cvi_img_vdev *idev)
{
	struct cvi_vip_dev *bdev =
		container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	int rc = 0;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), is_online_from_isp=%d, job_flags=0x%lx\n",
			idev->dev_idx, idev->img_type, idev->input_type,
			idev->is_online_from_isp, idev->job_flags);

	if (atomic_cmpxchg(&idev->is_streaming, 0, 1) != 0) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d) is running\n",
				idev->dev_idx, idev->img_type, idev->input_type);
		return rc;
	}

	if ((idev->img_type == SCL_IMG_D) && (idev->input_type != CVI_VIP_INPUT_MEM) && bdev->disp_online) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "IMG_D can't enable both disp_online and input from %s.\n"
			, (idev->input_type ? "ISP" : "DWA"));
		return -EPERM;
	}

	if (!rc) {
		if (!(debug & BIT(2)) && idev->clk)
			clk_prepare(idev->clk);
		tasklet_enable(&idev->job_work);
		if (!idev->is_online_from_isp) {
			idev->user_trig_cnt++;
			if (cvi_vip_try_schedule(idev, 0))
				idev->user_trig_fail_cnt++;
		}
	}

	return rc;
}

int cvi_img_streamoff(struct cvi_img_vdev *idev, bool isForce)
{
	int rc = 0, count = 10;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d), is_online_from_isp=%d, job_flags=0x%lx\n",
			idev->dev_idx, idev->img_type, idev->input_type,
			idev->is_online_from_isp, idev->job_flags);

	if (atomic_cmpxchg(&idev->is_streaming, 1, 0) != 1) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d) is off\n",
				idev->dev_idx, idev->img_type, idev->input_type);
		return rc;
	}

	tasklet_disable(&idev->job_work);

	while (--count > 0 && (!isForce)) {
		if (!cvi_vip_job_is_queued(idev))
			break;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 6 * 1000);
	}

	if (cvi_vip_job_is_queued(idev)) {
		union sclr_img_dbg_status status = sclr_img_get_dbg_status(idev->img_type, true);

		CVI_TRACE_VPSS(CVI_DBG_ERR, "img(%d) isn't idle\n", idev->dev_idx);
		CVI_TRACE_VPSS(CVI_DBG_ERR,
			"err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
			, status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
			 , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
			, status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
		CVI_TRACE_VPSS(CVI_DBG_ERR, "ip idle(%d)\t\tip int(%d)\n", status.b.ip_idle, status.b.ip_int);

		sclr_img_reset(idev->dev_idx);
	}

	if (!(debug & BIT(2)) && idev->clk)
		while (__clk_is_enabled(idev->clk))
			clk_disable(idev->clk);

	cvi_img_stop_streaming(idev);

	if (!(debug & BIT(2)) && idev->clk)
		clk_unprepare(idev->clk);

	idev->job_flags = 0;
	idev->is_tile = false;
	idev->is_work_on_r_tile = true;
	idev->img_sb_cfg.sb_mode = 0;
	return rc;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
int img_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct cvi_img_vdev *idev;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		idev = &bdev->img_vdev[i];

		idev->clk = devm_clk_get(&pdev->dev, CLK_IMG_NAME[i]);
		if (IS_ERR(idev->clk)) {
			pr_err("Cannot get clk for img-%d\n", i);
			idev->clk = NULL;
		}

		idev->dev_idx = i;
		idev->job_grp = 0;
		idev->img_type = (i == 0) ? SCL_IMG_D : SCL_IMG_V;
		idev->sc_bounding =
			(i == 0) ? CVI_VIP_IMG_2_SC_D : CVI_VIP_IMG_2_SC_V;
		idev->is_tile = false;
		idev->is_work_on_r_tile = true;
		idev->tile_mode = 0;
		idev->is_online_from_isp = false;
		idev->isp_triggered = false;
		idev->is_cmdq = false;
		idev->IntMask = 0;
		atomic_set(&idev->is_streaming, 0);
		spin_lock_init(&idev->job_lock);
		memset(&idev->src_size, 0, sizeof(idev->src_size));
		memset(&idev->crop_rect, 0, sizeof(idev->crop_rect));
		memset(&idev->post_para, 0, sizeof(idev->post_para));

		spin_lock_init(&idev->rdy_lock);
		INIT_LIST_HEAD(&idev->rdy_queue);
		idev->num_rdy = 0;

		idev->job_flags = 0;
		tasklet_init(&idev->job_work, cvi_img_device_run_work, (unsigned long)idev);
		tasklet_disable(&idev->job_work);

		CVI_TRACE_VPSS(CVI_DBG_INFO, "img registered\n");
	}

	return rc;
}

int img_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_img_vdev *idev;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		idev = &bdev->img_vdev[i];
		tasklet_kill(&idev->job_work);
	}

	return 0;
}

void img_irq_handler(union sclr_intr intr_status, u8 cmdq_intr_status, struct cvi_vip_dev *bdev)
{
	u8 img_idx = 0;
	struct cvi_img_vdev *idev = NULL;
	//struct sclr_img_checksum_status chk_sts;

	for (img_idx = 0; img_idx < CVI_VIP_IMG_MAX; ++img_idx) {
		struct vpss_img_buffer *img_b = NULL;

		// check if frame_done
		if (((img_idx == CVI_VIP_IMG_D) &&
				(intr_status.b.img_in_d_frame_end == 0 || bdev->img_vdev[img_idx].is_cmdq == true)) ||
		    ((img_idx == CVI_VIP_IMG_V) &&
				(intr_status.b.img_in_v_frame_end == 0)))
			continue;

		idev = &bdev->img_vdev[img_idx];
		idev->job_flags &= ~(TRANS_RUNNING);
		ktime_get_ts64(&idev->ts_end);
		idev->hw_duration = get_diff_in_us(idev->ts_start, idev->ts_end);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img-%d: job_grp(%d) frame_end\n",
				img_idx, idev->job_grp);

		//sclr_img_get_checksum_status(idev->img_type, &chk_sts);
		//CVI_TRACE_VPSS(CVI_DBG_DEBUG,
		//	"img(%d:%d:%d) checksum(raw=0x%x) enable=%d, in=0x%x, out=0x%x\n"
		//	, img_idx, idev->img_type, idev->input_type,
		//	chk_sts.checksum_base.raw,
		//	chk_sts.checksum_base.b.enable,
		//	chk_sts.axi_read_data,
		//	chk_sts.checksum_base.b.output_data);

		// in tile mode, only step forward if right-tile is done.
		if (idev->is_tile) {
			if (!idev->is_work_on_r_tile)
				idev->tile_mode &= ~(SCL_TILE_LEFT);
			else
				idev->tile_mode &= ~(SCL_TILE_RIGHT);

			if (idev->tile_mode != 0) {
				cvi_vip_job_finish(idev);
				continue;
			}
		}

		idev->irq_cnt[idev->job_grp]++;
		++idev->frame_number[idev->job_grp];

		// if input isn't memory, don't care buffer.
		if (idev->input_type != CVI_VIP_INPUT_MEM) {
			cvi_vip_job_finish(idev);
			continue;
		}

		// check buf-num if online to disp
		if ((img_idx == CVI_VIP_IMG_D) && bdev->disp_online)
			if (idev->num_rdy <= 1)
				continue;

		img_b = cvi_vip_buf_remove((struct cvi_base_vdev *)idev);
		if (!img_b) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "no img%d buf, intr-status(%#x)\n",
					img_idx, intr_status.raw);
			continue;
		}

		kfree(img_b);

		// update job-flag and see if there are other jobs
		cvi_vip_job_finish(idev);
	}
}

void cvi_img_update(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg)
{
	const struct cvi_vip_fmt *fmt;
	struct sclr_img_cfg *cfg;

	fmt = cvi_vip_get_format(grp_cfg->pixelformat);
	cfg = sclr_img_get_cfg(idev->img_type);

	if (idev->is_online_from_isp) {
		cfg->fmt = SCL_FMT_YUV422;
		cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
	} else {
		cfg->fmt = fmt->fmt;
		if (grp_cfg->pixelformat == PIXEL_FORMAT_YUV_PLANAR_444)
			cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
		else if (IS_YUV_FMT(cfg->fmt))
			cfg->csc = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_YUV2RGB;
		else
			cfg->csc = SCL_CSC_NONE;
	}
	cfg->mem.pitch_y = grp_cfg->bytesperline[0];
	cfg->mem.pitch_c = grp_cfg->bytesperline[1];

	cfg->mem.start_x = grp_cfg->crop.left;
	cfg->mem.start_y = grp_cfg->crop.top;
	cfg->mem.width	 = grp_cfg->crop.width;
	cfg->mem.height  = grp_cfg->crop.height;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG,
		"img(%d:%d:%d) -> sclr_img_set_cfg, is_online_from_isp=%d, fmt=%d, csc=%d\n",
		idev->dev_idx, idev->img_type, idev->input_type,
		idev->is_online_from_isp, cfg->fmt, cfg->csc);
	sclr_img_set_cfg(idev->img_type, cfg);

	sclr_img_set_csc(idev->img_type, (struct sclr_csc_matrix *)&grp_cfg->csc_cfg);
}

int img_set_vpss_grp_cfg(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg)
{
	if (grp_cfg->grp_id >= VPSS_ONLINE_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "img(%d) set cfg failed.\n", idev->dev_idx);
		return CVI_FAILURE;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d:%d:%d): grp_id(%d) chn_en(%d %d %d %d).\n",
		idev->dev_idx, idev->img_type, idev->input_type, grp_cfg->grp_id,
		grp_cfg->chn_enable[0], grp_cfg->chn_enable[1], grp_cfg->chn_enable[2],
		grp_cfg->chn_enable[3]);

	memcpy(&idev->vpss_grp_cfg[grp_cfg->grp_id], grp_cfg, sizeof(*grp_cfg));

	return 0;
}
