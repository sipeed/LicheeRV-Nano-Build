//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2014  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include <linux/string.h>
#include "vpuapifunc.h"
#include "coda9/coda9_regdefine.h"
#include "wave/common/common_vpuconfig.h"
#include "wave/common/common_regdefine.h"
#include "wave/wave4/wave4_regdefine.h"
#include "vpuerror.h"
#include "main_helper.h"
#ifdef PLATFORM_LINUX
#include <getopt.h>
#endif
#include "fw_h264.h"
#include "fw_h265.h"

#define BIT_DUMMY_READ_GEN 0x06000000
#define BIT_READ_LATENCY 0x06000004
#define W4_SET_READ_DELAY 0x01000000
#define W4_SET_WRITE_DELAY 0x01000004
#define MAX_CODE_BUF_SIZE (512 * 1024)

#ifdef PLATFORM_WIN32
#pragma warning(disable : 4996)
	//!<< disable waring C4996: The POSIX name for
	//!<this item is deprecated.
#endif

char *EncPicTypeStringH264[] = {
	"IDR/I",
	"P",
};

char *EncPicTypeStringMPEG4[] = {
	"I",
	"P",
};

char *productNameList[] = {
	"CODA980", "CODA960", "CODA7503", "WAVE320",  "WAVE410", "WAVE4102",
	"WAVE420", "WAVE412", "WAVE7Q",	  "WAVE420L", "WAVE510", "WAVE512",
	"WAVE515", "WAVE520", "Unknown",  "Unknown",
};

#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef FIRMWARE_H
Int32 LoadFirmwareH(Int32 productId, Uint8 **retFirmware, Uint32 *retSizeInWord)
{
	Uint32 readSize;

	if (productId == PRODUCT_ID_420L) {
		readSize = sizeof(fw_h265);
		*retFirmware = fw_h265;
	} else if (productId == PRODUCT_ID_980) {
		readSize = sizeof(fw_h264);
		*retFirmware = fw_h264;
	} else {
		CVI_VC_ERR("productId = %d\n", productId);
		return -1;
	}

	readSize = ((readSize + 1) >> 1) << 1;
	*retSizeInWord = readSize >> 1;

	CVI_VC_TRACE("productId = %d, retFirmware %p, retSizeInWord %u\n",
		     productId, *retFirmware, *retSizeInWord);

	return 0;
}
#else
Int32 LoadFirmware(Int32 productId, Uint8 **retFirmware, Uint32 *retSizeInWord,
		   const char *path)
{
	Uint8 *firmware = NULL;
#ifdef PLATFORM_NON_OS
#ifdef WAVE420
	Uint32 readSize = 147456;
#else
	Uint32 readSize = 256000;
#endif
#if CFG_MEM
	Uint8 *fwAddr = (Uint8 *)dramCfg.pucCodeAddr;
#else
	Uint8 *fwAddr = (Uint8 *)WAVE420L_CODE_ADDR;
#endif
#else
	Uint32 readSize = 1024 * 1024;
	osal_file_t fp;
#endif

#ifndef PLATFORM_NON_OS
	CVI_VC_TRACE("path = %s\n", path);
	fp = osal_fopen(path, "rb");
	if (fp == NULL) {
		VLOG(ERR, "Failed to open %s\n", path);
		return -1;
	}
	osal_fseek(fp, 0, 2);//seek file end
	readSize = osal_ftell(fp);
	osal_fseek(fp, 0, 0);//seek file start
#endif

#ifdef PLATFORM_NON_OS
	CVI_VC_TRACE("frimware addr : 0x%lX\n", fwAddr);
#endif

	CVI_VC_TRACE("productId = %d\n", productId);

	firmware = (Uint8 *)osal_malloc(readSize);
	if (firmware == NULL) {
		CVI_VC_ERR("allocation fail, firmware !\n");
	}

#ifdef PLATFORM_NON_OS
	memcpy(firmware, fwAddr, readSize);
#else
	osal_fread(firmware, 1, readSize, fp);
#endif

	*retSizeInWord = (readSize + 1) >> 1;

#ifndef PLATFORM_NON_OS
	osal_fclose(fp);
#endif

	*retFirmware = firmware;

	return 0;
}
#endif

void PrintVpuVersionInfo(Uint32 core_idx)
{
	Uint32 version;
	Uint32 revision;
	Uint32 productId;

	VPU_GetVersionInfo(core_idx, &version, &revision, &productId);

	VLOG(INFO, "VPU coreNum : [%d]\n", core_idx);
	VLOG(INFO,
	     "Firmware : CustomerCode: %04x | version : %d.%d.%d rev.%d\n",
	     (Uint32)(version >> 16), (Uint32)((version >> (12)) & 0x0f),
	     (Uint32)((version >> (8)) & 0x0f), (Uint32)((version)&0xff),
	     revision);
	VLOG(INFO, "Hardware : %04x\n", productId);
	VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR,
	     API_VERSION_MINOR, API_VERSION_PATCH);
}

#ifdef REDUNDENT_CODE
void FreePreviousFramebuffer(Uint32 coreIdx, DecGetFramebufInfo *fb)
{
	int i;
	if (fb->vbFrame.size > 0) {
		VDI_FREE_MEMORY(coreIdx, &fb->vbFrame);
		osal_memset((void *)&fb->vbFrame, 0x00, sizeof(vpu_buffer_t));
	}
	if (fb->vbWTL.size > 0) {
		VDI_FREE_MEMORY(coreIdx, &fb->vbWTL);
		osal_memset((void *)&fb->vbWTL, 0x00, sizeof(vpu_buffer_t));
	}
	for (i = 0; i < MAX_REG_FRAME; i++) {
		if (fb->vbFbcYTbl[i].size > 0) {
			VDI_FREE_MEMORY(coreIdx, &fb->vbFbcYTbl[i]);
			osal_memset((void *)&fb->vbFbcYTbl, 0x00,
				    sizeof(vpu_buffer_t));
		}
		if (fb->vbFbcCTbl[i].size > 0) {
			VDI_FREE_MEMORY(coreIdx, &fb->vbFbcCTbl[i]);
			osal_memset((void *)&fb->vbFbcCTbl, 0x00,
				    sizeof(vpu_buffer_t));
		}
	}
}
#endif

void PrintDecSeqWarningMessages(Uint32 productId, DecInitialInfo *seqInfo)
{
	if (PRODUCT_ID_W_SERIES(productId)) {
		if (seqInfo->seqInitErrReason & 0x00000001)
			VLOG(WARN,
			     "sps_max_sub_layer_minus1 shall be 0 to 6\n");
		if (seqInfo->seqInitErrReason & 0x00000002)
			VLOG(WARN,
			     "general_reserved_zero_44bits shall be 0.\n");
		if (seqInfo->seqInitErrReason & 0x00000004)
			VLOG(WARN, "reserved_zero_2bits shall be 0\n");
		if (seqInfo->seqInitErrReason & 0x00000008)
			VLOG(WARN, "sub_layer_reserved_zero_44bits shall be 0");
		if (seqInfo->seqInitErrReason & 0x00000010)
			VLOG(WARN,
			     "general_level_idc shall have one of level of Table A.1\n");
		if (seqInfo->seqInitErrReason & 0x00000020)
			VLOG(WARN,
			     "sps_max_dec_pic_buffering[i] <= MaxDpbSize\n");
		if (seqInfo->seqInitErrReason & 0x00000040)
			VLOG(WARN,
			     "trailing bits shall be 1000... pattern, 7.3.2.1\n");
		if (seqInfo->seqInitErrReason & 0x00100000)
			VLOG(WARN, "Not supported or undefined profile: %d\n",
			     seqInfo->profile);
		if (seqInfo->seqInitErrReason & 0x00200000)
			VLOG(WARN, "Spec over level(%d)\n", seqInfo->level);
	}
}

void DisplayDecodedInformationForHevc(DecHandle handle, Uint32 frameNo,
				      DecOutputInfo *decodedInfo)
{
	Int32 logLevel;

	if (decodedInfo == NULL) {
#ifdef REDUNDENT_CODE
		if (handle->productId == PRODUCT_ID_510 ||
		    handle->productId == PRODUCT_ID_512) {
			VLOG(TRACE,
			     "I    NO  T     POC     NAL  DECO  DISP PRESCAN DISPFLAG  RD_PTR   WR_PTR FRM_START FRM_END   WxH      SEQ  TEMP CYCLE (Seek, Parse, Dec)\n");
		} else
#endif
		{
			VLOG(TRACE,
			     "I    NO  T     POC     NAL  DECO  DISP PRESCAN DISPFLAG  RD_PTR   WR_PTR FRM_START FRM_END   WxH      SEQ  TEMP CYCLE\n");
		}
		VLOG(TRACE,
		     "------------------------------------------------------------------------------------------------------------\n");
	} else {
		logLevel = (decodedInfo->decodingSuccess & 0x01) == 0 ? ERR :
									      TRACE;
		if (handle->productId == PRODUCT_ID_4102) {
			logLevel = (decodedInfo->indexFramePrescan == -2) ?
						 TRACE :
						 logLevel;
		}
		// Print informations
#ifdef REDUNDENT_CODE
		if (handle->productId == PRODUCT_ID_510 ||
		    handle->productId == PRODUCT_ID_512) {
			VLOG(logLevel,
			     "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %7d %08x %08x %08x %08x %08x %4dx%-4d %4d %4d %d(%d,%d,%d)\n",
			     handle->instIndex, frameNo, decodedInfo->picType,
			     decodedInfo->h265Info.decodedPOC,
			     decodedInfo->h265Info.displayPOC,
			     decodedInfo->nalType,
			     decodedInfo->indexFrameDecoded,
			     decodedInfo->indexFrameDecodedForTiled,
			     decodedInfo->indexFrameDisplay,
			     decodedInfo->indexFrameDisplayForTiled,
			     decodedInfo->indexFramePrescan,
			     decodedInfo->frameDisplayFlag, decodedInfo->rdPtr,
			     decodedInfo->wrPtr, decodedInfo->bytePosFrameStart,
			     decodedInfo->bytePosFrameEnd,
			     decodedInfo->dispPicWidth,
			     decodedInfo->dispPicHeight,
			     decodedInfo->sequenceNo,
			     decodedInfo->h265Info.temporalId,
			     decodedInfo->frameCycle, decodedInfo->seekCycle,
			     decodedInfo->parseCycle, decodedInfo->decodeCycle);
		} else
#endif
		{
			VLOG(logLevel,
			     "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %7d %08x %08x %08x %08x %08x %4dx%-4d %4d %4d %d\n",
			     handle->instIndex, frameNo, decodedInfo->picType,
			     decodedInfo->h265Info.decodedPOC,
			     decodedInfo->h265Info.displayPOC,
			     decodedInfo->nalType,
			     decodedInfo->indexFrameDecoded,
			     decodedInfo->indexFrameDecodedForTiled,
			     decodedInfo->indexFrameDisplay,
			     decodedInfo->indexFrameDisplayForTiled,
			     decodedInfo->indexFramePrescan,
			     decodedInfo->frameDisplayFlag, decodedInfo->rdPtr,
			     decodedInfo->wrPtr, decodedInfo->bytePosFrameStart,
			     decodedInfo->bytePosFrameEnd,
			     decodedInfo->dispPicWidth,
			     decodedInfo->dispPicHeight,
			     decodedInfo->sequenceNo,
			     decodedInfo->h265Info.temporalId,
			     decodedInfo->frameCycle);
		}
		if (logLevel == ERR) {
			VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n",
			     decodedInfo->errorReason,
			     decodedInfo->errorReasonExt);
		}
		if (decodedInfo->numOfErrMBs) {
			VLOG(WARN, "\t>> ErrorBlock: %d\n",
			     decodedInfo->numOfErrMBs);
		}
	}
}

