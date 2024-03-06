
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "devmem.h"
#include "cvi_venc.h"

#include <linux/cvi_vc_drv_ioctl.h>

#define UNUSED(x)	((void)(x))

typedef struct _VENC_STREAM_EX_S {
	VENC_STREAM_S *pstStream;
	CVI_S32 s32MilliSec;
} VENC_STREAM_EX_S;

typedef struct _VENC_USER_DATA_S {
	CVI_U8 *pu8Data;
	CVI_U32 u32Len;
} VENC_USER_DATA_S;

typedef struct _VIDEO_FRAME_INFO_EX_S {
	const VIDEO_FRAME_INFO_S *pstFrame;
	CVI_S32 s32MilliSec;
} VIDEO_FRAME_INFO_EX_S;

typedef struct _USER_FRAME_INFO_EX_S {
	const USER_FRAME_INFO_S *pstUserFrame;
	CVI_S32 s32MilliSec;
} USER_FRAME_INFO_EX_S;

#define CVI_VENC_NO_INPUT	-10
#define CVI_VENC_INPUT_ERR	-11
#define DUMP_YUV			"dump_src.yuv"
#define DUMP_BS				"dump_bs.bin"

venc_dbg vencDbg;
static CVI_S32 s32DevmemFd = -1;
static CVI_U32 u32ChannelCreatedCnt;

CVI_S32 s32VencFd[VENC_MAX_CHN_NUM] = {0};
CVI_U8 *pStreamPackArray[VENC_MAX_CHN_NUM][8] = {NULL};

static CVI_S32 cviGetEnv(CVI_CHAR *env, CVI_CHAR *fmt, CVI_VOID *param)
{
	CVI_CHAR *debugEnv;
	CVI_S32 val = CVI_VENC_NO_INPUT;

	debugEnv = getenv(env);
	if (debugEnv) {
		if (strcmp(fmt, "%s") == 0) {
			strcpy(param, debugEnv);
		} else {
			if (sscanf(debugEnv, fmt, &val) != 1)
				return CVI_VENC_INPUT_ERR;
			CVI_VENC_TRACE("%s = 0x%X\n", env, val);
		}
	}

	return val;
}

static CVI_VOID cviChangeMask(CVI_S32 frameIdx)
{
	venc_dbg *pDbg = &vencDbg;

	pDbg->currMask = pDbg->dbgMask;
	if (pDbg->startFn >= 0) {
		if (frameIdx >= pDbg->startFn &&
			frameIdx <= pDbg->endFn)
			pDbg->currMask = pDbg->dbgMask;
		else
			pDbg->currMask = CVI_VENC_MASK_ERR;
	}

	CVI_VENC_TRACE("currMask = 0x%X\n", pDbg->currMask);
}

CVI_VOID cviGetMask(void)
{
	venc_dbg *pDbg = &vencDbg;

	CVI_VENC_TRACE("\n");

	memset(pDbg, 0, sizeof(venc_dbg));

	pDbg->dbgMask = cviGetEnv("venc_mask", "0x%x", NULL);
	if (pDbg->dbgMask == CVI_VENC_NO_INPUT ||
		pDbg->dbgMask == CVI_VENC_INPUT_ERR)
		pDbg->dbgMask = CVI_VENC_MASK_ERR;
	else
		pDbg->dbgMask |= CVI_VENC_MASK_ERR;

	pDbg->currMask = pDbg->dbgMask;
	pDbg->startFn = cviGetEnv("venc_sfn", "%d", NULL);
	pDbg->endFn = cviGetEnv("venc_efn", "%d", NULL);
	cviGetEnv("venc_dir", "%s", pDbg->dbgDir);

	cviChangeMask(0);
}

