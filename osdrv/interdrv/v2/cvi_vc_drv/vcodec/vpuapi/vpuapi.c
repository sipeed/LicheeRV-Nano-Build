//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "vpuapifunc.h"
#include "product.h"
#include "main_helper.h"
#include "wave/common/common_regdefine.h"

#ifdef BIT_CODE_FILE_PATH
#include BIT_CODE_FILE_PATH
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]))
#endif

static const Uint16 *s_pusBitCode[MAX_NUM_VPU_CORE] = {
	NULL,
};
static int s_bitCodeSize[MAX_NUM_VPU_CORE] = {
	0,
};

static int s_coreStatus[MAX_NUM_VPU_CORE] = {
	0,
};

Uint32 __VPU_BUSY_TIMEOUT = VPU_BUSY_CHECK_TIMEOUT;

static RetCode cviConfigEncParam(CodecInst *pCodec, EncOpenParam *param);
static RetCode CheckInstanceValidity(CodecInst *pCodecInst)
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

	return RETCODE_INVALID_HANDLE;
}

static RetCode CheckDecInstanceValidity(CodecInst *pCodecInst)
{
	RetCode ret;

	if (pCodecInst == NULL)
		return RETCODE_INVALID_HANDLE;

	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}

	return ProductVpuDecCheckCapability(pCodecInst);
}

Int32 VPU_IsBusy(Uint32 coreIdx)
{
	Uint32 ret = 0;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	SetClockGate(coreIdx, 1);
	ret = ProductVpuIsBusy(coreIdx);
	SetClockGate(coreIdx, 0);

	return ret != 0;
}

Int32 VPU_IsInit(Uint32 coreIdx)
{
	Int32 pc;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	SetClockGate(coreIdx, 1);
	pc = ProductVpuIsInit(coreIdx);
	SetClockGate(coreIdx, 0);

	return pc;
}

Int32 VPU_WaitInterrupt(Uint32 coreIdx, int timeout)
{
	Int32 ret;
	CodecInst *instance;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	instance = GetPendingInst(coreIdx);
	if (instance != NULL) {
		ret = ProductVpuWaitInterrupt(instance, timeout);
	} else {
		ret = -1;
	}

	return ret;
}

#if 0
Int32 VPU_GetFd(Uint32 coreIdx)
{
	Int32 fd = -1;
	CodecInst *instance;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	instance = GetPendingInst(coreIdx);
	if (instance != NULL) {
		fd = ProductVpuGetFd(instance);
	}

	if (fd <= 0) {
		CVI_VC_TRACE("fd = %d\n", fd);
		return -1;
	}

	return fd;
}
#endif

Int32 VPU_GetFrameCycle(Uint32 coreIdx)
{
	return ProductVpuGetFrameCycle(coreIdx);
}

#ifdef REDUNDENT_CODE
Int32 VPU_WaitInterruptEx(VpuHandle handle, int timeout)
{
	Int32 ret;
	CodecInst *pCodecInst;

	pCodecInst = handle;

	if (pCodecInst->coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	ret = ProductVpuWaitInterrupt(pCodecInst, timeout);

	return ret;
}
#endif

void VPU_ClearInterrupt(Uint32 coreIdx)
{
	/* clear all interrupt flags */
	ProductVpuClearInterrupt(coreIdx);
}

#ifdef REDUNDENT_CODE
void VPU_ClearInterruptEx(VpuHandle handle, Int32 intrFlag)
{
	CodecInst *pCodecInst;

	UNREFERENCED_PARAMETER(intrFlag);

	pCodecInst = handle;

	ProductVpuClearInterrupt(pCodecInst->coreIdx);
}

int VPU_GetMvColBufSize(CodStd codStd, int width, int height, int num)
{
	int size_mvcolbuf = ProductCalculateAuxBufferSize(
		AUX_BUF_TYPE_MVCOL, codStd, width, height);

	if (codStd == STD_AVC || codStd == STD_HEVC || codStd == STD_VP9)
		size_mvcolbuf *= num;

	return size_mvcolbuf;
}
#endif

RetCode VPU_GetFBCOffsetTableSize(CodStd codStd, int width, int height,
				  int *ysize, int *csize)
{
	if (ysize == NULL || csize == NULL)
		return RETCODE_INVALID_PARAM;

	*ysize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_Y_OFFSET,
					       codStd, width, height);
	*csize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_C_OFFSET,
					       codStd, width, height);

	return RETCODE_SUCCESS;
}

int VPU_GetFrameBufSize(int coreIdx, int stride, int height, int mapType,
			int format, int interleave, DRAMConfig *pDramCfg)
{
	int productId;
	UNREFERENCED_PARAMETER(interleave); /*!<< for backward compatiblity */

	if (coreIdx < 0 || coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	productId = ProductVpuGetId(coreIdx);

	return ProductCalculateFrameBufSize(productId, stride, height,
					    (TiledMapType)mapType,
					    (FrameBufferFormat)format,
					    (BOOL)interleave, pDramCfg);
}

int VPU_GetProductId(int coreIdx)
{
	Int32 productId;

	CVI_VC_TRACE("IN\n");

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	productId = ProductVpuGetId(coreIdx);
	if (productId != PRODUCT_ID_NONE) {
		CVI_VC_INFO("Already got product id\n");
		return productId;
	}

	if (vdi_init(coreIdx, FALSE) < 0) {
		CVI_VC_ERR("vdi_init error\n");
		return -1;
	}

	if (ProductVpuScan(coreIdx) == FALSE)
		productId = -1;
	else
		productId = ProductVpuGetId(coreIdx);

	//	vdi_release(coreIdx); // vdi_release will be call after all jobs are done

	ProductVpuSetId(coreIdx, productId);

	return productId;
}

#ifdef REDUNDENT_CODE
int VPU_GetOpenInstanceNum(Uint32 coreIdx)
{
	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	return vdi_get_instance_num(coreIdx);
}
#endif

#if 0
Int32 cviVPU_GetFd(Uint32 coreIdx)
{
	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	return vdi_get_vpu_fd(coreIdx);
}
#endif

static RetCode InitializeVPU(Uint32 coreIdx, const Uint16 *code, Uint32 size)
{
	RetCode ret;

	if (vdi_init(coreIdx, TRUE) < 0)
		return RETCODE_FAILURE;

	if (vdi_get_instance_num(coreIdx) > 0) {
		if (ProductVpuScan(coreIdx) == 0) {
			return RETCODE_NOT_FOUND_VPU_DEVICE;
		}
	}

	if (VPU_IsInit(coreIdx) != 0) {
		int SingleCore;

		SetClockGate(coreIdx, 1);

		SingleCore = vdi_get_single_core(coreIdx);

		if (SingleCore == 1) {
			ProductVpuInit(coreIdx, (void *)code, size);
		} else if (SingleCore == 0) {
#ifdef ARCH_CV183X
			ProductVpuReInit(coreIdx, (void *)code, size);
#else
			if (vdi_get_instance_num(coreIdx) > 0) {
				ProductVpuReInit(coreIdx, (void *)code, size);
			} else {
				ProductVpuInit(coreIdx, (void *)code, size);
			}
#endif
		} else {
			CVI_VC_ERR("get_single_core fail\n");
			return RETCODE_FAILURE;
		}
		return RETCODE_CALLED_BEFORE;
	}

	InitCodecInstancePool(coreIdx);

	SetClockGate(coreIdx, 1);
	ret = ProductVpuReset(coreIdx, SW_RESET_ON_BOOT);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	ret = ProductVpuInit(coreIdx, (void *)code, size);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ProductVpuInit err %d\n", ret);
		return ret;
	}
	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
RetCode VPU_Init(Uint32 coreIdx)
{
#ifdef BIT_CODE_FILE_PATH
	s_pusBitCode[coreIdx] = bit_code;
	s_bitCodeSize[coreIdx] = ARRAY_SIZE(bit_code);
#endif

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	if (s_bitCodeSize[coreIdx] == 0)
		return RETCODE_NOT_FOUND_BITCODE_PATH;

	return InitializeVPU(coreIdx, s_pusBitCode[coreIdx],
			     s_bitCodeSize[coreIdx]);
}
#endif

RetCode VPU_InitWithBitcode(Uint32 coreIdx, const Uint16 *code, Uint32 size)
{
	if (coreIdx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}
	if (code == NULL || size == 0)
		return RETCODE_INVALID_PARAM;

// load fw twice due to initMcuEnv done it before
#if 0
	if (s_pusBitCode[coreIdx] == NULL) {
		s_pusBitCode[coreIdx] = (Uint16 *)osal_malloc(size * sizeof(Uint16));
		if (!s_pusBitCode[coreIdx])
			return RETCODE_INSUFFICIENT_RESOURCE;

		CVI_VC_MCU("s_pusBitCode[%d] = %p\n", coreIdx, s_pusBitCode[coreIdx]);

		osal_memcpy((void *)s_pusBitCode[coreIdx], (const void *)code,
			    size * sizeof(Uint16));
		s_bitCodeSize[coreIdx] = size;
#else
	if (s_pusBitCode[coreIdx] == NULL) {
#ifdef FIRMWARE_H
		s_pusBitCode[coreIdx] = (Uint16 *)code;
#endif
		s_bitCodeSize[coreIdx] = size;

		CVI_VC_MCU("s_pusBitCode[%d] = %p\n", coreIdx,
			   s_pusBitCode[coreIdx]);
	}
#endif

	return InitializeVPU(coreIdx, code, size);
}

RetCode VPU_DeInit(Uint32 coreIdx)
{
	int ret = 0, num = 0;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

		//#define WORK_AROUND_FREE

#ifdef BIT_CODE_FILE_PATH
#else
#ifndef WORK_AROUND_FREE
#if 0
	if (s_pusBitCode[coreIdx]) {
		osal_free((void *)s_pusBitCode[coreIdx]);
	}
#endif
#endif
#endif
#if 0
	s_pusBitCode[coreIdx] = NULL;
	s_bitCodeSize[coreIdx] = 0;
#endif
	ret = vdi_release(coreIdx);
	if (ret != 0)
		return RETCODE_FAILURE;

#if 1
	num = VPU_GetCoreTaskNum(coreIdx);
	if (num == 0) {
		s_pusBitCode[coreIdx] = NULL;
		s_bitCodeSize[coreIdx] = 0;
	}
#endif

	return RETCODE_SUCCESS;
}

RetCode VPU_GetVersionInfo(Uint32 coreIdx, Uint32 *versionInfo,
			   Uint32 *revision, Uint32 *productId)
{
	RetCode ret;

	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	if (ProductVpuIsInit(coreIdx) == 0) {
		return RETCODE_NOT_INITIALIZED;
	}

	if (GetPendingInst(coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	if (productId != NULL) {
		*productId = ProductVpuGetId(coreIdx);
	}
	ret = ProductVpuGetVersion(coreIdx, versionInfo, revision);

	return ret;
}
RetCode VPU_DecOpen(DecHandle *pHandle, DecOpenParam *pop)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;

	ret = ProductCheckDecOpenParam(pop);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (VPU_IsInit(pop->coreIdx) == 0) {
		return RETCODE_NOT_INITIALIZED;
	}

	ret = GetCodecInstance(pop->coreIdx, &pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		*pHandle = 0;
		return ret;
	}

	pCodecInst->isDecoder = TRUE;
	*pHandle = pCodecInst;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	osal_memset(pDecInfo, 0x00, sizeof(DecInfo));
	osal_memcpy((void *)&pDecInfo->openParam, pop, sizeof(DecOpenParam));

#ifdef REDUNDENT_CODE
	if (pop->bitstreamFormat == STD_MPEG4) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_MP4_DEC :
									MP4_DEC);
		pCodecInst->codecModeAux = MP4_AUX_MPEG4;
	} else if (pop->bitstreamFormat == STD_AVC) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_AVC_DEC :
									AVC_DEC);
		pCodecInst->codecModeAux = pop->avcExtension;
	} else if (pop->bitstreamFormat == STD_VC1) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_VC1_DEC :
									VC1_DEC);
	} else if (pop->bitstreamFormat == STD_MPEG2) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_MP2_DEC :
									MP2_DEC);
	} else if (pop->bitstreamFormat == STD_H263) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_MP4_DEC :
									MP4_DEC);
		pCodecInst->codecModeAux = MP4_AUX_MPEG4;
	} else if (pop->bitstreamFormat == STD_DIV3) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_DV3_DEC :
									DV3_DEC);
		pCodecInst->codecModeAux = MP4_AUX_DIVX3;
		pDecInfo->div3Width = pop->div3Width;
		pDecInfo->div3Height = pop->div3Height;
	} else if (pop->bitstreamFormat == STD_RV) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_RVX_DEC :
									RV_DEC);
	} else if (pop->bitstreamFormat == STD_AVS) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_AVS_DEC :
									AVS_DEC);
	} else if (pop->bitstreamFormat == STD_THO) {
		pCodecInst->codecMode = VPX_DEC;
		pCodecInst->codecModeAux = VPX_AUX_THO;
	} else if (pop->bitstreamFormat == STD_VP3) {
		pCodecInst->codecMode = VPX_DEC;
		pCodecInst->codecModeAux = VPX_AUX_THO;
	} else if (pop->bitstreamFormat == STD_VP8) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_VP8_DEC :
									VPX_DEC);
		pCodecInst->codecModeAux = VPX_AUX_VP8;
	} else if (pop->bitstreamFormat == STD_HEVC) {
		pCodecInst->codecMode = HEVC_DEC;
	} else if (pop->bitstreamFormat == STD_VP9) {
		pCodecInst->codecMode = C7_VP9_DEC;
	} else if (pop->bitstreamFormat == STD_AVS2) {
		pCodecInst->codecMode = C7_AVS2_DEC;
	} else {
		return RETCODE_INVALID_PARAM;
	}
#else
	pCodecInst->s32ChnNum = pop->s32ChnNum;
	if (pop->bitstreamFormat == STD_AVC) {
		pCodecInst->codecMode =
			(pCodecInst->productId == PRODUCT_ID_7Q ? C7_AVC_DEC :
									AVC_DEC);
		pCodecInst->codecModeAux = pop->avcExtension;
	} else if (pop->bitstreamFormat == STD_HEVC) {
		pCodecInst->codecMode = HEVC_DEC;
	} else {
		return RETCODE_INVALID_PARAM;
	}
#endif

	pDecInfo->wtlEnable = pop->wtlEnable;
	pDecInfo->wtlMode = pop->wtlMode;
	if (!pDecInfo->wtlEnable)
		pDecInfo->wtlMode = 0;

	pDecInfo->streamWrPtr = pop->bitstreamBuffer;
	pDecInfo->streamRdPtr = pop->bitstreamBuffer;
	pDecInfo->frameDelay = -1;
	pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
	pDecInfo->streamBufSize = pop->bitstreamBufferSize;
	pDecInfo->streamBufEndAddr =
		pop->bitstreamBuffer + pop->bitstreamBufferSize;
	pDecInfo->reorderEnable = pop->reorderEnable;
	pDecInfo->mirrorDirection = MIRDIR_NONE;
	pDecInfo->prevFrameEndPos = pop->bitstreamBuffer;

	SetClockGate(pop->coreIdx, TRUE);
	ret = ProductVpuDecBuildUpOpenParam(pCodecInst, pop);
	if (ret != RETCODE_SUCCESS) {
		SetClockGate(pop->coreIdx, FALSE);
		*pHandle = 0;
		return ret;
	}
	SetClockGate(pop->coreIdx, FALSE);

	pDecInfo->tiled2LinearEnable = pop->tiled2LinearEnable;
	pDecInfo->tiled2LinearMode = pop->tiled2LinearMode;
	if (!pDecInfo->tiled2LinearEnable)
		pDecInfo->tiled2LinearMode = 0; // coda980 only

	if (!pDecInfo->wtlEnable) // coda980, wave320, wave410 only
		pDecInfo->wtlMode = 0;

	osal_memset((void *)&pDecInfo->cacheConfig, 0x00,
		    sizeof(MaverickCacheConfig));

	return RETCODE_SUCCESS;
}

