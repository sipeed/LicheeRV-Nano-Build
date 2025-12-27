/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: Aug, 2020
 */
#ifdef ENABLE_DEC
#include <linux/string.h>
#include "cvi_dec_internal.h"
#include "cvi_h265_interface.h"
#include "product.h"
#include "cvi_vc_getopt.h"

#define FEEDING_SIZE 0x20000
#define USERDATA_BUFFER_SIZE (512 * 1024)
#define STREAM_BUF_SIZE_HEVC 0xA00000

#define SECONDARY_AXI_H264 0xf
#define SECONDARY_AXI_H265 0x7

static int cviBitstreamFeeder_Act(cviVideoDecoder *pvdec);
static int cviInitSeq(cviVideoDecoder *pvdec);

#ifdef CVI_WRITE_FRAME
static int cviWriteFrame(cviVideoDecoder *pvdec);
static int cviWriteComp(cviVideoDecoder *pvdec, PhysicalAddress buf, int width,
			int height, int stride);
#endif

CVI_VDEC_CALLBACK pVdecDrvCbFunc;

cviVideoDecoder *cviAllocVideoDecoder(void)
{
	cviVideoDecoder *pvdec;

	pvdec = (cviVideoDecoder *)osal_malloc(sizeof(cviVideoDecoder));
	if (!pvdec) {
		CVI_VC_ERR("malloc, cviVideoDecoder\n");
		return NULL;
	}
	memset(pvdec, 0, sizeof(cviVideoDecoder));
	return pvdec;
}

void cviFreeVideoDecoder(cviVideoDecoder **ppvdec)
{
	cviVideoDecoder *pvdec;

	if (!ppvdec) {
		CVI_VC_ERR("ppvdec\n");
		return;
	}

	pvdec = *ppvdec;

	if (!pvdec) {
		CVI_VC_ERR("pvdec\n");
		return;
	}

	osal_free(pvdec);

	*ppvdec = NULL;
}

int cviInitDecMcuEnv(cviDecConfig *pdeccfg)
{
	Uint32 productId = PRODUCT_ID_980;
#ifndef FIRMWARE_H
	char *fwPath = NULL;
#endif

	cviSetCoreIdx(&pdeccfg->coreIdx, pdeccfg->bitFormat);

	if (pdeccfg->bitFormat == STD_HEVC) {
		productId = PRODUCT_ID_420L;
	}

	CVI_VC_TRACE("\n");
#ifdef FIRMWARE_H
	if (pdeccfg->sizeInWord == 0 && pdeccfg->pusBitCode == NULL) {
		if (LoadFirmwareH(productId, (Uint8 **)&pdeccfg->pusBitCode,
				  &pdeccfg->sizeInWord) < 0) {
			CVI_VC_ERR("Failed to load firmware: productId = %d\n", productId);
			return CVI_ERR_DEC_MCU;
		}
	}
#else
	switch (productId) {
	case PRODUCT_ID_980:
		fwPath = CORE_1_BIT_CODE_FILE_PATH;
		break;
	case PRODUCT_ID_420L:
		fwPath = CORE_5_BIT_CODE_FILE_PATH;
		break;
	default:
		CVI_VC_ERR("Unknown product id: %d\n", productId);
		return -1;
	}
	if (pdeccfg->sizeInWord == 0 && pdeccfg->pusBitCode == NULL) {
		if (LoadFirmware(productId, (Uint8 **)&pdeccfg->pusBitCode,
				  &pdeccfg->sizeInWord, fwPath) < 0) {
			CVI_VC_ERR("Failed to load firmware: %s\n", fwPath);
			return 1;
		}
	}
#endif

	return 0;
}

void cviFreeDecMcuEnv(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;

	if (pdeccfg->pusBitCode) {
#ifndef FIRMWARE_H
		osal_free(pdeccfg->pusBitCode);
#endif
		pdeccfg->pusBitCode = NULL;
		pdeccfg->sizeInWord = 0;
	}
}

static int _cviH265_InitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecOpenParam dop, *pdop = &dop;
	RetCode ret = RETCODE_FAILURE;
#ifndef CVI_H26X_USE_ION_MEM
	Uint32 index;
#else
#ifdef BITSTREAM_ION_CACHED_MEM
	Int32 bBsStreamCached = 1;
#else
	Int32 bBsStreamCached = 0;
#endif
#endif
	Uint64 start_time, end_time;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	memset(&dop, 0, sizeof(dop));

	pvdec->seqInited = FALSE;
	pvdec->feeder = NULL;
	pvdec->renderer = NULL;
	pvdec->comparator = NULL;
	pvdec->enablePPU = FALSE;
	pvdec->ppuFbCount = PPU_FB_COUNT;
	pvdec->decodedIdx = 0;
	pvdec->framebufHeight = 0;
	pvdec->doDumpImage = FALSE;
	pvdec->waitPostProcessing = FALSE;
	pvdec->needStream = FALSE;
	pvdec->seqChangeRequest = FALSE;
	pvdec->decHandle = NULL;
	pvdec->dispIdx = 0;
	pvdec->noFbCount = 0;
	pvdec->assertedFieldDoneInt = FALSE;
	//pvdec->ppuQ = NULL;
	pvdec->noResponseCount = MAX_COUNT_NO_RESPONSE;
	pvdec->seqChangedStreamEndFlag = 0;
	pvdec->seqChangedRdPtr = 0;
	pvdec->seqChangedWrPtr = 0;

	pvdec->bsBufferCount = 1;
	pvdec->bsQueueIndex = 0;

	pdeccfg->enableCrop = TRUE;

	pvdec->productId = VPU_GetProductId(pdeccfg->coreIdx);
	if (pvdec->productId == -1) {
		CVI_VC_ERR("Failed to get product ID\n");
		return CVI_ERR_DEC_INIT;
	}

	start_time = cviGetCurrentTime();

	ret = VPU_InitWithBitcode(pdeccfg->coreIdx,
				  (const Uint16 *)pdeccfg->pusBitCode,
				  pdeccfg->sizeInWord);
	if (ret != RETCODE_CALLED_BEFORE && ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"Failed to boot up VPU(coreIdx: %d, productId: %d)\n",
			pdeccfg->coreIdx, pvdec->productId);
		return CVI_ERR_DEC_INIT;
	}

	end_time = cviGetCurrentTime();
	CVI_VC_PERF("VPU_InitWithBitcode = %llu us\n", end_time - start_time);

	PrintVpuVersionInfo(pdeccfg->coreIdx);

	pvdec->bsBufferCount =
		(pdeccfg->bitstreamMode == BS_MODE_PIC_END) ? 2 : 1;

	sprintf(ionName, "VDEC_%d_BitStreamBuffer", pvdec->chnNum);
#ifndef CVI_H26X_USE_ION_MEM
	for (index = 0; index < pvdec->bsBufferCount; index++) {
		pvdec->vbStream[index].size = pdeccfg->bsSize;
		CVI_VC_MEM("vbStream[%d].size = 0x%X\n", index,
			   pvdec->vbStream[index].size);
		if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx,
					&pvdec->vbStream[index], 0,
					ionName) < 0) {
			VLOG(ERR, "fail to allocate bitstream buffer\n");
			return CVI_ERR_DEC_INIT;
		}
	}
#else
	pvdec->vbStream[0].size = pdeccfg->bsSize * pvdec->bsBufferCount;
	if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, &pvdec->vbStream[0],
				bBsStreamCached, ionName) < 0) {
		VLOG(ERR, "fail to allocate bitstream buffer\n");
		return CVI_ERR_DEC_INIT;
	}
	pvdec->vbStream[0].size = pvdec->vbStream[1].size = pdeccfg->bsSize;
	pvdec->vbStream[1].phys_addr =
		pvdec->vbStream[0].phys_addr + pdeccfg->bsSize;
	pvdec->vbStream[1].virt_addr =
		pvdec->vbStream[0].virt_addr + pdeccfg->bsSize;
#endif

	CVI_VC_TRACE("cviApiMode = %d, bitstreamMode = %d\n", pvdec->cviApiMode,
		     pdeccfg->bitstreamMode);
#ifdef VC_DRIVER_TEST
	if (pvdec->cviApiMode == API_MODE_DRIVER) {
		if (pdeccfg->feedingMode == FEEDING_METHOD_FRAME_SIZE) {
			CodStd codecId;
			pvdec->feeder = BitstreamFeeder_Create(
				pdeccfg->inputPath, FEEDING_METHOD_FRAME_SIZE,
				pvdec->vbStream[0].phys_addr,
				pvdec->vbStream[0].size, pdeccfg->bitFormat,
				&codecId, NULL, NULL, NULL);
			pdeccfg->bitFormat = codecId;
		} else if (pdeccfg->feedingMode == FEEDING_METHOD_FIXED_SIZE) {
			pvdec->feeder = BitstreamFeeder_Create(
				pdeccfg->inputPath, FEEDING_METHOD_FIXED_SIZE,
				pvdec->vbStream[0].phys_addr,
				pvdec->vbStream[0].size, FEEDING_SIZE);
		} else {
			/* FEEDING_METHOD_SIZE_PLUS_ES */
			pvdec->feeder = BitstreamFeeder_Create(
				pdeccfg->inputPath, FEEDING_METHOD_SIZE_PLUS_ES,
				pvdec->vbStream[0].phys_addr,
				pvdec->vbStream[0].size);
		}

		if (pvdec->feeder == NULL) {
			VLOG(ERR, "fail to create feeder\n");
			return CVI_ERR_DEC_INIT;
		}
		BitstreamFeeder_SetFillMode(pvdec->feeder,
					    (pdeccfg->bitstreamMode ==
					     BS_MODE_PIC_END) ?
							  BSF_FILLING_LINEBUFFER :
							  BSF_FILLING_RINGBUFFER);
	} else
#endif
	{
		pvdec->feeder = BitstreamFeeder_Create(
			pdeccfg->inputPath, FEEDING_METHOD_ES_IN,
			pvdec->vbStream[0].phys_addr, pvdec->vbStream[0].size,
			FEEDING_SIZE);
	}

	/* set up decoder configurations */
	pdop->bitstreamFormat = (CodStd)pdeccfg->bitFormat;
	pdop->coreIdx = pdeccfg->coreIdx;
	pdop->bitstreamBuffer = pvdec->vbStream[0].phys_addr;
	pdop->bitstreamBufferSize =
		pvdec->vbStream[0].size * pvdec->bsBufferCount;
	pdop->bitstreamMode = pdeccfg->bitstreamMode;
	pdop->wtlEnable = pdeccfg->enableWTL;
	pdop->wtlMode = pdeccfg->wtlMode;
	pdop->cbcrInterleave = pdeccfg->cbcrInterleave;
	pdop->nv21 = pdeccfg->nv21;
	pdop->streamEndian = pdeccfg->streamEndian;
	pdop->frameEndian = pdeccfg->frameEndian;
	pdop->fbc_mode = pdeccfg->wave4.fbcMode;
	pdop->bwOptimization = pdeccfg->wave4.bwOptimization;
	pdop->s32ChnNum = pvdec->chnNum;

	/********************************************************************************
	 * CREATE INSTANCE *
	 ********************************************************************************/
	ret = VPU_DecOpen(&pvdec->decHandle, pdop);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_DecOpen failed Error code is 0x%x\n", ret);
		return CVI_ERR_DECODE_END;
	}

	pvdec->vbUserData.size = USERDATA_BUFFER_SIZE;
	CVI_VC_MEM("vbUserData.size = 0x%X\n", pvdec->vbUserData.size);

	memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
	sprintf(ionName, "VDEC_%d_UserData", pvdec->chnNum);
	if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, &pvdec->vbUserData, 0,
				ionName) < 0) {
		VLOG(ERR, "%s:%d Failed to allocate memory\n", __func__,
		     __LINE__);
		return CVI_ERR_DECODE_END;
	}
	//pBase = (Uint8*)osal_malloc(USERDATA_BUFFER_SIZE);
	VPU_DecGiveCommand(pvdec->decHandle, SET_ADDR_REP_USERDATA,
			   (void *)&pvdec->vbUserData.phys_addr);
	VPU_DecGiveCommand(pvdec->decHandle, SET_SIZE_REP_USERDATA,
			   (void *)&pvdec->vbUserData.size);
	VPU_DecGiveCommand(pvdec->decHandle, ENABLE_REP_USERDATA,
			   (void *)&pdeccfg->enableUserData);

	if (pdeccfg->thumbnailMode == TRUE) {
		VPU_DecGiveCommand(pvdec->decHandle, ENABLE_DEC_THUMBNAIL_MODE,
				   NULL);
	}

	pvdec->renderer = SimpleRenderer_Create(
		pvdec->decHandle, pdeccfg->renderType, pdeccfg->outputPath);
	if (pvdec->renderer == NULL) {
		CVI_VC_ERR("renderer = NULL\n");
		return CVI_ERR_DECODE_END;
	}

	pvdec->displayQ = Queue_Create(MAX_REG_FRAME, sizeof(DecOutputInfo));
	if (pvdec->displayQ == NULL) {
		CVI_VC_ERR("Create displayQ fail\n");
		return CVI_ERR_DECODE_END;
	}

	pvdec->sequenceQ = Queue_Create(MAX_REG_FRAME, sizeof(Uint32));
	if (pvdec->sequenceQ == NULL) {
		CVI_VC_ERR("Create sequenceQ fail\n");
		return CVI_ERR_DECODE_END;
	}

	return CVI_INIT_DECODER_OK;
}

static int _cviH264_InitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	VpuReportConfig_t *pdrc = &pvdec->decReportCfg;
	vpu_buffer_t *pvbs = &pvdec->vbStream[0];
	DecOpenParam dop, *pdop = &dop;
	RetCode ret = RETCODE_FAILURE;
	Uint64 start_time, end_time;
#ifdef CVI_H26X_USE_ION_MEM
#ifdef BITSTREAM_ION_CACHED_MEM
	Int32 bBsStreamCached = 1;
#else
	Int32 bBsStreamCached = 0;
#endif
#endif
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	memset(&dop, 0, sizeof(dop));
	pvdec->seqInited = FALSE;
	pvdec->feeder = NULL;
	pvdec->renderer = NULL;
	pvdec->comparator = NULL;
	pvdec->enablePPU = FALSE;
	pvdec->ppuFbCount = PPU_FB_COUNT;
	pvdec->decodedIdx = 0;
	pvdec->framebufHeight = 0;
	pvdec->doDumpImage = FALSE;
	pvdec->waitPostProcessing = FALSE;
	pvdec->needStream = FALSE;
	pvdec->seqChangeRequest = FALSE;
	pvdec->decHandle = NULL;
	pvdec->dispIdx = 0;
	pvdec->noFbCount = 0;
	pvdec->assertedFieldDoneInt = FALSE;
	pvdec->ppuQ = NULL;
	pvdec->noResponseCount = MAX_COUNT_NO_RESPONSE;
	pvdec->seqChangedStreamEndFlag = 0;
	pvdec->seqChangedRdPtr = 0;
	pvdec->seqChangedWrPtr = 0;

	pdeccfg->enableCrop = TRUE;

	pvdec->productId = VPU_GetProductId(pdeccfg->coreIdx);
	if (pvdec->productId == -1) {
		CVI_VC_ERR("Failed to get product ID\n");
		return CVI_ERR_DEC_INIT;
	}

	start_time = cviGetCurrentTime();

	ret = VPU_InitWithBitcode(pdeccfg->coreIdx,
				  (const Uint16 *)pdeccfg->pusBitCode,
				  pdeccfg->sizeInWord);
	if (ret != RETCODE_CALLED_BEFORE && ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"Failed to boot up VPU(coreIdx: %d, productId: %d)\n",
			pdeccfg->coreIdx, pvdec->productId);
		return CVI_ERR_DEC_INIT;
	}

	end_time = cviGetCurrentTime();
	CVI_VC_PERF("VPU_InitWithBitcode = %llu us\n", end_time - start_time);

	PrintVpuVersionInfo(pdeccfg->coreIdx);

	if (pvdec->cviApiMode == API_MODE_SDK) {
		pvbs->size = pdeccfg->bsSize;
	} else {
		pvbs->size = STREAM_BUF_SIZE;
	}
	CVI_VC_MEM("vbStream, pvbs.size = 0x%X\n", pvbs->size);

	sprintf(ionName, "VDEC_%d_BitStreamBuffer", pvdec->chnNum);
	if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, pvbs, bBsStreamCached,
				ionName) < 0) {
		CVI_VC_ERR("fail to allocate bitstream buffer\n");
		return CVI_ERR_DEC_INIT;
	}

	CVI_VC_TRACE("cviApiMode = %d, bitstreamMode = %d\n", pvdec->cviApiMode,
		     pdeccfg->bitstreamMode);
