#ifndef __IMX327_CMOS_EX_H_
#define __IMX327_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <cif_uapi.h>
#include <vi_snsr.h>
#include <cvi_type.h>
#include "cvi_sns_ctrl.h"
#include "sensor.h"

enum imx327_linear_regs_e {
	LINEAR_HOLD = 0,
	LINEAR_SHS1_0,
	LINEAR_SHS1_1,
	LINEAR_SHS1_2,
	LINEAR_GAIN,
	LINEAR_HCG,
	LINEAR_VMAX_0,
	LINEAR_VMAX_1,
	LINEAR_VMAX_2,
	LINEAR_REL,
	LINEAR_REGS_NUM
};

enum imx327_dol2_regs_e {
	DOL2_HOLD = 0,
	DOL2_SHS1_0,
	DOL2_SHS1_1,
	DOL2_SHS1_2,
	DOL2_GAIN,
	DOL2_HCG,
	DOL2_GAIN1,
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

typedef enum _IMX327_MODE_E {
	IMX327_MODE_1080P30 = 0,
	IMX327_MODE_1080P60,
	IMX327_MODE_720P30,
	IMX327_MODE_LINEAR_NUM,
	IMX327_MODE_1080P30_HDR = IMX327_MODE_LINEAR_NUM,
	IMX327_MODE_720P30_HDR,
	IMX327_MODE_NUM
} IMX327_MODE_E;

typedef struct _IMX327_STATE_S {
	CVI_U8       u8Hcg;
	CVI_U32      u32BRL;
	CVI_U32      u32RHS1;
	CVI_U32      u32RHS1_MAX;
} IMX327_STATE_S;

typedef struct _IMX327_MODE_S {
	ISP_HDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U16 u16RHS1;
	CVI_U16 u16BRL;
	CVI_U16 u16OpbSize;
	CVI_U16 u16MarginVtop;
	CVI_U16 u16MarginVbot;
	char name[64];
} IMX327_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/
#define IMX327_SLAVE_ID		0x1A
#define IMX327_ADDR_LEN		2
#define IMX327_DATA_LEN		1
#define IMX327_CMD_LEN		(IMX327_ADDR_LEN + IMX327_DATA_LEN)

#define IMX327_CHIP_ID_ADDR	0x31dc
#define IMX327_CHIP_ID		0x6
#define IMX327_CHIP_ID_MASK	0x6

extern ISP_SNS_STATE_S *g_pastImx327[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx327_BusInfo[];
extern CVI_U16 g_au16Imx327_GainMode[];
extern const CVI_U8 imx327_i2c_addr;
extern void imx327_init(VI_PIPE ViPipe);
extern void imx327_exit(VI_PIPE ViPipe);
extern void imx327_standby(VI_PIPE ViPipe);
extern void imx327_restart(VI_PIPE ViPipe);
// extern int  imx327_write_register(VI_PIPE ViPipe, int addr, int data);
// extern int  imx327_read_register(VI_PIPE ViPipe, int addr);
extern void imx327_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  imx327_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX327_CMOS_EX_H_ */