RetCode VPU_DecClose(DecHandle handle)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
	int i;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	ret = ProductVpuDecFiniSeq(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, DEC_SEQ_END, 0);
#endif

		if (ret == RETCODE_VPU_STILL_RUNNING) {
			return ret;
		}
	}

	if (pDecInfo->vbSlice.size)
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pDecInfo->vbSlice);

	if (pDecInfo->vbWork.size) {
		if (pDecInfo->workBufferAllocExt == 0) {
			VDI_FREE_MEMORY(pCodecInst->coreIdx, &pDecInfo->vbWork);
		} else {
			vdi_dettach_dma_memory(pCodecInst->coreIdx,
					       &pDecInfo->vbWork);
		}
	}

	if (pDecInfo->vbFrame.size) {
		if (pDecInfo->frameAllocExt == 0) {
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFrame);
		}
	}
	for (i = 0; i < MAX_REG_FRAME; i++) {
		if (pDecInfo->vbMV[i].size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbMV[i]);
		if (pDecInfo->vbFbcYTbl[i].size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcYTbl[i]);
		if (pDecInfo->vbFbcCTbl[i].size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcCTbl[i]);
	}

	if (pDecInfo->vbTemp.size)
		vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbTemp);

	if (pDecInfo->vbPPU.size) {
		if (pDecInfo->ppuAllocExt == 0)
			VDI_FREE_MEMORY(pCodecInst->coreIdx, &pDecInfo->vbPPU);
	}

	if (pDecInfo->vbWTL.size)
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pDecInfo->vbWTL);

	if (pDecInfo->vbUserData.size)
		vdi_dettach_dma_memory(pCodecInst->coreIdx,
				       &pDecInfo->vbUserData);

	if (pDecInfo->vbReport.size)
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pDecInfo->vbReport);

	if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
		ClearPendingInst(pCodecInst->coreIdx);

	FreeCodecInstance(pCodecInst);

	return ret;
}

RetCode VPU_DecSetEscSeqInit(DecHandle handle, int escape)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	if (pDecInfo->openParam.bitstreamMode != BS_MODE_INTERRUPT)
		return RETCODE_INVALID_PARAM;

	pDecInfo->seqInitEscape = escape;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGetInitialInfo(DecHandle handle, DecInitialInfo *info)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
	Int32 flags;
	Uint32 interruptBit;

	if (PRODUCT_ID_W_SERIES(handle->productId)) {
		if (handle->productId == PRODUCT_ID_510 ||
		    handle->productId == PRODUCT_ID_512) {
			interruptBit = INT_WAVE5_INIT_SEQ;
		} else {
			interruptBit = INT_WAVE_DEC_PIC_HDR;
		}
	} else {
		/* CODA9xx */
		interruptBit = INT_BIT_SEQ_INIT;
	}

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (info == NULL)
		return RETCODE_INVALID_PARAM;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	ret = ProductVpuDecCheckCapability(pCodecInst);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (GetPendingInst(pCodecInst->coreIdx)) {
		/* The other instance is running */
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	if (DecBitstreamBufEmpty(pDecInfo)) {
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	ret = ProductVpuDecInitSeq(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	flags = ProductVpuWaitInterrupt(pCodecInst, __VPU_BUSY_TIMEOUT);

	if (flags == -1) {
		info->rdPtr = VpuReadReg(pCodecInst->coreIdx,
					 pDecInfo->streamRdPtrRegAddr);
		info->wrPtr = VpuReadReg(pCodecInst->coreIdx,
					 pDecInfo->streamWrPtrRegAddr);
		ret = RETCODE_VPU_RESPONSE_TIMEOUT;
	} else {
		if (flags & (1 << interruptBit))
			ProductVpuClearInterrupt(pCodecInst->coreIdx);

		if (flags != (1 << interruptBit))
			ret = RETCODE_FAILURE;
		else
			ret = ProductVpuDecGetSeqInfo(handle, info);
	}

	info->rdPtr =
		VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr);
	info->wrPtr =
		VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamWrPtrRegAddr);

	pDecInfo->initialInfo = *info;
	if (ret == RETCODE_SUCCESS) {
		pDecInfo->initialInfoObtained = 1;
	}

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return ret;
}

RetCode VPU_DecIssueSeqInit(DecHandle handle)
{
	CodecInst *pCodecInst;
	RetCode ret;
	VpuAttr *pAttr;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (GetPendingInst(pCodecInst->coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	CVI_VC_BS("\n");
	ret = ProductVpuDecInitSeq(handle);
	if (ret == RETCODE_SUCCESS) {
		SetPendingInst(pCodecInst->coreIdx, pCodecInst, __func__,
			       __LINE__);
	}

	if (pAttr->supportCommandQueue == TRUE) {
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
	}

	return ret;
}

RetCode VPU_DecCompleteSeqInit(DecHandle handle, DecInitialInfo *info)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
	VpuAttr *pAttr;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (pAttr->supportCommandQueue == FALSE) {
		if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
			SetPendingInst(pCodecInst->coreIdx, NULL, __func__,
				       __LINE__);
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
	}

	ret = ProductVpuDecGetSeqInfo(handle, info);
	if (ret == RETCODE_SUCCESS) {
		pDecInfo->initialInfoObtained = 1;
	}

	info->rdPtr =
		VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr);
	info->wrPtr =
		VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamWrPtrRegAddr);
	CVI_VC_BS("rdPtr = 0x%llX, wrPtr = 0x%llX\n", info->rdPtr, info->wrPtr);

	pDecInfo->prevFrameEndPos = info->rdPtr;

	pDecInfo->initialInfo = *info;

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return ret;
}

static RetCode DecRegisterFrameBuffer(DecHandle handle, FrameBuffer *bufArray,
				      int numFbsForDecoding, int numFbsForWTL,
				      int stride, int height, int mapType)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	Int32 i;
	Uint32 size, totalAllocSize;
	RetCode ret;
	FrameBuffer *fb, nullFb;
	vpu_buffer_t *vb;
	FrameBufferFormat format = FORMAT_420;
	Int32 totalNumOfFbs;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (numFbsForDecoding > MAX_FRAMEBUFFER_COUNT ||
	    numFbsForWTL > MAX_FRAMEBUFFER_COUNT) {
		return RETCODE_INVALID_PARAM;
	}

	osal_memset(&nullFb, 0x00, sizeof(FrameBuffer));
	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	pDecInfo->numFbsForDecoding = numFbsForDecoding;
	pDecInfo->numFbsForWTL = numFbsForWTL;
	pDecInfo->numFrameBuffers = numFbsForDecoding + numFbsForWTL;
	pDecInfo->stride = stride;
	if (pCodecInst->codecMode == VPX_DEC ||
	    pCodecInst->codecMode == C7_VP8_DEC)
		pDecInfo->frameBufferHeight = VPU_ALIGN64(height);
	else if (pCodecInst->codecMode == C7_VP9_DEC)
		pDecInfo->frameBufferHeight = VPU_ALIGN64(height);
	else
		pDecInfo->frameBufferHeight = height;
	pDecInfo->mapType = mapType;
	pDecInfo->mapCfg.productId = pCodecInst->productId;

	ret = ProductVpuDecCheckCapability(pCodecInst);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (!pDecInfo->initialInfoObtained)
		return RETCODE_WRONG_CALL_SEQUENCE;

	if ((stride < pDecInfo->initialInfo.picWidth) || (stride % 8 != 0) ||
	    (height < pDecInfo->initialInfo.picHeight)) {
		return RETCODE_INVALID_STRIDE;
	}

	if (GetPendingInst(pCodecInst->coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	/* clear frameBufPool */
	for (i = 0;
	     i < (int)(sizeof(pDecInfo->frameBufPool) / sizeof(FrameBuffer));
	     i++) {
		pDecInfo->frameBufPool[i] = nullFb;
	}

	/* LinearMap or TiledMap, compressed framebuffer inclusive. */
	if (pDecInfo->initialInfo.lumaBitdepth > 8 ||
	    pDecInfo->initialInfo.chromaBitdepth > 8)
		format = FORMAT_420_P10_16BIT_LSB;

	totalNumOfFbs = numFbsForDecoding + numFbsForWTL;
	if (bufArray) {
		for (i = 0; i < totalNumOfFbs; i++)
			pDecInfo->frameBufPool[i] = bufArray[i];
	} else {
		vb = &pDecInfo->vbFrame;
		fb = &pDecInfo->frameBufPool[0];
		ret = ProductVpuAllocateFramebuffer(
			(CodecInst *)handle, fb, (TiledMapType)mapType,
			numFbsForDecoding, stride, height, format,
			pDecInfo->openParam.cbcrInterleave,
			pDecInfo->openParam.nv21,
			pDecInfo->openParam.frameEndian, vb, 0, FB_TYPE_CODEC);
		if (ret != RETCODE_SUCCESS) {
			return ret;
		}
	}
	totalAllocSize = 0;
	if (pCodecInst->productId != PRODUCT_ID_960) {
		pDecInfo->mapCfg.tiledBaseAddr = pDecInfo->frameBufPool[0].bufY;
	}

	if (numFbsForDecoding == 1) {
		size = ProductCalculateFrameBufSize(
			handle->productId, stride, height,
			(TiledMapType)mapType, format,
			pDecInfo->openParam.cbcrInterleave, &pDecInfo->dramCfg);
	} else {
		size = pDecInfo->frameBufPool[1].bufY -
		       pDecInfo->frameBufPool[0].bufY;
	}
	size *= numFbsForDecoding;
	totalAllocSize += size;

	/* LinearMap */
	if (pDecInfo->wtlEnable == TRUE || numFbsForWTL != 0) {
		pDecInfo->stride = stride;
		if (bufArray) {
			format = pDecInfo->frameBufPool[0].format;
		} else {
			TiledMapType map;
			map = pDecInfo->wtlMode == FF_FRAME ? LINEAR_FRAME_MAP :
								    LINEAR_FIELD_MAP;
			format = pDecInfo->wtlFormat;
			vb = &pDecInfo->vbWTL;
			fb = &pDecInfo->frameBufPool[numFbsForDecoding];

			ret = ProductVpuAllocateFramebuffer(
				(CodecInst *)handle, fb, map, numFbsForWTL,
				stride, height, pDecInfo->wtlFormat,
				pDecInfo->openParam.cbcrInterleave,
				pDecInfo->openParam.nv21,
				pDecInfo->openParam.frameEndian, vb, 0,
				FB_TYPE_PPU);

			if (ret != RETCODE_SUCCESS) {
				return ret;
			}
		}
		if (numFbsForWTL == 1) {
			size = ProductCalculateFrameBufSize(
				handle->productId, stride, height,
				(TiledMapType)mapType, format,
				pDecInfo->openParam.cbcrInterleave,
				&pDecInfo->dramCfg);
		} else {
			size = pDecInfo->frameBufPool[numFbsForDecoding + 1]
				       .bufY -
			       pDecInfo->frameBufPool[numFbsForDecoding].bufY;
		}
		size *= numFbsForWTL;
		totalAllocSize += size;
	}

	ret = ProductVpuRegisterFramebuffer(pCodecInst);

	return ret;
}

RetCode VPU_DecRegisterFrameBuffer(DecHandle handle, FrameBuffer *bufArray,
				   int num, int stride, int height, int mapType)
{
	DecInfo *pDecInfo = &handle->CodecInfo->decInfo;
	Uint32 numWTL = 0;

	if (pDecInfo->wtlEnable == TRUE)
		numWTL = num;
	return DecRegisterFrameBuffer(handle, bufArray, num, numWTL, stride,
				      height, mapType);
}

RetCode VPU_DecRegisterFrameBufferEx(DecHandle handle, FrameBuffer *bufArray,
				     int numOfDecFbs, int numOfDisplayFbs,
				     int stride, int height, int mapType)
{
	return DecRegisterFrameBuffer(handle, bufArray, numOfDecFbs,
				      numOfDisplayFbs, stride, height, mapType);
}

#ifdef REDUNDENT_CODE
RetCode VPU_DecGetFrameBuffer(DecHandle handle, int frameIdx,
			      FrameBuffer *frameBuf)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (frameBuf == 0)
		return RETCODE_INVALID_PARAM;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	if (frameIdx < 0 || frameIdx >= pDecInfo->numFrameBuffers)
		return RETCODE_INVALID_PARAM;

	*frameBuf = pDecInfo->frameBufPool[frameIdx];

	return RETCODE_SUCCESS;
}

RetCode VPU_DecUpdateFrameBuffer(DecHandle handle)
{
	if (handle == NULL) {
		return RETCODE_INVALID_HANDLE;
	}

	return ProductVpuDecUpdateFrameBuffer();
}

RetCode VPU_DecSetBitstreamBuffer(DecHandle handle, PhysicalAddress rdPtr,
				  PhysicalAddress wrPtr)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	SetClockGate(pCodecInst->coreIdx, 1);

	if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
		VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr,
			    rdPtr);
	else
		pDecInfo->streamRdPtr = rdPtr;

	SetClockGate(pCodecInst->coreIdx, 0);

	pDecInfo->streamWrPtr = wrPtr;

	return RETCODE_SUCCESS;
}
#endif

RetCode VPU_DecGetBitstreamBuffer(DecHandle handle, PhysicalAddress *prdPtr,
				  PhysicalAddress *pwrPtr, Uint32 *size)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	PhysicalAddress tempPtr;
	int room;
	Int32 coreIdx;
	VpuAttr *pAttr;

	coreIdx = handle->coreIdx;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	SetClockGate(coreIdx, TRUE);

	if (GetPendingInst(coreIdx) == pCodecInst) {
		if (pCodecInst->codecMode == AVC_DEC &&
		    pCodecInst->codecModeAux == AVC_AUX_MVC) {
			rdPtr = pDecInfo->streamRdPtr;
		} else {
			rdPtr = VpuReadReg(coreIdx,
					   pDecInfo->streamRdPtrRegAddr);
		}
	} else {
		rdPtr = pDecInfo->streamRdPtr;
	}

	SetClockGate(coreIdx, FALSE);

	wrPtr = pDecInfo->streamWrPtr;

	pAttr = &g_VpuCoreAttributes[coreIdx];
	if (pCodecInst->productId == PRODUCT_ID_4102 ||
	    pCodecInst->productId == PRODUCT_ID_420 ||
	    pCodecInst->productId == PRODUCT_ID_412 ||
	    pCodecInst->productId == PRODUCT_ID_420L ||
	    (pCodecInst->productId == PRODUCT_ID_7Q &&
	     pCodecInst->codecMode == C7_HEVC_DEC)) {
		if (pDecInfo->openParam.bitstreamMode != BS_MODE_PIC_END) {
			tempPtr = pDecInfo->prevFrameEndPos;
		} else {
			tempPtr = rdPtr;
		}
	} else {
		tempPtr = rdPtr;
	}

	if (pDecInfo->openParam.bitstreamMode != BS_MODE_PIC_END) {
		if (wrPtr < tempPtr) {
			room = tempPtr - wrPtr -
			       pAttr->bitstreamBufferMargin * 2;
		} else {
			room = (pDecInfo->streamBufEndAddr - wrPtr) +
			       (tempPtr - pDecInfo->streamBufStartAddr) -
			       pAttr->bitstreamBufferMargin * 2;
		}
		room--;
	} else {
		room = (pDecInfo->streamBufEndAddr - wrPtr);
	}

	if (prdPtr)
		*prdPtr = tempPtr;
	if (pwrPtr)
		*pwrPtr = wrPtr;
	if (size)
		*size = room;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecUpdateBitstreamBuffer(DecHandle handle, int size)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	PhysicalAddress wrPtr;
	PhysicalAddress rdPtr;
	RetCode ret;
	BOOL running;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	wrPtr = pDecInfo->streamWrPtr;

	SetClockGate(pCodecInst->coreIdx, 1);

	running = (BOOL)(GetPendingInst(pCodecInst->coreIdx) == pCodecInst);

	if (size > 0) {
		Uint32 room = 0;

		if (running == TRUE)
			rdPtr = VpuReadReg(pCodecInst->coreIdx,
					   pDecInfo->streamRdPtrRegAddr);
		else
			rdPtr = pDecInfo->streamRdPtr;

		if (wrPtr < rdPtr) {
			if (rdPtr <= wrPtr + size) {
				SetClockGate(pCodecInst->coreIdx, 0);
				return RETCODE_INVALID_PARAM;
			}
		}

		wrPtr += size;

		if (wrPtr > pDecInfo->streamBufEndAddr) {
			room = wrPtr - pDecInfo->streamBufEndAddr;
			wrPtr = pDecInfo->streamBufStartAddr;
			wrPtr += room;
		} else if (wrPtr == pDecInfo->streamBufEndAddr) {
			wrPtr = pDecInfo->streamBufStartAddr;
		}

		pDecInfo->streamWrPtr = wrPtr;
		pDecInfo->streamRdPtr = rdPtr;

		if (running == TRUE) {
			VpuAttr *pAttr =
				&g_VpuCoreAttributes[pCodecInst->coreIdx];
			if (pAttr->supportCommandQueue == FALSE) {
				VpuWriteReg(pCodecInst->coreIdx,
					    pDecInfo->streamWrPtrRegAddr,
					    wrPtr);
			}
		}
	}

	ret = ProductVpuDecSetBitstreamFlag(pCodecInst, running, size);

	SetClockGate(pCodecInst->coreIdx, 0);
	return ret;
}

