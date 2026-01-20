//------------------------------------------------------------------------------
// File: Mixer.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#include <linux/string.h>
#include <linux/module.h>
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "main_helper.h"
#include "cvi_vc_getopt.h"

#ifdef VC_DRIVER_TEST
const HevcCfgInfo hevcCfgInfo[MAX_CFG] = {
	// name                          min            max              default
	{ "InputFile", 0, 0, 0 },
	{ "SourceWidth", 0, W4_MAX_ENC_PIC_WIDTH, 0 },
	{ "SourceHeight", 0, W4_MAX_ENC_PIC_WIDTH, 0 },
	{ "InputBitDepth", 8, 10, 8 },
	{ "FrameRate", 0, 240, 0 }, // 5
	{ "FrameSkip", 0, INT_MAX, 0 },
	{ "FramesToBeEncoded", 0, INT_MAX, 0 },
	{ "IntraPeriod", 0, UI16_MAX, 0 },
	{ "DecodingRefreshType", 0, 2, 1 },
	{ "DeriveLambdaWeight", 0, 1, 0 }, // 10
	{ "GOPSize", 1, MAX_GOP_NUM, 1 },
	{ "EnIntraInInterSlice", 0, 1, 1 },
	{ "IntraNxN", 0, 1, 1 },
	{ "EnCu8x8", 0, 1, 1 },
	{ "EnCu16x16", 0, 1, 1 }, // 15
	{ "EnCu32x32", 0, 1, 1 },
	{ "IntraTransSkip", 0, 2, 1 },
	{ "ConstrainedIntraPred", 0, 1, 0 },
	{ "IntraCtuRefreshMode", 0, 4, 0 },
	{ "IntraCtuRefreshArg", 0, UI16_MAX, 0 }, // 20
	{ "MaxNumMerge", 0, 2, 2 },
	{ "EnDynMerge", 0, 1, 1 },
	{ "EnTemporalMVP", 0, 1, 1 },
	{ "ScalingList", 0, 1, 0 },
	{ "IndeSliceMode", 0, 1, 0 }, // 25
	{ "IndeSliceArg", 0, UI16_MAX, 0 },
	{ "DeSliceMode", 0, 2, 0 },
	{ "DeSliceArg", 0, UI16_MAX, 0 },
	{ "EnDBK", 0, 1, 1 },
	{ "EnSAO", 0, 1, 1 }, // 30
	{ "LFCrossSliceBoundaryFlag", 0, 1, 1 },
	{ "BetaOffsetDiv2", -6, 6, 0 },
	{ "TcOffsetDiv2", -6, 6, 0 },
	{ "WaveFrontSynchro", 0, 1, 0 },
	{ "LosslessCoding", 0, 1, 0 }, // 35
	{ "UsePresetEncTools", 0, 3, 0 },
	{ "NumTemporalLayers", 0, 7, 1 },
	{ "GopPreset", 0, 20, 0 },
	{ "RateControl", 0, 1, 0 },
	{ "EncBitrate", 0, 700000000, 0 }, // 40
	{ "TransBitrate", 0, 700000000, 0 },
	{ "InitialDelay", 10, 3000, 3000 },
	{ "EnHvsQp", 0, 1, 1 },
	{ "CULevelRateControl", 0, 1, 1 },
	{ "ConfWindSizeTop", 0, W4_MAX_ENC_PIC_HEIGHT, 0 }, // 45
	{ "ConfWindSizeBot", 0, W4_MAX_ENC_PIC_HEIGHT, 0 },
	{ "ConfWindSizeRight", 0, W4_MAX_ENC_PIC_WIDTH, 0 },
	{ "ConfWindSizeLeft", 0, W4_MAX_ENC_PIC_WIDTH, 0 },
	{ "HvsQpScaleDiv2", 0, 4, 2 },
	{ "MinQp", 0, 51, 8 }, // 50
	{ "MaxQp", 0, 51, 51 },
	{ "MaxDeltaQp", 0, 51, 10 },
	{ "NumRoi", 0, MAX_ROI_NUMBER, 0 },
	{ "QP", 0, 51, 30 },
	{ "RoiDeltaQP", 0, 51, 3 }, // 55
	{ "IntraQpOffset", -10, 10, 0 },
	{ "InitBufLevelx8", 0, 8, 1 },
	{ "BitAllocMode", 0, 2, 0 },
	{ "FixedBitRatio%d", 1, 255, 1 },
	{ "InternalBitDepth", 0, 10, 0 }, // 60
	{ "EnUserDataSei", 0, 1, 0 },
	{ "UserDataEncTiming", 0, 1, 0 },
	{ "UserDataSize", 0, (1 << 24) - 1, 1 },
	{ "UserDataPos", 0, 1, 0 },
	{ "EnRoi", 0, 1, 0 }, // 65
	{ "VuiParamFlags", 0, INT_MAX, 0 },
	{ "VuiAspectRatioIdc", 0, 255, 0 },
	{ "VuiSarSize", 0, INT_MAX, 0 },
	{ "VuiOverScanAppropriate", 0, 1, 0 },
	{ "VideoSignal", 0, INT_MAX, 0 }, // 70
	{ "VuiChromaSampleLoc", 0, INT_MAX, 0 },
	{ "VuiDispWinLeftRight", 0, INT_MAX, 0 },
	{ "VuiDispWinTopBottom", 0, INT_MAX, 0 },
	{ "NumUnitsInTick", 0, INT_MAX, 0 },
	{ "TimeScale", 0, INT_MAX, 0 }, // 75
	{ "NumTicksPocDiffOne", 0, INT_MAX, 0 },
	{ "EncAUD", 0, 1, 0 },
	{ "EncEOS", 0, 1, 0 },
	{ "EncEOB", 0, 1, 0 },
	{ "CbQpOffset", -12, 12, 0 }, // 80
	{ "CrQpOffset", -12, 12, 0 },
	{ "RcInitialQp", -1, 63, 63 },
	{ "EnNoiseReductionY", 0, 1, 0 },
	{ "EnNoiseReductionCb", 0, 1, 0 },
	{ "EnNoiseReductionCr", 0, 1, 0 }, // 85
	{ "EnNoiseEst", 0, 1, 1 },
	{ "NoiseSigmaY", 0, 255, 0 },
	{ "NoiseSigmaCb", 0, 255, 0 },
	{ "NoiseSigmaCr", 0, 255, 0 },
	{ "IntraNoiseWeightY", 0, 31, 7 }, // 90
	{ "IntraNoiseWeightCb", 0, 31, 7 },
	{ "IntraNoiseWeightCr", 0, 31, 7 },
	{ "InterNoiseWeightY", 0, 31, 4 },
	{ "InterNoiseWeightCb", 0, 31, 4 },
	{ "InterNoiseWeightCr", 0, 31, 4 }, // 95
	{ "IntraMinQp", 0, 51, 8 },
	{ "IntraMaxQp", 0, 51, 51 },
	{ "MdFlag0", 0, 1, 0 },
	{ "MdFlag1", 0, 1, 0 },
	{ "MdFlag2", 0, 1, 0 }, // 100
	{ "EnSmartBackground", 0, 1, 0 },
	{ "ThrPixelNumCnt", 0, 63, 0 },
	{ "ThrMean0", 0, 255, 5 },
	{ "ThrMean1", 0, 255, 5 },
	{ "ThrMean2", 0, 255, 5 }, // 105
	{ "ThrMean3", 0, 255, 5 },
	{ "MdQpY", 0, 51, 30 },
	{ "MdQpC", 0, 51, 30 },
	{ "ThrDcY0", 0, UI16_MAX, 2 },
	{ "ThrDcC0", 0, UI16_MAX, 2 }, // 110
	{ "ThrDcY1", 0, UI16_MAX, 2 },
	{ "ThrDcC1", 0, UI16_MAX, 2 },
	{ "ThrDcY2", 0, UI16_MAX, 2 },
	{ "ThrDcC2", 0, UI16_MAX, 2 },
	{ "ThrAcNumY0", 0, 63, 12 }, // 115
	{ "ThrAcNumC0", 0, 15, 3 },
	{ "ThrAcNumY1", 0, 255, 51 },
	{ "ThrAcNumC1", 0, 63, 12 },
	{ "ThrAcNumY2", 0, 1023, 204 },
	{ "ThrAcNumC2", 0, 255, 51 }, // 120
	{ "UseAsLongTermRefPeriod", 0, INT_MAX, 0 },
	{ "RefLongTermPeriod", 0, INT_MAX, 0 },
	{ "EnCtuMode", 0, 1, 0 },
	{ "EnCtuQp", 0, 1, 0 },
	{ "CropXPos", 0, W4_MAX_ENC_PIC_WIDTH, 0 }, // 125
	{ "CropYPos", 0, W4_MAX_ENC_PIC_HEIGHT, 0 },
	{ "CropXSize", 0, W4_MAX_ENC_PIC_WIDTH, 0 },
	{ "CropYSize", 0, W4_MAX_ENC_PIC_HEIGHT, 0 },
	{ "EncodeRbspVui", 0, 1, 0 },
	{ "RbspVuiSize", 0, INT_MAX, 0 }, // 130
	{ "EncodeRbspHrdInVps", 0, 1, 0 },
	{ "EncodeRbspHrdInVui", 0, 1, 0 },
	{ "RbspHrdSize", 0, INT_MAX, 0 },
	{ "EnPrefixSeiData", 0, 1, 0 },
	{ "PrefixSeiDataSize", 0, UI16_MAX, 0 }, // 135
	{ "PrefixSeiTimingFlag", 0, 1, 0 },
	{ "EnSuffixSeiData", 0, 1, 0 },
	{ "SuffixSeiDataSize", 0, UI16_MAX, 0 },
	{ "SuffixSeiTimingFlag", 0, 1, 0 },
	{ "EnReportMvCol", 0, 1, 0 }, // 140
	{ "EnReportDistMap", 0, 1, 0 },
	{ "EnReportBitInfo", 0, 1, 0 },
	{ "EnReportFrameDist", 0, 1, 0 },
	{ "EnReportQpHisto", 0, 1, 0 },
	{ "BitstreamFile", 0, 0, 0 }, // 145
	{ "EnCustomVpsHeader", 0, 1, 0 },
	{ "EnCustomSpsHeader", 0, 1, 0 },
	{ "EnCustomPpsHeader", 0, 1, 0 },
	{ "CustomVpsPsId", 0, 15, 0 },
	{ "CustomSpsPsId", 0, 15, 0 }, // 150
	{ "CustomSpsActiveVpsId", 0, 15, 0 },
	{ "CustomPpsActiveSpsId", 0, 15, 0 },
	{ "CustomVpsIntFlag", 0, 1, 1 },
	{ "CustomVpsAvailFlag", 0, 1, 1 },
	{ "CustomVpsMaxLayerMinus1", 0, 62, 0 }, // 155
	{ "CustomVpsMaxSubLayerMinus1", 0, 6, 0 },
	{ "CustomVpsTempIdNestFlag", 0, 1, 0 },
	{ "CustomVpsMaxLayerId", 0, 31, 0 },
	{ "CustomVpsNumLayerSetMinus1", 0, 2, 0 },
	{ "CustomVpsExtFlag", 0, 1, 0 }, // 160
	{ "CustomVpsExtDataFlag", 0, 1, 0 },
	{ "CustomVpsSubOrderInfoFlag", 0, 1, 0 },
	{ "CustomSpsSubOrderInfoFlag", 0, 1, 0 },
	{ "CustomVpsLayerId0", 0, 0xFFFFFFFF, 0 },
	{ "CustomVpsLayerId1", 0, 0xFFFFFFFF, 0 }, // 165
	{ "CustomSpsLog2MaxPocMinus4", 0, 12, 4 },
	{ "EnForcedIDRHeader", 0, 1, 0 },

	// newly added for WAVE520
	{ "EncMonochrome", 0, 1, 0 },
	{ "StrongIntraSmoothing", 0, 1, 1 },
	{ "RoiAvgQP", 0, 51, 0 }, // 170
	{ "WeightedPred", 0, 1, 0 },
	{ "EnBgDetect", 0, 1, 0 },
	{ "BgThDiff", 0, 255, 8 },
	{ "BgThMeanDiff", 0, 255, 1 },
	{ "BgLambdaQp", 0, 51, 32 }, // 175
	{ "BgDeltaQp", -16, 15, 3 },
	{ "TileNumColumns", 1, 6, 1 },
	{ "TileNumRows", 1, 6, 1 },
	{ "TileUniformSpace", 0, 1, 1 },
	{ "EnLambdaMap", 0, 1, 0 }, // 180
	{ "EnCustomLambda", 0, 1, 0 },
	{ "EnCustomMD", 0, 1, 0 },
	{ "PU04DeltaRate", -128, 127, 0 },
	{ "PU08DeltaRate", -128, 127, 0 },
	{ "PU16DeltaRate", -128, 127, 0 }, // 185
	{ "PU32DeltaRate", -128, 127, 0 },
	{ "PU04IntraPlanarDeltaRate", -128, 127, 0 },
	{ "PU04IntraDcDeltaRate", -128, 127, 0 },
	{ "PU04IntraAngleDeltaRate", -128, 127, 0 },
	{ "PU08IntraPlanarDeltaRate", -128, 127, 0 }, // 190
	{ "PU08IntraDcDeltaRate", -128, 127, 0 },
	{ "PU08IntraAngleDeltaRate", -128, 127, 0 },
	{ "PU16IntraPlanarDeltaRate", -128, 127, 0 },
	{ "PU16IntraDcDeltaRate", -128, 127, 0 },
	{ "PU16IntraAngleDeltaRate", -128, 127, 0 }, // 195
	{ "PU32IntraPlanarDeltaRate", -128, 127, 0 },
	{ "PU32IntraDcDeltaRate", -128, 127, 0 },
	{ "PU32IntraAngleDeltaRate", -128, 127, 0 },
	{ "CU08IntraDeltaRate", -128, 127, 0 },
	{ "CU08InterDeltaRate", -128, 127, 0 }, // 200
	{ "CU08MergeDeltaRate", -128, 127, 0 },
	{ "CU16IntraDeltaRate", -128, 127, 0 },
	{ "CU16InterDeltaRate", -128, 127, 0 },
	{ "CU16MergeDeltaRate", -128, 127, 0 },
	{ "CU32IntraDeltaRate", -128, 127, 0 }, // 205
	{ "CU32InterDeltaRate", -128, 127, 0 },
	{ "CU32MergeDeltaRate", -128, 127, 0 },
	{ "DisableCoefClear", 0, 1, 0 },
	{ "EnModeMap", 0, 3, 0 },
	{ "EnTemporalLayerQp", 0, 1, 0 },
	{ "TID_0_Qp", 0, 51, 30 },
	{ "TID_1_Qp", 0, 51, 33 },
	{ "TID_2_Qp", 0, 51, 36 },
	{ "TID_0_Period", 2, 128, 60 },
#ifdef SUPPORT_HOST_RC_PARAM
	{ "EnHostPicRC", 0, 1, 0 },
	{ "HostPicRcFile", 0, 0, 0 },
#endif
};

