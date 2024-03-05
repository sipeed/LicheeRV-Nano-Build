#ifndef __SC8238_CMOS_EX_H_
#define __SC8238_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum sc8238_linear_regs_e {
	LINEAR_SHS1_0_ADDR,
	LINEAR_SHS1_1_ADDR,
	LINEAR_SHS1_2_ADDR,
	LINEAR_AGAIN_0_ADDR,
	LINEAR_AGAIN_1_ADDR,
	LINEAR_DGAIN_0_ADDR,
	LINEAR_DGAIN_1_ADDR,
	LINEAR_VMAX_0_ADDR,
	LINEAR_VMAX_1_ADDR,
	LINEAR_REGS_NUM
};

enum sc8238_dol2_regs_e {
	WDR2_SHS1_0_ADDR,
	WDR2_SHS1_1_ADDR,
	WDR2_SHS1_2_ADDR,
	WDR2_SHS2_0_ADDR,
	WDR2_SHS2_1_ADDR,
	WDR2_AGAIN1_0_ADDR,
	WDR2_AGAIN1_1_ADDR,
	WDR2_DGAIN1_0_ADDR,
	WDR2_DGAIN1_1_ADDR,
	WDR2_AGAIN2_0_ADDR,
	WDR2_AGAIN2_1_ADDR,
	WDR2_DGAIN2_0_ADDR,
	WDR2_DGAIN2_1_ADDR,
	WDR2_VMAX_0_ADDR,
	WDR2_VMAX_1_ADDR,
	WDR2_MAXSEXP_0_ADDR,
	WDR2_MAXSEXP_1_ADDR,
	WDR2_REGS_NUM
};

typedef enum _SC8238_MODE_E {
	SC8238_MODE_2160P30 = 0,
	SC8238_MODE_LINEAR_NUM,
	SC8238_MODE_2160P15_WDR = SC8238_MODE_LINEAR_NUM,
	SC8238_MODE_NUM
} SC8238_MODE_E;

typedef struct _SC8238_STATE_S {
	CVI_U32		u32Sexp_MAX;	/* (2*{16’h3e23,16’h3e24} – 'd10)/2 */
} SC8238_STATE_S;

typedef struct _SC8238_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U16 u16SexpMaxReg;		/* {16’h3e23,16’h3e24} */
	char name[64];
} SC8238_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSC8238[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSC8238_BusInfo[];
extern CVI_U16 g_au16SC8238_GainMode[];
extern CVI_U16 g_au16SC8238_L2SMode[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeSC8238_MirrorFip[];
extern const CVI_U8 sc8238_i2c_addr;
extern const CVI_U32 sc8238_addr_byte;
extern const CVI_U32 sc8238_data_byte;
extern void sc8238_init(VI_PIPE ViPipe);
extern void sc8238_exit(VI_PIPE ViPipe);
extern void sc8238_standby(VI_PIPE ViPipe);
extern void sc8238_restart(VI_PIPE ViPipe);
extern int  sc8238_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc8238_read_register(VI_PIPE ViPipe, int addr);
extern void sc8238_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC8238_CMOS_EX_H_ */
