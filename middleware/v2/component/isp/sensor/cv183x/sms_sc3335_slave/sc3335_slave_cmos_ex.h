#ifndef __SC3335_SLAVE_CMOS_EX_H_
#define __SC3335_SLAVE_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum sc3335_slave_linear_regs_e {
	LINEAR_EXP_H_ADDR,
	LINEAR_EXP_M_ADDR,
	LINEAR_EXP_L_ADDR,
	LINEAR_AGAIN_H_ADDR,
	LINEAR_AGAIN_L_ADDR,
	LINEAR_DGAIN_H_ADDR,
	LINEAR_DGAIN_L_ADDR,
	LINEAR_VMAX_H_ADDR,
	LINEAR_VMAX_L_ADDR,
	LINEAR_GAIN_DPC_ADDR,
	LINEAR_HOLD,
	LINEAR_GAIN_LOGIC_H_ADDR,
	LINEAR_GAIN_LOGIC_L_ADDR,
	LINEAR_REL,
	LINEAR_REGS_NUM
};

typedef enum _SC3335_SLAVE_MODE_E {
	SC3335_SLAVE_MODE_2304X1296P30 = 0,
	SC3335_SLAVE_MODE_LINEAR_NUM,
	SC3335_SLAVE_MODE_2304X1296P30_WDR = SC3335_SLAVE_MODE_LINEAR_NUM,
	SC3335_SLAVE_MODE_NUM
} SC3335_SLAVE_MODE_E;

typedef struct _SC3335_SLAVE_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	char name[64];
} SC3335_SLAVE_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSC3335_SLAVE[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSC3335_SLAVE_BusInfo[];
extern CVI_U16 g_au16SC3335_SLAVE_GainMode[];
extern CVI_U16 g_au16SC3335_SLAVE_L2SMode[];
extern const CVI_U8 sc3335_slave_i2c_addr;
extern const CVI_U32 sc3335_slave_addr_byte;
extern const CVI_U32 sc3335_slave_data_byte;
extern void sc3335_slave_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern void sc3335_slave_init(VI_PIPE ViPipe);
extern void sc3335_slave_exit(VI_PIPE ViPipe);
extern void sc3335_slave_standby(VI_PIPE ViPipe);
extern void sc3335_slave_restart(VI_PIPE ViPipe);
extern int  sc3335_slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc3335_slave_read_register(VI_PIPE ViPipe, int addr);
extern int  sc3335_slave_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC3335_SLAVE_CMOS_EX_H_ */
