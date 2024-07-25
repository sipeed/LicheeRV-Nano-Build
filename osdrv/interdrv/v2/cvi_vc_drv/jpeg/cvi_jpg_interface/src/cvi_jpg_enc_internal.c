#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif
#include <linux/kthread.h>
#include <linux/cvi_defines.h>
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
#include "cvi_vcom.h"

#define CVI_RC_MIN_I_PROP		1
#define CVI_RC_MDL_UPDATE_TYPE	0
#define Q_CLIP_RANGE			12
#define CVI_RC_DEF_ALPHA		18
#define CVI_RC_DEF_BETA			2
#define STD_JPEG	14

static int JpgEncParamSet_size;
uint32_t maxIQp = 1;
module_param(maxIQp, uint, 0644);
uint32_t minIQp = 100;
module_param(minIQp, uint, 0644);
extern void jpu_set_channel_num(int chnIdx);

static void cviJpeRc_RcKernelInit(stRcInfo *pRcInfo, stRcCfg *pRcCfg);
static int cviJpeRc_RcKernelEstimatePic(stRcInfo *pRcInfo);
static void cviJpeRc_RcKernelUpdatePic(stRcInfo *pRcInfo, int encByte);
static int cvi_jpeg_quality_scaling(int quality);
static int cviJpgGetEnv(char *envVar);
static int cviJpegEncWaitInterrupt(CVIJpgHandle jpgHandle, JpgRet *pRet);
static int cviJpgEncGetOneFrameData(CVIJpgHandle jpgHandle, void *data);

void add_stats(stSlideWinStats *ptSWStats, int stats)
{
	int curr_ptr = ptSWStats->ptrIdx;
	ptSWStats->total -= ptSWStats->stats[curr_ptr];
	ptSWStats->total += stats;
	ptSWStats->stats[curr_ptr] = stats;
	ptSWStats->ptrIdx = (ptSWStats->ptrIdx + 1) % ptSWStats->winSize;
}

int get_stats(stSlideWinStats *ptSWStats)
{
	return ptSWStats->total;
}

// ----------------------------------------------
typedef enum {
	E_BITRATE = 0,
	E_FRAMERATE = 1,
} eRcParam;

// ----------------------------------------------
// main API
void cviJpeRc_Open(stRcInfo *pRcInfo, stRcCfg *pRcCfg)
{
	CVI_JPG_DBG_RC("targetBitrate %dk byte\n", pRcCfg->targetBitrate);
	CVI_JPG_DBG_RC("picAvgBit %d byte\n", pRcInfo->picAvgBit);
	CVI_JPG_DBG_RC("picPelNum %d\n", pRcInfo->picPelNum);
	CVI_JPG_DBG_RC("fps %d\n", pRcInfo->fps);
	CVI_JPG_DBG_RC("minQ %d\n", pRcInfo->minQ);
	CVI_JPG_DBG_RC("maxQ %d\n", pRcInfo->maxQ);
	CVI_JPG_DBG_RC("qClipRange %d\n", pRcInfo->qClipRange);

	if (jpeg_mask & CVI_MASK_RC) {
		stSlideWinStats *ptBitRateStats = &pRcInfo->stBitRateStats;
		int frameRateDiv, frameRateRes;

		frameRateDiv = (pRcCfg->fps >> 16) + 1;
		frameRateRes = pRcCfg->fps & 0xFFFF;

		ptBitRateStats->winSize = frameRateRes / frameRateDiv;
		ptBitRateStats->total = 0;
		ptBitRateStats->ptrIdx = 0;
		memset(ptBitRateStats->stats, 0,
		       pRcCfg->fps * sizeof(ptBitRateStats->stats[0]));
		CVI_JPG_DBG_RC("maxBitrateLimit %dk byte\n",
			       pRcCfg->maxBitrateLimit);
	}

	pRcInfo->cviRcEn = pRcCfg->cviRcEn;
	CVI_JPG_DBG_RC("cviRcEn = %d\n", pRcInfo->cviRcEn);
	if (pRcInfo->cviRcEn) {
		cviJpeRc_RcKernelInit(pRcInfo, pRcCfg);
	}
}

static void cviJpeRc_RcKernelInit(stRcInfo *pRcInfo, stRcCfg *pRcCfg)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelCfg rcKerCfg, *pRcKerCfg = &rcKerCfg;
	int frameRateDiv, frameRateRes;

	pRcInfo->maxQs = cviJpgGetEnv("maxIQp");
	pRcInfo->maxQs = (pRcInfo->maxQs >= 0) ? cvi_jpeg_quality_scaling(pRcInfo->maxQs) : 1;
	pRcInfo->minQs = cviJpgGetEnv("minIQp");
	pRcInfo->minQs = (pRcInfo->minQs >= 0) ? cvi_jpeg_quality_scaling(pRcInfo->minQs) : 100;

	pRcKerCfg->targetBitrate = pRcCfg->targetBitrate * 1000;
	pRcKerCfg->codec = STD_JPEG;

	frameRateDiv = (pRcCfg->fps >> 16) + 1;
	frameRateRes = pRcCfg->fps & 0xFFFF;

	pRcKerCfg->framerate =
		CVI_FLOAT_DIV(INT_TO_CVI_FLOAT(frameRateRes), INT_TO_CVI_FLOAT(frameRateDiv));
	pRcKerCfg->intraPeriod = 1;
	pRcKerCfg->statTime = 2;
	pRcKerCfg->ipQpDelta = 0;
	pRcKerCfg->numOfPixel = pRcCfg->width * pRcCfg->height;
	pRcKerCfg->maxIprop = 100;
	pRcKerCfg->minIprop = CVI_RC_MIN_I_PROP;
	pRcKerCfg->maxIQp = 51;
	pRcKerCfg->minIQp = 1;
	pRcKerCfg->maxQp = pRcKerCfg->maxIQp;
	pRcKerCfg->minQp = pRcKerCfg->minIQp;
	pRcKerCfg->firstFrmstartQp = 1;
	pRcKerCfg->rcMdlUpdatType = CVI_RC_MDL_UPDATE_TYPE;

	CVI_JPG_DBG_CVRC("targetBitrate = %d, codec = %d, framerate = %d, intraPeriod = %d\n",
			pRcKerCfg->targetBitrate, pRcKerCfg->codec, pRcKerCfg->framerate, pRcKerCfg->intraPeriod);
	CVI_JPG_DBG_CVRC("statTime = %d, ipQpDelta = %d, numOfPixel = %d, maxIprop = %d, minIprop = %d\n",
			pRcKerCfg->statTime,
			pRcKerCfg->ipQpDelta,
			pRcKerCfg->numOfPixel,
			pRcKerCfg->maxIprop,
			pRcKerCfg->minIprop);
	CVI_JPG_DBG_CVRC("maxQp = %d, minQp = %d, maxIQp = %d, minIQp = %d\n",
			pRcKerCfg->maxQp,
			pRcKerCfg->minQp,
			pRcKerCfg->maxIQp,
			pRcKerCfg->minIQp);
	CVI_JPG_DBG_CVRC("firstFrmstartQp = %d, rcMdlUpdatType = %d\n",
			pRcKerCfg->firstFrmstartQp,
			pRcKerCfg->rcMdlUpdatType);
	CVI_JPG_DBG_CVRC("minQs = %d, maxQs = %d\n",
			pRcInfo->minQs,
			pRcInfo->maxQs);
	cviRcKernel_init(pRcKerInfo, pRcKerCfg);

	cviRcKernel_setLastPicQpClip(pRcKerInfo, Q_CLIP_RANGE);
	cviRcKernel_setLevelPicQpClip(pRcKerInfo, Q_CLIP_RANGE);
	cviRcKernel_setpPicQpNormalClip(pRcKerInfo, Q_CLIP_RANGE);
	cviRcKernel_setRCModelParam(pRcKerInfo,
			INT_TO_CVI_FLOAT(CVI_RC_DEF_ALPHA),
			INT_TO_CVI_FLOAT(CVI_RC_DEF_BETA),
			0);

	CVI_PRNT("  fn, targetBit, qp, encodedBit\n");
	CVI_PRNT("-------------------------------\n");
}