#ifdef REDUNDENT_CODE
void DisplayDecodedInformationForVP9(DecHandle handle, Uint32 frameNo,
				     DecOutputInfo *decodedInfo)
{
	Int32 logLevel;

	if (decodedInfo == NULL) {
		// Print header
		VLOG(TRACE,
		     "I  NO    T   DECO   DISP PRESCAN DISPFLAG   RD_PTR   WR_PTR FRM_START FRM_END    WxH SEQ  CYCLE\n");
		VLOG(TRACE,
		     "--------------------------------------------------------------------------------------------\n");
	} else {
		logLevel = (decodedInfo->decodingSuccess & 0x01) == 0 ? ERR :
									      TRACE;
		// Print informations
		VLOG(logLevel,
		     "%02d %5d %d %2d(%2d) %2d(%2d) %7d %08x %08x %08x %08x %08x %4dx%-4d %4d %d\n",
		     handle->instIndex, frameNo, decodedInfo->picType,
		     decodedInfo->indexFrameDecoded,
		     decodedInfo->indexFrameDecodedForTiled,
		     decodedInfo->indexFrameDisplay,
		     decodedInfo->indexFrameDisplayForTiled,
		     decodedInfo->indexFramePrescan,
		     decodedInfo->frameDisplayFlag, decodedInfo->rdPtr,
		     decodedInfo->wrPtr, decodedInfo->bytePosFrameStart,
		     decodedInfo->bytePosFrameEnd, decodedInfo->dispPicWidth,
		     decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
		     decodedInfo->frameCycle);
		if (logLevel == ERR) {
			VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n",
			     decodedInfo->errorReason,
			     decodedInfo->errorReasonExt);
		}
		if (decodedInfo->numOfErrMBs) {
			VLOG(WARN, "\t>> ErrorBlock: %d\n",
			     decodedInfo->numOfErrMBs);
		}
	}
}
#endif

void DisplayDecodedInformationCommon(DecHandle handle, CodStd codec,
				     Uint32 frameNo, DecOutputInfo *decodedInfo)
{
	Int32 logLevel;

	if (decodedInfo == NULL) {
		// Print header
		VLOG(TRACE,
		     "I  NO    T TID DEC_POC   DECO   DISP  DISPFLAG RD_PTR   WR_PTR   FRM_START FRM_END WxH\n");
		VLOG(TRACE,
		     "---------------------------------------------------------------------------------------\n");
	} else {
		VpuRect rc = decodedInfo->rcDisplay;
		Uint32 width = rc.right - rc.left;
		Uint32 height = rc.bottom - rc.top;
		char strTemporalId[16];
		char strPoc[16];

#ifdef SUPPORT_980_ROI_RC_LIB
		if (STD_AVC == codec) {
			sprintf(strTemporalId, "%d",
				decodedInfo->avcTemporalId);
			sprintf(strPoc, "%8d", decodedInfo->avcPocPic);
		} else {
			strcpy(strTemporalId, "-");
			strcpy(strPoc, "--------");
		}
#endif

		logLevel = (decodedInfo->decodingSuccess & 0x01) == 0 ? ERR :
									      TRACE;
		// Print informations
		VLOG(logLevel,
		     "%02d %5d %d  %s  %s %2d(%2d) %2d(%2d) %08x %08x %08x %08x  %08x %dx%d\n",
		     handle->instIndex, frameNo, decodedInfo->picType,
		     strTemporalId, strPoc, decodedInfo->indexFrameDecoded,
		     decodedInfo->indexFrameDecodedForTiled,
		     decodedInfo->indexFrameDisplay,
		     decodedInfo->indexFrameDisplayForTiled,
		     decodedInfo->frameDisplayFlag, decodedInfo->rdPtr,
		     decodedInfo->wrPtr, (Uint32)decodedInfo->bytePosFrameStart,
		     (Uint32)decodedInfo->bytePosFrameEnd, width, height);
		if (logLevel == ERR) {
			VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n",
			     decodedInfo->errorReason,
			     decodedInfo->errorReasonExt);
		}
		if (decodedInfo->numOfErrMBs) {
			VLOG(WARN, "\t>> ErrorBlock: %d\n",
			     decodedInfo->numOfErrMBs);
		}
	}
}

/**
 * \brief                   Print out decoded information such like RD_PTR,
 * WR_PTR, PIC_TYPE, .. \param   decodedInfo     If this parameter is not NULL
 * then print out decoded informations otherwise print out header.
 */
void DisplayDecodedInformation(DecHandle handle, CodStd codec, Uint32 frameNo,
			       DecOutputInfo *decodedInfo)
{
	switch (codec) {
	case STD_HEVC:
		DisplayDecodedInformationForHevc(handle, frameNo, decodedInfo);
		break;
	default:
		DisplayDecodedInformationCommon(handle, codec, frameNo,
						decodedInfo);
		break;
	}
}

static void Wave4DisplayEncodedInformation(EncHandle handle,
					   EncOutputInfo *encodedInfo,
					   Int32 srcEndFlag, Int32 srcFrameIdx)
{
	if (encodedInfo == NULL) {
#ifdef REPORT_PIC_SUM_VAR
		VLOG(INFO,
		     "--------------------------------------------------------------------------------------------\n");
		VLOG(INFO,
		     "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX Cycle    SumVariance\n");
		VLOG(INFO,
		     "--------------------------------------------------------------------------------------------\n");
#else
		VLOG(INFO,
		     "------------------------------------------------------------------------------\n");
		VLOG(INFO,
		     "C    I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX Cycle\n");
		VLOG(INFO,
		     "------------------------------------------------------------------------------\n");
#endif
	} else {
#ifdef REPORT_PIC_SUM_VAR
		VLOG(INFO,
		     "HEVC %02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d   %8d\n",
		     handle->instIndex, encodedInfo->encPicCnt,
		     encodedInfo->picType, encodedInfo->reconFrameIndex,
		     encodedInfo->rdPtr, encodedInfo->wrPtr,
		     encodedInfo->bitstreamSize,
		     (srcEndFlag == 1 ? -1 : srcFrameIdx),
		     encodedInfo->encSrcIdx, encodedInfo->frameCycle,
		     encodedInfo->sumPicVar);
#else
		VLOG(INFO,
		     "HEVC %02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d\n",
		     handle->instIndex, encodedInfo->encPicCnt,
		     encodedInfo->picType, encodedInfo->reconFrameIndex,
		     encodedInfo->rdPtr, encodedInfo->wrPtr,
		     encodedInfo->bitstreamSize,
		     (srcEndFlag == 1 ? -1 : srcFrameIdx),
		     encodedInfo->encSrcIdx, encodedInfo->frameCycle);
#endif
	}
}

#ifdef REDUNDENT_CODE
static void Wave5DisplayEncodedInformation(EncHandle handle,
					   EncOutputInfo *encodedInfo,
					   Int32 srcEndFlag, Int32 srcFrameIdx)
{
	if (encodedInfo == NULL) {
		VLOG(INFO,
		     "--------------------------------------------------------------------------------------------------------------\n");
		VLOG(INFO,
		     "                                                                        | Cycle\n");
		VLOG(INFO,
		     "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX | FRAME PREPARING PROCESSING ENCODING\n");
		VLOG(INFO,
		     "--------------------------------------------------------------------------------------------------------------\n");
	} else {
		VLOG(INFO,
		     "%02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d %8d %8d %8d\n",
		     handle->instIndex, encodedInfo->encPicCnt,
		     encodedInfo->picType, encodedInfo->reconFrameIndex,
		     encodedInfo->rdPtr, encodedInfo->wrPtr,
		     encodedInfo->bitstreamSize,
		     (srcEndFlag == 1 ? -1 : srcFrameIdx),
		     encodedInfo->encSrcIdx, encodedInfo->frameCycle,
		     encodedInfo->encPrepareCycle,
		     encodedInfo->encProcessingCycle,
		     encodedInfo->encEncodingCycle);
	}
}
#endif

static void Coda9DisplayEncodedInformation(DecHandle handle, CodStd codec,
					   EncOutputInfo *encodedInfo)
{
	if (encodedInfo == NULL) {
		// Print header
		VLOG(INFO, "C    I    NO   T   RECON  RD_PTR   WR_PTR\n");
		VLOG(INFO, "-------------------------------------\n");
	} else {
		char **picTypeArray =
			(codec == STD_AVC ? EncPicTypeStringH264 :
						  EncPicTypeStringMPEG4);
		char *strPicType;
		FrameBuffer *pRec = NULL;

		if (encodedInfo->picType > 2)
			strPicType = "?";
		else
			strPicType = picTypeArray[encodedInfo->picType];
		// Print informations
		VLOG(INFO, "AVC  %02d %5d %5s %5d    %08x %08x\n",
		     handle->instIndex, encodedInfo->encPicCnt, strPicType,
		     encodedInfo->reconFrameIndex, encodedInfo->rdPtr,
		     encodedInfo->wrPtr);

		pRec = &encodedInfo->reconFrame;
		CVI_VC_INFO(
			"pRec, bufY = 0x%llX, bufCb = 0x%llX, bufCr = 0x%llX\n",
			pRec->bufY, pRec->bufCb, pRec->bufCr);
	}
}

/*lint -esym(438, ap) */
void DisplayEncodedInformation(EncHandle handle, CodStd codec,
			       EncOutputInfo *encodedInfo, ...)
{
	int srcEndFlag;
	int srcFrameIdx;
	va_list ap;

	switch (codec) {
	case STD_HEVC:
		va_start(ap, encodedInfo);
		srcEndFlag = va_arg(ap, Uint32);
		srcFrameIdx = va_arg(ap, Uint32);
		va_end(ap);
#ifdef REDUNDENT_CODE
		if (handle->productId == PRODUCT_ID_520)
			Wave5DisplayEncodedInformation(handle, encodedInfo,
						       srcEndFlag, srcFrameIdx);
		else
#endif
			Wave4DisplayEncodedInformation(handle, encodedInfo,
						       srcEndFlag, srcFrameIdx);
		break;
	default:
		Coda9DisplayEncodedInformation(handle, codec, encodedInfo);
		break;
	}
}

#ifdef REDUNDENT_CODE
void ChangePathStyle(char *str)
{
	UNREFERENCED_PARAMETER(str);
}
#endif

void ReleaseVideoMemory(Uint32 coreIndex, vpu_buffer_t *memoryArr, Uint32 count)
{
	Uint32 index;

	for (index = 0; index < count; index++) {
		if (memoryArr[index].size)
			VDI_FREE_MEMORY(coreIndex, &memoryArr[index]);
	}
}

