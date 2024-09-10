#include <linux/slab.h>

#include <vo_sdk_layer.h>

#include <linux/cvi_base.h>
#include <linux/cvi_base_ctx.h>
#include <linux/cvi_buffer.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_defines.h>
#include <vb.h>
#include <linux/uaccess.h>
#include "scaler.h"
#include "sys.h"
#include "vo.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern struct cvi_vo_ctx *gVoCtx;
extern const struct vo_disp_pattern patterns[CVI_VIP_PAT_MAX];
static struct cvi_vo_dev *gvdev;

struct vo_fmt vo_sdk_formats[] = {
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_420,
	.fmt		 = SCL_FMT_YUV420,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_422,
	.fmt		 = SCL_FMT_YUV422,
	.bit_depth	 = { 8, 4, 4 },
	.buffers	 = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_PLANAR_444,
	.fmt		 = SCL_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV12,
	.fmt		 = SCL_FMT_NV12,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV21,
	.fmt		 = SCL_FMT_NV21,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV16,
	.fmt		 = SCL_FMT_YUV422SP1,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_NV61,
	.fmt		 = SCL_FMT_YUV422SP2,
	.bit_depth	 = { 8, 8, 0 },
	.buffers	 = 2,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUYV,
	.fmt		 = SCL_FMT_YUYV,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc		 = PIXEL_FORMAT_YVYU,
	.fmt		 = SCL_FMT_YVYU,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_UYVY,
	.fmt		 = SCL_FMT_UYVY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_VYUY,
	.fmt		 = SCL_FMT_VYUY,
	.bit_depth	 = { 16 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888_PLANAR, /* rgb */
	.fmt		 = SCL_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_RGB_888_PLANAR, /* rgb */
	.fmt		 = SCL_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},

	{
	.fourcc	 = PIXEL_FORMAT_RGB_888, /* rgb */
	.fmt		 = SCL_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_BGR_888, /* bgr */
	.fmt		 = SCL_FMT_BGR_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_YUV_400, /* Y-Only */
	.fmt		 = SCL_FMT_Y_ONLY,
	.bit_depth	 = { 8 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888, /* hsv */
	.fmt		 = SCL_FMT_RGB_PACKED,
	.bit_depth	 = { 24 },
	.buffers	 = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc	 = PIXEL_FORMAT_HSV_888_PLANAR, /* hsv */
	.fmt		 = SCL_FMT_RGB_PLANAR,
	.bit_depth	 = { 8, 8, 8 },
	.buffers	 = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

VO_SYNC_INFO_S stSyncInfo[VO_OUTPUT_BUTT] = {
	[VO_OUTPUT_800x600_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 600, .u16Vbb = 24, .u16Vfb = 1
		, .u16Hact = 800, .u16Hbb = 88, .u16Hfb = 40
		, .u16Vpw = 4, .u16Hpw = 128, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P24] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 24
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 638
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P25] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 25
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P30] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 30
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 440
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 110
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_576P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 576, .u16Vbb = 39, .u16Vfb = 5
		, .u16Hact = 720, .u16Hbb = 68, .u16Hfb = 12
		, .u16Vpw = 5, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_480P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 480, .u16Vbb = 30, .u16Vfb = 9
		, .u16Hact = 720, .u16Hbb = 60, .u16Hfb = 16
		, .u16Vpw = 6, .u16Hpw = 62, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720x1280_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1280, .u16Vbb = 4, .u16Vfb = 6
		, .u16Hact = 720, .u16Hbb = 36, .u16Hfb = 128
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_1080x1920_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1920, .u16Vbb = 36, .u16Vfb = 6
		, .u16Hact = 1080, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_480x800_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 800, .u16Vbb = 20, .u16Vfb = 20
		, .u16Hact = 480, .u16Hbb = 50, .u16Hfb = 50
		, .u16Vpw = 10, .u16Hpw = 10, .bIdv = 0, .bIhs = 0, .bIvs = 1},
};

/****************************************************************************
 * SDK layer Defines
 ****************************************************************************/
#define DEFAULT_MESH_PADDR	0x80000000

#define VO_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400))

#define GDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||           \
	 (fmt == PIXEL_FORMAT_YUV_400))
/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/


CVI_S32 vo_clear_chnbuf(VO_LAYER VoLayer, VO_CHN VoChn, bool bClrAll)
{
	int i = 0;
	struct vb_s *vb;

	CVI_S32 ret = CVI_FAILURE;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};
	struct vb_jobs_t *jobs = base_get_jobs_by_chn(chn, CHN_TYPE_IN);

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!bClrAll)
		return CVI_SUCCESS;

	gVoCtx->clearchnbuf = 1;

	if (FIFO_EMPTY(&jobs->workq))
		return CVI_SUCCESS;

	mutex_lock(&jobs->lock);
	FIFO_GET_TAIL(&jobs->workq, &vb);

	if ((gVoCtx->stLayerAttr.enPixFormat == PIXEL_FORMAT_YUV_PLANAR_420)
	 || (gVoCtx->stLayerAttr.enPixFormat == PIXEL_FORMAT_NV12)
	 || (gVoCtx->stLayerAttr.enPixFormat == PIXEL_FORMAT_NV21)) {

		for (i = 0; i < 3; i++) {
			if (vb->buf.length[i] == 0)
				continue;

			if (i == 0)
				memset(vb->vir_addr, 0x00, vb->buf.length[i]);
			else if (i == 1)
				memset(vb->vir_addr + vb->buf.length[0], 0x80, vb->buf.length[i]);
			else
				memset(vb->vir_addr + vb->buf.length[0] + vb->buf.length[1], 0x80, vb->buf.length[i]);

			sys_cache_flush(vb->buf.phy_addr[i], vb->vir_addr, vb->buf.length[i]);
		}
	} else {
		vo_pr(VO_ERR, "enPixFormat not support yet.\n");

		gVoCtx->clearchnbuf = 0;
		mutex_unlock(&jobs->lock);

		return ret;
	}
	mutex_unlock(&jobs->lock);
	return ret;
}