/*
#define NUM_GOP_PRESET (5)
const int32_t GOP_SIZE_PRESET[NUM_GOP_PRESET + 1] = { 0, 1, 1, 1, 2, 2 };

static const int32_t SVC_T_ENC_GOP_PRESET_1[5] = { 1, 0, 0, 0, 0 };
static const int32_t SVC_T_ENC_GOP_PRESET_2[5] = { 1, 0, 0, 1, 0 };
static const int32_t SVC_T_ENC_GOP_PRESET_3[5] = {
	1, 0, 0, 1, 1,
};

static const int32_t SVC_T_ENC_GOP_PRESET_4[10] = {
	1, 0, 0, 2, 0, 2, 2, 0, 1, 0
};

static const int32_t SVC_T_ENC_GOP_PRESET_5[10] = {
	1, 0, 0, 2, 0, 2, 2, 0, 1, 1
};

static const int32_t *AVC_GOP_PRESET[NUM_GOP_PRESET + 1] = {
	NULL,
	SVC_T_ENC_GOP_PRESET_1,
	SVC_T_ENC_GOP_PRESET_2,
	SVC_T_ENC_GOP_PRESET_3,
	SVC_T_ENC_GOP_PRESET_4,
	SVC_T_ENC_GOP_PRESET_5,
};
*/
#endif
#define NUM_GOP_PRESET (3)
const int32_t GOP_SIZE_PRESET[NUM_GOP_PRESET + 1] = { 0, 1, 2, 4 };

static const int32_t SVC_T_ENC_GOP_PRESET_1[5] = { 1, 0, 0, 0, 0 };
static const int32_t SVC_T_ENC_GOP_PRESET_2[10] = {
	1, 2, 0, 2, 0, 2, 0, 0, 1, 0
};
static const int32_t SVC_T_ENC_GOP_PRESET_3[20] = { 1, 4, 0, 3, 0, 2, 2,
						    0, 2, 0, 3, 4, 2, 3,
						    0, 4, 0, 0, 1, 0 };

static const int32_t *AVC_GOP_PRESET[NUM_GOP_PRESET + 1] = {
	NULL,
	SVC_T_ENC_GOP_PRESET_1,
	SVC_T_ENC_GOP_PRESET_2,
	SVC_T_ENC_GOP_PRESET_3,
};

void set_gop_info(ENC_CFG *pEncCfg)
{
	int i, j;
	const int32_t *src_gop = AVC_GOP_PRESET[pEncCfg->GopPreset];

	pEncCfg->set_dqp_pic_num = GOP_SIZE_PRESET[pEncCfg->GopPreset];

	for (i = 0, j = 0; i < pEncCfg->set_dqp_pic_num; i++) {
		pEncCfg->gop_entry[i].curr_poc = src_gop[j++];
		pEncCfg->gop_entry[i].qp_offset = src_gop[j++];
		pEncCfg->gop_entry[i].ref_poc = src_gop[j++];
		pEncCfg->gop_entry[i].temporal_id = src_gop[j++];
		pEncCfg->gop_entry[i].ref_long_term = src_gop[j++];
	}
}
#ifdef	VC_DRIVER_TEST
static int HEVC_SetGOPInfo(char *lineStr, CustomGopPicParam *gopPicParam,
			   int *gopPicLambda, int useDeriveLambdaWeight,
			   int intraQp)
{
	int numParsed;
	char sliceType;
	//double lambda;  origal
	int lambda; //need double type here

	osal_memset(gopPicParam, 0, sizeof(CustomGopPicParam));
	*gopPicLambda = 0;

	numParsed = sscanf(lineStr, "%c %d %d %d %d %d %d", &sliceType,
			   &gopPicParam->pocOffset, &gopPicParam->picQp,
			   &lambda, &gopPicParam->temporalId,
			   &gopPicParam->refPocL0, &gopPicParam->refPocL1);


#if FLOATING_POINT_LAMBDA == 0
	lambda = (int)(lambda * 256); // * 2^10
#endif

	if (sliceType == 'I') {
		gopPicParam->picType = PIC_TYPE_I;
	} else if (sliceType == 'P') {
		gopPicParam->picType = PIC_TYPE_P;
		if (numParsed == 6)
			gopPicParam->numRefPicL0 = 2;
		else
			gopPicParam->numRefPicL0 = 1;
	} else if (sliceType == 'B') {
		gopPicParam->picType = PIC_TYPE_B;
	} else {
		return 0;
	}
	if (sliceType == 'P' && numParsed != 6) {
		return 0;
	}
	if (sliceType == 'B' && numParsed != 7) {
		return 0;
	}
	if (gopPicParam->temporalId < 0) {
		return 0;
	}

	gopPicParam->picQp = gopPicParam->picQp + intraQp;

	if (useDeriveLambdaWeight == 0) {
		*gopPicLambda = (int)(lambda * LAMBDA_SCALE_FACTOR);
	} else {
		*gopPicLambda = 0;
	}

	return 1;
}

