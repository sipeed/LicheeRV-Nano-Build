#ifndef __IMX347_CMOS_EX_H_
#define __IMX347_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

enum imx347_linear_regs_e {
	LINEAR_VMAX_L,
	LINEAR_VMAX_M,
	LINEAR_VMAX_H,
	LINEAR_SHR_L,
	LINEAR_SHR_M,
	LINEAR_SHR_H,
	LINEAR_GAIN_L,
	LINEAR_GAIN_H,
	LINEAR_HCG,
	LINEAR_REGS_NUM
};

enum imx347_dol2_regs_e {
	DOL2_VMAX_L,
	DOL2_VMAX_M,
	DOL2_VMAX_H,
	DOL2_SHR0_L,
	DOL2_SHR0_M,
	DOL2_SHR0_H,
	DOL2_RHS1_L,
	DOL2_RHS1_M,
	DOL2_RHS1_H,
	DOL2_SHR1_L,
	DOL2_SHR1_M,
	DOL2_SHR1_H,
	DOL2_GAIN_L,
	DOL2_GAIN_H,
	DOL2_GAIN_SHORT_L,
	DOL2_GAIN_SHORT_H,
	DOL2_REGS_NUM
};

typedef enum _IMX347_MODE_E {
	IMX347_MODE_4M60,
	IMX347_MODE_LINEAR_NUM,
	IMX347_MODE_4M30_WDR = IMX347_MODE_LINEAR_NUM,
	IMX347_MODE_NUM
} IMX347_MODE_E;

typedef struct _IMX347_STATE_S {
	CVI_U8       u8Hcg;
	CVI_U32      u32BRL;
	CVI_U32      u32SHR1;
	CVI_U32      u32RHS1;
	CVI_U32      u32RHS1_MAX;
} IMX347_STATE_S;

typedef struct _IMX347_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	CVI_U16 u16RHS1;
	CVI_U16 u16BRL;
	CVI_U16 u16OpbSize;
	CVI_U16 u16MarginVtop;
	CVI_U16 u16MarginVbot;
	char name[64];
} IMX347_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/
#define IMX347_ADDR_LEN	2
#define IMX347_DATA_LEN	1

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

extern ISP_SNS_STATE_S *g_pastImx347[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx347_BusInfo[];
extern CVI_U16 g_au16Imx347_GainMode[];
extern const CVI_U8 imx347_i2c_addr;
extern void imx347_init(VI_PIPE ViPipe);
extern void imx347_exit(VI_PIPE ViPipe);
extern void imx347_standby(VI_PIPE ViPipe);
extern void imx347_restart(VI_PIPE ViPipe);
extern int  imx347_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  imx347_read_register(VI_PIPE ViPipe, int addr);
extern void imx347_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __IMX347_CMOS_EX_H_ */
