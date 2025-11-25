#include <linux/slab.h>
#include <linux/version.h>
#include "../jpuapi/regdefine.h"
#include "../jpuapi/jpuapi.h"
#include "../jpuapi/jpuapifunc.h"
#include "../jpuapi/jputable.h"
#include "../include/jpulog.h"
#include "../include/jpuhelper.h"

//#define INCREASE_Q_MATRIX

static int FillSdramBurst(BufInfo *pBufInfo, PhysicalAddress targetAddr,
			  PhysicalAddress bsBufStartAddr,
			  PhysicalAddress bsBufEndAddr, Uint32 size,
			  int checkeos, int *streameos, int endian);
#ifdef REDUNDENT_CODE
static int StoreYuvImageBurstFormat(Uint8 *dst, int picWidth, int picHeight,
				    unsigned long addrY, unsigned long addrCb,
				    unsigned long addrCr, int stride,
				    int interLeave, int format, int endian,
				    int packed);
static void ProcessEncodedBitstreamBurst(FILE *fp, unsigned long targetAddr,
					 PhysicalAddress bsBufStartAddr,
					 PhysicalAddress bsBufEndAddr, int size,
					 int endian);
#endif

// Figure A.6 - Zig-zag sequence of quantized DCT coefficients
const int InvScanTable[64] = { 0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,
			       13, 16, 26, 29, 42, 3,  8,  12, 17, 25, 30,
			       41, 43, 9,  11, 18, 24, 31, 40, 44, 53, 10,
			       19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38,
			       46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59,
			       61, 35, 36, 48, 49, 57, 58, 62, 63 };

const int ScanTable[64] = { 0,	1,  8,	16, 9,	2,  3,	10, 17, 24, 32, 25, 18,
			    11, 4,  5,	12, 19, 26, 33, 40, 48, 41, 34, 27, 20,
			    13, 6,  7,	14, 21, 28, 35, 42, 49, 56, 57, 50, 43,
			    36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45,
			    38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };

JpgRet WriteJpgBsBufHelper(JpgDecHandle handle, BufInfo *pBufInfo,
			   PhysicalAddress paBsBufStart,
			   PhysicalAddress paBsBufEnd, int defaultsize,
			   int checkeos, int *pstreameos, int endian)
{
	JpgRet ret = JPG_RET_SUCCESS;
	int size = 0;
	int fillSize = 0;
	PhysicalAddress paRdPtr, paWrPtr;

	ret = JPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_DecGetBitstreamBuffer failed Error code is 0x%x\n",
		     ret);
		goto FILL_BS_ERROR;
	}

	if (size <= 0) {
#ifdef MJPEG_INTERFACE_API
		size += pBufInfo->size - pBufInfo->point;
#else
		/* !! error designed !!*/
		return JPG_RET_SUCCESS;
#endif
	}

	if (defaultsize) {
		if (size < defaultsize)
			fillSize = size;
		else
			fillSize = defaultsize;
	} else {
		fillSize = size;
	}

	fillSize = FillSdramBurst(pBufInfo, paWrPtr, paBsBufStart, paBsBufEnd,
				  fillSize, checkeos, pstreameos, endian);

	if (*pstreameos == 0) {
		ret = JPU_DecUpdateBitstreamBuffer(handle, fillSize);
		if (ret != JPG_RET_SUCCESS) {
			JLOG(ERR,
			     "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x\n",
			     ret);
			goto FILL_BS_ERROR;
		}

		if ((pBufInfo->size - pBufInfo->point) <= 0) {
			ret = JPU_DecUpdateBitstreamBuffer(handle,
							   STREAM_END_SIZE);
			if (ret != JPG_RET_SUCCESS) {
				JLOG(ERR,
				     "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x\n",
				     ret);
				goto FILL_BS_ERROR;
			}

			pBufInfo->fillendbs = 1;
		}
	} else {
		if (!pBufInfo->fillendbs) {
			ret = JPU_DecUpdateBitstreamBuffer(handle,
							   STREAM_END_SIZE);
			if (ret != JPG_RET_SUCCESS) {
				JLOG(ERR,
				     "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x\n",
				     ret);
				goto FILL_BS_ERROR;
			}
			pBufInfo->fillendbs = 1;
		}
	}

FILL_BS_ERROR:

	return ret;
}

#ifdef REDUNDENT_CODE
/**
 * Bitstream Read for encoders
 */
