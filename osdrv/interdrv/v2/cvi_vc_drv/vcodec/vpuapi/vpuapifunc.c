//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "vpuapifunc.h"
#include "product.h"
#include "coda9/coda9.h"
#include "coda9/coda9_regdefine.h"
#include "coda9/coda9_vpuconfig.h"
#include "wave/common/common_regdefine.h"
#include "wave/wave4/wave4_regdefine.h"
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define MAX_LAVEL_IDX 16
#define UNREFERENCED_PARAM(x) ((void)(x))

static const int g_anLevel[MAX_LAVEL_IDX] = { 10, 11, 11, 12, 13,
					      // 10, 16, 11, 12, 13,
					      20, 21, 22, 30, 31, 32, 40, 41,
					      42, 50, 51 };

static const int g_anLevelMaxMBPS[MAX_LAVEL_IDX] = {
	1485,  1485,   3000,   6000,   11880,  11880,  19800,  20250,
	40500, 108000, 216000, 245760, 245760, 522240, 589824, 983040
};

static const int g_anLevelMaxFS[MAX_LAVEL_IDX] = { 99,	 99,   396,   396,
						   396,	 396,  792,   1620,
						   1620, 3600, 5120,  8192,
						   8192, 8704, 22080, 36864 };

static const int g_anLevelMaxBR[MAX_LAVEL_IDX] = {
	64,    64,    192,   384,   768,   2000,  4000,	  4000,
	10000, 14000, 20000, 20000, 50000, 50000, 135000, 240000
};

static const int g_anLevelSliceRate[MAX_LAVEL_IDX] = { 0,  0,  0,  0,  0,  0,
						       0,  0,  22, 60, 60, 60,
						       24, 24, 24, 24 };

static const int g_anLevelMaxMbs[MAX_LAVEL_IDX] = {
	28, 28, 56, 56, 56, 56, 79, 113, 113, 169, 202, 256, 256, 263, 420, 543
};

/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/

RetCode InitCodecInstancePool(Uint32 coreIdx)
{
	int i;
	CodecInst *pCodecInst;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	if (vip->instance_pool_inited == 0) {
		for (i = 0; i < MAX_NUM_INSTANCE; i++) {
			pCodecInst = (CodecInst *)vip->codecInstPool[i];
			pCodecInst->instIndex = i;
			pCodecInst->inUse = 0;

			CVI_VC_INFO(
				"%d, pCodecInst->instIndex %d, pCodecInst->inUse %d\n",
				i, pCodecInst->instIndex, pCodecInst->inUse);
		}
		vip->instance_pool_inited = 1;
	}
	return RETCODE_SUCCESS;
}

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst **ppInst)
{
	int i;
	CodecInst *pCodecInst = 0;
	vpu_instance_pool_t *vip;
	Uint32 handleSize;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pCodecInst = (CodecInst *)vip->codecInstPool[i];

		if (!pCodecInst) {
			return RETCODE_FAILURE;
		}

		if (!pCodecInst->inUse) {
			break;
		}
	}

	if (i == MAX_NUM_INSTANCE) {
		*ppInst = 0;
		return RETCODE_FAILURE;
	}

	pCodecInst->inUse = 1;
	pCodecInst->coreIdx = coreIdx;
	pCodecInst->codecMode = -1;
	pCodecInst->codecModeAux = -1;
#ifdef ENABLE_CNM_DEBUG_MSG
	pCodecInst->loggingEnable = 0;
#endif
	pCodecInst->isDecoder = TRUE;
	pCodecInst->productId = ProductVpuGetId(coreIdx);
	osal_memset((void *)&pCodecInst->CodecInfo, 0x00,
		    sizeof(pCodecInst->CodecInfo));

	handleSize = sizeof(DecInfo);
	if (handleSize < sizeof(EncInfo)) {
		handleSize = sizeof(EncInfo);
	}

	pCodecInst->CodecInfo = (void *)osal_malloc(handleSize);
	if (pCodecInst->CodecInfo == NULL) {
		return RETCODE_INSUFFICIENT_RESOURCE;
	}
	osal_memset(pCodecInst->CodecInfo, 0x00, sizeof(handleSize));

	*ppInst = pCodecInst;

	if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex) < 0) {
		return RETCODE_FAILURE;
	}

	return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst *pCodecInst)
{
	pCodecInst->inUse = 0;
	pCodecInst->codecMode = -1;
	pCodecInst->codecModeAux = -1;

	vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

	osal_free(pCodecInst->CodecInfo);
	pCodecInst->CodecInfo = NULL;
}

RetCode CheckInstanceValidity(CodecInst *pCodecInst)
{
	int i;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		CVI_VC_ERR("RETCODE_INSUFFICIENT_RESOURCE\n");
		return RETCODE_INSUFFICIENT_RESOURCE;
	}

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
			return RETCODE_SUCCESS;
	}

	CVI_VC_ERR("RETCODE_INVALID_HANDLE\n");
	return RETCODE_INVALID_HANDLE;
}

/******************************************************************************
    API Subroutines
******************************************************************************/

RetCode CheckDecOpenParam(DecOpenParam *pop)
{
	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBuffer % 8) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamBufferSize % 1024 ||
	    pop->bitstreamBufferSize < 1024 ||
	    pop->bitstreamBufferSize > (256 * 1024 * 1024 - 1)) {
		return RETCODE_INVALID_PARAM;
	}

#ifdef REDUNDENT_CODE
	if (pop->bitstreamFormat != STD_AVC &&
	    pop->bitstreamFormat != STD_VC1 &&
	    pop->bitstreamFormat != STD_MPEG2 &&
	    pop->bitstreamFormat != STD_H263 &&
	    pop->bitstreamFormat != STD_MPEG4 &&
	    pop->bitstreamFormat != STD_DIV3 &&
	    pop->bitstreamFormat != STD_RV && pop->bitstreamFormat != STD_AVS &&
	    pop->bitstreamFormat != STD_THO &&
	    pop->bitstreamFormat != STD_VP3 &&
	    pop->bitstreamFormat != STD_VP8 &&
	    pop->bitstreamFormat != STD_HEVC &&
	    pop->bitstreamFormat != STD_VP9) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->mp4DeblkEnable == 1 && !(pop->bitstreamFormat == STD_MPEG4 ||
					  pop->bitstreamFormat == STD_MPEG2 ||
					  pop->bitstreamFormat == STD_DIV3)) {
		return RETCODE_INVALID_PARAM;
	}
#endif
	if (pop->bitstreamFormat != STD_AVC &&
	    pop->bitstreamFormat != STD_HEVC) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->wtlEnable && pop->tiled2LinearEnable) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->wtlEnable) {
		if (pop->wtlMode != FF_FRAME && pop->wtlMode != FF_FIELD) {
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->coreIdx > MAX_NUM_VPU_CORE) {
		return RETCODE_INVALID_PARAM;
	}

	return RETCODE_SUCCESS;
}

Uint64 GetTimestamp(EncHandle handle)
{
	CodecInst *pCodecInst = (CodecInst *)handle;
	EncInfo *pEncInfo = NULL;
	Uint64 pts;
	Uint32 fps;

	if (pCodecInst == NULL) {
		return 0;
	}

	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	fps = pEncInfo->openParam.frameRateInfo;
	if (fps == 0) {
		fps = 30; /* 30 fps */
	}

	pts = pEncInfo->curPTS;
	pEncInfo->curPTS += 90000 / fps; /* 90KHz/fps */

	return pts;
}

RetCode CalcEncCropInfo(EncHevcParam *param, int rotMode, int srcWidth,
			int srcHeight)
{
	int width32, height32, pad_right, pad_bot;
	int crop_right, crop_left, crop_top, crop_bot;
	int prp_mode = rotMode >> 1; // remove prp_enable bit

	width32 = (srcWidth + 31) & ~31;
	height32 = (srcHeight + 31) & ~31;

	pad_right = width32 - srcWidth;
	pad_bot = height32 - srcHeight;

	if (param->confWinRight > 0)
		crop_right = param->confWinRight + pad_right;
	else
		crop_right = pad_right;

	if (param->confWinBot > 0)
		crop_bot = param->confWinBot + pad_bot;
	else
		crop_bot = pad_bot;

	crop_top = param->confWinTop;
	crop_left = param->confWinLeft;

	param->confWinTop = crop_top;
	param->confWinLeft = crop_left;
	param->confWinBot = crop_bot;
	param->confWinRight = crop_right;

	if (prp_mode == 1 || prp_mode == 15) {
		param->confWinTop = crop_right;
		param->confWinLeft = crop_top;
		param->confWinBot = crop_left;
		param->confWinRight = crop_bot;
	} else if (prp_mode == 2 || prp_mode == 12) {
		param->confWinTop = crop_bot;
		param->confWinLeft = crop_right;
		param->confWinBot = crop_top;
		param->confWinRight = crop_left;
	} else if (prp_mode == 3 || prp_mode == 13) {
		param->confWinTop = crop_left;
		param->confWinLeft = crop_bot;
		param->confWinBot = crop_right;
		param->confWinRight = crop_top;
	} else if (prp_mode == 4 || prp_mode == 10) {
		param->confWinTop = crop_bot;
		param->confWinBot = crop_top;
	} else if (prp_mode == 8 || prp_mode == 6) {
		param->confWinLeft = crop_right;
		param->confWinRight = crop_left;
	} else if (prp_mode == 5 || prp_mode == 11) {
		param->confWinTop = crop_left;
		param->confWinLeft = crop_top;
		param->confWinBot = crop_right;
		param->confWinRight = crop_bot;
	} else if (prp_mode == 7 || prp_mode == 9) {
		param->confWinTop = crop_right;
		param->confWinLeft = crop_bot;
		param->confWinBot = crop_left;
		param->confWinRight = crop_top;
	}

	CVI_VC_CFG("confWinTop = %d\n", param->confWinTop);
	CVI_VC_CFG("confWinBot = %d\n", param->confWinBot);
	CVI_VC_CFG("confWinBot = %d\n", param->confWinBot);
	CVI_VC_CFG("confWinRight = %d\n", param->confWinRight);

	return RETCODE_SUCCESS;
}

int DecBitstreamBufEmpty(DecInfo *pDecInfo)
{
	return (pDecInfo->streamRdPtr == pDecInfo->streamWrPtr);
}

#ifdef DRAM_TEST
RetCode ExecDRAMReadWriteTest(Uint32 coreIdx, Uint32 *dram_source_addr,
			      Uint32 *dram_destination_addr,
			      Uint32 *dram_data_size)
{
	VpuAttr *pAttr;
	RetCode ret;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	if (ProductVpuIsInit(coreIdx) == 0) {
		return RETCODE_NOT_INITIALIZED;
	}

	if (GetPendingInst(coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pAttr = &g_VpuCoreAttributes[coreIdx];

	ret = ProductVpuDRAMReadWriteTest(coreIdx, dram_source_addr,
					  dram_destination_addr,
					  dram_data_size);

	return ret;
}
#endif

#ifdef REDUNDENT_CODE
RetCode SetParaSet(DecHandle handle, int paraSetType, DecParamSet *para)
{
	CodecInst *pCodecInst;
	PhysicalAddress paraBuffer;
	int i;
	Uint32 *src;

	pCodecInst = handle;
	src = para->paraSet;

	paraBuffer = VpuReadReg(pCodecInst->coreIdx, BIT_PARA_BUF_ADDR);
	for (i = 0; i < para->size; i += 4) {
		VpuWriteReg(pCodecInst->coreIdx, paraBuffer + i, *src++);
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_TYPE,
		    paraSetType); // 0: SPS, 1: PPS
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_SIZE, para->size);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, DEC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 0);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 0);
#endif

	return RETCODE_SUCCESS;
}

void DecSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr,
			PhysicalAddress paraBuffer)
{
	BYTE tempBuf[8] = {
		0,
	}; // 64bit bus & endian
	Uint32 val;

	val = paraBuffer;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}
#endif

RetCode CheckEncInstanceValidity(EncHandle handle)
{
	CodecInst *pCodecInst;
	RetCode ret;

	if (handle == NULL) {
		CVI_VC_ERR("null handle %p\n", handle);
		return RETCODE_INVALID_HANDLE;
	}
	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("CheckInstanceValidity ret %d\n", ret);
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}

	if (pCodecInst->codecMode != MP4_ENC &&
	    pCodecInst->codecMode != HEVC_ENC &&
	    pCodecInst->codecMode != AVC_ENC &&
	    pCodecInst->codecMode != C7_MP4_ENC &&
	    pCodecInst->codecMode != C7_AVC_ENC) {
		return RETCODE_INVALID_HANDLE;
	}
	return RETCODE_SUCCESS;
}

RetCode CheckEncParam(EncHandle handle, EncParam *param)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	if (param == 0) {
		CVI_VC_ERR("null param\n");
		return RETCODE_INVALID_PARAM;
	}

	if (param->setROI.mode && (pCodecInst->codecMode == AVC_ENC ||
				   pCodecInst->codecMode == C7_AVC_ENC)) {
		if (param->setROI.number < 0 ||
		    param->setROI.number > MAX_CODA980_ROI_NUMBER) {
			CVI_VC_ERR("ROI number %d out of range\n",
				   param->setROI.number);
			return RETCODE_INVALID_PARAM;
		}
	}

	if (param->skipPicture != 0 && param->skipPicture != 1) {
		CVI_VC_ERR("skipPicture %d out of range\n", param->skipPicture);
		return RETCODE_INVALID_PARAM;
	}

	if (param->skipPicture == 0) {
		if (param->sourceFrame == 0) {
			CVI_VC_ERR("null sourceFrame\n");
			return RETCODE_INVALID_FRAME_BUFFER;
		}
		if (param->forceIPicture != 0 && param->forceIPicture != 1) {
			CVI_VC_ERR("forceIPicture %d out of range\n",
				   param->forceIPicture);
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pEncInfo->openParam.bitRate == 0) { // no rate control
#ifdef REDUNDENT_CODE
		if (pCodecInst->codecMode == MP4_ENC ||
		    pCodecInst->codecMode == C7_MP4_ENC) {
			if (param->quantParam < 1 || param->quantParam > 31) {
				CVI_VC_ERR("quantParam %d out of range\n",
					   param->quantParam);
				return RETCODE_INVALID_PARAM;
			}
		} else
#endif
			if (pCodecInst->codecMode == HEVC_ENC) {
			if (param->forcePicQpEnable == 1) {
				if (param->forcePicQpI < 0 ||
				    param->forcePicQpI > 51) {
					CVI_VC_ERR(
						"forcePicQpI %d out of range\n",
						param->forcePicQpI);
					return RETCODE_INVALID_PARAM;
				}

				if (param->forcePicQpP < 0 ||
				    param->forcePicQpP > 51) {
					CVI_VC_ERR(
						"forcePicQpP %d out of range\n",
						param->forcePicQpP);
					return RETCODE_INVALID_PARAM;
				}

				if (param->forcePicQpB < 0 ||
				    param->forcePicQpB > 51) {
					CVI_VC_ERR(
						"forcePicQpB %d out of range\n",
						param->forcePicQpB);
					return RETCODE_INVALID_PARAM;
				}
			}
			if (pEncInfo->ringBufferEnable == 0) {
				if (param->picStreamBufferAddr % 16 ||
				    param->picStreamBufferSize == 0) {
					CVI_VC_ERR(
						"picStreamBufferAddr / Size\n");
					return RETCODE_INVALID_PARAM;
				}
			}
		} else { // AVC_ENC
			if (param->quantParam < 0 || param->quantParam > 51) {
				CVI_VC_ERR("quantParam %d out of range\n",
					   param->quantParam);
				return RETCODE_INVALID_PARAM;
			}
		}
	}
	if (pEncInfo->ringBufferEnable == 0) {
		if (param->picStreamBufferAddr % 8 ||
		    param->picStreamBufferSize == 0) {
			CVI_VC_ERR("BufferAddr 0x%llX BufferSize %d fail\n",
				   param->picStreamBufferAddr,
				   param->picStreamBufferSize);
			return RETCODE_INVALID_PARAM;
		}
	}

	if (param->ctuOptParam.roiEnable && param->ctuOptParam.ctuQpEnable) {
		CVI_VC_ERR("roiEnable + ctuQpEnable, not support\n");
		return RETCODE_INVALID_PARAM;
	}
	return RETCODE_SUCCESS;
}

#ifdef SUPPORT_980_ROI_RC_LIB
static int32_t fw_util_round_divide(int32_t dividend, int32_t divisor)
{
	return (dividend + (divisor >> 1)) / divisor;
}
#endif

/**
 * GetEncHeader()
 *  1. Generate encoder header bitstream
 * @param handle         : encoder handle
 * @param encHeaderParam : encoder header parameter (buffer, size, type)
 * @return none
 */
RetCode GetEncHeader(EncHandle handle, EncHeaderParam *encHeaderParam)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	EncOpenParam *encOP;
	int encHeaderCode;
	int ppsCopySize = 0;
	unsigned char pps_temp[256];
	int spsID = 0;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	int flag = 0;
	Uint32 val = 0;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	encOP = &(pEncInfo->openParam);

	if (pEncInfo->ringBufferEnable == 0) {
		if (pEncInfo->lineBufIntEn)
			val |= (0x1 << 6);
		val |= (0x1 << 5);
		val |= (0x1 << 4);

	} else {
		val |= (0x1 << 3);
	}
	val |= pEncInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (pEncInfo->ringBufferEnable == 0) {
		SetEncBitStreamInfo(pCodecInst, encHeaderParam, NULL);
	}

	if ((encHeaderParam->headerType == SPS_RBSP ||
	     encHeaderParam->headerType == SPS_RBSP_MVC) &&
	    pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_PROFILE,
			    encOP->EncStdParam.avcParam.profile);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CHROMA_FORMAT,
			    encOP->EncStdParam.avcParam.chromaFormat400);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FIELD_FLAG,
			    (encOP->EncStdParam.avcParam.fieldRefMode << 1) |
				    encOP->EncStdParam.avcParam.fieldFlag);
		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_FRAME_CROP_H, CropH);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_FRAME_CROP_V, CropV);
		}

#ifdef SUPPORT_980_ROI_RC_LIB

		// AVC don't care about that It is only used for svc_t
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME,
			    pEncInfo->max_temporal_id << 16 |
				    pEncInfo->num_ref_frames_decoder);
#endif
	}
	if (encHeaderParam->headerType == SPS_RBSP_MVC)
		spsID = 1;
	encHeaderCode =
		encHeaderParam->headerType | (flag << 3); // paraSetType>>
	// SPS: 0, PPS:
	// 1, VOS: 1,
	// VO: 2, VOL:
	// 0

	encHeaderCode |= (pEncInfo->openParam.EncStdParam.avcParam.level) << 8;
	encHeaderCode |= (spsID << 24);
	if ((encHeaderParam->headerType == PPS_RBSP ||
	     encHeaderParam->headerType == PPS_RBSP_MVC) &&
	    pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		int ActivePPSIdx = pEncInfo->ActivePPSIdx;
		AvcPpsParam *ActivePPS =
			&encOP->EncStdParam.avcParam.ppsParam[ActivePPSIdx];
		if (ActivePPS->entropyCodingMode != 2) {
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE,
				    ActivePPS->entropyCodingMode);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_INIT_IDC,
				    ActivePPS->cabacInitIdc);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_TRANSFORM_8X8,
				    ActivePPS->transform8x8Mode);
			if (encHeaderParam->headerType == PPS_RBSP)
				encHeaderCode |= (ActivePPS->ppsId << 16);
			else if (encHeaderParam->headerType == PPS_RBSP_MVC)
				encHeaderCode |= (1 << 24) |
						 ((ActivePPS->ppsId + 1) << 16);
		} else {
			int mvcPPSOffset = 0;
			if (encHeaderParam->headerType == PPS_RBSP_MVC) {
				encHeaderCode |= (1 << 24); /* sps_id: 1 */
				mvcPPSOffset = 2;
			}
			// PPS for I frame (CAVLC)
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE, 0);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_INIT_IDC,
				    ActivePPS->cabacInitIdc);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_TRANSFORM_8X8,
				    ActivePPS->transform8x8Mode);
			encHeaderCode |=
				((ActivePPS->ppsId + mvcPPSOffset) << 16);
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CODE,
				    encHeaderCode);

			Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
					     ENCODE_HEADER);
			if (vdi_wait_vpu_busy(pCodecInst->coreIdx,
					      __VPU_BUSY_TIMEOUT,
					      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
				if (pCodecInst->loggingEnable)
					vdi_log(pCodecInst->coreIdx,
						ENCODE_HEADER, 2);
#endif
				SetPendingInst(pCodecInst->coreIdx, NULL,
					       __func__, __LINE__);
				CVI_VC_ERR("SetPendingInst clear\n");
				return RETCODE_VPU_RESPONSE_TIMEOUT;
			}
#ifdef ENABLE_CNM_DEBUG_MSG
			if (pCodecInst->loggingEnable)
				vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 0);
