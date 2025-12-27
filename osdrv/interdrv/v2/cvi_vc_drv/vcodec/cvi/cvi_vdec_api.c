/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: July, 2020
 */
#ifdef ENABLE_DEC
#include "vdi_osal.h"
#include "cvi_dec_internal.h"
#include "product.h"
#include "cvi_h265_interface.h"

#define IS_VALID_PA(pa) (((pa) != 0) && ((pa) != (PhysicalAddress)(-1)))

static int _cviVDecOpen(cviInitDecConfig *pInitDecCfg, void *pHandle);

int cviVDecOpen(cviInitDecConfig *pInitDecCfg, void *pHandle)
{
	cviVideoDecoder **ppvdec = (cviVideoDecoder **)pHandle;
	CVI_DEC_STATUS decStatus;
	Uint32 core_idx =
		pInitDecCfg->codec == CODEC_H265 ? CORE_H265 : CORE_H264;

	decStatus = cviVcodecInit();
	if (decStatus < 0) {
		CVI_VC_INFO("cviVcodecInit, %d\n", decStatus);
	}

	CVI_VC_IF("\n");

	decStatus = _cviVDecOpen(pInitDecCfg, pHandle);
	if (decStatus < 0) {
		CVI_VC_ERR("_cviVDecOpen, %d\n", decStatus);
	}

	if (decStatus == CVI_ERR_ALLOC_VDEC) {
		EnterVcodecLock(core_idx);
		goto CVI_VDEC_API_ERR_ALLOC_VDEC;
	} else if (decStatus == CVI_ERR_DEC_ARGV) {
		EnterVcodecLock(core_idx);
		goto CVI_VDEC_API_ARGV;
	} else if (decStatus == CVI_ERR_DEC_MCU) {
		EnterVcodecLock(core_idx);
		goto CVI_VDEC_API_MCU;
	} else if (decStatus == CVI_ERR_DEC_INIT) {
		EnterVcodecLock(core_idx);
		goto CVI_VDEC_API_INIT;
	} else if (decStatus == CVI_ERR_DECODE_END) {
		EnterVcodecLock(core_idx);
		goto CVI_VDEC_API_DECODE_END;
	}

	return decStatus;

CVI_VDEC_API_DECODE_END:
	cviCloseDecoder(*ppvdec);
CVI_VDEC_API_INIT:
	cviDeInitDecoder(*ppvdec);
CVI_VDEC_API_MCU:
	cviFreeDecMcuEnv(*ppvdec);
CVI_VDEC_API_ARGV:
	cviFreeVideoDecoder(ppvdec);
CVI_VDEC_API_ERR_ALLOC_VDEC:

	LeaveVcodecLock(core_idx);

	return decStatus;
}

static int _cviVDecOpen(cviInitDecConfig *pInitDecCfg, void *pHandle)
{
	cviVideoDecoder **ppvdec = (cviVideoDecoder **)pHandle;
	cviVideoDecoder *pvdec;
	cviDecConfig *pdeccfg;
	CVI_DEC_STATUS decStatus;
	int ret;
	Uint32 core_idx =
		pInitDecCfg->codec == CODEC_H265 ? CORE_H265 : CORE_H264;

	*ppvdec = cviAllocVideoDecoder();
	if (!(*ppvdec)) {
		CVI_VC_ERR("cviAllocVideoDecoder\n");
		return CVI_ERR_ALLOC_VDEC;
	}
	pvdec = *ppvdec;

	pdeccfg = &pvdec->decConfig;
	pvdec->cviApiMode = pInitDecCfg->cviApiMode;
	pvdec->chnNum = pInitDecCfg->chnNum;

	if (pInitDecCfg->codec == CODEC_H265)
		pdeccfg->bitFormat = STD_HEVC;
	else if (pInitDecCfg->codec == CODEC_H264)
		pdeccfg->bitFormat = STD_AVC;
#ifdef VC_DRIVER_TEST
	if (pvdec->cviApiMode == API_MODE_DRIVER) {

		ret = parseVdecArgs(pInitDecCfg->argc, pInitDecCfg->argv, pdeccfg);
		if (ret < 0) {
			CVI_VC_ERR("parseArgs, %d\n", ret);
			return CVI_ERR_DEC_ARGV;
		}

	} else
#endif
	{
		pdeccfg->bsSize = pInitDecCfg->bsBufferSize;
	}

	pvdec->eVdecVBSource = pInitDecCfg->vbSrcMode;
	pvdec->ReorderEnable = pInitDecCfg->reorderEnable;
	initDefaultcDecConfig(pvdec);

	ret = cviInitDecMcuEnv(pdeccfg);
	if (ret < 0) {
		CVI_VC_ERR("cviInitDecMcuEnv, ret = %d\n", ret);
		return CVI_ERR_DEC_MCU;
	}

	checkDecConfig(pdeccfg);

	EnterVcodecLock(core_idx);
	decStatus = cviInitDecoder(pvdec);
	LeaveVcodecLock(core_idx);

	if (decStatus == CVI_ERR_DEC_INIT) {
		CVI_VC_ERR("CVI_ERR_DEC_INIT\n");
		return CVI_ERR_DEC_INIT;
	} else if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("CVI_ERR_DECODE_END\n");
		return CVI_ERR_DECODE_END;
	} else if (decStatus < 0) {
		CVI_VC_ERR("decStatus = %d\n", decStatus);
		return decStatus;
	}

	pvdec->success = FALSE;

	return 0;
}

