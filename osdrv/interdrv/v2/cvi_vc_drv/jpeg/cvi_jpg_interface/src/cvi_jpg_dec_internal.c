#include "jputypes.h"
#include "jpuapi.h"
#include "regdefine.h"
#include "jpulog.h"
#include "mixer.h"
#include "jpulog.h"
#include "jpuhelper.h"
#include "jpurun.h"
#include "jpuapifunc.h"
#include "cvi_jpg_interface.h"
#include "cvi_jpg_internal.h"

int cviJpgDecOpen(CVIJpgHandle *pHandle, CVIDecConfigParam *pConfig)
{
	JpgRet ret = JPG_RET_SUCCESS;
	CVIJpgHandle handle;
	JpgInst *pJpgInst = 0;
	JpgDecInfo *pDecInfo = 0;
	JpgDecOpenParam decOP = { 0 };
	jpu_buffer_t vbStream = { 0 };
	int usePartialMode = 0;
	int partialBufNum = 1;
	int iHorScaleMode = 0;
	int iVerScaleMode = 0;
	int rotAngle = 0;
	int mirDir = 0;
	int rotEnable = 0;
#ifdef CVI_JPG_USE_ION_MEM
#ifdef BITSTREAM_ION_CACHED_MEM
	int bBsStreamCached = 1;
#else
	int bBsStreamCached = 0;
#endif
#endif

	CVI_JPG_DBG_IF("pHandle = %p\n", pHandle);

	/* Check Param */
	if (pConfig->usePartialMode && pConfig->roiEnable) {
		JLOG(ERR,
		     "Invalid operation mode : partial and ROI mode can not be worked\n");
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_DEC_INIT;
	}

	if (pConfig->dec_buf.packedFormat && pConfig->roiEnable) {
		JLOG(ERR,
		     "Invalid operation mode : packed mode and ROI mode can not be worked\n");
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_DEC_INIT;
	}

	if ((pConfig->iHorScaleMode || pConfig->iVerScaleMode) &&
	    pConfig->roiEnable) {
		JLOG(ERR,
		     "Invalid operation mode : Scaler mode and ROI mode can not be worked\n");
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_DEC_INIT;
	}

	if ((pConfig->rotAngle != 0) && pConfig->roiEnable) {
		JLOG(ERR,
		     "Invalid operation mode : Rotator mode and ROI mode can not be worked\n");
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_DEC_INIT;
	}

	if (pConfig->dec_buf.packedFormat < 0 ||
	    pConfig->dec_buf.packedFormat > 6) {
		JLOG(ERR,
		     "Invalid operation mode : packedFormat must between 0~5\n");
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_DEC_INIT;
	}

	/* set bit endian order */
	decOP.streamEndian = JPU_STREAM_ENDIAN;
	decOP.frameEndian = JPU_FRAME_ENDIAN;

	/* YUV fmt */
	/* [0](PLANAR), [1](YUYV), [2](UYVY), [3](YVYU), [4](VYUY), [5](YUV_444
	 * PACKED), [6](RGB_444 PACKED)
	 */
	if (PACKED_FORMAT_444_RGB ==
	    (PackedOutputFormat)pConfig->dec_buf.packedFormat) {
		decOP.packedFormat = PACKED_FORMAT_444;
	} else {
		decOP.packedFormat =
			(PackedOutputFormat)pConfig->dec_buf.packedFormat;
	}
	/* UV Interleave Mode */
	if (decOP.packedFormat)
		decOP.chromaInterleave = (CbCrInterLeave)CBCR_SEPARATED;
	else
		decOP.chromaInterleave =
			(CbCrInterLeave)pConfig->dec_buf.chromaInterleave;

	/* ROI param */
	decOP.roiEnable = pConfig->roiEnable;
	decOP.roiOffsetX = pConfig->roiOffsetX;
	decOP.roiOffsetY = pConfig->roiOffsetY;
	decOP.roiWidth = pConfig->roiWidth;
	decOP.roiHeight = pConfig->roiHeight;

	/* Frame Partial Mode (DON'T SUPPORT)*/
	// usePartialMode       = pConfig->usePartialMode;
	usePartialMode = 0;
	partialBufNum = 1;

	/* Rotation Angle (0, 90, 180, 270) */
	rotAngle = pConfig->rotAngle;

	/* mirror direction (0-no mirror, 1-vertical, 2-horizontal, 3-both) */
	mirDir = pConfig->mirDir;

	if (0 != rotAngle || 0 != mirDir)
		rotEnable = 1;
	else
		rotEnable = 0;

	/* Scale Mode */
	iHorScaleMode = pConfig->iHorScaleMode;
	iVerScaleMode = pConfig->iVerScaleMode;

	/* allocate bitstream buffer */
	// JLOG(INFO, "jdi_allocate_dma_memory\n");

	CVI_JPG_DBG_FLOW("iDataLen = 0x%X\n", pConfig->iDataLen);
	if (pConfig->iDataLen > STREAM_BUF_SIZE) {
		JLOG(ERR,
		     "The length 0f the input stream is greater than the length of the JPU input buffer!, vbStream addr = 0x%lX\n",
		     vbStream.phys_addr);
		goto ERR_DEC_INIT;
	} else {
		vbStream.size = (pConfig->iDataLen & 0xffffc000) +
				0x4000; // STREAM_BUF_SIZE;
	}

	CVI_JPG_DBG_FLOW("vbStream.size = 0x%x\n", vbStream.size);

	if (JDI_ALLOCATE_MEMORY(&vbStream, 0, bBsStreamCached) < 0) {
		JLOG(ERR, "fail to allocate bitstream buffer\n");
		ret = JPG_RET_FAILURE;
		goto ERR_DEC_INIT;
	}

	decOP.bitstreamBuffer = vbStream.phys_addr;
	decOP.bitstreamBufferSize = vbStream.size;
	// set virtual address mapped of physical address
	decOP.pBitStream = (BYTE *)vbStream.virt_addr;

	decOP.dst_type = pConfig->dst_type;

	if (decOP.dst_type == JPEG_MEM_EXTERNAL) {
		decOP.dst_info.bufY = pConfig->dec_buf.vbY.phys_addr;
		decOP.dst_info.bufCb = pConfig->dec_buf.vbCb.phys_addr;
		decOP.dst_info.bufCr = pConfig->dec_buf.vbCr.phys_addr;
	}

	// Open an instance and get initial information for decoding.
	// JLOG(INFO, "JPU_DecOpen\n");

	CVI_JPG_DBG_FLOW("JPU_DecOpen\n");

	ret = JPU_DecOpen((JpgDecHandle *)pHandle, &decOP);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR, "JPU_DecOpen failed Error code is 0x%x\n", ret);
		goto ERR_DEC_INIT;
	}

	handle = *pHandle;
	pJpgInst = *pHandle;
	pJpgInst->type = 1;
	pDecInfo = &pJpgInst->JpgInfo.decInfo;

	CVI_JPG_DBG_FLOW("initial param for decoder\n");

	/* initial param for decoder */
	ret = JPU_DecGiveCommand(handle, SET_JPG_USE_PARTIAL_MODE,
				 &(usePartialMode));
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_DecGiveCommand[SET_JPG_USE_PARTIAL_MODE] failed Error code is 0x%x\n",
		     ret);
		goto ERR_DEC_INIT;
	}
	ret = JPU_DecGiveCommand(handle, SET_JPG_PARTIAL_FRAME_NUM,
				 &(partialBufNum));
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_DecGiveCommand[SET_JPG_PARTIAL_FRAME_NUM] failed Error code is 0x%x\n",
		     ret);
		goto ERR_DEC_INIT;
	}

	if (rotEnable) {
		JPU_DecGiveCommand(handle, SET_JPG_ROTATION_ANGLE, &(rotAngle));
		JPU_DecGiveCommand(handle, SET_JPG_MIRROR_DIRECTION, &(mirDir));
		// JPU_DecGiveCommand(handle, SET_JPG_ROTATOR_OUTPUT,
		// &(pDecInfo->frameBuf[0]));
		JPU_DecGiveCommand(handle, ENABLE_JPG_ROTATION, 0);
		JPU_DecGiveCommand(handle, ENABLE_JPG_MIRRORING, 0);
	}

	ret = JPU_DecGiveCommand(handle, SET_JPG_SCALE_HOR, &(iHorScaleMode));
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_DecGiveCommand[SET_JPG_SCALE_HOR] failed Error code is 0x%x\n",
		     ret);
		goto ERR_DEC_INIT;
	}
	ret = JPU_DecGiveCommand(handle, SET_JPG_SCALE_VER, &(iVerScaleMode));
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_DecGiveCommand[SET_JPG_SCALE_VER] failed Error code is 0x%x\n",
		     ret);
		goto ERR_DEC_INIT;
	}

	return JPG_RET_SUCCESS;

