#include "cvi_enc_rc.h"
#include "cvi_vcom.h"

//
/*
typedef enum _RC_MODE_ {
	RC_MODE_CBR = 0,
	RC_MODE_VBR,
	RC_MODE_AVBR,
	RC_MODE_QVBR,
	RC_MODE_FIXQP,
	RC_MODE_QPMAP,
	RC_MODE_MAX,
} RC_MODE;
*/
#define CVI_RC_MIN_I_PROP		1
#define CVI_RC_MDL_UPDATE_TYPE	0
#define CVI_RC_DEF_STAT_TIME	2

static void cviEncRc_RcKernelInit(stRcInfo *pRcInfo, EncOpenParam *pEncOP);
static void cviEncRc_RcKernelUpdatePic(stRcInfo *pRcInfo, EncOutputInfo *pEncOutInfo);

static void set_pic_qp_by_delta(stRcInfo *pRcInfo, EncOpenParam *pEncOP,
				int delta)
{
	pRcInfo->picIMaxQp = (pEncOP->userQpMaxI > 0) ?
					   CLIP3(0, 51, pEncOP->userQpMaxI - delta) :
					   51;
	pRcInfo->picIMinQp = (pEncOP->userQpMinI > 0) ?
					   CLIP3(0, 51, pEncOP->userQpMinI + delta) :
					   0;
	pRcInfo->picPMaxQp = (pEncOP->userQpMaxP > 0) ?
					   CLIP3(0, 51, pEncOP->userQpMaxP - delta) :
					   51;
	pRcInfo->picPMinQp = (pEncOP->userQpMinP > 0) ?
					   CLIP3(0, 51, pEncOP->userQpMinP + delta) :
					   0;
}

// ---------------------------------------------------------
int _cviEncRc_getBitrate(stRcInfo *pRcInfo)
{
	if (!pRcInfo->rcEnable) {
		return 0;
	}
	return (pRcInfo->rcMode == RC_MODE_CBR ||
		pRcInfo->rcMode == RC_MODE_UBR) ?
			     pRcInfo->targetBitrate :
			     pRcInfo->maximumBitrate;
}

