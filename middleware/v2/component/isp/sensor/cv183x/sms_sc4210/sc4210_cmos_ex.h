#ifndef __SC4210_CMOS_EX_H_
#define __SC4210_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"


enum sc4210_linear_regs_e {
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

enum sc4210_dol2_regs_e {
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

typedef enum _SC4210_MODE_E {
	SC4210_MODE_1440P30 = 0,
	SC4210_MODE_LINEAR_NUM,
	SC4210_MODE_1440P30_WDR = SC4210_MODE_LINEAR_NUM,
	SC4210_MODE_NUM
} SC4210_MODE_E;

typedef struct _SC4210_STATE_S {
	CVI_U32		u32Sexp_MAX;	/* (2*{16’h3e23,16’h3e24} – 'd10)/2 */
} SC4210_STATE_S;

typedef struct _SC4210_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U16 u16SexpMax;
	char name[64];
} SC4210_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSC4210[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSC4210_BusInfo[];
extern CVI_U16 g_au16SC4210_GainMode[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeSC8238_MirrorFip[];
extern CVI_U8 sc4210_i2c_addr;
extern const CVI_U32 sc4210_addr_byte;
extern const CVI_U32 sc4210_data_byte;
extern void sc4210_init(VI_PIPE ViPipe);
extern void sc4210_exit(VI_PIPE ViPipe);
extern void sc4210_standby(VI_PIPE ViPipe);
extern void sc4210_restart(VI_PIPE ViPipe);
extern void sc4210_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  sc4210_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc4210_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC4210_CMOS_EX_H_ */