#ifdef REDUNDENT_CODE
RetCode VPU_HWReset(Uint32 coreIdx)
{
	if (vdi_hw_reset(coreIdx) < 0)
		return RETCODE_FAILURE;

	if (GetPendingInst(coreIdx)) {
		SetPendingInst(coreIdx, NULL, __func__, __LINE__);
		// state;
	}
	return RETCODE_SUCCESS;
}
#endif

/**
 * VPU_SWReset
 * IN
 *    forcedReset : 1 if there is no need to waiting for BUS transaction,
 *                  0 for otherwise
 * OUT
 *    RetCode : RETCODE_FAILURE if failed to reset,
 *              RETCODE_SUCCESS for otherwise
 */
RetCode VPU_SWReset(Uint32 coreIdx, SWResetMode resetMode, void *pendingInst)
{
	CodecInst *pCodecInst = (CodecInst *)pendingInst;
	RetCode ret = RETCODE_SUCCESS;

	SetClockGate(coreIdx, 1);

	if (pCodecInst) {
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
		SetClockGate(coreIdx, 1);
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable) {
			vdi_log(pCodecInst->coreIdx,
				(pCodecInst->productId == PRODUCT_ID_960 ||
				 pCodecInst->productId == PRODUCT_ID_980) ?
					      0x10 :
					      0x10000,
				1);
		}
#endif
	}

	ret = ProductVpuReset(coreIdx, resetMode);

	if (pCodecInst) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable) {
			vdi_log(pCodecInst->coreIdx,
				(pCodecInst->productId == PRODUCT_ID_960 ||
				 pCodecInst->productId == PRODUCT_ID_980) ?
					      0x10 :
					      0x10000,
				0);
		}
#endif
	}

	SetClockGate(coreIdx, 0);

	return ret;
}

#ifdef REDUNDENT_CODE
//---- VPU_SLEEP/WAKE
RetCode VPU_SleepWake(Uint32 coreIdx, int iSleepWake)
{
	RetCode ret;

	SetClockGate(coreIdx, TRUE);
	ret = ProductVpuSleepWake(coreIdx, iSleepWake, s_pusBitCode[coreIdx],
				  s_bitCodeSize[coreIdx]);
	SetClockGate(coreIdx, FALSE);

	return ret;
}
#endif

//---- VPU_SLEEP/WAKE EX
RetCode VPU_SleepWake_EX(Uint32 coreIdx, int iSleepWake)
{
	RetCode ret;

	SetClockGate(coreIdx, TRUE);
	ret = ProductVpuSleepWake_EX(coreIdx, iSleepWake, s_pusBitCode[coreIdx],
				     s_bitCodeSize[coreIdx]);
	SetClockGate(coreIdx, FALSE);

	return ret;
}

RetCode VPU_DecStartOneFrame(DecHandle handle, DecParam *param)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	Uint32 val = 0;
	RetCode ret = RETCODE_SUCCESS;
	VpuAttr *pAttr = NULL;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	pCodecInst = (CodecInst *)handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	if (pDecInfo->stride == 0) { // This means frame buffers have not been
		// registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (GetPendingInst(pCodecInst->coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	if (pAttr->supportCommandQueue == FALSE) {
		EnterDispFlagLock(pCodecInst->coreIdx);
		val = pDecInfo->frameDisplayFlag;
		val |= pDecInfo->setDisplayIndexes;
		val &= ~(Uint32)(pDecInfo->clearDisplayIndexes);
		CVI_VC_DISP(
			"frmDispFlag = 0x%X, DispIdx = 0x%X, clearDisplayIdx = 0x%X\n",
			pDecInfo->frameDisplayFlag, pDecInfo->setDisplayIndexes,
			pDecInfo->clearDisplayIndexes);

		VpuWriteReg(pCodecInst->coreIdx,
			    pDecInfo->frameDisplayFlagRegAddr, val);
		pDecInfo->clearDisplayIndexes = 0;
		pDecInfo->setDisplayIndexes = 0;
		LeaveDispFlagLock(pCodecInst->coreIdx);
	}

	pDecInfo->frameStartPos = pDecInfo->streamRdPtr;

	ret = ProductVpuDecode(pCodecInst, param);

	if (pAttr->supportCommandQueue == TRUE) {
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
	} else {
		SetPendingInst(pCodecInst->coreIdx, pCodecInst, __func__,
			       __LINE__);
	}

	return ret;
}

RetCode VPU_DecGetOutputInfo(DecHandle handle, DecOutputInfo *info)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
	VpuRect rectInfo;
	Uint32 val;
	Int32 decodedIndex;
	Int32 displayIndex;
	Uint32 maxDecIndex;
	VpuAttr *pAttr;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (pAttr->supportCommandQueue == FALSE) {
		if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
			SetPendingInst(pCodecInst->coreIdx, NULL, __func__,
				       __LINE__);
			CVI_VC_ERR("SetPendingInst clear\n");
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
	}

	osal_memset((void *)info, 0x00, sizeof(DecOutputInfo));

	ret = ProductVpuDecGetResult(pCodecInst, info);
	if (ret != RETCODE_SUCCESS) {
		info->rdPtr = pDecInfo->streamRdPtr;
		info->wrPtr = pDecInfo->streamWrPtr;
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
		CVI_VC_ERR("SetPendingInst clear\n");
		return ret;
	}

	decodedIndex = info->indexFrameDecoded;

	// Calculate display frame region
	val = 0;
	if (decodedIndex >= 0 && decodedIndex < MAX_GDI_IDX) {
		// default value
		rectInfo.left = 0;
		rectInfo.right = info->decPicWidth;
		rectInfo.top = 0;
		rectInfo.bottom = info->decPicHeight;

		if (pCodecInst->codecMode == HEVC_DEC ||
		    pCodecInst->codecMode == AVC_DEC ||
		    pCodecInst->codecMode == C7_AVC_DEC ||
		    pCodecInst->codecMode == AVS_DEC)
			rectInfo = pDecInfo->initialInfo.picCropRect;

		if (pCodecInst->codecMode == HEVC_DEC)
			pDecInfo->decOutInfo[decodedIndex].h265Info.decodedPOC =
				info->h265Info.decodedPOC;

		info->rcDecoded.left =
			pDecInfo->decOutInfo[decodedIndex].rcDecoded.left =
				rectInfo.left;
		info->rcDecoded.right =
			pDecInfo->decOutInfo[decodedIndex].rcDecoded.right =
				rectInfo.right;
		info->rcDecoded.top =
			pDecInfo->decOutInfo[decodedIndex].rcDecoded.top =
				rectInfo.top;
		info->rcDecoded.bottom =
			pDecInfo->decOutInfo[decodedIndex].rcDecoded.bottom =
				rectInfo.bottom;
	} else {
		info->rcDecoded.left = 0;
		info->rcDecoded.right = info->decPicWidth;
		info->rcDecoded.top = 0;
		info->rcDecoded.bottom = info->decPicHeight;
	}

	displayIndex = info->indexFrameDisplay;
	if (info->indexFrameDisplay >= 0 &&
	    info->indexFrameDisplay < MAX_GDI_IDX) {
#ifdef REDUNDENT_CODE
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
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.top;
				info->rcDisplay.right =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.bottom;
				info->rcDisplay.top =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.left;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.right;
			} else {
				info->rcDisplay.left =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.left;
				info->rcDisplay.right =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.right;
				info->rcDisplay.top =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.top;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.bottom;
			}
		} else
#endif
		{
			if (pDecInfo->rotationEnable) {
				switch (pDecInfo->rotationAngle) {
				case 90:
					info->rcDisplay.left =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.top;
					info->rcDisplay.right =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.bottom;
					info->rcDisplay.top =
						info->decPicWidth -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.right;
					info->rcDisplay.bottom =
						info->decPicWidth -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.left;
					break;
				case 270:
					info->rcDisplay.left =
						info->decPicHeight -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.bottom;
					info->rcDisplay.right =
						info->decPicHeight -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.top;
					info->rcDisplay.top =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.left;
					info->rcDisplay.bottom =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.right;
					break;
				case 180:
					info->rcDisplay.left =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.left;
					info->rcDisplay.right =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.right;
					info->rcDisplay.top =
						info->decPicHeight -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.bottom;
					info->rcDisplay.bottom =
						info->decPicHeight -
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.top;
					break;
				default:
					info->rcDisplay.left =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.left;
					info->rcDisplay.right =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.right;
					info->rcDisplay.top =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.top;
					info->rcDisplay.bottom =
						pDecInfo->decOutInfo[displayIndex]
							.rcDecoded.bottom;
					break;
				}

			} else {
				info->rcDisplay.left =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.left;
				info->rcDisplay.right =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.right;
				info->rcDisplay.top =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.top;
				info->rcDisplay.bottom =
					pDecInfo->decOutInfo[displayIndex]
						.rcDecoded.bottom;
			}

			if (pDecInfo->mirrorEnable) {
				Uint32 temp;
				if (pDecInfo->mirrorDirection & MIRDIR_VER) {
					temp = info->rcDisplay.top;
					info->rcDisplay.top =
						info->decPicHeight -
						info->rcDisplay.bottom;
					info->rcDisplay.bottom =
						info->decPicHeight - temp;
				}
				if (pDecInfo->mirrorDirection & MIRDIR_HOR) {
					temp = info->rcDisplay.left;
					info->rcDisplay.left =
						info->decPicWidth -
						info->rcDisplay.right;
					info->rcDisplay.right =
						info->decPicWidth - temp;
				}
			}

			switch (pCodecInst->codecMode) {
			case HEVC_DEC:
				info->h265Info.displayPOC =
					pDecInfo->decOutInfo[displayIndex]
						.h265Info.decodedPOC;
				break;
			default:
				break;
			}
		}

		if (info->indexFrameDisplay == info->indexFrameDecoded) {
			info->dispPicWidth = info->decPicWidth;
			info->dispPicHeight = info->decPicHeight;
		} else {
			/*
		When indexFrameDecoded < 0, and indexFrameDisplay >= 0
		info->decPicWidth and info->decPicHeight are still valid
		But those of pDecInfo->decOutInfo[displayIndex] are invalid in
		VP9
	    */
			info->dispPicWidth =
				pDecInfo->decOutInfo[displayIndex].decPicWidth;
			info->dispPicHeight =
				pDecInfo->decOutInfo[displayIndex].decPicHeight;
		}

	} else {
		info->rcDisplay.left = 0;
		info->rcDisplay.right = 0;
		info->rcDisplay.top = 0;
		info->rcDisplay.bottom = 0;

		if (pDecInfo->rotationEnable || pDecInfo->mirrorEnable ||
		    pDecInfo->tiled2LinearEnable || pDecInfo->deringEnable) {
			info->dispPicWidth = info->decPicWidth;
			info->dispPicHeight = info->decPicHeight;
		} else {
			info->dispPicWidth = 0;
			info->dispPicHeight = 0;
		}
	}

#ifdef REDUNDENT_CODE
	if ((pCodecInst->codecMode == VC1_DEC ||
	     pCodecInst->codecMode == C7_VC1_DEC) &&
	    info->indexFrameDisplay != -3) {
		if (pDecInfo->vc1BframeDisplayValid == 0) {
			if (info->picType == 2)
				info->indexFrameDisplay = -3;
			else
				pDecInfo->vc1BframeDisplayValid = 1;
		}
	}
#endif

	pDecInfo->streamRdPtr =
		VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr);

	pDecInfo->frameDisplayFlag = VpuReadReg(
		pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == C7_VP9_DEC) {
		pDecInfo->frameDisplayFlag &= 0xFFFF;
	}
#endif
	CVI_VC_DISP("frameDisplayFlag = 0x%X\n", pDecInfo->frameDisplayFlag);

	pDecInfo->frameEndPos = pDecInfo->streamRdPtr;

	if (pDecInfo->frameEndPos < pDecInfo->frameStartPos)
		info->consumedByte = pDecInfo->frameEndPos +
				     pDecInfo->streamBufSize -
				     pDecInfo->frameStartPos;
	else
		info->consumedByte =
			pDecInfo->frameEndPos - pDecInfo->frameStartPos;

	if (pDecInfo->deringEnable || pDecInfo->mirrorEnable ||
	    pDecInfo->rotationEnable || pDecInfo->tiled2LinearEnable) {
		info->dispFrame = pDecInfo->rotatorOutput;
		info->dispFrame.stride = pDecInfo->rotatorStride;
		CVI_VC_DISP("stride = %d\n", info->dispFrame.stride);

	} else {
#ifdef REDUNDENT_CODE
		if (pCodecInst->productId == PRODUCT_ID_7Q &&
		    pCodecInst->codecMode != C7_HEVC_DEC) {
			if ((pCodecInst->codecMode == C7_AVC_DEC &&
			     pDecInfo->initialInfo.interlace == 0)) // [TO BE
				// OPT] use
				// fbc
				// enable
				// information
				// to adjust
				// bfOffset
				val = pDecInfo->numFbsForDecoding;
			else
				val = 0;
		} else
#endif
		{
			val = (pDecInfo->openParam.wtlEnable == TRUE ?
					     pDecInfo->numFbsForDecoding :
					     0); // fbOffset
		}

		maxDecIndex =
			(pDecInfo->numFbsForDecoding > pDecInfo->numFbsForWTL) ?
				      pDecInfo->numFbsForDecoding :
				      pDecInfo->numFbsForWTL;

		if (0 <= info->indexFrameDisplay &&
		    info->indexFrameDisplay < (int)maxDecIndex) {
			info->dispFrame =
				pDecInfo->frameBufPool[val +
						       info->indexFrameDisplay];
			CVI_VC_DISP("indexFrameDisplay = %d\n",
				    info->indexFrameDisplay);
		}
	}

	info->rdPtr = pDecInfo->streamRdPtr;
	info->wrPtr = pDecInfo->streamWrPtr;
	info->frameDisplayFlag = pDecInfo->frameDisplayFlag;

	info->sequenceNo = pDecInfo->initialInfo.sequenceNo;
	if (decodedIndex >= 0 && decodedIndex < MAX_GDI_IDX) {
		pDecInfo->decOutInfo[decodedIndex] = *info;
	}

	if (displayIndex >= 0 && displayIndex < MAX_GDI_IDX) {
		info->numOfTotMBs = info->numOfTotMBs;
		info->numOfErrMBs = info->numOfErrMBs;
		info->numOfTotMBsInDisplay =
			pDecInfo->decOutInfo[displayIndex].numOfTotMBs;
		info->numOfErrMBsInDisplay =
			pDecInfo->decOutInfo[displayIndex].numOfErrMBs;
		info->dispFrame.sequenceNo = info->sequenceNo;
	} else {
		info->numOfTotMBsInDisplay = 0;
		info->numOfErrMBsInDisplay = 0;
	}

	if (info->sequenceChanged != 0) {
		if (!(pCodecInst->productId == PRODUCT_ID_960 ||
		      pCodecInst->productId == PRODUCT_ID_980 ||
		      pCodecInst->productId == PRODUCT_ID_7Q)) {
			/* Update new sequence information */
			osal_memcpy((void *)&pDecInfo->initialInfo,
				    (void *)&pDecInfo->newSeqInfo,
				    sizeof(DecInitialInfo));
		}
		if ((info->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE) !=
		    SEQ_CHANGE_INTER_RES_CHANGE) {
			pDecInfo->initialInfo.sequenceNo++;
		}
	}