#ifdef VC_DRIVER_TEST
	if (pvdec->cviApiMode == API_MODE_DRIVER) {
		if (pdeccfg->bitstreamMode == BS_MODE_PIC_END ||
		    pdeccfg->bitFormat == STD_THO) {
			CodStd codecId;
			Uint32 mp4Id;

			pvdec->feeder = BitstreamFeeder_Create(
				pdeccfg->inputPath, FEEDING_METHOD_FRAME_SIZE,
				pvbs->phys_addr, pvbs->size, pdeccfg->bitFormat,
				&codecId, &mp4Id, NULL, NULL);
			pdeccfg->bitFormat = codecId;
			pdeccfg->coda9.mp4class = mp4Id;
		} else {
			CVI_VC_TRACE("phys_addr = 0x%llX\n", pvbs->phys_addr);
			pvdec->feeder = BitstreamFeeder_Create(
				pdeccfg->inputPath, FEEDING_METHOD_FIXED_SIZE,
				pvbs->phys_addr, pvbs->size, 0x2000);
		}
	} else
#endif
	{
		pvdec->feeder = BitstreamFeeder_Create(pdeccfg->inputPath,
						       FEEDING_METHOD_ES_IN,
						       pvbs->phys_addr,
						       pvbs->size, 0x2000);
	}

	if (pvdec->feeder == NULL) {
		CVI_VC_ERR("feeder = NULL\n");
		return CVI_ERR_DECODE_END;
	}

	/* set up decoder configurations */
	pdop->bitstreamFormat = (CodStd)pdeccfg->bitFormat;
	pdop->avcExtension = pdeccfg->coda9.enableMvc;
	pdop->coreIdx = pdeccfg->coreIdx;
	pdop->bitstreamBuffer = pvbs->phys_addr;
	pdop->bitstreamBufferSize = pvbs->size;
	pdop->bitstreamMode = pdeccfg->bitstreamMode;
	pdop->tiled2LinearEnable = pdeccfg->coda9.enableTiled2Linear;
	pdop->tiled2LinearMode = pdeccfg->coda9.tiled2LinearMode;
	pdop->wtlEnable = pdeccfg->enableWTL;
	pdop->wtlMode = pdeccfg->wtlMode;
	pdop->cbcrInterleave = pdeccfg->cbcrInterleave;
	pdop->nv21 = pdeccfg->nv21;
	pdop->streamEndian = pdeccfg->streamEndian;
	pdop->frameEndian = pdeccfg->frameEndian;
	pdop->bwbEnable = pdeccfg->coda9.enableBWB;
	pdop->mp4DeblkEnable = pdeccfg->coda9.enableDeblock;
	pdop->mp4Class = pdeccfg->coda9.mp4class;
	pdop->s32ChnNum = pvdec->chnNum;
	pdop->reorderEnable = pvdec->ReorderEnable;

	VLOG(INFO,
	     "-------------- DECODER OPTIONS ------------------------------\n");
	VLOG(INFO, "[coreIdx            ]: %d\n", pdop->coreIdx);
	VLOG(INFO, "[mapType            ]: %d\n", pdeccfg->mapType);
	VLOG(INFO, "[tiled2linear       ]: %d\n",
	     pdeccfg->coda9.enableTiled2Linear);
	VLOG(INFO, "[wtlEnable          ]: %d\n", pdop->wtlEnable);
	VLOG(INFO, "[wtlMode            ]: %d\n", pdop->wtlMode);
	VLOG(INFO, "[bitstreamBuffer    ]: 0x%08x\n", pdop->bitstreamBuffer);
	VLOG(INFO, "[bitstreamBufferSize]: %d\n", pdop->bitstreamBufferSize);
	VLOG(INFO, "[bitstreamMode      ]: %d\n", pdop->bitstreamMode);
	VLOG(INFO, "[cbcrInterleave     ]: %d\n", pdop->cbcrInterleave);
	VLOG(INFO, "[nv21               ]: %d\n", pdop->nv21);
	VLOG(INFO, "[streamEndian       ]: %d\n", pdop->streamEndian);
	VLOG(INFO, "[frameEndian        ]: %d\n", pdop->frameEndian);
	VLOG(INFO, "[BWB                ]: %d\n", pdop->bwbEnable);
	VLOG(INFO, "[reorderEnable      ]: %d\n", pdop->reorderEnable);
	VLOG(WARN,
	     "-------------------------------------------------------------\n");

	/********************************************************************************
	 * CREATE INSTANCE *
	 ********************************************************************************/
	ret = VPU_DecOpen(&pvdec->decHandle, pdop);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_DecOpen failed Error code is 0x%x\n", ret);
		return CVI_ERR_DECODE_END;
	}

	if (pdop->bitstreamFormat == STD_AVC) {
		ret = VPU_DecGiveCommand(pvdec->decHandle, SET_LOW_DELAY_CONFIG,
					 &pdeccfg->coda9.lowDelay);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR(
				"VPU_DecGiveCommand[SET_LOW_DELAY_CONFIG] failed Error code is 0x%x\n",
				ret);
			return CVI_ERR_DECODE_END;
		}
	}

	pdrc->userDataEnable = pdeccfg->enableUserData;
	pdrc->userDataReportMode = 1;
#ifdef REDUNDENT_CODE
	OpenDecReport(pdeccfg->coreIdx, pdrc);
	ConfigDecReport(pdeccfg->coreIdx, pvdec->decHandle);
#endif

#ifdef ENABLE_CNM_DEBUG_MSG
	// VPU_DecGiveCommand(pvdec->decHandle, ENABLE_LOGGING, 0);
#endif
	pvdec->renderer = SimpleRenderer_Create(
		pvdec->decHandle, pdeccfg->renderType, pdeccfg->outputPath);
	if (pvdec->renderer == NULL) {
		CVI_VC_ERR("renderer = NULL\n");
		return CVI_ERR_DECODE_END;
	}

	return CVI_INIT_DECODER_OK;
}

int cviInitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	int ret = CVI_INIT_DECODER_OK;

	if (pdeccfg->bitFormat == STD_AVC)
		ret = _cviH264_InitDecoder(pvdec);
	else
		ret = _cviH265_InitDecoder(pvdec);

	return ret;
}

int cviInitSeqSetting(cviVideoDecoder *pvdec)
{
	CVI_DEC_STATUS decStatus;
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	vpu_buffer_t *pvbs = &pvdec->vbStream[0];

	CVI_VC_TRACE("\n");

	VPU_DecSetOutputFormat(pvdec->decHandle, pdeccfg->cbcrInterleave,
			       pdeccfg->nv21);

	if (pdeccfg->bitFormat == STD_HEVC) {
		pvdec->bsQueueIndex =
			(pvdec->bsQueueIndex + 1) % (pvdec->bsBufferCount);
		VPU_DecSetRdPtr(pvdec->decHandle,
				pvdec->vbStream[pvdec->bsQueueIndex].phys_addr,
				TRUE);
	} else {
		VPU_DecSetRdPtr(pvdec->decHandle, pvbs->phys_addr, TRUE);
	}

	pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);

	if (pvdec->nWritten < 0) {
		CVI_VC_ERR("nWritten < 0\n");
		return CVI_ERR_DECODE_END;
	}

#ifdef REDUNDENT_CODE
	if (pdeccfg->bitstreamMode == BS_MODE_PIC_END) {
		if ((CodStd)pdeccfg->bitFormat != STD_THO &&
		    (CodStd)pdeccfg->bitFormat != STD_H263 &&
		    (CodStd)pdeccfg->bitFormat != STD_RV) {
			// Need first picture. In case of Theora, ffmpeg returns
			// a coded frame including sequence header.
			//			cviBitstreamFeeder_Act(pvdec);
		}
	}
#endif

	decStatus = cviInitSeq(pvdec);
	if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("cviInitSeq, decStatus = %d\n", decStatus);
		return decStatus;
	}

	return 0;
}

static int cviBitstreamFeeder_Act(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	vpu_buffer_t *bsBuf = NULL;

	if (pvdec->cviApiMode == API_MODE_SDK) {
		bsBuf = &pvdec->sdkParam.bsBuf;
	}

	return BitstreamFeeder_Act(pvdec->decHandle, pvdec->feeder,
				   pdeccfg->streamEndian, bsBuf);
}

BOOL _AllocateH265DecFrameBuffer(cviVideoDecoder *pvdec, DecHandle decHandle,
				 cviDecConfig *pdeccfg, Uint32 tiledFbCount,
				 Uint32 linearFbCount, FrameBuffer *retFbArray,
				 vpu_buffer_t *retFbAddrs, Uint32 *retStride)
{
	Uint32 framebufSize;
	Uint32 totalFbCount;
	Uint32 coreIndex;
	Uint32 index;
	FrameBufferFormat format = pdeccfg->wtlFormat;
	DecInitialInfo seqInfo;
	FrameBufferAllocInfo fbAllocInfo;
	RetCode ret;
	vpu_buffer_t *pvb;
	size_t framebufStride;
	size_t framebufHeight;
	Uint32 productId;
	DRAMConfig *pDramCfg = NULL;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	coreIndex = VPU_HANDLE_CORE_INDEX(decHandle);
	productId = VPU_HANDLE_PRODUCT_ID(decHandle);
	VPU_DecGiveCommand(decHandle, DEC_GET_SEQ_INFO, (void *)&seqInfo);

	if (productId != PRODUCT_ID_420L)
		CVI_VC_WARN("productId != PRODUCT_ID_420L\n");
	CVI_VC_INFO("coreIndex=%d, productId=%d\n", coreIndex, productId);

	totalFbCount = tiledFbCount + linearFbCount;

	if (productId == PRODUCT_ID_4102 || productId == PRODUCT_ID_420 ||
	    productId == PRODUCT_ID_412 || productId == PRODUCT_ID_420L ||
	    productId == PRODUCT_ID_510 || productId == PRODUCT_ID_512 ||
	    productId == PRODUCT_ID_515) {
		format = (seqInfo.lumaBitdepth > 8 ||
			  seqInfo.chromaBitdepth > 8) ?
				       FORMAT_420_P10_16BIT_LSB :
				       FORMAT_420;
	}

	*retStride = VPU_ALIGN32(seqInfo.picWidth);
	framebufStride = CalcStride(seqInfo.picWidth, seqInfo.picHeight, format,
				    pdeccfg->cbcrInterleave, pdeccfg->mapType,
				    FALSE, TRUE);
	framebufHeight = seqInfo.picHeight;
	framebufSize =
		VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride,
				    seqInfo.picHeight, pdeccfg->mapType, format,
				    pdeccfg->cbcrInterleave, pDramCfg);

	osal_memset((void *)&fbAllocInfo, 0x00, sizeof(fbAllocInfo));
	if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
		osal_memset((void *)retFbArray, 0x00,
			    sizeof(FrameBuffer) * totalFbCount);
	}

	fbAllocInfo.format = format;
	fbAllocInfo.cbcrInterleave = pdeccfg->cbcrInterleave;
	fbAllocInfo.mapType = pdeccfg->mapType;
	fbAllocInfo.stride = framebufStride;
	fbAllocInfo.height = framebufHeight;
	fbAllocInfo.size = framebufSize;
	fbAllocInfo.lumaBitDepth = seqInfo.lumaBitdepth;
	fbAllocInfo.chromaBitDepth = seqInfo.chromaBitdepth;
	fbAllocInfo.num = tiledFbCount;
	fbAllocInfo.endian = pdeccfg->frameEndian;
	fbAllocInfo.type = FB_TYPE_CODEC;
	osal_memset((void *)retFbAddrs, 0x00,
		    sizeof(vpu_buffer_t) * totalFbCount);

	if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
		for (index = 0; index < tiledFbCount; index++) {
			pvb = &retFbAddrs[index];
			pvb->size = framebufSize;
			CVI_VC_MEM("FB[%d], size = 0x%X\n", index, pvb->size);
			sprintf(ionName, "VDEC_%d_FrameBuffer", pvdec->chnNum);
			if (VDI_ALLOCATE_MEMORY(coreIndex, pvb, 0, ionName) <
			    0) {
				CVI_VC_ERR("fail to allocate frame buffer\n");
				ReleaseVideoMemory(coreIndex, retFbAddrs,
						   totalFbCount);
				return FALSE;
			}
			retFbArray[index].bufY = pvb->phys_addr;
			retFbArray[index].bufCb = (PhysicalAddress)-1;
			retFbArray[index].bufCr = (PhysicalAddress)-1;
			retFbArray[index].updateFbInfo = TRUE;
			retFbArray[index].size = framebufSize;
		}
	} else {
		stCviCb_HostAllocFB hostAllocFb;
		Int32 result;

		hostAllocFb.iFrmBufNum = totalFbCount;
		hostAllocFb.iFrmBufSize = framebufSize;
		result = pVdecDrvCbFunc(pvdec->chnNum, CVI_H26X_DEC_CB_AllocFB,
					(void *)&hostAllocFb);
		if (result == 0) {
			CVI_VC_ERR("fail to allocate frame buffer\n");
			return FALSE;
		}
	}

	if (tiledFbCount != 0) {
		ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo,
						 retFbArray);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR(
				"failed to VPU_DecAllocateFrameBuffer(), ret(%d)\n",
				ret);
			ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
			return FALSE;
		}
	}

	if (pdeccfg->enableWTL == TRUE || linearFbCount != 0) {
		size_t linearStride;
		size_t picWidth;
		size_t picHeight;
		size_t fbHeight;
		Uint32 mapType = LINEAR_FRAME_MAP;
		FrameBufferFormat outFormat = pdeccfg->wtlFormat;
		picWidth = seqInfo.picWidth;
		picHeight = seqInfo.picHeight;
		fbHeight = picHeight;

		{
			linearStride =
				CalcStride(picWidth, picHeight, outFormat,
					   pdeccfg->cbcrInterleave,
					   (TiledMapType)mapType, FALSE, TRUE);
		}

		framebufSize =
			VPU_GetFrameBufSize(coreIndex, linearStride, fbHeight,
					    (TiledMapType)mapType, outFormat,
					    pdeccfg->cbcrInterleave, pDramCfg);

		if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
			for (index = tiledFbCount; index < totalFbCount;
			     index++) {
				pvb = &retFbAddrs[index];
				pvb->size = framebufSize;
				CVI_VC_MEM(
					"tiledFbCount, FB[%d], pvb.size = 0x%X\n",
					index, pvb->size);
				memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
				sprintf(ionName, "VDEC_%d_tiledFrameBuffer",
					pvdec->chnNum);
				if (VDI_ALLOCATE_MEMORY(coreIndex, pvb, 0,
							ionName) < 0) {
					CVI_VC_ERR(
						"fail to allocate frame buffer\n");
					ReleaseVideoMemory(coreIndex,
							   retFbAddrs,
							   totalFbCount);
					return FALSE;
				}
				retFbArray[index].bufY = pvb->phys_addr;
				retFbArray[index].bufCb = -1;
				retFbArray[index].bufCr = -1;
				retFbArray[index].updateFbInfo = TRUE;
				retFbArray[index].size = framebufSize;
			}
		}

		fbAllocInfo.nv21 = pdeccfg->nv21;
		fbAllocInfo.format = outFormat;
		fbAllocInfo.num = linearFbCount;
		fbAllocInfo.mapType = (TiledMapType)mapType;
		fbAllocInfo.stride = linearStride;
		fbAllocInfo.height = fbHeight;
		ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo,
						 &retFbArray[tiledFbCount]);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR(
				"failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
				ret);
			ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
			return FALSE;
		}
	}

	return TRUE;
}

