
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "devmem.h"
#include "cvi_vdec.h"

#include <linux/cvi_vc_drv_ioctl.h>

#define UNUSED(x)	((void)(x))

vdec_dbg vdecDbg;
static CVI_S32 s32DevmemFd = -1;
static CVI_U32 u32ChannelCreatedCnt;

typedef struct _VIDEO_FRAME_INFO_EX_S {
	const VIDEO_FRAME_INFO_S *pstFrame;
	CVI_S32 s32MilliSec;
} VIDEO_FRAME_INFO_EX_S;

typedef struct _VDEC_STREAM_EX_S {
	const VDEC_STREAM_S *pstStream;
	CVI_S32 s32MilliSec;
} VDEC_STREAM_EX_S;

CVI_S32 s32VdecFd[VDEC_MAX_CHN_NUM] = {0};

CVI_S32 CVI_VDEC_CreateChn(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	CVI_CHAR devName[255];

	if (s32DevmemFd == -1) {
		s32DevmemFd = devm_open_cached();
		if (s32DevmemFd < 0)
			return CVI_FAILURE;
	}

	if (s32VdecFd[VdChn] == 0) {
		sprintf(devName, "/dev/%s%d", CVI_VC_DRV_DECODER_DEV_NAME, VdChn);
		s32VdecFd[VdChn] = open(devName, O_RDWR | O_DSYNC | O_CLOEXEC);
		CVI_VDEC_TRACE("open device (%s) fd: %d\n", devName, s32VdecFd[VdChn]);
	}

	if (s32VdecFd[VdChn] > 0) {
		s32Ret = ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_CREATE_CHN, pstAttr);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VDEC_ERR("ioctl CVI_VC_VDEC_CREATE_CHN fail with %d\n", s32Ret);
			return s32Ret;
		}
		u32ChannelCreatedCnt += 1;
	} else {
		CVI_VDEC_ERR("fail to open device %s\n", devName);
		s32Ret = CVI_FAILURE;
	}

	return s32Ret;
}

CVI_S32 CVI_VDEC_DestroyChn(VDEC_CHN VdChn)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (s32VdecFd[VdChn] > 0) {
		s32Ret = ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_DESTROY_CHN, NULL);
		if (s32Ret != CVI_SUCCESS) {
			CVI_VDEC_ERR("ioctl CVI_VC_VDEC_DESTROY_CHN fail with %d\n", s32Ret);
			return s32Ret;
		}
		close(s32VdecFd[VdChn]);
		s32VdecFd[VdChn] = 0;
	}

	u32ChannelCreatedCnt -= 1;
	if (u32ChannelCreatedCnt == 0) {
		devm_close(s32DevmemFd);
		s32DevmemFd = -1;
	}

	return s32Ret;
}

CVI_S32 CVI_VDEC_GetChnAttr(VDEC_CHN VdChn, VDEC_CHN_ATTR_S *pstAttr)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_GET_CHN_ATTR, pstAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_SetChnAttr(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_SET_CHN_ATTR, pstAttr);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_StartRecvStream(VDEC_CHN VdChn)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_START_RECV_STREAM, NULL);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_StopRecvStream(VDEC_CHN VdChn)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_STOP_RECV_STREAM, NULL);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_QueryStatus(VDEC_CHN VdChn, VDEC_CHN_STATUS_S *pstStatus)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_QUERY_STATUS, pstStatus);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_GetFd(VDEC_CHN VdChn)
{
	if (s32VdecFd[VdChn] <= 0) {
		CVI_CHAR devName[255];

		sprintf(devName, "/dev/%s%d", CVI_VC_DRV_DECODER_DEV_NAME, VdChn);
		s32VdecFd[VdChn] = open(devName, O_RDWR | O_DSYNC | O_CLOEXEC);
		CVI_VDEC_TRACE("open device (%s) fd: %d\n", devName, s32VdecFd[VdChn]);
	}
	return s32VdecFd[VdChn];
}