#ifdef REDUNDENT_CODE
	if (pCodecInst->productId == PRODUCT_ID_412) {
		if ((info->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE) &&
		    info->indexInterFrameDecoded == -1) {
			pDecInfo->initialInfo.sequenceNo--;
		}
	}
#endif

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return RETCODE_SUCCESS;
}

RetCode VPU_DecFrameBufferFlush(DecHandle handle, DecOutputInfo *pRemainings,
				Uint32 *retNum)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	DecOutputInfo *pOut;
	RetCode ret;
	FramebufferIndex retIndex[MAX_GDI_IDX];
	Uint32 retRemainings = 0;
	Int32 i, index, val;
	VpuAttr *pAttr = NULL;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	if (GetPendingInst(pCodecInst->coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	osal_memset((void *)retIndex, 0xff, sizeof(retIndex));

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];
	if (pAttr->supportCommandQueue == FALSE) {
		EnterDispFlagLock(pCodecInst->coreIdx);
		val = pDecInfo->frameDisplayFlag;
		val |= pDecInfo->setDisplayIndexes;
		val &= ~(Uint32)(pDecInfo->clearDisplayIndexes);
		VpuWriteReg(pCodecInst->coreIdx,
			    pDecInfo->frameDisplayFlagRegAddr, val);
		pDecInfo->clearDisplayIndexes = 0;
		pDecInfo->setDisplayIndexes = 0;
		LeaveDispFlagLock(pCodecInst->coreIdx);
	}

	ret = ProductVpuDecFlush(pCodecInst, retIndex, MAX_GDI_IDX);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	if (pRemainings != NULL) {
		for (i = 0; i < MAX_GDI_IDX; i++) {
			index = (pDecInfo->wtlEnable == TRUE) ?
					      retIndex[i].tiledIndex :
					      retIndex[i].linearIndex;
			if (index < 0)
				break;
			pRemainings[i] = pDecInfo->decOutInfo[index];
			pOut = &pRemainings[i];
			pOut->indexFrameDisplay = pOut->indexFrameDecoded;
			pOut->indexFrameDisplayForTiled =
				pOut->indexFrameDecodedForTiled;
			if (pDecInfo->wtlEnable == TRUE)
				pOut->dispFrame =
					pDecInfo->frameBufPool
						[pDecInfo->numFbsForDecoding +
						 retIndex[i].linearIndex];
			else
				pOut->dispFrame = pDecInfo->frameBufPool[index];

			pOut->dispFrame.sequenceNo = pOut->sequenceNo;
#ifdef REDUNDENT_CODE
			if (pCodecInst->codecMode == C7_VP9_DEC ||
			    pCodecInst->codecMode == C7_AVS2_DEC) {
				Uint32 regVal;
				regVal = VpuReadReg(pCodecInst->coreIdx,
						    W4_RET_DEC_DISPLAY_SIZE);
				pOut->dispPicWidth = regVal >> 16;
				pOut->dispPicHeight = regVal & 0xffff;
			} else
#endif
			{
				pOut->dispPicWidth = pOut->decPicWidth;
				pOut->dispPicHeight = pOut->decPicHeight;
			}
#ifdef REDUNDENT_CODE
			if (pCodecInst->codecMode == VC1_DEC ||
			    pCodecInst->codecMode == C7_VC1_DEC) // vc1 rotates
			// decoded
			// frame buffer
			// region. the
			// other std
			// rotated
			// whole frame
			// buffer
			// region.
			{
				if (pDecInfo->rotationEnable &&
				    (pDecInfo->rotationAngle == 90 ||
				     pDecInfo->rotationAngle == 270)) {
					pOut->rcDisplay.left =
						pDecInfo->decOutInfo[index]
							.rcDecoded.top;
					pOut->rcDisplay.right =
						pDecInfo->decOutInfo[index]
							.rcDecoded.bottom;
					pOut->rcDisplay.top =
						pDecInfo->decOutInfo[index]
							.rcDecoded.left;
					pOut->rcDisplay.bottom =
						pDecInfo->decOutInfo[index]
							.rcDecoded.right;
				} else {
					pOut->rcDisplay.left =
						pDecInfo->decOutInfo[index]
							.rcDecoded.left;
					pOut->rcDisplay.right =
						pDecInfo->decOutInfo[index]
							.rcDecoded.right;
					pOut->rcDisplay.top =
						pDecInfo->decOutInfo[index]
							.rcDecoded.top;
					pOut->rcDisplay.bottom =
						pDecInfo->decOutInfo[index]
							.rcDecoded.bottom;
				}
			} else
#endif
			{
				if (pDecInfo->rotationEnable) {
					switch (pDecInfo->rotationAngle) {
					case 90:
						pOut->rcDisplay.left =
							pDecInfo->decOutInfo[index]
								.rcDecoded.top;
						pOut->rcDisplay.right =
							pDecInfo->decOutInfo[index]
								.rcDecoded
								.bottom;
						pOut->rcDisplay.top =
							pOut->decPicWidth -
							pDecInfo->decOutInfo[index]
								.rcDecoded.right;
						pOut->rcDisplay.bottom =
							pOut->decPicWidth -
							pDecInfo->decOutInfo[index]
								.rcDecoded.left;
						break;
					case 270:
						pOut->rcDisplay.left =
							pOut->decPicHeight -
							pDecInfo->decOutInfo[index]
								.rcDecoded
								.bottom;
						pOut->rcDisplay.right =
							pOut->decPicHeight -
							pDecInfo->decOutInfo[index]
								.rcDecoded.top;
						pOut->rcDisplay.top =
							pDecInfo->decOutInfo[index]
								.rcDecoded.left;
						pOut->rcDisplay.bottom =
							pDecInfo->decOutInfo[index]
								.rcDecoded.right;
						break;
					case 180:
						pOut->rcDisplay.left =
							pDecInfo->decOutInfo[index]
								.rcDecoded.left;
						pOut->rcDisplay.right =
							pDecInfo->decOutInfo[index]
								.rcDecoded.right;
						pOut->rcDisplay.top =
							pOut->decPicHeight -
							pDecInfo->decOutInfo[index]
								.rcDecoded
								.bottom;
						pOut->rcDisplay.bottom =
							pOut->decPicHeight -
							pDecInfo->decOutInfo[index]
								.rcDecoded.top;
						break;
					default:
						pOut->rcDisplay.left =
							pDecInfo->decOutInfo[index]
								.rcDecoded.left;
						pOut->rcDisplay.right =
							pDecInfo->decOutInfo[index]
								.rcDecoded.right;
						pOut->rcDisplay.top =
							pDecInfo->decOutInfo[index]
								.rcDecoded.top;
						pOut->rcDisplay.bottom =
							pDecInfo->decOutInfo[index]
								.rcDecoded
								.bottom;
						break;
					}

				} else {
					pOut->rcDisplay.left =
						pDecInfo->decOutInfo[index]
							.rcDecoded.left;
					pOut->rcDisplay.right =
						pDecInfo->decOutInfo[index]
							.rcDecoded.right;
					pOut->rcDisplay.top =
						pDecInfo->decOutInfo[index]
							.rcDecoded.top;
					pOut->rcDisplay.bottom =
						pDecInfo->decOutInfo[index]
							.rcDecoded.bottom;
				}
			}
			retRemainings++;
		}
	}

	if (retNum)
		*retNum = retRemainings;

#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, DEC_BUF_FLUSH, 0);
#endif

	return ret;
}

RetCode VPU_DecSetRdPtr(DecHandle handle, PhysicalAddress addr, int updateWrPtr)
{
	CodecInst *pCodecInst;
	CodecInst *pPendingInst;
	DecInfo *pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}
	pCodecInst = (CodecInst *)handle;
	ret = ProductVpuDecCheckCapability(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	pPendingInst = GetPendingInst(pCodecInst->coreIdx);
	if (pCodecInst == pPendingInst) {
		VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr,
			    addr);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr,
			    addr);
	}

	pDecInfo->streamRdPtr = addr;
	pDecInfo->prevFrameEndPos = addr;
	if (updateWrPtr == TRUE) {
		pDecInfo->streamWrPtr = addr;
	}
	pDecInfo->rdPtrValidFlag = 1;

	return RETCODE_SUCCESS;
}

RetCode VPU_EncSetWrPtr(EncHandle handle, PhysicalAddress addr, int updateRdPtr)
{
	CodecInst *pCodecInst;
	CodecInst *pPendingInst;
	EncInfo *pEncInfo;
	RetCode ret;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;
	pCodecInst = (CodecInst *)handle;

	if (pCodecInst->productId == PRODUCT_ID_960 ||
	    pCodecInst->productId == PRODUCT_ID_980) {
		return RETCODE_NOT_SUPPORTED_FEATURE;
	}

	pEncInfo = &handle->CodecInfo->encInfo;
	pPendingInst = GetPendingInst(pCodecInst->coreIdx);
	if (pCodecInst == pPendingInst) {
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
			    addr);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
			    addr);
	}
	pEncInfo->streamWrPtr = addr;
	CVI_VC_TRACE("streamWrPtr = 0x%llX\n", pEncInfo->streamWrPtr);

	if (updateRdPtr)
		pEncInfo->streamRdPtr = addr;

	return RETCODE_SUCCESS;
}

RetCode VPU_GetCoreStatus(Uint32 coreIdx, int *pStatus)
{
	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	*pStatus = s_coreStatus[coreIdx];

	return RETCODE_SUCCESS;
}

RetCode VPU_SetCoreStatus(Uint32 coreIdx, int Status)
{
	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	s_coreStatus[coreIdx] = Status;

	return RETCODE_SUCCESS;
}

int VPU_GetCoreTaskNum(Uint32 coreIdx)
{
	if (coreIdx >= MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	return vdi_get_task_num(coreIdx);
}

RetCode VPU_DecClrDispFlag(DecHandle handle, int index)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret = RETCODE_SUCCESS;
	Int32 endIndex;
	VpuAttr *pAttr = NULL;
	BOOL supportCommandQueue;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	endIndex = (pDecInfo->openParam.wtlEnable == TRUE) ?
				 pDecInfo->numFbsForWTL :
				 pDecInfo->numFbsForDecoding;

	if ((index < 0) || (index > (endIndex - 1))) {
		return RETCODE_INVALID_PARAM;
	}

	supportCommandQueue = (pAttr->supportCommandQueue == TRUE);
	if (supportCommandQueue == TRUE) {
		ret = ProductClrDispFlag();
	} else {
		EnterDispFlagLock(pCodecInst->coreIdx);

		pDecInfo->clearDisplayIndexes |= (1 << index);
		CVI_VC_DISP("clearDisplayIndexes = 0x%X\n",
			    pDecInfo->clearDisplayIndexes);

		LeaveDispFlagLock(pCodecInst->coreIdx);
	}

	return ret;
}

RetCode VPU_DecGiveCommand(DecHandle handle, CodecCommand cmd, void *param)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
#ifdef REDUNDENT_CODE
	PhysicalAddress addr;
