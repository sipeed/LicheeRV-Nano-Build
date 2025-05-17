#include <linux/slab.h>
#include "jputypes.h"
#include "jpuapi.h"
#include "regdefine.h"
#include "jpulog.h"
#include "mixer.h"
#include "jpulog.h"
#include "jpuhelper.h"
#include "jpuapifunc.h"

#include "cvi_jpg_interface.h"
#include "cvi_jpg_internal.h"
#include "version.h"

#ifdef CLI_DEBUG_SUPPORT
#include "tcli.h"
#endif

#ifndef UNREFERENCED_PARAM
#define UNREFERENCED_PARAM(x) ((void)(x))
#endif

#define RET_JPG_TIMEOUT (-2)

void cviJpgGetVersion(void)
{
	CVI_JPG_DBG_INFO("JPEG_VERSION = %s\n", JPEG_VERSION);
}

/* initial jpu core */
int CVIJpgInit(void)
{
	JpgRet ret = JPG_RET_SUCCESS;

	CVI_JPG_DBG_IF("\n");

	cviJpgGetVersion();
	JpgEnterLock();
	ret = JPU_Init();
	if (ret != JPG_RET_SUCCESS && ret != JPG_RET_CALLED_BEFORE) {
		JLOG(ERR, "JPU_Init failed Error code is 0x%x\n", ret);
		JpgLeaveLock();
		return ret;
	}
	JpgLeaveLock();
	return JPG_RET_SUCCESS;
}

/* uninitial jpu core */
void CVIJpgUninit(void)
{
	// JLOG(INFO, "CVIJpgUninit ...\n");
	JpgEnterLock();
	JPU_DeInit();
	JpgLeaveLock();
}

/* alloc a jpu handle for dcoder or encoder */
CVIJpgHandle CVIJpgOpen(CVIJpgConfig config)
{
	JpgRet ret = JPG_RET_INVALID_PARAM;
	CVIJpgHandle handle = NULL;
	JpgInst *pJpgInst = NULL;

	JpgEnterLock();
	CVI_JPG_DBG_IF("\n");
	/* check param */
	// if (CVIJPGCOD_DEC == config.type)
	//     ret = CheckJpgDecOpenParam( pop );
	// else if (CVIJPGCOD_ENC == config.type)
	//     ret = CheckJpgDecOpenParam( pop );
	// else {
	//     ret = JPG_RET_INVALID_PARAM;
	//     goto OPEN_ERROR;
	// }

	/* get new instance handle */
	if (CVIJPGCOD_DEC == config.type) {
		// printf("Open decoder devices!\n");
		ret = cviJpgDecOpen(&handle, &config.u.dec);
		if (JPG_RET_SUCCESS != ret) {
			CVI_JPG_DBG_ERR("Open Decode Device fail, ret %d\n",
					ret);
		}
	} else if (CVIJPGCOD_ENC == config.type) {
		// printf("Open encoder devices!\n");
		ret = cviJpgEncOpen(&handle, &config.u.enc);
		if (JPG_RET_SUCCESS != ret) {
			CVI_JPG_DBG_ERR("Open Encode Device fail, ret %d\n",
					ret);
		}
	}

	pJpgInst = (JpgInst *)handle;
	pJpgInst->s32ChnNum = config.s32ChnNum;

	CVI_JPG_DBG_IF("handle = %p\n", handle);
	JpgLeaveLock();
	return handle;
}

/* close and free alloced jpu handle */
int CVIJpgClose(CVIJpgHandle jpgHandle)
{
	int ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = jpgHandle;
	JpgEnterLock();
	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	/* close instance handle */
	if (NULL == jpgHandle) {
		JpgLeaveLock();
		CVI_JPG_DBG_ERR("jpgHandle = NULL\n");
		return -1;
	}

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		ret = cviJpgDecClose(jpgHandle);
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncClose(jpgHandle);
	}

	JpgLeaveLock();
	return ret;
}

/* */
int CVIJpgGetCaps(CVIJpgHandle jpgHandle)
{
	UNREFERENCED_PARAM(jpgHandle);
	return JPG_RET_SUCCESS;
}