static void _cviEncRc_setBitrateParam(stRcInfo *pRcInfo, EncOpenParam *pEncOP)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	int frameRateDiv, frameRateRes;

	// VBR
	if (pRcInfo->rcMode == RC_MODE_VBR) {
		pRcInfo->convergStateBufThr = pEncOP->bitRate * 1000;
		pRcInfo->targetBitrate =
			(pEncOP->bitRate * pEncOP->changePos) / 100;
		pRcInfo->maximumBitrate = pEncOP->bitRate;
	} else if (pRcInfo->rcMode == RC_MODE_AVBR) {
		int minPercent = pEncOP->minStillPercent;
		int motLv = pRcInfo->periodMotionLv;
		int motBitRatio = 0, bitrateByMotLv = 0;

		if (pEncOP->svc_enable && pEncOP->complex_scene_detect_en) {
			minPercent = pRcInfo->periodDciLv <=
						 pEncOP->complex_scene_low_th ?
						 minPercent : pEncOP->middle_min_percent;
			minPercent = pRcInfo->periodDciLv >=
						 pEncOP->complex_scene_hight_th ?
						 pEncOP->complex_min_percent : minPercent;
		}

		pRcInfo->lastMinPercent = minPercent;
		motBitRatio =
			(((100 - minPercent) * (motLv)) / 255) + minPercent;
		bitrateByMotLv = (pEncOP->bitRate * motBitRatio) / 100;
		//bitrateByMotLv = (pRcInfo->avbrLastPeriodBitrate * 0.25) + (bitrateByMotLv * 0.75);
		pRcInfo->avbrLastPeriodBitrate = bitrateByMotLv;
		pRcInfo->convergStateBufThr = bitrateByMotLv * 1000;
		pRcInfo->targetBitrate =
			(bitrateByMotLv * pEncOP->changePos) / 100;
		pRcInfo->maximumBitrate = pEncOP->bitRate;
		CVI_VC_INFO("minPercent %d new targetBitrate = %d periodDciLv %d\n",
			   minPercent, pRcInfo->targetBitrate, pRcInfo->periodDciLv);
	} else {
		pRcInfo->targetBitrate = pEncOP->bitRate;
	}

	frameRateDiv = (pEncOP->frameRateInfo >> 16) + 1;
	frameRateRes = pEncOP->frameRateInfo & 0xFFFF;

	if (pRcInfo->cviRcEn) {
		// TODO: use soft-floating to replace it
		pRcInfo->picAvgBit = (int)(pRcInfo->targetBitrate * 1000 *
				frameRateDiv / frameRateRes);
	} else {
		CVI_VC_RC("cviRcEn = %d\n", pRcInfo->cviRcEn);
	}

	CVI_VC_RC("gopSize = %d, maxIprop = %d\n", pEncOP->gopSize,
		  pEncOP->maxIprop);

	if ((pRcInfo->codec == STD_AVC || pRcInfo->rcMode == RC_MODE_UBR) &&
	    pEncOP->maxIprop > 0 && pEncOP->gopSize > 1) {
		if (pRcInfo->cviRcEn) {
			pRcInfo->maxIPicBit =
				(int)(pEncOP->maxIprop * pEncOP->gopSize *
						pRcInfo->targetBitrate * 1000 * frameRateDiv /
						frameRateRes /
						(pEncOP->maxIprop + pEncOP->gopSize - 1));
		} else {
			CVI_VC_RC("cviRcEn = %d\n", pRcInfo->cviRcEn);
		}
		CVI_VC_RC("maxIPicBit = %d\n", pRcInfo->maxIPicBit);
	}

	if (pRcInfo->cviRcEn) {
		RC_Float frameRate = CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(frameRateRes),
				INT_TO_CVI_FLOAT(frameRateDiv));

		if (vcodec_mask & CVI_MASK_CVRC) {
			CVI_VCOM_FLOAT("bitRate = %d, frameRate = %f\n", pEncOP->bitRate, getFloat(frameRate));
		}
		cviRcKernel_setBitrateAndFrameRate(pRcKerInfo,
				pRcInfo->targetBitrate * 1000, frameRate);
	}
}

int _cviEncRc_getFramerate(stRcInfo *pRcInfo)
{
	if (!pRcInfo->rcEnable) {
		return 0;
	}

	return pRcInfo->framerate;
}

static void _cviEncRc_setFramerateParam(stRcInfo *pRcInfo, EncOpenParam *pEncOP)
{
	pRcInfo->framerate = pEncOP->frameRateInfo;
	CVI_VC_CFG("framerate = %d\n", pRcInfo->framerate);
}

int cviEncRc_GetParam(stRcInfo *pRcInfo, eRcParam eParam)
{
	int ret = 0;

	if (!pRcInfo->rcEnable) {
		return ret;
	}

	switch (eParam) {
	case E_BITRATE:
		ret = _cviEncRc_getBitrate(pRcInfo);
		break;
	case E_FRAMERATE:
		ret = _cviEncRc_getFramerate(pRcInfo);
		break;
	default:
		break;
	}

	return ret;
}