int cviJpeRc_EstimatePicQs(stRcInfo *pRcInfo)
{
	return cviJpeRc_RcKernelEstimatePic(pRcInfo);
}

static int cviJpeRc_RcKernelEstimatePic(stRcInfo *pRcInfo)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelPicOut *pRcPicOut = &pRcInfo->rcPicOut;
	int qs;

	cviRcKernel_estimatePic(pRcKerInfo, pRcPicOut, 1, pRcInfo->picIdx);

	CVI_JPG_DBG_FLOAT("qp = %d, lambda = %f, targetBit = %d\n",
			pRcPicOut->qp, getFloat(pRcPicOut->lambda), pRcPicOut->targetBit);

	qs = CVI_FLOAT_TO_INT(pRcPicOut->lambda);
	CVI_JPG_DBG_CVRC("qs = %d, minQs = %d, maxQs = %d\n", qs, pRcInfo->minQs, pRcInfo->maxQs);

	if (qs < pRcInfo->minQs) {
		qs = pRcInfo->minQs;
		pRcPicOut->lambda = INT_TO_CVI_FLOAT(qs);
		CVI_JPG_DBG_CVRC("qs = %d, to min\n", qs);
	} else if (qs > pRcInfo->maxQs) {
		qs = pRcInfo->maxQs;
		pRcPicOut->lambda = INT_TO_CVI_FLOAT(qs);
		CVI_JPG_DBG_CVRC("qs = %d, to max\n", qs);
	}

	return qs;
}

void cviJpeRc_UpdatePic(stRcInfo *pRcInfo, int encByte)
{
	CVI_JPG_DBG_RC("(%d)qs %d target %dk, actual %dk\n", pRcInfo->picIdx,
		       pRcInfo->lastPicQ, pRcInfo->picTargetBit / 1000,
		       encByte / 1000);

	if (jpeg_mask & CVI_MASK_RC) {
		add_stats(&pRcInfo->stBitRateStats, encByte);

		if (pRcInfo->picIdx >= (unsigned int)pRcInfo->fps) {
			int currKByteRate =
				get_stats(&pRcInfo->stBitRateStats) / 1000;

			if (currKByteRate > pRcInfo->maxBitrateLimit) {
				CVI_JPG_DBG_RC(
					"[WARNING] BITRATE OVERFLOW %dkbyte (%dk)\n",
					currKByteRate,
					pRcInfo->maxBitrateLimit);
			} else {
				CVI_JPG_DBG_RC("SLIDE_WIN %dkbyte\n",
					       currKByteRate);
			}
		}
	}

	pRcInfo->bitErr = CLIP(
		INT_MIN, INT_MAX,
		((long long)pRcInfo->bitErr + (pRcInfo->picAvgBit - encByte)));
	pRcInfo->picIdx++;

	if (pRcInfo->cviRcEn)
		cviJpeRc_RcKernelUpdatePic(pRcInfo, encByte);
}

static void cviJpeRc_RcKernelUpdatePic(stRcInfo *pRcInfo, int encByte)
{
	stRcKernelInfo *pRcKerInfo = &pRcInfo->rcKerInfo;
	stRcKernelPicOut *pRcPicOut = &pRcInfo->rcPicOut;
	stRcKernelPicIn rcPicIn, *pRcPicIn = &rcPicIn;

	pRcPicIn->encodedQp = INT_TO_CVI_FLOAT(-1);
	pRcPicIn->encodedLambda = pRcPicOut->lambda;
	pRcPicIn->madi = INT_TO_CVI_FLOAT(-1);
	pRcPicIn->encodedBit = encByte << 3;
	pRcPicIn->mse = INT_TO_CVI_FLOAT(-1);
	pRcPicIn->skipRatio = INT_TO_CVI_FLOAT(-1);

	cviRcKernel_updatePic(pRcKerInfo, pRcPicIn, 1);

	if (jpeg_mask & CVI_MASK_CVRC) {
		CVI_PRNT("%4d, %9d, %2d, %11d\n",
				pRcInfo->picIdx, pRcPicOut->targetBit, pRcPicOut->qp, pRcPicIn->encodedBit);
	}
}

#define DCTSIZE2 64

static const unsigned int std_luminance_quant_tbl[DCTSIZE2] = {
	16,  11, 12, 14,  12,  10,  16,	 14,  13, 14,  18,  17,	 16,
	19,  24, 40, 26,  24,  22,  22,	 24,  49, 35,  37,  29,	 40,
	58,  51, 61, 60,  57,  51,  56,	 55,  64, 72,  92,  78,	 64,
	68,  87, 69, 55,  56,  80,  109, 81,  87, 95,  98,  103, 104,
	103, 62, 77, 113, 121, 112, 100, 120, 92, 101, 103, 99,
};

static const unsigned int std_chrominance_quant_tbl[DCTSIZE2] = {
	17, 18, 18, 24, 21, 24, 47, 26, 26, 47, 99, 66, 56, 66, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
};

void cvi_jpeg_add_quant_table(unsigned char *q_table,
			      const unsigned int base_table[DCTSIZE2],
			      int scale_factor, unsigned int force_baseline)
{
	int i = 0;

	for (i = 0; i < DCTSIZE2; i++) {
		unsigned int temp =
			((long)base_table[i] * scale_factor + 50L) / 100L;
		/* limit the values to the valid range */
		if (temp <= 0L)
			temp = 1L;
		if (temp > 32767L)
			temp = 32767L; /* max quantizer needed for 12 bits */
		if (force_baseline && temp > 255L)
			temp = 255L; /* limit to baseline range if requested */
		q_table[i] = (unsigned char)temp;
	}
}

static int cvi_jpeg_quality_scaling(int quality)
/* Convert a user-specified quality rating to a percentage scaling factor
 * for an underlying quantization table, using our recommended scaling curve.
 * The input 'quality' factor should be 0 (terrible) to 100 (very good).
 */
{
	/* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
	if (quality <= 0)
		quality = 1;
	if (quality > 100)
		quality = 100;

	/* The basic table is used as-is (scaling 100) for a quality of 50.
	 * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
	 * note that at Q=100 the scaling is 0, which will cause cvi_jpeg_add_quant_table
	 * to make all the table entries 1 (hence, minimum quantization loss).
	 * Qualities 1..50 are converted to scaling percentage 5000/Q.
	 */
	if (quality < 50)
		quality = 5000 / quality;
	else
		quality = 200 - quality * 2;

	return quality;
}

void cvi_jpgGetQMatrix(int scale_factor, unsigned char *qMatTab0,
		       unsigned char *qMatTab1)
{
	unsigned int force_baseline = 1;
	cvi_jpeg_add_quant_table(qMatTab0, std_luminance_quant_tbl,
				 scale_factor, force_baseline);
	cvi_jpeg_add_quant_table(qMatTab1, std_chrominance_quant_tbl,
				 scale_factor, force_baseline);
}