static CVI_S32 openDevice(VENC_CHN VeChn)
{
	if (s32VencFd[VeChn] <= 0) {
		CVI_CHAR devName[255];

		sprintf(devName, "/dev/%s%d", CVI_VC_DRV_ENCODER_DEV_NAME, VeChn);
		s32VencFd[VeChn] = open(devName, O_RDWR | O_DSYNC | O_CLOEXEC);

		if (s32VencFd[VeChn] <= 0) {
			CVI_VENC_ERR("open device (%s) fail\n", devName);
			return CVI_FAILURE;
		}
		CVI_VENC_TRACE("open device (%s) fd: %d\n", devName, s32VencFd[VeChn]);
	}

	if (s32DevmemFd == -1) {
		s32DevmemFd = devm_open_cached();
		if (s32DevmemFd < 0) {
			CVI_VENC_ERR("devm_open fail\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_CreateChn(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	CVI_CHAR devName[255];

	if (openDevice(VeChn) != CVI_SUCCESS) {
		CVI_VENC_ERR("openDevice fail\n");
		return s32Ret;
	}

	if (s32VencFd[VeChn] > 0) {
		s32Ret = ioctl(s32VencFd[VeChn], CVI_VC_VENC_CREATE_CHN, pstAttr);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VENC_ERR("ioctl CVI_VC_VENC_CREATE_CHN fail with %d\n", s32Ret);
			return s32Ret;
		}
		u32ChannelCreatedCnt += 1;
	} else {
		CVI_VENC_ERR("fail to open device %s\n", devName);
		s32Ret = CVI_FAILURE;
	}
	return s32Ret;
}

CVI_S32 CVI_VENC_DestroyChn(VENC_CHN VeChn)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (s32VencFd[VeChn] > 0) {
		s32Ret = ioctl(s32VencFd[VeChn], CVI_VC_VENC_DESTROY_CHN);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VENC_ERR("ioctl CVI_VC_VENC_DESTROY_CHN fail with %d\n", s32Ret);
			return s32Ret;
		}
		close(s32VencFd[VeChn]);
		s32VencFd[VeChn] = 0;
	}

	u32ChannelCreatedCnt -= 1;
	if (u32ChannelCreatedCnt == 0) {
		devm_close(s32DevmemFd);
		s32DevmemFd = -1;
	}

	return s32Ret;
}

