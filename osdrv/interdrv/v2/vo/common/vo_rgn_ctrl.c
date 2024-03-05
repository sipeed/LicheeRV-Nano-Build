#include <linux/cvi_base.h>
#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>

#include <vo.h>
#include <vo_cb.h>

static CVI_S32 _check_vo_status(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = CHECK_VO_CHN_VALID(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = CHECK_VO_CHN_ENABLE(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;
	return ret;
}

extern struct cvi_vo_ctx *gVoCtx;

CVI_S32 vo_cb_get_rgn_hdls(VO_LAYER VoLayer, VO_CHN VoChn,
	RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	CVI_S32 ret, i;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	//mutex_lock(&gVoCtx[VpssGrp].lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			hdls[i] = gVoCtx->rgn_handle[i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			hdls[i] = gVoCtx->rgn_coverEx_handle[i];
	} else {
		ret = CVI_ERR_VO_NOT_SUPPORT;
	}
	//mutex_unlock(&vpssCtx[VpssGrp].lock);

	return ret;
}

CVI_S32 vo_cb_set_rgn_hdls(VO_LAYER VoLayer, VO_CHN VoChn,
	RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	CVI_S32 ret, i;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	//mutex_lock(&vpssCtx[VpssGrp].lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			gVoCtx->rgn_handle[i] = hdls[i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			gVoCtx->rgn_coverEx_handle[i] = hdls[i];
	} else {
		ret = CVI_ERR_VO_NOT_SUPPORT;
	}
	//vpssCtx[VpssGrp].is_cfg_changed = CVI_TRUE;
	//mutex_unlock(&vpssCtx[VpssGrp].lock);

	return ret;
}

CVI_S32 vo_cb_set_rgn_cfg(VO_LAYER VoLayer, VO_CHN VoChn, struct cvi_rgn_cfg *cfg)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	//mutex_lock(&vpssCtx[VpssGrp].lock);
	memcpy(&gVoCtx->rgn_cfg, cfg, sizeof(*cfg));
	//vpssCtx[VpssGrp].is_cfg_changed = CVI_TRUE;
	//mutex_unlock(&vpssCtx[VpssGrp].lock);

	return CVI_SUCCESS;
}

CVI_S32 vo_cb_set_rgn_coverex_cfg(VO_LAYER VoLayer, VO_CHN VoChn, struct cvi_rgn_coverex_cfg *cfg)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	//mutex_lock(&vpssCtx[VpssGrp].lock);
	memcpy(&gVoCtx->rgn_coverex_cfg, cfg, sizeof(*cfg));
	//mutex_unlock(&vpssCtx[VpssGrp].lock);

	return CVI_SUCCESS;
}

CVI_S32 vo_cb_get_chn_size(VO_LAYER VoLayer, VO_CHN VoChn, RECT_S *rect)
{
	CVI_S32 ret;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VO, rect);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer, VoChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(rect, &gVoCtx->stChnAttr.stRect, sizeof(*rect));

	return CVI_SUCCESS;
}