int cviVDecClose(void *pHandle)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;
	Uint32 core_idx = pvdec->decConfig.coreIdx;

	CVI_VC_IF("\n");

	EnterVcodecLock(core_idx);

	cviCloseDecoder(pvdec);
	cviDeInitDecoder(pvdec);
	cviFreeDecMcuEnv(pvdec);
	cviFreeVideoDecoder(&pvdec);

	LeaveVcodecLock(core_idx);

	return 0;
}

static int cviVDecLock(int core_idx, int timeout_ms)
{
	int err;
	int ret = RETCODE_SUCCESS;

	if (timeout_ms <= -1) {
		//block mode
		EnterVcodecLock(core_idx);
	} else {
		//time lock mode
		err = VcodecTimeLock(core_idx, timeout_ms);
		if (err != RETCODE_SUCCESS) {
			if (err == RETCODE_VPU_RESPONSE_TIMEOUT) {
				//timeout
				ret = RETCODE_VPU_RESPONSE_TIMEOUT;
			} else {
				//never been lock success
				CVI_VC_ERR("VcodecTimeLock abnormal ret:%d\n",
					   ret);
				ret = RETCODE_FAILURE;
			}
		}
	}
	return ret;
}

int cviVDecReset(void *pHandle)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;
	Uint32 core_idx = pvdec->decConfig.coreIdx;
	CVI_DEC_STATUS decStatus;
	cviDecConfig *pdeccfg;
	Int32 cviApiMode;
	Int32 chnNum;
	E_CVI_VB_SOURCE eVdecVBSource;
	Int32 bitFormat;
	size_t bsSize;
	Uint32 sizeInWord;
	Uint16 *pusBitCode;
	Int32 ReorderEnable;

	CVI_VC_IF("\n");

	EnterVcodecLock(core_idx);

	// record previous settings
	cviApiMode = pvdec->cviApiMode;
	chnNum = pvdec->chnNum;
	eVdecVBSource = pvdec->eVdecVBSource;

	pdeccfg = &pvdec->decConfig;
	bitFormat = pdeccfg->bitFormat;
	bsSize = pdeccfg->bsSize;
	sizeInWord = pdeccfg->sizeInWord;
	pusBitCode = pdeccfg->pusBitCode;
	ReorderEnable = pdeccfg->reorderEnable;

	cviCloseDecoder(pvdec);
	cviDeInitDecoder(pvdec);

	memset(pvdec, 0, sizeof(cviVideoDecoder));

	pvdec->cviApiMode = cviApiMode;
	pvdec->chnNum = chnNum;
	pvdec->eVdecVBSource = eVdecVBSource;
	pvdec->ReorderEnable = ReorderEnable;

	pdeccfg->bitFormat = bitFormat;
	pdeccfg->bsSize = bsSize;

	initDefaultcDecConfig(pvdec);

	pdeccfg->coreIdx = core_idx;
	pdeccfg->sizeInWord = sizeInWord;
	pdeccfg->pusBitCode = pusBitCode;

	checkDecConfig(pdeccfg);

	decStatus = cviInitDecoder(pvdec);
	if (decStatus < 0) {
		CVI_VC_ERR("decStatus = %d\n", decStatus);
		LeaveVcodecLock(core_idx);
		return decStatus;
	}

	LeaveVcodecLock(core_idx);

	return 0;
}