CVI_S32 vo_send_frame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = VoLayer, .s32ChnId = VoChn};
	VB_BLK blk;
	SIZE_S stSize;
	CVI_S32 ret = CVI_FAILURE;

	UNUSED(s32MilliSec);

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if ((gVoCtx->enRotation == ROTATION_90) || (gVoCtx->enRotation == ROTATION_270)) {

		stSize.u32Width = gVoCtx->stLayerAttr.stImageSize.u32Height;
		stSize.u32Height = gVoCtx->stLayerAttr.stImageSize.u32Width;
	} else
		stSize = gVoCtx->stLayerAttr.stImageSize;

	if (gVoCtx->stLayerAttr.enPixFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) PixelFormat(%d) mismatch.\n"
			, VoLayer, VoChn, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((stSize.u32Width != (pstVideoFrame->stVFrame.u32Width -
		pstVideoFrame->stVFrame.s16OffsetLeft - pstVideoFrame->stVFrame.s16OffsetRight))
	 || (stSize.u32Height != (pstVideoFrame->stVFrame.u32Height -
		pstVideoFrame->stVFrame.s16OffsetTop - pstVideoFrame->stVFrame.s16OffsetBottom))) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) Size(%d * %d) mismatch.\n"
			, VoLayer, VoChn, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) frame offset (%d %d %d %d) invalid\n",
			VoLayer, VoChn,
			pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight,
			pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (IS_FMT_YUV420(gVoCtx->stLayerAttr.enPixFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) YUV420 can't accept odd frame valid width\n",
				VoLayer, VoChn);
			vo_pr(VO_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if ((pstVideoFrame->stVFrame.u32Height - pstVideoFrame->stVFrame.s16OffsetTop -
		     pstVideoFrame->stVFrame.s16OffsetBottom) & 0x01) {
			vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) YUV420 can't accept odd frame valid height\n",
				VoLayer, VoChn);
			vo_pr(VO_ERR, "u32Height(%d) s16OffsetTop(%d) s16OffsetBottom(%d)\n",
				pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.s16OffsetTop,
				pstVideoFrame->stVFrame.s16OffsetBottom);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(gVoCtx->stLayerAttr.enPixFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) YUV422 can't accept odd frame valid width\n",
				VoLayer, VoChn);
			vo_pr(VO_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
	}

	gVoCtx->clearchnbuf = 0;

	blk = vb_physAddr2Handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) Invalid phy-addr(%llx). Can't locate VB_BLK.\n"
			      , VoLayer, VoChn, pstVideoFrame->stVFrame.u64PhyAddr[0]);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) Invalid parameter\n", VoLayer, VoChn);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	ret = vb_qbuf(chn, CHN_TYPE_IN, blk);
	vo_pr(VO_INFO, "vb_qbuf ret[%d]\n", ret);

	return ret;
}

CVI_S32 vo_get_chn_attr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);

	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstChnAttr, &gVoCtx->stChnAttr, sizeof(*pstChnAttr));

	return CVI_SUCCESS;
}