static int HEVC_GetStringValue(char **ppArray, int ArrayCnt, char *para, char *value)
{
	int pos = 0;
	char *token = NULL;
	int j = 0;
	char *valueStr = vzalloc(256);
	char *lineStr = vmalloc(256);

	while (j < ArrayCnt && ppArray[j]) {
		memset(lineStr, 0, 256);
		memcpy(lineStr, ppArray[j], strlen(ppArray[j]));
		j++;

		if ((lineStr[0] == '#') || (lineStr[0] == ';') ||
		    (lineStr[0] == ':')) { // check comment
			continue;
		}

		token = cvi_strtok(lineStr,
			       ": "); // parameter name is separated by ' ' or
		// ':'
		if (token != NULL) {
			if (strcasecmp(para, token) != 0) {
				continue;
			} else {
				// check parameter name
				token = cvi_strtok(NULL, ":\r\n");
				osal_memcpy(valueStr, token, strlen(token));
				while (valueStr[pos] == ' ') { // check space
					pos++;
				}

				strcpy(value, &valueStr[pos]);
				vfree(valueStr);
				vfree(lineStr);
				return 1;
			}
		} else {
			continue;
		}
	}

	vfree(valueStr);
	vfree(lineStr);
	return 0;
}

int HEVC_GetValue(char **ppArray, int ArrayCnt, HevcCfgName cfgName, int *value)
{
	int iValue;
	char *sValue = vmalloc(256);

	if (HEVC_GetStringValue(ppArray, ArrayCnt, hevcCfgInfo[cfgName].name, sValue) == 1) {
		iValue = atoi(sValue);
		if ((iValue >= hevcCfgInfo[cfgName].min) &&
		    (iValue <= hevcCfgInfo[cfgName].max)) { // Check min, max
			*value = iValue;
			vfree(sValue);
			return 1;
		}
		VLOG(ERR,
		     "CFG file error : %s value is not available. ( min = %d, max = %d)\n",
		     hevcCfgInfo[cfgName].name,
		     hevcCfgInfo[cfgName].min,
		     hevcCfgInfo[cfgName].max);
		vfree(sValue);
		return 0;
	}

	*value = hevcCfgInfo[cfgName].def;
	vfree(sValue);
	return 1;
}


int parseHevcCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
	osal_file_t fp;
	int array_cnt = 0;
	char *save_ptr;
	char *pstrArray[120];
	//char sValue[256];
	//char tempStr[256];
	char *sValue = vmalloc(256);
	char *tempStr = vmalloc(256);
	char *sRead = vmalloc(1000*20);
	char *pstr = sRead;
	int iValue = 0, ret = 0, i = 0;
	int intra8 = 0, intra16 = 0, intra32 = 0, frameSkip = 0,
	    dynamicMergeEnable; // temp value
	UNREFERENCED_PARAMETER(frameSkip);

	if (!sRead) {
		return -1;
	}
	memset(sRead, 0, 1000*20);
	fp = osal_fopen(FileName, "rb");
	if (fp == NULL) {
		CVI_VC_ERR("open fail\n");
		vfree(sRead);
		vfree(sValue);
		vfree(tempStr);
		return 0;
	}

	ret = osal_fread(sRead, 1, 1000*20, fp);
	osal_fclose(fp);

	i = 0;
	while (NULL != (pstrArray[i] = cvi_strtok_r(pstr, "\n", &save_ptr))) {
		i++;
		if (i >= 120) {
			break;
		}
		pstr = NULL;
	}
	array_cnt = i;

	if (HEVC_GetStringValue(pstrArray, array_cnt, hevcCfgInfo[BITSTREAM_FILE].name, sValue) !=
	    0)
		strcpy(pEncCfg->BitStreamFileName, sValue);

	if (HEVC_GetStringValue(pstrArray, array_cnt, hevcCfgInfo[INPUT_FILE].name, sValue) == 0)
		goto __end_parse;
	else
		strcpy(pEncCfg->SrcFileName, sValue);

	if (HEVC_GetValue(pstrArray, array_cnt, SOURCE_WIDTH, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.picX = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, SOURCE_HEIGHT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.picY = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, FRAMES_TO_BE_ENCODED, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->NumFrame = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, INPUT_BIT_DEPTH, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->SrcBitDepth = iValue; // BitDepth == 8 ?
			// HEVC_PROFILE_MAIN :
			// HEVC_PROFILE_MAIN10

	if (HEVC_GetValue(pstrArray, array_cnt, INTERNAL_BITDEPTH, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.internalBitDepth = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, LOSSLESS_CODING, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.losslessEnable = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, CONSTRAINED_INTRA_PRED, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.constIntraPredFlag = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, DECODING_REFRESH_TYPE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.decodingRefreshType = iValue;

	// RoiDetaQp
	if (HEVC_GetValue(pstrArray, array_cnt, ROI_DELTA_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.ctuOptParam.roiDeltaQp = iValue;
	CVI_VC_CFG("roiDeltaQp = %d\n",
		   pEncCfg->hevcCfg.ctuOptParam.roiDeltaQp);

	// IntraQpOffset
	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_QP_OFFSET, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraQpOffset = iValue;

	// Initial Buf level x 8
	if (HEVC_GetValue(pstrArray, array_cnt, INIT_BUF_LEVELx8, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.initBufLevelx8 = iValue;

	// BitAllocMode
	if (HEVC_GetValue(pstrArray, array_cnt, BIT_ALLOC_MODE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.bitAllocMode = iValue;

	// FixedBitRatio 0 ~ 7
	for (i = 0; i < MAX_GOP_NUM; i++) {
		sprintf(tempStr, "FixedBitRatio%d", i);
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			iValue = atoi(sValue);
			if (iValue >= hevcCfgInfo[FIXED_BIT_RATIO].min &&
			    iValue <= hevcCfgInfo[FIXED_BIT_RATIO].max)
				pEncCfg->hevcCfg.fixedBitRatio[i] = iValue;
			else
				pEncCfg->hevcCfg.fixedBitRatio[i] =
					hevcCfgInfo[FIXED_BIT_RATIO].def;
		} else
			pEncCfg->hevcCfg.fixedBitRatio[i] =
				hevcCfgInfo[FIXED_BIT_RATIO].def;
	}

	// IntraQp
	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraQP = iValue;

	if (pEncCfg->hevcCfg.losslessEnable)
		pEncCfg->hevcCfg.intraQP = 4;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_PERIOD, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraPeriod = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CONF_WIND_SIZE_TOP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.confWinTop = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CONF_WIND_SIZE_BOT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.confWinBot = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CONF_WIND_SIZE_RIGHT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.confWinRight = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CONF_WIND_SIZE_LEFT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.confWinLeft = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, FRAME_RATE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.frameRate = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INDE_SLICE_MODE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.independSliceMode = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INDE_SLICE_ARG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.independSliceModeArg = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, DE_SLICE_MODE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.dependSliceMode = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, DE_SLICE_ARG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.dependSliceModeArg = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_INTRA_IN_INTER_SLICE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraInInterSliceEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_CTU_REFRESH_MODE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraRefreshMode = iValue;

	if (pEncCfg->hevcCfg.intraInInterSliceEnable == 0) {
		pEncCfg->hevcCfg.intraRefreshMode = 0;
	}

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_CTU_REFRESH_ARG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraRefreshArg = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, USE_PRESENT_ENC_TOOLS, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.useRecommendEncParam = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, SCALING_LIST, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.scalingListEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_CU_8X8, &iValue) == 0)
		goto __end_parse;
	else
		intra8 = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_CU_16X16, &iValue) == 0)
		goto __end_parse;
	else
		intra16 = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_CU_32X32, &iValue) == 0)
		goto __end_parse;
	else
		intra32 = iValue;

	intra8 = intra16 = intra32 = 1; // force enable all cu mode. [CEZ-1865]
	pEncCfg->hevcCfg.cuSizeMode =
		(intra8 & 0x01) | (intra16 & 0x01) << 1 | (intra32 & 0x01) << 2;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_TEMPORAL_MVP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.tmvpEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, WAVE_FRONT_SYNCHRO, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.wppenable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, MAX_NUM_MERGE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.maxNumMerge = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_DYN_MERGE, &iValue) == 0)
		goto __end_parse;
	else
		dynamicMergeEnable = iValue;

	pEncCfg->hevcCfg.dynamicMerge8x8Enable = dynamicMergeEnable; // [FIXME]
	pEncCfg->hevcCfg.dynamicMerge16x16Enable =
		dynamicMergeEnable; // [FIXME]
	pEncCfg->hevcCfg.dynamicMerge32x32Enable =
		dynamicMergeEnable; // [FIXME]

	if (HEVC_GetValue(pstrArray, array_cnt, EN_DBK, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.disableDeblk = !(iValue);

	if (HEVC_GetValue(pstrArray, array_cnt, LF_CROSS_SLICE_BOUNDARY_FLAG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.lfCrossSliceBoundaryEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, BETA_OFFSET_DIV2, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.betaOffsetDiv2 = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, TC_OFFSET_DIV2, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.tcOffsetDiv2 = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_TRANS_SKIP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.skipIntraTrans = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_SAO, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.saoEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_NXN, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraNxNEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, RATE_CONTROL, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->RcEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, ENC_BITRATE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->RcBitRate = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, TRANS_BITRATE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.transRate = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CU_LEVEL_RATE_CONTROL, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.cuLevelRCEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_HVS_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.hvsQPEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, HVS_QP_SCALE_DIV2, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.hvsQpScale = iValue;

	pEncCfg->hevcCfg.hvsQpScaleEnable = (iValue > 0) ? 1 : 0;

	if (HEVC_GetValue(pstrArray, array_cnt, INITIAL_DELAY, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->RcInitDelay = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, MIN_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.minQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, MAX_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.maxQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, MAX_DELTA_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.maxDeltaQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, GOP_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.customGopSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, DERIVE_LAMBDA_WEIGHT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.useDeriveLambdaWeight = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, ROI_ENABLE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.ctuOptParam.roiEnable = iValue;
	CVI_VC_CFG("roiEnable = %d\n", pEncCfg->hevcCfg.ctuOptParam.roiEnable);

	if (HEVC_GetValue(pstrArray, array_cnt, GOP_PRESET, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopPresetIdx = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, EN_TEMPORAL_LAYER_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.enTemporalLayerQp = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, TID_0_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.tidQp0 = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, TID_1_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.tidQp1 = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, TID_2_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.tidQp2 = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, TID_0_PERIOD, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.gopParam.tidPeriod0 = iValue;
	if (HEVC_GetValue(pstrArray, array_cnt, FRAME_SKIP, &iValue) == 0)
		goto __end_parse;
	else
		frameSkip = iValue;

	// VUI encoding
	if (HEVC_GetValue(pstrArray, array_cnt, VUI_PARAM_FLAG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiParamFlags = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_ASPECT_RATIO_IDC, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiAspectRatioIdc = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_SAR_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiSarSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_OVERSCAN_APPROPRIATE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiOverScanAppropriate = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VIDEO_SIGNAL, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.videoSignal = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_CHROMA_SAMPLE_LOC, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiChromaSampleLoc = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_DISP_WIN_LEFT_RIGHT, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiDispWinLeftRight = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_DISP_WIN_TOP_BOTTOM, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiParam.vuiDispWinTopBottom = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, NUM_UNITS_IN_TICK, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.numUnitsInTick = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, TIME_SCALE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.timeScale = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, NUM_TICKS_POC_DIFF_ONE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.numTicksPocDiffOne = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, ENC_AUD, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.encAUD = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, ENC_EOS, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.encEOS = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, ENC_EOB, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.encEOB = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CB_QP_OFFSET, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.chromaCbQpOffset = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, CR_QP_OFFSET, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.chromaCrQpOffset = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, RC_INIT_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.initialRcQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_NR_Y, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrYEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_NR_CB, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrCbEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_NR_CR, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrCrEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_NOISE_EST, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrNoiseEstEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, NOISE_SIGMA_Y, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrNoiseSigmaY = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, NOISE_SIGMA_CB, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrNoiseSigmaCb = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, NOISE_SIGMA_CR, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrNoiseSigmaCr = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_NOISE_WEIGHT_Y, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrIntraWeightY = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_NOISE_WEIGHT_CB, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrIntraWeightCb = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_NOISE_WEIGHT_CR, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrIntraWeightCr = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTER_NOISE_WEIGHT_Y, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrInterWeightY = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTER_NOISE_WEIGHT_CB, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrInterWeightCb = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTER_NOISE_WEIGHT_CR, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.nrInterWeightCr = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_MIN_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraMinQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, INTRA_MAX_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.intraMaxQp = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, USE_LONGTERM_PRRIOD, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.useAsLongtermPeriod = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, REF_LONGTERM_PERIOD, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.refLongtermPeriod = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_CTU_MODE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.ctuOptParam.ctuModeEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_CTU_QP, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.ctuOptParam.ctuQpEnable = iValue;

	CVI_VC_CFG("ctuQpEnable = %d\n",
		   pEncCfg->hevcCfg.ctuOptParam.ctuQpEnable);

	if (HEVC_GetValue(pstrArray, array_cnt, EN_VUI_DATA, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiDataEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, VUI_DATA_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.vuiDataSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_HRD_IN_VPS, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.hrdInVPS = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_HRD_IN_VUI, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.hrdInVUI = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, HRD_DATA_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.hrdDataSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_PREFIX_SEI_DATA, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.prefixSeiEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, PREFIX_SEI_DATA_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.prefixSeiDataSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, PREFIX_SEI_TIMING_FLAG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.prefixSeiTimingFlag = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_SUFFIX_SEI_DATA, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.suffixSeiEnable = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, SUFFIX_SEI_DATA_SIZE, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.suffixSeiDataSize = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, SUFFIX_SEI_TIMING_FLAG, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.suffixSeiTimingFlag = iValue;

	if (HEVC_GetValue(pstrArray, array_cnt, EN_FORCED_IDR_HEADER, &iValue) == 0)
		goto __end_parse;
	else
		pEncCfg->hevcCfg.forcedIdrHeaderEnable = iValue;


	// GOP
	if (pEncCfg->hevcCfg.intraPeriod == 1) {
		pEncCfg->hevcCfg.gopParam.picParam[0].picType = PIC_TYPE_I;
		pEncCfg->hevcCfg.gopParam.picParam[0].picQp =
			pEncCfg->hevcCfg.intraQP;
		if (pEncCfg->hevcCfg.gopParam.customGopSize > 1) {
			VLOG(ERR,
			     "CFG file error : gop size should be smaller than 2 for all intra case\n");
			goto __end_parse;
		}
	} else {
		for (i = 0;
		     pEncCfg->hevcCfg.gopPresetIdx == PRESET_IDX_CUSTOM_GOP &&
		     i < pEncCfg->hevcCfg.gopParam.customGopSize;
		     i++) {
			sprintf(tempStr, "Frame%d", i + 1);
			if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) == 0) {
				VLOG(ERR,
				     "CFG file error : %s value is not available.\n",
				     tempStr);
				goto __end_parse;
			}
			if (HEVC_SetGOPInfo(
				    sValue,
				    &pEncCfg->hevcCfg.gopParam.picParam[i],
				    &pEncCfg->hevcCfg.gopParam.gopPicLambda[i],
				    pEncCfg->hevcCfg.gopParam.useDeriveLambdaWeight,
				    pEncCfg->hevcCfg.intraQP) == 0) {
				VLOG(ERR,
				     "CFG file error : %s value is not available.\n",
				     tempStr);
				goto __end_parse;
			}
#if TEMP_SCALABLE_RC
			if ((pEncCfg->hevcCfg.gopParam.picParam[i].temporalId +
			     1) > MAX_NUM_TEMPORAL_LAYER) {
				VLOG(ERR,
				     "CFG file error : %s MaxTempLayer %d exceeds MAX_TEMP_LAYER(7).\n",
				     tempStr,
				     pEncCfg->hevcCfg.gopParam.picParam[i]
						     .temporalId +
					     1);
				goto __end_parse;
			}
#endif
		}
	}


	CVI_VC_CFG("roiEnable = %d\n", pEncCfg->hevcCfg.ctuOptParam.roiEnable);
	// ROI
	if (pEncCfg->hevcCfg.ctuOptParam.roiEnable) {
		sprintf(tempStr, "RoiFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.roiFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.ctuOptParam.ctuModeEnable) {
		sprintf(tempStr, "CtuModeFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.ctuModeFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.ctuOptParam.ctuQpEnable) {
		sprintf(tempStr, "CtuQpFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.ctuQpFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.prefixSeiEnable) {
		sprintf(tempStr, "PrefixSeiDataFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.prefixSeiDataFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.suffixSeiEnable) {
		sprintf(tempStr, "SuffixSeiDataFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.suffixSeiDataFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.hrdInVPS || pEncCfg->hevcCfg.hrdInVUI) {
		sprintf(tempStr, "RbspHrdFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.hrdDataFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.vuiDataEnable) {
		sprintf(tempStr, "RbspVuiFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.vuiDataFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}

	if (pEncCfg->hevcCfg.losslessEnable) {
		pEncCfg->hevcCfg.disableDeblk = 1;
		pEncCfg->hevcCfg.saoEnable = 0;
		pEncCfg->RcEnable = 0;
	}

#ifdef SUPPORT_HOST_RC_PARAM
	if (pEncCfg->hevcCfg.hostPicRcEnable) {
		sprintf(tempStr, "HostPicRcFile");
		if (HEVC_GetStringValue(pstrArray, array_cnt, tempStr, sValue) != 0) {
			if (sscanf(sValue, "%s\n", pEncCfg->hevcCfg.hostPicRcFileName) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		}
	}
#endif

	ret = 1; /* Success */

__end_parse:
	vfree(sRead);
	vfree(sValue);
	vfree(tempStr);
	return ret;
}


int GetValueFrameInfo(char **ppArray, int ArrayCnt, char *para, char *value1, char *value2,
			     char *value3, char *value4, char *value5,
			     int init_ref_poc)
{
	static int LineNum = 1;
	char lineStr[256];
	char paraStr[256];
	int i = 0;

	while (i < ArrayCnt && ppArray[i]) {
		memset(lineStr, 0, 256);
		memset(paraStr, 0, 256);
		memcpy(lineStr, ppArray[i], strlen(ppArray[i]));

		*value1 = *value2 = *value4 = *value5 = 0;
		*value3 = init_ref_poc;

		if (sscanf(lineStr, "%s %s %s %s %s %s", paraStr, value1, value2, value3, value4, value5) != 6)
			VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		if (paraStr[0] != ';') {
			if (strcmp(para, paraStr) == 0) {
				// printf("    * %s %s\n", paraStr, value);
				return 1;
			}
		}
		// printf("Error in cmd file line <%d> : %s",
		LineNum++;
	}

	return 1;
}

static int GetValues(char **ppArray, int ArrayCnt, char *para, int *values, int num)
{
	char line[256];
	int j = 0;
	int i;

	while (j < ArrayCnt && ppArray[j]) {
		char *str;

		memset(line, 0, 256);
		memcpy(line, ppArray[j], strlen(ppArray[j]));
		j++;
		// empty line
		str = cvi_strtok(line, " ");
		if (str == NULL)
			continue;

		if (strcmp(str, para) != 0)
			continue;

		for (i = 0; i < num; i++) {
			str = cvi_strtok(NULL, " ");
			if (str == NULL)
				return 1;
			if (!cvi_isdigit((Int32)str[0]))
				return 1;
			values[i] = atoi(str);
		}
		return 1;
	}

	return 0;
}


static int GetValue(char **ppArray, int ArrayCnt, char *para, char *value)
{
	char lineStr[256];
	char paraStr[256];
	int i = 0;

	while (i < ArrayCnt && ppArray[i]) {
		memset(lineStr, 0, 256);
		memcpy(lineStr, ppArray[i], 256);
		if (sscanf(lineStr, "%s %s", paraStr, value) != 2)
			VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		if (paraStr[0] != ';') {
			if (strcmp(para, paraStr) == 0) {
				return 1;
			}
		}
		i++;
	}

	return 0;
}

int parseAvcCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
	osal_file_t fp;
	int ret = 0;
	int i;
	int array_cnt = 0;
	char *save_ptr;
	char *pstrArray[120];
	char sValue[256];
	char *sRead = vmalloc(1000*20);
	char *pstr = sRead;

	if (!sRead) {
		return -1;
	}
	memset(sRead, 0, 1000*20);
	fp = osal_fopen(FileName, "rb");
	if (fp == NULL) {
		CVI_VC_ERR("open fail\n");
		vfree(sRead);
		return 0;
	}

	ret = osal_fread(sRead, 1, 1000*20, fp);
	osal_fclose(fp);

	i = 0;
	while (NULL != (pstrArray[i] = cvi_strtok_r(pstr, "\n", &save_ptr))) {
		i++;
		if (i >= 120) {
			break;
		}
		pstr = NULL;  //important
	}
	array_cnt = i;

	if (GetValue(pstrArray, array_cnt, "YUV_SRC_IMG", sValue) == 0)
		goto __end_parseAvcCfgFile;
	else
		strcpy(pEncCfg->SrcFileName, sValue);
	if (GetValue(pstrArray, array_cnt, "OUT_BS", sValue) != 0) {
		strcpy(pEncCfg->BitStreamFileName, sValue);
		CVI_VC_INFO("pEncCfg->BitStreamFileName %s\n",
			    pEncCfg->BitStreamFileName);
	}
	if (GetValue(pstrArray, array_cnt, "FRAME_NUMBER_ENCODED", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->NumFrame = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "PICTURE_WIDTH", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->PicX = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "PICTURE_HEIGHT", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->PicY = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "FRAME_RATE", sValue) == 0)
		goto __end_parseAvcCfgFile;
	{
		double frameRate;
		int timeRes, timeInc;

#ifdef ANDROID
		frameRate = atoi(sValue);
#else
		//frameRate = atof(sValue);
		frameRate = atoi(sValue);
#endif
		timeInc = 1;
		while ((int)frameRate != frameRate) {
			timeInc *= 10;
			frameRate *= 10;
		}
		timeRes = (int)frameRate;
		// divide 2 or 5
		if (timeInc % 2 == 0 && timeRes % 2 == 0) {
			timeInc >>= 1;
			timeRes >>= 1;
		}
		if (timeInc % 5 == 0 && timeRes % 5 == 0) {
			timeInc /= 5;
			timeRes /= 5;
		}

		if (timeRes == 2997 && timeInc == 100) {
			timeRes = 30000;
			timeInc = 1001;
		}
		pEncCfg->FrameRate = (timeInc - 1) << 16;
		pEncCfg->FrameRate |= timeRes;
	}
	if (GetValue(pstrArray, array_cnt, "CONSTRAINED_INTRA", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->ConstIntraPredFlag = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "DISABLE_DEBLK", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->DisableDeblk = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "DEBLK_ALPHA", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->DeblkOffsetA = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "DEBLK_BETA", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->DeblkOffsetB = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "CHROMA_QP_OFFSET", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->ChromaQpOffset = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "LEVEL", sValue) == 0) {
		pEncCfg->level = 0; // note : 0 means auto calculation.
	} else {
		pEncCfg->level = atoi(sValue);
		if (pEncCfg->level < 0 || pEncCfg->level > 51)
			goto __end_parseAvcCfgFile;
	}
	if (GetValue(pstrArray, array_cnt, "PIC_QP_Y", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->PicQpY = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "GOP_PIC_NUMBER", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->GopPicNum = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "IDR_INTERVAL", sValue) == 0)
		pEncCfg->IDRInterval = 0;
	else
		pEncCfg->IDRInterval = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "SLICE_MODE", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->SliceMode = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "SLICE_SIZE_MODE", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->SliceSizeMode = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "SLICE_SIZE_NUMBER", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->SliceSizeNum = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "AUD_ENABLE", sValue) == 0)
		pEncCfg->aud_en = 0;
	else
		pEncCfg->aud_en = atoi(sValue);

	/**
	 * Error Resilience
	 */
	// Intra Cost Weight : not mandatory. default zero
	if (GetValue(pstrArray, array_cnt, "WEIGHT_INTRA_COST", sValue) == 0)
		pEncCfg->intraCostWeight = 0;
	else
		pEncCfg->intraCostWeight = atoi(sValue);

	/**
	 * CROP information
	 */
	if (GetValue(pstrArray, array_cnt, "FRAME_CROP_LEFT", sValue) == 0)
		pEncCfg->frameCropLeft = 0;
	else
		pEncCfg->frameCropLeft = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "FRAME_CROP_RIGHT", sValue) == 0)
		pEncCfg->frameCropRight = 0;
	else
		pEncCfg->frameCropRight = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "FRAME_CROP_TOP", sValue) == 0)
		pEncCfg->frameCropTop = 0;
	else
		pEncCfg->frameCropTop = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "FRAME_CROP_BOTTOM", sValue) == 0)
		pEncCfg->frameCropBottom = 0;
	else
		pEncCfg->frameCropBottom = atoi(sValue);
	/**
	 * ME Option
	 */

	if (GetValue(pstrArray, array_cnt, "ME_USE_ZERO_PMV", sValue) == 0)
		pEncCfg->MeUseZeroPmv = 0;
	else
		pEncCfg->MeUseZeroPmv = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "ME_BLK_MODE_ENABLE", sValue) == 0)
		pEncCfg->MeBlkModeEnable = 0;
	else
		pEncCfg->MeBlkModeEnable = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "RATE_CONTROL_ENABLE", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->RcEnable = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "BIT_RATE_KBPS", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->RcBitRate = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "DELAY_IN_MS", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->RcInitDelay = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "VBV_BUFFER_SIZE", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->RcBufSize = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "INTRA_MB_REFRESH", sValue) == 0)
		goto __end_parseAvcCfgFile;
	pEncCfg->IntraRefreshNum = atoi(sValue);

	pEncCfg->ConscIntraRefreshEnable = 0;
	pEncCfg->CountIntraMbEnable = 0;
	pEncCfg->FieldSeqIntraRefreshEnable = 0;
	if (pEncCfg->IntraRefreshNum > 0) {
		if (GetValue(pstrArray, array_cnt, "CONSC_INTRA_REFRESH_EN", sValue) == 0)
			pEncCfg->ConscIntraRefreshEnable = 0;
		else
			pEncCfg->ConscIntraRefreshEnable = atoi(sValue);
		if (GetValue(pstrArray, array_cnt, "COUNT_INTRA_MB_EN", sValue) == 0)
			pEncCfg->CountIntraMbEnable = 0;
		else
			pEncCfg->CountIntraMbEnable = atoi(sValue);

		if (GetValue(pstrArray, array_cnt, "FIELD_SEQ_INTRA_REFRESH_EN", sValue) == 0)
			pEncCfg->FieldSeqIntraRefreshEnable = 0;
		else
			pEncCfg->FieldSeqIntraRefreshEnable = atoi(sValue);
	}

	if (GetValue(pstrArray, array_cnt, "FRAME_SKIP_DISABLE", sValue) == 0)
		pEncCfg->frameSkipDisable = 0;
	else
		pEncCfg->frameSkipDisable = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "CONST_INTRAQP_ENABLE", sValue) == 0)
		pEncCfg->ConstantIntraQPEnable = 0;
	else
		pEncCfg->ConstantIntraQPEnable = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "RC_INTRA_QP", sValue) == 0)
		pEncCfg->RCIntraQP = 0;
	else
		pEncCfg->RCIntraQP = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "MAX_QP_SET_ENABLE", sValue) == 0)
		pEncCfg->MaxQpSetEnable = 0;
	else
		pEncCfg->MaxQpSetEnable = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "MAX_QP", sValue) == 0)
		pEncCfg->MaxQp = 0;
	else
		pEncCfg->MaxQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "MIN_QP_SET_ENABLE", sValue) == 0)
		pEncCfg->MinQpSetEnable = 0;
	else
		pEncCfg->MinQpSetEnable = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "MIN_QP", sValue) == 0)
		pEncCfg->MinQp = 0;
	else
		pEncCfg->MinQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "MAX_DELTA_QP_SET_ENABLE", sValue) == 0)
		pEncCfg->MaxDeltaQpSetEnable = 0;
	else
		pEncCfg->MaxDeltaQpSetEnable = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "MAX_DELTA_QP", sValue) == 0)
		pEncCfg->MaxDeltaQp = 0;
	else
		pEncCfg->MaxDeltaQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "MIN_DELTA_QP_SET_ENABLE", sValue) == 0)
		pEncCfg->MinDeltaQpSetEnable = 0;
	else
		pEncCfg->MinDeltaQpSetEnable = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "MIN_DELTA_QP", sValue) == 0)
		pEncCfg->MinDeltaQp = 0;
	else
		pEncCfg->MinDeltaQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "GAMMA_SET_ENABLE", sValue) == 0)
		pEncCfg->GammaSetEnable = 0;
	else
		pEncCfg->GammaSetEnable = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "GAMMA", sValue) == 0)
		pEncCfg->Gamma = 0;
	else
		pEncCfg->Gamma = atoi(sValue);
	/* CODA960 features */
	if (GetValue(pstrArray, array_cnt, "RC_INTERVAL_MODE", sValue) == 0)
		pEncCfg->rcIntervalMode = 0;
	else
		pEncCfg->rcIntervalMode = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "RC_MB_INTERVAL", sValue) == 0)
		pEncCfg->RcMBInterval = 0;
	else
		pEncCfg->RcMBInterval = atoi(sValue);
	/***************************************/
	if (GetValue(pstrArray, array_cnt, "MAX_INTRA_SIZE", sValue) == 0)
		pEncCfg->RcMaxIntraSize = 0;
	else
		pEncCfg->RcMaxIntraSize = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "RC_INTERVAL_MODE", sValue) == 0)
		pEncCfg->rcIntervalMode = 0;
	else {
		pEncCfg->rcIntervalMode = atoi(sValue);
		if (pEncCfg->rcIntervalMode != 0 &&
		    pEncCfg->rcIntervalMode != 3) {
			VLOG(ERR, "RC_INTERVAL_MODE Error: %d\n",
			     pEncCfg->rcIntervalMode);
			goto __end_parseAvcCfgFile;
		}
	}
	if (GetValue(pstrArray, array_cnt, "RC_MB_INTERVAL", sValue) == 0)
		pEncCfg->RcMBInterval = 1;
	else {
		pEncCfg->RcMBInterval = atoi(sValue); // [1-64]
	}

	if (pEncCfg->RcEnable == 0) {
		pEncCfg->RcGopIQpOffsetEn = 0;
		pEncCfg->RcGopIQpOffset = 0;
	} else {
		if (GetValue(pstrArray, array_cnt, "RC_GOP_I_QP_OFFSET_EN", sValue) == 0) {
			pEncCfg->RcGopIQpOffsetEn = 0;
			pEncCfg->RcGopIQpOffset = 0;
		} else {
			pEncCfg->RcGopIQpOffsetEn = atoi(sValue);

			if (GetValue(pstrArray, array_cnt, "RC_GOP_I_QP_OFFSET", sValue) == 0)
				pEncCfg->RcGopIQpOffset = 0;
			else
				pEncCfg->RcGopIQpOffset = atoi(sValue);

			if (pEncCfg->RcGopIQpOffset < -4)
				pEncCfg->RcGopIQpOffset = -4;
			else if (pEncCfg->RcGopIQpOffset > 4)
				pEncCfg->RcGopIQpOffset = 4;
		}
	}
	if (GetValue(pstrArray, array_cnt, "SEARCH_RANGE_X", sValue) == 0) // 3: -16~15, 2:-32~31,
		// 1:-48~47, 0:-64~63,
		// H.263(Short Header :
		// always 0)
		pEncCfg->SearchRangeX = 0;
	else
		pEncCfg->SearchRangeX = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "SEARCH_RANGE_Y", sValue) == 0) // 2: -16~15, 1:-32~31,
		// 0:-48~47,
		// H.263(Short Header :
		// always 0)
		pEncCfg->SearchRangeY = 0;
	else
		pEncCfg->SearchRangeY = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "ENTROPY_CODING_MODE", sValue) == 0) // 0 : CAVLC, 1 :
		// CABAC, 2:
		// CAVLC/CABAC
		// select
		// according to
		// PicType
		pEncCfg->entropyCodingMode = 0;
	else
		pEncCfg->entropyCodingMode = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "CABAC_INIT_IDC", sValue) == 0) // cabac init idc ( 0 ~
		// 2 )
		pEncCfg->cabacInitIdc = 0;
	else
		pEncCfg->cabacInitIdc = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "TRANSFORM_8X8_MODE", sValue) == 0) // 0 : disable(BP),
		// 1 : enable(HP)
		pEncCfg->transform8x8Mode = 0;
	else
		pEncCfg->transform8x8Mode = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "CHROMA_FORMAT_400", sValue) == 0) // 0 : 420, 1 : 400
		// (MONO mode
		// allowed only on
		// HP)
		pEncCfg->chroma_format_400 = 0;
	else
		pEncCfg->chroma_format_400 = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "INTERLACED_PIC", sValue) == 0)
		pEncCfg->field_flag = 0;
	else
		pEncCfg->field_flag = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "FIELD_REFERENCE_MODE", sValue) == 0)
		pEncCfg->field_ref_mode = 1;
	else
		pEncCfg->field_ref_mode = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "EN_ROI", sValue) == 0)
		pEncCfg->coda9RoiEnable = 0;
	else
		pEncCfg->coda9RoiEnable = atoi(sValue) != 0;

	if (pEncCfg->coda9RoiEnable == 1) {
		if (GetValue(pstrArray, array_cnt, "ROI_FILE", sValue) != 0)
			snprintf(pEncCfg->RoiFile, MAX_FILE_PATH, "%s", sValue);

		if (GetValue(pstrArray, array_cnt, "ROI_PIC_AVG_QP", sValue) == 0)
			pEncCfg->RoiPicAvgQp = 25;
		else
			pEncCfg->RoiPicAvgQp = atoi(sValue);
	}

	if (GetValue(pstrArray, array_cnt, "SET_CYCLIC_PIC_NUM", sValue) == 0)
		pEncCfg->set_dqp_pic_num = 1;
	else
		pEncCfg->set_dqp_pic_num = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "GOP_PRESET", sValue) == 0)
		pEncCfg->GopPreset = 1;
	else
		pEncCfg->GopPreset = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "LONG_TERM_PERIOD", sValue) != 0)
		pEncCfg->LongTermPeriod = atoi(sValue);
	else
		pEncCfg->LongTermPeriod = -1;
	if (GetValue(pstrArray, array_cnt, "LONG_TERM_DELTA_QP", sValue) != 0)
		pEncCfg->LongTermDeltaQp = atoi(sValue);
	else
		pEncCfg->LongTermDeltaQp = 0;

	if (GetValue(pstrArray, array_cnt, "VIRTUAL_I_PERIOD", sValue) != 0)
		pEncCfg->VirtualIPeriod = atoi(sValue);
	else
		pEncCfg->VirtualIPeriod = 0;
	// LongTermPeriod should be larger than 1
	if (pEncCfg->LongTermPeriod == 0)
		pEncCfg->LongTermPeriod = -1;

	if (GetValue(pstrArray, array_cnt, "HVS_QP_SCALE_DIV2", sValue) == 0)
		pEncCfg->HvsQpScaleDiv2 = 4;
	else
		pEncCfg->HvsQpScaleDiv2 = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "EN_HVS_QP", sValue) == 0)
		pEncCfg->EnHvsQp = 1;
	else
		pEncCfg->EnHvsQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "EN_ROW_LEVEL_RC", sValue) == 0)
		pEncCfg->EnRowLevelRc = 1;
	else
		pEncCfg->EnRowLevelRc = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "RC_INITIAL_QP", sValue) == 0)
		pEncCfg->RcInitialQp = -1;
	else
		pEncCfg->RcInitialQp = atoi(sValue);

	if (GetValue(pstrArray, array_cnt, "HVS_MAX_DELTA_QP", sValue) == 0)
		pEncCfg->RcHvsMaxDeltaQp = 10;
	else
		pEncCfg->RcHvsMaxDeltaQp = atoi(sValue);