int _cviH265_FreeFrmBuf(cviVideoDecoder *pvdec)
{
	if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
		CVI_VC_WARN("Don't support E_CVI_VB_SRC_COMMON now!!\n");
		//		ReleaseVideoMemory(pdeccfg->coreIdx, pvdec->fbMem, totalFbCount);
	} else {
		pVdecDrvCbFunc(pvdec->chnNum, CVI_H26X_DEC_CB_FreeFB,
			       (void *)NULL);
	}

	return CVI_INIT_SEQ_OK;
}

static void _ReleasePreviousSequenceResources(DecHandle handle,
					      vpu_buffer_t *arrFbMem,
					      DecGetFramebufInfo *prevSeqFbInfo)
{
	Uint32 i;
	Uint32 coreIndex;

	if (handle == NULL) {
		return;
	}

	coreIndex = VPU_HANDLE_CORE_INDEX(handle);

	if (arrFbMem != NULL) {
		for (i = 0; i < MAX_REG_FRAME; i++) {
			if (arrFbMem[i].size > 0)
				VDI_FREE_MEMORY(coreIndex, &arrFbMem[i]);
		}
	}

	for (i = 0; i < MAX_REG_FRAME; i++) {
		if (prevSeqFbInfo->vbFbcYTbl[i].size > 0)
			VDI_FREE_MEMORY(coreIndex,
					&prevSeqFbInfo->vbFbcYTbl[i]);
		if (prevSeqFbInfo->vbFbcCTbl[i].size > 0)
			VDI_FREE_MEMORY(coreIndex,
					&prevSeqFbInfo->vbFbcCTbl[i]);
		if (prevSeqFbInfo->vbMvCol[i].size > 0)
			VDI_FREE_MEMORY(coreIndex, &prevSeqFbInfo->vbMvCol[i]);
	}
}

int _cviH265_Free_ReAlloc_FrmBuf(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;
	Uint32 index;
	RetCode ret;
	Uint32 *ptr;
	SequenceMemInfo *p;
	Uint32 framebufStride;
	Int32 compressedFbCount, linearFbCount;

	for (index = 0; index < MAX_REG_FRAME; index++) {
		// free allocated framebuffer
		if (pvdec->fbMem[index].size > 0) {
			VDI_FREE_MEMORY(pdeccfg->coreIdx, &pvdec->fbMem[index]);
		}
	}

	_cviH265_FreeFrmBuf(pvdec);

	ptr = (Uint32 *)Queue_Peek(pvdec->sequenceQ);
	index = (*ptr) % MAX_SEQUENCE_MEM_COUNT;
	p = &pvdec->seqMemInfo[index];
	_ReleasePreviousSequenceResources(pvdec->decHandle, NULL, &p->fbInfo);
	osal_memset(p, 0x00, sizeof(SequenceMemInfo));
	Queue_Dequeue(pvdec->sequenceQ);

	VPU_DecGiveCommand(pvdec->decHandle, DEC_RESET_FRAMEBUF_INFO, NULL);

	// Get current(changed) sequence information.
	VPU_DecGiveCommand(pvdec->decHandle, DEC_GET_SEQ_INFO, pSeqInfo);

	compressedFbCount =
		pSeqInfo->minFrameBufferCount +
		EXTRA_FRAME_BUFFER_NUM; /* max_dec_pic_buffering + @, @ >= 1 */
	linearFbCount = pSeqInfo->frameBufDelay +
			(1 + EXTRA_FRAME_BUFFER_NUM * 2);
			/* max_num_reorder_pics + @,  @ >= 1,
			 * In most case, # of linear fbs must be greater or equal than max_num_reorder,
			 * but the expression of @ in the sample code is in order to make the situation
			 * that # of linear is greater than # of fbc.
			 */
	osal_memset((void *)pvdec->fbMem, 0x00,
		    sizeof(vpu_buffer_t) * MAX_REG_FRAME);

	if (_AllocateH265DecFrameBuffer(pvdec, pvdec->decHandle, pdeccfg,
					compressedFbCount, linearFbCount,
					pvdec->Frame, pvdec->fbMem,
					&framebufStride) == FALSE) {
		CVI_VC_ERR("[SEQ_CHANGE] AllocateDecFrameBuffer failed\n");
		return CVI_ERR_DECODE_END;
	}
	ret = VPU_DecRegisterFrameBufferEx(pvdec->decHandle, pvdec->Frame,
					   compressedFbCount, linearFbCount,
					   framebufStride, pSeqInfo->picHeight,
					   COMPRESSED_FRAME_MAP);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"[SEQ_CHANGE] VPU_DecRegisterFrameBufferEx failed Error code is 0x%x\n",
			ret);
		return CVI_ERR_DECODE_END;
	}

	return CVI_INIT_SEQ_OK;
}

int _cviH264_Free_ReAlloc_FrmBuf(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	CVI_DEC_STATUS decStatus;
	Int32 index;
	vpu_buffer_t *pvb = NULL;

	VPU_DecGiveCommand(pvdec->decHandle, DEC_FREE_FRAME_BUFFER, 0x00);
	for (index = 0; index < MAX_REG_FRAME; index++) {
		pvb = &pvdec->fbMem[index];
		if (pvb->size > 0)
			VDI_FREE_MEMORY(pdeccfg->coreIdx, pvb);

		pvb = &pvdec->PPUFbMem[index];
		if (pvb->size > 0)
			VDI_FREE_MEMORY(pdeccfg->coreIdx, pvb);
	}
	pvdec->seqInited = FALSE;

	decStatus = cviInitSeq(pvdec);
	if (decStatus == CVI_ERR_DECODE_END) {
		CVI_VC_ERR("cviInitSeq, decStatus = %d\n", decStatus);
	}

	return decStatus;
}

int _cviH265_AllocFrmBuf(cviVideoDecoder *pvdec)
{
	DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	SecAxiUse *psau = &pvdec->secAxiUse;
	Uint32 framebufStride; //, framebufSize;
	Int32 compressedFbCount, linearFbCount;
	RetCode ret;
	Uint32 val;
	BOOL bRet = FALSE;

	/********************************************************************************
    * ALLOCATE FRAME BUFFER                                                        *
    ********************************************************************************/
	/* Set up the secondary AXI is depending on H/W configuration.
    * Note that turn off all the secondary AXI configuration
    * if target ASIC has no memory only for IP, LF and BIT.
    */
	psau->u.wave4.useIpEnable =
		(pdeccfg->secondaryAXI & 0x01) ? TRUE : FALSE;
	psau->u.wave4.useLfRowEnable =
		(pdeccfg->secondaryAXI & 0x02) ? TRUE : FALSE;
	psau->u.wave4.useBitEnable =
		(pdeccfg->secondaryAXI & 0x04) ? TRUE : FALSE;
	VPU_DecGiveCommand(pvdec->decHandle, SET_SEC_AXI, psau);

	compressedFbCount = pSeqInfo->minFrameBufferCount +
			    EXTRA_FRAME_BUFFER_NUM; // max_dec_pic_buffering
	if (pdeccfg->enableWTL == TRUE) {
		linearFbCount = compressedFbCount;
		VPU_DecGiveCommand(pvdec->decHandle, DEC_SET_WTL_FRAME_FORMAT,
				   &pdeccfg->wtlFormat);
	} else {
		linearFbCount = 0;
	}

	CVI_VC_INFO(
		"min FB Buf Count = %d, compressedFbCount = %d, linearFbCount = %d\n",
		pSeqInfo->minFrameBufferCount, compressedFbCount,
		linearFbCount);

	osal_memset((void *)pvdec->fbMem, 0x00,
		    sizeof(vpu_buffer_t) * MAX_REG_FRAME);

	bRet = _AllocateH265DecFrameBuffer(pvdec, pvdec->decHandle, pdeccfg,
					   compressedFbCount, linearFbCount,
					   pvdec->Frame, pvdec->fbMem,
					   &framebufStride);
	if (bRet == FALSE) {
		return CVI_ERR_DEC_INIT;
	}

	/********************************************************************************
    * REGISTER FRAME BUFFER                                                        *
    ********************************************************************************/
	CVI_VC_FLOW("REGISTER FRAME BUFFER\n");
	ret = VPU_DecRegisterFrameBufferEx(pvdec->decHandle, pvdec->Frame,
					   compressedFbCount, linearFbCount,
					   framebufStride, pSeqInfo->picHeight,
					   COMPRESSED_FRAME_MAP);
	if (ret != RETCODE_SUCCESS) {
		VLOG(ERR,
		     "VPU_DecRegisterFrameBuffer failed Error code is 0x%x\n",
		     ret);
		return CVI_ERR_DEC_INIT;
	}

	pvdec->doDumpImage = (BOOL)(pdeccfg->compareType == YUV_COMPARE ||
				    (strlen(pdeccfg->outputPath) > 0) ||
				    pdeccfg->renderType == RENDER_DEVICE_FBDEV);

	val = (pdeccfg->bitFormat == STD_HEVC) ? SEQ_CHANGE_ENABLE_ALL_HEVC :
						       SEQ_CHANGE_ENABLE_ALL_VP9;
	VPU_DecGiveCommand(pvdec->decHandle, DEC_SET_SEQ_CHANGE_MASK,
			   (void *)&val);

	return CVI_INIT_SEQ_OK;
}

int _cviH264_FreeFrmBuf(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	vpu_buffer_t *pvb = NULL;
	int index;

	if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
		for (index = 0; index < pvdec->fbCount; index++) {
			pvb = &pvdec->fbMem[index];
			if (pvb->size > 0)
				VDI_FREE_MEMORY(pdeccfg->coreIdx, pvb);
		}
	} else {
		pVdecDrvCbFunc(pvdec->chnNum, CVI_H26X_DEC_CB_FreeFB,
			       (void *)NULL);
	}

	return CVI_INIT_SEQ_OK;
}

