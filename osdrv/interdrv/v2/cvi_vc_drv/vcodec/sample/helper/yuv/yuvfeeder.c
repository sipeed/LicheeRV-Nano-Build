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

#include "main_helper.h"

extern YuvFeederImpl loaderYuvFeederImpl;

#ifdef REDUNDENT_CODE
BOOL loaderYuvFeeder_Create(YuvFeederImpl *impl, const char *path,
			    Uint32 packed, Uint32 fbStride, Uint32 fbHeight);

BOOL loaderYuvFeeder_Destory(YuvFeederImpl *impl);

BOOL loaderYuvFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
			  size_t picWidth, size_t picHeight, void *arg);

BOOL loaderYuvFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv);

BOOL fpgaYuvFeeder_Create(YuvFeederImpl *impl, const char *path, Uint32 packed,
			  Uint32 fbStride, Uint32 fbHeight);

BOOL fpgaYuvFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
			size_t picWidth, size_t picHeight, void *arg);

BOOL fpgaYuvFeeder_Destory(YuvFeederImpl *impl);

BOOL fpgaYuvFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv);
#endif

BOOL yuvAddrFeeder_Create(YuvFeederImpl *impl, const char *path, Uint32 packed,
			  Uint32 fbStride, Uint32 fbHeight);

BOOL yuvAddrFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
			size_t picWidth, size_t picHeight, void *arg);

BOOL yuvAddrFeeder_Destory(YuvFeederImpl *impl);

BOOL yuvAddrFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv);

static void printYuvInfo(YuvInfo *yuvInfo);

static BOOL yuvYuvFeeder_Create(YuvFeederImpl *impl, const char *path,
				Uint32 packed, Uint32 fbStride, Uint32 fbHeight)
{
	yuvContext *ctx;
	osal_file_t *fp;
	Uint8 *pYuv;

	fp = osal_fopen(path, "rb");
	if (fp == NULL) {
		CVI_VC_ERR("failed to open yuv file: %s\n", path);
		return FALSE;
	}

	if (packed == 1)
		pYuv = osal_malloc((fbStride * fbHeight * 3) << 2); // packed,
			// unsigned
			// short
	else if (packed == 0)
		pYuv = osal_malloc((fbStride * fbHeight * 3) >> 1); // unsigned
			// short
	else
		pYuv = osal_malloc((fbStride * fbHeight * 3) << 1); // unsigned
			// short

	ctx = (yuvContext *)osal_malloc(sizeof(yuvContext));
	if (ctx == NULL) {
		osal_free(pYuv);
		osal_fclose(fp);
		return FALSE;
	}

	osal_memset(ctx, 0, sizeof(yuvContext));

	ctx->fp = fp;
	ctx->pYuv = pYuv;
	impl->context = ctx;

	return TRUE;
}

static BOOL yuvYuvFeeder_Destory(YuvFeederImpl *impl)
{
	yuvContext *ctx = (yuvContext *)impl->context;

	osal_fclose(ctx->fp);
	osal_free(ctx->pYuv);
	osal_free(ctx);
	return TRUE;
}