CVI_S32 CVI_VENC_ResetChn(VENC_CHN VeChn)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_RESET_CHN);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_StartRecvFrame(VENC_CHN VeChn, const VENC_RECV_PIC_PARAM_S *pstRecvParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_START_RECV_FRAME, pstRecvParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_StopRecvFrame(VENC_CHN VeChn)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_STOP_RECV_FRAME);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_QueryStatus(VENC_CHN VeChn, VENC_CHN_STATUS_S *pstStatus)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_QUERY_STATUS, pstStatus);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetChnAttr(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstChnAttr)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_CHN_ATTR, pstChnAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetChnAttr(VENC_CHN VeChn, VENC_CHN_ATTR_S *pstChnAttr)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_CHN_ATTR, pstChnAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream, CVI_S32 S32MilliSec)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (s32VencFd[VeChn] > 0) {
		VENC_STREAM_EX_S stStreamEx, *pstStreamEx = &stStreamEx;

		pstStreamEx->pstStream = pstStream;
		pstStreamEx->s32MilliSec = S32MilliSec;
		s32Ret = ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_STREAM, pstStreamEx);

		if (s32Ret == CVI_SUCCESS) {
			CVI_U32 i = 0;
			VENC_PACK_S *ppack;

			for (i = 0; i < pstStreamEx->pstStream->u32PackCount; i++) {
				ppack = &pstStreamEx->pstStream->pstPack[i];
				if (ppack->u64PhyAddr && ppack->u32Len) {
					pStreamPackArray[VeChn][i] = ppack->pu8Addr;
					ppack->pu8Addr = devm_map(s32DevmemFd, ppack->u64PhyAddr, ppack->u32Len);
				}
			}
		}
		return s32Ret;
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_ReleaseStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream)
{
	if (s32VencFd[VeChn] > 0) {
		CVI_U32 i = 0;
		VENC_PACK_S *ppack;

		for (i = 0; i < pstStream->u32PackCount; i++) {
			ppack = &pstStream->pstPack[i];
			if (ppack->u64PhyAddr && ppack->u32Len) {
				devm_unmap(ppack->pu8Addr, ppack->u32Len);
				ppack->pu8Addr = pStreamPackArray[VeChn][i];
			}
		}
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_RELEASE_STREAM, pstStream);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_InsertUserData(VENC_CHN VeChn, CVI_U8 *pu8Data, CVI_U32 u32Len)
{
	if (s32VencFd[VeChn] > 0) {
		VENC_USER_DATA_S stUserData, *pstUserData = &stUserData;

		pstUserData->pu8Data = pu8Data;
		pstUserData->u32Len = u32Len;
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_INSERT_USERDATA, pstUserData);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SendFrame(VENC_CHN VeChn, const VIDEO_FRAME_INFO_S *pstFrame, CVI_S32 s32MilliSec)
{
	if (s32VencFd[VeChn] > 0) {
		VIDEO_FRAME_INFO_EX_S stFrameEx, *pstFrameEx = &stFrameEx;

		pstFrameEx->pstFrame = pstFrame;
		pstFrameEx->s32MilliSec = s32MilliSec;
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SEND_FRAME, pstFrameEx);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SendFrameEx(VENC_CHN VeChn, const USER_FRAME_INFO_S *pstFrame, CVI_S32 s32MilliSec)
{
	if (s32VencFd[VeChn] > 0) {
		USER_FRAME_INFO_EX_S stUserFrameEx, *pstUserFrameEx = &stUserFrameEx;

		pstUserFrameEx->pstUserFrame = pstFrame;
		pstUserFrameEx->s32MilliSec = s32MilliSec;
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SEND_FRAMEEX, pstUserFrameEx);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_RequestIDR(VENC_CHN VeChn, CVI_BOOL bInstant)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_REQUEST_IDR, &bInstant);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetFd(VENC_CHN VeChn)
{
	if (openDevice(VeChn) != CVI_SUCCESS) {
		CVI_VENC_ERR("openDevice fail\n");
		return CVI_FAILURE;
	}
	return s32VencFd[VeChn];
}

CVI_S32 CVI_VENC_CloseFd(VENC_CHN VeChn)
{
	// close fd in destroy channel
	UNUSED(VeChn);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetRoiAttr(VENC_CHN VeChn, const VENC_ROI_ATTR_S *pstRoiAttr)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_ROI_ATTR, pstRoiAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetRoiAttr(VENC_CHN VeChn, CVI_U32 u32Index, VENC_ROI_ATTR_S *pstRoiAttr)
{
	if (s32VencFd[VeChn] > 0) {
		pstRoiAttr->u32Index = u32Index;
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_ROI_ATTR, pstRoiAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264Trans(VENC_CHN VeChn, const VENC_H264_TRANS_S *pstH264Trans)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_TRANS, pstH264Trans);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264Trans(VENC_CHN VeChn, VENC_H264_TRANS_S *pstH264Trans)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_TRANS, pstH264Trans);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264Entropy(VENC_CHN VeChn, const VENC_H264_ENTROPY_S *pstH264EntropyEnc)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_ENTROPY, pstH264EntropyEnc);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264Entropy(VENC_CHN VeChn, VENC_H264_ENTROPY_S *pstH264EntropyEnc)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_ENTROPY, pstH264EntropyEnc);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264Vui(VENC_CHN VeChn, const VENC_H264_VUI_S *pstH264Vui)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_VUI, pstH264Vui);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264Vui(VENC_CHN VeChn, VENC_H264_VUI_S *pstH264Vui)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_VUI, pstH264Vui);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH265Vui(VENC_CHN VeChn, const VENC_H265_VUI_S *pstH265Vui)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H265_VUI, pstH265Vui);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH265Vui(VENC_CHN VeChn, VENC_H265_VUI_S *pstH265Vui)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H265_VUI, pstH265Vui);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetJpegParam(VENC_CHN VeChn, const VENC_JPEG_PARAM_S *pstJpegParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_JPEG_PARAM, pstJpegParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetJpegParam(VENC_CHN VeChn, VENC_JPEG_PARAM_S *pstJpegParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_JPEG_PARAM, pstJpegParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetRcParam(VENC_CHN VeChn, VENC_RC_PARAM_S *pstRcParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_RC_PARAM, pstRcParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetRcParam(VENC_CHN VeChn, const VENC_RC_PARAM_S *pstRcParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_RC_PARAM, pstRcParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetRefParam(VENC_CHN VeChn, const VENC_REF_PARAM_S *pstRefParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_REF_PARAM, pstRefParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetRefParam(VENC_CHN VeChn, VENC_REF_PARAM_S *pstRefParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_REF_PARAM, pstRefParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH265Trans(VENC_CHN VeChn, const VENC_H265_TRANS_S *pstH265Trans)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H265_TRANS, pstH265Trans);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH265Trans(VENC_CHN VeChn, VENC_H265_TRANS_S *pstH265Trans)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H265_TRANS, pstH265Trans);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetFrameLostStrategy(VENC_CHN VeChn, const VENC_FRAMELOST_S *pstFrmLostParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_FRAMELOST_STRATEGY, pstFrmLostParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetFrameLostStrategy(VENC_CHN VeChn, VENC_FRAMELOST_S *pstFrmLostParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_FRAMELOST_STRATEGY, pstFrmLostParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetSuperFrameStrategy(VENC_CHN VeChn, const VENC_SUPERFRAME_CFG_S *pstSuperFrmParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_SUPERFRAME_STRATEGY, pstSuperFrmParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetSuperFrameStrategy(VENC_CHN VeChn, VENC_SUPERFRAME_CFG_S *pstSuperFrmParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_SUPERFRAME_STRATEGY, pstSuperFrmParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetChnParam(VENC_CHN VeChn, const VENC_CHN_PARAM_S *pstChnParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_CHN_PARAM, pstChnParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetChnParam(VENC_CHN VeChn, VENC_CHN_PARAM_S *pstChnParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_CHN_PARAM, pstChnParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetModParam(const VENC_PARAM_MOD_S *pstModParam)
{
	VENC_CHN VeChn = 0;	// default hard-code

	if (s32DevmemFd == -1) {
		s32DevmemFd = devm_open_cached();
		if (s32DevmemFd < 0)
			return CVI_FAILURE;
	}

	if (openDevice(VeChn) != CVI_SUCCESS) {
		CVI_VENC_ERR("openDevice fail\n");
		return CVI_FAILURE;
	}

	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_MOD_PARAM, pstModParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetModParam(VENC_PARAM_MOD_S *pstModParam)
{
	VENC_CHN VeChn = 0;	// default hard-code

	if (openDevice(VeChn) != CVI_SUCCESS) {
		CVI_VENC_ERR("openDevice fail\n");
		return CVI_FAILURE;
	}

	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_MOD_PARAM, pstModParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_AttachVbPool(VENC_CHN VeChn, const VENC_CHN_POOL_S *pstPool)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_ATTACH_VBPOOL, pstPool);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_DetachVbPool(VENC_CHN VeChn)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_DETACH_VBPOOL);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetCuPrediction(VENC_CHN VeChn,
		const VENC_CU_PREDICTION_S *pstCuPrediction)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_CUPREDICTION, pstCuPrediction);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetCuPrediction(VENC_CHN VeChn, VENC_CU_PREDICTION_S *pstCuPrediction)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_CUPREDICTION, pstCuPrediction);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_CalcFrameParam(VENC_CHN VeChn, VENC_FRAME_PARAM_S *pstFrameParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_CALC_FRAME_PARAM, pstFrameParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetFrameParam(VENC_CHN VeChn, const VENC_FRAME_PARAM_S *pstFrameParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_FRAME_PARAM, pstFrameParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetFrameParam(VENC_CHN VeChn, VENC_FRAME_PARAM_S *pstFrameParam)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_FRAME_PARAM, pstFrameParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264SliceSplit(VENC_CHN VeChn, const VENC_H264_SLICE_SPLIT_S *pstSliceSplit)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_SLICE_SPLIT, pstSliceSplit);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264SliceSplit(VENC_CHN VeChn, VENC_H264_SLICE_SPLIT_S *pstSliceSplit)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_SLICE_SPLIT, pstSliceSplit);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH265SliceSplit(VENC_CHN VeChn, const VENC_H265_SLICE_SPLIT_S *pstSliceSplit)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H265_SLICE_SPLIT, pstSliceSplit);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH265SliceSplit(VENC_CHN VeChn, VENC_H265_SLICE_SPLIT_S *pstSliceSplit)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H265_SLICE_SPLIT, pstSliceSplit);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264Dblk(VENC_CHN VeChn, const VENC_H264_DBLK_S *pstH264Dblk)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_Dblk, pstH264Dblk);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264Dblk(VENC_CHN VeChn, VENC_H264_DBLK_S *pstH264Dblk)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_Dblk, pstH264Dblk);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH265Dblk(VENC_CHN VeChn, const VENC_H265_DBLK_S *pstH265Dblk)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H265_Dblk, pstH265Dblk);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH265Dblk(VENC_CHN VeChn, VENC_H265_DBLK_S *pstH265Dblk)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H265_Dblk, pstH265Dblk);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_SetH264IntraPred(VENC_CHN VeChn, const VENC_H264_INTRA_PRED_S *pstH264IntraPred)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_SET_H264_INTRA_PRED, pstH264IntraPred);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VENC_GetH264IntraPred(VENC_CHN VeChn, VENC_H264_INTRA_PRED_S *pstH264IntraPred)
{
	if (s32VencFd[VeChn] > 0) {
		return ioctl(s32VencFd[VeChn], CVI_VC_VENC_GET_H264_INTRA_PRED, pstH264IntraPred);
	}
	return CVI_FAILURE;
}