CVI_S32 vo_get_panelstatus(VO_LAYER VoLayer, VO_CHN VoChn, CVI_U32 *is_init)
{
	CVI_U32 is_init_panel = 0;
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;
	if (sclr_disp_mux_get() == SCLR_VO_SEL_I80) {
		is_init_panel = sclr_disp_check_i80_enable();
	} else {
		is_init_panel = sclr_disp_check_tgen_enable();
	}

	memcpy(is_init, &is_init_panel, sizeof(CVI_U32));

	return CVI_SUCCESS;
}
CVI_S32 vo_set_pub_attr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	struct cvi_disp_intf_cfg cfg;

	struct vo_dv_timings dv_timings;
	CVI_U16 rgb[3], i;
	CVI_U32  panel_status = 0;
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstPubAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->is_dev_enable[VoDev]) {
		vo_pr(VO_ERR, "VO DEV(%d) should be disabled.\n", VoDev);
		return CVI_ERR_VO_DEV_HAS_ENABLED;
	}

	memset(&cfg, 0, sizeof(cfg));

	if (pstPubAttr->enIntfSync == VO_OUTPUT_USER) {
		//vo_get_dv_timings(d->fd, &dv_timings);
		dv_timings.bt.interlaced = !pstPubAttr->stSyncInfo.bIop;
		dv_timings.bt.height = pstPubAttr->stSyncInfo.u16Vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = pstPubAttr->stSyncInfo.u16Vbb;
		dv_timings.bt.vfrontporch = pstPubAttr->stSyncInfo.u16Vfb;
		dv_timings.bt.width = pstPubAttr->stSyncInfo.u16Hact;
		dv_timings.bt.hbackporch = pstPubAttr->stSyncInfo.u16Hbb;
		dv_timings.bt.hfrontporch = pstPubAttr->stSyncInfo.u16Hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = pstPubAttr->stSyncInfo.u16Hpw;
		dv_timings.bt.vsync = pstPubAttr->stSyncInfo.u16Vpw;
		dv_timings.bt.polarities = ((pstPubAttr->stSyncInfo.bIvs) ? 0 : 0x1)
					| ((pstPubAttr->stSyncInfo.bIhs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = pstPubAttr->stSyncInfo.u16FrameRate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
	} else if (pstPubAttr->enIntfSync < VO_OUTPUT_USER) {
		dv_timings.bt.interlaced = !stSyncInfo[pstPubAttr->enIntfSync].bIop;
		dv_timings.bt.height = stSyncInfo[pstPubAttr->enIntfSync].u16Vact << dv_timings.bt.interlaced;
		dv_timings.bt.vbackporch = stSyncInfo[pstPubAttr->enIntfSync].u16Vbb;
		dv_timings.bt.vfrontporch = stSyncInfo[pstPubAttr->enIntfSync].u16Vfb;
		dv_timings.bt.width = stSyncInfo[pstPubAttr->enIntfSync].u16Hact;
		dv_timings.bt.hbackporch = stSyncInfo[pstPubAttr->enIntfSync].u16Hbb;
		dv_timings.bt.hfrontporch = stSyncInfo[pstPubAttr->enIntfSync].u16Hfb;
		dv_timings.bt.il_vbackporch = 0;
		dv_timings.bt.il_vfrontporch = 0;
		dv_timings.bt.il_vsync = 0;
		dv_timings.bt.hsync = stSyncInfo[pstPubAttr->enIntfSync].u16Hpw;
		dv_timings.bt.vsync = stSyncInfo[pstPubAttr->enIntfSync].u16Vpw;
		dv_timings.bt.polarities = ((stSyncInfo[pstPubAttr->enIntfSync].bIvs) ? 0 : 0x1)
					| ((stSyncInfo[pstPubAttr->enIntfSync].bIhs) ? 0 : 0x2);
		dv_timings.bt.pixelclock = stSyncInfo[pstPubAttr->enIntfSync].u16FrameRate
					* (dv_timings.bt.vbackporch + dv_timings.bt.height
					   + dv_timings.bt.vfrontporch + dv_timings.bt.vsync)
					* (dv_timings.bt.hbackporch + dv_timings.bt.width
					   + dv_timings.bt.hfrontporch + dv_timings.bt.hsync);
	} else {
		vo_pr(VO_ERR, "VO Sync Info(%d) invalid.\n", pstPubAttr->enIntfSync);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (dv_timings.bt.interlaced) {
		vo_pr(VO_ERR, "VO not support interlaced timing.\n");
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	if ((dv_timings.bt.pixelclock == 0) || (dv_timings.bt.height == 0) || (dv_timings.bt.width == 0)) {
		vo_pr(VO_ERR, "VO Sync timing) invalid. width(%d) height(%d) pixelclock(%llu)\n"
			, dv_timings.bt.width, dv_timings.bt.height, dv_timings.bt.pixelclock);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstPubAttr->enIntfType >= VO_INTF_LCD_18BIT) && (pstPubAttr->enIntfType <= VO_INTF_LCD_30BIT)) {
		cfg.intf_type = CVI_VIP_DISP_INTF_LVDS;
		if (pstPubAttr->enIntfType == VO_INTF_LCD_18BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_6BIT;
		else if (pstPubAttr->enIntfType == VO_INTF_LCD_24BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_8BIT;
		else if (pstPubAttr->enIntfType == VO_INTF_LCD_30BIT)
			cfg.lvds_cfg.out_bits = LVDS_OUT_10BIT;
		else
			cfg.lvds_cfg.out_bits = LVDS_OUT_8BIT;

		cfg.lvds_cfg.mode = (enum LVDS_MODE)pstPubAttr->stLvdsAttr.mode;
		cfg.lvds_cfg.chn_num = pstPubAttr->stLvdsAttr.chn_num;
		cfg.lvds_cfg.chn_num = 1;
		cfg.lvds_cfg.vs_out_en = 1;
		cfg.lvds_cfg.hs_out_en = 1;
		cfg.lvds_cfg.hs_blk_en = 1;
		cfg.lvds_cfg.msb_lsb_data_swap = 1;
		cfg.lvds_cfg.serial_msb_first = pstPubAttr->stLvdsAttr.data_big_endian;
		cfg.lvds_cfg.even_odd_link_swap = 0;
		cfg.lvds_cfg.enable = 1;
		cfg.lvds_cfg.pixelclock = pstPubAttr->stLvdsAttr.pixelclock;
		//skip u64 devide
		cfg.lvds_cfg.pixelclock = div_u64(dv_timings.bt.pixelclock, 1000);
		cfg.lvds_cfg.backlight_gpio_num = pstPubAttr->stLvdsAttr.backlight_pin.gpio_num;

		for (i = 0; i < VO_LVDS_LANE_MAX; ++i) {
			cfg.lvds_cfg.lane_id[i] = pstPubAttr->stLvdsAttr.lane_id[i];
			cfg.lvds_cfg.lane_pn_swap[i] = pstPubAttr->stLvdsAttr.lane_pn_swap[i];
		}

		if (vo_set_interface(gvdev, &cfg) != 0) {
			vo_pr(VO_ERR, "VO INTF configure failured.\n");
			return CVI_FAILURE;
		}
	} else if ((pstPubAttr->enIntfType == VO_INTF_MIPI) || (pstPubAttr->enIntfType == VO_INTF_MIPI_SLAVE)) {
		cfg.intf_type = CVI_VIP_DISP_INTF_DSI;
		vo_pr(VO_DBG, "MIPI-DSI should be setup by mipi-tx.\n");
	} else if (pstPubAttr->enIntfType == VO_INTF_I80) {
		const VO_I80_CFG_S *psti80Cfg = &pstPubAttr->sti80Cfg;

		if ((psti80Cfg->lane_s.CS > 3) || (psti80Cfg->lane_s.RS > 3) ||
		    (psti80Cfg->lane_s.WR > 3) || (psti80Cfg->lane_s.RD > 3)) {
			vo_pr(VO_ERR, "VO DEV(%d) I80 lane should be less than 3.\n", VoDev);
			vo_pr(VO_ERR, "CS(%d) RS(%d) WR(%d) RD(%d).\n",
				     psti80Cfg->lane_s.CS, psti80Cfg->lane_s.RS,
				     psti80Cfg->lane_s.WR, psti80Cfg->lane_s.RD);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if ((psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RS) || (psti80Cfg->lane_s.CS == psti80Cfg->lane_s.WR) ||
		    (psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RD) || (psti80Cfg->lane_s.RS == psti80Cfg->lane_s.WR) ||
		    (psti80Cfg->lane_s.CS == psti80Cfg->lane_s.RD) || (psti80Cfg->lane_s.WR == psti80Cfg->lane_s.RD)) {
			vo_pr(VO_ERR, "VO DEV(%d) I80 lane can't duplicate CS(%d) RS(%d) WR(%d) RD(%d).\n",
				     VoDev, psti80Cfg->lane_s.CS, psti80Cfg->lane_s.RS,
				     psti80Cfg->lane_s.WR, psti80Cfg->lane_s.RD);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if (psti80Cfg->cycle_time > 250) {
			vo_pr(VO_ERR, "VO DEV(%d) cycle time %d > 250.\n",
				     VoDev, psti80Cfg->cycle_time);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
		if (psti80Cfg->fmt >= VO_I80_FORMAT_MAX) {
			vo_pr(VO_ERR, "VO DEV(%d) invalid I80 Format(%d).\n",
				     VoDev, psti80Cfg->fmt);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}
#if 0//I80

		cfg.intf_type = CVI_VIP_DISP_INTF_I80;
		if (vo_set_interface(gvdev, &cfg) != 0) {
			vo_pr(VO_ERR, "VO INTF configure failured.\n");
			//return CVI_FAILURE;
		}

		i80_ctrl[I80_CTRL_CMD] = BIT(psti80Cfg->lane_s.RD) |
					((BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.WR)) << 4);
		i80_ctrl[I80_CTRL_DATA] = (BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.RS)) |
			((BIT(psti80Cfg->lane_s.RD) | BIT(psti80Cfg->lane_s.WR) | BIT(psti80Cfg->lane_s.RS)) << 4);
		i80_ctrl[I80_CTRL_EOF] = 0xff;

		vo_pr(VO_ERR, "VO I80 ctrl CMD(%#x) DATA(%#x)\n",
			     i80_ctrl[I80_CTRL_CMD], i80_ctrl[I80_CTRL_DATA]);



		d = get_dev_info(VDEV_TYPE_DISP, 0);
		if (vo_set_clk(d->fd, 1000000 / (psti80Cfg->cycle_time / 2)) != 0) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO I80 update cycle_time(%d) fail\n", psti80Cfg->cycle_time);
			return CVI_FAILURE;
		}
#endif
	} else if (pstPubAttr->enIntfType == VO_INTF_HW_MCU) {
		const VO_HW_MCU_CFG_S *McuCfg = &pstPubAttr->stMcuCfg;
		if (McuCfg->mode >= VO_MCU_MODE_MAX) {
			vo_pr(VO_ERR, "VO DEV(%d) invalid MCU Format(%d).\n",
				     VoDev, McuCfg->mode);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}

		cfg.intf_type = CVI_VIP_DISP_INTF_HW_MCU;
		cfg.mcu_cfg.mode = (enum MCU_MODE)McuCfg->mode;
		cfg.mcu_cfg.lcd_power_gpio_num = McuCfg->lcd_power_gpio_num;
		cfg.mcu_cfg.lcd_power_avtive = McuCfg->lcd_power_avtive;
		cfg.mcu_cfg.backlight_gpio_num = McuCfg->backlight_gpio_num;
		cfg.mcu_cfg.backlight_avtive = McuCfg->backlight_avtive;
		cfg.mcu_cfg.reset_gpio_num = McuCfg->reset_gpio_num;
		cfg.mcu_cfg.reset_avtive = McuCfg->reset_avtive;
		cfg.mcu_cfg.pixelclock = div_u64(dv_timings.bt.pixelclock, 1000);
		memcpy(&cfg.mcu_cfg.pins, &pstPubAttr->stMcuCfg.pins, sizeof(struct vo_pins));
		memcpy(&cfg.mcu_cfg.instrs, &pstPubAttr->stMcuCfg.instrs, sizeof(struct VO_MCU_INSTRS));
		if (vo_set_interface(gvdev, &cfg) != 0) {
			vo_pr(VO_ERR, "VO INTF configure failured.\n");
			return CVI_FAILURE;
		}
	} else if (pstPubAttr->enIntfType == VO_INTF_BT656) {
		cfg.intf_type = CVI_VIP_DISP_INTF_BT;
		cfg.bt_cfg.mode = BT_MODE_656;
		cfg.bt_cfg.pixelclock = div_u64(dv_timings.bt.pixelclock, 1000);
		memcpy(&cfg.bt_cfg.pins, &pstPubAttr->stBtAttr.pins, sizeof(struct vo_pins));

		if (vo_set_interface(gvdev, &cfg) != 0) {
			vo_pr(VO_ERR, "VO BT656 configure failured.\n");
			return CVI_FAILURE;
		}
	} else if (pstPubAttr->enIntfType == VO_INTF_BT1120) {
		cfg.intf_type = CVI_VIP_DISP_INTF_BT;
		cfg.bt_cfg.mode = BT_MODE_1120;
		cfg.bt_cfg.pixelclock = div_u64(dv_timings.bt.pixelclock, 1000);
		memcpy(&cfg.bt_cfg.pins, &pstPubAttr->stBtAttr.pins, sizeof(struct vo_pins));

		if (vo_set_interface(gvdev, &cfg) != 0) {
			vo_pr(VO_ERR, "VO BT1120 configure failured.\n");
			return CVI_FAILURE;
		}
	} else {
		vo_pr(VO_ERR, "VO invalid INTF type(0x%x)\n", pstPubAttr->enIntfType);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	vo_get_panelstatus(0, 0, &panel_status);

	vo_pr(VO_INFO, "panel_status[%d], intf_type[%d]\n", panel_status, cfg.intf_type);
	if ((cfg.intf_type != CVI_VIP_DISP_INTF_DSI) && !panel_status) {
		struct sclr_disp_timing timing;

		vo_fill_disp_timing(&timing, &dv_timings.bt);
		sclr_disp_set_timing(&timing);
	}

	rgb[2] = pstPubAttr->u32BgColor & 0x3ff;
	rgb[1] = (pstPubAttr->u32BgColor >> 10) & 0x3ff;
	rgb[0] = (pstPubAttr->u32BgColor >> 20) & 0x3ff;

	sclr_disp_set_frame_bgcolor(rgb[0], rgb[1], rgb[2]);

	memcpy(&gVoCtx->stPubAttr, pstPubAttr, sizeof(*pstPubAttr));

	return CVI_SUCCESS;
}
CVI_S32 vo_get_pub_attr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	enum sclr_vo_sel vo_sel;
	struct sclr_disp_timing *timing = sclr_disp_get_timing();
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstPubAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	//vo_get_videolayer_size(d->fd, &vsize);
	gVoCtx->stPubAttr.stSyncInfo.u16Hact = timing->hfde_end - timing->hfde_start + 1;
	gVoCtx->stPubAttr.stSyncInfo.u16Vact = timing->vfde_end - timing->vfde_start + 1;

	//vo_get_intf_type(d->fd, (CVI_S32 *)&vo_sel);
	vo_sel = sclr_disp_mux_get();

	switch (vo_sel) {
	case SCLR_VO_SEL_I80:
		gVoCtx->stPubAttr.enIntfType = VO_INTF_I80;
		break;

	case SCLR_VO_SEL_HW_MCU:
		gVoCtx->stPubAttr.enIntfType = VO_INTF_HW_MCU;
		break;

	case SCLR_VO_SEL_BT656:
		gVoCtx->stPubAttr.enIntfType = VO_INTF_BT656;
		break;

	case SCLR_VO_SEL_BT1120:
		gVoCtx->stPubAttr.enIntfType = VO_INTF_BT1120;
		break;

	default:
		gVoCtx->stPubAttr.enIntfType = VO_INTF_MIPI;
		break;
	}

	memcpy(pstPubAttr, &gVoCtx->stPubAttr, sizeof(VO_PUB_ATTR_S));

	return CVI_SUCCESS;

}

CVI_S32 vo_set_chn_attr(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr)
{
	struct sclr_rect area;
	u8 i = 0;
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		vo_pr(VO_INFO, "VoLayer(%d) VoChn(%d) already enabled.\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}
	if ((pstChnAttr->stRect.u32Width < VO_MIN_CHN_WIDTH) || (pstChnAttr->stRect.u32Height < VO_MIN_CHN_HEIGHT)
	 || (pstChnAttr->stRect.u32Width + pstChnAttr->stRect.s32X > gVoCtx->stLayerAttr.stImageSize.u32Width)
	 || (pstChnAttr->stRect.u32Height + pstChnAttr->stRect.s32Y > gVoCtx->stLayerAttr.stImageSize.u32Height)) {
		vo_pr(VO_INFO, "VoLayer(%d) VoChn(%d) rect(%d %d %d %d) invalid.\n"
			, VoLayer, VoChn, pstChnAttr->stRect.s32X, pstChnAttr->stRect.s32Y
			, pstChnAttr->stRect.u32Width, pstChnAttr->stRect.u32Height);
		return CVI_FAILURE_ILLEGAL_PARAM;
	}

	area.x = pstChnAttr->stRect.s32X;
	area.y = pstChnAttr->stRect.s32Y;
	area.w = pstChnAttr->stRect.u32Width;
	area.h = pstChnAttr->stRect.u32Height;

	vo_pr(VO_INFO, "Compose Area (%d,%d,%d,%d)\n", area.x, area.y, area.w, area.h);
	if (sclr_disp_set_rect(area) == 0) {
		gvdev->compose_out.left = area.x;
		gvdev->compose_out.top = area.y;
		gvdev->compose_out.width = area.w;
		gvdev->compose_out.height = area.h;
	}

	memcpy(&gVoCtx->stChnAttr, pstChnAttr, sizeof(*pstChnAttr));

	for (i = 0; i < RGN_MAX_NUM_VO; ++i)
		gVoCtx->rgn_handle[i] = RGN_INVALID_HANDLE;
	for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
		gVoCtx->rgn_coverEx_handle[i] = RGN_INVALID_HANDLE;

	return CVI_SUCCESS;
}

CVI_S32 vo_enable(VO_DEV VoDev)
{
	CVI_S32 ret = CVI_FAILURE;
	struct base_exe_m_cb exe_cb;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->stPubAttr.enIntfType == 0) {
		vo_pr(VO_ERR, "VO DEV(%d) isn't correctly configured.\n", VoDev);
		//return CVI_ERR_VO_DEV_NOT_CONFIG;
	}

	if (gVoCtx->is_dev_enable[VoDev]) {
		vo_pr(VO_ERR, "VO DEV(%d) should be disabled.\n", VoDev);
		//return CVI_ERR_VO_DEV_HAS_ENABLED;
	}

	gVoCtx->is_dev_enable[VoDev] = CVI_TRUE;

	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = VPSS_CB_SET_FB_ON_VPSS;
	exe_cb.data   = (void *)&gVoCtx->fb_on_vpss;
	if (base_exe_module_cb(&exe_cb))
		vo_pr(VO_DBG, "set fb_on_vpss(%d) failed!\n", gVoCtx->fb_on_vpss);

	return CVI_SUCCESS;

}
CVI_S32 vo_disable(VO_DEV VoDev)
{
	u8 VoLayer = 0;
	CVI_S32 ret = CVI_FAILURE;
	struct base_exe_m_cb exe_cb;
	CVI_BOOL fb_on_vpss = false;

	ret = CHECK_VO_DEV_VALID(VoDev);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!gVoCtx->is_dev_enable[VoDev]) {
		vo_pr(VO_ERR, "VO_DEV(%d) already disabled.\n", VoDev);
		return CVI_SUCCESS;
	}
	for (VoLayer = 0; VoLayer < VO_MAX_LAYER_NUM; ++VoLayer) {
		if (gVoCtx->is_layer_enable[VoLayer]) {
			vo_pr(VO_ERR, "VoLayer(%d) isn't disabled yet.\n", VoLayer);
			return CVI_FAILURE;
		}
	}

	gVoCtx->is_dev_enable[VoDev] = CVI_FALSE;
	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = VPSS_CB_SET_FB_ON_VPSS;
	exe_cb.data   = (void *)&fb_on_vpss;
	if (base_exe_module_cb(&exe_cb))
		vo_pr(VO_DBG, "set fb_on_vpss(%d) failed!\n", fb_on_vpss);

	return CVI_SUCCESS;

}

CVI_S32 vo_enable_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	u8 create_thread = true;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_ENABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) already enabled.\n", VoLayer, VoChn);
		return CVI_ERR_VO_CHN_NOT_DISABLED;
	}

	base_mod_jobs_init(chn, CHN_TYPE_OUT, gVoCtx->u32DisBufLen - 1, 2, 0);

	gVoCtx->is_chn_enable[VoLayer][VoChn] = CVI_TRUE;

	if (vo_start_streaming(gvdev)) {
		vo_pr(VO_ERR, "Failed to vo start streaming\n");
		return -EAGAIN;
	}

	atomic_set(&gvdev->disp_streamon, 1);

	if (create_thread) {
		ret = vo_create_thread(gvdev, E_VO_TH_DISP);
		if (ret) {
			vo_pr(VO_ERR, "Failed to create E_VO_TH_DISP thread\n");
		}
	}

	return ret;
}