static BOOL yuvYuvFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx,
			      FrameBuffer *fb, size_t picWidth,
			      size_t picHeight, void *arg)
{
	yuvContext *ctx = (yuvContext *)impl->context;
	Uint8 *pYuv = ctx->pYuv;
	size_t frameSize;
	size_t frameSizeY;
	size_t frameSizeC;
	Int32 bitdepth = 0;
	Int32 yuv3p4b = 0;
	Int32 packedFormat = 0;
	Uint32 outWidth = 0;
	Uint32 outHeight = 0;

	CalcYuvSize(fb->format, picWidth, picHeight, fb->cbcrInterleave,
		    &frameSizeY, &frameSizeC, &frameSize, &bitdepth,
		    &packedFormat, &yuv3p4b);

	// Load source one picture image to encode to SDRAM frame buffer.
	if (!osal_fread(pYuv, 1, frameSize, ctx->fp)) {
		if (osal_feof(ctx->fp) == 0)
			VLOG(ERR,
			     "Yuv Data osal_fread failed file handle is 0x%x\n",
			     ctx->fp);
		return FALSE;
	}

	if (fb->mapType == LINEAR_FRAME_MAP) {
		outWidth = (yuv3p4b && packedFormat == 0) ?
					 (((picWidth + 31) >> 5) << 5) :
					 picWidth;
		outHeight =
			(yuv3p4b) ? (((picHeight + 7) >> 3) << 3) : picHeight;

		if (yuv3p4b && packedFormat) {
			outWidth = ((picWidth * 2) + 2) / 3 * 4;
		} else if (packedFormat) {
			outWidth *= 2; // 8bit packed mode(YUYV) only. (need to
				// add 10bit cases later)
			if (bitdepth != 0) // 10bit packed
				outWidth *= 2;
		}
		LoadYuvImageBurstFormat(coreIdx, pYuv, outWidth, outHeight, fb,
					ctx->srcPlanar);
	} else {
		TiledMapConfig mapConfig;

		osal_memset((void *)&mapConfig, 0x00, sizeof(TiledMapConfig));
		if (arg != NULL) {
			osal_memcpy((void *)&mapConfig, arg,
				    sizeof(TiledMapConfig));
		}

		LoadTiledImageYuvBurst(coreIdx, pYuv, picWidth, picHeight, fb,
				       mapConfig);
	}

	return TRUE;
}

static BOOL yuvYuvFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv)
{
	yuvContext *ctx = (yuvContext *)impl->context;
	UNREFERENCED_PARAMETER(cmd);

	ctx->fbStride = yuv.srcStride;
	ctx->cbcrInterleave = yuv.cbcrInterleave;
	ctx->srcPlanar = yuv.srcPlanar;

	return TRUE;
}

YuvFeederImpl yuvYuvFeederImpl = { NULL, yuvYuvFeeder_Create, yuvYuvFeeder_Feed,
				   yuvYuvFeeder_Destory,
				   yuvYuvFeeder_Configure };

/*lint -esym(438, ap) */
YuvFeeder YuvFeeder_Create(Uint32 type, const char *srcFilePath,
			   YuvInfo yuvInfo)
{
	AbstractYuvFeeder *feeder;
	YuvFeederImpl *impl;
	BOOL success = FALSE;

	CVI_VC_TRACE("type = %d\n", type);
	printYuvInfo(&yuvInfo);

	if (srcFilePath == NULL) {
		CVI_VC_ERR("src path is NULL\n");
		return NULL;
	}

	// Create YuvFeeder for type.
	switch (type) {
	case SOURCE_YUV:
		impl = osal_malloc(sizeof(YuvFeederImpl));
		impl->Create = &yuvYuvFeeder_Create;
		impl->Feed = &yuvYuvFeeder_Feed;
		impl->Destroy = &yuvYuvFeeder_Destory;
		impl->Configure = &yuvYuvFeeder_Configure;

		success = impl->Create(impl, srcFilePath, yuvInfo.packedFormat,
				       yuvInfo.srcStride, yuvInfo.srcHeight);
		if (success == TRUE) {
			impl->Configure(impl, 0, yuvInfo);
		}
		break;
#ifdef REDUNDENT_CODE
	case SOURCE_YUV_WITH_LOADER:
		impl = osal_malloc(sizeof(YuvFeederImpl));
		impl->Create = &loaderYuvFeeder_Create;
		impl->Feed = &loaderYuvFeeder_Feed;
		impl->Destroy = &loaderYuvFeeder_Destory;
		impl->Configure = &loaderYuvFeeder_Configure;

		success = impl->Create(impl, srcFilePath, yuvInfo.packedFormat,
				       yuvInfo.srcStride, yuvInfo.srcHeight);
		if (success == TRUE) {
			impl->Configure(impl, 0, yuvInfo);
		}
		break;
	case SOURCE_YUV_FPGA:
		impl = osal_malloc(sizeof(YuvFeederImpl));
		impl->Create = &fpgaYuvFeeder_Create;
		impl->Feed = &fpgaYuvFeeder_Feed;
		impl->Destroy = &fpgaYuvFeeder_Destory;
		impl->Configure = &fpgaYuvFeeder_Configure;

		success = impl->Create(impl, srcFilePath, yuvInfo.packedFormat,
				       yuvInfo.srcStride, yuvInfo.srcHeight);
		if (success == TRUE) {
			impl->Configure(impl, 0, yuvInfo);
		} else {
			CVI_VC_ERR("impl->Create\n");
		}
		break;
#endif
	case SOURCE_YUV_ADDR:
		impl = osal_malloc(sizeof(YuvFeederImpl));
		impl->Create = &yuvAddrFeeder_Create;
		impl->Feed = &yuvAddrFeeder_Feed;
		impl->Destroy = &yuvAddrFeeder_Destory;
		impl->Configure = &yuvAddrFeeder_Configure;

		success = impl->Create(impl, srcFilePath, yuvInfo.packedFormat,
				       yuvInfo.srcStride, yuvInfo.srcHeight);
		if (success == TRUE) {
			impl->Configure(impl, 0, yuvInfo);
		} else {
			CVI_VC_ERR("impl->Create\n");
		}
		break;
	default:
		CVI_VC_ERR("Unknown YuvFeeder Type\n");
		success = FALSE;
		break;
	}

	if (success == FALSE)
		return NULL;

	feeder = (AbstractYuvFeeder *)osal_malloc(sizeof(AbstractYuvFeeder));
	feeder->impl = impl;

	CVI_VC_TRACE("feeder = 0x%p, impl = 0x%p\n", feeder, impl);
	return feeder;
}

