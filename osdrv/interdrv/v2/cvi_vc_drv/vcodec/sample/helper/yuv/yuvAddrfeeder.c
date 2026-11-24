#include "main_helper.h"

typedef struct _YUV_ADDR_FP_ {
	int frameNo;
} yuvAddrFp;

BOOL yuvAddrFeeder_Create(YuvFeederImpl *impl, const char *path, Uint32 packed,
			  Uint32 fbStride, Uint32 fbHeight)
{
	yuvContext *ctx;
	yuvAddrFp *fp;
	Uint8 *pYuv;

	UNREFERENCED_PARAMETER(path);

	fp = osal_malloc(sizeof(yuvAddrFp));
	if (fp == NULL) {
		VLOG(ERR, "%s:%d failed to malloc yuvAddrFp\n", __func__,
		     __LINE__);
		return FALSE;
	}

	fp->frameNo = 0;

	CVI_VC_TRACE("packed = %d, fbStride = %d, fbHeight = %d\n", packed,
		     fbStride, fbHeight);

	pYuv = osal_malloc(0x100); // unsigned short

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

BOOL yuvAddrFeeder_Feed(YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
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
	Uint32 outWidth = 0;
	Uint32 outHeight = 0;

	yuvAddrFp *fp;
	feederYuvaddr *pYuvAddr = (feederYuvaddr *)arg;

	UNUSED(pYuv);
	CVI_VC_TRACE("impl = 0x%p, ctx = 0x%p, pYuv = 0x%p\n", impl, ctx, pYuv);

	if (!pYuvAddr) {
		CVI_VC_ERR("pYuvAddr = NULL\n");
		return FALSE;
	}
	CalcYuvSize(fb->format, picWidth, picHeight, fb->cbcrInterleave,
		    &frameSizeY, &frameSizeC, &frameSize, &bitdepth,
		    &packedFormat, &yuv3p4b);

	// Load source one picture image to encode to SDRAM frame buffer.
	fp = (yuvAddrFp *)ctx->fp;

	CVI_VC_FLOW("bufY = 0x%llx\n", fb->bufY);
	CVI_VC_TRACE(
		"format = %d, endian = %d, frameSizeY = 0x%zx, frameSizeC = 0x%zx\n",
		fb->format, fb->endian, frameSizeY, frameSizeC);
	CVI_VC_TRACE("fp = 0x%p, frameSize = 0x%zx\n", fp, frameSize);

#if 0
	osal_memcpy((void *)fb->bufY, (void *)(pYuvAddr->addrY), frameSize);
	osal_memcpy((void *)fb->bufCb, (void *)(pYuvAddr->addrCb),
		    frameSize / 4);
	osal_memcpy((void *)fb->bufCr, (void *)(pYuvAddr->addrCr),
		    frameSize / 4);
#endif

	{
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
		LoadYuvImageBurstFormat3(coreIdx, pYuvAddr->addrY,
					 pYuvAddr->addrCb, pYuvAddr->addrCr,
					 outWidth, outHeight, fb,
					 ctx->srcPlanar);
	}

	fp->frameNo++;

	return TRUE;
}

BOOL yuvAddrFeeder_Destory(YuvFeederImpl *impl)
{
	yuvContext *ctx = (yuvContext *)impl->context;

	osal_free(ctx->fp);
	osal_free(ctx->pYuv);
	osal_free(ctx);
	return TRUE;
}

BOOL yuvAddrFeeder_Configure(YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv)
{
	yuvContext *ctx = (yuvContext *)impl->context;
	UNREFERENCED_PARAMETER(cmd);

	ctx->fbStride = yuv.srcStride;
	ctx->cbcrInterleave = yuv.cbcrInterleave;
	ctx->srcPlanar = yuv.srcPlanar;

	return TRUE;
}