#endif

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	switch (cmd) {
	case ENABLE_ROTATION: {
		if (pDecInfo->rotatorStride == 0) {
			return RETCODE_ROTATOR_STRIDE_NOT_SET;
		}
		pDecInfo->rotationEnable = 1;
		break;
	}

#ifdef REDUNDENT_CODE
	case DISABLE_ROTATION: {
		pDecInfo->rotationEnable = 0;
		break;
	}
#endif

	case ENABLE_MIRRORING: {
		if (pDecInfo->rotatorStride == 0) {
			return RETCODE_ROTATOR_STRIDE_NOT_SET;
		}
		pDecInfo->mirrorEnable = 1;
		break;
	}
#ifdef REDUNDENT_CODE
	case DISABLE_MIRRORING: {
		pDecInfo->mirrorEnable = 0;
		break;
	}
#endif
	case SET_MIRROR_DIRECTION: {
		MirrorDirection mirDir;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		mirDir = *(MirrorDirection *)param;
		if (!(mirDir == MIRDIR_NONE) && !(mirDir == MIRDIR_HOR) &&
		    !(mirDir == MIRDIR_VER) && !(mirDir == MIRDIR_HOR_VER)) {
			return RETCODE_INVALID_PARAM;
		}
		pDecInfo->mirrorDirection = mirDir;

		break;
	}
	case SET_ROTATION_ANGLE: {
		int angle;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		angle = *(int *)param;
		if (angle != 0 && angle != 90 && angle != 180 && angle != 270) {
			return RETCODE_INVALID_PARAM;
		}
		if (pDecInfo->rotatorStride != 0) {
			if (angle == 90 || angle == 270) {
				if (pDecInfo->initialInfo.picHeight >
				    pDecInfo->rotatorStride) {
					return RETCODE_INVALID_PARAM;
				}
			} else {
				if (pDecInfo->initialInfo.picWidth >
				    pDecInfo->rotatorStride) {
					return RETCODE_INVALID_PARAM;
				}
			}
		}

		pDecInfo->rotationAngle = angle;
		break;
	}
	case SET_ROTATOR_OUTPUT: {
		FrameBuffer *frame;
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		frame = (FrameBuffer *)param;

		pDecInfo->rotatorOutput = *frame;
		pDecInfo->rotatorOutputValid = 1;
		break;
	}

	case SET_ROTATOR_STRIDE: {
		int stride;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		stride = *(int *)param;
		if (stride % 8 != 0 || stride == 0) {
			return RETCODE_INVALID_STRIDE;
		}

		if (pDecInfo->rotationAngle == 90 ||
		    pDecInfo->rotationAngle == 270) {
			if (pDecInfo->initialInfo.picHeight > stride) {
				return RETCODE_INVALID_STRIDE;
			}
		} else {
			if (pDecInfo->initialInfo.picWidth > stride) {
				return RETCODE_INVALID_STRIDE;
			}
		}

		pDecInfo->rotatorStride = stride;
		break;
	}
#ifdef REDUNDENT_CODE
	case DEC_SET_SPS_RBSP: {
		if (pCodecInst->codecMode != AVC_DEC &&
		    pCodecInst->codecMode != C7_AVC_DEC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		return SetParaSet(handle, 0, (DecParamSet *)param);
	}

	case DEC_SET_PPS_RBSP: {
		if (pCodecInst->codecMode != AVC_DEC &&
		    pCodecInst->codecMode != C7_AVC_DEC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		return SetParaSet(handle, 1, (DecParamSet *)param);
	}
#endif
	case ENABLE_DERING: {
		if (pDecInfo->rotatorStride == 0) {
			return RETCODE_ROTATOR_STRIDE_NOT_SET;
		}
		pDecInfo->deringEnable = 1;
		break;
	}

#ifdef REDUNDENT_CODE
	case DISABLE_DERING: {
		pDecInfo->deringEnable = 0;
		break;
	}
#endif
	case SET_SEC_AXI: {
		SecAxiUse secAxiUse;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		secAxiUse = *(SecAxiUse *)param;
		if (handle->productId == PRODUCT_ID_410 ||
		    handle->productId == PRODUCT_ID_4102 ||
		    handle->productId == PRODUCT_ID_420 ||
		    handle->productId == PRODUCT_ID_412 ||
		    handle->productId == PRODUCT_ID_420L ||
		    handle->productId == PRODUCT_ID_510 ||
		    handle->productId == PRODUCT_ID_512 ||
		    handle->productId == PRODUCT_ID_515 ||
		    handle->productId == PRODUCT_ID_520) {
			pDecInfo->secAxiInfo.u.wave4.useIpEnable =
				secAxiUse.u.wave4.useIpEnable;
			pDecInfo->secAxiInfo.u.wave4.useLfRowEnable =
				secAxiUse.u.wave4.useLfRowEnable;
			pDecInfo->secAxiInfo.u.wave4.useBitEnable =
				secAxiUse.u.wave4.useBitEnable;
#ifdef REDUNDENT_CODE
		} else if (handle->productId == PRODUCT_ID_7Q) {
			if (handle->codecMode == C7_HEVC_DEC) {
				pDecInfo->secAxiInfo.u.wave4.useIpEnable =
					secAxiUse.u.wave4.useIpEnable;
				pDecInfo->secAxiInfo.u.wave4.useLfRowEnable =
					secAxiUse.u.wave4.useLfRowEnable;
				pDecInfo->secAxiInfo.u.wave4.useBitEnable =
					secAxiUse.u.wave4.useBitEnable;
			} else {
				pDecInfo->secAxiInfo.u.coda9.useBitEnable =
					secAxiUse.u.coda9.useBitEnable;
				pDecInfo->secAxiInfo.u.coda9.useIpEnable =
					secAxiUse.u.coda9.useIpEnable;
				pDecInfo->secAxiInfo.u.coda9.useDbkYEnable =
					secAxiUse.u.coda9.useDbkYEnable;
				pDecInfo->secAxiInfo.u.coda9.useDbkCEnable =
					secAxiUse.u.coda9.useDbkCEnable;
				pDecInfo->secAxiInfo.u.coda9.useOvlEnable =
					secAxiUse.u.coda9.useOvlEnable;
			}
#endif
		} else {
			pDecInfo->secAxiInfo.u.coda9.useBitEnable =
				secAxiUse.u.coda9.useBitEnable;
			pDecInfo->secAxiInfo.u.coda9.useIpEnable =
				secAxiUse.u.coda9.useIpEnable;
			pDecInfo->secAxiInfo.u.coda9.useDbkYEnable =
				secAxiUse.u.coda9.useDbkYEnable;
			pDecInfo->secAxiInfo.u.coda9.useDbkCEnable =
				secAxiUse.u.coda9.useDbkCEnable;
			pDecInfo->secAxiInfo.u.coda9.useOvlEnable =
				secAxiUse.u.coda9.useOvlEnable;
			pDecInfo->secAxiInfo.u.coda9.useBtpEnable =
				secAxiUse.u.coda9.useBtpEnable;
		}

		break;
	}
	case ENABLE_REP_USERDATA: {
		if (!pDecInfo->userDataBufAddr) {
			return RETCODE_USERDATA_BUF_NOT_SET;
		}
		if (pDecInfo->userDataBufSize == 0) {
			return RETCODE_USERDATA_BUF_NOT_SET;
		}
		switch (pCodecInst->productId) {
		case PRODUCT_ID_4102:
		case PRODUCT_ID_410:
		case PRODUCT_ID_412:
		case PRODUCT_ID_7Q:
		case PRODUCT_ID_510:
		case PRODUCT_ID_512:
		case PRODUCT_ID_515:
		case PRODUCT_ID_420:
		case PRODUCT_ID_420L:
			pDecInfo->userDataEnable = *(Uint32 *)param;
			break;
		case PRODUCT_ID_960:
		case PRODUCT_ID_980:
			pDecInfo->userDataEnable = TRUE;
			break;
		default:
			VLOG(INFO,
			     "%s(ENABLE_REP_DATA) invalid productId(%d)\n",
			     __func__, pCodecInst->productId);
			return RETCODE_INVALID_PARAM;
		}
		break;
	}
	case DISABLE_REP_USERDATA: {
		pDecInfo->userDataEnable = 0;
		break;
	}
	case SET_ADDR_REP_USERDATA: {
		PhysicalAddress userDataBufAddr;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		userDataBufAddr = *(PhysicalAddress *)param;
		if (userDataBufAddr % 8 != 0 || userDataBufAddr == 0) {
			return RETCODE_INVALID_PARAM;
		}

		pDecInfo->userDataBufAddr = userDataBufAddr;
		break;
	}
#ifdef REDUNDENT_CODE
	case SET_VIRT_ADDR_REP_USERDATA: {
		__u8 *userDataVirtAddr;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		if (!pDecInfo->userDataBufAddr) {
			return RETCODE_USERDATA_BUF_NOT_SET;
		}
		if (pDecInfo->userDataBufSize == 0) {
			return RETCODE_USERDATA_BUF_NOT_SET;
		}

		userDataVirtAddr = *(void **)param;
		if (!userDataVirtAddr) {
			return RETCODE_INVALID_PARAM;
		}

		pDecInfo->vbUserData.phys_addr = pDecInfo->userDataBufAddr;
		pDecInfo->vbUserData.size = pDecInfo->userDataBufSize;
		pDecInfo->vbUserData.virt_addr = userDataVirtAddr;
		if (vdi_attach_dma_memory(pCodecInst->coreIdx,
					  &pDecInfo->vbUserData) != 0) {
			return RETCODE_INSUFFICIENT_RESOURCE;
		}
		break;
	}
#endif
	case SET_SIZE_REP_USERDATA: {
		PhysicalAddress userDataBufSize;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		userDataBufSize = *(PhysicalAddress *)param;

		pDecInfo->userDataBufSize = userDataBufSize;
		break;
	}

#ifdef REDUNDENT_CODE
	case SET_USERDATA_REPORT_MODE: {
		int userDataMode;

		userDataMode = *(int *)param;
		if (userDataMode != 1 && userDataMode != 0) {
			return RETCODE_INVALID_PARAM;
		}
		pDecInfo->userDataReportMode = userDataMode;
		break;
	}
		/** WAVE4 CU DATA REPORT INTERFACE **/
	case ENABLE_REP_CUDATA:
		if (!pDecInfo->cuDataBufAddr) {
			/* share error code with USERDATA */
			return RETCODE_USERDATA_BUF_NOT_SET;
		}
		if (pDecInfo->cuDataBufSize == 0) {
			/* share error code with USERDATA */
			return RETCODE_USERDATA_BUF_NOT_SET;
		}
		pDecInfo->cuDataEnable = TRUE;
		break;
	case DISABLE_REP_CUDATA:
		pDecInfo->cuDataEnable = FALSE;
		break;
	case SET_ADDR_REP_CUDATA:
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		addr = *(PhysicalAddress *)param;
		if ((addr % 8) != 0 || addr == 0) {
			return RETCODE_INVALID_PARAM;
		}

		pDecInfo->cuDataBufAddr = addr;
		break;
	case SET_SIZE_REP_CUDATA:
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		pDecInfo->cuDataBufSize = *(PhysicalAddress *)param;
		break;
#endif
		/************************************/
	case SET_CACHE_CONFIG: {
		MaverickCacheConfig *mcCacheConfig;
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		mcCacheConfig = (MaverickCacheConfig *)param;
		pDecInfo->cacheConfig = *mcCacheConfig;
	} break;
	case SET_LOW_DELAY_CONFIG: {
		LowDelayInfo *lowDelayInfo;
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		if (pCodecInst->productId != PRODUCT_ID_980) {
			return RETCODE_NOT_SUPPORTED_FEATURE;
		}
		lowDelayInfo = (LowDelayInfo *)param;

		if (lowDelayInfo->lowDelayEn) {
			if ((pCodecInst->codecMode != AVC_DEC &&
			     pCodecInst->codecMode != C7_AVC_DEC) ||
			    pDecInfo->rotationEnable ||
			    pDecInfo->mirrorEnable ||
			    pDecInfo->tiled2LinearEnable ||
			    pDecInfo->deringEnable) {
				return RETCODE_INVALID_PARAM;
			}
		}

		pDecInfo->lowDelayInfo.lowDelayEn = lowDelayInfo->lowDelayEn;
		pDecInfo->lowDelayInfo.numRows = lowDelayInfo->numRows;
	} break;

#ifdef REDUNDENT_CODE
	case SET_DECODE_FLUSH: // interrupt mode to pic_end
		ret = ProductCpbFlush((CodecInst *)handle);
		break;

	case DEC_SET_FRAME_DELAY: {
		pDecInfo->frameDelay = *(int *)param;
		break;
	}
	case DEC_ENABLE_REORDER: {
		if ((handle->productId == PRODUCT_ID_980) ||
		    (handle->productId == PRODUCT_ID_960) ||
		    (handle->productId == PRODUCT_ID_950) ||
		    (handle->productId == PRODUCT_ID_7503) ||
		    (handle->productId == PRODUCT_ID_320)) {
			if (pDecInfo->initialInfoObtained) {
				return RETCODE_WRONG_CALL_SEQUENCE;
			}
		}

		pDecInfo->reorderEnable = 1;
		break;
	}
	case DEC_DISABLE_REORDER: {
		if ((handle->productId == PRODUCT_ID_980) ||
		    (handle->productId == PRODUCT_ID_960) ||
		    (handle->productId == PRODUCT_ID_950) ||
		    (handle->productId == PRODUCT_ID_7503) ||
		    (handle->productId == PRODUCT_ID_320)) {
			if (pDecInfo->initialInfoObtained) {
				return RETCODE_WRONG_CALL_SEQUENCE;
			}

			if (pCodecInst->codecMode != AVC_DEC &&
			    pCodecInst->codecMode != VC1_DEC &&
			    pCodecInst->codecMode != AVS_DEC &&
			    pCodecInst->codecMode != C7_AVC_DEC &&
			    pCodecInst->codecMode != C7_VC1_DEC &&
			    pCodecInst->codecMode != C7_AVS_DEC) {
				return RETCODE_INVALID_COMMAND;
			}
		}

		pDecInfo->reorderEnable = 0;
		break;
	}
	case DEC_SET_AVC_ERROR_CONCEAL_MODE: {
		if (pCodecInst->codecMode != AVC_DEC &&
		    pCodecInst->codecMode != C7_AVC_DEC) {
			return RETCODE_INVALID_COMMAND;
		}

		pDecInfo->avcErrorConcealMode = *(int *)param;
		break;
	}
#endif
	case DEC_FREE_FRAME_BUFFER: {
		int i;
		if (pDecInfo->vbSlice.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbSlice);

		if (pDecInfo->vbFrame.size) {
			if (pDecInfo->frameAllocExt == 0)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbFrame);
		}
		for (i = 0; i < MAX_REG_FRAME; i++) {
			if (pDecInfo->vbFbcYTbl[i].size)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbFbcYTbl[i]);

			if (pDecInfo->vbFbcCTbl[i].size)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbFbcCTbl[i]);

			if (pDecInfo->vbMV[i].size)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbMV[i]);
		}

		if (pDecInfo->vbPPU.size) {
			if (pDecInfo->ppuAllocExt == 0)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbPPU);
		}

		if (pDecInfo->wtlEnable) {
			if (pDecInfo->vbWTL.size)
				VDI_FREE_MEMORY(pCodecInst->coreIdx,
						&pDecInfo->vbWTL);
		}
		break;
	}
	case DEC_GET_FRAMEBUF_INFO: {
		DecGetFramebufInfo *fbInfo = (DecGetFramebufInfo *)param;
		Uint32 i;
		fbInfo->vbFrame = pDecInfo->vbFrame;
		fbInfo->vbWTL = pDecInfo->vbWTL;
		for (i = 0; i < MAX_REG_FRAME; i++) {
			fbInfo->vbFbcYTbl[i] = pDecInfo->vbFbcYTbl[i];
			fbInfo->vbFbcCTbl[i] = pDecInfo->vbFbcCTbl[i];
			fbInfo->vbMvCol[i] = pDecInfo->vbMV[i];
		}

		for (i = 0; i < MAX_GDI_IDX * 2; i++) {
			fbInfo->framebufPool[i] = pDecInfo->frameBufPool[i];
		}
	} break;
	case DEC_RESET_FRAMEBUF_INFO: {
		int i;

		pDecInfo->vbFrame.base = 0;
		pDecInfo->vbFrame.phys_addr = 0;
		pDecInfo->vbFrame.virt_addr = 0;
		pDecInfo->vbFrame.size = 0;
		pDecInfo->vbWTL.base = 0;
		pDecInfo->vbWTL.phys_addr = 0;
		pDecInfo->vbWTL.virt_addr = 0;
		pDecInfo->vbWTL.size = 0;
		for (i = 0; i < MAX_REG_FRAME; i++) {
			pDecInfo->vbFbcYTbl[i].base = 0;
			pDecInfo->vbFbcYTbl[i].phys_addr = 0;
			pDecInfo->vbFbcYTbl[i].virt_addr = 0;
			pDecInfo->vbFbcYTbl[i].size = 0;
			pDecInfo->vbFbcCTbl[i].base = 0;
			pDecInfo->vbFbcCTbl[i].phys_addr = 0;
			pDecInfo->vbFbcCTbl[i].virt_addr = 0;
			pDecInfo->vbFbcCTbl[i].size = 0;
			pDecInfo->vbMV[i].base = 0;
			pDecInfo->vbMV[i].phys_addr = 0;
			pDecInfo->vbMV[i].virt_addr = 0;
			pDecInfo->vbMV[i].size = 0;
		}

		pDecInfo->frameDisplayFlag = 0;
		pDecInfo->setDisplayIndexes = 0;
		pDecInfo->clearDisplayIndexes = 0;
		break;
	}
#ifdef REDUNDENT_CODE
	case DEC_GET_QUEUE_STATUS: {
		DecQueueStatusInfo *queueInfo = (DecQueueStatusInfo *)param;
		queueInfo->instanceQueueCount = pDecInfo->instanceQueueCount;
		queueInfo->totalQueueCount = pDecInfo->totalQueueCount;
		break;
	}
#endif

	case ENABLE_DEC_THUMBNAIL_MODE: {
		pDecInfo->thumbnailMode = 1;
		break;
	}
	case DEC_GET_SEQ_INFO: {
		DecInitialInfo *seqInfo = (DecInitialInfo *)param;
		*seqInfo = pDecInfo->initialInfo;
		break;
	}
#ifdef REDUNDENT_CODE
	case DEC_GET_FIELD_PIC_TYPE: {
		return RETCODE_FAILURE;
	}
	case DEC_GET_DISPLAY_OUTPUT_INFO: {
		DecOutputInfo *pDecOutInfo = (DecOutputInfo *)param;
		*pDecOutInfo =
			pDecInfo->decOutInfo[pDecOutInfo->indexFrameDisplay];
		break;
	}
#endif
	case GET_TILEDMAP_CONFIG: {
		TiledMapConfig *pMapCfg = (TiledMapConfig *)param;
		if (!pMapCfg) {
			return RETCODE_INVALID_PARAM;
		}
		if (!pDecInfo->stride) {
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
		*pMapCfg = pDecInfo->mapCfg;
		break;
	}
#ifdef REDUNDENT_CODE
	case SET_DRAM_CONFIG: {
		DRAMConfig *cfg = (DRAMConfig *)param;

		if (!cfg) {
			return RETCODE_INVALID_PARAM;
		}

		pDecInfo->dramCfg = *cfg;
		break;
	}
	case GET_DRAM_CONFIG: {
		DRAMConfig *cfg = (DRAMConfig *)param;

		if (!cfg) {
			return RETCODE_INVALID_PARAM;
		}

		*cfg = pDecInfo->dramCfg;

		break;
	}
	case GET_LOW_DELAY_OUTPUT: {
		DecOutputInfo *lowDelayOutput;
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		if (!pDecInfo->lowDelayInfo.lowDelayEn ||
		    pCodecInst->codecMode != AVC_DEC) {
			return RETCODE_INVALID_COMMAND;
		}

		if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
			return RETCODE_WRONG_CALL_SEQUENCE;
		}

		lowDelayOutput = (DecOutputInfo *)param;

		GetLowDelayOutput(pCodecInst, lowDelayOutput);
	} break;
