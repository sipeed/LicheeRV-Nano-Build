#ifndef __GC2093_SLAVE_CMOS_EX_H_
#define __GC2093_SLAVE_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

enum gc2093_slave_linear_regs_e {
	LINEAR_EXP_H = 0,   //03
	LINEAR_EXP_L,       //04
	LINEAR_AGAIN_L,     //b3
	LINEAR_COL_AGAIN_H, //b8
	LINEAR_COL_AGAIN_L, //b9
	LINEAR_AGAIN_MAG1,  //0x155
	LINEAR_AGAIN_HOLD,  //0x031d
	LINEAR_AGAIN_MAG2,  //0xc2
	LINEAR_AGAIN_MAG3,  //0xcf
	LINEAR_AGAIN_MAG4,  //0xd9
	LINEAR_AGAIN_REL,   //0x031d
	LINEAR_DGAIN_H,     //b1
	LINEAR_DGAIN_L,     //b2
	LINEAR_VTS_H,       //0x41  (frame length)
	LINEAR_VTS_L,       //0x42
	LINEAR_REGS_NUM
};

enum gc2093_slave_wdr_regs_e {
	WDR_SEXP_H = 0,  //01
	WDR_SEXP_L,      //02
	WDR_LEXP_H,      //03
	WDR_LEXP_L,      //04
	WDR_AGAIN_L,     //b3
	WDR_COL_AGAIN_H, //b8
	WDR_COL_AGAIN_L, //b9
	WDR_AGAIN_MAG1,  //0x155
	WDR_AGAIN_HOLD,  //0x031d
	WDR_AGAIN_MAG2,  //0xc2
	WDR_AGAIN_MAG3,  //0xcf
	WDR_AGAIN_MAG4,  //0xd9
	WDR_AGAIN_REL,   //0x031d
	WDR_DGAIN_H,     //b1
	WDR_DGAIN_L,     //b2
	WDR_VTS_H,       //0x41  (frame length)
	WDR_VTS_L,       //0x42
	WDR_REGS_NUM
};
typedef enum _GC2093_SLAVE_MODE_E {
	GC2093_SLAVE_MODE_1920X1080P30 = 0,
	GC2093_SLAVE_MODE_LINEAR_NUM,
	GC2093_SLAVE_MODE_1920X1080P30_WDR = GC2093_SLAVE_MODE_LINEAR_NUM,
	GC2093_SLAVE_MODE_NUM
} GC2093_SLAVE_MODE_E;

typedef struct _GC2093_SLAVE_STATE_S {
	CVI_U32		u32Sexp_MAX;
} GC2093_SLAVE_STATE_S;

typedef struct _GC2093_SLAVE_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
} GC2093_SLAVE_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastGc2093_Slave[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunGc2093_Slave_BusInfo[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeGc2093_Slave_MirrorFip[VI_MAX_PIPE_NUM];
extern const CVI_U8 gc2093_slave_i2c_addr;
extern const CVI_U32 gc2093_slave_addr_byte;
extern const CVI_U32 gc2093_slave_data_byte;
extern void gc2093_slave_init(VI_PIPE ViPipe);
extern void gc2093_slave_exit(VI_PIPE ViPipe);
extern void gc2093_slave_standby(VI_PIPE ViPipe);
extern void gc2093_slave_restart(VI_PIPE ViPipe);
extern int  gc2093_slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  gc2093_slave_read_register(VI_PIPE ViPipe, int addr);
extern void gc2093_slave_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  gc2093_slave_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __GC2093_SLAVE_CMOS_EX_H_ */