int LoadYuvImageBurstFormat2(Uint8 *src, unsigned long addrY,
			     unsigned long addrCb, unsigned long addrCr,
			     int picWidth, int picHeight, int stride,
			     int interLeave, int format, int endian, int packed)
{
	int y, nY = 0, nCb = 0, nCr = 0;
	unsigned long addr;
	int size;
	int lumaSize = 0, chromaSize = 0, chromaStride = 0, chromaWidth = 0;
	CVIFRAMEBUF *srcFrameBuffer = (CVIFRAMEBUF *)src;
	if (NULL == srcFrameBuffer) {
		JLOG(ERR, "Invalid input param.\n");
		return 0;
	}

	switch (format) {
	case FORMAT_420:
		nY = picHeight;
		nCb = nCr = picHeight >> 1;
		chromaSize = (picWidth * picHeight) >> 2;
		chromaStride = stride >> 1;
		chromaWidth = picWidth >> 1;
		break;
	case FORMAT_224:
		nY = picHeight;
		nCb = nCr = picHeight >> 1;
		chromaSize = (picWidth * picHeight) >> 1;
		chromaStride = stride;
		chromaWidth = picWidth;
		break;
	case FORMAT_422:
		nY = picHeight;
		nCb = nCr = picHeight;
		chromaSize = (picWidth * picHeight) >> 1;
		chromaStride = stride >> 1;
		chromaWidth = picWidth >> 1;
		break;
	case FORMAT_444:
		nY = picHeight;
		nCb = nCr = picHeight;
		chromaSize = picWidth * picHeight;
		chromaStride = stride;
		chromaWidth = picWidth;
		break;
	case FORMAT_400:
		nY = picHeight;
		nCb = nCr = 0;
		chromaSize = (picWidth * picHeight) >> 2;
		chromaStride = stride >> 1;
		chromaWidth = picWidth >> 1;
		break;
	}

	addr = addrY;

	if (packed) {
		if (packed == PACKED_FORMAT_444)
			picWidth *= 3;
		else
			picWidth *= 2;

		chromaSize = 0;
	}

	lumaSize = picWidth * nY;

	size = lumaSize + chromaSize * 2;

	// for fast write
	if (picWidth == stride) {
		JpuWriteMem(addr, (Uint8 *)(srcFrameBuffer->vbY.virt_addr),
			    lumaSize, endian);

		if (format == FORMAT_400)
			return size;

		if (packed)
			return size;

		if (interLeave) {
			Uint8 t0, t1, t2, t3, t4, t5, t6, t7;
			int i, height, width;
			int stride;
			Uint8 *pTemp;
			Uint8 *dstAddrCb;
			Uint8 *dstAddrCr;

			addr = addrCb;
			stride = chromaStride * 2;

			height = nCb;
			width = chromaWidth * 2;

			dstAddrCb = (Uint8 *)(srcFrameBuffer->vbCb.virt_addr);
			dstAddrCr = (Uint8 *)(srcFrameBuffer->vbCr.virt_addr);

			pTemp = MEM_MALLOC(width + 16);
			if (!pTemp) {
				return 0;
			}

			for (y = 0; y < height; ++y) {
				for (i = 0; i < width; i += 8) {
					t0 = *dstAddrCb++;
					t2 = *dstAddrCb++;
					t4 = *dstAddrCb++;
					t6 = *dstAddrCb++;
					t1 = *dstAddrCr++;
					t3 = *dstAddrCr++;
					t5 = *dstAddrCr++;
					t7 = *dstAddrCr++;

					if (interLeave == CBCR_INTERLEAVE) {
						pTemp[i] = t0;
						pTemp[i + 1] = t1;
						pTemp[i + 2] = t2;
						pTemp[i + 3] = t3;
						pTemp[i + 4] = t4;
						pTemp[i + 5] = t5;
						pTemp[i + 6] = t6;
						pTemp[i + 7] = t7;
					} else {
						pTemp[i] = t1;
						pTemp[i + 1] = t0;
						pTemp[i + 2] = t3;
						pTemp[i + 3] = t2;
						pTemp[i + 4] = t5;
						pTemp[i + 5] = t4;
						pTemp[i + 6] = t7;
						pTemp[i + 7] = t6;
					}
				}
				JpuWriteMem(addr + stride * y,
					    (unsigned char *)pTemp, width,
					    endian);
			}
			MEM_FREE(pTemp);
		} else {
			if (chromaStride == chromaWidth) {
				addr = addrCb;
				JpuWriteMem(
					addr,
					(Uint8 *)(unsigned long)(srcFrameBuffer
									 ->vbCb
									 .virt_addr),
					chromaSize, endian);

				addr = addrCr;
				JpuWriteMem(
					addr,
					(Uint8 *)(unsigned long)(srcFrameBuffer
									 ->vbCr
									 .virt_addr),
					chromaSize, endian);
			} else {
				addr = addrCb;
				for (y = 0; y < nCb; ++y) {
					JpuWriteMem(
						addr + chromaStride * y,
						(Uint8 *)(srcFrameBuffer->vbCb
								  .virt_addr +
							  y * chromaWidth),
						chromaWidth, endian);
				}

				addr = addrCr;
				for (y = 0; y < nCr; ++y) {
					JpuWriteMem(
						addr + chromaStride * y,
						(Uint8 *)(srcFrameBuffer->vbCr
								  .virt_addr +
							  y * chromaWidth),
						chromaWidth, endian);
				}
			}
		}
	} else {
		for (y = 0; y < nY; ++y) {
			JpuWriteMem(addr + stride * y,
				    (Uint8 *)(srcFrameBuffer->vbY.virt_addr +
					      y * picWidth),
				    picWidth, endian);
		}

		if (format == FORMAT_400)
			return size;

		if (packed)
			return size;

		if (interLeave) {
			Uint8 t0, t1, t2, t3, t4, t5, t6, t7;
			int i, width, height, stride;
			Uint8 *pTemp;
			Uint8 *dstAddrCb;
			Uint8 *dstAddrCr;

			addr = addrCb;
			stride = chromaStride * 2;
			height = nCb;
			width = chromaWidth * 2;

			dstAddrCb = (Uint8 *)(srcFrameBuffer->vbCb.virt_addr);
			dstAddrCr = (Uint8 *)(srcFrameBuffer->vbCr.virt_addr);

			pTemp = MEM_MALLOC(width + 16);
			if (!pTemp) {
				return 0;
			}

			// it may be not occur that pic_width in not 8byte
			// alined.
			for (y = 0; y < height; ++y) {
				for (i = 0; i < width; i += 8) {
					t0 = *dstAddrCb++;
					t2 = *dstAddrCb++;
					t4 = *dstAddrCb++;
					t6 = *dstAddrCb++;
					t1 = *dstAddrCr++;
					t3 = *dstAddrCr++;
					t5 = *dstAddrCr++;
					t7 = *dstAddrCr++;

					if (interLeave == CBCR_INTERLEAVE) {
						pTemp[i] = t0;
						pTemp[i + 1] = t1;
						pTemp[i + 2] = t2;
						pTemp[i + 3] = t3;
						pTemp[i + 4] = t4;
						pTemp[i + 5] = t5;
						pTemp[i + 6] = t6;
						pTemp[i + 7] = t7;
					} else {
						pTemp[i] = t1;
						pTemp[i + 1] = t0;
						pTemp[i + 2] = t3;
						pTemp[i + 3] = t2;
						pTemp[i + 4] = t5;
						pTemp[i + 5] = t3;
						pTemp[i + 6] = t7;
						pTemp[i + 7] = t6;
					}
				}
				JpuWriteMem(addr + stride * y,
					    (unsigned char *)pTemp, width,
					    endian);
			}
			MEM_FREE(pTemp);
		} else {
			addr = addrCb;
			for (y = 0; y < nCb; ++y) {
				JpuWriteMem(addr + chromaStride * y,
					    (Uint8 *)(srcFrameBuffer->vbCb
							      .virt_addr +
						      y * chromaWidth),
					    chromaWidth, endian);
			}

			addr = addrCr;
			for (y = 0; y < nCr; ++y) {
				JpuWriteMem(addr + chromaStride * y,
					    (Uint8 *)(srcFrameBuffer->vbCr
							      .virt_addr +
						      y * chromaWidth),
					    chromaWidth, endian);
			}
		}
	}

	return size;
}

