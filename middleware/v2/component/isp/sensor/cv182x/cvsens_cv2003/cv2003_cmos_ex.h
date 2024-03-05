#ifndef __CV2003_CMOS_EX_H_
#define __CV2003_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef ARCH_CV182X
#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#else
#include <linux/cif_uapi.h>
#include <linux/vi_snsr.h>
#include <linux/cvi_type.h>
#endif
#include "cvi_sns_ctrl.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

enum cv2003_linear_regs_e {
	LINEAR_EXP_0,       //0x3062 bit[19:16]
	LINEAR_EXP_1,       //0x3061
	LINEAR_EXP_2,       //0x3060
	LINEAR_GAINENABLE,  //0x3180 bit[7:0]
	LINEAR_AGAIN,       //0x3180 bit[7:0]
	LINEAR_DGAIN_H,     //0x3179 bit[15:8]
	LINEAR_DGAIN_L,     //0x3178 bit[7:0]
	LINEAR_VTS_0,       //0x302A bit[19:16]
	LINEAR_VTS_1,       //0x3029
	LINEAR_VTS_2,       //0x3028
	LINEAR_FLIP_MIRROR, //0x3034
	LINEAR_REGS_NUM
};

typedef enum _CV2003_MODE_E {
	CV2003_MODE_1920X1080P30 = 0,
	CV2003_MODE_LINEAR_NUM,
	CV2003_MODE_1920X1080P15_WDR = CV2003_MODE_LINEAR_NUM,
	CV2003_MODE_NUM
} CV2003_MODE_E;

typedef struct _CV2003_STATE_S {
	CVI_U32		u32Sexp_MAX;
} CV2003_STATE_S;

typedef struct _CV2003_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	CVI_U32 u32IspResTime;
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
} CV2003_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastCV2003[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunCV2003_BusInfo[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeCV2003_MirrorFip[VI_MAX_PIPE_NUM];
extern CVI_U8 cv2003_i2c_addr;
extern const CVI_U32 cv2003_addr_byte;
extern const CVI_U32 cv2003_data_byte;
extern void cv2003_init(VI_PIPE ViPipe);
extern void cv2003_exit(VI_PIPE ViPipe);
extern void cv2003_standby(VI_PIPE ViPipe);
extern void cv2003_restart(VI_PIPE ViPipe);
extern int  cv2003_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  cv2003_read_register(VI_PIPE ViPipe, int addr);
extern int  cv2003_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __CV2003_CMOS_EX_H_ */