JpgRet ReadJpgBsBufHelper(JpgEncHandle handle, FILE *bsFp,
			  PhysicalAddress paBsBufStart,
			  PhysicalAddress paBsBufEnd, int encHeaderSize,
			  int endian)
{
	JpgRet ret = JPG_RET_SUCCESS;
	int loadSize = 0;
	int stuffSize;
	PhysicalAddress paRdPtr, paWrPtr;
	int size = 0;

	ret = JPU_EncGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
	if (ret != JPG_RET_SUCCESS) {
		JLOG(ERR,
		     "JPU_EncGetBitstreamBuffer failed Error code is 0x%x\n",
		     ret);
		goto LOAD_BS_ERROR;
	}

	if (size > 0) {
		stuffSize = 0;
		if (encHeaderSize && (size + encHeaderSize) % 8) {
			stuffSize = (size + encHeaderSize) -
				    (((size + encHeaderSize) >> 3) << 3);
			stuffSize = 8 - stuffSize;
		}

		loadSize = size;

		if (loadSize > 0) {
			ProcessEncodedBitstreamBurst(bsFp, paRdPtr,
						     paBsBufStart, paBsBufEnd,
						     loadSize, endian);

			ret = JPU_EncUpdateBitstreamBuffer(handle, loadSize);
			if (ret != JPG_RET_SUCCESS) {
				JLOG(ERR,
				     "JPU_EncUpdateBitstreamBuffer failed Error code is 0x%x\n",
				     ret);
				goto LOAD_BS_ERROR;
			}
		}
	}

LOAD_BS_ERROR:

	return ret;
}
#endif

/******************************************************************************
    DPB Image Data Control
******************************************************************************/
#ifdef REDUNDENT_CODE
int LoadYuvPartialImageHelperFormat(
	FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY, PhysicalAddress addrCb,
	PhysicalAddress addrCr, int picWidth, int picHeight,
	int picHeightPartial, int stride, int interleave, int format,
	int endian, int partPosIdx, int frameIdx, int packed)
{
	int LumaPicSize;
	int ChromaPicSize;
	int LumaPartialSize;
	int ChromaPartialSize;
	int pos;
	int divX, divY;
	int frameSize;

	divX = format == FORMAT_420 || format == FORMAT_422 ? 2 : 1;
	divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

	LumaPicSize = picWidth * picHeight;
	ChromaPicSize = LumaPicSize / divX / divY;

	LumaPartialSize = picWidth * picHeightPartial;
	ChromaPartialSize = LumaPartialSize / divX / divY;

	if (format == FORMAT_400)
		frameSize = LumaPicSize;
	else
		frameSize = LumaPicSize + ChromaPicSize * 2;
	// Load source one picture image to encode to SDRAM frame buffer.

	if (packed) {
		if (packed == PACKED_FORMAT_444) {
			LumaPicSize = picWidth * 3 * picHeight;
			LumaPartialSize = picWidth * 3 * picHeightPartial;
		} else {
			LumaPicSize = picWidth * 2 * picHeight;
			LumaPartialSize = picWidth * 2 * picHeightPartial;
		}
		frameSize = LumaPicSize;
		ChromaPicSize = 0;
		ChromaPartialSize = 0;
	}

	// Y
	fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
	pos = LumaPartialSize * partPosIdx;
	fseek(yuvFp, pos, SEEK_CUR);

	if (!fread(pYuv, 1, LumaPartialSize, yuvFp)) {
		if (!feof(yuvFp))
			JLOG(ERR,
			     "Yuv Data fread failed file handle is 0x%x\n",
			     yuvFp);
		return 0;
	}

	if (format != FORMAT_400 && packed == 0) {
		// Cb
		fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
		pos = LumaPicSize + ChromaPartialSize * partPosIdx;
		fseek(yuvFp, pos, SEEK_CUR);

		if (!fread(pYuv + LumaPartialSize, 1, ChromaPartialSize,
			   yuvFp)) {
			if (!feof(yuvFp))
				JLOG(ERR,
				     "Yuv Data fread failed file handle is 0x%x\n",
				     yuvFp);
			return 0;
		}

		// Cr
		fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
		pos = LumaPicSize + ChromaPicSize +
		      ChromaPartialSize * partPosIdx;
		fseek(yuvFp, pos, SEEK_CUR);

		if (!fread(pYuv + LumaPartialSize + ChromaPartialSize, 1,
			   ChromaPartialSize, yuvFp)) {
			if (!feof(yuvFp))
				JLOG(ERR,
				     "Yuv Data fread failed file handle is 0x%x\n",
				     yuvFp);
			return 0;
		}
	}

	LoadYuvImageBurstFormat(pYuv, picWidth, picHeightPartial, addrY, addrCb,
				addrCr, stride, interleave, format, endian,
				packed);

	return 1;
}

int SaveYuvImageHelperFormat(FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY,
			     PhysicalAddress addrCb, PhysicalAddress addrCr,
			     int picWidth, int picHeight, int stride,
			     int interLeave, int format, int endian, int packed,
			     int *pFrameSize)
{
	int frameSize;
	frameSize = StoreYuvImageBurstFormat(pYuv, picWidth, picHeight, addrY,
					     addrCb, addrCr, stride, interLeave,
					     format, endian, packed);

	if (pFrameSize != NULL) {
		*pFrameSize = frameSize;
	}

	if (yuvFp) {
		if (!fwrite(pYuv, sizeof(Uint8), frameSize, yuvFp)) {
			JLOG(ERR,
			     "Frame Data fwrite failed file handle is 0x%x\n",
			     yuvFp);
			return 0;
		}
		fflush(yuvFp);
	}
	return 1;
}

