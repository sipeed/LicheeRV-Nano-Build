#ifndef __CVI_ENC_RC_H__
#define __CVI_ENC_RC_H__

#include "../vpuapi/vpuapi.h"
#include "../sample/cvi_h265_interface.h"
#include "rcKernel/cvi_rc_kernel.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AVBR_MAX_BITRATE_WIN 20
#define MOT_LV_DEFAULT 32
#define MOT_LOWER_THR 64

typedef enum { STEADY = 0, UNSTABLE = 1, RECOVERY = 2 } eRcState;

typedef enum {
	E_BITRATE = 0,
	E_FRAMERATE = 1,
} eRcParam;

typedef struct _stRQModel_ {
	int picPelNum;
	int lastQ;
	float alpha;
	float beta;
	float alphaStep;
	float betaStep;
} stRQModel;

typedef struct _stRcInfo_ {
	int frameIdx;
	int numOfPixel;
	int rcEnable;
	int targetBitrate;
	int picAvgBit;
	int maximumBitrate; // VBR, AVBR
	int bitrateBuffer;
	int frameSkipBufThr;
	int frameSkipCnt;
	int contiSkipNum;
	int skipPicture;

	int convergStateBufThr;
	int ConvergenceState;
	int codec;
	int rcMode; // 0:CBR, 1:VBR, 2:AVBR, 5:QpMap, 6:UBR
	int picIMinQp;
	int picIMaxQp;
	int picPMinQp;
	int picPMaxQp;
	int picMinMaxQpClipRange;
	int lastPicQp;
	eRcState rcState;
	int framerate;
	// AVBR
	int bitrateChangePeriod;
	int periodMotionLv;
	int lastPeriodMotionLv;
	int periodMotionLvRaw;
	int lastMinPercent;
	int avbrContiSkipNum;
	int picMotionLvWindow[AVBR_MAX_BITRATE_WIN];
	int avbrLastPeriodBitrate;
	BOOL avbrChangeBrEn;
	BOOL isLastPicI;
	int avbrChangeValidCnt;
	int qDelta;
	int picDciLvWindow[AVBR_MAX_BITRATE_WIN];
	int periodDciLv;
	int lastPeriodDciLv;
	// maxIprop
	int maxIPicBit;
	stRQModel IntraRqMdl;
	BOOL isReEncodeIdr;
	int s32SuperFrmBitsThr;
	BOOL bTestUbrEn;
	int s32HrdBufLevel;

	stRcKernelInfo rcKerInfo;
	stRcKernelPicOut rcPicOut;
	bool cviRcEn;
	bool svcEnable;
} stRcInfo;

#ifdef __cplusplus
}
#endif /* __cplusplus */

void cviEncRc_Open(stRcInfo *pRcInfo, EncOpenParam *pEncOP);
void cviEncRc_RcKernelEstimatePic(stRcInfo *pRcInfo, EncParam *pEncParam, int frameIdx);
void cviEncRc_UpdatePicInfo(stRcInfo *pRcInfo, EncOutputInfo *pEncOutInfo);
void cviEncRc_UpdateFrameSkipSetting(stRcInfo *pRcInfo, int frmLostOpen,
				     int encFrmGaps, int frmLostBpsThr);
int cviEncRc_ChkFrameSkipByBitrate(stRcInfo *pRcInfo, int isIFrame);
BOOL cviEncRc_StateCheck(stRcInfo *pRcInfo, BOOL detect);
BOOL cviEnc_Avbr_PicCtrl(stRcInfo *pRcInfo, EncOpenParam *pEncOP, int frameIdx);
BOOL cviEncRc_Avbr_CheckFrameSkip(stRcInfo *pRcInfo, EncOpenParam *pEncOP,
				  int isIFrame);
int cviEncRc_Avbr_GetQpDelta(stRcInfo *pRcInfo, EncOpenParam *pEncOP);

int cviEncRc_GetParam(stRcInfo *pRcInfo, eRcParam eParam);
int cviEncRc_SetParam(stRcInfo *pRcInfo, EncOpenParam *pEncOP, eRcParam eParam);

#endif
