#ifndef __OS02D10_CMOS_EX_H_
#define __OS02D10_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum os02d10_linear_regs_e {
	LINEAR_PAGE_1,
	LINEAR_EXP_0,
	LINEAR_EXP_1,
	LINEAR_AGAIN_0,
	LINEAR_DGAIN_0,
	LINEAR_FLIP_MIRROR,
	LINEAR_HB_H,
	LINEAR_HB_L,
	LINEAR_EXP_EN,
	LINEAR_REGS_NUM
};


typedef enum _OS02D10_MODE_E {
	OS02D10_MODE_1920X1080P30 = 0,
	OS02D10_MODE_LINEAR_NUM,
	OS02D10_MODE_NUM
} OS02D10_MODE_E;

typedef struct _OS02D10_STATE_S {
	CVI_U32		u32Sexp_MAX;
} OS02D10_STATE_S;

typedef struct _OS02D10_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	CVI_U16 u16L2sOffset;
	CVI_U16 u16TopBoundary;
	CVI_U16 u16BotBoundary;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	CVI_U32 u32L2S_offset;
	CVI_U32 u32IspResTime;
	CVI_U32 u32HdrMargin;
	char name[64];
} OS02D10_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastOs02d10[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunOs02d10_BusInfo[];
extern CVI_U16 g_au16Os02d10_GainMode[];
extern CVI_U16 g_au16Os02d10_UseHwSync[VI_MAX_PIPE_NUM];
extern CVI_U8 os02d10_i2c_addr;
extern const CVI_U32 os02d10_addr_byte;
extern const CVI_U32 os02d10_data_byte;
extern void os02d10_init(VI_PIPE ViPipe);
extern void os02d10_exit(VI_PIPE ViPipe);
extern void os02d10_standby(VI_PIPE ViPipe);
extern void os02d10_restart(VI_PIPE ViPipe);
extern int  os02d10_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  os02d10_read_register(VI_PIPE ViPipe, int addr);
extern void os02d10_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int os02d10_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __OS02D10_CMOS_EX_H_ */