#ifdef ROI_MB_RC
	if (GetValue(pstrArray, array_cnt, "QP_MAP_MAX_DELTA_QP_PLUSE", sValue) != 0)
		pEncCfg->roi_max_delta_qp_plus = atoi(sValue);
	else
		pEncCfg->roi_max_delta_qp_plus = 0;

	if (GetValue(pstrArray, array_cnt, "QP_MAP_MAX_DELTA_QP_MINUS", sValue) != 0)
		pEncCfg->roi_max_delta_qp_minus = atoi(sValue);
	else
		pEncCfg->roi_max_delta_qp_minus = 0;
#endif

#ifdef AUTO_FRM_SKIP_DROP
	if (pEncCfg->RcEnable == 4) {
		if (GetValue(pstrArray, array_cnt, "EN_AUTO_FRM_SKIP", sValue) != 0)
			pEncCfg->enAutoFrmSkip = atoi(sValue);
		else
			pEncCfg->enAutoFrmSkip = 0;

		if (GetValue(pstrArray, array_cnt, "EN_AUTO_FRM_DROP", sValue) != 0)
			pEncCfg->enAutoFrmDrop = atoi(sValue);
		else
			pEncCfg->enAutoFrmDrop = 0;

		if (GetValue(pstrArray, array_cnt, "VBV_THRESHOLD", sValue) != 0) {
			pEncCfg->vbvThreshold = atoi(sValue);
			if (pEncCfg->vbvThreshold > 3000)
				pEncCfg->vbvThreshold = 3000;
			else if (pEncCfg->vbvThreshold < 10)
				pEncCfg->vbvThreshold = 10;
		} else
			pEncCfg->vbvThreshold = 3000;

		if (GetValue(pstrArray, array_cnt, "QP_THRESHOLD", sValue) != 0) {
			pEncCfg->qpThreshold = atoi(sValue);
			if (pEncCfg->qpThreshold > 51)
				pEncCfg->qpThreshold = 51;
			else if (pEncCfg->qpThreshold < 0)
				pEncCfg->qpThreshold = 0;
		} else
			pEncCfg->qpThreshold = 0;

		if (GetValue(pstrArray, array_cnt, "MAX_CONTINUOUS_FRAME_SKIP_NUM", sValue) != 0)
			pEncCfg->maxContinuosFrameSkipNum = atoi(sValue);
		else
			pEncCfg->maxContinuosFrameSkipNum = 0;

		if (GetValue(pstrArray, array_cnt, "MAX_CONTINUOUS_FRAME_DROP_NUM", sValue) != 0)
			pEncCfg->maxContinuosFrameDropNum = atoi(sValue);
		else
			pEncCfg->maxContinuosFrameDropNum = 0;

		if (GetValue(pstrArray, array_cnt, "RC_WEIGHT_UPDATE_FACTOR", sValue) != 0)
			pEncCfg->rcWeightFactor = atoi(sValue);
		else
			pEncCfg->rcWeightFactor = 1;
	}