#ifdef REDUNDENT_CODE
BOOL AllocateDecFrameBuffer(DecHandle decHandle, TestDecConfig *config,
			    Uint32 tiledFbCount, Uint32 linearFbCount,
			    FrameBuffer *retFbArray, vpu_buffer_t *retFbAddrs,
			    Uint32 *retStride)
{
	Uint32 framebufSize;
	Uint32 totalFbCount;
	Uint32 coreIndex;
	Uint32 index;
	FrameBufferFormat format = config->wtlFormat;
	DecInitialInfo seqInfo;
	FrameBufferAllocInfo fbAllocInfo;
	RetCode ret;
	vpu_buffer_t *pvb;
	size_t framebufStride;
	size_t framebufHeight;
	Uint32 productId;
	DRAMConfig *pDramCfg = NULL;
	DRAMConfig dramCfg = { 0 };

	coreIndex = VPU_HANDLE_CORE_INDEX(decHandle);
	productId = VPU_HANDLE_PRODUCT_ID(decHandle);
	VPU_DecGiveCommand(decHandle, DEC_GET_SEQ_INFO, (void *)&seqInfo);

	if (productId == PRODUCT_ID_960) {
		pDramCfg = &dramCfg;
		ret = VPU_DecGiveCommand(decHandle, GET_DRAM_CONFIG, pDramCfg);
	}

	totalFbCount = tiledFbCount + linearFbCount;

	if (productId == PRODUCT_ID_4102 || productId == PRODUCT_ID_420 ||
	    productId == PRODUCT_ID_412 || productId == PRODUCT_ID_420L ||
	    productId == PRODUCT_ID_510 || productId == PRODUCT_ID_512 ||
	    productId == PRODUCT_ID_515) {
		format = (seqInfo.lumaBitdepth > 8 ||
			  seqInfo.chromaBitdepth > 8) ?
				       FORMAT_420_P10_16BIT_LSB :
				       FORMAT_420;
	} else if (productId == PRODUCT_ID_7Q) {
		if (decHandle->codecMode == HEVC_DEC)
			format = (seqInfo.lumaBitdepth > 8 ||
				  seqInfo.chromaBitdepth > 8) ?
					       FORMAT_420_P10_16BIT_LSB :
					       FORMAT_420;
		else
			format = FORMAT_420;
	}

	if (decHandle->codecMode == C7_VP9_DEC) {
		framebufStride = CalcStride(VPU_ALIGN64(seqInfo.picWidth),
					    seqInfo.picHeight, format,
					    config->cbcrInterleave,
					    config->mapType, TRUE, TRUE);
		framebufHeight = VPU_ALIGN64(seqInfo.picHeight);
		framebufSize = VPU_GetFrameBufSize(
			decHandle->coreIdx, framebufStride, framebufHeight,
			config->mapType, format, config->cbcrInterleave, NULL);
		*retStride = framebufStride;
	} else if (productId == PRODUCT_ID_7Q &&
		   decHandle->codecMode != C7_HEVC_DEC) {
		framebufStride = CalcStride(seqInfo.picWidth, seqInfo.picHeight,
					    format, config->cbcrInterleave,
					    config->mapType, FALSE, TRUE);
		framebufHeight = seqInfo.interlace ?
					       VPU_ALIGN32(seqInfo.picHeight) :
					       VPU_ALIGN16(seqInfo.picHeight);
		framebufSize = VPU_GetFrameBufSize(
			decHandle->coreIdx, framebufStride, framebufHeight,
			config->mapType, format, config->cbcrInterleave, NULL);
		*retStride = framebufStride;
	} else if (decHandle->codecMode == C7_AVS2_DEC) {
		framebufStride = CalcStride(seqInfo.picWidth, seqInfo.picHeight,
					    format, config->cbcrInterleave,
					    config->mapType, FALSE, TRUE);
		framebufHeight = VPU_ALIGN8(seqInfo.picHeight);
		framebufSize =
			VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride,
					    framebufHeight, config->mapType,
					    format, config->cbcrInterleave,
					    pDramCfg);
		*retStride = framebufStride;
	} else {
		*retStride = VPU_ALIGN32(seqInfo.picWidth);
		framebufStride = CalcStride(seqInfo.picWidth, seqInfo.picHeight,
					    format, config->cbcrInterleave,
					    config->mapType, FALSE, TRUE);
		framebufHeight = seqInfo.picHeight;
		framebufSize =
			VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride,
					    seqInfo.picHeight, config->mapType,
					    format, config->cbcrInterleave,
					    pDramCfg);
	}

	osal_memset((void *)&fbAllocInfo, 0x00, sizeof(fbAllocInfo));
	osal_memset((void *)retFbArray, 0x00,
		    sizeof(FrameBuffer) * totalFbCount);
	fbAllocInfo.format = format;
	fbAllocInfo.cbcrInterleave = config->cbcrInterleave;
	fbAllocInfo.mapType = config->mapType;
	fbAllocInfo.stride = framebufStride;
	fbAllocInfo.height = framebufHeight;
	fbAllocInfo.size = framebufSize;
	fbAllocInfo.lumaBitDepth = seqInfo.lumaBitdepth;
	fbAllocInfo.chromaBitDepth = seqInfo.chromaBitdepth;
	fbAllocInfo.num = tiledFbCount;
	fbAllocInfo.endian = config->frameEndian;
	fbAllocInfo.type = FB_TYPE_CODEC;
	osal_memset((void *)retFbAddrs, 0x00,
		    sizeof(vpu_buffer_t) * totalFbCount);
	for (index = 0; index < tiledFbCount; index++) {
		pvb = &retFbAddrs[index];
		pvb->size = framebufSize;
		CVI_VC_MEM("FB[%d], size = 0x%X\n", index, pvb->size);
		if (VDI_ALLOCATE_MEMORY(coreIndex, pvb, 0) < 0) {
			VLOG(ERR, "%s:%d fail to allocate frame buffer\n",
			     __func__, __LINE__);
			ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
			return FALSE;
		}
		retFbArray[index].bufY = pvb->phys_addr;
		retFbArray[index].bufCb = (PhysicalAddress)-1;
		retFbArray[index].bufCr = (PhysicalAddress)-1;
		retFbArray[index].updateFbInfo = TRUE;
		retFbArray[index].size = framebufSize;
	}

	if (tiledFbCount != 0) {
		ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo,
						 retFbArray);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "%s:%d failed to VPU_DecAllocateFrameBuffer(), ret(%d)\n",
			     __func__, __LINE__, ret);
			ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
			return FALSE;
		}
	}

	if (config->enableWTL == TRUE || linearFbCount != 0) {
		size_t linearStride;
		size_t picWidth;
		size_t picHeight;
		size_t fbHeight;
		Uint32 mapType = LINEAR_FRAME_MAP;
		FrameBufferFormat outFormat = config->wtlFormat;
		picWidth = seqInfo.picWidth;
		picHeight = seqInfo.picHeight;
		fbHeight = picHeight;
		if (decHandle->codecMode == C7_VP9_DEC) {
			fbHeight = VPU_ALIGN64(picHeight);
		} else if (decHandle->codecMode == C7_AVS2_DEC) {
			fbHeight = VPU_ALIGN8(picHeight);
		} else if (productId == PRODUCT_ID_7Q &&
			   decHandle->codecMode != C7_HEVC_DEC) {
			fbHeight = seqInfo.interlace ? VPU_ALIGN32(picHeight) :
							     VPU_ALIGN16(picHeight);
		} else if (productId == PRODUCT_ID_960 ||
			   productId == PRODUCT_ID_980) {
			fbHeight = VPU_ALIGN32(picHeight);
		}
		if (decHandle->codecMode == C7_VP9_DEC) {
			linearStride =
				CalcStride(VPU_ALIGN64(picWidth), picHeight,
					   outFormat, config->cbcrInterleave,
					   (TiledMapType)mapType, TRUE, TRUE);
		} else {
			linearStride =
				CalcStride(picWidth, picHeight, outFormat,
					   config->cbcrInterleave,
					   (TiledMapType)mapType, FALSE, TRUE);
		}
		framebufSize =
			VPU_GetFrameBufSize(coreIndex, linearStride, fbHeight,
					    (TiledMapType)mapType, outFormat,
					    config->cbcrInterleave, pDramCfg);

		for (index = tiledFbCount; index < totalFbCount; index++) {
			pvb = &retFbAddrs[index];
			pvb->size = framebufSize;
			CVI_VC_MEM("FB[%d], size = 0x%X\n", index, pvb->size);
			if (VDI_ALLOCATE_MEMORY(coreIndex, pvb, 0) < 0) {
				VLOG(ERR,
				     "%s:%d fail to allocate frame buffer\n",
				     __func__, __LINE__);
				ReleaseVideoMemory(coreIndex, retFbAddrs,
						   totalFbCount);
				return FALSE;
			}
			retFbArray[index].bufY = pvb->phys_addr;
			retFbArray[index].bufCb = -1;
			retFbArray[index].bufCr = -1;
			retFbArray[index].updateFbInfo = TRUE;
			retFbArray[index].size = framebufSize;
		}

		fbAllocInfo.nv21 = config->nv21;
		fbAllocInfo.format = outFormat;
		fbAllocInfo.num = linearFbCount;
		fbAllocInfo.mapType = (TiledMapType)mapType;
		fbAllocInfo.stride = linearStride;
		fbAllocInfo.height = fbHeight;
		ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo,
						 &retFbArray[tiledFbCount]);
		if (ret != RETCODE_SUCCESS) {
			VLOG(ERR,
			     "%s:%d failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
			     __func__, __LINE__, ret);
			ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
			return FALSE;
		}
	}

	return TRUE;
}

#if defined(_WIN32) || defined(__MSDOS__)
#define DOS_FILESYSTEM
#define IS_DIR_SEPARATOR(__c) ((__c == '/') || (__c == '\\'))
#else
/* UNIX style */
#define IS_DIR_SEPARATOR(__c) (__c == '/')
#endif

char *GetDirname(const char *path)
{
	int length;
	int i;
	char *upper_dir;

	if (path == NULL)
		return NULL;

	length = strlen(path);
	for (i = length - 1; i >= 0; i--) {
		if (IS_DIR_SEPARATOR(path[i]))
			break;
	}

	if (i < 0) {
		upper_dir = strdup(".");
	} else {
		upper_dir = strdup(path);
		upper_dir[i] = 0;
	}

	return upper_dir;
}

char *GetBasename(const char *pathname)
{
	const char *base = NULL;
	const char *p = pathname;

	if (p == NULL) {
		return NULL;
	}

#if defined(DOS_FILESYSTEM)
	if (isalpha((int)p[0]) && p[1] == ':') {
		p += 2;
	}
#endif

	for (base = p; *p; p++) {
		if (IS_DIR_SEPARATOR(*p)) {
			base = p + 1;
		}
	}

	return (char *)base;
}

char *GetFileExtension(const char *filename)
{
	Uint32 len;
	Int32 i;

	len = strlen(filename);
	for (i = (Int32)len - 1; i >= 0; i--) {
		if (filename[i] == '.') {
			return (char *)&filename[i + 1];
		}
	}

	return NULL;
}

void byte_swap(unsigned char *data, int len)
{
	Uint8 temp;
	Int32 i;

	for (i = 0; i < len; i += 2) {
		temp = data[i];
		data[i] = data[i + 1];
		data[i + 1] = temp;
	}
}

BOOL IsEndOfFile(FILE *fp)
{
	BOOL result = FALSE;
	Int32 index = 0;
	char cTemp;

	// Check current fp pos
	if (osal_feof(fp) != 0) {
		result = TRUE;
	}

	// Check next fp pos
	// Ignore newline character
	do {
		cTemp = fgetc(fp);
		index++;

		if (osal_feof(fp) != 0) {
			result = TRUE;
			break;
		}
	} while (cTemp == '\n' || cTemp == '\r');

	// Revert fp pos
	index *= (-1);
	osal_fseek(fp, index, SEEK_CUR);

	return result;
}
#endif

