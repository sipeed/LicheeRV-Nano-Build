#ifndef __TP2850_CMOS_EX_H_
#define __TP2850_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

typedef enum _TP2850_MODE_E {
	TP2850_MODE_1080P_30P,
	TP2850_MODE_1440P_30P,
	TP2850_MODE_NUM
} TP2850_MODE_E;

typedef struct _TP2850_MODE_S {
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
} TP2850_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastTP2850[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunTP2850_BusInfo[];
extern const CVI_U8 tp2850_i2c_addr;
extern const CVI_U32 tp2850_addr_byte;
extern const CVI_U32 tp2850_data_byte;
extern void tp2850_init(VI_PIPE ViPipe);
extern void tp2850_exit(VI_PIPE ViPipe);
extern void tp2850_standby(VI_PIPE ViPipe);
extern void tp2850_restart(VI_PIPE ViPipe);
extern int  tp2850_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  tp2850_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __TP2850_CMOS_EX_H_ */
