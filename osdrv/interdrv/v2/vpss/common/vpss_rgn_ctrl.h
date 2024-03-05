#ifndef _VPSS_RGN_CTRL_H_
#define _VPSS_RGN_CTRL_H_

CVI_S32 vpss_get_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U8 layer,
	RGN_TYPE_E enType, RGN_HANDLE hdls[]);
CVI_S32 vpss_set_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer,
		RGN_TYPE_E enType, RGN_HANDLE hdls[]);
CVI_S32 vpss_set_rgn_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer, struct cvi_rgn_cfg *cfg);
CVI_S32 vpss_set_rgn_coverex_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_coverex_cfg *cfg);
CVI_S32 vpss_set_rgn_mosaic_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_mosaic_cfg *cfg);
CVI_S32 vpss_get_rgn_ow_addr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, CVI_U32 layer,
	RGN_HANDLE handle, CVI_U64 *addr);

#endif /* _VPSS_RGN_CTRL_H_ */