/* reset jpu core */
int CVIJpgReset(CVIJpgHandle jpgHandle)
{
	UNREFERENCED_PARAM(jpgHandle);
	return JPG_RET_SUCCESS;
}

/* flush data */
int CVIJpgFlush(CVIJpgHandle jpgHandle)
{
	JpgInst *pJpgInst = jpgHandle;
	int ret = JPG_RET_SUCCESS;
	if (NULL == jpgHandle)
		return -1;
	JpgEnterLock();

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		ret = cviJpgDecFlush(jpgHandle);
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncFlush(jpgHandle);
	}
	JpgLeaveLock();
	return ret;
}

/* send jpu data to decode or encode */
int CVIJpgSendFrameData(CVIJpgHandle jpgHandle, void *data, int length,
			int s32TimeOut)
{
	JpgInst *pJpgInst = jpgHandle;
	int ret = JPG_RET_SUCCESS;
	int count = 0;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	if (NULL == jpgHandle) {
		CVI_JPG_DBG_ERR("jpgHandle = NULL\n");
		return -1;
	}

	if (s32TimeOut <= -1) {
		//block mode
		JpgEnterLock();
	} else if (s32TimeOut == 0) {
		//try once mode
		ret = JpgEnterTryLock();
		if (ret != JPG_RET_SUCCESS) {
			//timeout
			CVI_JPG_DBG_IF("try lock failure\n");
			return RET_JPG_TIMEOUT;

		} else {
			//lock success
			CVI_JPG_DBG_IF("try lock success\n");
		}
	} else {
		//time lock
		ret = JpgEnterTimeLock(s32TimeOut);
		if (ret != JPG_RET_SUCCESS) {
			//timeout
			CVI_JPG_DBG_IF("JPEG time lock timeout\n");
			return RET_JPG_TIMEOUT;

		} else {
			//lock success
			CVI_JPG_DBG_IF("time lock success\n");
		}
	}

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		ret = cviJpgDecFlush(jpgHandle);
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncFlush(jpgHandle);
	}

	do {
		if (CVIJPGCOD_DEC == pJpgInst->type) {
			ret = cviJpgDecSendFrameData(jpgHandle, data, length);
		} else if (CVIJPGCOD_ENC == pJpgInst->type) {
			ret = cviJpgEncSendFrameData(jpgHandle, data, length);
		}
	} while ((ret == JPG_RET_HWRESET_SUCCESS) && (count++ < 3));

	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("SendFrameData fail, ret %d\n", ret);
		JpgLeaveLock();
	}

	return ret;
}

/* after decoded or encoded, get data from jpu */
int CVIJpgGetFrameData(CVIJpgHandle jpgHandle, void *data, int length,
		       unsigned long int *pu64HwTime)
{
	JpgInst *pJpgInst = jpgHandle;
	int ret = JPG_RET_SUCCESS;

	UNREFERENCED_PARAM(length);

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	if (NULL == jpgHandle) {
		CVI_JPG_DBG_ERR("jpgHandle = NULL\n");
		JpgLeaveLock();
		return -1;
	}

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		ret = cviJpgDecGetFrameData(jpgHandle, data);
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncGetFrameData(jpgHandle, data);
	}
	if (pu64HwTime) {
		*pu64HwTime = pJpgInst->u64EndTime - pJpgInst->u64StartTime;

		CVI_JPG_DBG_PERF("*pu64HwTime = %lu, u64StartTime = %llu, u64EndTime = %llu\n",
				*pu64HwTime, pJpgInst->u64StartTime, pJpgInst->u64EndTime);
	}

	return ret;
}

/* release stream buffer */
int CVIJpgReleaseFrameData(CVIJpgHandle jpgHandle)
{
	int ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = jpgHandle;
	JpgEncInfo *pEncInfo;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	if (NULL == jpgHandle) {
		CVI_JPG_DBG_ERR("jpgHandle = NULL\n");
		JpgLeaveLock();
		return -1;
	}

	pEncInfo = &pJpgInst->JpgInfo.encInfo;
	if (pEncInfo->pFinalStream) {
		MEM_KFREE(pEncInfo->pFinalStream);
		pEncInfo->pFinalStream = NULL;
	}

	// if sharing es buffer, to unlock when releasing frame
	if (jdi_use_single_es_buffer())
		JpgLeaveLock();
	return ret;
}