#endif /* AUTO_FRM_SKIP_DROP */

	osal_memset(pEncCfg->skipPicNums, 0, sizeof(pEncCfg->skipPicNums));
	GetValues(pstrArray, array_cnt, "SKIP_PIC_NUMS", pEncCfg->skipPicNums,
		  sizeof(pEncCfg->skipPicNums));
	if (GetValue(pstrArray, array_cnt, "INTERVIEW_ENABLE", sValue) == 0)
		pEncCfg->interviewEn = 0;
	else
		pEncCfg->interviewEn = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "PARASET_REFRESH_ENABLE", sValue) == 0)
		pEncCfg->parasetRefreshEn = 0;
	else
		pEncCfg->parasetRefreshEn = atoi(sValue);
	if (GetValue(pstrArray, array_cnt, "PREFIX_NAL_ENABLE", sValue) == 0)
		pEncCfg->prefixNalEn = 0;
	else
		pEncCfg->prefixNalEn = atoi(sValue);
	/**
	 * VUI Parameter
	 */
	if (GetValue(pstrArray, array_cnt, "VUI_PARAMETERS_PRESENT_FLAG", sValue) == 0)
		pEncCfg->VuiPresFlag = 0;
	else
		pEncCfg->VuiPresFlag = atoi(sValue);

	if (pEncCfg->VuiPresFlag == 1) {
		if (GetValue(pstrArray, array_cnt, "VIDEO_SIGNAL_TYPE_PRESENT_FLAG", sValue) == 0)
			pEncCfg->VideoSignalTypePresFlag = 0;
		else
			pEncCfg->VideoSignalTypePresFlag = atoi(sValue);

		if (pEncCfg->VideoSignalTypePresFlag) {
			if (GetValue(pstrArray, array_cnt, "VIDEO_FORMAT", sValue) == 0)
				pEncCfg->VideoFormat = 5;
			else
				pEncCfg->VideoFormat = atoi(sValue);

			if (GetValue(pstrArray, array_cnt, "VIDEO_FULL_RANGE_FLAG", sValue) == 0)
				pEncCfg->VideoFullRangeFlag = 0;
			else
				pEncCfg->VideoFullRangeFlag = atoi(sValue);

			if (GetValue(pstrArray, array_cnt, "COLOUR_DESCRIPTION_PRESENT_FLAG",
				     sValue) == 0)
				pEncCfg->ColourDescripPresFlag = 1;
			else
				pEncCfg->ColourDescripPresFlag = atoi(sValue);

			if (pEncCfg->ColourDescripPresFlag) {
				if (GetValue(pstrArray, array_cnt, "COLOUR_PRIMARIES", sValue) ==
				    0)
					pEncCfg->ColourPrimaries = 1;
				else
					pEncCfg->ColourPrimaries = atoi(sValue);

				if (GetValue(pstrArray, array_cnt, "TRANSFER_CHARACTERISTICS",
					     sValue) == 0)
					pEncCfg->TransferCharacteristics = 2;
				else
					pEncCfg->TransferCharacteristics =
						atoi(sValue);

				if (GetValue(pstrArray, array_cnt, "MATRIX_COEFFICIENTS",
					     sValue) == 0)
					pEncCfg->MatrixCoeff = 2;
				else
					pEncCfg->MatrixCoeff = atoi(sValue);
			}
		}
	}
	if (GetValue(pstrArray, array_cnt, "CHANGE_PARAM_CONFIG_FILE", sValue) != 0) {
		strcpy(pEncCfg->pchChangeCfgFileName, sValue);
		if (GetValue(pstrArray, array_cnt, "CHANGE_PARAM_FRAME_NUM", sValue) != 0)
			pEncCfg->ChangeFrameNum = atoi(sValue);
		else
			pEncCfg->ChangeFrameNum = 0;
	} else {
		/* Disable parameter change */
		strcpy(pEncCfg->pchChangeCfgFileName, "");
		pEncCfg->ChangeFrameNum = 0;
	}
	ret = 1; /* Success */
