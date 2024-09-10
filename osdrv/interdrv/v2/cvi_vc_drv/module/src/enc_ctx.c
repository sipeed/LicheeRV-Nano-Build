#include "venc.h"
#include "enc_ctx.h"
#include "module_common.h"
#include "vdi_osal.h"
#include <linux/slab.h>


#ifndef UNREFERENCED_PARAM
#define UNREFERENCED_PARAM(x) ((void)(x))
#endif

#define DEF_IQP 32
#define DEF_PQP 32
#define ENC_TIMEOUT (-2)
#define DEAULT_QP_MAP_BITRATE 6000

static CVI_S32 jpege_init(CVI_VOID);
static CVI_S32 jpege_open(CVI_VOID *handle, CVI_VOID *pchnctx);
static CVI_S32 jpege_close(CVI_VOID *ctx);
static CVI_S32 jpege_enc_one_pic(CVI_VOID *ctx,
				 const VIDEO_FRAME_INFO_S *pstFrame,
				 int s32MilliSec);
static CVI_VOID setSrcInfo(CVIFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
			   const VIDEO_FRAME_INFO_S *pstFrame);
static CVI_S32 jpege_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
				CVI_S32 s32MilliSec);
static CVI_S32 jpege_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream);
static CVI_S32 jpege_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg);

static CVI_S32 vidEnc_init(CVI_VOID);
static CVI_S32 vidEnc_open(CVI_VOID *handle, CVI_VOID *pchnctx);
static CVI_S32 vidEnc_close(CVI_VOID *ctx);
static CVI_S32 vidEnc_enc_one_pic(CVI_VOID *ctx,
				  const VIDEO_FRAME_INFO_S *pstFrame,
				  CVI_S32 s32MilliSec);
static CVI_S32 vidEnc_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
				 CVI_S32 s32MilliSec);
static CVI_S32 vidEnc_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream);
static CVI_S32 vidEnc_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg);

#define H264_NALU_TYPE_IDR 5

static CVI_VOID h265e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx);
static CVI_VOID h265e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_VOID h265e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_VOID h265e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
				     CVI_VOID *pchnctx);
static CVI_VOID h265e_setInitCfgQpMap(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx);
static CVI_VOID h265e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_S32 h265e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 naluType);

#define H265_NALU_TYPE_W_RADL 19
#define H265_NALU_TYPE_N_LP 20

static CVI_VOID cviSetInitCfgGop(cviInitEncConfig *pInitEncCfg,
				 venc_chn_context *pChnHandle);
static CVI_VOID h264e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx);
static CVI_VOID h264e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_VOID h264e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_VOID h264e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
				     CVI_VOID *pchnctx);
static CVI_VOID h264e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx);
static CVI_S32 h264e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 naluType);

CVI_S32 venc_create_enc_ctx(venc_enc_ctx *pEncCtx, CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	CVI_S32 status = 0;

	VENC_MEMSET(pEncCtx, 0, sizeof(venc_enc_ctx));

	switch (pVencAttr->enType) {
	case PT_JPEG:
	case PT_MJPEG:
		pEncCtx->base.init = &jpege_init;
		pEncCtx->base.open = &jpege_open;
		pEncCtx->base.close = &jpege_close;
		pEncCtx->base.encOnePic = &jpege_enc_one_pic;
		pEncCtx->base.getStream = &jpege_get_stream;
		pEncCtx->base.releaseStream = &jpege_release_stream;
		pEncCtx->base.ioctl = &jpege_ioctl;
		break;
	case PT_H264:
		pEncCtx->base.init = &vidEnc_init;
		pEncCtx->base.open = &vidEnc_open;
		pEncCtx->base.close = &vidEnc_close;
		pEncCtx->base.encOnePic = &vidEnc_enc_one_pic;
		pEncCtx->base.getStream = &vidEnc_get_stream;
		pEncCtx->base.releaseStream = &vidEnc_release_stream;
		pEncCtx->base.ioctl = &vidEnc_ioctl;

		pEncCtx->ext.vid.setInitCfgFixQp = h264e_setInitCfgFixQp;
		pEncCtx->ext.vid.setInitCfgCbr = h264e_setInitCfgCbr;
		pEncCtx->ext.vid.setInitCfgVbr = h264e_setInitCfgVbr;
		pEncCtx->ext.vid.setInitCfgAVbr = h264e_setInitCfgAVbr;
		pEncCtx->ext.vid.setInitCfgUbr = h264e_setInitCfgUbr;
		pEncCtx->ext.vid.mapNaluType = h264e_mapNaluType;

		pEncCtx->base.u32Profile = pVencAttr->u32Profile;
		pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H264CBR;

		if (pEncCtx->base.rcMode == RC_MODE_CBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgCbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH264Cbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgVbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH264Vbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgAVbr;
			pEncCtx->base.bVariFpsEn =
				prcatt->stH264AVbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgFixQp;
			pEncCtx->base.bVariFpsEn =
				prcatt->stH264FixQp.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgUbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH264Ubr.bVariFpsEn;
		}
		break;
	case PT_H265:
		pEncCtx->base.init = &vidEnc_init;
		pEncCtx->base.open = &vidEnc_open;
		pEncCtx->base.close = &vidEnc_close;
		pEncCtx->base.encOnePic = &vidEnc_enc_one_pic;
		pEncCtx->base.getStream = &vidEnc_get_stream;
		pEncCtx->base.releaseStream = &vidEnc_release_stream;
		pEncCtx->base.ioctl = &vidEnc_ioctl;

		pEncCtx->ext.vid.setInitCfgFixQp = h265e_setInitCfgFixQp;
		pEncCtx->ext.vid.setInitCfgCbr = h265e_setInitCfgCbr;
		pEncCtx->ext.vid.setInitCfgVbr = h265e_setInitCfgVbr;
		pEncCtx->ext.vid.setInitCfgAVbr = h265e_setInitCfgAVbr;
		pEncCtx->ext.vid.setInitCfgQpMap = h265e_setInitCfgQpMap;
		pEncCtx->ext.vid.setInitCfgUbr = h265e_setInitCfgUbr;
		pEncCtx->ext.vid.mapNaluType = h265e_mapNaluType;

		pEncCtx->base.u32Profile = pVencAttr->u32Profile;
		pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H265CBR;

		if (pEncCtx->base.rcMode == RC_MODE_CBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgCbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH265Cbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgVbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH265Vbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgAVbr;
			pEncCtx->base.bVariFpsEn =
				prcatt->stH265AVbr.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgFixQp;
			pEncCtx->base.bVariFpsEn =
				prcatt->stH265FixQp.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_QPMAP) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgQpMap;
			pEncCtx->base.bVariFpsEn =
				prcatt->stH265QpMap.bVariFpsEn;
		} else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
			pEncCtx->ext.vid.setInitCfgRc =
				pEncCtx->ext.vid.setInitCfgUbr;
			pEncCtx->base.bVariFpsEn = prcatt->stH265Ubr.bVariFpsEn;
		}
		break;
	default:
		CVI_VENC_ERR("enType = %d\n", pVencAttr->enType);
		return -1;
	}

	pEncCtx->base.width = pVencAttr->u32PicWidth;
	pEncCtx->base.height = pVencAttr->u32PicHeight;

	return status;
}