BOOL CalcYuvSize(Int32 format, Int32 picWidth, Int32 picHeight,
		 Int32 cbcrInterleave, size_t *lumaSize, size_t *chromaSize,
		 size_t *frameSize, Int32 *bitDepth, Int32 *packedFormat,
		 Int32 *yuv3p4b)
{
	Int32 temp_picWidth;
	Int32 chromaWidth;

	if (bitDepth != 0)
		*bitDepth = 0;
	if (packedFormat != 0)
		*packedFormat = 0;
	if (yuv3p4b != 0)
		*yuv3p4b = 0;

	CVI_VC_TRACE("format = %d\n", format);

	switch (format) {
	case FORMAT_420:
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = (picWidth * picHeight) >> 1;
		if (frameSize)
			*frameSize = (picWidth * picHeight * 3) >> 1;
		break;
	case FORMAT_YUYV:
	case FORMAT_YVYU:
	case FORMAT_UYVY:
	case FORMAT_VYUY:
		if (packedFormat != 0)
			*packedFormat = 1;
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = picWidth * picHeight;
		if (frameSize)
			*frameSize = *lumaSize + *chromaSize;
		break;
	case FORMAT_224:
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = picWidth * picHeight;
		if (frameSize)
			*frameSize = picWidth * picHeight * 2;
		break;
	case FORMAT_422:
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = picWidth * picHeight;
		if (frameSize)
			*frameSize = picWidth * picHeight * 2;
		break;
	case FORMAT_444:
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = picWidth * picHeight * 2;
		if (frameSize)
			*frameSize = picWidth * picHeight * 3;
		break;
	case FORMAT_400:
		if (lumaSize)
			*lumaSize = picWidth * picHeight;
		if (chromaSize)
			*chromaSize = 0;
		if (frameSize)
			*frameSize = picWidth * picHeight;
		break;
	case FORMAT_422_P10_16BIT_MSB:
	case FORMAT_422_P10_16BIT_LSB:
		if (bitDepth != NULL) {
			*bitDepth = 10;
		}
		if (lumaSize)
			*lumaSize = picWidth * picHeight * 2;
		if (chromaSize)
			*chromaSize = *lumaSize;
		if (frameSize)
			*frameSize = *lumaSize + *chromaSize;
		break;
	case FORMAT_420_P10_16BIT_MSB:
	case FORMAT_420_P10_16BIT_LSB:
		if (bitDepth != 0)
			*bitDepth = 10;
		if (lumaSize)
			*lumaSize = picWidth * picHeight * 2;
		if (chromaSize)
			*chromaSize = picWidth * picHeight;
		if (frameSize)
			*frameSize = *lumaSize + *chromaSize;
		break;
	case FORMAT_YUYV_P10_16BIT_MSB: // 4:2:2 10bit packed
	case FORMAT_YUYV_P10_16BIT_LSB:
	case FORMAT_YVYU_P10_16BIT_MSB:
	case FORMAT_YVYU_P10_16BIT_LSB:
	case FORMAT_UYVY_P10_16BIT_MSB:
	case FORMAT_UYVY_P10_16BIT_LSB:
	case FORMAT_VYUY_P10_16BIT_MSB:
	case FORMAT_VYUY_P10_16BIT_LSB:
		if (bitDepth != 0)
			*bitDepth = 10;
		if (packedFormat != 0)
			*packedFormat = 1;
		if (lumaSize)
			*lumaSize = picWidth * picHeight * 2;
		if (chromaSize)
			*chromaSize = picWidth * picHeight * 2;
		if (frameSize)
			*frameSize = *lumaSize + *chromaSize;
		break;
	case FORMAT_420_P10_32BIT_MSB:
	case FORMAT_420_P10_32BIT_LSB:
		if (bitDepth != 0)
			*bitDepth = 10;
		if (yuv3p4b != 0)
			*yuv3p4b = 1;
		temp_picWidth = VPU_ALIGN32(picWidth);
		chromaWidth = ((VPU_ALIGN16((temp_picWidth >> 1) *
					    (1 << cbcrInterleave)) +
				2) /
			       3 * 4);
		if (cbcrInterleave == 1) {
			if (lumaSize)
				*lumaSize =
					(temp_picWidth + 2) / 3 * 4 * picHeight;
			if (chromaSize)
				*chromaSize = (chromaWidth * picHeight) >> 1;
		} else {
			if (lumaSize)
				*lumaSize =
					(temp_picWidth + 2) / 3 * 4 * picHeight;
			if (chromaSize)
				*chromaSize = ((chromaWidth * picHeight) >> 1)
					      << 1;
		}
		if (frameSize)
			*frameSize = *lumaSize + *chromaSize;
		break;
	case FORMAT_YUYV_P10_32BIT_MSB:
	case FORMAT_YUYV_P10_32BIT_LSB:
	case FORMAT_YVYU_P10_32BIT_MSB:
	case FORMAT_YVYU_P10_32BIT_LSB:
	case FORMAT_UYVY_P10_32BIT_MSB:
	case FORMAT_UYVY_P10_32BIT_LSB:
	case FORMAT_VYUY_P10_32BIT_MSB:
	case FORMAT_VYUY_P10_32BIT_LSB:
		if (bitDepth != 0)
			*bitDepth = 10;
		if (packedFormat != 0)
			*packedFormat = 1;
		if (yuv3p4b != 0)
			*yuv3p4b = 1;
		if (frameSize)
			*frameSize = ((picWidth * 2) + 2) / 3 * 4 * picHeight;
		if (lumaSize)
			*lumaSize = *frameSize >> 1;
		if (chromaSize)
			*chromaSize = *frameSize >> 1;
		break;
	default:
		if (frameSize)
			*frameSize = (picWidth * picHeight * 3) >> 1;
		VLOG(ERR, "%s:%d Not supported format(%d)\n", __FILE__,
		     __LINE__, format);
		return FALSE;
	}
	return TRUE;
}

FrameBufferFormat GetPackedFormat(int srcBitDepth, PackedFormatNum packedType,
				  int p10bits, int msb)
{
	int format = FORMAT_YUYV;

	// default pixel format = P10_16BIT_LSB (p10bits = 16, msb = 0)
	if (srcBitDepth == 8) {
		switch (packedType) {
		case PACKED_YUYV:
			format = FORMAT_YUYV;
			break;
		case PACKED_YVYU:
			format = FORMAT_YVYU;
			break;
		case PACKED_UYVY:
			format = FORMAT_UYVY;
			break;
		case PACKED_VYUY:
			format = FORMAT_VYUY;
			break;
		default:
			format = -1;
		}
	} else if (srcBitDepth == 10) {
		switch (packedType) {
		case PACKED_YUYV:
			if (p10bits == 16) {
				format = (msb == 0) ?
						       FORMAT_YUYV_P10_16BIT_LSB :
						       FORMAT_YUYV_P10_16BIT_MSB;
			} else if (p10bits == 32) {
				format = (msb == 0) ?
						       FORMAT_YUYV_P10_32BIT_LSB :
						       FORMAT_YUYV_P10_32BIT_MSB;
			} else {
				format = -1;
			}
			break;
		case PACKED_YVYU:
			if (p10bits == 16) {
				format = (msb == 0) ?
						       FORMAT_YVYU_P10_16BIT_LSB :
						       FORMAT_YVYU_P10_16BIT_MSB;
			} else if (p10bits == 32) {
				format = (msb == 0) ?
						       FORMAT_YVYU_P10_32BIT_LSB :
						       FORMAT_YVYU_P10_32BIT_MSB;
			} else {
				format = -1;
			}
			break;
		case PACKED_UYVY:
			if (p10bits == 16) {
				format = (msb == 0) ?
						       FORMAT_UYVY_P10_16BIT_LSB :
						       FORMAT_UYVY_P10_16BIT_MSB;
			} else if (p10bits == 32) {
				format = (msb == 0) ?
						       FORMAT_UYVY_P10_32BIT_LSB :
						       FORMAT_UYVY_P10_32BIT_MSB;
			} else {
				format = -1;
			}
			break;
		case PACKED_VYUY:
			if (p10bits == 16) {
				format = (msb == 0) ?
						       FORMAT_VYUY_P10_16BIT_LSB :
						       FORMAT_VYUY_P10_16BIT_MSB;
			} else if (p10bits == 32) {
				format = (msb == 0) ?
						       FORMAT_VYUY_P10_32BIT_LSB :
						       FORMAT_VYUY_P10_32BIT_MSB;
			} else {
				format = -1;
			}
			break;
		default:
			format = -1;
		}
	} else {
		format = -1;
	}

	return format;
}

void GenRegionToMap(VpuRect *region,
		    int *roiLevel, int num, Uint32 mapWidth, Uint32 mapHeight,
		    Uint8 *roiCtuMap)
{
	Int32 roi_id, blk_addr;
	Uint32 roi_map_size = mapWidth * mapHeight;
	Uint32 x, y;
	// init roi map
	for (blk_addr = 0; blk_addr < (Int32)roi_map_size; blk_addr++)
		roiCtuMap[blk_addr] = 0;

	// set roi map. roi_entry[i+1] has higher priority than roi_entry[i]
	for (roi_id = 0; roi_id < (Int32)num; roi_id++) {

		VpuRect *roi = region + roi_id;

		CVI_VC_TRACE("roi level:%d\n", *(roiLevel + roi_id));
		for (y = roi->top; y <= roi->bottom; y++) {
			for (x = roi->left; x <= roi->right; x++) {
				roiCtuMap[y * mapWidth + x] = *(roiLevel + roi_id);
			}
		}
	}
	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++)
			CVI_VC_TRACE("%d ", roiCtuMap[y*mapWidth + x]);
		CVI_VC_TRACE("\n");
	}
}

#ifdef REDUNDENT_CODE
#ifdef SUPPORT_980_ROI_RC_LIB
void GenRegionToMap980(VpuRect *region, /**< The size of the ROI region (start
					   X/Y in pixel, end X/Y in pixel)  */
		       int *roiLevel, int num, Uint32 mapWidth,
		       Uint32 mapHeight, Uint8 *roiCtuMap)
{
	Int32 roi_id, blk_addr;
	Uint32 roi_map_size = mapWidth * mapHeight;

	// init roi map
	for (blk_addr = 0; blk_addr < (Int32)roi_map_size; blk_addr++)
		roiCtuMap[blk_addr] = 0;

	// set roi map. roi_entry[i+1] has higher priority than roi_entry[i]
	for (roi_id = (Int32)num - 1; roi_id >= 0; roi_id--) {
		Uint32 x, y, top, bottom, left, right;
		VpuRect *roi = region + roi_id;

		// convert pixel unit to ctu(64x64) unit.
		top = (roi->top >> 6);
		bottom = (roi->bottom >> 6);
		left = (roi->left >> 6);
		right = (roi->right >> 6);

		for (y = top; y <= bottom; y++) {
			for (x = left; x <= right; x++) {
				roiCtuMap[y * mapWidth + x] =
					*(roiLevel + roi_id);
			}
		}
	}
}
#endif
#endif

void GenRegionToQpMap(
	VpuRect *region, /**< The size of the ROI region for H.265 (start X/Y in
						   CTU, end X/Y int CTU)  */
	int *roiLevel, int num, int initQp, Uint32 mapWidth, Uint32 mapHeight,
	Uint8 *roiCtuMap)
{
	Int32 roi_id, blk_addr;
	Uint32 roi_map_size = mapWidth * mapHeight;

	// init roi map
	for (blk_addr = 0; blk_addr < (Int32)roi_map_size; blk_addr++)
		roiCtuMap[blk_addr] = initQp;

	// set roi map. roi_entry[i] has higher priority than roi_entry[i+1]
	for (roi_id = 0; roi_id < num; roi_id++) {
		Uint32 x, y;
		VpuRect *roi = region + roi_id;

		for (y = roi->top; y <= roi->bottom; y++) {
			for (x = roi->left; x <= roi->right; x++) {
				roiCtuMap[y * mapWidth + x] =
					*(roiLevel + roi_id);
			}
		}
	}
}

Int32 writeVuiRbsp(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   vpu_buffer_t *vbVuiRbsp)
{
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	CVI_VC_TRACE("\n");

	if (pEncOP->encodeVuiRbsp == TRUE) {
		vbVuiRbsp->size = VUI_HRD_RBSP_BUF_SIZE;

		CVI_VC_MEM("vbVuiRbsp->size = 0x%X\n", vbVuiRbsp->size);
		sprintf(ionName, "VENC_%d_VuiRbsp", pEncConfig->s32ChnNum);
		if (VDI_ALLOCATE_MEMORY(coreIdx, vbVuiRbsp, 0, ionName) < 0) {
			VLOG(ERR, "fail to allocate VUI rbsp buffer\n");
			return FALSE;
		}
		pEncOP->vuiRbspDataAddr = vbVuiRbsp->phys_addr;
	}
	return TRUE;
}

Int32 writeHrdRbsp(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   vpu_buffer_t *vbHrdRbsp)
{
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	CVI_VC_TRACE("\n");

	if (pEncOP->encodeHrdRbspInVPS || pEncOP->encodeHrdRbspInVUI) {
		vbHrdRbsp->size = VUI_HRD_RBSP_BUF_SIZE;
		CVI_VC_MEM("vbHrdRbsp->size = 0x%X\n", vbHrdRbsp->size);
		sprintf(ionName, "VENC_%d_HrdRbsp", pEncConfig->s32ChnNum);
		if (VDI_ALLOCATE_MEMORY(coreIdx, vbHrdRbsp, 0, ionName) < 0) {
			VLOG(ERR, "fail to allocate HRD rbsp buffer\n");
			return FALSE;
		}

		pEncOP->hrdRbspDataAddr = vbHrdRbsp->phys_addr;
	}
	return TRUE;
}