static void printYuvInfo(YuvInfo *yuvInfo)
{
	CVI_VC_TRACE("cbcrInterleave = %d\n", yuvInfo->cbcrInterleave);
	CVI_VC_TRACE("nv21 = %d\n", yuvInfo->nv21);
	CVI_VC_TRACE("packedFormat = %d\n", yuvInfo->packedFormat);
	CVI_VC_TRACE("srcFormat = %d\n", yuvInfo->srcFormat);
	CVI_VC_TRACE("srcPlanar = %d\n", yuvInfo->srcPlanar);
	CVI_VC_TRACE("srcStride = %d\n", yuvInfo->srcStride);
	CVI_VC_TRACE("srcHeight = %d\n", yuvInfo->srcHeight);
}

/*lint +esym(438, ap) */

BOOL YuvFeeder_Destroy(YuvFeeder feeder)
{
	YuvFeederImpl *impl = NULL;
	AbstractYuvFeeder *yuvfeeder = (AbstractYuvFeeder *)feeder;

	if (yuvfeeder == NULL) {
		CVI_VC_ERR("Invalid handle\n");
		return FALSE;
	}

	impl = yuvfeeder->impl;

	impl->Destroy(impl);
	osal_free(impl);
	return TRUE;
}

BOOL YuvFeeder_Feed(YuvFeeder feeder, Uint32 coreIdx, FrameBuffer *fb,
		    size_t picWidth, size_t picHeight, void *arg)
{
	YuvFeederImpl *impl = NULL;
	AbstractYuvFeeder *absFeeder = (AbstractYuvFeeder *)feeder;
	Int32 ret;

	CVI_VC_TRACE("feeder = 0x%p, absFeeder = 0x%p fb=%p\n", feeder,
		     absFeeder, fb);
	if (absFeeder == NULL) {
		CVI_VC_ERR("Invalid handle\n");
		return FALSE;
	}

	impl = absFeeder->impl;
	CVI_VC_TRACE("impl = 0x%p impl->Feed=%p\n", impl, impl->Feed);

	ret = impl->Feed(impl, coreIdx, fb, picWidth, picHeight, arg);
	if (ret == TRUE)
		fb->srcBufState = SRC_BUFFER_SRC_LOADED;
	return ret;
}