static CVI_S32 jpege_init(CVI_VOID)
{
	CVI_S32 status = CVI_SUCCESS;

	status = CVIJpgInit();
	if (status != CVI_SUCCESS) {
		CVIJpgUninit();
		CVI_VENC_ERR("CVIJpgUninit\n");
		return CVI_FAILURE;
	}

	return status;
}

static CVI_S32 jpege_open(CVI_VOID *handle, CVI_VOID *pchnctx)
{
	venc_context *pHandle = (venc_context *)handle;
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	CVI_S32 status = CVI_SUCCESS;
	CVIJpgConfig config;
	CVIJpgHandle jpghandle = NULL;
	VENC_RC_ATTR_S *prcatt = NULL;
	venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

	memset(&config, 0, sizeof(CVIJpgConfig));

	// JPEG marker order
	memcpy(config.u.enc.jpgMarkerOrder,
	       pHandle->ModParam.stJpegeModParam.JpegMarkerOrder,
	       CVI_JPG_MARKER_ORDER_BUF_SIZE);

	config.type = CVIJPGCOD_ENC;
	config.u.enc.picWidth = pEncCtx->base.width;
	config.u.enc.picHeight = pEncCtx->base.height;
	config.u.enc.sourceFormat = CVI_FORMAT_420; // TODO
	config.u.enc.chromaInterleave = 0; // CbCr Separated
	config.u.enc.mirDir = 0; // no mirror
	config.u.enc.packedFormat = 0; // planar
	config.u.enc.src_type = JPEG_MEM_EXTERNAL;
	config.u.enc.singleEsBuffer =
		pHandle->ModParam.stJpegeModParam.bSingleEsBuf;
	if (config.u.enc.singleEsBuffer)
		config.u.enc.bitstreamBufSize =
			pHandle->ModParam.stJpegeModParam.u32SingleEsBufSize;
	else
		config.u.enc.bitstreamBufSize = pChnAttr->stVencAttr.u32BufSize;

	if ((config.u.enc.bitstreamBufSize & 0x3FF) != 0) {
		CVI_VENC_ERR("%s bitstreamBufSize (0x%x) must align to 1024\n",
			     __func__, config.u.enc.bitstreamBufSize);
		return CVI_ERR_VENC_NOT_SUPPORT;
	}
	prcatt = &pChnAttr->stRcAttr;

	if (prcatt->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
		VENC_MJPEG_FIXQP_S *pstMJPEGFixQp = &prcatt->stMjpegFixQp;

		config.u.enc.quality = pstMJPEGFixQp->u32Qfactor;
		CVI_VENC_TRACE("enRcMode = %d, u32Qfactor = %d\n",
			       prcatt->enRcMode, pstMJPEGFixQp->u32Qfactor);
	} else {
		VENC_MJPEG_CBR_S *pstMJPEGCbr = &prcatt->stMjpegCbr;
		int frameRateDiv, frameRateRes;

		config.u.enc.bitrate = pstMJPEGCbr->u32BitRate;
		frameRateDiv = (pstMJPEGCbr->fr32DstFrameRate >> 16);
		frameRateRes = pstMJPEGCbr->fr32DstFrameRate & 0xFFFF;

		if (frameRateDiv == 0) {
			config.u.enc.framerate = frameRateRes;
		} else {
			config.u.enc.framerate =
				((frameRateDiv - 1) << 16) + frameRateRes;
		}
	}

	config.s32ChnNum = pChnHandle->VeChn;
	jpghandle = CVIJpgOpen(config);
	if (jpghandle == NULL) {
		CVI_VENC_ERR("%s CVIJpgOpen failed !\n", __func__);
		jpege_close(pEncCtx);
		return CVI_ERR_VENC_NULL_PTR;
	}
	pEncCtx->ext.jpeg.handle = jpghandle;

	return status;
}

static CVI_S32 jpege_close(CVI_VOID *ctx)
{
	CVI_S32 status = CVI_SUCCESS;
	CVIJpgHandle *pHandle;

	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

	pHandle = pEncCtx->ext.jpeg.handle;
	if (pHandle != NULL) {
		CVIJpgClose(pHandle);
		pEncCtx->ext.jpeg.handle = NULL;
		CVIJpgUninit();
	}

	return status;
}