int SaveYuvPartialImageHelperFormat(
	FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY, PhysicalAddress addrCb,
	PhysicalAddress addrCr, int picWidth, int picHeight,
	int picHeightPartial, int stride, int interLeave, int format,
	int endian, int partPosIdx, int frameIdx, int packed)
{
	int LumaPicSize = 0;
	int ChromaPicSize = 0;
	int frameSize = 0;

	int LumaPartialSize = 0;
	int ChromaPartialSize = 0;

	int pos = 0;
	// int divX, divY;

	// divX = format == FORMAT_420 || format == FORMAT_422 ? 2 : 1;
	// divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

	switch (format) {
	case FORMAT_420:
		LumaPicSize = picWidth * (((picHeight + 1) >> 1) << 1);
		ChromaPicSize = ((picWidth + 1) >> 1) * ((picHeight + 1) >> 1);
		frameSize = (LumaPicSize + ChromaPicSize) << 1;

		LumaPartialSize =
			picWidth * (((picHeightPartial + 1) >> 1) << 1);
		ChromaPartialSize =
			((picWidth + 1) >> 1) * ((picHeightPartial + 1) >> 1);
		if (interLeave)
			ChromaPartialSize = (((picWidth + 1) >> 1) << 1) *
					    ((picHeightPartial + 1) >> 1);
		break;
	case FORMAT_224:
		LumaPicSize = picWidth * (((picHeight + 1) >> 1) << 1);
		ChromaPicSize = picWidth * ((picHeight + 1) >> 1);
		frameSize = (LumaPicSize + ChromaPicSize) << 1;

		LumaPartialSize =
			picWidth * (((picHeightPartial + 1) >> 1) << 1);
		ChromaPartialSize = picWidth * ((picHeightPartial + 1) >> 1);
		if (interLeave)
			ChromaPartialSize =
				picWidth * 2 * ((picHeightPartial + 1) >> 1);
		break;
	case FORMAT_422:
		LumaPicSize = picWidth * picHeight;
		ChromaPicSize = ((picWidth + 1) >> 1) * picHeight;
		frameSize = (LumaPicSize + ChromaPicSize) << 1;

		LumaPartialSize = picWidth * picHeightPartial;
		ChromaPartialSize = ((picWidth + 1) >> 1) * picHeightPartial;
		if (interLeave)
			ChromaPartialSize =
				(((picWidth + 1) >> 1) << 1) * picHeightPartial;
		break;
	case FORMAT_444:
		LumaPicSize = picWidth * picHeight;
		ChromaPicSize = picWidth * picHeight;
		frameSize = (LumaPicSize + ChromaPicSize) << 1;

		LumaPartialSize = picWidth * picHeightPartial;
		ChromaPartialSize = picWidth * picHeightPartial;
		if (interLeave)
			ChromaPartialSize = picWidth * 2 * picHeightPartial;
		break;
	case FORMAT_400:
		LumaPicSize = picWidth * picHeight;
		ChromaPicSize = 0;
		frameSize = (LumaPicSize + ChromaPicSize) << 1;

		LumaPartialSize = picWidth * picHeightPartial;
		ChromaPartialSize = 0;
		break;
	}

	if (packed) {
		if (packed == PACKED_FORMAT_444)
			picWidth *= 3;
		else
			picWidth *= 2;

		LumaPicSize = picWidth * picHeight;
		ChromaPicSize = 0;
		frameSize = LumaPicSize;
		LumaPartialSize = picWidth * picHeightPartial;
		ChromaPartialSize = 0;
	}

	StoreYuvImageBurstFormat(pYuv, picWidth, picHeightPartial, addrY,
				 addrCb, addrCr, stride, interLeave, format,
				 endian, packed);

	if (yuvFp) {
		// Y
		fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
		pos = LumaPartialSize * partPosIdx;
		fseek(yuvFp, pos, SEEK_CUR);
		if (!fwrite(pYuv, sizeof(Uint8), LumaPartialSize, yuvFp)) {
			JLOG(ERR,
			     "Frame Data fwrite failed file handle is 0x%x\n",
			     yuvFp);
			return 0;
		}

		if (packed) {
			fflush(yuvFp);
			return 1;
		}

		if (format != FORMAT_400) {
			// Cb
			fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
			pos = LumaPicSize + ChromaPartialSize * partPosIdx;
			fseek(yuvFp, pos, SEEK_CUR);
			if (!fwrite(pYuv + LumaPartialSize, sizeof(Uint8),
				    ChromaPartialSize, yuvFp)) {
				JLOG(ERR,
				     "Frame Data fwrite failed file handle is 0x%x\n",
				     yuvFp);
				return 0;
			}

			if (interLeave) {
				fflush(yuvFp);
				return 1;
			}

			// Cr
			fseek(yuvFp, (frameIdx * frameSize), SEEK_SET);
			pos = LumaPicSize + ChromaPicSize +
			      ChromaPartialSize * partPosIdx;
			fseek(yuvFp, pos, SEEK_CUR);
			if (!fwrite(pYuv + LumaPartialSize + ChromaPartialSize,
				    sizeof(Uint8), ChromaPartialSize, yuvFp)) {
				JLOG(ERR,
				     "Frame Data fwrite failed file handle is 0x%x\n",
				     yuvFp);
				return 0;
			}
		}
		fflush(yuvFp);
	}
	return 1;
}
#endif