int _cviH264_AllocFrmBuf(cviVideoDecoder *pvdec)
{
	MaverickCacheConfig *pCacheCfg = &pvdec->cacheCfg;
	FrameBufferAllocInfo *pFbAllocInfo = &pvdec->fbAllocInfo;
	DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	SecAxiUse *psau = &pvdec->secAxiUse;
	Uint32 framebufStride, framebufSize;
	Int32 index = 0;
	FrameBuffer *pfb;
	vpu_buffer_t *pvb = NULL;
	RetCode ret;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	/********************************************************************************
	 * ALLOCATE RECON FRAMEBUFFERS *
	 ********************************************************************************/
	pvdec->framebufHeight = VPU_ALIGN32(pSeqInfo->picHeight);
	framebufStride = CalcStride(pSeqInfo->picWidth, pSeqInfo->picHeight,
				    FORMAT_420, pdeccfg->cbcrInterleave,
				    pdeccfg->mapType, FALSE, TRUE);
	framebufSize =
		VPU_GetFrameBufSize(pdeccfg->coreIdx, framebufStride,
				    pvdec->framebufHeight, pdeccfg->mapType,
				    FORMAT_420, pdeccfg->cbcrInterleave, NULL);
	pvdec->fbCount = pSeqInfo->minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;

	if (pvdec->fbCount > MAX_REG_FRAME) {
		CVI_VC_ERR("pvdec->fbCount > MAX_REG_FRAME\n");
		return CVI_ERR_DECODE_END;
	}

	pFbAllocInfo->format = FORMAT_420;
	pFbAllocInfo->cbcrInterleave = pdeccfg->cbcrInterleave;
	pFbAllocInfo->mapType = pdeccfg->mapType;
	pFbAllocInfo->stride = framebufStride;
	pFbAllocInfo->height = pvdec->framebufHeight;
	pFbAllocInfo->lumaBitDepth = pSeqInfo->lumaBitdepth;
	pFbAllocInfo->chromaBitDepth = pSeqInfo->chromaBitdepth;
	pFbAllocInfo->num = pvdec->fbCount;
	pFbAllocInfo->endian = pdeccfg->frameEndian;
	pFbAllocInfo->type = FB_TYPE_CODEC;

	if (pvdec->eVdecVBSource == E_CVI_VB_SRC_COMMON) {
		for (index = 0; index < pvdec->fbCount; index++) {
			pvb = &pvdec->fbMem[index];
			pvb->size = framebufSize;
			CVI_VC_MEM("FB[%d], pvb.size = 0x%X\n", index,
				   pvb->size);
			sprintf(ionName, "VDEC_%d_FrameBuffer", pvdec->chnNum);
			if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, pvb, 0,
						ionName) < 0) {
				CVI_VC_ERR("fail to allocate frame buffer\n");
				return CVI_ERR_DECODE_END;
			}
			pfb = &pvdec->Frame[index];
			pfb->bufY = pvb->phys_addr;
			pfb->bufCb = -1;
			pfb->bufCr = -1;
			pfb->updateFbInfo = TRUE;
		}
	} else {
		stCviCb_HostAllocFB hostAllocFb;
		Int32 result;

		hostAllocFb.iFrmBufNum = pvdec->fbCount;
		hostAllocFb.iFrmBufSize = framebufSize;
		result = pVdecDrvCbFunc(pvdec->chnNum, CVI_H26X_DEC_CB_AllocFB,
					(void *)&hostAllocFb);

		if (result == 0) {
			CVI_VC_ERR("fail to allocate frame buffer\n");
			return CVI_ERR_DECODE_END;
		}
	}

	ret = VPU_DecAllocateFrameBuffer(pvdec->decHandle, pvdec->fbAllocInfo,
					 pvdec->Frame);
	if (ret != RETCODE_SUCCESS) {
		VLOG(ERR,
		     "%s:%d failed to VPU_DecAllocateFrameBuffer(), ret(%d)\n",
		     __func__, __LINE__, ret);
		return CVI_ERR_DECODE_END;
	}
	/********************************************************************************
	 * ALLOCATE WTL FRAMEBUFFERS *
	 ********************************************************************************/
	if (pdeccfg->enableWTL == TRUE) {
		TiledMapType linearMapType = pdeccfg->wtlMode == FF_FRAME ?
							   LINEAR_FRAME_MAP :
							   LINEAR_FIELD_MAP;
		Uint32 strideWtl;

		strideWtl = CalcStride(pSeqInfo->picWidth, pSeqInfo->picHeight,
				       FORMAT_420, pdeccfg->cbcrInterleave,
				       linearMapType, FALSE, TRUE);
		framebufSize =
			VPU_GetFrameBufSize(pdeccfg->coreIdx, strideWtl,
					    pvdec->framebufHeight,
					    linearMapType, FORMAT_420,
					    pdeccfg->cbcrInterleave, NULL);

		for (index = pvdec->fbCount; index < pvdec->fbCount * 2;
		     index++) {
			pvb = &pvdec->fbMem[index];
			pvb->size = framebufSize;
			CVI_VC_MEM("wtlEnable = 1, FB[%d], pvb.size = 0x%X\n",
				   index, pvb->size);
			memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
			sprintf(ionName, "VDEC_%d_FrameBuffer", pvdec->chnNum);
			if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, pvb, 0,
						ionName) < 0) {
				VLOG(ERR,
				     "%s:%d fail to allocate frame buffer\n",
				     __func__, __LINE__);
				return CVI_ERR_DECODE_END;
			}
			pfb = &pvdec->Frame[index];
			pfb->bufY = pvb->phys_addr;
			pfb->bufCb = -1;
			pfb->bufCr = -1;
			pfb->updateFbInfo = TRUE;
		}
		pFbAllocInfo->mapType = linearMapType;
		pFbAllocInfo->cbcrInterleave = pdeccfg->cbcrInterleave;
		pFbAllocInfo->nv21 = pdeccfg->nv21;
		pFbAllocInfo->format = FORMAT_420;
		pFbAllocInfo->stride = strideWtl;
		pFbAllocInfo->height = pvdec->framebufHeight;
		pFbAllocInfo->endian = pdeccfg->frameEndian;
		pFbAllocInfo->lumaBitDepth = 8;
		pFbAllocInfo->chromaBitDepth = 8;
		pFbAllocInfo->num = pvdec->fbCount;
		pFbAllocInfo->type = FB_TYPE_CODEC;
		ret = VPU_DecAllocateFrameBuffer(pvdec->decHandle,
						 pvdec->fbAllocInfo,
						 &pvdec->Frame[pvdec->fbCount]);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "%s:%d failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
			     __func__, __LINE__, ret);
			return CVI_ERR_DECODE_END;
		}
	}

	/********************************************************************************
	 * SET_FRAMEBUF *
	 ********************************************************************************/
	psau->u.coda9.useBitEnable = (pdeccfg->secondaryAXI >> 0) & 0x01;
	psau->u.coda9.useIpEnable = (pdeccfg->secondaryAXI >> 1) & 0x01;
	psau->u.coda9.useDbkYEnable = (pdeccfg->secondaryAXI >> 2) & 0x01;
	psau->u.coda9.useDbkCEnable = (pdeccfg->secondaryAXI >> 3) & 0x01;
	psau->u.coda9.useOvlEnable = (pdeccfg->secondaryAXI >> 4) & 0x01;
	psau->u.coda9.useBtpEnable = (pdeccfg->secondaryAXI >> 5) & 0x01;
	VPU_DecGiveCommand(pvdec->decHandle, SET_SEC_AXI, psau);

	MaverickCache2Config(pCacheCfg, TRUE /*Decoder*/,
			     pdeccfg->cbcrInterleave,
			     pdeccfg->coda9.frameCacheBypass,
			     pdeccfg->coda9.frameCacheBurst,
			     pdeccfg->coda9.frameCacheMerge, pdeccfg->mapType,
			     pdeccfg->coda9.frameCacheWayShape);
	VPU_DecGiveCommand(pvdec->decHandle, SET_CACHE_CONFIG, pCacheCfg);

	framebufStride =
		CalcStride(pSeqInfo->picWidth, pSeqInfo->picHeight, FORMAT_420,
			   pdeccfg->cbcrInterleave,
			   pdeccfg->enableWTL == TRUE ? LINEAR_FRAME_MAP :
							      pdeccfg->mapType,
			   FALSE, TRUE);
	ret = VPU_DecRegisterFrameBuffer(pvdec->decHandle, pvdec->Frame,
					 pvdec->fbCount, framebufStride,
					 pvdec->framebufHeight,
					 (int)pdeccfg->mapType);
	if (ret != RETCODE_SUCCESS) {
#ifdef REDUNDENT_CODE
		if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
			PrintMemoryAccessViolationReason(pdeccfg->coreIdx,
							 NULL);
#endif
		VLOG(ERR,
		     "VPU_DecRegisterFrameBuffer failed Error code is 0x%x\n",
		     ret);
		return CVI_ERR_DECODE_END;
	}
	/********************************************************************************
	 * ALLOCATE PPU FRAMEBUFFERS (When rotator, mirror or tiled2linear are
	 *enabled) * NOTE: VPU_DecAllocateFrameBuffer() WITH PPU FRAMEBUFFER
	 *SHOULD BE CALLED     * AFTER VPU_DecRegisterFrameBuffer(). *
	 ********************************************************************************/
	pvdec->enablePPU =
		(BOOL)(pdeccfg->coda9.rotate > 0 || pdeccfg->coda9.mirror > 0 ||
		       pdeccfg->coda9.enableTiled2Linear == TRUE ||
		       pdeccfg->coda9.enableDering == TRUE);
	if (pvdec->enablePPU == TRUE) {
		Uint32 stridePpu;
		Uint32 sizePPUFb;
		Uint32 rotate = pdeccfg->coda9.rotate;
		Uint32 rotateWidth = pSeqInfo->picWidth;
		Uint32 rotateHeight = pSeqInfo->picHeight;

		if (rotate == 90 || rotate == 270) {
			rotateWidth = pSeqInfo->picHeight;
			rotateHeight = pSeqInfo->picWidth;
		}
		rotateWidth = VPU_ALIGN32(rotateWidth);
		rotateHeight = VPU_ALIGN32(rotateHeight);

		stridePpu = CalcStride(rotateWidth, rotateHeight, FORMAT_420,
				       pdeccfg->cbcrInterleave,
				       LINEAR_FRAME_MAP, FALSE, TRUE);
		sizePPUFb = VPU_GetFrameBufSize(pdeccfg->coreIdx, stridePpu,
						rotateHeight, LINEAR_FRAME_MAP,
						FORMAT_420,
						pdeccfg->cbcrInterleave, NULL);

		for (index = 0; index < pvdec->ppuFbCount; index++) {
			pvb = &pvdec->PPUFbMem[index];
			pvb->size = sizePPUFb;
			CVI_VC_MEM("PPU, FB[%d], pvb.size = 0x%X\n", index,
				   pvb->size);
			memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
			sprintf(ionName, "VDEC_%d_PPUFb", pvdec->chnNum);
			if (VDI_ALLOCATE_MEMORY(pdeccfg->coreIdx, pvb, 0,
						ionName) < 0) {
				VLOG(ERR,
				     "%s:%d fail to allocate frame buffer\n",
				     __func__, __LINE__);
				return CVI_ERR_DECODE_END;
			}
			pfb = &pvdec->PPUFrame[index];
			pfb->bufY = pvb->phys_addr;
			pfb->bufCb = -1;
			pfb->bufCr = -1;
			pfb->updateFbInfo = TRUE;
		}

		pFbAllocInfo->mapType = LINEAR_FRAME_MAP;
		pFbAllocInfo->cbcrInterleave = pdeccfg->cbcrInterleave;
		pFbAllocInfo->nv21 = pdeccfg->nv21;
		pFbAllocInfo->format = FORMAT_420;
		pFbAllocInfo->stride = stridePpu;
		pFbAllocInfo->height = rotateHeight;
		pFbAllocInfo->endian = pdeccfg->frameEndian;
		pFbAllocInfo->lumaBitDepth = 8;
		pFbAllocInfo->chromaBitDepth = 8;
		pFbAllocInfo->num = pvdec->ppuFbCount;
		pFbAllocInfo->type = FB_TYPE_PPU;

		ret = VPU_DecAllocateFrameBuffer(
			pvdec->decHandle, pvdec->fbAllocInfo, pvdec->PPUFrame);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "%s:%d failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
			     __func__, __LINE__, ret);
			return CVI_ERR_DECODE_END;
		}
		// Note: Please keep the below call sequence.
		VPU_DecGiveCommand(pvdec->decHandle, SET_ROTATION_ANGLE,
				   (void *)&pdeccfg->coda9.rotate);
		VPU_DecGiveCommand(pvdec->decHandle, SET_MIRROR_DIRECTION,
				   (void *)&pdeccfg->coda9.mirror);
		VPU_DecGiveCommand(pvdec->decHandle, SET_ROTATOR_STRIDE,
				   (void *)&stridePpu);

		pvdec->ppuQ = Queue_Create(MAX_REG_FRAME, sizeof(FrameBuffer));
		if (pvdec->ppuQ == NULL) {
			CVI_VC_ERR("ppuQ = NULL\n");
			return CVI_ERR_DECODE_END;
		}
		for (index = 0; index < pvdec->ppuFbCount; index++) {
			Queue_Enqueue(pvdec->ppuQ,
				      (void *)&pvdec->PPUFrame[index]);
		}
	}

#ifdef REDUNDENT_CODE
	PrepareDecoderTest(pvdec->decHandle);
#endif
	pvdec->doDumpImage = (BOOL)(pdeccfg->compareType == YUV_COMPARE ||
				    pdeccfg->renderType == RENDER_DEVICE_FBDEV);
	pvdec->waitPostProcessing = pvdec->enablePPU;
	pvdec->needStream = FALSE;
	pvdec->prevFbIndex = -1;

	return CVI_INIT_SEQ_OK;
}

static int cviInitSeq(cviVideoDecoder *pvdec)
{
	DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	BOOL seqInitEscape;
	RetCode ret;

	seqInitEscape = FALSE;
	ret = VPU_DecSetEscSeqInit(pvdec->decHandle, seqInitEscape);
	if (ret != RETCODE_SUCCESS) {
		seqInitEscape = 0;
		CVI_VC_INFO(
			"can not to set seqInitEscape in current bitstream mode Option\n");
	}

	if (seqInitEscape) {
		// if you set to seqInitEscape option to TRUE, It is more easy
		// to control that APP uses VPU_DecGetInitialInfo instead of
		// VPU_DecIssueSeqInit and VPU_DecCompleteSeqInit
		ret = VPU_DecGetInitialInfo(pvdec->decHandle, pSeqInfo);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "VPU_DecGetInitialInfo failed Error code is 0x%x\n",
			     ret);
			CVI_VC_ERR("Error reason is 0x%x\n",
				   pSeqInfo->seqInitErrReason);
			return CVI_ERR_DECODE_END;
		}
	} else {
		Int32 timeoutCount = 0;
		Int32 interruptFlag = 0;

		// Scan and retrieve sequence header information from the
		// bitstream buffer.
		ret = VPU_DecIssueSeqInit(pvdec->decHandle);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR("VPU_DecIssueSeqInit, 0x%x\n", ret);
			return CVI_ERR_DECODE_END;
		}
		timeoutCount = 0;
		while (pvdec->seqInited == FALSE) {
			// wait for 10ms to save stream filling time.
			interruptFlag = VPU_WaitInterrupt(pdeccfg->coreIdx,
							  VPU_WAIT_TIME_OUT);
			if (interruptFlag == -1) {
				if (timeoutCount * VPU_WAIT_TIME_OUT >
				    VPU_DEC_TIMEOUT) {
					VLOG(ERR,
					     "\n VPU interrupt wait timeout\n");
					VPU_SWReset(pdeccfg->coreIdx,
						    SW_RESET_SAFETY,
						    pvdec->decHandle);
					return CVI_ERR_DECODE_END;
				}
				timeoutCount++;
				interruptFlag = 0;
			}

			if (interruptFlag) {
				VPU_ClearInterrupt(pdeccfg->coreIdx);
				if (interruptFlag & (1 << INT_BIT_SEQ_INIT)) {
					pvdec->seqInited = TRUE;
					CVI_VC_BS("seqInited = TRUE\n");
					break;
				}
			}

			if (pdeccfg->bitstreamMode != BS_MODE_PIC_END) {
				pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);
				if (pvdec->nWritten < 0) {
					CVI_VC_ERR("nWritten < 0\n");
					return CVI_ERR_DECODE_END;
				}
			}
		}

		ret = VPU_DecCompleteSeqInit(pvdec->decHandle, pSeqInfo);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR("SEQ_INIT(ERROR REASON: %d(0x%x)\n",
				   pSeqInfo->seqInitErrReason,
				   pSeqInfo->seqInitErrReason);
			pvdec->seqInited = FALSE;
			CVI_VC_ERR("seqInited = FALSE\n");
			return CVI_ERR_DECODE_END;
		}
		if (pvdec->seqInited == FALSE) {
			CVI_VC_ERR("seqInited = FALSE\n");
			return CVI_ERR_DECODE_END;
		}
	}

	PrintDecSeqWarningMessages(pvdec->productId, pSeqInfo);
#ifdef REDUNDENT_CODE
	if (pvdec->comparator == NULL) {
		char *goldenPath = NULL;

		switch (pdeccfg->compareType) {
		case NO_COMPARE:
			goldenPath = NULL;
			pvdec->comparator = Comparator_Create(
				pdeccfg->compareType, goldenPath);
			break;

		case YUV_COMPARE:
			goldenPath = pdeccfg->refYuvPath;
			pvdec->comparator = Comparator_Create(
				pdeccfg->compareType, goldenPath,
				VPU_ALIGN16(pSeqInfo->picWidth),
				VPU_ALIGN16(pSeqInfo->picHeight),
				pdeccfg->wtlFormat, pdeccfg->cbcrInterleave);
			break;

		default:
			CVI_VC_ERR("compareType = %d\n", pdeccfg->compareType);
			return CVI_ERR_DECODE_END;
		}
	}

	if (pvdec->comparator == NULL) {
		CVI_VC_ERR("comparator = NULL\n");
		return CVI_ERR_DECODE_END;
	}
#endif
	if (pdeccfg->bitFormat == STD_AVC) {
		ret = _cviH264_FreeFrmBuf(pvdec);
	} else {
		ret = _cviH265_FreeFrmBuf(pvdec);
	}

	if ((int)ret != CVI_INIT_SEQ_OK) {
		CVI_VC_ERR("Alloc VDEC Frame Buffer Fail!\n");
		return CVI_ERR_DECODE_END;
	}

	if (pdeccfg->bitFormat == STD_AVC)
		ret = _cviH264_AllocFrmBuf(pvdec);
	else
		ret = _cviH265_AllocFrmBuf(pvdec);

	if ((int)ret != CVI_INIT_SEQ_OK) {
		CVI_VC_ERR("Alloc VDEC Frame Buffer Fail!\n");
		return CVI_ERR_DECODE_END;
	}

	DisplayDecodedInformation(pvdec->decHandle, (CodStd)pdeccfg->bitFormat,
				  0, NULL);

	return CVI_INIT_SEQ_OK;
}

int _cviH265_DecodeOneFrame(cviVideoDecoder *pvdec)
{
	/********************************************************************************
	* DEC_PIC                                                                       *
	********************************************************************************/

	cviDecConfig *pdeccfg = &pvdec->decConfig;
	RetCode ret;
#ifdef VC_DRIVER_TEST
	DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;
	DecOutputInfo *pDisplayInfo;
	Uint32 index;
	if (pvdec->cviApiMode == API_MODE_DRIVER) {
		pDisplayInfo = (DecOutputInfo *)SimpleRenderer_GetFreeFrameInfo(
			pvdec->renderer);

		if (pDisplayInfo != NULL) {
			Uint32 *ptr;

			ptr = (Uint32 *)Queue_Peek(pvdec->sequenceQ);
			if (ptr && *ptr != pDisplayInfo->sequenceNo) {
				/* Release all framebuffers of previous sequence */
				SequenceMemInfo *p;
				index = (*ptr) % MAX_SEQUENCE_MEM_COUNT;
				p = &pvdec->seqMemInfo[index];
				_ReleasePreviousSequenceResources(
					pvdec->decHandle, p->allocFbMem,
					&p->fbInfo);
				osal_memset(p, 0x00, sizeof(SequenceMemInfo));
				Queue_Dequeue(pvdec->sequenceQ);
			}
			if (pDisplayInfo->sequenceNo == pSeqInfo->sequenceNo) {
				VPU_DecClrDispFlag(
					pvdec->decHandle,
					pDisplayInfo->indexFrameDisplay);
			}
		}
	}
#endif
	if ((pvdec->cviApiMode == API_MODE_SDK) &&
	    (pvdec->waitFrmBufFree == TRUE)) {
		Int32 count;

		count = pVdecDrvCbFunc(pvdec->chnNum,
				       CVI_H26X_DEC_CB_GET_DISPQ_COUNT, NULL);

		if (count == 0) {
			pvdec->waitFrmBufFree = FALSE;

			CVI_VC_TRACE("Seq Chnge: Free Buffer. Realloc Buffer\n");
			// Free Buffer. Realloc Buffer
			if (_cviH265_Free_ReAlloc_FrmBuf(pvdec) != CVI_INIT_SEQ_OK) {
				return CVI_ERR_DECODE_END;
			}
		} else {
			return CVI_DECODE_NO_FB;
		}
	}

	if (pvdec->needStream == TRUE) {
		pvdec->bsQueueIndex =
			(pvdec->bsQueueIndex + 1) % (pvdec->bsBufferCount);
		VPU_DecSetRdPtr(pvdec->decHandle,
				pvdec->vbStream[pvdec->bsQueueIndex].phys_addr,
				TRUE);
		pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);

		if (pvdec->nWritten < 0) {
			CVI_VC_ERR("nWritten = %d\n", pvdec->nWritten);
			return CVI_ERR_DECODE_END;
		}
	}

	pvdec->decParam.skipframeMode = pdeccfg->skipMode;
	if (pvdec->decParam.skipframeMode == 1) {
		SimpleRenderer_Flush(pvdec->renderer);
	}

	if (pvdec->bStreamOfEnd == TRUE) {
		// Now that we are done with decoding, close the open instance.
		VPU_DecUpdateBitstreamBuffer(pvdec->decHandle, STREAM_END_SIZE);
	}

	// Start decoding a frame.
	pvdec->decParam.craAsBlaFlag = pdeccfg->wave4.craAsBla;
	ret = VPU_DecStartOneFrame(pvdec->decHandle, &pvdec->decParam);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_DecStartOneFrame failed Error code is 0x%x\n",
			   ret);
		return CVI_ERR_DECODE_END;
	}

	return CVI_DECODE_ONE_FRAME_OK;
}