#endif

			if (pEncInfo->ringBufferEnable == 0) {
				ppsCopySize =
					vdi_remap_memory_address(
						pCodecInst->coreIdx,
						VpuReadReg(
							pCodecInst->coreIdx,
							pEncInfo->streamWrPtrRegAddr)) -
					encHeaderParam->buf;
				VpuReadMem(pCodecInst->coreIdx,
					   encHeaderParam->buf, &pps_temp[0],
					   ppsCopySize,
					   pEncInfo->openParam.streamEndian);
			}

			// PPS for P frame (CABAC)
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE, 1);
			encHeaderCode |= ((ActivePPS->ppsId + mvcPPSOffset + 1)
					  << 16); // ppsId=1;
		}
	}

	// SET_SEI param
	if (pEncInfo->openParam.bitstreamFormat == STD_AVC &&
	    encHeaderParam->headerType == SVC_RBSP_SEI) {
		int data = 0;
		if (pEncInfo->max_temporal_id) {
			int i;
			int num_layer;
			int frm_rate[MAX_GOP_SIZE] = {
				0,
			};

			if (pEncInfo->openParam.gopSize == 1) // ||
			// seq->intra_period
			// == 1) //all
			// intra
			{
				num_layer = 1;
				frm_rate[0] = pEncInfo->openParam.frameRateInfo;
			} else {
				num_layer = 1;
				for (i = 0; i < pEncInfo->gop_size; i++) {
					// assert(seq->gop_entry[i].temporal_id
					// < MAX_GOP_SIZE);
					num_layer = MAX(
						num_layer,
						pEncInfo->gop_entry[i]
								.temporal_id +
							1);
					frm_rate[pEncInfo->gop_entry[i]
							 .temporal_id]++;
				}

				for (i = 0; i < num_layer; i++) {
					frm_rate[i] = fw_util_round_divide(
						frm_rate[i] *
							pEncInfo->openParam
								.frameRateInfo *
							256,
						pEncInfo->gop_size);
					if (i > 0)
						frm_rate[i] += frm_rate[i - 1];
				}
			}

			data = num_layer << 16 | frm_rate[0];
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_SVC_SEI_INFO1, data);
			data = frm_rate[1] << 16 | frm_rate[2];
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_SVC_SEI_INFO2, data);
		} else {
			data = 0;
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_SVC_SEI_INFO1, data);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_SVC_SEI_INFO2, data);
		}
	}

	encHeaderCode |= (encHeaderParam->zeroPaddingEnable & 1) << 31;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CODE, encHeaderCode);

	CVI_VC_TRACE("streamRdPtr = 0x%llX, streamWrPtr = 0x%llX\n",
		     pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);

	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
		    pEncInfo->streamRdPtr);
	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
		    pEncInfo->streamWrPtr);

	CVI_VC_FLOW("ENCODE_HEADER\n");

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENCODE_HEADER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 2);
#endif
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
		CVI_VC_ERR("SetPendingInst clear\n");
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 0);
#endif

	if (pEncInfo->ringBufferEnable == 0 && pEncInfo->bEsBufQueueEn == 0) {
		rdPtr = encHeaderParam->buf;
		wrPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr));

		if (ppsCopySize) {
			int size = wrPtr - rdPtr;
			VpuReadMem(pCodecInst->coreIdx, rdPtr,
				   &pps_temp[ppsCopySize], size,
				   pEncInfo->openParam.streamEndian);
			ppsCopySize += size;
			VpuWriteMem(pCodecInst->coreIdx, rdPtr, &pps_temp[0],
				    ppsCopySize,
				    pEncInfo->openParam.streamEndian);
			encHeaderParam->size = ppsCopySize;
			wrPtr = rdPtr + ppsCopySize;
			VpuWriteReg(pCodecInst->coreIdx,
				    pEncInfo->streamWrPtrRegAddr, wrPtr);
		} else {
			encHeaderParam->size = wrPtr - rdPtr;
		}
	} else {
		rdPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamRdPtrRegAddr));
		wrPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr));
		encHeaderParam->buf = rdPtr;
		encHeaderParam->size = wrPtr - rdPtr;
	}

	pEncInfo->streamWrPtr = wrPtr;
	pEncInfo->streamRdPtr = rdPtr;

	CVI_VC_BS("streamRdPtr = 0x%llX, streamWrPtr = 0x%llX, size = 0x%zX\n",
		  pEncInfo->streamRdPtr, pEncInfo->streamWrPtr,
		  encHeaderParam->size);

	return RETCODE_SUCCESS;
}

RetCode SetEncBitStreamInfo(CodecInst *pCodecInst,
			    EncHeaderParam *encHeaderParam, EncParam *param)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	PhysicalAddress paBitStreamWrPtr = VPU_ALIGN8(pEncInfo->streamWrPtr);
	PhysicalAddress paBitStreamStart;
	int bitStreamBufSize;

	if (encHeaderParam) {
		if (pEncInfo->bEsBufQueueEn == 0) {
			paBitStreamStart = encHeaderParam->buf;
			bitStreamBufSize = (int)encHeaderParam->size;
		} else {
			paBitStreamStart = paBitStreamWrPtr;
			bitStreamBufSize = (int)(pEncInfo->streamBufEndAddr -
						 paBitStreamWrPtr);
		}

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_START,
			    paBitStreamStart);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_SIZE,
			    bitStreamBufSize >> 10); // size in KB

		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
			    paBitStreamStart);
		pEncInfo->streamRdPtr = paBitStreamStart;

		CVI_VC_BS(
			"headerStreamBufferAddr = 0x%llX, headerStreamBufferSize = 0x%X\n",
			paBitStreamStart, bitStreamBufSize);

	} else if (param) {
		// reset RdPtr/WrPtr to streamBufStartAddr:
		// 1. bEsBufQueueEn disable or
		// 2. empty size (pEncInfo->streamBufEndAddr - pEncInfo->streamWrPtr) lower than threshold
		if (pEncInfo->bEsBufQueueEn == 0 ||
		    ((pEncInfo->streamBufEndAddr - paBitStreamWrPtr) <
		     H264_MIN_BITSTREAM_SIZE)) {
			paBitStreamStart = param->picStreamBufferAddr;
			bitStreamBufSize = param->picStreamBufferSize;
		} else {
			paBitStreamStart = paBitStreamWrPtr;
			bitStreamBufSize = (int)(pEncInfo->streamBufEndAddr -
						 paBitStreamWrPtr);
		}

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_BB_START,
			    paBitStreamStart);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_BB_SIZE,
			    bitStreamBufSize >> 10); // size in KB
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
			    paBitStreamStart);
		pEncInfo->streamRdPtr = paBitStreamStart;

		CVI_VC_BS(
			"picStreamBufferAddr = 0x%llX, picStreamBufferSize = 0x%X\n",
			paBitStreamStart, bitStreamBufSize);
	}

	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
/**
 * EncParaSet()
 *  1. Setting encoder header option
 *  2. Get RBSP format header in PARA_BUF
 * @param handle      : encoder handle
 * @param paraSetType : encoder header type >> SPS: 0, PPS: 1, VOS: 1, VO: 2,
 * VOL: 0
 * @return none
 */
RetCode EncParaSet(EncHandle handle, int paraSetType)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	int flag = 0;
	int encHeaderCode = paraSetType;
	EncOpenParam *encOP;
	int spsID = 0;
	int picWidth, picHeight;
	int SliceNum = 0;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	encOP = &(pEncInfo->openParam);

	if ((paraSetType == SPS_RBSP_MVC || paraSetType == SPS_RBSP) &&
	    pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CHROMA_FORMAT,
			    encOP->EncStdParam.avcParam.chromaFormat400);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FIELD_FLAG,
			    encOP->EncStdParam.avcParam.fieldFlag |
				    encOP->EncStdParam.avcParam.fieldFlag);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_PROFILE,
			    encOP->EncStdParam.avcParam.profile);

		picWidth = encOP->picWidth;
		picHeight = encOP->picHeight;

		if (encOP->sliceMode.sliceMode == 1 &&
		    encOP->sliceMode.sliceSizeMode == 1)
			SliceNum = encOP->sliceMode.sliceSize;

		if (!encOP->EncStdParam.avcParam.level) {
			if (encOP->EncStdParam.avcParam.fieldFlag)
				encOP->EncStdParam.avcParam.level =
					LevelCalculation(picWidth >> 4,
							 (picHeight + 31) >> 5,
							 encOP->frameRateInfo,
							 1, encOP->bitRate,
							 SliceNum);
			else
				encOP->EncStdParam.avcParam.level =
					LevelCalculation(picWidth >> 4,
							 picHeight >> 4,
							 encOP->frameRateInfo,
							 0, encOP->bitRate,
							 SliceNum);
			if (encOP->EncStdParam.avcParam.level < 0)
				return RETCODE_INVALID_PARAM;
		}

		encHeaderCode |= (encOP->EncStdParam.avcParam.level) << 8;

		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_FRAME_CROP_H, CropH);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_FRAME_CROP_V, CropV);
		}
	}
	encHeaderCode |= paraSetType | (flag << 3); // paraSetType>> SPS: 0,
		// PPS: 1, VOS: 1, VO: 2,
		// VOL: 0

	if (paraSetType == SPS_RBSP_MVC)
		spsID = 1;
	encHeaderCode |= (pEncInfo->openParam.EncStdParam.avcParam.level) << 8;
	encHeaderCode |= (spsID << 24);

	if ((paraSetType == PPS_RBSP || paraSetType == PPS_RBSP_MVC) &&
	    pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		int ActivePPSIdx = pEncInfo->ActivePPSIdx;
		AvcPpsParam *ActivePPS =
			&encOP->EncStdParam.avcParam.ppsParam[ActivePPSIdx];

		// PPS ID
		if (ActivePPS->entropyCodingMode != 2) {
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE,
				    ActivePPS->entropyCodingMode);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_INIT_IDC,
				    ActivePPS->cabacInitIdc);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_TRANSFORM_8X8,
				    ActivePPS->transform8x8Mode);
			encHeaderCode |= (ActivePPS->ppsId << 16);

			if (paraSetType == PPS_RBSP)
				encHeaderCode |= (ActivePPS->ppsId << 16);
			else if (paraSetType == PPS_RBSP_MVC) {
				spsID = 1;
				encHeaderCode |= ((ActivePPS->ppsId + 1)
						  << 16); /* sps_id : 1 */
				encHeaderCode |= (spsID << 24) |
						 ((ActivePPS->ppsId + 1)
						  << 16); /* sps_id : 1 */
			}
		} else {
			// CAVLC for I frame, CABAC for P frame.
			int mvcPPSOffset = 0;
			if (paraSetType == PPS_RBSP_MVC) {
				encHeaderCode |= (1 << 24); /* sps_id: 1 */
				mvcPPSOffset = 2;
			}

			// 1st PPS
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE, 0);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_INIT_IDC,
				    ActivePPS->cabacInitIdc);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_TRANSFORM_8X8,
				    ActivePPS->transform8x8Mode);
			encHeaderCode |=
				((ActivePPS->ppsId + mvcPPSOffset) << 16);

			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARA_SET_TYPE,
				    encHeaderCode);
			Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
					     ENC_PARA_SET);
			if (vdi_wait_vpu_busy(pCodecInst->coreIdx,
					      __VPU_BUSY_TIMEOUT,
					      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
				if (pCodecInst->loggingEnable)
					vdi_log(pCodecInst->coreIdx,
						ENC_PARA_SET, 2);
#endif
				return RETCODE_VPU_RESPONSE_TIMEOUT;
			}
#ifdef ENABLE_CNM_DEBUG_MSG
			if (pCodecInst->loggingEnable)
				vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 0);
#endif
			// 2nd PPS
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_HEADER_CABAC_MODE, 1);
			encHeaderCode |= ((ActivePPS->ppsId + mvcPPSOffset + 1)
					  << 16); // ppsId=1;
		}
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARA_SET_TYPE, encHeaderCode);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 0);
#endif

	return RETCODE_SUCCESS;
}

RetCode SetGopNumber(EncHandle handle, Uint32 *pGopNumber)
{
	CodecInst *pCodecInst;
	int data = 0;
	Uint32 gopNumber = *pGopNumber;

	pCodecInst = handle;

	data = 1;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_GOP_NUM,
		    gopNumber);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

RetCode SetIntraQp(EncHandle handle, Uint32 *pIntraQp)
{
	CodecInst *pCodecInst;
	int data = 0;
	Uint32 intraQp = *pIntraQp;

	pCodecInst = handle;

	data = 1 << 1;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_INTRA_QP,
		    intraQp);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}
#endif

RetCode SetBitrate(EncHandle handle, Uint32 *pBitrate)
{
	CodecInst *pCodecInst;
	int data = 0;
	Uint32 bitrate = *pBitrate;

	pCodecInst = handle;

	data = 1 << 2;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_BITRATE, bitrate);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
RetCode SetFramerate(EncHandle handle, Uint32 *pFramerate)
{
	CodecInst *pCodecInst;
	int data = 0;
	Uint32 frameRate = *pFramerate;

	pCodecInst = handle;

	data = 1 << 3;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_F_RATE,
		    frameRate);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

RetCode SetIntraRefreshNum(EncHandle handle, Uint32 *pIntraRefreshNum)
{
	CodecInst *pCodecInst;
	Uint32 intraRefreshNum = *pIntraRefreshNum;
	int data = 0;

	pCodecInst = handle;

	data = 1 << 4;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_INTRA_REFRESH,
		    intraRefreshNum);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif
	return RETCODE_SUCCESS;
}

RetCode SetSliceMode(EncHandle handle, EncSliceMode *pSliceMode)
{
	CodecInst *pCodecInst;
	Uint32 data = 0;
	int data2 = 0;

	pCodecInst = handle;

	data = 0;
	if (pSliceMode->sliceMode != 0) {
		data = (pSliceMode->sliceSize << 2) |
		       (pSliceMode->sliceSizeMode + 1); // encoding mode
	}
	data2 = 1 << 5;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data2);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_SLICE_MODE, data);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

RetCode SetHecMode(EncHandle handle, int mode)
{
	CodecInst *pCodecInst;
	Uint32 HecMode = mode;
	int data = 0;

	pCodecInst = handle;

	data = 1 << 6;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_HEC_MODE,
		    HecMode);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}
#endif

RetCode SetMinMaxQp(EncHandle handle, MinMaxQpChangeParam *param)
{
	CodecInst *pCodecInst;
	int data = 0;
	pCodecInst = handle;

	data = 1 << 6;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);

	data = (param->maxQpI << 0) | (param->maxQpIEnable << 6) |
	       (param->minQpI << 8) | (param->maxQpIEnable << 14) |
	       (param->maxQpP << 16) | (param->maxQpPEnable << 22) |
	       (param->minQpP << 24) | (param->maxQpPEnable << 30);

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_MIN_MAX_QP, data);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
#if defined(RC_PIC_PARACHANGE) && defined(RC_CHANGE_PARAMETER_DEF)
RetCode SetChangePicPara(EncHandle handle, ChangePicParam *param)
{
	CodecInst *pCodecInst;
	int data = 0;
	int gopIQpOffset = 0;
	int gamma = 0;
	pCodecInst = handle;

	data = 0x4000;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_CHANGE_ENABLE, data);

	data = param->MindeltaQp << 8 | param->MaxdeltaQp;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_MIN_MAX_DELTA_QP, data);

	data = (param->RcHvsMaxDeltaQp << 6) | (param->HvsQpScaleDiv2 << 2) |
	       (param->EnHvsQp << 1) | param->EnRowLevelRc;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_PIC_RC, data);

	if (param->RcGopIQpOffsetEn != 0)
		gopIQpOffset = param->RcGopIQpOffset;

	if (param->GammaEn != 0)
		gamma = param->Gamma;

	data = (param->RcInitDelay << 16) | (param->RcGopIQpOffsetEn << 5) |
	       ((gopIQpOffset & 0xF) << 1) | (param->GammaEn);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_PIC_RC_2, data);

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARAM_PIC_GAMMA, gamma);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}
#endif

void EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr,
			PhysicalAddress paraAddr)
{
	BYTE tempBuf[8] = {
		0,
	}; // 64bit bus & endian
	Uint32 val;

	val = paraAddr;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}

RetCode VcodecTryLock(Uint32 core_idx)
{
	CVI_VC_LOCK("core_idx %d\n", core_idx);

	if (core_idx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("\n");
		return -1;
	}

	int ret = vdi_vcodec_trylock(core_idx);

	if (ret == 0) {
		//trylock success
		vdi_set_clock_gate(core_idx, CLK_ENABLE);
	} else {
		//trylock failure
		ret = RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	return ret;
}
#endif

RetCode VcodecTimeLock(Uint32 core_idx, int timeout_ms)
{
	int ret = 0;

	CVI_VC_LOCK("core_idx %d\n", core_idx);

	if (core_idx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("\n");
		return -1;
	}

	if (timeout_ms == 0) {
		ret = vdi_vcodec_trylock(core_idx);
		if (ret == 0) {
			//trylock success
			vdi_set_clock_gate(core_idx, CLK_ENABLE);
		} else {
			//trylock failure
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}
	} else {
		ret = vdi_vcodec_timelock(core_idx, timeout_ms);

		if (ret != 0) {
			CVI_VC_TRACE("TimeLock Timeout, ret %p\n",
				     __builtin_return_address(0));
			if (ret == ETIMEDOUT)
				return RETCODE_VPU_RESPONSE_TIMEOUT;

			CVI_VC_ERR("vdi_vcodec_timelock error[%d]\n", ret);
			return RETCODE_FAILURE;
		}
	}

	vdi_set_clock_gate(core_idx, CLK_ENABLE);

	CoreSleepWake(core_idx, 0);

	return RETCODE_SUCCESS;
}
/* EnterVcodecLock:
 *     Acquire mutex lock for 264/265 dec/enc thread in one process
 */
void EnterVcodecLock(Uint32 core_idx)
{
	CVI_VC_LOCK("core_idx %d\n", core_idx);

	if (core_idx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("core_idx %d\n", core_idx);
		return;
	}

	vdi_vcodec_lock(core_idx);
	vdi_set_clock_gate(core_idx, CLK_ENABLE);

	CoreSleepWake(core_idx, 0);
}

/* LeaveVcodecLock:
 *     Release mutex lock for 264/265 dec/enc thread in one process
 */
void LeaveVcodecLock(Uint32 core_idx)
{
	CVI_VC_LOCK("core_idx %d\n", core_idx);

	if (core_idx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("core_idx %d\n", core_idx);
		return;
	}

	CoreSleepWake(core_idx, 1);

	vdi_set_clock_gate(core_idx, CLK_DISABLE);
	vdi_vcodec_unlock(core_idx);
}

void CoreSleepWake(Uint32 coreIdx, int iSleepWake)
{
#ifdef ARCH_CV183X
	UNREFERENCED_PARAMETER(coreIdx);
	UNREFERENCED_PARAMETER(iSleepWake);
#else
	int coreStatus = CORE_STATUS_WAIT_INIT;

	if (coreIdx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("\n");
		return;
	}

	if (iSleepWake == 0) {
		VPU_GetCoreStatus(coreIdx, &coreStatus);

		if (coreStatus == CORE_STATUS_SLEEP) {
			VPU_SleepWake_EX(coreIdx, 0);
			VPU_SetCoreStatus(coreIdx, CORE_STATUS_WAKE);
		}
	} else if (iSleepWake == 1) {
		int TaskNum = VPU_GetCoreTaskNum(coreIdx);

		if (TaskNum == 0) {
			VPU_SetCoreStatus(coreIdx, CORE_STATUS_WAIT_INIT);
		} else {
			VPU_GetCoreStatus(coreIdx, &coreStatus);
			if (coreStatus != CORE_STATUS_SLEEP) {
				VPU_SleepWake_EX(coreIdx, 1);
				VPU_SetCoreStatus(coreIdx, CORE_STATUS_SLEEP);
			}
		}

		VPU_GetCoreStatus(coreIdx, &coreStatus);
	}
#endif
}

RetCode EnterDispFlagLock(Uint32 coreIdx)
{
	if (vdi_disp_lock(coreIdx) != 0)
		return RETCODE_FAILURE;
	return RETCODE_SUCCESS;
}

RetCode LeaveDispFlagLock(Uint32 coreIdx)
{
	vdi_disp_unlock(coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
	CodecInst *inst;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip) {
		VLOG(ERR, "SetClockGate: RETCODE_INSUFFICIENT_RESOURCE\n");
		return RETCODE_INSUFFICIENT_RESOURCE;
	}

	inst = (CodecInst *)vip->pendingInst;

	if (!on && (inst || !vdi_lock_check(coreIdx)))
		return RETCODE_SUCCESS;

	vdi_set_clock_gate(coreIdx, on);

	return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst, const char *caller_fn,
		    int caller_line)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	if (inst)
		CVI_VC_TRACE(
			"coreIdx %d, set pending inst = %p, inst->instIndex %d, s_n %s, s_line %d\n",
			coreIdx, inst, inst->instIndex, caller_fn, caller_line);
	else
		CVI_VC_TRACE(
			"coreIdx %d, clear pending inst, s_n %s, s_line %d\n",
			coreIdx, caller_fn, caller_line);

	vip->pendingInst = inst;
	if (inst)
		vip->pendingInstIdxPlus1 = (inst->instIndex + 1);
	else
		vip->pendingInstIdxPlus1 = 0;
}

void ClearPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	if (vip->pendingInst) {
		vip->pendingInst = 0;
		vip->pendingInstIdxPlus1 = 0;
	} else {
		CVI_VC_ERR("pendingInst is NULL\n");
	}
}

CodecInst *GetPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;
	int pendingInstIdx;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip) {
		CVI_VC_ERR("VIP null\n");
		return NULL;
	}

	if (!vip->pendingInst) {
		return NULL;
	}

	pendingInstIdx = vip->pendingInstIdxPlus1 - 1;
	if (pendingInstIdx < 0) {
		CVI_VC_ERR("pendingInstIdx < 0, vip->pendingInst %p,  lr %p\n",
			   vip->pendingInst, __builtin_return_address(0));
		CVI_VC_ERR("pendingInstIdx %d, vip->pendingInstIdxPlus1 %d\n",
			   pendingInstIdx, vip->pendingInstIdxPlus1);
		return NULL;
	}

	if (pendingInstIdx > MAX_NUM_INSTANCE) {
		CVI_VC_ERR("pendingInstIdx > MAX_NUM_INSTANCE\n");
		return NULL;
	}
	return (CodecInst *)vip->codecInstPool[pendingInstIdx];
}

#ifdef REDUNDENT_CODE
int GetPendingInstIdx(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return -1;

	return (vip->pendingInstIdxPlus1 - 1);
}
#endif

Int32 MaverickCache2Config(MaverickCacheConfig *pCache, BOOL decoder,
			   BOOL interleave, Uint32 bypass, Uint32 burst,
			   Uint32 merge, TiledMapType mapType, Uint32 wayshape)
{
	unsigned int cacheConfig = 0;

	if (decoder == TRUE) {
		if (mapType == 0) { // LINEAR_FRAME_MAP
			// VC1 opposite field padding is not allowable in UV
			// separated, burst 8 and linear map
			if (!interleave)
				burst = 0;

			wayshape = 15;

			if (merge == 1)
				merge = 3;

			// GDI constraint. Width should not be over 64
			if ((merge == 1) && (burst))
				burst = 0;
		} else {
			// horizontal merge constraint in tiled map
			if (merge == 1)
				merge = 3;
		}
	} else { // encoder
		if (mapType == LINEAR_FRAME_MAP) {
			wayshape = 15;
			// GDI constraint. Width should not be over 64
			if ((merge == 1) && (burst))
				burst = 0;
		} else {
			// horizontal merge constraint in tiled map
			if (merge == 1)
				merge = 3;
		}
	}

	cacheConfig = (merge & 0x3) << 9;
	cacheConfig = cacheConfig | ((wayshape & 0xf) << 5);
	cacheConfig = cacheConfig | ((burst & 0x1) << 3);
	cacheConfig = cacheConfig | (bypass & 0x3);

	if (mapType != 0) // LINEAR_FRAME_MAP
		cacheConfig = cacheConfig | 0x00000004;

	///{16'b0, 5'b0, merge[1:0], wayshape[3:0], 1'b0, burst[0], map[0],
	///bypass[1:0]};
	pCache->type2.CacheMode = cacheConfig;

	return 1;
}

#ifdef REDUNDENT_CODE
int GetLowDelayOutput(CodecInst *pCodecInst, DecOutputInfo *info)
{
	Uint32 val = 0;
	Uint32 val2 = 0;
	Int32 endIndex;
	VpuRect rectInfo;
	DecInfo *pDecInfo;

	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	info->indexFrameDisplay =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DISPLAY_IDX);
	info->indexFrameDecoded =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DECODED_IDX);

	val = VpuReadReg(pCodecInst->coreIdx,
			 RET_DEC_PIC_SIZE); // decoding picture size
	info->decPicWidth = (val >> 16) & 0xFFFF;
	info->decPicHeight = (val)&0xFFFF;

	if (info->indexFrameDecoded >= 0 &&
	    info->indexFrameDecoded < MAX_GDI_IDX) {
		// default value
		rectInfo.left = 0;
		rectInfo.right = info->decPicWidth;
		rectInfo.top = 0;
		rectInfo.bottom = info->decPicHeight;

		if (pCodecInst->codecMode == AVC_DEC ||
		    pCodecInst->codecMode == MP2_DEC ||
		    pCodecInst->codecMode == C7_AVC_DEC ||
		    pCodecInst->codecMode == C7_MP2_DEC) {
			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_CROP_LEFT_RIGHT); // frame
			// crop
			// information(left,
			// right)
			val2 = VpuReadReg(
				pCodecInst->coreIdx,
				RET_DEC_PIC_CROP_TOP_BOTTOM); // frame crop
			// information(top,
			// bottom)

			if (val == (Uint32)-1 || val == 0) {
				rectInfo.left = 0;
				rectInfo.right = info->decPicWidth;
			} else {
				rectInfo.left = ((val >> 16) & 0xFFFF);
				rectInfo.right =
					info->decPicWidth - (val & 0xFFFF);
			}
			if (val2 == (Uint32)-1 || val2 == 0) {
				rectInfo.top = 0;
				rectInfo.bottom = info->decPicHeight;
			} else {
				rectInfo.top = ((val2 >> 16) & 0xFFFF);
				rectInfo.bottom =
					info->decPicHeight - (val2 & 0xFFFF);
			}
		}

		info->rcDecoded.left =
			pDecInfo->decOutInfo[info->indexFrameDecoded]
				.rcDecoded.left = rectInfo.left;
		info->rcDecoded.right =
			pDecInfo->decOutInfo[info->indexFrameDecoded]
				.rcDecoded.right = rectInfo.right;
		info->rcDecoded.top =
			pDecInfo->decOutInfo[info->indexFrameDecoded]
				.rcDecoded.top = rectInfo.top;
		info->rcDecoded.bottom =
			pDecInfo->decOutInfo[info->indexFrameDecoded]
				.rcDecoded.bottom = rectInfo.bottom;
	} else {
		info->rcDecoded.left = 0;
		info->rcDecoded.right = info->decPicWidth;
		info->rcDecoded.top = 0;
		info->rcDecoded.bottom = info->decPicHeight;
	}

	if (info->indexFrameDisplay >= 0 &&
	    info->indexFrameDisplay < MAX_GDI_IDX) {
		if (pCodecInst->codecMode == VC1_DEC ||
		    pCodecInst->codecMode == C7_VC1_DEC) // vc1 rotates decoded
		// frame buffer region.
		// the other std
		// rotated whole frame
		// buffer region.
		{
			if (pDecInfo->rotationEnable &&
			    (pDecInfo->rotationAngle == 90 ||
			     pDecInfo->rotationAngle == 270)) {
				info->rcDisplay.left =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.top;
				info->rcDisplay.right =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.bottom;
				info->rcDisplay.top =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.left;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.right;
			} else {
				info->rcDisplay.left =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.left;
				info->rcDisplay.right =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.right;
				info->rcDisplay.top =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.top;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.bottom;
			}
		} else {
			if (pDecInfo->rotationEnable) {
				switch (pDecInfo->rotationAngle) {
				case 90:
				case 270:
					info->rcDisplay.left =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.top;
					info->rcDisplay.right =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded
								.bottom;
					info->rcDisplay.top =
						pDecInfo->rotatorOutput.height -
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.right;
					info->rcDisplay.bottom =
						pDecInfo->rotatorOutput.height -
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.left;
					break;
				default:
					info->rcDisplay.left =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.left;
					info->rcDisplay.right =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.right;
					info->rcDisplay.top =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded.top;
					info->rcDisplay.bottom =
						pDecInfo->decOutInfo
							[info->indexFrameDisplay]
								.rcDecoded
								.bottom;
					break;
				}

				if (pDecInfo->mirrorEnable) {
					Uint32 temp;
					if (pDecInfo->mirrorDirection &
					    MIRDIR_VER) {
						temp = info->rcDisplay.top;
						info->rcDisplay.top =
							info->decPicHeight -
							info->rcDisplay.bottom;
						info->rcDisplay.bottom =
							info->decPicHeight -
							temp;
					}
					if (pDecInfo->mirrorDirection &
					    MIRDIR_HOR) {
						temp = info->rcDisplay.left;
						info->rcDisplay.left =
							info->decPicWidth -
							info->rcDisplay.right;
						info->rcDisplay.right =
							info->decPicWidth -
							temp;
					}
				}
			} else {
				info->rcDisplay.left =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.left;
				info->rcDisplay.right =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.right;
				info->rcDisplay.top =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.top;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.rcDecoded.bottom;
			}
		}

		if (info->indexFrameDisplay == info->indexFrameDecoded) {
			info->dispPicWidth = info->decPicWidth;
			info->dispPicHeight = info->decPicHeight;
		} else {
			info->dispPicWidth =
				pDecInfo->decOutInfo[info->indexFrameDisplay]
					.decPicWidth;
			info->dispPicHeight =
				pDecInfo->decOutInfo[info->indexFrameDisplay]
					.decPicHeight;
		}
	} else {
		info->rcDisplay.left = 0;
		info->rcDisplay.right = 0;
		info->rcDisplay.top = 0;
		info->rcDisplay.bottom = 0;

		info->dispPicWidth = 0;
		info->dispPicHeight = 0;
	}

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_TYPE);
	info->interlacedFrame = (val >> 18) & 0x1;
	info->topFieldFirst = (val >> 21) & 0x0001; // TopFieldFirst[21]
	if (info->interlacedFrame) {
		info->picTypeFirst = (val & 0x38) >> 3; // pic_type of 1st field
		info->picType = val & 7; // pic_type of 2nd field

	} else {
		info->picTypeFirst = PIC_TYPE_MAX; // no meaning
		info->picType = val & 7;
	}

	info->pictureStructure = (val >> 19) & 0x0003; // MbAffFlag[17],
		// FieldPicFlag[16]
	info->repeatFirstField = (val >> 22) & 0x0001;
	info->progressiveFrame = (val >> 23) & 0x0003;

	if (pCodecInst->codecMode == AVC_DEC ||
	    pCodecInst->codecMode == C7_AVC_DEC) {
		info->nalRefIdc = (val >> 7) & 0x03;
		info->decFrameInfo = (val >> 15) & 0x0001;
		info->picStrPresent = (val >> 27) & 0x0001;
		info->picTimingStruct = (val >> 28) & 0x000f;
		// update picture type when IDR frame
		if (val & 0x40) { // 6th bit
			if (info->interlacedFrame)
				info->picTypeFirst = PIC_TYPE_IDR;
			else
				info->picType = PIC_TYPE_IDR;
		}
		info->decFrameInfo = (val >> 16) & 0x0003;
		if (info->indexFrameDisplay >= 0) {
			if (info->indexFrameDisplay == info->indexFrameDecoded)
				info->avcNpfFieldInfo = info->decFrameInfo;
			else
				info->avcNpfFieldInfo =
					pDecInfo->decOutInfo
						[info->indexFrameDisplay]
							.decFrameInfo;
		}
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_HRD_INFO);
		info->avcHrdInfo.cpbMinus1 = val >> 2;
		info->avcHrdInfo.vclHrdParamFlag = (val >> 1) & 1;
		info->avcHrdInfo.nalHrdParamFlag = val & 1;

		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_INFO);
		info->avcVuiInfo.fixedFrameRateFlag = val & 1;
		info->avcVuiInfo.timingInfoPresent = (val >> 1) & 0x01;
		info->avcVuiInfo.chromaLocBotField = (val >> 2) & 0x07;
		info->avcVuiInfo.chromaLocTopField = (val >> 5) & 0x07;
		info->avcVuiInfo.chromaLocInfoPresent = (val >> 8) & 0x01;
		info->avcVuiInfo.colorPrimaries = (val >> 16) & 0xff;
		info->avcVuiInfo.colorDescPresent = (val >> 24) & 0x01;
		info->avcVuiInfo.isExtSAR = (val >> 25) & 0x01;
		info->avcVuiInfo.vidFullRange = (val >> 26) & 0x01;
		info->avcVuiInfo.vidFormat = (val >> 27) & 0x07;
		info->avcVuiInfo.vidSigTypePresent = (val >> 30) & 0x01;
		info->avcVuiInfo.vuiParamPresent = (val >> 31) & 0x01;
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_PIC_VUI_PIC_STRUCT);
		info->avcVuiInfo.vuiPicStructPresent = (val & 0x1);
		info->avcVuiInfo.vuiPicStruct = (val >> 1);
	}

	info->fRateNumerator = VpuReadReg(pCodecInst->coreIdx,
					  RET_DEC_PIC_FRATE_NR); // Frame rate,
	// Aspect ratio
	// can be
	// changed frame
	// by frame.
	info->fRateDenominator =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_DR);
	if ((pCodecInst->codecMode == AVC_DEC ||
	     pCodecInst->codecMode == C7_AVC_DEC) &&
	    info->fRateDenominator > 0)
		info->fRateDenominator *= 2;

	info->aspectRateInfo =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ASPECT);

	// User Data
	if (pDecInfo->userDataEnable) {
		int userDataNum;
		int userDataSize;
		BYTE tempBuf[8] = {
			0,
		};

		VpuReadMem(pCodecInst->coreIdx, pDecInfo->userDataBufAddr + 0,
			   tempBuf, 8, VPU_USER_DATA_ENDIAN);

		val = ((tempBuf[0] << 24) & 0xFF000000) |
		      ((tempBuf[1] << 16) & 0x00FF0000) |
		      ((tempBuf[2] << 8) & 0x0000FF00) |
		      ((tempBuf[3] << 0) & 0x000000FF);

		userDataNum = (val >> 16) & 0xFFFF;
		userDataSize = (val >> 0) & 0xFFFF;
		if (userDataNum == 0)
			userDataSize = 0;

		info->decOutputExtData.userDataNum = userDataNum;
		info->decOutputExtData.userDataSize = userDataSize;

		val = ((tempBuf[4] << 24) & 0xFF000000) |
		      ((tempBuf[5] << 16) & 0x00FF0000) |
		      ((tempBuf[6] << 8) & 0x0000FF00) |
		      ((tempBuf[7] << 0) & 0x000000FF);

		if (userDataNum == 0)
			info->decOutputExtData.userDataBufFull = 0;
		else
			info->decOutputExtData.userDataBufFull =
				(val >> 16) & 0xFFFF;

		info->decOutputExtData.activeFormat =
			VpuReadReg(pCodecInst->coreIdx,
				   RET_DEC_PIC_ATSC_USER_DATA_INFO) &
			0xf;
	}

	info->numOfErrMBs = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ERR_MB);

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SUCCESS);
	info->decodingSuccess = val;
	info->sequenceChanged = ((val >> 20) & 0x1);
	info->streamEndFlag = ((pDecInfo->streamEndflag >> 2) & 0x01);

	endIndex = (pDecInfo->openParam.wtlEnable == TRUE) ?
				 pDecInfo->numFbsForWTL :
				 pDecInfo->numFbsForDecoding;
	if (0 <= info->indexFrameDisplay &&
	    info->indexFrameDisplay < endIndex) {
		info->dispFrame =
			pDecInfo->frameBufPool[info->indexFrameDisplay];
		if (pDecInfo->openParam.wtlEnable) { // coda980 only
			info->dispFrame = pDecInfo->frameBufPool
						  [info->indexFrameDisplay +
						   pDecInfo->numFbsForDecoding];
		}
	}

	if (pDecInfo->deringEnable || pDecInfo->mirrorEnable ||
	    pDecInfo->rotationEnable || pDecInfo->tiled2LinearEnable) {
		info->dispFrame = pDecInfo->rotatorOutput;
	}

	if ((pCodecInst->codecMode == AVC_DEC ||
	     pCodecInst->codecMode == C7_AVC_DEC) &&
	    pCodecInst->codecModeAux == AVC_AUX_MVC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_MVC_REPORT);
		info->mvcPicInfo.viewIdxDisplay = (val >> 0) & 1;
		info->mvcPicInfo.viewIdxDecoded = (val >> 1) & 1;
	}

	if (pCodecInst->codecMode == AVC_DEC ||
	    pCodecInst->codecMode == C7_AVC_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI0);

		if (val == (Uint32)-1) {
			info->avcFpaSei.exist = 0;
		} else {
			info->avcFpaSei.exist = 1;
			info->avcFpaSei.framePackingArrangementId = val;

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_AVC_FPA_SEI1);
			info->avcFpaSei.contentInterpretationType =
				val & 0x3F; // [5:0]
			info->avcFpaSei.framePackingArrangementType =
				(val >> 6) & 0x7F; // [12:6]
			info->avcFpaSei.framePackingArrangementExtensionFlag =
				(val >> 13) & 0x01; // [13]
			info->avcFpaSei.frame1SelfContainedFlag =
				(val >> 14) & 0x01; // [14]
			info->avcFpaSei.frame0SelfContainedFlag =
				(val >> 15) & 0x01; // [15]
			info->avcFpaSei.currentFrameIsFrame0Flag =
				(val >> 16) & 0x01; // [16]
			info->avcFpaSei.fieldViewsFlag =
				(val >> 17) & 0x01; // [17]
			info->avcFpaSei.frame0FlippedFlag =
				(val >> 18) & 0x01; // [18]
			info->avcFpaSei.spatialFlippingFlag =
				(val >> 19) & 0x01; // [19]
			info->avcFpaSei.quincunxSamplingFlag =
				(val >> 20) & 0x01; // [20]
			info->avcFpaSei.framePackingArrangementCancelFlag =
				(val >> 21) & 0x01; // [21]

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_AVC_FPA_SEI2);
			info->avcFpaSei.framePackingArrangementRepetitionPeriod =
				val & 0x7FFF; // [14:0]
			info->avcFpaSei.frame1GridPositionY =
				(val >> 16) & 0x0F; // [19:16]
			info->avcFpaSei.frame1GridPositionX =
				(val >> 20) & 0x0F; // [23:20]
			info->avcFpaSei.frame0GridPositionY =
				(val >> 24) & 0x0F; // [27:24]
			info->avcFpaSei.frame0GridPositionX =
				(val >> 28) & 0x0F; // [31:28]
		}

		info->avcPocTop =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_TOP);
		info->avcPocBot =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_BOT);

		if (info->interlacedFrame) {
			if (info->avcPocTop > info->avcPocBot) {
				info->avcPocPic = info->avcPocBot;
			} else {
				info->avcPocPic = info->avcPocTop;
			}
		} else
			info->avcPocPic = VpuReadReg(pCodecInst->coreIdx,
						     RET_DEC_PIC_POC);
	}

	// pDecInfo->streamRdPtr //NA
	pDecInfo->frameDisplayFlag = VpuReadReg(
		pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
	// info->consumedByte //NA
	// info->notSufficientSliceBuffer; // NA
	// info->notSufficientPsBuffer;  // NA
	// info->bytePosFrameStart //NA
	// info->bytePosFrameEnd   //NA
	// info->rdPtr //NA
	// info->wrPtr //NA

	// info->frameCycle  //NA
	// Vp8ScaleInfo vp8ScaleInfo; //NA
	// Vp8PicInfo vp8PicInfo; //NA
	// MvcPicInfo mvcPicInfo; ////NA
	// info->wprotErrReason; avcVuiInfo
	// PhysicalAddress wprotErrAddress; avcVuiInfo

	// Report Information
	// info->frameDct; //NA
	// info->progressiveSequence; //NA
	// info->mp4TimeIncrement; //NA
	// info->mp4ModuloTimeBase; //NA

	return 1;
}
#endif

