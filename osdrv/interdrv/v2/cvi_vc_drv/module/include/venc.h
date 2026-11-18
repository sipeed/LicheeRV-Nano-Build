#ifndef __VENC_H__
#define __VENC_H__

#include <base_ctx.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_comm_venc.h>
#include "enc_ctx.h"
#include "cvi_jpg_interface.h"

#define VENC_MEMSET memset

typedef enum _VENC_CHN_STATE_ {
	VENC_CHN_STATE_NONE = 0,
	VENC_CHN_STATE_INIT,
	VENC_CHN_STATE_START_ENC,
	VENC_CHN_STATE_STOP_ENC,
} VENC_CHN_STATE;

typedef enum _VENC_SBM_STATE {
	VENC_SBM_STATE_IDLE = 0,
	VENC_SBM_STATE_FRM_RUN,
	VENC_SBM_STATE_FRM_SKIP,
	VENC_SBM_STATE_CHN_CLOSED,
	VENC_SBM_MAX_STATE
} VENC_SBM_STATE;

typedef struct _venc_frc {
	CVI_BOOL bFrcEnable;
	CVI_S32 srcFrameDur;
	CVI_S32 dstFrameDur;
	CVI_S32 srcTs;
	CVI_S32 dstTs;
} venc_frc;

typedef struct _venc_vfps {
	CVI_BOOL bVfpsEnable;
	CVI_S32 s32NumFrmsInOneSec;
	CVI_U64 u64prevSec;
	CVI_U64 u64StatTime;
} venc_vfps;

#define CVI_DEF_VFPFS_STAT_TIME 2
#define MAX_VENC_FRM_NUM 32

typedef struct _venc_chn_vars {
	CVI_U64 u64TimeOfSendFrame;
	CVI_U64 u64LastGetStreamTimeStamp;
	CVI_U64 u64LastSendFrameTimeStamp;
	CVI_U64 currPTS;
	CVI_U64 totalTime;
	CVI_S32 frameIdx;
	CVI_S32 s32RecvPicNum;
	CVI_S32 bind_event_fd;
	venc_frc frc;
	venc_vfps vfps;
	VENC_STREAM_S stStream;
	VENC_JPEG_PARAM_S stJpegParam;
	VENC_CHN_PARAM_S stChnParam;
	VENC_CHN_STATUS_S chnStatus;
	VENC_CU_PREDICTION_S cuPrediction;
	VENC_FRAME_PARAM_S frameParam;
	VENC_CHN_STATE chnState;
	USER_RC_INFO_S stUserRcInfo;
	VENC_SUPERFRAME_CFG_S stSuperFrmParam;
	struct semaphore sem_send;
	struct semaphore sem_release;
	CVI_BOOL bAttrChange;
	VENC_FRAMELOST_S frameLost;
	CVI_BOOL bHasVbPool;
	VENC_CHN_POOL_S vbpool;
	VB_BLK vbBLK[VB_COMM_POOL_MAX_CNT];
	cviBufInfo FrmArray[MAX_VENC_FRM_NUM];
	CVI_U32 FrmNum;
	CVI_U32 u32SendFrameCnt;
	CVI_U32 u32GetStreamCnt;
	CVI_S32 s32BindModeGetStreamRet;
	CVI_U32 u32FirstPixelFormat;
	CVI_BOOL bSendFirstFrm;
	CVI_U32 u32Stride[3];
	VIDEO_FRAME_INFO_S stFrameInfo;
	VENC_ROI_ATTR_S stRoiAttr[8];
	VCODEC_PERF_FPS_S stFPS;
} venc_chn_vars;

typedef struct _venc_chn_context {
	VENC_CHN VeChn;
	VENC_CHN_ATTR_S *pChnAttr;
	VENC_RC_PARAM_S rcParam;
	VENC_REF_PARAM_S refParam;
	VENC_FRAMELOST_S frameLost;
	VENC_H264_ENTROPY_S h264Entropy;
	VENC_H264_TRANS_S h264Trans;
	VENC_H265_TRANS_S h265Trans;
	VENC_H264_SLICE_SPLIT_S h264Split;
	VENC_H265_SLICE_SPLIT_S h265Split;
	union {
		VENC_H264_VUI_S h264Vui;
		VENC_H265_VUI_S h265Vui;
	};
	VENC_H264_INTRA_PRED_S h264IntraPred;
	VENC_SVC_PARAM_S svcParam;
	CVI_BOOL bSvcEnable;
	venc_enc_ctx encCtx;
	venc_chn_vars *pChnVars;
	struct cvi_venc_vb_ctx *pVbCtx;
	struct mutex chnMutex;
	struct mutex chnShmMutex;
	CVI_BOOL bSbSkipFrm;
	VENC_SBM_STATE sbm_state;
	CVI_U32 jpgFrmSkipCnt;
	cviVencSbSetting stSbSetting;
	VIDEO_FRAME_INFO_S stVideoFrameInfo;
	CVI_BOOL bChnEnable;
	union {
		VENC_H264_DBLK_S h264Dblk;
		VENC_H265_DBLK_S h265Dblk;
	};
} venc_chn_context;

typedef struct _venc_sbm_context {
	CVI_U32 SbmNum;
	VENC_CHN CurrSbmChn;
	struct mutex SbmMutex;
	struct task_struct *pSBMSendFrameThread;
} venc_sbm_context;

typedef struct _CVI_VENC_MODPARAM_S {
	VENC_MOD_VENC_S stVencModParam;
	VENC_MOD_H264E_S stH264eModParam;
	VENC_MOD_H265E_S stH265eModParam;
	VENC_MOD_JPEGE_S stJpegeModParam;
	VENC_MOD_RC_S stRcModParam;
} CVI_VENC_PARAM_MOD_S;

typedef struct _venc_context {
	venc_chn_context * chn_handle[VENC_MAX_CHN_NUM];
	CVI_U32 chn_status[VENC_MAX_CHN_NUM];
	CVI_VENC_PARAM_MOD_S ModParam;
	venc_sbm_context sbm_context;
} venc_context;

#endif