#ifdef TEST_ENCODE_CUSTOM_HEADER
Int32 writeCustomHeader(int coreIdx, EncOpenParam *pEncOP,
			vpu_buffer_t *vbCustomHeader, hrd_t *hrd)
{
	Int32 rbspBitSize;
	Uint8 *pRbspBuf;
	Int32 rbspByteSize;
	vui_t vui;

	CVI_VC_TRACE("\n");

	pEncOP->encodeVuiRbsp = 1;

	vbCustomHeader->size = VUI_HRD_RBSP_BUF_SIZE;

	CVI_VC_MEM("vbCustomHeader->size = 0x%X\n", vbCustomHeader->size);
	if (VDI_ALLOCATE_MEMORY(coreIdx, vbCustomHeader, 0) < 0) {
		VLOG(ERR, "fail to allocate VUI rbsp buffer\n");
		return FALSE;
	}

	pEncOP->vuiRbspDataAddr = vbCustomHeader->phys_addr;

	pRbspBuf = (Uint8 *)osal_malloc(VUI_HRD_RBSP_BUF_SIZE);
	if (pRbspBuf) {
		osal_memset(pRbspBuf, 0, VUI_HRD_RBSP_BUF_SIZE);
		vui.vui_parameters_presesent_flag = 1;
		vui.vui_timing_info_present_flag = 1;
		vui.vui_num_units_in_tick = 1000;
		vui.vui_time_scale = 60 * 1000.0;
		vui.vui_hrd_parameters_present_flag = 1;
		vui.def_disp_win_left_offset = 1;
		vui.def_disp_win_right_offset = 1;
		vui.def_disp_win_top_offset = 1;
		vui.def_disp_win_bottom_offset = 1;

		// HRD param : refer xInitHrdParameters in HM
		{
			int useSubCpbParams = 0;
			int bitRate = pEncOP->bitRate;
			int isRandomAccess;
			int cpbSize = bitRate; // Adjusting value to be equal to
				// TargetBitrate
			int bitRateScale;
			int cpbSizeScale;
			int i, j, numSubLayersM1;
			Uint32 bitrateValue, cpbSizeValue;
			Uint32 duCpbSizeValue;
			Uint32 duBitRateValue = 0;

			if (bitRate > 0) {
				hrd->nal_hrd_parameters_present_flag = 1;
				hrd->vcl_hrd_parameters_present_flag = 1;
			} else {
				hrd->nal_hrd_parameters_present_flag = 0;
				hrd->vcl_hrd_parameters_present_flag = 0;
			}

			if (pEncOP->EncStdParam.hevcParam.independSliceMode !=
			    0)
				useSubCpbParams = 1;

			if (pEncOP->EncStdParam.hevcParam.intraPeriod > 0)
				isRandomAccess = 1;
			else
				isRandomAccess = 0;

			hrd->sub_pic_hrd_params_present_flag = useSubCpbParams;
			if (useSubCpbParams) {
				hrd->tick_divisor_minus2 = 100 - 2;
				hrd->du_cpb_removal_delay_increment_length_minus1 =
					7; // 8-bit precision ( plus 1 for last
				// DU in AU )
				hrd->sub_pic_cpb_params_in_pic_timing_sei_flag =
					1;
				hrd->dpb_output_delay_du_length_minus1 =
					5 + 7; // With sub-clock tick factor of
				// 100, at least 7 bits to have
				// the same value as AU dpb delay
			} else
				hrd->sub_pic_cpb_params_in_pic_timing_sei_flag =
					0;

			//  calculate scale value of bitrate and initial delay
			bitRateScale = calcScale(bitRate);
			if (bitRateScale <= 6)
				hrd->bit_rate_scale = 0;
			else
				hrd->bit_rate_scale = bitRateScale - 6;

			cpbSizeScale = calcScale(cpbSize);
			if (cpbSizeScale <= 4)
				hrd->cpb_size_scale = 0;
			else
				hrd->cpb_size_scale = cpbSizeScale - 4;

			hrd->cpb_size_du_scale = 6; // in units of 2^( 4 + 6 ) =
				// 1,024 bit
			hrd->initial_cpb_removal_delay_length_minus1 =
				15; // assuming 0.5 sec, log2( 90,000 * 0.5 ) =
			// 16-bit

			if (isRandomAccess > 0) {
				hrd->au_cpb_removal_delay_length_minus1 = 5;
				hrd->dpb_output_delay_length_minus1 = 5;
			} else {
				hrd->au_cpb_removal_delay_length_minus1 = 9;
				hrd->dpb_output_delay_length_minus1 = 9;
			}

			numSubLayersM1 = 0;
			if (pEncOP->EncStdParam.hevcParam.gopPresetIdx ==
			    0) { // custom GOP
				for (i = 0; i < pEncOP->EncStdParam.hevcParam
							.gopParam.customGopSize;
				     i++) {
					if (numSubLayersM1 <
					    pEncOP->EncStdParam.hevcParam
						    .gopParam.picParam[i]
						    .temporalId)
						numSubLayersM1 =
							pEncOP->EncStdParam
								.hevcParam
								.gopParam
								.picParam[i]
								.temporalId;
				}
			}
			hrd->vps_max_sub_layers_minus1 = numSubLayersM1;
			// sub_layer_hrd_parameters
			// BitRate[ i ] = ( bit_rate_value_minus1[ i ] + 1 ) *
			// 2^( 6 + bit_rate_scale )
			bitrateValue =
				bitRate /
				(1 << (6 + hrd->bit_rate_scale)); // bitRate is
			// in bits, so
			// it needs to
			// be scaled
			// down
			// CpbSize[ i ] = ( cpb_size_value_minus1[ i ] + 1 ) *
			// 2^( 4 + cpb_size_scale )
			cpbSizeValue =
				cpbSize /
				(1 << (4 + hrd->cpb_size_scale)); // using
			// bitRate
			// results
			// in 1
			// second
			// CPB
			// size

			// DU CPB size could be smaller (i.e. bitrateValue /
			// number of DUs), but we don't know in how many DUs the
			// slice segment settings will result
			duCpbSizeValue = bitrateValue;
			duBitRateValue = cpbSizeValue;

			for (i = 0; i < (int)hrd->vps_max_sub_layers_minus1;
			     i++) {
				hrd->fixed_pic_rate_general_flag[i] = 1;
				hrd->fixed_pic_rate_within_cvs_flag[i] =
					1; // fixed_pic_rate_general_flag[ i ]
				// is equal to 1, the value of
				// fixed_pic_rate_within_cvs_flag[ i
				// ] is inferred to be equal to 1
				hrd->elemental_duration_in_tc_minus1[i] = 0;

				hrd->low_delay_hrd_flag[i] = 0;
				hrd->cpb_cnt_minus1[i] = 0;

				if (hrd->nal_hrd_parameters_present_flag) {
					for (j = 0; hrd->cpb_cnt_minus1[i];
					     j++) {
						hrd->bit_rate_value_minus1[j][i] =
							bitrateValue - 1;
						hrd->cpb_size_value_minus1[j][i] =
							cpbSize - 1;
						hrd->cpb_size_du_value_minus1[j]
									     [i] =
							duCpbSizeValue - 1;
						hrd->bit_rate_du_value_minus1[j]
									     [i] =
							duBitRateValue - 1;
						hrd->cbr_flag[j][i] = 0;
					}
				}
				if (hrd->vcl_hrd_parameters_present_flag) {
					for (j = 0; hrd->cpb_cnt_minus1[i];
					     j++) {
						hrd->bit_rate_value_minus1[j][i] =
							bitrateValue - 1;
						hrd->cpb_size_value_minus1[j][i] =
							cpbSize - 1;
						hrd->cpb_size_du_value_minus1[j]
									     [i] =
							duCpbSizeValue - 1;
						hrd->bit_rate_du_value_minus1[j]
									     [i] =
							duBitRateValue - 1;
						hrd->cbr_flag[j][i] = 0;
					}
				}
			}
		}

		EncodeVUI(hrd, &vui, pRbspBuf, VUI_HRD_RBSP_BUF_SIZE,
			  &rbspByteSize, &rbspBitSize, 60);
		pEncOP->vuiRbspDataSize = rbspBitSize;
		vdi_write_memory(coreIdx, pEncOP->vuiRbspDataAddr, pRbspBuf,
				 rbspByteSize, pEncOP->streamEndian);
		osal_free(pRbspBuf);
	}
	return TRUE;
}

Int32 allocateSeiNalDataBuf(int coreIdx, vpu_buffer_t *vbSeiNal, int srcFbNum)
{
	Int32 i;
	for (i = 0; i < srcFbNum; i++) { // the number of roi buffer should be
		// the same as source buffer num.
		vbSeiNal[i].size = SEI_NAL_DATA_BUF_SIZE;
		CVI_VC_MEM("vbSeiNal[%d].size = 0x%X\n", i, vbSeiNal[i].size);
		if (VDI_ALLOCATE_MEMORY(coreIdx, &vbSeiNal[i], 0) < 0) {
			VLOG(ERR, "fail to allocate SeiNalData buffer\n");
			return FALSE;
		}
	}
	return TRUE;
}

Int32 writeSeiNalData(EncHandle handle, int streamEndian,
		      PhysicalAddress prefixSeiNalAddr, hrd_t *hrd)
{
	sei_buffering_period_t buffering_period_sei;
	Uint8 *pSeiBuf;
	sei_active_parameter_t active_parameter_sei;
	Int32 seiByteSize = 0;
	HevcSEIDataEnc seiDataEnc;
	sei_pic_timing_t pic_timing_sei;

	pSeiBuf = (Uint8 *)osal_malloc(SEI_NAL_DATA_BUF_SIZE);
	if (pSeiBuf) {
		osal_memset(pSeiBuf, 0x00, SEI_NAL_DATA_BUF_SIZE);
		osal_memset(&seiDataEnc, 0x00, sizeof(seiDataEnc));

		seiDataEnc.prefixSeiNalEnable = 1;
		seiDataEnc.prefixSeiDataEncOrder = 0;
		seiDataEnc.prefixSeiNalAddr = prefixSeiNalAddr;

		active_parameter_sei.active_video_parameter_set_id =
			0; // vps_video_parameter_set_id of the VPS. wave420 is
		// 0
		active_parameter_sei.self_contained_cvs_flag = 0;
		active_parameter_sei.no_parameter_set_update_flag = 0;
		active_parameter_sei.num_sps_ids_minus1 = 0;
		active_parameter_sei.active_seq_parameter_set_id[0] =
			0; // sps_seq_parameter_set_id of the SPS. wave420 is 0

		// put sei_pic_timing
		pic_timing_sei.pic_struct = 0;
		pic_timing_sei.source_scan_type = 1;
		pic_timing_sei.duplicate_flag = 0;

		buffering_period_sei.nal_initial_cpb_removal_delay[0] = 2229;
		buffering_period_sei.nal_initial_cpb_removal_offset[0] = 0;

		seiByteSize =
			EncodePrefixSEI(&active_parameter_sei, &pic_timing_sei,
					&buffering_period_sei, &hrd, pSeiBuf,
					SEI_NAL_DATA_BUF_SIZE);
		seiDataEnc.prefixSeiDataSize = seiByteSize;
		vdi_write_memory(handle->coreIdx, seiDataEnc.prefixSeiNalAddr,
				 pSeiBuf, seiDataEnc.prefixSeiDataSize,
				 streamEndian);

		free(pSeiBuf);
	}
	VPU_EncGiveCommand(handle, ENC_SET_SEI_NAL_DATA, &seiDataEnc);
	return TRUE;
}
#endif

void setRoiMapFromMap(int coreIdx, TestEncConfig *pEncConfig,
		      EncOpenParam *pEncOP, PhysicalAddress addrRoiMap,
		      Uint8 *roiMapBuf, EncParam *encParam, int frmNum)
{
	int mapWidth, mapHeight, roi_map_size;
	int x, y;
	int foregroundCnt = 0;
	int roi_percent = 0;
	CVI_VC_TRACE("roi_enable = %d\n", pEncConfig->roi_enable);

	if ((!(pEncConfig->roi_enable && encParam->srcEndFlag != 1)) &&
		pEncConfig->cviApiMode == API_MODE_DRIVER) {
		return;
	}
	mapWidth = ((pEncOP->picWidth + 63) & ~63) >> 6;
	mapHeight = ((pEncOP->picHeight + 63) & ~63) >> 6;
	roi_map_size = mapWidth * mapHeight;
	encParam->ctuOptParam.mapEndian = VDI_LITTLE_ENDIAN;
	encParam->ctuOptParam.mapStride = ((pEncOP->picWidth + 63) & ~63) >> 6;
	encParam->ctuOptParam.addrRoiCtuMap = addrRoiMap;

	encParam->ctuOptParam.roiDeltaQp = ABS(pEncOP->fg_dealt_qp);
	// DEBUG
	CVI_VC_MOTMAP("###############################\n");
	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {
			if (pEncOP->picMotionMap[(y * mapWidth) + x]) {
				roiMapBuf[(y * mapWidth) + x] = 2;
				foregroundCnt++;
			} else {
				roiMapBuf[(y * mapWidth) + x] = 0;
			}
		}
	}

	encParam->ctuOptParam.roiEnable = (foregroundCnt > 0) ? TRUE : FALSE;
	roi_percent = foregroundCnt * 100 / roi_map_size;
	if (roi_percent > 80) {
		encParam->ctuOptParam.roiEnable = FALSE;
	} else if (roi_percent > 70 &&
				encParam->ctuOptParam.roiDeltaQp > 1) {
		encParam->ctuOptParam.roiDeltaQp = 1;
	} else if (roi_percent > 50 &&
				encParam->ctuOptParam.roiDeltaQp > 2) {
		encParam->ctuOptParam.roiDeltaQp = 2;
	} else if (roi_percent > 35 &&
				encParam->ctuOptParam.roiDeltaQp > 3) {
		encParam->ctuOptParam.roiDeltaQp = 3;
	} else if (roi_percent > 15 &&
				encParam->ctuOptParam.roiDeltaQp > 5) {
		encParam->ctuOptParam.roiDeltaQp = 5;
	}

	if (!(frmNum % 20)) {
		char buf_dbg[128] = {0};
		u32 pos = 0;
		int i = 0;

		for (i = 0; i < mapHeight * mapWidth; i++) {
			sprintf(buf_dbg + pos, "%d ",
			       (pEncOP->picMotionMap[i]) != 0);
			pos += 2;
			if ((i + 1) % mapWidth == 0) {
				CVI_VC_MOTMAP("%s\n", buf_dbg);
				pos = 0;
			}
		}

	}

	vdi_write_memory(coreIdx, encParam->ctuOptParam.addrRoiCtuMap,
					roiMapBuf, mapHeight * mapWidth,
					encParam->ctuOptParam.mapEndian);