RetCode AllocateLinearFrameBuffer(TiledMapType mapType, FrameBuffer *fbArr,
				  Uint32 numOfFrameBuffers, Uint32 sizeLuma,
				  Uint32 sizeChroma)
{
	Uint32 i;
	BOOL yuv422Interleave = FALSE;
	BOOL fieldFrame = (BOOL)(mapType == LINEAR_FIELD_MAP);
	BOOL cbcrInterleave = (BOOL)(mapType == COMPRESSED_FRAME_MAP ||
				     fbArr[0].cbcrInterleave == TRUE);
	BOOL reuseFb = FALSE;

	if (mapType != COMPRESSED_FRAME_MAP) {
		switch (fbArr[0].format) {
		case FORMAT_YUYV:
		case FORMAT_YUYV_P10_16BIT_MSB:
		case FORMAT_YUYV_P10_16BIT_LSB:
		case FORMAT_YUYV_P10_32BIT_MSB:
		case FORMAT_YUYV_P10_32BIT_LSB:
		case FORMAT_YVYU:
		case FORMAT_YVYU_P10_16BIT_MSB:
		case FORMAT_YVYU_P10_16BIT_LSB:
		case FORMAT_YVYU_P10_32BIT_MSB:
		case FORMAT_YVYU_P10_32BIT_LSB:
		case FORMAT_UYVY:
		case FORMAT_UYVY_P10_16BIT_MSB:
		case FORMAT_UYVY_P10_16BIT_LSB:
		case FORMAT_UYVY_P10_32BIT_MSB:
		case FORMAT_UYVY_P10_32BIT_LSB:
		case FORMAT_VYUY:
		case FORMAT_VYUY_P10_16BIT_MSB:
		case FORMAT_VYUY_P10_16BIT_LSB:
		case FORMAT_VYUY_P10_32BIT_MSB:
		case FORMAT_VYUY_P10_32BIT_LSB:
			yuv422Interleave = TRUE;
			break;
		default:
			yuv422Interleave = FALSE;
			break;
		}
	}

	for (i = 0; i < numOfFrameBuffers; i++) {
		reuseFb = (fbArr[i].bufY != (PhysicalAddress)-1 &&
			   fbArr[i].bufCb != (PhysicalAddress)-1 &&
			   fbArr[i].bufCr != (PhysicalAddress)-1);
		if (reuseFb == FALSE) {
			if (yuv422Interleave == TRUE) {
				fbArr[i].bufCb = (PhysicalAddress)-1;
				fbArr[i].bufCr = (PhysicalAddress)-1;
			} else {
				if (fbArr[i].bufCb == (PhysicalAddress)-1) {
					fbArr[i].bufCb =
						fbArr[i].bufY +
						(sizeLuma >> fieldFrame);
					CVI_VC_TRACE(
						"bufY = 0x%llX, bufCb = 0x%llX\n",
						fbArr[i].bufY, fbArr[i].bufCb);
				}
				if (fbArr[i].bufCr == (PhysicalAddress)-1) {
					if (cbcrInterleave == TRUE) {
						fbArr[i].bufCr =
							(PhysicalAddress)-1;
					} else {
						fbArr[i].bufCr =
							fbArr[i].bufCb +
							(sizeChroma >>
							 fieldFrame);
					}
				}
			}
		}
	}

	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
/* \brief   Allocate tiled framebuffer on GDI version 1.0 H/W.
 */
RetCode AllocateTiledFrameBufferGdiV1(TiledMapType mapType,
				      PhysicalAddress tiledBaseAddr,
				      FrameBuffer *fbArr,
				      Uint32 numOfFrameBuffers, Uint32 sizeLuma,
				      Uint32 sizeChroma, DRAMConfig *pDramCfg)
{
	Uint32 rasLowBitsForHor;
	Uint32 i;
	Uint32 cas, ras, bank, bus;
	Uint32 lumRasTop, lumRasBot, chrRasTop, chrRasBot;
	Uint32 lumFrameRasSize, lumFieldRasSize, chrFieldRasSize;
	PhysicalAddress addrY, addrYRas;

	if (mapType == TILED_FRAME_MB_RASTER_MAP ||
	    mapType == TILED_FIELD_MB_RASTER_MAP) {
		for (i = 0; i < numOfFrameBuffers; i++) {
			int lum_top_base;
			int lum_bot_base;
			int chr_top_base;
			int chr_bot_base;

			addrY = ((fbArr[i].bufY + (16384 - 1)) & ~(16384 - 1));

			lum_top_base = addrY;
			lum_bot_base = addrY + (sizeLuma >> 1);
			chr_top_base = addrY + sizeLuma;
			chr_bot_base = addrY + sizeLuma + sizeChroma; // cbcr is
				// interleaved

			lum_top_base = (lum_top_base >> 12) & 0xfffff;
			lum_bot_base = (lum_bot_base >> 12) & 0xfffff;
			chr_top_base = (chr_top_base >> 12) & 0xfffff;
			chr_bot_base = (chr_bot_base >> 12) & 0xfffff;

			fbArr[i].bufY =
				(lum_top_base << 12) | (chr_top_base >> 8);
			fbArr[i].bufCb = ((chr_top_base & 0xff) << 24) |
					 (lum_bot_base << 4) |
					 (chr_bot_base >> 16);
			fbArr[i].bufCr = ((chr_bot_base & 0xffff) << 16);
			fbArr[i].bufYBot = (PhysicalAddress)-1;
			fbArr[i].bufCbBot = (PhysicalAddress)-1;
			fbArr[i].bufCrBot = (PhysicalAddress)-1;
		}
	} else {
		cas = pDramCfg->casBit;
		ras = pDramCfg->rasBit;
		bank = pDramCfg->bankBit;
		bus = pDramCfg->busBit;
		if (cas == 9 && bank == 2 && ras == 13) {
			rasLowBitsForHor = 3;
		} else if (cas == 10 && bank == 3 && ras == 13) {
			rasLowBitsForHor = 2;
		} else {
			return RETCODE_INVALID_PARAM;
		}

		for (i = 0; i < numOfFrameBuffers; i++) {
			addrY = fbArr[i].bufY - tiledBaseAddr;
			// align base_addr to RAS boundary
			addrYRas = (addrY + ((1 << (bank + cas + bus)) - 1)) >>
				   (bank + cas + bus);
			// round up RAS lower 3(or 4) bits
			addrYRas =
				((addrYRas + ((1 << (rasLowBitsForHor)) - 1)) >>
				 rasLowBitsForHor)
				<< rasLowBitsForHor;

			chrFieldRasSize = sizeChroma >>
					  (pDramCfg->bankBit +
					   pDramCfg->casBit + pDramCfg->busBit);
			lumFieldRasSize = (sizeLuma >> 1) >>
					  (pDramCfg->bankBit +
					   pDramCfg->casBit + pDramCfg->busBit);
			lumFrameRasSize = lumFieldRasSize * 2;
			lumRasTop = addrYRas;
			lumRasBot = lumRasTop + lumFieldRasSize;
			chrRasTop = lumRasTop + lumFrameRasSize;
			chrRasBot = chrRasTop + chrFieldRasSize;

			fbArr[i].bufY = (lumRasBot << 16) + lumRasTop;
			fbArr[i].bufCb = (chrRasBot << 16) + chrRasTop;
			if (rasLowBitsForHor == 4) {
				fbArr[i].bufCr =
					((((chrRasBot >> 4) << 4) + 8) << 16) +
					(((chrRasTop >> 4) << 4) + 8);
			} else if (rasLowBitsForHor == 3) {
				fbArr[i].bufCr =
					((((chrRasBot >> 3) << 3) + 4) << 16) +
					(((chrRasTop >> 3) << 3) + 4);
			} else if (rasLowBitsForHor == 2) {
				fbArr[i].bufCr =
					((((chrRasBot >> 2) << 2) + 2) << 16) +
					(((chrRasTop >> 2) << 2) + 2);
			} else if (rasLowBitsForHor == 1) {
				fbArr[i].bufCr =
					((((chrRasBot >> 1) << 1) + 1) << 16) +
					(((chrRasTop >> 1) << 1) + 1);
			} else {
				return RETCODE_INSUFFICIENT_RESOURCE; // Invalid
					// RasLowBit
					// value
			}
		}
	}

	return RETCODE_SUCCESS;
}
#endif

/* \brief   Allocate tiled framebuffer on GDI version 2.0 H/W
 */
RetCode AllocateTiledFrameBufferGdiV2(TiledMapType mapType, FrameBuffer *fbArr,
				      Uint32 numOfFrameBuffers, Uint32 sizeLuma,
				      Uint32 sizeChroma)
{
	Uint32 i;
	Uint32 fieldFrame;
	Uint32 sizeFb;
	BOOL cbcrInterleave;

	sizeFb = sizeLuma + sizeChroma * 2;
	fieldFrame = (mapType == TILED_FIELD_V_MAP ||
		      mapType == TILED_FIELD_NO_BANK_MAP ||
		      mapType == LINEAR_FIELD_MAP);

	for (i = 0; i < numOfFrameBuffers; i++) {
		cbcrInterleave = fbArr[0].cbcrInterleave;

		if (fbArr[i].bufCb == (PhysicalAddress)-1)
			fbArr[i].bufCb =
				fbArr[i].bufY + (sizeLuma >> fieldFrame);

		if (fbArr[i].bufCr == (PhysicalAddress)-1)
			fbArr[i].bufCr =
				fbArr[i].bufCb + (sizeChroma >> fieldFrame);

		switch (mapType) {
		case TILED_FIELD_V_MAP:
		case TILED_FIELD_NO_BANK_MAP:
			fbArr[i].bufYBot =
				fbArr[i].bufY + (sizeFb >> fieldFrame);
			fbArr[i].bufCbBot =
				fbArr[i].bufYBot + (sizeLuma >> fieldFrame);
			if (cbcrInterleave == FALSE) {
				fbArr[i].bufCrBot = fbArr[i].bufCbBot +
						    (sizeChroma >> fieldFrame);
			}
			break;
		case TILED_FRAME_V_MAP:
		case TILED_FRAME_H_MAP:
		case TILED_MIXED_V_MAP:
		case TILED_FRAME_NO_BANK_MAP:
			fbArr[i].bufYBot = fbArr[i].bufY;
			fbArr[i].bufCbBot = fbArr[i].bufCb;
			if (cbcrInterleave == FALSE) {
				fbArr[i].bufCrBot = fbArr[i].bufCr;
			}
			break;
		case TILED_FIELD_MB_RASTER_MAP:
			fbArr[i].bufYBot = fbArr[i].bufY + (sizeLuma >> 1);
			fbArr[i].bufCbBot = fbArr[i].bufCb + sizeChroma;
			break;
		default:
			fbArr[i].bufYBot = 0;
			fbArr[i].bufCbBot = 0;
			fbArr[i].bufCrBot = 0;
			break;
		}
	}

	return RETCODE_SUCCESS;
}

Int32 _ConfigSecAXICoda9_182x(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa,
			      Uint32 width)
{
	vpu_buffer_t vb;
	int offset;
	Uint32 MbNumX = ((width & 0xFFFF) + 15) / 16;
	int free_size;

	if (vdi_get_sram_memory(coreIdx, &vb) < 0) {
		return 0;
	}

	if (!vb.size) {
		sa->bufSize = 0;
		sa->u.coda9.useBitEnable = 0;
		sa->u.coda9.useIpEnable = 0;
		sa->u.coda9.useDbkYEnable = 0;
		sa->u.coda9.useDbkCEnable = 0;
		sa->u.coda9.useOvlEnable = 0;
		sa->u.coda9.useBtpEnable = 0;
		return 0;
	}

	CVI_VC_CFG("Get sram: addr = 0x%llx, size = 0x%x\n",
		   (Uint64)vb.phys_addr, vb.size);

	vb.phys_addr = (PhysicalAddress)vb.size;
	free_size = vb.size;

	offset = 0;
	// BIT
	if (sa->u.coda9.useBitEnable) {
		sa->u.coda9.useBitEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 144;
			break; // AVC
		case AVC_ENC: {
			offset = offset + MbNumX * 16;
		} break;
		default:
			offset = offset + MbNumX * 16;
			break; // MPEG-4, Divx3
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufBitUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufBitUse = 0x%llx, offset= 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufBitUse, offset, free_size);
	}

	// Intra Prediction, ACDC
	if (sa->u.coda9.useIpEnable) {
		//sa->u.coda9.bufIpAcDcUse = vb.phys_addr + offset;
		sa->u.coda9.useIpEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 32;
			break; // AVC
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 128;
			break; // MPEG-4, Divx3
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufIpAcDcUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufIpAcDcUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufIpAcDcUse, offset, free_size);
	}

	// Deblock Chroma
	if (sa->u.coda9.useDbkCEnable) {
		sa->u.coda9.useDbkCEnable = 1;
		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 64;
			break; // AVC
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 64;
			break;
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufDbkCUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufDbkCUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufDbkCUse, offset, free_size);
	}

	// Deblock Luma
	if (sa->u.coda9.useDbkYEnable) {
		sa->u.coda9.useDbkYEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 64;
			break; // AVC
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 128;
			break;
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufDbkYUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufDbkYUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufDbkYUse, offset, free_size);
	}

	// check the buffer address which is 256 byte is available.
	if (((offset + 255) & (~255)) > (int)vb.size) {
		VLOG(ERR, "%s:%d NOT ENOUGH SRAM: required(%d), sram(%d)\n",
		     __func__, __LINE__, offset, vb.size);
		sa->bufSize = 0;
		CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
			   free_size);
		return 0;
	}

	// VC1 Bit-plane
	if (sa->u.coda9.useBtpEnable) {
		if (codecMode != VC1_DEC) {
			sa->u.coda9.useBtpEnable = 0;
		}
	}

	// VC1 Overlap
	if (sa->u.coda9.useOvlEnable) {
		if (codecMode != VC1_DEC) {
			sa->u.coda9.useOvlEnable = 0;
		}
	}

	sa->bufSize = offset;
	sa->bufBase = vb.phys_addr - offset;
	CVI_VC_CFG("Calc sram used size=0x%x, sa->bufBase = 0x%llx\n", offset,
		   sa->bufBase);

	return 1;
}

Int32 ConfigSecAXICoda9(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa,
			Uint32 width, Uint32 profile, Uint32 interlace)
{
	vpu_buffer_t vb;
	int offset;
	Uint32 MbNumX = ((width & 0xFFFF) + 15) >> 4;
#ifdef REDUNDENT_CODE
	Uint32 productId;
#endif
	int free_size;

	if ((codecMode == AVC_DEC) &&
	    (vdi_get_chip_version() == CHIP_ID_1821A ||
	     vdi_get_chip_version() == CHIP_ID_1822) &&
	    ((profile == 66) || (interlace == 0))) {
		return _ConfigSecAXICoda9_182x(coreIdx, codecMode, sa, width);
	}

	if (vdi_get_sram_memory(coreIdx, &vb) < 0) {
		return 0;
	}

#ifdef REDUNDENT_CODE
	productId = ProductVpuGetId(coreIdx);
#endif

	if (!vb.size) {
		sa->bufSize = 0;
		sa->u.coda9.useBitEnable = 0;
		sa->u.coda9.useIpEnable = 0;
		sa->u.coda9.useDbkYEnable = 0;
		sa->u.coda9.useDbkCEnable = 0;
		sa->u.coda9.useOvlEnable = 0;
		sa->u.coda9.useBtpEnable = 0;
		return 0;
	}

	CVI_VC_CFG("Get sram: addr = 0x%llx, size = 0x%x\n",
		   (Uint64)vb.phys_addr, vb.size);

	vb.phys_addr = (PhysicalAddress)vb.size;
	free_size = vb.size;

	offset = 0;
	// BIT
	if (sa->u.coda9.useBitEnable) {
		sa->u.coda9.useBitEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 144;
			break; // AVC
#ifdef REDUNDENT_CODE
		case RV_DEC:
			offset = offset + MbNumX * 128;
			break;
		case VC1_DEC:
			offset = offset + MbNumX * 64;
			break;
		case AVS_DEC:
			offset = offset + ((MbNumX + 3) & ~3) * 32;
			break;
		case MP2_DEC:
			offset = offset + MbNumX * 0;
			break;
		case VPX_DEC:
			offset = offset + MbNumX * 0;
			break;
#endif
		case AVC_ENC: {
#ifdef REDUNDENT_CODE
			if (productId == PRODUCT_ID_960)
				offset = offset + MbNumX * 128;
			else
#endif
				offset = offset + MbNumX * 16;
		} break;
#ifdef REDUNDENT_CODE
		case MP4_ENC:
			offset = offset + MbNumX * 16;
			break;
#endif
		default:
			offset = offset + MbNumX * 16;
			break; // MPEG-4, Divx3
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufBitUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufBitUse = 0x%llx, offset= 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufBitUse, offset, free_size);
	}

	// Intra Prediction, ACDC
	if (sa->u.coda9.useIpEnable) {
		//sa->u.coda9.bufIpAcDcUse = vb.phys_addr + offset;
		sa->u.coda9.useIpEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = offset + MbNumX * 64;
			break; // AVC
		case RV_DEC:
			offset = offset + MbNumX * 64;
			break;
		case VC1_DEC:
			offset = offset + MbNumX * 128;
			break;
		case AVS_DEC:
			offset = offset + MbNumX * 64;
			break;
		case MP2_DEC:
			offset = offset + MbNumX * 0;
			break;
		case VPX_DEC:
			offset = offset + MbNumX * 64;
			break;
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		case MP4_ENC:
			offset = offset + MbNumX * 128;
			break;
		default:
			offset = offset + MbNumX * 128;
			break; // MPEG-4, Divx3
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufIpAcDcUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufIpAcDcUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufIpAcDcUse, offset, free_size);
	}

	// Deblock Chroma
	if (sa->u.coda9.useDbkCEnable) {
		sa->u.coda9.useDbkCEnable = 1;
		switch (codecMode) {
		case AVC_DEC:
			offset = (profile == 66 /*AVC BP decoder*/) ?
					       offset + (MbNumX * 64) :
					       offset + (MbNumX * 128);
			break; // AVC
		case RV_DEC:
			offset = offset + MbNumX * 128;
			break;
		case VC1_DEC:
			offset = profile == 2 ? offset + MbNumX * 256 :
						      offset + MbNumX * 128;
			break;
		case AVS_DEC:
			offset = offset + MbNumX * 64;
			break;
		case MP2_DEC:
			offset = offset + MbNumX * 64;
			break;
		case VPX_DEC:
			offset = offset + MbNumX * 128;
			break;
		case MP4_DEC:
			offset = offset + MbNumX * 64;
			break;
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		case MP4_ENC:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 64;
			break;
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufDbkCUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufDbkCUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufDbkCUse, offset, free_size);
	}

	// Deblock Luma
	if (sa->u.coda9.useDbkYEnable) {
		sa->u.coda9.useDbkYEnable = 1;

		switch (codecMode) {
		case AVC_DEC:
			offset = (profile == 66 /*AVC BP decoder*/) ?
					       offset + (MbNumX * 64) :
					       offset + (MbNumX * 128);
			break; // AVC
		case RV_DEC:
			offset = offset + MbNumX * 128;
			break;
		case VC1_DEC:
			offset = profile == 2 ? offset + MbNumX * 256 :
						      offset + MbNumX * 128;
			break;
		case AVS_DEC:
			offset = offset + MbNumX * 64;
			break;
		case MP2_DEC:
			offset = offset + MbNumX * 128;
			break;
		case VPX_DEC:
			offset = offset + MbNumX * 128;
			break;
		case MP4_DEC:
			offset = offset + MbNumX * 64;
			break;
		case AVC_ENC:
			offset = offset + MbNumX * 64;
			break;
		case MP4_ENC:
			offset = offset + MbNumX * 64;
			break;
		default:
			offset = offset + MbNumX * 128;
			break;
		}

		if (offset > free_size) {
			sa->bufSize = 0;
			CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
				   free_size);
			return 0;
		}

		sa->u.coda9.bufDbkYUse = vb.phys_addr - offset;
		CVI_VC_CFG(
			"bufDbkYUse = 0x%llx, offset = 0x%X, free_size = 0x%X\n",
			(Uint64)sa->u.coda9.bufDbkYUse, offset, free_size);
	}

	// check the buffer address which is 256 byte is available.
	if (((offset + 255) & (~255)) > (int)vb.size) {
		VLOG(ERR, "%s:%d NOT ENOUGH SRAM: required(%d), sram(%d)\n",
		     __func__, __LINE__, offset, vb.size);
		sa->bufSize = 0;
		CVI_VC_ERR("offset (0x%X) > free_size (0x%X)\n", offset,
			   free_size);
		return 0;
	}

	// VC1 Bit-plane
	if (sa->u.coda9.useBtpEnable) {
		if (codecMode != VC1_DEC) {
			sa->u.coda9.useBtpEnable = 0;
		} /* else {
			int oneBTP;

			offset = ((offset + 255) & ~255);
			sa->u.coda9.bufBtpUse = vb.phys_addr + offset;
			sa->u.coda9.useBtpEnable = 1;

			oneBTP = (((MbNumX + 15) / 16) * MbNumY + 1) * 2;
			oneBTP = (oneBTP % 256) ? ((oneBTP / 256) + 1) * 256 :
						  oneBTP;

			offset = offset + oneBTP * 3;

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}*/
	}

	// VC1 Overlap
	if (sa->u.coda9.useOvlEnable) {
		if (codecMode != VC1_DEC) {
			sa->u.coda9.useOvlEnable = 0;
		} /* else {
			sa->u.coda9.bufOvlUse = vb.phys_addr + offset;
			sa->u.coda9.useOvlEnable = 1;

			offset = offset + MbNumX * 80;

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}*/
	}

	sa->bufSize = offset;
	sa->bufBase = vb.phys_addr - offset;
	CVI_VC_CFG("Calc sram used size=0x%x, sa->bufBase = 0x%llx\n", offset,
		   sa->bufBase);

	return 1;
}