static int vo_hide_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{

	enum sclr_disp_pat_type type = SCL_PAT_TYPE_FULL;
	enum  sclr_disp_pat_color color = SCL_PAT_COLOR_USR;
	u16 rgb[3] = {0, 0, 0};
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	sclr_disp_set_pattern(type, color, rgb);

	gVoCtx->show = CVI_FALSE;

	return CVI_SUCCESS;
}

static int vo_pause_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	CHECK_VO_CHN_VALID(VoLayer, VoChn);

	gVoCtx->pause = CVI_TRUE;

	return CVI_SUCCESS;
}

static int vo_resume_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (gVoCtx->pause) {
		gVoCtx->pause = CVI_FALSE;
	}
	return CVI_SUCCESS;
}

static int vo_show_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	enum sclr_disp_pat_type type = SCL_PAT_TYPE_OFF;
	enum  sclr_disp_pat_color color = SCL_PAT_COLOR_MAX;
	u16 rgb[3] = {0, 0, 0};
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	gVoCtx->show = CVI_TRUE;

	sclr_disp_set_pattern(type, color, rgb);

	return CVI_SUCCESS;
}
static int vo_set_displaybuflen(VO_LAYER VoLayer, CVI_U32 u32BufLen)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	if (u32BufLen < 3) {
		vo_pr(VO_ERR, "VoLayer(%d) u32BufLen(%d) low than min(3).\n", VoLayer, u32BufLen);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	gVoCtx->u32DisBufLen = u32BufLen;

	return CVI_SUCCESS;
}

