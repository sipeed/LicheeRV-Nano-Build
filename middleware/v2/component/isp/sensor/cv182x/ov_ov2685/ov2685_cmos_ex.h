#ifndef __OV2685_CMOS_EX_H_
#define __OV2685_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef ARCH_CV182X
#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#else
#include <linux/cif_uapi.h>
#include <linux/vi_snsr.h>
#include <linux/cvi_type.h>
#endif
#include "cvi_sns_ctrl.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

enum ov2685_linear_regs_e {
	LINEAR_EXP_0,
	LINEAR_EXP_1,
	LINEAR_EXP_2,
	LINEAR_AGAIN_0,
	LINEAR_AGAIN_1,
	LINEAR_VTS_0,
	LINEAR_VTS_1,
	LINEAR_REGS_NUM
};

typedef enum _OV2685_MODE_E {
	OV2685_MODE_1600X1200P30 = 0,
	OV2685_MODE_LINEAR_NUM,
	OV2685_MODE_NUM
} OV2685_MODE_E;

typedef struct _OV2685_STATE_S {
	CVI_U32		u32Sexp_MAX;
} OV2685_STATE_S;

typedef struct _OV2685_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
} OV2685_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastOv2685[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunOv2685_BusInfo[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeOv2685_MirrorFip[VI_MAX_PIPE_NUM];
extern CVI_U8 ov2685_i2c_addr;
extern const CVI_U32 ov2685_addr_byte;
extern const CVI_U32 ov2685_data_byte;
extern void ov2685_init(VI_PIPE ViPipe);
extern void ov2685_exit(VI_PIPE ViPipe);
extern void ov2685_standby(VI_PIPE ViPipe);
extern void ov2685_restart(VI_PIPE ViPipe);
extern int  ov2685_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  ov2685_read_register(VI_PIPE ViPipe, int addr);
extern void ov2685_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  ov2685_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __OV2685_CMOS_EX_H_ */

