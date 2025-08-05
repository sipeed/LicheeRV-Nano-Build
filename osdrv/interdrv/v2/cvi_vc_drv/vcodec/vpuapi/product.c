//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//  This confidential and proprietary software may be used only
//  as authorized by a licensing agreement from Chips&Media Inc.
//  In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//   The entire notice above must be reproduced on all authorized copies.
//
//--=========================================================================--
#include "product.h"
#include "wave/common/common.h"
#include "wave/wave4/wave4.h"
#include "coda9/coda9.h"

#define UNREFERENCED_PARAM(x) ((void)(x))

extern void vpu_set_channel_core_mapping(int chnIdx, int coreIdx);

VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

static Int32 s_ProductIds[MAX_NUM_VPU_CORE] = { [0 ... MAX_NUM_VPU_CORE - 1] =
							PRODUCT_ID_NONE };

typedef struct FrameBufInfoStruct {
	Uint32 unitSizeHorLuma;
	Uint32 sizeLuma;
	Uint32 sizeChroma;
	BOOL fieldMap;
} FrameBufInfo;

Uint32 ProductVpuScan(Uint32 coreIdx)
{
	Uint32 productId = 0;
	Uint32 foundProducts = 0;

	/* Already scanned */
	if (s_ProductIds[coreIdx] != PRODUCT_ID_NONE)
		return 1;

	if (CORE_H265 == coreIdx) {
		productId = WaveVpuGetProductId(CORE_H265);
	} else if (CORE_H264 == coreIdx) {
		productId = Coda9VpuGetProductId(CORE_H264);
	}

	if (productId != PRODUCT_ID_NONE) {
		s_ProductIds[coreIdx] = productId;
		foundProducts++;
	}

	return (foundProducts >= 1);
}

Int32 ProductVpuGetId(Uint32 coreIdx)
{
	return s_ProductIds[coreIdx];
}

void ProductVpuSetId(Uint32 coreIdx, Uint32 productId)
{
	CVI_VC_INFO("set productId %d\n", productId);

	s_ProductIds[coreIdx] = productId;
}

RetCode ProductVpuGetVersion(Uint32 coreIdx, Uint32 *versionInfo,
			     Uint32 *revision)
{
	Int32 productId = s_ProductIds[coreIdx];
	RetCode ret = RETCODE_SUCCESS;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuGetVersion(coreIdx, versionInfo, revision);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuGetVersion(coreIdx, versionInfo, revision);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}
#ifdef DRAM_TEST
RetCode ProductVpuDRAMReadWriteTest(Uint32 coreIdx, Uint32 *dram_source_addr,
				    Uint32 *dram_destination_addr,
				    Uint32 *dram_data_size)
{
	Int32 productId = s_ProductIds[coreIdx];
	RetCode ret = RETCODE_SUCCESS;

	switch (productId) {
	case PRODUCT_ID_960:
	case PRODUCT_ID_980:
		break;
	case PRODUCT_ID_410:
	case PRODUCT_ID_4102:
	case PRODUCT_ID_510:
		ret = Wave4VpuDRAMReadWriteTest(coreIdx, dram_source_addr,
						dram_destination_addr,
						dram_data_size);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}
#endif

RetCode ProductVpuInit(Uint32 coreIdx, void *firmware, Uint32 size)
{
	RetCode ret = RETCODE_SUCCESS;
	int productId;

	CVI_VC_INFO("\n");

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuInit(coreIdx, firmware, size);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuInit(coreIdx, firmware, size);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuReInit(Uint32 coreIdx, void *firmware, Uint32 size)
{
	RetCode ret = RETCODE_SUCCESS;
	int productId;

	CVI_VC_INFO("\n");

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuReInit(coreIdx, firmware, size);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuReInit(coreIdx, firmware, size);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

Uint32 ProductVpuIsInit(Uint32 coreIdx)
{
	Uint32 pc = 0;
	int productId;

	productId = s_ProductIds[coreIdx];

	if (productId == PRODUCT_ID_NONE) {
		ProductVpuScan(coreIdx);
		productId = s_ProductIds[coreIdx];
	}

	switch (productId) {
	case PRODUCT_ID_980:
		pc = Coda9VpuIsInit(coreIdx);
		break;
	case PRODUCT_ID_420L:
		pc = Wave4VpuIsInit(coreIdx);
		break;
	}

	return pc;
}

Int32 ProductVpuIsBusy(Uint32 coreIdx)
{
	Int32 busy;
	int productId;

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		busy = Coda9VpuIsBusy(coreIdx);
		break;
	case PRODUCT_ID_420L:
		busy = Wave4VpuIsBusy(coreIdx);
		break;
	default:
		busy = 0;
		break;
	}

	return busy;
}

Int32 ProductVpuGetFrameCycle(Uint32 coreIdx)
{
	int productId;
	int flag = -1;

	productId = s_ProductIds[coreIdx];

	CVI_VC_BS("\n");

	switch (productId) {
	case PRODUCT_ID_980:
		flag = Coda9VpuGetFrameCycle(coreIdx);
		break;
	case PRODUCT_ID_420L:
		flag = Wave4VpuGetFrameCycle(coreIdx);
		break;
	default:
		flag = -1;
		break;
	}

	return flag;
}

Int32 ProductVpuWaitInterrupt(CodecInst *instance, Int32 timeout)
{
	int productId;
	int flag = -1;

	productId = s_ProductIds[instance->coreIdx];

	CVI_VC_BS("\n");

	switch (productId) {
	case PRODUCT_ID_980:
		flag = Coda9VpuWaitInterrupt(instance, timeout);
		break;
	case PRODUCT_ID_420L:
		flag = Wave4VpuWaitInterrupt(instance, timeout);
		break;
	default:
		flag = -1;
		break;
	}

	return flag;
}

#if 0
Int32 ProductVpuGetFd(CodecInst *instance)
{
	int productId;
	int fd = -1;

	productId = s_ProductIds[instance->coreIdx];

	switch (productId) {
	case PRODUCT_ID_960:
	case PRODUCT_ID_980:
		break;
	case PRODUCT_ID_420L:
		fd = Wave4VpuGetFd(instance);
		break;
	default:
		fd = -1;
		break;
	}

	return fd;
}
#endif

RetCode ProductVpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
	int productId;
	RetCode ret = RETCODE_SUCCESS;

	CVI_VC_INFO("resetMode %d\n", resetMode);

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuReset(coreIdx, resetMode);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuReset(coreIdx, resetMode);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

#ifdef REDUNDENT_CODE
RetCode ProductVpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16 *code,
			    Uint32 size)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	CVI_VC_INFO("iSleepWake %d\n", iSleepWake);

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuSleepWake(coreIdx, iSleepWake, (void *)code,
					size);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuSleepWake(coreIdx, iSleepWake, size);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}