/******************************************************************************
    JPEG specific Helper
******************************************************************************/
int jpgGetHuffTable(char *huffFileName, EncMjpgParam *param)
{
	{
		// Rearrange and insert pre-defined Huffman table to deticated
		// variable.
		memcpy(param->huffBits[DC_TABLE_INDEX0], lumaDcBits,
		       16); // Luma DC BitLength
		memcpy(param->huffVal[DC_TABLE_INDEX0], lumaDcValue,
		       16); // Luma DC HuffValue

		memcpy(param->huffBits[AC_TABLE_INDEX0], lumaAcBits,
		       16); // Luma DC BitLength
		memcpy(param->huffVal[AC_TABLE_INDEX0], lumaAcValue,
		       162); // Luma DC HuffValue

		memcpy(param->huffBits[DC_TABLE_INDEX1], chromaDcBits,
		       16); // Chroma DC BitLength
		memcpy(param->huffVal[DC_TABLE_INDEX1], chromaDcValue,
		       16); // Chroma DC HuffValue

		memcpy(param->huffBits[AC_TABLE_INDEX1], chromaAcBits,
		       16); // Chroma AC BitLength
		memcpy(param->huffVal[AC_TABLE_INDEX1], chromaAcValue,
		       162); // Chorma AC HuffValue
	}

	return 1;
}

int jpgGetQMatrix(char *qMatFileName, EncMjpgParam *param)
{
	{
		// Rearrange and insert pre-defined Q-matrix to deticated
		// variable.
#ifdef INCREASE_Q_MATRIX
		for (int idx = 0; idx < 64; idx++) {
			param->qMatTab[DC_TABLE_INDEX0][idx] =
				(lumaQ2[idx] * 3 > 255) ? 255 : lumaQ2[idx] * 3;
			param->qMatTab[AC_TABLE_INDEX0][idx] =
				(chromaBQ2[idx] * 3 > 255) ? 255 :
								   chromaBQ2[idx] * 3;
		}
#else
		memcpy(param->qMatTab[DC_TABLE_INDEX0], lumaQ2, 64);
		memcpy(param->qMatTab[AC_TABLE_INDEX0], chromaBQ2, 64);
#endif
		memcpy(param->qMatTab[DC_TABLE_INDEX1],
		       param->qMatTab[DC_TABLE_INDEX0], 64);
		memcpy(param->qMatTab[AC_TABLE_INDEX1],
		       param->qMatTab[AC_TABLE_INDEX0], 64);
	}

	return 1;
}

/******************************************************************************
    EncOpenParam Initialization
******************************************************************************/

int getJpgEncOpenParamDefault(JpgEncOpenParam *pEncOP,
			      EncConfigParam *pEncConfig)
{
	int ret;
	EncMjpgParam *pMjpgParam = kzalloc(sizeof(EncMjpgParam), GFP_KERNEL);

	{
		pEncOP->picWidth = pEncConfig->picWidth;
		pEncOP->picHeight = pEncConfig->picHeight;
	}

	pEncOP->sourceFormat = pEncConfig->sourceFormat;
	pEncOP->restartInterval = 0;
	pEncOP->chromaInterleave = pEncConfig->chromaInterleave;
	pEncOP->packedFormat = pEncConfig->packedFormat;
	pMjpgParam->sourceFormat = pEncConfig->sourceFormat;
	ret = jpgGetHuffTable(pEncConfig->huffFileName, pMjpgParam);
	if (ret == 0) {
		kfree(pMjpgParam);
		return ret;
	}
	ret = jpgGetQMatrix(pEncConfig->qMatFileName, pMjpgParam);
	if (ret == 0) {
		kfree(pMjpgParam);
		return ret;
	}
	memcpy(pEncOP->huffVal, pMjpgParam->huffVal, 4 * 162);
	memcpy(pEncOP->huffBits, pMjpgParam->huffBits, 4 * 256);
	memcpy(pEncOP->qMatTab, pMjpgParam->qMatTab, 4 * 64);
	kfree(pMjpgParam);
	return 1;
}

int FillSdramBurst(BufInfo *pBufInfo, PhysicalAddress targetAddr,
		   PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
		   Uint32 size, int checkeos, int *streameos, int endian)
{
	Uint8 *pBuf;
	int room;

	pBufInfo->count = 0;

	if (checkeos == 1 && (pBufInfo->point >= pBufInfo->size)) {
		*streameos = 1;
		return 0;
	}

	if ((pBufInfo->size - pBufInfo->point) < (int)size)
		pBufInfo->count = (pBufInfo->size - pBufInfo->point);
	else
		pBufInfo->count = size;

	pBuf = pBufInfo->buf + pBufInfo->point;
	if ((targetAddr + pBufInfo->count) > bsBufEndAddr) {
		room = bsBufEndAddr - targetAddr;
		JpuWriteMem(targetAddr, pBuf, room, endian);
#if (defined CVI_JPG_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		jdi_flush_ion_cache(targetAddr, pBuf, room);
#endif
		JpuWriteMem(bsBufStartAddr, pBuf + room,
			    (pBufInfo->count - room), endian);
#if (defined CVI_JPG_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		jdi_flush_ion_cache(bsBufStartAddr, (pBuf + room),
				    (pBufInfo->count - room));
#endif
	} else {
		JpuWriteMem(targetAddr, pBuf, pBufInfo->count, endian);
#if (defined CVI_JPG_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		jdi_flush_ion_cache(targetAddr, pBuf, pBufInfo->count);
#endif
	}

	pBufInfo->point += pBufInfo->count;
	return pBufInfo->count;
}

