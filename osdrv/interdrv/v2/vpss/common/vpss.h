#ifndef _VPSS_H_
#define _VPSS_H_

#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_region.h>
#include <linux/cvi_errno.h>
#include <linux/vpss_uapi.h>
#include <linux/cvi_vpss_ctx.h>

#include <base_ctx.h>
#include <base_cb.h>
#include <dwa_cb.h>
#include "vpss_debug.h"
#include "vpss_grp_hw_cfg.h"

/* Configured from user, IOCTL */
CVI_S32 vpss_create_grp(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);
CVI_S32 vpss_destroy_grp(VPSS_GRP VpssGrp);
VPSS_GRP vpss_get_available_grp(void);

CVI_S32 vpss_start_grp(VPSS_GRP VpssGrp);
CVI_S32 vpss_stop_grp(VPSS_GRP VpssGrp);

CVI_S32 vpss_reset_grp(VPSS_GRP VpssGrp);

CVI_S32 vpss_set_grp_attr(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);
CVI_S32 vpss_get_grp_attr(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstGrpAttr);

CVI_S32 vpss_set_grp_crop(VPSS_GRP VpssGrp, const VPSS_CROP_INFO_S *pstCropInfo);
CVI_S32 vpss_get_grp_crop(VPSS_GRP VpssGrp, VPSS_CROP_INFO_S *pstCropInfo);

CVI_S32 vpss_get_grp_frame(VPSS_GRP VpssGrp, VIDEO_FRAME_INFO_S *pstVideoFrame);
CVI_S32 vpss_release_grp_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame);

CVI_S32 vpss_send_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec);
CVI_S32 vpss_send_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn
	, const VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec);

CVI_S32 vpss_set_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_ATTR_S *pstChnAttr);
CVI_S32 vpss_get_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_ATTR_S *pstChnAttr);

CVI_S32 vpss_enable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
CVI_S32 vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

CVI_S32 vpss_set_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CROP_INFO_S *pstCropInfo);
CVI_S32 vpss_get_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo);

CVI_S32 vpss_set_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation);
CVI_S32 vpss_get_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E *penRotation);

CVI_S32 vpss_set_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation,
			const VPSS_LDC_ATTR_S *pstLDCAttr, CVI_U64 mesh_addr);
CVI_S32 vpss_get_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr);

CVI_S32 vpss_get_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
			 CVI_S32 s32MilliSec);
CVI_S32 vpss_release_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame);

CVI_S32 vpss_set_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 u32Align);
CVI_S32 vpss_get_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 *pu32Align);

CVI_S32 vpss_set_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 YRatio);
CVI_S32 vpss_get_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 *pYRatio);

CVI_S32 vpss_set_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E enCoef);
CVI_S32 vpss_get_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E *penCoef);

CVI_S32 vpss_show_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
CVI_S32 vpss_hide_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

CVI_S32 vpss_set_chn_bufwrap_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap);
CVI_S32 vpss_get_chn_bufwrap_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_BUF_WRAP_S *pstVpssChnBufWrap);

CVI_S32 vpss_attach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VB_POOL hVbPool);
CVI_S32 vpss_detach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

CVI_S32 vpss_trigger_snap_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 frame_cnt);

/* INTERNAL */
CVI_S32 vpss_set_vivpss_mode(const VI_VPSS_MODE_S *pstVIVPSSMode);
CVI_S32 vpss_set_mode(void *arg, VPSS_MODE_E enVPSSMode);
VPSS_MODE_E vpss_get_mode(void);
CVI_S32 vpss_set_mode_ex(void *arg, const VPSS_MODE_S *pstVPSSMode);
CVI_S32 vpss_get_mode_ex(VPSS_MODE_S *pstVPSSMode);
void vpss_mode_init(void *arg);
CVI_S32 vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg);
CVI_S32 vpss_get_proc_amp_ctrl(PROC_AMP_E type, PROC_AMP_CTRL_S *ctrl);
CVI_S32 vpss_get_proc_amp(VPSS_GRP VpssGrp, CVI_S32 *proc_amp);
CVI_S32 vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg);
CVI_S32 vpss_get_binscene(struct vpss_scene *cfg);