int cviJpgEncOpen(CVIJpgHandle *pHandle, CVIEncConfigParam *pConfig)
{
	EncConfigParam *pEncConfig;
	CVIJpgHandle handle;
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = 0;
	JpgEncInfo *pEncInfo = 0;
	JpgEncOpenParam *pEncOP = 0;
	jpu_buffer_t vbStream = { 0 };
	JpgEncInitialInfo initialInfo = { 0 };
	int usePartialMode = 0;
	int partialBufNum = 1;
	int partialHeight = HEIGHT_BUFFER_SIZE;
#ifdef CVI_JPG_USE_ION_MEM
#ifdef BITSTREAM_ION_CACHED_MEM
	int bBsStreamCached = 1;
#else
	int bBsStreamCached = 0;
#endif
#endif
	Uint32 framebufWidth = 0;
	Uint32 framebufHeight = 0;
	Uint32 framebufStride = 0;
	int srcFrameFormat = 0;
	Uint32 framebufFormat = FORMAT_420;

	CVI_JPG_DBG_IF("pHandle = %p\n", pHandle);

	pEncConfig = MEM_MALLOC(sizeof(EncConfigParam));
	pEncOP = MEM_CALLOC(1, sizeof(JpgEncOpenParam));
	if (pEncConfig == NULL || pEncOP == NULL) {
		JLOG(ERR, "no memory for pEncConfig(%p) or pEncOP(%p)\n", pEncConfig, pEncOP);
		return JPG_RET_FAILURE;
	}
	memset(pEncConfig, 0x0, sizeof(EncConfigParam));

	pEncConfig->picWidth = pConfig->picWidth;
	pEncConfig->picHeight = pConfig->picHeight;
	pEncConfig->sourceFormat = (FrameFormat)pConfig->sourceFormat;
	pEncConfig->packedFormat = (PackedOutputFormat)pConfig->packedFormat;
	pEncConfig->chromaInterleave = (CbCrInterLeave)pConfig->chromaInterleave;

	if (jdi_set_enc_task(pConfig->singleEsBuffer, &pConfig->bitstreamBufSize) !=
	    0) {
		MEM_FREE(pEncConfig);
		MEM_FREE(pEncOP);
		return JPG_RET_INVALID_PARAM;
	}

	ret = getJpgEncOpenParamDefault(pEncOP, pEncConfig);
	if (ret == 0) {
		MEM_FREE(pEncConfig);
		MEM_FREE(pEncOP);
		return JPG_RET_INVALID_PARAM;
	}

	if (PACKED_FORMAT_444_RGB == (PackedOutputFormat)pEncOP->packedFormat) {
		pEncOP->rgbPacked = 1;
		pEncOP->packedFormat = PACKED_FORMAT_444;
	}
	pEncOP->bitrate = pConfig->bitrate;
	pEncOP->framerate = pConfig->framerate;
	pEncOP->quality = pConfig->quality;

	/* set bit endian order */
	pEncOP->streamEndian = JPU_STREAM_ENDIAN;
	pEncOP->frameEndian = JPU_FRAME_ENDIAN;
	pEncOP->chromaInterleave = (CbCrInterLeave)pConfig->chromaInterleave;

	/* allocate bitstream buffer */
	// JLOG(INFO, "jdi_allocate_dma_memory\n");

	if (pConfig->src_type == JPEG_MEM_EXTERNAL) {
		if (pConfig->bitstreamBufSize > 0)
			vbStream.size = pConfig->bitstreamBufSize;
		else
			vbStream.size = LARGE_STREAM_BUF_SIZE;
	} else {
		vbStream.size = STREAM_BUF_SIZE;
	}

	CVI_JPG_DBG_MEM("vbStream.size = 0x%x, task num %d\n", vbStream.size,
			jdi_get_task_num());

	if (jdi_use_single_es_buffer() && jdi_get_enc_task_num() > 1) {
		if (jdi_get_allocated_memory(&vbStream, 1) < 0) {
			JLOG(ERR, "fail to allocate bitstream buffer\n");
			ret = JPG_RET_FAILURE;
			goto ERR_ENC_INIT;
		}
	} else {
		if (JDI_ALLOCATE_MEMORY(&vbStream, 1, bBsStreamCached) < 0) {
			JLOG(ERR, "fail to allocate bitstream buffer\n");
			ret = JPG_RET_FAILURE;
			goto ERR_ENC_INIT;
		}
	}

	pEncOP->bitstreamBuffer = vbStream.phys_addr;
	pEncOP->bitstreamBufferSize = vbStream.size;

	/* Open an instance and get initial information for encoding */
	// JLOG(INFO, "JPU_EncOpen\n");
	ret = JPU_EncOpen((JpgEncHandle *)pHandle, pEncOP);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR, "JPU_EncOpen failed Error code is 0x%x\n", ret);
		goto ERR_ENC_INIT;
	}

	handle = *pHandle;
	pJpgInst = *pHandle;
	pJpgInst->type = 2;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;
	// set virtual address mapped of physical address
	pEncInfo->pBitStream = (BYTE *)vbStream.virt_addr;

	/* JPEG marker order */
	memcpy(pEncInfo->jpgMarkerOrder, pConfig->jpgMarkerOrder,
	       JPG_MARKER_ORDER_BUF_SIZE);

	if ((pConfig->rotAngle != 0) || (pConfig->mirDir != 0)) {
		JPU_EncGiveCommand(handle, ENABLE_JPG_ROTATION, 0);
		JPU_EncGiveCommand(handle, ENABLE_JPG_MIRRORING, 0);
		JPU_EncGiveCommand(handle, SET_JPG_ROTATION_ANGLE,
				   &pConfig->rotAngle);
		JPU_EncGiveCommand(handle, SET_JPG_MIRROR_DIRECTION,
				   &pConfig->mirDir);
	}
	JPU_EncGiveCommand(handle, SET_JPG_USE_PARTIAL_MODE, &usePartialMode);
	JPU_EncGiveCommand(handle, SET_JPG_PARTIAL_FRAME_NUM, &partialBufNum);
	JPU_EncGiveCommand(handle, SET_JPG_PARTIAL_LINE_NUM, &partialHeight);
	JPU_EncGiveCommand(handle, SET_JPG_USE_STUFFING_BYTE_FF,
			   &pConfig->bEnStuffByte);

	/* get encode instance info */
	ret = JPU_EncGetInitialInfo(handle, &initialInfo);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR, "JPU_EncGetInitialInfo failed Error code is 0x%x\n",
		     ret);
		goto ERR_ENC_INIT;
	}

	srcFrameFormat = pEncOP->sourceFormat;

	if (pConfig->rotAngle == 90 || pConfig->rotAngle == 270)
		framebufFormat =
			(srcFrameFormat == FORMAT_422) ? FORMAT_224 :
			(srcFrameFormat == FORMAT_224) ? FORMAT_422 :
							       srcFrameFormat;
	else
		framebufFormat = srcFrameFormat;

	if (framebufFormat == FORMAT_420 || framebufFormat == FORMAT_422)
		framebufWidth = (((pEncOP->picWidth + 15) >> 4) << 4);
	else
		framebufWidth = (((pEncOP->picWidth + 7) >> 3) << 3);

	if (framebufFormat == FORMAT_420 || framebufFormat == FORMAT_224)
		framebufHeight = (((pEncOP->picHeight + 15) >> 4) << 4);
	else
		framebufHeight = (((pEncOP->picHeight + 7) >> 3) << 3);

	framebufStride = framebufWidth;

	if (pEncOP->packedFormat >= PACKED_FORMAT_422_YUYV &&
	    pEncOP->packedFormat <= PACKED_FORMAT_422_VYUY) {
		framebufStride = framebufStride * 2;
		framebufFormat = FORMAT_422;
		if (pConfig->rotAngle == 90 || pConfig->rotAngle == 270)
			framebufFormat = FORMAT_224;
	} else if (pEncOP->packedFormat == PACKED_FORMAT_444) {
		framebufStride = framebufStride * 3;
		framebufFormat = FORMAT_444;
	}

#ifdef CVIDEBUG_V
	JLOG(INFO, "framebuffer stride: %d,  width: %d, height = %d\n",
	     framebufStride, framebufWidth, framebufHeight);

	/* Initialize frame buffers for encoding and source frame */
	JLOG(INFO, "AllocateFrameBuffer\n");
#endif

	pEncInfo->framebufStride = framebufStride;