int cviEncRc_SetParam(stRcInfo *pRcInfo, EncOpenParam *pEncOP, eRcParam eParam)
{
	int ret = 0;

	if (!pRcInfo->rcEnable) {
		return ret;
	}

	switch (eParam) {
	case E_BITRATE:
		_cviEncRc_setBitrateParam(pRcInfo, pEncOP);
		ret = 1;
		break;
	case E_FRAMERATE:
		_cviEncRc_setFramerateParam(pRcInfo, pEncOP);
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

static int calc_avg_motionLv(stRcInfo *pRcInfo)
{
	int winSize = pRcInfo->bitrateChangePeriod;
	// trimmed average
	/*
	int idx = 0, motionWin[AVBR_MAX_BITRATE_WIN];
	for(idx=0; idx<winSize; idx++) {
      motionWin[idx] = pRcInfo->picMotionLvWindow[idx];
	}
	insertion_sort(motionWin, winSize);
	int startIdx = (winSize >> 2);
	int endIdx = startIdx + ((winSize)>>1);
	int accum = 0;
	for(idx=startIdx; idx<endIdx; idx++) {
		accum += motionWin[idx % winSize];
	}
	return CLIP3(0, 255, (accum / (winSize >> 1)));
	*/
	int accum = 0, idx = 0;
	for (idx = 0; idx < winSize; idx++) {
		accum += pRcInfo->picMotionLvWindow[idx % winSize];
	}
	return CLIP3(0, 255, (accum / winSize));
}

static int calc_avg_dcilv(stRcInfo *pRcInfo)
{
	int winSize = pRcInfo->bitrateChangePeriod;
	int accum = 0, idx = 0;

	for (idx = 0; idx < winSize; idx++)
		accum += pRcInfo->picDciLvWindow[idx % winSize];
	return accum / winSize;
}

BOOL cviEnc_Avbr_PicCtrl(stRcInfo *pRcInfo, EncOpenParam *pEncOP, int frameIdx)
{
	BOOL changeBrEn = FALSE;
	int winSize, coring_offset, coring_in, coring_out, coring_gain;

	if (pRcInfo->rcMode != RC_MODE_AVBR) {
		return FALSE;
	}
	winSize = pRcInfo->bitrateChangePeriod;
	coring_offset = pEncOP->pureStillThr;
	coring_in = pEncOP->picMotionLevel;
	coring_out = CLIP3(0, 255, (coring_in - coring_offset));
	CVI_VC_INFO("pic mot in = %d, mot lv = %d\n", coring_in, coring_out);

	pEncOP->picMotionLevel = coring_out;
	pRcInfo->picMotionLvWindow[frameIdx % winSize] = pEncOP->picMotionLevel;
	pRcInfo->picDciLvWindow[frameIdx % winSize] = pEncOP->picDciLv;
	pRcInfo->avbrChangeBrEn = FALSE;
	coring_gain = pEncOP->motionSensitivy;
	pRcInfo->lastPeriodDciLv = pRcInfo->periodDciLv;
	pRcInfo->periodDciLv = calc_avg_dcilv(pRcInfo);
	pRcInfo->periodMotionLvRaw = calc_avg_motionLv(pRcInfo);
	pRcInfo->periodMotionLv =
		CLIP3(0, 255, (pRcInfo->periodMotionLvRaw * coring_gain) / 10);
	CVI_VC_INFO("periodMotionLvRaw = %d periodMotionLv = %d\n",
		    pRcInfo->periodMotionLvRaw, pRcInfo->periodMotionLv);
	pRcInfo->avbrChangeValidCnt =
		(pRcInfo->avbrChangeValidCnt > 0) ?
			      (pRcInfo->avbrChangeValidCnt - 1) :
			      0;
	CVI_VC_INFO("avbrChangeValidCnt = %d\n", pRcInfo->avbrChangeValidCnt);
	if (pRcInfo->avbrChangeValidCnt == 0) {
		int MotionLvLower;
		// quantize motion diff to avoid too frequently bitrate change
		int diffMotLv = (pRcInfo->lastPeriodMotionLv >
				 pRcInfo->periodMotionLv) ?
					      (pRcInfo->lastPeriodMotionLv -
					 pRcInfo->periodMotionLv) >>
						4 :
					      (pRcInfo->periodMotionLv -
					 pRcInfo->lastPeriodMotionLv) >>
						4;
		int diffDciLv = abs(pRcInfo->lastPeriodDciLv - pRcInfo->periodDciLv);

		changeBrEn =
			(diffMotLv >= 1) || (pRcInfo->periodMotionLv == 0 &&
					     pRcInfo->lastPeriodMotionLv > 0 &&
					     pRcInfo->lastPeriodMotionLv <= 16) ||
						 (diffDciLv >= 80);
		MotionLvLower = CLIP3(
			0, 255, pRcInfo->lastPeriodMotionLv - MOT_LOWER_THR);
		pRcInfo->periodMotionLv =
			MAX(MotionLvLower, pRcInfo->periodMotionLv);

		if (changeBrEn) {
			pRcInfo->lastPeriodMotionLv = pRcInfo->periodMotionLv;
			pRcInfo->avbrChangeBrEn = TRUE;
			pRcInfo->avbrChangeValidCnt = MAX(
				20, pRcInfo->framerate); //AVBR_MAX_BITRATE_WIN;
		}
		//CVI_VC_INFO("avbr bitrate = %d\n",  pRcInfo->targetBitrate);
	}
	return (changeBrEn) ? TRUE : FALSE;
}

int cviEncRc_Avbr_GetQpDelta(stRcInfo *pRcInfo, EncOpenParam *pEncOP)
{
	int qDelta = 0;

	if ((!pRcInfo->rcEnable) || (pRcInfo->rcMode != RC_MODE_AVBR)) {
		return 0;
	}

	do {
		int qDiff = 0;
		int stillQp = pEncOP->maxStillQp;
		int maxQp = (pEncOP->userQpMaxI + pEncOP->userQpMaxP) >> 1;
		int ratio = CLIP3(0, 64, 64 - pRcInfo->periodMotionLvRaw);
		if (pRcInfo->svcEnable)
			qDiff = MIN(0, stillQp - pEncOP->userQpMaxI);
		else
			qDiff = MIN(0, stillQp - maxQp);

		qDelta = (ratio * qDiff) >> 6;
		pRcInfo->qDelta = qDelta;
	} while (0);
	return pRcInfo->svcEnable ? 0 : qDelta;
}

BOOL cviEncRc_Avbr_CheckFrameSkip(stRcInfo *pRcInfo, EncOpenParam *pEncOP,
				  int isIFrame)
{
	BOOL isFrameSkip = FALSE;
	BOOL isStillPic = TRUE;
	int idx;

	if ((!pRcInfo->rcEnable) || (pRcInfo->rcMode != RC_MODE_AVBR) ||
	    pRcInfo->avbrContiSkipNum == 0) {
		return FALSE;
	}

	for (idx = 0; idx < pRcInfo->bitrateChangePeriod; idx++) {
		if (pRcInfo->picMotionLvWindow[idx] > 0) {
			isStillPic = FALSE;
			break;
		}
	}

	if (isStillPic && pRcInfo->frameSkipCnt < pRcInfo->avbrContiSkipNum &&
	    !isIFrame) {
		isFrameSkip = TRUE;
		pRcInfo->frameSkipCnt++;
	} else {
		isFrameSkip = FALSE;
		pRcInfo->frameSkipCnt = 0;
	}
	CVI_VC_INFO("FrameSkipByStillPic: %d %d %d\n", isFrameSkip,
		    pEncOP->picMotionLevel, pRcInfo->periodMotionLv);
	return isFrameSkip;
}

void cviEncRc_Open(stRcInfo *pRcInfo, EncOpenParam *pEncOP)
{
	pRcInfo->rcEnable = pEncOP->rcEnable;
	pRcInfo->cviRcEn = pEncOP->cviRcEn;
	pRcInfo->svcEnable = pEncOP->svc_enable;
	if (!pRcInfo->rcEnable) {
		return;
	}

	pRcInfo->codec = pEncOP->bitstreamFormat;

	// params setting
	pRcInfo->rcMode = pEncOP->rcMode;
	pRcInfo->numOfPixel = pEncOP->picWidth * pEncOP->picHeight;
	// frame skipping
	pRcInfo->contiSkipNum = (!pEncOP->frmLostOpen)	  ? 0 :
				(pEncOP->encFrmGaps == 0) ? 65535 :
								  pEncOP->encFrmGaps;
	pRcInfo->frameSkipBufThr = pEncOP->frmLostBpsThr;

	// avbr setting
	if (pRcInfo->rcMode == RC_MODE_AVBR) {
		int basePeriod, idx;
		for (basePeriod = AVBR_MAX_BITRATE_WIN; basePeriod > 0;
		     basePeriod--) {
			if (pEncOP->gopSize % basePeriod == 0) {
				pRcInfo->bitrateChangePeriod = basePeriod;
				break;
			}
		}
		for (idx = 0; idx < AVBR_MAX_BITRATE_WIN; idx++) {
			pRcInfo->picMotionLvWindow[idx] = MOT_LV_DEFAULT;
		}
	}
	pRcInfo->periodMotionLv = MOT_LV_DEFAULT;
	pRcInfo->lastPeriodMotionLv = MOT_LV_DEFAULT;
	pRcInfo->avbrContiSkipNum =
		(!pEncOP->avbrFrmLostOpen) ? 0 :
		(pEncOP->avbrFrmGaps == 0) ? 65535 :
						   pEncOP->avbrFrmGaps;

	pRcInfo->maxIPicBit = -1;
	// bitrate
	cviEncRc_SetParam(pRcInfo, pEncOP, E_BITRATE);
	cviEncRc_SetParam(pRcInfo, pEncOP, E_FRAMERATE);

	// pic min/max qp
	pRcInfo->picMinMaxQpClipRange = 0;
	set_pic_qp_by_delta(pRcInfo, pEncOP, pRcInfo->picMinMaxQpClipRange);

	// initial value setup
	pRcInfo->frameSkipCnt = 0;
	pRcInfo->bitrateBuffer =
		pRcInfo->targetBitrate * 400; // initial buffer level
	pRcInfo->ConvergenceState = 0;
	pRcInfo->rcState = STEADY;
	pRcInfo->avbrLastPeriodBitrate = pRcInfo->targetBitrate;
	pRcInfo->avbrChangeBrEn = FALSE;
	pRcInfo->isLastPicI = FALSE;
	pRcInfo->avbrChangeValidCnt = 20;
	pRcInfo->periodMotionLvRaw = 32;
	pRcInfo->s32HrdBufLevel = pRcInfo->targetBitrate * pEncOP->rcInitDelay / 4;

// I frame RQ model for max I frame size constraint

	CVI_VC_INFO("rcEnable = %d\n", pRcInfo->rcEnable);
	CVI_VC_INFO("rc mode = %d\n", pRcInfo->rcMode);
	CVI_VC_INFO("targetBitrate = %dk\n", pRcInfo->targetBitrate);
	CVI_VC_INFO("frame rate = %d\n", pEncOP->frameRateInfo);
	CVI_VC_INFO("picAvgBit = %d\n", pRcInfo->picAvgBit);
	CVI_VC_INFO("convergStateBufThr = %d\n", pRcInfo->convergStateBufThr);
	if (pRcInfo->rcMode == RC_MODE_AVBR) {
		CVI_VC_INFO("bitrateChangePeriod = %d\n",
			    pRcInfo->bitrateChangePeriod);
		CVI_VC_INFO("motionSensitivy = %d\n", pEncOP->motionSensitivy);
		CVI_VC_INFO("minStillPercent = %d\n", pEncOP->minStillPercent);
		CVI_VC_INFO("maxStillQp = %d\n", pEncOP->maxStillQp);
		CVI_VC_INFO("pureStillThr = %d\n", pEncOP->pureStillThr);
		CVI_VC_INFO("avbrContiSkipNum = %d\n",
			    pRcInfo->avbrContiSkipNum);
	}
	CVI_VC_INFO("bFrmLostOpen = %d\n", pEncOP->frmLostOpen);
	if (pEncOP->frmLostOpen) {
		CVI_VC_INFO("bitrateBuffer = %d\n", pRcInfo->bitrateBuffer);
		CVI_VC_INFO("frameSkipBufThr = %d\n", pRcInfo->frameSkipBufThr);
		CVI_VC_INFO("contiSkipNum = %d\n", pRcInfo->contiSkipNum);
	}

	CVI_VC_INFO("maxIprop = %d, gopSize = %d, maxIPicBit = %d\n",
			pEncOP->maxIprop, pEncOP->gopSize, pRcInfo->maxIPicBit);

	CVI_VC_RC("cviRcEn = %d\n", pRcInfo->cviRcEn);
	if (pRcInfo->cviRcEn)
		cviEncRc_RcKernelInit(pRcInfo, pEncOP);
}

static void cviEncRc_RcKernelInit(stRcInfo *pRcInfo, EncOpenParam *pEncOP)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelCfg rcKerCfg, *pRcKerCfg = &rcKerCfg;
	int frameRateDiv, frameRateRes;

	pRcKerCfg->targetBitrate = pRcInfo->targetBitrate * 1000;
	pRcKerCfg->codec = pRcInfo->codec;

	frameRateDiv = (pRcInfo->framerate >> 16) + 1;
	frameRateRes = pRcInfo->framerate & 0xFFFF;

	CVI_VC_INFO("framerate = %d\n", pRcInfo->framerate);
	pRcKerCfg->framerate =
		CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(frameRateRes), INT_TO_CVI_FLOAT(frameRateDiv));
	pRcKerCfg->intraPeriod = pEncOP->gopSize;
	pRcKerCfg->statTime = (pEncOP->statTime < 0) ? CVI_RC_DEF_STAT_TIME : pEncOP->statTime;
	pRcKerCfg->ipQpDelta = -pEncOP->rcGopIQpOffset;
	pRcKerCfg->numOfPixel = pRcInfo->numOfPixel;
	pRcKerCfg->maxIprop = pEncOP->maxIprop;
	pRcKerCfg->minIprop = CVI_RC_MIN_I_PROP;
	pRcKerCfg->maxQp = pEncOP->userQpMaxP;
	pRcKerCfg->minQp = pEncOP->userQpMinP;
	pRcKerCfg->maxIQp = pEncOP->userQpMaxI;
	pRcKerCfg->minIQp = pEncOP->userQpMinI;
	pRcKerCfg->firstFrmstartQp = pEncOP->RcInitialQp;
	pRcKerCfg->rcMdlUpdatType = CVI_RC_MDL_UPDATE_TYPE;

	CVI_VC_CVRC("targetBitrate = %d, codec = %d, framerate = %d, intraPeriod = %d\n",
			pRcKerCfg->targetBitrate, pRcKerCfg->codec, pRcKerCfg->framerate, pRcKerCfg->intraPeriod);
	CVI_VC_CVRC("statTime = %d, ipQpDelta = %d, numOfPixel = %d, maxIprop = %d, minIprop = %d\n",
			pRcKerCfg->statTime,
			pRcKerCfg->ipQpDelta,
			pRcKerCfg->numOfPixel,
			pRcKerCfg->maxIprop,
			pRcKerCfg->minIprop);
	CVI_VC_CVRC("maxQp = %d, minQp = %d, maxIQp = %d, minIQp = %d, firstFrmstartQp = %d, rcMdlUpdatType = %d\n",
			pRcKerCfg->maxQp,
			pRcKerCfg->minQp,
			pRcKerCfg->maxIQp,
			pRcKerCfg->minIQp,
			pRcKerCfg->firstFrmstartQp,
			pRcKerCfg->rcMdlUpdatType);

	cviRcKernel_init(pRcKerInfo, pRcKerCfg);

	if (pRcInfo->codec == STD_AVC) {
		cviRcKernel_setLastPicQpClip(pRcKerInfo, 10);
		cviRcKernel_setLevelPicQpClip(pRcKerInfo, 10);
		cviRcKernel_setpPicQpNormalClip(pRcKerInfo, 10);
		cviRcKernel_setRCModelUpdateStep(pRcKerInfo, FLOAT_VAL_p2, FLOAT_VAL_p1);
	}
	if (vcodec_mask & CVI_MASK_CVRC) {
		CVI_PRNT("  fn, targetBit, qp, encodedBit\n");
		CVI_PRNT("-------------------------------\n");
	}
}