Int32 ConfigSecAXIWave(Uint32 coreIdx, Int32 codecMode, SecAxiInfo *sa,
		       Uint32 width, Uint32 height, Uint32 profile,
		       Uint32 levelIdc)
{
	vpu_buffer_t vb;
	int offset;
	Uint32 size;
	Uint32 lumaSize;
	Uint32 chromaSize;
	Uint32 productId;

	UNREFERENCED_PARAMETER(codecMode);
	UNREFERENCED_PARAMETER(height);

	if (vdi_get_sram_memory(coreIdx, &vb) < 0)
		return 0;

	CVI_VC_CFG("Get sram: addr=0x%llx, size=0x%x\n", (Uint64)vb.phys_addr,
		   vb.size);

	productId = ProductVpuGetId(coreIdx);

	if (!vb.size) {
		sa->bufSize = 0;
		sa->u.wave4.useIpEnable = 0;
		sa->u.wave4.useLfRowEnable = 0;
		sa->u.wave4.useBitEnable = 0;
		sa->u.wave4.useEncImdEnable = 0;
		sa->u.wave4.useEncLfEnable = 0;
		sa->u.wave4.useEncRdoEnable = 0;
		return 0;
	}

	sa->bufBase = vb.phys_addr;
	offset = 0;
	/* Intra Prediction */
	if (sa->u.wave4.useIpEnable == TRUE) {
		sa->u.wave4.bufIp = sa->bufBase + offset;

		CVI_VC_CFG("u.wave4.bufIp=0x%llx, offset=%d\n",
			   (Uint64)sa->u.wave4.bufIp, offset);

		switch (productId) {
#ifdef REDUNDENT_CODE
		case PRODUCT_ID_410:
			lumaSize = VPU_ALIGN16(width);
			chromaSize = VPU_ALIGN16(width);
			break;
		case PRODUCT_ID_412:
		case PRODUCT_ID_512:
		case PRODUCT_ID_515:
			if (codecMode == C7_VP9_DEC) {
				lumaSize = (VPU_ALIGN128(width) * 10) >> 3;
				chromaSize = (VPU_ALIGN128(width) * 10) >> 3;
			} else {
				lumaSize =
					((VPU_ALIGN16(width) * 10 + 127) >> 7)
					<< 4;
				chromaSize =
					((VPU_ALIGN16(width) * 10 + 127) >> 7)
					<< 4;
			}
			break;
#endif
		case PRODUCT_ID_4102:
		case PRODUCT_ID_420:
		case PRODUCT_ID_420L:
		case PRODUCT_ID_510:
			lumaSize = ((VPU_ALIGN16(width) * 10 + 127) >> 7) << 4;
			chromaSize = ((VPU_ALIGN16(width) * 10 + 127) >> 7)
				     << 4;
			break;
		default:
			return 0;
		}

		offset = lumaSize + chromaSize;
		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	/* Loopfilter row */
	if (sa->u.wave4.useLfRowEnable == TRUE) {
		sa->u.wave4.bufLfRow = sa->bufBase + offset;

		CVI_VC_CFG("u.wave4.bufLfRow=0x%llx, offset=%d\n",
			   (Uint64)sa->u.wave4.bufLfRow, offset);

		if (codecMode == C7_VP9_DEC) {
			if (profile == VP9_PROFILE_2) {
				lumaSize = VPU_ALIGN64(width) * 8 * 10 /
					   8; /* lumaLIne   : 8 */
				chromaSize = VPU_ALIGN64(width) * 8 * 10 /
					     8; /* chromaLine : 8 */
			} else {
				lumaSize = VPU_ALIGN64(width) * 8; /* lumaLIne
								      : 8 */
				chromaSize =
					VPU_ALIGN64(width) * 8; /* chromaLine
									: 8 */
			}
			offset += lumaSize + chromaSize;
		} else {
			Uint32 level = levelIdc / 30;
			if (level >= 5) {
				size = (VPU_ALIGN32(width) >> 1) * 13 +
				       VPU_ALIGN64(width) * 4;
			} else {
				size = VPU_ALIGN64(width) * 13;
			}
			offset += size;
		}
		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	if (sa->u.wave4.useBitEnable == TRUE) {
		sa->u.wave4.bufBit = sa->bufBase + offset;
		if (codecMode == C7_VP9_DEC) {
			size = (VPU_ALIGN64(width) >> 6) * (70 * 8);
		} else {
			size = 34 * 1024; /* Fixed size */
		}
		offset += size;
		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	if (sa->u.wave4.useEncImdEnable == TRUE) {
		/* Main   profile(8bit) : Align32(picWidth)
		 * Main10 profile(10bit): Align32(picWidth)
		 */
		sa->u.wave4.bufImd = sa->bufBase + offset;
		CVI_VC_CFG("u.wave4.bufImd=0x%llx, offset=%d\n",
			   (Uint64)sa->u.wave4.bufImd, offset);

		offset += VPU_ALIGN32(width);
		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	if (sa->u.wave4.useEncLfEnable == TRUE) {
		/* Main   profile(8bit) :
		 *              Luma   = Align64(picWidth) * 5
		 *              Chroma = Align64(picWidth) * 3
		 * Main10 profile(10bit) :
		 *              Luma   = Align64(picWidth) * 7
		 *              Chroma = Align64(picWidth) * 5
		 */
		Uint32 luma = (profile == HEVC_PROFILE_MAIN10 ? 7 : 5);
		Uint32 chroma = (profile == HEVC_PROFILE_MAIN10 ? 5 : 3);

		sa->u.wave4.bufLf = sa->bufBase + offset;
		CVI_VC_CFG("u.wave4.bufLf=0x%llx, offset=%d\n",
			   (Uint64)sa->u.wave4.bufLf, offset);

		lumaSize = VPU_ALIGN64(width) * luma;
		chromaSize = VPU_ALIGN64(width) * chroma;
		offset += lumaSize + chromaSize;

		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	if (sa->u.wave4.useEncRdoEnable == TRUE) {
		switch (productId) {
		case PRODUCT_ID_520:
			sa->u.wave4.bufRdo = sa->bufBase + offset;
			offset += ((288 * VPU_ALIGN64(width)) >> 6);
			break;
		default:
			/* Main   profile(8bit) : (Align64(picWidth)/32) * 22*16
			 * Main10 profile(10bit): (Align64(picWidth)/32) * 22*16
			 */
			sa->u.wave4.bufRdo = sa->bufBase + offset;
			offset += (VPU_ALIGN64(width) >> 5) * 22 * 16;
		}

		CVI_VC_CFG("u.wave4.bufRdo=0x%llx, offset=%d\n",
			   (Uint64)sa->u.wave4.bufRdo, offset);

		if (offset > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}
	}

	sa->bufSize = offset;

	CVI_VC_CFG("Calc sram used size=0x%x, sa->bufBase = 0x%llx\n", offset,
		   sa->bufBase);

	return 1;
}

#ifdef REDUNDENT_CODE
Int32 ConfigSecAXICoda7(Uint32 coreIdx, CodStd codStd, SecAxiInfo *sa,
			Uint32 width, Uint32 height, Uint32 profile)
{
	vpu_buffer_t vb;
	int offset;
	Uint32 size;
	Uint32 lumaSize;
	Uint32 chromaSize;
	Uint32 MbNumX = ((width & 0xFFFF) + 15) >> 4;
	Uint32 MbNumY = ((height & 0xFFFF) + 15) >> 4;

	if (vdi_get_sram_memory(coreIdx, &vb) < 0)
		return 0;

	if (!vb.size) {
		sa->bufSize = 0;
		sa->u.wave4.useIpEnable = 0;
		sa->u.wave4.useLfRowEnable = 0;
		sa->u.wave4.useBitEnable = 0;
		sa->u.coda9.useBitEnable = 0;
		sa->u.coda9.useIpEnable = 0;
		sa->u.coda9.useDbkYEnable = 0;
		sa->u.coda9.useDbkCEnable = 0;
		sa->u.coda9.useOvlEnable = 0;
		sa->u.coda9.useMeEnable = 0;
		return 0;
	}

	sa->bufBase = vb.phys_addr;
	offset = 0;

	if (codStd == STD_HEVC) {
		/* Intra Prediction */
		if (sa->u.wave4.useIpEnable == TRUE) {
			sa->u.wave4.bufIp = sa->bufBase + offset;
			lumaSize = ((VPU_ALIGN16(width) * 10 + 127) >> 7) << 4;
			chromaSize = ((VPU_ALIGN16(width) * 10 + 127) >> 7)
				     << 4;

			offset = lumaSize + chromaSize;
			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		/* Loopfilter row */
		if (sa->u.wave4.useLfRowEnable == TRUE) {
			/* Main   profile: Luma 5 lines + Chroma 3 lines
		Main10 profile: Luma 7 lines + Chroma 4 lines
	    */
			Uint32 lumaLines =
				(profile == HEVC_PROFILE_MAIN10 ? 7 : 5);
			Uint32 chromaLines =
				(profile == HEVC_PROFILE_MAIN10 ? 4 : 3);

			sa->u.wave4.bufLfRow = sa->bufBase + offset;
			lumaSize = VPU_ALIGN64(width) * lumaLines;
			chromaSize = VPU_ALIGN64(width) * chromaLines;

			offset += lumaSize + chromaSize;
			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		if (sa->u.wave4.useBitEnable == TRUE) {
			sa->u.wave4.bufBit = sa->bufBase + offset;
			size = VPU_ALIGN64(width) * 4;

			offset += size;
			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}
	} else { // for legacy codec
		/* BIT, VC1 Bit-plane */
		/* Coda7Q use same register for BIT and Bit-plane */
		if (sa->u.coda9.useBitEnable == TRUE) {
			sa->u.coda9.bufBitUse = sa->bufBase + offset;
			sa->u.coda9.useBitEnable = 1;

			switch (codStd) {
			case STD_AVC:
				offset = offset + MbNumX * 128;
				break; // AVC
			case STD_RV:
				offset = offset + MbNumX * 128;
				break;
			case STD_VC1: {
				int oneBTP;

				oneBTP =
					(((MbNumX + 15) >> 4) * MbNumY + 1) * 2;
				oneBTP = (oneBTP % 256) ?
						       ((oneBTP >> 8) + 1) * 256 :
						       oneBTP;

				offset = offset + oneBTP * 3;
				offset = offset + MbNumX * 64;
			} break;
			case STD_AVS:
				offset = offset + ((MbNumX + 3) & ~3) * 32;
				break;
			case STD_MPEG2:
				offset = offset + MbNumX * 0;
				break;
			case STD_VP8:
				offset = offset + MbNumX * 128;
				break;
			default:
				offset = offset + MbNumX * 16;
				break; // MPEG-4, Divx3
			}

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		/* Intra Prediction, ACDC */
		if (sa->u.coda9.useIpEnable == TRUE) {
			sa->u.coda9.bufIpAcDcUse = sa->bufBase + offset;
			sa->u.coda9.useIpEnable = 1;

			switch (codStd) {
			case STD_AVC:
				offset = offset + MbNumX * 64 * 2; // MBAFF
					// needs
					// double
					// buffer;
				break; // AVC
			case STD_RV:
				offset = offset + MbNumX * 64;
				break;
			case STD_VC1:
				offset = offset + MbNumX * 128;
				break;
			case STD_AVS:
				offset = offset + MbNumX * 64;
				break;
			case STD_MPEG2:
				offset = offset + MbNumX * 0;
				break;
			case STD_VP8:
				offset = offset + MbNumX * 64;
				break;
			default:
				offset = offset + MbNumX * 128;
				break; // MPEG-4, Divx3
			}

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		/* Deblock Chroma */
		if (sa->u.coda9.useDbkCEnable == TRUE) {
			sa->u.coda9.bufDbkCUse = sa->bufBase + offset;
			sa->u.coda9.useDbkCEnable = 1;
			switch (codStd) {
			case STD_AVC:
				offset = (profile == 66 || profile == 0) ?
						       (offset + (MbNumX * 64)) :
						       (offset + (MbNumX * 128));
				break; // AVC
			case STD_RV:
				offset = offset + MbNumX * 128;
				break;
			case STD_VC1:
				offset = profile == 2 ? offset + MbNumX * 256 :
							      offset + MbNumX * 128;
				offset = (offset + 255) & (~255);
				break;
			case STD_AVS:
				offset = offset + MbNumX * 128;
				break;
			case STD_MPEG2:
				offset = offset + MbNumX * 64;
				break;
			case STD_VP8:
				offset = offset + MbNumX * 128;
				break;
			default:
				offset = offset + MbNumX * 64;
				break;
			}

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		/* Deblock Luma */
		if (sa->u.coda9.useDbkYEnable == TRUE) {
			sa->u.coda9.bufDbkYUse = sa->bufBase + offset;
			sa->u.coda9.useDbkYEnable = 1;

			switch (codStd) {
			case STD_AVC:
				offset = (profile == 66 /*AVC BP decoder*/ ||
					  profile == 0 /* AVC encoder */) ?
						       (offset + (MbNumX * 64)) :
						       (offset + (MbNumX * 128));
				break; // AVC
			case STD_RV:
				offset = offset + MbNumX * 128;
				break;
			case STD_VC1:
				offset = profile == 2 ? offset + MbNumX * 256 :
							      offset + MbNumX * 128;
				offset = (offset + 255) & (~255);
				break;
			case STD_AVS:
				offset = offset + MbNumX * 128;
				break;
			case STD_MPEG2:
				offset = offset + MbNumX * 128;
				break;
			case STD_VP8:
				offset = offset + MbNumX * 128;
				break;
			default:
				offset = offset + MbNumX * 128;
				break;
			}

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		// check the buffer address which is 256 byte is available.
		if (((offset + 255) & (~255)) > (int)vb.size) {
			sa->bufSize = 0;
			return 0;
		}

		/* Motion Estimation */
		if (sa->u.coda9.useMeEnable == TRUE) {
			offset = (offset + 255) & (~255);
			sa->u.coda9.bufMeUse = sa->bufBase + offset;
			sa->u.coda9.useMeEnable = 1;

			offset = offset + MbNumX * 16 * 36 + 2048;

			if (offset > (int)vb.size) {
				sa->bufSize = 0;
				return 0;
			}
		}

		/* VC1 Overlap */
		if (sa->u.coda9.useOvlEnable == TRUE) {
			if (codStd != STD_VC1) {
				sa->u.coda9.useOvlEnable = 0;
			} else {
				sa->u.coda9.bufOvlUse = sa->bufBase + offset;
				sa->u.coda9.useOvlEnable = 1;

				offset = offset + MbNumX * 80;

				if (offset > (int)vb.size) {
					sa->bufSize = 0;
					return 0;
				}
			}
		}
	}

	sa->bufSize = offset;

	return 1;
}
#endif

static int SetTiledMapTypeV20(Uint32 coreIdx, TiledMapConfig *pMapCfg,
			      int mapType, int width, int interleave)
{
#define GEN_XY2AXI(INV, ZER, TBX, XY, BIT)                                     \
	((INV) << 7 | (ZER) << 6 | (TBX) << 5 | (XY) << 4 | (BIT))
#define GEN_CONFIG(A, B, C, D, E, F, G, H, I)                                  \
	((A) << 20 | (B) << 19 | (C) << 18 | (D) << 17 | (E) << 16 |           \
	 (F) << 12 | (G) << 8 | (H) << 4 | (I))
#define X_SEL 0
#define Y_SEL 1

	const int luma_map = 0x40; // zero, inv = 1'b0, zero = 1'b1 , tbxor =
		// 1'b0, xy = 1'b0, bit = 4'd0
	const int chro_map = 0x40; // zero, inv = 1'b0, zero = 1'b1 , tbxor =
		// 1'b0, xy = 1'b0, bit = 4'd0
	int width_chr;
	int i;

	pMapCfg->mapType = mapType;

	for (i = 0; i < 32; i = i + 1) {
		pMapCfg->xy2axiLumMap[i] = luma_map;
		pMapCfg->xy2axiChrMap[i] = chro_map;
	}
	pMapCfg->xy2axiConfig = 0;

	width_chr = (interleave) ? width : (width >> 1);

	switch (mapType) {
	case LINEAR_FRAME_MAP:
	case LINEAR_FIELD_MAP:
		pMapCfg->xy2axiConfig = 0;
#ifndef PLATFORM_NON_OS
		return 1;
#endif
		break;
	case TILED_FRAME_V_MAP: {
		// luma
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiLumMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiLumMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiLumMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiLumMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiLumMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiLumMap[13] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);

		if (width <= 512) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 1024) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 2048) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		// chroma
		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiChrMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiChrMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiChrMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiChrMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiChrMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiChrMap[12] = GEN_XY2AXI(1, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiChrMap[13] = GEN_XY2AXI(1, 0, 0, Y_SEL, 4);

		if (width_chr <= 512) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 1024) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 2048) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		// xy2axiConfig
		pMapCfg->xy2axiConfig = GEN_CONFIG(0, 0, 0, 1, 1, 15, 0, 15, 0);
		break;
	} // case TILED_FRAME_V_MAP
	case TILED_FRAME_H_MAP: {
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[8] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[9] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[10] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiLumMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiLumMap[13] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		if (width <= 512) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 1024) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 2048) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiChrMap[7] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[8] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[9] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[10] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiChrMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiChrMap[12] = GEN_XY2AXI(1, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiChrMap[13] = GEN_XY2AXI(1, 0, 0, Y_SEL, 4);
		if (width_chr <= 512) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 1024) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 2048) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		// xy2axiConfig
		pMapCfg->xy2axiConfig =
			GEN_CONFIG(0, 0, 0, 1, 0, 15, 15, 15, 15);
		break;
	} // case TILED_FRAME_H_MAP:
	case TILED_FIELD_V_MAP: {
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiLumMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiLumMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiLumMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiLumMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiLumMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiLumMap[13] = GEN_XY2AXI(0, 0, 1, Y_SEL, 5);
		if (width <= 512) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 1024) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 2048) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiChrMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiChrMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiChrMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiChrMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiChrMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiChrMap[12] = GEN_XY2AXI(1, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiChrMap[13] = GEN_XY2AXI(1, 0, 1, Y_SEL, 4);
		if (width_chr <= 512) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 1024) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 2048) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}

		// xy2axiConfig
		pMapCfg->xy2axiConfig =
			GEN_CONFIG(0, 1, 1, 1, 1, 15, 15, 15, 15);
		break;
	}
	case TILED_MIXED_V_MAP: {
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiLumMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiLumMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiLumMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiLumMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiLumMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiLumMap[13] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		if (width <= 512) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 1024) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 2048) {
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else { // 4K size
			pMapCfg->xy2axiLumMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiLumMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiChrMap[8] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiChrMap[9] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiChrMap[10] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiChrMap[11] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiChrMap[12] = GEN_XY2AXI(1, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiChrMap[13] = GEN_XY2AXI(1, 0, 0, Y_SEL, 4);
		if (width_chr <= 512) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 1024) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 2048) {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else {
			pMapCfg->xy2axiChrMap[14] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 8);
			pMapCfg->xy2axiChrMap[15] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		// xy2axiConfig
		pMapCfg->xy2axiConfig = GEN_CONFIG(0, 0, 1, 1, 1, 7, 7, 7, 7);
		break;
	}
	case TILED_FRAME_MB_RASTER_MAP: {
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);

		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);

		//-----------------------------------------------------------
		// mb_addr = mby*stride + mbx
		// mb_addr mapping:
		//   luma   : axi_addr[~:8] => axi_addr =
		//   {mb_addr[23:0],map_addr[7:0]} chroma : axi_addr[~:7] =>
		//   axi_addr = {mb_addr[23:0],map_addr[6:0]}
		//-----------------------------------------------------------

		// xy2axiConfig
		pMapCfg->xy2axiConfig = GEN_CONFIG(0, 0, 0, 1, 1, 15, 0, 7, 0);
		break;
	}
	case TILED_FIELD_MB_RASTER_MAP: {
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);

		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);

		//-----------------------------------------------------------
		// mb_addr = mby*stride + mbx
		// mb_addr mapping:
		//   luma   : axi_addr[~:7] => axi_addr =
		//   {mb_addr[23:0],map_addr[6:0]} chroma : axi_addr[~:6] =>
		//   axi_addr = {mb_addr[23:0],map_addr[5:0]}
		//-----------------------------------------------------------

		// xy2axiConfig
		pMapCfg->xy2axiConfig = GEN_CONFIG(0, 1, 1, 1, 1, 7, 7, 3, 3);

		break;
	}
	case TILED_FRAME_NO_BANK_MAP:
	case TILED_FIELD_NO_BANK_MAP: {
		// luma
		pMapCfg->xy2axiLumMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiLumMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiLumMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiLumMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiLumMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiLumMap[8] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiLumMap[9] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiLumMap[10] = GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
		pMapCfg->xy2axiLumMap[11] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiLumMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiLumMap[13] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiLumMap[14] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiLumMap[15] = GEN_XY2AXI(0, 0, 0, X_SEL, 8);
		if (width <= 512) {
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 1024) {
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width <= 2048) {
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else {
			pMapCfg->xy2axiLumMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiLumMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiLumMap[18] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiLumMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiLumMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiLumMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiLumMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiLumMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}
		// chroma
		pMapCfg->xy2axiChrMap[3] = GEN_XY2AXI(0, 0, 0, Y_SEL, 0);
		pMapCfg->xy2axiChrMap[4] = GEN_XY2AXI(0, 0, 0, Y_SEL, 1);
		pMapCfg->xy2axiChrMap[5] = GEN_XY2AXI(0, 0, 0, Y_SEL, 2);
		pMapCfg->xy2axiChrMap[6] = GEN_XY2AXI(0, 0, 0, Y_SEL, 3);
		pMapCfg->xy2axiChrMap[7] = GEN_XY2AXI(0, 0, 0, X_SEL, 3);
		pMapCfg->xy2axiChrMap[8] = GEN_XY2AXI(0, 0, 0, Y_SEL, 4);
		pMapCfg->xy2axiChrMap[9] = GEN_XY2AXI(0, 0, 0, Y_SEL, 5);
		pMapCfg->xy2axiChrMap[10] = GEN_XY2AXI(0, 0, 0, Y_SEL, 6);
		pMapCfg->xy2axiChrMap[11] = GEN_XY2AXI(0, 0, 0, X_SEL, 4);
		pMapCfg->xy2axiChrMap[12] = GEN_XY2AXI(0, 0, 0, X_SEL, 5);
		pMapCfg->xy2axiChrMap[13] = GEN_XY2AXI(0, 0, 0, X_SEL, 6);
		pMapCfg->xy2axiChrMap[14] = GEN_XY2AXI(0, 0, 0, X_SEL, 7);
		pMapCfg->xy2axiChrMap[15] = GEN_XY2AXI(0, 0, 0, X_SEL, 8);

		if (width_chr <= 512) {
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 1024) {
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else if (width_chr <= 2048) {
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		} else {
			pMapCfg->xy2axiChrMap[16] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 9);
			pMapCfg->xy2axiChrMap[17] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 10);
			pMapCfg->xy2axiChrMap[18] =
				GEN_XY2AXI(0, 0, 0, X_SEL, 11);
			pMapCfg->xy2axiChrMap[19] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 7);
			pMapCfg->xy2axiChrMap[20] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 8);
			pMapCfg->xy2axiChrMap[21] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 9);
			pMapCfg->xy2axiChrMap[22] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 10);
			pMapCfg->xy2axiChrMap[23] =
				GEN_XY2AXI(0, 0, 0, Y_SEL, 11);
		}

		// xy2axiConfig
		if (mapType == TILED_FRAME_NO_BANK_MAP)
			pMapCfg->xy2axiConfig =
				GEN_CONFIG(0, 0, 0, 1, 1, 15, 0, 15, 0);
		else
			pMapCfg->xy2axiConfig =
				GEN_CONFIG(0, 1, 1, 1, 1, 15, 15, 15, 15);

		break;
	}
	default:
		return 0;
	}

	for (i = 0; i < 32; i++) { // xy2axiLumMap
		VpuWriteReg(coreIdx, GDI_XY2AXI_LUM_BIT00 + 4 * i,
			    pMapCfg->xy2axiLumMap[i]);
	}

	for (i = 0; i < 32; i++) { // xy2axiChrMap
		VpuWriteReg(coreIdx, GDI_XY2AXI_CHR_BIT00 + 4 * i,
			    pMapCfg->xy2axiChrMap[i]);
	}

	// xy2axiConfig
	VpuWriteReg(coreIdx, GDI_XY2AXI_CONFIG, pMapCfg->xy2axiConfig);

	// fast access for reading
	pMapCfg->tbSeparateMap = (pMapCfg->xy2axiConfig >> 19) & 0x1;
	pMapCfg->topBotSplit = (pMapCfg->xy2axiConfig >> 18) & 0x1;
	pMapCfg->tiledMap = (pMapCfg->xy2axiConfig >> 17) & 0x1;

	return 1;
}

