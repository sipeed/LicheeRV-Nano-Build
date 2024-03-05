#ifndef _VPSS_GRP_HW_CFG_H_
#define _VPSS_GRP_HW_CFG_H_

#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_vip_sc.h>
#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_region.h>

struct VPSS_CHN_HW_CFG {
	struct cvi_vpss_rect rect_crop;
	struct cvi_vpss_rect rect_output;
	struct cvi_sc_border_param border_param;
	struct cvi_sc_quant_param quantCfg;
	struct cvi_rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct cvi_rgn_coverex_cfg rgn_coverex_cfg;
	struct cvi_rgn_mosaic_cfg rgn_mosaic_cfg;
	enum cvi_sc_scaling_coef coef;
	struct cvi_csc_cfg csc_cfg;
	CVI_U32 bytesperline[2];
};

struct VPSS_GRP_HW_CFG {
	struct VPSS_CHN_HW_CFG stChnCfgs[VPSS_MAX_CHN_NUM];

	struct cvi_csc_cfg csc_cfg;
	struct cvi_vpss_rect rect_crop;
	CVI_U32 bytesperline[2];
};

#endif /* _VPSS_GRP_HW_H_ */