static CVI_S32 jpege_enc_one_pic(CVI_VOID *ctx,
				 const VIDEO_FRAME_INFO_S *pstFrame,
				 CVI_S32 s32MIlliSec)
{
	CVIJpgHandle *pHandle;
	CVIFRAMEBUF srcInfo, *psi = &srcInfo;
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

	pHandle = pEncCtx->ext.jpeg.handle;

	setSrcInfo(psi, pEncCtx, pstFrame);

	status = CVIJpgSendFrameData(pHandle, (void *)&srcInfo,
				     JPEG_MEM_EXTERNAL, s32MIlliSec);

	if (status == ENC_TIMEOUT) {
		//jpeg_enc_one_pic TimeOut..dont close
		//otherwise parallel / multiple jpg encode will failure
		return CVI_ERR_VENC_BUSY;
	}

	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("Failed to CVIJpgSendFrameData, ret = %x\n",
			     status);
		jpege_close(pEncCtx);
		return CVI_ERR_VENC_BUSY;
	}

	return status;
}

static CVI_VOID setSrcInfo(CVIFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
			   const VIDEO_FRAME_INFO_S *pstFrame)
{
	venc_enc_ctx_base *pvecb = &pEncCtx->base;
	const VIDEO_FRAME_S *pstVFrame;

	pstVFrame = &pstFrame->stVFrame;

	psi->strideY = pstVFrame->u32Stride[0];
	psi->strideC = pstVFrame->u32Stride[1];
	psi->vbY.phys_addr =
		pstVFrame->u64PhyAddr[0] + psi->strideY * pvecb->y + pvecb->x;

	switch (pstVFrame->enPixelFormat) {
	case PIXEL_FORMAT_YUV_PLANAR_422:
		psi->format = CVI_FORMAT_422;
		psi->packedFormat = CVI_PACKED_FORMAT_NONE;
		psi->chromaInterleave = CVI_CBCR_SEPARATED;
		psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
				      psi->strideC * pvecb->y + pvecb->x / 2;
		psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
				      psi->strideC * pvecb->y + pvecb->x / 2;
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
		psi->format = CVI_FORMAT_420;
		psi->packedFormat = CVI_PACKED_FORMAT_NONE;
		psi->chromaInterleave =
			(pstVFrame->enPixelFormat == PIXEL_FORMAT_NV12) ?
				      CVI_CBCR_INTERLEAVE :
				      CVI_CRCB_INTERLEAVE;
		psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
				      psi->strideC * pvecb->y / 2 + pvecb->x;
		psi->vbCr.phys_addr = 0;
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
	default:
		psi->format = CVI_FORMAT_420;
		psi->packedFormat = CVI_PACKED_FORMAT_NONE;
		psi->chromaInterleave = CVI_CBCR_SEPARATED;
		psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
				      psi->strideC * pvecb->y / 2 +
				      pvecb->x / 2;
		psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
				      psi->strideC * pvecb->y / 2 +
				      pvecb->x / 2;
		break;
	}

	psi->vbY.virt_addr = (void *)pstVFrame->pu8VirAddr[0] +
			     (psi->vbY.phys_addr - pstVFrame->u64PhyAddr[0]);
	psi->vbCb.virt_addr = (void *)pstVFrame->pu8VirAddr[1] +
			      (psi->vbCb.phys_addr - pstVFrame->u64PhyAddr[1]);
	psi->vbCr.virt_addr = (void *)pstVFrame->pu8VirAddr[2] +
			      (psi->vbCr.phys_addr - pstVFrame->u64PhyAddr[2]);

	CVI_VENC_SRC("x = %d, y = %d, stride = %d\n", pvecb->x, pvecb->y,
		     psi->strideY);
	CVI_VENC_SRC("orig pa Y = 0x%llx, Cb = 0x%llx, Cr = 0x%llx\n",
		     pstVFrame->u64PhyAddr[0], pstVFrame->u64PhyAddr[1],
		     pstVFrame->u64PhyAddr[2]);
	CVI_VENC_SRC(
		"phyAddrY = 0x%llX, phyAddrCb = 0x%llX, phyAddrCr = 0x%llX\n",
		psi->vbY.phys_addr, psi->vbCb.phys_addr, psi->vbCr.phys_addr);
}

static CVI_S32 jpege_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
				CVI_S32 s32MilliSec)
{
	CVIBUF cviBuf = { 0 };
	CVI_S32 status = CVI_SUCCESS;
	CVIJpgHandle *pHandle;
	VENC_PACK_S *ppack;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

	UNREFERENCED_PARAM(s32MilliSec);

	pHandle = pEncCtx->ext.jpeg.handle;

	status = CVIJpgGetFrameData(
		pHandle, (CVI_U8 *)&cviBuf, sizeof(CVIBUF),
		(unsigned long int *)&pEncCtx->base.u64EncHwTime);
#if 0
	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("Failed to CVIJpgGetFrameData, ret = %x\n",
			     status);
		jpege_close(pEncCtx);
		return CVI_ERR_VENC_BUSY;
	}
#else
	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("Failed to CVIJpgGetFrameData, ret = %x\n", status);
		return CVI_ERR_VENC_BUSY;
	}
#endif
	pstStream->u32PackCount = 1;
	ppack = &pstStream->pstPack[0];

	memset(ppack, 0, sizeof(VENC_PACK_S));

	ppack->pu8Addr = (CVI_U8 *)cviBuf.virt_addr;
	ppack->u64PhyAddr = cviBuf.phys_addr;
	ppack->u32Len = cviBuf.size;
	ppack->u64PTS = pEncCtx->base.u64PTS;

	return status;
}