#ifdef VC_DRIVER_TEST
	if (pConfig->src_type != JPEG_MEM_EXTERNAL) {
		Uint32 i = 0;
		Uint32 needFrameBufCount = 1;

		if (!AllocateFrameBuffer(pJpgInst->instIndex, srcFrameFormat,
					 framebufStride, framebufHeight,
					 needFrameBufCount,
					 pEncOP->packedFormat)) {
			JLOG(INFO, "alloc encode instance frame buffer fail\n");
			ret = JPG_RET_FAILURE;
			goto ERR_ENC_INIT;
		}

		for (i = 0; i < needFrameBufCount; ++i) {
			pEncInfo->pFrame[i] =
				GetFrameBuffer(pJpgInst->instIndex, i);
			pEncInfo->frameBuf[i].stride =
				pEncInfo->pFrame[i]->strideY;
			pEncInfo->frameBuf[i].bufY =
				pEncInfo->pFrame[i]->vbY.phys_addr;
			pEncInfo->frameBuf[i].bufCb =
				pEncInfo->pFrame[i]->vbCb.phys_addr;
			pEncInfo->frameBuf[i].bufCr =
				pEncInfo->pFrame[i]->vbCr.phys_addr;
		}
		pEncInfo->framebufStride = framebufStride;
	}
#endif
	pEncInfo->encHeaderMode = pConfig->encHeaderMode;

	if (pEncInfo->openParam.bitrate != 0) {
		stRcInfo *pRcInfo = &pEncInfo->openParam.RcInfo;
		stRcCfg *pRcCfg = &pEncInfo->openParam.RcCfg;

		pRcCfg->fps = pEncInfo->openParam.framerate;
		pRcCfg->height = pEncInfo->openParam.picHeight;
		pRcCfg->width = pEncInfo->openParam.picWidth;
		pRcCfg->targetBitrate = pEncInfo->openParam.bitrate;
		pRcCfg->minQ = 1;
		pRcCfg->maxQ = 1200;
		//pRcCfg->qClipRange = 2;
		pRcCfg->qClipRange = Q_CLIP_RANGE;
		pRcCfg->maxBitrateLimit = 20000; // 24M

		pRcCfg->cviRcEn = 1;

		CVI_JPG_DBG_INFO("cviRcEn = %d\n", pRcCfg->cviRcEn);
		cviJpeRc_Open(pRcInfo, pRcCfg);
	}

	if (pEncConfig != NULL) {
		MEM_FREE(pEncConfig);
	}
	if (pEncOP != NULL) {
		MEM_FREE(pEncOP);
	}

	/* initial param for decoder */
	return JPG_RET_SUCCESS;

ERR_ENC_INIT:
	if (pEncConfig != NULL) {
		MEM_FREE(pEncConfig);
	}
	if (pEncOP != NULL) {
		MEM_FREE(pEncOP);
	}
#ifdef VC_DRIVER_TEST
	if (pEncInfo && (pEncInfo->pFrame)) {
		FreeFrameBuffer(pJpgInst->instIndex);
		pEncInfo->pFrame[0] = 0;
	}
#endif
	if (!(jdi_get_enc_task_num() > 1 && jdi_use_single_es_buffer())) {
		JDI_FREE_MEMORY(&vbStream);
	}
	return ret;
}

static int cviJpgGetEnv(char *envVar)
{
	extern uint32_t cviRcEn;

	if (strcmp(envVar, "maxIQp") == 0)
		return maxIQp;
	if (strcmp(envVar, "minIQp") == 0)
		return minIQp;
	if (strcmp(envVar, "cviRcEn") == 0)
		return cviRcEn;

	return -1;
}

int cviJpgEncClose(CVIJpgHandle handle)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = handle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;
	jpu_buffer_t vbStream = { 0 };

	/* check handle valid */
	ret = CheckJpgInstValidity(handle);
	if (ret != JPG_RET_SUCCESS)
		return ret;

	/* stop encode operator */
	JPU_EncIssueStop(handle);
#ifdef VC_DRIVER_TEST
	/* free frame buffer */
	if (pEncInfo->pFrame[0])
		FreeFrameBuffer(pJpgInst->instIndex);
#endif
	pEncInfo->pFrame[0] = 0;
	pEncInfo->frameBuf[0].bufY = 0;
	pEncInfo->frameBuf[0].bufCb = 0;
	pEncInfo->frameBuf[0].bufCr = 0;
	pEncInfo->frameBuf[0].stride = 0;

	/* free bitstream buffer */
	vbStream.size = pEncInfo->streamBufSize;
	vbStream.phys_addr = pEncInfo->streamBufStartAddr;

	if (!(jdi_get_enc_task_num() > 1 && jdi_use_single_es_buffer())) {
		JDI_FREE_MEMORY(&vbStream);
	}
	memset(&vbStream, 0, sizeof(vbStream));
	pEncInfo->streamBufStartAddr = 0;

	if (pEncInfo->tPthreadId) {
		complete(&pEncInfo->semSendEncCmd);
		kthread_stop(pEncInfo->tPthreadId);
		pEncInfo->tPthreadId = NULL;
	}

	jdi_delete_enc_task();
	/* close handle */
	JPU_EncClose(pJpgInst);
	return JPG_RET_SUCCESS;
}

static int cviJpgEncBufFullSaveStream(JpgEncInfo *pEncInfo)
{
	BYTE *p = MEM_MALLOC(pEncInfo->preStreamLen + pEncInfo->streamBufSize);

	if (p == NULL) {
		JLOG(ERR, "null pointer\n");
		return JPG_RET_FAILURE;
	}

#if (defined CVI_JPG_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
	jdi_invalidate_ion_cache(pEncInfo->streamBufStartAddr,
				 pEncInfo->pBitStream, pEncInfo->streamBufSize);
#endif
	if (pEncInfo->pPreStream) {
		memcpy(p, pEncInfo->pPreStream, pEncInfo->preStreamLen);
		memcpy(p + pEncInfo->preStreamLen, pEncInfo->pBitStream,
		       pEncInfo->streamBufSize);
		MEM_FREE(pEncInfo->pPreStream);
		pEncInfo->pPreStream = NULL;
	} else {
		memcpy(p, pEncInfo->pBitStream, pEncInfo->streamBufSize);
	}

	pEncInfo->pPreStream = p;
	pEncInfo->preStreamLen += pEncInfo->streamBufSize;
	return JPG_RET_SUCCESS;
}

int cviJpgEncSendFrameData(CVIJpgHandle handle, void *data, int srcType)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgEncParamSet encHeaderParam = { 0 };
	JpgEncParam encParam = { 0 };
	FrameBuffer externalBuf;
#ifdef VC_DRIVER_TEST
	int srcFrameIdx = 0;
#endif
	JpgInst *pJpgInst = NULL;
	JpgEncInfo *pEncInfo = NULL;
	stRcInfo *pRcInfo = NULL;

	ret = CheckJpgInstValidity(handle);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
		return ret;
	}
	pJpgInst = handle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;
	pRcInfo = &pEncInfo->openParam.RcInfo;
#ifdef VC_DRIVER_TEST
	/* write data to frame buffer */
	if (0 == srcType) {

		encParam.sourceFrame = &pEncInfo->frameBuf[srcFrameIdx];
	} else if (1 == srcType) {
		LoadYuvImageBurstFormat2(
			(BYTE *)data, pEncInfo->frameBuf[srcFrameIdx].bufY,
			pEncInfo->frameBuf[srcFrameIdx].bufCb,
			pEncInfo->frameBuf[srcFrameIdx].bufCr,
			pEncInfo->picWidth, pEncInfo->picHeight,
			pEncInfo->framebufStride, pEncInfo->chromaInterleave,
			pEncInfo->sourceFormat, pEncInfo->frameEndian,
			pEncInfo->packedFormat);

		encParam.sourceFrame = &pEncInfo->frameBuf[srcFrameIdx];
	} else if (JPEG_MEM_MODULE == srcType) {
#ifdef CVIDEBUG_V
		JLOG(INFO,
		     "Must call cviJpgEncGetInputDataBuf before call this interface.\n");
#endif /* CVIDEBUG_V */

		encParam.sourceFrame = &pEncInfo->frameBuf[srcFrameIdx];
	} else