static int GetXY2AXILogic(int map_val, int xpos, int ypos, int tb)
{
	int invert;
	int assign_zero;
	int tbxor;
	int xysel;
	int bitsel;

	int xypos, xybit, xybit_st1, xybit_st2, xybit_st3;

	invert = map_val >> 7;
	assign_zero = (map_val & 0x78) >> 6;
	tbxor = (map_val & 0x3C) >> 5;
	xysel = (map_val & 0x1E) >> 4;
	bitsel = map_val & 0x0f;

	xypos = (xysel) ? ypos : xpos;
	xybit = (xypos >> bitsel) & 0x01;
	xybit_st1 = (tbxor) ? xybit ^ tb : xybit;
	xybit_st2 = (assign_zero) ? 0 : xybit_st1;
	xybit_st3 = (invert) ? !xybit_st2 : xybit_st2;

	return xybit_st3;
}

static int GetXY2AXIAddr20(TiledMapConfig *pMapCfg, int ycbcr, int posY,
			   int posX, int stride, FrameBuffer *fb)
{
	int tbSeparateMap;
	int use_linear_field;
	int ypos_field;
	int tb;
	int chr_flag;
	int ypos_mod;
	int i;
	int mbx, mby;
	int mbx_num;
	int mb_addr;
	int xy2axiLumMap;
	int xy2axiChrMap;
	int xy2axi_map_sel;
	int temp_bit;
	int tmp_addr;
	int axi_conv;

	int y_top_base;
	int cb_top_base;
	int cr_top_base;
	int y_bot_base;
	int cb_bot_base;
	int cr_bot_base;
	int top_base_addr;
	int bot_base_addr;
	int base_addr;
	int pix_addr;
	int mapType;
	mapType = fb->mapType;

	if (!pMapCfg)
		return -1;

	tbSeparateMap = pMapCfg->tbSeparateMap;
	use_linear_field = (mapType == 9);
	ypos_field = (posY >> 1);
	tb = posY & 1;
	ypos_mod = (tbSeparateMap | use_linear_field) ? ypos_field : posY;
	chr_flag = (ycbcr >> 1) & 0x1;

	mbx_num = (stride >> 4);

	y_top_base = fb->bufY;
	cb_top_base = fb->bufCb;
	cr_top_base = fb->bufCr;
	y_bot_base = fb->bufYBot;
	cb_bot_base = fb->bufCbBot;
	cr_bot_base = fb->bufCrBot;

	if (mapType == LINEAR_FRAME_MAP) {
		base_addr = (ycbcr == 0) ? y_top_base :
			    (ycbcr == 2) ? cb_top_base :
						 cr_top_base;
		pix_addr = ((posY * stride) + posX) + base_addr;
	} else if (mapType == LINEAR_FIELD_MAP) {
		top_base_addr = (ycbcr == 0) ? y_top_base :
				(ycbcr == 2) ? cb_top_base :
						     cr_top_base;
		bot_base_addr = (ycbcr == 0) ? y_bot_base :
				(ycbcr == 2) ? cb_bot_base :
						     cr_bot_base;
		base_addr = tb ? bot_base_addr : top_base_addr;

		pix_addr = ((ypos_mod * stride) + posX) + base_addr;
	} else if (mapType == TILED_FRAME_MB_RASTER_MAP ||
		   mapType == TILED_FIELD_MB_RASTER_MAP) {
		top_base_addr = (ycbcr == 0) ? y_top_base :
				(ycbcr == 2) ? cb_top_base :
						     cr_top_base;
		bot_base_addr = (ycbcr == 0) ? y_bot_base :
				(ycbcr == 2) ? cb_bot_base :
						     cr_bot_base;

		if (tbSeparateMap & tb)
			base_addr = bot_base_addr;
		else
			base_addr = top_base_addr;

		if (ycbcr == 0) {
			mbx = posX >> 4;
			mby = posY >> 4;
		} else { // always interleave
			mbx = posX >> 4;
			mby = posY >> 3;
		}

		mb_addr = mbx_num * mby + mbx;

		// axi_conv[7:0]
		axi_conv = 0;
		for (i = 0; i < 8; i++) {
			xy2axiLumMap = pMapCfg->xy2axiLumMap[i];
			xy2axiChrMap = pMapCfg->xy2axiChrMap[i];
			xy2axi_map_sel =
				(chr_flag) ? xy2axiChrMap : xy2axiLumMap;
			temp_bit = GetXY2AXILogic(xy2axi_map_sel, posX,
						  ypos_mod, tb);
			axi_conv = axi_conv + (temp_bit << i);
		}

		if (mapType == TILED_FRAME_MB_RASTER_MAP) {
			if (chr_flag == 0)
				tmp_addr = (mb_addr << 8) + axi_conv;
			else // chroma, interleaved only
				tmp_addr = (mb_addr << 7) + axi_conv;
		} else { // TILED_FIELD_MB_RASTER_MAP
			if (chr_flag == 0)
				tmp_addr = (mb_addr << 7) + axi_conv;
			else // chroma, interleaved only
				tmp_addr = (mb_addr << 6) + axi_conv;
		}

		pix_addr = tmp_addr + base_addr;
	} else {
		top_base_addr = (ycbcr == 0) ? y_top_base :
				(ycbcr == 2) ? cb_top_base :
						     cr_top_base;
		bot_base_addr = (ycbcr == 0) ? y_bot_base :
				(ycbcr == 2) ? cb_bot_base :
						     cr_bot_base;
		if (tbSeparateMap & tb)
			base_addr = bot_base_addr;
		else
			base_addr = top_base_addr;

		// axi_conv[31:0]
		axi_conv = 0;
		for (i = 0; i < 32; i++) {
			xy2axiLumMap = pMapCfg->xy2axiLumMap[i];
			xy2axiChrMap = pMapCfg->xy2axiChrMap[i];
			xy2axi_map_sel =
				(chr_flag) ? xy2axiChrMap : xy2axiLumMap;
			temp_bit = GetXY2AXILogic(xy2axi_map_sel, posX,
						  ypos_mod, tb);
			axi_conv = axi_conv + (temp_bit << i);
		}

		pix_addr = axi_conv + base_addr;
	}

	return pix_addr;
}

#ifdef REDUNDENT_CODE
// GDI related functios for GDI 1.0
PhysicalAddress GetTiledFrameBase(Uint32 coreIdx, FrameBuffer *frame, int num)
{
	PhysicalAddress baseAddr;
	int i;

	UNREFERENCED_PARAMETER(coreIdx);
	baseAddr = frame[0].bufY;
	for (i = 0; i < num; i++) {
		if (frame[i].bufY < baseAddr)
			baseAddr = frame[i].bufY;
	}

	return baseAddr;
}

void SetTiledFrameBase(Uint32 coreIdx, PhysicalAddress baseAddr)
{
	VpuWriteReg(coreIdx, GDI_TILEDBUF_BASE, baseAddr);
}

static int SetTiledMapTypeV10(Uint32 coreIdx, TiledMapConfig *pMapCfg,
			      DRAMConfig *dramCfg, int stride, int mapType)
{
#define XY2CONFIG(A, B, C, D, E, F, G, H, I)                                   \
	((A) << 20 | (B) << 19 | (C) << 18 | (D) << 17 | (E) << 16 |           \
	 (F) << 12 | (G) << 8 | (H) << 4 | (I))
#define XY2(A, B, C, D) ((A) << 12 | (B) << 8 | (C) << 4 | (D))
#define XY2BANK(A, B, C, D, E, F)                                              \
	((A) << 13 | (B) << 12 | (C) << 8 | (D) << 5 | (E) << 4 | (F))
#define RBC(A, B, C, D) ((A) << 10 | (B) << 6 | (C) << 4 | (D))
#define RBC_SAME(A, B) ((A) << 10 | (B) << 6 | (A) << 4 | (B))
#define X_SEL 0
#define Y_SEL 1
#define CA_SEL 0
#define BA_SEL 1
#define RA_SEL 2
#define Z_SEL 3
	int ret;
	int luma_map;
	int chro_map;
	int i;

	UNREFERENCED_PARAMETER(stride);
	pMapCfg->mapType = mapType;
	//         inv = 1'b0, zero = 1'b1 , tbxor = 1'b0, xy = 1'b0, bit = 4'd0
	luma_map = 64;
	chro_map = 64;

	for (i = 0; i < 16; i = i + 1) {
		pMapCfg->xy2caMap[i] = luma_map << 8 | chro_map;
	}

	for (i = 0; i < 4; i = i + 1) {
		pMapCfg->xy2baMap[i] = luma_map << 8 | chro_map;
	}

	for (i = 0; i < 16; i = i + 1) {
		pMapCfg->xy2raMap[i] = luma_map << 8 | chro_map;
	}

	ret = stride; // this will be removed after map size optimizing.
	ret = 0;
	switch (mapType) {
	case LINEAR_FRAME_MAP:
		pMapCfg->xy2rbcConfig = 0;
		ret = 1;
		break;
	case TILED_FRAME_V_MAP:
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 &&
		    dramCfg->rasBit == 13) {
			// CNN setting
			// cas
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 4);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 8, X_SEL, 8);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[2] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[12] = XY2(Y_SEL, 15, Y_SEL, 15);

		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 13) {
			// cas
			pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[4] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 5);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 16) {
			// DDR3 3BA, DDR4 1BG+2BA
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);

			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(X_SEL, 7, X_SEL, 7);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 5);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 9, 4, X_SEL, 9);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 4);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[1] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 10, Y_SEL, 10);

			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 4 &&
			   dramCfg->rasBit == 15) {
			// DDR3 4BA, DDR4 2BG+2BA
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);

			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 4);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 5, Y_SEL, 6);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, X_SEL, 9, 4, X_SEL, 9);
			pMapCfg->xy2baMap[3] =
				XY2BANK(0, Y_SEL, 6, 4, Y_SEL, 5);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[1] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 10, Y_SEL, 10);

			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 15, Y_SEL, 15);
		}
		// xy2rbcConfig
		pMapCfg->xy2rbcConfig = XY2CONFIG(0, 0, 0, 1, 1, 15, 0, 15, 0);
		break;
	case TILED_FRAME_H_MAP:
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 &&
		    dramCfg->rasBit == 13) {
			// CNN setting
			// cas
			pMapCfg->xy2caMap[0] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[1] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[2] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[3] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[4] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[5] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[6] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[7] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 4);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 8, X_SEL, 8);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[2] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[12] = XY2(Y_SEL, 15, Y_SEL, 15);

		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 13) {
			// cas
			pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[2] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[3] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[4] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[5] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[6] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[7] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 5);
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 15, Y_SEL, 15);
		}
		// xy2rbcConfig
		pMapCfg->xy2rbcConfig =
			XY2CONFIG(0, 0, 0, 1, 0, 15, 15, 15, 15);
		break;
	case TILED_FIELD_V_MAP:
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 &&
		    dramCfg->rasBit == 13) {
			// CNN setting
			// cas
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(1, Y_SEL, 5, 5, Y_SEL, 4);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 8, X_SEL, 8);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[2] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[12] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 13) {
			// cas
			pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[4] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 5);
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 16) {
			// // DDR3 3BA, DDR4 1BG+2BA
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);

			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(X_SEL, 7, X_SEL, 7);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 5);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 9, 4, X_SEL, 9);
			pMapCfg->xy2baMap[2] =
				XY2BANK(1, Y_SEL, 5, 5, Y_SEL, 4);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[1] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 10, Y_SEL, 10);

			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 4 &&
			   dramCfg->rasBit == 15) {
			// // DDR3 4BA, DDR4 2BG+2BA
			// cas
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);

			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 4);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 5, Y_SEL, 6);

			// xy2rbcConfig
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, X_SEL, 9, 4, X_SEL, 9);
			pMapCfg->xy2baMap[3] =
				XY2BANK(1, Y_SEL, 6, 5, Y_SEL, 5);

			// ras
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[1] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 10, Y_SEL, 10);

			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 15, Y_SEL, 15);
		}
		// xy2rbcConfig
		pMapCfg->xy2rbcConfig =
			XY2CONFIG(0, 1, 1, 1, 1, 15, 15, 15, 15);
		break;
	case TILED_MIXED_V_MAP:
		// cas
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 &&
		    dramCfg->rasBit == 13) {
			// CNN setting
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 4);

			pMapCfg->xy2raMap[0] = XY2(X_SEL, 8, X_SEL, 8);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[2] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[12] = XY2(Y_SEL, 15, Y_SEL, 15);
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 13) {
			// cas
			pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
			pMapCfg->xy2caMap[4] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
			pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
			pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
			pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

			// bank
			pMapCfg->xy2baMap[0] =
				XY2BANK(0, X_SEL, 7, 4, X_SEL, 7);
			pMapCfg->xy2baMap[1] =
				XY2BANK(0, X_SEL, 8, 4, X_SEL, 8);
			pMapCfg->xy2baMap[2] =
				XY2BANK(0, Y_SEL, 5, 4, Y_SEL, 5);
			pMapCfg->xy2raMap[0] = XY2(X_SEL, 9, X_SEL, 9);
			pMapCfg->xy2raMap[1] = XY2(X_SEL, 10, X_SEL, 10);
			pMapCfg->xy2raMap[2] = XY2(Y_SEL, 6, Y_SEL, 6);
			pMapCfg->xy2raMap[3] = XY2(Y_SEL, 7, Y_SEL, 7);
			pMapCfg->xy2raMap[4] = XY2(Y_SEL, 8, Y_SEL, 8);
			pMapCfg->xy2raMap[5] = XY2(Y_SEL, 9, Y_SEL, 9);
			pMapCfg->xy2raMap[6] = XY2(Y_SEL, 10, Y_SEL, 10);
			pMapCfg->xy2raMap[7] = XY2(Y_SEL, 11, Y_SEL, 11);
			pMapCfg->xy2raMap[8] = XY2(Y_SEL, 12, Y_SEL, 12);
			pMapCfg->xy2raMap[9] = XY2(Y_SEL, 13, Y_SEL, 13);
			pMapCfg->xy2raMap[10] = XY2(Y_SEL, 14, Y_SEL, 14);
			pMapCfg->xy2raMap[11] = XY2(Y_SEL, 15, Y_SEL, 15);
		}

		// xy2rbcConfig
		pMapCfg->xy2rbcConfig = XY2CONFIG(0, 0, 1, 1, 1, 7, 7, 7, 7);
		break;
	case TILED_FRAME_MB_RASTER_MAP:
		// cas
		pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
		pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
		pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
		pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, X_SEL, 3);
		pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, 4, 0);

		// xy2rbcConfig
		pMapCfg->xy2rbcConfig = XY2CONFIG(0, 0, 0, 1, 1, 15, 0, 7, 0);
		break;
	case TILED_FIELD_MB_RASTER_MAP:
		// cas
		pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
		pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
		pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, X_SEL, 3);
		pMapCfg->xy2caMap[3] = XY2(X_SEL, 3, 4, 0);

		// xy2rbcConfig
		pMapCfg->xy2rbcConfig = XY2CONFIG(0, 1, 1, 1, 1, 7, 7, 3, 3);
		break;
	default:
		return 0;
	}

	if (mapType == TILED_FRAME_MB_RASTER_MAP) {
		pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[2] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 0, CA_SEL, 0);
		pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 1, CA_SEL, 1);
		pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 2, CA_SEL, 2);
		pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 3, CA_SEL, 3);
		pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 4, CA_SEL, 8);
		pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 8, CA_SEL, 9);
		pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 9, CA_SEL, 10);
		pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 10, CA_SEL, 11);
		pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 11, CA_SEL, 12);
		pMapCfg->rbc2axiMap[12] = RBC(CA_SEL, 12, CA_SEL, 13);
		pMapCfg->rbc2axiMap[13] = RBC(CA_SEL, 13, CA_SEL, 14);
		pMapCfg->rbc2axiMap[14] = RBC(CA_SEL, 14, CA_SEL, 15);
		pMapCfg->rbc2axiMap[15] = RBC(CA_SEL, 15, RA_SEL, 0);
		pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 0, RA_SEL, 1);
		pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 1, RA_SEL, 2);
		pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 2, RA_SEL, 3);
		pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 3, RA_SEL, 4);
		pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 4, RA_SEL, 5);
		pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 5, RA_SEL, 6);
		pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 6, RA_SEL, 7);
		pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 7, RA_SEL, 8);
		pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 8, RA_SEL, 9);
		pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 9, RA_SEL, 10);
		pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 10, RA_SEL, 11);
		pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 11, RA_SEL, 12);
		pMapCfg->rbc2axiMap[28] = RBC(RA_SEL, 12, RA_SEL, 13);
		pMapCfg->rbc2axiMap[29] = RBC(RA_SEL, 13, RA_SEL, 14);
		pMapCfg->rbc2axiMap[30] = RBC(RA_SEL, 14, RA_SEL, 15);
		pMapCfg->rbc2axiMap[31] = RBC(RA_SEL, 15, Z_SEL, 0);
		ret = 1;
	} else if (mapType == TILED_FIELD_MB_RASTER_MAP) {
		pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[2] = RBC(Z_SEL, 0, Z_SEL, 0);
		pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 0, CA_SEL, 0);
		pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 1, CA_SEL, 1);
		pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 2, CA_SEL, 2);
		pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 3, CA_SEL, 8);
		pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 8, CA_SEL, 9);
		pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 9, CA_SEL, 10);
		pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 10, CA_SEL, 11);
		pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 11, CA_SEL, 12);
		pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 12, CA_SEL, 13);
		pMapCfg->rbc2axiMap[12] = RBC(CA_SEL, 13, CA_SEL, 14);
		pMapCfg->rbc2axiMap[13] = RBC(CA_SEL, 14, CA_SEL, 15);
		pMapCfg->rbc2axiMap[14] = RBC(CA_SEL, 15, RA_SEL, 0);

		pMapCfg->rbc2axiMap[15] = RBC(RA_SEL, 0, RA_SEL, 1);
		pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 1, RA_SEL, 2);
		pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 2, RA_SEL, 3);
		pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 3, RA_SEL, 4);
		pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 4, RA_SEL, 5);
		pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 5, RA_SEL, 6);
		pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 6, RA_SEL, 7);
		pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 7, RA_SEL, 8);
		pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 8, RA_SEL, 9);
		pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 9, RA_SEL, 10);
		pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 10, RA_SEL, 11);
		pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 11, RA_SEL, 12);
		pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 12, RA_SEL, 13);
		pMapCfg->rbc2axiMap[28] = RBC(RA_SEL, 13, RA_SEL, 14);
		pMapCfg->rbc2axiMap[29] = RBC(RA_SEL, 14, RA_SEL, 15);
		pMapCfg->rbc2axiMap[30] = RBC(RA_SEL, 15, Z_SEL, 0);
		pMapCfg->rbc2axiMap[31] = RBC(Z_SEL, 0, Z_SEL, 0);
		ret = 1;
	} else {
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 &&
		    dramCfg->rasBit == 13) {
			pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[2] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 0, CA_SEL, 0);
			pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 1, CA_SEL, 1);
			pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 2, CA_SEL, 2);
			pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 3, CA_SEL, 3);
			pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 4, CA_SEL, 4);
			pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 5, CA_SEL, 5);
			pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 6, CA_SEL, 6);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 7, CA_SEL, 7);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 8, CA_SEL, 8);

			pMapCfg->rbc2axiMap[12] = RBC(BA_SEL, 0, BA_SEL, 0);
			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL, 1, BA_SEL, 1);

			pMapCfg->rbc2axiMap[14] = RBC(RA_SEL, 0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[15] = RBC(RA_SEL, 1, RA_SEL, 1);
			pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 2, RA_SEL, 2);
			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 3, RA_SEL, 3);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 4, RA_SEL, 4);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 5, RA_SEL, 5);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 6, RA_SEL, 6);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 7, RA_SEL, 7);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 8, RA_SEL, 8);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 9, RA_SEL, 9);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 10, RA_SEL, 10);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 11, RA_SEL, 11);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 12, RA_SEL, 12);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 13, RA_SEL, 13);
			pMapCfg->rbc2axiMap[28] = RBC(RA_SEL, 14, RA_SEL, 14);
			pMapCfg->rbc2axiMap[29] = RBC(RA_SEL, 15, RA_SEL, 15);
			pMapCfg->rbc2axiMap[30] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[31] = RBC(Z_SEL, 0, Z_SEL, 0);

			ret = 1;
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 13) {
			pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[2] = RBC(CA_SEL, 0, CA_SEL, 0);
			pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 1, CA_SEL, 1);
			pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 2, CA_SEL, 2);
			pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 3, CA_SEL, 3);
			pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 4, CA_SEL, 4);
			pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 5, CA_SEL, 5);
			pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 6, CA_SEL, 6);
			pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 7, CA_SEL, 7);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 8, CA_SEL, 8);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 9, CA_SEL, 9);

			pMapCfg->rbc2axiMap[12] = RBC(BA_SEL, 0, BA_SEL, 0);
			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL, 1, BA_SEL, 1);
			pMapCfg->rbc2axiMap[14] = RBC(BA_SEL, 2, BA_SEL, 2);

			pMapCfg->rbc2axiMap[15] = RBC(RA_SEL, 0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 1, RA_SEL, 1);
			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 2, RA_SEL, 2);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 3, RA_SEL, 3);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 4, RA_SEL, 4);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 5, RA_SEL, 5);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 6, RA_SEL, 6);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 7, RA_SEL, 7);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 8, RA_SEL, 8);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 9, RA_SEL, 9);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 10, RA_SEL, 10);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 11, RA_SEL, 11);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 12, RA_SEL, 12);
			pMapCfg->rbc2axiMap[28] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[29] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[30] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[31] = RBC(Z_SEL, 0, Z_SEL, 0);

			ret = 1;
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 3 &&
			   dramCfg->rasBit == 16) {
			// DDR3 3BA, DDR4 1BG+2BA
			pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[2] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 0, CA_SEL, 0);
			pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 1, CA_SEL, 1);
			pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 2, CA_SEL, 2);
			pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 3, CA_SEL, 3);
			pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 4, CA_SEL, 4);
			pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 5, CA_SEL, 5);
			pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 6, CA_SEL, 6);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 7, CA_SEL, 7);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 8, CA_SEL, 8);
			pMapCfg->rbc2axiMap[12] = RBC(CA_SEL, 9, CA_SEL, 9);

			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL, 0, BA_SEL, 0);
			pMapCfg->rbc2axiMap[14] = RBC(BA_SEL, 1, BA_SEL, 1);
			pMapCfg->rbc2axiMap[15] = RBC(BA_SEL, 2, BA_SEL, 2);

			pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 1, RA_SEL, 1);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 2, RA_SEL, 2);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 3, RA_SEL, 3);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 4, RA_SEL, 4);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 5, RA_SEL, 5);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 6, RA_SEL, 6);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 7, RA_SEL, 7);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 8, RA_SEL, 8);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 9, RA_SEL, 9);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 10, RA_SEL, 10);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 11, RA_SEL, 11);
			pMapCfg->rbc2axiMap[28] = RBC(RA_SEL, 12, RA_SEL, 12);
			pMapCfg->rbc2axiMap[29] = RBC(RA_SEL, 13, RA_SEL, 13);
			pMapCfg->rbc2axiMap[30] = RBC(RA_SEL, 14, RA_SEL, 14);
			pMapCfg->rbc2axiMap[31] = RBC(RA_SEL, 15, RA_SEL, 15);

			ret = 1;
		} else if (dramCfg->casBit == 10 && dramCfg->bankBit == 4 &&
			   dramCfg->rasBit == 15) {
			// DDR3 4BA, DDR4 2BG+2BA
			pMapCfg->rbc2axiMap[0] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[1] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[2] = RBC(Z_SEL, 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[3] = RBC(CA_SEL, 0, CA_SEL, 0);
			pMapCfg->rbc2axiMap[4] = RBC(CA_SEL, 1, CA_SEL, 1);
			pMapCfg->rbc2axiMap[5] = RBC(CA_SEL, 2, CA_SEL, 2);
			pMapCfg->rbc2axiMap[6] = RBC(CA_SEL, 3, CA_SEL, 3);
			pMapCfg->rbc2axiMap[7] = RBC(CA_SEL, 4, CA_SEL, 4);
			pMapCfg->rbc2axiMap[8] = RBC(CA_SEL, 5, CA_SEL, 5);
			pMapCfg->rbc2axiMap[9] = RBC(CA_SEL, 6, CA_SEL, 6);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL, 7, CA_SEL, 7);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL, 8, CA_SEL, 8);
			pMapCfg->rbc2axiMap[12] = RBC(CA_SEL, 9, CA_SEL, 9);

			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL, 2, BA_SEL, 2);
			pMapCfg->rbc2axiMap[14] = RBC(BA_SEL, 3, BA_SEL, 3);
			pMapCfg->rbc2axiMap[15] = RBC(BA_SEL, 0, BA_SEL, 0);
			pMapCfg->rbc2axiMap[16] = RBC(BA_SEL, 1, BA_SEL, 1);

			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 1, RA_SEL, 1);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 2, RA_SEL, 2);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 3, RA_SEL, 3);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 4, RA_SEL, 4);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 5, RA_SEL, 5);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 6, RA_SEL, 6);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 7, RA_SEL, 7);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 8, RA_SEL, 8);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL, 9, RA_SEL, 9);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL, 10, RA_SEL, 10);
			pMapCfg->rbc2axiMap[28] = RBC(RA_SEL, 11, RA_SEL, 11);
			pMapCfg->rbc2axiMap[29] = RBC(RA_SEL, 12, RA_SEL, 12);
			pMapCfg->rbc2axiMap[30] = RBC(RA_SEL, 13, RA_SEL, 13);
			pMapCfg->rbc2axiMap[31] = RBC(RA_SEL, 14, RA_SEL, 14);

			ret = 1;
		}
	}

	for (i = 0; i < 16; i++) { // xy2ca_map
		VpuWriteReg(coreIdx, GDI_XY2_CAS_0 + 4 * i,
			    pMapCfg->xy2caMap[i]);
	}

	for (i = 0; i < 4; i++) { // xy2baMap
		VpuWriteReg(coreIdx, GDI_XY2_BA_0 + 4 * i,
			    pMapCfg->xy2baMap[i]);
	}

	for (i = 0; i < 16; i++) { // xy2raMap
		VpuWriteReg(coreIdx, GDI_XY2_RAS_0 + 4 * i,
			    pMapCfg->xy2raMap[i]);
	}

	// xy2rbcConfig
	VpuWriteReg(coreIdx, GDI_XY2_RBC_CONFIG, pMapCfg->xy2rbcConfig);
	//// fast access for reading
	pMapCfg->tbSeparateMap = (pMapCfg->xy2rbcConfig >> 19) & 0x1;
	pMapCfg->topBotSplit = (pMapCfg->xy2rbcConfig >> 18) & 0x1;
	pMapCfg->tiledMap = (pMapCfg->xy2rbcConfig >> 17) & 0x1;

	// RAS, BA, CAS -> Axi Addr
	for (i = 0; i < 32; i++) {
		VpuWriteReg(coreIdx, GDI_RBC2_AXI_0 + 4 * i,
			    pMapCfg->rbc2axiMap[i]);
	}

	return ret;
}