#endif

RetCode ProductVpuSleepWake_EX(Uint32 coreIdx, int iSleepWake,
			       const Uint16 *code, Uint32 size)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	Uint64 start_time, end_time;

	start_time = cviGetCurrentTime();

	CVI_VC_LOCK("iSleepWake %d\n", iSleepWake);

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuSleepWake_EX(coreIdx, iSleepWake, (void *)code,
					   size);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuSleepWake_EX(coreIdx, iSleepWake, size);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	end_time = cviGetCurrentTime();
	CVI_VC_PERF("SleepWake(%d) = %llu us\n", iSleepWake,
		    end_time - start_time);

	return ret;
}

RetCode ProductVpuClearInterrupt(Uint32 coreIdx)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuClearInterrupt(coreIdx);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuClearInterrupt(coreIdx, 0xffff);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuDecBuildUpOpenParam(CodecInst *pCodec, DecOpenParam *param)
{
	Int32 productId;
	Uint32 coreIdx;
	RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

	coreIdx = pCodec->coreIdx;
	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuBuildUpDecParam(pCodec, param);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuBuildUpDecParam(pCodec, param);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuEncBuildUpOpenParam(CodecInst *pCodec, EncOpenParam *param)
{
	Int32 productId;
	Uint32 coreIdx;
	RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

	coreIdx = pCodec->coreIdx;
	productId = s_ProductIds[coreIdx];

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuBuildUpEncParam(pCodec, param);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuBuildUpEncParam(pCodec, param);
		break;
	default:
		ret = RETCODE_NOT_SUPPORTED_FEATURE;
	}

	return ret;
}