#ifdef REDUNDENT_CODE
int StoreYuvImageBurstFormat(Uint8 *dst, int picWidth, int picHeight,
			     unsigned long addrY, unsigned long addrCb,
			     unsigned long addrCr, int stride, int interLeave,
			     int format, int endian, int packed)
{
	int size;
	int y, nY = 0, nCb = 0, nCr = 0;
	unsigned long addr;
	int lumaSize, chromaSize = 0, chromaStride = 0, chromaWidth = 0,
		      chromaHeight = 0;

	Uint8 *puc;

	switch (format) {
	case FORMAT_420:
		nY = ((picHeight + 1) >> 1) << 1;
		nCb = nCr = (picHeight + 1) >> 1;
		chromaSize = ((picWidth + 1) >> 1) * ((picHeight + 1) >> 1);
		chromaStride = stride >> 1;
		chromaWidth = (picWidth + 1) >> 1;
		chromaHeight = nY;
		break;
	case FORMAT_224:
		nY = ((picHeight + 1) >> 1) << 1;
		nCb = nCr = (picHeight + 1) >> 1;
		chromaSize = (picWidth) * ((picHeight + 1) >> 1);
		chromaStride = stride;
		chromaWidth = picWidth;
		chromaHeight = nY;
		break;
	case FORMAT_422:
		nY = picHeight;
		nCb = nCr = picHeight;
		chromaSize = ((picWidth + 1) >> 1) * picHeight;
		chromaStride = stride >> 1;
		chromaWidth = (picWidth + 1) >> 1;
		chromaHeight = nY * 2;
		break;
	case FORMAT_444:
		nY = picHeight;
		nCb = nCr = picHeight;
		chromaSize = picWidth * picHeight;
		chromaStride = stride;
		chromaWidth = picWidth;
		chromaHeight = nY * 2;
		break;
	case FORMAT_400:
		nY = picHeight;
		nCb = nCr = 0;
		chromaSize = 0;
		chromaStride = 0;
		chromaWidth = 0;
		chromaHeight = 0;
		break;
	}

	puc = dst;
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

	if (picWidth == stride) {
		JpuReadMem(addr, (Uint8 *)(puc), lumaSize, endian);

		if (packed)
			return size;

		if (interLeave) {
			puc = dst + lumaSize;
			addr = addrCb;
			JpuReadMem(addr, (Uint8 *)(puc), chromaSize * 2,
				   endian);
		} else {
			puc = dst + lumaSize;
			addr = addrCb;
			JpuReadMem(addr, (Uint8 *)(puc), chromaSize, endian);

			puc = dst + lumaSize + chromaSize;
			addr = addrCr;
			JpuReadMem(addr, (Uint8 *)(puc), chromaSize, endian);
		}
	} else {
		for (y = 0; y < nY; ++y) {
			JpuReadMem(addr + stride * y,
				   (Uint8 *)(puc + y * picWidth), picWidth,
				   endian);
		}

		if (packed)
			return size;

		if (interLeave) {
			puc = dst + lumaSize;
			addr = addrCb;
			for (y = 0; y < (chromaHeight >> 1); ++y) {
				JpuReadMem(
					addr + (chromaStride * 2) * y,
					(Uint8 *)(puc + y * (chromaWidth * 2)),
					(chromaWidth * 2), endian);
			}
		} else {
			puc = dst + lumaSize;
			addr = addrCb;
			for (y = 0; y < nCb; ++y) {
				JpuReadMem(addr + chromaStride * y,
					   (Uint8 *)(puc + y * chromaWidth),
					   chromaWidth, endian);
			}

			puc = dst + lumaSize + chromaSize;
			addr = addrCr;
			for (y = 0; y < nCr; ++y) {
				JpuReadMem(addr + chromaStride * y,
					   (Uint8 *)(puc + y * chromaWidth),
					   chromaWidth, endian);
			}
		}
	}

	return size;
}

void ProcessEncodedBitstreamBurst(FILE *fp, unsigned long targetAddr,
				  PhysicalAddress bsBufStartAddr,
				  PhysicalAddress bsBufEndAddr, int size,
				  int endian)
{
	Uint8 *val = 0;
	int room = 0;

	val = (Uint8 *)malloc(size);
	if ((targetAddr + size) > (unsigned long)bsBufEndAddr) {
		room = bsBufEndAddr - targetAddr;
		JpuReadMem(targetAddr, val, room, endian);
		JpuReadMem(bsBufStartAddr, val + room, (size - room), endian);
	} else {
		JpuReadMem(targetAddr, val, size, endian);
	}

	if (fp) {
		fwrite(val, sizeof(Uint8), size, fp);
		fflush(fp);
	}

	free(val);
}
#endif