CVI_S32 CVI_VDEC_CloseFd(VDEC_CHN VdChn)
{
	// close fd in destroy channel
	UNUSED(VdChn);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VDEC_ResetChn(VDEC_CHN VdChn)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_RESET_CHN);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_SetChnParam(VDEC_CHN VdChn, const VDEC_CHN_PARAM_S *pstParam)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_SET_CHN_PARAM, pstParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_GetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S *pstParam)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_GET_CHN_PARAM, pstParam);
	}
	return CVI_FAILURE;
}

/* s32MilliSec: -1 is block,0 is no block,other positive number is timeout */
CVI_S32 CVI_VDEC_SendStream(VDEC_CHN VdChn, const VDEC_STREAM_S *pstStream, CVI_S32 s32MilliSec)
{
	if (s32VdecFd[VdChn] > 0) {
		VDEC_STREAM_EX_S stStreamEx, *pstStreamEx = &stStreamEx;

		pstStreamEx->pstStream = pstStream;
		pstStreamEx->s32MilliSec = s32MilliSec;

		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_SEND_STREAM, pstStreamEx);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_GetFrame(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec)
{
	if (s32VdecFd[VdChn] > 0) {
		CVI_S32 s32Ret = CVI_SUCCESS;
		VIDEO_FRAME_INFO_EX_S stFrameInfoEx, *pstFrameInfoEx = &stFrameInfoEx;

		pstFrameInfoEx->pstFrame = pstFrameInfo;
		pstFrameInfoEx->s32MilliSec = s32MilliSec;

		s32Ret = ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_GET_FRAME, pstFrameInfoEx);
		if (s32Ret == CVI_SUCCESS) {
			int i = 0;

			for (i = 0; i < 3; i++) {
				if (pstFrameInfo->stVFrame.u64PhyAddr[i] &&
					pstFrameInfo->stVFrame.u32Length[i]) {
					pstFrameInfo->stVFrame.pu8VirAddr[i] =
						devm_map(s32DevmemFd,
								pstFrameInfo->stVFrame.u64PhyAddr[i],
								pstFrameInfo->stVFrame.u32Length[i]);
				}
			}
		}
		return s32Ret;
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_ReleaseFrame(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	if (s32VdecFd[VdChn] > 0) {
		int i = 0;

		for (i = 0; i < 3; i++) {
			if (pstFrameInfo->stVFrame.pu8VirAddr[i] && pstFrameInfo->stVFrame.u32Length[i]) {
				devm_unmap(pstFrameInfo->stVFrame.pu8VirAddr[i], pstFrameInfo->stVFrame.u32Length[i]);
			}
		}
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_RELEASE_FRAME, pstFrameInfo);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_AttachVbPool(VDEC_CHN VdChn, const VDEC_CHN_POOL_S *pstPool)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_ATTACH_VBPOOL, pstPool);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_DetachVbPool(VDEC_CHN VdChn)
{
	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_DETACH_VBPOOL, NULL);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_SetModParam(const VDEC_MOD_PARAM_S *pstModParam)
{
	VDEC_CHN VdChn = 0;	// default hard-code

	if (s32VdecFd[VdChn] == 0) {
		CVI_CHAR devName[255];

		sprintf(devName, "/dev/%s%d", CVI_VC_DRV_DECODER_DEV_NAME, VdChn);
		s32VdecFd[VdChn] = open(devName, O_RDWR | O_DSYNC | O_CLOEXEC);

		if (s32DevmemFd == -1) {
			s32DevmemFd = devm_open_cached();
			if (s32DevmemFd < 0)
				return CVI_FAILURE;
		}
	}

	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[0], CVI_VC_VDEC_SET_MOD_PARAM, pstModParam);
	}
	return CVI_FAILURE;
}

CVI_S32 CVI_VDEC_GetModParam(VDEC_MOD_PARAM_S *pstModParam)
{
	VDEC_CHN VdChn = 0;	// default hard-code

	if (s32VdecFd[VdChn] > 0) {
		return ioctl(s32VdecFd[VdChn], CVI_VC_VDEC_GET_MOD_PARAM, pstModParam);
	}
	return CVI_FAILURE;
}