/* get jpu encoder input data buffer */
int CVIJpgGetInputDataBuf(CVIJpgHandle jpgHandle, void *data, int length)
{
	JpgInst *pJpgInst = jpgHandle;
	int ret = JPG_RET_SUCCESS;

	UNREFERENCED_PARAM(length);

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	if (NULL == jpgHandle) {
		CVI_JPG_DBG_ERR("jpgHandle = NULL\n");
		return -1;
	}

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		CVI_JPG_DBG_ERR("DO NOT SUPPORT DECODER!!\n");
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncGetInputDataBuf(jpgHandle, data);
	}

	return ret;
}

int CVIVidJpuReset(void)
{
	JPU_HWReset();

	return JPG_RET_SUCCESS;
}

int cviJpegSetQuality(CVIJpgHandle jpgHandle, void *data)
{
	int ret = 0;
	int *quality = data;
	JpgInst *pJpgInst;
	JpgEncInfo *pEncInfo;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	// update correct quality to driver
	pEncInfo->openParam.quality = *quality;
	CVI_JPG_DBG_RC("quality = %d\n", pEncInfo->openParam.quality);

	return ret;
}

static int cviJpegSetChnAttr(CVIJpgHandle jpgHandle, void *arg)
{
	JpgInst *pJpgInst;
	JpgEncInfo *pEncInfo;
	cviJpegChnAttr *pChnAttr = (cviJpegChnAttr *)arg;
	int ret = 0;
	unsigned int u32Sec = 0;
	unsigned int u32Frm = 0;

	pJpgInst = jpgHandle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	pEncInfo->openParam.bitrate = pChnAttr->u32BitRate;

	u32Sec = pChnAttr->fr32DstFrameRate >> 16;
	u32Frm = pChnAttr->fr32DstFrameRate & 0xFFFF;

	if (u32Sec == 0) {
		pEncInfo->openParam.framerate = u32Frm;
	} else {
		pEncInfo->openParam.framerate = u32Frm / u32Sec;
	}

	pEncInfo->picWidth = pChnAttr->picWidth;
	pEncInfo->picHeight = pChnAttr->picHeight;

	return ret;
}

static int cviJpegSetMCUPerECS(CVIJpgHandle jpgHandle, void *data)
{
	int ret = 0;
	int *MCUPerECS = data;
	JpgInst *pJpgInst;
	JpgEncInfo *pEncInfo;

	pJpgInst = jpgHandle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	pEncInfo->openParam.restartInterval = *MCUPerECS;
	pEncInfo->rstIntval = pEncInfo->openParam.restartInterval;
	CVI_JPG_DBG_RC("MCUPerECS = %d\n", pEncInfo->openParam.restartInterval);

	return ret;
}

int cviJpegResetChn(CVIJpgHandle jpgHandle, void *data)
{
	int ret = 0;
	JpgInst *pJpgInst;
	JpgEncInfo *pEncInfo;

	UNREFERENCED_PARAM(data);

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	// reset frameIdx since JpgEncEncodeHeader() use frameIdx as header
	pEncInfo->frameIdx = 0;

	// reset quality-related table
	if (CVIJPGCOD_DEC == pJpgInst->type) {
		// do nothing now
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncResetQualityTable(jpgHandle);
	}

	// reset pixel_format setting for alignedWidth/alignedHeight calculation
	pEncInfo->sourceFormat = FORMAT_420;

	return ret;
}

int cviJpegSetUserData(CVIJpgHandle jpgHandle, void *data)
{
	int ret = 0;
	JpgInst *pJpgInst;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;

	if (CVIJPGCOD_DEC == pJpgInst->type) {
		ret = JPG_RET_WRONG_CALL_SEQUENCE;
		CVI_JPG_DBG_ERR("decoder does not support set user data\n");
	} else if (CVIJPGCOD_ENC == pJpgInst->type) {
		ret = cviJpgEncEncodeUserData(jpgHandle, data);
		if (ret != JPG_RET_SUCCESS)
			CVI_JPG_DBG_ERR("cviJpegSetUserData %d\n", ret);
	}

	return ret;
}