#endif
	if (srcType == JPEG_MEM_EXTERNAL) {
		CVIFRAMEBUF *pSrcInfo = (CVIFRAMEBUF *)data;
		CVI_JPG_DBG_TRACE("srcType = %d\n", srcType);

		externalBuf.bufY = pSrcInfo->vbY.phys_addr;
		externalBuf.bufCb = pSrcInfo->vbCb.phys_addr;
		externalBuf.bufCr = pSrcInfo->vbCr.phys_addr;
		externalBuf.stride = pSrcInfo->strideY;
		encParam.sourceFrame = &externalBuf;
		pEncInfo->sourceFormat = (FrameFormat)pSrcInfo->format;
		pEncInfo->packedFormat =
			(PackedOutputFormat)pSrcInfo->packedFormat;
		pEncInfo->chromaInterleave =
			(CbCrInterLeave)pSrcInfo->chromaInterleave;

		// Picture size alignment
		if (pEncInfo->sourceFormat == FORMAT_420 ||
		    pEncInfo->sourceFormat == FORMAT_422)
			pEncInfo->alignedWidth =
				((pEncInfo->picWidth + 15) >> 4) << 4;
		else
			pEncInfo->alignedWidth = ((pEncInfo->picWidth + 7) >> 3)
						 << 3;

		if (pEncInfo->sourceFormat == FORMAT_420 ||
		    pEncInfo->sourceFormat == FORMAT_224)
			pEncInfo->alignedHeight =
				((pEncInfo->picHeight + 15) >> 4) << 4;
		else
			pEncInfo->alignedHeight =
				((pEncInfo->picHeight + 7) >> 3) << 3;

		if (pEncInfo->sourceFormat == FORMAT_400) {
			pEncInfo->compInfo[1] = 0;
			pEncInfo->compInfo[2] = 0;
		} else {
			pEncInfo->compInfo[1] = 5;
			pEncInfo->compInfo[2] = 5;
		}

		if (pEncInfo->sourceFormat == FORMAT_400)
			pEncInfo->compNum = 1;
		else
			pEncInfo->compNum = 3;

		if (pEncInfo->sourceFormat == FORMAT_420) {
			pEncInfo->mcuBlockNum = 6;
			pEncInfo->compInfo[0] = 10;
			pEncInfo->busReqNum = 2;
		} else if (pEncInfo->sourceFormat == FORMAT_422) {
			pEncInfo->mcuBlockNum = 4;
			pEncInfo->busReqNum = 3;
			pEncInfo->compInfo[0] = 9;
		} else if (pEncInfo->sourceFormat == FORMAT_224) {
			pEncInfo->mcuBlockNum = 4;
			pEncInfo->busReqNum = 3;
			pEncInfo->compInfo[0] = 6;
		} else if (pEncInfo->sourceFormat == FORMAT_444) {
			pEncInfo->mcuBlockNum = 3;
			pEncInfo->compInfo[0] = 5;
			pEncInfo->busReqNum = 4;
		} else if (pEncInfo->sourceFormat == FORMAT_400) {
			pEncInfo->mcuBlockNum = 1;
			pEncInfo->busReqNum = 4;
			pEncInfo->compInfo[0] = 5;
		}

	} else {
#ifdef CVIDEBUG_V
		JLOG(INFO, "Invalid source buffer format.\n");
#endif /* CVIDEBUG_V */
		ret = JPG_RET_INVALID_PARAM;
		goto ERR_ENC_FEAME;
	}
	// Write picture header
	if (ENC_HEADER_MODE_NORMAL == pEncInfo->encHeaderMode) {
		encHeaderParam.size = STREAM_BUF_SIZE;
		encHeaderParam.headerMode =
			ENC_HEADER_MODE_NORMAL; // Encoder header disable/enable
		// control. Annex:A 1.2.3 item 13
		encHeaderParam.quantMode = JPG_TBL_NORMAL; // JPG_TBL_MERGE  //
			// Merge quantization
			// table.
			// Annex:A 1.2.3 item
			// 7
		encHeaderParam.huffMode = JPG_TBL_NORMAL; // JPG_TBL_MERGE
			// //Merge huffman
			// table.
			// Annex:A 1.2.3 item
			// 6
		encHeaderParam.disableAPPMarker = 0; // Remove APPn. Annex:A
			// item 11

		if (encHeaderParam.headerMode == ENC_HEADER_MODE_NORMAL) {
			// make picture header
			JPU_EncGiveCommand(
				handle, ENC_JPG_GET_HEADER,
				&encHeaderParam); // return exact header size
			// int endHeaderparam.siz;
			JpgEncParamSet_size = encHeaderParam.size;
		}
	}

	if ((pEncInfo->openParam.bitrate != pRcInfo->targetBitrate) ||
	    (pEncInfo->openParam.framerate != pRcInfo->fps)) {
		stRcCfg *pRcCfg = &pEncInfo->openParam.RcCfg;

		pRcCfg->targetBitrate = pEncInfo->openParam.bitrate;
		pRcCfg->fps = pEncInfo->openParam.framerate;

	}

#ifdef CVIDEBUG_V
	JLOG(INFO, "JPU_EncStartOneFrame\n");
#endif /* CVIDEBUG_V */
	if (pEncInfo->openParam.bitrate != 0 ||
	    pEncInfo->openParam.quality != 0) {
		if (pEncInfo->openParam.quality == 0) {
			cvi_jpgGetQMatrix(
				cviJpeRc_EstimatePicQs(
					&pEncInfo->openParam.RcInfo),
				pEncInfo->openParam.qMatTab[DC_TABLE_INDEX0],
				pEncInfo->openParam.qMatTab[AC_TABLE_INDEX0]);
		} else {
			cvi_jpgGetQMatrix(
				cvi_jpeg_quality_scaling(
					pEncInfo->openParam.quality),
				pEncInfo->openParam.qMatTab[DC_TABLE_INDEX0],
				pEncInfo->openParam.qMatTab[AC_TABLE_INDEX0]);
		}
		memcpy(pEncInfo->openParam.qMatTab[DC_TABLE_INDEX1],
		       pEncInfo->openParam.qMatTab[DC_TABLE_INDEX0], 64);
		memcpy(pEncInfo->openParam.qMatTab[AC_TABLE_INDEX1],
		       pEncInfo->openParam.qMatTab[AC_TABLE_INDEX0], 64);
	}

	jpu_set_channel_num(pJpgInst->s32ChnNum);
	ret = JPU_EncStartOneFrame(handle, &encParam);

	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR, "JPU_EncStartOneFrame failed Error code is 0x%x\n",
		     ret);
		goto ERR_ENC_FEAME;
	}

	if (pEncInfo->bIsoSendFrmEn && pEncInfo->tPthreadId) {
		complete(&pEncInfo->semSendEncCmd);
	}
	return JPG_RET_SUCCESS;

ERR_ENC_FEAME:
	return ret;
}
extern wait_queue_head_t tWaitQueue[];

static int cviJpegEncWaitInterrupt(CVIJpgHandle jpgHandle, JpgRet *pRet)
{
	JpgInst *pJpgInst = jpgHandle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;
	JpgRet ret = JPG_RET_SUCCESS;
	int int_reason = 0;

	while (1) {
		int_reason = JPU_WaitInterrupt(JPU_INTERRUPT_TIMEOUT_MS);
		if (-1 == int_reason) {
			ret = JPU_HWReset();
			if (ret < 0) {
				JLOG(ERR, "Error : jpu reset failed\n");
				return JPG_RET_HWRESET_FAILURE;
			}
#ifdef CVIDEBUG_V
			else {
				JLOG(ERR, "info : jpu enc interruption timeout happened and is reset automatically\n");
			}
#endif
			SetJpgPendingInst(pJpgInst);

			return JPG_RET_HWRESET_SUCCESS;
		}

		// Must catch PIC_DONE interrupt before catching EMPTY interrupt
		if (int_reason & (1 << INT_JPU_DONE)) {
			// Do no clear INT_JPU_DONE these will be cleared in
			// JPU_EncGetOutputInfo.
			*pRet = ret;
			wake_up(&tWaitQueue[pJpgInst->s32ChnNum]);
			return JPG_RET_BREAK;
		}

		if (int_reason & (1 << INT_JPU_BIT_BUF_FULL)) {
			JLOG(INFO, "[%d] bitstream buffer full\n", pJpgInst->instIndex);
			ret = cviJpgEncBufFullSaveStream(pEncInfo);
			if (ret != JPG_RET_SUCCESS) {
				JLOG(ERR, "cviJpgEncBufFullSaveStream failed 0x%x\n", ret);
				*pRet = ret;
				return JPG_RET_GOTO;
			}

			JPU_EncUpdateBitstreamBuffer(jpgHandle, pEncInfo->streamBufSize);
			JPU_ClrStatus((1 << INT_JPU_BIT_BUF_FULL));
			JpuWriteReg(MJPEG_INTR_MASK_REG, 0x0);
		}

		// expect this interrupt after stop is enabled.
		if (int_reason & (1 << INT_JPU_BIT_BUF_STOP)) {
			ret = JPU_EncCompleteStop(jpgHandle);
			if (ret != JPG_RET_SUCCESS) {
				JLOG(ERR,
					 "JPU_EncCompleteStop failed Error code is 0x%x\n",
					 ret);
				cviJpgEncFlush((CVIJpgHandle)jpgHandle);
			}

			JPU_ClrStatus((1 << INT_JPU_BIT_BUF_STOP));
			*pRet = ret;
			return JPG_RET_BREAK;
		}

		if (int_reason & (1 << INT_JPU_PARIAL_OVERFLOW)) {
			JLOG(ERR, "ENC::ERROR frame buffer parial_overflow?\n");
			JPU_ClrStatus((1 << INT_JPU_PARIAL_OVERFLOW));
		}
	}

	*pRet = ret;

	return JPG_RET_BREAK;
}

