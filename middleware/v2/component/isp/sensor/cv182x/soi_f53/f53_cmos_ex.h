#ifndef __F53_CMOS_EX_H_
#define __F53_CMOS_EX_H_

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


enum f53_linear_regs_e {
	LINEAR_SHS1_0_DATA,
	LINEAR_SHS1_1_DATA,
	LINEAR_AGAIN_DATA,
	LINEAR_VMAX_0_DATA,
	LINEAR_VMAX_1_DATA,
	LINEAR_REGS_NUM
};

typedef enum _F53_MODE_E {
	F53_MODE_1080P30 = 0,
	F53_MODE_LINEAR_NUM,
	F53_MODE_1080P30_WDR = F53_MODE_LINEAR_NUM,
	F53_MODE_NUM
} F53_MODE_E;

typedef struct _F53_STATE_S {
	CVI_U32		u8SexpReg;
	CVI_U32		u32Sexp_MAX;
} F53_STATE_S;

typedef struct _F53_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U8 u8DgainReg;
	CVI_U32 u32L2S_MAX;
	char name[64];
} F53_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastF53[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunF53_BusInfo[];
extern CVI_U16 g_au16F53_GainMode[];
extern CVI_U16 g_au16F53_L2SMode[];
extern CVI_U8 f53_i2c_addr;
extern const CVI_U32 f53_addr_byte;
extern const CVI_U32 f53_data_byte;
extern void f53_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern void f53_init(VI_PIPE ViPipe);
extern void f53_exit(VI_PIPE ViPipe);
extern void f53_standby(VI_PIPE ViPipe);
extern void f53_restart(VI_PIPE ViPipe);
extern int  f53_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  f53_read_register(VI_PIPE ViPipe, int addr);
extern int  f53_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __F53_CMOS_EX_H_ */