CVI_VOID vpss_set_mlv_info(CVI_U8 snr_num, struct mlv_i_s *p_m_lv_i);
CVI_VOID vpss_get_mlv_info(CVI_U8 snr_num, struct mlv_i_s *p_m_lv_i);

CVI_VOID vpss_set_isp_bypassfrm(CVI_U8 snr_num, CVI_U8 bypass_frm);

int _vpss_call_cb(u32 m_id, u32 cmd_id, void *data);
void vpss_init(void *arg);
void vpss_deinit(void);
void vpss_post_job(CVI_S32 VpssGrp);
CVI_VOID vpss_gdc_callback(CVI_VOID *pParam, VB_BLK blk);

int vpss_set_grp_sbm(struct vpss_grp_sbm_cfg *cfg);
int vpss_set_vc_sbm_flow(struct vpss_vc_sbm_flow_cfg *cfg);

CVI_VOID vpss_notify_wkup_evt(CVI_U8 u8VpssDev);
CVI_VOID vpss_notify_isr_evt(CVI_U8 img_idx);
CVI_VOID vpss_notify_vi_err_evt(CVI_U8 img_idx);
CVI_S32 vpss_set_rgn_lut_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_lut_cfg *cfg);
CVI_S32 get_dev_info_by_chn(MMF_CHN_S chn, enum CHN_TYPE_E chn_type);
CVI_S32 check_vpss_id(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
CVI_S32 vpss_overflow_check(VPSS_GRP VpssGrp, struct cvi_vpss_info *vpss_info, struct cvi_vc_info *vc_info);
CVI_VOID vpss_print_vb_info(CVI_U8 grp_id, CVI_U8 sc_idx);
CVI_S32 vpss_sbm_notify_venc(VPSS_GRP VpssGrp, CVI_U8 sc_idx);
CVI_VOID set_fb_on_vpss(CVI_BOOL is_fb_on_vpss);

//Check GRP and CHN VALID, CREATED and FMT
#define VPSS_GRP_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))

#define VPSS_CHN_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_HSV_888) || (fmt == PIXEL_FORMAT_HSV_888_PLANAR) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY))

#define FRC_INVALID(ctx, VpssChn)	\
	(ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate.s32DstFrameRate <= 0 ||		\
		ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate.s32SrcFrameRate <= 0 ||		\
		ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate.s32DstFrameRate >=		\
		ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate.s32SrcFrameRate)

static inline CVI_S32 MOD_CHECK_NULL_PTR(MOD_ID_E mod, const void *ptr)
{
	if (mod >= CVI_ID_BUTT)
		return CVI_FAILURE;
	if (!ptr) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_VPSS_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VPSS_GRP_VALID(VPSS_GRP grp)
{
	if ((grp >= VPSS_MAX_GRP_NUM) || (grp < 0)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "VpssGrp(%d) exceeds Max(%d)\n", grp, VPSS_MAX_GRP_NUM);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_YUV_PARAM(enum _PIXEL_FORMAT_E fmt, CVI_U32 w, CVI_U32 h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_422 width(%d) should be even.\n", w);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)
		   || (fmt == PIXEL_FORMAT_NV12)
		   || (fmt == PIXEL_FORMAT_NV21)) {
		if (w & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		if (h & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VPSS_GRP_FMT(VPSS_GRP grp, enum _PIXEL_FORMAT_E fmt)
{
	if (!VPSS_GRP_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) enPixelFormat(%d) unsupported\n"
		, grp, fmt);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VPSS_CHN_FMT(VPSS_GRP grp, VPSS_CHN chn, enum _PIXEL_FORMAT_E fmt)
{
	if (!VPSS_CHN_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) enPixelFormat(%d) unsupported\n"
		, grp, chn, fmt);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VPSS_GDC_FMT(VPSS_GRP grp, VPSS_CHN chn, enum _PIXEL_FORMAT_E fmt)
{
	if (!GDC_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for GDC.\n"
		, grp, chn, (fmt));
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

#endif /* _VPSS_H_ */