int _cviH264_DecodeOneFrame(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecParam *pdp = &pvdec->decParam;
	RetCode ret;
	int status;
#ifdef VC_DRIVER_TEST
	DecOutputInfo *pDisplayInfo;

	if (pvdec->cviApiMode == API_MODE_DRIVER) {
		do {
			pDisplayInfo = (DecOutputInfo *)
				SimpleRenderer_GetFreeFrameInfo(
					pvdec->renderer);
			if (!pDisplayInfo)
				break;

			if (pvdec->enablePPU == TRUE) {
				Queue_Enqueue(pvdec->ppuQ,
					      (void *)&pDisplayInfo->dispFrame);
			} else {
				CVI_VC_DISP("myIndex = %d\n",
					    pDisplayInfo->dispFrame.myIndex);
				VPU_DecClrDispFlag(
					pvdec->decHandle,
					pDisplayInfo->dispFrame.myIndex);
			}
		} while (1);
	}
#endif
	if (pvdec->needStream == TRUE) {
		VPU_DecSetRdPtr(pvdec->decHandle, pvdec->vbStream[0].phys_addr,
				TRUE);

		if (pvdec->assertedFieldDoneInt == FALSE) {
			pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);
		} else {
			pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);
		}

		if (pvdec->nWritten < 0) {
			CVI_VC_ERR("nWritten = %d\n", pvdec->nWritten);
			status = CVI_ERR_DECODE_END;
			goto ERR_CVI_H264_DECODE_ONE_FRAME;
		}

		pvdec->needStream = FALSE;
	}

	if ((pvdec->cviApiMode == API_MODE_SDK) &&
	    (pvdec->waitFrmBufFree == TRUE)) {
		Int32 count;

		count = pVdecDrvCbFunc(pvdec->chnNum,
				       CVI_H26X_DEC_CB_GET_DISPQ_COUNT, NULL);
		if (count == 0) {
			pvdec->waitFrmBufFree = FALSE;

			CVI_VC_TRACE("Seq Chnge: Free Buffer. Realloc Buffer\n");
			// Release all memory related to framebuffer.
			if (_cviH264_Free_ReAlloc_FrmBuf(pvdec) == CVI_ERR_DECODE_END) {
				return CVI_ERR_DECODE_END;
			}
		} else {
			pvdec->needStream = TRUE;
			return CVI_DECODE_NO_FB;
		}
	}

	if (pvdec->assertedFieldDoneInt == TRUE) {
		VPU_ClearInterrupt(pdeccfg->coreIdx);
		pvdec->assertedFieldDoneInt = FALSE;
	} else {
		// When the field done interrupt is asserted, just fill
		// elementary stream into the bitstream buffer.
		if (pvdec->enablePPU == TRUE) {
			pvdec->ppuFb =
				(FrameBuffer *)Queue_Dequeue(pvdec->ppuQ);
			if (pvdec->ppuFb == NULL) {
				MSleep(0);
				pvdec->needStream = FALSE;
				status = CVI_DECODE_CONTINUE;
				goto ERR_CVI_H264_DECODE_ONE_FRAME;
			}
			VPU_DecGiveCommand(pvdec->decHandle, SET_ROTATOR_OUTPUT,
					   (void *)pvdec->ppuFb);
			if (pdeccfg->coda9.rotate > 0) {
				VPU_DecGiveCommand(pvdec->decHandle,
						   ENABLE_ROTATION, NULL);
			}
			if (pdeccfg->coda9.mirror > 0) {
				VPU_DecGiveCommand(pvdec->decHandle,
						   ENABLE_MIRRORING, NULL);
			}
			if (pdeccfg->coda9.enableDering == TRUE) {
				VPU_DecGiveCommand(pvdec->decHandle,
						   ENABLE_DERING, NULL);
			}
		}

		VPU_DecGiveCommand(pvdec->decHandle, DEC_SET_TARGET_TEMPORAL_ID,
				   (void *)&pdeccfg->tid);

		if (pvdec->bStreamOfEnd == TRUE) {
			// Now that we are done with decoding, close the open instance.
			VPU_DecUpdateBitstreamBuffer(pvdec->decHandle,
						     STREAM_END_SIZE);
		}

		// Start decoding a frame.
		ret = VPU_DecStartOneFrame(pvdec->decHandle, pdp);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR(
				"VPU_DecStartOneFrame failed Error code is 0x%x\n",
				ret);
			status = CVI_ERR_DECODE_END;
			goto ERR_CVI_H264_DECODE_ONE_FRAME;
		}
	}

	return CVI_DECODE_ONE_FRAME_OK;

ERR_CVI_H264_DECODE_ONE_FRAME:
	return status;
}

int cviDecodeOneFrame(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	int ret = CVI_DECODE_ONE_FRAME_OK;

	if (pdeccfg->bitFormat == STD_AVC)
		ret = _cviH264_DecodeOneFrame(pvdec);
	else
		ret = _cviH265_DecodeOneFrame(pvdec);

	return ret;
}

int _cviH265_WaitInterrupt(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	BOOL repeat;
	Int32 timeoutCount;
	Int32 interruptFlag;

	timeoutCount = 0;
	repeat = TRUE;
	while (repeat == TRUE) {
		interruptFlag =
			VPU_WaitInterrupt(pdeccfg->coreIdx, VPU_WAIT_TIME_OUT);
		if (interruptFlag == -1) {
			// wait for 10ms to save stream filling time.
			if (timeoutCount * VPU_WAIT_TIME_OUT >
			    VPU_DEC_TIMEOUT) {
#ifdef ENABLE_CNM_DEBUG_MSG
				PrintVpuStatus(pdeccfg->coreIdx,
					       pvdec->productId);
#endif
				CVI_VC_WARN(
					"\n VPU interrupt wait timeout instIdx=%d\n",
					pdeccfg->instIdx);
				VPU_SWReset(pdeccfg->coreIdx, SW_RESET_SAFETY,
					    pvdec->decHandle);
				return CVI_ERR_DECODE_END;
			}
			timeoutCount++;
			interruptFlag = 0;
		}

		if (interruptFlag > 0) {
			VPU_ClearInterrupt(pdeccfg->coreIdx);
		}

		if (interruptFlag & (1 << INT_WAVE_DEC_PIC)) {
			repeat = FALSE;
			CVI_VC_TRACE("DEC_PIC\n");
		}

		if (interruptFlag & (1 << INT_WAVE_BIT_BUF_EMPTY)) {
			/* TODO: handling empty interrupt */
			Uint32 avail;
			VPU_DecGetBitstreamBuffer(pvdec->decHandle, NULL, NULL,
						  &avail);
			if (avail < FEEDING_SIZE) {
				VPU_DecUpdateBitstreamBuffer(
					pvdec->decHandle,
					0); // Set end-of-stream
				VLOG(ERR, "Not enough bitstream buffer\n");
				break;
			}
		}

		if (repeat == FALSE)
			break;

		// In PICEND mode, the below codes are not called.
		if (pdeccfg->bitstreamMode == BS_MODE_INTERRUPT &&
		    pvdec->seqChangeRequest == FALSE) {
			if (pvdec->cviApiMode == API_MODE_SDK) {
				PhysicalAddress rdPtr, wrPtr;
				Uint32 room;

				CVI_VC_BS("SDK, BS_MODE_INTERRUPT\n");
				VPU_DecGetBitstreamBuffer(pvdec->decHandle,
							  &rdPtr, &wrPtr,
							  &room);
				CVI_VC_BS(
					"rdPtr = 0x%llX, wrPtr = 0x%llX, room = 0x%X\n",
					rdPtr, wrPtr, room);
			}
#ifdef VC_DRIVER_TEST
			else {
				pvdec->nWritten = BitstreamFeeder_Act(
					pvdec->decHandle, pvdec->feeder,
					pdeccfg->streamEndian, NULL);
				if (pvdec->nWritten < 0) {
					return CVI_ERR_DECODE_END;
				}
			}
#endif
		}
	}

	return CVI_WAIT_INT_OK;
}

int _cviH264_WaitInterrupt(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
#ifdef REDUNDENT_CODE
	DecOutputInfo *pdoi = &pvdec->outputInfo;
#endif
	BOOL repeat;
	Int32 timeoutCount;
	Int32 interruptFlag;

	timeoutCount = 0;
	repeat = TRUE;
	while (repeat == TRUE) {
		interruptFlag =
			VPU_WaitInterrupt(pdeccfg->coreIdx, VPU_WAIT_TIME_OUT);
		if (interruptFlag == -1) {
			// wait for 10ms to save stream filling time.
			if (timeoutCount * VPU_WAIT_TIME_OUT >
			    VPU_DEC_TIMEOUT) {
#ifdef ENABLE_CNM_DEBUG_MSG
				PrintVpuStatus(pdeccfg->coreIdx,
					       pvdec->productId);
#endif
				CVI_VC_WARN(
					"\n VPU interrupt wait timeout instIdx=%d\n",
					pdeccfg->instIdx);
				VPU_SWReset(pdeccfg->coreIdx, SW_RESET_SAFETY,
					    pvdec->decHandle);
				return CVI_ERR_DECODE_END;
			}
			timeoutCount++;
			interruptFlag = 0;
		}

#ifdef REDUNDENT_CODE
		CheckUserDataInterrupt(pdeccfg->coreIdx,
				       pdoi->indexFrameDecoded,
				       (CodStd)pdeccfg->bitFormat,
				       interruptFlag);
#endif

		if (interruptFlag & (1 << INT_BIT_PIC_RUN)) {
			repeat = FALSE;
			CVI_VC_TRACE("PIC_RUN\n");
		}

		if (interruptFlag & (1 << INT_BIT_DEC_FIELD)) {
			if (pdeccfg->bitstreamMode == BS_MODE_PIC_END) {
				// do not clear interrupt until feeding
				// next field picture.
				pvdec->assertedFieldDoneInt = TRUE;
				break;
			}
		}
		if (interruptFlag & (1 << INT_BIT_DEC_MB_ROWS)) {
			VPU_ClearInterrupt(pdeccfg->coreIdx);
			// VPU_DecGiveCommand(pvdec->decHandle,
			// GET_LOW_DELAY_OUTPUT, pdoi); VLOG(INFO,
			// "MB ROW interrupt is generated displayIdx=%d
			// pvdec->decodedIdx=%d picType=%d decodeSuccess=%d\n",
			////    pdoi->indexFrameDisplay,
			///pdoi->indexFrameDecoded,
			///pdoi->picType,
			///pdoi->decodingSuccess);
		}

		if (interruptFlag > 0)
			VPU_ClearInterrupt(pdeccfg->coreIdx);

		if (repeat == FALSE)
			break;

		// In PICEND mode, the below codes are not called.
		if (pdeccfg->bitstreamMode == BS_MODE_INTERRUPT &&
		    pvdec->seqChangeRequest == FALSE) {
			if (pvdec->cviApiMode == API_MODE_SDK) {
				PhysicalAddress rdPtr, wrPtr;
				Uint32 room;
				//////////
				CVI_VC_BS("SDK, BS_MODE_INTERRUPT\n");
				VPU_DecGetBitstreamBuffer(pvdec->decHandle,
							  &rdPtr, &wrPtr,
							  &room);
				CVI_VC_BS(
					"rdPtr = 0x%llX, wrPtr = 0x%llX, room = 0x%X\n",
					rdPtr, wrPtr, room);
			}
#ifdef VC_DRIVER_TEST
			else {
				pvdec->nWritten = cviBitstreamFeeder_Act(pvdec);
				if (pvdec->nWritten < 0) {
					return CVI_ERR_DECODE_END;
				}
			}
#endif
		}
	}

	if (pvdec->assertedFieldDoneInt == TRUE) {
		pvdec->needStream = TRUE;
		return CVI_DECODE_CONTINUE;
	}

	return CVI_WAIT_INT_OK;
}

