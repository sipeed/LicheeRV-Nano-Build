#ifndef __IMX385_CMOS_EX_H_
#define __IMX385_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum imx385_linear_regs_e {
	LINEAR_HOLD = 0,
	LINEAR_SHS1_0,
	LINEAR_SHS1_1,
	LINEAR_SHS1_2,
	LINEAR_GAIN_0,
	LINEAR_GAIN_1,
	LINEAR_HCG,
	LINEAR_VMAX_0,
	LINEAR_VMAX_1,
	LINEAR_VMAX_2,
	LINEAR_REL,
	LINEAR_REGS_NUM
};

enum imx385_dol2_regs_e {
	DOL2_HOLD = 0,
	DOL2_SHS1_0,
	DOL2_SHS1_1,
	DOL2_SHS1_2,
	DOL2_GAIN_0,
	DOL2_GAIN_1,
	DOL2_HCG,
	DOL2_GAIN1_0,
	DOL2_GAIN1_1,
	DOL2_RHS1_0,
	DOL2_RHS1_1,
	DOL2_RHS1_2,
	DOL2_SHS2_0,
	DOL2_SHS2_1,
	DOL2_SHS2_2,
	DOL2_VMAX_0,
	DOL2_VMAX_1,
	DOL2_VMAX_2,
	DOL2_YOUT_SIZE_0,
	DOL2_YOUT_SIZE_1,
	DOL2_REL,
	DOL2_REGS_NUM
};

typedef enum _IMX385_MODE_E {
	IMX385_MODE_1080P30 = 0,
	IMX385_MODE_LINEAR_NUM,
	IMX385_MODE_1080P30_WDR = IMX385_MODE_LINEAR_NUM,
	IMX385_MODE_NUM
} IMX385_MODE_E;

typedef struct _IMX385_STATE_S {
	CVI_U8       u8Hcg;
	CVI_U32      u32BRL;
	CVI_U32      u32RHS1;
	CVI_U32      u32RHS1_MAX;
} IMX385_STATE_S;

typedef struct _IMX385_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_LARGE_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	CVI_U32 u32RHS1;
	CVI_U32 u32BRL;
	CVI_U32 u32OpbSize;
	CVI_U32 u32MarginVtop;
	CVI_U32 u32MarginVbot;
	char name[64];
} IMX385_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastImx385[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx385_BusInfo[];
extern CVI_U16 g_au16Imx385_GainMode[];
extern const CVI_U8 imx385_i2c_addr;
extern const CVI_U32 imx385_addr_byte;
extern const CVI_U32 imx385_data_byte;
extern void imx385_init(VI_PIPE ViPipe);
extern void imx385_exit(VI_PIPE ViPipe);
extern void imx385_standby(VI_PIPE ViPipe);
extern void imx385_restart(VI_PIPE ViPipe);
extern int  imx385_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  imx385_read_register(VI_PIPE ViPipe, int addr);
extern void imx385_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX385_CMOS_EX_H_ */