#endif
#ifdef ENABLE_CNM_DEBUG_MSG
	case ENABLE_LOGGING: {
		pCodecInst->loggingEnable = 1;
		break;
	}
	case DISABLE_LOGGING: {
		pCodecInst->loggingEnable = 0;
		break;
	}
#endif
	case DEC_SET_SEQ_CHANGE_MASK:
		if (PRODUCT_ID_NOT_W_SERIES(pCodecInst->productId))
			return RETCODE_INVALID_PARAM;
		pDecInfo->seqChangeMask = *(int *)param;
		break;
	case DEC_SET_WTL_FRAME_FORMAT:
		pDecInfo->wtlFormat = *(FrameBufferFormat *)param;
		break;
#ifdef REDUNDENT_CODE
	case DEC_SET_DISPLAY_FLAG: {
		Int32 index;
		VpuAttr *pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];
		BOOL supportCommandQueue = FALSE;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		index = *(Int32 *)param;

		supportCommandQueue = (pAttr->supportCommandQueue == TRUE);
		if (supportCommandQueue == TRUE) {
			ProductSetDispFlag();
		} else {
			EnterDispFlagLock(pCodecInst->coreIdx);

			pDecInfo->setDisplayIndexes |= (1 << index);
			CVI_VC_DISP("DispIdx = 0x%X\n",
				    pDecInfo->setDisplayIndexes);

			LeaveDispFlagLock(pCodecInst->coreIdx);
		}

	} break;
#endif
#ifdef DRAM_TEST
	case DRAM_READ_WRITE_TEST: {
		Uint32 dram_source_addr;
		Uint32 dram_destination_addr;
		Uint32 dram_data_size;
		Uint32 result;
		Uint32 core_idx;

		unsigned int i;
		// unsigned char probeData[64];
		unsigned char *probeSourceData;
		unsigned char *probeDestinationData;

		dram_source_addr = 0x80000000;
		dram_destination_addr = 0x90000000;
		dram_data_size = 0x00000010;
		core_idx = 0;

		result = ExecDRAMReadWriteTest(core_idx, &dram_source_addr,
					       &dram_destination_addr,
					       &dram_data_size);
		if (result == RETCODE_SUCCESS)
			VLOG(INFO,
			     "DRAM Read/Write Test is successful result = %x\r\n",
			     result);
		else
			VLOG(INFO,
			     "DRAM Read/Write Test is not successful result = %x\r\n",
			     result);

		probeSourceData =
			(unsigned char *)osal_malloc(dram_data_size * 4);
		probeDestinationData =
			(unsigned char *)osal_malloc(dram_data_size * 4);
		// osal_memset(probeData, 0xff, 64);

		vdi_read_memory(core_idx, dram_source_addr, probeSourceData,
				dram_data_size * 4, VDI_BIG_ENDIAN);

		for (i = 0; i < (dram_data_size * 4); i = i + 8) {
			printf("probeSourceData 0x%04xh: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			       dram_source_addr + i, probeSourceData[i],
			       probeSourceData[i + 1], probeSourceData[i + 2],
			       probeSourceData[i + 3], probeSourceData[i + 4],
			       probeSourceData[i + 5], probeSourceData[i + 6],
			       probeSourceData[i + 7]);
		}

		vdi_read_memory(core_idx, dram_destination_addr,
				probeDestinationData, dram_data_size * 4,
				VDI_BIG_ENDIAN);

		for (i = 0; i < (dram_data_size * 4); i = i + 8) {
			printf("probeDestinationData 0x%04xh: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			       dram_destination_addr + i,
			       probeDestinationData[i],
			       probeDestinationData[i + 1],
			       probeDestinationData[i + 2],
			       probeDestinationData[i + 3],
			       probeDestinationData[i + 4],
			       probeDestinationData[i + 5],
			       probeDestinationData[i + 6],
			       probeDestinationData[i + 7]);
		}

		if (memcmp(probeSourceData, probeDestinationData,
			   dram_data_size * 4) == 0) {
			printf("Source Data and Destination Data are identical\r\n");
			result = RETCODE_SUCCESS;
		} else {
			printf("Source Data and Destination Data are not identical\r\n");
			result = RETCODE_FAILURE;
		}

		free(probeDestinationData);
		free(probeSourceData);
		break;
	}
#endif /* SUPPORT DRAM_TEST */
	case DEC_SET_TARGET_TEMPORAL_ID:
		if (param == NULL) {
			return RETCODE_INVALID_PARAM;
		}

		if (AVC_DEC == pCodecInst->codecMode ||
		    HEVC_DEC == pCodecInst->codecMode) {
			Uint32 targetSublayerId = *(Uint32 *)param;
			Uint32 maxTargetId =
				(AVC_DEC == pCodecInst->codecMode) ?
					      AVC_MAX_SUB_LAYER_ID :
					      HEVC_MAX_SUB_LAYER_ID;

			if (targetSublayerId > maxTargetId) {
				ret = RETCODE_INVALID_PARAM;
			} else {
				pDecInfo->targetSubLayerId = targetSublayerId;
			}
		} else {
			ret = RETCODE_NOT_SUPPORTED_FEATURE;
		}
		break;
#ifdef REDUNDENT_CODE
	case DEC_SET_BWB_CUR_FRAME_IDX:
		pDecInfo->chBwbFrameIdx = *(Uint32 *)param;
		break;
	case DEC_SET_FBC_CUR_FRAME_IDX:
		pDecInfo->chFbcFrameIdx = *(Uint32 *)param;
		break;
	case DEC_SET_INTER_RES_INFO_ON:
		pDecInfo->interResChange = 1;
		break;
	case DEC_SET_INTER_RES_INFO_OFF:
		pDecInfo->interResChange = 0;
		break;
	case DEC_FREE_FBC_TABLE_BUFFER: {
		Uint32 fbcCurFrameIdx = *(Uint32 *)param;
		if (pDecInfo->vbFbcYTbl[fbcCurFrameIdx].size > 0) {
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcYTbl[fbcCurFrameIdx]);
			pDecInfo->vbFbcYTbl[fbcCurFrameIdx].size = 0;
		}
		if (pDecInfo->vbFbcCTbl[fbcCurFrameIdx].size > 0) {
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcCTbl[fbcCurFrameIdx]);
			pDecInfo->vbFbcCTbl[fbcCurFrameIdx].size = 0;
		}
	} break;
	case DEC_FREE_MV_BUFFER: {
		Uint32 fbcCurFrameIdx = *(Uint32 *)param;
		if (pDecInfo->vbMV[fbcCurFrameIdx].size > 0) {
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbMV[fbcCurFrameIdx]);
			pDecInfo->vbMV[fbcCurFrameIdx].size = 0;
		}
	} break;
	case DEC_ALLOC_MV_BUFFER: {
		Uint32 fbcCurFrameIdx = *(Uint32 *)param;
		Uint32 size;
		size = WAVE4_DEC_VP9_MVCOL_BUF_SIZE(
			pDecInfo->initialInfo.picWidth,
			pDecInfo->initialInfo.picHeight);
		pDecInfo->vbMV[fbcCurFrameIdx].phys_addr = 0;
		pDecInfo->vbMV[fbcCurFrameIdx].size =
			((size + 4095) & ~4095) + 4096; /* 4096 is a margin */

		CVI_VC_MEM("vbMV[%d].size = 0x%X\n", fbcCurFrameIdx,
			   pDecInfo->vbMV[fbcCurFrameIdx].size);
		if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbMV[fbcCurFrameIdx], 0) < 0)
			return RETCODE_INSUFFICIENT_RESOURCE;
	} break;
	case DEC_ALLOC_FBC_Y_TABLE_BUFFER: {
		Uint32 fbcCurFrameIdx = *(Uint32 *)param;
		Uint32 size;
		size = WAVE4_FBC_LUMA_TABLE_SIZE(
			VPU_ALIGN64(pDecInfo->initialInfo.picWidth),
			VPU_ALIGN64(pDecInfo->initialInfo.picHeight));
		size = VPU_ALIGN16(size);
		pDecInfo->vbFbcYTbl[fbcCurFrameIdx].phys_addr = 0;
		pDecInfo->vbFbcYTbl[fbcCurFrameIdx].size =
			((size + 4095) & ~4095) + 4096;
		CVI_VC_MEM("vbFbcYTbl[%d].size = 0x%X\n", fbcCurFrameIdx,
			   pDecInfo->vbFbcYTbl[fbcCurFrameIdx].size);
		if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcYTbl[fbcCurFrameIdx],
					0) < 0)
			return RETCODE_INSUFFICIENT_RESOURCE;
	} break;
	case DEC_ALLOC_FBC_C_TABLE_BUFFER: {
		Uint32 fbcCurFrameIdx = *(Uint32 *)param;
		Uint32 size;
		size = WAVE4_FBC_CHROMA_TABLE_SIZE(
			VPU_ALIGN64(pDecInfo->initialInfo.picWidth),
			VPU_ALIGN64(pDecInfo->initialInfo.picHeight));
		size = VPU_ALIGN16(size);
		pDecInfo->vbFbcCTbl[fbcCurFrameIdx].phys_addr = 0;
		pDecInfo->vbFbcCTbl[fbcCurFrameIdx].size =
			((size + 4095) & ~4095) + 4096;
		CVI_VC_MEM("vbFbcCTbl[%d].size = 0x%X\n", fbcCurFrameIdx,
			   pDecInfo->vbFbcCTbl[fbcCurFrameIdx].size);
		if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx,
					&pDecInfo->vbFbcCTbl[fbcCurFrameIdx],
					0) < 0)
			return RETCODE_INSUFFICIENT_RESOURCE;
	} break;
#endif
	default:
		return RETCODE_INVALID_COMMAND;
	}

	return ret;
}

RetCode VPU_DecAllocateFrameBuffer(DecHandle handle, FrameBufferAllocInfo info,
				   FrameBuffer *frameBuffer)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	RetCode ret;
	Uint32 gdiIndex;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	if (!frameBuffer) {
		return RETCODE_INVALID_PARAM;
	}

	if (info.type == FB_TYPE_PPU) {
		if (pDecInfo->numFrameBuffers == 0)
			return RETCODE_WRONG_CALL_SEQUENCE;
		if (frameBuffer[0].updateFbInfo == TRUE) {
			pDecInfo->ppuAllocExt = TRUE;
		}
		pDecInfo->ppuAllocExt = frameBuffer[0].updateFbInfo;
		gdiIndex = pDecInfo->numFbsForDecoding;
		ret = ProductVpuAllocateFramebuffer(
			pCodecInst, frameBuffer, (TiledMapType)info.mapType,
			(Int32)info.num, info.stride, info.height, info.format,
			info.cbcrInterleave, info.nv21, info.endian,
			&pDecInfo->vbPPU, gdiIndex, FB_TYPE_PPU);
	} else if (info.type == FB_TYPE_CODEC) {
		gdiIndex = 0;
		if (frameBuffer[0].updateFbInfo == TRUE) {
			pDecInfo->frameAllocExt = TRUE;
		}
		ret = ProductVpuAllocateFramebuffer(
			pCodecInst, frameBuffer, (TiledMapType)info.mapType,
			(Int32)info.num, info.stride, info.height, info.format,
			info.cbcrInterleave, info.nv21, info.endian,
			&pDecInfo->vbFrame, gdiIndex,
			(FramebufferAllocType)info.type);

		pDecInfo->mapCfg.tiledBaseAddr = pDecInfo->vbFrame.phys_addr;
	}

	return ret;
}

RetCode VPU_EncOpen(EncHandle *pHandle, EncOpenParam *pop)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;

	CVI_VC_TRACE("\n");

	ret = ProductCheckEncOpenParam(pop);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ret = %d\n", ret);
		return ret;
	}

	if (VPU_IsInit(pop->coreIdx) == 0) {
		return RETCODE_NOT_INITIALIZED;
	}

	ret = GetCodecInstance(pop->coreIdx, &pCodecInst);
	if (ret == RETCODE_FAILURE) {
		*pHandle = 0;
		CVI_VC_ERR("\n");
		return RETCODE_FAILURE;
	}

	pCodecInst->isDecoder = FALSE;
	*pHandle = pCodecInst;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	osal_memset(pEncInfo, 0x00, sizeof(EncInfo));
	pEncInfo->openParam = *pop;

	cviConfigEncParam(pCodecInst, pop);

	SetClockGate(pop->coreIdx, TRUE);

	ret = ProductVpuEncBuildUpOpenParam(pCodecInst, pop);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("\n");
		*pHandle = 0;
	}
	SetClockGate(pop->coreIdx, FALSE);

	return ret;
}

RetCode cviConfigEncParam(CodecInst *pCodec, EncOpenParam *param)
{
	RetCode ret = RETCODE_SUCCESS;
	EncInfo *pEncInfo = &pCodec->CodecInfo->encInfo;

	pCodec->s32ChnNum = param->s32ChnNum;
	pEncInfo->cviRcEn = param->cviRcEn;
	pEncInfo->addrRemapEn = param->addrRemapEn;

	return ret;
}

void VPU_WaitPendingInst(CodecInst *pCodecInst)
{
RETRY:
	if (GetPendingInst(pCodecInst->coreIdx)) {
		goto RETRY;
	}

	SetPendingInst(pCodecInst->coreIdx, pCodecInst, __func__, __LINE__);
}

RetCode VPU_EncClose(EncHandle handle)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	VPU_WaitPendingInst(pCodecInst);

	if (pEncInfo->initialInfoObtained) {
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
			    pEncInfo->streamWrPtr);
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
			    pEncInfo->streamRdPtr);

		ret = ProductVpuEncFiniSeq(pCodecInst);
		if (ret != RETCODE_SUCCESS) {
#ifdef ENABLE_CNM_DEBUG_MSG
			if (pCodecInst->loggingEnable)
				vdi_log(pCodecInst->coreIdx, ENC_SEQ_END, 0);
#endif
		}
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENC_SEQ_END, 0);
#endif
		pEncInfo->streamWrPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr));
	}

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	if (pEncInfo->vbScratch.size) {
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pEncInfo->vbScratch);
	}

	if (pEncInfo->vbWork.size) {
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pEncInfo->vbWork);
	}

	if (pEncInfo->vbFrame.size) {
		if (pEncInfo->frameAllocExt == 0)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbFrame);
	}

	if (pCodecInst->codecMode == HEVC_ENC) {
		if (pEncInfo->vbSubSamBuf.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbSubSamBuf);

		if (pEncInfo->vbMV.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx, &pEncInfo->vbMV);

		if (pEncInfo->vbFbcYTbl.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbFbcYTbl);

		if (pEncInfo->vbFbcCTbl.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbFbcCTbl);

		if (pEncInfo->vbTemp.size)
			vdi_dettach_dma_memory(pCodecInst->coreIdx,
					       &pEncInfo->vbTemp);
	}

	if (pCodecInst->codecMode == C7_AVC_ENC) {
		if (pEncInfo->vbFbcYTbl.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbFbcYTbl);

		if (pEncInfo->vbFbcCTbl.size)
			VDI_FREE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbFbcCTbl);
	}

	if (pEncInfo->vbPPU.size) {
		if (pEncInfo->ppuAllocExt == 0)
			VDI_FREE_MEMORY(pCodecInst->coreIdx, &pEncInfo->vbPPU);
	}
	if (pEncInfo->vbSubSampFrame.size)
		VDI_FREE_MEMORY(pCodecInst->coreIdx, &pEncInfo->vbSubSampFrame);
	if (pEncInfo->vbMvcSubSampFrame.size)
		VDI_FREE_MEMORY(pCodecInst->coreIdx,
				&pEncInfo->vbMvcSubSampFrame);

	FreeCodecInstance(pCodecInst);

	return ret;
}