ERR_DEC_INIT:
#ifdef VC_DRIVER_TEST
	if (0 != pDecInfo->pFrame[0]) {
		FreeFrameBuffer(pJpgInst->instIndex);
		pDecInfo->pFrame[0] = 0;
	}
#endif
	if (vbStream.phys_addr) {
		JDI_FREE_MEMORY(&vbStream);
	}
	return ret;
}

int cviJpgDecClose(CVIJpgHandle handle)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = handle;
	JpgDecInfo *pDecInfo = &pJpgInst->JpgInfo.decInfo;
	jpu_buffer_t vbStream = { 0 };

	/* check handle valid */
	ret = CheckJpgInstValidity(handle);
	if (ret != JPG_RET_SUCCESS) {
		pr_err("cviJpgDecClose   CheckJpgInstValidity failed Error code is 0x%x\n",
		       ret);
		return ret;
	}
	/* stop decode operator */
	JPU_DecIssueStop(handle);
#ifdef VC_DRIVER_TEST
	if (pDecInfo->dst_type != JPEG_MEM_EXTERNAL) {
		/* free frame buffer */
		FreeFrameBuffer(pJpgInst->instIndex);
		pDecInfo->pFrame[0] = 0;
		pDecInfo->frameBuf[0].bufY = 0;
		pDecInfo->frameBuf[0].bufCb = 0;
	}