RetCode ProductCheckDecOpenParam(DecOpenParam *param)
{
	Int32 productId;
	Uint32 coreIdx;
	VpuAttr *pAttr;

	if (param == 0) {
		CVI_VC_ERR("param = 0\n");
		return RETCODE_INVALID_PARAM;
	}

	if (param->coreIdx > MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("coreIdx > MAX_NUM_VPU_CORE\n");
		return RETCODE_INVALID_PARAM;
	}

	coreIdx = param->coreIdx;
	productId = s_ProductIds[coreIdx];
	pAttr = &g_VpuCoreAttributes[coreIdx];

	if (param->bitstreamBuffer % 8) {
		CVI_VC_ERR("bitstreamBuffer %% 8 != 0\n");
		return RETCODE_INVALID_PARAM;
	}

	if (param->bitstreamMode == BS_MODE_INTERRUPT) {
		if (param->bitstreamBufferSize % 1024 ||
		    param->bitstreamBufferSize < 1024) {
			CVI_VC_ERR("bitstreamBufferSize\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	if (PRODUCT_ID_W_SERIES(productId)) {
		if (param->virtAxiID > 16) {
			// Maximum number of AXI channels is 15
			return RETCODE_INVALID_PARAM;
		}
	}

	// Check bitstream mode
	if ((pAttr->supportBitstreamMode & (1 << param->bitstreamMode)) == 0) {
		CVI_VC_ERR("bitstreamMode\n");
		return RETCODE_INVALID_PARAM;
	}

	if ((pAttr->supportDecoders & (1 << param->bitstreamFormat)) == 0) {
		CVI_VC_ERR("bitstreamFormat\n");
		return RETCODE_INVALID_PARAM;
	}

	/* check framebuffer endian */
	if ((pAttr->supportEndianMask & (1 << param->frameEndian)) == 0) {
		CVI_VC_ERR("Invalid frame endian\n");
		APIDPRINT("%s:%d Invalid frame endian(%d)\n",
			  (Int32)param->frameEndian);
		return RETCODE_INVALID_PARAM;
	}

	/* check streambuffer endian */
	if ((pAttr->supportEndianMask & (1 << param->streamEndian)) == 0) {
		CVI_VC_ERR("streamEndian\n");
		APIDPRINT("%s:%d Invalid stream endian(%d)\n",
			  (Int32)param->streamEndian);
		return RETCODE_INVALID_PARAM;
	}

	/* check WTL */
	if (param->wtlEnable) {
		if (pAttr->supportWTL == 0)
			return RETCODE_NOT_SUPPORTED_FEATURE;
		switch (productId) {
		case PRODUCT_ID_960:
		case PRODUCT_ID_980:
		case PRODUCT_ID_320:
			if (param->wtlMode != FF_FRAME &&
			    param->wtlMode != FF_FIELD) {
				CVI_VC_ERR("wtlMode\n");
				return RETCODE_INVALID_PARAM;
			}
		default:
			break;
		}
	}

	/* Tiled2Linear */
	if (param->tiled2LinearEnable) {
		if (pAttr->supportTiled2Linear == 0) {
			CVI_VC_ERR("supportTiled2Linear == 0\n");
			return RETCODE_NOT_SUPPORTED_FEATURE;
		}

		if (productId == PRODUCT_ID_960 ||
		    productId == PRODUCT_ID_980 ||
		    productId == PRODUCT_ID_320) {
			if (param->tiled2LinearMode != FF_FRAME &&
			    param->tiled2LinearMode != FF_FIELD) {
				CVI_VC_ERR("tiled2LinearMode\n");
				APIDPRINT(
					"%s:%d Invalid Tiled2LinearMode(%d)\n",
					(Int32)param->tiled2LinearMode);
				return RETCODE_INVALID_PARAM;
			}
		}
	}
	if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980 ||
	    productId == PRODUCT_ID_7Q) {
		if (param->mp4DeblkEnable == 1 &&
		    !(param->bitstreamFormat == STD_MPEG4 ||
		      param->bitstreamFormat == STD_H263 ||
		      param->bitstreamFormat == STD_MPEG2 ||
		      param->bitstreamFormat == STD_DIV3)) {
			CVI_VC_ERR("bitstreamFormat\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->wtlEnable && param->tiled2LinearEnable) {
			CVI_VC_ERR("wtlEnable / tiled2LinearEnable\n");
			return RETCODE_INVALID_PARAM;
		}
	}
#ifdef REDUNDENT_CODE
	else {
		if (param->mp4DeblkEnable || param->mp4Class) {
			CVI_VC_ERR("mp4DeblkEnable / mp4Class\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->avcExtension) {
			CVI_VC_ERR("avcExtension\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->tiled2LinearMode != FF_NONE) {
			CVI_VC_ERR("tiled2LinearMode\n");
			return RETCODE_INVALID_PARAM;
		}
	}
#endif

	return RETCODE_SUCCESS;
}

RetCode ProductVpuDecInitSeq(CodecInst *instance)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = instance->productId;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuDecInitSeq(instance);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecInitSeq(instance);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuDecFiniSeq(CodecInst *instance)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = instance->productId;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuFiniSeq(instance);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecFiniSeq(instance);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuDecGetSeqInfo(CodecInst *instance, DecInitialInfo *info)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = instance->productId;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuDecGetSeqInfo(instance, info);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecGetSeqInfo(instance, info);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuDecCheckCapability(CodecInst *instance)
{
	DecInfo *pDecInfo;
	VpuAttr *pAttr = &g_VpuCoreAttributes[instance->coreIdx];

	pDecInfo = &instance->CodecInfo->decInfo;

	if ((pAttr->supportDecoders &
	     (1 << pDecInfo->openParam.bitstreamFormat)) == 0)
		return RETCODE_NOT_SUPPORTED_FEATURE;

	switch (instance->productId) {
	case PRODUCT_ID_980:
		if (pDecInfo->mapType >= COMPRESSED_FRAME_MAP)
			return RETCODE_NOT_SUPPORTED_FEATURE;
		break;
	case PRODUCT_ID_420L:
		if (pDecInfo->mapType != LINEAR_FRAME_MAP &&
		    pDecInfo->mapType != COMPRESSED_FRAME_MAP)
			return RETCODE_NOT_SUPPORTED_FEATURE;
		break;
	}

	return RETCODE_SUCCESS;
}

RetCode ProductVpuDecode(CodecInst *instance, DecParam *option)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	vpu_set_channel_core_mapping(instance->s32ChnNum, instance->coreIdx);

	productId = instance->productId;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuDecode(instance, option);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecode(instance, option);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

RetCode ProductVpuDecGetResult(CodecInst *instance, DecOutputInfo *result)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	vpu_set_channel_core_mapping(instance->s32ChnNum, -1);

	productId = instance->productId;

	switch (productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuDecGetResult(instance, result);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecGetResult(instance, result);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	result->decHwTime = instance->u64EndTime - instance->u64StartTime;
	CVI_VC_PERF("decHwTime %llu\n", result->decHwTime);

	return ret;
}

RetCode ProductVpuDecFlush(CodecInst *instance, FramebufferIndex *retIndexes,
			   Uint32 size)
{
	RetCode ret = RETCODE_SUCCESS;

	switch (instance->productId) {
	case PRODUCT_ID_420L:
		ret = Wave4VpuDecFlush(instance, retIndexes, size);
		break;
	default:
		ret = Coda9VpuDecFlush(instance, retIndexes, size);
		break;
	}

	return ret;
}

/************************************************************************/
/* Decoder & Encoder                                                    */
/************************************************************************/

RetCode ProductVpuDecSetBitstreamFlag(CodecInst *instance, BOOL running,
				      Int32 size)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	BOOL eos;
	BOOL checkEos;
	BOOL explicitEnd;
	DecInfo *pDecInfo = &instance->CodecInfo->decInfo;

	productId = instance->productId;

	eos = (BOOL)(size == 0);
	checkEos = (BOOL)(size > 0);
	explicitEnd = (BOOL)(size == -2);

	switch (productId) {
	case PRODUCT_ID_980:
		if (checkEos)
			eos = (BOOL)((pDecInfo->streamEndflag & 0x04) == 0x04);
		ret = Coda9VpuDecSetBitstreamFlag(instance, running, eos);
		break;
	case PRODUCT_ID_420L:
		if (checkEos)
			eos = (BOOL)pDecInfo->streamEndflag;
		ret = Wave4VpuDecSetBitstreamFlag(instance, running, eos,
						  explicitEnd);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return ret;
}

#ifdef REDUNDENT_CODE
RetCode ProductCpbFlush(CodecInst *instance)
{
	RetCode ret;

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuDecCpbFlush(instance);
		break;
	default:
		ret = RETCODE_NOT_SUPPORTED_FEATURE;
		break;
	}

	return ret;
}
#endif

/**
 * \param   stride          stride of framebuffer in pixel.
 */
RetCode ProductVpuAllocateFramebuffer(
	CodecInst *inst, FrameBuffer *fbArr, TiledMapType mapType, Int32 num,
	Int32 stride, Int32 height, FrameBufferFormat format,
	BOOL cbcrInterleave, BOOL nv21, Int32 endian, vpu_buffer_t *vb,
	Int32 gdiIndex, FramebufferAllocType fbType)
{
	Int32 i;
#ifdef REDUNDENT_CODE
	Uint32 coreIdx;
#endif
	vpu_buffer_t vbFrame;
	FrameBufInfo fbInfo;
	DecInfo *pDecInfo = &inst->CodecInfo->decInfo;
	EncInfo *pEncInfo = &inst->CodecInfo->encInfo;
	// Variables for TILED_FRAME/FILED_MB_RASTER
	Uint32 sizeLuma;
	Uint32 sizeChroma;
#ifdef REDUNDENT_CODE
	ProductId productId = (ProductId)inst->productId;
#endif
	RetCode ret = RETCODE_SUCCESS;

	osal_memset((void *)&vbFrame, 0x00, sizeof(vpu_buffer_t));
	osal_memset((void *)&fbInfo, 0x00, sizeof(FrameBufInfo));

#ifdef REDUNDENT_CODE
	coreIdx = inst->coreIdx;
#else
	UNREFERENCED_PARAM(vb);
	UNREFERENCED_PARAM(fbType);
#endif

	if (inst->codecMode == AVC_ENC)
		format = pEncInfo->openParam.EncStdParam.avcParam
					 .chromaFormat400 ?
				       FORMAT_400 :
				       FORMAT_420;
#ifdef REDUNDENT_CODE
	if (inst->codecMode == C7_VP9_DEC) {
		Uint32 framebufHeight = VPU_ALIGN64(height);
		sizeLuma = CalcLumaSize(inst->productId, stride, framebufHeight,
					cbcrInterleave, mapType, NULL);
		sizeChroma =
			CalcChromaSize(inst->productId, stride, framebufHeight,
				       format, cbcrInterleave, mapType, NULL);
	} else
#endif
	{
		DRAMConfig *dramConfig = NULL;
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			dramConfig = &pDecInfo->dramCfg;
			dramConfig = (inst->isDecoder == TRUE) ?
						   &pDecInfo->dramCfg :
						   &pEncInfo->dramCfg;
		}
#endif
		sizeLuma = CalcLumaSize(inst->productId, stride, height,
					cbcrInterleave, mapType, dramConfig);
		sizeChroma =
			CalcChromaSize(inst->productId, stride, height, format,
				       cbcrInterleave, mapType, dramConfig);
	}

	// Framebuffer common informations
	for (i = 0; i < num; i++) {
		if (fbArr[i].updateFbInfo == TRUE) {
			fbArr[i].updateFbInfo = FALSE;
			fbArr[i].myIndex = i + gdiIndex;
			CVI_VC_TRACE("fbArr[%d], myIndex = %d\n", i,
				     fbArr[i].myIndex);

			fbArr[i].stride = stride;
			fbArr[i].height = height;
			fbArr[i].mapType = mapType;
			fbArr[i].format = format;
			fbArr[i].cbcrInterleave =
				(mapType == COMPRESSED_FRAME_MAP ?
					       TRUE :
					       cbcrInterleave);
			fbArr[i].nv21 = nv21;
			fbArr[i].endian = endian;
			fbArr[i].lumaBitDepth =
				pDecInfo->initialInfo.lumaBitdepth;
			fbArr[i].chromaBitDepth =
				pDecInfo->initialInfo.chromaBitdepth;
			fbArr[i].sourceLBurstEn = FALSE;
			if (inst->codecMode == HEVC_ENC) {
				if (gdiIndex != 0) { // FB_TYPE_PPU
					fbArr[i].srcBufState =
						SRC_BUFFER_ALLOCATED;
				}
				fbArr[i].lumaBitDepth =
					pEncInfo->openParam.EncStdParam
						.hevcParam.internalBitDepth;
				fbArr[i].chromaBitDepth =
					pEncInfo->openParam.EncStdParam
						.hevcParam.internalBitDepth;
			}
		}
	}

	switch (mapType) {
	case LINEAR_FRAME_MAP:
	case LINEAR_FIELD_MAP:
	case COMPRESSED_FRAME_MAP:
		ret = AllocateLinearFrameBuffer(mapType, fbArr, num, sizeLuma,
						sizeChroma);
		break;

	default:
		/* Tiled map */
#ifdef REDUNDENT_CODE
		if (productId == PRODUCT_ID_960) {
			DRAMConfig *pDramCfg;
			PhysicalAddress tiledBaseAddr = 0;
			TiledMapConfig *pMapCfg;

			pDramCfg = (inst->isDecoder == TRUE) ?
						 &pDecInfo->dramCfg :
						 &pEncInfo->dramCfg;
			pMapCfg = (inst->isDecoder == TRUE) ?
						&pDecInfo->mapCfg :
						&pEncInfo->mapCfg;
			vbFrame.phys_addr =
				GetTiledFrameBase(coreIdx, fbArr, num);
			if (fbType == FB_TYPE_PPU) {
				tiledBaseAddr = pMapCfg->tiledBaseAddr;
			} else {
				pMapCfg->tiledBaseAddr = vbFrame.phys_addr;
				tiledBaseAddr = vbFrame.phys_addr;
			}
			*vb = vbFrame;
			ret = AllocateTiledFrameBufferGdiV1(
				mapType, tiledBaseAddr, fbArr, num, sizeLuma,
				sizeChroma, pDramCfg);
		} else
#endif
		{
			// PRODUCT_ID_980
			ret = AllocateTiledFrameBufferGdiV2(
				mapType, fbArr, num, sizeLuma, sizeChroma);
		}
		break;
	}
	for (i = 0; i < num; i++) {
		if (inst->codecMode == HEVC_ENC) {
			if (gdiIndex != 0) { // FB_TYPE_PPU
				APIDPRINT(
					"SOURCE FB[%02d] Y(0x%08x), Cb(0x%08x), Cr(0x%08x)\n",
					i, fbArr[i].bufY, fbArr[i].bufCb,
					fbArr[i].bufCr);
			}
		}
	}
	return ret;
}

RetCode ProductVpuRegisterFramebuffer(CodecInst *instance)
{
	RetCode ret = RETCODE_FAILURE;
	FrameBuffer *fb;
	DecInfo *pDecInfo = &instance->CodecInfo->decInfo;
	Int32 gdiIndex = 0;
	EncInfo *pEncInfo = &instance->CodecInfo->encInfo;

	switch (instance->productId) {
	case PRODUCT_ID_980:
		if (IS_DECODER_HANDLE(instance))
			ret = Coda9VpuDecRegisterFramebuffer(instance);
		else
			ret = Coda9VpuEncRegisterFramebuffer(instance);
		break;
	case PRODUCT_ID_420L:
		if (instance->codecMode == HEVC_DEC) {
			if (pDecInfo->mapType != COMPRESSED_FRAME_MAP)
				return RETCODE_NOT_SUPPORTED_FEATURE;

			fb = pDecInfo->frameBufPool;

			gdiIndex = 0;
			if (pDecInfo->wtlEnable == TRUE) {
				if (fb[0].mapType == COMPRESSED_FRAME_MAP)
					gdiIndex = pDecInfo->numFbsForDecoding;
				ret = Wave4VpuDecRegisterFramebuffer(
					instance, &fb[gdiIndex],
					LINEAR_FRAME_MAP,
					pDecInfo->numFbsForWTL);
				if (ret != RETCODE_SUCCESS)
					return ret;
				gdiIndex = gdiIndex == 0 ?
							 pDecInfo->numFbsForDecoding :
							 0;
			}
			ret = Wave4VpuDecRegisterFramebuffer(
				instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP,
				pDecInfo->numFbsForDecoding);
			if (ret != RETCODE_SUCCESS)
				return ret;
#ifdef REDUNDENT_CODE
		} else if (instance->codecMode == C7_VP9_DEC) {
			if (pDecInfo->mapType != LINEAR_FRAME_MAP &&
			    pDecInfo->mapType != COMPRESSED_FRAME_MAP &&
			    pDecInfo->mapType != ARM_COMPRESSED_FRAME_MAP)
				return RETCODE_NOT_SUPPORTED_FEATURE;

			fb = pDecInfo->frameBufPool;

			gdiIndex = 0;
			if (pDecInfo->wtlEnable == TRUE) {
				if (fb[0].mapType == COMPRESSED_FRAME_MAP)
					gdiIndex = pDecInfo->numFbsForDecoding;
				ret = Wave4VpuDecRegisterFramebuffer(
					instance, &fb[gdiIndex],
					LINEAR_FRAME_MAP,
					pDecInfo->numFbsForWTL);
				if (ret != RETCODE_SUCCESS)
					return ret;
				gdiIndex = gdiIndex == 0 ?
							 pDecInfo->numFbsForDecoding :
							 0;
			}
			ret = Wave4VpuDecRegisterFramebuffer(
				instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP,
				pDecInfo->numFbsForDecoding);
			if (ret != RETCODE_SUCCESS)
				return ret;
#endif
		} else {
			// ********************************************
			//    HEVC_ENC
			//*********************************************
			if (pEncInfo->mapType != COMPRESSED_FRAME_MAP)
				return RETCODE_NOT_SUPPORTED_FEATURE;

			fb = pEncInfo->frameBufPool;

			gdiIndex = 0;
			ret = Wave4VpuEncRegisterFramebuffer(
				instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP,
				pEncInfo->numFrameBuffers);

			if (ret != RETCODE_SUCCESS)
				return ret;
		}

		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}
	return ret;
}

#ifdef REDUNDENT_CODE
RetCode ProductVpuDecUpdateFrameBuffer(void)
{
	RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

	return ret;
}
#endif

Int32 ProductCalculateFrameBufSize(Int32 productId, Int32 stride, Int32 height,
				   TiledMapType mapType,
				   FrameBufferFormat format, BOOL interleave,
				   DRAMConfig *pDramCfg)
{
	Int32 size_dpb_lum, size_dpb_chr, size_dpb_all;
#ifdef REDUNDENT_CODE
	Int32 size_mvcolbuf = 0;
#endif

	size_dpb_lum = CalcLumaSize(productId, stride, height, interleave,
				    mapType, pDramCfg);
	size_dpb_chr = CalcChromaSize(productId, stride, height, format,
				      interleave, mapType, pDramCfg);
	size_dpb_all = size_dpb_lum + size_dpb_chr * 2;

	CVI_VC_MEM(
		"size_dpb_lum = 0x%X, size_dpb_chr = 0x%X, size_dpb_all = 0x%X, productId = %d\n",
		size_dpb_lum, size_dpb_chr, size_dpb_all, productId);

#ifdef REDUNDENT_CODE
	if (productId == PRODUCT_ID_320) {
		size_mvcolbuf = VPU_ALIGN32(stride) * VPU_ALIGN32(height);
		size_mvcolbuf = (size_mvcolbuf * 3) >> 1;
		size_mvcolbuf = (size_mvcolbuf + 4) / 5;
		size_mvcolbuf = VPU_ALIGN8(size_mvcolbuf);
		size_dpb_all += size_mvcolbuf;
	}
#endif

	return size_dpb_all;
}

Int32 ProductCalculateAuxBufferSize(AUX_BUF_TYPE type, CodStd codStd,
				    Int32 width, Int32 height)
{
	Int32 size = 0;

	switch (type) {
	case AUX_BUF_TYPE_MVCOL:
		if (codStd == STD_AVC || codStd == STD_VC1 ||
		    codStd == STD_MPEG4 || codStd == STD_H263 ||
		    codStd == STD_RV || codStd == STD_AVS) {
			size = VPU_ALIGN32(width) * VPU_ALIGN32(height);
			size = (size * 3) >> 1;
			size = (size + 4) / 5;
			size = ((size + 7) >> 3) << 3;
		} else if (codStd == STD_HEVC) {
			size = WAVE4_DEC_HEVC_MVCOL_BUF_SIZE(width, height);
#ifdef REDUNDENT_CODE
		} else if (codStd == STD_VP9) {
			size = WAVE4_DEC_VP9_MVCOL_BUF_SIZE(width, height);
#endif
		} else {
			size = 0;
		}
		break;
	case AUX_BUF_TYPE_FBC_Y_OFFSET:
		size = WAVE4_FBC_LUMA_TABLE_SIZE(width, height);
		break;
	case AUX_BUF_TYPE_FBC_C_OFFSET:
		size = WAVE4_FBC_CHROMA_TABLE_SIZE(width, height);
		break;
	}

	return size;
}

RetCode ProductClrDispFlag(void)
{
	RetCode ret = RETCODE_SUCCESS;
	return ret;
}

#ifdef REDUNDENT_CODE
RetCode ProductSetDispFlag(void)
{
	RetCode ret = RETCODE_SUCCESS;
	return ret;
}
#endif

/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
RetCode ProductCheckEncOpenParam(EncOpenParam *pop)
{
	Int32 coreIdx;
	Int32 picWidth;
	Int32 picHeight;
	Int32 productId;
	VpuAttr *pAttr;

	if (pop == 0)
		return RETCODE_INVALID_PARAM;

	if (pop->coreIdx > MAX_NUM_VPU_CORE)
		return RETCODE_INVALID_PARAM;

	coreIdx = pop->coreIdx;
	picWidth = pop->picWidth;
	picHeight = pop->picHeight;
	productId = s_ProductIds[coreIdx];
	pAttr = &g_VpuCoreAttributes[coreIdx];

	if ((pAttr->supportEncoders & (1 << pop->bitstreamFormat)) == 0) {
		CVI_VC_ERR(
			"supportEncoders, supportEncoders = %d, bitstreamFormat = %d\n",
			pAttr->supportEncoders, pop->bitstreamFormat);
		return RETCODE_NOT_SUPPORTED_FEATURE;
	}

	if (pop->ringBufferEnable == TRUE) {
		if (pop->bitstreamBuffer % 8) {
			CVI_VC_ERR("bitstreamBuffer %% 8\n");
			return RETCODE_INVALID_PARAM;
		}

		if (productId == PRODUCT_ID_420 ||
		    productId == PRODUCT_ID_420L ||
		    productId == PRODUCT_ID_520) {
			if (pop->bitstreamBuffer % 16) {
				CVI_VC_ERR("bitstreamBuffer %% 16\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		if (productId == PRODUCT_ID_420 ||
		    productId == PRODUCT_ID_420L ||
		    productId == PRODUCT_ID_520) {
			if (pop->bitstreamBufferSize < (1024 * 64)) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		if (pop->bitstreamBufferSize % 1024 ||
		    pop->bitstreamBufferSize < 1024) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	if (productId == PRODUCT_ID_980 &&
	    pop->bitstreamBufferSize < H264_MIN_BITSTREAM_SIZE) {
		CVI_VC_ERR("bsSize (0x%x) is smaller than HW limitation (0x%x)",
			   pop->bitstreamBufferSize, H264_MIN_BITSTREAM_SIZE);
		return RETCODE_INVALID_PARAM;
	} else if (productId == PRODUCT_ID_420L &&
		   pop->bitstreamBufferSize < H265_MIN_BITSTREAM_SIZE) {
		CVI_VC_ERR("bsSize (0x%x) is smaller than HW limitation (0x%x)",
			   pop->bitstreamBufferSize, H265_MIN_BITSTREAM_SIZE);
		return RETCODE_INVALID_PARAM;
	}

	if (pop->frameRateInfo == 0) {
		CVI_VC_ERR("frameRateInfo = 0\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat == STD_AVC) {
		if (productId == PRODUCT_ID_980 ||
		    productId == PRODUCT_ID_320) {
			// RC bitrate setting check
			if (pop->bitRate > 524288 || pop->bitRate < 0) {
				CVI_VC_ERR(
					"wrong bitRate setting! bitRate = %d\n",
					pop->bitRate);
				return RETCODE_INVALID_PARAM;
			}
		}
		if (pop->set_dqp_pic_num == 0) {
			CVI_VC_ERR("set_dqp_pic_num must larger than 0");
			return RETCODE_INVALID_PARAM;
		}
		// temporal layer coding constraint
		if (pop->gopPreset >= 2 &&
		    (pop->gopSize % pop->set_dqp_pic_num) != 0) {
			CVI_VC_ERR(
				"gopSize should be multiple of %d for %d T-layer\n",
				pop->set_dqp_pic_num, pop->gopPreset);
			return RETCODE_INVALID_PARAM;
		}

		if (pop->LongTermPeriod > 0 && pop->gopSize > 0) {
			if (pop->gopSize % pop->LongTermPeriod != 0) {
				CVI_VC_ERR(
					"gopSize %d should be LongTermPeriod %d\n",
					pop->gopSize, pop->LongTermPeriod);
				return RETCODE_INVALID_PARAM;
			}
		}

		if (pop->gopPreset >= 2 && pop->VirtualIPeriod > 0) {
			if (pop->VirtualIPeriod % pop->set_dqp_pic_num != 0) {
				CVI_VC_ERR(
					"VirtualIPeriod should be multiple of %d for %d T-layer\n",
					pop->set_dqp_pic_num, pop->gopPreset);
				return RETCODE_INVALID_PARAM;
			}
			if (pop->gopPreset == 3) {
				CVI_VC_ERR(
					"not support 3 Tlayer coding with smartP structure\n");
				return RETCODE_INVALID_PARAM;
			}
		}
	} else if (pop->bitstreamFormat == STD_HEVC) {
		if (productId == PRODUCT_ID_420 ||
		    productId == PRODUCT_ID_420L ||
		    productId == PRODUCT_ID_520) {
			if (pop->bitRate > 700000 || pop->bitRate < 0) {
				CVI_VC_ERR(
					"wrong bitRate setting! bitRate = %d\n",
					pop->bitRate);
				return RETCODE_INVALID_PARAM;
			}
		}
	} else {
		if (pop->bitRate > 32767 || pop->bitRate < 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->bitRate != 0 && pop->initialDelay > 32767) {
		CVI_VC_ERR("wrong initialDelay setting! initialDelay = %d\n",
			   pop->initialDelay);
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitRate != 0 && pop->initialDelay != 0 &&
	    pop->vbvBufferSize < 0) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->frameSkipDisable != 0 && pop->frameSkipDisable != 1) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->sliceMode.sliceMode != 0 && pop->sliceMode.sliceMode != 1) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->sliceMode.sliceMode == 1) {
		if (pop->sliceMode.sliceSizeMode != 0 &&
		    pop->sliceMode.sliceSizeMode != 1) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
		if (pop->sliceMode.sliceSizeMode == 1 &&
		    pop->sliceMode.sliceSize == 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->intraRefresh < 0) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->MEUseZeroPmv != 0 && pop->MEUseZeroPmv != 1) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (pop->intraCostWeight < 0 || pop->intraCostWeight >= 65535) {
		CVI_VC_ERR("\n");
		return RETCODE_INVALID_PARAM;
	}

	if (productId == PRODUCT_ID_980 || productId == PRODUCT_ID_320) {
		if (pop->MESearchRangeX < 0 || pop->MESearchRangeX > 4) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
		if (pop->MESearchRangeY < 0 || pop->MESearchRangeY > 3) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	} else {
		if (pop->MESearchRange < 0 || pop->MESearchRange >= 4) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

#ifdef REDUNDENT_CODE
	if (pop->bitstreamFormat == STD_MPEG4) {
		EncMp4Param *param = &pop->EncStdParam.mp4Param;
		if (param->mp4DataPartitionEnable != 0 &&
		    param->mp4DataPartitionEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->mp4DataPartitionEnable == 1) {
			if (param->mp4ReversibleVlcEnable != 0 &&
			    param->mp4ReversibleVlcEnable != 1) {
				return RETCODE_INVALID_PARAM;
			}
		}
		if (param->mp4IntraDcVlcThr < 0 ||
		    7 < param->mp4IntraDcVlcThr) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < MIN_ENC_PIC_WIDTH ||
		    picWidth > MAX_ENC_PIC_WIDTH) {
			return RETCODE_INVALID_PARAM;
		}

		if (picHeight < MIN_ENC_PIC_HEIGHT) {
			return RETCODE_INVALID_PARAM;
		}
	} else if (pop->bitstreamFormat == STD_H263) {
		EncH263Param *param = &pop->EncStdParam.h263Param;
		Uint32 frameRateInc, frameRateRes;

		if (param->h263AnnexJEnable != 0 &&
		    param->h263AnnexJEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexKEnable != 0 &&
		    param->h263AnnexKEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexTEnable != 0 &&
		    param->h263AnnexTEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < MIN_ENC_PIC_WIDTH ||
		    picWidth > MAX_ENC_PIC_WIDTH) {
			return RETCODE_INVALID_PARAM;
		}
		if (picHeight < MIN_ENC_PIC_HEIGHT) {
			return RETCODE_INVALID_PARAM;
		}

		frameRateInc = ((pop->frameRateInfo >> 16) & 0xFFFF) + 1;
		frameRateRes = pop->frameRateInfo & 0xFFFF;

		if ((frameRateRes / frameRateInc) < 15) {
			return RETCODE_INVALID_PARAM;
		}
	} else
#endif
		if (pop->bitstreamFormat == STD_AVC) {
		EncAvcParam *param = &pop->EncStdParam.avcParam;

		AvcPpsParam *ActivePPS = NULL;

		if (productId == PRODUCT_ID_980)
			ActivePPS = &param->ppsParam[0];

		if (param->constrainedIntraPredFlag != 0 &&
		    param->constrainedIntraPredFlag != 1)
			return RETCODE_INVALID_PARAM;
		if (param->disableDeblk != 0 && param->disableDeblk != 1 &&
		    param->disableDeblk != 2)
			return RETCODE_INVALID_PARAM;
		if (param->deblkFilterOffsetAlpha < -6 ||
		    6 < param->deblkFilterOffsetAlpha)
			return RETCODE_INVALID_PARAM;
		if (param->deblkFilterOffsetBeta < -6 ||
		    6 < param->deblkFilterOffsetBeta)
			return RETCODE_INVALID_PARAM;
		if (param->chromaQpOffset < -12 || 12 < param->chromaQpOffset)
			return RETCODE_INVALID_PARAM;
		if (param->audEnable != 0 && param->audEnable != 1)
			return RETCODE_INVALID_PARAM;
		if (param->frameCroppingFlag != 0 &&
		    param->frameCroppingFlag != 1)
			return RETCODE_INVALID_PARAM;
		if (param->frameCropLeft & 0x01 ||
		    param->frameCropRight & 0x01 ||
		    param->frameCropTop & 0x01 ||
		    param->frameCropBottom & 0x01) {
			return RETCODE_INVALID_PARAM;
		}

		if (productId == PRODUCT_ID_980 ||
		    productId == PRODUCT_ID_320) {
			if (picWidth < MIN_ENC_PIC_WIDTH ||
			    picWidth > MAX_ENC_AVC_PIC_WIDTH) {
				CVI_VC_ERR("picWidth = %d\n", picWidth);
				return RETCODE_INVALID_PARAM;
			}
			if (picHeight < MIN_ENC_PIC_HEIGHT ||
			    picHeight > MAX_ENC_AVC_PIC_HEIGHT) {
				CVI_VC_ERR("picHeight = %d\n", picHeight);
				return RETCODE_INVALID_PARAM;
			}
			if (ActivePPS->entropyCodingMode > 2 &&
			    ActivePPS->entropyCodingMode < 0)
				return RETCODE_INVALID_PARAM;
			if (ActivePPS->cabacInitIdc < 0 ||
			    ActivePPS->cabacInitIdc > 2)
				return RETCODE_INVALID_PARAM;
			if (ActivePPS->transform8x8Mode != 1 &&
			    ActivePPS->transform8x8Mode != 0)
				return RETCODE_INVALID_PARAM;
			if (param->chromaFormat400 != 1 &&
			    param->chromaFormat400 != 0)
				return RETCODE_INVALID_PARAM;
			if (param->fieldFlag != 1 && param->fieldFlag != 0)
				return RETCODE_INVALID_PARAM;
		} else {
			if (picWidth < MIN_ENC_PIC_WIDTH ||
			    picWidth > MAX_ENC_PIC_WIDTH)
				return RETCODE_INVALID_PARAM;
			if (picHeight < MIN_ENC_PIC_HEIGHT)
				return RETCODE_INVALID_PARAM;
		}
	} else if (pop->bitstreamFormat == STD_HEVC) {
		EncHevcParam *param = &pop->EncStdParam.hevcParam;
		if (picWidth < W4_MIN_ENC_PIC_WIDTH ||
		    picWidth > W4_MAX_ENC_PIC_WIDTH) {
			CVI_VC_ERR("picWidth = %d\n", picWidth);
			return RETCODE_INVALID_PARAM;
		}

		if (picHeight < W4_MIN_ENC_PIC_HEIGHT ||
		    picHeight > W4_MAX_ENC_PIC_HEIGHT) {
			CVI_VC_ERR("picHeight = %d\n", picHeight);
			return RETCODE_INVALID_PARAM;
		}

		if (param->profile != HEVC_PROFILE_MAIN &&
		    param->profile != HEVC_PROFILE_MAIN10) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->internalBitDepth != 8 &&
		    param->internalBitDepth != 10) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->internalBitDepth > 8 &&
		    param->profile == HEVC_PROFILE_MAIN) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->chromaFormatIdc < 0 || param->chromaFormatIdc > 3) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->decodingRefreshType < 0 ||
		    param->decodingRefreshType > 2) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->gopParam.useDeriveLambdaWeight != 1 &&
		    param->gopParam.useDeriveLambdaWeight != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->gopPresetIdx < PRESET_IDX_T0S ||
		    param->gopPresetIdx > PRESET_IDX_T2ST1L) {
			if (param->gopParam.enTemporalLayerQp != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
			// seems not necessary
			//if (param->gopParam.tidPeriod0 != 60) {
			//	CVI_VC_ERR("\n");
			//	return RETCODE_INVALID_PARAM;
			//}
		}

		if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
			if (param->gopParam.customGopSize < 1 ||
			    param->gopParam.customGopSize > MAX_GOP_NUM) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		if (param->constIntraPredFlag != 1 &&
		    param->constIntraPredFlag != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (productId == PRODUCT_ID_520) {
			if (param->intraRefreshMode < 0 ||
			    param->intraRefreshMode > 4)
				return RETCODE_INVALID_PARAM;
		} else {
			// wave420
			if (param->intraRefreshMode < 0 ||
			    param->intraRefreshMode > 3) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		if (param->independSliceMode < 0 ||
		    param->independSliceMode > 1) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->independSliceMode != 0) {
			if (param->dependSliceMode < 0 ||
			    param->dependSliceMode > 2) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		if (param->useRecommendEncParam < 0 &&
		    param->useRecommendEncParam > 3) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->useRecommendEncParam == 0 ||
		    param->useRecommendEncParam == 2 ||
		    param->useRecommendEncParam == 3) {
			if (param->intraInInterSliceEnable != 1 &&
			    param->intraInInterSliceEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->intraNxNEnable != 1 &&
			    param->intraNxNEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->skipIntraTrans != 1 &&
			    param->skipIntraTrans != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->scalingListEnable != 1 &&
			    param->scalingListEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->tmvpEnable != 1 && param->tmvpEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->wppEnable != 1 && param->wppEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->useRecommendEncParam != 3) { // in FAST mode
				// (recommendEncParam==3),
				// maxNumMerge
				// value will be
				// decided in FW
				if (param->maxNumMerge < 0 ||
				    param->maxNumMerge > 3) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}
			}

			if (param->disableDeblk != 1 &&
			    param->disableDeblk != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->disableDeblk == 0 || param->saoEnable != 0) {
				if (param->lfCrossSliceBoundaryEnable != 1 &&
				    param->lfCrossSliceBoundaryEnable != 0) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}
			}

			if (param->disableDeblk == 0) {
				if (param->betaOffsetDiv2 < -6 ||
				    param->betaOffsetDiv2 > 6) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}

				if (param->tcOffsetDiv2 < -6 ||
				    param->tcOffsetDiv2 > 6) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}
			}
		}

		if (param->losslessEnable != 1 && param->losslessEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->intraQP < 0 || param->intraQP > 51) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (pop->rcEnable != 1 && pop->rcEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (pop->rcEnable == 1) {
			if (pop->userQpMinP < 0 || pop->userQpMinP > 51) {
				CVI_VC_ERR("Invalid min Qp setting = %d\n",
					   pop->userQpMinP);
				return RETCODE_INVALID_PARAM;
			}

			if (pop->userQpMaxP < 0 || pop->userQpMaxP > 51) {
				CVI_VC_ERR("Invalid max Qp setting = %d\n",
					   pop->userQpMaxP);
				return RETCODE_INVALID_PARAM;
			}

			if (productId == PRODUCT_ID_420L) {
				if (param->initialRcQp < 0 ||
				    param->initialRcQp > 63) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}

				if ((param->initialRcQp < 52) &&
				    ((param->initialRcQp > pop->userQpMaxP) ||
				     (param->initialRcQp < pop->userQpMinP))) {
					CVI_VC_ERR("Invalid initialRcQp = %d, maxQp = %d, minQp = %d\n",
						   param->initialRcQp,
						   pop->userQpMaxP,
						   pop->userQpMinP);
					return RETCODE_INVALID_PARAM;
				}
			}
			if (param->ctuOptParam.ctuQpEnable) // can't enable both
			// rcEnable and
			// ctuQpEnable
			// together.
			{
				CVI_VC_ERR("RC must disable as ctuQp enable\n");
				return RETCODE_INVALID_PARAM;
			}
			if (param->intraQpOffset < -10 &&
			    param->intraQpOffset > 10) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->cuLevelRCEnable != 1 &&
			    param->cuLevelRCEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->hvsQPEnable != 1 &&
			    param->hvsQPEnable != 0) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->hvsQPEnable) {
				if (param->maxDeltaQp < 0 ||
				    param->maxDeltaQp > 12) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}

				if (param->hvsQpScaleEnable != 1 &&
				    param->hvsQpScaleEnable != 0) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}

				if (param->hvsQpScaleEnable == 1) {
					if (param->hvsQpScale < 0 ||
					    param->hvsQpScale > 4) {
						CVI_VC_ERR("\n");
						return RETCODE_INVALID_PARAM;
					}
				}
			}

			if (param->ctuOptParam.roiEnable) {
				if (param->ctuOptParam.roiDeltaQp < 1 ||
				    param->ctuOptParam.roiDeltaQp > 51) {
					CVI_VC_ERR("\n");
					return RETCODE_INVALID_PARAM;
				}
			}
			// no this constraint after FW_WAVE420L_v1.7.12
			/*
			if (param->ctuOptParam.roiEnable &&
			    param->hvsQPEnable) // can not use both ROI and
						// hvsQp
			{
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
*/
			if (param->bitAllocMode < 0 &&
			    param->bitAllocMode > 2) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (param->initBufLevelx8 < 0 ||
			    param->initBufLevelx8 > 8) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}

			if (pop->initialDelay < 10 ||
			    pop->initialDelay > 3000) {
				CVI_VC_ERR("\n");
				return RETCODE_INVALID_PARAM;
			}
		}

		// packed format & cbcrInterleave & nv12 can't be set at the
		// same time.
		if (pop->packedFormat == 1 && pop->cbcrInterleave == 1) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (pop->packedFormat == 1 && pop->nv21 == 1) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		// check valid for common param
		if (CheckEncCommonParamValid(pop) == RETCODE_FAILURE) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		// check valid for RC param
		if (CheckEncRcParamValid(pop) == RETCODE_FAILURE) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->wppEnable && pop->ringBufferEnable) // WPP can be
		// processed on
		// only
		// linebuffer
		// mode.
		{
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		// check additional features for WAVE420L
		if (param->chromaCbQpOffset < -12 ||
		    param->chromaCbQpOffset > 12) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->chromaCrQpOffset < -12 ||
		    param->chromaCrQpOffset > 12) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrYEnable != 1 && param->nrYEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrCbEnable != 1 && param->nrCbEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrCrEnable != 1 && param->nrCrEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrNoiseEstEnable != 1 &&
		    param->nrNoiseEstEnable != 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrNoiseSigmaY > 255) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrNoiseSigmaCb > 255) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrNoiseSigmaCr > 255) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrIntraWeightY > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrIntraWeightCb > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrIntraWeightCr > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrInterWeightY > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrInterWeightCb > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->nrInterWeightCr > 31) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if ((param->nrYEnable == 1 || param->nrCbEnable == 1 ||
		     param->nrCrEnable == 1) &&
		    (param->losslessEnable == 1)) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->intraRefreshMode == 3 &&
		    param->intraRefreshArg == 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->ctuOptParam.ctuModeEnable == 1 &&
		    param->maxNumMerge == 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}

		if (param->ctuOptParam.ctuModeEnable == 1 &&
		    param->intraInInterSliceEnable == 0) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->linear2TiledEnable == TRUE) {
		if (pop->linear2TiledMode != FF_FRAME &&
		    pop->linear2TiledMode != FF_FIELD) {
			CVI_VC_ERR("\n");
			return RETCODE_INVALID_PARAM;
		}
	}

	return RETCODE_SUCCESS;
}