__end_parseAvcCfgFile:
	vfree(sRead);
	return ret;
}


#ifdef REDUNDENT_CODE
int ParseChangeParamCfgFile(ENC_CFG *pEncCfg, char *FileName)
{
	FILE *fp;
	char sValue[1024];

	/* Initialize enable flags */
	pEncCfg->paraEnable = 0;

	fp = osal_fopen(FileName, "rt");
	if (fp == NULL) {
		fprintf(stderr, "File not exist <%s>\n", FileName);
		return 0;
	}

	/*
	Change Param Enable
	[1] : GOP_NUM
	[2] : RC_INTRA_QP
	[3] : RC_BIT_RATE
	[4] : RC_FRAME_RATE
	[5] : RC_INTRA_REFRESH
    */
	if (GetValue(fp, "PARAM_CHANGE_ENABLE", sValue) != 0)
		pEncCfg->paraEnable = (PARA_CHANGE_ENABLE)atoi(sValue);
	else
		pEncCfg->paraEnable = (PARA_CHANGE_ENABLE)0;

	/* Change GOP NUM */
	if (pEncCfg->paraEnable & EN_GOP_NUM) {
		if (GetValue(fp, "PARAM_CHANGE_GOP_NUM", sValue) != 0)
			pEncCfg->NewGopNum = atoi(sValue);
		else
			pEncCfg->NewGopNum = 30;
	}
	/* Change Constant Intra QP */
	if (pEncCfg->paraEnable & EN_RC_INTRA_QP) {
		// if (GetValue(fp, "PARAM_CHANGE_CONST_INTRAQP_EN", sValue)!=0)
		//    pEncCfg->NewIntraQpEn           = atoi(sValue);
		// else
		//    pEncCfg->NewIntraQpEn           = 0;
		if (GetValue(fp, "PARAM_CHANGE_INTRA_QP", sValue) != 0)
			pEncCfg->NewIntraQp = atoi(sValue);
		else
			pEncCfg->NewIntraQp = 25;
	}
	/* Change Bitrate */
	if (pEncCfg->paraEnable & EN_RC_BIT_RATE) {
		if (GetValue(fp, "PARAM_CHANGE_BIT_RATE", sValue) != 0)
			pEncCfg->NewBitrate = atoi(sValue);
		else
			pEncCfg->NewBitrate = 0; // RC will be disabled
	}
	/* Change Frame Rate */
	if (pEncCfg->paraEnable & EN_RC_FRAME_RATE) {
		if (GetValue(fp, "PARAM_CHANGE_FRAME_RATE", sValue) != 0) {
			int frame_rate;
			frame_rate = atoi(sValue);

			pEncCfg->NewFrameRate = frame_rate;
		} else {
			pEncCfg->NewFrameRate = 30;
		}
	}
	/* Change Intra Refresh */
	if (pEncCfg->paraEnable & EN_INTRA_REFRESH) {
		if (GetValue(fp, "PARAM_CHANGE_INTRA_REFRESH", sValue) != 0)
			pEncCfg->NewIntraRefresh = atoi(sValue);
		else
			pEncCfg->NewIntraRefresh = 0;
	}

	if (pEncCfg->paraEnable & EN_RC_MIN_MAX_QP_CHANGE) {
		if (GetValue(fp, "PARAM_CHANGE_MAX_QP_I", sValue) != 0) {
			pEncCfg->minMaxQpParam.maxQpI = atoi(sValue);
			pEncCfg->minMaxQpParam.maxQpIEnable = 1;
		} else
			pEncCfg->minMaxQpParam.maxQpIEnable = 0;

		if (GetValue(fp, "PARAM_CHANGE_MIN_QP_I", sValue) != 0) {
			pEncCfg->minMaxQpParam.minQpI = atoi(sValue);
			pEncCfg->minMaxQpParam.minQpIEnable = 1;
		} else
			pEncCfg->minMaxQpParam.minQpIEnable = 0;

		if (GetValue(fp, "PARAM_CHANGE_MAX_QP_P", sValue) != 0) {
			pEncCfg->minMaxQpParam.maxQpP = atoi(sValue);
			pEncCfg->minMaxQpParam.maxQpPEnable = 1;
		} else
			pEncCfg->minMaxQpParam.maxQpPEnable = 0;

		if (GetValue(fp, "PARAM_CHANGE_MIN_QP_P", sValue) != 0) {
			pEncCfg->minMaxQpParam.minQpP = atoi(sValue);
			pEncCfg->minMaxQpParam.minQpPEnable = 1;
		} else
			pEncCfg->minMaxQpParam.minQpPEnable = 0;
	}

#if defined(RC_PIC_PARACHANGE) && defined(RC_CHANGE_PARAMETER_DEF)
	if (pEncCfg->paraEnable & EN_PIC_PARA_CHANGE) {
		if (GetValue(fp, "PARAM_CHANGE_MAX_DELTA_QP", sValue) != 0) {
			pEncCfg->MaxDeltaQp = atoi(sValue);
			pEncCfg->MaxDeltaQpSetEnable = 1;
		} else {
			pEncCfg->MaxDeltaQpSetEnable = 0;
			pEncCfg->MaxDeltaQp = 51;
		}

		if (GetValue(fp, "PARAM_CHANGE_MIN_DELTA_QP", sValue) != 0) {
			pEncCfg->MinDeltaQp = atoi(sValue);
			pEncCfg->MinDeltaQpSetEnable = 1;
		} else {
			pEncCfg->MinDeltaQp = 0;
			pEncCfg->MinDeltaQpSetEnable = 0;
		}

		if (GetValue(fp, "PARAM_CHANGE_INTERVAL_MODE", sValue) != 0)
			pEncCfg->rcIntervalMode = atoi(sValue);
		else
			pEncCfg->rcIntervalMode = 0;

		if (pEncCfg->rcIntervalMode != 0 &&
		    pEncCfg->rcIntervalMode != 3) { // support only 3
			printf("Check RC_INTERVAL_MODE: %d (we support only mode 0, 3)\n",
			       pEncCfg->rcIntervalMode);
			return 0;
		}

		if (GetValue(fp, "PARAM_CHANGE_HVS_QP_SCALE_DIV2", sValue) == 0)
			pEncCfg->HvsQpScaleDiv2 = 4;
		else
			pEncCfg->HvsQpScaleDiv2 = atoi(sValue);

		if (GetValue(fp, "PARAM_CHANGE_EN_HVS_QP", sValue) == 0)
			pEncCfg->EnHvsQp = 1;
		else
			pEncCfg->EnHvsQp = atoi(sValue);

		if (pEncCfg->EnHvsQp == 1 && pEncCfg->HvsQpScaleDiv2 == 0) {
			printf(" HVS_QP_SCALE_DIV2 is larger than zero");
			return 0;
		}

		if (GetValue(fp, "PARAM_CHANGE_ROW_LEVEL_RC", sValue) == 0)
			pEncCfg->EnRowLevelRc = 1;
		else
			pEncCfg->EnRowLevelRc = atoi(sValue);

		if (GetValue(fp, "PARAM_CHANGE_HVS_MAX_DELTA_QP", sValue) == 0)
			pEncCfg->RcHvsMaxDeltaQp = 10;
		else
			pEncCfg->RcHvsMaxDeltaQp = atoi(sValue);

#ifdef AUTO_FRM_SKIP_DROP
		// if (pEncCfg->RcEnable == 4)
		{
			if (GetValue(fp, "PARAM_CHANGE_EN_AUTO_FRM_SKIP",
				     sValue) != 0)
				pEncCfg->enAutoFrmSkip = atoi(sValue);
			else
				pEncCfg->enAutoFrmSkip = 0;

			if (GetValue(fp, "PARAM_CHANGE_EN_AUTO_FRM_DROP",
				     sValue) != 0)
				pEncCfg->enAutoFrmDrop = atoi(sValue);
			else
				pEncCfg->enAutoFrmDrop = 0;

			if (GetValue(fp, "PARAM_CHANGE_VBV_THRESHOLD",
				     sValue) != 0) {
				pEncCfg->vbvThreshold = atoi(sValue);
				if (pEncCfg->vbvThreshold > 3000)
					pEncCfg->vbvThreshold = 3000;
				else if (pEncCfg->vbvThreshold < 10)
					pEncCfg->vbvThreshold = 10;
			} else
				pEncCfg->vbvThreshold = 3000;

			if (GetValue(fp, "PARAM_CHANGE_QP_THRESHOLD", sValue) !=
			    0) {
				pEncCfg->qpThreshold = atoi(sValue);
				if (pEncCfg->qpThreshold > 51)
					pEncCfg->qpThreshold = 51;
				else if (pEncCfg->qpThreshold < 0)
					pEncCfg->qpThreshold = 0;
			} else
				pEncCfg->qpThreshold = 0;

			if (GetValue(
				    fp,
				    "PARAM_CHANGE_MAX_CONTINUOUS_FRAME_SKIP_NUM",
				    sValue) != 0)
				pEncCfg->maxContinuosFrameSkipNum =
					atoi(sValue);
			else
				pEncCfg->maxContinuosFrameSkipNum = 0;

			if (GetValue(
				    fp,
				    "PARAM_CHANGE_MAX_CONTINUOUS_FRAME_DROP_NUM",
				    sValue) != 0)
				pEncCfg->maxContinuosFrameDropNum =
					atoi(sValue);
			else
				pEncCfg->maxContinuosFrameDropNum = 0;

			if (GetValue(fp, "PARAM_CHANGE_GAMMA_SET_ENABLE",
				     sValue) == 0)
				pEncCfg->GammaSetEnable = 0;
			else
				pEncCfg->GammaSetEnable = atoi(sValue);

			if (GetValue(fp, "PARAM_CHANGE_GAMMA", sValue) == 0)
				pEncCfg->Gamma = 0;
			else
				pEncCfg->Gamma = atoi(sValue);

			if (pEncCfg->GammaSetEnable == 0)
				pEncCfg->Gamma = -1;

			if (GetValue(fp, "PARAM_CHANGE_DELAY_IN_MS", sValue) ==
			    0)
				pEncCfg->RcInitDelay = 1000;
			else
				pEncCfg->RcInitDelay = atoi(sValue);

			if (GetValue(fp, "PARAM_CHANGE_SET_DPQ_PIC_NUM",
				     sValue) == 0)
				pEncCfg->set_dqp_pic_num = 0;
			else
				pEncCfg->set_dqp_pic_num = atoi(sValue);

			if (GetValue(fp, "PARAM_CHNAGE_GOP_I_QP_OFFSET_EN",
				     sValue) == 0) {
				pEncCfg->RcGopIQpOffsetEn = 0;
				pEncCfg->RcGopIQpOffset = 0;
			} else {
				pEncCfg->RcGopIQpOffsetEn = atoi(sValue);

				if (GetValue(fp, "PARAM_CHANGE_GOP_I_QP_OFFSET",
					     sValue) == 0)
					pEncCfg->RcGopIQpOffset = 0;
				else
					pEncCfg->RcGopIQpOffset = atoi(sValue);

				if (pEncCfg->RcGopIQpOffset < -4)
					pEncCfg->RcGopIQpOffset = -4;
				else if (pEncCfg->RcGopIQpOffset > 4)
					pEncCfg->RcGopIQpOffset = 4;
			}

			if (pEncCfg->set_dqp_pic_num == 0) {
				printf("PARAM_CHANGE_SET_DPQ_PIC_NUM must larger than 0");
				return 0;
			}
			if (pEncCfg->set_dqp_pic_num >= 1) {
				int i;
				char name[1024];
				for (i = 0; i < pEncCfg->set_dqp_pic_num; i++) {
					sprintf(name, "Frame%d", i);
					if (GetValue(fp, name, sValue) != 0) {
						pEncCfg->dqp[i] = atoi(sValue);
					} else {
						printf("dqp Error");
						return 0;
					}
				}
			}
		}
#endif /* AUTO_FRM_SKIP_DROP */
	}
#endif /* (RC_PIC_PARACHANGE) && (RC_CHANGE_PARAMETER_DEF) */

	/* Read next config for another param change, if exist */
	if (GetValue(fp, "CHANGE_PARAM_CONFIG_FILE", sValue) != 0) {
		strcpy(pEncCfg->pchChangeCfgFileName, sValue);
		if (GetValue(fp, "CHANGE_PARAM_FRAME_NUM", sValue) != 0)
			pEncCfg->ChangeFrameNum = atoi(sValue);
		else
			pEncCfg->ChangeFrameNum = 0;
	} else {
		/* Disable parameter change */
		strcpy(pEncCfg->pchChangeCfgFileName, "");
		pEncCfg->ChangeFrameNum = 0;
	}

	osal_fclose(fp);
	return 1;
}
#endif
#endif