static CVI_S32 jpege_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream)
{
	CVI_S32 status = CVI_SUCCESS;
	CVIJpgHandle *pHandle;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

	UNREFERENCED_PARAM(pstStream);

	pHandle = pEncCtx->ext.jpeg.handle;
	status = CVIJpgReleaseFrameData(pHandle);

	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("Failed to CVIJpgReleaseFrameData, ret = %x\n",
			     status);
		jpege_close(pEncCtx);
		return CVI_ERR_VENC_BUSY;
	}

	return CVI_SUCCESS;
}

static CVI_S32 jpege_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
	CVI_S32 currOp;

	CVI_VENC_TRACE("\n");

	currOp = (op & CVI_JPEG_OP_MASK) >> CVI_JPEG_OP_SHIFT;
	if (currOp == 0) {
		CVI_VENC_TRACE("op = 0x%X, currOp = 0x%X\n", op, currOp);
		return 0;
	}

	status = cviJpegIoctl(pEncCtx->ext.jpeg.handle, currOp, arg);
	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("cviJpgIoctl, currOp = 0x%X, status = %d\n",
			     currOp, status);
		return status;
	}

	return status;
}

static CVI_S32 vidEnc_init(CVI_VOID)
{
	CVI_S32 status = CVI_SUCCESS;
	return status;
}

static CVI_S32 vidEnc_open(CVI_VOID *handle, CVI_VOID *pchnctx)
{
	venc_context *pHandle = (venc_context *)handle;
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
	VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
	cviInitEncConfig initEncCfg, *pInitEncCfg;

	pInitEncCfg = &initEncCfg;
	memset(pInitEncCfg, 0, sizeof(cviInitEncConfig));

	pInitEncCfg->codec =
		(pVencAttr->enType == PT_H265) ? CODEC_H265 : CODEC_H264;
	pInitEncCfg->width = pEncCtx->base.width;
	pInitEncCfg->height = pEncCtx->base.height;
	pInitEncCfg->u32Profile = pEncCtx->base.u32Profile;
	pInitEncCfg->rcMode = pEncCtx->base.rcMode;
	CVI_VENC_TRACE("codec = %d, width = %d, height = %d, rcMode = %d\n",
		       pInitEncCfg->codec, pInitEncCfg->width,
		       pInitEncCfg->height, pInitEncCfg->rcMode);

	pInitEncCfg->bitrate = 0;
	if (pInitEncCfg->codec == CODEC_H264) {
		pInitEncCfg->userDataMaxLength =
			pHandle->ModParam.stH264eModParam.u32UserDataMaxLen;
		pInitEncCfg->singleLumaBuf =
			pVencAttr->stAttrH264e.bSingleLumaBuf;
		pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
			pHandle->ModParam.stH264eModParam.bSingleEsBuf;
	} else if (pInitEncCfg->codec == CODEC_H265) {
		pInitEncCfg->userDataMaxLength =
			pHandle->ModParam.stH265eModParam.u32UserDataMaxLen;
		pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
			pHandle->ModParam.stH265eModParam.bSingleEsBuf;
		if (pHandle->ModParam.stH265eModParam.enRefreshType ==
		    H265E_REFRESH_IDR)
			pInitEncCfg->decodingRefreshType = H265_RT_IDR;
		else
			pInitEncCfg->decodingRefreshType = H265_RT_CRA;
	}

	if (pInitEncCfg->bSingleEsBuf) {
		if (pInitEncCfg->codec == CODEC_H264)
			pInitEncCfg->bitstreamBufferSize =
				pHandle->ModParam.stH264eModParam
					.u32SingleEsBufSize;
		else if (pInitEncCfg->codec == CODEC_H265)
			pInitEncCfg->bitstreamBufferSize =
				pHandle->ModParam.stH265eModParam
					.u32SingleEsBufSize;
	} else {
		pInitEncCfg->bitstreamBufferSize = pVencAttr->u32BufSize;
	}
	pInitEncCfg->s32ChnNum = pChnHandle->VeChn;
	pInitEncCfg->bEsBufQueueEn = pVencAttr->bEsBufQueueEn;
	pInitEncCfg->bIsoSendFrmEn = pVencAttr->bIsoSendFrmEn;
	pInitEncCfg->svc_enable = pChnHandle->bSvcEnable;
	pInitEncCfg->fg_protect_en = pChnHandle->svcParam.fg_protect_en;
	pInitEncCfg->fg_dealt_qp = pChnHandle->svcParam.fg_dealt_qp;
	pInitEncCfg->complex_scene_detect_en = pChnHandle->svcParam
											.complex_scene_detect_en;
	pInitEncCfg->complex_scene_low_th = pChnHandle->svcParam
										.complex_scene_low_th;
	pInitEncCfg->complex_scene_hight_th = pChnHandle->svcParam
										  .complex_scene_hight_th;

	pInitEncCfg->middle_min_percent = pChnHandle->svcParam
										.middle_min_percent;
	pInitEncCfg->complex_min_percent = pChnHandle->svcParam
										  .complex_min_percent;

	pInitEncCfg->smart_ai_en = pChnHandle->svcParam.smart_ai_en;

	CVI_VENC_TRACE("width = %d, height = %d, rcMode = %d\n",
		       pInitEncCfg->width, pInitEncCfg->height,
		       pInitEncCfg->rcMode);
	CVI_VENC_TRACE("es buf size = 0x%X, singleLumaBuf = %d\n",
		       pInitEncCfg->bitstreamBufferSize,
		       pInitEncCfg->singleLumaBuf);

	if (pEncCtx->ext.vid.setInitCfgRc)
		pEncCtx->ext.vid.setInitCfgRc(pInitEncCfg, pChnHandle);

	pEncCtx->ext.vid.pHandle = cviVEncOpen(pInitEncCfg);
	if (!pEncCtx->ext.vid.pHandle) {
		CVI_VENC_ERR("cviVEncOpen\n");
		return CVI_ERR_VENC_NULL_PTR;
	}

	return status;
}