#endif
	/* free bitstream buffer */
	vbStream.size = pDecInfo->streamBufSize;
	vbStream.phys_addr = pDecInfo->streamBufStartAddr;
	JDI_FREE_MEMORY(&vbStream);
	memset(&vbStream, 0, sizeof(vbStream));

	/* close handle */
	JPU_DecClose(pJpgInst);
	return JPG_RET_SUCCESS;
}

int cviJpgDecSendFrameData(CVIJpgHandle handle, void *data, int length)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgDecParam decParam = { 0 };
	Uint32 needFrameBufCount = 1;
	Uint32 regFrameBufCount = 0;
	Uint32 framebufWidth = 0;
	Uint32 framebufHeight = 0;
	Uint32 framebufStride = 0;
	Uint32 framebufFormat = FORMAT_420;
	int int_reason = 0;
	int streameos = 0;
	BufInfo bufInfo = { 0 };
	JpgDecInitialInfo initialInfo = { 0 };
	JpgInst *pJpgInst = handle;
	JpgDecInfo *pDecInfo = &pJpgInst->JpgInfo.decInfo;
	int partialHeight = 0;
	int partMaxIdx = 0;

	/* check handle valid */
	ret = CheckJpgInstValidity(handle);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
		return ret;
	}

	// cviJpgDecFlush( handle );

	/* send jpeg data to jpu */
	// JLOG(INFO, "cviJpgSendDecFrameData\n");
	bufInfo.buf = data;
	bufInfo.size = length;
	bufInfo.point = 0;
	ret = WriteJpgBsBufHelper(handle, &bufInfo,
				  pDecInfo->streamBufStartAddr,
				  pDecInfo->streamBufEndAddr, 0, 0, &streameos,
				  pDecInfo->streamEndian);
	if (ret != JPG_RET_SUCCESS) {
		pr_err("WriteBsBufHelper failed Error code is 0x%x\n", ret);
		goto SEND_DEC_DATA_ERR;
	}

	if (1 != pDecInfo->initialInfoObtained) {
		ret = JPU_DecGetInitialInfo(handle, &initialInfo);
		if (ret != JPG_RET_SUCCESS) {
			pr_err("JPU_DecGetInitialInfo failed Error code is 0x%x\n",
			       ret);
			return ret;
		}
	}
	if (pDecInfo->usePartial) {
		// disable Rotator, Scaler
		pDecInfo->rotationEnable = 0;
		pDecInfo->mirrorEnable = 0;
		pDecInfo->iHorScaleMode = 0;
		pDecInfo->iVerScaleMode = 0;
		partialHeight = (initialInfo.sourceFormat == FORMAT_420 ||
				 initialInfo.sourceFormat == FORMAT_224) ?
					      16 :
					      8;

		partMaxIdx =
			((initialInfo.picHeight + 15) & ~15) / partialHeight;
		if (partMaxIdx < pDecInfo->bufNum)
			pDecInfo->bufNum = partMaxIdx;
	}

	framebufWidth = ((initialInfo.picWidth + 63) >> 6) << 6; //  64 byte
		//  align for
		//  vpp of
		//  yuv2bgr

	if (initialInfo.sourceFormat == FORMAT_420 ||
	    initialInfo.sourceFormat == FORMAT_224)
		framebufHeight = ((initialInfo.picHeight + 15) >> 4) << 4;
	else
		framebufHeight = ((initialInfo.picHeight + 7) >> 3) << 3;

	if (pDecInfo->roiEnable) {
		framebufWidth = initialInfo.roiFrameWidth;
		framebufHeight = initialInfo.roiFrameHeight;
	}

	// scaler constraint when conformance test is disable
	if (framebufWidth < 128 || framebufHeight < 128) {
		if (pDecInfo->iHorScaleMode || pDecInfo->iVerScaleMode)
			JLOG(WARN,
			     "Invalid operation mode : Not supported resolution with Scaler, width=%d, height=%d\n",
			     framebufWidth, framebufHeight);

		pDecInfo->iHorScaleMode = 0;
		pDecInfo->iVerScaleMode = 0;
	}

	// JLOG(INFO, "* Dec InitialInfo =>\n instance #%d,\n
	// minframeBuffercount: %u\n", instIdx,
	// initialInfo.minFrameBufferCount);
	JLOG(INFO,
	     "picWidth: %u\n picHeight: %u\n roiWidth: %u\n rouHeight: %u\n",
	     initialInfo.picWidth, initialInfo.picHeight,
	     initialInfo.roiFrameWidth, initialInfo.roiFrameHeight);

	if (pDecInfo->usePartial) {
		JLOG(INFO, "Partial Mode Enable\n");
		JLOG(INFO, "Num of Buffer for Partial : %d\n",
		     pDecInfo->bufNum);
		JLOG(INFO, "Num of Line for Partial   : %d\n", partialHeight);
	}

	framebufFormat = initialInfo.sourceFormat;

	framebufWidth >>= pDecInfo->iHorScaleMode;
	framebufHeight >>= pDecInfo->iVerScaleMode;
	if (pDecInfo->iHorScaleMode || pDecInfo->iVerScaleMode) {
		framebufHeight = ((framebufHeight + 1) >> 1) << 1;
		framebufWidth = ((framebufWidth + 1) >> 1) << 1;
	}

	if (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) {
		framebufHeight = ((framebufHeight + 63) >> 6) << 6; // 64 byte
			// align for
			// vpp of
			// yuv2bgr
		framebufStride = framebufHeight;
		framebufHeight = framebufWidth;
		framebufFormat =
			(framebufFormat == FORMAT_422) ? FORMAT_224 :
			(framebufFormat == FORMAT_224) ? FORMAT_422 :
							       framebufFormat;
	} else {
		framebufStride = framebufWidth;
	}

	if (pDecInfo->iHorScaleMode || pDecInfo->iVerScaleMode) {
		framebufStride = ((framebufStride + 15) >> 4) << 4;
	}

	if (pDecInfo->packedFormat >= PACKED_FORMAT_422_YUYV &&
	    pDecInfo->packedFormat <= PACKED_FORMAT_422_VYUY) {
		framebufStride = framebufStride * 2;
		framebufFormat = FORMAT_422;
		if (pDecInfo->rotationAngle == 90 ||
		    pDecInfo->rotationAngle == 270)
			framebufFormat = FORMAT_224;
	} else if (pDecInfo->packedFormat == PACKED_FORMAT_444) {
		framebufStride = framebufStride * 3;
		framebufFormat = FORMAT_444;
	}

	// printf( "framebuffer stride: %d,  width: %d, height = %d,framebuffer
	// format: %d , packed format = %d\n", framebufStride, framebufWidth,
	// framebufHeight, framebufFormat,  pDecInfo->packedFormat);

	// Allocate frame buffer

	regFrameBufCount = initialInfo.minFrameBufferCount;

	if (pDecInfo->usePartial) {
		if (pDecInfo->bufNum > 4)
			pDecInfo->bufNum = 4;

		regFrameBufCount *= pDecInfo->bufNum;
		ret = JPU_DecGiveCommand(handle, SET_JPG_PARTIAL_LINE_NUM,
					 &(partialHeight));
		if (ret != JPG_RET_SUCCESS) {
			JLOG(ERR,
			     "JPU_DecGiveCommand[SET_JPG_PARTIAL_LINE_NUM] failed Error code is 0x%x\n",
			     ret);
			return ret;
		}
	}

	needFrameBufCount = regFrameBufCount;
