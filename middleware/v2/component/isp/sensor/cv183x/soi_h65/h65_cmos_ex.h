#ifndef __H65_CMOS_EX_H_
#define __H65_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

enum h65_linear_regs_e {
	LINEAR_SHS1_0_ADDR,
	LINEAR_SHS1_0_DATA,
	LINEAR_SHS1_1_ADDR,
	LINEAR_SHS1_1_DATA,
	LINEAR_AGAIN_ADDR,
	LINEAR_AGAIN_DATA,
	LINEAR_DGAIN_ADDR,
	LINEAR_DGAIN_DATA,
	LINEAR_VMAX_0_ADDR,
	LINEAR_VMAX_0_DATA,
	LINEAR_VMAX_1_ADDR,
	LINEAR_VMAX_1_DATA,
	LINEAR_REL,
	LINEAR_REGS_NUM
};

enum h65_dol2_regs_e {
	WDR2_SHS1_0_ADDR,
	WDR2_SHS1_0_DATA,
	WDR2_SHS1_1_ADDR,
	WDR2_SHS1_1_DATA,
	WDR2_SHS2_ADDR,
	WDR2_SHS2_DATA,
	WDR2_AGAIN_ADDR,
	WDR2_AGAIN_DATA,
	WDR2_DGAIN_ADDR,
	WDR2_DGAIN_DATA,
	WDR2_VMAX_0_ADDR,
	WDR2_VMAX_0_DATA,
	WDR2_VMAX_1_ADDR,
	WDR2_VMAX_1_DATA,
	WDR2_L2S_ADDR,
	WDR2_L2S_DATA,
	WDR2_REL,
	WDR2_REGS_NUM
};

typedef enum _H65_MODE_E {
	H65_MODE_720P30 = 0,
	H65_MODE_LINEAR_NUM,
	H65_MODE_720P30_WDR = H65_MODE_LINEAR_NUM,
	H65_MODE_NUM
} H65_MODE_E;

typedef struct _H65_STATE_S {
	CVI_U32		u8SexpReg;
	CVI_U32		u32Sexp_MAX;
} H65_STATE_S;

typedef struct _H65_MODE_S {
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
} H65_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastH65[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunH65_BusInfo[];
extern CVI_U16 g_au16H65_GainMode[];
extern CVI_U16 g_au16H65_L2SMode[];
extern  CVI_U8 h65_i2c_addr;
extern const CVI_U32 h65_addr_byte;
extern const CVI_U32 h65_data_byte;
extern void h65_init(VI_PIPE ViPipe);
extern void h65_exit(VI_PIPE ViPipe);
extern void h65_standby(VI_PIPE ViPipe);
extern void h65_restart(VI_PIPE ViPipe);
extern int  h65_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  h65_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __H65_CMOS_EX_H_ */