int cviWaitInterrupt(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	int ret = CVI_DECODE_ONE_FRAME_OK;

	if (pdeccfg->bitFormat == STD_AVC)
		ret = _cviH264_WaitInterrupt(pvdec);
	else
		ret = _cviH265_WaitInterrupt(pvdec);

	return ret;
}
#ifdef VC_DRIVER_TEST
static void _cviH265FreeAllocatedFrmBuffer(cviVideoDecoder *pvdec,
	DecOutputInfo *pDecOutInfo, Uint32 curSeqNo, Uint32 retNum)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecOutputInfo *pdoi = &pvdec->outputInfo;
	Uint32 index, dispFlag;
	Int32 fbIndex;
	BOOL remainingFbs[MAX_REG_FRAME];

	osal_memset((void *)remainingFbs, 0x00,
			sizeof(remainingFbs));

	// Free allocated framebuffers except ones to be displayed.
	for (index = 0; index < retNum; index++) {
		fbIndex = pDecOutInfo[index]
				  .indexFrameDisplay;
		if (fbIndex >= 0) {
			VLOG(INFO,
			     "PUSH SEQ[%02d] LINEAR(%02d) COMPRESSED(%02d)\n",
			     curSeqNo,
			     pDecOutInfo[index]
				     .indexFrameDisplay,
			     pDecOutInfo[index]
				     .indexFrameDisplayForTiled);

			Queue_Enqueue(
				pvdec->displayQ,
				(void *)&pDecOutInfo
					[index]);
			if (pdeccfg->enableWTL ==
			    TRUE) {
				fbIndex = VPU_CONVERT_WTL_INDEX(
					pvdec->decHandle,
					fbIndex);
				remainingFbs[fbIndex] =
					TRUE;
			} else {
				fbIndex =
					pDecOutInfo[index]
						.indexFrameDisplayForTiled;
				remainingFbs[fbIndex] =
					TRUE;
			}
		}
	}

	/* Check Not displayed framebuffers */
	dispFlag = pdoi->frameDisplayFlag;
	for (index = 0; index < MAX_GDI_IDX; index++) {
		fbIndex = index;
		if ((dispFlag >> index) & 0x01) {
			if (pdeccfg->enableWTL ==
			    TRUE) {
				fbIndex = VPU_CONVERT_WTL_INDEX(
					pvdec->decHandle,
					fbIndex);
			}
			remainingFbs[fbIndex] = TRUE;
		}
	}

	for (index = 0; index < MAX_REG_FRAME;
	     index++) {
		if (remainingFbs[index] == FALSE) {
			// free allocated framebuffer
			if (pvdec->fbMem[index].size >
			    0) {
				VDI_FREE_MEMORY(
					pdeccfg->coreIdx,
					&pvdec->fbMem
						 [index]);
			}
		}
	}
}
#endif
int _cviH265_GetDecodedData(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecOutputInfo *pdoi = &pvdec->outputInfo;
	DecOutputInfo *pDisplayInfo = NULL;
	RetCode ret;
	Int32 repeat;

	ret = VPU_DecGetOutputInfo(pvdec->decHandle, pdoi);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"VPU_DecGetOutputInfo failed Error code is 0x%x , instIdx=%d\n",
			ret, pdeccfg->instIdx);
		if (ret == RETCODE_MEMORY_ACCESS_VIOLATION) {
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(pdeccfg->coreIdx, pvdec->productId);
#endif
#ifdef REDUNDENT_CODE
			PrintMemoryAccessViolationReason(pdeccfg->coreIdx,
							 pdoi);
#endif
			return CVI_DECODE_BREAK;
		}

		return CVI_DECODE_CONTINUE;
	}

	// To do, user data hanlding
	// -------------------------------------

	if ((pdoi->decodingSuccess & 0x01) == 0) {
		if (pdoi->indexFramePrescan == -2) {
			CVI_VC_WARN("Stream is insufficient to prescan\n");
		} else {
			CVI_VC_ERR(
				"instance(%d) VPU_DecGetOutputInfo decode fail framdIdx %d error(0x%08x) reason(0x%08x), reasonExt(0x%08x)\n",
				pvdec->decHandle->instIndex, pvdec->decodedIdx,
				pdoi->decodingSuccess, pdoi->errorReason,
				pdoi->errorReasonExt);
			if (pdoi->errorReason ==
			    WAVE4_SYSERR_WATCHDOG_TIMEOUT) {
				VPU_SWReset(pdeccfg->coreIdx, SW_RESET_SAFETY,
					    pvdec->decHandle);
				CVI_VC_ERR("VPU_SWReset (SW_RESET_SAFETY)\n");
			}
		}
	}

	pvdec->sequenceChangeFlag = pdoi->sequenceChanged;

	if (pvdec->sequenceChangeFlag) {
		BOOL dpbChanged, sizeChanged, bitDepthChanged;

		dpbChanged = (pvdec->sequenceChangeFlag &
			      SEQ_CHANGE_ENABLE_DPB_COUNT) ?
					   TRUE :
					   FALSE;
		sizeChanged =
			(pvdec->sequenceChangeFlag & SEQ_CHANGE_ENABLE_SIZE) ?
				      TRUE :
				      FALSE;
		bitDepthChanged = (pvdec->sequenceChangeFlag &
				   SEQ_CHANGE_ENABLE_BITDEPTH) ?
						TRUE :
						FALSE;

		if (dpbChanged || sizeChanged || bitDepthChanged) {
			DecOutputInfo *pDecOutInfo =
				(DecOutputInfo *)osal_malloc(
					sizeof(DecOutputInfo) * MAX_GDI_IDX);
			Uint32 retNum;
			Uint32 curSeqNo = pdoi->sequenceNo;
			Uint32 seqMemIndex = curSeqNo % MAX_SEQUENCE_MEM_COUNT;
			SequenceMemInfo *pSeqMem =
				&pvdec->seqMemInfo[seqMemIndex];
			DecGetFramebufInfo *prevSeqFbInfo =
				(DecGetFramebufInfo *)osal_malloc(
					sizeof(DecGetFramebufInfo));

			VLOG(INFO, "---> SEQUENCE CHANGED <---\n");
			if (prevSeqFbInfo == NULL) {
				CVI_VC_ERR("no memory for prevSeqFbInfo\n");
				if (pDecOutInfo != NULL) {
					osal_free(pDecOutInfo);
				}
				return CVI_ERR_ALLOC_VDEC;
			}
			memset(prevSeqFbInfo, 0x0, sizeof(DecGetFramebufInfo));
#ifdef VC_DRIVER_TEST
			if (pvdec->cviApiMode != API_MODE_SDK) {
				Uint32 framebufStride;
				Int32 compressedFbCount, linearFbCount;
				BOOL remainingFbs[MAX_REG_FRAME];
				DecInitialInfo *pSeqInfo = &pvdec->sequenceInfo;

				Queue_Enqueue(pvdec->sequenceQ,
					      (void *)&curSeqNo);
				osal_memset((void *)remainingFbs, 0x00,
					    sizeof(remainingFbs));
				// Get previous memory related to framebuffer
				VPU_DecGiveCommand(pvdec->decHandle,
						   DEC_GET_FRAMEBUF_INFO,
						   (void *)prevSeqFbInfo);
				// Get current(changed) sequence information.
				VPU_DecGiveCommand(pvdec->decHandle,
						   DEC_GET_SEQ_INFO, pSeqInfo);
				// Flush all remaining framebuffers of previous sequence.
				VPU_DecFrameBufferFlush(pvdec->decHandle,
							pDecOutInfo, &retNum);

				VLOG(INFO, "sequenceChanged : %x\n",
				     pvdec->sequenceChangeFlag);
				VLOG(INFO, "SEQUENCE NO : %d\n",
				     pdoi->sequenceNo);
				VLOG(INFO, "DPB COUNT: %d\n",
				     pSeqInfo->minFrameBufferCount);
				VLOG(INFO, "BITDEPTH : LUMA(%d), CHROMA(%d)\n",
				     pSeqInfo->lumaBitdepth,
				     pSeqInfo->chromaBitdepth);
				VLOG(INFO,
				     "SIZE	 : WIDTH(%d), HEIGHT(%d)\n",
				     pSeqInfo->picWidth, pSeqInfo->picHeight);

				_cviH265FreeAllocatedFrmBuffer(pvdec,
					pDecOutInfo, curSeqNo, retNum);

				ret = _cviH265_FreeFrmBuf(pvdec);

				osal_memset(pSeqMem, 0x00,
					    sizeof(SequenceMemInfo));
				osal_memcpy(&pSeqMem->fbInfo, prevSeqFbInfo,
					    sizeof(DecGetFramebufInfo));
				osal_memcpy(pSeqMem->allocFbMem, pvdec->fbMem,
					    sizeof(pvdec->fbMem));

				VPU_DecGiveCommand(pvdec->decHandle,
						   DEC_RESET_FRAMEBUF_INFO,
						   NULL);

				compressedFbCount =
					pSeqInfo->minFrameBufferCount +
					EXTRA_FRAME_BUFFER_NUM; /* max_dec_pic_buffering + @, @ >= 1 */
				linearFbCount =
					pSeqInfo->frameBufDelay +
					(1 +
					 EXTRA_FRAME_BUFFER_NUM *
					 2);
				/* max_num_reorder_pics + @,  @ >= 1,
				 * In most case, # of linear fbs must be greater or equal than max_num_reorder,
				 * but the expression of @ in the sample code is in order to make the situation
				 * that # of linear is greater than # of fbc.
				 */
				osal_memset((void *)pvdec->fbMem, 0x00,
					    sizeof(vpu_buffer_t) *
						    MAX_REG_FRAME);

				if (_AllocateH265DecFrameBuffer(
					    pvdec, pvdec->decHandle, pdeccfg,
					    compressedFbCount, linearFbCount,
					    pvdec->Frame, pvdec->fbMem,
					    &framebufStride) == FALSE) {
					if (pDecOutInfo != NULL) {
						osal_free(pDecOutInfo);
					}
					if (prevSeqFbInfo != NULL) {
						osal_free(prevSeqFbInfo);
					}

					CVI_VC_ERR(
						"[SEQ_CHANGE] AllocateDecFrameBuffer failed\n");
					return CVI_ERR_DECODE_END;
				}
				ret = VPU_DecRegisterFrameBufferEx(
					pvdec->decHandle, pvdec->Frame,
					compressedFbCount, linearFbCount,
					framebufStride, pSeqInfo->picHeight,
					COMPRESSED_FRAME_MAP);
				if (ret != RETCODE_SUCCESS) {
					if (pDecOutInfo != NULL) {
						osal_free(pDecOutInfo);
					}
					if (prevSeqFbInfo != NULL) {
						osal_free(prevSeqFbInfo);
					}
					CVI_VC_ERR(
						"[SEQ_CHANGE] VPU_DecRegisterFrameBufferEx failed Error code is 0x%x\n",
						ret);
					return CVI_ERR_DECODE_END;
				}

				VLOG(INFO, "----------------------------\n");

				if (pDecOutInfo != NULL) {
					osal_free(pDecOutInfo);
				}
				if (prevSeqFbInfo != NULL) {
					osal_free(prevSeqFbInfo);
				}
			} else
#endif
			{
				Queue_Enqueue(pvdec->sequenceQ,
					      (void *)&curSeqNo);
				// Get previous memory related to framebuffer
				VPU_DecGiveCommand(pvdec->decHandle,
						   DEC_GET_FRAMEBUF_INFO,
						   (void *)prevSeqFbInfo);
				osal_memcpy(&pSeqMem->fbInfo, prevSeqFbInfo,
					    sizeof(DecGetFramebufInfo));

				// Flush all remaining framebuffers of previous sequence.
				VPU_DecFrameBufferFlush(pvdec->decHandle,
							pDecOutInfo, &retNum);

				if (pDecOutInfo != NULL) {
					osal_free(pDecOutInfo);
				}
				if (prevSeqFbInfo != NULL) {
					osal_free(prevSeqFbInfo);
				}
				pvdec->needStream = TRUE;
				pvdec->waitFrmBufFree = TRUE;

				return CVI_SEQ_CHG_WAIT_BUF_FREE;
			}
		}
	}

	if (pdoi->indexFrameDisplay != DISPLAY_IDX_FLAG_SEQ_END) {
		if (pvdec->sequenceChangeFlag == 0) {
			Queue_Enqueue(pvdec->displayQ, (void *)pdoi);
		}
	}

	repeat = TRUE;
	do {
		pDisplayInfo = (DecOutputInfo *)Queue_Dequeue(pvdec->displayQ);
		if (pDisplayInfo == NULL)
			break;

		osal_memcpy((void *)pdoi, pDisplayInfo, sizeof(DecOutputInfo));
		DisplayDecodedInformation(pvdec->decHandle,
					  (CodStd)pdeccfg->bitFormat,
					  pvdec->decodedIdx, pdoi);
		if (pdoi->indexFrameDecoded < 0 &&
		    pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_NO_FB) {
			//notDecodedCount++;
			continue;
		}
		break;
	} while (repeat == TRUE);

	if (pDisplayInfo != NULL) {
		if (pdoi->indexFrameDisplay >= 0) {
#ifdef REDUNDENT_CODE
			Uint32 width = 0, height = 0, Bpp;
			size_t frameSizeInByte = 0;
#else
			Uint32 width = 0, height = 0;
#endif
			Uint8 *pYuv = NULL;
#ifdef REDUNDENT_CODE
			void *decodedData = NULL;
			Uint32 decodedDataSize = 0;
#endif
			VpuRect rcDisplay;

			rcDisplay.left = 0;
			rcDisplay.top = 0;
			rcDisplay.right = pdoi->dispPicWidth;
			rcDisplay.bottom = pdoi->dispPicHeight;

			if (pvdec->doDumpImage == TRUE) {
				if (strlen(pdeccfg->outputPath) > 0) {
				}
#ifdef REDUNDENT_CODE
				if (pdeccfg->compareType == YUV_COMPARE ||
				    pdeccfg->renderType ==
					    RENDER_DEVICE_FBDEV) {
					pYuv = GetYUVFromFrameBuffer(
						pvdec->decHandle,
						&pdoi->dispFrame, rcDisplay,
						&width, &height, &Bpp,
						&frameSizeInByte);
				}
#endif
			} else {
				width = pdoi->dispPicWidth;
				height = pdoi->dispPicHeight;
			}
#ifdef REDUNDENT_CODE
			switch (pdeccfg->compareType) {
			case NO_COMPARE:
				break;

			case YUV_COMPARE:
				decodedData = (void *)pYuv;
				decodedDataSize = frameSizeInByte;
				break;
			}

			if ((Comparator_Act(pvdec->comparator, decodedData,
					    decodedDataSize)) != TRUE) {
				CVI_VC_ERR("Comparator_Act\n");
				return CVI_ERR_DECODE_END;
			}
#endif
			/*
			* pYuv is released at the renderer module.
			* SimpleRenderer releases all framebuffer memory of the previous sequence.
			*/
			if (pvdec->seqChangeRequest != TRUE) {
				SimpleRenderer_Act(pvdec->renderer, pdoi, pYuv,
						   width, height);
				pvdec->dispIdx++;
			} else {
				VPU_DecClrDispFlag(
					pvdec->decHandle,
					pDisplayInfo->indexFrameDisplay);
			}
		}

		if (pvdec->dispIdx > 0 &&
		    pvdec->dispIdx == (Uint32)pdeccfg->forceOutNum) {
			return CVI_DECODE_BREAK;
		}

		if (pdoi->indexFrameDecoded >= 0)
			pvdec->decodedIdx++;
	}

	if (pdeccfg->bitstreamMode == BS_MODE_PIC_END) {
		pvdec->needStream = TRUE;
		if (pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB) {
			pvdec->needStream = FALSE;
		}

		if (pvdec->decHandle->codecMode == C7_HEVC_DEC) {
			//if (pdoi->indexFramePrescan == -1 || pdoi->sequenceChanged != 0) {
			if (pdoi->indexFramePrescan == -1 ||
			    pvdec->seqChangeRequest != 0) {
				pvdec->needStream = FALSE;
				VPU_DecSetRdPtr(
					pvdec->decHandle,
					pvdec->vbStream[pvdec->bsQueueIndex]
						.phys_addr,
					FALSE);
			}
		}
	}

	if (pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
		if (Queue_Peek(pvdec->displayQ) != NULL) {
			VLOG(ERR, "Queue_Peek=%d\n",
			     Queue_Peek(pvdec->displayQ));
			pvdec->success = FALSE;
		}

		return CVI_DISP_LAST_FRM;
	}

	if ((pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB) &&
	    (pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_NO_FB)) {
		return CVI_DECODE_NO_FB;
	}

	return CVI_DECODE_DATA_OK;
}

