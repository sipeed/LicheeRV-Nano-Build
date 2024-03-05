#ifndef __MCS369Q_CMOS_EX_H_
#define __MCS369Q_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

typedef enum _MCS369Q_MODE_E {
	MCS369Q_MODE_1440P30,
	MCS369Q_MODE_NUM
} MCS369Q_MODE_E;

typedef struct _MCS369Q_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U8 u8DgainReg;
	char name[64];
} MCS369Q_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastMCS369Q[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunMCS369Q_BusInfo[];
extern const CVI_U8 mcs369q_i2c_addr;
extern const CVI_U32 mcs369q_addr_byte;
extern const CVI_U32 mcs369q_data_byte;
extern void mcs369q_init(VI_PIPE ViPipe);
extern void mcs369q_exit(VI_PIPE ViPipe);
extern void mcs369q_standby(VI_PIPE ViPipe);
extern void mcs369q_restart(VI_PIPE ViPipe);
extern int  mcs369q_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  mcs369q_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __MCS369Q_CMOS_EX_H_ */
