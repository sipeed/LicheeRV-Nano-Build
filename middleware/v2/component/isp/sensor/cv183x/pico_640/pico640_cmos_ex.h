#ifndef __PICO640_CMOS_EX_H_
#define __PICO640_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

typedef enum _PICO640_MODE_E {
	PICO640_MODE_480P30,
	PICO640_MODE_NUM
} PICO640_MODE_E;


typedef struct _PICO640_MODE_S {
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
} PICO640_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastPICO640[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunPICO640_BusInfo[];
extern const CVI_U8 pico640_i2c_addr;
extern const CVI_U32 pico640_addr_byte;
extern const CVI_U32 pico640_data_byte;
extern void pico640_init(VI_PIPE ViPipe);
extern void pico640_exit(VI_PIPE ViPipe);
extern void pico640_standby(VI_PIPE ViPipe);
extern void pico640_restart(VI_PIPE ViPipe);
extern int  pico640_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  pico640_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __PICO640_CMOS_EX_H_ */