static int GetXY2RBCLogic(int map_val, int xpos, int ypos, int tb)
{
	int invert;
	int assign_zero;
	int tbxor;
	int xysel;
	int bitsel;

	int xypos, xybit, xybit_st1, xybit_st2, xybit_st3;

	invert = map_val >> 7;
	assign_zero = (map_val & 0x78) >> 6;
	tbxor = (map_val & 0x3C) >> 5;
	xysel = (map_val & 0x1E) >> 4;
	bitsel = map_val & 0x0f;

	xypos = (xysel) ? ypos : xpos;
	xybit = (xypos >> bitsel) & 0x01;
	xybit_st1 = (tbxor) ? xybit ^ tb : xybit;
	xybit_st2 = (assign_zero) ? 0 : xybit_st1;
	xybit_st3 = (invert) ? !xybit_st2 : xybit_st2;

	return xybit_st3;
}

static int GetRBC2AXILogic(int map_val, int ra_in, int ba_in, int ca_in)
{
	int rbc;
	int rst_bit;
	int rbc_sel = map_val >> 4;
	int bit_sel = map_val & 0x0f;

	if (rbc_sel == 0)
		rbc = ca_in;
	else if (rbc_sel == 1)
		rbc = ba_in;
	else if (rbc_sel == 2)
		rbc = ra_in;
	else
		rbc = 0;

	rst_bit = ((rbc >> bit_sel) & 1);

	return rst_bit;
}

static int GetXY2AXIAddrV10(TiledMapConfig *pMapCfg, int ycbcr, int posY,
			    int posX, int stride, FrameBuffer *fb)
{
	int ypos_mod;
	int temp;
	int temp_bit;
	int i;
	int tb;
	int ra_base;
	int ras_base;
	int ra_conv, ba_conv, ca_conv;

	int pix_addr;

	int lum_top_base, chr_top_base;
	int lum_bot_base, chr_bot_base;

	int mbx, mby, mb_addr;
	int temp_val12bit, temp_val6bit;
	int Addr;
	int mb_raster_base;

	if (!pMapCfg)
		return -1;

	pix_addr = 0;
	mb_raster_base = 0;
	ra_conv = 0;
	ba_conv = 0;
	ca_conv = 0;

	tb = posY & 0x1;

	ypos_mod = pMapCfg->tbSeparateMap ? posY >> 1 : posY;

	Addr = ycbcr == 0 ? fb->bufY : ycbcr == 2 ? fb->bufCb : fb->bufCr;

	if (fb->mapType == LINEAR_FRAME_MAP)
		return ((posY * stride) + posX) + Addr;

	// 20bit = AddrY [31:12]
	lum_top_base = fb->bufY >> 12;

	// 20bit = AddrY [11: 0], AddrCb[31:24]
	chr_top_base = ((fb->bufY & 0xfff) << 8) |
		       ((fb->bufCb >> 24) & 0xff); // 12bit +  (32-24) bit

	// 20bit = AddrCb[23: 4]
	lum_bot_base = (fb->bufCb >> 4) & 0xfffff;

	// 20bit = AddrCb[ 3: 0], AddrCr[31:16]
	chr_bot_base = ((fb->bufCb & 0xf) << 16) | ((fb->bufCr >> 16) & 0xffff);

	if (fb->mapType == TILED_FRAME_MB_RASTER_MAP ||
	    fb->mapType == TILED_FIELD_MB_RASTER_MAP) {
		if (ycbcr == 0) {
			mbx = posX >> 4;
			mby = posY >> 4;
		} else {
			// always interleave
			mbx = posX >> 4;
			mby = posY >> 3;
		}

		mb_addr = (stride >> 4) * mby + mbx;

		// ca[7:0]
		for (i = 0; i < 8; i++) {
			if (ycbcr == 2 || ycbcr == 3)
				temp = pMapCfg->xy2caMap[i] & 0xff;
			else
				temp = pMapCfg->xy2caMap[i] >> 8;
			temp_bit = GetXY2RBCLogic(temp, posX, ypos_mod, tb);
			ca_conv = ca_conv + (temp_bit << i);
		}

		// ca[15:8]
		ca_conv = ca_conv + ((mb_addr & 0xff) << 8);

		// ra[15:0]
		ra_conv = mb_addr >> 8;

		// ra,ba,ca -> axi
		for (i = 0; i < 32; i++) {
			temp_val12bit = pMapCfg->rbc2axiMap[i];
			temp_val6bit = (ycbcr == 0) ? (temp_val12bit >> 6) :
							    (temp_val12bit & 0x3f);

			temp_bit = GetRBC2AXILogic(temp_val6bit, ra_conv,
						   ba_conv, ca_conv);

			pix_addr = pix_addr + (temp_bit << i);
		}

		if (pMapCfg->tbSeparateMap == 1 && tb == 1)
			mb_raster_base =
				ycbcr == 0 ? lum_bot_base : chr_bot_base;
		else
			mb_raster_base =
				ycbcr == 0 ? lum_top_base : chr_top_base;

		pix_addr = pix_addr + (mb_raster_base << 12);
	} else {
		// ca
		for (i = 0; i < 16; i++) {
			if (ycbcr == 0 || ycbcr == 1) // clair : there are no
				// case ycbcr = 1
				temp = pMapCfg->xy2caMap[i] >> 8;
			else
				temp = pMapCfg->xy2caMap[i] & 0xff;

			temp_bit = GetXY2RBCLogic(temp, posX, ypos_mod, tb);
			ca_conv = ca_conv + (temp_bit << i);
		}

		// ba
		for (i = 0; i < 4; i++) {
			if (ycbcr == 2 || ycbcr == 3)
				temp = pMapCfg->xy2baMap[i] & 0xff;
			else
				temp = pMapCfg->xy2baMap[i] >> 8;

			temp_bit = GetXY2RBCLogic(temp, posX, ypos_mod, tb);
			ba_conv = ba_conv + (temp_bit << i);
		}

		// ras
		for (i = 0; i < 16; i++) {
			if (ycbcr == 2 || ycbcr == 3)
				temp = pMapCfg->xy2raMap[i] & 0xff;
			else
				temp = pMapCfg->xy2raMap[i] >> 8;

			temp_bit = GetXY2RBCLogic(temp, posX, ypos_mod, tb);
			ra_conv = ra_conv + (temp_bit << i);
		}

		if (pMapCfg->tbSeparateMap == 1 && tb == 1)
			ras_base = Addr >> 16;
		else
			ras_base = Addr & 0xffff;

		ra_base = ra_conv + ras_base;
		pix_addr = 0;

		// ra,ba,ca -> axi
		for (i = 0; i < 32; i++) {
			temp_val12bit = pMapCfg->rbc2axiMap[i];
			temp_val6bit = (ycbcr == 0) ? (temp_val12bit >> 6) :
							    (temp_val12bit & 0x3f);

			temp_bit = GetRBC2AXILogic(temp_val6bit, ra_base,
						   ba_conv, ca_conv);

			pix_addr = pix_addr + (temp_bit << i);
		}
		pix_addr += pMapCfg->tiledBaseAddr;
	}

	return pix_addr;
}
#endif

int GetXY2AXIAddr(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX,
		  int stride, FrameBuffer *fb)
{
	if (pMapCfg->productId == PRODUCT_ID_980 ||
	    PRODUCT_ID_W_SERIES(pMapCfg->productId))
		return GetXY2AXIAddr20(pMapCfg, ycbcr, posY, posX, stride, fb);
#ifdef REDUNDENT_CODE
	else if (pMapCfg->productId == PRODUCT_ID_960)
		return GetXY2AXIAddrV10(pMapCfg, ycbcr, posY, posX, stride, fb);
#endif

	return 0;
}