#ifdef VC_DRIVER_TEST
	if (pDecInfo->dst_type != JPEG_MEM_EXTERNAL) {
		Uint32 i = 0;

		CVI_JPG_DBG_FLOW("AllocateFrameBuffer\n");
		if (!AllocateFrameBuffer(pJpgInst->instIndex, framebufFormat,
					 framebufStride, framebufHeight,
					 needFrameBufCount, 0)) {
			JLOG(INFO, "alloc encode instance frame buffer fail\n");
			// goto ERR_DEC_INIT;
			return -1;
		}

		for (i = 0; i < needFrameBufCount; ++i) {
			pDecInfo->pFrame[i] =
				GetFrameBuffer(pJpgInst->instIndex, i);
			pDecInfo->frameBuf[i].bufY =
				pDecInfo->pFrame[i]->vbY.phys_addr;
			pDecInfo->frameBuf[i].bufCb =
				pDecInfo->pFrame[i]->vbCb.phys_addr;
			if (pDecInfo->chromaInterleave == CBCR_SEPARATED)
				pDecInfo->frameBuf[i].bufCr =
					pDecInfo->pFrame[i]->vbCr.phys_addr;
		}

		if (pDecInfo->rotationEnable == 1 &&
		    pDecInfo->mirrorEnable == 1) {
			JPU_DecGiveCommand(
				handle, SET_JPG_ROTATOR_OUTPUT,
				&(pDecInfo->frameBuf[0])); // bug for rotator
			JPU_DecGiveCommand(handle, SET_JPG_ROTATOR_STRIDE,
					   &framebufStride);
		}

		// Register frame buffers requested by the decoder.
		ret = JPU_DecRegisterFrameBuffer(handle, pDecInfo->frameBuf,
						 regFrameBufCount,
						 framebufStride);
		if (ret != JPG_RET_SUCCESS) {
			pr_err("JPU_DecRegisterFrameBuffer failed Error code is 0x%x\n",
			       ret);
			goto SEND_DEC_DATA_ERR;
		}
	} else
