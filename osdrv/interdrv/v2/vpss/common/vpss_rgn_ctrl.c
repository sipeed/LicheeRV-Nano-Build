#include <linux/clk.h>

#include <linux/cvi_base.h>
#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vpss_ctx.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>

#include <vpss_cb.h>
#include "vpss_debug.h"
#include "vpss.h"
#include "vpss_core.h"
#include "vpss_grp_hw_cfg.h"
#include "scaler.h"


CVI_S32 vpss_get_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer,
			RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	CVI_S32 ret, i;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].coverEx_handle[i];
	} else if (enType == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].mosaic_handle[i];
	} else {
		ret = CVI_ERR_VPSS_NOT_SUPPORT;
	}

	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return ret;
}

CVI_S32 vpss_set_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer,
			RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	CVI_S32 ret, i;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i] = hdls[i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].coverEx_handle[i] = hdls[i];
	} else if (enType == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].mosaic_handle[i] = hdls[i];
	} else {
		ret = CVI_ERR_VPSS_NOT_SUPPORT;
	}
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return ret;
}

CVI_S32 vpss_set_rgn_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer, struct cvi_rgn_cfg *cfg)
{
	CVI_S32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();
	struct VPSS_GRP_HW_CFG *vpss_hw_cfgs;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	vpss_hw_cfgs = (struct VPSS_GRP_HW_CFG *)pVpssCtx[VpssGrp]->hw_cfgs;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&vpss_hw_cfgs->stChnCfgs[VpssChn].rgn_cfg[layer], cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_rgn_coverex_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_coverex_cfg *cfg)
{
	CVI_S32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();
	struct VPSS_GRP_HW_CFG *vpss_hw_cfgs;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	vpss_hw_cfgs = (struct VPSS_GRP_HW_CFG *)pVpssCtx[VpssGrp]->hw_cfgs;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&vpss_hw_cfgs->stChnCfgs[VpssChn].rgn_coverex_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

CVI_S32 vpss_set_rgn_mosaic_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_mosaic_cfg *cfg)
{
	CVI_S32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();
	struct VPSS_GRP_HW_CFG *vpss_hw_cfgs;

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	vpss_hw_cfgs = (struct VPSS_GRP_HW_CFG *)pVpssCtx[VpssGrp]->hw_cfgs;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&vpss_hw_cfgs->stChnCfgs[VpssChn].rgn_mosaic_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

CVI_S32 vpss_get_rgn_ow_addr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer, RGN_HANDLE handle, CVI_U64 *addr)
{
	CVI_S32 ret, dev_idx, i;
	CVI_U8 ow_inst;
	MMF_CHN_S stChn;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();

	ret = MOD_CHECK_NULL_PTR(CVI_ID_VPSS, addr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	stChn.enModId = CVI_ID_VPSS;
	stChn.s32DevId = VpssGrp;
	stChn.s32ChnId = VpssChn;
	dev_idx = get_dev_info_by_chn(stChn, CHN_TYPE_OUT);

	for (i = 0; i < RGN_MAX_NUM_VPSS; ++i) {
		if (pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i] == handle) {
			ow_inst = i;
			break;
		}
	}
	if (i == RGN_MAX_NUM_VPSS) {
		mutex_unlock(&pVpssCtx[VpssGrp]->lock);
		return CVI_FAILURE;
	}

	sclr_gop_ow_get_addr(dev_idx, layer, ow_inst, addr);
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}