static CVI_VOID cviSetInitCfgGop(cviInitEncConfig *pInitEncCfg,
				 venc_chn_context *pChnHandle)
{
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_GOP_ATTR_S *pga = &pChnAttr->stGopAttr;

	pInitEncCfg->virtualIPeriod = 0;

	if (pga->enGopMode == VENC_GOPMODE_NORMALP) {
		pInitEncCfg->s32IPQpDelta = pga->stNormalP.s32IPQpDelta;
		CVI_VENC_CFG("s32IPQpDelta = %d\n", pInitEncCfg->s32IPQpDelta);

	} else if (pga->enGopMode == VENC_GOPMODE_SMARTP) {
		pInitEncCfg->virtualIPeriod = pInitEncCfg->gop;
		pInitEncCfg->gop = pga->stSmartP.u32BgInterval;
		pInitEncCfg->s32BgQpDelta = pga->stSmartP.s32BgQpDelta;
		pInitEncCfg->s32ViQpDelta = pga->stSmartP.s32ViQpDelta;
		CVI_VENC_CFG(
			"virtualIPeriod = %d, gop = %d, bgQpDelta = %d, ViQpDelta = %d\n",
			pInitEncCfg->virtualIPeriod, pInitEncCfg->gop,
			pInitEncCfg->s32BgQpDelta, pInitEncCfg->s32ViQpDelta);
	}
}

