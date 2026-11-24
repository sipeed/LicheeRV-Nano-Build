#include "main_helper.h"

typedef struct _FPGA_YUV_FEEDER_FP_ {
	unsigned char *base;
	int offset;
	int frameNo;
	int size;
} fpgaYuvFp;

BOOL fpgaYuvFeeder_Create(YuvFeederImpl *impl, const char *path, Uint32 packed,
			  Uint32 fbStride, Uint32 fbHeight)
{
	yuvContext *ctx;
	fpgaYuvFp *fp;
	Uint8 *pYuv;

	fp = osal_malloc(sizeof(fpgaYuvFp));
	if (fp == NULL) {
		VLOG(ERR, "%s:%d failed to malloc fpgaYuvFp\n", __func__,
		     __LINE__);
		return FALSE;
	}

#if CFG_MEM
	fp->base = (unsigned char *)dramCfg.pucSrcYuvAddr;
	fp->size = dramCfg.iSrcYuvSize;
#else
	fp->base = (unsigned char *)SRC_YUV_BASE;
	fp->size = SRC_YUV_SIZE;
#endif

	fp->offset = 0;
	fp->frameNo = 0;

	CVI_VC_TRACE("packed = %d, fbStride = %d, fbHeight = %d\n", packed,
		     fbStride, fbHeight);

#if 1
	pYuv = osal_malloc(0x100); // unsigned short
#else
	if (packed == 1)
		pYuv = osal_malloc(fbStride * fbHeight * 3 * 2 * 2); // packed,
			// unsigned
			// short
	else
		pYuv = osal_malloc(fbStride * fbHeight * 3 * 2); // unsigned
			// short
#endif

	if (pYuv == NULL) {
		VLOG(ERR, "%s:%d failed to malloc pYuv\n", __func__,
		     __LINE__);
		return FALSE;
	}

	ctx = (yuvContext *)osal_malloc(sizeof(yuvContext));
	if (ctx == NULL) {
		osal_free(pYuv);
		osal_free(fp);
		return FALSE;
	}

	osal_memset(ctx, 0, sizeof(yuvContext));

	ctx->fp = (osal_file_t *)fp;
	ctx->pYuv = pYuv;
	impl->context = ctx;

	return TRUE;
}

BOOL fpgaYuvFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
			size_t picWidth, size_t picHeight, void *arg)
{
	yuvContext *ctx = (yuvContext *)impl->context;
	Uint8 *pYuv = ctx->pYuv;
	size_t frameSize;
	size_t frameSizeY;
	size_t frameSizeC;
	Int32 bitdepth = 0;
	Int32 yuv3p4b = 0;
	Int32 packedFormat = 0;
#ifndef PLATFORM_NON_OS
	Uint32 outWidth = 0;
	Uint32 outHeight = 0;
#endif
	fpgaYuvFp *fp;

	CVI_VC_TRACE("impl = 0x%lX, ctx = 0x%lX, pYuv = 0x%lX\n", impl, ctx,
		     pYuv);

	CalcYuvSize(fb->format, picWidth, picHeight, fb->cbcrInterleave,
		    &frameSizeY, &frameSizeC, &frameSize, &bitdepth,
		    &packedFormat, &yuv3p4b);

	// Load source one picture image to encode to SDRAM frame buffer.
	fp = (fpgaYuvFp *)ctx->fp;

#if SRC_YUV_CYCLIC
	fp->offset = (fp->frameNo % 10) * frameSize;
#endif

	if (fp->offset + frameSize > fp->size) {
		CVI_VC_ERR("no more source frames to read\n");
		return FALSE;
	}
	CVI_VC_FLOW("bufY = 0x%lX\n", fb->bufY);
	CVI_VC_TRACE(
		"format = %d, endian = %d, frameSizeY = 0x%X, frameSizeC = 0x%X\n",
		fb->format, fb->endian, frameSizeY, frameSizeC);
	CVI_VC_TRACE(
		"fp = 0x%lX, base = 0x%lX, offset = 0x%X, frameSize = 0x%X\n",
		fp, fp->base, fp->offset, frameSize);
#if DIRECT_YUV
	fb->bufY = (PhysicalAddress)(fp->base + fp->offset);
	fb->bufCb = (PhysicalAddress)(fb->bufY + picWidth * picHeight);
	fb->bufCr = (PhysicalAddress)(fb->bufCb + (picWidth * picHeight >> 2));
#else
	osal_memcpy((void *)fb->bufY, (void *)(fp->base + fp->offset),
		    frameSize);
#endif

	fp->frameNo++;
	fp->offset += frameSize;

#ifndef PLATFORM_NON_OS
#ifdef FPGA_YUV_ERROR
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
#endif
#endif
	return TRUE;
}

BOOL fpgaYuvFeeder_Destory(YuvFeederImpl *impl)
{
	yuvContext *ctx = (yuvContext *)impl->context;

	osal_free(ctx->fp);
	osal_free(ctx->pYuv);
	osal_free(ctx);
	return TRUE;
}

BOOL fpgaYuvFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv)
{
	yuvContext *ctx = (yuvContext *)impl->context;
	UNREFERENCED_PARAMETER(cmd);

	ctx->fbStride = yuv.srcStride;
	ctx->cbcrInterleave = yuv.cbcrInterleave;
	ctx->srcPlanar = yuv.srcPlanar;

	return TRUE;
}
