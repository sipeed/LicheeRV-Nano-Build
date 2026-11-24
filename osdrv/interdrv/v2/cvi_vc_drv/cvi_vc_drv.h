#ifndef __CVI_VC_DRV_H__
#define __CVI_VC_DRV_H__

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/io.h>

#include <linux/cvi_vc_drv_ioctl.h>
#include "cvi_venc.h"
#include "cvi_vdec.h"

#define CVI_VC_DRV_PLATFORM_DEVICE_NAME "cvi_vc_drv"
#define CVI_VC_DRV_CLASS_NAME "cvi_vc_drv"

typedef struct cvi_vc_drv_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base; /* kernel logical address in use kernel */
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} cvi_vc_drv_buffer_t;

struct cvi_vc_drv_ops {
	//	void	(*clk_get)(struct cvi_vc_drv_device *vdev);
};

struct cvi_vc_drv_pltfm_data {
	unsigned int quirks;
	unsigned int version;
};

struct cvi_vc_drv_device {
	struct device *dev;

	struct class *cvi_vc_class;

	dev_t venc_cdev_id;
	struct cdev *p_venc_cdev;
	int s_venc_major;

	dev_t vdec_cdev_id;
	struct cdev *p_vdec_cdev;
	int s_vdec_major;

	cvi_vc_drv_buffer_t ctrl_register;
	cvi_vc_drv_buffer_t remap_register;
	cvi_vc_drv_buffer_t sbm_register;

	const struct cvi_vc_drv_ops *pdata;
};

typedef struct _VENC_STREAM_EX_S {
	VENC_STREAM_S *pstStream;
	CVI_S32 s32MilliSec;
} VENC_STREAM_EX_S;

typedef struct _VENC_USER_DATA_S {
	CVI_U8 *pu8Data;
	CVI_U32 u32Len;
} VENC_USER_DATA_S;

typedef struct _VIDEO_FRAME_INFO_EX_S {
	VIDEO_FRAME_INFO_S *pstFrame;
	CVI_S32 s32MilliSec;
} VIDEO_FRAME_INFO_EX_S;

typedef struct _USER_FRAME_INFO_EX_S {
	USER_FRAME_INFO_S *pstUserFrame;
	CVI_S32 s32MilliSec;
} USER_FRAME_INFO_EX_S;

typedef struct _VDEC_STREAM_EX_S {
	VDEC_STREAM_S *pstStream;
	CVI_S32 s32MilliSec;
} VDEC_STREAM_EX_S;

typedef enum {
	REG_CTRL = 0,
	REG_SBM,
	REG_REMAP,
} REG_TYPE;

unsigned int cvi_vc_drv_read_vc_reg(REG_TYPE eRegType, unsigned long addr);
void cvi_vc_drv_write_vc_reg(REG_TYPE eRegType, unsigned long addr,
			     unsigned int data);

#define CtrlWriteReg(CORE, ADDR, DATA)	\
	cvi_vc_drv_write_vc_reg(REG_CTRL, ADDR, DATA)
#define CtrlReadReg(CORE, ADDR)	\
	cvi_vc_drv_read_vc_reg(REG_CTRL, ADDR)
#define RemapWriteReg(CORE, ADDR, DATA)	\
	cvi_vc_drv_write_vc_reg(REG_REMAP, ADDR, DATA)
#define RemapReadReg(CORE, ADDR)	\
	cvi_vc_drv_read_vc_reg(REG_REMAP, ADDR)

#endif /* __CVI_VC_DRV_H__ */