#ifdef DUMP_MOTIONMAP
	{
		int i = 0;

		for (i = 0; i < mapHeight * mapWidth; i++)
			roiMapBuf[i] = 127 * roiMapBuf[i];

		kernel_write(pEncConfig->filp, roiMapBuf,
			    mapHeight * mapWidth, &pEncConfig->pos);
	}
#endif
}

int cviSetCtuRoiLevelMap(int coreIdx, TestEncConfig *pEncConfig,
			     EncOpenParam *pEncOP, PhysicalAddress addrRoiMap,
			     Uint8 *roiMapBuf, EncParam *encParam,
			     int maxCtuNum)
{
	CVI_VC_TRACE("roi_enable = %d\n", pEncConfig->roi_enable);

	if (pEncConfig->roi_enable && encParam->srcEndFlag != 1) {
		int roiNum = pEncConfig->actRegNum;
		encParam->ctuOptParam.addrRoiCtuMap = addrRoiMap;
		encParam->ctuOptParam.mapEndian = VDI_LITTLE_ENDIAN;
		encParam->ctuOptParam.mapStride =
			((pEncOP->picWidth + 63) & ~63) >> 6;
		encParam->ctuOptParam.roiEnable =
			(roiNum != 0) ? pEncConfig->roi_enable : 0;
			encParam->ctuOptParam.roiDeltaQp = pEncConfig->roi_delta_qp;
		CVI_VC_TRACE("roiEnable:%d, roi_delta_qp:%d\n",
			    encParam->ctuOptParam.roiEnable, pEncConfig->roi_delta_qp);
		if (encParam->ctuOptParam.roiEnable) {
			vdi_write_memory(coreIdx,
					 encParam->ctuOptParam.addrRoiCtuMap,
					 roiMapBuf, maxCtuNum,
					 encParam->ctuOptParam.mapEndian);
		}
	}
	return TRUE;
}

int cviSmartRoiLevelCfg(int coreIdx, TestEncConfig *pEncConfig,
		       EncParam *pEncParam,  EncOpenParam *pEncOP,
		       PhysicalAddress addrRoiMap, Uint8 *roiMapBuf,
		       int frmNum)
{
	if (pEncConfig->cviEc.bRoiBinValid == 1 &&  pEncOP->smart_ai_en) {
		int map_width, map_height;
		uint8_t *roiBinBuffer;

		map_height = ((pEncOP->picHeight + 63) & ~63) >> 6;
		map_width = ((pEncOP->picWidth + 63) & ~63) >> 6;
		roiBinBuffer = pEncConfig->cviEc.pu8QpMap;
		pEncParam->ctuOptParam.addrRoiCtuMap = addrRoiMap;
		pEncParam->ctuOptParam.mapEndian = VDI_LITTLE_ENDIAN;
		pEncParam->ctuOptParam.mapStride = map_width;
		pEncParam->ctuOptParam.roiEnable = 1;

		if (pEncConfig->cviEc.roideltaqp < 0) {
			CVI_VC_ERR("smart ai roideltaqp cfg err\n");
			return 0;
		}

		if (pEncOP->fg_protect_en) {
			int roi_level = (pEncOP->fg_dealt_qp +
							pEncConfig->cviEc.roideltaqp - 1) /
							pEncConfig->cviEc.roideltaqp;
			int x, y;

			if (pEncOP->picMotionLevel > 230)
				roi_level = 0;
			else if (pEncOP->picMotionLevel > 192 && roi_level > 1)
				roi_level = 1;
			else if (pEncOP->picMotionLevel > 128 && roi_level > 2)
				roi_level = 2;
			else if (pEncOP->picMotionLevel > 64 && roi_level > 3)
				roi_level = 3;

			for (y = 0; y < map_height; y++) {
				for (x = 0; x < map_width; x++) {
					if (!roiBinBuffer[(y * map_width) + x] &&
					   pEncOP->picMotionMap[(y * map_width) + x]) {
						roiBinBuffer[(y * map_width) + x] = roi_level;
					}
				}
			}

		}
#ifdef DUMP_MOTIONMAP
		{
			int i = 0;

			for (i = 0; i < map_height * map_width; i++)
				pEncOP->picMotionMap[i] = 230 * pEncOP->picMotionMap[i];

			kernel_write(pEncConfig->filp, pEncOP->picMotionMap,
				    map_height * map_width, &pEncConfig->pos);
		}
#endif
		pEncParam->ctuOptParam.roiDeltaQp = pEncConfig->cviEc.roideltaqp;
		CVI_VC_TRACE("bRoiBinValid is 1, roideltaqp:%d\n",
			    pEncConfig->cviEc.roideltaqp);
		vdi_write_memory(coreIdx,
				pEncParam->ctuOptParam.addrRoiCtuMap,
				roiBinBuffer, map_height * map_width,
				VDI_LITTLE_ENDIAN);
		pEncConfig->cviEc.bRoiBinValid = 0;
#ifdef DUMP_MOTIONMAP
		{
			int i = 0;

			for (i = 0; i < map_height * map_width; i++)
				roiBinBuffer[i] = 35 * roiBinBuffer[i];

			kernel_write(pEncConfig->filp, roiBinBuffer,
				    map_height * map_width, &pEncConfig->pos);
		}
#endif
	} else if (pEncOP->fg_protect_en) {
		setRoiMapFromMap(coreIdx, pEncConfig,
				pEncOP, addrRoiMap,
				roiMapBuf, pEncParam,
				frmNum);
	}
	return TRUE;
}

int checkParamRestriction(Uint32 productId, TestEncConfig *pEncConfig)
{
	CVI_VC_TRACE("IN, productId = 0x%X\n", productId);

	if ((pEncConfig->compare_type & (1 << MODE_SAVE_ENCODED)) &&
	    pEncConfig->bitstreamFileName[0] == 0) {
		pEncConfig->compare_type &= ~(1 << MODE_SAVE_ENCODED);
		VLOG(ERR, "You want to Save bitstream data. Set the path\n");
	}

	if ((pEncConfig->compare_type & (1 << MODE_COMP_ENCODED)) &&
	    pEncConfig->ref_stream_path[0] == 0) {
		pEncConfig->compare_type &= ~(1 << MODE_COMP_ENCODED);
		VLOG(ERR, "You want to Compare bitstream data. Set the path\n");
	}

	/* CHECK PARAMETERS */
	if (productId == PRODUCT_ID_420L) {
		if (pEncConfig->rotAngle != 0) {
			VLOG(ERR, "WAVE420L Not support rotation option\n");
			return FALSE;
		}
		if (pEncConfig->mirDir != 0) {
			VLOG(ERR, "WAVE420L Not support mirror option\n");
			return FALSE;
		}
		if (pEncConfig->srcFormat3p4b == TRUE) {
			VLOG(ERR,
			     "WAVE420L Not support 3 pixel 4byte format option\n");
			return FALSE;
		}

		if (pEncConfig->ringBufferEnable == TRUE) {
			pEncConfig->ringBufferEnable = FALSE;
			VLOG(ERR, "WAVE420L doesn't support ring-buffer.\n");
		}
	}
	return TRUE;
}

void GenQpMap(TestEncConfig *pEncConfig, EncParam *param,
	     EncOpenParam *pEncOP, Uint8 *QpMapBuf)
{
	Int32 roi_idx, valid_roi_cnt = 0;
	// Int32 blk_addr;
	CVI_U32 row, col;
	BOOL roi_param_change = param->roi_request;
	Int32 maxCtuWidth, maxCtuHeight;

	if (roi_param_change) {

		maxCtuWidth = ((pEncOP->picWidth + 63) & ~63) >> 6;
		maxCtuHeight = ((pEncOP->picHeight + 63) & ~63) >> 6;

		for (roi_idx = 0; roi_idx < VENC_MAX_ROI_NUM; roi_idx++) {

			VpuRect *rect = &pEncConfig->region[roi_idx];

			if (param->roi_enable[roi_idx]) {

				rect->left =
					CLIP3(0, maxCtuWidth,
					      param->roi_rect_x[roi_idx] >> 6);
				rect->top =
					CLIP3(0, maxCtuHeight,
					      param->roi_rect_y[roi_idx] >> 6);
				rect->right = CLIP3(
					0, maxCtuWidth,
					(Int32)(rect->left +
						((param->roi_rect_width[roi_idx] +
						  63) >>  6) - 1));
				rect->bottom = CLIP3(
					0, maxCtuHeight,
					(Int32)(rect->top +
						((param->roi_rect_height[roi_idx] +
						  63) >> 6) - 1));


				valid_roi_cnt++;
			} else {
				continue;
			}
			for (row = rect->top; row <= rect->bottom; row++) {
				for (col = rect->left; col <= rect->right; col++) {
					if (param->roi_qp_mode[roi_idx] == TRUE)
						QpMapBuf[maxCtuWidth * row + col] = param->roi_qp[roi_idx];
				}
			}

		}
		pEncConfig->roi_enable = (valid_roi_cnt > 0) ? 1 : 0;
	}
}

