#ifndef __VDEC_H__
#define __VDEC_H__

#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>

#define DISPLAY_QUEUE_SIZE 32
#define MAX_VDEC_FRM_NUM 32

typedef struct _vdec_chn_context {
	VDEC_CHN VdChn;
	CVI_U64 totalDecTime;
	VDEC_CHN_ATTR_S ChnAttr;
	VDEC_CHN_PARAM_S ChnParam;
	VDEC_CHN_STATUS_S stStatus;
	VIDEO_FRAME_INFO_S *VideoFrameArray;
	CVI_U32 VideoFrameArrayNum;
	CVI_S8 display_queue[DISPLAY_QUEUE_SIZE];
	CVI_U32 w_idx;
	CVI_U32 r_idx;
	CVI_U32 seqNum;
	struct mutex display_queue_lock;
	struct mutex status_lock;
	struct mutex chnShmMutex;
	CVI_VOID *pHandle;
	CVI_BOOL bHasVbPool;
	VDEC_CHN_POOL_S vbPool;
	VB_BLK vbBLK[MAX_VDEC_FRM_NUM];
	cviBufInfo FrmArray[MAX_VDEC_FRM_NUM];
	CVI_U32 FrmNum;
	CVI_BOOL bStartRecv;
	struct cvi_vdec_vb_ctx *pVbCtx;

	CVI_U64 u64LastSendStreamTimeStamp;
	CVI_U64 u64LastGetFrameTimeStamp;
	CVI_U32 u32SendStreamCnt;
	CVI_U32 u32GetFrameCnt;
	VIDEO_FRAME_INFO_S stVideoFrameInfo;
	VCODEC_PERF_FPS_S stFPS;
	CVI_U32 u32VBSize;
} vdec_chn_context;

typedef struct _vdec_context {
	vdec_chn_context *chn_handle[VDEC_MAX_CHN_NUM];
	VDEC_MOD_PARAM_S g_stModParam;
} vdec_context;

#endif