int cviJpegStart(CVIJpgHandle jpgHandle, void *data)
{
	int ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;

	if (pJpgInst->type == CVIJPGCOD_DEC) {
		// do nothing now
	} else if (pJpgInst->type == CVIJPGCOD_ENC) {
		ret = cviJpgEncStart(jpgHandle, data);
	}
	return ret;
}

int cviJpegSetSbmEnable(CVIJpgHandle jpgHandle, void *data)
{
	int ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;

	if (pJpgInst->type == CVIJPGCOD_DEC) {
		ret = JPG_RET_FAILURE;
	} else if (pJpgInst->type == CVIJPGCOD_ENC) {
		JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

		pEncInfo->bSbmEn = *(bool *)data;
	}
	return ret;
}

int cviJpegWaitEncodeDone(CVIJpgHandle jpgHandle, void *data)
{
	int ret = JPG_RET_SUCCESS;

	return ret;
}





#ifdef CLI_DEBUG_SUPPORT
extern void cli_show_jdi_info(void);

int cviJpegShowChnInfo(CVIJpgHandle jpgHandle, void *data)
{
	int ret = 0;
	JpgInst *pJpgInst;

	CVI_JPG_DBG_IF("handle = %p\n", jpgHandle);
	UNREFERENCED_PARAM(data);

	ret = CheckJpgInstValidity(jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("CheckJpgInstValidity, %d\n", ret);
		return ret;
	}

	pJpgInst = jpgHandle;

	tcli_print("JpgInst_info:\n");
	tcli_print(" instIndex:%d\n", pJpgInst->instIndex);
	tcli_print(" inUse:%d\n", pJpgInst->inUse);
	tcli_print(" loggingEnable:%d\n", pJpgInst->loggingEnable);
	tcli_print(" u64StartTime:%llu\n", pJpgInst->u64StartTime);
	tcli_print(" u64EndTime:%llu\n", pJpgInst->u64EndTime);
	tcli_print(" inUse:%d\n", pJpgInst->inUse);
	tcli_print(" type:%d\n", pJpgInst->type);

	if (pJpgInst->type == CVIJPGCOD_DEC) {
	} else if (pJpgInst->type == CVIJPGCOD_ENC) {
		JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

		tcli_print("Jpg Encinfo:\n");
		tcli_print("  openParam.bitstreamBufferSize:%d\n",
			   pEncInfo->openParam.bitstreamBufferSize);
		tcli_print("  openParam.picWidth:%d\n",
			   pEncInfo->openParam.picWidth);
		tcli_print("  openParam.picHeight:%d\n",
			   pEncInfo->openParam.picHeight);
		tcli_print("  openParam.sourceFormat:%d\n",
			   pEncInfo->openParam.sourceFormat);
		tcli_print("  openParam.restartInterval:%d\n",
			   pEncInfo->openParam.restartInterval);
		tcli_print("  openParam.streamEndian:%d\n",
			   pEncInfo->openParam.streamEndian);
		tcli_print("  openParam.frameEndian:%d\n",
			   pEncInfo->openParam.frameEndian);
		tcli_print("  openParam.chromaInterleave:%d\n",
			   pEncInfo->openParam.chromaInterleave);
		tcli_print("  openParam.packedFormat:%d\n",
			   pEncInfo->openParam.packedFormat);
		tcli_print("  openParam.rgbPacked:%d\n",
			   pEncInfo->openParam.rgbPacked);
		tcli_print("  openParam.quality:%d\n",
			   pEncInfo->openParam.quality);
		tcli_print("  openParam.bitrate:%d\n",
			   pEncInfo->openParam.bitrate);
		tcli_print("  openParam.framerate:%d\n",
			   pEncInfo->openParam.framerate);
		stRcInfo *pRcInfo = &pEncInfo->openParam.RcInfo;

		tcli_print("   RcInfo.targetBitrate:%d\n",
			   pRcInfo->targetBitrate);
		tcli_print("   RcInfo.fps:%d\n", pRcInfo->fps);
		tcli_print("   RcInfo.picPelNum:%d\n", pRcInfo->picPelNum);
		tcli_print("   RcInfo.picAvgBit:%d\n", pRcInfo->picAvgBit);
		tcli_print("   RcInfo.picTargetBit:%d\n",
			   pRcInfo->picTargetBit);
		tcli_print("   RcInfo.bitErr:%d\n", pRcInfo->bitErr);
		tcli_print("   RcInfo.errCompSize:%d\n", pRcInfo->errCompSize);
		tcli_print("   RcInfo.minQ:%d\n", pRcInfo->minQ);
		tcli_print("   RcInfo.maxQ:%d\n", pRcInfo->maxQ);
		tcli_print("   RcInfo.qClipRange:%d\n", pRcInfo->qClipRange);
		tcli_print("   RcInfo.lastPicQ:%d\n", pRcInfo->lastPicQ);
		tcli_print("   RcInfo.picIdx:%d\n", pRcInfo->picIdx);
		tcli_print("   RcInfo.alpha:%f\n", pRcInfo->alpha);
		tcli_print("   RcInfo.beta:%f\n", pRcInfo->beta);
		tcli_print("   RcInfo.alphaStep:%f\n", pRcInfo->alphaStep);
		tcli_print("   RcInfo.betaStep:%f\n", pRcInfo->betaStep);
		tcli_print("   RcInfo.maxBitrateLimit:%d KB\n",
			   pRcInfo->maxBitrateLimit);

		JpgEncInitialInfo *pInitialInfo = &pEncInfo->initialInfo;

		tcli_print("   initialInfo.minFrameBufferCount:%d\n",
			   pInitialInfo->minFrameBufferCount);
		tcli_print("   initialInfo.colorComponents:%d\n",
			   pInitialInfo->colorComponents);

		tcli_print("  streamBufSize:%d\n", pEncInfo->streamBufSize);
		tcli_print("  pBitStream:%p\n", pEncInfo->pBitStream);

		tcli_print("  streamBufSize:%d\n", pEncInfo->streamBufSize);
		tcli_print("  numFrameBuffers:%d\n", pEncInfo->numFrameBuffers);
		tcli_print("  stride:%d\n", pEncInfo->stride);
		tcli_print("  rotationEnable:%d\n", pEncInfo->rotationEnable);
		tcli_print("  mirrorEnable:%d\n", pEncInfo->mirrorEnable);
		tcli_print("  mirrorDirection:%d\n", pEncInfo->mirrorDirection);
		tcli_print("  rotationAngle:%d\n", pEncInfo->rotationAngle);
		tcli_print("  initialInfoObtained:%d\n",
			   pEncInfo->initialInfoObtained);

		tcli_print("  picWidth:%d\n", pEncInfo->picWidth);
		tcli_print("  picHeight:%d\n", pEncInfo->picHeight);
		tcli_print("  alignedWidth:%d\n", pEncInfo->alignedWidth);
		tcli_print("  alignedHeight:%d\n", pEncInfo->alignedHeight);
		tcli_print("  seqInited:%d\n", pEncInfo->seqInited);
		tcli_print("  frameIdx:%d\n", pEncInfo->frameIdx);
		tcli_print("  sourceFormat:%d\n", pEncInfo->sourceFormat);
		tcli_print("  streamEndian:%d\n", pEncInfo->streamEndian);
		tcli_print("  frameEndian:%d\n", pEncInfo->frameEndian);
		tcli_print("  chromaInterleave:%d\n",
			   pEncInfo->chromaInterleave);
		tcli_print("  rstIntval:%d\n", pEncInfo->rstIntval);

		tcli_print("  busReqNum:%d\n", pEncInfo->busReqNum);
		tcli_print("  mcuBlockNum:%d\n", pEncInfo->mcuBlockNum);
		tcli_print("  compNum:%d\n", pEncInfo->compNum);
		tcli_print("  compInfo:%d %d %d\n", pEncInfo->compInfo[0],
			   pEncInfo->compInfo[1], pEncInfo->compInfo[2]);
		tcli_print("  disableAPPMarker:%d\n",
			   pEncInfo->disableAPPMarker);

		tcli_print("  quantMode:%d\n", pEncInfo->quantMode);
		tcli_print("  stuffByteEnable:%d\n", pEncInfo->stuffByteEnable);
		tcli_print("  usePartial:%d\n", pEncInfo->usePartial);
		tcli_print("  partiallineNum:%d\n", pEncInfo->partiallineNum);

		tcli_print("  partialBufNum:%d\n", pEncInfo->partialBufNum);
		tcli_print("  packedFormat:%d\n", pEncInfo->packedFormat);
		tcli_print("  userDataLen:%d\n", pEncInfo->userDataLen);
		tcli_print("  preStreamLen:%d\n", pEncInfo->preStreamLen);
#ifdef MJPEG_INTERFACE_API
		tcli_print("  framebufStride:%d\n", pEncInfo->framebufStride);
		tcli_print("  encHeaderMode:%d\n", pEncInfo->encHeaderMode);
		tcli_print("  rgbPacked:%d\n", pEncInfo->rgbPacked);
#endif /* MJPEG_INTERFACE_API */

		JpgEncParamSet *paraSet = pEncInfo->paraSet;

		if (paraSet) {
			tcli_print("   paraSet.disableAPPMarker:%d\n",
				   paraSet->disableAPPMarker);
			tcli_print("   paraSet.size:%d\n", paraSet->size);
			tcli_print("   paraSet.headerMode:%d\n",
				   paraSet->headerMode);
			tcli_print("   paraSet.quantMode:%d\n",
				   paraSet->quantMode);
			tcli_print("   paraSet.huffMode:%d\n",
				   paraSet->huffMode);
			tcli_print("   paraSet.rgbPackd:%d\n",
				   paraSet->rgbPackd);
		}
	}

	cli_show_jdi_info();

	return ret;
}