void GenRoiLevelMap(TestEncConfig *pEncConfig, EncParam *param,
		   EncOpenParam *pEncOP, Int32 base_qp, Uint8 *roiCtuMap)
{
	Int32 roi_idx, qp_delta, roi_level, blk_addr, valid_roi_cnt = 0;
	Uint32 roi_map_size;
	Uint32 x, y;

	Int32 mapWidth = pEncOP->picWidth >> 6;
	Int32 mapHeight = pEncOP->picHeight >> 6;
	BOOL roi_param_change = param->roi_request;

	if (roi_param_change || param->roi_base_qp != base_qp) {
		for (roi_idx = 0; roi_idx < 8; roi_idx++) {
			if (param->roi_enable[roi_idx]) {
				if (param->roi_qp_mode[roi_idx] == TRUE) {
					CVI_VC_TRACE("base qp:%d  param->roi_qp[roi_idx]:%d\n",
						    base_qp, param->roi_qp[roi_idx]);
					qp_delta = (base_qp - param->roi_qp[roi_idx]);
				} else {
					qp_delta = abs(param->roi_qp[roi_idx]);
				}

				if (qp_delta >= 0 && qp_delta < 8) {
					pEncConfig->roi_delta_qp = 1;
					roi_level = (qp_delta / pEncConfig->roi_delta_qp);
				}  else if (qp_delta >= 8 && qp_delta < 15) {
					pEncConfig->roi_delta_qp = 2;
					roi_level = (qp_delta / pEncConfig->roi_delta_qp);
				} else if (qp_delta >= 16 && qp_delta < 24) {
					pEncConfig->roi_delta_qp = 3;
					roi_level = (qp_delta / pEncConfig->roi_delta_qp);
				} else if (qp_delta >= 24 && qp_delta < 32) {
					pEncConfig->roi_delta_qp = 4;
					roi_level = (qp_delta / pEncConfig->roi_delta_qp);
				} else if (qp_delta >= 32 && qp_delta < 40) {
					pEncConfig->roi_delta_qp = 5;
					roi_level = (qp_delta / pEncConfig->roi_delta_qp);
				}

				pEncConfig->roiLevel[roi_idx] = roi_level;
			} else {
				continue;
			}
		}
	}
	CVI_VC_TRACE("qp_delta:%d  pEncConfig->roi_delta_qp:%d  roi level:%d\n",
		    qp_delta, pEncConfig->roi_delta_qp, roi_level);

	roi_map_size = mapWidth * mapHeight;

	for (blk_addr = 0; blk_addr < (Int32)roi_map_size; blk_addr++)
		roiCtuMap[blk_addr] = 0;

	if (roi_param_change) {
		for (roi_idx = 0; roi_idx < 8; roi_idx++) {
			VpuRect *rect = &pEncConfig->region[roi_idx];

			if (param->roi_enable[roi_idx]) {
				rect->left = CLIP3(0, mapWidth - 1, param->roi_rect_x[roi_idx] >> 6);

				rect->top = CLIP3(0, mapHeight - 1, param->roi_rect_y[roi_idx] >> 6);

				rect->right = CLIP3(0, mapWidth - 1,
					      (Int32)(rect->left + ((param->roi_rect_width[roi_idx] + 63) >> 6) - 1));

				rect->bottom = CLIP3(0, mapHeight - 1,
					       (Int32)(rect->top + ((param->roi_rect_height[roi_idx] + 63) >> 6) - 1));

				for (y = rect->top; y <= rect->bottom; y++) {
					for (x = rect->left; x <= rect->right; x++)
						roiCtuMap[y * mapWidth + x] = pEncConfig->roiLevel[roi_idx];
				}
				valid_roi_cnt++;

			} else {
				continue;
			}
		}
		pEncConfig->actRegNum = 8;
		pEncConfig->roi_enable = (valid_roi_cnt > 0) ? 1 : 0;
		param->roi_request = FALSE;
	}
	param->roi_base_qp = base_qp;
}

void cviSetCodaRoiBySdk(EncParam *param, EncOpenParam *pEncOP, Int32 base_qp)
{
	Int32 roi_idx, valid_roi_cnt = 0;
	BOOL roi_param_change = param->roi_request;
	if (roi_param_change) {
		Int32 maxMbWidth = (pEncOP->picWidth >> 4) - 1;
		Int32 maxMbHeight = (pEncOP->picHeight >> 4) - 1;
		for (roi_idx = 0; roi_idx < 8; roi_idx++) {
			VpuRect *rect = &param->setROI.region[roi_idx];
			if (param->roi_enable[roi_idx]) {
				rect->left =
					CLIP3(0, maxMbWidth,
					      param->roi_rect_x[roi_idx] >> 4);
				rect->top =
					CLIP3(0, maxMbHeight,
					      param->roi_rect_y[roi_idx] >> 4);
				rect->right = CLIP3(
					0, maxMbWidth,
					(Int32)(rect->left +
						((param->roi_rect_width[roi_idx] +
						  15) >>
						 4) -
						1));
				rect->bottom = CLIP3(
					0, maxMbHeight,
					(Int32)(rect->top +
						((param->roi_rect_height[roi_idx] +
						  15) >>
						 4) -
						1));
				valid_roi_cnt++;
			} else {
				rect->left = rect->top = -1;
				rect->right = rect->bottom = 0;
			}
		}
		param->setROI.number = 8;
		param->coda9RoiEnable = (valid_roi_cnt > 0) ? 1 : 0;
		param->roi_request = FALSE;
	}
	if (roi_param_change || (param->roi_base_qp != base_qp)) {
		if (param->coda9RoiEnable == 0) {
			return;
		}
		param->nonRoiQp = base_qp;
		param->coda9RoiPicAvgQp = base_qp;
		for (roi_idx = 0; roi_idx < 8; roi_idx++) {
			if (param->roi_enable[roi_idx]) {
				param->setROI.qp[roi_idx] =
					(param->roi_qp_mode[roi_idx] == TRUE) ?
						      param->roi_qp[roi_idx] :
						      base_qp +
							param->roi_qp[roi_idx];
			} else {
				param->setROI.qp[roi_idx] = 0;
			}
		}
	}
	param->roi_base_qp = base_qp;
}

int allocateRoiMapBuf(int coreIdx, TestEncConfig *pEncConfig,
		      vpu_buffer_t *vbRoi, int srcFbNum, int ctuNum)
{
	int i;
	if (pEncConfig->roi_enable ||
	    pEncConfig->cviApiMode != API_MODE_DRIVER) {
		// number of roi buffer should be the same as source buffer num.
		for (i = 0; i < srcFbNum; i++) {
			char ionName[MAX_VPU_ION_BUFFER_NAME];

			vbRoi[i].size = ctuNum;
			CVI_VC_MEM("vbRoi[%d].size = 0x%X\n", i, vbRoi[i].size);
			sprintf(ionName, "VENC_%d_H265_RoiMap",
				pEncConfig->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(coreIdx, &vbRoi[i], 0,
						ionName) < 0) {
				CVI_VC_ERR("fail to allocate ROI buffer\n");
				return FALSE;
			}
		}
	}
	return TRUE;
}

#ifdef REDUNDENT_CODE
int openCtuModeMapFile(TestEncConfig *pEncConfig)
{
	if (pEncConfig->ctu_mode_enable) {
		if (pEncConfig->ctumode_file_name[0]) {
			ChangePathStyle(pEncConfig->ctumode_file_name);
			pEncConfig->ctumode_file =
				osal_fopen(pEncConfig->ctumode_file_name, "r");
			if (pEncConfig->ctumode_file == NULL) {
				VLOG(ERR, "fail to open CTU mode file, %s\n",
				     pEncConfig->ctumode_file_name);
				return FALSE;
			}
		}
	}
	return TRUE;
}

int allocateCtuModeMapBuf(int coreIdx, TestEncConfig encConfig,
			  vpu_buffer_t *vbCtuMode, int srcFbNum, int ctuNum)
{
	char ionName[MAX_VPU_ION_BUFFER_NAME];
	int i;

	if (encConfig.ctu_mode_enable) {
		// the number of CTU mode buffer should be the same as source buffer num.
		for (i = 0; i < srcFbNum; i++) {
			vbCtuMode[i].size = ctuNum;

			CVI_VC_MEM("vbCtuMode[%d].size = 0x%X\n", i,
				   vbCtuMode[i].size);
			sprintf(ionName, "VENC_%d_CtuMode",
				encConfig.s32ChnNum);

			if (VDI_ALLOCATE_MEMORY(coreIdx, &vbCtuMode[i], 0,
						ionName) < 0) {
				VLOG(ERR, "fail to allocate CTU mode buffer\n");
				return FALSE;
			}
		}
	}
	return TRUE;
}