void cviEncRc_RcKernelEstimatePic(stRcInfo *pRcInfo, EncParam *pEncParam, int frameIdx)
{
	int maxQp, minQp;
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelPicOut *pRcPicOut = &pRcInfo->rcPicOut;

	cviRcKernel_estimatePic(pRcKerInfo, pRcPicOut, pEncParam->is_idr_frame, frameIdx);

	CVI_VC_TRACE("frameIdx = %d, qp = %d, targetBit = %d\n",
			frameIdx, pRcPicOut->qp, pRcPicOut->targetBit);

	pEncParam->u32FrameQp = pRcPicOut->qp;
	pEncParam->u32FrameBits = pRcPicOut->targetBit;
	pEncParam->s32HrdBufLevel = pRcInfo->s32HrdBufLevel;

	pRcInfo->skipPicture = pEncParam->skipPicture;

	if (pRcInfo->rcMode == RC_MODE_AVBR) {
		if (!pRcInfo->svcEnable) {
			maxQp = (pEncParam->is_idr_frame) ? pRcInfo->picIMaxQp : pRcInfo->picPMaxQp;
			minQp = (pEncParam->is_idr_frame) ? pRcInfo->picIMinQp : pRcInfo->picPMinQp;
			pEncParam->u32FrameQp = CLIP3(minQp, maxQp + pRcInfo->qDelta, pRcPicOut->qp);
		} else if (pEncParam->is_idr_frame) {
			pEncParam->u32FrameQp = CLIP3(pRcInfo->picIMinQp,
						     pRcInfo->picIMaxQp + pRcInfo->qDelta,
						     pRcPicOut->qp);
		}
	}
}

