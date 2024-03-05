#ifndef __F35_SLAVE_CMOS_EX_H_
#define __F35_SLAVE_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum f35_slave_linear_regs_e {
	LINEAR_SHS1_0_DATA,
	LINEAR_SHS1_1_DATA,
	LINEAR_AGAIN_DATA,
	LINEAR_DGAIN_DATA,
	LINEAR_VMAX_0_DATA,
	LINEAR_VMAX_1_DATA,
	LINEAR_REGS_NUM
};

enum f35_slave_dol2_regs_e {
	WDR2_SHS1_0_DATA,
	WDR2_SHS1_1_DATA,
	WDR2_SHS2_DATA,
	WDR2_AGAIN_DATA,
	WDR2_DGAIN_DATA,
	WDR2_VMAX_0_DATA,
	WDR2_VMAX_1_DATA,
	WDR2_L2S_DATA,
	WDR2_REGS_NUM
};

typedef enum _F35_SLAVE_MODE_E {
	F35_SLAVE_MODE_1080P30 = 0,
	F35_SLAVE_MODE_LINEAR_NUM,
	F35_SLAVE_MODE_1080P30_WDR = F35_SLAVE_MODE_LINEAR_NUM,
	F35_SLAVE_MODE_NUM
} F35_SLAVE_MODE_E;

typedef struct _F35_SLAVE_STATE_S {
	CVI_U32		u8SexpReg;
	CVI_U32		u32Sexp_MAX;
} F35_SLAVE_STATE_S;

typedef struct _F35_SLAVE_MODE_S {
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
} F35_SLAVE_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastF35_Slave[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunF35_Slave_BusInfo[];
extern CVI_U16 g_au16F35_Slave_GainMode[];
extern CVI_U16 g_au16F35_Slave_L2SMode[];
extern const CVI_U8 f35_slave_i2c_addr;
extern const CVI_U32 f35_slave_addr_byte;
extern const CVI_U32 f35_slave_data_byte;
extern void f35_slave_init(VI_PIPE ViPipe);
extern void f35_slave_exit(VI_PIPE ViPipe);
extern void f35_slave_standby(VI_PIPE ViPipe);
extern void f35_slave_restart(VI_PIPE ViPipe);
extern int  f35_slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  f35_slave_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __F35_SLAVE_CMOS_EX_H_ */
