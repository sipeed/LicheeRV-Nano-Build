#ifndef __CVI_VC_DRV_PROC_H__
#define __CVI_VC_DRV_PROC_H__

#include <linux/device.h>

#define VENC_PROC_NAME "cvitek/venc"
#define H265E_PROC_NAME "cvitek/h265e"
#define H264E_PROC_NAME "cvitek/h264e"
#define JPEGE_PROC_NAME "cvitek/jpege"
#define CODEC_PROC_NAME "cvitek/codec"
#define RC_PROC_NAME "cvitek/rc"
#define VDEC_PROC_NAME "cvitek/vdec"
#define VIDEO_PROC_PERMS (0644)
#define VIDEO_PROC_PARENT (NULL)
#define MAX_PROC_STR_SIZE (255)
#define MAX_DIR_STR_SIZE (255)

typedef struct _proc_debug_config_t {
	uint32_t u32DbgMask;
	uint32_t u32StartFrmIdx;
	uint32_t u32EndFrmIdx;
	char cDumpPath[MAX_DIR_STR_SIZE];
	uint32_t u32NoDataTimeout;
} proc_debug_config_t;

int venc_proc_init(struct device *dev);
int venc_proc_deinit(void);
int h265e_proc_init(struct device *dev);
int codecinst_proc_init(struct device *dev);
int codecinst_proc_deinit(void);

int h265e_proc_deinit(void);
int h264e_proc_init(struct device *dev);
int h264e_proc_deinit(void);
int jpege_proc_init(struct device *dev);
int jpege_proc_deinit(void);
int rc_proc_init(struct device *dev);
int rc_proc_deinit(void);
int vdec_proc_init(struct device *dev);
int vdec_proc_deinit(void);

#endif /* __CVI_VC_DRV_PROC_H__ */