static int cviJpegEncGetFinalStream(JpgEncInfo *pEncInfo, BYTE **virt_addr,
				    unsigned int *size)
{
	pEncInfo->pFinalStream = MEM_KMALLOC(pEncInfo->preStreamLen + *size);

	if (pEncInfo->pFinalStream == NULL) {
		JLOG(ERR, "null pointer\n");
		return JPG_RET_FAILURE;
	}

	memcpy(pEncInfo->pFinalStream, pEncInfo->pPreStream,
	       pEncInfo->preStreamLen);
	memcpy(pEncInfo->pFinalStream + pEncInfo->preStreamLen, *virt_addr,
	       *size);

	*virt_addr = pEncInfo->pFinalStream;

	MEM_FREE(pEncInfo->pPreStream);
	pEncInfo->pPreStream = NULL;

	*size += pEncInfo->preStreamLen;
	pEncInfo->preStreamLen = 0;
	return JPG_RET_SUCCESS;
}

int cviJpgEncGetFrameData(CVIJpgHandle jpgHandle, void *data)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = jpgHandle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

	if (pEncInfo->bIsoSendFrmEn) {
		wait_for_completion(&pEncInfo->semGetStreamCmd);
		memcpy(data, &pEncInfo->tEncBitstreamData, sizeof(jpu_buffer_t));
		return ret;
	}

	ret = cviJpgEncGetOneFrameData(jpgHandle, data);

	return ret;
}

static int cviJpgEncGetOneFrameData(CVIJpgHandle jpgHandle, void *data)
{
	JpgRet ret = JPG_RET_SUCCESS;
	int iRet = 0;
	JpgEncOutputInfo outputInfo = { 0 };
#ifdef CVIDEBUG_V
	JLOG(INFO, "Enter cviJpgEncGetFrameData\n");
#endif /* CVIDEBUG_V */
	jpu_buffer_t *vbStream = NULL;
	JpgInst *pJpgInst = jpgHandle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

	iRet = cviJpegEncWaitInterrupt(jpgHandle, &ret);
	pJpgInst->u64EndTime = jpgGetCurrentTime();
	if (iRet == JPG_RET_GOTO) {
		CVI_JPG_DBG_ERR("cviJpegEncWaitInterrupt, iRet = %d\n", iRet);
		goto GET_ENC_DATA_ERR;
	} else if (iRet == JPG_RET_BREAK) {
	} else {
		CVI_JPG_DBG_ERR("cviJpegEncWaitInterrupt, iRet = %d\n", iRet);
		return iRet;
	}

	/* check handle valid */
	ret = JPU_EncGetOutputInfo(jpgHandle, &outputInfo);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR, "JPU_EncGetOutputInfo failed Error code is 0x%x\n",
		     ret);
		goto GET_ENC_DATA_ERR;
	}

	// store jpeg image
#ifdef __FOR_TEST__
	FILE *fpJpg = fopen("enc.jpg", "wb");
	BYTE *pBuffer = (BYTE *)(outputInfo.bitstreamBuf_virt);
	fwrite(pBuffer, sizeof(BYTE), outputInfo.bitstreamSize, fpJpg);
	fclose(fpJpg);
#endif /* __FOR_TEST__ */

	if (NULL == data) {
		ret = JPG_RET_INVALID_PARAM;
		goto GET_ENC_DATA_ERR;
	}

	vbStream = (jpu_buffer_t *)data;
	vbStream->phys_addr = outputInfo.bitstreamBuffer;
	vbStream->virt_addr = (void *)outputInfo.bitstreamBuf_virt;
	vbStream->size = outputInfo.bitstreamSize;

#if (defined CVI_JPG_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
	jdi_invalidate_ion_cache(vbStream->phys_addr, vbStream->virt_addr,
				 vbStream->size);
#endif
	if (pEncInfo->rgbPacked) {
		unsigned char *pStream = (unsigned char *)(vbStream->virt_addr);
		if ((0x01 == pStream[0x256]) && (0x02 == pStream[0x259]) &&
		    (0x03 == pStream[0x25C]) && (0x01 == pStream[0x264]) &&
		    (0x02 == pStream[0x266]) && (0x03 == pStream[0x268])) {
			pStream[0x256] = 0x52;
			pStream[0x259] = 0x47;
			pStream[0x25C] = 0x42;
			pStream[0x264] = 0x52;
			pStream[0x266] = 0x47;
			pStream[0x268] = 0x42;
		}
	}
	if (pEncInfo->openParam.bitrate != 0)
		cviJpeRc_UpdatePic(&pEncInfo->openParam.RcInfo, vbStream->size);

	if (pEncInfo->preStreamLen) {
		ret = cviJpegEncGetFinalStream(pEncInfo, &vbStream->virt_addr,
					       &vbStream->size);
		if (ret != JPG_RET_SUCCESS) {
			JLOG(ERR, "cviJpegEncGetFinalStream failed 0x%x\n",
			     ret);
			goto GET_ENC_DATA_ERR;
		}
		vbStream->phys_addr = virt_to_phys(vbStream->virt_addr);
		jdi_flush_ion_cache(vbStream->phys_addr, vbStream->virt_addr, vbStream->size);
	}
	jpu_set_channel_num(-1);

#ifdef CVIDEBUG_V
	JLOG(INFO, "Level cviJpgEncGetOneFrameData\n");
#endif /* CVIDEBUG_V */

	// if NOT sharing es buffer, we can unlock to speed-up enc process
	if (!jdi_use_single_es_buffer()) {
		JpgLeaveLock();
	}
	return JPG_RET_SUCCESS;

GET_ENC_DATA_ERR:
	// if NOT sharing es buffer, we can unlock to speed-up enc process
	if (!jdi_use_single_es_buffer()) {
		JpgLeaveLock();
	}
	return ret;
}

int cviJpgEncFlush(CVIJpgHandle jpgHandle)
{
	// These seem to be out of sync because of the thread ,and they are
	// temporarily commented out

	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = (JpgEncHandle)jpgHandle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

	// JLOG(INFO, "Enter cviJpgEncFlush\n");
	// check handle valid
	ret = CheckJpgInstValidity((JpgEncHandle)jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(INFO, "Leave cviJpgEncFlush\n");
		return ret;
	}

	pEncInfo->streamWrPtr = pEncInfo->streamBufStartAddr;
	pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
	JpuWriteReg(MJPEG_BBC_RD_PTR_REG, pEncInfo->streamRdPtr);
	JpuWriteReg(MJPEG_BBC_WR_PTR_REG, pEncInfo->streamWrPtr);
	JpuWriteReg(MJPEG_BBC_STRM_CTRL_REG, 0);

	JPU_SWReset();
	// JLOG(INFO, "Leave cviJpgEncFlush\n");

	return JPG_RET_SUCCESS;
}