int cviVDecDecOnePic(void *pHandle, cviDecOnePicCfg *pdopc, int timeout_ms)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	CVI_DEC_STATUS decStatus;
	int ret;

	CVI_VC_IF("\n");
	//EnterVcodecLock(pdeccfg->coreIdx);
	ret = cviVDecLock(pdeccfg->coreIdx, timeout_ms);

	if (ret != RETCODE_SUCCESS) {
		CVI_VC_TRACE("cviVDecLock, ret = %d\n", ret);
		if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
			return CVI_VDEC_RET_LOCK_TIMEOUT;
			//timeout return
		} else {
			CVI_VC_ERR("cviVDecLock  error[%d]timeout_ms[%d]\n",
				   ret, timeout_ms);
			return CVI_VDEC_RET_STOP;
		}
	}

	if (pvdec->cviApiMode == API_MODE_SDK) {
		cviSdkParam *psdkp = &pvdec->sdkParam;

		if (!pdopc) {
			CVI_VC_ERR("pdopc NULL, CVI_ERR_DECODE_END\n");
			decStatus = CVI_ERR_DECODE_END;
			goto ERR_CVI_VDEC_DEC_ONEPIC;
		}

		psdkp->bsBuf.virt_addr = pdopc->bsAddr;
		psdkp->bsBuf.size = pdopc->bsLen;
		pvdec->bStreamOfEnd = pdopc->bEndOfStream;
		pdeccfg->cbcrInterleave = pdopc->cbcrInterleave;
		pdeccfg->nv21 = pdopc->nv21;

		CVI_VC_BS("virt_addr = %p, size = %d, bStreamOfEnd = %d\n",
			  psdkp->bsBuf.virt_addr, psdkp->bsBuf.size,
			  pvdec->bStreamOfEnd);
	}

	if (pvdec->frameNum == 0 && pvdec->seqInited == FALSE) {
		decStatus = cviInitSeqSetting(pvdec);
		if (decStatus == CVI_ERR_DECODE_END) {
			CVI_VC_ERR("CVI_ERR_DECODE_END\n");
			goto ERR_CVI_VDEC_DEC_ONEPIC;
		} else {
			CVI_VC_TRACE("decStatus = %d\n", decStatus);
		}
	}