#endif
	{
		pDecInfo->dst_info.stride = framebufStride;
		pDecInfo->stride = framebufStride;
	}

	// Start decoding a frame.
#ifdef CVIDEBUG_V
	JLOG(INFO, "JPU_DecStartOneFrame\n");
#endif /* CVIDEBUG_V */
	// JPU_SWReset();
	ret = JPU_DecStartOneFrame(handle, &decParam);
	jpu_set_channel_num(pJpgInst->s32ChnNum);
	if (ret != JPG_RET_SUCCESS && ret != JPG_RET_EOS) {
		if (ret == JPG_RET_BIT_EMPTY) {
			ret = WriteJpgBsBufHelper(
				handle, &bufInfo, pDecInfo->streamBufStartAddr,
				pDecInfo->streamBufEndAddr, STREAM_FILL_SIZE, 0,
				&streameos, pDecInfo->streamEndian);
			if (ret != JPG_RET_SUCCESS) {
				pr_err("WriteBsBufHelper failed Error code is 0x%x\n",
				       ret);
				goto SEND_DEC_DATA_ERR;
			}
		} else {
			pr_err("JPU_DecStartOneFrame failed Error code is 0x%x\n",
			       ret);
			goto SEND_DEC_DATA_ERR;
		}
	}

	if (ret == JPG_RET_EOS) {
		JLOG(INFO, "Receive jpeg end.\n");
		goto JPU_END_OF_STREAM;
	}

	while (1) {
		int_reason = JPU_WaitInterrupt(JPU_INTERRUPT_TIMEOUT_MS);
		if (-1 == int_reason) {
			ret = JPU_HWReset();
			if (ret < 0) {
				pr_err("Error : jpu reset failed\n");
				return JPG_RET_HWRESET_FAILURE;
			}
#ifdef CVIDEBUG_V
			else {
				pr_err("info : jpu dec interruption timeout happened and is reset automatically\n");
			}
#endif
			SetJpgPendingInst(pJpgInst);
			return JPG_RET_HWRESET_SUCCESS;
		}

		// Must catch PIC_DONE interrupt before catching EMPTY interrupt
		if (int_reason & (1 << INT_JPU_DONE) ||
		    int_reason & (1 << INT_JPU_ERROR)) {
			// Do no clear INT_JPU_DONE and INT_JPU_ERROR interrupt.
			// these will be cleared in JPU_DecGetOutputInfo.
			break;
		}

		if (int_reason & (1 << INT_JPU_BIT_BUF_EMPTY)) {
#ifdef CVIDEBUG_V
			JLOG(ERR, "JPU dec need more data\n");
#endif
			ret = WriteJpgBsBufHelper(
				handle, &bufInfo, pDecInfo->streamBufStartAddr,
				pDecInfo->streamBufEndAddr, STREAM_FILL_SIZE, 0,
				&streameos, pDecInfo->streamEndian);
			if (ret != JPG_RET_SUCCESS) {
				pr_err("WriteBsBufHelper failed Error code is 0x%x\n",
				       ret);
				goto SEND_DEC_DATA_ERR;
			}

			JPU_ClrStatus((1 << INT_JPU_BIT_BUF_EMPTY));
			JpuWriteReg(MJPEG_INTR_MASK_REG, 0x0);
		}

		if (int_reason & (1 << INT_JPU_BIT_BUF_STOP)) {
			ret = JPU_DecCompleteStop(handle);
			if (ret != JPG_RET_SUCCESS) {
				pr_err("JPU_DecCompleteStop failed Error code is 0x%x\n",
				       ret);
				goto SEND_DEC_DATA_ERR;
			}

			JPU_ClrStatus((1 << INT_JPU_BIT_BUF_STOP));
			break;
		}

		if (int_reason & (1 << INT_JPU_PARIAL_OVERFLOW))
			JPU_ClrStatus((1 << INT_JPU_PARIAL_OVERFLOW));
	}