RetCode VPU_EncGetInitialInfo(EncHandle handle, EncInitialInfo *info)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;

	CVI_VC_TRACE("\n");

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("CheckEncInstanceValidity ret error %d\n", ret);
		return ret;
	}

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	VPU_WaitPendingInst(pCodecInst);

	ret = ProductVpuEncSetup(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ProductVpuEncSetup ret error %d\n", ret);
		return ret;
	}

	CVI_VC_TRACE("productId = 0x%X\n", pCodecInst->productId);
	if (pCodecInst->codecMode == AVC_ENC &&
	    pCodecInst->codecModeAux == AVC_AUX_MVC)
		info->minFrameBufferCount = 3; // reconstructed frame + 2
			// reference frame
#ifdef REDUNDENT_CODE
	else if (pCodecInst->codecMode == C7_AVC_ENC &&
		 pCodecInst->codecModeAux == AVC_AUX_MVC)
		info->minFrameBufferCount = 3; // reconstructed frame + 2
			// reference frame
#endif
	else if (pCodecInst->codecMode == HEVC_ENC) {
		info->minFrameBufferCount =
			pEncInfo->initialInfo.minFrameBufferCount;
		info->minSrcFrameCount = pEncInfo->initialInfo.minSrcFrameCount;
	}
#ifdef SUPPORT_980_ROI_RC_LIB
	else if (pCodecInst->codecMode == AVC_ENC &&
		 pCodecInst->codecModeAux == AVC_AUX_SVC) {
		info->minFrameBufferCount = pEncInfo->num_total_frames;
	}
#endif
	else
		info->minFrameBufferCount = 2; // reconstructed frame +
			// reference frame

	pEncInfo->initialInfo = *info;
	pEncInfo->initialInfoObtained = TRUE;

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return RETCODE_SUCCESS;
}

RetCode VPU_EncRegisterFrameBuffer(EncHandle handle, FrameBuffer *bufArray,
				   int num, int stride, int height, int mapType)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	Int32 i;
	RetCode ret;
	EncOpenParam *openParam;
	FrameBuffer *fb;

	CVI_VC_TRACE("\n");

	ret = CheckEncInstanceValidity(handle);
	// FIXME temp
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	openParam = &pEncInfo->openParam;

	if (pEncInfo->stride)
		return RETCODE_CALLED_BEFORE;

	if (!pEncInfo->initialInfoObtained)
		return RETCODE_WRONG_CALL_SEQUENCE;

	if (num < pEncInfo->initialInfo.minFrameBufferCount)
		return RETCODE_INSUFFICIENT_FRAME_BUFFERS;

	if (stride == 0 || (stride % 8 != 0) || stride < 0)
		return RETCODE_INVALID_STRIDE;

	if (height == 0 || height < 0)
		return RETCODE_INVALID_PARAM;

	if (openParam->bitstreamFormat == STD_HEVC) {
		if (stride % 32 != 0)
			return RETCODE_INVALID_STRIDE;
	}

	VPU_WaitPendingInst(pCodecInst);

	pEncInfo->numFrameBuffers = num;
	pEncInfo->stride = stride;
	pEncInfo->frameBufferHeight = height;
	pEncInfo->mapType = mapType;
	pEncInfo->mapCfg.productId = pCodecInst->productId;

	if (bufArray) {
		for (i = 0; i < num; i++)
			pEncInfo->frameBufPool[i] = bufArray[i];
	}

	if (pEncInfo->frameAllocExt == FALSE) {
		fb = pEncInfo->frameBufPool;
		if (bufArray) {
			if (bufArray[0].bufCb == (PhysicalAddress)-1 &&
			    bufArray[0].bufCr == (PhysicalAddress)-1) {
				Uint32 size;
				pEncInfo->frameAllocExt = TRUE;
				size = ProductCalculateFrameBufSize(
					pCodecInst->productId, stride, height,
					(TiledMapType)mapType,
					openParam->srcFormat,
					(BOOL)openParam->cbcrInterleave, NULL);
				if (mapType == LINEAR_FRAME_MAP) {
					pEncInfo->vbFrame.phys_addr =
						bufArray[0].bufY;
					pEncInfo->vbFrame.size = size * num;
				}
			}
		}
		ret = ProductVpuAllocateFramebuffer(
			pCodecInst, fb, (TiledMapType)mapType, num, stride,
			height, openParam->srcFormat, openParam->cbcrInterleave,
			FALSE, openParam->frameEndian, &pEncInfo->vbFrame, 0,
			FB_TYPE_CODEC);
		if (ret != RETCODE_SUCCESS) {
			SetPendingInst(pCodecInst->coreIdx, NULL, __func__,
				       __LINE__);
			CVI_VC_ERR("SetPendingInst clear\n");
			return ret;
		}
	}
	ret = ProductVpuRegisterFramebuffer(pCodecInst);

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return ret;
}

#ifdef REDUNDENT_CODE
RetCode VPU_EncGetFrameBuffer(EncHandle handle, int frameIdx,
			      FrameBuffer *frameBuf)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	if (frameIdx < 0 || frameIdx > pEncInfo->numFrameBuffers)
		return RETCODE_INVALID_PARAM;

	if (frameBuf == 0)
		return RETCODE_INVALID_PARAM;

	*frameBuf = pEncInfo->frameBufPool[frameIdx];

	return RETCODE_SUCCESS;
}
#endif

RetCode VPU_EncGetBitstreamBuffer(EncHandle handle, PhysicalAddress *prdPrt,
				  PhysicalAddress *pwrPtr, int *size)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	Uint32 room;
	RetCode ret;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (prdPrt == 0 || pwrPtr == 0 || size == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	rdPtr = pEncInfo->streamRdPtr;

	SetClockGate(pCodecInst->coreIdx, 1);

	if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst) {
		wrPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr));
	} else {
		wrPtr = vdi_remap_memory_address(pCodecInst->coreIdx,
						 pEncInfo->streamWrPtr);
	}

	SetClockGate(pCodecInst->coreIdx, 0);
	if (pEncInfo->ringBufferEnable == 1 || pEncInfo->lineBufIntEn == 1) {
		if (wrPtr >= rdPtr) {
			room = wrPtr - rdPtr;
		} else {
			room = (pEncInfo->streamBufEndAddr - rdPtr) +
			       (wrPtr - pEncInfo->streamBufStartAddr);
		}
	} else {
		if (wrPtr >= rdPtr)
			room = wrPtr - rdPtr;
		else {
			CVI_VC_ERR("RETCODE_INVALID_PARAM\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	*prdPrt = pEncInfo->streamRdPtr;
	*pwrPtr = wrPtr;
	*size = room;

	return RETCODE_SUCCESS;
}

RetCode VPU_EncUpdateBitstreamBuffer(EncHandle handle, int size)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	PhysicalAddress wrPtr;
	PhysicalAddress rdPtr;
	RetCode ret;
	int room = 0;
	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	rdPtr = pEncInfo->streamRdPtr;

	SetClockGate(pCodecInst->coreIdx, 1);

	if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
		wrPtr = vdi_remap_memory_address(
			pCodecInst->coreIdx,
			VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr));
	else
		wrPtr = pEncInfo->streamWrPtr;

	if (rdPtr < wrPtr) {
		if (rdPtr + size > wrPtr) {
			SetClockGate(pCodecInst->coreIdx, 0);
			CVI_VC_ERR(
				"invalid param! rdPtr 0x%llx, wrPtr 0x%llx, size 0x%x\n",
				rdPtr, wrPtr, size);
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pEncInfo->ringBufferEnable == TRUE ||
	    pEncInfo->lineBufIntEn == TRUE) {
		rdPtr += size;
		if (rdPtr > pEncInfo->streamBufEndAddr  + STREAM_BUF_SIZE_RESERVE) {
			if (pEncInfo->lineBufIntEn == TRUE) {
				CVI_VC_ERR(
					"invalid param! rdPtr 0x%llx, streamBufEndAddr 0x%llx\n",
					rdPtr, pEncInfo->streamBufEndAddr);
				return RETCODE_INVALID_PARAM;
			}
			room = rdPtr - pEncInfo->streamBufEndAddr;
			rdPtr = pEncInfo->streamBufStartAddr;
			rdPtr += room;
		}

		if (rdPtr == pEncInfo->streamBufEndAddr) {
			rdPtr = pEncInfo->streamBufStartAddr;
		}
	} else {
		rdPtr = pEncInfo->streamBufStartAddr;
	}

	CVI_VC_TRACE("rdPtr = 0x%llX, wrPtr = 0x%llX\n", rdPtr, wrPtr);

	pEncInfo->streamRdPtr = rdPtr;
	pEncInfo->streamWrPtr = wrPtr;
	if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
		VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
			    rdPtr);

	if (pEncInfo->lineBufIntEn == TRUE) {
		// reset RdPtr to BufStartAddr if bEsBufQueueEn disable
		if (pEncInfo->bEsBufQueueEn == 0) {
			pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
		}
	}

	SetClockGate(pCodecInst->coreIdx, 0);
	return RETCODE_SUCCESS;
}

RetCode VPU_EncStartOneFrame(EncHandle handle, EncParam *param)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;
	VpuAttr *pAttr = NULL;
	vpu_instance_pool_t *vip;

	ret = CheckEncInstanceValidity(handle);

	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("CheckEncInstanceValidity error\n");
		return ret;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		return RETCODE_INVALID_HANDLE;
	}

	if (pEncInfo->stride == 0) { // This means frame buffers have not been
		// registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	ret = CheckEncParam(handle, param);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("CheckEncParam, ret = %d\n", ret);
		return ret;
	}

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];
	(void)pAttr;

	pEncInfo->ptsMap[param->srcIdx] =
		(pEncInfo->openParam.enablePTS == TRUE) ? GetTimestamp(handle) :
								param->pts;

	ret = ProductVpuEncode(pCodecInst, param);

	pCodecInst->cvi.frameNo++;

	return ret;
}

RetCode cviVPU_LockAndCheck(EncHandle handle, int targetState, int timeout_ms)
{
	CodecInst *pCodecInst;
	RetCode ret;
	vpu_instance_pool_t *vip;
	int err;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		//return invalid
		return RETCODE_INVALID_HANDLE;
	}

	if (timeout_ms <= -1) {
		//block mode
		while (1) {
			EnterVcodecLock(pCodecInst->coreIdx);

			if (pCodecInst->state != targetState) {
				CVI_VC_TRACE("state = %d\n", pCodecInst->state);
				LeaveVcodecLock(pCodecInst->coreIdx);
				cond_resched();
			} else {
				break;
			}
		}
	} else {
		//time lock mode
		err = VcodecTimeLock(pCodecInst->coreIdx, timeout_ms);
		if (err == RETCODE_SUCCESS) {
			//lock success in time < timeout_ms
			if (pCodecInst->state == targetState) {
				//inside the state we required , keep lock
				ret = RETCODE_SUCCESS;
			} else {
				//not the require state ...release mutex
				CVI_VC_ERR(
					"pCodecInst->state mismatch target:%d\n",
					targetState);
				LeaveVcodecLock(pCodecInst->coreIdx);
				ret = RETCODE_FAILURE;
			}
		} else if (err == RETCODE_VPU_RESPONSE_TIMEOUT) {
			//time out
			//cannot get the lock < timeout_ms
			ret = RETCODE_VPU_RESPONSE_TIMEOUT;
		} else {
			//never been lock success
			CVI_VC_ERR("VcodecTimeLock abnormal ret:%d\n", ret);
			ret = RETCODE_FAILURE;
		}
	}
	return ret;
}

RetCode cviVPU_ChangeState(EncHandle handle)
{
	CodecInst *pCodecInst;
	RetCode ret;
	vpu_instance_pool_t *vip;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		return RETCODE_INVALID_HANDLE;
	}

	pCodecInst->state = (pCodecInst->state + 1) % CODEC_STAT_MAX;
	CVI_VC_TRACE("state = %d\n", pCodecInst->state);

	return ret;
}