void cviEncRc_UpdatePicInfo(stRcInfo *pRcInfo, EncOutputInfo *pEncOutInfo)
{
	int encPicBit = pEncOutInfo->encPicByte << 3;

	if (!pRcInfo->rcEnable) {
		return;
	}

	pRcInfo->bitrateBuffer =
		MAX(pRcInfo->bitrateBuffer + encPicBit - pRcInfo->picAvgBit, 0);
	pRcInfo->isLastPicI = (pEncOutInfo->picType == PIC_TYPE_I) ||
			      (pEncOutInfo->picType == PIC_TYPE_IDR);
	// for Coda, last picture qp is updated in driver code.
	if (pRcInfo->codec == STD_HEVC) {
		pRcInfo->lastPicQp = pEncOutInfo->avgCtuQp;
	}

	if (pRcInfo->cviRcEn)
		cviEncRc_RcKernelUpdatePic(pRcInfo, pEncOutInfo);
}

static void cviEncRc_RcKernelUpdatePic(stRcInfo *pRcInfo, EncOutputInfo *pEncOutInfo)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelPicOut *pRcPicOut = &pRcInfo->rcPicOut;
	stRcKernelPicIn rcPicIn, *pRcPicIn = &rcPicIn;
	int encodedBit;

	encodedBit = pEncOutInfo->encPicByte << 3;

	pRcPicIn->madi = INT_TO_CVI_FLOAT(-1);
	if (pRcInfo->codec == STD_AVC) {
		int mbs = pRcInfo->numOfPixel / 256;	// for avc, mb:16*16

		pRcPicIn->encodedQp = CVI_FLOAT_DIV(
				INT_TO_CVI_FLOAT(pEncOutInfo->u32SumQp),
				INT_TO_CVI_FLOAT(mbs));

		if (pRcInfo->isLastPicI)
			pRcPicIn->madi = CVI_FLOAT_DIV(
					INT_TO_CVI_FLOAT(pEncOutInfo->picVariance),
					INT_TO_CVI_FLOAT(mbs));

		pRcInfo->s32HrdBufLevel += encodedBit - pRcKerInfo->picAvgBit;
	} else if (pRcInfo->codec == STD_HEVC) {
		int mbs = pRcInfo->numOfPixel / 1024;	// for hevc, subCTU:32*32
		pRcPicIn->encodedQp = INT_TO_CVI_FLOAT(pEncOutInfo->avgCtuQp);
		if (pRcInfo->isLastPicI) {
			pRcPicIn->madi = CVI_FLOAT_DIV(
					INT_TO_CVI_FLOAT(pEncOutInfo->sumPicVar),
					INT_TO_CVI_FLOAT(mbs));
		}

		//		pEncOutInfo->numOfSkipBlock
	}

	pRcPicIn->encodedLambda = INT_TO_CVI_FLOAT(-1);
	pRcPicIn->encodedBit = encodedBit;
	pRcPicIn->mse = INT_TO_CVI_FLOAT(-1);
	pRcPicIn->skipRatio = (pRcInfo->skipPicture) ? FLOAT_VAL_minus_1 : FLOAT_VAL_0;

	CVI_VC_CVRC("isLastPicI = %d, encodedBit = %d avg qp = %d\n",
		   pRcInfo->isLastPicI, pRcPicIn->encodedBit, CVI_FLOAT_TO_INT(pRcPicIn->encodedQp));
	cviRcKernel_updatePic(pRcKerInfo, pRcPicIn, pRcInfo->isLastPicI);

	if (vcodec_mask & CVI_MASK_CVRC) {
		CVI_PRNT("rc update: %4d, %9d, %2d, %11d, %4d\n",
			pRcInfo->frameIdx, pRcPicOut->targetBit, pRcPicOut->qp, pRcPicIn->encodedBit,
			CVI_FLOAT_TO_INT(pRcPicIn->encodedQp));
	}
}