static int vo_get_displaybuflen(VO_LAYER VoLayer, CVI_U32 *pu32BufLen)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	*pu32BufLen = gVoCtx->u32DisBufLen;
	return CVI_SUCCESS;
}

int vo_get_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, penRotation);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(penRotation, &gVoCtx->enRotation, sizeof(*penRotation));

	return CVI_SUCCESS;
}
struct vo_fmt *vo_sdk_get_format(u32 pixelformat)
{
	struct vo_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(vo_sdk_formats); k++) {
		fmt = &vo_sdk_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

static void _vo_sdk_fill_disp_cfg(struct sclr_disp_cfg *cfg,
		struct vo_video_format_mplane *mp)
{
	struct vo_fmt *fmt;
	struct vo_video_plane_pix_format *pfmt = mp->plane_fmt;

	fmt = vo_sdk_get_format(mp->pixelformat);

	cfg->fmt = fmt->fmt;

	if (mp->colorspace == VO_COLORSPACE_SRGB)
		cfg->in_csc = SCL_CSC_NONE;
	else if (mp->colorspace == VO_COLORSPACE_SMPTE170M)
		cfg->in_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	else
		cfg->in_csc = SCL_CSC_709_LIMIT_YUV2RGB;

	vo_pr(VO_DBG, "bytesperline 0(%d))\n", pfmt[0].bytesperline);
	vo_pr(VO_DBG, "bytesperline 1(%d))\n", pfmt[1].bytesperline);
	cfg->mem.pitch_y = pfmt[0].bytesperline;
	cfg->mem.pitch_c = pfmt[1].bytesperline;

	vo_pr(VO_DBG, " width(%d), heigh(%d)\n", mp->width, mp->height);
	cfg->mem.width = mp->width;
	cfg->mem.height = mp->height;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;
}


int _vo_sdk_setfmt(CVI_S32 width, CVI_S32 height, CVI_U32 pxlfmt)
{
	int p = 0, i = 0;
	u8 align = 0;
	struct vo_video_format_mplane *mp;
	struct vo_video_format fmt;
	const struct vo_fmt *_vo_fmt;
	struct sclr_disp_cfg *cfg;
	unsigned int bytesperline;

	memset(&fmt, 0, sizeof(struct vo_video_format));

	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = pxlfmt;
	fmt.fmt.pix_mp.field = 0;

	if (align < VIP_ALIGNMENT)
		align = VIP_ALIGNMENT;

	switch (pxlfmt) {
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:
		fmt.fmt.pix_mp.colorspace = VO_COLORSPACE_SMPTE170M;
		break;
	default:
#if 0
	case V4L2_PIX_FMT_HSVM:
	case V4L2_PIX_FMT_HSV24:
	case V4L2_PIX_FMT_RGBM:
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
		fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
#endif
		break;
	}
	switch (pxlfmt) {
	default:
	case PIXEL_FORMAT_HSV_888_PLANAR:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
#if 0
	case V4L2_PIX_FMT_HSVM:
	case V4L2_PIX_FMT_RGBM:
#endif
		fmt.fmt.pix_mp.num_planes = 3;
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		fmt.fmt.pix_mp.num_planes = 2;
		break;
#if 0
	case V4L2_PIX_FMT_GREY:
	case V4L2_PIX_FMT_HSV24:
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
#endif
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_VYUY:

		fmt.fmt.pix_mp.num_planes = 1;
		break;
	}
	gvdev->numOfPlanes = fmt.fmt.pix_mp.num_planes;

	_vo_fmt = vo_sdk_get_format(pxlfmt);
	mp = &fmt.fmt.pix_mp;

	for (p = 0; p < mp->num_planes; p++) {
		u8 plane_sub_v = (p == 0) ? 1 : _vo_fmt->plane_sub_v;
		/* Calculate the minimum supported bytesperline value */
		bytesperline = ALIGN((mp->width * _vo_fmt->bit_depth[p]) >> 3, align);

		if (fmt.fmt.pix_mp.plane_fmt[p].bytesperline < bytesperline)
			fmt.fmt.pix_mp.plane_fmt[p].bytesperline = bytesperline;

		fmt.fmt.pix_mp.plane_fmt[p].sizeimage = fmt.fmt.pix_mp.plane_fmt[p].bytesperline
		* mp->height / plane_sub_v;

		vo_pr(VO_DBG, "plane-%d: bytesperline(%d) sizeimage(%x)\n", p,
			fmt.fmt.pix_mp.plane_fmt[p].bytesperline, fmt.fmt.pix_mp.plane_fmt[p].sizeimage);
		memset(fmt.fmt.pix_mp.plane_fmt[p].reserved, 0, sizeof(fmt.fmt.pix_mp.plane_fmt[p].reserved));
	}

	gvdev->colorspace = mp->colorspace;
	for (i = 0; i < mp->num_planes; i++) {
		gvdev->bytesperline[i] = mp->plane_fmt[i].bytesperline;
		gvdev->sizeimage[i] = mp->plane_fmt[i].sizeimage;
	}

	cfg = sclr_disp_get_cfg();
	_vo_sdk_fill_disp_cfg(cfg, mp);
	sclr_disp_set_cfg(cfg);

	return fmt.fmt.pix.sizeimage;

}

static int vo_set_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E enRotation)
{
	CVI_S32 ret = CVI_FAILURE;
	struct vo_rect area;
	struct sclr_disp_cfg *cfg;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!GDC_SUPPORT_FMT(gVoCtx->stLayerAttr.enPixFormat)) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) invalid PixFormat(%d).\n"
			, VoLayer, VoChn, gVoCtx->stLayerAttr.enPixFormat);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (enRotation >= ROTATION_MAX) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) invalid rotation(%d).\n", VoLayer, VoChn, enRotation);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	} else if (enRotation == ROTATION_0) {
		gVoCtx->enRotation = enRotation;

		return CVI_SUCCESS;
	}

	// TODO: dummy settings
	gVoCtx->mesh.paddr = DEFAULT_MESH_PADDR;

	area.width = gVoCtx->stChnAttr.stRect.u32Width;
	area.height = gVoCtx->stChnAttr.stRect.u32Height;
	area.left = (enRotation == ROTATION_90) ?
			 ALIGN(area.width, LDC_ALIGN) - area.width : 0;
	area.top = 0;
	gvdev->align = (enRotation == ROTATION_0) ? DEFAULT_ALIGN : LDC_ALIGN;

	_vo_sdk_setfmt(gVoCtx->stLayerAttr.stImageSize.u32Width, gVoCtx->stLayerAttr.stImageSize.u32Height,
			gVoCtx->stLayerAttr.enPixFormat);

	//vo_set_tgt_crop(d->fd, &area);
	cfg = sclr_disp_get_cfg();
	cfg->mem.start_x = area.left;
	cfg->mem.start_y = area.top;
	cfg->mem.width	 = area.width;
	cfg->mem.height  = area.height;

	vo_pr(VO_INFO, "Crop Area (%d,%d,%d,%d)\n", cfg->mem.start_x, cfg->mem.start_y,
						cfg->mem.width, cfg->mem.height);
	sclr_disp_set_mem(&cfg->mem);
	gvdev->crop_rect = area;

	gVoCtx->enRotation = enRotation;

	return CVI_SUCCESS;
}