JPU_END_OF_STREAM:
SEND_DEC_DATA_ERR:
	pJpgInst->u64EndTime = jpgGetCurrentTime();
	return JPG_RET_SUCCESS;
}

int cviJpgDecGetFrameData(CVIJpgHandle jpgHandle, void *data)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgDecOutputInfo outputInfo = { 0 };
	// JpgDecParam decParam    = {0};
	// int rotEnable           = 0;
	// int usePartialMode      = 0;
	// int streameos           = 0;
	// BufInfo bufInfo         = {0};
	// int tmp_framesize       = 0;

	JpgInst *pJpgInst = (JpgDecHandle)jpgHandle;
	JpgDecInfo *pDecInfo = &pJpgInst->JpgInfo.decInfo;
	CVIFRAMEBUF *cviFrameBuf = NULL;

	// JLOG(INFO, "Enter cviJpgDecGetFrameData\n");
	/* check handle valid */
	ret = CheckJpgInstValidity((JpgDecHandle)jpgHandle);
	if (ret != JPG_RET_SUCCESS)
		return ret;

	ret = JPU_DecGetOutputInfo((JpgDecHandle)jpgHandle, &outputInfo);

	if (ret != JPG_RET_SUCCESS) {
		pr_err("JPU_DecGetOutputInfo failed Error code is 0x%x\n",
		       ret);
		goto GET_DEC_DATA_ERR;
	}

	if (outputInfo.decodingSuccess == 0)
		pr_err("JPU_DecGetOutputInfo decode fail\n");

#ifdef CVIDEBUG_V
	JLOG(TRACE,
	     "#%d || consumedByte %d || frameStart=0x%x || ecsStart=0x%x || rdPtr=0x%x || wrPtr=0x%x || pos=%d\n",
	     outputInfo.indexFrameDisplay, outputInfo.consumedByte,
	     outputInfo.bytePosFrameStart,
	     outputInfo.bytePosFrameStart + outputInfo.ecsPtr,
	     JpuReadReg(MJPEG_BBC_RD_PTR_REG), JpuReadReg(MJPEG_BBC_WR_PTR_REG),
	     JpuReadReg(MJPEG_BBC_CUR_POS_REG));