int _cviH264_GetDecodedData(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	DecOutputInfo *pdoi = &pvdec->outputInfo;
	RetCode ret;
#ifdef REDUNDENT_CODE
	BOOL result;
#endif
#ifdef VC_DRIVER_TEST
	Int32 index;
	CVI_DEC_STATUS decStatus;
	vpu_buffer_t *pvb;
#endif
	ret = VPU_DecGetOutputInfo(pvdec->decHandle, pdoi);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"VPU_DecGetOutputInfo failed Error code is 0x%x , instIdx=%d\n",
			ret, pdeccfg->instIdx);
		if (ret == RETCODE_MEMORY_ACCESS_VIOLATION) {
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(pdeccfg->coreIdx, pvdec->productId);
#endif
#ifdef REDUNDENT_CODE
			PrintMemoryAccessViolationReason(pdeccfg->coreIdx,
							 pdoi);
#endif
		}
		if (pvdec->noResponseCount == 0) {
			return CVI_DECODE_BREAK;
		}
		return CVI_DECODE_CONTINUE;
	}

	if (pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB &&
	    pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_NO_FB) {
		pvdec->noFbCount++;
	} else {
		pvdec->noFbCount = 0;
	}
	pvdec->noResponseCount = MAX_COUNT_NO_RESPONSE;
	DisplayDecodedInformation(pvdec->decHandle, (CodStd)pdeccfg->bitFormat,
				  pvdec->decodedIdx, pdoi);

	if (pdoi->sequenceChanged == TRUE) {
		pvdec->seqChangeRequest = TRUE;
		pvdec->seqChangedRdPtr = pdoi->rdPtr;
		pvdec->seqChangedWrPtr = pdoi->wrPtr;
		VLOG(INFO, "seqChangeRdPtr: 0x%08x, WrPtr: 0x%08x\n",
		     pvdec->seqChangedRdPtr, pvdec->seqChangedWrPtr);

		ret = VPU_DecSetRdPtr(pvdec->decHandle, pvdec->seqChangedRdPtr,
				      TRUE);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR("Failed to VPU_DecSetRdPtr(%d), ret(%d)\n",
				   pvdec->seqChangedRdPtr, ret);
			return CVI_DECODE_BREAK;
		}
		pvdec->seqChangedStreamEndFlag = pdoi->streamEndFlag;
		VPU_DecUpdateBitstreamBuffer(pvdec->decHandle,
					     1); // let f/w to know
		// stream end condition
		// in bitstream buffer.
		// force to know that
		// bitstream buffer
		// will be empty.
		VPU_DecUpdateBitstreamBuffer(
			pvdec->decHandle,
			STREAM_END_SET_FLAG); // set to stream end
		// condition to pump out a
		// delayed framebuffer.
		VLOG(INFO, "---> SEQUENCE CHANGED <---\n");
	}

	if (pdoi->indexFrameDecoded >= 0)
		pvdec->decodedIdx++;

	if (pvdec->enablePPU == TRUE) {
		if (pvdec->prevFbIndex >= 0) {
			VPU_DecClrDispFlag(pvdec->decHandle,
					   pvdec->prevFbIndex);
		}
		pvdec->prevFbIndex = pdoi->indexFrameDisplay;

		if (pvdec->waitPostProcessing != TRUE) {
			if (pdoi->indexFrameDisplay < 0) {
				pvdec->waitPostProcessing = TRUE;
			}
		} else {
			if (pdoi->indexFrameDisplay >= 0) {
				pvdec->waitPostProcessing = FALSE;
			}
			pvdec->rcPpu = pdoi->rcDisplay;
			/* Release framebuffer for PPU */
			Queue_Enqueue(pvdec->ppuQ, (void *)pvdec->ppuFb);
			pvdec->needStream =
				(pdeccfg->bitstreamMode == BS_MODE_PIC_END);
			if (pdoi->chunkReuseRequired == TRUE) {
				pvdec->needStream = FALSE;
			}
			// Not ready for ppu buffer.
			return CVI_DECODE_CONTINUE;
		}
	}

	if (pdoi->indexFrameDisplay >= 0 || pvdec->enablePPU == TRUE) {
		Uint32 width = 0, height = 0;
#ifdef REDUNDENT_CODE
		void *decodedData = NULL;
		Uint32 decodedDataSize = 0;
#endif
#ifndef CVI_WRITE_FRAME
		Uint8 *pYuv = NULL;
#endif
		VpuRect rc = pvdec->enablePPU == TRUE ? pvdec->rcPpu :
							      pdoi->rcDisplay;
#ifdef VC_DRIVER_TEST
		if (pvdec->cviApiMode == API_MODE_DRIVER) {
			if (pvdec->doDumpImage == TRUE) {
				size_t frameSizeInByte;
				Uint32 Bpp;
#ifdef CVI_WRITE_FRAME
				if (pvdec->saveFp != NULL) {
					ret = cviWriteFrame(pvdec);
					CVI_VC_TRACE(
						"saveFp, width = %d, height = %d\n",
						width, height);
				}
#else
				pYuv = GetYUVFromFrameBuffer(pvdec->decHandle,
							     &pdoi->dispFrame,
							     rc, &width,
							     &height, &Bpp,
							     &frameSizeInByte);
#endif
			}
		} else
#endif
		{
			width = rc.right - rc.left;
			height = rc.bottom - rc.top;
		}
#ifdef REDUNDENT_CODE
		switch (pdeccfg->compareType) {
		case NO_COMPARE:
			break;

		case YUV_COMPARE:
			decodedData = (void *)pYuv;
			decodedDataSize = frameSizeInByte;
			break;

		}

		result = Comparator_Act(pvdec->comparator, decodedData,
					decodedDataSize);
		if (result == FALSE) {
			CVI_VC_ERR("Comparator_Act\n");
			return CVI_ERR_DECODE_END;
		}
#endif

		/*
		 * pYuv is released at the renderer module.
		 * SimpleRenderer releases all framebuffer memory of the
		 * previous sequence.
		 */
		CVI_VC_DISP("width = %d, height = %d\n", width, height);
		SimpleRenderer_Act(pvdec->renderer, pdoi, pYuv, width, height);
		pvdec->dispIdx++;
	}

	if (pvdec->dispIdx > 0 &&
	    pvdec->dispIdx == (Uint32)pdeccfg->forceOutNum) {
		return CVI_DECODE_BREAK;
	}

	if (pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
		if (pvdec->seqChangeRequest == TRUE) {
			int ret;

			pvdec->seqChangeRequest = FALSE;

			if (pvdec->cviApiMode == API_MODE_SDK) {
				pvdec->needStream = TRUE;
				pvdec->waitFrmBufFree = TRUE;
				if (pvdec->seqChangedStreamEndFlag == 1) {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						STREAM_END_SET_FLAG);
				} else {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						STREAM_END_CLEAR_FLAG);
				}

				ret = CVI_SEQ_CHG_WAIT_BUF_FREE;
			}
#ifdef VC_DRIVER_TEST
			else {
				VPU_DecSetRdPtr(pvdec->decHandle,
						pvdec->seqChangedRdPtr, TRUE);

				if (pvdec->seqChangedStreamEndFlag == 1) {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						STREAM_END_SET_FLAG);
				} else {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						STREAM_END_CLEAR_FLAG);
				}

				if (pvdec->seqChangedWrPtr >=
				    pvdec->seqChangedRdPtr) {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						pvdec->seqChangedWrPtr -
							pvdec->seqChangedRdPtr);
				} else {
					VPU_DecUpdateBitstreamBuffer(
						pvdec->decHandle,
						(pvdec->vbStream[0].phys_addr +
						 pvdec->vbStream[0].size) -
							pvdec->seqChangedRdPtr +
							(pvdec->seqChangedWrPtr -
							 pvdec->vbStream[0]
								 .phys_addr));
				}

				// Flush renderer: waiting all picture displayed.
				SimpleRenderer_Flush(pvdec->renderer);

				// Release all memory related to framebuffer.
				VPU_DecGiveCommand(pvdec->decHandle,
						   DEC_FREE_FRAME_BUFFER, 0x00);
				for (index = 0; index < MAX_REG_FRAME;
				     index++) {
					pvb = &pvdec->fbMem[index];
					if (pvb->size > 0)
						VDI_FREE_MEMORY(
							pdeccfg->coreIdx, pvb);

					pvb = &pvdec->PPUFbMem[index];
					if (pvb->size > 0)
						VDI_FREE_MEMORY(
							pdeccfg->coreIdx, pvb);
				}
				pvdec->seqInited = FALSE;

				decStatus = cviInitSeq(pvdec);
				if (decStatus == CVI_ERR_DECODE_END) {
					CVI_VC_ERR(
						"cviInitSeq, decStatus = %d\n",
						decStatus);
					return CVI_ERR_DECODE_END;
				}

				ret = CVI_SEQ_CHANGE;
			}
#endif
			return ret;
		}

		return CVI_DISP_LAST_FRM;
	}

	if (pdeccfg->bitstreamMode == BS_MODE_PIC_END) {
		if (pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB) {
			pvdec->needStream = FALSE;
		} else {
			pvdec->needStream = TRUE;
		}
	}

	if (pdoi->chunkReuseRequired == TRUE) {
		pvdec->needStream = FALSE;
	}

#ifdef REDUNDENT_CODE
	SaveDecReport(pdeccfg->coreIdx, pdoi, (CodStd)pdeccfg->bitFormat);
#endif

	if (pvdec->seqChangeRequest == TRUE) {
		CVI_VC_TRACE("CVI_SEQ_CHG_FLUSH\n");
		return CVI_SEQ_CHG_FLUSH;
	}

	if ((pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB) &&
	    (pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_NO_FB)) {
		return CVI_DECODE_NO_FB;
	}

	// There is no available frame buffer for decoding the current input,
	// but a previously decoded frame is ready for display.
	if (pdoi->indexFrameDecoded == DECODED_IDX_FLAG_NO_FB &&
	    pdoi->indexFrameDisplay != DISPLAY_IDX_FLAG_SEQ_END &&
	    pdoi->indexFrameDisplay != DISPLAY_IDX_FLAG_NO_FB) {
		return CVI_DECODE_NO_FB_WITH_DISP;
	}

	return CVI_DECODE_DATA_OK;
}

int cviGetDecodedData(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	int ret = CVI_DECODE_ONE_FRAME_OK;

	if (pdeccfg->bitFormat == STD_AVC)
		ret = _cviH264_GetDecodedData(pvdec);
	else
		ret = _cviH265_GetDecodedData(pvdec);

	return ret;
}

#ifdef CVI_WRITE_FRAME
static int cviWriteFrame(cviVideoDecoder *pvdec)
{
	DecOutputInfo *pdoi = &pvdec->outputInfo;
	FrameBuffer *fb = &pdoi->dispFrame;
	VpuRect rc = pvdec->enablePPU == TRUE ? pvdec->rcPpu : pdoi->rcDisplay;
	int width, height;
	int ret;

	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	ret = cviWriteComp(pvdec, fb->bufY, width, height, fb->stride);
	if (ret < 0) {
		CVI_VC_ERR("cviWriteComp, Y\n");
		return CVI_ERR_WRITE_ERR;
	}

	ret = cviWriteComp(pvdec, fb->bufCb, width >> 1, height >> 1,
			   fb->stride >> 1);
	if (ret < 0) {
		CVI_VC_ERR("cviWriteComp, Cb\n");
		return CVI_ERR_WRITE_ERR;
	}

	ret = cviWriteComp(pvdec, fb->bufCr, width >> 1, height >> 1,
			   fb->stride >> 1);
	if (ret < 0) {
		CVI_VC_ERR("cviWriteComp, Cr\n");
		return CVI_ERR_WRITE_ERR;
	}

	return 0;
}

static int cviWriteComp(cviVideoDecoder *pvdec, PhysicalAddress buf, int width,
			int height, int stride)
{
	void *addr;
	int i;

	addr = vdi_get_vir_addr(VPU_HANDLE_CORE_INDEX(pvdec->decHandle), buf);
	if (!addr) {
		CVI_VC_ERR("vdi_get_vir_addr\n");
		return CVI_ERR_VIR_ADDR;
	}
	CVI_VC_DISP("width = %d, height = %d, stride = %d\n", width, height,
		    stride);

	for (i = 0; i < height; i++) {
		fwrite(addr, width, 1, pvdec->saveFp);
		addr += stride;
	}

	return 0;
}
#endif

void _cviH265_CloseDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	//DecOutputInfo *pdoi = &pvdec->outputInfo;
	Uint32 index;
	//vpu_buffer_t *pvb = NULL;

	// Now that we are done with decoding, close the open instance.
	VPU_DecUpdateBitstreamBuffer(pvdec->decHandle, STREAM_END_SIZE);

	/* Release all previous sequence resources */
	if (pvdec->decHandle) {
		for (index = 0; index < MAX_SEQUENCE_MEM_COUNT; index++) {
			_ReleasePreviousSequenceResources(
				pvdec->decHandle,
				pvdec->seqMemInfo[index].allocFbMem,
				&pvdec->seqMemInfo[index].fbInfo);
		}
	}

	/* Release current sequence resources */
	for (index = 0; index < MAX_REG_FRAME; index++) {
		if (pvdec->fbMem[index].size > 0)
			VDI_FREE_MEMORY(pdeccfg->coreIdx, &pvdec->fbMem[index]);
	}

#ifndef CVI_H26X_USE_ION_MEM
	for (index = 0; index < pvdec->bsBufferCount; index++) {
		VDI_FREE_MEMORY(pdeccfg->coreIdx, &pvdec->vbStream[index]);
	}
#else
	pvdec->vbStream[0].size *= pvdec->bsBufferCount;
	VDI_FREE_MEMORY(pdeccfg->coreIdx, &pvdec->vbStream[0]);
	pvdec->vbStream[0].size /= pvdec->bsBufferCount;
#endif

	VDI_FREE_MEMORY(pdeccfg->coreIdx, &pvdec->vbUserData);

	/********************************************************************************
	 * DESTROY INSTANCE *
	 ********************************************************************************/
	VPU_DecClose(pvdec->decHandle);
	if (pvdec->feeder != NULL)
		BitstreamFeeder_Destroy(pvdec->feeder);

	if (pvdec->renderer != NULL)
		SimpleRenderer_Destroy(pvdec->renderer);
#ifdef REDUNDENT_CODE
	if (pvdec->comparator != NULL) {
		Comparator_Destroy(pvdec->comparator);
		osal_free(pvdec->comparator);
	}
#endif
	if (pvdec->displayQ != NULL)
		Queue_Destroy(pvdec->displayQ);

	if (pvdec->sequenceQ != NULL)
		Queue_Destroy(pvdec->sequenceQ);

	VLOG(INFO, "\nDec End. Tot Frame %d\n", pvdec->decodedIdx);
}

void _cviH264_CloseDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
#ifdef REDUNDENT_CODE
	DecOutputInfo *pdoi = &pvdec->outputInfo;
#endif
	Int32 index;
	vpu_buffer_t *pvb = NULL;

#ifdef REDUNDENT_CODE
	CloseDecReport(pdeccfg->coreIdx);
#endif
	// Now that we are done with decoding, close the open instance.
	VPU_DecUpdateBitstreamBuffer(pvdec->decHandle, STREAM_END_SIZE);
#ifdef REDUNDENT_CODE
	if (pdeccfg->compareType &&
	    pdoi->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END &&
	    pdeccfg->forceOutNum == 0) {
		if (pvdec->success == TRUE) {
			pvdec->success =
				Comparator_CheckFrameCount(pvdec->comparator);
		}
	}
#endif
	/********************************************************************************
	 * DESTROY INSTANCE *
	 ********************************************************************************/
	VPU_DecClose(pvdec->decHandle);

	if (pvdec->feeder != NULL)
		BitstreamFeeder_Destroy(pvdec->feeder);
	if (pvdec->renderer != NULL)
		SimpleRenderer_Destroy(pvdec->renderer);
#ifdef REDUNDENT_CODE
	if (pvdec->comparator != NULL) {
		Comparator_Destroy(pvdec->comparator);
		osal_free(pvdec->comparator);
	}
#endif
	if (pvdec->ppuQ != NULL)
		Queue_Destroy(pvdec->ppuQ);

	for (index = 0; index < MAX_REG_FRAME; index++) {
		pvb = &pvdec->fbMem[index];
		if (pvb->size > 0)
			VDI_FREE_MEMORY(pdeccfg->coreIdx, pvb);
		pvb = &pvdec->PPUFbMem[index];
		if (pvb->size > 0)
			VDI_FREE_MEMORY(pdeccfg->coreIdx, pvb);
	}
	VLOG(INFO, "\nDec End. Tot Frame %d\n", pvdec->decodedIdx);
}

void cviCloseDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;

	if (pdeccfg->bitFormat == STD_AVC)
		_cviH264_CloseDecoder(pvdec);
	else
		_cviH265_CloseDecoder(pvdec);
}

void _cviH265_DeInitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;

	VPU_DeInit(pdeccfg->coreIdx);
}

void _cviH264_DeInitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;
	vpu_buffer_t *pvbs = &pvdec->vbStream[0];

	if (pvbs->size > 0)
		VDI_FREE_MEMORY(pdeccfg->coreIdx, pvbs);

	VPU_DeInit(pdeccfg->coreIdx);
}

void cviDeInitDecoder(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;

	if (pdeccfg->bitFormat == STD_AVC)
		_cviH264_DeInitDecoder(pvdec);
	else
		_cviH265_DeInitDecoder(pvdec);
}

void initDefaultcDecConfig(cviVideoDecoder *pvdec)
{
	cviDecConfig *pdeccfg = &pvdec->decConfig;

	pdeccfg->enableWTL = TRUE;
	if (pvdec->cviApiMode == API_MODE_DRIVER) {
		pdeccfg->bitstreamMode = BS_MODE_INTERRUPT;
		pdeccfg->renderType = RENDER_DEVICE_NULL;
	} else {
		pdeccfg->bitstreamMode = BS_MODE_PIC_END;
		pdeccfg->renderType = RENDER_DEVICE_SDK;
	}

	if (pdeccfg->bitFormat == STD_HEVC) {
		pdeccfg->feedingMode = FEEDING_METHOD_FIXED_SIZE;
		pdeccfg->streamEndian = VPU_STREAM_ENDIAN;
		pdeccfg->frameEndian = VPU_FRAME_ENDIAN;
		pdeccfg->cbcrInterleave = FALSE;
		pdeccfg->nv21 = FALSE;
		pdeccfg->bitFormat = STD_HEVC;
		pdeccfg->mapType = COMPRESSED_FRAME_MAP;
		pdeccfg->enableWTL = TRUE;
		pdeccfg->wtlMode = FF_FRAME;
		pdeccfg->wtlFormat = FORMAT_420;
		pdeccfg->wave4.fbcMode = 0x0c;
		if (pvdec->cviApiMode == API_MODE_DRIVER)
			pdeccfg->bsSize = STREAM_BUF_SIZE_HEVC;
		pdeccfg->wave4.numVCores = 1;
		pdeccfg->wave4.bwOptimization = FALSE;
		pdeccfg->secondaryAXI = SECONDARY_AXI_H265;
	} else {
		pdeccfg->streamEndian = VDI_LITTLE_ENDIAN;
		pdeccfg->frameEndian = VDI_LITTLE_ENDIAN;
		pdeccfg->cbcrInterleave = FALSE;
		pdeccfg->nv21 = FALSE;
		pdeccfg->bitFormat = STD_AVC;
		pdeccfg->mapType = LINEAR_FRAME_MAP;
		pdeccfg->coda9.enableBWB = VPU_ENABLE_BWB;
		pdeccfg->coda9.frameCacheBypass = 0;
		pdeccfg->coda9.frameCacheBurst = 0;
		pdeccfg->coda9.frameCacheMerge = 3;
		pdeccfg->coda9.frameCacheWayShape = 15;
		pdeccfg->tid = AVC_MAX_SUB_LAYER_ID;
		pdeccfg->secondaryAXI = SECONDARY_AXI_H264;
		pdeccfg->reorderEnable = pvdec->ReorderEnable;

		strcpy(pdeccfg->outputPath, "decoded.yuv");
	}
}