int cviJpgEncGetInputDataBuf(CVIJpgHandle jpgHandle, void *data)
{
	JpgRet ret = JPG_RET_SUCCESS;
	JpgInst *pJpgInst = NULL;
	JpgEncInfo *pEncInfo = NULL;
	CVIFRAMEBUF *cviFrameBuf = NULL;

#ifdef CVIDEBUG_V
	JLOG(INFO, "Enter cviJpgEncGetInputDataBuf\n");
#endif /* CVIDEBUG_V */

	/* check handle valid */
	ret = CheckJpgInstValidity((JpgEncHandle)jpgHandle);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(INFO, "Invalid handle at cviJpgEncGetInputDataBuf.\n");
		return ret;
	}
	pJpgInst = (JpgEncHandle)jpgHandle;
	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	if (NULL == pEncInfo->pFrame)
		return JPG_RET_WRONG_CALL_SEQUENCE;
	if (NULL == data)
		return JPG_RET_INVALID_PARAM;

		// store image
#ifdef CVIDEBUG_V
	assert(sizeof(jpu_buffer_t) == sizeof(CVIBUF));
#endif /* CVIDEBUG_V */
	cviFrameBuf = (CVIFRAMEBUF *)data;
	cviFrameBuf->format = (CVIFrameFormat)pEncInfo->sourceFormat;
	memcpy(&(cviFrameBuf->vbY), &(pEncInfo->pFrame[0]->vbY),
	       sizeof(jpu_buffer_t));
	memcpy(&(cviFrameBuf->vbCb), &(pEncInfo->pFrame[0]->vbCb),
	       sizeof(jpu_buffer_t));
	memcpy(&(cviFrameBuf->vbCr), &(pEncInfo->pFrame[0]->vbCr),
	       sizeof(jpu_buffer_t));
	cviFrameBuf->strideY = pEncInfo->pFrame[0]->strideY;
	cviFrameBuf->strideC = pEncInfo->pFrame[0]->strideC;
	if (pEncInfo->chromaInterleave)
		cviFrameBuf->strideC *= 2;

#ifdef CVIDEBUG_V
	JLOG(INFO, "Level cviJpgEncGetInputDataBuf\n");
#endif /* CVIDEBUG_V */

	return JPG_RET_SUCCESS;
}

int cviJpgEncResetQualityTable(CVIJpgHandle jpgHandle)
{
	JpgInst *pJpgInst = jpgHandle;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;
	JpgEncOpenParam *pEncOP;
	EncConfigParam *pEncConfig;

	pEncOP = MEM_MALLOC(sizeof(JpgEncOpenParam));
	pEncConfig = MEM_MALLOC(sizeof(EncConfigParam));
	if (pEncOP == NULL) {
		JLOG(ERR, "no memory for pEncOP\n");
		return JPG_RET_FAILURE;
	}
	if (pEncConfig == NULL) {
		JLOG(ERR, "no memory for pEncConfig\n");
		return JPG_RET_FAILURE;
	}
	memset(pEncOP, 0x0, sizeof(JpgEncOpenParam));
	memset(pEncConfig, 0x0, sizeof(EncConfigParam));

	if (getJpgEncOpenParamDefault(pEncOP, pEncConfig) == 0) {
		MEM_FREE(pEncOP);
		MEM_FREE(pEncConfig);
		return JPG_RET_INVALID_PARAM;
	}

	memcpy(pEncInfo->openParam.huffVal, pEncOP->huffVal, 4 * 162);
	memcpy(pEncInfo->openParam.huffBits, pEncOP->huffBits, 4 * 256);
	memcpy(pEncInfo->openParam.qMatTab, pEncOP->qMatTab, 4 * 64);

	if (pEncOP != NULL) {
		MEM_FREE(pEncOP);
	}
	if (pEncConfig != NULL) {
		MEM_FREE(pEncConfig);
	}
	return JPG_RET_SUCCESS;
}

int cviJpgEncEncodeUserData(CVIJpgHandle jpgHandle, void *data)
{
	unsigned int i;
	JpgRet ret;
	JpgInst *pJpgInst = (JpgInst *)jpgHandle;
	JpgEncInfo *pEncInfo;
	cviJpegUserData *pSrc = (cviJpegUserData *)data;
	BYTE *pDst;

	/* check handle valid */
	ret = CheckJpgInstValidity(pJpgInst);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("Invalid handle at cviJpgEncEncodeUserData\n");
		return ret;
	}

	if (pSrc == NULL || pSrc->userData == NULL || pSrc->len == 0) {
		CVI_JPG_DBG_ERR("no user data\n");
		return JPG_RET_INVALID_PARAM;
	}

	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	// check user data buffer size
	if (pEncInfo->userDataLen + USER_DATA_MARKER_CODE_SIZE +
		    USER_DATA_LENGTH_CODE_SIZE + pSrc->len >
	    USER_DATA_BUF_SIZE) {
		CVI_JPG_DBG_ERR("not enough user data buf left\n");
		return JPG_RET_INVALID_PARAM;
	}

	pDst = pEncInfo->userData + pEncInfo->userDataLen;
	*(pDst++) = (USER_DATA_MARKER >> 8) & 0xFF;
	*(pDst++) = USER_DATA_MARKER & 0xFF;
	*(pDst++) = ((USER_DATA_LENGTH_CODE_SIZE + pSrc->len) >> 8) & 0xFF;
	*(pDst++) = (USER_DATA_LENGTH_CODE_SIZE + pSrc->len) & 0xFF;
	for (i = 0; i < pSrc->len; i++)
		*(pDst++) = pSrc->userData[i];

	pEncInfo->userDataLen += (USER_DATA_MARKER_CODE_SIZE +
				  USER_DATA_LENGTH_CODE_SIZE + pSrc->len);
	return JPG_RET_SUCCESS;
}

static int pfnJpgWaitEncodeDone(void *param)
{
	int ret;
	JpgInst *pJpgInst = (JpgInst *)param;
	JpgEncInfo *pEncInfo = &pJpgInst->JpgInfo.encInfo;

	while (!kthread_should_stop()) {
		// wait for enc cmd trigger
		wait_for_completion(&pEncInfo->semSendEncCmd);
		if (kthread_should_stop() || pEncInfo->streamBufStartAddr == 0) {
			break;
		}
		ret = cviJpgEncGetOneFrameData(param, &pEncInfo->tEncBitstreamData);
		if (ret == JPG_RET_SUCCESS) {
			complete(&pEncInfo->semGetStreamCmd);
		} else {
			CVI_JPG_DBG_ERR("cviJpgEncGetOneFrameData, ret = %d\n", ret);
			return 0;
		}
	}
	return 0;
}

int cviJpgEncStart(CVIJpgHandle jpgHandle, void *data)
{
	JpgRet ret;
	JpgInst *pJpgInst = (JpgInst *)jpgHandle;
	JpgEncInfo *pEncInfo;

	/* check handle valid */
	ret = CheckJpgInstValidity(pJpgInst);
	if (ret != JPG_RET_SUCCESS) {
		CVI_JPG_DBG_ERR("Invalid handle at cviJpgEncStart\n");
		return ret;
	}

	pEncInfo = &pJpgInst->JpgInfo.encInfo;

	if (jdi_use_single_es_buffer()) {
		pEncInfo->bIsoSendFrmEn = false;
	} else {
		pEncInfo->bIsoSendFrmEn = false;//*((bool *)data);
	}

	if (pEncInfo->bIsoSendFrmEn && !pEncInfo->tPthreadId) {
		init_completion(&pEncInfo->semSendEncCmd);
		init_completion(&pEncInfo->semGetStreamCmd);
		pEncInfo->tPthreadId = kthread_run(pfnJpgWaitEncodeDone,
						 (void *)jpgHandle,
						 "chn%dVencJpgWaitEncodeDone", pJpgInst->s32ChnNum);
		if (IS_ERR(pEncInfo->tPthreadId)) {
			CVI_JPG_DBG_ERR("pfnJpgWaitEncodeDone task error!\n");
			return JPG_RET_FAILURE;
		}
	}

	return JPG_RET_SUCCESS;
}