#endif

	// store image
	cviFrameBuf = (CVIFRAMEBUF *)data;
	cviFrameBuf->format = (CVIFrameFormat)pDecInfo->format;
	cviFrameBuf->packedFormat = (CVIPackedFormat)pDecInfo->packedFormat;
	cviFrameBuf->chromaInterleave =
		(CVICbCrInterLeave)pDecInfo->chromaInterleave;
	if (pDecInfo->dst_type == JPEG_MEM_EXTERNAL) {
		cviFrameBuf->vbY.phys_addr = pDecInfo->dst_info.bufY;
		cviFrameBuf->strideY = pDecInfo->dst_info.stride;
		cviFrameBuf->width = pDecInfo->picWidth;
		cviFrameBuf->height = pDecInfo->picHeight;

		switch (cviFrameBuf->format) {
		case CVI_FORMAT_400:
			cviFrameBuf->vbCb.phys_addr = 0;
			cviFrameBuf->vbCr.phys_addr = 0;
			cviFrameBuf->strideC = 0;
			break;
		case CVI_FORMAT_422:
			cviFrameBuf->vbCb.phys_addr = pDecInfo->dst_info.bufCb;
			cviFrameBuf->vbCr.phys_addr = pDecInfo->dst_info.bufCr;
			cviFrameBuf->strideC = pDecInfo->dst_info.stride >> 1;
			break;
		case CVI_FORMAT_444:
			cviFrameBuf->vbCb.phys_addr = pDecInfo->dst_info.bufCb;
			cviFrameBuf->vbCr.phys_addr = pDecInfo->dst_info.bufCr;
			cviFrameBuf->strideC = pDecInfo->dst_info.stride;
			break;
		case CVI_FORMAT_420:
		default:
			if (pDecInfo->chromaInterleave) {
				cviFrameBuf->vbCb.phys_addr =
					pDecInfo->dst_info.bufCb;
				cviFrameBuf->vbCr.phys_addr = 0;
				cviFrameBuf->strideC =
					pDecInfo->dst_info.stride;
			} else {
				cviFrameBuf->vbCb.phys_addr =
					pDecInfo->dst_info.bufCb;
				cviFrameBuf->vbCr.phys_addr =
					pDecInfo->dst_info.bufCr;
				cviFrameBuf->strideC =
					pDecInfo->dst_info.stride >> 1;
			}
			break;
		}
	}
#ifdef VC_DRIVER_TEST
	else {
		memcpy(&(cviFrameBuf->vbY), &(pDecInfo->pFrame[0]->vbY),
		       sizeof(jpu_buffer_t));
		memcpy(&(cviFrameBuf->vbCb), &(pDecInfo->pFrame[0]->vbCb),
		       sizeof(jpu_buffer_t));
		memcpy(&(cviFrameBuf->vbCr), &(pDecInfo->pFrame[0]->vbCr),
		       sizeof(jpu_buffer_t));

		if (pDecInfo->rotationAngle == 90 ||
		    pDecInfo->rotationAngle == 270) {
			cviFrameBuf->width =
				outputInfo.decPicHeight; // pDecInfo->picHeight;
			cviFrameBuf->height =
				outputInfo.decPicWidth; // pDecInfo->picWidth;
		} else {
			cviFrameBuf->width = pDecInfo->picWidth;
			cviFrameBuf->height = pDecInfo->picHeight;
			if (pDecInfo->iHorScaleMode ||
			    pDecInfo->iVerScaleMode) {
				cviFrameBuf->width = outputInfo.decPicWidth;
				cviFrameBuf->height = outputInfo.decPicHeight;
			}
		}
		cviFrameBuf->strideY = pDecInfo->pFrame[0]->strideY;
		cviFrameBuf->strideC = pDecInfo->pFrame[0]->strideC;
		if (pDecInfo->chromaInterleave)
			cviFrameBuf->strideC *= 2;
	}
#endif
//#define __FOR_TEST__   1
//#define _USE_PHYSIC_ADDRESS_  1
#ifdef __FOR_TEST__
	FILE *fpYuv = fopen("dst2.yuv", "wb");
	int i, j, k, iDecPicHeight = 0;

	Uint8 *buff = malloc(WIDTH_BUFFER_SIZE);

#ifdef _OUT_I420
	iDecPicHeight = outputInfo.decPicHeight >> 1;
#else // dump I422
	iDecPicHeight = outputInfo.decPicHeight;