void cviEncRc_UpdateFrameSkipSetting(stRcInfo *pRcInfo, int frmLostOpen,
				     int encFrmGaps, int frmLostBpsThr)
{
	pRcInfo->contiSkipNum = (!frmLostOpen)	  ? 0 :
				(encFrmGaps == 0) ? 65535 :
							  encFrmGaps;
	pRcInfo->frameSkipBufThr = frmLostBpsThr;
}

int cviEncRc_ChkFrameSkipByBitrate(stRcInfo *pRcInfo, int isIFrame)
{
	int isFrameDrop = 0;

	if ((!pRcInfo->rcEnable) || (pRcInfo->contiSkipNum == 0)) {
		return 0;
	}

	if (pRcInfo->bitrateBuffer > pRcInfo->frameSkipBufThr &&
	    pRcInfo->frameSkipCnt < pRcInfo->contiSkipNum && !isIFrame) {
		isFrameDrop = 1;
		pRcInfo->frameSkipCnt++;
	} else {
		isFrameDrop = 0;
		pRcInfo->frameSkipCnt = 0;
	}
	CVI_VC_INFO("FrameDrop: %d  %d / %d\n", isFrameDrop,
		    pRcInfo->bitrateBuffer, pRcInfo->frameSkipBufThr);
	return isFrameDrop;
}

BOOL cviEncRc_StateCheck(stRcInfo *pRcInfo, BOOL detect)
{
	eRcState curState = pRcInfo->rcState;
	BOOL transition = FALSE;
	switch (curState) {
	case UNSTABLE:
		transition |= (!detect);
		pRcInfo->rcState += (!detect);
		break;
	case RECOVERY:
		pRcInfo->rcState = (detect) ? UNSTABLE : STEADY;
		transition = TRUE;
		break;
	default:
		transition |= detect;
		pRcInfo->rcState += detect;
		break;
	}

	return transition;
}