CVI_S32 vo_resume(void)
{
	VO_CHN VoChn = 0;
	VO_LAYER VoLayer = 0;
	CVI_S32 ret = CVI_FAILURE;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};

	vo_pr(VO_DBG, "vo resume DispBufLen[%d]\n", gVoCtx->u32DisBufLen);

	if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		base_mod_jobs_init(chn, CHN_TYPE_OUT, gVoCtx->u32DisBufLen - 1, 2, 0);

		ret = vo_start_streaming(gvdev);
		if (ret) {
			vo_pr(VO_ERR, "Failed to vo start streaming\n");
			return -EAGAIN;
		}

		ret = vo_create_thread(gvdev, E_VO_TH_DISP);
		if (ret) {
			vo_pr(VO_ERR, "Failed to create E_VO_TH_DISP thread\n");
		}

		vo_set_chnrotation(VoLayer, VoChn, gVoCtx->enRotation);
	}
	return ret;
}

CVI_S32 vo_suspend(void)
{
	VO_CHN VoChn = 0;
	VO_LAYER VoLayer = 0;
	CVI_S32 ret = CVI_FAILURE;
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};

	if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		gVoCtx->is_chn_enable[VoLayer][VoChn] = CVI_FALSE;
		ret = vo_stop_streaming(gvdev);
		if (ret) {
			vo_pr(VO_ERR, "Failed to vo stop streaming\n");
			return -EAGAIN;
		}

		ret = vo_destroy_thread(gvdev, E_VO_TH_DISP);
		if (ret) {
			vo_pr(VO_ERR, "Failed to vo destory thread\n");
			return -EAGAIN;
		}

		gVoCtx->is_chn_enable[VoLayer][VoChn] = CVI_TRUE;

		base_mod_jobs_exit(chn, CHN_TYPE_OUT);
	}

	return ret;
}

