#ifndef __VO_SDK_LAYER_H__
#define __VO_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <vo_defines.h>
#include <linux/vo_uapi.h>
#include <linux/cvi_vo_ctx.h>
#include <linux/types.h>
#include <linux/cvi_errno.h>
#include "scaler.h"
#include <vo_common.h>
/*****************************************************************************
 *  vo structure and enum for vo sdk layer
 ****************************************************************************/


/*****************************************************************************
 *  vo function prototype for vo sdk layer
 ****************************************************************************/
int vo_create_thread(struct cvi_vo_dev *vdev, enum E_VO_TH th_id);
int vo_destroy_thread(struct cvi_vo_dev *vdev, enum E_VO_TH th_id);
int vo_start_streaming(struct cvi_vo_dev *vdev);
int vo_stop_streaming(struct cvi_vo_dev *vdev);
void vo_fill_disp_timing(struct sclr_disp_timing *timing,
		struct vo_bt_timings *bt_timing);
int vo_set_interface(struct cvi_vo_dev *vdev, struct cvi_disp_intf_cfg *cfg);
int vo_disable_chn(VO_LAYER VoLayer, VO_CHN VoChn);
int vo_disablevideolayer(VO_LAYER VoLayer);
int vo_disable(VO_DEV VoDev);
int vo_get_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation);

extern struct cvi_vo_ctx *gVoCtx;

static inline CVI_S32 CHECK_VO_LAYER_DISABLE(VO_LAYER VoLayer)
{
	if (gVoCtx->is_layer_enable[VoLayer]) {
		vo_pr(VO_ERR, "VoLayer(%d) already enabled.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_DISABLED;
	}

	return CVI_SUCCESS;
}


static inline CVI_S32 CHECK_VO_LAYER_VALID(VO_LAYER VoLayer)
{
	if (VoLayer >= VO_MAX_LAYER_NUM) {
		vo_pr(VO_ERR, "VoLayer(%d) invalid.\n", VoLayer);
		return CVI_ERR_VO_INVALID_LAYERID;
	}

	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VO_LAYER_ENABLE(VO_LAYER VoLayer)
{

	if (!gVoCtx->is_layer_enable[VoLayer]) {
		vo_pr(VO_ERR, "VoLayer(%d) not enable.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_ENABLED;
	}

	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VO_DEV_VALID(VO_DEV VoDev)
{
	if ((VoDev >= VO_MAX_DEV_NUM) || (VoDev < 0)) {
		vo_pr(VO_ERR, "VoDev(%d) invalid.\n", VoDev);
		return CVI_ERR_VO_INVALID_DEVID;
	}

	return CVI_SUCCESS;
}
static inline CVI_S32 CHECK_VO_CHN_VALID(VO_LAYER VoLayer, VO_CHN VoChn)
{
	if ((VoLayer >= VO_MAX_LAYER_NUM) || (VoLayer < 0)) {
		vo_pr(VO_ERR, "VoLayer(%d) invalid.\n", VoLayer);
		return CVI_ERR_VO_INVALID_LAYERID;
	}
	if ((VoChn >= VO_MAX_CHN_NUM) || (VoChn < 0)) {
		vo_pr(VO_ERR, "VoChn(%d) invalid.\n", VoChn);
		return CVI_ERR_VO_INVALID_CHNID;
	}

	return CVI_SUCCESS;
}

static inline CVI_S32 CHECK_VO_CHN_ENABLE(VO_LAYER VoLayer, VO_CHN VoChn)
{
	if (!gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		vo_pr(VO_ERR, "VoLayer(%d) VoChn(%d) isn't enabled yet.\n", VoLayer, VoChn);
		return CVI_ERR_VO_CHN_NOT_ENABLED;
	}

	return CVI_SUCCESS;
}

static inline CVI_S32 MOD_CHECK_NULL_PTR(MOD_ID_E mod, const void *ptr)
{
	if (mod >= CVI_ID_BUTT)
		return CVI_FAILURE;

	if (!ptr) {
		vo_pr(VO_ERR, "NULL pointer\n");
		return CVI_ERR_VO_NULL_PTR;
	}
	return CVI_SUCCESS;
}

/*****************************************************************************
 *  vo sdk ioctl function prototype for vi layer
 ****************************************************************************/
long vo_sdk_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p);

/*****************************************************************************
 *  vo sdk function prototype for vo_cb usage
 ****************************************************************************/
CVI_S32 vo_get_chn_attr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr);

#ifdef __cplusplus
}
#endif

#endif //__VO_SDK_LAYER_H__