#ifdef REDUNDENT_CODE
void GetMcuUnitSize(int format, int *mcuWidth, int *mcuHeight)
{
	switch (format) {
	case FORMAT_420:
		*mcuWidth = 16;
		*mcuHeight = 16;
		break;
	case FORMAT_422:
		*mcuWidth = 16;
		*mcuHeight = 8;
		break;
	case FORMAT_224:
		*mcuWidth = 8;
		*mcuHeight = 16;
		break;
	default: // FORMAT_444,400
		*mcuWidth = 8;
		*mcuHeight = 8;
		break;
	}
}


unsigned int GetFrameBufSize(int framebufFormat, int picWidth, int picHeight)
{
	unsigned int framebufSize = 0;
	unsigned int framebufWidth, framebufHeight;

	if (framebufFormat == FORMAT_420 || framebufFormat == FORMAT_422)
		framebufWidth = ((picWidth + 15) >> 4) << 4;
	else
		framebufWidth = ((picWidth + 7) >> 3) << 3;

	if (framebufFormat == FORMAT_420 || framebufFormat == FORMAT_224)
		framebufHeight = ((picHeight + 15) >> 4) << 4;
	else
		framebufHeight = ((picHeight + 7) >> 3) << 3;

	switch (framebufFormat) {
	case FORMAT_420:
		framebufSize =
			(framebufWidth * (((framebufHeight + 1) >> 1) << 1)) +
			((((framebufWidth + 1) >> 1) *
			  ((framebufHeight + 1) >> 1))
			 << 1);
		break;
	case FORMAT_224:
		framebufSize =
			framebufWidth * (((framebufHeight + 1) >> 1) << 1) +
			((framebufWidth * ((framebufHeight + 1) >> 1)) << 1);
		break;
	case FORMAT_422:
		framebufSize =
			framebufWidth * framebufHeight +
			((((framebufWidth + 1) >> 1) * framebufHeight) << 1);
		break;
	case FORMAT_444:
		framebufSize = framebufWidth * framebufHeight * 3;
		break;
	case FORMAT_400:
		framebufSize = framebufWidth * framebufHeight;
		break;
	}

	framebufSize = ((framebufSize + 7) & ~7);

	return framebufSize;
}

// inteleave : 0 (chroma separate mode), 1 (cbcr interleave mode), 2 (crcb
// interleave mode)
yuv2rgb_color_format
convert_jpuapi_format_to_yuv2rgb_color_format(int planar_format,
					      int pack_format, int interleave)
{
	// typedef enum { YUV444, YUV422, YUV420, NV12, NV21,  YUV400, YUYV,
	// YVYU, UYVY, VYUY, YYY, RGB_PLANAR, RGB32, RGB24, RGB16 }
	// yuv2rgb_color_format;
	yuv2rgb_color_format format = YUV420; // TODO : add default
	if (!pack_format) {
		switch (planar_format) {
		case FORMAT_400:
			format = YUV400;
			break;
		case FORMAT_444:
			format = YUV444;
			break;
		case FORMAT_224:
		case FORMAT_422:
			format = YUV422;
			break;
		case FORMAT_420:
			if (interleave == 0)
				format = YUV420;
			else if (interleave == 1)
				format = NV12;
			else
				format = NV21;
			break;
		}
	} else {
		switch (pack_format) {
		case PACKED_FORMAT_422_YUYV:
			format = YUYV;
			break;
		case PACKED_FORMAT_422_UYVY:
			format = UYVY;
			break;
		case PACKED_FORMAT_422_YVYU:
			format = YVYU;
			break;
		case PACKED_FORMAT_422_VYUY:
			format = VYUY;
			break;
		case PACKED_FORMAT_444:
			format = YYY;
			break;
		}
	}

	return format;
}