RetCode ProductVpuEncFiniSeq(CodecInst *instance)
{
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuFiniSeq(instance);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuEncFiniSeq(instance);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}
	return ret;
}

RetCode ProductVpuEncSetup(CodecInst *instance)
{
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	Uint64 start_time, end_time;

	start_time = cviGetCurrentTime();

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuEncSetup(instance);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuEncSetup(instance);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	end_time = cviGetCurrentTime();
	CVI_VC_PERF("EncSetup = %llu us\n", end_time - start_time);

	return ret;
}

RetCode ProductVpuEncode(CodecInst *instance, EncParam *param)
{
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	vpu_set_channel_core_mapping(instance->s32ChnNum, instance->coreIdx);

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuEncode(instance, param);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuEncode(instance, param);
		break;
	default:
		break;
	}

	return ret;
}

RetCode ProductVpuEncGetResult(CodecInst *instance, EncOutputInfo *result)
{
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	vpu_set_channel_core_mapping(instance->s32ChnNum, -1);

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuEncGetResult(instance, result);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuEncGetResult(instance, result);
		break;
	default:
		ret = RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	result->encHwTime = instance->u64EndTime - instance->u64StartTime;
	CVI_VC_PERF("encHwTime %llu\n", result->encHwTime);

	return ret;
}

#ifdef REDUNDENT_CODE
RetCode ProductVpuEncGiveCommand(CodecInst *instance, CodecCommand cmd,
				 void *param)
{
	RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

	switch (instance->productId) {
	case PRODUCT_ID_980:
		ret = Coda9VpuEncGiveCommand(instance, cmd, param);
		break;
	case PRODUCT_ID_420L:
		ret = Wave4VpuEncGiveCommand(instance, cmd, param);
		break;
	}

	return ret;
}

RetCode ProductVpuEncInitSeq(CodecInst *instance)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = instance->productId;

	switch (productId) {
	default:
		break;
	}

	return ret;
}

RetCode ProductVpuEncGetSeqInfo(CodecInst *instance)
{
	int productId;
	RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

	productId = instance->productId;

	switch (productId) {
	default:
		break;
	}

	return ret;
}
#endif