static int vo_enablevideolayer(VO_LAYER VoLayer)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	sclr_disp_enable_window_bgcolor(0);

	gVoCtx->is_layer_enable[VoLayer] = CVI_TRUE;

	return CVI_SUCCESS;
}

int vo_disablevideolayer(VO_LAYER VoLayer)
{
	int VoChn = 0;
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!gVoCtx->is_layer_enable[VoLayer]) {
		vo_pr(VO_ERR, "VoLayer(%d) isn't enabled yet.\n", VoLayer);
		return CVI_SUCCESS;
	}
	for (VoChn = 0; VoChn < VO_MAX_CHN_NUM; ++VoChn) {
		if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
			vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) isn't disabled yet.\n", VoLayer, VoChn);
			return CVI_FAILURE;
		}
	}

	sclr_disp_enable_window_bgcolor(1);

	gVoCtx->is_layer_enable[VoLayer] = CVI_FALSE;

	return CVI_SUCCESS;
}

static int vo_get_videolayerattr(VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstLayerAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstLayerAttr, &gVoCtx->stLayerAttr, sizeof(*pstLayerAttr));

	return CVI_SUCCESS;
}

static int vo_set_videolayerattr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	struct sclr_rect rect;
	CVI_U16 rgb[3] = {0, 0, 0};
	CVI_S32 ret = CVI_FAILURE;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, pstLayerAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_LAYER_DISABLE(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!VO_SUPPORT_FMT(pstLayerAttr->enPixFormat)) {
		vo_pr(VO_DBG, "VoLayer(%d) enPixFormat(%d) unsupported\n"
			, VoLayer, pstLayerAttr->enPixFormat);
		//return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (gVoCtx->stPubAttr.enIntfType == VO_INTF_I80)
		if ((pstLayerAttr->enPixFormat != PIXEL_FORMAT_RGB_888)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_BGR_888)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_RGB_888_PLANAR)
		 && (pstLayerAttr->enPixFormat != PIXEL_FORMAT_BGR_888_PLANAR)) {
			vo_pr(VO_ERR, "I80 only accept RGB/BGR pixel format.\n");
			//return CVI_ERR_VO_ILLEGAL_PARAM;
		}

	if ((pstLayerAttr->stImageSize.u32Width != pstLayerAttr->stDispRect.u32Width)
	 || (pstLayerAttr->stImageSize.u32Height != pstLayerAttr->stDispRect.u32Height)) {
		vo_pr(VO_ERR, "VoLayer(%d) stImageSize(%d %d) stDispRect(%d %d) isn't the same.\n"
			, VoLayer, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height
			, pstLayerAttr->stDispRect.u32Width, pstLayerAttr->stDispRect.u32Height);
		//return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstLayerAttr->stImageSize.u32Width < VO_MIN_CHN_WIDTH)
	 || (pstLayerAttr->stImageSize.u32Height < VO_MIN_CHN_HEIGHT)) {
		vo_pr(VO_ERR, "VoLayer(%d) Size(%d %d) too small.\n"
			, VoLayer, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height);
		//return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	if (gVoCtx->stPubAttr.enIntfType != VO_INTF_I80) {
		//format = v4l2_remap_pxlfmt(pstLayerAttr->enPixFormat);
		//v4l2_setfmt(d, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height,
		//		d->type, format);

		_vo_sdk_setfmt(pstLayerAttr->stImageSize.u32Width,
					pstLayerAttr->stImageSize.u32Height, pstLayerAttr->enPixFormat);
	}

#if 0
	d = get_dev_info(VDEV_TYPE_DISP, 0);

	if (vo_ctx.stPubAttr.enIntfType != VO_INTF_I80) {
		format = v4l2_remap_pxlfmt(pstLayerAttr->enPixFormat);
		v4l2_setfmt(d, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height,
				d->type, format);
	} else {
		CVI_U8 byte_cnt = (vo_ctx.stPubAttr.sti80Cfg.fmt == VO_I80_FORMAT_RGB666) ? 3 : 2;

		format = V4L2_PIX_FMT_BGR24;
		v4l2_setfmt(d, pstLayerAttr->stImageSize.u32Width * byte_cnt + 1,
				pstLayerAttr->stImageSize.u32Height,
				d->type, format);
	}
	//TODO
	//init_disp_device();


	if (vo_set_window_bgcolor(d->fd, rgb) != 0)
		CVI_TRACE_VO(CVI_DBG_ERR, "VO SET_BG_COLOR failed - %s.\n", strerror(errno));
#endif

	gVoCtx->bVideoFrameValid = CVI_FALSE;
	sclr_disp_set_pattern(patterns[CVI_VIP_PAT_BLACK].type, patterns[CVI_VIP_PAT_BLACK].color, rgb);
	sclr_disp_set_window_bgcolor(rgb[0], rgb[1], rgb[2]);

	rect.w = pstLayerAttr->stDispRect.u32Width;
	rect.h = pstLayerAttr->stDispRect.u32Height;
	rect.x = pstLayerAttr->stDispRect.s32X;
	rect.y = pstLayerAttr->stDispRect.s32Y;

	//vo_set_tgt_compose(d->fd, &area);
	sclr_disp_set_rect(rect);

	vo_pr(VO_DBG, "VoLayer(%d) image-size(%d * %d) disp-rect(%d-%d-%d-%d).\n", VoLayer
		, pstLayerAttr->stImageSize.u32Width, pstLayerAttr->stImageSize.u32Height
		, pstLayerAttr->stDispRect.s32X, pstLayerAttr->stDispRect.s32Y
		, pstLayerAttr->stDispRect.u32Width, pstLayerAttr->stDispRect.u32Height);

	memcpy(&gVoCtx->stLayerAttr, pstLayerAttr, sizeof(*pstLayerAttr));

	return CVI_SUCCESS;
}