void jpu_yuv2rgb(int width, int height, yuv2rgb_color_format format,
		 unsigned char *src, unsigned char *rgba, int cbcr_reverse)
{
#define jpu_clip(var) ((var >= 255) ? 255 : (var <= 0) ? 0 : var)
	int j, i;
	int c, d, e;

	unsigned char *line = rgba;
	unsigned char *cur = NULL;
	unsigned char *y = NULL;
	unsigned char *u = NULL;
	unsigned char *v = NULL;
	unsigned char *misc = NULL;

	int frame_size_y;
	int frame_size_uv;
	// int frame_size;
	int t_width;

	frame_size_y = width * height;

	if (format == YUV444 || format == RGB_PLANAR)
		frame_size_uv = width * height;
	else if (format == YUV422)
		frame_size_uv = (width * height) >> 1;
	else if (format == YUV420 || format == NV12 || format == NV21)
		frame_size_uv = (width * height) >> 2;
	else
		frame_size_uv = 0;

#if 0
	if (format == YUYV || format == YVYU || format == UYVY ||
	    format == VYUY)
		frame_size = frame_size_y * 2;
	else if (format == RGB32)
		frame_size = frame_size_y * 4;
	else if (format == RGB24)
		frame_size = frame_size_y * 3;
	else if (format == RGB16)
		frame_size = frame_size_y * 2;
	else
		frame_size = frame_size_y + frame_size_uv * 2;
#endif

	t_width = width;

	if (format == YUYV || format == YVYU || format == UYVY ||
	    format == VYUY) {
		misc = src;
	} else if (format == NV12 || format == NV21) {
		y = src;
		misc = src + frame_size_y;
	} else if (format == RGB32 || format == RGB24 || format == RGB16) {
		misc = src;
	} else {
		y = src;
		u = src + frame_size_y;
		v = src + frame_size_y + frame_size_uv;
	}

	if (format == YUV444) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				c = y[j * width + i] - 16;
				d = u[j * width + i] - 128;
				e = v[j * width + i] - 128;

				if (!cbcr_reverse) {
					d = u[j * width + i] - 128;
					e = v[j * width + i] - 128;
				} else {
					e = u[j * width + i] - 128;
					e = v[j * width + i] - 128;
				}
				(*cur) = jpu_clip((298 * c + 409 * e + 128) >>
						  8);
				cur++;
				(*cur) = jpu_clip(
					(298 * c - 100 * d - 208 * e + 128) >>
					8);
				cur++;
				(*cur) = jpu_clip((298 * c + 516 * d + 128) >>
						  8);
				cur++;
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else if (format == YUV422) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				c = y[j * width + i] - 16;
				d = u[j * (width >> 1) + (i >> 1)] - 128;
				e = v[j * (width >> 1) + (i >> 1)] - 128;

				if (!cbcr_reverse) {
					d = u[j * (width >> 1) + (i >> 1)] -
					    128;
					e = v[j * (width >> 1) + (i >> 1)] -
					    128;
				} else {
					e = u[j * (width >> 1) + (i >> 1)] -
					    128;
					d = v[j * (width >> 1) + (i >> 1)] -
					    128;
				}

				(*cur) = jpu_clip((298 * c + 409 * e + 128) >>
						  8);
				cur++;
				(*cur) = jpu_clip(
					(298 * c - 100 * d - 208 * e + 128) >>
					8);
				cur++;
				(*cur) = jpu_clip((298 * c + 516 * d + 128) >>
						  8);
				cur++;
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else if (format == YUYV || format == YVYU || format == UYVY ||
		   format == VYUY) {
		unsigned char *t = misc;
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i += 2) {
				switch (format) {
				case YUYV:
					c = *(t)-16;
					if (!cbcr_reverse) {
						d = *(t + 1) - 128;
						e = *(t + 3) - 128;
					} else {
						e = *(t + 1) - 128;
						d = *(t + 3) - 128;
					}
					break;
				case YVYU:
					c = *(t)-16;
					if (!cbcr_reverse) {
						d = *(t + 3) - 128;
						e = *(t + 1) - 128;
					} else {
						e = *(t + 3) - 128;
						d = *(t + 1) - 128;
					}
					break;
				case UYVY:
					c = *(t + 1) - 16;
					if (!cbcr_reverse) {
						d = *(t)-128;
						e = *(t + 2) - 128;
					} else {
						e = *(t)-128;
						d = *(t + 2) - 128;
					}
					break;
				case VYUY:
					c = *(t + 1) - 16;
					if (!cbcr_reverse) {
						d = *(t + 2) - 128;
						e = *(t)-128;
					} else {
						e = *(t + 2) - 128;
						d = *(t)-128;
					}
					break;
				default: // like YUYV
					c = *(t)-16;
					if (!cbcr_reverse) {
						d = *(t + 1) - 128;
						e = *(t + 3) - 128;
					} else {
						e = *(t + 1) - 128;
						d = *(t + 3) - 128;
					}
					break;
				}

				(*cur) = jpu_clip((298 * c + 409 * e + 128) >>
						  8);
				cur++;
				(*cur) = jpu_clip(
					(298 * c - 100 * d - 208 * e + 128) >>
					8);
				cur++;
				(*cur) = jpu_clip((298 * c + 516 * d + 128) >>
						  8);
				cur++;
				(*cur) = 0;
				cur++;

				switch (format) {
				case YUYV:
				case YVYU:
					c = *(t + 2) - 16;
					break;

				case VYUY:
				case UYVY:
					c = *(t + 3) - 16;
					break;
				default: // like YUYV
					c = *(t + 2) - 16;
					break;
				}

				(*cur) = jpu_clip((298 * c + 409 * e + 128) >>
						  8);
				cur++;
				(*cur) = jpu_clip(
					(298 * c - 100 * d - 208 * e + 128) >>
					8);
				cur++;
				(*cur) = jpu_clip((298 * c + 516 * d + 128) >>
						  8);
				cur++;
				(*cur) = 0;
				cur++;

				t += 4;
			}
			line += t_width << 2;
		}
	} else if (format == YUV420 || format == NV12 || format == NV21) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				c = y[j * width + i] - 16;
				if (format == YUV420) {
					if (!cbcr_reverse) {
						d = u[(j >> 1) * (width >> 1) +
						      (i >> 1)] -
						    128;
						e = v[(j >> 1) * (width >> 1) +
						      (i >> 1)] -
						    128;
					} else {
						e = u[(j >> 1) * (width >> 1) +
						      (i >> 1)] -
						    128;
						d = v[(j >> 1) * (width >> 1) +
						      (i >> 1)] -
						    128;
					}
				} else if (format == NV12) {
					if (!cbcr_reverse) {
						d = misc[(j >> 1) * width +
							 (i >> 1 << 1)] -
						    128;
						e = misc[(j >> 1) * width +
							 (i >> 1 << 1) + 1] -
						    128;
					} else {
						e = misc[(j >> 1) * width +
							 (i >> 1 << 1)] -
						    128;
						d = misc[(j >> 1) * width +
							 (i >> 1 << 1) + 1] -
						    128;
					}
				} else { // if (m_color == NV21)
					if (!cbcr_reverse) {
						d = misc[(j >> 1) * width +
							 (i >> 1 << 1) + 1] -
						    128;
						e = misc[(j >> 1) * width +
							 (i >> 1 << 1)] -
						    128;
					} else {
						e = misc[(j >> 1) * width +
							 (i >> 1 << 1) + 1] -
						    128;
						d = misc[(j >> 1) * width +
							 (i >> 1 << 1)] -
						    128;
					}
				}
				(*cur) = jpu_clip((298 * c + 409 * e + 128) >>
						  8);
				cur++;
				(*cur) = jpu_clip(
					(298 * c - 100 * d - 208 * e + 128) >>
					8);
				cur++;
				(*cur) = jpu_clip((298 * c + 516 * d + 128) >>
						  8);
				cur++;
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else if (format == RGB_PLANAR) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				(*cur) = y[j * width + i];
				cur++;
				(*cur) = u[j * width + i];
				cur++;
				(*cur) = v[j * width + i];
				cur++;
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else if (format == RGB32) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				(*cur) = misc[j * width * 4 + i];
				cur++; // R
				(*cur) = misc[j * width * 4 + i + 1];
				cur++; // G
				(*cur) = misc[j * width * 4 + i + 2];
				cur++; // B
				(*cur) = misc[j * width * 4 + i + 3];
				cur++; // A
			}
			line += t_width << 2;
		}
	} else if (format == RGB24) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				(*cur) = misc[j * width * 3 + i];
				cur++; // R
				(*cur) = misc[j * width * 3 + i + 1];
				cur++; // G
				(*cur) = misc[j * width * 3 + i + 2];
				cur++; // B
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else if (format == RGB16) {
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				int tmp = misc[j * width * 2 + i] << 8 |
					  misc[j * width * 2 + i + 1];
				(*cur) = ((tmp >> 11) & 0x1F << 3);
				cur++; // R(5bit)
				(*cur) = ((tmp >> 5) & 0x3F << 2);
				cur++; // G(6bit)
				(*cur) = ((tmp)&0x1F << 3);
				cur++; // B(5bit)
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	} else { // YYY
		for (j = 0; j < height; j++) {
			cur = line;
			for (i = 0; i < width; i++) {
				(*cur) = y[j * width + i];
				cur++;
				(*cur) = y[j * width + i];
				cur++;
				(*cur) = y[j * width + i];
				cur++;
				(*cur) = 0;
				cur++;
			}
			line += t_width << 2;
		}
	}
}