RetCode VPU_EncGetOutputInfo(EncHandle handle, EncOutputInfo *info)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;
	VpuAttr *pAttr;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ret %d\n", ret);
		return ret;
	}

	if (info == 0) {
		CVI_VC_ERR("info == 0\n");
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (pAttr->supportCommandQueue == FALSE) {
		CodecInst *p = GetPendingInst(pCodecInst->coreIdx);
		if (pCodecInst != p) {
			CVI_VC_ERR("Different pCodecInst %p, p %p\n",
				   pCodecInst, p);
			SetPendingInst(pCodecInst->coreIdx, NULL, __func__,
				       __LINE__);
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
	}

	ret = ProductVpuEncGetResult(pCodecInst, info);

	if (ret == RETCODE_SUCCESS) {
		info->pts = pEncInfo->ptsMap[info->encSrcIdx];
	} else {
		info->pts = 0LL;
	}

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return ret;
}

RetCode VPU_EncGiveCommand(EncHandle handle, CodecCommand cmd, void *param)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;
	switch (cmd) {
	case ENABLE_ROTATION: {
		pEncInfo->rotationEnable = 1;
	} break;
#ifdef REDUNDENT_CODE
	case DISABLE_ROTATION: {
		pEncInfo->rotationEnable = 0;
	} break;
#endif
	case ENABLE_MIRRORING: {
		pEncInfo->mirrorEnable = 1;
	} break;
#ifdef REDUNDENT_CODE
	case DISABLE_MIRRORING: {
		pEncInfo->mirrorEnable = 0;
	} break;
#endif
	case SET_MIRROR_DIRECTION: {
		MirrorDirection mirDir;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		mirDir = *(MirrorDirection *)param;
		if (!(mirDir == MIRDIR_NONE) && !(mirDir == MIRDIR_HOR) &&
		    !(mirDir == MIRDIR_VER) && !(mirDir == MIRDIR_HOR_VER)) {
			return RETCODE_INVALID_PARAM;
		}
		pEncInfo->mirrorDirection = mirDir;
	} break;
	case SET_ROTATION_ANGLE: {
		int angle;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		angle = *(int *)param;
		if (angle != 0 && angle != 90 && angle != 180 && angle != 270) {
			return RETCODE_INVALID_PARAM;
		}
		if (pEncInfo->initialInfoObtained &&
		    (angle == 90 || angle == 270)) {
			return RETCODE_INVALID_PARAM;
		}
		pEncInfo->rotationAngle = angle;
	} break;
	case SET_CACHE_CONFIG: {
		MaverickCacheConfig *mcCacheConfig;
		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		mcCacheConfig = (MaverickCacheConfig *)param;
		pEncInfo->cacheConfig = *mcCacheConfig;
	} break;
	case ENC_PUT_MP4_HEADER:
	case ENC_PUT_AVC_HEADER:
	case ENC_PUT_VIDEO_HEADER: {
		EncHeaderParam *encHeaderParam;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		encHeaderParam = (EncHeaderParam *)param;
		if (pCodecInst->codecMode == MP4_ENC ||
		    pCodecInst->codecMode == C7_MP4_ENC) {
			if (!(VOL_HEADER <= encHeaderParam->headerType &&
			      encHeaderParam->headerType <= VIS_HEADER)) {
				return RETCODE_INVALID_PARAM;
			}
		} else if (pCodecInst->codecMode == AVC_ENC ||
			   pCodecInst->codecMode == C7_AVC_ENC) {
			if (!(SPS_RBSP <= encHeaderParam->headerType &&
			      encHeaderParam->headerType <= SVC_RBSP_SEI)) {
				return RETCODE_INVALID_PARAM;
			}
		} else if (pCodecInst->codecMode == HEVC_ENC) {
			if (!(CODEOPT_ENC_VPS <= encHeaderParam->headerType &&
			      encHeaderParam->headerType <=
				      (CODEOPT_ENC_VPS | CODEOPT_ENC_SPS |
				       CODEOPT_ENC_PPS))) {
				return RETCODE_INVALID_PARAM;
			}
			if (pEncInfo->ringBufferEnable == 0) {
				if (encHeaderParam->buf % 16 ||
				    encHeaderParam->size == 0)
					return RETCODE_INVALID_PARAM;
			}
			if (encHeaderParam->headerType &
			    CODEOPT_ENC_VCL) // ENC_PUT_VIDEO_HEADER encode only
				// non-vcl header.
				return RETCODE_INVALID_PARAM;

		} else
			return RETCODE_INVALID_PARAM;

		if (pEncInfo->ringBufferEnable == 0) {
			if (encHeaderParam->buf % 8 ||
			    encHeaderParam->size == 0) {
				return RETCODE_INVALID_PARAM;
			}
		}
		if (pCodecInst->codecMode == HEVC_ENC) {
			return Wave4VpuEncGetHeader(handle, encHeaderParam);
		} else {
			return GetEncHeader(handle, encHeaderParam);
		}
	}
		break;
#ifdef REDUNDENT_CODE
	case ENC_SET_ACTIVE_PPS:
		int ActivePPSIdx = (int)(*(int *)param);
		if (pCodecInst->codecMode != AVC_ENC && pCodecInst->codecMode != C7_AVC_ENC)
			return RETCODE_INVALID_COMMAND;
		if (ActivePPSIdx < 0 || ActivePPSIdx > pEncInfo->openParam.EncStdParam.avcParam.ppsNum)
			return RETCODE_INVALID_COMMAND;

		pEncInfo->ActivePPSIdx = ActivePPSIdx;
		return EncParaSet(handle, PPS_RBSP);
	case ENC_GET_ACTIVE_PPS:
		if (pCodecInst->codecMode != AVC_ENC && pCodecInst->codecMode != C7_AVC_ENC)
			return RETCODE_INVALID_COMMAND;

		*((int *)param) = pEncInfo->ActivePPSIdx;
		break;
	case ENC_SET_GOP_NUMBER: {
		int *pGopNumber = (int *)param;
		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != AVC_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC &&
		    pCodecInst->codecMode != C7_AVC_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (*pGopNumber < 0)
			return RETCODE_INVALID_PARAM;
		pEncInfo->openParam.gopSize = *pGopNumber;
		SetGopNumber(handle, (Uint32 *)pGopNumber);
	} break;
	case ENC_SET_INTRA_QP: {
		int *pIntraQp = (int *)param;
		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != AVC_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC &&
		    pCodecInst->codecMode != C7_AVC_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (pCodecInst->codecMode == MP4_ENC ||
		    pCodecInst->codecMode == C7_MP4_ENC) {
			if (*pIntraQp < 1 || *pIntraQp > 31)
				return RETCODE_INVALID_PARAM;
		}
		if (pCodecInst->codecMode == AVC_ENC ||
		    pCodecInst->codecMode == C7_AVC_ENC) {
			if (*pIntraQp < 0 || *pIntraQp > 51)
				return RETCODE_INVALID_PARAM;
		}
		SetIntraQp(handle, (Uint32 *)pIntraQp);
	} break;
#endif
	case ENC_SET_BITRATE: {
		int *pBitrate = (int *)param;
		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != AVC_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC &&
		    pCodecInst->codecMode != C7_AVC_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (pCodecInst->codecMode == AVC_ENC ||
		    pCodecInst->codecMode == C7_AVC_ENC) {
			if (*pBitrate < 0 || *pBitrate > 524288) {
				return RETCODE_INVALID_PARAM;
			}

		} else {
			// MP4_ENC
			if (*pBitrate < 0 || *pBitrate > 32767) {
				return RETCODE_INVALID_PARAM;
			}
		}
		SetBitrate(handle, (Uint32 *)pBitrate);
	} break;
#ifdef REDUNDENT_CODE
	case ENC_SET_FRAME_RATE: {
		int *pFramerate = (int *)param;

		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != AVC_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC &&
		    pCodecInst->codecMode != C7_AVC_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		if (*pFramerate <= 0) {
			return RETCODE_INVALID_PARAM;
		}
		SetFramerate(handle, (Uint32 *)pFramerate);
	} break;
	case ENC_SET_INTRA_MB_REFRESH_NUMBER: {
		int *pIntraRefreshNum = (int *)param;
		SetIntraRefreshNum(handle, (Uint32 *)pIntraRefreshNum);
	} break;
#endif
	case ENC_SET_MIN_MAX_QP: {
		MinMaxQpChangeParam *minMapQpParam =
			(MinMaxQpChangeParam *)param;
		SetMinMaxQp(handle, minMapQpParam);
	} break;
#ifdef REDUNDENT_CODE
#if defined(RC_PIC_PARACHANGE) && defined(RC_CHANGE_PARAMETER_DEF)
	case ENC_SET_PIC_CHANGE_PARAM: {
		ChangePicParam *changePicParam = (ChangePicParam *)param;
		SetChangePicPara(handle, changePicParam);
	} break;
#endif
	case ENC_SET_SLICE_INFO: {
		EncSliceMode *pSliceMode = (EncSliceMode *)param;
		if (pSliceMode->sliceMode < 0 || pSliceMode->sliceMode > 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (pSliceMode->sliceSizeMode < 0 ||
		    pSliceMode->sliceSizeMode > 1) {
			return RETCODE_INVALID_PARAM;
		}

		SetSliceMode(handle, (EncSliceMode *)pSliceMode);
	} break;
	case ENC_ENABLE_HEC: {
		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		SetHecMode(handle, 1);
	} break;
	case ENC_DISABLE_HEC: {
		if (pCodecInst->codecMode != MP4_ENC &&
		    pCodecInst->codecMode != C7_MP4_ENC) {
			return RETCODE_INVALID_COMMAND;
		}
		SetHecMode(handle, 0);
	} break;
#endif
	case SET_SEC_AXI: {
		SecAxiUse secAxiUse;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}
		secAxiUse = *(SecAxiUse *)param;
		if (handle->productId == PRODUCT_ID_410 ||
		    handle->productId == PRODUCT_ID_4102 ||
		    handle->productId == PRODUCT_ID_420 ||
		    handle->productId == PRODUCT_ID_412 ||
		    handle->productId == PRODUCT_ID_420L ||
		    handle->productId == PRODUCT_ID_510 ||
		    handle->productId == PRODUCT_ID_512 ||
		    handle->productId == PRODUCT_ID_515) {
			if (handle->codecMode == HEVC_DEC) {
				pEncInfo->secAxiInfo.u.wave4.useIpEnable =
					secAxiUse.u.wave4.useIpEnable;
				pEncInfo->secAxiInfo.u.wave4.useLfRowEnable =
					secAxiUse.u.wave4.useLfRowEnable;
				pEncInfo->secAxiInfo.u.wave4.useBitEnable =
					secAxiUse.u.wave4.useBitEnable;
			} else {
				pEncInfo->secAxiInfo.u.wave4.useEncImdEnable =
					secAxiUse.u.wave4.useEncImdEnable;
				pEncInfo->secAxiInfo.u.wave4.useEncRdoEnable =
					secAxiUse.u.wave4.useEncRdoEnable;
				pEncInfo->secAxiInfo.u.wave4.useEncLfEnable =
					secAxiUse.u.wave4.useEncLfEnable;
			}
		} else if (handle->productId == PRODUCT_ID_520) {
			pEncInfo->secAxiInfo.u.wave4.useEncRdoEnable =
				secAxiUse.u.wave4.useEncRdoEnable;
			pEncInfo->secAxiInfo.u.wave4.useEncLfEnable =
				secAxiUse.u.wave4.useEncLfEnable;
		} else { // coda9 or coda7q or ...
			pEncInfo->secAxiInfo.u.coda9.useBitEnable =
				secAxiUse.u.coda9.useBitEnable;
			pEncInfo->secAxiInfo.u.coda9.useIpEnable =
				secAxiUse.u.coda9.useIpEnable;
			pEncInfo->secAxiInfo.u.coda9.useDbkYEnable =
				secAxiUse.u.coda9.useDbkYEnable;
			pEncInfo->secAxiInfo.u.coda9.useDbkCEnable =
				secAxiUse.u.coda9.useDbkCEnable;
			pEncInfo->secAxiInfo.u.coda9.useOvlEnable =
				secAxiUse.u.coda9.useOvlEnable;
			pEncInfo->secAxiInfo.u.coda9.useBtpEnable =
				secAxiUse.u.coda9.useBtpEnable;
		}
	} break;
	case GET_TILEDMAP_CONFIG: {
		TiledMapConfig *pMapCfg = (TiledMapConfig *)param;
		if (!pMapCfg) {
			return RETCODE_INVALID_PARAM;
		}
		*pMapCfg = pEncInfo->mapCfg;
		break;
	}
#ifdef REDUNDENT_CODE
	case SET_DRAM_CONFIG: {
		DRAMConfig *cfg = (DRAMConfig *)param;

		if (!cfg) {
			return RETCODE_INVALID_PARAM;
		}

		pEncInfo->dramCfg = *cfg;
		break;
	}
	case GET_DRAM_CONFIG:
		DRAMConfig *cfg = (DRAMConfig *)param;
		if (!cfg)
			return RETCODE_INVALID_PARAM;
		*cfg = pEncInfo->dramCfg;
		break;
#endif
#ifdef ENABLE_CNM_DEBUG_MSG
	case ENABLE_LOGGING:
		pCodecInst->loggingEnable = 1;
		break;
	case DISABLE_LOGGING:
		pCodecInst->loggingEnable = 0;
		break;
#endif
	case ENC_SET_PARA_CHANGE: {
		EncChangeParam *option = (EncChangeParam *)param;
		return Wave4VpuEncParaChange(handle, option);
	}
	case ENC_SET_SEI_NAL_DATA: {
		HevcSEIDataEnc *option = (HevcSEIDataEnc *)param;
		pEncInfo->prefixSeiNalEnable = option->prefixSeiNalEnable;
		pEncInfo->prefixSeiDataSize = option->prefixSeiDataSize;
		pEncInfo->prefixSeiDataEncOrder = option->prefixSeiDataEncOrder;
		pEncInfo->prefixSeiNalAddr = option->prefixSeiNalAddr;

		pEncInfo->suffixSeiNalEnable = option->suffixSeiNalEnable;
		pEncInfo->suffixSeiDataSize = option->suffixSeiDataSize;
		pEncInfo->suffixSeiDataEncOrder = option->suffixSeiDataEncOrder;
		pEncInfo->suffixSeiNalAddr = option->suffixSeiNalAddr;
	} break;
#ifdef SUPPORT_W5ENC_BW_REPORT
	case ENC_GET_BW_REPORT: {
		EncBwMonitor *encBwMon;

		if (param == 0) {
			return RETCODE_INVALID_PARAM;
		}

		if (handle->productId != PRODUCT_ID_520)
			return RETCODE_INVALID_COMMAND;

		encBwMon = (EncBwMonitor *)param;
		return Wave5VpuEncGetBwReport(handle, encBwMon);
	} break;
#endif
	case ENC_GET_FRAMEBUF_IDX: {
		Int32 *srcFrameIdx = (Int32 *)param;
		EncInfo *pEncInfo;
		CodecInst *pCodecInst;

		if (!srcFrameIdx) {
			return RETCODE_INVALID_PARAM;
		}

		pCodecInst = handle;
		pEncInfo = &pCodecInst->CodecInfo->encInfo;

		dpb_pic_init(pEncInfo); //, NULL, NULL);
		*srcFrameIdx = pEncInfo->prev_idx;
	} break;
	default:
		return RETCODE_INVALID_COMMAND;
	}
	return RETCODE_SUCCESS;
}

RetCode VPU_EncAllocateFrameBuffer(EncHandle handle, FrameBufferAllocInfo info,
				   FrameBuffer *frameBuffer)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;
	int gdiIndex;

	CVI_VC_TRACE("\n");

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	if (!frameBuffer) {
		return RETCODE_INVALID_PARAM;
	}
	if (info.num == 0 || info.num < 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (info.stride == 0 || info.stride < 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (info.height == 0 || info.height < 0) {
		return RETCODE_INVALID_PARAM;
	}

	if (info.type == FB_TYPE_PPU) {
		if (pEncInfo->numFrameBuffers == 0) {
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
		pEncInfo->ppuAllocExt = frameBuffer[0].updateFbInfo;
		gdiIndex = pEncInfo->numFrameBuffers;
		ret = ProductVpuAllocateFramebuffer(
			pCodecInst, frameBuffer, (TiledMapType)info.mapType,
			(Int32)info.num, info.stride, info.height, info.format,
			info.cbcrInterleave, info.nv21, info.endian,
			&pEncInfo->vbPPU, gdiIndex,
			(FramebufferAllocType)info.type);
	} else if (info.type == FB_TYPE_CODEC) {
		gdiIndex = 0;
		pEncInfo->frameAllocExt = frameBuffer[0].updateFbInfo;
		ret = ProductVpuAllocateFramebuffer(
			pCodecInst, frameBuffer, (TiledMapType)info.mapType,
			(Int32)info.num, info.stride, info.height, info.format,
			info.cbcrInterleave, FALSE, info.endian,
			&pEncInfo->vbFrame, gdiIndex,
			(FramebufferAllocType)info.type);
	} else {
		ret = RETCODE_INVALID_PARAM;
	}

	return ret;
}

#ifdef REDUNDENT_CODE
RetCode VPU_EncIssueSeqInit(EncHandle handle)
{
	CodecInst *pCodecInst;
	RetCode ret;
	VpuAttr *pAttr;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (GetPendingInst(pCodecInst->coreIdx)) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	ret = ProductVpuEncInitSeq(handle);
	if (ret == RETCODE_SUCCESS) {
		SetPendingInst(pCodecInst->coreIdx, pCodecInst, __func__,
			       __LINE__);
	}

	if (pAttr->supportCommandQueue == TRUE) {
		SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);
	}

	return ret;
}

RetCode VPU_EncCompleteSeqInit(EncHandle handle, EncInitialInfo *info)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	RetCode ret;
	VpuAttr *pAttr;

	ret = CheckEncInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

	if (pAttr->supportCommandQueue == FALSE) {
		if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
			SetPendingInst(pCodecInst->coreIdx, NULL, __func__,
				       __LINE__);
			CVI_VC_ERR("SetPendingInst clear\n");
			return RETCODE_WRONG_CALL_SEQUENCE;
		}
	}

	ret = ProductVpuEncGetSeqInfo(handle);
	if (ret == RETCODE_SUCCESS) {
		pEncInfo->initialInfoObtained = 1;
	}

	// info->rdPtr = VpuReadReg(pCodecInst->coreIdx,
	// pEncInfo->streamRdPtrRegAddr); info->wrPtr =
	// VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);

	// pEncInfo->prevFrameEndPos = info->rdPtr;

	pEncInfo->initialInfo = *info;

	SetPendingInst(pCodecInst->coreIdx, NULL, __func__, __LINE__);

	return ret;
}
#endif

RetCode VPU_DecSetOutputFormat(DecHandle handle, int cbcrInterleave, int nv21)
{
	CodecInst *pCodecInst = (CodecInst *)handle;
	DecOpenParam *pdop;

	if (pCodecInst == NULL) {
		CVI_VC_ERR("Null pointer to codec instance\n");
		return RETCODE_INVALID_HANDLE;
	}

	pdop = &pCodecInst->CodecInfo->decInfo.openParam;
	pdop->cbcrInterleave = cbcrInterleave;
	pdop->nv21 = nv21;

	return RETCODE_SUCCESS;
}