int setCtuModeMap(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		  PhysicalAddress addrCtuModeMap, Uint8 *ctuModeMapBuf,
		  int srcFrameWidth, int srcFrameHeight, EncParam *encParam,
		  int maxCtuNum)
{
	int i;
	if (pEncConfig->ctu_mode_enable && encParam->srcEndFlag != 1) {
		int ctuModeNum = 0;
		// sample code to convert CTU coordinate to CTU MODE map.
		osal_memset(&pEncConfig->region[0], 0,
			    sizeof(VpuRect) * MAX_ROI_NUMBER);
		osal_memset(&pEncConfig->ctuMode[0], 0,
			    sizeof(int) * MAX_ROI_NUMBER);
		if (pEncConfig->ctumode_file_name) {
			char lineStr[256] = {
				0,
			};
			int val;

			fgets(lineStr, 256, pEncConfig->ctumode_file);
			if (sscanf(lineStr, "%x\n", &val) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
			if (val != 0xFFFF)
				VLOG(ERR, "can't find the picture delimiter\n");
			// picture index
			fgets(lineStr, 256, pEncConfig->ctumode_file);
			if (sscanf(lineStr, "%d\n", &val) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
			// number of roi regions
			fgets(lineStr, 256, pEncConfig->ctumode_file);
			if (sscanf(lineStr, "%d\n", &ctuModeNum) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);

			for (i = 0; i < ctuModeNum; i++) {
				fgets(lineStr, 256, pEncConfig->ctumode_file);
				if (parseRoiCtuModeParam(
					    lineStr, &pEncConfig->region[i],
					    &pEncConfig->ctuMode[i],
					    srcFrameWidth,
					    srcFrameHeight) == 0) {
					VLOG(ERR,
					     "CFG file error : %s value is not available.\n",
					     pEncConfig->ctumode_file_name);
				}
			}
		}

		encParam->ctuOptParam.addrCtuModeMap = addrCtuModeMap;
		encParam->ctuOptParam.mapEndian = VDI_LITTLE_ENDIAN;
		encParam->ctuOptParam.mapStride =
			((pEncOP->picWidth + 63) & ~63) >> 6;
		encParam->ctuOptParam.ctuModeEnable =
			(ctuModeNum != 0) ? pEncConfig->ctu_mode_enable : 0;

		if (encParam->ctuOptParam.ctuModeEnable) {
			GenRegionToMap(&pEncConfig->region[0],
				       &pEncConfig->ctuMode[0], ctuModeNum,
				       encParam->ctuOptParam.mapStride,
				       ((pEncOP->picHeight + 63) & ~63) >> 6,
				       &ctuModeMapBuf[0]);
			vdi_write_memory(coreIdx,
					 encParam->ctuOptParam.addrCtuModeMap,
					 ctuModeMapBuf, maxCtuNum,
					 encParam->ctuOptParam.mapEndian);
		}
	}
	return TRUE;
}
#endif

int cviSetCtuQpMap(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   PhysicalAddress addrCtuQpMap, Uint8 *ctuQpMapBuf,
		   EncParam *encParam, int maxCtuNum)
{
	int ctbHeight, ctbwidth;

	if (pEncConfig->ctu_qpMap_enable == 1  && encParam->srcEndFlag != 1) {
		ctbHeight = ((pEncOP->picHeight + 63) & ~63) >> 6;
		ctbwidth = ((pEncOP->picWidth + 63) & ~63) >> 6;

		if (ctbHeight * ctbwidth > maxCtuNum) {
			CVI_VC_ERR(
				"ctbHeight = %d, ctbwidth = %d, maxCtuNum = %d\n",
				ctbHeight, ctbwidth, maxCtuNum);
			return FALSE;
		}

		encParam->ctuOptParam.addrCtuQpMap = addrCtuQpMap;
		encParam->ctuOptParam.mapEndian = VDI_LITTLE_ENDIAN;
		encParam->ctuOptParam.mapStride = ctbwidth;
		encParam->ctuOptParam.ctuQpEnable = pEncConfig->ctu_qpMap_enable;

		CVI_VC_CFG(
			"bQpMapValid = %d, mapStride = %d, ctuQpMapBuf = %p\n",
			pEncConfig->cviEc.bQpMapValid,
			encParam->ctuOptParam.mapStride, ctuQpMapBuf);
		CVI_VC_TRACE("addrCtuQpMap = 0x%llX\n", addrCtuQpMap);

		vdi_write_memory(coreIdx,
				encParam->ctuOptParam.addrCtuQpMap,
				ctuQpMapBuf, ctbHeight * ctbwidth,
				encParam->ctuOptParam.mapEndian);

		if (vcodec_mask & CVI_MASK_TRACE) {
			int row, col;
			CVI_VC_TRACE(
				"ctbwidth = %d, ctbHeight = %d, QpMap =%p\n",
				ctbwidth, ctbHeight, ctuQpMapBuf);

			CVI_VC_TRACE("ctuQpMapBuf =\n");
			for (row = 0; row < ctbHeight; row++) {
				for (col = 0; col < ctbwidth; col++)
					CVI_VC_TRACE("%d ", ctuQpMapBuf[ctbwidth * row + col]);

				CVI_VC_TRACE("\n");
			}
		}
	}
	return TRUE;
}

int allocateCtuQpMapBuf(int coreIdx, TestEncConfig *pEncConfig,
			vpu_buffer_t *vbCtuQp, int srcFbNum, int ctuNum)
{
	int i;

	if (pEncConfig->ctu_qpMap_enable) {
		// the number of CTU mode buffer should be the same as source
		// buffer num.

		for (i = 0; i < srcFbNum; i++) {
			char ionName[MAX_VPU_ION_BUFFER_NAME];

			vbCtuQp[i].size = ctuNum;
			CVI_VC_MEM("vbCtuQp[%d].size = 0x%X\n", i,
				   vbCtuQp[i].size);
			sprintf(ionName, "VENC_%d_H265_CtuQpMap",
				pEncConfig->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(coreIdx, &vbCtuQp[i], 0,
						ionName) < 0) {
				CVI_VC_ERR("fail to allocate CTU QP buffer\n");
				return FALSE;
			}
		}
	}
	return TRUE;
}

#ifdef SUPPORT_HOST_RC_PARAM
#ifdef REDUNDENT_CODE
int openHostPicRcFile(TestEncConfig *pEncConfig)
{
	if (pEncConfig->host_pic_rc_enable) {
		if (pEncConfig->host_pic_rc_file_name[0]) {
#ifdef REDUNDENT_CODE
			ChangePathStyle(pEncConfig->host_pic_rc_file_name);
#endif
			pEncConfig->host_pic_rc_file = osal_fopen(
				pEncConfig->host_pic_rc_file_name, "rt");
			if (pEncConfig->host_pic_rc_file == NULL) {
				VLOG(ERR, "fail to open HOST PIC RC file, %s\n",
				     pEncConfig->host_pic_rc_file_name);
				return FALSE;
			}
		}
	}
	return TRUE;
}

int setHostPicRc(TestEncConfig *pEncConfig, EncParam *encParam)
{
	if (pEncConfig->host_pic_rc_enable) {
		if (pEncConfig->host_pic_rc_file_name) {
			char lineStr[256] = {
				0,
			};
			int qp, pic_bit;
			fgets(lineStr, 256, pEncConfig->host_pic_rc_file);
			if (sscanf(lineStr, "%d %d\n", &qp, &pic_bit) != 2)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
			encParam->hostRcPicQp = qp;
			encParam->hostRcTargetPicBit = pic_bit;
		}
	}
	return TRUE;
}
#endif
#endif

#ifdef REDUNDENT_CODE
// Define tokens for parsing scaling list file
const char *MatrixType[SCALING_LIST_SIZE_NUM][SL_NUM_MATRIX] = {
	{ "INTRA4X4_LUMA", "INTRA4X4_CHROMAU", "INTRA4X4_CHROMAV",
	  "INTER4X4_LUMA", "INTER4X4_CHROMAU", "INTER4X4_CHROMAV" },
	{ "INTRA8X8_LUMA", "INTRA8X8_CHROMAU", "INTRA8X8_CHROMAV",
	  "INTER8X8_LUMA", "INTER8X8_CHROMAU", "INTER8X8_CHROMAV" },
	{ "INTRA16X16_LUMA", "INTRA16X16_CHROMAU", "INTRA16X16_CHROMAV",
	  "INTER16X16_LUMA", "INTER16X16_CHROMAU", "INTER16X16_CHROMAV" },
	{ "INTRA32X32_LUMA", "INTRA32X32_CHROMAU_FROM16x16_CHROMAU",
	  "INTRA32X32_CHROMAV_FROM16x16_CHROMAV", "INTER32X32_LUMA",
	  "INTER32X32_CHROMAU_FROM16x16_CHROMAU",
	  "INTER32X32_CHROMAV_FROM16x16_CHROMAV" }
};

const char *MatrixType_DC[SCALING_LIST_SIZE_NUM - 2][SL_NUM_MATRIX] = {
	{ "INTRA16X16_LUMA_DC", "INTRA16X16_CHROMAU_DC",
	  "INTRA16X16_CHROMAV_DC", "INTER16X16_LUMA_DC",
	  "INTER16X16_CHROMAU_DC", "INTER16X16_CHROMAV_DC" },
	{ "INTRA32X32_LUMA_DC", "INTRA32X32_CHROMAU_DC_FROM16x16_CHROMAU",
	  "INTRA32X32_CHROMAV_DC_FROM16x16_CHROMAV", "INTER32X32_LUMA_DC",
	  "INTER32X32_CHROMAU_DC_FROM16x16_CHROMAU",
	  "INTER32X32_CHROMAV_DC_FROM16x16_CHROMAV" },
};

static Uint8 *get_sl_addr(UserScalingList *sl, Uint32 size_id, Uint32 mat_id)
{
	Uint8 *addr = NULL;

	switch (size_id) {
	case SCALING_LIST_4x4:
		addr = sl->s4[mat_id];
		break;
	case SCALING_LIST_8x8:
		addr = sl->s8[mat_id];
		break;
	case SCALING_LIST_16x16:
		addr = sl->s16[mat_id];
		break;
	case SCALING_LIST_32x32:
		addr = sl->s32[mat_id];
		break;
	}
	return addr;
}

int parse_user_scaling_list(UserScalingList *sl, FILE *fp_sl)
{
#define LINE_SIZE (1024)
	const Uint32 scaling_list_size[SCALING_LIST_SIZE_NUM] = { 16, 64, 64,
								  64 };
	char line[LINE_SIZE];
	Uint32 i;
	Uint32 size_id, mat_id, data, num_coef = 0;
	Uint8 *src = NULL;
	Uint8 *ref = NULL;
	char *ret;
	const char *type_str;

	for (size_id = 0; size_id < SCALING_LIST_SIZE_NUM; size_id++) // for 4,
	// 8, 16,
	// 32
	{
		num_coef = scaling_list_size[size_id];

		for (mat_id = 0; mat_id < SL_NUM_MATRIX; mat_id++) // for
		// intra_y,
		// intra_cb,
		// intra_cr,
		// inter_y,
		// inter_cb,
		// inter_cr
		{
			src = get_sl_addr(sl, size_id, mat_id);

			if (size_id == SCALING_LIST_32x32 &&
			    (mat_id % 3)) // derive scaling list of chroma32x32
			// from that of chrom16x16
			{
				ref = get_sl_addr(sl, size_id - 1, mat_id);

				for (i = 0; i < num_coef; i++)
					src[i] = ref[i];
			} else {
				fseek(fp_sl, 0, 0);
				type_str = MatrixType[size_id][mat_id];

				do {
					ret = fgets(line, LINE_SIZE, fp_sl);
					if ((ret == NULL) ||
					    (strstr(line, type_str) == NULL &&
					     feof(fp_sl))) {
						printf("Error: can't read a scaling list matrix(%s)\n",
						       type_str);
						return 0;
					}
				} while (strstr(line, type_str) == NULL);

				// get all coeff
				for (i = 0; i < num_coef; i++) {
					if (fscanf(fp_sl, "%d,", &data) != 1) {
						printf("Error: can't read a scaling list matrix(%s)\n",
						       type_str);
						return 0;
					}
					src[i] = data;
				}

				// get DC coeff for 16, 32
				if (size_id <= SCALING_LIST_8x8) {
					continue;
				}

				fseek(fp_sl, 0, 0);
				type_str = MatrixType_DC[size_id - 2]
							[mat_id];

				do {
					ret = fgets(line, LINE_SIZE,
						    fp_sl);
					if ((ret == NULL) ||
					    (strstr(line, type_str) ==
						     NULL &&
					     feof(fp_sl))) {
						printf("Error: can't read a scaling list matrix(%s)\n",
						       type_str);
						return 0;
					}
				} while (strstr(line, type_str) ==
					 NULL);

				if (fscanf(fp_sl, "%d,", &data) != 1) {
					printf("Error: can't read a scaling list matrix(%s)\n",
					       type_str);
					return 0;
				}

				// overwrite DC value
				src[0] = data;
			}
		} // for matrix id
	} // for size id
	return 1;
}

int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], FILE *fp)
{
	int i, j = 0;
	char lineStr[256] = {
		0,
	};
	for (i = 0; i < 52; i++) {
		if (NULL != fgets(lineStr, 256, fp)) {
			if (sscanf(lineStr, "%d\n", &buf[j++]) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		} else {
			printf("Error: can't read custom_lambda\n");
			return 0;
		}
	}
	for (i = 0; i < 52; i++) {
		if (NULL != fgets(lineStr, 256, fp)) {
			if (sscanf(lineStr, "%d\n", &buf[j++]) != 1)
				VLOG(ERR, "sscanf input is wrong %d\n", __LINE__);
		} else {
			printf("Error: can't read custom_lambda\n");
			return 0;
		}
	}

	return 1;
}

#ifdef PLATFORM_LINUX
struct option *ConvertOptions(struct OptionExt *cnmOpt, Uint32 nItems)
{
	struct option *opt;
	Uint32 i;

	opt = (struct option *)osal_malloc(sizeof(struct option) * nItems);
	if (opt == NULL) {
		return NULL;
	}

	for (i = 0; i < nItems; i++) {
		osal_memcpy((void *)&opt[i], (void *)&cnmOpt[i],
			    sizeof(struct option));
	}

	return opt;
}

int mkdir_recursive(char *path, mode_t omode)
{
	struct stat sb;
	mode_t numask, oumask;
	int first, last, retval;
	char *p;

	p = path;
	oumask = 0;
	retval = 0;
	if (p[0] == '/') /* Skip leading '/'. */
		++p;
	for (first = 1, last = 0; !last; ++p) {
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (p[1] == '\0')
			last = 1;
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those cased by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(0300);
			(void)umask(numask);
			first = 0;
		}
		if (last)
			(void)umask(oumask);
		if (mkdir(path, last ? omode : 0777) <
		    0) {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(path, &sb) < 0) {
					VLOG(INFO, "%s", path);
					retval = 1;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last)
						errno = EEXIST;
					else
						errno = ENOTDIR;
					VLOG(INFO, "%s", path);
					retval = 1;
					break;
				}
			} else {
				VLOG(INFO, "%s", path);
				retval = 1;
				break;
			}
		} else if (1) {
			VLOG(INFO, "%s", path);
			chmod(path, omode);
		}
		if (!last)
			*p = '/';
	}
	if (!first && !last)
		(void)umask(oumask);
	return retval;
}
#endif

BOOL TestMachineSetup(TestMachine *machine)
{
	BOOL success = TRUE;

	osal_init_keyboard();

	InitializeDebugEnv(machine->testEnvOptions);

	return success;
}

void TestMachineCleanUp(TestMachine *machine)
{
	Uint32 i;
	Listener *o;

	for (i = 0; i < machine->numObservers; i++) {
		o = &machine->observers[i];
		o->destruct(o);
	}
	ReleaseDebugEnv();
	osal_close_keyboard();
}

void TestMachineAddListener(TestMachine *machine, Listener observer,
			    void *handle)
{
	Uint32 i = machine->numObservers;

	if (i == MAX_OBSERVERS) {
		return;
	}

	observer.construct(&observer, handle);
	osal_memcpy(&machine->observers[i], (void *)&observer,
		    sizeof(Listener));
	machine->numObservers++;
}

BOOL TestMachineSetData(TestMachine *machine, void *data)
{
	Listener *o = NULL;
	Uint32 i;
	BOOL success = TRUE;

	for (i = 0; i < machine->numObservers; i++) {
		o = &machine->observers[i];
		if (o->update(o, data) == FALSE) {
			success = FALSE;
		}
	}

	return success;
}

static int file_exist(char *path)
{
#ifdef _MSC_VER
	DWORD attributes;
	char temp[4096];
	LPCTSTR lp_path = (LPCTSTR)temp;

	if (path == NULL) {
		return False;
	}

	strcpy(temp, path);

	replace_character(temp, '/', '\\');

	attributes = GetFileAttributes(lp_path);
	return (attributes != (DWORD)-1);
#else
#ifdef PLATFORM_NON_OS
	return 1;
#else
	return !access(path, F_OK);
#endif
#endif
}

BOOL MkDir(char *path)
{
#if defined(PLATFORM_NON_OS) || defined(PLATFORM_QNX)
	/* need to implement */
	return FALSE;
#else
#ifdef _MSC_VER
	char cmd[4096];
#endif
	if (file_exist(path))
		return TRUE;

#ifdef _MSC_VER
	sprintf(cmd, "mkdir %s", path);
	replace_character(cmd, '/', '\\');
	if (system(cmd)) {
		return FALSE;
	}
	return True;
#else
	return mkdir_recursive(path, 0777);
#endif
#endif
}
#endif