#endif
#ifdef _USE_PHYSIC_ADDRESS_
	PhysicalAddress addrY = pDecInfo->frameBuf[0].bufY;
	PhysicalAddress addrCb = pDecInfo->frameBuf[0].bufCb;
	PhysicalAddress addrCr = pDecInfo->frameBuf[0].bufCr;
	for (i = 0; i < outputInfo.decPicHeight; i++) {
		JpuReadMem((addrY + i * cviFrameBuf->strideY), (Uint8 *)(buff),
			   outputInfo.decPicWidth, pDecInfo->frameEndian);
		fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth, fpYuv);
	}

	for (i = 0; i < iDecPicHeight; i++) {
		JpuReadMem((addrCb + i * cviFrameBuf->strideC), (Uint8 *)(buff),
			   outputInfo.decPicWidth >> 1, pDecInfo->frameEndian);
		fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth >> 1, fpYuv);
	}

	for (i = 0; i < iDecPicHeight; i++) {
		JpuReadMem((addrCr + i * cviFrameBuf->strideC), (Uint8 *)(buff),
			   outputInfo.decPicWidth >> 1, pDecInfo->frameEndian);
		fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth >> 1, fpYuv);
	}
#else /* !_USE_PHYSIC_ADDRESS_ */
	Uint8 *addrY_v = (Uint8 *)(pDecInfo->pFrame[0]->vbY.virt_addr);
	Uint8 *addrCb_v = (Uint8 *)(pDecInfo->pFrame[0]->vbCb.virt_addr);
	Uint8 *addrCr_v = (Uint8 *)(pDecInfo->pFrame[0]->vbCr.virt_addr);

	Uint8 *address = addrY_v;
	for (i = 0; i < outputInfo.decPicHeight; i++) {
		memcpy((Uint8 *)(buff), (const void *)address,
		       outputInfo.decPicWidth);
		fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth, fpYuv);
		address = address + cviFrameBuf->strideY;
	}

	address = addrCb_v;
	if (CBCR_SEPARATED != pDecInfo->chromaInterleave) {
		for (i = 0; i < iDecPicHeight; i++) {
			memcpy((Uint8 *)(buff), (Uint8 *)address,
			       outputInfo.decPicWidth);
			fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth,
			       fpYuv);
			address = address + cviFrameBuf->strideC * 2;
		}
	} else {
		for (i = 0; i < iDecPicHeight; i++) {
			memcpy((Uint8 *)(buff), (Uint8 *)address,
			       outputInfo.decPicWidth >> 1);
			fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth >> 1,
			       fpYuv);
			address = address + cviFrameBuf->strideC;
		}
		address = addrCr_v;
		for (i = 0; i < iDecPicHeight; i++) {
			memcpy((Uint8 *)(buff), (Uint8 *)address,
			       outputInfo.decPicWidth >> 1);
			fwrite(buff, sizeof(Uint8), outputInfo.decPicWidth >> 1,
			       fpYuv);
			address = address + cviFrameBuf->strideC;
		}
	}
#endif /* _USE_PHYSIC_ADDRESS_ */

	fclose(fpYuv);
	free(buff);
#endif /* __FOR_TEST__ */

	jpu_set_channel_num(-1);

	JpgLeaveLock();
	// JLOG(INFO, "Level cviJpgDecGetFrameData\n");
	return JPG_RET_SUCCESS;

GET_DEC_DATA_ERR:
	JpgLeaveLock();
	return ret;
}

int cviJpgDecFlush(CVIJpgHandle jpgHandle)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = (JpgDecHandle)jpgHandle;
	JpgDecInfo *pDecInfo = &pJpgInst->JpgInfo.decInfo;

	// JLOG(INFO, "Enter cviJpgDecFlush\n");

	ret = CheckJpgInstValidity((JpgDecHandle)jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(INFO, "Leave cviJpgDecFlush\n");
		return ret;
	}

	pDecInfo->streamWrPtr = pDecInfo->streamBufStartAddr;
	pDecInfo->streamRdPtr = pDecInfo->streamBufStartAddr;
	if (GetJpgPendingInst() == pJpgInst) {
		JpuWriteReg(MJPEG_BBC_RD_PTR_REG, pDecInfo->streamRdPtr);
		JpuWriteReg(MJPEG_BBC_WR_PTR_REG, pDecInfo->streamWrPtr);
		JpuWriteReg(MJPEG_BBC_STRM_CTRL_REG, 0);
	}
	pDecInfo->frameOffset = 0;
	pDecInfo->consumeByte = 0;
	pDecInfo->nextOffset = 0;
	pDecInfo->currOffset = 0;
	pDecInfo->ecsPtr = 0;
	pDecInfo->streamEndflag = 0;
	// pDecInfo->headerSize    = 0;

	JPU_SWReset();
	// JLOG(INFO, "Leave cviJpgDecFlush\n");

	return JPG_RET_SUCCESS;
}