int comparateYuv(Uint8 *pYuv, Uint8 *pRefYuv, int picWidth, int picHeight,
		 int stride, int format)
{
	if (picHeight % 16 != 0 || picWidth % 16 != 0) {
		Uint8 *pY = pYuv;
		Uint8 *pRefY = pRefYuv;
		int i = 0;

		// only compare luma
		for (i = 0; i < picHeight; i++) {
			int j = 0;

			for (j = 0; j < picWidth; j++) {
				if (pY[j] != pRefY[j]) {
					pr_err("comp index : %d, %d, src: 0x%x, ref: 0x%x\n",
					       i, j, pY[j], pRefY[j]);
					return -1;
				}
			}
			pY += stride;
			pRefY += picWidth;
		}
	} else {
		int frameBufferSize =
			GetFrameBufSize(format, picWidth, picHeight);
		Uint64 *p64 = (Uint64 *)pYuv;
		Uint64 *pR64 = (Uint64 *)pRefYuv;
		int i = 0;
		pr_err("framebuffer addr: 0x%p, ref addr: 0x%p, size : %d\n",
		       p64, pR64, frameBufferSize);
		for (i = 0; i < (frameBufferSize >> 3); i++) {
			if (*p64 != *pR64) {
				pr_err("comp index : %d, src: 0x%llx, ref: 0x%llx\n",
				       i * 4, *p64, *pR64);
				return -1;
			}
			p64++;
			pR64++;
		}

		// return osal_memcmp(pYuv, pRefYuv, frameBufferSize);
	}
	return 0;
}
#endif
Uint64 jpgGetCurrentTime(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
#else
	struct timespec ts;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_ts64(&ts);
#else
	ktime_get_ts(&ts);
#endif
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // in us
}