#ifdef VC_DRIVER_TEST
static void Help(const char *programName)
{
	CVI_PRNT(
		"-------------------------------------------------------------------\n");
	CVI_PRNT("%s(API v%d.%d.%d)\n", programName, API_VERSION_MAJOR,
		 API_VERSION_MINOR, API_VERSION_PATCH);
	CVI_PRNT("\tAll rights reserved by Chips&Media(C)\n");
	CVI_PRNT(
		"-------------------------------------------------------------------\n");
	CVI_PRNT("%s [option] --input bistream\n", programName);
	CVI_PRNT("-h                          help\n");
	CVI_PRNT("-n [num]                    output frame number\n");
	CVI_PRNT("-v                          print version information\n");
	CVI_PRNT("-c                          compare with golden\n");
	CVI_PRNT("                            0 : no comparison\n");
	CVI_PRNT(
		"                            1 : compare with golden specified --ref-yuv option\n");
	CVI_PRNT("-u                          Enable userdata\n");
	CVI_PRNT("--input                     bitstream path\n");
	CVI_PRNT("--output                    YUV path\n");
	CVI_PRNT("--codec                     The index of codec (H.264:0)\n");
	CVI_PRNT(
		"--bsmode                    0: INTERRUPT MODE, 1: reserved, 2: PICEND MODE\n");
	CVI_PRNT("--coreIdx                   core index: default 0\n");
	CVI_PRNT(
		"--loop-count                integer number. loop test, default 0\n");
	CVI_PRNT("--stream-endian             0~3, default 0(LE)\n");
	CVI_PRNT("--frame-endian              0~3, default 0(LE)\n");
	CVI_PRNT(
		"--enable-cbcrinterleave     enable cbcrInterleave(NV12), default off\n");
	CVI_PRNT("--enable-nv21               enable NV21, default off\n");
	CVI_PRNT("--secondary-axi             0~63: bit oring valuest\n");
	CVI_PRNT("--rotate                    90, 180, 270\n");
	CVI_PRNT(
		"--mirror                    0: none, 1: vertical, 2: horizontal, 3: both\n");
	CVI_PRNT("--enable-dering             MPEG-2, MPEG-4 only\n");
	CVI_PRNT("--enable-deblock            MPEG-2, MPEG-4 only\n");
	CVI_PRNT(
		"--maptype                   CODA960: 0~6, CODA980: 0~9, default 0\n");
	CVI_PRNT(
		"                            Please refer TiledMapType in vpuapi.h\n");
	CVI_PRNT(
		"--enable-tiled2linear       enable tiled2linear. default off\n");
	CVI_PRNT("--enable-wtl                enable WTL. default off\n");
	CVI_PRNT("--enable-mvc                enable H.264 MVC, default off\n");
	CVI_PRNT(
		"--low-delay   mbrows        enable low delay option with mbrows. H.264\n");
	CVI_PRNT("--render                    0 : no rendering picture\n");
	CVI_PRNT(
		"                            1 : render a picture with the framebuffer device\n");
	CVI_PRNT("--ref-yuv                   golden yuv path\n");
	CVI_PRNT("--tid N                     target temporal id (0..2)\n");
}

//extern char *optarg; /* argument associated with option */
static struct OptionExt options_help[] = {
	{ "output", 1, NULL, 0, "--output" },
	{ "input", 1, NULL, 0, "--input" },
	{ "codec", 1, NULL, 0, "--codec" },
	{ "render", 1, NULL, 0, "--render" },
	{ "maptype", 1, NULL, 0, "--maptype" },
	{ "disable-wtl", 0, NULL, 0, "--disable-wtl" },
	{ "coreIdx", 1, NULL, 0, "--coreIdx" },
	{ "loop-count", 1, NULL, 0, "--loop-count" },
	{ "enable-cbcrinterleave", 0, NULL, 0,
		"--enable-cbcrinterleave" },
	{ "stream-endian", 1, NULL, 0, "--stream-endian" },
	{ "frame-endian", 1, NULL, 0, "--frame-endian" },
	{ "enable-nv21", 0, NULL, 0, "--enable-nv21" },
	{ "secondary-axi", 1, NULL, 0, "--secondary-axi" },
	{ "bsmode", 1, NULL, 0, "--bsmode" },
	{ "enable-tiled2linear", 0, NULL, 0, "--enable-tiled2linear" },
	{ "rotate", 1, NULL, 0, "--rotate" },
	{ "mirror", 1, NULL, 0, "--mirror" },
	{ "enable-dering", 0, NULL, 0, "--enable-dering" },
	{ "enable-deblock", 0, NULL, 0, "--enable-deblock" },
	{ "mp4class", 1, NULL, 0, "--mp4class" },
	{ "ref-yuv", 1, NULL, 0, "--ref-yuv" },
	{ "enable-mvc", 0, NULL, 0, "--enable-mvc" },
	{ "low-delay", 1, NULL, 0, "--low-delay" },
	{ "tid", 1, NULL, 0, "--tid" },
	{ NULL, 0, NULL, 0, NULL },
};

static struct option options[MAX_GETOPT_OPTIONS];
int parseVdecArgs(int argc, char **argv, cviDecConfig *pdeccfg)
{
	char *optString = "c:hvn:u";
	Int32 ret = 0;
	Int32 i, index, val;
	Int32 opt;

	for (i = 0; i < MAX_GETOPT_OPTIONS; i++) {
		if (options_help[i].name == NULL)
			break;
		osal_memcpy(&options[i], &options_help[i],
			    sizeof(struct option));
	}

	while ((opt = getopt_long(argc, argv, optString, options, &index)) !=
	       -1) {
		switch (opt) {
		case 'c':
			pdeccfg->compareType = atoi(optarg);
			if (pdeccfg->compareType < NO_COMPARE ||
			    pdeccfg->compareType > YUV_COMPARE) {
				CVI_VC_ERR("Invalid compare type(%d)\n",
					   pdeccfg->compareType);
				Help(argv[0]);
				return -1;
			}
			break;
		case 'n':
			pdeccfg->forceOutNum = atoi(optarg);
			break;
		case 'h':
			Help(argv[0]);
			return 0;
		case 'u':
			pdeccfg->enableUserData = TRUE;
			break;
		case 0:
			if (strcmp(options[index].name, "output") == 0) {
				memcpy(pdeccfg->outputPath, optarg,
				       strlen(optarg));
#ifdef REDUNDENT_CODE
				ChangePathStyle(pdeccfg->outputPath);
#endif
			} else if (strcmp(options[index].name, "input") == 0) {
				memcpy(pdeccfg->inputPath, optarg,
				       strlen(optarg));
#ifdef REDUNDENT_CODE
				ChangePathStyle(pdeccfg->inputPath);
#endif
			} else if (strcmp(options[index].name, "codec") == 0) {
				pdeccfg->bitFormat = atoi(optarg);
			} else if (strcmp(options[index].name, "render") == 0) {
				pdeccfg->renderType =
					(RenderDeviceType)atoi(optarg);
				if (pdeccfg->renderType < RENDER_DEVICE_NULL ||
				    pdeccfg->renderType >= RENDER_DEVICE_MAX) {
					CVI_VC_ERR(
						"unknown render device type(%d)\n",
						pdeccfg->renderType);
					Help(argv[0]);
					return -1;
				}
			} else if (strcmp(options[index].name, "maptype") ==
				   0) {
				pdeccfg->mapType = (TiledMapType)atoi(optarg);
			} else if (strcmp(options[index].name, "disable-wtl") ==
				   0) {
				pdeccfg->enableWTL = FALSE;
			} else if (strcmp(options[index].name, "coreIdx") ==
				   0) {
				pdeccfg->coreIdx = atoi(optarg);
			} else if (strcmp(options[index].name, "loop-count") ==
				   0) {
				pdeccfg->loopCount = atoi(optarg);
			} else if (strcmp(options[index].name,
					  "enable-cbcrinterleave") == 0) {
				pdeccfg->cbcrInterleave = TRUE;
			} else if (strcmp(options[index].name,
					  "stream-endian") == 0) {
				pdeccfg->streamEndian =
					(EndianMode)atoi(optarg);
			} else if (strcmp(options[index].name,
					  "frame-endian") == 0) {
				pdeccfg->frameEndian = (EndianMode)atoi(optarg);
			} else if (strcmp(options[index].name, "enable-nv21") ==
				   0) {
				pdeccfg->nv21 = TRUE;
				pdeccfg->cbcrInterleave = TRUE;
			} else if (strcmp(options[index].name, "secondary-axi") == 0) {
				long long secondaryAXI = 0;

				if (kstrtoll(optarg, !strncmp("0x", optarg, 2) ? 16 : 10,
					&secondaryAXI) != 0) {
					pr_err("secondaryAXI input error\n");
				}
				pdeccfg->secondaryAXI = secondaryAXI;
			} else if (strcmp(options[index].name, "bsmode") == 0) {
				pdeccfg->bitstreamMode = atoi(optarg);
			} else if (strcmp(options[index].name,
					  "enable-tiled2linear") == 0) {
				pdeccfg->coda9.enableTiled2Linear = TRUE;
				pdeccfg->coda9.tiled2LinearMode = FF_FRAME;
				pdeccfg->enableWTL = FALSE;
			} else if (strcmp(options[index].name, "rotate") == 0) {
				val = atoi(optarg);
				if ((val % 90) != 0) {
					CVI_VC_ERR(
						"Invalid rotation value: %d\n",
						val);
					Help(argv[0]);
					return -1;
				}
				pdeccfg->coda9.rotate = val;
			} else if (strcmp(options[index].name, "mirror") == 0) {
				val = atoi(optarg);
				if (val < 0 || val > 3) {
					CVI_VC_ERR(
						"Invalid mirror option: %d\n",
						val);
					Help(argv[0]);
					return -1;
				}
				pdeccfg->coda9.mirror = val;
			} else if (strcmp(options[index].name,
					  "enable-dering") == 0) {
				pdeccfg->coda9.enableDering = TRUE;
			} else if (strcmp(options[index].name,
					  "enable-deblock") == 0) {
				pdeccfg->coda9.enableDeblock = TRUE;
			} else if (strcmp(options[index].name, "mp4class") ==
				   0) {
				pdeccfg->coda9.mp4class = atoi(optarg);
			} else if (strcmp(options[index].name, "ref-yuv") ==
				   0) {
				memcpy(pdeccfg->refYuvPath, optarg,
				       strlen(optarg));
#ifdef REDUNDENT_CODE
				ChangePathStyle(pdeccfg->refYuvPath);
#endif
			} else if (strcmp(options[index].name, "enable-mvc") ==
				   0) {
				pdeccfg->coda9.enableMvc = TRUE;
			} else if (strcmp(options[index].name, "low-delay") ==
				   0) {
				pdeccfg->coda9.lowDelay.lowDelayEn = TRUE;
				pdeccfg->coda9.lowDelay.numRows = atoi(optarg);
			} else if (strcmp(options[index].name, "tid") == 0) {
				pdeccfg->tid = (Uint32)atoi(optarg);
			}
			break;
		case '?':
		default:
			CVI_VC_ERR("%s\n", optarg);
			Help(argv[0]);
			return -1;
		}
	}

	if (strlen(pdeccfg->inputPath) == 0) {
		CVI_VC_ERR("No input bitstream\n");
		Help(argv[0]);
		return -1;
	}

	return ret;
}
#endif

void checkDecConfig(cviDecConfig *pdeccfg)
{
	/* Check combination of parameters of decoder */
	CVI_VC_WARN("------ WRONG PARAMETER COMBINATION ------n");
	if (pdeccfg->mapType != LINEAR_FRAME_MAP) {
		if (pdeccfg->enableWTL == TRUE) {
			pdeccfg->enableWTL = TRUE;
			pdeccfg->wtlMode = FF_FRAME;
		}
	}

	switch (pdeccfg->mapType) {
	case LINEAR_FRAME_MAP:
	case LINEAR_FIELD_MAP:
		if (pdeccfg->coda9.enableTiled2Linear == TRUE ||
		    pdeccfg->enableWTL == TRUE) {
			CVI_VC_WARN(
				"can't enable Tiled2Linear OR WTL where map is LINEAR_FRAME\n");
			CVI_VC_WARN("Disable WTL or Tiled2Linear\n");
			pdeccfg->coda9.enableTiled2Linear = FALSE;
			pdeccfg->coda9.tiled2LinearMode = FF_NONE;
			pdeccfg->enableWTL = FALSE;
			pdeccfg->wtlMode = FF_NONE;
		}
		break;
	case TILED_FRAME_MB_RASTER_MAP:
	case TILED_FIELD_MB_RASTER_MAP:
		if (pdeccfg->cbcrInterleave == FALSE) {
			CVI_VC_WARN(
				"CBCR-interleave must be enable when maptype is TILED_FRAME/FIELD_MB_RASTER_MAP.\n");
			CVI_VC_WARN("Enable cbcr-interleave\n");
			pdeccfg->cbcrInterleave = TRUE;
		}
		break;
	default:
		break;
	}

	if (pdeccfg->coda9.enableTiled2Linear == TRUE) {
		CVI_VC_WARN(
			"In case of Tiledmap, disabled BWB for better performance.\n");
		pdeccfg->coda9.enableBWB = FALSE;
	}

	if (pdeccfg->coda9.lowDelay.lowDelayEn == TRUE &&
	    pdeccfg->bitFormat != STD_AVC) {
		CVI_VC_WARN(
			"The low-delay decoding option is valid when a codec is H.264\n");
		pdeccfg->coda9.lowDelay.lowDelayEn = FALSE;
		pdeccfg->coda9.lowDelay.numRows = 0;
	}
	CVI_VC_WARN("--------------------------------\n");
	/* END OF CHECK COMBINATION */
}

int cviAttachFrmBuf(cviVideoDecoder *pvdec, cviBufInfo *pFrmBufArray,
		    int nFrmNum)
{
	int ret = 0;
	int i = 0;

	CVI_VC_TRACE(" nFrmNum=%d\n", nFrmNum);

	for (i = 0; i < nFrmNum; i++) {
		pvdec->Frame[i].bufY = pFrmBufArray[i].phyAddr;
		pvdec->Frame[i].bufCb = -1;
		pvdec->Frame[i].bufCr = -1;
		pvdec->Frame[i].updateFbInfo = TRUE;
		pvdec->Frame[i].size = pFrmBufArray[i].size;

		pvdec->vbFrame[i].phys_addr = pFrmBufArray[i].phyAddr;
		pvdec->vbFrame[i].virt_addr = pFrmBufArray[i].virtAddr;
		pvdec->vbFrame[i].size = pFrmBufArray[i].size;

		CVI_VC_TRACE("vbAddr=0x%llx, vbsize=0x%x, vbVirt=%p\n",
			     (Uint64)pvdec->vbFrame[i].phys_addr,
			     pvdec->vbFrame[i].size,
			     (void *)pvdec->vbFrame[i].virt_addr);
	}

	pvdec->nVbFrameNum = nFrmNum;

	return ret;
}

void cviDecAttachCallBack(CVI_VDEC_CALLBACK pCbFunc)
{
	if (pCbFunc == NULL) {
		CVI_VC_ERR("pCbFunc == NULL\n");
		return;
	}

	CVI_VC_TRACE("\n");

	if (pVdecDrvCbFunc == NULL) {
		pVdecDrvCbFunc = pCbFunc;
	}
}
#endif