int vo_disable_chn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VO, .s32DevId = 0, .s32ChnId = 0};
	CVI_S32 ret = CVI_FAILURE;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) already disabled.\n", VoLayer, VoChn);
		return CVI_SUCCESS;
	}

	gVoCtx->is_chn_enable[VoLayer][VoChn] = CVI_FALSE;

	if (vo_stop_streaming(gvdev)) {
		vo_pr(VO_ERR, "Failed to vo stop streaming\n");
		return -EAGAIN;
	}

	vo_destroy_thread(gvdev, E_VO_TH_DISP);

	base_mod_jobs_exit(chn, CHN_TYPE_OUT);

	if (gVoCtx->mesh.paddr) {
		if (gVoCtx->mesh.paddr != DEFAULT_MESH_PADDR) {
			sys_ion_free(gVoCtx->mesh.paddr);
		}
		gVoCtx->mesh.paddr = 0;
		gVoCtx->mesh.vaddr = 0;
	}

	gVoCtx->chnStatus[VoLayer][VoChn].u32frameCnt = 0;
	gVoCtx->chnStatus[VoLayer][VoChn].u64PrevTime = 0;
	gVoCtx->chnStatus[VoLayer][VoChn].u32RealFrameRate = 0;

	atomic_set(&gvdev->disp_streamon, 0);

	return CVI_SUCCESS;
}

/*****************************************************************************
 *  SDK layer ioctl operations for vi.c
 ****************************************************************************/
long vo_sdk_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p)
{
	u32 id = p->sdk_id;
	long rc = -EINVAL;

	gvdev = vdev;

	switch (id) {
	case VO_SDK_SET_CHNATTR:
	{
		struct vo_chn_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_set_chn_attr(cfg->VoLayer, cfg->VoChn, &cfg->pstChnAttr);

		break;
	}
	case VO_SDK_GET_CHNATTR:
	{
		struct vo_chn_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_chn_attr(cfg->VoLayer, cfg->VoChn, &cfg->pstChnAttr);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_chn_attr_cfg))) {
			vo_pr(VO_ERR, "copy_to_user failed.\n");
			rc = -1;
		}

		break;
	}
	case VO_SDK_GET_PUBATTR:
	{
		struct vo_pub_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_pub_attr(cfg->VoDev, &cfg->pstPubAttr);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_pub_attr_cfg))) {
			vo_pr(VO_ERR, "copy_to_user failed.\n");
			rc = -1;
		}

		break;
	}

	case VO_SDK_SET_PUBATTR:
	{
		struct vo_pub_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_pub_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_set_pub_attr(cfg->VoDev, &cfg->pstPubAttr);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_pub_attr_cfg))) {
			break;
		}

		break;
	}

	case VO_SDK_SUSPEND:
	{
		rc = vo_suspend();

		break;
	}

	case VO_SDK_RESUME:
	{
		rc = vo_resume();

		break;
	}

	case VO_SDK_GET_PANELSTATUE:
	{
		struct vo_panel_status_cfg *cfg, _cfg_;

		cfg = &_cfg_;
		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_panel_status_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_panelstatus(cfg->VoLayer, cfg->VoChn, &cfg->is_init);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_chn_cfg)))
			break;

		break;
	}
	case VO_SDK_ENABLE_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_enable_chn(cfg->VoLayer, cfg->VoChn);

		break;
	}
	case VO_SDK_DISABLE_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_disable_chn(cfg->VoLayer, cfg->VoChn);

		break;
	}
	case VO_SDK_ENABLE:
	{
		struct vo_dev_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_enable(cfg->VoDev);

		break;
	}

	case VO_SDK_DISABLE:
	{
		struct vo_dev_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_dev_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_disable(cfg->VoDev);

		break;
	}

	case VO_SDK_SEND_FRAME:
	{
		struct vo_snd_frm_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_snd_frm_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		rc = vo_send_frame(cfg->VoLayer, cfg->VoChn, &cfg->stVideoFrame, cfg->s32MilliSec);
		break;
	}

	case VO_SDK_CLEAR_CHNBUF:
	{
		struct vo_clear_chn_buf_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_clear_chn_buf_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		rc = vo_clear_chnbuf(cfg->VoLayer, cfg->VoChn, cfg->bClrAll);
		break;
	}

	case VO_SDK_SET_DISPLAYBUFLEN:
	{
		struct vo_display_buflen_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_set_displaybuflen(cfg->VoLayer, cfg->u32BufLen);

		break;
	}

	case VO_SDK_GET_DISPLAYBUFLEN:
	{
		struct vo_display_buflen_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_display_buflen_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_displaybuflen(cfg->VoLayer, &cfg->u32BufLen);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_display_buflen_cfg))) {
			vo_pr(VO_ERR, "copy_to_user failed.\n");
			rc = -1;
		}
		break;
	}

	case VO_SDK_GET_CHNROTATION:
	{
		struct vo_chn_rotation_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_chnrotation(cfg->VoLayer, cfg->VoChn, &cfg->enRotation);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_display_buflen_cfg))) {
			vo_pr(VO_ERR, "copy_to_user failed.\n");
			rc = -1;
		}
		break;
	}

	case VO_SDK_SET_CHNROTATION:
	{
		struct vo_chn_rotation_cfg *cfg, _cfg_;

		cfg = &_cfg_;
		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_rotation_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_set_chnrotation(cfg->VoLayer, cfg->VoChn, cfg->enRotation);

		break;
	}

	case VO_SDK_SET_VIDEOLAYERATTR:
	{
		struct vo_video_layer_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_set_videolayerattr(cfg->VoLayer, &cfg->pstLayerAttr);

		break;
	}

	case VO_SDK_GET_VIDEOLAYERATTR:
	{
		struct vo_video_layer_attr_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_video_layer_attr_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_get_videolayerattr(cfg->VoLayer, &cfg->pstLayerAttr);

		if (copy_to_user(p->ptr, cfg, sizeof(struct vo_video_layer_attr_cfg))) {
			vo_pr(VO_ERR, "copy_to_user failed.\n");
			rc = -1;

		}

		break;
	}

	case VO_SDK_ENABLE_VIDEOLAYER:
	{
		struct vo_video_layer_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_enablevideolayer(cfg->VoLayer);

		break;
	}

	case VO_SDK_DISABLE_VIDEOLAYER:
	{
		struct vo_video_layer_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_video_layer_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_disablevideolayer(cfg->VoLayer);

		break;
	}

	case VO_SDK_SHOW_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_show_chn(cfg->VoLayer, cfg->VoChn);
		break;
	}
	case VO_SDK_HIDE_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_hide_chn(cfg->VoLayer, cfg->VoChn);
		break;
	}
	case VO_SDK_PAUSE_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_pause_chn(cfg->VoLayer, cfg->VoChn);

		break;
	}

	case VO_SDK_RESUME_CHN:
	{
		struct vo_chn_cfg *cfg, _cfg_;

		cfg = &_cfg_;

		if (copy_from_user(cfg, p->ptr, sizeof(struct vo_chn_cfg))) {
			vo_pr(VO_ERR, "copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = vo_resume_chn(cfg->VoLayer, cfg->VoChn);

		break;
	}

	default:
		break;
	}

	return rc;
}