int SetTiledMapType(Uint32 coreIdx, TiledMapConfig *pMapCfg, int mapType,
		    int stride, int interleave, DRAMConfig *pDramCfg)
{
	int ret;
#ifndef REDUNDENT_CODE
	UNREFERENCED_PARAM(pDramCfg);
#endif

	switch (pMapCfg->productId) {
	case PRODUCT_ID_980:
		ret = SetTiledMapTypeV20(coreIdx, pMapCfg, mapType, stride,
					 interleave);
		break;
#ifdef REDUNDENT_CODE
	case PRODUCT_ID_960:
		ret = SetTiledMapTypeV10(coreIdx, pMapCfg, pDramCfg, stride,
					 mapType);
		break;
#endif
	case PRODUCT_ID_7503:
	case PRODUCT_ID_410:
	case PRODUCT_ID_4102:
	case PRODUCT_ID_420:
	case PRODUCT_ID_420L:
	case PRODUCT_ID_412:
	case PRODUCT_ID_7Q:
	case PRODUCT_ID_510:
	case PRODUCT_ID_512:
	case PRODUCT_ID_515:
	case PRODUCT_ID_520:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

Int32 CalcStride(Uint32 width, Uint32 height, FrameBufferFormat format,
		 BOOL cbcrInterleave, TiledMapType mapType, BOOL isVP9,
		 BOOL isDec)
{
	Uint32 lumaStride = 0;
	Uint32 chromaStride = 0;

	lumaStride = VPU_ALIGN32(width);

	if (height > width) {
		if ((mapType >= TILED_FRAME_V_MAP &&
		     mapType <= TILED_MIXED_V_MAP) ||
		    mapType == TILED_FRAME_NO_BANK_MAP ||
		    mapType == TILED_FIELD_NO_BANK_MAP)
			width = VPU_ALIGN16(height); // TiledMap constraints
	}
	if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
		Uint32 twice = 0;

		twice = cbcrInterleave == TRUE ? 2 : 1;
		switch (format) {
		case FORMAT_420:
			/* nothing to do */
			break;
		case FORMAT_420_P10_16BIT_LSB:
		case FORMAT_420_P10_16BIT_MSB:
		case FORMAT_422_P10_16BIT_MSB:
		case FORMAT_422_P10_16BIT_LSB:
			lumaStride = VPU_ALIGN32(width) * 2;
			break;
		case FORMAT_420_P10_32BIT_LSB:
		case FORMAT_420_P10_32BIT_MSB:
		case FORMAT_422_P10_32BIT_MSB:
		case FORMAT_422_P10_32BIT_LSB:
			if (isVP9 == TRUE) {
				lumaStride =
					VPU_ALIGN32(((width + 11) / 12) * 16);
				chromaStride =
					(((width >> 1) + 11) * twice / 12) * 16;
			} else {
				width = VPU_ALIGN32(width);
				lumaStride =
					((VPU_ALIGN16(width) + 11) / 12) * 16;
				chromaStride =
					((VPU_ALIGN16((width >> 1)) + 11) *
					 twice / 12) *
					16;
				// if ( isWAVE410 == TRUE ) {
				if ((chromaStride * 2) > lumaStride) {
					lumaStride = chromaStride * 2;
					VLOG(ERR,
					     "double chromaStride size is bigger than lumaStride\n");
				}
				//}
			}
			if (cbcrInterleave == TRUE) {
				lumaStride = MAX(lumaStride, chromaStride);
			}
			break;
		case FORMAT_422:
			/* nothing to do */
			break;
		case FORMAT_YUYV: // 4:2:2 8bit packed
		case FORMAT_YVYU:
		case FORMAT_UYVY:
		case FORMAT_VYUY:
			lumaStride = VPU_ALIGN32(width) * 2;
			break;
		case FORMAT_YUYV_P10_16BIT_MSB: // 4:2:2 10bit packed
		case FORMAT_YUYV_P10_16BIT_LSB:
		case FORMAT_YVYU_P10_16BIT_MSB:
		case FORMAT_YVYU_P10_16BIT_LSB:
		case FORMAT_UYVY_P10_16BIT_MSB:
		case FORMAT_UYVY_P10_16BIT_LSB:
		case FORMAT_VYUY_P10_16BIT_MSB:
		case FORMAT_VYUY_P10_16BIT_LSB:
			lumaStride = VPU_ALIGN32(width) * 4;
			break;
		case FORMAT_YUYV_P10_32BIT_MSB:
		case FORMAT_YUYV_P10_32BIT_LSB:
		case FORMAT_YVYU_P10_32BIT_MSB:
		case FORMAT_YVYU_P10_32BIT_LSB:
		case FORMAT_UYVY_P10_32BIT_MSB:
		case FORMAT_UYVY_P10_32BIT_LSB:
		case FORMAT_VYUY_P10_32BIT_MSB:
		case FORMAT_VYUY_P10_32BIT_LSB:
			lumaStride = VPU_ALIGN32(width * 2) * 2;
			break;
		default:
			break;
		}
	} else if (mapType == COMPRESSED_FRAME_MAP) {
		switch (format) {
		case FORMAT_420:
		case FORMAT_422:
		case FORMAT_YUYV:
		case FORMAT_YVYU:
		case FORMAT_UYVY:
		case FORMAT_VYUY:
			break;
		case FORMAT_420_P10_16BIT_LSB:
		case FORMAT_420_P10_16BIT_MSB:
		case FORMAT_420_P10_32BIT_LSB:
		case FORMAT_420_P10_32BIT_MSB:
		case FORMAT_422_P10_16BIT_MSB:
		case FORMAT_422_P10_16BIT_LSB:
		case FORMAT_422_P10_32BIT_MSB:
		case FORMAT_422_P10_32BIT_LSB:
		case FORMAT_YUYV_P10_16BIT_MSB:
		case FORMAT_YUYV_P10_16BIT_LSB:
		case FORMAT_YVYU_P10_16BIT_MSB:
		case FORMAT_YVYU_P10_16BIT_LSB:
		case FORMAT_YVYU_P10_32BIT_MSB:
		case FORMAT_YVYU_P10_32BIT_LSB:
		case FORMAT_UYVY_P10_16BIT_MSB:
		case FORMAT_UYVY_P10_16BIT_LSB:
		case FORMAT_VYUY_P10_16BIT_MSB:
		case FORMAT_VYUY_P10_16BIT_LSB:
		case FORMAT_YUYV_P10_32BIT_MSB:
		case FORMAT_YUYV_P10_32BIT_LSB:
		case FORMAT_UYVY_P10_32BIT_MSB:
		case FORMAT_UYVY_P10_32BIT_LSB:
		case FORMAT_VYUY_P10_32BIT_MSB:
		case FORMAT_VYUY_P10_32BIT_LSB:
			lumaStride = VPU_ALIGN32(VPU_ALIGN16(width) * 5) >> 2;
			lumaStride = VPU_ALIGN32(lumaStride);
			break;
		default:
			return -1;
		}
	} else if (mapType == TILED_FRAME_NO_BANK_MAP ||
		   mapType == TILED_FIELD_NO_BANK_MAP) {
		lumaStride = (width > 4096) ? 8192 :
			     (width > 2048) ? 4096 :
			     (width > 1024) ? 2048 :
			     (width > 512)  ? 1024 :
						    512;
	} else if (mapType == TILED_FRAME_MB_RASTER_MAP ||
		   mapType == TILED_FIELD_MB_RASTER_MAP) {
		lumaStride = VPU_ALIGN32(width);
	} else {
		width = (width < height) ? height : width;

		lumaStride = (width > 4096) ? 8192 :
			     (width > 2048) ? 4096 :
			     (width > 1024) ? 2048 :
			     (width > 512)  ? 1024 :
						    512;
	}

#ifdef ARCH_CV183X
	UNREFERENCED_PARAMETER(isDec);
#else
	if ((format == FORMAT_420) && (isDec == TRUE)) {
		if (cbcrInterleave == TRUE) {
			lumaStride = VPU_ALIGN64(lumaStride);
		} else {
			lumaStride = VPU_ALIGN128(lumaStride);
		}
	}
#endif

	return lumaStride;
}

int LevelCalculation(int MbNumX, int MbNumY, int frameRateInfo,
		     int interlaceFlag, int BitRate, int SliceNum)
{
	int mbps;
	int frameRateDiv, frameRateRes, frameRate;
	int mbPicNum = (MbNumX * MbNumY);
	int mbFrmNum;
	int MaxSliceNum;

	int LevelIdc = 0;
	int i, maxMbs;

	if (interlaceFlag) {
		mbFrmNum = mbPicNum * 2;
		MbNumY *= 2;
	} else
		mbFrmNum = mbPicNum;

	frameRateDiv = (frameRateInfo >> 16) + 1;
	frameRateRes = frameRateInfo & 0xFFFF;
	frameRate = math_div(frameRateRes, frameRateDiv);
	mbps = mbFrmNum * frameRate;

	for (i = 0; i < MAX_LAVEL_IDX; i++) {
		maxMbs = g_anLevelMaxMbs[i];
		if (mbps <= g_anLevelMaxMBPS[i] &&
		    mbFrmNum <= g_anLevelMaxFS[i] && MbNumX <= maxMbs &&
		    MbNumY <= maxMbs && BitRate <= g_anLevelMaxBR[i]) {
			LevelIdc = g_anLevel[i];
			break;
		}
	}

	if (i == MAX_LAVEL_IDX)
		i = MAX_LAVEL_IDX - 1;

	if (SliceNum) {
		SliceNum = math_div(mbPicNum, SliceNum);

		if (g_anLevelSliceRate[i]) {
			MaxSliceNum = math_div(
				MAX(mbPicNum,
				    g_anLevelMaxMBPS[i] /
					    (172 / (1 + interlaceFlag))),
				g_anLevelSliceRate[i]);

			if (SliceNum > MaxSliceNum)
				return -1;
		}
	}

	return LevelIdc;
}

Int32 CalcLumaSize(Int32 productId, Int32 stride, Int32 height, BOOL cbcrIntl,
		   TiledMapType mapType, DRAMConfig *pDramCfg)
{
	Int32 unit_size_hor_lum, unit_size_ver_lum, size_dpb_lum, field_map,
		size_dpb_lum_4k;
	UNREFERENCED_PARAMETER(cbcrIntl);
#ifndef REDUNDENT_CODE
	UNREFERENCED_PARAM(productId);
	UNREFERENCED_PARAM(pDramCfg);
#endif

	if (mapType == TILED_FIELD_V_MAP ||
	    mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
		field_map = 1;
	} else {
		field_map = 0;
	}

	if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
		size_dpb_lum = stride * height;
	} else if (mapType == COMPRESSED_FRAME_MAP) {
		size_dpb_lum = stride * height;
	} else if (mapType == TILED_FRAME_NO_BANK_MAP ||
		   mapType == TILED_FIELD_NO_BANK_MAP) {
		unit_size_hor_lum = stride;
		unit_size_ver_lum = ((((height >> field_map) + 127) >> 7) << 7);
		// unit vertical size is 128 pixel
		// (8MB)
		size_dpb_lum =
			unit_size_hor_lum * (unit_size_ver_lum << field_map);
	} else if (mapType == TILED_FRAME_MB_RASTER_MAP ||
		   mapType == TILED_FIELD_MB_RASTER_MAP) {
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			size_dpb_lum = stride * height;

			// aligned to 8192*2 (0x4000) for top/bot field
			// use upper 20bit address only
			size_dpb_lum_4k = ((size_dpb_lum + 16383) >> 14) << 14;

			if (mapType == TILED_FIELD_MB_RASTER_MAP) {
				size_dpb_lum_4k =
					((size_dpb_lum_4k + (0x8000 - 1)) &
					 ~(0x8000 - 1));
			}

			size_dpb_lum = size_dpb_lum_4k;
		} else
#endif
		{
			size_dpb_lum = stride * (height >> field_map);
			size_dpb_lum_4k = ((size_dpb_lum + (16384 - 1)) >> 14)
					  << 14;
			size_dpb_lum = size_dpb_lum_4k << field_map;
		}
	} else {
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			Int32 VerSizePerRas, Ras1DBit;
			Int32 ChrSizeYField;
			Int32 ChrFieldRasSize, ChrFrameRasSize, LumFieldRasSize,
				LumFrameRasSize;

			ChrSizeYField = ((height >> 1) + 1) >> 1;

			if (pDramCfg == NULL)
				return 0;
			if (mapType == TILED_FRAME_V_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 16) { // BITMAIN
					// setting
					VerSizePerRas = 64; // Tile (16x2)*(4*2)
					Ras1DBit = 1;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 4 &&
					   pDramCfg->rasBit == 15) { // BITMAIN
					// setting
					VerSizePerRas = 128; // Tile (8x4)*(8x2)
					Ras1DBit = 1;
				} else
					return 0;

			} else if (mapType == TILED_FRAME_H_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else
					return 0;

			} else if (mapType == TILED_FIELD_V_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 16) { // BITMAIN
					// setting
					VerSizePerRas = 64; // Tile (16x2)*(4*2)
					Ras1DBit = 1;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 4 &&
					   pDramCfg->rasBit == 15) { // BITMAIN
					// setting
					VerSizePerRas = 128; // Tile (8x4)*(8x2)
					Ras1DBit = 1;
				} else
					return 0;
			} else { // TILED_FIELD_H_MAP
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else
					return 0;
			}
			ChrFieldRasSize =
				((ChrSizeYField + (VerSizePerRas - 1)) /
				 VerSizePerRas)
				<< Ras1DBit;
			ChrFrameRasSize = ChrFieldRasSize * 2;
			LumFieldRasSize = ChrFrameRasSize;
			LumFrameRasSize = LumFieldRasSize * 2;
			size_dpb_lum = LumFrameRasSize
				       << (pDramCfg->bankBit +
					   pDramCfg->casBit + pDramCfg->busBit);
		} else
#endif
		{ // productId != 960
			unit_size_hor_lum = stride;
			unit_size_ver_lum = (((height >> field_map) + 63) >> 6)
					    << 6;
			// unit vertical size is 64 pixel (4MB)
			size_dpb_lum = unit_size_hor_lum *
				       (unit_size_ver_lum << field_map);
		}
	}

	if (vdi_get_chip_version() == CHIP_ID_1821A ||
	    vdi_get_chip_version() == CHIP_ID_1822)
		return size_dpb_lum;

	// 4k-aligned for frame buffer addr (VPSS usage)
	return VPU_ALIGN4096(size_dpb_lum);
}

Int32 CalcChromaSize(Int32 productId, Int32 stride, Int32 height,
		     FrameBufferFormat format, BOOL cbcrIntl,
		     TiledMapType mapType, DRAMConfig *pDramCfg)
{
	Int32 chr_size_y, chr_size_x;
	Int32 chr_vscale, chr_hscale;
	Int32 unit_size_hor_chr, unit_size_ver_chr;
	Int32 size_dpb_chr, size_dpb_chr_4k;
	Int32 field_map;
#ifndef REDUNDENT_CODE
	UNREFERENCED_PARAM(productId);
	UNREFERENCED_PARAM(pDramCfg);
#endif

	unit_size_hor_chr = 0;
	unit_size_ver_chr = 0;

	chr_hscale = 1;
	chr_vscale = 1;

	switch (format) {
	case FORMAT_420_P10_16BIT_LSB:
	case FORMAT_420_P10_16BIT_MSB:
	case FORMAT_420_P10_32BIT_LSB:
	case FORMAT_420_P10_32BIT_MSB:
	case FORMAT_420:
		chr_hscale = 2;
		chr_vscale = 2;
		break;
	case FORMAT_224:
		chr_vscale = 2;
		break;
	case FORMAT_422:
	case FORMAT_422_P10_16BIT_LSB:
	case FORMAT_422_P10_16BIT_MSB:
	case FORMAT_422_P10_32BIT_LSB:
	case FORMAT_422_P10_32BIT_MSB:
		chr_hscale = 2;
		break;
	case FORMAT_444:
	case FORMAT_400:
	case FORMAT_YUYV:
	case FORMAT_YVYU:
	case FORMAT_UYVY:
	case FORMAT_VYUY:
	case FORMAT_YUYV_P10_16BIT_MSB: // 4:2:2 10bit packed
	case FORMAT_YUYV_P10_16BIT_LSB:
	case FORMAT_YVYU_P10_16BIT_MSB:
	case FORMAT_YVYU_P10_16BIT_LSB:
	case FORMAT_UYVY_P10_16BIT_MSB:
	case FORMAT_UYVY_P10_16BIT_LSB:
	case FORMAT_VYUY_P10_16BIT_MSB:
	case FORMAT_VYUY_P10_16BIT_LSB:
	case FORMAT_YUYV_P10_32BIT_MSB:
	case FORMAT_YUYV_P10_32BIT_LSB:
	case FORMAT_YVYU_P10_32BIT_MSB:
	case FORMAT_YVYU_P10_32BIT_LSB:
	case FORMAT_UYVY_P10_32BIT_MSB:
	case FORMAT_UYVY_P10_32BIT_LSB:
	case FORMAT_VYUY_P10_32BIT_MSB:
	case FORMAT_VYUY_P10_32BIT_LSB:
		break;
	default:
		return 0;
	}

	if (mapType == TILED_FIELD_V_MAP ||
	    mapType == TILED_FIELD_NO_BANK_MAP || mapType == LINEAR_FIELD_MAP) {
		field_map = 1;
	} else {
		field_map = 0;
	}

	if (mapType == LINEAR_FRAME_MAP || mapType == LINEAR_FIELD_MAP) {
		switch (format) {
		case FORMAT_420:
			unit_size_hor_chr = stride >> 1;
			unit_size_ver_chr = height >> 1;
			break;
		case FORMAT_420_P10_16BIT_LSB:
		case FORMAT_420_P10_16BIT_MSB:
			unit_size_hor_chr = stride >> 1;
			unit_size_ver_chr = height >> 1;
			break;
		case FORMAT_420_P10_32BIT_LSB:
		case FORMAT_420_P10_32BIT_MSB:
			unit_size_hor_chr = VPU_ALIGN16((stride >> 1));
			unit_size_ver_chr = height >> 1;
			break;
		case FORMAT_422:
		case FORMAT_422_P10_16BIT_LSB:
		case FORMAT_422_P10_16BIT_MSB:
		case FORMAT_422_P10_32BIT_LSB:
		case FORMAT_422_P10_32BIT_MSB:
			unit_size_hor_chr = VPU_ALIGN32((stride >> 1));
			unit_size_ver_chr = height;
			break;
		case FORMAT_YUYV:
		case FORMAT_YVYU:
		case FORMAT_UYVY:
		case FORMAT_VYUY:
		case FORMAT_YUYV_P10_16BIT_MSB: // 4:2:2 10bit packed
		case FORMAT_YUYV_P10_16BIT_LSB:
		case FORMAT_YVYU_P10_16BIT_MSB:
		case FORMAT_YVYU_P10_16BIT_LSB:
		case FORMAT_UYVY_P10_16BIT_MSB:
		case FORMAT_UYVY_P10_16BIT_LSB:
		case FORMAT_VYUY_P10_16BIT_MSB:
		case FORMAT_VYUY_P10_16BIT_LSB:
		case FORMAT_YUYV_P10_32BIT_MSB:
		case FORMAT_YUYV_P10_32BIT_LSB:
		case FORMAT_YVYU_P10_32BIT_MSB:
		case FORMAT_YVYU_P10_32BIT_LSB:
		case FORMAT_UYVY_P10_32BIT_MSB:
		case FORMAT_UYVY_P10_32BIT_LSB:
		case FORMAT_VYUY_P10_32BIT_MSB:
		case FORMAT_VYUY_P10_32BIT_LSB:
			unit_size_hor_chr = 0;
			unit_size_ver_chr = 0;
			break;
		default:
			break;
		}
		size_dpb_chr = (format == FORMAT_400) ?
					     0 :
					     unit_size_ver_chr * unit_size_hor_chr;
	} else if (mapType == COMPRESSED_FRAME_MAP) {
		switch (format) {
		case FORMAT_420:
		case FORMAT_YUYV: // 4:2:2 8bit packed
		case FORMAT_YVYU:
		case FORMAT_UYVY:
		case FORMAT_VYUY:
			size_dpb_chr = VPU_ALIGN16((stride >> 1)) * height;
			break;
		default:
			/* 10bit */
			stride = VPU_ALIGN64((stride >> 1)) + 12; /* FIXME: need
								  width
								  information */
			size_dpb_chr = VPU_ALIGN32(stride) * VPU_ALIGN4(height);
			break;
		}
		size_dpb_chr = size_dpb_chr >> 1;
	} else if (mapType == TILED_FRAME_NO_BANK_MAP ||
		   mapType == TILED_FIELD_NO_BANK_MAP) {
		chr_size_y = (height >> field_map) / chr_hscale;
		chr_size_x = stride / chr_vscale;

		unit_size_hor_chr = (chr_size_x > 4096) ? 8192 :
				    (chr_size_x > 2048) ? 4096 :
				    (chr_size_x > 1024) ? 2048 :
				    (chr_size_x > 512)	? 1024 :
								512;
		unit_size_ver_chr = ((chr_size_y + 127) >> 7)
				    << 7; // unit vertical size
		// is 128 pixel (8MB)

		size_dpb_chr = (format == FORMAT_400) ?
					     0 :
					     (unit_size_hor_chr *
					(unit_size_ver_chr << field_map));
	} else if (mapType == TILED_FRAME_MB_RASTER_MAP ||
		   mapType == TILED_FIELD_MB_RASTER_MAP) {
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			chr_size_x = stride / chr_hscale;
			chr_size_y = height / chr_hscale;
			size_dpb_chr = chr_size_y * chr_size_x;

			// aligned to 8192*2 (0x4000) for top/bot field
			// use upper 20bit address only
			size_dpb_chr_4k = ((size_dpb_chr + 16383) >> 14) << 14;

			if (mapType == TILED_FIELD_MB_RASTER_MAP) {
				size_dpb_chr_4k =
					((size_dpb_chr_4k + (0x8000 - 1)) &
					 ~(0x8000 - 1));
			}

			size_dpb_chr = size_dpb_chr_4k;
		} else
#endif
		{
			size_dpb_chr =
				(format == FORMAT_400) ?
					      0 :
					      ((stride * (height >> field_map)) /
					 (chr_hscale * chr_vscale));
			size_dpb_chr_4k = ((size_dpb_chr + (16384 - 1)) >> 14)
					  << 14;
			size_dpb_chr = size_dpb_chr_4k << field_map;
		}
	} else {
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			int VerSizePerRas, Ras1DBit;
			int ChrSizeYField;
			int divY;
			int ChrFieldRasSize, ChrFrameRasSize;

			divY = format == FORMAT_420 || format == FORMAT_224 ?
					     2 :
					     1;

			ChrSizeYField = ((height / divY) + 1) >> 1;
			if (pDramCfg == NULL)
				return 0;
			if (mapType == TILED_FRAME_V_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 16) { // BITMAIN
					// setting
					VerSizePerRas = 64; // Tile (16x2)*(4*2)
					Ras1DBit = 1;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 4 &&
					   pDramCfg->rasBit == 15) { // BITMAIN
					// setting
					VerSizePerRas = 128; // Tile (8x4)*(8x2)
					Ras1DBit = 1;
				} else
					return 0;

			} else if (mapType == TILED_FRAME_H_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else
					return 0;

			} else if (mapType == TILED_FIELD_V_MAP) {
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 16) { // BITMAIN
					// setting
					VerSizePerRas = 64; // Tile (16x2)*(4*2)
					Ras1DBit = 1;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 4 &&
					   pDramCfg->rasBit == 15) { // BITMAIN
					// setting
					VerSizePerRas = 128; // Tile (8x4)*(8x2)
					Ras1DBit = 1;
				} else
					return 0;
			} else { // TILED_FIELD_H_MAP
				if (pDramCfg->casBit == 9 &&
				    pDramCfg->bankBit == 2 &&
				    pDramCfg->rasBit == 13) { // CNN setting
					VerSizePerRas = 64;
					Ras1DBit = 3;
				} else if (pDramCfg->casBit == 10 &&
					   pDramCfg->bankBit == 3 &&
					   pDramCfg->rasBit == 13) {
					VerSizePerRas = 64;
					Ras1DBit = 2;
				} else
					return 0;
			}
			ChrFieldRasSize =
				((ChrSizeYField + (VerSizePerRas - 1)) /
				 VerSizePerRas)
				<< Ras1DBit;
			ChrFrameRasSize = ChrFieldRasSize * 2;
			size_dpb_chr = (ChrFrameRasSize << (pDramCfg->bankBit +
							    pDramCfg->casBit +
							    pDramCfg->busBit)) /
				       2; // divide 2  = to calucate one Cb(or
			// Cr) size;

		} else
#endif
		{ // productId != 960
			chr_size_y = (height >> field_map) / chr_hscale;
			chr_size_x =
				cbcrIntl == TRUE ? stride : stride / chr_vscale;

			unit_size_hor_chr = (chr_size_x > 4096) ? 8192 :
					    (chr_size_x > 2048) ? 4096 :
					    (chr_size_x > 1024) ? 2048 :
					    (chr_size_x > 512)	? 1024 :
									512;
			unit_size_ver_chr = ((chr_size_y + 63) >> 6)
					    << 6; // unit vertical
			// size is 64
			// pixel (4MB)

			size_dpb_chr =
				(format == FORMAT_400) ?
					      0 :
					      unit_size_hor_chr * (unit_size_ver_chr
							     << field_map);
			if (cbcrIntl == TRUE)
				size_dpb_chr >>= 1;
		}
	}

	if (vdi_get_chip_version() == CHIP_ID_1821A ||
	    vdi_get_chip_version() == CHIP_ID_1822)
		return size_dpb_chr;

	// 4k-aligned for frame buffer addr (VPSS usage)
	return VPU_ALIGN4096(size_dpb_chr);
}