#else
int cviJpegShowChnInfo(CVIJpgHandle jpgHandle, void *data)
{
	UNREFERENCED_PARAM(jpgHandle);
	UNREFERENCED_PARAM(data);

	return 0;
}

#endif

typedef struct _CVI_JPEG_IOCTL_OP_ {
	int opNum;
	int (*ioctlFunc)(CVIJpgHandle jpgHandle, void *arg);
} CVI_JPEG_IOCTL_OP;

CVI_JPEG_IOCTL_OP cviJpegIoctlOp[] = {
	{ CVI_JPEG_OP_NONE, NULL },
	{ CVI_JPEG_OP_SET_QUALITY, cviJpegSetQuality },
	{ CVI_JPEG_OP_SET_CHN_ATTR, cviJpegSetChnAttr },
	{ CVI_JPEG_OP_SET_MCUPerECS, cviJpegSetMCUPerECS },
	{ CVI_JPEG_OP_RESET_CHN, cviJpegResetChn },
	{ CVI_JPEG_OP_SET_USER_DATA, cviJpegSetUserData },
	{ CVI_JPEG_OP_SHOW_CHN_INFO, cviJpegShowChnInfo },
	{ CVI_JPEG_OP_START, cviJpegStart },
	{ CVI_JPEG_OP_SET_SBM_ENABLE, cviJpegSetSbmEnable },
	{ CVI_JPEG_OP_WAIT_FRAME_DONE, cviJpegWaitEncodeDone },
};

int cviJpegIoctl(void *handle, int op, void *arg)
{
	CVIJpgHandle jpgHandle = (CVIJpgHandle)handle;
	int ret = 0;
	int currOp;

	CVI_JPG_DBG_IF("\n");

	if (op <= 0 || op >= CVI_JPEG_OP_MAX) {
		CVI_JPG_DBG_ERR("op = %d\n", op);
		return -1;
	}

	currOp = (cviJpegIoctlOp[op].opNum & CVI_JPEG_OP_MASK) >>
		 CVI_JPEG_OP_SHIFT;
	if (op != currOp) {
		CVI_JPG_DBG_ERR("op = %d\n", op);
		return -1;
	}

	ret = cviJpegIoctlOp[op].ioctlFunc(jpgHandle, arg);

	return ret;
}