static CVI_VOID h264e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H264_FIXQP_S *pstH264FixQp = &prcatt->stH264FixQp;

	pInitEncCfg->iqp = pstH264FixQp->u32IQp;
	pInitEncCfg->pqp = pstH264FixQp->u32PQp;
	pInitEncCfg->gop = pstH264FixQp->u32Gop;
	pInitEncCfg->framerate = (int)pstH264FixQp->fr32DstFrameRate;

	CVI_VENC_CFG("iqp = %d, pqp = %d, gop = %d, framerate = %d\n",
		     pInitEncCfg->iqp, pInitEncCfg->pqp, pInitEncCfg->gop,
		     pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H264_CBR_S *pstH264Cbr = &prcatt->stH264Cbr;

	pInitEncCfg->statTime = pstH264Cbr->u32StatTime;
	pInitEncCfg->gop = pstH264Cbr->u32Gop;
	pInitEncCfg->bitrate = pstH264Cbr->u32BitRate;
	pInitEncCfg->framerate = (int)pstH264Cbr->fr32DstFrameRate;

	CVI_VENC_CFG("statTime = %d, gop = %d, bitrate = %d, fps = %d\n",
		     pInitEncCfg->statTime, pInitEncCfg->gop,
		     pInitEncCfg->bitrate, pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H264_VBR_S *pstH264Vbr = &prcatt->stH264Vbr;
	VENC_RC_PARAM_S *prcparam = &pChnHandle->rcParam;

	pInitEncCfg->statTime = pstH264Vbr->u32StatTime;
	pInitEncCfg->gop = pstH264Vbr->u32Gop;
	pInitEncCfg->maxbitrate = pstH264Vbr->u32MaxBitRate;
	pInitEncCfg->framerate = (int)pstH264Vbr->fr32DstFrameRate;
	pInitEncCfg->s32ChangePos = prcparam->stParamH264Vbr.s32ChangePos;

	CVI_VENC_CFG(
		"statTime = %d, gop = %d, maxbitrate = %d, fps = %d, s32ChangePos = %d\n",
		pInitEncCfg->statTime, pInitEncCfg->gop,
		pInitEncCfg->maxbitrate, pInitEncCfg->framerate,
		pInitEncCfg->s32ChangePos);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
				     CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H264_AVBR_S *pstH264AVbr = &prcatt->stH264AVbr;
	VENC_PARAM_H264_AVBR_S *pprc = &pChnHandle->rcParam.stParamH264AVbr;

	pInitEncCfg->statTime = pstH264AVbr->u32StatTime;
	pInitEncCfg->gop = pstH264AVbr->u32Gop;
	pInitEncCfg->framerate = (int)pstH264AVbr->fr32DstFrameRate;
	pInitEncCfg->maxbitrate = pstH264AVbr->u32MaxBitRate;
	pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
	pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
	pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
	pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
	pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
	pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
	pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;
	CVI_VENC_CFG(
		"statTime = %d, gop = %d, maxbitrate = %d, fps = %d, s32ChangePos = %d\n",
		pInitEncCfg->statTime, pInitEncCfg->gop,
		pInitEncCfg->maxbitrate, pInitEncCfg->framerate,
		pInitEncCfg->s32ChangePos);
	CVI_VENC_CFG(
		"StillPercent = %d, StillQP = %d, MotionSensitivity = %d\n",
		pInitEncCfg->s32MinStillPercent, pInitEncCfg->u32MaxStillQP,
		pInitEncCfg->u32MotionSensitivity);
	CVI_VENC_CFG("FrmLostOpen = %d, FrmGap = %d, PureStillThr = %d\n",
		     pInitEncCfg->s32AvbrFrmLostOpen,
		     pInitEncCfg->s32AvbrFrmGap,
		     pInitEncCfg->s32AvbrPureStillThr);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H264_UBR_S *pstH264Ubr = &prcatt->stH264Ubr;

	pInitEncCfg->statTime = pstH264Ubr->u32StatTime;
	pInitEncCfg->gop = pstH264Ubr->u32Gop;
	pInitEncCfg->bitrate = pstH264Ubr->u32BitRate;
	pInitEncCfg->framerate = (int)pstH264Ubr->fr32DstFrameRate;

	CVI_VENC_CFG("statTime = %d, gop = %d, bitrate = %d, fps = %d\n",
		     pInitEncCfg->statTime, pInitEncCfg->gop,
		     pInitEncCfg->bitrate, pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_S32 h264e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 cviNalType)
{
	int h264naluType[] = {
		-1,
		H264E_NALU_ISLICE,
		H264E_NALU_PSLICE,
		H264E_NALU_BSLICE,
		H264E_NALU_IDRSLICE,
		H264E_NALU_SPS,
		H264E_NALU_PPS,
		H264E_NALU_SEI,
	};
	int naluType;

	if (!ppack) {
		CVI_VENC_ERR("ppack is NULL\n");
		return -1;
	}

	if (!ppack->pu8Addr) {
		CVI_VENC_ERR("ppack->pu8Addr is NULL\n");
		return -1;
	}

	naluType = ppack->pu8Addr[4] & 0x1f;

	if (cviNalType <= NAL_NONE || cviNalType >= NAL_VPS) {
		CVI_VENC_ERR("cviNalType = %d\n", cviNalType);
		return -1;
	}

	if (naluType == H264_NALU_TYPE_IDR)
		ppack->DataType.enH264EType = H264E_NALU_IDRSLICE;
	else
		ppack->DataType.enH264EType = h264naluType[cviNalType];

	CVI_VENC_BS("enH264EType = %d\n", ppack->DataType.enH264EType);

	return 0;
}

static CVI_VOID h265e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_FIXQP_S *pstH265FixQp = &prcatt->stH265FixQp;

	pInitEncCfg->iqp = pstH265FixQp->u32IQp;
	pInitEncCfg->pqp = pstH265FixQp->u32PQp;
	pInitEncCfg->gop = pstH265FixQp->u32Gop;
	pInitEncCfg->framerate = (int)pstH265FixQp->fr32DstFrameRate;

	CVI_VENC_CFG("iqp = %d, pqp = %d, gop = %d, framerate = %d\n",
		     pInitEncCfg->iqp, pInitEncCfg->pqp, pInitEncCfg->gop,
		     pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_CBR_S *pstH265Cbr = &prcatt->stH265Cbr;

	pInitEncCfg->statTime = pstH265Cbr->u32StatTime;
	pInitEncCfg->gop = pstH265Cbr->u32Gop;
	pInitEncCfg->bitrate = pstH265Cbr->u32BitRate;
	pInitEncCfg->framerate = (int)pstH265Cbr->fr32DstFrameRate;

	CVI_VENC_CFG("statTime = %d, gop = %d, bitrate = %d, fps = %d\n",
		     pInitEncCfg->statTime, pInitEncCfg->gop,
		     pInitEncCfg->bitrate, pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_VBR_S *pstH265Vbr = &prcatt->stH265Vbr;
	VENC_RC_PARAM_S *prcparam = &pChnHandle->rcParam;

	pInitEncCfg->statTime = pstH265Vbr->u32StatTime;
	pInitEncCfg->gop = pstH265Vbr->u32Gop;
	pInitEncCfg->maxbitrate = pstH265Vbr->u32MaxBitRate;
	pInitEncCfg->s32ChangePos = prcparam->stParamH265Vbr.s32ChangePos;
	pInitEncCfg->framerate = (int)pstH265Vbr->fr32DstFrameRate;

	CVI_VENC_CFG(
		"statTime = %d, gop = %d, maxbitrate = %d, fps = %d, s32ChangePos = %d\n",
		pInitEncCfg->statTime, pInitEncCfg->gop,
		pInitEncCfg->maxbitrate, pInitEncCfg->framerate,
		pInitEncCfg->s32ChangePos);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
				     CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_AVBR_S *pstH265AVbr = &prcatt->stH265AVbr;
	VENC_PARAM_H265_AVBR_S *pprc = &pChnHandle->rcParam.stParamH265AVbr;

	pInitEncCfg->statTime = pstH265AVbr->u32StatTime;
	pInitEncCfg->gop = pstH265AVbr->u32Gop;
	pInitEncCfg->framerate = (int)pstH265AVbr->fr32DstFrameRate;
	pInitEncCfg->maxbitrate = pstH265AVbr->u32MaxBitRate;
	pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
	pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
	pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
	pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
	pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
	pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
	pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;

	CVI_VENC_CFG(
		"statTime = %d, gop = %d, maxbitrate = %d, fps = %d, s32ChangePos = %d\n",
		pInitEncCfg->statTime, pInitEncCfg->gop,
		pInitEncCfg->maxbitrate, pInitEncCfg->framerate,
		pInitEncCfg->s32ChangePos);
	CVI_VENC_CFG(
		"StillPercent = %d, StillQP = %d, MotionSensitivity = %d\n",
		pInitEncCfg->s32MinStillPercent, pInitEncCfg->u32MaxStillQP,
		pInitEncCfg->u32MotionSensitivity);
	CVI_VENC_CFG("FrmLostOpen = %d, FrmGap = %d, PureStillThr = %d\n",
		     pInitEncCfg->s32AvbrFrmLostOpen,
		     pInitEncCfg->s32AvbrFrmGap,
		     pInitEncCfg->s32AvbrPureStillThr);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgQpMap(cviInitEncConfig *pInitEncCfg,
				      CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_QPMAP_S *pstH265QpMap = &prcatt->stH265QpMap;

	pInitEncCfg->statTime = pstH265QpMap->u32StatTime;
	pInitEncCfg->gop = pstH265QpMap->u32Gop;
	pInitEncCfg->bitrate =
		DEAULT_QP_MAP_BITRATE; // QpMap uses CBR as basic settings
	pInitEncCfg->framerate = (int)pstH265QpMap->fr32DstFrameRate;

	CVI_VENC_CFG("statTime = %d, gop = %d, bitrate = %d, fps = %d\n",
		     pInitEncCfg->statTime, pInitEncCfg->gop,
		     pInitEncCfg->bitrate, pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
				    CVI_VOID *pchnctx)
{
	venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
	VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
	VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
	VENC_H265_UBR_S *pstH265Ubr = &prcatt->stH265Ubr;

	pInitEncCfg->statTime = pstH265Ubr->u32StatTime;
	pInitEncCfg->gop = pstH265Ubr->u32Gop;
	pInitEncCfg->bitrate = pstH265Ubr->u32BitRate;
	pInitEncCfg->framerate = (int)pstH265Ubr->fr32DstFrameRate;

	CVI_VENC_CFG("statTime = %d, gop = %d, bitrate = %d, fps = %d\n",
		     pInitEncCfg->statTime, pInitEncCfg->gop,
		     pInitEncCfg->bitrate, pInitEncCfg->framerate);

	cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_S32 h265e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 cviNalType)
{
	int h265naluType[] = {
		-1,
		H265E_NALU_ISLICE,
		H265E_NALU_PSLICE,
		H265E_NALU_BSLICE,
		H265E_NALU_IDRSLICE,
		H265E_NALU_SPS,
		H265E_NALU_PPS,
		H265E_NALU_SEI,
		H265E_NALU_VPS,
	};
	int naluType;

	if (!ppack) {
		CVI_VENC_ERR("ppack is NULL\n");
		return -1;
	}

	if (!ppack->pu8Addr) {
		CVI_VENC_ERR("ppack->pu8Addr is NULL\n");
		return -1;
	}

	naluType = (ppack->pu8Addr[4] & 0x7f) >> 1;

	if (cviNalType <= NAL_NONE || cviNalType >= NAL_MAX) {
		CVI_VENC_ERR("cviNalType = %d\n", cviNalType);
		return -1;
	}

	if (naluType == H265_NALU_TYPE_W_RADL ||
	    naluType == H265_NALU_TYPE_N_LP)
		ppack->DataType.enH265EType = H265E_NALU_IDRSLICE;
	else
		ppack->DataType.enH265EType = h265naluType[cviNalType];

	CVI_VENC_BS("enH265EType = %d\n", ppack->DataType.enH265EType);

	return 0;
}

static CVI_S32 vidEnc_close(CVI_VOID *ctx)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

	if (pEncCtx->ext.vid.pHandle) {
		status = cviVEncClose(pEncCtx->ext.vid.pHandle);
		if (status < 0) {
			CVI_VENC_ERR("cviVEncClose, status = %d\n", status);
			return status;
		}
	}

	return status;
}

static CVI_S32 vidEnc_enc_one_pic(CVI_VOID *ctx,
				  const VIDEO_FRAME_INFO_S *pstFrame,
				  CVI_S32 s32MilliSec)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
	cviEncOnePicCfg encOnePicCfg;
	cviEncOnePicCfg *pPicCfg = &encOnePicCfg;
	CVIFRAMEBUF srcInfo, *psi = &srcInfo;
	struct vb_s *blk = (struct vb_s *)pstFrame->stVFrame.pPrivateData;
	CVI_U8 *mtable = kzalloc(MO_TBL_SIZE, GFP_ATOMIC);

	if (!mtable) {
		CVI_VENC_SRC("fail to kzalloc(%d)\n", MO_TBL_SIZE);
		return CVI_FAILURE;
	}

	memset(pPicCfg, 0, sizeof(cviEncOnePicCfg));
	setSrcInfo(psi, pEncCtx, pstFrame);

	pPicCfg->addrY = psi->vbY.virt_addr;
	pPicCfg->addrCb = psi->vbCb.virt_addr;
	pPicCfg->addrCr = psi->vbCr.virt_addr;

	pPicCfg->phyAddrY = psi->vbY.phys_addr;
	pPicCfg->phyAddrCb = psi->vbCb.phys_addr;
	pPicCfg->phyAddrCr = psi->vbCr.phys_addr;
	pPicCfg->u64Pts = pstFrame->stVFrame.u64PTS;

	pPicCfg->stride = psi->strideY;
	switch (pstFrame->stVFrame.enPixelFormat) {
	case PIXEL_FORMAT_NV12:
		pPicCfg->cbcrInterleave = 1;
		pPicCfg->nv21 = 0;
		break;
	case PIXEL_FORMAT_NV21:
		pPicCfg->cbcrInterleave = 1;
		pPicCfg->nv21 = 1;
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
	default:
		pPicCfg->cbcrInterleave = 0;
		pPicCfg->nv21 = 0;
		break;
	}

	if (pEncCtx->base.rcMode <= RC_MODE_AVBR) {
		if (blk) {
			pPicCfg->picMotionLevel = blk->buf.motion_lv;
			pPicCfg->picDciLv = 0;
			pPicCfg->picMotionMap = mtable;
			pPicCfg->picMotionMapSize = MO_TBL_SIZE;
			memcpy(mtable, blk->buf.motion_table, MO_TBL_SIZE);
		}
		pPicCfg->picMotionMap = mtable;
		cviCopyMotionMap(pEncCtx->ext.vid.pHandle, pPicCfg, pEncCtx->ext.vid.pHandle);
	}

	CVI_VENC_SRC("pa Y = 0x%llX, pa Cb = 0x%llX, pa Cr = 0x%llX\n",
		     pPicCfg->phyAddrY, pPicCfg->phyAddrCb, pPicCfg->phyAddrCr);
	CVI_VENC_SRC("MLevel = %d, MSize = %d\n", pPicCfg->picMotionLevel,
		     pPicCfg->picMotionMapSize);

	status = cviVEncEncOnePic(pEncCtx->ext.vid.pHandle, pPicCfg,
				  s32MilliSec);
	kfree(mtable);

	if (status == ENC_TIMEOUT)
		return CVI_ERR_VENC_BUSY;

	return status;
}

// should mutex psp->packMutex in caller
static CVI_S32 drop_all_packs(stStreamPack *psp)
{
	CVI_U32 idx = 0;
	stPack *pPack;

	if (psp->totalPacks) {
		for (idx = 0; idx < psp->totalPacks && (idx < MAX_NUM_PACKS); idx++) {
			pPack = &psp->pack[idx];

			if (pPack->addr && pPack->need_free) {
				if (pPack->cviNalType >= NAL_I && pPack->cviNalType <= NAL_IDR) {
					osal_ion_free(pPack->addr);
				} else {
					osal_kfree(pPack->addr);
				}
				pPack->addr = NULL;
			}
			pPack->len = 0;
			pPack->bUsed = CVI_FALSE;
		}
		psp->totalPacks = 0;
		psp->dropCnt++;
		psp->seq++;
	}

	return 0;
}


static CVI_S32 vidEnc_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
				 CVI_S32 s32MilliSec)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
	cviVEncStreamInfo *pStreamInfo = &pEncCtx->ext.vid.streamInfo;
	stStreamPack *psp;
	VENC_PACK_S *ppack;
	CVI_U32 idx = 0;
	status = cviVEncGetStream(pEncCtx->ext.vid.pHandle, pStreamInfo,
				  s32MilliSec);
	CVI_VENC_INFO("cviVEncGetStream status %d\n", status);
	if (status == ENC_TIMEOUT) {
		return CVI_ERR_VENC_BUSY;
	} else if (status) {
		CVI_VENC_ERR("get stream failed,status %d\n", status);
		return CVI_ERR_VENC_INVALILD_RET;
	}

	psp = pStreamInfo->psp;
	if (!psp) {
		CVI_VENC_ERR("psp is null\n");
		return CVI_ERR_VENC_NULL_PTR;
	}

	MUTEX_LOCK(&psp->packMutex);
	pstStream->u32PackCount = 0;
	for (idx = 0; (idx < psp->totalPacks) && (idx < MAX_NUM_PACKS); idx++) {
		ppack = &pstStream->pstPack[idx];

		if (!ppack) {
			CVI_VENC_ERR("get NULL pack, uPackCount:%d idx:%d\n", pstStream->u32PackCount, idx);
			break;
		}
		memset(ppack, 0, sizeof(VENC_PACK_S));

		if (!psp->pack[idx].addr || !psp->pack[idx].len || !psp->pack[idx].u64PhyAddr) {
			CVI_VENC_ERR("packs error, idx:%d addr:%p, len:%d phy:%llx\n",
				idx, psp->pack[idx].addr, psp->pack[idx].len, psp->pack[idx].u64PhyAddr);
				drop_all_packs(psp);
				MUTEX_UNLOCK(&psp->packMutex);
				return CVI_ERR_VENC_ILLEGAL_PARAM;
		}

		psp->pack[idx].bUsed = CVI_TRUE;
		ppack->u64PhyAddr = psp->pack[idx].u64PhyAddr;
		ppack->pu8Addr = psp->pack[idx].addr;
		ppack->u32Len = psp->pack[idx].len;
		ppack->u64PTS = psp->pack[idx].u64PTS;
		status = pEncCtx->ext.vid.mapNaluType(
			ppack, psp->pack[idx].cviNalType);
		if (status) {
			CVI_VENC_ERR("mapNaluType, status = %d, idx:%d\n", status, idx);
			drop_all_packs(psp);
			MUTEX_UNLOCK(&psp->packMutex);
			return status;
		}

		pstStream->u32PackCount++;

		// division [P] + [P]
		// division [P] + [VPS SPS PPS I]
		// division [SEI P] + [VPS SPS PPS SEI I]
		if (psp->pack[idx].cviNalType == NAL_P
			|| psp->pack[idx].cviNalType == NAL_I
			|| psp->pack[idx].cviNalType == NAL_IDR) {
			break;
		}
	}
	pstStream->u32Seq = psp->seq;
	psp->seq++;
	MUTEX_UNLOCK(&psp->packMutex);
	pEncCtx->base.u64EncHwTime = (CVI_U64)pStreamInfo->encHwTime;

	return status;
}

static CVI_S32 vidEnc_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
	cviVEncStreamInfo *pStreamInfo = &pEncCtx->ext.vid.streamInfo;

	UNREFERENCED_PARAM(pstStream);

	status = cviVEncReleaseStream(pEncCtx->ext.vid.pHandle, pStreamInfo);
	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("cviVEncReleaseStream, status = %d\n", status);
		return status;
	}

	return status;
}

static CVI_S32 vidEnc_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg)
{
	CVI_S32 status = CVI_SUCCESS;
	venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
	CVI_S32 currOp;

	currOp = (op & CVI_H26X_OP_MASK) >> CVI_H26X_OP_SHIFT;
	if (currOp == 0) {
		CVI_VENC_TRACE("op = 0x%X, currOp = 0x%X\n", op, currOp);
		return 0;
	}

	status = cviVEncIoctl(pEncCtx->ext.vid.pHandle, currOp, arg);
	if (status != CVI_SUCCESS) {
		CVI_VENC_ERR("cviVEncIoctl, currOp = 0x%X, status = %d\n",
			     currOp, status);
		return status;
	}

	return status;
}
