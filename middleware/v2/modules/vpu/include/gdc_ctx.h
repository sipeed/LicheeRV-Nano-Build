#ifndef MODULES_VPU_INCLUDE_GDC_CTX_H_
#define MODULES_VPU_INCLUDE_GDC_CTX_H_

#include <linux/cvi_vi_ctx.h>
#include "cvi_base.h"

extern struct cvi_gdc_mesh mesh[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];
extern struct cvi_gdc_mesh g_vi_mesh[VI_MAX_CHN_NUM];
extern struct cvi_vi_ctx *gViCtx;

#endif /* MODULES_VPU_INCLUDE_VPSS_CTX_H_ */