SEQ_CHANGE_FLUSH:
	decStatus = cviDecodeOneFrame(pvdec);
	if (decStatus == CVI_DECODE_CONTINUE) {
		CVI_VC_TRACE("CVI_DECODE_CONTINUE\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("CVI_ERR_DECODE_END\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_DECODE_NO_FB) {
		CVI_VC_TRACE("CVI_DECODE_NO_FB\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else {
		CVI_VC_TRACE("decStatus = %d\n", decStatus);
	}

	decStatus = cviWaitInterrupt(pvdec);
	if (decStatus == CVI_DECODE_CONTINUE) {
		CVI_VC_TRACE("CVI_DECODE_CONTINUE\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("CVI_ERR_DECODE_END\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_WAIT_INT_OK) {
		CVI_VC_TRACE("CVI_WAIT_INT_OK\n");
	} else {
		CVI_VC_TRACE("decStatus = %d\n", decStatus);
	}

	decStatus = cviGetDecodedData(pvdec);
	if (decStatus == CVI_DECODE_CONTINUE) {
		CVI_VC_TRACE("CVI_DECODE_CONTINUE\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_DECODE_BREAK) {
		CVI_VC_TRACE("CVI_DECODE_BREAK\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("CVI_ERR_DECODE_END\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_DECODE_NO_FB) {
		CVI_VC_TRACE("CVI_DECODE_NO_FB\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_DECODE_NO_FB_WITH_DISP) {
		CVI_VC_TRACE("CVI_DECODE_NO_FB_WITH_DISP\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	} else if (decStatus == CVI_SEQ_CHANGE) {
		CVI_VC_TRACE("CVI_SEQ_CHANGE\n");
		goto SEQ_CHANGE_FLUSH;
	} else if (decStatus == CVI_SEQ_CHG_FLUSH) {
		CVI_VC_TRACE("CVI_SEQ_CHG_FLUSH\n");
		goto SEQ_CHANGE_FLUSH;
	} else if (decStatus == CVI_SEQ_CHG_WAIT_BUF_FREE) {
		CVI_VC_TRACE("CVI_SEQ_CHG_WAIT_BUF_FREE\n");
		goto ERR_CVI_VDEC_DEC_ONEPIC;
	}

	pvdec->frameNum++;

ERR_CVI_VDEC_DEC_ONEPIC:
	switch (decStatus) {
	case CVI_DECODE_CONTINUE:
		ret = CVI_VDEC_RET_CONTI;
		break;
	case CVI_ERR_DECODE_END:
		ret = CVI_VDEC_RET_DEC_ERR;
		break;
	case CVI_DECODE_BREAK:
		ret = CVI_VDEC_RET_STOP;
		break;
	case CVI_DECODE_NO_FB:
	case CVI_SEQ_CHG_WAIT_BUF_FREE:
		ret = CVI_VDEC_RET_NO_FB;
		break;
	case CVI_DECODE_NO_FB_WITH_DISP:
		ret = CVI_VDEC_RET_NO_FB_WITH_DISP;
		break;
	case CVI_DECODE_DATA_OK:
		ret = CVI_VDEC_RET_FRM_DONE;
		break;
	case CVI_DISP_LAST_FRM:
		ret = CVI_VDEC_RET_LAST_FRM;
		break;
	default:
		ret = CVI_DECODE_CONTINUE;
		break;
	}

	LeaveVcodecLock(pdeccfg->coreIdx);

	return ret;
}

int cviVDecGetFrame(void *pHandle, cviDispFrameCfg *pdfc)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;
	DecOutputInfo outputInfo, *pdoi = &outputInfo;
	FrameBuffer *fb;
	void *pDispInfo;

	CVI_VC_IF("\n");

	pDispInfo = cviGetDispFrame(pvdec->renderer, pdoi);

	if (!pDispInfo) {
		CVI_VC_WARN("no frame\n");
		return -1;
	}

	fb = &pdoi->dispFrame;
	pdfc->phyAddrY = fb->bufY;
	pdfc->phyAddrCb = fb->bufCb;

	if (IS_VALID_PA(fb->bufCr)) {
		pdfc->phyAddrCr = fb->bufCr;
	} else {
		pdfc->phyAddrCr = 0;
		pdfc->addrCr = NULL;
	}

	pdfc->width = pvdec->sequenceInfo.picCropRect.right;
	pdfc->height = pvdec->sequenceInfo.picCropRect.bottom;

	pdfc->strideY = fb->stride;
	pdfc->strideC = fb->cbcrInterleave ? fb->stride : (fb->stride >> 1);
	pdfc->cbcrInterleave = fb->cbcrInterleave;
	pdfc->nv21 = fb->nv21;
	pdfc->indexFrameDisplay = pdoi->indexFrameDisplay;
	CVI_VC_DISP("width = %d, height = %d\n", fb->width, fb->height);
	pdfc->decHwTime = pdoi->decHwTime;

	return 0;
}

void cviVDecReleaseFrame(void *pHandle, void *arg)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;

	CVI_VC_IF("\n");

	cviReleaseDispFrameSDK(pvdec->renderer, arg);
}

void cviVDecAttachFrmBuf(void *pHandle, void *pFrmBufArray, int nFrmNum)
{
	cviVideoDecoder *pvdec = (cviVideoDecoder *)pHandle;

	CVI_VC_IF("\n");

	cviAttachFrmBuf(pvdec, pFrmBufArray, nFrmNum);
}

void cviVDecAttachCallBack(CVI_VDEC_DRV_CALLBACK pCbFunc)
{
	if (pCbFunc == NULL) {
		CVI_VC_ERR("pCbFunc == NULL\n");
		return;
	}

	cviDecAttachCallBack(pCbFunc);
}
#endif
