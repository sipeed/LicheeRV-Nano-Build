//------------------------------------------------------------------------------
// File: main.c
//
// Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
//------------------------------------------------------------------------------
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <uapi/linux/sched/types.h>

#include "module_common.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "cvi_vcodec_version.h"
#include "main_helper.h"
#include "vpuapi.h"
#include "main_enc_cvitest.h"
#include "cvitest_internal.h"
#include "cvi_enc_internal.h"
#include "coda9/coda9.h"
#include <base_cb.h>
#include <vi_cb.h>
#include <vpss_cb.h>
#include <vdi.h>
#include "product.h"
#include "../cvi/cvi_enc_rc.h"
#include "cvi_vcodec_lib.h"
#include <linux/kthread.h>
#include "cvi_vc_drv.h"
#include "cvi_vc_getopt.h"

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif

#ifdef CLI_DEBUG_SUPPORT
#include "tcli.h"
#endif

#define STREAM_READ_SIZE (512 * 16)
#define STREAM_READ_ALL_SIZE (0)
#define STREAM_BUF_MIN_SIZE (0x18000)

#define STREAM_BUF_SIZE 0x400000 // max bitstream size (4MB)
#define WAVE4_ENC_REPORT_SIZE 144
#define INIT_TEST_ENCODER_OK 10
#define YUV_NAME "../img/3-1920x1080.yuv"
#define BS_NAME "CVISDK"

#define SECONDARY_AXI_H264 0xf
#define SECONDARY_AXI_H265 0x7
#define TIME_BLOCK_MODE (-1)
#define RET_VCODEC_TIMEOUT (-2)

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

typedef enum _E_VENC_VB_SOURCE {
	VB_SOURCE_TYPE_COMMON = 0,
	VB_SOURCE_TYPE_PRIVATE = 2,
	VB_SOURCE_TYPE_USER = 3
} E_VENC_VB_SOURCE;

#define COMP(a, b)                                                             \
	do {                                                                   \
		if ((a) != (b)) {                                              \
			CVI_VC_ERR("0x%08X != 0x%X\n", (a), (b));              \
		}                                                              \
	} while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)    (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef ROUND_UP_N_BIT
#define ROUND_UP_N_BIT(val, bit)	((((val) + (1 << (bit)) - 1) >> (bit)) << (bit))
#endif

#if CFG_MEM
DRAM_CFG dramCfg;
#endif

#if USE_KERNEL_MODE
extern CVI_U32 err_frm_skip[VENC_MAX_CHN_NUM];
#endif

#define REPEAT_SPS 1

int gNumInstance = 1;

static char optYuvPath[MAX_FILE_PATH];
#ifdef VC_DRIVER_TEST
char gCfgFileName[MAX_NUM_INSTANCE][MAX_FILE_PATH];
#endif

static int initMcuEnv(TestEncConfig *pEncConfig);
static int initEncOneFrame(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig);
static void cviDumpEncConfig(TestEncConfig *pEncConfig);
#ifndef VC_DEBUG_BASIC_LEVEL
static void cviDumpRcAttr(TestEncConfig *pEncConfig);
static void cviDumpRcParam(TestEncConfig *pEncConfig);
static void cviDumpCodingParam(TestEncConfig *pEncConfig);
static void cviDumpGopMode(TestEncConfig *pEncConfig);
#endif
#ifdef SUPPORT_DONT_READ_STREAM
static int updateBitstream(vpu_buffer_t *vb, Int32 size);
#endif
static int cviEncodeOneFrame(stTestEncoder *pTestEnc);
/*
static void RcChangeSetParameter(EncHandle handle, TestEncConfig encConfig,
				 EncOpenParam *encOP, EncParam *encParam,
				 Uint32 frameIdx, Uint32 fieldDone,
				 int field_flag, vpu_buffer_t vbRoi); */
static void cviPicParamChangeCtrl(EncHandle handle, TestEncConfig *pEncConfig,
				  EncOpenParam *pEncOP, EncParam *encParam,
				  Uint32 frameIdx);

#ifdef VC_DRIVER_TEST
static int cviGetStream(stTestEncoder *pTestEnc);
#endif
static int cviCheckOutputInfo(stTestEncoder *pTestEnc);
static int cviCheckEncodeEnd(stTestEncoder *pTestEnc);
static int cviCheckAndCompareBitstream(stTestEncoder *pTestEnc);
static int cviGetEncodedInfo(stTestEncoder *pTestEnc, int s32MilliSec);
static void cviCloseVpuEnc(stTestEncoder *pTestEnc);
static void cviDeInitVpuEnc(stTestEncoder *pTestEnc);
static void *cviInitEncoder(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig);
static int cviGetOneStream(void *handle, cviVEncStreamInfo *pStreamInfo,
			   int s32MilliSec);
static int cviCheckSuperFrame(stTestEncoder *pTestEnc);
static int cviProcessSuperFrame(stTestEncoder *pTestEnc,
				cviVEncStreamInfo *pStreamInfo,
				int s32MilliSec);
static int cviReEncodeIDR(stTestEncoder *pTestEnc,
			  cviVEncStreamInfo *pStreamInfo, int s32MilliSec);
#ifdef VC_DRIVER_TEST
static int parseArgs(int argc, char **argv, TestEncConfig *pEncConfig);
#endif
static int checkEncConfig(TestEncConfig *pEncConfig, Uint32 productId);
#ifdef VC_DRIVER_TEST
static int initEncConfigByArgv(int argc, char **argv, TestEncConfig *pEncConfig);
#endif
static void initEncConfigByCtx(TestEncConfig *pEncConfig,
		cviInitEncConfig * pInitEncCfg);
static int cviVEncGetVbInfo(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSetVbBuffer(stTestEncoder *pTestEnc, void *arg);
static int _cviVencCalculateBufInfo(stTestEncoder *pTestEnc, int *butcnt,
				    int *bufsize);
static int cviVEncRegReconBuf(stTestEncoder *pTestEnc, void *arg);
static int _cviVEncRegFrameBuffer(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSetInPixelFormat(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSetFrameParam(stTestEncoder *pTestEnc, void *arg);
static int cviVEncCalcFrameParam(stTestEncoder *pTestEnc, void *arg);
#if 0
static int cviVEncSetSbMode(stTestEncoder *pTestEnc, void *arg);
static int cviVEncStartSbMode(stTestEncoder *pTestEnc, void *arg);
static int cviVEncUpdateSbWptr(stTestEncoder *pTestEnc, void *arg);
static int cviVEncResetSb(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSbEnDummyPush(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSbTrigDummyPush(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSbDisDummyPush(stTestEncoder *pTestEnc, void *arg);
static int cviVEncSbGetSkipFrmStatus(stTestEncoder *pTestEnc, void *arg);
#endif
#if CFG_MEM
static int checkDramCfg(DRAM_CFG *pDramCfg);
#endif
static int pfnWaitEncodeDone(void *param);
static CVI_S32 cviSetSbSetting(cviVencSbSetting *pstSbSetting);

extern void cvi_VENC_SBM_IrqEnable(void);


void cviVcodecGetVersion(void)
{
	CVI_VC_INFO("VCODEC_VERSION = %s\n", VCODEC_VERSION);
}
#ifdef VC_DRIVER_TEST
static void help(struct OptionExt *opt, const char *programName)
{
	int i;

	CVI_VC_INFO(
		"------------------------------------------------------------------------------\n");
	CVI_VC_INFO("%s(API v%d.%d.%d)\n", programName, API_VERSION_MAJOR,
		    API_VERSION_MINOR, API_VERSION_PATCH);
	CVI_VC_INFO("\tAll rights reserved by Chips&Media(C)\n");
	CVI_VC_INFO("\tSample program controlling the Chips&Media VPU\n");
	CVI_VC_INFO(
		"------------------------------------------------------------------------------\n");
	CVI_VC_INFO("%s [option]\n", programName);
	CVI_VC_INFO("-h                          help\n");
	CVI_VC_INFO("-n [num]                    output frame number\n");
	CVI_VC_INFO("-v                          print version information\n");
	CVI_VC_INFO(
		"-c                          compare with golden bitstream\n");

	for (i = 0; i < MAX_GETOPT_OPTIONS; i++) {
		if (opt[i].name == NULL)
			break;
		CVI_VC_INFO("%s", opt[i].help);
	}
}
#endif
static void cviDumpEncConfig(TestEncConfig *pEncConfig)
{
#ifndef VC_DEBUG_BASIC_LEVEL
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	cviDumpRcAttr(pEncConfig);
	cviDumpRcParam(pEncConfig);
	cviDumpCodingParam(pEncConfig);
	cviDumpGopMode(pEncConfig);

	CVI_VC_CFG("enSuperFrmMode = %d\n", pCviEc->enSuperFrmMode);
	CVI_VC_CFG("cviRcEn = %d\n", pCviEc->cviRcEn);
	CVI_VC_CFG("rotAngle %d\n", pEncConfig->rotAngle);
	CVI_VC_CFG("mirDir %d\n", pEncConfig->mirDir);
	CVI_VC_CFG("useRot %d\n", pEncConfig->useRot);
	CVI_VC_CFG("qpReport %d\n", pEncConfig->qpReport);
	CVI_VC_CFG("ringBufferEnable %d\n", pEncConfig->ringBufferEnable);
	CVI_VC_CFG("rcIntraQp %d\n", pEncConfig->rcIntraQp);
	CVI_VC_CFG("outNum %d\n", pEncConfig->outNum);
	CVI_VC_CFG("instNum %d\n", pEncConfig->instNum);
	CVI_VC_CFG("coreIdx %d\n", pEncConfig->coreIdx);
	CVI_VC_CFG("mapType %d\n", pEncConfig->mapType);
	CVI_VC_CFG("lineBufIntEn %d\n", pEncConfig->lineBufIntEn);
	CVI_VC_CFG("bEsBufQueueEn %d\n", pEncConfig->bEsBufQueueEn);
	CVI_VC_CFG("en_container %d\n", pEncConfig->en_container);
	CVI_VC_CFG("container_frame_rate %d\n",
		   pEncConfig->container_frame_rate);
	CVI_VC_CFG("picQpY %d\n", pEncConfig->picQpY);
	CVI_VC_CFG("cbcrInterleave %d\n", pEncConfig->cbcrInterleave);
	CVI_VC_CFG("nv21 %d\n", pEncConfig->nv21);
	CVI_VC_CFG("needSourceConvert %d\n", pEncConfig->needSourceConvert);
	CVI_VC_CFG("packedFormat %d\n", pEncConfig->packedFormat);
	CVI_VC_CFG("srcFormat %d\n", pEncConfig->srcFormat);
	CVI_VC_CFG("srcFormat3p4b %d\n", pEncConfig->srcFormat3p4b);
	CVI_VC_CFG("decodingRefreshType %d\n", pEncConfig->decodingRefreshType);
	CVI_VC_CFG("tempLayer %d\n", pEncConfig->tempLayer);
	CVI_VC_CFG("useDeriveLambdaWeight %d\n",
		   pEncConfig->useDeriveLambdaWeight);
	CVI_VC_CFG("dynamicMergeEnable %d\n", pEncConfig->dynamicMergeEnable);
	CVI_VC_CFG("independSliceMode %d\n", pEncConfig->independSliceMode);
	CVI_VC_CFG("independSliceModeArg %d\n",
		   pEncConfig->independSliceModeArg);
	CVI_VC_CFG("RcEnable %d\n", pEncConfig->RcEnable);
	CVI_VC_CFG("bitdepth %d\n", pEncConfig->bitdepth);
	CVI_VC_CFG("secondary_axi %d\n", pEncConfig->secondary_axi);
	CVI_VC_CFG("stream_endian %d\n", pEncConfig->stream_endian);
	CVI_VC_CFG("frame_endian %d\n", pEncConfig->frame_endian);
	CVI_VC_CFG("source_endian %d\n", pEncConfig->source_endian);
	CVI_VC_CFG("compare_type %d\n", pEncConfig->compare_type);
	CVI_VC_CFG("yuv_mode %d\n", pEncConfig->yuv_mode);
	CVI_VC_CFG("loopCount %d\n", pEncConfig->loopCount);
	CVI_VC_CFG("bIsoSendFrmEn %d\n", pEncConfig->bIsoSendFrmEn);
#endif
}

#ifndef VC_DEBUG_BASIC_LEVEL
static void cviDumpRcAttr(TestEncConfig *pEncConfig)
{

	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	CVI_VC_CFG("stdMode %d\n", pEncConfig->stdMode);
	CVI_VC_CFG("picWidth %d\n", pEncConfig->picWidth);
	CVI_VC_CFG("picHeight %d\n", pEncConfig->picHeight);
	CVI_VC_CFG("rcMode %d\n", pEncConfig->rcMode);
	if (pCviEc->virtualIPeriod) {
		CVI_VC_CFG("gop %d\n", pCviEc->virtualIPeriod);
	} else {
		CVI_VC_CFG("gop %d\n", pCviEc->gop);
	}
	CVI_VC_CFG("framerate %d\n", pCviEc->framerate);
	CVI_VC_CFG("statTime %d\n", pCviEc->statTime);
	CVI_VC_CFG("bitrate %d\n", pCviEc->bitrate);
	CVI_VC_CFG("maxbitrate %d\n", pCviEc->maxbitrate);
	CVI_VC_CFG("iqp %d\n", pCviEc->iqp);
	CVI_VC_CFG("pqp %d\n", pCviEc->pqp);
}

static void cviDumpRcParam(TestEncConfig *pEncConfig)
{
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	CVI_VC_CFG("RowQpDelta %d\n", pCviEc->u32RowQpDelta);
	CVI_VC_CFG("ThrdLv %d\n", pCviEc->u32ThrdLv);
	CVI_VC_CFG("firstFrmstartQp %d\n", pCviEc->firstFrmstartQp);
	CVI_VC_CFG("initialDelay %d\n", pCviEc->initialDelay);
	CVI_VC_CFG("MaxIprop %d\n", pCviEc->u32MaxIprop);
	CVI_VC_CFG("MaxQp %d, MinQp %d\n", pCviEc->u32MaxQp, pCviEc->u32MinQp);
	CVI_VC_CFG("MaxIQp %d, MinIQp %d\n", pCviEc->u32MaxIQp,
		   pCviEc->u32MinIQp);
	CVI_VC_CFG("changePos %d\n", pEncConfig->changePos);
	CVI_VC_CFG("StillPercent %d\n", pCviEc->s32MinStillPercent);
	CVI_VC_CFG("StillQP %d\n", pCviEc->u32MaxStillQP);
	CVI_VC_CFG("MotionSensitivity %d\n", pCviEc->u32MotionSensitivity);
	CVI_VC_CFG("AvbrPureStillThr %d\n", pCviEc->s32AvbrPureStillThr);
	CVI_VC_CFG("AvbrFrmLostOpen %d\n", pCviEc->s32AvbrFrmLostOpen);
}

static void cviDumpCodingParam(TestEncConfig *pEncConfig)
{
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	CVI_VC_CFG("frmLostOpen %d\n", pEncConfig->frmLostOpen);
	CVI_VC_CFG("frameSkipBufThr %d\n", pCviEc->frmLostBpsThr);
	CVI_VC_CFG("encFrmGaps %d\n", pCviEc->encFrmGaps);
	CVI_VC_CFG("IntraCost %d\n", pCviEc->u32IntraCost);
	CVI_VC_CFG("chroma_qp_index_offset %d\n",
		   pCviEc->h264Trans.chroma_qp_index_offset);
	CVI_VC_CFG("cb_qp_offset %d\n", pCviEc->h265Trans.cb_qp_offset);
	CVI_VC_CFG("cr_qp_offset %d\n", pCviEc->h265Trans.cr_qp_offset);
	CVI_VC_CFG("slice_split %d\n", pCviEc->h265Split.bSplitEnable);
}

static void cviDumpGopMode(TestEncConfig *pEncConfig)
{
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	CVI_VC_CFG("IPQpDelta %d\n", pCviEc->s32IPQpDelta);
	if (pCviEc->virtualIPeriod) {
		CVI_VC_CFG("BgInterval %d\n", pCviEc->gop);
	}
}
#endif

static int _updateEncFrameInfo(stTestEncoder *pTestEnc,
			       FrameBufferFormat *psrcFrameFormat)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	// width = 8-aligned (CU unit)
	pTestEnc->srcFrameWidth = VPU_ALIGN8(pTestEnc->encOP.picWidth);
	CVI_VC_CFG("srcFrameWidth = %d\n", pTestEnc->srcFrameWidth);

	// height = 8-aligned (CU unit)
	pTestEnc->srcFrameHeight = VPU_ALIGN8(pTestEnc->encOP.picHeight);
	CVI_VC_CFG("srcFrameHeight = %d\n", pTestEnc->srcFrameHeight);

	// stride should be a 32-aligned.
	pTestEnc->srcFrameStride = VPU_ALIGN32(pTestEnc->encOP.picWidth);

	if (strlen(pTestEnc->encConfig.cfgFileName) != 0) {
		if (pTestEnc->encOP.srcBitDepth == 8) {
			pTestEnc->encConfig.srcFormat = FORMAT_420;
		} else if (pTestEnc->encOP.srcBitDepth == 10) {
			pTestEnc->encConfig.srcFormat =
				FORMAT_420_P10_16BIT_LSB;
			if (pTestEnc->encConfig.yuv_mode ==
			    YUV_MODE_YUV_LOADER) {
				CVI_VC_INFO("Need to check YUV style.\n");
				pTestEnc->encConfig.srcFormat =
					FORMAT_420_P10_16BIT_MSB;
			}
			if (pTestEnc->encConfig.srcFormat3p4b == 1) {
				pTestEnc->encConfig.srcFormat =
					FORMAT_420_P10_32BIT_MSB;
			}
			if (pTestEnc->encConfig.srcFormat3p4b == 2) {
				pTestEnc->encConfig.srcFormat =
					FORMAT_420_P10_32BIT_LSB;
			}
		}
	}

	if (pTestEnc->encConfig.packedFormat >= PACKED_YUYV)
		pTestEnc->encConfig.srcFormat = FORMAT_422;

	CVI_VC_TRACE("packedFormat = %d\n", pTestEnc->encConfig.packedFormat);

	if (pTestEnc->encConfig.srcFormat == FORMAT_422 &&
	    pTestEnc->encConfig.packedFormat >= PACKED_YUYV) {
		int p10bits = pTestEnc->encConfig.srcFormat3p4b == 0 ? 16 : 32;
		FrameBufferFormat srcFormat =
			GetPackedFormat(pTestEnc->encOP.srcBitDepth,
					pTestEnc->encConfig.packedFormat,
					p10bits, 1);

		if (srcFormat == (FrameBufferFormat)-1) {
			CVI_VC_ERR("fail to GetPackedFormat\n");
			return TE_ERR_ENC_INIT;
		}
		pTestEnc->encOP.srcFormat = srcFormat;
		*psrcFrameFormat = srcFormat;
		pTestEnc->encOP.nv21 = 0;
		pTestEnc->encOP.cbcrInterleave = 0;
	} else {
		pTestEnc->encOP.srcFormat = pTestEnc->encConfig.srcFormat;
		*psrcFrameFormat = pTestEnc->encConfig.srcFormat;
		pTestEnc->encOP.nv21 = pTestEnc->encConfig.nv21;
	}
	pTestEnc->encOP.packedFormat = pTestEnc->encConfig.packedFormat;

	CVI_VC_TRACE("packedFormat = %d\n", pTestEnc->encConfig.packedFormat);

	if (pEncOP->bitstreamFormat == STD_AVC) {
		if (pEncCfg->rotAngle == 90 || pEncCfg->rotAngle == 270) {
			pTestEnc->framebufWidth = pEncOP->picHeight;
			pTestEnc->framebufHeight = pEncOP->picWidth;
		} else {
			pTestEnc->framebufWidth = pEncOP->picWidth;
			pTestEnc->framebufHeight = pEncOP->picHeight;
		}

		pTestEnc->framebufWidth = VPU_ALIGN16(pTestEnc->framebufWidth);

		// To cover interlaced picture
		pTestEnc->framebufHeight =
			VPU_ALIGN32(pTestEnc->framebufHeight);

	} else {
		pTestEnc->framebufWidth = VPU_ALIGN8(pTestEnc->encOP.picWidth);
		pTestEnc->framebufHeight =
			VPU_ALIGN8(pTestEnc->encOP.picHeight);

		if ((pTestEnc->encConfig.rotAngle != 0 ||
		     pTestEnc->encConfig.mirDir != 0) &&
		    !(pTestEnc->encConfig.rotAngle == 180 &&
		      pTestEnc->encConfig.mirDir == MIRDIR_HOR_VER)) {
			pTestEnc->framebufWidth =
				VPU_ALIGN32(pTestEnc->encOP.picWidth);
			pTestEnc->framebufHeight =
				VPU_ALIGN32(pTestEnc->encOP.picHeight);
		}

		if (pTestEnc->encConfig.rotAngle == 90 ||
		    pTestEnc->encConfig.rotAngle == 270) {
			pTestEnc->framebufWidth =
				VPU_ALIGN32(pTestEnc->encOP.picHeight);
			pTestEnc->framebufHeight =
				VPU_ALIGN32(pTestEnc->encOP.picWidth);
		}
	}

	return RETCODE_SUCCESS;
}

static int initEncOneFrame(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	cviEncCfg *pcviEc = &pEncCfg->cviEc;
	cviCapability cap, *pCap = &cap;
	MirrorDirection mirrorDirection;
	FrameBufferFormat srcFrameFormat;
	int FrameBufSize;
	int addrremapsize[2];
	int remapsizeInPage[2];
	int extraSizeInPage[2];
	int framebufStride = 0;
	int productId;
	int numExtraLine;
	RetCode ret = RETCODE_SUCCESS;
	int i, mapType;
	int coreIdx;
	int instIdx;
	Uint64 start_time, end_time;

#if defined(CVI_H26X_USE_ION_MEM)
#if defined(BITSTREAM_ION_CACHED_MEM)
	int bBsStreamCached = 1;
#else
	int bBsStreamCached = 0;
#endif
#endif

	if (pTestEnc->bIsEncoderInited) {
		CVI_VC_FLOW("is already init.\n");
		return INIT_TEST_ENCODER_OK;
	}

	memset(pTestEnc, 0, sizeof(stTestEncoder));

	pTestEnc->bsBufferCount = 1;
	pTestEnc->bsQueueIndex = 1;
	pTestEnc->srcFrameIdx = 0;
	pTestEnc->frameIdx = 0;
	pTestEnc->framebufWidth = 0;
	pTestEnc->framebufHeight = 0;
	pTestEnc->regFrameBufCount = 0;
	pTestEnc->comparatorBitStream = NULL;
	pTestEnc->bsReader = NULL;
	pTestEnc->yuvFeeder = NULL;

	osal_memcpy(pEncCfg, pEncConfig, sizeof(TestEncConfig));
	osal_memset(&pTestEnc->yuvFeederInfo, 0x00, sizeof(YuvInfo));
	osal_memset(&pTestEnc->fbSrc[0], 0x00, sizeof(pTestEnc->fbSrc));
	osal_memset(&pTestEnc->fbRecon[0], 0x00, sizeof(pTestEnc->fbRecon));
	osal_memset(pTestEnc->vbReconFrameBuf, 0x00,
		    sizeof(pTestEnc->vbReconFrameBuf));
	osal_memset(pTestEnc->vbSourceFrameBuf, 0x00,
		    sizeof(pTestEnc->vbSourceFrameBuf));
	osal_memset(pEncOP, 0x00, sizeof(EncOpenParam));
	osal_memset(&pTestEnc->encParam, 0x00, sizeof(EncParam));
	osal_memset(&pTestEnc->initialInfo, 0x00, sizeof(EncInitialInfo));
	osal_memset(&pTestEnc->outputInfo, 0x00, sizeof(EncOutputInfo));
	osal_memset(&pTestEnc->secAxiUse, 0x00, sizeof(SecAxiUse));
	osal_memset(pTestEnc->vbStream, 0x00, sizeof(pTestEnc->vbStream));
	osal_memset(&pTestEnc->vbReport, 0x00, sizeof(vpu_buffer_t));
	osal_memset(&pTestEnc->encHeaderParam, 0x00, sizeof(EncHeaderParam));
	osal_memset(pTestEnc->vbRoi, 0x00, sizeof(pTestEnc->vbRoi));
	osal_memset(pTestEnc->vbCtuQp, 0x00, sizeof(pTestEnc->vbCtuQp));
	osal_memset(pTestEnc->vbPrefixSeiNal, 0x00,
		    sizeof(pTestEnc->vbPrefixSeiNal));
	osal_memset(pTestEnc->vbSuffixSeiNal, 0x00,
		    sizeof(pTestEnc->vbSuffixSeiNal));
	osal_memset(&pTestEnc->vbVuiRbsp, 0x00, sizeof(vpu_buffer_t));
	osal_memset(&pTestEnc->vbHrdRbsp, 0x00, sizeof(vpu_buffer_t));
#ifdef TEST_ENCODE_CUSTOM_HEADER
	osal_memset(&pTestEnc->vbSeiNal, 0x00, sizeof(pTestEnc->vbSeiNal));
	osal_memset(&pTestEnc->vbCustomHeader, 0x00, sizeof(vpu_buffer_t));
#endif
	osal_memset(&pTestEnc->streamPack, 0x00, sizeof(pTestEnc->streamPack));

	MUTEX_INIT(&pTestEnc->streamPack.packMutex, 0);

	instIdx = pEncCfg->instNum;
	coreIdx = pEncCfg->coreIdx;

	pTestEnc->coreIdx = coreIdx;
	pTestEnc->productId = VPU_GetProductId(coreIdx);

	start_time = cviGetCurrentTime();

	ret = VPU_InitWithBitcode(coreIdx, pEncCfg->pusBitCode,
				  pEncCfg->sizeInWord);
	if (ret != RETCODE_CALLED_BEFORE && ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"Failed to boot up VPU(coreIdx: %d, productId: %d)\n",
			coreIdx, pTestEnc->productId);
#ifdef ENABLE_CNM_DEBUG_MSG
		PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
		return TE_ERR_ENC_INIT;
	}
#ifndef FIRMWARE_H
	if (pTestEnc->encConfig.pusBitCode) {
		osal_free(pTestEnc->encConfig.pusBitCode);
		pTestEnc->encConfig.pusBitCode = NULL;
		pTestEnc->encConfig.sizeInWord = 0;
	}
#endif

	end_time = cviGetCurrentTime();
	CVI_VC_PERF("VPU_InitWithBitcode = %llu us\n", end_time - start_time);

	PrintVpuVersionInfo(coreIdx);

	ret = (RetCode)vdi_set_single_es_buf(coreIdx, pcviEc->bSingleEsBuf,
				    &pcviEc->bitstreamBufferSize);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("set_single_es_buf fail with ret %d\n", ret);
		return TE_ERR_ENC_INIT;
	}

	pEncOP->bitstreamFormat = pEncCfg->stdMode;
	mapType = (pEncCfg->mapType & 0x0f);
#ifdef VC_DRIVER_TEST
	strcpy(pEncCfg->cfgFileName, gCfgFileName[pEncCfg->instNum]);
	memset(gCfgFileName[pEncCfg->instNum], 0, MAX_FILE_PATH);

	CVI_VC_TRACE("set encConfig, cfgFileName[%d] = %s\n", pEncCfg->instNum,
		     pEncCfg->cfgFileName);

	if (strlen(pEncCfg->cfgFileName) != 0) {
		ret = (RetCode) GetEncOpenParam(pEncOP, pEncCfg, NULL,
				      &pTestEnc->encParam);
	} else
#endif
	{
		ret = (RetCode) GetEncOpenParamDefault(pEncOP, pEncCfg);
	}

	vdi_get_chip_capability(coreIdx, pCap);

#ifdef ENV_SET_ADDR_REMAP
	if (pCap->addrRemapEn) {
		pCap->addrRemapEn = cviVcodecGetEnv("addrRemapEn");
	}
#endif

	if ((pEncOP->bitstreamFormat == STD_AVC) && (pcviEc->singleLumaBuf > 0))
		pEncOP->addrRemapEn = 0;
	else
		pEncOP->addrRemapEn = pCap->addrRemapEn;

	cviPrintRc(pTestEnc);

	if (ret == RETCODE_SUCCESS) {
		CVI_VC_ERR("\n");
		return TE_ERR_ENC_INIT;
	}

	if (pEncOP->bitstreamFormat == STD_AVC) {
		pEncOP->linear2TiledEnable = pEncCfg->coda9.enableLinear2Tiled;
		if (pEncOP->linear2TiledEnable == TRUE) {
			pEncOP->linear2TiledMode = FF_FRAME;
		}

		// enable SVC for advanced termporal reference structure
		pEncCfg->coda9.enableSvc = (pEncOP->gopPreset != 1) ||
					   (pEncOP->VirtualIPeriod > 1);
		pEncOP->EncStdParam.avcParam.mvcExtension =
			pEncCfg->coda9.enableMvc;
		pEncOP->EncStdParam.avcParam.svcExtension =
			pEncCfg->coda9.enableSvc;

		if (pEncOP->EncStdParam.avcParam.fieldFlag == TRUE) {
			if (pEncCfg->rotAngle != 0 || pEncCfg->mirDir != 0) {
				VLOG(WARN,
				     "%s:%d When field Flag is enabled. VPU doesn't support rotation or mirror in field encoding mode.\n",
				     __func__, __LINE__);
				pEncCfg->rotAngle = 0;
				pEncCfg->mirDir = MIRDIR_NONE;
			}
		}
	}

	if (optYuvPath[0] != 0)
		strcpy(pEncCfg->yuvFileName, optYuvPath);

	ret = (RetCode) _updateEncFrameInfo(pTestEnc, &srcFrameFormat);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("_updateEncFrameInfo failure:%d\n", ret);
		return TE_ERR_ENC_INIT;
	}

	if (pEncOP->addrRemapEn == 1) {
		productId = ProductVpuGetId(coreIdx);
		numExtraLine = cviVcodecGetEnv("ARExtraLine");

		framebufStride =
			CalcStride(pTestEnc->framebufWidth, pTestEnc->framebufHeight,
				   pTestEnc->encOP.srcFormat,
				   pTestEnc->encOP.cbcrInterleave,
				   (TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
				   FALSE, FALSE);

		FrameBufSize = VPU_GetFrameBufSize(
			pTestEnc->encConfig.coreIdx, framebufStride,
			pTestEnc->framebufHeight,
			(TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
			pTestEnc->encOP.srcFormat, pTestEnc->encOP.cbcrInterleave, NULL);

		CVI_VC_AR("FrameBufSize = %d\n", FrameBufSize);

		//remap mode luma size
		addrremapsize[0] = CalcLumaSize(productId,
				framebufStride, pTestEnc->framebufHeight,
				pEncOP->cbcrInterleave, (TiledMapType)(pEncCfg->mapType & 0x0f), NULL);
		//remap mode chroma size
		addrremapsize[1] = CalcChromaSize(productId,
				framebufStride, pTestEnc->framebufHeight, pEncOP->srcFormat,
				pEncOP->cbcrInterleave, (TiledMapType)(pEncCfg->mapType & 0x0f), NULL);

		remapsizeInPage[0] = ROUND_UP_N_BIT(addrremapsize[0], AR_PAGE_256KB + AR_PAGE_SIZE_OFFSET);
		remapsizeInPage[1] = ROUND_UP_N_BIT(addrremapsize[1], AR_PAGE_256KB + AR_PAGE_SIZE_OFFSET);

		CVI_VC_AR("remapsizeInPage_Y = %d, remapsizeInPage_UV = %d\n", remapsizeInPage[0], remapsizeInPage[1]);

		extraSizeInPage[0] =
			ROUND_UP_N_BIT(framebufStride * numExtraLine, AR_PAGE_256KB + AR_PAGE_SIZE_OFFSET);
		extraSizeInPage[1] =
			ROUND_UP_N_BIT(extraSizeInPage[0] >> 1, AR_PAGE_256KB + AR_PAGE_SIZE_OFFSET);

		CVI_VC_AR("extraSizeInPage_Y = %d, extraSizeInPage_UV = %d\n", extraSizeInPage[0], extraSizeInPage[1]);

		remapsizeInPage[0] += extraSizeInPage[0];
		remapsizeInPage[1] += extraSizeInPage[1];

		CVI_VC_AR("remapsize_Y = %d, remapsize_UV = %d\n", remapsizeInPage[0], remapsizeInPage[1]);

		//remap recon size bigger than frame mode
		if (remapsizeInPage[0] + remapsizeInPage[1] > FrameBufSize * 2) {
			CVI_VC_AR("use two recon fb is slmaller,addrremap off\n");
			pEncOP->addrRemapEn = 0;
		}
	}

	CVI_VC_CFG("addrRemapEn = %d\n", pEncOP->addrRemapEn);

	pcviEc->addrRemapEn = pEncOP->addrRemapEn;

	cviDumpEncConfig(pEncCfg);

	pTestEnc->bsBufferCount = NUM_OF_BS_BUF;

	for (i = 0; i < (int)pTestEnc->bsBufferCount; i++) {
		char ionName[MAX_VPU_ION_BUFFER_NAME];

		if (pEncConfig->cviEc.bitstreamBufferSize != 0)
			pTestEnc->vbStream[i].size =
				pEncConfig->cviEc.bitstreamBufferSize;
		else
			pTestEnc->vbStream[i].size = STREAM_BUF_SIZE;
		CVI_VC_MEM("vbStream[%d].size = 0x%X\n", i,
			   pTestEnc->vbStream[i].size);

		sprintf(ionName, "VENC_%d_BitStreamBuffer",
			pEncConfig->s32ChnNum);
		if (vdi_get_is_single_es_buf(coreIdx))
			ret = (RetCode) vdi_allocate_single_es_buf_memory(
				coreIdx, &pTestEnc->vbStream[i]);
		else
			ret = (RetCode) VDI_ALLOCATE_MEMORY(coreIdx,
						  &pTestEnc->vbStream[i],
						  bBsStreamCached, ionName);

		if (ret < RETCODE_SUCCESS) {
			CVI_VC_ERR("fail to allocate bitstream buffer\n");
			return TE_ERR_ENC_INIT;
		}
		CVI_VC_TRACE(
			"i = %d, STREAM_BUF = 0x%llX, STREAM_BUF_SIZE = 0x%X\n",
			i, pTestEnc->vbStream[i].phys_addr,
			pTestEnc->vbStream[i].size);
	}

	pEncOP->bitstreamBuffer = pTestEnc->vbStream[0].phys_addr;
	pEncOP->bitstreamBufferSize =
		pTestEnc->vbStream[0].size; //* bsBufferCount;//
	CVI_VC_BS("bitstreamBuffer = 0x%llX, bitstreamBufferSize = 0x%X\n",
		  pEncOP->bitstreamBuffer, pEncOP->bitstreamBufferSize);

	// -- HW Constraint --
	// Required size = actual size of bitstream buffer + 32KBytes
	// Minimum size of bitstream : 96KBytes
	// Margin : 32KBytes
	// Please refer to 3.2.4.4 Encoder Stream Handling in WAVE420
	// programmer's guide.
	if (pEncOP->bitstreamFormat == STD_HEVC) {
		if (pEncCfg->ringBufferEnable)
			pEncOP->bitstreamBufferSize -= 0x8000;
	}

	// when irq return is 0x8000 (buff full),
	// VPU maybe write out of bitstreamBuffer
	// reserve some size avoid over write other ion
	if (pEncOP->bitstreamBufferSize < STREAM_BUF_SIZE_RESERVE) {
		CVI_VC_ERR("bitstreamBufferSize is to small\n");
		return TE_ERR_ENC_INIT;
	}
	pEncOP->bitstreamBufferSize -= STREAM_BUF_SIZE_RESERVE;

	CVI_VC_TRACE("ringBufferEnable = %d\n", pEncCfg->ringBufferEnable);

	pEncOP->ringBufferEnable = pEncCfg->ringBufferEnable;
	pEncOP->cbcrInterleave = pEncCfg->cbcrInterleave;
	pEncOP->frameEndian = pEncCfg->frame_endian;
	pEncOP->streamEndian = pEncCfg->stream_endian;
	pEncOP->sourceEndian = pEncCfg->source_endian;
	if (pEncOP->bitstreamFormat == STD_AVC) {
		pEncOP->bwbEnable = VPU_ENABLE_BWB;
	}

	pEncOP->lineBufIntEn = pEncCfg->lineBufIntEn;
	pEncOP->bEsBufQueueEn = pEncCfg->bEsBufQueueEn;
	pEncOP->coreIdx = coreIdx;
	pEncOP->cbcrOrder = CBCR_ORDER_NORMAL;
	// host can set useLongTerm to 1 or 0 directly.
	pEncOP->EncStdParam.hevcParam.useLongTerm =
		(pEncCfg->useAsLongtermPeriod > 0 &&
		 pEncCfg->refLongtermPeriod > 0) ?
			      1 :
			      0;
	pEncOP->s32ChnNum = pEncCfg->s32ChnNum;

	// slice split
	if (pTestEnc->encConfig.cviEc.h265Split.bSplitEnable) {
		pEncOP->EncStdParam.hevcParam.independSliceMode =
			pTestEnc->encConfig.cviEc.h265Split.bSplitEnable;
		pEncOP->EncStdParam.hevcParam.independSliceModeArg =
			pTestEnc->encConfig.cviEc.h265Split.u32LcuLineNum * (pEncOP->picWidth + 63) / 64;
	} else if (pTestEnc->encConfig.cviEc.h264Split.bSplitEnable) {
		pEncOP->sliceMode.sliceMode = pTestEnc->encConfig.cviEc.h264Split.bSplitEnable;
		pEncOP->sliceMode.sliceSizeMode = 1;
		pEncOP->sliceMode.sliceSize =
			pTestEnc->encConfig.cviEc.h264Split.u32MbLineNum * (pEncOP->picWidth + 15) / 16;
	}

	// h264 intra pred
	if (pEncOP->bitstreamFormat == STD_AVC) {
		pEncOP->EncStdParam.avcParam.constrainedIntraPredFlag =
				pTestEnc->encConfig.cviEc.h264IntraPred.constrained_intra_pred_flag;
	}

#ifdef SUPPORT_MULTIPLE_PPS // if SUPPORT_MULTIPLE_PPS is enabled. encoder can include multiple pps in bitstream output.
	if (pEncOP->bitstreamFormat == STD_AVC) {
		getAvcEncPPS(&encOP); //add PPS before OPEN
	}
#endif

	if (writeVuiRbsp(coreIdx, pEncCfg, pEncOP, &pTestEnc->vbVuiRbsp) ==
	    FALSE)
		return TE_ERR_ENC_INIT;
	if (writeHrdRbsp(coreIdx, pEncCfg, pEncOP, &pTestEnc->vbHrdRbsp) ==
	    FALSE)
		return TE_ERR_ENC_INIT;

#ifdef TEST_ENCODE_CUSTOM_HEADER
	if (writeCustomHeader(coreIdx, pEncOP, &pTestEnc->vbCustomHeader,
			      &pTestEnc->hrd) == FALSE)
		return TE_ERR_ENC_INIT;
#endif

	// Open an instance and get initial information for encoding.
	ret = VPU_EncOpen(&pTestEnc->handle, pEncOP);

	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_EncOpen failed Error code is 0x%x\n", ret);
#ifdef ENABLE_CNM_DEBUG_MSG
		PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
		return TE_ERR_ENC_INIT;
	}

	cviEncRc_Open(&pTestEnc->handle->rcInfo, pEncOP);

	cviSetApiMode(pTestEnc->handle, pEncCfg->cviApiMode);

#ifdef ENABLE_CNM_DEBUG_MSG
	// VPU_EncGiveCommand(pTestEnc->handle, ENABLE_LOGGING, 0);
#endif
	if (pEncCfg->useRot == TRUE) {
		VPU_EncGiveCommand(pTestEnc->handle, ENABLE_ROTATION, 0);
		VPU_EncGiveCommand(pTestEnc->handle, ENABLE_MIRRORING, 0);
		VPU_EncGiveCommand(pTestEnc->handle, SET_ROTATION_ANGLE,
				   &pEncCfg->rotAngle);

		mirrorDirection = (MirrorDirection)pEncCfg->mirDir;
		VPU_EncGiveCommand(pTestEnc->handle, SET_MIRROR_DIRECTION,
				   &mirrorDirection);
	}

	if (pEncOP->bitstreamFormat == STD_AVC) {
		pTestEnc->secAxiUse.u.coda9.useBitEnable =
			(pEncCfg->secondary_axi >> 0) & 0x01;
		pTestEnc->secAxiUse.u.coda9.useIpEnable =
			(pEncCfg->secondary_axi >> 1) & 0x01;
		pTestEnc->secAxiUse.u.coda9.useDbkYEnable =
			(pEncCfg->secondary_axi >> 2) & 0x01;
		pTestEnc->secAxiUse.u.coda9.useDbkCEnable =
			(pEncCfg->secondary_axi >> 3) & 0x01;
		pTestEnc->secAxiUse.u.coda9.useBtpEnable =
			(pEncCfg->secondary_axi >> 4) & 0x01;
		pTestEnc->secAxiUse.u.coda9.useOvlEnable =
			(pEncCfg->secondary_axi >> 5) & 0x01;
		VPU_EncGiveCommand(pTestEnc->handle, SET_SEC_AXI,
				   &pTestEnc->secAxiUse);
	}

	/*******************************************
	* INIT_SEQ                                *
	*******************************************/
	ret = VPU_EncGetInitialInfo(pTestEnc->handle, &pTestEnc->initialInfo);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"VPU_EncGetInitialInfo failed Error code is 0x%x %p\n",
			ret, __builtin_return_address(0));
#ifdef ENABLE_CNM_DEBUG_MSG
		PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
		return TE_ERR_ENC_OPEN;
	}

	if (pEncOP->bitstreamFormat == STD_AVC) {
		// Note: The below values of MaverickCache configuration are best values.
		MaverickCacheConfig encCacheConfig;
		MaverickCache2Config(&encCacheConfig,
				     0, //encoder
				     pEncOP->cbcrInterleave, // cb cr interleave
				     0, /* bypass */
				     0, /* burst  */
				     3, /* merge mode */
				     mapType, 15 /* shape */);
		VPU_EncGiveCommand(pTestEnc->handle, SET_CACHE_CONFIG,
				   &encCacheConfig);
	}

	CVI_VC_INFO(
		"* Enc InitialInfo =>\n instance #%d\n minframeBuffercount: %u\n minSrcBufferCount: %d\n",
		instIdx, pTestEnc->initialInfo.minFrameBufferCount,
		pTestEnc->initialInfo.minSrcFrameCount);
	CVI_VC_INFO(" picWidth: %u\n picHeight: %u\n ", pEncOP->picWidth,
		    pEncOP->picHeight);

	CVI_VC_TRACE("compare_type = %d\n", pEncCfg->compare_type);

#ifdef REDUNDENT_CODE
	if (pEncCfg->compare_type & (1 << MODE_COMP_ENCODED)) {
		pTestEnc->comparatorBitStream =
			Comparator_Create(STREAM_COMPARE,
					  pEncCfg->ref_stream_path,
					  pEncCfg->cfgFileName);
		if (pTestEnc->comparatorBitStream == NULL) {
			CVI_VC_ERR("pTestEnc->comparatorBitStream == NULL\n");
			return TE_ERR_ENC_OPEN;
		}
	}
#endif

	if (pEncOP->bitstreamFormat == STD_HEVC) {
		pTestEnc->secAxiUse.u.wave4.useEncImdEnable =
			(pEncCfg->secondary_axi & 0x1) ?
				      TRUE :
				      FALSE; // USE_IMD_INTERNAL_BUF
		pTestEnc->secAxiUse.u.wave4.useEncRdoEnable =
			(pEncCfg->secondary_axi & 0x2) ?
				      TRUE :
				      FALSE; // USE_RDO_INTERNAL_BUF
		pTestEnc->secAxiUse.u.wave4.useEncLfEnable =
			(pEncCfg->secondary_axi & 0x4) ?
				      TRUE :
				      FALSE; // USE_LF_INTERNAL_BUF
		VPU_EncGiveCommand(pTestEnc->handle, SET_SEC_AXI,
				   &pTestEnc->secAxiUse);

		CVI_VC_FLOW(
			"SET_SEC_AXI: useEncImdEnable = %d, useEncRdoEnable = %d, useEncLfEnable = %d\n",
			pTestEnc->secAxiUse.u.wave4.useEncImdEnable,
			pTestEnc->secAxiUse.u.wave4.useEncRdoEnable,
			pTestEnc->secAxiUse.u.wave4.useEncLfEnable);
	}
#ifdef VC_DRIVER_TEST
	if (pEncCfg->cviApiMode == API_MODE_DRIVER) {
		int iRet = _cviVEncRegFrameBuffer(pTestEnc, NULL);

		if (iRet != 0)
			return iRet;
	}
#endif
	/*************************
	* BUILD SEQUENCE HEADER *
	************************/
#ifndef SUPPORT_DONT_READ_STREAM
	pTestEnc->bsReader = BitstreamReader_Create(
		pEncOP->ringBufferEnable, pEncCfg->bitstreamFileName,
		(EndianMode)pEncOP->streamEndian, &pTestEnc->handle);
	BitstreamReader_SetVbStream(pTestEnc->bsReader, &pTestEnc->vbStream[0]);
#endif

	pTestEnc->encParam.quantParam = pEncCfg->picQpY;
	CVI_VC_TRACE("picQpY = %d\n", pEncCfg->picQpY);
	pTestEnc->encParam.skipPicture = 0;
	pTestEnc->encParam.forcePicQpI = 0;

	do {
		int iRet = TRUE;

		if (pEncOP->bitstreamFormat == STD_AVC) {
			if (pEncOP->ringBufferEnable == TRUE) {
				VPU_EncSetWrPtr(pTestEnc->handle,
						pEncOP->bitstreamBuffer, 1);
			} else {
				pTestEnc->encHeaderParam.buf = pEncOP->bitstreamBuffer;
				pTestEnc->encHeaderParam.size =
					pEncOP->bitstreamBufferSize;
			}

	#if REPEAT_SPS
			iRet = TRUE;
	#else
			iRet = cviEncode264Header(pTestEnc);
	#endif
		} else {
			pTestEnc->encParam.forcePicQpEnable = 0;
			pTestEnc->encParam.forcePicQpP = 0;
			pTestEnc->encParam.forcePicQpB = 0;
			pTestEnc->encParam.forcePicTypeEnable = 0;
			pTestEnc->encParam.forcePicType = 0;
			pTestEnc->encParam.codeOption.implicitHeaderEncode =
				1; // FW will encode header data implicitly when changing the
			// header syntaxes
			pTestEnc->encParam.codeOption.encodeAUD = pEncCfg->encAUD;
			pTestEnc->encParam.codeOption.encodeEOS = 0;

			CVI_VC_TRACE("ringBufferEnable = %d\n",
					pEncOP->ringBufferEnable);

	#if REPEAT_SPS
			iRet = TRUE;
	#else
			iRet = cviEncode265Header(pTestEnc);
	#endif
		}

		if (iRet == FALSE) {
			CVI_VC_ERR("iRet = %d\n", iRet);
			return TE_ERR_ENC_OPEN;
		}

		CVI_VC_TRACE("iRet = %d\n", iRet);
	} while (0);

	pTestEnc->yuvFeederInfo.cbcrInterleave = pEncCfg->cbcrInterleave;
	pTestEnc->yuvFeederInfo.nv21 = pEncCfg->nv21;
	pTestEnc->yuvFeederInfo.packedFormat = pEncCfg->packedFormat;
	pTestEnc->yuvFeederInfo.srcFormat = pEncOP->srcFormat;
	pTestEnc->yuvFeederInfo.srcPlanar = TRUE;
	pTestEnc->yuvFeederInfo.srcStride = pTestEnc->srcFrameStride;
	pTestEnc->yuvFeederInfo.srcHeight = pTestEnc->srcFrameHeight;
	pTestEnc->yuvFeeder =
		YuvFeeder_Create(pEncCfg->yuv_mode, pEncCfg->yuvFileName,
				 pTestEnc->yuvFeederInfo);
	CVI_VC_TRACE("yuvFeeder = 0x%p\n", pTestEnc->yuvFeeder);
	if (pTestEnc->yuvFeeder == NULL) {
		CVI_VC_ERR("YuvFeeder_Create error\n");
		return TE_ERR_ENC_OPEN;
	}

	if (pEncOP->ringBufferEnable == TRUE) {
		// this function shows that HOST can set wrPtr to
		// start position of encoded output in ringbuffer enable mode
		VPU_EncSetWrPtr(pTestEnc->handle, pEncOP->bitstreamBuffer, 1);
	}

	DisplayEncodedInformation(pTestEnc->handle, pEncOP->bitstreamFormat,
				  NULL, 0, 0);
	pTestEnc->bIsEncoderInited = TRUE;

	if (pEncCfg->bIsoSendFrmEn && !pTestEnc->tPthreadId) {
		struct sched_param param = {
			.sched_priority = 95,
		};

		init_completion(&pTestEnc->semSendEncCmd);
		init_completion(&pTestEnc->semGetStreamCmd);
		init_completion(&pTestEnc->semEncDoneCmd);
		pTestEnc->tPthreadId = kthread_run(pfnWaitEncodeDone,
						 (CVI_VOID *)pTestEnc,
						 "cvitask_vc_wt%d", pEncCfg->s32ChnNum);
		if (IS_ERR(pTestEnc->tPthreadId)) {
			CVI_VC_ERR("WaitEncodeDone task error!\n");
			return RETCODE_FAILURE;
		}
		sched_setscheduler(pTestEnc->tPthreadId, SCHED_FIFO, &param);
	}

	return INIT_TEST_ENCODER_OK;
}

#ifdef SUPPORT_DONT_READ_STREAM
static int updateBitstream(vpu_buffer_t *vb, Int32 size)
{
	int status = TRUE;
	if (size > vb->size) {
		CVI_VC_ERR(
			"bitstream buffer is not enough, size = 0x%X, bitstream size = 0x%X!\n",
			size, vb->size);
		return FALSE;
	}

#if 1
	unsigned char *ptr;
	int len, idx;

	ptr = (unsigned char *)(vb->phys_addr + size);
	len = size;
	size = (size + 16) & 0xfffffff0;
	len = size - len;

	for (idx = 0; idx < len; idx++, ptr++)
		*ptr = 0x0;
#else
	size = (size + 16) & 0xfffffff0;
#endif

	vb->phys_addr += size;
	vb->size -= size;

#if ES_CYCLIC
	if (vb->phys_addr + STREAM_BUF_MIN_SIZE >=
	    dramCfg.pucVpuDramAddr + STREAM_BUF_SIZE + vb->base) {
		vb->phys_addr = dramCfg.pucVpuDramAddr + vb->base;
		vb->size = STREAM_BUF_SIZE;
		CVI_VC_BS(
			"vb, pucVpuDramAddr = 0x%lX, base = 0x%lX, phys_addr = 0x%lX, size = 0x%X\n",
			dramCfg.pucVpuDramAddr, vb->base, vb->phys_addr,
			vb->size);
	}
#endif

	CVI_VC_BS("vb, phys_addr = 0x%lX, size = 0x%X\n", vb->phys_addr,
		  vb->size);

	return status;
}
#endif

static void cviUpdateOneRoi(EncParam *param, const cviEncRoiCfg *proi,
			    int index)
{
	param->roi_request = TRUE;
	param->roi_enable[index] = proi->roi_enable;
	param->roi_qp_mode[index] = proi->roi_qp_mode;
	param->roi_qp[index] = proi->roi_qp;
	param->roi_rect_x[index] = proi->roi_rect_x;
	param->roi_rect_y[index] = proi->roi_rect_y;
	param->roi_rect_width[index] = proi->roi_rect_width;
	param->roi_rect_height[index] = proi->roi_rect_height;
}

static void cviUpdateRoiBySdk(EncParam *param, cviEncRoiCfg *cviRoi)
{
	int i;

	param->roi_request = TRUE;
	for (i = 0; i <= 7; i++) {
		cviUpdateOneRoi(param, &cviRoi[i], i);
	}
}

#if REPEAT_SPS

static int cviH264SpsAddVui(stTestEncoder *pTestEnc)
{
	stStreamPack *psp = &pTestEnc->streamPack;
	cviH264Vui *pVui = &pTestEnc->encConfig.cviEc.h264Vui;
	int i;

	MUTEX_LOCK(&psp->packMutex);
	for (i = 0; i < psp->totalPacks; i++) {
		if (psp->pack[i].cviNalType == NAL_SPS) {
			H264SpsAddVui(pVui, &psp->pack[i].addr,
				      &psp->pack[i].len);
			psp->pack[i].u64PhyAddr = virt_to_phys(psp->pack[i].addr);
			vdi_flush_ion_cache(psp->pack[i].u64PhyAddr, psp->pack[i].addr, psp->pack[i].len);
		}
	}
	MUTEX_UNLOCK(&psp->packMutex);

	return TRUE;
}

static int cviH265SpsAddVui(stTestEncoder *pTestEnc)
{
	stStreamPack *psp = &pTestEnc->streamPack;
	cviH265Vui *pVui = &pTestEnc->encConfig.cviEc.h265Vui;
	int i;

	MUTEX_LOCK(&psp->packMutex);
	for (i = 0; i < psp->totalPacks; i++) {
		if (psp->pack[i].cviNalType == NAL_SPS) {
			H265SpsAddVui(pVui, &psp->pack[i].addr,
				      &psp->pack[i].len);
			psp->pack[i].u64PhyAddr = virt_to_phys(psp->pack[i].addr);
			vdi_flush_ion_cache(psp->pack[i].u64PhyAddr, psp->pack[i].addr, psp->pack[i].len);
		}
	}
	MUTEX_UNLOCK(&psp->packMutex);

	return TRUE;
}
#endif
static int cviEncodeOneFrame(stTestEncoder *pTestEnc)
{
	EncParam *pEncParam = &pTestEnc->encParam;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	FrameBufferAllocInfo *pFbAllocInfo = &pTestEnc->fbAllocInfo;
#ifdef VC_DRIVER_TEST
	void *pYuvFeeder = pTestEnc->yuvFeeder;
#endif
	int coreIdx = pTestEnc->coreIdx;
	int ret = 0, i;
	feederYuvaddr *pYuvAddr;
	CodecInst *pCodecInst;


	CVI_VC_FLOW("frameIdx = %d\n", pTestEnc->frameIdx);

#if 0
RETRY:
	ret = cviVPU_LockAndCheckState(pTestEnc->handle, CODEC_STAT_ENC_PIC);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("cviVPU_LockAndCheckState, ret = %d\n", ret);
		return ret;
	}
#endif
	if (GetPendingInst(coreIdx)) {
		//LeaveLock(coreIdx);
		CVI_VC_TRACE("GetPendingInst, RETCODE_FRAME_NOT_COMPLETE\n");
		return RETCODE_FAILURE;
	}

	pCodecInst = pTestEnc->handle;
	pCodecInst->CodecInfo->encInfo.openParam.cbcrInterleave =
		pEncOP->cbcrInterleave;
	pCodecInst->CodecInfo->encInfo.openParam.nv21 = pEncOP->nv21;

	SetPendingInst(coreIdx, pCodecInst, __func__, __LINE__);
	ret = cviVPU_ChangeState(pTestEnc->handle);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("cviVPU_ChangeState, ret = %d\n", ret);
	}

	if (pTestEnc->encConfig.cviEc.roi_request)
		cviUpdateRoiBySdk(pEncParam, pTestEnc->encConfig.cviEc.cviRoi);
	pTestEnc->encConfig.cviEc.roi_request = FALSE;

	pEncParam->is_idr_frame = cviCheckIdrPeriod(pTestEnc);

#ifdef DROP_FRAME
	 // because bitstream not enough, need request IDR and reduce target bitrate
	if (pTestEnc->bDrop) {
		if (pTestEnc->originBitrate == 0)
			pTestEnc->originBitrate = pEncOP->bitRate;

		pEncOP->bitRate /= 2;
		// if bitRate is too small, will overflow
		if (pEncOP->bitRate < 100)
			pEncOP->bitRate = 100;

		pEncParam->idr_request = TRUE;
		pTestEnc->bDrop = FALSE;
		CVI_VENC_BS("orignBitRate:%d, targetBitrate:%d\n", pTestEnc->originBitrate, pEncOP->bitRate);
	} else if (pTestEnc->originBitrate && (pEncOP->bitRate != pTestEnc->originBitrate)) {
		pEncOP->bitRate = pTestEnc->originBitrate;
		pTestEnc->originBitrate = 0;
		CVI_VENC_BS("restore originBitrate:%d\n", pEncOP->bitRate);
	}
#endif

	cviPicParamChangeCtrl(pTestEnc->handle, &pTestEnc->encConfig, pEncOP,
			      pEncParam, pTestEnc->frameIdx);

	ret = cviSetupPicConfig(pTestEnc);
	if (ret != 0) {
		CVI_VC_ERR("cviSetupPicConfig, ret = %d\n", ret);
		return RETCODE_FAILURE;
	}

#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
	for (i = 0; i < (int)pTestEnc->bsBufferCount; i++) {
		if (pTestEnc->vbStream[i].size != 0)
			vdi_invalidate_ion_cache(pTestEnc->vbStream[i].phys_addr,
			pTestEnc->vbStream[i].virt_addr, pTestEnc->vbStream[i].size);
	}
#endif

#if REPEAT_SPS
	if (pEncParam->is_idr_frame == TRUE) {
		if (pEncOP->bitstreamFormat == STD_AVC) {
			ret = cviEncode264Header(pTestEnc);
			if (ret == FALSE) {
				CVI_VC_ERR("cviEncode264Header = %d\n", ret);
				return TE_ERR_ENC_OPEN;
			}
			ret = cviH264SpsAddVui(pTestEnc);
			if (ret == FALSE) {
				CVI_VC_ERR("cviH264AddTimingInfo = %d\n", ret);
				return TE_ERR_ENC_OPEN;
			}
		} else {
			ret = cviEncode265Header(pTestEnc);
			if (ret == FALSE) {
				CVI_VC_ERR("cviEncode265Header = %d\n", ret);
				return TE_ERR_ENC_OPEN;
			}
			ret = cviH265SpsAddVui(pTestEnc);
			if (ret == FALSE) {
				CVI_VC_ERR("cviH265AddTimingInfo = %d\n", ret);
				return TE_ERR_ENC_OPEN;
			}
		}
	}
#endif

	ret = cviInsertUserData(pTestEnc);
	if (ret == FALSE) {
		CVI_VC_ERR("cviInsertUserData failed %d\n", ret);
		//return TE_ERR_ENC_USER_DATA;
	}

	if (pEncOP->bitstreamFormat == STD_AVC &&
	    pEncOP->EncStdParam.avcParam.svcExtension == TRUE)
		VPU_EncGiveCommand(pTestEnc->handle, ENC_GET_FRAMEBUF_IDX,
				   &pTestEnc->srcFrameIdx);
	else
		pTestEnc->srcFrameIdx =
			(pTestEnc->frameIdx % pFbAllocInfo->num);

	pEncParam->srcIdx = pTestEnc->srcFrameIdx;

	CVI_VC_TRACE("Read YUV, srcIdx = %d\n", pEncParam->srcIdx);

#ifdef SUPPORT_DIRECT_YUV
	ret = YuvFeeder_Feed(pYuvFeeder, coreIdx, &pTestEnc->fbSrcFixed,
			     pTestEnc->encOP.picWidth,
			     pTestEnc->encOP.picHeight, NULL);
#else
	if (pTestEnc->encConfig.yuv_mode == SOURCE_YUV_ADDR)
		pYuvAddr = &pTestEnc->yuvAddr;
	else
		pYuvAddr = NULL;

	ret = TRUE;
#ifdef VC_DRIVER_TEST
	if (pTestEnc->encConfig.cviApiMode == API_MODE_DRIVER) {
		ret = YuvFeeder_Feed(pYuvFeeder, coreIdx,
				     &pTestEnc->fbSrc[pTestEnc->srcFrameIdx],
				     pTestEnc->encOP.picWidth,
				     pTestEnc->encOP.picHeight, pYuvAddr);
	}
#endif
#endif

	if (ret == 0) {
		pEncParam->srcEndFlag = 1; // when there is no more source image
			// to be encoded, srcEndFlag should
			// be set 1. because of encoding
			// delay for WAVE420
	}

	if (pEncParam->srcEndFlag != 1) {
		if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
			pTestEnc->fbSrc[pTestEnc->srcFrameIdx].srcBufState =
				SRC_BUFFER_USE_ENCODE;
			pEncParam->sourceFrame =
				&pTestEnc->fbSrc[pTestEnc->srcFrameIdx];
			pEncParam->sourceFrame->endian = 0x10;
			pEncParam->sourceFrame->sourceLBurstEn = 0;
			pEncParam->sourceFrame->bufY =
				(PhysicalAddress)pYuvAddr->phyAddrY;
			pEncParam->sourceFrame->bufCb =
				(PhysicalAddress)pYuvAddr->phyAddrCb;
			pEncParam->sourceFrame->bufCr =
				(PhysicalAddress)pYuvAddr->phyAddrCr;
			pEncParam->sourceFrame->stride =
				pTestEnc->srcFrameStride;
			pEncParam->sourceFrame->cbcrInterleave =
				pEncOP->cbcrInterleave;
			pEncParam->sourceFrame->nv21 = pEncOP->nv21;

			CVI_VC_SRC(
				"srcFrameIdx = %d, bufY = 0x%llX, bufCb = 0x%llX, bufCr = 0x%llX, stride = %d\n",
				pTestEnc->srcFrameIdx,
				pEncParam->sourceFrame->bufY,
				pEncParam->sourceFrame->bufCb,
				pEncParam->sourceFrame->bufCr,
				pEncParam->sourceFrame->stride);
		} else {
			pTestEnc->fbSrc[pTestEnc->srcFrameIdx].srcBufState =
				SRC_BUFFER_USE_ENCODE;
			pEncParam->sourceFrame =
				&pTestEnc->fbSrc[pTestEnc->srcFrameIdx];
			pEncParam->sourceFrame->sourceLBurstEn = 0;
		}
	}

	if (pTestEnc->encOP.ringBufferEnable == FALSE) {
		pTestEnc->bsQueueIndex =
			(pTestEnc->bsQueueIndex + 1) % pTestEnc->bsBufferCount;
		pEncParam->picStreamBufferAddr =
			pTestEnc->vbStream[pTestEnc->bsQueueIndex]
				.phys_addr; // can set the newly allocated
		// buffer.
#ifdef SUPPORT_DONT_READ_STREAM
		pEncParam->picStreamBufferSize =
			pTestEnc->vbStream[pTestEnc->bsQueueIndex].size;
#else
		pEncParam->picStreamBufferSize =
			pTestEnc->encOP.bitstreamBufferSize;
#endif

		CVI_VC_BS(
			"picStreamBufferAddr = 0x%llX, picStreamBufferSize = 0x%X\n",
			pEncParam->picStreamBufferAddr,
			pEncParam->picStreamBufferSize);
	}

	if ((pTestEnc->encConfig.seiDataEnc.prefixSeiNalEnable ||
	     pTestEnc->encConfig.seiDataEnc.suffixSeiNalEnable) &&
	    pEncParam->srcEndFlag != 1) {
		pTestEnc->encConfig.seiDataEnc.prefixSeiNalAddr =
			pTestEnc->vbPrefixSeiNal[pTestEnc->srcFrameIdx]
				.phys_addr;
		pTestEnc->encConfig.seiDataEnc.suffixSeiNalAddr =
			pTestEnc->vbSuffixSeiNal[pTestEnc->srcFrameIdx]
				.phys_addr;
		VPU_EncGiveCommand(pTestEnc->handle, ENC_SET_SEI_NAL_DATA,
				   &pTestEnc->encConfig.seiDataEnc);
	}

	// set ROI map/qp map in dram for hevc encode
	if (pEncOP->bitstreamFormat == STD_HEVC) {
		// for qp Map
		if (pTestEnc->encConfig.ctu_qpMap_enable) {
			TestEncConfig *pEncConfig = &pTestEnc->encConfig;

			if (pEncConfig->cviEc.bQpMapValid) {
				cviSetCtuQpMap(coreIdx, &pTestEnc->encConfig,
					&pTestEnc->encOP,
					pTestEnc->vbCtuQp[pTestEnc->srcFrameIdx].phys_addr,
					pEncConfig->cviEc.pu8QpMap, pEncParam,
					MAX_CTU_NUM);
			}

		}
		//for roiLevel Map
		if (pTestEnc->encConfig.ctu_roiLevel_enable) {
			int baseQp = pTestEnc->handle->CodecInfo->encInfo.pic_ctu_avg_qp;

			if (baseQp == 0) {  //if first frame, rc have not been started
				baseQp = pTestEnc->encOP.RcInitialQp;
			}
			CVI_VC_TRACE("roi cfg is 1");
			CVI_VC_TRACE("baseQp:%d\n", baseQp);
			GenRoiLevelMap(&pTestEnc->encConfig, pEncParam,
				      &pTestEnc->encOP, baseQp,  &pTestEnc->roiMapBuf[0]);

			cviSetCtuRoiLevelMap(coreIdx, &pTestEnc->encConfig,
					    &pTestEnc->encOP,
					    pTestEnc->vbRoi[pTestEnc->srcFrameIdx].phys_addr,
					    &pTestEnc->roiMapBuf[0], pEncParam, MAX_CTU_NUM);

			pTestEnc->encConfig.ctu_roiLevel_enable = 0;
		}
		// for ai output => roi level importance map
		if (pTestEnc->encOP.svc_enable) {
			cviSmartRoiLevelCfg(coreIdx,  &pTestEnc->encConfig,
					   pEncParam, pEncOP,
					   pTestEnc->vbRoi[pTestEnc->srcFrameIdx]
					   .phys_addr, &pTestEnc->roiMapBuf[0],
					   pTestEnc->frameIdx);

		}
#ifdef TEST_OTHER_QMAP
		if (pTestEnc->encConfig.ctu_qpMap_enable) {
			cviSetCtuQpMap(coreIdx, &pTestEnc->encConfig,
				       &pTestEnc->encOP,
				       pTestEnc->vbCtuQp[pTestEnc->srcFrameIdx]
					       .phys_addr,
				       pCviEc->pu8QpMap, pEncParam,
				       MAX_CTU_NUM);
		} else {
			osal_memset(&pEncParam->ctuOptParam, 0,
				    sizeof(HevcCtuOptParam));
			if ((pTestEnc->encConfig.roi_cfg_type == 1 &&
			     pTestEnc->encConfig.cviApiMode ==
				     API_MODE_DRIVER) ||
			    (pTestEnc->encOP.bgEnhanceEn == TRUE &&
			     pTestEnc->encConfig.cviApiMode == API_MODE_SDK)) {
				setRoiMapFromMap(
					coreIdx, &pTestEnc->encConfig,
					&pTestEnc->encOP,
					pTestEnc->vbRoi[pTestEnc->srcFrameIdx]
						.phys_addr,
					&pTestEnc->roiMapBuf[0], pEncParam,
					MAX_CTU_NUM);
			} else {
				if (pTestEnc->encConfig.cviApiMode !=
				    API_MODE_DRIVER) {
					GenRoiLevelMap(
						&pTestEnc->encConfig, pEncParam,
						&pTestEnc->encOP,
						pTestEnc->handle->CodecInfo
							->encInfo
							.pic_ctu_avg_qp);
				}
				cviSetCtuRoiLevelMap(
					coreIdx, &pTestEnc->encConfig,
					&pTestEnc->encOP,
					pTestEnc->vbRoi[pTestEnc->srcFrameIdx]
						.phys_addr,
					&pTestEnc->roiMapBuf[0], pEncParam,
					MAX_CTU_NUM);
			}
		}
#endif
	}

#ifdef TEST_ENCODE_CUSTOM_HEADER
	if (writeSeiNalData(pTestEnc->handle, pTestEnc->encOP.streamEndian,
			    pTestEnc->vbSeiNal[pTestEnc->srcFrameIdx].phys_addr,
			    &pTestEnc->hrd) == FALSE) {
		return TE_ERR_ENC_OPEN;
	}
#endif

	if (pTestEnc->encConfig.useAsLongtermPeriod > 0 &&
	    pTestEnc->encConfig.refLongtermPeriod > 0) {
		pEncParam->useCurSrcAsLongtermPic =
			(pTestEnc->frameIdx %
			 pTestEnc->encConfig.useAsLongtermPeriod) == 0 ?
				      1 :
				      0;
		pEncParam->useLongtermRef =
			(pTestEnc->frameIdx %
			 pTestEnc->encConfig.refLongtermPeriod) == 0 ?
				      1 :
				      0;
	}

	// Start encoding a frame.
	pTestEnc->frameIdx++;

#if DUMP_SRC
	{
		FILE *fp;
		int fsize =
			pTestEnc->encOP.picWidth * pTestEnc->encOP.picHeight;

		fp = fopen("src.dat", "ab");
		if (fp) {
			fwrite(pYuvAddr->addrY, 1, fsize, fp);
			fwrite(pYuvAddr->addrCb, 1, fsize >> 2, fp);
			fwrite(pYuvAddr->addrCr, 1, fsize >> 2, fp);
			fflush(fp);
		}
	}
#endif
	pTestEnc->handle->bBuffFull = CVI_FALSE;

	ret = VPU_EncStartOneFrame(pTestEnc->handle, pEncParam);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_EncStartOneFrame failed Error code is 0x%x\n",
			   ret);
		return TE_ERR_ENC_OPEN;
	}

	return ret;
}

static void setInitRcQp(EncChangeParam *changeParam, EncHevcParam *param,
			EncOpenParam *pEncOP, int initQp)
{
	changeParam->enable_option |= ENC_RC_PARAM_CHANGE;
	changeParam->initialRcQp = initQp;
	changeParam->rcEnable = 1;
	changeParam->cuLevelRCEnable = param->cuLevelRCEnable;
	changeParam->hvsQPEnable = param->hvsQPEnable;
	changeParam->hvsQpScaleEnable = param->hvsQpScaleEnable;
	changeParam->hvsQpScale = param->hvsQpScale;
	changeParam->bitAllocMode = param->bitAllocMode;
	changeParam->initBufLevelx8 = param->initBufLevelx8;
	changeParam->initialDelay = pEncOP->initialDelay;
}

static void setMinMaxQpByDelta(EncHandle handle, EncOpenParam *pEncOP,
			       int deltaQp, int isIFrame,
			       int intraQpOffsetForce0)
{
	if (pEncOP->bitstreamFormat == STD_AVC) {
		MinMaxQpChangeParam param;
		param.maxQpIEnable = 1;
		param.maxQpI = CLIP3(0, 51, pEncOP->userQpMaxI + deltaQp);
		param.minQpIEnable = 1;
		param.minQpI = CLIP3(0, 51, pEncOP->userQpMinI);
		param.maxQpPEnable = 1;
		param.maxQpP = CLIP3(0, 51, pEncOP->userQpMaxP + deltaQp);
		param.minQpPEnable = 1;
		param.minQpP = CLIP3(0, 51, pEncOP->userQpMinP);
		VPU_EncGiveCommand(handle, ENC_SET_MIN_MAX_QP, &param);
	} else if (pEncOP->bitstreamFormat == STD_HEVC) {
		int maxQp =
			(isIFrame) ? pEncOP->userQpMaxI : pEncOP->userQpMaxP;
		int minQp =
			(isIFrame) ? pEncOP->userQpMinI : pEncOP->userQpMinP;
		EncChangeParam param;
		param.changeParaMode = OPT_COMMON;
		param.enable_option = ENC_RC_MIN_MAX_QP_CHANGE;
		param.maxQp = CLIP3(0, 51, maxQp + deltaQp);
		param.minQp = CLIP3(0, 51, minQp);
		param.maxDeltaQp = pEncOP->EncStdParam.hevcParam.maxDeltaQp;
		param.intraQpOffset =
			(intraQpOffsetForce0 == 0) ?
				      0 :
				      pEncOP->EncStdParam.hevcParam.intraQpOffset;

		CVI_VC_TRACE(
			"userQpMaxI = %d, userQpMaxP = %d, userQpMinI = %d, userQpMinP = %d\n",
			pEncOP->userQpMaxI, pEncOP->userQpMaxP,
			pEncOP->userQpMinI, pEncOP->userQpMinP);
		CVI_VC_TRACE("minQp = %d, maxQp = %d\n", minQp, maxQp);
		CVI_VC_TRACE(
			"pminQp = %d, pmaxQp = %d, maxDeltaQp = %d, intraQpOffset = %d\n",
			param.minQp, param.maxQp, param.maxDeltaQp,
			param.intraQpOffset);

		VPU_EncGiveCommand(handle, ENC_SET_PARA_CHANGE, &param);
	}
}

static BOOL cviCheckIdrValid(EncOpenParam *pEncOP, Uint32 frameIdx)
{
	int keyframe_period =
		(pEncOP->bitstreamFormat == STD_AVC) ?
			      pEncOP->set_dqp_pic_num :
			      pEncOP->EncStdParam.hevcParam.gopParam.tidPeriod0;
	return (keyframe_period == 0 || (frameIdx % keyframe_period == 0)) ?
			     TRUE :
			     FALSE;
}

static void cviForcePicTypeCtrl(EncOpenParam *pEncOP, EncParam *encParam,
				EncInfo *pEncInfo, BOOL is_intra_period,
				BOOL force_idr, BOOL force_skip_frame,
				Uint32 frameIdx)
{
	encParam->skipPicture = 0;
	encParam->forceIPicture = 0;
	encParam->forcePicTypeEnable = 0;
	pEncInfo->force_as_long_term_ref = 0;

	if (is_intra_period && (!encParam->force_i_for_gop_sync)) {
		return;
	}

	if (force_idr) {
		CVI_VC_TRACE("force IDR request\n");
	}

	encParam->idr_registered |= (force_idr && !is_intra_period);

	if ((encParam->idr_registered && cviCheckIdrValid(pEncOP, frameIdx)) ||
	    (is_intra_period && encParam->force_i_for_gop_sync)) {
		if (pEncOP->bitstreamFormat == STD_HEVC) {
			encParam->forcePicTypeEnable = 1;
			encParam->forcePicType = 3;
		} else { // encOP->bitstreamFormat == STD_AVC
			encParam->forceIPicture = 1;
			pEncInfo->force_as_long_term_ref = 1;
			encParam->force_i_for_gop_sync =
				encParam->idr_registered;
		}
		encParam->idr_registered = 0;
		encParam->is_idr_frame |= 1;
		CVI_VC_TRACE("force IDR ack\n");
	} else if (force_skip_frame) {
		encParam->skipPicture = 1;
		CVI_VC_TRACE("force skip ack\n");
	}
}

static void cviPicParamChangeCtrl(EncHandle handle, TestEncConfig *pEncConfig,
				  EncOpenParam *pEncOP, EncParam *encParam,
				  Uint32 frameIdx)
{
	stRcInfo *pRcInfo = &handle->rcInfo;
	BOOL rateChangeCmd = FALSE;
	// dynamic bitrate change
	CVI_VC_TRACE("gopSize = %d, frameIdx = %d\n", pEncOP->gopSize,
		     frameIdx);
	if (pRcInfo->rcMode == RC_MODE_AVBR) {
		// avbr bitrate change period check
		if (cviEnc_Avbr_PicCtrl(pRcInfo, pEncOP, frameIdx)) {
			cviEncRc_SetParam(&handle->rcInfo, pEncOP, E_BITRATE);
			rateChangeCmd = TRUE;
		}
	} else {
		// detect bitrate change
		if (encParam->is_i_period &&
		    pEncOP->bitRate != cviEncRc_GetParam(pRcInfo, E_BITRATE)) {
			cviEncRc_SetParam(&handle->rcInfo, pEncOP, E_BITRATE);
			rateChangeCmd = TRUE;
		}
	}
	// detect framerate change
	if (encParam->is_i_period &&
	    (pEncOP->frameRateInfo !=
	     cviEncRc_GetParam(pRcInfo, E_FRAMERATE))) {
		cviEncRc_SetParam(pRcInfo, pEncOP, E_FRAMERATE);
		rateChangeCmd = TRUE;
	}

	// bitrate/framerate change handle
	if (rateChangeCmd) {
		if (pEncOP->bitstreamFormat == STD_AVC) {
			VPU_EncGiveCommand(handle, ENC_SET_BITRATE,
					   &pRcInfo->targetBitrate);
		} else if (pEncOP->bitstreamFormat == STD_HEVC) {
			EncChangeParam changeParam;

			changeParam.changeParaMode = OPT_COMMON;
			changeParam.enable_option = ENC_RC_TRANS_RATE_CHANGE |
						    ENC_RC_TARGET_RATE_CHANGE |
						    ENC_FRAME_RATE_CHANGE;
			changeParam.bitRate = handle->rcInfo.targetBitrate;
			changeParam.transRate =
				((pEncOP->rcMode != RC_MODE_CBR &&
				  pEncOP->rcMode != RC_MODE_QPMAP &&
				  pEncOP->rcMode != RC_MODE_UBR) ||
				 (pEncOP->statTime <= 0)) ?
					      MAX_TRANSRATE :
					      handle->rcInfo.targetBitrate *
						pEncOP->statTime * 1000;
			changeParam.frameRate = pEncOP->frameRateInfo;

			if (pRcInfo->rcMode == RC_MODE_AVBR) {
				EncHevcParam *param =
					&pEncOP->EncStdParam.hevcParam;
				int initQp =
					(encParam->is_i_period) ?
						      pRcInfo->lastPicQp +
							(param->intraQpOffset /
							 2) :
						      pRcInfo->lastPicQp;
				setInitRcQp(&changeParam, param, pEncOP,
					    initQp);
			}

			CVI_VC_TRACE("bitRate = %d, transRate = %d\n",
				     changeParam.bitRate,
				     changeParam.transRate);
			CVI_VC_INFO(
				"bitRate = %d, transRate = %d, frameRate = %d\n",
				changeParam.bitRate, changeParam.transRate,
				changeParam.frameRate);
			VPU_EncGiveCommand(handle, ENC_SET_PARA_CHANGE,
					   &changeParam);
		}
	}

	// auto frame-skipping
	CVI_VC_TRACE("gopSize = %d, frameIdx = %d\n", pEncOP->gopSize,
		     frameIdx);

	cviEncRc_UpdateFrameSkipSetting(pRcInfo, pEncConfig->frmLostOpen,
					pEncConfig->cviEc.encFrmGaps,
					pEncConfig->cviEc.frmLostBpsThr);
	do {
		BOOL force_skip_frame =
			cviEncRc_ChkFrameSkipByBitrate(pRcInfo, encParam->is_i_period);
		if (!force_skip_frame) {
			force_skip_frame |= cviEncRc_Avbr_CheckFrameSkip(
				pRcInfo, pEncOP, encParam->is_i_period);
		}

		cviForcePicTypeCtrl(pEncOP, encParam, &handle->CodecInfo->encInfo,
					encParam->is_i_period, encParam->idr_request,
					force_skip_frame, frameIdx);
	} while (0);

	encParam->idr_request = 0;

	CVI_VC_TRACE("skipPicture = %d\n", encParam->skipPicture);

	// TBD: scene change detect signal generate
	/*if(cviEncRc_StateCheck(&handle->rcInfo, FALSE)) {
		int deltaQp = (handle->rcInfo.rcState==UNSTABLE)
			? 2
			: (handle->rcInfo.rcState==RECOVERY)
			? -1 : 0;
		setMinMaxQpByDelta(handle, encOP, deltaQp\);
	}*/

	// avbr adaptive max Qp
	if (pRcInfo->rcEnable && pRcInfo->rcMode == RC_MODE_AVBR) {
		if (pRcInfo->avbrChangeBrEn == TRUE) {
			int deltaQp = cviEncRc_Avbr_GetQpDelta(pRcInfo, pEncOP);
			int intraQpOffsetForce0 =
				(pRcInfo->periodMotionLvRaw > 64) ? 0 : -1;
			CVI_VC_INFO("avbr qp delta: %d %d\n", deltaQp,
				    encParam->is_i_period);
			setMinMaxQpByDelta(handle, pEncOP, deltaQp,
					   encParam->is_i_period,
					   intraQpOffsetForce0);
			pRcInfo->avbrChangeBrEn = FALSE;
		}
	}
}

// this is chip-and-media legacy code, not in used now
/*
static void RcChangeSetParameter(EncHandle handle, TestEncConfig encConfig,
				 EncOpenParam *encOP, EncParam *encParam,
				 Uint32 frameIdx, Uint32 fieldDone,
				 int field_flag, vpu_buffer_t vbRoi)
{
	BOOL isIdr = FALSE;
	BOOL isIPic = FALSE;
	Uint32 minSkipNum = 0;
	Uint32 idx = 0;
	Int32 iPicCnt = 0;

	if (encOP->EncStdParam.avcParam.fieldFlag) {
		encParam->fieldRun = TRUE;
	}

	if (0 == frameIdx) {
		isIdr = TRUE;
		isIPic = TRUE;
	} else {
		Int32 gopNum = encOP->EncStdParam.avcParam.fieldFlag ?
				       (2 * encOP->gopSize) :
				       encOP->gopSize;
		Int32 picIdx = encOP->EncStdParam.avcParam.fieldFlag ?
				       (2 * frameIdx + fieldDone) :
				       frameIdx;
		if ((encOP->idrInterval > 0) && (gopNum > 0)) {
			if ((picIdx >= gopNum)) {
				picIdx = 0;
			}
			if (picIdx == 0) {
				isIPic = TRUE;
				iPicCnt++;
			}
		}
	}

	if (encOP->bitstreamFormat == STD_MPEG4) {
		if (isIPic == TRUE && encOP->idrInterval > 0) {
			if ((iPicCnt % encOP->idrInterval) == 0) {
				isIdr = TRUE;
			}
		}
	}
	encParam->forceIPicture = fieldDone ? FALSE : isIdr;

	encParam->skipPicture = 0;

	minSkipNum = encOP->EncStdParam.avcParam.mvcExtension ? 1 : 0;

	for (idx = 0; idx < MAX_PIC_SKIP_NUM; idx++) {
		Uint32 numPicSkip = field_flag ?
					    encConfig.skipPicNums[idx] / 2 :
					    encConfig.skipPicNums[idx];
		if (numPicSkip > minSkipNum && numPicSkip == (Uint32)frameIdx) {
			if (field_flag == FALSE) {
				encParam->skipPicture = TRUE;
			} else {
				if (encConfig.skipPicNums[idx] % 2)
					encParam->skipPicture = fieldDone;
				else
					encParam->skipPicture = !(fieldDone);
				// check next skip field
				if ((idx + 1) < MAX_PIC_SKIP_NUM) {
					numPicSkip =
						encConfig.skipPicNums[idx + 1] /
						2;
					if (numPicSkip == (Uint32)frameIdx)
						encParam->skipPicture = TRUE;
				}
			}
			break;
		}
	}

	// TODO: Need to do refactoring
	if (encOP->bitstreamFormat == STD_AVC && frameIdx > 0 &&
	    frameIdx == encOP->paramChange.ChangeFrameNum) {
		if (strcmp(encOP->paramChange.pchChangeCfgFileName, "") != 0) {
			EncInfo *pEncInfo;
			CodecInst *pCodecInst;

			pCodecInst = handle;
			pEncInfo = &pCodecInst->CodecInfo->encInfo;

			GetEncOpenParamChange(encOP, encConfig.cfgFileName,
					      NULL);
			if (encOP->paramChange.paraEnable & EN_RC_FRAME_RATE) {
				VPU_EncGiveCommand(
					handle, ENC_SET_FRAME_RATE,
					&(encOP->paramChange.NewFrameRate));
				pEncInfo->openParam.frameRateInfo =
					(encOP->paramChange.NewFrameRate &
					 0xffff);
			}
			if (encOP->paramChange.paraEnable & EN_GOP_NUM) {
				VPU_EncGiveCommand(
					handle, ENC_SET_GOP_NUMBER,
					&(encOP->paramChange.NewGopNum));
				pEncInfo->openParam.gopSize =
					encOP->paramChange.NewGopNum;
			}
			if (encOP->paramChange.paraEnable & EN_RC_BIT_RATE) {
				VPU_EncGiveCommand(
					handle, ENC_SET_BITRATE,
					&encOP->paramChange.NewBitrate);
				pEncInfo->openParam.bitRate =
					encOP->paramChange.NewBitrate;
			}
			if (encOP->paramChange.paraEnable & EN_RC_INTRA_QP) {
				VPU_EncGiveCommand(
					handle, ENC_SET_INTRA_QP,
					&(encOP->paramChange.NewIntraQp));
			}
			if (encOP->paramChange.paraEnable & EN_INTRA_REFRESH) {
				VPU_EncGiveCommand(
					handle, ENC_SET_INTRA_MB_REFRESH_NUMBER,
					&encOP->paramChange.NewIntraRefresh);
				pEncInfo->openParam.intraRefreshNum =
					encOP->paramChange.NewIntraRefresh;
			}

			if (encOP->paramChange.paraEnable &
			    EN_RC_MIN_MAX_QP_CHANGE) {
				int rc_max_qp = 51;
				int rc_min_qp = 12;
				VPU_EncGiveCommand(
					handle, ENC_SET_MIN_MAX_QP,
					&(encOP->paramChange.minMaxQpParam));

				if (encOP->paramChange.minMaxQpParam
					    .maxQpIEnable)
					pEncInfo->openParam.userQpMaxI =
						encOP->paramChange.minMaxQpParam
							.maxQpI;
				else
					pEncInfo->openParam.userQpMaxI =
						rc_max_qp;

				if (encOP->paramChange.minMaxQpParam
					    .minQpIEnable)
					pEncInfo->openParam.userQpMinI =
						encOP->paramChange.minMaxQpParam
							.minQpI;
				else
					pEncInfo->openParam.userQpMinI =
						rc_min_qp;

				if (encOP->paramChange.minMaxQpParam
					    .maxQpPEnable)
					pEncInfo->openParam.userQpMaxP =
						encOP->paramChange.minMaxQpParam
							.maxQpP;
				else
					pEncInfo->openParam.userQpMaxP =
						rc_max_qp;

				if (encOP->paramChange.minMaxQpParam
					    .maxQpPEnable)
					pEncInfo->openParam.userQpMinP =
						encOP->paramChange.minMaxQpParam
							.minQpP;
				else
					pEncInfo->openParam.userQpMinP =
						rc_min_qp;
			}
#if defined(RC_PIC_PARACHANGE) && defined(RC_CHANGE_PARAMETER_DEF)
			if (encOP->paramChange.paraEnable &
			    EN_PIC_PARA_CHANGE) {
				VPU_EncGiveCommand(
					handle, ENC_SET_PIC_CHANGE_PARAM,
					&(encOP->paramChange.changePicParam));

				pEncInfo->openParam.userMaxDeltaQp =
					encOP->paramChange.changePicParam
						.MaxdeltaQp;
				pEncInfo->openParam.userMinDeltaQp =
					encOP->paramChange.changePicParam
						.MindeltaQp;
				pEncInfo->openParam.HvsQpScaleDiv2 =
					encOP->paramChange.changePicParam
						.HvsQpScaleDiv2;
				pEncInfo->openParam.EnHvsQp =
					encOP->paramChange.changePicParam
						.EnHvsQp;
				pEncInfo->openParam.EnRowLevelRc =
					encOP->paramChange.changePicParam
						.EnRowLevelRc;
				pEncInfo->openParam.RcHvsMaxDeltaQp =
					encOP->paramChange.changePicParam
						.RcHvsMaxDeltaQp;
				pEncInfo->openParam.rcInitDelay =
					encOP->paramChange.changePicParam
						.RcInitDelay;
				pEncInfo->openParam.userGamma =
					encOP->paramChange.changePicParam.Gamma;
				pEncInfo->openParam.rcGopIQpOffsetEn =
					encOP->paramChange.changePicParam
						.RcGopIQpOffsetEn;
				pEncInfo->openParam.rcGopIQpOffset =
					encOP->paramChange.changePicParam
						.RcGopIQpOffset;
				pEncInfo->openParam.rcWeightFactor =
					encOP->paramChange.changePicParam
						.rcWeightFactor;
#ifdef AUTO_FRM_SKIP_DROP
				pEncInfo->openParam.enAutoFrmDrop =
					encOP->paramChange.changePicParam
						.EnAuToFrmDrop;
				pEncInfo->openParam.enAutoFrmSkip =
					encOP->paramChange.changePicParam
						.EnAutoFrmSkip;
				pEncInfo->openParam.vbvThreshold =
					encOP->paramChange.changePicParam
						.VbvThreshold;
				pEncInfo->openParam.qpThreshold =
					encOP->paramChange.changePicParam
						.QpThreshold;
				pEncInfo->openParam.maxContinuosFrameDropNum =
					encOP->paramChange.changePicParam
						.MaxContinuousFrameDropNum;
				pEncInfo->openParam.maxContinuosFrameSkipNum =
					encOP->paramChange.changePicParam
						.MaxContinuousFrameSkipNum;

				frm_rc_set_auto_skip_param(
					&pEncInfo->frm_rc,
					pEncInfo->openParam.enAutoFrmSkip,
					pEncInfo->openParam.enAutoFrmDrop,
					pEncInfo->openParam.vbvThreshold,
					pEncInfo->openParam.qpThreshold,
					pEncInfo->openParam
						.maxContinuosFrameSkipNum,
					pEncInfo->openParam
						.maxContinuosFrameDropNum);
#endif // AUTO_FRM_SKIP_DROP

				if (pEncInfo->max_temporal_id != 0) {
					if (pEncInfo->gop_size !=
					    encOP->paramChange.changePicParam
						    .SetDqpNum)
						pEncInfo->openParam
							.set_dqp_pic_num =
							pEncInfo->gop_size;

					pEncInfo->openParam.enAutoFrmDrop =
						0; // encOP->paramChange.changePicParam.EnAuToFrmDrop;
					pEncInfo->openParam.enAutoFrmSkip =
						0; // encOP->paramChange.changePicParam.EnAutoFrmSkip;
				} else
					pEncInfo->openParam.set_dqp_pic_num =
						encOP->paramChange
							.changePicParam
							.SetDqpNum;

				for (idx = 0; idx < 8; idx++)
					pEncInfo->openParam.dqp[idx] =
						encOP->paramChange
							.changePicParam.dqp[idx];
			}
#endif // (RC_PIC_PARACHANGE) && (RC_CHANGE_PARAMETER_DEF)
			if (encOP->paramChange.paraEnable & EN_GOP_NUM) {
				int encoded_frames;

				encoded_frames =
					encParam->fieldRun ?
						2 * pEncInfo->encoded_frames_in_gop :
						pEncInfo->encoded_frames_in_gop;
				if (encoded_frames -
					    encOP->paramChange.NewGopNum >=
				    0)
					pEncInfo->encoded_frames_in_gop = 0;
			}
			if (encOP->paramChange.paraEnable &
			    (EN_RC_FRAME_RATE | EN_RC_BIT_RATE |
			     EN_PIC_PARA_CHANGE)) {
				int pic_width;
				int pic_height;
				int is_first_pic = (pEncInfo->frameIdx == 0 &&
						    fieldDone == FALSE);
				int frame_rate =
					(pEncInfo->openParam.frameRateInfo &
					 0xffff) /
					((pEncInfo->openParam.frameRateInfo >>
					  16) +
					 1);
				int gop_size = pEncInfo->openParam.gopSize;
				int gamma = pEncInfo->openParam.userGamma;
#ifdef CLIP_PIC_DELTA_QP
				int max_delta_qp_minus =
					pEncInfo->openParam.userMinDeltaQp;
				int max_delta_qp_plus =
					pEncInfo->openParam.userMaxDeltaQp;
#endif // CLIP_PIC_DELTA_QP
				if (pEncInfo->rotationAngle == 90 ||
				    pEncInfo->rotationAngle == 270) {
					pic_width =
						pEncInfo->openParam.picHeight;
					pic_height =
						pEncInfo->openParam.picWidth;
				} else {
					pic_width =
						pEncInfo->openParam.picWidth;
					pic_height =
						pEncInfo->openParam.picHeight;
				}

				if (pEncInfo->openParam.EncStdParam.avcParam
					    .fieldFlag) {
					pic_height = pic_height >> 1;
					frame_rate = frame_rate << 1;
					if (gop_size != 1)
						gop_size = pEncInfo->openParam
								   .gopSize
							   << 1;
				}
				if (gamma == -1)
					gamma = 1;
#ifdef CLIP_PIC_DELTA_QP
				if (max_delta_qp_minus == -1)
					max_delta_qp_minus = 51;
				if (max_delta_qp_plus == -1)
					max_delta_qp_plus = 51;
#endif // CLIP_PIC_DELTA_QP
				{
					int i;

					for (i = 0;
					     i <
					     pEncInfo->openParam.set_dqp_pic_num;
					     i++)
						pEncInfo->openParam.gopEntry[i]
							.qp_offset =
							encOP->paramChange
								.changePicParam
								.dqp[i];
				}
				frm_rc_seq_init(
					&pEncInfo->frm_rc,
					pEncInfo->openParam.bitRate * 1000,
					pEncInfo->openParam.rcInitDelay,
					frame_rate, pic_width, pic_height,
					gop_size, pEncInfo->openParam.rcEnable,
					pEncInfo->openParam.set_dqp_pic_num,
					pEncInfo->openParam.gopEntry,
					pEncInfo->openParam.LongTermDeltaQp, -1,
					is_first_pic, gamma,
					pEncInfo->openParam.rcWeightFactor
#ifdef CLIP_PIC_DELTA_QP
					,
					max_delta_qp_minus, max_delta_qp_plus
#endif // CLIP_PIC_DELTA_QP
				);
			}
		}
	}

	encParam->coda9RoiEnable = encOP->coda9RoiEnable;

	if (encOP->coda9RoiEnable) {
		encParam->roiQpMapAddr = vbRoi.phys_addr;
		encParam->coda9RoiPicAvgQp = encOP->RoiPicAvgQp;
		if (encConfig.roi_file_name[0]) {
#ifdef ROI_MB_RC
			char lineStr[256] = {
				0,
			};
			int val;
			int roiNum;
			int non_roi_qp, roi_avg_qp;
			fgets(lineStr, 256, encConfig.roi_file);
			sscanf(lineStr, "%x\n", &val);
			if (val != 0xFFFF)
				VLOG(ERR,
				     "can't find the picture delimiter\n");

			fgets(lineStr, 256, encConfig.roi_file);
			sscanf(lineStr, "%d\n", &val); // picture index

			fgets(lineStr, 256, encConfig.roi_file);
			sscanf(lineStr, "%d %d %d\n", &roiNum, &non_roi_qp,
			       &roi_avg_qp); // number of roi regions
			encParam->setROI.number = roiNum;
			encParam->nonRoiQp = non_roi_qp;
			encParam->coda9RoiPicAvgQp = roi_avg_qp;
			for (idx = 0; idx < roiNum; idx++) {
				fgets(lineStr, 256, encConfig.roi_file);
				sscanf(lineStr, "%d %d %d %d %d\n",
				       &encParam->setROI.region[idx].left,
				       &encParam->setROI.region[idx].right,
				       &encParam->setROI.region[idx].top,
				       &encParam->setROI.region[idx].bottom,
				       &encParam->setROI.qp[idx]);
			}
#else
			Uint8 *pRoiBuf;
			int pic_height = encOP->EncStdParam.avcParam.fieldFlag ?
						 encOP->picHeight / 2 :
						 encOP->picHeight;
			Int32 roiMapSize = VPU_ALIGN16(encOP->picWidth) *
					   VPU_ALIGN16(pic_height) /
					   256; // 1 Byte per 1 MB.

			pRoiBuf = (Uint8 *)osal_malloc(roiMapSize);

			if (osal_fread(pRoiBuf, 1, roiMapSize,
				       encConfig.roi_file) != roiMapSize) {
				osal_fseek(encConfig.roi_file, 0, SEEK_SET);
				osal_fread(pRoiBuf, 1, roiMapSize,
					   encConfig.roi_file);
			}

			vdi_write_memory(encConfig.coreIdx,
					 encParam->roiQpMapAddr, pRoiBuf,
					 roiMapSize, VDI_BIG_ENDIAN);
			osal_free(pRoiBuf);
#endif // ROI_MB_RC
		}
	}
}
*/

#ifdef DROP_FRAME
static int cviHandlePartialBitstreamDrop(stTestEncoder *pTestEnc, BOOL bStreamEnd)
{
	int ret = RETCODE_SUCCESS;
	CodecInst *pCodecInst = pTestEnc->handle;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	PhysicalAddress paRdPtr, paWrPtr;
	Uint8 *vaRdPtr;
	int size = 0;

	// NOTE: h265 will update pEncInfo->streamBufStartAddr,
	// h264 not change pEncInfo->streamBufStartAddr
	// Use pTestEnc->encParam.picStreamBufferAddr to get the bistStreamStartAddr
	PhysicalAddress streamBufStartAddr = pTestEnc->encParam.picStreamBufferAddr;

	ret = VPU_EncGetBitstreamBuffer(pTestEnc->handle, &paRdPtr, &paWrPtr,
			&size);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_EncGetBitstreamBuffer fail with ret %d\n", ret);
		return ret;
	}

	vaRdPtr = vdi_get_vir_addr(pTestEnc->coreIdx, paRdPtr);
	CVI_VENC_BS("coreIdx:%d\n", pTestEnc->coreIdx);

	// partial encoded bitstream size is larger than free bitstream buffer size
	// need to drop

	CVI_VENC_BS("streamBufAddr:0x%llx - 0x%llx paRdPtr:0x%llx\n",
		streamBufStartAddr, pEncInfo->streamBufEndAddr, paRdPtr);
	CVI_VENC_BS("freeStream:0x%llx UseStream:0x%x\n", paRdPtr - streamBufStartAddr, size);
	if ((streamBufStartAddr + size) > paRdPtr) {
		pTestEnc->bDrop = TRUE;
		// u32PreStreamLen is only for store size here
		pTestEnc->u32PreStreamLen += size;
		CVI_VENC_BS("Drop bitream:%dkB\n", pTestEnc->u32PreStreamLen >> 10);
	} else {
		// partial encoded bitstream size is less than free bitstream buffer size
		// rotate the partial bitstream to the start addr

		// RotateOffset+szie overwrite RdPtr
		if ((Uint64)(pTestEnc->u32BsRotateOffset + size) >= paRdPtr - streamBufStartAddr) {
			CVI_VENC_BS("RotateOffse:%lld > BuffSzie:%lld, continue\n",
					(Uint64) (pTestEnc->u32BsRotateOffset + size),
					paRdPtr - streamBufStartAddr);

			pTestEnc->bDrop = TRUE;

			CVI_VENC_BS("Drop bitream\n");
			return RETCODE_SUCCESS;
		}

#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		vdi_invalidate_ion_cache(paRdPtr, vaRdPtr, size);
#endif
		memcpy((pTestEnc->vbStream[0].virt_addr +
					pTestEnc->u32BsRotateOffset),
				vaRdPtr, size);
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		vdi_flush_ion_cache(pTestEnc->vbStream[0].phys_addr +
				pTestEnc->u32BsRotateOffset,
				pTestEnc->vbStream[0].virt_addr +
				pTestEnc->u32BsRotateOffset,
				size);
#endif
		pTestEnc->u32BsRotateOffset += size;
		CVI_VENC_BS("u32BsRotateOffset:%dKb\n", pTestEnc->u32BsRotateOffset >> 10);
	}

	if (bStreamEnd) {
		if (pTestEnc->bDrop) {
			// reset RdPtr/WrPtr
			VpuWriteReg(pCodecInst->coreIdx,
					pEncInfo->streamWrPtrRegAddr,
					streamBufStartAddr);
			pEncInfo->streamWrPtr = streamBufStartAddr;

			VpuWriteReg(pTestEnc->coreIdx,
					pEncInfo->streamRdPtrRegAddr,
					streamBufStartAddr);
			pEncInfo->streamRdPtr = streamBufStartAddr;
			CVI_VENC_BS("reset RdPtr/WrPtr, streamWrPtr:%llx streamRdPtr:%llx\n",
				pEncInfo->streamWrPtr, pEncInfo->streamRdPtr);
		} else if (pTestEnc->u32BsRotateOffset) {
			// update WrPtr to BsStartAddr + BsRotateOffset
			VpuWriteReg(pCodecInst->coreIdx,
					pEncInfo->streamWrPtrRegAddr,
					streamBufStartAddr + pTestEnc->u32BsRotateOffset);
			pEncInfo->streamWrPtr = streamBufStartAddr + pTestEnc->u32BsRotateOffset;

			// update RdPtr to BsStartAdd
			VpuWriteReg(pTestEnc->coreIdx,
					pEncInfo->streamRdPtrRegAddr,
					streamBufStartAddr);
			pEncInfo->streamRdPtr = streamBufStartAddr;

			CVI_VENC_BS("update RdPtr/WrPtr, streamWrPtr:%llx streamRdPtr:%llx\n",
				pEncInfo->streamWrPtr, pEncInfo->streamRdPtr);
		}
		pTestEnc->u32BsRotateOffset = 0;
		pTestEnc->u32PreStreamLen = 0;
	}

	return RETCODE_SUCCESS;
}
#else
static int cviHandlePartialBitstream(stTestEncoder *pTestEnc, BOOL bStreamEnd)
{
	int ret = RETCODE_SUCCESS;
	CodecInst *pCodecInst = pTestEnc->handle;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	PhysicalAddress paRdPtr, paWrPtr;
	Uint8 *vaRdPtr;
	int size = 0;

	ret = VPU_EncGetBitstreamBuffer(pTestEnc->handle, &paRdPtr, &paWrPtr,
					&size);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_EncGetBitstreamBuffer fail with ret %d\n", ret);
		return ret;
	}

	vaRdPtr = vdi_get_vir_addr(pTestEnc->coreIdx, paRdPtr);

	CVI_VC_TRACE("paRdPtr 0x%llx, paWrPtr 0x%llx, size 0x%x\n", paRdPtr,
		     paWrPtr, size);

	// using heap to save partial bitstream,
	// if partial encoded bitstream size is larger than free bitstream buffer size:
	//    => RdPtr is less than (streamBufStartAddr + partial bitstream size)
	// otherwise, rotate the partial bitstream to the start addr. of bitstream buffer
	if ((pEncInfo->streamBufStartAddr + size) > paRdPtr) {
		Uint8 *pu8PartialBitstreamBuf;

		pu8PartialBitstreamBuf =
			osal_malloc(pTestEnc->u32PreStreamLen + size);
		if (!pu8PartialBitstreamBuf) {
			CVI_VC_ERR(
				"Can not allocate memory for partial bitstream\n");
			return RETCODE_INSUFFICIENT_RESOURCE;
		}

		CVI_VC_WARN(
			"heap allocation (0x%X) for BIT_BUF_FULL interrupt!\n",
			pTestEnc->u32PreStreamLen + size);

#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		vdi_invalidate_ion_cache(paRdPtr, vaRdPtr, size);
#endif
		if (pTestEnc->pu8PreStream) {
			memcpy(pu8PartialBitstreamBuf, pTestEnc->pu8PreStream,
			       pTestEnc->u32PreStreamLen);
			memcpy(pu8PartialBitstreamBuf +
				       pTestEnc->u32PreStreamLen,
			       vaRdPtr, size);
			osal_free(pTestEnc->pu8PreStream);
			pTestEnc->pu8PreStream = NULL;
		} else {
			memcpy(pu8PartialBitstreamBuf, vaRdPtr, size);
		}

		pTestEnc->pu8PreStream = pu8PartialBitstreamBuf;
		pTestEnc->u32PreStreamLen += size;
		CVI_VC_TRACE("current partial bitstream size 0x%x\n",
			     pTestEnc->u32PreStreamLen);
	} else {
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		vdi_invalidate_ion_cache(paRdPtr, vaRdPtr, size);
#endif
		memcpy((pTestEnc->vbStream[0].virt_addr +
			pTestEnc->u32BsRotateOffset),
		       vaRdPtr, size);
#if (defined CVI_H26X_USE_ION_MEM && defined BITSTREAM_ION_CACHED_MEM)
		vdi_flush_ion_cache(pTestEnc->vbStream[0].phys_addr +
					    pTestEnc->u32BsRotateOffset,
				    pTestEnc->vbStream[0].virt_addr +
					    pTestEnc->u32BsRotateOffset,
				    size);
#endif
		pTestEnc->u32BsRotateOffset += size;

		CVI_VC_TRACE("u32BsRotateOffset 0x%x, bStreamEnd %d\n",
			     pTestEnc->u32BsRotateOffset, bStreamEnd);
		if (bStreamEnd) {
			// to update both RdPtr/WrPtr for later VPU_EncGetOutputInfo()

			// update WrPtr to (BsStartAddr + BsRotateOffset)
			VpuWriteReg(pCodecInst->coreIdx,
				    pEncInfo->streamWrPtrRegAddr,
				    pEncInfo->streamBufStartAddr +
					    pTestEnc->u32BsRotateOffset);
			pEncInfo->streamWrPtr = pEncInfo->streamBufStartAddr +
						pTestEnc->u32BsRotateOffset;
			// update RdPtr to (BsStartAddr)
			VpuWriteReg(pTestEnc->coreIdx,
				    pEncInfo->streamRdPtrRegAddr,
				    pEncInfo->streamBufStartAddr);
			pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;

			pTestEnc->u32BsRotateOffset = 0;

			CVI_VC_TRACE("streamRdPtr 0x%llx, streamWrPtr 0x%llx\n",
				     pEncInfo->streamRdPtr,
				     pEncInfo->streamWrPtr);
		}
	}

	return RETCODE_SUCCESS;
}
#endif

static int cviGetEncodedInfo(stTestEncoder *pTestEnc, int s32MilliSec)
{
	EncParam *pEncParam = &pTestEnc->encParam;
	int coreIdx = pTestEnc->coreIdx;
	int int_reason = 0;
	Uint32 timeoutCount = 0;
	int ret = RETCODE_SUCCESS;
	static Uint32 enc_cnt;
	CodStd curCodec;
	EncHandle handle = pTestEnc->handle;

	UNUSED(enc_cnt);
	while (1) {
		int_reason = VPU_WaitInterrupt(coreIdx, s32MilliSec);

		CVI_VC_TRACE("int_reason = 0x%X, timeoutCount = %d\n",
			     int_reason, timeoutCount);

		if (int_reason == -1) {
			if (s32MilliSec >= 0) {
				CVI_VC_ERR(
					"Error : encoder timeout happened in non_block mode\n");
				return TE_STA_ENC_TIMEOUT;
			}

			if (pTestEnc->interruptTimeout > 0 &&
			    timeoutCount * s32MilliSec >
				    pTestEnc->interruptTimeout) {
				CVI_VC_ERR(
					"Error : encoder timeout happened\n");
#ifdef ENABLE_CNM_DEBUG_MSG
				PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
				VPU_SWReset(coreIdx, SW_RESET_SAFETY,
					    pTestEnc->handle);
				break;
			}
			CVI_VC_TRACE(
				"VPU_WaitInterrupt ret -1, retry timeoutCount[%d]\n",
				timeoutCount);

			int_reason = 0;
			timeoutCount++;
		}

		if (pTestEnc->encOP.ringBufferEnable == TRUE) {
			if (!BitstreamReader_Act(
				    pTestEnc->bsReader,
				    pTestEnc->encOP.bitstreamBuffer,
				    pTestEnc->encOP.bitstreamBufferSize,
				    STREAM_READ_SIZE,
				    pTestEnc->comparatorBitStream)) {
#ifdef ENABLE_CNM_DEBUG_MSG
				PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
				break;
			}
		}

		if (int_reason & (1 << INT_BIT_BIT_BUF_FULL)) {
			CVI_VC_WARN("INT_BIT_BIT_BUF_FULL\n");
			handle->bBuffFull = CVI_TRUE;
			if (pTestEnc->encConfig.cviApiMode == API_MODE_SDK) {
#ifdef DROP_FRAME
				ret = cviHandlePartialBitstreamDrop(pTestEnc, FALSE);
#else
				ret = cviHandlePartialBitstream(pTestEnc, FALSE);
#endif
				if (ret != RETCODE_SUCCESS) {
					if (ret != RETCODE_INSUFFICIENT_RESOURCE) {
						CVI_VC_ERR(
						"cviHandlePartialBitstream fail\n");
						return RETCODE_FAILURE;
					}
				}
			}
		#ifdef VC_DRIVER_TEST
			else {
				BitstreamReader_Act(
					pTestEnc->bsReader,
					pTestEnc->encOP.bitstreamBuffer,
					pTestEnc->encOP.bitstreamBufferSize,
					STREAM_READ_ALL_SIZE,
					pTestEnc->comparatorBitStream);
			}
		#endif
			if (pTestEnc->encConfig.stdMode == STD_HEVC) {
				// CnM workaround for BUF_FULL interrupt (BITMONET-68)
				VpuWriteReg(coreIdx, 0x1EC, 0);
			}
		}

		if (int_reason) {
			VPU_ClearInterrupt(coreIdx);
			if (int_reason & (1 << INT_WAVE_ENC_PIC)) {
				break;
			}
		}
	}

	CVI_VC_PERF("%04u, enc time : %d ms\n", enc_cnt++,
		    (VPU_GetFrameCycle(coreIdx) /
		     vdi_get_clk_rate(coreIdx) * 1000)); // convert to ms
#ifdef DROP_FRAME
	if ((pTestEnc->encOP.bEsBufQueueEn == 1 && pTestEnc->u32BsRotateOffset != 0)
		|| (pTestEnc->bDrop == TRUE)) {
		if (cviHandlePartialBitstreamDrop(pTestEnc, TRUE) !=
		    RETCODE_SUCCESS) {
			CVI_VC_ERR("cviHandlePartialBitstream fail\n");
			return RETCODE_FAILURE;
		}
	}
#else
	if ((pTestEnc->encOP.bEsBufQueueEn == 1 && pTestEnc->u32BsRotateOffset != 0)) {
		if (cviHandlePartialBitstream(pTestEnc, TRUE) !=
			RETCODE_SUCCESS) {
			CVI_VC_ERR("cviHandlePartialBitstream fail\n");
			return RETCODE_FAILURE;
		}
	}
#endif

	ret = VPU_EncGetOutputInfo(pTestEnc->handle, &pTestEnc->outputInfo);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("VPU_EncGetOutputInfo failed Error code is 0x%x int_reason 0x%x\n",
			   ret, int_reason);
		if (ret == RETCODE_STREAM_BUF_FULL) {
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
		} else if (ret == RETCODE_MEMORY_ACCESS_VIOLATION ||
			   ret == RETCODE_CP0_EXCEPTION ||
			   ret == RETCODE_ACCESS_VIOLATION_HW) {
#ifdef REDUNDENT_CODE
			PrintMemoryAccessViolationReason(
				coreIdx, (void *)&pTestEnc->outputInfo);
#endif
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
			VPU_SWReset(coreIdx, SW_RESET_SAFETY, pTestEnc->handle);
		} else {
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
			VPU_SWReset(coreIdx, SW_RESET_SAFETY, pTestEnc->handle);
		}

		CVI_VC_ERR("VPU_EncGetOutputInfo, ret = %d\n", ret);
		return TE_ERR_ENC_OPEN;
	}
	if (pTestEnc->encOP.cviRcEn)
		cviEncRc_UpdatePicInfo(&pTestEnc->handle->rcInfo,
				      &pTestEnc->outputInfo);

	if (coreIdx == 0)
		curCodec = STD_HEVC;
	else
		curCodec = STD_AVC;

	DisplayEncodedInformation(pTestEnc->handle, curCodec,
				  &pTestEnc->outputInfo, pEncParam->srcEndFlag,
				  pTestEnc->srcFrameIdx);

	return ret;
}

static int cviCheckOutputInfo(stTestEncoder *pTestEnc)
{
	int ret = RETCODE_SUCCESS;

	if (pTestEnc->outputInfo.bitstreamWrapAround == 1) {
		CVI_VC_WARN(
			"Warnning!! BitStream buffer wrap arounded. prepare more large buffer\n");
	}

	if (pTestEnc->outputInfo.bitstreamSize == 0 &&
	    pTestEnc->outputInfo.reconFrameIndex >= 0) {
		CVI_VC_ERR("ERROR!!! bitstreamsize = 0\n");
	}

	if (pTestEnc->encOP.lineBufIntEn == 0) {
		if (pTestEnc->outputInfo.wrPtr < pTestEnc->outputInfo.rdPtr) {
			CVI_VC_ERR("wrptr < rdptr\n");
			return TE_ERR_ENC_OPEN;
		}
	}

	return ret;
}

static int cviCheckEncodeEnd(stTestEncoder *pTestEnc)
{
	EncParam *pEncParam = &pTestEnc->encParam;
	int ret = 0;

	// end of encoding
	if (pTestEnc->outputInfo.reconFrameIndex == -1) {
		CVI_VC_INFO("reconFrameIndex = -1\n");
		return TE_STA_ENC_BREAK;
	}

#ifdef ENC_RECON_FRAME_DISPLAY
	SimpleRenderer_Act();
#endif

	if (pTestEnc->frameIdx >
	    (Uint32)MAX(0, (pTestEnc->encConfig.outNum - 1))) {
		if (pTestEnc->encOP.bitstreamFormat == STD_AVC)
			ret = TE_STA_ENC_BREAK;
		else
			pEncParam->srcEndFlag = 1;
	}

	return ret;
}

static int cviCheckAndCompareBitstream(stTestEncoder *pTestEnc)
{
#ifdef ENABLE_CNM_DEBUG_MSG
	int coreIdx = pTestEnc->coreIdx;
#endif
	int ret = 0;

	if (pTestEnc->encOP.ringBufferEnable == 1) {
		ret = BitstreamReader_Act(pTestEnc->bsReader,
					  pTestEnc->encOP.bitstreamBuffer,
					  pTestEnc->encOP.bitstreamBufferSize,
					  0, pTestEnc->comparatorBitStream);
		if (ret == FALSE) {
#ifdef ENABLE_CNM_DEBUG_MSG
			PrintVpuStatus(coreIdx, pTestEnc->productId);
#endif
			return TE_ERR_ENC_OPEN;
		}
	}
#ifdef REDUNDENT_CODE
	if (pTestEnc->encConfig.outNum == 0) {
		if (pTestEnc->comparatorBitStream) {
			if (Comparator_CheckEOF(
				    pTestEnc->comparatorBitStream) == FALSE) {
				VLOG(ERR,
				     "MISMATCH BitStream data size. There is still data to compare.\n");
				return TE_ERR_ENC_OPEN;
			}
		}
	}
#endif
	return ret;
}

static void cviCloseVpuEnc(stTestEncoder *pTestEnc)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	int coreIdx = pTestEnc->coreIdx;
	FrameBufferAllocInfo *pFbAllocInfo = &pTestEnc->fbAllocInfo;
	int i;
	int buf_cnt;
	cviEncCfg *pcviEc = &pTestEnc->encConfig.cviEc;

	if (pTestEnc->encConfig.cviEc.singleLumaBuf > 0) {
		buf_cnt = 1;
		// smart-p with additional 1 framebuffer
		if (pTestEnc->encConfig.cviEc.virtualIPeriod > 0) {
			buf_cnt += 1;
		}
	} else {
		buf_cnt = pTestEnc->regFrameBufCount;
	}

	if (pcviEc->VbBufCfg.VbType == VB_SOURCE_TYPE_PRIVATE ||
	    pcviEc->VbBufCfg.VbType == VB_SOURCE_TYPE_USER) {
		//skip dma free under non common mode

	} else {
		for (i = 0; i < buf_cnt; i++) {
			if (pTestEnc->vbReconFrameBuf[i].size > 0) {
				VDI_FREE_MEMORY(coreIdx,
						&pTestEnc->vbReconFrameBuf[i]);
			}
		}
	}
	for (i = 0; i < pFbAllocInfo->num; i++) {
		if (pTestEnc->vbSourceFrameBuf[i].size > 0) {
			VDI_FREE_MEMORY(coreIdx,
					&pTestEnc->vbSourceFrameBuf[i]);
		}
		if (pTestEnc->vbRoi[i].size) {
			VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbRoi[i]);
		}
		if (pTestEnc->vbCtuQp[i].size) {
			VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbCtuQp[i]);
		}
		if (pTestEnc->vbPrefixSeiNal[i].size)
			VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbPrefixSeiNal[i]);
		if (pTestEnc->vbSuffixSeiNal[i].size)
			VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbSuffixSeiNal[i]);
#ifdef TEST_ENCODE_CUSTOM_HEADER
		if (pTestEnc->vbSeiNal[i].size)
			VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbSeiNal[i]);
#endif
	}

	if (pTestEnc->vbHrdRbsp.size)
		VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbHrdRbsp);

	if (pTestEnc->vbVuiRbsp.size)
		VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbVuiRbsp);

#ifdef TEST_ENCODE_CUSTOM_HEADER
	if (pTestEnc->vbCustomHeader.size)
		VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbCustomHeader);
#endif

	VPU_EncClose(pTestEnc->handle);
	CoreSleepWake(coreIdx, 1); //ensure vpu sleep

	if (pEncOP->bitstreamFormat == STD_AVC) {
		CVI_VC_INFO("\ninst %d Enc End. Tot Frame %d\n",
			    pTestEnc->encConfig.instNum,
			    pTestEnc->outputInfo.encPicCnt);
	} else {
		CVI_VC_INFO("\ninst %d Enc End. Tot Frame %d\n",
			    pTestEnc->encConfig.instNum,
			    pTestEnc->outputInfo.encPicCnt);
	}
}

static void cviDeInitVpuEnc(stTestEncoder *pTestEnc)
{
	void *pYuvFeeder = pTestEnc->yuvFeeder;
	int coreIdx = pTestEnc->coreIdx;
	Uint32 i = 0;

	for (i = 0; i < pTestEnc->bsBufferCount; i++) {
		if (pTestEnc->vbStream[i].size) {
			if (vdi_get_is_single_es_buf(coreIdx))
				vdi_free_single_es_buf_memory(
					coreIdx, &pTestEnc->vbStream[i]);
			else
				VDI_FREE_MEMORY(coreIdx,
						&pTestEnc->vbStream[i]);
		}
	}

	if (pTestEnc->vbReport.size)
		VDI_FREE_MEMORY(coreIdx, &pTestEnc->vbReport);
#ifdef REDUNDENT_CODE
	if (pTestEnc->comparatorBitStream != NULL) {
		Comparator_Destroy(pTestEnc->comparatorBitStream);
		osal_free(pTestEnc->comparatorBitStream);
	}
#endif

	BitstreamReader_Destroy(pTestEnc->bsReader);

	if (pYuvFeeder != NULL) {
		YuvFeeder_Destroy(pYuvFeeder);
		osal_free(pYuvFeeder);
	}

	VPU_DeInit(coreIdx);
}

static void *cviInitEncoder(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig)
{
	int ret = 0;

	ret = initEncoder(pTestEnc, pEncConfig);
	if (ret == TE_ERR_ENC_INIT)
		goto INIT_ERR_ENC_INIT;
	else if (ret == TE_ERR_ENC_OPEN)
		goto INIT_ERR_ENC_OPEN;

	return (void *)pTestEnc;

INIT_ERR_ENC_OPEN:
	cviCloseVpuEnc(pTestEnc);
INIT_ERR_ENC_INIT:
	cviDeInitVpuEnc(pTestEnc);
	return NULL;
}

int initEncoder(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig)
{
	int ret = 0;

	VDI_POWER_ON_DOING_JOB(pEncConfig->coreIdx, ret,
			       initEncOneFrame(pTestEnc, pEncConfig));

	if (ret == TE_ERR_ENC_INIT) {
		CVI_VC_ERR("TE_ERR_ENC_INIT\n");
		return ret;
	} else if (ret == TE_ERR_ENC_OPEN) {
		// TODO -- error handling
		CVI_VC_ERR("TE_ERR_ENC_OPEN\n");
		return ret;
	}
#ifdef DUMP_MOTIONMAP
	pTestEnc->encConfig.filp = filp_open("motionmap.bin", O_RDWR | O_CREAT, 0644);
	pEncConfig->pos = 0;
#endif
	return ret;
}

void cvi_vc_get_motion_tbl(void *arg)
{
	struct base_exe_m_cb exe_cb;
	/* Notify vi to send buffer as soon as possible */
	exe_cb.callee = E_MODULE_VPSS;
	exe_cb.caller = E_MODULE_VCODEC;
	exe_cb.cmd_id = VPSS_CB_GET_MLV_INFO;
	exe_cb.data   = arg;
	base_exe_module_cb(&exe_cb);
}


void cviCopyMotionMap(void *handle, cviEncOnePicCfg *pPicCfg, void *phandle)
{
	struct vpss_grp_mlv_info *mlv_info = NULL;
	CodecInst *pCodecInst = NULL;
	stTestEncoder *pTestEnc = (stTestEncoder *) handle;

	if (!pTestEnc) {
		return;
	}
	pCodecInst = pTestEnc->handle;
	if (!pCodecInst)
		return;

	mlv_info = kzalloc(sizeof(*mlv_info), GFP_ATOMIC);
	if (!mlv_info) {
		CVI_VC_ERR("fail to kzalloc(%zu)\n", sizeof(struct vpss_grp_mlv_info));
		return;
	}
	if (pCodecInst->CodecInfo->encInfo.bSbmEn) {
		mlv_info->vpss_grp = 0;
		cvi_vc_get_motion_tbl((void *) mlv_info);
		pPicCfg->picMotionLevel = mlv_info->m_lv_i.mlv_i_level;
		pPicCfg->picDciLv = 0;
		pPicCfg->picMotionMapSize = MO_TBL_SIZE;
		memcpy(pPicCfg->picMotionMap, mlv_info->m_lv_i.mlv_i_table, MO_TBL_SIZE);
	}

	kfree(mlv_info);
}

static int cviVEncStartSb(stTestEncoder *pTestEnc)
{
	int reg = 0;

	// Set Register 0x0
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);

	// bit7  sbc0 h265
	// bit11 sbc1 h264
	// bit15 sbc2 jpeg
	if (pTestEnc->encOP.bitstreamFormat == STD_AVC)
		reg |= (1 << 11);
	else if (pTestEnc->encOP.bitstreamFormat == STD_HEVC)
		reg |= (1 << 7);
	else {
		CVI_VC_ERR("bitstreamFormat:%d not support", pTestEnc->encOP.bitstreamFormat);
		return -1;
	}

	cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);

	return RETCODE_SUCCESS;
}


int cviVEncEncOnePic(void *handle, cviEncOnePicCfg *pPicCfg, int s32MilliSec)
{
	int ret = 0;
	stTestEncoder *pTestEnc = (stTestEncoder *)handle;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	CodecInst *pCodecInst = pTestEnc->handle;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 ts;
#else
	struct timespec ts;
#endif
	CVI_VC_IF("\n");

	ret = cviVPU_LockAndCheck(pTestEnc->handle, CODEC_STAT_ENC_PIC,
				  s32MilliSec);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_TRACE("cviVPU_LockAndCheck, ret = %d\n", ret);
		if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
			return RET_VCODEC_TIMEOUT;
			//timeout return
		} else {
			CVI_VC_ERR(
				"cviVPU_LockAndCheck error[%d]s32MilliSec[%d]\n",
				ret, s32MilliSec);
			return ret;
		}
	}

	pTestEnc->yuvAddr.addrY = pPicCfg->addrY;
	pTestEnc->yuvAddr.addrCb = pPicCfg->addrCb;
	pTestEnc->yuvAddr.addrCr = pPicCfg->addrCr;

	pTestEnc->yuvAddr.phyAddrY = pPicCfg->phyAddrY;
	pTestEnc->yuvAddr.phyAddrCb = pPicCfg->phyAddrCb;
	pTestEnc->yuvAddr.phyAddrCr = pPicCfg->phyAddrCr;
	pTestEnc->u64Pts = pPicCfg->u64Pts;

	if (pCodecInst->CodecInfo->encInfo.bSbmEn) {
		ret = cviSetSbSetting(&pTestEnc->SbSetting);
		if (ret != RETCODE_SUCCESS) {
			CVI_VENC_ERR("cviSetSbSetting fail, s32Ret=%d\n", ret);
		}

		ret = cviVEncStartSb(pTestEnc);
		if (ret != RETCODE_SUCCESS) {
			CVI_VC_ERR("StartSb fail\n");
			LeaveVcodecLock(pTestEnc->coreIdx);
			return ret;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_ts64(&ts);
#else
		ktime_get_ts(&ts);
#endif
		if (pTestEnc->u64Pts == 0) {
			pTestEnc->u64Pts = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
		}
	}

	pTestEnc->srcFrameStride = pPicCfg->stride;
	pEncOP->cbcrInterleave = pPicCfg->cbcrInterleave;
	pEncOP->nv21 = pPicCfg->nv21;
	pEncOP->picMotionLevel = pPicCfg->picMotionLevel;
	pEncOP->picDciLv = pPicCfg->picDciLv;
	pEncOP->picMotionMap = pPicCfg->picMotionMap;
	pEncOP->picMotionMapSize = pPicCfg->picMotionMapSize;

	CVI_VC_TRACE("addrY = 0x%p, addrCb = 0x%p, addrCr = 0x%p\n",
		     pTestEnc->yuvAddr.addrY, pTestEnc->yuvAddr.addrCb,
		     pTestEnc->yuvAddr.addrCr);
	CVI_VC_TRACE(
		"phyAddrY = 0x%llX, phyAddrCb = 0x%llX, phyAddrCr = 0x%llX\n",
		pTestEnc->yuvAddr.phyAddrY, pTestEnc->yuvAddr.phyAddrCb,
		pTestEnc->yuvAddr.phyAddrCr);
	CVI_VC_TRACE("motion lv = %d, mSize = %d\n", pPicCfg->picMotionLevel,
		     pPicCfg->picMotionMapSize);

	ret = cviEncodeOneFrame(pTestEnc);

	if (ret == TE_ERR_ENC_OPEN || ret == TE_STA_ENC_BREAK) {
		CVI_VC_INFO("cviEncodeOneFrame, ret = %d\n", ret);
		cviVPU_ChangeState(pTestEnc->handle);
		LeaveVcodecLock(pTestEnc->coreIdx);
	} else if (ret == RETCODE_SUCCESS && pTestEnc->encConfig.bIsoSendFrmEn &&
		!pTestEnc->handle->rcInfo.isReEncodeIdr) {
		complete(&pTestEnc->semSendEncCmd);
	}

	return ret;
}

int cviVEncGetStream(void *handle, cviVEncStreamInfo *pStreamInfo,
		     int s32MilliSec)
{
	stTestEncoder *pTestEnc = (stTestEncoder *)handle;
	int ret = RETCODE_SUCCESS;

	CVI_VC_IF("\n");

	if (pTestEnc->encConfig.bIsoSendFrmEn) {
		ret = wait_for_completion_timeout(&pTestEnc->semGetStreamCmd,
				usecs_to_jiffies(s32MilliSec * 1000));
		if (ret == 0) {
			CVI_VC_WARN("get stream timeout!\n");
			return RET_VCODEC_TIMEOUT;
		}
		memcpy(pStreamInfo, &pTestEnc->tStreamInfo, sizeof(cviVEncStreamInfo));
		return RETCODE_SUCCESS;
	}

	pTestEnc->encConfig.cviEc.originPicType = PIC_TYPE_MAX;

	ret = cviGetOneStream(handle, pStreamInfo, s32MilliSec);

	if (ret == TE_ERR_ENC_IS_SUPER_FRAME)
		ret = cviProcessSuperFrame(pTestEnc, pStreamInfo, s32MilliSec);

	return ret;
}

// should mutex psp->packMutex in caller
static CVI_S32 cviDropEsPacks(stStreamPack *psp)
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

static int cviGetOneStream(void *handle, cviVEncStreamInfo *pStreamInfo,
			   int s32MilliSec)
{
	stTestEncoder *pTestEnc = (stTestEncoder *)handle;
	cviEncCfg *pCviEc = &pTestEnc->encConfig.cviEc;
	EncOpenParam *peop = &pTestEnc->encOP;
	EncOutputInfo *pOutputInfo = &pTestEnc->outputInfo;
	CodecInst *pCodecInst = pTestEnc->handle;
	int ret = RETCODE_SUCCESS;

	CVI_VC_IF("\n");

	ret = cviGetEncodedInfo(pTestEnc, s32MilliSec);
	if (ret == TE_STA_ENC_TIMEOUT) {
		//user should keep get frame until success
		CVI_VC_TRACE("Time out please retry\n");
		return RET_VCODEC_TIMEOUT;
	} else if (ret) {
		CVI_VC_ERR("cviGetEncodedInfo, ret = %d\n", ret);
		goto ERR_CVI_VENC_GET_STREAM;
	}

	pStreamInfo->encHwTime = pOutputInfo->encHwTime;
	pStreamInfo->u32MeanQp = pOutputInfo->u32MeanQp;


#ifdef DROP_FRAME
	if (pTestEnc->bDrop) {
		CVI_VENC_BS("Drop isIdr:%d\n", pTestEnc->encParam.is_idr_frame);

		// if drop IDR, need clear vps sps pps header
		if (pTestEnc->encParam.is_idr_frame) {
			MUTEX_LOCK(&pTestEnc->streamPack.packMutex);
			cviDropEsPacks(&pTestEnc->streamPack);
			MUTEX_UNLOCK(&pTestEnc->streamPack.packMutex);
		}
		ret = RETCODE_SUCCESS;

		goto ERR_CVI_VENC_GET_STREAM;
	}
#endif

	if (pTestEnc->encOP.ringBufferEnable == 0) {
		ret = cviCheckOutputInfo(pTestEnc);
		if (ret == TE_ERR_ENC_OPEN) {
			CVI_VC_ERR("cviCheckOutputInfo, ret = %d\n", ret);
			goto ERR_CVI_VENC_GET_STREAM;
		}

		if (pOutputInfo->bitstreamSize) {
			ret = cviPutEsInPack(pTestEnc,
					     pOutputInfo->bitstreamBuffer,
					     pOutputInfo->bitstreamBuffer +
						     peop->bitstreamBufferSize,
					     pOutputInfo->bitstreamSize,
					     NAL_I + pOutputInfo->picType);

			if (ret == FALSE) {
				CVI_VC_ERR("cviPutEsInPack, ret = %d\n", ret);
				ret = TE_ERR_ENC_OPEN;
				goto ERR_CVI_VENC_GET_STREAM;
			} else {
				ret = RETCODE_SUCCESS;
			}
		}

		if (pOutputInfo->bitstreamSize)
			pStreamInfo->psp = &pTestEnc->streamPack;

		CVI_VC_TRACE("frameIdx = %d, isLastPicI = %d, size = %d\n",
			     pTestEnc->frameIdx,
			     (pOutputInfo->picType == PIC_TYPE_I) ||
				     (pOutputInfo->picType == PIC_TYPE_IDR),
			     pOutputInfo->bitstreamSize << 3);

		if (pCviEc->enSuperFrmMode != CVI_SUPERFRM_NONE)
			ret = cviCheckSuperFrame(pTestEnc);
	}

ERR_CVI_VENC_GET_STREAM:
	cviVPU_ChangeState(pTestEnc->handle);
	// if NOT sharing es buffer, we can unlock to speed-up enc process
	if (!pCviEc->bSingleEsBuf) {
		LeaveVcodecLock(pTestEnc->coreIdx);
	}

	if (pCodecInst->CodecInfo->encInfo.bSbmEn == CVI_TRUE) {
		cvi_VENC_SBM_IrqEnable();
	}

	return ret;
}

static int cviCheckSuperFrame(stTestEncoder *pTestEnc)
{
	EncOutputInfo *pOutputInfo = &pTestEnc->outputInfo;
	unsigned int currFrmBits = pOutputInfo->bitstreamSize << 3;
	cviEncCfg *pCviEc = &pTestEnc->encConfig.cviEc;
	stRcInfo *pRcInfo = &pTestEnc->handle->rcInfo;
	int isSuperFrame = 0;

	if (pCviEc->originPicType == PIC_TYPE_MAX)
		pCviEc->originPicType = pOutputInfo->picType;

	if (pCviEc->originPicType == PIC_TYPE_I ||
	    pCviEc->originPicType == PIC_TYPE_IDR) {
		CVI_VC_FLOW("currFrmBits = %d, SuperIFrmBitsThr = %d\n",
			    currFrmBits, pCviEc->u32SuperIFrmBitsThr);

		if (currFrmBits > pCviEc->u32SuperIFrmBitsThr) {
			isSuperFrame = TE_ERR_ENC_IS_SUPER_FRAME;
			pRcInfo->s32SuperFrmBitsThr =
				(int)pCviEc->u32SuperIFrmBitsThr;
		}
	} else if (pCviEc->originPicType == PIC_TYPE_P) {
		CVI_VC_FLOW("currFrmBits = %d, SuperPFrmBitsThr = %d\n",
			    currFrmBits, pCviEc->u32SuperPFrmBitsThr);

		if (currFrmBits > pCviEc->u32SuperPFrmBitsThr) {
			isSuperFrame = TE_ERR_ENC_IS_SUPER_FRAME;
			pRcInfo->s32SuperFrmBitsThr =
				(int)pCviEc->u32SuperPFrmBitsThr;
		}
	}

	return isSuperFrame;
}

static int cviProcessSuperFrame(stTestEncoder *pTestEnc,
				cviVEncStreamInfo *pStreamInfo, int s32MilliSec)
{
	cviEncCfg *pCviEc = &pTestEnc->encConfig.cviEc;
	stRcInfo *pRcInfo = &pTestEnc->handle->rcInfo;
	int currReEncodeTimes = 0;
	int ret = 0;

	pRcInfo->isReEncodeIdr = 1;

	while (currReEncodeTimes++ < pCviEc->s32MaxReEncodeTimes) {
		ret = cviReEncodeIDR(pTestEnc, pStreamInfo, s32MilliSec);
		if (ret != TE_ERR_ENC_IS_SUPER_FRAME)
			break;
	}

	if (ret == TE_ERR_ENC_IS_SUPER_FRAME) {
		CVI_VC_FLOW("only re-Enccode IDR %d time\n",
			    pCviEc->s32MaxReEncodeTimes);
		ret = 0;
	}

	pRcInfo->isReEncodeIdr = 0;
	CVI_VC_FLOW("re-Enccode IDR %d time\n", pCviEc->s32MaxReEncodeTimes);

	return ret;
}

static int cviReEncodeIDR(stTestEncoder *pTestEnc,
			  cviVEncStreamInfo *pStreamInfo, int s32MilliSec)
{
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	cviEncOnePicCfg picCfg, *pPicCfg = &picCfg;
	int ret = 0;
	int totalPacks = 0;
	stPack *pPack;

	CVI_VC_FLOW("\n");

	pPicCfg->addrY = pTestEnc->yuvAddr.addrY;
	pPicCfg->addrCb = pTestEnc->yuvAddr.addrCb;
	pPicCfg->addrCr = pTestEnc->yuvAddr.addrCr;

	pPicCfg->phyAddrY = pTestEnc->yuvAddr.phyAddrY;
	pPicCfg->phyAddrCb = pTestEnc->yuvAddr.phyAddrCb;
	pPicCfg->phyAddrCr = pTestEnc->yuvAddr.phyAddrCr;

	pPicCfg->stride = pTestEnc->srcFrameStride;
	pPicCfg->cbcrInterleave = pEncOP->cbcrInterleave;
	pPicCfg->nv21 = pEncOP->nv21;
	pPicCfg->picMotionLevel = pEncOP->picMotionLevel;
	pPicCfg->picMotionMap = pEncOP->picMotionMap;
	pPicCfg->picMotionMapSize = pEncOP->picMotionMapSize;

	pTestEnc->encParam.idr_request = TRUE;

	MUTEX_LOCK(&pStreamInfo->psp->packMutex);
	// only drop latest super frame
	totalPacks = pStreamInfo->psp->totalPacks;
	if (totalPacks >= 1) {
		pPack = &(pStreamInfo->psp->pack[totalPacks-1]);
		if (pPack) {
			pPack->bUsed = TRUE;
		}

		// I frame need drop vps/sps/pps
		if (pPack->cviNalType == NAL_I || pPack->cviNalType == NAL_IDR) {
			// h265 idr include vps/sps/pps
			if (totalPacks >= 4 && pStreamInfo->psp->pack[totalPacks-2].cviNalType == NAL_PPS
				&& pStreamInfo->psp->pack[totalPacks-3].cviNalType == NAL_SPS
				&& pStreamInfo->psp->pack[totalPacks-4].cviNalType == NAL_VPS) {
				pStreamInfo->psp->pack[totalPacks-2].bUsed = true;
				pStreamInfo->psp->pack[totalPacks-3].bUsed = true;
				pStreamInfo->psp->pack[totalPacks-4].bUsed = true;
			}

			// h264 idr include sps/pps
			if (totalPacks >= 3
				&& pStreamInfo->psp->pack[totalPacks-2].cviNalType == NAL_PPS
				&& pStreamInfo->psp->pack[totalPacks-3].cviNalType == NAL_SPS) {
				pStreamInfo->psp->pack[totalPacks-2].bUsed = true;
				pStreamInfo->psp->pack[totalPacks-3].bUsed = true;
			}
		}
	}
	MUTEX_UNLOCK(&pStreamInfo->psp->packMutex);

	ret = cviVEncReleaseStream((void *)pTestEnc, pStreamInfo);
	if (ret) {
		CVI_VC_ERR("cviVEncReleaseStream, %d\n", ret);
		return ret;
	}

	ret = cviVEncEncOnePic((void *)pTestEnc, pPicCfg, 20000);
	if (ret != 0) {
		CVI_VC_ERR("cviVEncEncOnePic, %d\n", ret);
		return ret;
	}

	ret = cviGetOneStream((void *)pTestEnc, pStreamInfo, s32MilliSec);
	if (ret != 0 && ret != TE_ERR_ENC_IS_SUPER_FRAME) {
		CVI_VC_ERR("cviGetOneStream, %d\n", ret);
		return ret;
	}

	return ret;
}

int cviVEncReleaseStream(void *handle, cviVEncStreamInfo *pStreamInfo)
{
	stTestEncoder *pTestEnc = (stTestEncoder *)handle;
	cviEncCfg *pCviEc = &pTestEnc->encConfig.cviEc;
	stStreamPack *psp = pStreamInfo->psp;
	stPack *pPack;
	int ret = 0;
	int idx;
	int j = 0;
	int usedCnt = 0;

	CVI_VC_IF("\n");

	MUTEX_LOCK(&psp->packMutex);
	if (psp->totalPacks) {
		for (idx = 0; idx < psp->totalPacks; idx++) {
			pPack = &psp->pack[idx];

			if (pPack->bUsed) {
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
				usedCnt++;
			}
		}
		psp->totalPacks -= usedCnt;
	}
	if (psp->totalPacks) {
		for (idx = 0; idx < MAX_NUM_PACKS; idx++) {
			pPack = &psp->pack[idx];
			if (pPack->len && !pPack->bUsed && pPack->addr) {
				memcpy(&psp->pack[j], pPack, sizeof(stPack));
				j++;
			}
		}
	}
	MUTEX_UNLOCK(&psp->packMutex);

	ret = cviCheckEncodeEnd(pTestEnc);
	if (ret == TE_STA_ENC_BREAK) {
		CVI_VC_INFO("TE_STA_ENC_BREAK\n");
	}

	// if sharing es buffer, to unlock when releasing frame
	if (pCviEc->bSingleEsBuf) {
		LeaveVcodecLock(pTestEnc->coreIdx);
	}

	return ret;
}

static int cviVEncSetRequestIDR(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;

	UNREFERENCED_PARAMETER(arg);

	CVI_VC_IF("\n");

	pTestEnc->encParam.idr_request = TRUE;

	return ret;
}

int _cviVencCalculateBufInfo(stTestEncoder *pTestEnc, int *butcnt, int *bufsize)
{
	int FrameBufSize;
	int framebufStride = 0;

	if (pTestEnc->bIsEncoderInited == false) {
		CVI_VC_ERR("\n");
		return TE_ERR_ENC_INIT;
	}

	// reference frame
	// calculate stride / size / apply variable
	framebufStride =
		CalcStride(pTestEnc->framebufWidth, pTestEnc->framebufHeight,
			   pTestEnc->encOP.srcFormat,
			   pTestEnc->encOP.cbcrInterleave,
			   (TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
			   FALSE, FALSE);
	FrameBufSize = VPU_GetFrameBufSize(
		pTestEnc->encConfig.coreIdx, framebufStride,
		pTestEnc->framebufHeight,
		(TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
		pTestEnc->encOP.srcFormat, pTestEnc->encOP.cbcrInterleave,
		NULL);

	pTestEnc->regFrameBufCount = pTestEnc->initialInfo.minFrameBufferCount;
	*butcnt = pTestEnc->regFrameBufCount;
	*bufsize = FrameBufSize;

	return 0;
}

static int cviVEncGetVbInfo(stTestEncoder *pTestENC, void *arg)
{
	int ret = 0;
	cviVbVencBufConfig *pcviVbVencCfg = (cviVbVencBufConfig *)arg;

	CVI_VC_IF("\n");

	ret = _cviVencCalculateBufInfo(pTestENC,
				       &pcviVbVencCfg->VbGetFrameBufCnt,
				       &pcviVbVencCfg->VbGetFrameBufSize);

	return ret;
}

static int cviVEncSetVbBuffer(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	cviVbVencBufConfig *pcviVbVencCfg = (cviVbVencBufConfig *)arg;

	CVI_VC_IF("\n");

	CVI_VC_TRACE("[Before]cviVEncSetVbBuffer type[%d][%d] [%d]\n",
		     pcviVbVencCfg->VbType, pcviVbVencCfg->VbSetFrameBufCnt,
		     pcviVbVencCfg->VbSetFrameBufSize);

	memcpy(&pTestEnc->encConfig.cviEc.VbBufCfg, pcviVbVencCfg,
	       sizeof(cviVbVencBufConfig));

	CVI_VC_TRACE(
		"[After MW Venc]cviVEncSetVbBuffer again type[%d][%d] [%d]\n",
		pTestEnc->encConfig.cviEc.VbBufCfg.VbType,
		pTestEnc->encConfig.cviEc.VbBufCfg.VbSetFrameBufCnt,
		pTestEnc->encConfig.cviEc.VbBufCfg.VbSetFrameBufSize);

	return ret;
}

static int cviVEncRegReconBuf(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	int core_idx =
		pTestEnc->encConfig.stdMode == STD_AVC ? CORE_H264 : CORE_H265;

	UNREFERENCED_PARAMETER(arg);

	CVI_VC_IF("\n");

	EnterVcodecLock(core_idx);

	ret = _cviVEncRegFrameBuffer(pTestEnc, arg);

	LeaveVcodecLock(core_idx);

	return ret;
}

static int _cviVEncRegFrameBuffer(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	int coreIdx;
	int FrameBufSize;
	int framebufStride = 0;
	FrameBufferAllocInfo *pFbAllocInfo = &pTestEnc->fbAllocInfo;
	cviEncCfg *pcviEc = &pTestEnc->encConfig.cviEc;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	int i, mapType;
	CodecInst *pCodecInst = pTestEnc->handle;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	UNREFERENCED_PARAMETER(arg);

	coreIdx = pTestEnc->encConfig.coreIdx;

	/* Allocate framebuffers */
	framebufStride =
		CalcStride(pTestEnc->framebufWidth, pTestEnc->framebufHeight,
			   pTestEnc->encOP.srcFormat,
			   pTestEnc->encOP.cbcrInterleave,
			   (TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
			   FALSE, FALSE);
	FrameBufSize = VPU_GetFrameBufSize(
		pTestEnc->encConfig.coreIdx, framebufStride,
		pTestEnc->framebufHeight,
		(TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
		pTestEnc->encOP.srcFormat, pTestEnc->encOP.cbcrInterleave,
		NULL);
	pTestEnc->regFrameBufCount = pTestEnc->initialInfo.minFrameBufferCount;

	CVI_VC_TRACE(
		"framebufWidth = %d, framebufHeight = %d, framebufStride = %d\n",
		pTestEnc->framebufWidth, pTestEnc->framebufHeight,
		framebufStride);
	CVI_VC_MEM("FrameBufSize = 0x%X, regFrameBufCount = %d\n", FrameBufSize,
		   pTestEnc->regFrameBufCount);

	if (pEncOP->addrRemapEn)
		pcviEc->singleLumaBuf = 0;

	pCodecInst->CodecInfo->encInfo.singleLumaBuf = pcviEc->singleLumaBuf;

	if (pcviEc->singleLumaBuf > 0) {
		int ber, aft;
		int luma_size, chr_size;
		int productId;

		productId = ProductVpuGetId(coreIdx);
		luma_size = CalcLumaSize(
			productId, framebufStride, pTestEnc->framebufHeight,
			pTestEnc->encOP.cbcrInterleave,
			(TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
			NULL);

		chr_size = CalcChromaSize(
			productId, framebufStride, pTestEnc->framebufHeight,
			pTestEnc->encOP.srcFormat,
			pTestEnc->encOP.cbcrInterleave,
			(TiledMapType)(pTestEnc->encConfig.mapType & 0x0f),
			NULL);

		i = 0;
		// smart-p case
		if (pcviEc->virtualIPeriod > 0 &&
		    pTestEnc->regFrameBufCount > 2) {
			if (pcviEc->VbBufCfg.VbType == VB_SOURCE_TYPE_COMMON) {
				for (i = 0;
				     i < (pTestEnc->regFrameBufCount - 2);
				     i++) {
					pTestEnc->vbReconFrameBuf[i].size =
						FrameBufSize;

					sprintf(ionName,
						"VENC_%d_ReconFrameBuf",
						pCodecInst->s32ChnNum);
					if (VDI_ALLOCATE_MEMORY(
						    coreIdx,
						    &pTestEnc->vbReconFrameBuf[i],
						    0, ionName) < 0) {
						CVI_VC_ERR(
							"fail to allocate recon buffer\n");
						return TE_ERR_ENC_OPEN;
					}

					pTestEnc->fbRecon[i].bufY =
						pTestEnc->vbReconFrameBuf[i]
							.phys_addr;
					pTestEnc->fbRecon[i].bufCb =
						(PhysicalAddress)-1;
					pTestEnc->fbRecon[i].bufCr =
						(PhysicalAddress)-1;
					pTestEnc->fbRecon[i].size =
						FrameBufSize;
					pTestEnc->fbRecon[i].updateFbInfo =
						TRUE;
				}
			} else {
				for (i = 0;
				     i < (pTestEnc->regFrameBufCount - 2);
				     i++) {
					int FrameBufSizeSet =
						pcviEc->VbBufCfg
							.VbSetFrameBufSize;
					if (FrameBufSize > FrameBufSizeSet) {
						CVI_VC_ERR(
							"FrameBufSize > FrameBufSizeSet\n");
						return TE_ERR_ENC_OPEN;
					}

					pTestEnc->vbReconFrameBuf[i].size =
						FrameBufSizeSet;
					pTestEnc->fbRecon[i].bufY =
						pcviEc->VbBufCfg.vbModeAddr[i];
					pTestEnc->fbRecon[i].size =
						FrameBufSizeSet;
				}
			}
		}

		aft = i;
		ber = aft + 1;

		if (pcviEc->VbBufCfg.VbType != VB_SOURCE_TYPE_COMMON) {
			int FrameBufSizeSet =
				pcviEc->VbBufCfg.VbSetFrameBufSize;

			CVI_VC_TRACE(" LumbBuf FrameBufSize[%d] set in[%d]\n",
				     FrameBufSize, FrameBufSizeSet);
			if (FrameBufSize > FrameBufSizeSet) {
				CVI_VC_ERR("FrameBufSize > FrameBufSizeSet\n");
				return TE_ERR_ENC_OPEN;
			}

			pTestEnc->vbReconFrameBuf[ber].size =
				FrameBufSizeSet + chr_size * 2;
			pTestEnc->fbRecon[ber].bufY =
				pcviEc->VbBufCfg.vbModeAddr[ber];
			pTestEnc->fbRecon[ber].size = FrameBufSizeSet;
		} else {
			pTestEnc->vbReconFrameBuf[ber].size =
				FrameBufSize + chr_size * 2;
			//pTestEnc->vbReconFrameBuf[1].size = FrameBufSize * 2;
			memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
			sprintf(ionName, "VENC_%d_ReconFrameBuf",
				pCodecInst->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(coreIdx,
						&pTestEnc->vbReconFrameBuf[ber],
						0, ionName) < 0) {
				CVI_VC_ERR("fail to allocate recon buffer\n");
				return TE_ERR_ENC_OPEN;
			}

			pTestEnc->fbRecon[ber].bufY =
				pTestEnc->vbReconFrameBuf[ber].phys_addr;
			pTestEnc->fbRecon[ber].size = FrameBufSize;
		}
		pTestEnc->fbRecon[aft].bufY = pTestEnc->fbRecon[ber].bufY;
		pTestEnc->fbRecon[ber].bufCb =
			pTestEnc->fbRecon[aft].bufY + luma_size;
		pTestEnc->fbRecon[ber].bufCr =
			pTestEnc->fbRecon[ber].bufCb + chr_size;
		pTestEnc->fbRecon[aft].bufCb =
			pTestEnc->fbRecon[ber].bufCr + chr_size;
		pTestEnc->fbRecon[aft].bufCr =
			pTestEnc->fbRecon[aft].bufCb + chr_size;

		pTestEnc->fbRecon[ber].updateFbInfo = TRUE;
		memcpy(&pTestEnc->vbReconFrameBuf[aft],
		       &pTestEnc->vbReconFrameBuf[ber], sizeof(vpu_buffer_t));
		pTestEnc->fbRecon[aft].size = pTestEnc->fbRecon[ber].size;
		pTestEnc->fbRecon[aft].updateFbInfo = TRUE;
	} else {
		if (pcviEc->VbBufCfg.VbType == VB_SOURCE_TYPE_COMMON) {

			if (pEncOP->addrRemapEn) {
				ret = cviInitAddrRemap(pTestEnc);
				if (ret != 0) {
					CVI_VC_ERR("cviInitAddrRemap\n");
					return ret;
				}
			} else {
				for (i = 0; i < pTestEnc->regFrameBufCount; i++) {
					pTestEnc->vbReconFrameBuf[i].size =
						FrameBufSize;

					memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
					sprintf(ionName, "VENC_%d_ReconFrameBuf",
							pCodecInst->s32ChnNum);
					if (VDI_ALLOCATE_MEMORY(
								coreIdx,
								&pTestEnc->vbReconFrameBuf[i], 0,
								ionName) < 0) {
						CVI_VC_ERR(
								"fail to allocate recon buffer\n");
						return TE_ERR_ENC_OPEN;
					}

					pTestEnc->fbRecon[i].bufY =
						pTestEnc->vbReconFrameBuf[i].phys_addr;
					pTestEnc->fbRecon[i].bufCb =
						(PhysicalAddress)-1;
					pTestEnc->fbRecon[i].bufCr =
						(PhysicalAddress)-1;
					pTestEnc->fbRecon[i].size = FrameBufSize;
					pTestEnc->fbRecon[i].updateFbInfo = TRUE;
				}
			}
		} else {
			CVI_VC_TRACE("pcviEc->VbBufCfg.VbType[%d]\n",
				     pcviEc->VbBufCfg.VbType);
			CVI_VC_TRACE("bufcnt compare[%d][%d]\n",
				     pTestEnc->regFrameBufCount,
				     pcviEc->VbBufCfg.VbSetFrameBufCnt);
			CVI_VC_TRACE("bufsize compare[%d][%d]\n", FrameBufSize,
				     pcviEc->VbBufCfg.VbSetFrameBufSize);
			for (i = 0; i < pcviEc->VbBufCfg.VbSetFrameBufCnt;
			     i++) {
				pTestEnc->vbReconFrameBuf[i].size =
					pcviEc->VbBufCfg.VbSetFrameBufSize;
				pTestEnc->fbRecon[i].bufY =
					pcviEc->VbBufCfg.vbModeAddr[i];
				pTestEnc->fbRecon[i].bufCb =
					(PhysicalAddress)-1;
				pTestEnc->fbRecon[i].bufCr =
					(PhysicalAddress)-1;
				pTestEnc->fbRecon[i].size =
					pcviEc->VbBufCfg.VbSetFrameBufSize;
				pTestEnc->fbRecon[i].updateFbInfo = TRUE;
			}
		}
	}

	mapType = (TiledMapType)(pTestEnc->encConfig.mapType & 0x0f);

	ret = VPU_EncRegisterFrameBuffer(pTestEnc->handle, pTestEnc->fbRecon,
					 pTestEnc->regFrameBufCount,
					 framebufStride,
					 pTestEnc->framebufHeight, mapType);

	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"VPU_EncRegisterFrameBuffer failed Error code is 0x%x\n",
			ret);
		return TE_ERR_ENC_OPEN;
	}

	for (i = 0; i < pTestEnc->regFrameBufCount; i++) {
		CVI_VC_INFO(
			"Rec, bufY = 0x%llX, bufCb = 0x%llX, bufCr = 0x%llX\n",
			pTestEnc->fbRecon[i].bufY, pTestEnc->fbRecon[i].bufCb,
			pTestEnc->fbRecon[i].bufCr);
	}

	/*****************************
	* ALLOCATE SROUCE BUFFERS	*
	****************************/
	if (pEncOP->bitstreamFormat == STD_AVC) {
		if (pEncOP->linear2TiledEnable == TRUE) {
			pFbAllocInfo->mapType = LINEAR_FRAME_MAP;
			pFbAllocInfo->stride = pEncOP->picWidth;
		} else {
			pFbAllocInfo->mapType = mapType;
			pFbAllocInfo->stride =
				CalcStride(pEncOP->picWidth, pEncOP->picHeight,
					   pEncOP->srcFormat,
					   pEncOP->cbcrInterleave, mapType,
					   FALSE, FALSE);
		}

		pFbAllocInfo->height = VPU_ALIGN16(pTestEnc->srcFrameHeight);
		pFbAllocInfo->num = ENC_SRC_BUF_NUM;

		FrameBufSize = VPU_GetFrameBufSize(
			coreIdx, pFbAllocInfo->stride, pFbAllocInfo->height,
			pFbAllocInfo->mapType, pEncOP->srcFormat,
			pEncOP->cbcrInterleave, NULL);
	} else {
		pFbAllocInfo->mapType = LINEAR_FRAME_MAP;

		pTestEnc->srcFrameStride = CalcStride(
			pTestEnc->srcFrameWidth, pTestEnc->srcFrameHeight,
			(FrameBufferFormat)pEncOP->srcFormat,
			pTestEnc->encOP.cbcrInterleave,
			(TiledMapType)pFbAllocInfo->mapType, FALSE, FALSE);

		FrameBufSize = VPU_GetFrameBufSize(
			coreIdx, pTestEnc->srcFrameStride,
			pTestEnc->srcFrameHeight,
			(TiledMapType)pFbAllocInfo->mapType, pEncOP->srcFormat,
			pTestEnc->encOP.cbcrInterleave, NULL);

		pFbAllocInfo->stride = pTestEnc->srcFrameStride;
		pFbAllocInfo->height = pTestEnc->srcFrameHeight;
		pFbAllocInfo->num = pTestEnc->initialInfo.minSrcFrameCount +
				    EXTRA_SRC_BUFFER_NUM;
		pFbAllocInfo->num = (pFbAllocInfo->num > ENC_SRC_BUF_NUM) ?
						  ENC_SRC_BUF_NUM :
						  pFbAllocInfo->num;
	}

	CVI_VC_TRACE("minSrcFrameCount = %d, num = %d, stride = %d\n",
		     pTestEnc->initialInfo.minSrcFrameCount, pFbAllocInfo->num,
		     pFbAllocInfo->stride);

	pFbAllocInfo->format = (FrameBufferFormat)pEncOP->srcFormat;
	pFbAllocInfo->cbcrInterleave = pTestEnc->encOP.cbcrInterleave;
	pFbAllocInfo->endian = pTestEnc->encOP.sourceEndian;
	pFbAllocInfo->type = FB_TYPE_PPU;
	pFbAllocInfo->nv21 = pTestEnc->encOP.nv21;

	if (pTestEnc->encConfig.cviApiMode == API_MODE_DRIVER ||
	    pEncOP->bitstreamFormat == STD_AVC) {
		for (i = 0; i < pFbAllocInfo->num; i++) {
		#ifdef VC_DRIVER_TEST
			if (pTestEnc->encConfig.cviApiMode == API_MODE_DRIVER) {
				if (i == 0) {
					CVI_VC_MEM(
						"src FB, num = %d, cbcrInterleave = %d, nv21 = %d, FrameBufSize = 0x%X\n",
						pFbAllocInfo->num,
						pFbAllocInfo->cbcrInterleave,
						pFbAllocInfo->nv21,
						FrameBufSize);
				}

				pTestEnc->vbSourceFrameBuf[i].size =
					FrameBufSize;
				memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
				sprintf(ionName, "VENC_%d_SourceFrameBuf",
					pCodecInst->s32ChnNum);
				if (VDI_ALLOCATE_MEMORY(
					    coreIdx,
					    &pTestEnc->vbSourceFrameBuf[i], 0,
					    ionName) < 0) {
					CVI_VC_ERR(
						"fail to allocate frame buffer, pFbAllocInfo->num %d\n",
						pFbAllocInfo->num);
					return TE_ERR_ENC_OPEN;
				}

				vdi_clear_memory(
						coreIdx,
						pTestEnc->vbSourceFrameBuf[i].phys_addr,
						FrameBufSize);

				pTestEnc->fbSrc[i].bufY =
					pTestEnc->vbSourceFrameBuf[i].phys_addr;
				pTestEnc->fbSrc[i].bufCb = (PhysicalAddress)-1;
				pTestEnc->fbSrc[i].bufCr = (PhysicalAddress)-1;
			}
		#endif
			pTestEnc->fbSrc[i].size = FrameBufSize;
			pTestEnc->fbSrc[i].updateFbInfo = TRUE;
		}
	}

	ret = VPU_EncAllocateFrameBuffer(pTestEnc->handle, *pFbAllocInfo,
					 pTestEnc->fbSrc);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR(
			"VPU_EncAllocateFrameBuffer fail to allocate source frame buffer is 0x%x\n",
			ret);
		return TE_ERR_ENC_OPEN;
	}

	for (i = 0; i < pFbAllocInfo->num; i++) {
		CVI_VC_TRACE(
			"fbSrc[%d], bufY = 0x%llX, bufCb = 0x%llX, bufCr = 0x%llX\n",
			i, pTestEnc->fbSrc[i].bufY, pTestEnc->fbSrc[i].bufCb,
			pTestEnc->fbSrc[i].bufCr);
	}

	if (pEncOP->bitstreamFormat == STD_HEVC) {
		if (allocateRoiMapBuf(coreIdx, &pTestEnc->encConfig,
				      &pTestEnc->vbRoi[0], pFbAllocInfo->num,
				      MAX_CTU_NUM) == FALSE) {
			CVI_VC_ERR("allocateRoiMapBuf fail\n");
			return TE_ERR_ENC_OPEN;
		}

		if (allocateCtuQpMapBuf(coreIdx, &pTestEnc->encConfig,
					&pTestEnc->vbCtuQp[0],
					pFbAllocInfo->num,
					MAX_CTU_NUM) == FALSE) {
			CVI_VC_ERR("allocateCtuQpMapBuf fail\n");
			return TE_ERR_ENC_OPEN;
		}
	}

	// allocate User data buffer amount of source buffer num
	if (pTestEnc->encConfig.seiDataEnc.prefixSeiNalEnable) {
		for (i = 0; i < pFbAllocInfo->num; i++) { // the number of roi
			// buffer should be
			// the same as source
			// buffer num.
			pTestEnc->vbPrefixSeiNal[i].size =
				SEI_NAL_DATA_BUF_SIZE;
			memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
			sprintf(ionName, "VENC_%d_PrefixSeiNal",
				pCodecInst->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(coreIdx,
						&pTestEnc->vbPrefixSeiNal[i], 0,
						ionName) < 0) {
				CVI_VC_ERR("fail to allocate ROI buffer\n");
				return TE_ERR_ENC_OPEN;
			}
		}
	}

	if (pTestEnc->encConfig.seiDataEnc.suffixSeiNalEnable) {
		for (i = 0; i < pFbAllocInfo->num; i++) { // the number of roi
			// buffer should be
			// the same as source
			// buffer num.
			pTestEnc->vbSuffixSeiNal[i].size =
				SEI_NAL_DATA_BUF_SIZE;
			memset(ionName, 0, MAX_VPU_ION_BUFFER_NAME);
			sprintf(ionName, "VENC_%d_SuffixSeiNal",
				pCodecInst->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(coreIdx,
						&pTestEnc->vbSuffixSeiNal[i], 0,
						ionName) < 0) {
				CVI_VC_ERR("fail to allocate ROI buffer\n");
				return TE_ERR_ENC_OPEN;
			}
		}
	}

#ifdef TEST_ENCODE_CUSTOM_HEADER
	if (allocateSeiNalDataBuf(coreIdx, &pTestEnc->vbSeiNal[0],
				  pFbAllocInfo->num) == FALSE) {
		CVI_VC_ERR("\n");
		return TE_ERR_ENC_OPEN;
	}
#endif

	return ret;
}

static int cviVEncSetInPixelFormat(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;

	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	EncOpenParam *pEncOP = &pTestEnc->encOP;
	cviInPixelFormat *pInFormat = (cviInPixelFormat *)arg;

	CVI_VC_IF("\n");

	pEncOP->cbcrInterleave = pInFormat->bCbCrInterleave;
	pEncOP->nv21 = pInFormat->bNV21;

	if (pEncOP->bitstreamFormat == STD_AVC) {
		if (pEncOP->cbcrInterleave == true) {
			pEncCfg->mapType = TILED_FRAME_MB_RASTER_MAP;
			pEncCfg->coda9.enableLinear2Tiled = TRUE;
			pEncCfg->coda9.linear2TiledMode = FF_FRAME;
		}
	}

	return ret;
}

static int cviVEncSetFrameParam(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;

	cviFrameParam *pfp = (cviFrameParam *)arg;
	EncParam *pEncParam = &pTestEnc->encParam;

	pEncParam->u32FrameQp = pfp->u32FrameQp;
	pEncParam->u32FrameBits = pfp->u32FrameBits;
	CVI_VC_UBR("u32FrameQp = %d, u32FrameBits = %d\n",
		   pEncParam->u32FrameQp, pEncParam->u32FrameBits);

	return ret;
}

static int cviVEncCalcFrameParam(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	/*
	This API is only used for testing UBR. It will use rcLib to
	do CBR and get Qp & target bits.
	Then these parameters will be sent to encoder by using UBR API.
	*/

	cviFrameParam *pfp = (cviFrameParam *)arg;
	CodecInst *pCodecInst = (CodecInst *)pTestEnc->handle;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	EncOpenParam *pOpenParam = &pEncInfo->openParam;
	int s32RowMaxDqpMinus, s32RowMaxDqpPlus;
	int s32HrdBufLevel, s32HrdBufSize;

	pCodecInst->rcInfo.bTestUbrEn = 1;

	if (pOpenParam->bitstreamFormat == STD_HEVC &&
	    pTestEnc->frameIdx == 0) {
		rcLibSetupRc(pCodecInst);
	}

	ret = rcLibCalcPicQp(pCodecInst,
#ifdef CLIP_PIC_DELTA_QP
			     &s32RowMaxDqpMinus, &s32RowMaxDqpPlus,
#endif
			     (int *)&pfp->u32FrameQp, (int *)&pfp->u32FrameBits,
			     &s32HrdBufLevel, &s32HrdBufSize);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ret = %d\n", ret);
		return ret;
	}
	pTestEnc->encParam.s32HrdBufLevel = s32HrdBufLevel;

	CVI_VC_UBR("fidx = %d, Qp = %d, Bits = %d\n", pTestEnc->frameIdx,
		   pfp->u32FrameQp, pfp->u32FrameBits);

	return ret;
}

static CVI_S32 cviSetSbSetting(cviVencSbSetting *pstSbSetting)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_U32 srcHeightAlign = 0;
	int reg = 0;

	CVI_VC_INFO("\n");

	// Set Register 0x0
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);
	reg &= ~0xFF00FFF0;

	// sb_nb:3, sbc1_mode:2, //sbc1_frm_start:1
	if (pstSbSetting->codec & 0x1)
		reg |= (pstSbSetting->sb_mode << 4);

	if (pstSbSetting->codec & 0x2)
		reg |= (pstSbSetting->sb_mode << 8);

	if (pstSbSetting->codec & 0x4)
		reg |= (pstSbSetting->sb_mode << 12);

	reg |= (pstSbSetting->sb_size << 17);
	reg |= (pstSbSetting->sb_nb << 24);

	if ((pstSbSetting->sb_ybase1 != 0) && (pstSbSetting->sb_uvbase1 != 0))
		reg |= (1<<19); // 1: sbc0/sbc1 sync with interface0 and  sbc2  sync with  interface1

	cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);
	CVI_VC_INFO("VC_REG_BANK_SBM 0x00 = 0x%x\n", reg);

	// pri interface
	if ((pstSbSetting->sb_ybase != 0) && (pstSbSetting->sb_uvbase != 0)) {
		// pri address setting /////////////////////////////////////////////
		// Set Register 0x20
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x20);
		reg = ((pstSbSetting->src_height + 63) >> 6) << 22;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x20, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x20 = 0x%x\n", reg);

		// Set Register 0x24
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x24);
		reg = ((pstSbSetting->y_stride>>4)<<4) + ((pstSbSetting->uv_stride>>4)<<20);

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x24, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x24 = 0x%x\n", reg);

		// Set Register 0x28   src y base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x28);
		reg = 0x80000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x28, reg);
		pstSbSetting->src_ybase = reg;

		CVI_VC_INFO("psbSetting->src_ybase  = 0x%x\n", pstSbSetting->src_ybase);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x28 = 0x%x\n", reg);

		// Set Register 0x2C   src y end addr
		srcHeightAlign = ((pstSbSetting->src_height + 15)>>4)<<4;
		reg = 0x80000000 + srcHeightAlign*pstSbSetting->y_stride;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x2C, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x2C = 0x%x\n", reg);

		// Set Register 0x30   sb y base addr
		reg = pstSbSetting->sb_ybase;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x30, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x30 = 0x%x\n", reg);

		// Set Register 0x34   src c base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x34);
		reg = 0x88000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x34, reg);
		pstSbSetting->src_uvbase = reg;
		CVI_VC_INFO("VC_REG_BANK_SBM 0x34 = 0x%x\n", reg);

		// Set Register 0x38   src c end addr
		reg = 0x88000000 + srcHeightAlign*pstSbSetting->y_stride/2;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x38, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x38 = 0x%x\n", reg);

		// Set Register 0x3C   sb c base addr
		reg = pstSbSetting->sb_uvbase;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x3C, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x3C = 0x%x\n", reg);
	}

	// sec interface
	if ((pstSbSetting->sb_ybase1 != 0) && (pstSbSetting->sb_uvbase1 != 0)) {
		// sec address setting /////////////////////////////////////////////
		// Set Register 0x40
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x40);
		reg = ((pstSbSetting->src_height + 63) >> 6) << 22;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x40, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x40 = 0x%x\n", reg);

		// Set Register 0x44
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x44);
		reg = ((pstSbSetting->y_stride>>4)<<4) + ((pstSbSetting->uv_stride>>4)<<20);

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x44, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x44 = 0x%x\n", reg);

		// Set Register 0x48   src y base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x48);
		reg = 0x90000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x48, reg);
		pstSbSetting->src_ybase1 = reg;

		CVI_VC_INFO("psbSetting->src_ybase1  = 0x%x\n", pstSbSetting->src_ybase1);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x48 = 0x%x\n", reg);

		// Set Register 0x4C   src y end addr
		srcHeightAlign = ((pstSbSetting->src_height + 15)>>4)<<4;
		reg = 0x90000000 + srcHeightAlign*pstSbSetting->y_stride;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x4C, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x4C = 0x%x\n", reg);

		// Set Register 0x50   sb y base addr
		reg = pstSbSetting->sb_ybase1;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x50, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x50 = 0x%x\n", reg);

		// Set Register 0x54   src c base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x54);
		reg = 0x98000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x54, reg);
		pstSbSetting->src_uvbase1 = reg;
		CVI_VC_INFO("VC_REG_BANK_SBM 0x54 = 0x%x\n", reg);

		// Set Register 0x58   src c end addr
		reg = 0x98000000 + srcHeightAlign*pstSbSetting->y_stride/2;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x58, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x58 = 0x%x\n", reg);

		// Set Register 0x5C   sb c base addr
		reg = pstSbSetting->sb_uvbase1;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x5C, reg);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x5C = 0x%x\n", reg);
	}

	return s32Ret;
}

#if 0
static int cviVEncSetSbMode(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	unsigned int srcHeightAlign = 0;
	int reg = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;

	CVI_VC_IF("\n");
	UNREFERENCED_PARAMETER(pTestEnc);

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	// Set Register 0x0
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);

	// sb_nb:3, sbc1_mode:2, //sbc1_frm_start:1
	if (psbSetting->codec & 0x1)
		reg |= (psbSetting->sb_mode << 4);

	if (psbSetting->codec & 0x2)
		reg |= (psbSetting->sb_mode << 8);

	if (psbSetting->codec & 0x4)
		reg |= (psbSetting->sb_mode << 12);

	reg |= (psbSetting->sb_size << 17);
	reg |= (psbSetting->sb_nb << 24);

	if ((psbSetting->sb_ybase1 != 0) && (psbSetting->sb_uvbase1 != 0))
		reg |= (1
			<< 19); // 1: sbc0/sbc1 sync with interface0 and  sbc2  sync with  interface1

	cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);

	CVI_VC_REG("VC_REG_BANK_SBM 0x00 = 0x%x\n", reg);

	if ((psbSetting->sb_ybase != 0) && (psbSetting->sb_uvbase != 0)) {
		// pri address setting /////////////////////////////////////////////
		// Set Register 0x20
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x20);
		reg = ((psbSetting->src_height + 63) >> 6) << 22;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x20, reg);

		CVI_VC_REG("VC_REG_BANK_SBM 0x20 = 0x%x\n", reg);

		// Set Register 0x24
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x24);
		reg = ((psbSetting->y_stride >> 4) << 4) +
		      ((psbSetting->uv_stride >> 4) << 20);

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x24, reg);

		CVI_VC_REG("VC_REG_BANK_SBM 0x24 = 0x%x\n", reg);

		// Set Register 0x28   src y base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x28);
		reg = 0x80000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x28, reg);
		psbSetting->src_ybase = reg;

		CVI_VC_INFO("VC_REG_BANK_SBM 0x28 = 0x%x\n", reg);

		// Set Register 0x2C   src y end addr
		srcHeightAlign = ((psbSetting->src_height + 15) >> 4) << 4;
		reg = 0x80000000 + srcHeightAlign * psbSetting->y_stride;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x2C, reg);
		CVI_VC_REG("VC_REG_BANK_SBM 0x2C = 0x%x\n", reg);

		// Set Register 0x30   sb y base addr
		reg = psbSetting->sb_ybase;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x30, reg);

		CVI_VC_INFO("VC_REG_BANK_SBM 0x30 = 0x%x\n", reg);

		// Set Register 0x34   src c base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x34);
		reg = 0x88000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x34, reg);
		psbSetting->src_uvbase = reg;

		CVI_VC_REG("VC_REG_BANK_SBM 0x34 = 0x%x\n", reg);

		// Set Register 0x38   src c end addr
		reg = 0x88000000 + srcHeightAlign * psbSetting->y_stride / 2;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x38, reg);

		CVI_VC_REG("VC_REG_BANK_SBM 0x38 = 0x%x\n", reg);

		// Set Register 0x3C   sb c base addr
		reg = psbSetting->sb_uvbase;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x3C, reg);

		CVI_VC_REG("VC_REG_BANK_SBM 0x3C = 0x%x\n", reg);
	}

	if ((psbSetting->sb_ybase1 != 0) && (psbSetting->sb_uvbase1 != 0)) {
		// sec address setting /////////////////////////////////////////////
		// Set Register 0x40
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x40);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x40, reg);

		reg = ((psbSetting->src_height + 63) >> 6) << 22;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x40, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x40);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x40, reg);

		// Set Register 0x44
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x44);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x44, reg);

		reg = ((psbSetting->y_stride >> 4) << 4) +
		      ((psbSetting->uv_stride >> 4) << 20);

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x44, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x44);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x44, reg);

		// Set Register 0x48   src y base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x48);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x48, reg);

		reg = 0x90000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x48, reg);
		psbSetting->src_ybase1 = reg;

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x48);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x48, reg);

		// Set Register 0x4C   src y end addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x4C);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x4C, reg);

		srcHeightAlign = ((psbSetting->src_height + 15) >> 4) << 4;
		CVI_VC_REG("[%s][%d] src_height=%d, srcHeightAlign=%d\n",
			   __func__, __LINE__, psbSetting->src_height,
			   srcHeightAlign);
		reg = 0x90000000 + srcHeightAlign * psbSetting->y_stride;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x4C, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x4C);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x4C, reg);

		// Set Register 0x50   sb y base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x50);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x50, reg);

		reg = psbSetting->sb_ybase1;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x50, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x50);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x50, reg);

		// Set Register 0x54   src c base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x54);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x54, reg);

		reg = 0x98000000;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x54, reg);
		psbSetting->src_uvbase1 = reg;

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x54);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x54, reg);

		// Set Register 0x58   src c end addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x58);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x58, reg);

		reg = 0x98000000 + srcHeightAlign * psbSetting->y_stride / 2;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x58, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x58);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x58, reg);

		// Set Register 0x5C   sb c base addr
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x5C);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x5C, reg);

		reg = psbSetting->sb_uvbase1;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x5C, reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x5C);
		CVI_VC_REG("[%s][%d] SBM Reg 0x%x = 0x%x\n", __func__,
			   __LINE__, 0x5C, reg);
	}

	return ret;
}

static int cviVEncStartSbMode(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;

	UNUSED(pTestEnc);
	UNUSED(arg);

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	// Set Register 0x0
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);

	// frm_start:1
	if (psbSetting->codec & 0x1)
		reg |= (1 << 7);

	if (psbSetting->codec & 0x2)
		reg |= (1 << 11);

	if (psbSetting->codec & 0x4)
		reg |= (1 << 15);

	cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);

	CVI_VC_REG("VC_REG_BANK_SBM 0x00 = 0x%x\n", reg);

	return ret;
}

static int cviVEncUpdateSbWptr(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	int sw_mode = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;

	UNUSED(pTestEnc);

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);

	if (coreIdx == 0)
		sw_mode = (reg & 0x30) >> 4;
	else
		sw_mode = (reg & 0x300) >> 8;

	if (sw_mode == 3) { // SW mode
		// Set Register 0x0c
		int wptr = 0;

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x88);
		CVI_VC_INFO("VC_REG_BANK_SBM 0x88 = 0x%x\n", reg);

		wptr = (reg >> 16) & 0x1F;

		CVI_VC_INFO("wptr = 0x%x\n", reg);

		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x0C);
		reg = (reg & 0xFFFFFFE0) | wptr;

		cvi_vc_drv_write_vc_reg(REG_SBM, 0x0C, reg);

		CVI_VC_INFO("VC_REG_BANK_SBM 0x0C = 0x%x\n", reg);
	}

	return ret;
}

static int cviVEncResetSb(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;
	UNUSED(pTestEnc);

	CVI_VC_FLOW("\n");

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	CVI_VC_INFO("Before sw reset sb =================\n");
	CVI_VC_INFO("VC_REG_BANK_SBM 0x80 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x80));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x84 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x84));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x88 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x88));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x90 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x90));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x94 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x94));

	// Reset VC SB ctrl
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);
#if 0
	reg |= 0x8;  // reset all
	cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);
#else
	if (psbSetting->codec & 0x1) { // h265
		reg |= 0x1;
		cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);
	} else if (psbSetting->codec & 0x2) { // h264
		reg |= 0x2;
		cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);
	} else { // jpeg
		reg |= 0x4;
		cvi_vc_drv_write_vc_reg(REG_SBM, 0x00, reg);
	}
#endif

	CVI_VC_INFO("After sw reset sb =================\n");
	CVI_VC_INFO("VC_REG_BANK_SBM 0x80 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x80));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x84 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x84));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x88 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x88));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x90 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x90));
	CVI_VC_INFO("VC_REG_BANK_SBM 0x94 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x94));

	return ret;
}

static int cviVEncSbEnDummyPush(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;
	UNUSED(pTestEnc);

	CVI_VC_FLOW("\n");

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	// Enable sb dummy push
	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x14);
	reg |= 0x1; // reg_pri_push_ow_en bit0
	cvi_vc_drv_write_vc_reg(REG_SBM, 0x14, reg);

	CVI_VC_INFO("VC_REG_BANK_SBM 0x14 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x14));

	return ret;
}

static int cviVEncSbTrigDummyPush(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;
	int pop_cnt_pri = 0;
	int push_cnt_pri = 0;
	UNUSED(pTestEnc);

	CVI_VC_FLOW("\n");

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x94);
	CVI_VC_INFO("VC_REG_BANK_SBM 0x94 = 0x%x\n", reg);

	push_cnt_pri = reg & 0x3F;
	pop_cnt_pri = (reg >> 16) & 0x3F;

	CVI_VC_INFO("push_cnt_pri=%d, pop_cnt_pri=%d\n", push_cnt_pri,
		    pop_cnt_pri);

	if (push_cnt_pri == pop_cnt_pri) {
		reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x14);
		reg |= 0x4; // reg_pri_push_ow bit2
		cvi_vc_drv_write_vc_reg(REG_SBM, 0x14, reg);

		CVI_VC_INFO("VC_REG_BANK_SBM 0x94 = 0x%x\n",
			    cvi_vc_drv_read_vc_reg(REG_SBM, 0x94));
	}

	return ret;
}

static int cviVEncSbDisDummyPush(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;

	UNUSED(pTestEnc);

	CVI_VC_FLOW("\n");

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x14);
	reg &= (~0x1); // reg_pri_push_ow_en bit0
	cvi_vc_drv_write_vc_reg(REG_SBM, 0x14, reg);

	CVI_VC_INFO("VC_REG_BANK_SBM 0x14 = 0x%x\n",
		    cvi_vc_drv_read_vc_reg(REG_SBM, 0x14));

	return ret;
}

static int cviVEncSbGetSkipFrmStatus(stTestEncoder *pTestEnc, void *arg)
{
	int ret = 0;
	unsigned long coreIdx = 0;
	cviVencSbSetting *psbSetting = (cviVencSbSetting *)arg;
	int reg = 0;
	int pop_cnt_pri = 0;
	int push_cnt_pri = 0;
	int target_slice_cnt = 0;
	UNUSED(pTestEnc);

	CVI_VC_FLOW("\n");

	if (psbSetting->codec & 0x1)
		coreIdx = 0;
	else
		coreIdx = 1;

	reg = cvi_vc_drv_read_vc_reg(REG_SBM, 0x94);
	CVI_VC_INFO("VC_REG_BANK_SBM 0x94 = 0x%x\n", reg);

	push_cnt_pri = reg & 0x3F;
	pop_cnt_pri = (reg >> 16) & 0x3F;

	CVI_VC_INFO("push_cnt_pri=%d, pop_cnt_pri=%d\n", push_cnt_pri,
		    pop_cnt_pri);

	if (psbSetting->sb_size == 0)
		target_slice_cnt = (psbSetting->src_height + 63) / 64;
	else
		target_slice_cnt = (psbSetting->src_height + 127) / 128;

	CVI_VC_INFO(
		"psbSetting->src_height=%d, psbSetting->sb_size=%d, target_slice_cnt=%d\n",
		psbSetting->src_height,
		psbSetting->sb_size, target_slice_cnt);

	if (pop_cnt_pri == target_slice_cnt) {
		psbSetting->status = 1;

		CVI_VC_INFO(
			"psbSetting->src_height=%d, psbSetting->sb_size=%d, target_slice_cnt=%d\n",
			psbSetting->src_height,
			psbSetting->sb_size, target_slice_cnt);
	}

	return ret;
}
#endif

static int cviVEncWaitEncodeDone(stTestEncoder *pTestEnc, void *arg)
{

	int ret = RETCODE_SUCCESS;

	ret = wait_for_completion_timeout(&pTestEnc->semEncDoneCmd,
				usecs_to_jiffies(2000 * 1000));
	if (ret == 0) {
		CVI_VC_WARN("get stream timeout!\n");
		return RET_VCODEC_TIMEOUT;
	}

	return 0;
}

static int cviVEncSetSbmEnable(stTestEncoder *pTestEnc, void *arg)
{
	CodecInst *pCodecInst;

	if (!pTestEnc)
		return -1;

	pCodecInst = pTestEnc->handle;
	if (pCodecInst == NULL || pCodecInst->CodecInfo == NULL)
		return -1;

	pCodecInst->CodecInfo->encInfo.bSbmEn = *(bool *)arg;

	return 0;
}

static int cviVEncEncodeUserData(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviUserData *pSrc = (cviUserData *)arg;
	unsigned int len;
	UserDataList *userdataNode;

	CVI_VC_IF("\n");

	if (pSrc == NULL || pSrc->userData == NULL || pSrc->len == 0) {
		CVI_VC_ERR("no user data\n");
		return TE_ERR_ENC_USER_DATA;
	}

	userdataNode = (UserDataList *)osal_malloc(sizeof(UserDataList));
	if (userdataNode == NULL)
		return TE_ERR_ENC_USER_DATA;

	userdataNode->userDataBuf = (Uint8 *)osal_malloc(pEncCfg->userDataBufSize);
	if (userdataNode->userDataBuf == NULL) {
		osal_free(userdataNode);
		return TE_ERR_ENC_USER_DATA;
	}

	len = seiEncode(pEncCfg->stdMode, pSrc->userData,
			  pSrc->len, userdataNode->userDataBuf, pEncCfg->userDataBufSize);

	if (len > pEncCfg->userDataBufSize) {
		CVI_VC_ERR(
			"encoded user data len %d exceeds buffer size %d\n",
			len, pEncCfg->userDataBufSize);
		osal_free(userdataNode);
		osal_free(userdataNode->userDataBuf);
		return TE_ERR_ENC_USER_DATA;
	}

	userdataNode->userDataLen = len;
	list_add_tail(&userdataNode->list, &pEncCfg->userdataList);

	return 0;
}

static int cviVEncSetH264Entropy(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264Entropy *pEntropy = &pEncCfg->cviEc.h264Entropy;
	cviH264Entropy *pSrc = (cviH264Entropy *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 entropy data\n");
		return TE_ERR_ENC_H264_ENTROPY;
	}

	memcpy(pEntropy, pSrc, sizeof(cviH264Entropy));
	return 0;
}

static int cviVEncSetH264Trans(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264Trans *pTrans = &pEncCfg->cviEc.h264Trans;
	cviH264Trans *pSrc = (cviH264Trans *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 trans data\n");
		return TE_ERR_ENC_H264_TRANS;
	}

	memcpy(pTrans, pSrc, sizeof(cviH264Trans));
	return 0;
}

static int cviVEncSetH265Trans(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH265Trans *pTrans = &pEncCfg->cviEc.h265Trans;
	cviH265Trans *pSrc = (cviH265Trans *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h265 trans data\n");
		return TE_ERR_ENC_H265_TRANS;
	}

	memcpy(pTrans, pSrc, sizeof(cviH265Trans));
	return 0;
}

static int cviVEncSetH264Vui(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264Vui *pVui = &pEncCfg->cviEc.h264Vui;
	cviH264Vui *pSrc = (cviH264Vui *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 vui data\n");
		return TE_ERR_ENC_H264_VUI;
	}

	memcpy(pVui, pSrc, sizeof(cviH264Vui));
	return 0;
}

static int cviVEncSetH265Vui(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH265Vui *pVui = &pEncCfg->cviEc.h265Vui;
	cviH265Vui *pSrc = (cviH265Vui *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h265 vui data\n");
		return TE_ERR_ENC_H265_VUI;
	}

	memcpy(pVui, pSrc, sizeof(cviH265Vui));
	return 0;
}

static int cviVEncSetH264Split(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264Split *pSplit = &pEncCfg->cviEc.h264Split;
	cviH264Split *pSrc = (cviH264Split *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 split data\n");
		return TE_ERR_ENC_H264_SPLIT;
	}

	memcpy(pSplit, pSrc, sizeof(cviH264Split));
	return 0;
}

static int cviVEncSetH265Split(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH265Split *pSplit = &pEncCfg->cviEc.h265Split;
	cviH265Split *pSrc = (cviH265Split *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h265 split data\n");
		return TE_ERR_ENC_H265_SPLIT;
	}

	memcpy(pSplit, pSrc, sizeof(cviH265Split));
	return 0;
}

static int cviVEncSetH264IntraPred(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264IntraPred *pIntraPred = &pEncCfg->cviEc.h264IntraPred;
	cviH264IntraPred *pSrc = (cviH264IntraPred *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 intra pred data\n");
		return TE_ERR_ENC_H264_INTRA_PRED;
	}

	memcpy(pIntraPred, pSrc, sizeof(cviH264IntraPred));
	return 0;
}

static int cviVEncSetRc(stTestEncoder *pTestEnc, void *arg)
{
	cviRcParam *prcp = (cviRcParam *)arg;
	TestEncConfig *pEncConfig = &pTestEnc->encConfig;
	cviEncCfg *pCviEc = &pEncConfig->cviEc;
	int ret = 0;

	CVI_VC_IF("\n");

	pCviEc->u32RowQpDelta = prcp->u32RowQpDelta;
	pCviEc->firstFrmstartQp = prcp->firstFrmstartQp;
	pCviEc->initialDelay = prcp->s32InitialDelay;
	pCviEc->u32ThrdLv = prcp->u32ThrdLv;
	CVI_VC_CFG("firstFrmstartQp = %d, initialDelay = %d, u32ThrdLv = %d\n",
		   pCviEc->firstFrmstartQp, pCviEc->initialDelay,
		   pCviEc->u32ThrdLv);

	pEncConfig->changePos = prcp->s32ChangePos;
	pCviEc->bBgEnhanceEn = prcp->bBgEnhanceEn;
	pCviEc->s32BgDeltaQp = prcp->s32BgDeltaQp;
	CVI_VC_CFG("s32ChangePos = %d, bBgEnhanceEn = %d, s32BgDeltaQp = %d\n",
		   prcp->s32ChangePos, pCviEc->bBgEnhanceEn,
		   pCviEc->s32BgDeltaQp);

	pCviEc->u32MaxIprop = prcp->u32MaxIprop;
	pCviEc->u32MinIprop = prcp->u32MinIprop;
	pCviEc->s32MaxReEncodeTimes = prcp->s32MaxReEncodeTimes;
	CVI_VC_CFG(
		"u32MaxIprop = %d, u32MinIprop = %d, s32MaxReEncodeTimes = %d\n",
		pCviEc->u32MaxIprop, pCviEc->u32MinIprop,
		pCviEc->s32MaxReEncodeTimes);

	pCviEc->u32MaxQp = prcp->u32MaxQp;
	pCviEc->u32MaxIQp = prcp->u32MaxIQp;
	pCviEc->u32MinQp = prcp->u32MinQp;
	pCviEc->u32MinIQp = prcp->u32MinIQp;
	CVI_VC_CFG(
		"u32MaxQp = %d, u32MaxIQp = %d, u32MinQp = %d, u32MinIQp = %d\n",
		pCviEc->u32MaxQp, pCviEc->u32MaxIQp, pCviEc->u32MinQp,
		pCviEc->u32MinIQp);

	pCviEc->s32MinStillPercent = prcp->s32MinStillPercent;
	pCviEc->u32MaxStillQP = prcp->u32MaxStillQP;
	pCviEc->u32MotionSensitivity = prcp->u32MotionSensitivity;
	CVI_VC_CFG("StillPercent = %d, StillQP = %d, MotionSensitivity = %d\n",
		   prcp->s32MinStillPercent, prcp->u32MaxStillQP,
		   prcp->u32MotionSensitivity);

	pCviEc->s32AvbrFrmLostOpen = prcp->s32AvbrFrmLostOpen;
	pCviEc->s32AvbrFrmGap = prcp->s32AvbrFrmGap;
	pCviEc->s32AvbrPureStillThr = prcp->s32AvbrPureStillThr;
	CVI_VC_CFG("FrmLostOpen = %d, FrmGap = %d, PureStillThr = %d\n",
		   prcp->s32AvbrFrmLostOpen, prcp->s32AvbrFrmGap,
		   prcp->s32AvbrPureStillThr);

	return ret;
}

static int cviVEncSetRef(stTestEncoder *pTestEnc, void *arg)
{
	unsigned int *tempLayer = (unsigned int *)arg;
	int ret = 0;

	CVI_VC_IF("\n");
	pTestEnc->encConfig.tempLayer = *tempLayer;

	return ret;
}

static int cviVEncSetPred(stTestEncoder *pTestEnc, void *arg)
{
	cviEncCfg *pCviEc = &pTestEnc->encConfig.cviEc;
	cviPred *pPred = (cviPred *)arg;

	CVI_VC_IF("\n");

	pCviEc->u32IntraCost = pPred->u32IntraCost;

	return 0;
}

static int cviVEncSetRoi(stTestEncoder *pTestEnc, void *arg)
{
	cviRoiParam *proi = (cviRoiParam *)arg;
	int ret = 0;
	int index = proi->roi_index;
	cviEncRoiCfg *pcviRoi = NULL;

	if (index >= 8 || index < 0) {
		CVI_VC_ERR("Set ROI index = %d\n", index);
		return -1;
	}

	CVI_VC_IF("\n");
	if (pTestEnc->encOP.bitstreamFormat == STD_HEVC)
		pTestEnc->encConfig.ctu_roiLevel_enable = 1;

	pcviRoi = &pTestEnc->encConfig.cviEc.cviRoi[index];
	pTestEnc->encConfig.cviEc.roi_request = TRUE;
	pTestEnc->encConfig.roi_enable = TRUE;
	pcviRoi->roi_enable = proi->roi_enable;
	pcviRoi->roi_qp_mode = proi->roi_qp_mode;
	pcviRoi->roi_qp = proi->roi_qp;
	pcviRoi->roi_rect_x = proi->roi_rect_x;
	pcviRoi->roi_rect_y = proi->roi_rect_y;
	pcviRoi->roi_rect_width = proi->roi_rect_width;
	pcviRoi->roi_rect_height = proi->roi_rect_height;
	cviUpdateOneRoi(&pTestEnc->encParam, pcviRoi, index);

	CVI_VC_TRACE("cviVEncSetRoi [%d]enable %d, qp mode %d, qp %d\n", index,
		     proi->roi_enable, proi->roi_qp_mode, proi->roi_qp);
	CVI_VC_TRACE("cviVEncSetRoi [%d]X:%d,Y:%d,W:%d,H:%d\n", index,
		     proi->roi_rect_x, proi->roi_rect_y, proi->roi_rect_width,
		     proi->roi_rect_height);
	return ret;
}

static int cviVEncGetRoi(stTestEncoder *pTestEnc, void *arg)
{
	cviRoiParam *proi = (cviRoiParam *)arg;

	int ret = 0;
	int index = proi->roi_index;
	const cviEncRoiCfg *pcviRoi = NULL;

	if (index >= 8 || index < 0) {
		CVI_VC_ERR("Get ROI index = %d\n", index);
		return -1;
	}

	CVI_VC_IF("\n");

	pcviRoi = &pTestEnc->encConfig.cviEc.cviRoi[index];

	proi->roi_enable = pcviRoi->roi_enable;
	proi->roi_qp_mode = pcviRoi->roi_qp_mode;
	proi->roi_qp = pcviRoi->roi_qp;
	proi->roi_rect_x = pcviRoi->roi_rect_x;
	proi->roi_rect_y = pcviRoi->roi_rect_y;
	proi->roi_rect_width = pcviRoi->roi_rect_width;
	proi->roi_rect_height = pcviRoi->roi_rect_height;

	return ret;
}

static int cviVEncStart(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncConfig = vmalloc(sizeof(TestEncConfig));
	int ret = 0;
	int idx;
	stStreamPack *psp;
	stPack *pPack;
	int core_idx =
		pTestEnc->encConfig.stdMode == STD_AVC ? CORE_H264 : CORE_H265;

	UNREFERENCED_PARAMETER(arg);

	CVI_VC_IF("\n");

	EnterVcodecLock(core_idx);
	memcpy(pEncConfig, &pTestEnc->encConfig, sizeof(TestEncConfig));

	if (cviInitEncoder(pTestEnc, pEncConfig) == NULL) {
		ret = -1;
	}

	psp = &pTestEnc->streamPack;
	MUTEX_LOCK(&psp->packMutex);
	if (psp->totalPacks) {
		for (idx = 0; idx < psp->totalPacks; idx++) {
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
	}
	MUTEX_UNLOCK(&psp->packMutex);
	pTestEnc->encParam.idr_request = TRUE;

	LeaveVcodecLock(core_idx);

	vfree(pEncConfig);
	return ret;
}

#if 0
static int cviVEncOpGetFd(stTestEncoder *pTestEnc, void *arg)
{
	int *fd = (int *)arg;
	int coreIdx = pTestEnc->coreIdx;
	int ret = 0;

	CVI_VC_IF("\n");

	*fd = cviVPU_GetFd(coreIdx);

	if (*fd < 0) {
		CVI_VC_ERR("get-fd failed\n");
		*fd = -1;
		ret = -1;
	}

	CVI_VC_TRACE("fd = %d\n", *fd);

	return ret;
}
#endif

static int cviVEncOpSetChnAttr(stTestEncoder *pTestEnc, void *arg)
{
	cviVidChnAttr *pChnAttr = (cviVidChnAttr *)arg;
	int ret = 0;
	unsigned int u32Sec = 0;
	unsigned int u32Frm = 0;

	CVI_VC_IF("\n");

	pTestEnc->encOP.bitRate = pChnAttr->u32BitRate;

	u32Sec = pChnAttr->fr32DstFrameRate >> 16;
	u32Frm = pChnAttr->fr32DstFrameRate & 0xFFFF;

	if (u32Sec == 0) {
		pTestEnc->encOP.frameRateInfo = u32Frm;
	} else {
		pTestEnc->encOP.frameRateInfo = ((u32Sec - 1) << 16) + u32Frm;
	}

	CVI_VC_IF("\n");

	return ret;
}

static int cviVEncSetFrameLost(stTestEncoder *pTestEnc, void *arg)
{
	cviFrameLost *pfrLost = (cviFrameLost *)arg;
	TestEncConfig *pEncConfig = &pTestEnc->encConfig;
	int ret = 0;

	CVI_VC_IF("\n");

	pEncConfig->frmLostOpen = pfrLost->bFrameLostOpen;
	pEncConfig->cviEc.frmLostBpsThr = pfrLost->u32FrmLostBpsThr;
	pEncConfig->cviEc.encFrmGaps = pfrLost->u32EncFrmGaps;

	CVI_VC_CFG("frmLostOpen = %d\n", pEncConfig->frmLostOpen);
	CVI_VC_CFG("frameSkipBufThr = %d\n", pEncConfig->cviEc.frmLostBpsThr);
	CVI_VC_CFG("encFrmGaps = %d\n", pEncConfig->cviEc.encFrmGaps);

	return ret;
}

#ifdef CLI_DEBUG_SUPPORT
extern void showCodecInstPoolInfo(CodecInst *pCodecInst);
static int cviVEncShowChnInfo(stTestEncoder *pTestEnc, void *arg)
{
	if (pTestEnc == NULL) {
		tcli_print("error params.\n");
		return -1;
	}

	UNUSED(arg);
	EncOpenParam *pEncOP = &pTestEnc->encOP;

	tcli_print("bitstreamBufferSize:%d\n", pEncOP->bitstreamBufferSize);
	tcli_print("bitstreamFormat:%d\n", pEncOP->bitstreamFormat);
	tcli_print("picWidth:%d\n", pEncOP->picWidth);
	tcli_print("picHeight:%d\n", pEncOP->picHeight);
	FrameBufferAllocInfo *pfbAllocInfo = &pTestEnc->fbAllocInfo;

	if (pfbAllocInfo) {
		tcli_print("pfbAllocInfo_stride:%d\n", pfbAllocInfo->stride);
		tcli_print("pfbAllocInfo_height:%d\n", pfbAllocInfo->height);
		tcli_print("pfbAllocInfo_size:%d\n", pfbAllocInfo->size);
		tcli_print("pfbAllocInfo_numt:%d\n", pfbAllocInfo->num);
	}

	TestEncConfig *pEncConfig = &pTestEnc->encConfig;

	tcli_print("encConfig info:\n");
	tcli_print("stdMode:%d\n", pEncConfig->stdMode);
	tcli_print("picWidth:%d\n", pEncConfig->picWidth);
	tcli_print("picHeight:%d\n", pEncConfig->picHeight);
	tcli_print("kbps:%d\n", pEncConfig->kbps);
	tcli_print("rcMode:%d\n", pEncConfig->rcMode);
	tcli_print("changePos:%d\n", pEncConfig->changePos);
	tcli_print("frmLostOpen:%d\n", pEncConfig->frmLostOpen);
	tcli_print("rotAngle:%d\n", pEncConfig->rotAngle);
	tcli_print("mirDir:%d\n", pEncConfig->mirDir);
	tcli_print("useRot:%d\n", pEncConfig->useRot);
	tcli_print("qpReport:%d\n", pEncConfig->qpReport);
	tcli_print("ringBufferEnable:%d\n", pEncConfig->ringBufferEnable);
	tcli_print("rcIntraQp:%d\n", pEncConfig->rcIntraQp);
	tcli_print("instNum:%d\n", pEncConfig->instNum);
	tcli_print("coreIdx:%d\n", pEncConfig->coreIdx);

	tcli_print("mapType:%d\n", pEncConfig->mapType);
	tcli_print("lineBufIntEn:%d\n", pEncConfig->lineBufIntEn);
	tcli_print("bEsBufQueueEn:%d\n", pEncConfig->bEsBufQueueEn);
	tcli_print("en_container:%d\n", pEncConfig->en_container);
	tcli_print("container_frame_rate:%d\n",
		   pEncConfig->container_frame_rate);
	tcli_print("picQpY:%d\n", pEncConfig->picQpY);
	tcli_print("cbcrInterleave:%d\n", pEncConfig->cbcrInterleave);
	tcli_print("nv21:%d\n", pEncConfig->nv21);
	tcli_print("needSourceConvert:%d\n", pEncConfig->needSourceConvert);
	tcli_print("packedFormat:%d\n", pEncConfig->packedFormat);
	tcli_print("srcFormat:%d\n", pEncConfig->srcFormat);
	tcli_print("srcFormat3p4b:%d\n", pEncConfig->srcFormat3p4b);
	tcli_print("decodingRefreshType:%d\n", pEncConfig->decodingRefreshType);
	tcli_print("gopSize:%d\n", pEncConfig->gopSize);

	tcli_print("tempLayer:%d\n", pEncConfig->tempLayer);
	tcli_print("useDeriveLambdaWeight:%d\n",
		   pEncConfig->useDeriveLambdaWeight);
	tcli_print("dynamicMergeEnable:%d\n", pEncConfig->dynamicMergeEnable);
	tcli_print("independSliceMode:%d\n", pEncConfig->independSliceMode);
	tcli_print("independSliceModeArg:%d\n",
		   pEncConfig->independSliceModeArg);
	tcli_print("RcEnable:%d\n", pEncConfig->RcEnable);
	tcli_print("bitdepth:%d\n", pEncConfig->bitdepth);
	tcli_print("secondary_axi:%d\n", pEncConfig->secondary_axi);
	tcli_print("stream_endian:%d\n", pEncConfig->stream_endian);
	tcli_print("frame_endian:%d\n", pEncConfig->frame_endian);
	tcli_print("source_endian:%d\n", pEncConfig->source_endian);
	tcli_print("compare_type:%d\n", pEncConfig->compare_type);
	tcli_print("yuv_mode:%d\n", pEncConfig->yuv_mode);
	tcli_print("loopCount:%d\n", pEncConfig->loopCount);

	tcli_print("roi_enable:%d\n", pEncConfig->roi_enable);
	tcli_print("roi_delta_qp:%d\n", pEncConfig->roi_delta_qp);
	tcli_print("ctu_qpMap_enable:%d\n", pEncConfig->ctu_qpMap_enable);
	tcli_print("encAUD:%d\n", pEncConfig->encAUD);
	tcli_print("encEOS:%d\n", pEncConfig->encEOS);
	tcli_print("encEOB:%d\n", pEncConfig->encEOB);
	tcli_print("actRegNum:%d\n", pEncConfig->actRegNum);
	tcli_print("useAsLongtermPeriod:%d\n", pEncConfig->useAsLongtermPeriod);
	tcli_print("refLongtermPeriod:%d\n", pEncConfig->refLongtermPeriod);
	tcli_print("testEnvOptions:%d\n", pEncConfig->testEnvOptions);
	tcli_print("cviApiMode:%d\n", pEncConfig->cviApiMode);
	tcli_print("sizeInWord:%d\n", pEncConfig->sizeInWord);
	tcli_print("userDataBufSize:%d\n", pEncConfig->userDataBufSize);
	tcli_print("bIsoSendFrmEn:%d\n", pEncConfig->bIsoSendFrmEn);

	tcli_print("cviEncCfg cviEc info:\n");
	cviEncCfg *pcviEc = &pEncConfig->cviEc;

	tcli_print("	rcMode:%d\n", pcviEc->rcMode);
	tcli_print("	s32IPQpDelta:%d\n", pcviEc->s32IPQpDelta);
	tcli_print("	iqp:%d\n", pcviEc->iqp);
	tcli_print("	pqp:%d\n", pcviEc->pqp);
	tcli_print("	gop:%d\n", pcviEc->gop);
	tcli_print("	bitrate:%d\n", pcviEc->bitrate);
	tcli_print("	firstFrmstartQp:%d\n", pcviEc->firstFrmstartQp);
	tcli_print("	framerate:%d\n", pcviEc->framerate);
	tcli_print("	u32MaxIprop:%d\n", pcviEc->u32MaxIprop);
	tcli_print("	u32MinIprop:%d\n", pcviEc->u32MinIprop);
	tcli_print("	u32MaxQp:%d\n", pcviEc->u32MaxQp);
	tcli_print("	u32MinQp:%d\n", pcviEc->u32MinQp);
	tcli_print("	u32MaxIQp:%d\n", pcviEc->u32MaxIQp);
	tcli_print("	u32MinIQp:%d\n", pcviEc->u32MinIQp);
	tcli_print("	maxbitrate:%d\n", pcviEc->maxbitrate);
	tcli_print("	initialDelay:%d\n", pcviEc->initialDelay);
	tcli_print("	statTime:%d\n", pcviEc->statTime);

	tcli_print("	bitstreamBufferSize:%d\n", pcviEc->bitstreamBufferSize);
	tcli_print("	singleLumaBuf:%d\n", pcviEc->singleLumaBuf);
	tcli_print("	bSingleEsBuf:%d\n", pcviEc->bSingleEsBuf);
	tcli_print("	roi_request:%d\n", pcviEc->roi_request);
	tcli_print("	roi_enable	mode	roi_qp		x");
	tcli_print("		y		width		height\n");
	for (int j = 0; j < 8; j++) {
		tcli_print(
			"	:%d		%d		%d		%d",
			pcviEc->cviRoi[j].roi_enable,
			pcviEc->cviRoi[j].roi_qp_mode, pcviEc->cviRoi[j].roi_qp,
			pcviEc->cviRoi[j].roi_rect_x);
		tcli_print("		%d		%d		%d\n",
			   pcviEc->cviRoi[j].roi_rect_y,
			   pcviEc->cviRoi[j].roi_rect_width,
			   pcviEc->cviRoi[j].roi_rect_height);
	}
	tcli_print("	virtualIPeriod:%d\n", pcviEc->virtualIPeriod);
	tcli_print("	frmLostBpsThr:%d\n", pcviEc->frmLostBpsThr);
	tcli_print("	encFrmGaps:%d\n", pcviEc->encFrmGaps);
	tcli_print("	s32ChangePos:%d\n", pcviEc->s32ChangePos);

	tcli_print("interruptTimeout:%d\n", pTestEnc->interruptTimeout);
	tcli_print("bsBufferCount:%d\n", pTestEnc->bsBufferCount);
	tcli_print("srcFrameIdx:%d\n", pTestEnc->srcFrameIdx);
	tcli_print("frameIdx:%d\n", pTestEnc->frameIdx);
	tcli_print("coreIdx:%d\n", pTestEnc->coreIdx);
	CodecInst *pCodecInst = pTestEnc->handle;

	showCodecInstPoolInfo(pCodecInst);

	return 0;
}
#else
static int cviVEncShowChnInfo(stTestEncoder *pTestEnc, void *arg)
{
	CodecInst *pCodecInst = pTestEnc->handle;
	EncInfo *pEncInfo;
	struct seq_file *m = (struct seq_file *)arg;

	if (pCodecInst && pCodecInst->CodecInfo) {
		pEncInfo = &pCodecInst->CodecInfo->encInfo;
		seq_printf(m, "chn num:%d yuvCnt:%llu dropCnt:%u\n",
			pCodecInst->s32ChnNum, pCodecInst->yuvCnt, pTestEnc->streamPack.dropCnt);
		seq_printf(m, "mapType:%d bSbmEn:%d\n", pEncInfo->mapType, pEncInfo->bSbmEn);
	} else {
		seq_printf(m, "pCodecInst %p or CodecInfo is NULL\n", pCodecInst);
	}

	return 0;
}

#endif

static int cviVEncSetUserRcInfo(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncConfig = &pTestEnc->encConfig;
	cviEncCfg *pCviEc = &pEncConfig->cviEc;
	cviUserRcInfo *puri = (cviUserRcInfo *)arg;
	int ret = 0;

	CVI_VC_IF("\n");

	pCviEc->bQpMapValid = puri->bQpMapValid;
	pCviEc->bRoiBinValid = puri->bRoiBinValid;
	pCviEc->roideltaqp = puri->roideltaqp;
	pCviEc->pu8QpMap = (Uint8 *)((uintptr_t)puri->u64QpMapPhyAddr);

	CVI_VC_CFG("bQpMapValid = %d\n", pCviEc->bQpMapValid);
	CVI_VC_CFG("pu8QpMap = %p\n", pCviEc->pu8QpMap);

	return ret;
}

static int cviVEncSetSuperFrame(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncConfig = &pTestEnc->encConfig;
	cviEncCfg *pCviEc = &pEncConfig->cviEc;
	cviSuperFrame *pSuper = (cviSuperFrame *)arg;
	int ret = 0;

	CVI_VC_IF("\n");

	pCviEc->enSuperFrmMode = pSuper->enSuperFrmMode;
	pCviEc->u32SuperIFrmBitsThr = pSuper->u32SuperIFrmBitsThr;
	pCviEc->u32SuperPFrmBitsThr = pSuper->u32SuperPFrmBitsThr;

	CVI_VC_CFG("enSuperFrmMode = %d, IFrmBitsThr = %d, PFrmBitsThr = %d\n",
		   pCviEc->enSuperFrmMode, pCviEc->u32SuperIFrmBitsThr,
		   pCviEc->u32SuperPFrmBitsThr);

	return ret;
}

static int cviVEncSetH264Dblk(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH264Dblk *pDblk = &pEncCfg->cviEc.h264Dblk;
	cviH264Dblk *pSrc = (cviH264Dblk *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h264 dblk data\n");
		return TE_ERR_ENC_H264_DBLK;
	}

	memcpy(pDblk, pSrc, sizeof(cviH264Dblk));
	return 0;
}

static int cviVEncSetH265Dblk(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviH265Dblk *pDblk = &pEncCfg->cviEc.h265Dblk;
	cviH265Dblk *pSrc = (cviH265Dblk *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no h265 dblk data\n");
		return TE_ERR_ENC_H265_DBLK;
	}

	memcpy(pDblk, pSrc, sizeof(cviH265Dblk));
	return 0;
}

int cviVEncDropFrame(stTestEncoder *pTestEnc, void *arg)
{
	pTestEnc->bDrop = TRUE;

	return 0;
}

int cviVEncSbmSetting(stTestEncoder *pTestEnc, void *arg)
{
	cviVencSbSetting *SbSetting = (cviVencSbSetting *)arg;

	memcpy(&pTestEnc->SbSetting, SbSetting, sizeof(cviVencSbSetting));

	//Setting Sbm regieter before enable Sbm Irq
	cviSetSbSetting(SbSetting);

	return 0;
}

int cviVEncSvcEnable(stTestEncoder *pTestEnc, void *arg)
{
	bool *svc_enable = (bool *)arg;
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;

	pTestEnc->encOP.svc_enable = *svc_enable;
	pEncCfg->cviEc.svcEnable = *svc_enable;
	return 0;
}

int cviVEncSetSvcParam(stTestEncoder *pTestEnc, void *arg)
{
	TestEncConfig *pEncCfg = &pTestEnc->encConfig;
	cviSvcParam *svc_param = &pEncCfg->cviEc.svcParam;
	cviSvcParam *pSrc = (cviSvcParam *)arg;

	CVI_VC_IF("\n");

	if (pSrc == NULL) {
		CVI_VC_ERR("no svc param data\n");
		return TE_ERR_ENC_SVC_PARAM;
	}

	memcpy(svc_param, pSrc, sizeof(cviSvcParam));
	pTestEnc->encOP.fg_protect_en = svc_param->fg_protect_en;
	pTestEnc->encOP.fg_dealt_qp =  svc_param->fg_dealt_qp;
	pTestEnc->encOP.complex_scene_detect_en =  svc_param->complex_scene_detect_en;
	pTestEnc->encOP.complex_scene_low_th =  svc_param->complex_scene_low_th;
	pTestEnc->encOP.complex_scene_hight_th =  svc_param->complex_scene_hight_th;
	pTestEnc->encOP.middle_min_percent =  svc_param->middle_min_percent;
	pTestEnc->encOP.complex_min_percent =  svc_param->complex_min_percent;
	pTestEnc->encOP.smart_ai_en =  svc_param->smart_ai_en;
	return 0;
}

int cviVEncShowRcRealInfo(stTestEncoder *pTestEnc, void *arg)
{
	stRcInfo *pRcInfo = &pTestEnc->handle->rcInfo;
	struct seq_file *m = (struct seq_file *)arg;

	seq_printf(
			m,
			"realBitrate: %d\t minPercent: %d\t MotionLv: %d\t senecCplx: %d\n",
			pRcInfo->targetBitrate, pRcInfo->lastMinPercent,
			pRcInfo->lastPeriodMotionLv, pRcInfo->periodDciLv);

	seq_printf(
			m,
			"rcMinIqp: %d\t rcMaxIqp: %d\t rcMinqp: %d\t rcMaxqp: %d\t cur_qp: %d\n",
			pRcInfo->rcKerInfo.minIQp, pRcInfo->rcKerInfo.maxIQp,
			pRcInfo->rcKerInfo.minQp, pRcInfo->rcKerInfo.maxQp,
			pRcInfo->rcPicOut.qp);
	return 0;
}

typedef struct _CVI_VENC_IOCTL_OP_ {
	int opNum;
	int (*ioctlFunc)(stTestEncoder *pTestEnc, void *arg);
} CVI_VENC_IOCTL_OP;

CVI_VENC_IOCTL_OP cviIoctlOp[] = {
	{ CVI_H26X_OP_NONE, NULL },
	{ CVI_H26X_OP_SET_RC_PARAM, cviVEncSetRc },
	{ CVI_H26X_OP_START, cviVEncStart },
#if 0
	{ CVI_H26X_OP_GET_FD, cviVEncOpGetFd },
#endif
	{ CVI_H26X_OP_SET_REQUEST_IDR, cviVEncSetRequestIDR },
	{ CVI_H26X_OP_SET_CHN_ATTR, cviVEncOpSetChnAttr },
	{ CVI_H26X_OP_SET_REF_PARAM, cviVEncSetRef },
	{ CVI_H26X_OP_SET_ROI_PARAM, cviVEncSetRoi },
	{ CVI_H26X_OP_GET_ROI_PARAM, cviVEncGetRoi },
	{ CVI_H26X_OP_SET_FRAME_LOST_STRATEGY, cviVEncSetFrameLost },
	{ CVI_H26X_OP_GET_VB_INFO, cviVEncGetVbInfo },
	{ CVI_H26X_OP_SET_VB_BUFFER, cviVEncSetVbBuffer },
	{ CVI_H26X_OP_SET_USER_DATA, cviVEncEncodeUserData },
	{ CVI_H26X_OP_SET_PREDICT, cviVEncSetPred },
	{ CVI_H26X_OP_SET_H264_ENTROPY, cviVEncSetH264Entropy },
	{ CVI_H26X_OP_SET_H264_TRANS, cviVEncSetH264Trans },
	{ CVI_H26X_OP_SET_H265_TRANS, cviVEncSetH265Trans },
	{ CVI_H26X_OP_REG_VB_BUFFER, cviVEncRegReconBuf },
	{ CVI_H26X_OP_SET_IN_PIXEL_FORMAT, cviVEncSetInPixelFormat },
	{ CVI_H26X_OP_GET_CHN_INFO, cviVEncShowChnInfo },
	{ CVI_H26X_OP_SET_USER_RC_INFO, cviVEncSetUserRcInfo },
	{ CVI_H26X_OP_SET_SUPER_FRAME, cviVEncSetSuperFrame },
	{ CVI_H26X_OP_SET_H264_VUI, cviVEncSetH264Vui },
	{ CVI_H26X_OP_SET_H265_VUI, cviVEncSetH265Vui },
	{ CVI_H26X_OP_SET_FRAME_PARAM, cviVEncSetFrameParam },
	{ CVI_H26X_OP_CALC_FRAME_PARAM, cviVEncCalcFrameParam },
#if 0
	{ CVI_H26X_OP_SET_SB_MODE, cviVEncSetSbMode },
	{ CVI_H26X_OP_START_SB_MODE, cviVEncStartSbMode },
	{ CVI_H26X_OP_UPDATE_SB_WPTR, cviVEncUpdateSbWptr },
	{ CVI_H26X_OP_RESET_SB, cviVEncResetSb },
	{ CVI_H26X_OP_SB_EN_DUMMY_PUSH, cviVEncSbEnDummyPush },
	{ CVI_H26X_OP_SB_TRIG_DUMMY_PUSH, cviVEncSbTrigDummyPush },
	{ CVI_H26X_OP_SB_DIS_DUMMY_PUSH, cviVEncSbDisDummyPush },
	{ CVI_H26X_OP_SB_GET_SKIP_FRM_STATUS, cviVEncSbGetSkipFrmStatus },
#endif
	{ CVI_H26X_OP_SET_SBM_ENABLE, cviVEncSetSbmEnable },
	{ CVI_H26X_OP_WAIT_FRAME_DONE, cviVEncWaitEncodeDone },
	{ CVI_H26X_OP_SET_H264_SPLIT, cviVEncSetH264Split },
	{ CVI_H26X_OP_SET_H265_SPLIT, cviVEncSetH265Split },
	{ CVI_H26X_OP_SET_H264_DBLK, cviVEncSetH264Dblk },
	{ CVI_H26X_OP_SET_H265_DBLK, cviVEncSetH265Dblk },
	{ CVI_H26X_OP_SET_H264_INTRA_PRED, cviVEncSetH264IntraPred },
	{ CVI_H26X_OP_DROP_FRAME, cviVEncDropFrame},
	{ CVI_H26X_OP_SET_SBM_SETTING, cviVEncSbmSetting},
	{ CVI_H26X_OP_SET_ENABLE_SVC, cviVEncSvcEnable},
	{ CVI_H26X_OP_SET_SVC_PARAM, cviVEncSetSvcParam},
	{ CVI_H26X_OP_GET_RC_REAL_INFO, cviVEncShowRcRealInfo},
};

int cviVEncIoctl(void *handle, int op, void *arg)
{
	stTestEncoder *pTestEnc = (stTestEncoder *)handle;
	int ret = 0;
	int currOp;

	CVI_VC_IF("\n");

	if (op <= 0 || op >= CVI_H26X_OP_MAX) {
		CVI_VC_ERR("op = %d\n", op);
		return -1;
	}

	currOp = (cviIoctlOp[op].opNum & CVI_H26X_OP_MASK) >> CVI_H26X_OP_SHIFT;
	if (op != currOp) {
		CVI_VC_ERR("op = %d\n", op);
		return -1;
	}

	ret = cviIoctlOp[op].ioctlFunc(pTestEnc, arg);

	return ret;
}

static int initMcuEnv(TestEncConfig *pEncConfig)
{
	Uint32 productId;
	int ret = 0;
#ifndef FIRMWARE_H
	char *fwPath = NULL;
#endif

	ret = cviSetCoreIdx(&pEncConfig->coreIdx, pEncConfig->stdMode);
	if (ret) {
		ret = TE_ERR_ENC_INIT;
		CVI_VC_ERR("cviSetCoreIdx, ret = %d\n", ret);
		return ret;
	}

	EnterVcodecLock(pEncConfig->coreIdx);
	productId = VPU_GetProductId(pEncConfig->coreIdx);
	LeaveVcodecLock(pEncConfig->coreIdx);

	if (checkEncConfig(pEncConfig, productId)) {
		CVI_VC_ERR("checkEncConfig\n");
		return -1;
	}

#ifdef FIRMWARE_H
	if (productId != PRODUCT_ID_420L && productId != PRODUCT_ID_980) {
		CVI_VC_ERR("productId = %d\n", productId);
		return -1;
	}

	if (pEncConfig->sizeInWord == 0 && pEncConfig->pusBitCode == NULL) {
		if (LoadFirmwareH(productId, (Uint8 **)&pEncConfig->pusBitCode,
				  &pEncConfig->sizeInWord) < 0) {
			CVI_VC_ERR("Failed to load firmware: productId = %d\n",
				   productId);
			return 1;
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
	if (pEncConfig->sizeInWord == 0 && pEncConfig->pusBitCode == NULL) {
		if (LoadFirmware(productId, (Uint8 **)&pEncConfig->pusBitCode,
					&pEncConfig->sizeInWord, fwPath) < 0) {
			CVI_VC_ERR("Failed to load firmware: %s\n", fwPath);
			return 1;
		}
	}
#endif

	return INIT_TEST_ENCODER_OK;
}

extern wait_queue_head_t tWaitQueue[];

static int pfnWaitEncodeDone(void *param)
{
	int ret;
	stTestEncoder *pTestEnc = (stTestEncoder *)param;
	CodecInst *pCodecInst = pTestEnc->handle;
	pTestEnc->tPthreadRunFlag = CVI_TRUE;

	while (!kthread_should_stop()) {
		// wait for enc cmd trigger
		wait_for_completion(&pTestEnc->semSendEncCmd);

		if (!pTestEnc->tPthreadRunFlag || kthread_should_stop())
			break;
		pTestEnc->encConfig.cviEc.originPicType = PIC_TYPE_MAX;
		CVI_VENC_DEBUG("get s chn:%d %llu\n", pCodecInst->s32ChnNum, pCodecInst->yuvCnt);
		ret = cviGetOneStream(pTestEnc, &pTestEnc->tStreamInfo, TIME_BLOCK_MODE);
		CVI_VENC_DEBUG("get s done chn:%d HwTime:%llu cnt:%llu ret:%d\n", pCodecInst->s32ChnNum,
				pTestEnc->tStreamInfo.encHwTime, pCodecInst->yuvCnt++, ret);
		if (ret == TE_ERR_ENC_IS_SUPER_FRAME) {
			ret = cviProcessSuperFrame(pTestEnc, &pTestEnc->tStreamInfo, TIME_BLOCK_MODE);
			complete(&pTestEnc->semGetStreamCmd);
		} else if (ret == RETCODE_SUCCESS) {
			complete(&pTestEnc->semEncDoneCmd);
			complete(&pTestEnc->semGetStreamCmd);
			wake_up(&tWaitQueue[pCodecInst->s32ChnNum]);
		} else {
			CVI_VC_ERR("cviGetOneStream, ret = %d\n", ret);
			return 0;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		cond_resched();
	}

	return 0;
}

void *cviVEncOpen(cviInitEncConfig *pInitEncCfg)
{
	stTestEncoder *pTestEnc = NULL;
	int ret = 0;
	TestEncConfig *pEncConfig;

	ret = cviVcodecInit();
	if (ret < 0) {
		CVI_VC_INFO("cviVcodecInit, %d\n", ret);
	}

	CVI_VC_IF("\n");

	cvi_vc_drv_write_vc_reg(REG_CTRL, 0x28, 0x7);
#if defined(__SOC_PHOBOS__)
	/* disable cpu access vc sram for 180x  romcode use vc sram*/
	cvi_vc_drv_write_vc_reg(REG_CTRL, 0x10, cvi_vc_drv_read_vc_reg(REG_CTRL, 0x10) & (~0x1));
#endif
	pTestEnc = (stTestEncoder *)osal_malloc(sizeof(stTestEncoder));
	if (!pTestEnc) {
		CVI_VC_ERR("VENC context malloc\n");
		goto ERR_CVI_VENC_OPEN;
	}

	pEncConfig = &pTestEnc->encConfig;
	initEncConfigByCtx(pEncConfig, pInitEncCfg);

	ret = initMcuEnv(pEncConfig);

	if (ret != INIT_TEST_ENCODER_OK) {
		CVI_VC_ERR("initMcuEnv, ret = %d\n", ret);
		osal_free(pTestEnc);
		pTestEnc = NULL;
	}

ERR_CVI_VENC_OPEN:

	return (void *)pTestEnc;
}

int cviVEncClose(void *handle)
{
	int i, ret = 0;
	stTestEncoder *pTestEnc = NULL;
	UserDataList *userdataNode = NULL;
	UserDataList *n;

	if (handle == NULL)
		return -1;
	pTestEnc = (stTestEncoder *)handle;

	CVI_VC_IF("\n");

	EnterVcodecLock(pTestEnc->coreIdx);

	ret = cviCheckAndCompareBitstream(pTestEnc);
	if (ret == TE_ERR_ENC_OPEN) {
		CVI_VC_ERR("cviCheckAndCompareBitstream, ret = %d\n", ret);
		LeaveVcodecLock(pTestEnc->coreIdx);
		return ret;
	}

#if CACHE_ENCODE_HEADER
	if (pTestEnc->bEncHeader) {
		for (i = 0; i < ARRAY_SIZE(pTestEnc->headerBackup); i++) {
			if (pTestEnc->headerBackup[i].size)
				osal_kfree(pTestEnc->headerBackup[i].pBuf);
		}
	}
#endif

	cviCloseVpuEnc(pTestEnc);
	cviDeInitVpuEnc(pTestEnc);

	if (pTestEnc->encConfig.pusBitCode) {
#ifndef FIRMWARE_H
		osal_free(pTestEnc->encConfig.pusBitCode);
#endif
		pTestEnc->encConfig.pusBitCode = NULL;
		pTestEnc->encConfig.sizeInWord = 0;
	}

	list_for_each_entry_safe(userdataNode, n, &pTestEnc->encConfig.userdataList, list) {
		if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
			osal_free(userdataNode->userDataBuf);
			list_del(&userdataNode->list);
			osal_free(userdataNode);
		}
	}
	pTestEnc->encConfig.userDataBufSize = 0;

	if (pTestEnc->tPthreadId) {
		pTestEnc->tPthreadRunFlag = CVI_FALSE;
		complete(&pTestEnc->semSendEncCmd);
		kthread_stop(pTestEnc->tPthreadId);
		pTestEnc->tPthreadId = NULL;
	}

	LeaveVcodecLock(pTestEnc->coreIdx);

	if (pTestEnc)
		osal_free(pTestEnc);

	return ret;
}


static void initEncConfigByCtx(TestEncConfig *pEncConfig, cviInitEncConfig *pInitEncCfg)
{
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	initDefaultEncConfig(pEncConfig);

	pEncConfig->cviApiMode = API_MODE_SDK;
	pEncConfig->lineBufIntEn = 1;
#ifdef CVI_H26X_ES_BUFFER_QUEUE_ENABLE
	if (pInitEncCfg->bSingleEsBuf == 1)
		pEncConfig->bEsBufQueueEn = 0;
	else
		pEncConfig->bEsBufQueueEn = pInitEncCfg->bEsBufQueueEn;
#endif
	if (pInitEncCfg->bSingleEsBuf == 1)
		pEncConfig->bIsoSendFrmEn = 0;
	else
		pEncConfig->bIsoSendFrmEn = pInitEncCfg->bIsoSendFrmEn;
	pEncConfig->s32ChnNum = pInitEncCfg->s32ChnNum;

	if (pInitEncCfg->codec == CODEC_H264) {
		pEncConfig->stdMode = STD_AVC;

		pEncConfig->mapType = TILED_FRAME_V_MAP;
		pEncConfig->coda9.enableLinear2Tiled = TRUE;
		pEncConfig->coda9.linear2TiledMode = FF_FRAME;

		pEncConfig->secondary_axi = SECONDARY_AXI_H264;
		pEncConfig->cviEc.h264Entropy.entropyEncModeI = CVI_CABAC;
		pEncConfig->cviEc.h264Entropy.entropyEncModeP = CVI_CABAC;
	} else {
		pEncConfig->stdMode = STD_HEVC;
		pEncConfig->secondary_axi = SECONDARY_AXI_H265;
	}

	CVI_VC_CFG("stdMode = %s, 2ndAXI = 0x%X\n",
		   (pInitEncCfg->codec == CODEC_H264) ? "264" : "265",
		   pEncConfig->secondary_axi);

	pEncConfig->yuv_mode = SOURCE_YUV_ADDR;
	pEncConfig->outNum = 0x7fffffff;

	pEncConfig->picWidth = pInitEncCfg->width;
	pEncConfig->picHeight = pInitEncCfg->height;
	CVI_VC_CFG("picWidth = %d, picHeight = %d\n", pEncConfig->picWidth,
		   pEncConfig->picHeight);

	pCviEc->u32Profile = pInitEncCfg->u32Profile;
	pEncConfig->rcMode = pInitEncCfg->rcMode;
	pCviEc->rcMode = pInitEncCfg->rcMode;
	pEncConfig->decodingRefreshType = pInitEncCfg->decodingRefreshType;
	pCviEc->s32IPQpDelta = pInitEncCfg->s32IPQpDelta;
	pCviEc->s32BgQpDelta = pInitEncCfg->s32BgQpDelta;
	pCviEc->s32ViQpDelta = pInitEncCfg->s32ViQpDelta;
	CVI_VC_CFG(
		"Profile = %d, rcMode = %d, RefreshType = %d, s32IPQpDelta = %d, s32BgQpDelta = %d, s32ViQpDelta = %d\n",
		pCviEc->u32Profile, pCviEc->rcMode,
		pEncConfig->decodingRefreshType, pCviEc->s32IPQpDelta,
		pCviEc->s32BgQpDelta, pCviEc->s32ViQpDelta);

	pCviEc->iqp = pInitEncCfg->iqp;
	pCviEc->pqp = pInitEncCfg->pqp;
	pCviEc->gop = pInitEncCfg->gop;
	pCviEc->bitrate = pInitEncCfg->bitrate;
	CVI_VC_CFG("iqp = %d, pqp = %d, gop = %d, bitrate = %d\n",
		   pInitEncCfg->iqp, pInitEncCfg->pqp, pInitEncCfg->gop,
		   pInitEncCfg->bitrate);

	pCviEc->framerate = pInitEncCfg->framerate;
	pCviEc->maxbitrate = pInitEncCfg->maxbitrate;
	pCviEc->statTime = pInitEncCfg->statTime;
	pCviEc->initialDelay = pInitEncCfg->initialDelay;
	CVI_VC_CFG(
		"framerate = %d, maxbitrate = %d, statTime = %d, initialDelay = %d\n",
		pInitEncCfg->framerate, pCviEc->maxbitrate, pCviEc->statTime,
		pCviEc->initialDelay);

	pCviEc->bitstreamBufferSize = pInitEncCfg->bitstreamBufferSize;
	pCviEc->singleLumaBuf = pInitEncCfg->singleLumaBuf;
	pCviEc->bSingleEsBuf = pInitEncCfg->bSingleEsBuf;
	CVI_VC_CFG("bs size = 0x%X, 1YBuf = %d, bSingleEsBuf = %d\n",
		   pCviEc->bitstreamBufferSize, pCviEc->singleLumaBuf,
		   pCviEc->bSingleEsBuf);

	if (pInitEncCfg->virtualIPeriod) {
		pCviEc->virtualIPeriod = pInitEncCfg->virtualIPeriod;
		CVI_VC_CFG("virtualIPeriod = %d\n", pCviEc->virtualIPeriod);
	}
	pCviEc->svcEnable = pInitEncCfg->svc_enable;
	pCviEc->svcParam.fg_protect_en = pInitEncCfg->fg_protect_en;
	pCviEc->svcParam.fg_dealt_qp = pInitEncCfg->fg_dealt_qp;
	pCviEc->svcParam.complex_scene_detect_en = pInitEncCfg->complex_scene_detect_en;
	pCviEc->svcParam.complex_scene_low_th = pInitEncCfg->complex_scene_low_th;
	pCviEc->svcParam.complex_scene_hight_th = pInitEncCfg->complex_scene_hight_th;
	pCviEc->svcParam.middle_min_percent = pInitEncCfg->middle_min_percent;
	pCviEc->svcParam.complex_min_percent = pInitEncCfg->complex_min_percent;
	pCviEc->svcParam.smart_ai_en = pInitEncCfg->smart_ai_en;
	strcpy(pEncConfig->bitstreamFileName, BS_NAME);
	strcpy(pEncConfig->yuvFileName, YUV_NAME);

	// user data bufs
	pEncConfig->userDataBufSize = pInitEncCfg->userDataMaxLength;
	INIT_LIST_HEAD(&pEncConfig->userdataList);

// from BITMONET-68 in CnM Jira:
// There are two bit-stream handling method such as 'ring-buffer' and 'line-bufer'.
// Also these two method cannot be activated together.
}

void initDefaultEncConfig(TestEncConfig *pEncConfig)
{
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

#ifndef PLATFORM_NON_OS
	cviVcodecMask();
#endif

	CVI_VC_TRACE("\n");

	cviVcodecGetVersion();

#if CFG_MEM
	osal_memset(&dramCfg, 0x0, sizeof(DRAM_CFG));
#endif

#ifdef ENABLE_LOG
	InitLog();
#endif

	osal_memset(pEncConfig, 0, sizeof(TestEncConfig));
#ifdef CODA980
	pEncConfig->stdMode = STD_AVC;
	pEncConfig->mapType = LINEAR_FRAME_MAP;
#else
	pEncConfig->stdMode = STD_HEVC;
	pEncConfig->mapType = COMPRESSED_FRAME_MAP;
#endif
	pEncConfig->frame_endian = VPU_FRAME_ENDIAN;
	pEncConfig->stream_endian = VPU_STREAM_ENDIAN;
	pEncConfig->source_endian = VPU_SOURCE_ENDIAN;
#ifdef PLATFORM_NON_OS
	pEncConfig->yuv_mode = SOURCE_YUV_FPGA;
#endif
	pEncConfig->RcEnable = -1;
	pEncConfig->picQpY = -1;
	pCviEc->firstFrmstartQp = -1;
	pCviEc->framerate = 30;

	COMP(pEncConfig->coreIdx, 0);
}
#ifdef VC_DRIVER_TEST
struct OptionExt options_help[] = {
	{ "output", 1, NULL, 0,
	  "--output                    bitstream path\n" },
	{ "input", 1, NULL, 0,
	  "--input                     YUV file path\n" },
	  /* The value of InputFile in a cfg is replaced to this value */
	{ "codec", 1, NULL, 0,
	  "--codec                     codec index, HEVC = 12, AVC = 0\n" },
	{ "cfgFileName", 1, NULL, 0,
	  "--cfgFileName               cfg file path\n" },
	{ "cfgFileName0", 1, NULL, 0,
	  "--cfgFileName0              cfg file 0 path\n" },
	{ "cfgFileName1", 1, NULL, 0,
	  "--cfgFileName1              cfg file 1 path\n" },
	{ "cfgFileName2", 1, NULL, 0,
	  "--cfgFileName2              cfg file 2 path\n" },
	{ "cfgFileName3", 1, NULL, 0,
	  "--cfgFileName3              cfg file 3 path\n" },
	{ "coreIdx", 1, NULL, 0,
	  "--coreIdx                   core index: default 0\n" },
	{ "picWidth", 1, NULL, 0,
	  "--picWidth                  source width\n" },
	{ "picHeight", 1, NULL, 0,
	  "--picHeight                 source height\n" },
	{ "EncBitrate", 1, NULL, 0,
	  "--EncBitrate                RC bitrate in kbps\n" },
	  /* In case of without cfg file, if this option has value then RC will be enabled */
	{ "RcMode", 1, NULL, 0,
	  "--RcMode                    RC mode. 0: CBR\n" },
	{ "changePos", 1, NULL, 0,
	  "--changePos                 VBR bitrate change poistion\n" },
	{ "frmLostOpen", 1, NULL, 0,
	  "--frmLostOpen               auto-skip frame enable\n" },
	{ "maxIprop", 1, NULL, 0,
	  "--maxIprop                  max I frame bitrate ratio to P frame\n" },
	{ "enable-ringBuffer", 0, NULL, 0,
	  "--enable-ringBuffer         enable stream ring buffer mode\n" },
	{ "enable-lineBufInt", 0, NULL, 0,
	  "--enable-lineBufInt         enable linebuffer interrupt\n" },
	{ "mapType", 1, NULL, 0,
	  "--mapType                   mapType\n" },
	{ "loop-count", 1, NULL, 0,
	  "--loop-count                integer number. loop test, default 0\n" },
	{ "enable-cbcrInterleave", 0, NULL, 0,
	  "--enable-cbcrInterleave     enable cbcr interleave\n" },
	{ "nv21", 1, NULL, 0,
	  "--nv21                      enable NV21(must set enable-cbcrInterleave)\n" },
	{ "packedFormat", 1, NULL, 0,
	  "--packedFormat              1:YUYV, 2:YVYU, 3:UYVY, 4:VYUY\n" },
	{ "rotAngle", 1, NULL, 0,
	  "--rotAngle                  rotation angle(0,90,180,270), Not supported on WAVE420L\n" },
	{ "mirDir", 1, NULL, 0,
	  "--mirDir                    1:Vertical, 2: Horizontal, 3:Vert-Horz, Not supported on WAVE420L\n" },
	  /* 20 */
	{ "secondary-axi", 1, NULL, 0,
	  "--secondary-axi             0~7: bit mask values, Please refer programmer's guide or datasheet\n" },
	  /* 1:IMD(not supported on WAVE420L), 2: RDO, 4: LF */
	{ "frame-endian", 1, NULL, 0,
	  "--frame-endian              16~31, default 31(LE) Please refer programmer's guide or datasheet\n" },
	{ "stream-endian", 1, NULL, 0,
	  "--stream-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n" },
	{ "source-endian", 1, NULL, 0,
	  "--source-endian             16~31, default 31(LE) Please refer programmer's guide or datasheet\n" },
	{ "ref_stream_path", 1, NULL, 0,
	  "--ref_stream_path           golden data which is compared with encoded stream when -c option\n" },
	{ "srcFormat3p4b", 1, NULL, 0,
	  "--srcFormat3p4b             [WAVE420]MUST BE enabled when yuv src format 3pixel 4byte format\n" },
	{ "decodingRefreshType", 1, NULL, 0,
	  "--decRefreshType            decodingRefreshType, 0 = disable, 1 = enable\n" },
	{ "gopSize", 1, NULL, 0,
	  "--gopSize                   gopSize, The interval of 2 Intra frames\n" },
	{ "tempLayer", 1, NULL, 0,
	  "--tempLayer                 temporal layer coding, Range from 1 to 3\n" },
	{ "useDeriveLambdaWeight", 1, NULL, 0,
	  "--useDeriveLambdaWeight     useDeriveLambdaWeight, 1 = use derived Lambda weight, 0 = No\n" },
	{ "dynamicMergeEnable", 1, NULL, 0,
	  "--dynamicMergeEnable        dynamicMergeEnable, 1 = enable dynamic merge, 0 = disable\n" },
	{ "IndeSliceMode", 1, NULL, 0,
	  "--IndeSliceMode             IndeSliceMode, 1 = enable, 0 = disable\n" },
	{ "IndeSliceArg", 1, NULL, 0,
	  "--IndeSliceArg              IndeSliceArg\n" },
	{ "RateControl", 1, NULL, 0,
	  "--RateControl               RateControl\n" },
	{ "RcInitQp", 1, NULL, 0,
	  "--RcInitQp                  Initial QP of rate control\n" },
	{ "PIC_QP_Y", 1, NULL, 0,
	  "--PIC_QP_Y                  PIC_QP_Y\n" },
	{ "RoiCfgType", 1, NULL, 0,
	  "--RoiCfgType                Roi map cfg type\n" },
#if CFG_MEM
	{ "code-addr", 1, NULL, 0,
	  "--code-addr                 cvitest, fw address\n" },
	{ "code-size", 1, NULL, 0,
	  "--code-size                 cvitest, fw size\n" },
	{ "vpu-dram-addr", 1, NULL, 0,
	  "--vpu-dram-addr             cvitest, dram address for vcodec\n" },
	{ "vpu-dram-size", 1, NULL, 0,
	  "--vpu-dram-size             cvitest, dram size for vcodec\n" },
	{ "src-yuv-addr", 1, NULL, 0,
	  "--src-yuv-addr              cvitest, source yuv address\n" },
	{ "src-yuv-size", 1, NULL, 0,
	  "--src-yuv-size              cvitest, source yuv size\n" },
#endif
	{ NULL, 0, NULL, 0, NULL },
};

struct option options[MAX_GETOPT_OPTIONS];
static int parseArgs(int argc, char **argv, TestEncConfig *pEncConfig)
{
	char *optString = "c:rbhvn:t:";
	int opt, index = 0, ret = 0, i;
	cviEncCfg *pCviEc = &pEncConfig->cviEc;

	for (i = 0; i < MAX_GETOPT_OPTIONS; i++) {
		if (options_help[i].name == NULL)
			break;
		osal_memcpy(&options[i], &options_help[i],
			    sizeof(struct option));
	}

	CVI_VC_TRACE("argc = %d, optString = %s\n", argc, optString);
	getopt_init();
	memset(gCfgFileName, 0, MAX_NUM_INSTANCE * MAX_FILE_PATH);
	while ((opt = getopt_long(argc, argv, optString, options,
				  &index)) != -1) {
		switch (opt) {
		case 'n':
			pEncConfig->outNum = atoi(optarg);
			CVI_VC_TRACE("optarg = %s, outNum = %d\n", optarg,
				     pEncConfig->outNum);
			break;
		case 'c':
			pEncConfig->compare_type |= (1 << MODE_COMP_ENCODED);
			CVI_VC_FLOW("Stream compare Enable\n");
			break;
		case 'h':
			help(options_help, argv[0]);
			return 0;
		case 't':
			gNumInstance = atoi(optarg);
			CVI_VC_CFG("gNumInstance %d\n", gNumInstance);
			break;
		case 0:
			CVI_VC_TRACE("name = %s\n", options[index].name);
			if (!strcmp(options[index].name, "output")) {
				osal_memcpy(pEncConfig->bitstreamFileName,
					    optarg, strlen(optarg));
#ifdef REDUNDENT_CODE
				ChangePathStyle(pEncConfig->bitstreamFileName);
#endif
			} else if (!strcmp(options[index].name, "input")) {
				strcpy(optYuvPath, optarg);
#ifdef REDUNDENT_CODE
				ChangePathStyle(optYuvPath);
#endif
			} else if (!strcmp(options[index].name, "codec")) {
				pEncConfig->stdMode = (CodStd)atoi(optarg);
				if (pEncConfig->stdMode == STD_AVC)
					pEncConfig->mapType = LINEAR_FRAME_MAP;
				else
					pEncConfig->mapType =
						COMPRESSED_FRAME_MAP;
			} else if (!strcmp(options[index].name,
					   "cfgFileName") ||
				   !strcmp(options[index].name,
					   "cfgFileName0")) {
				osal_memcpy(gCfgFileName[0], optarg,
					    strlen(optarg));
			} else if (!strcmp(options[index].name,
					   "cfgFileName1")) {
				osal_memcpy(gCfgFileName[1], optarg,
					    strlen(optarg));
			} else if (!strcmp(options[index].name,
					   "cfgFileName2")) {
				osal_memcpy(gCfgFileName[2], optarg,
					    strlen(optarg));
			} else if (!strcmp(options[index].name,
					   "cfgFileName3")) {
				osal_memcpy(gCfgFileName[3], optarg,
					    strlen(optarg));
			} else if (!strcmp(options[index].name, "coreIdx")) {
				pEncConfig->coreIdx = atoi(optarg);
			} else if (!strcmp(options[index].name, "picWidth")) {
				pEncConfig->picWidth = atoi(optarg);
				CVI_VC_TRACE("picWidth = %d\n",
					     pEncConfig->picWidth);
			} else if (!strcmp(options[index].name, "picHeight")) {
				pEncConfig->picHeight = atoi(optarg);
				CVI_VC_TRACE("picHeight = %d\n",
					     pEncConfig->picHeight);
			} else if (!strcmp(options[index].name, "EncBitrate")) {
				pEncConfig->kbps = atoi(optarg);
			} else if (!strcmp(options[index].name, "RcMode")) {
				pEncConfig->rcMode = atoi(optarg);
			} else if (!strcmp(options[index].name, "changePos")) {
				pEncConfig->changePos = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "frmLostOpen")) {
				pEncConfig->frmLostOpen = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "enable-ringBuffer")) {
				pEncConfig->ringBufferEnable = TRUE;
			} else if (!strcmp(options[index].name,
					   "enable-lineBufInt")) {
				pEncConfig->lineBufIntEn = TRUE;
			} else if (!strcmp(options[index].name, "loop-count")) {
				pEncConfig->loopCount = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "enable-cbcrInterleave")) {
				pEncConfig->cbcrInterleave = 1;
				if (pEncConfig->stdMode == STD_AVC) {
					pEncConfig->mapType = TILED_FRAME_MB_RASTER_MAP;
					pEncConfig->coda9.enableLinear2Tiled = TRUE;
					pEncConfig->coda9.linear2TiledMode = FF_FRAME;
				}
			} else if (!strcmp(options[index].name, "nv21")) {
				pEncConfig->nv21 = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "packedFormat")) {
				pEncConfig->packedFormat = atoi(optarg);
			} else if (!strcmp(options[index].name, "rotAngle")) {
				pEncConfig->rotAngle = atoi(optarg);
			} else if (!strcmp(options[index].name, "mirDir")) {
				pEncConfig->mirDir = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "secondary-axi")) {
				pEncConfig->secondary_axi = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "frame-endian")) {
				pEncConfig->frame_endian = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "stream-endian")) {
				pEncConfig->stream_endian = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "source-endian")) {
				pEncConfig->source_endian = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "ref_stream_path")) {
				osal_memcpy(pEncConfig->ref_stream_path, optarg,
					    strlen(optarg));
#ifdef REDUNDENT_CODE
				ChangePathStyle(pEncConfig->ref_stream_path);
#endif
			} else if (!strcmp(options[index].name,
					   "srcFormat3p4b")) {
				pEncConfig->srcFormat3p4b = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "decodingRefreshType")) {
				pEncConfig->decodingRefreshType = atoi(optarg);
			} else if (!strcmp(options[index].name, "gopSize")) {
				pEncConfig->gopSize = atoi(optarg);
			} else if (!strcmp(options[index].name, "tempLayer")) {
				pEncConfig->tempLayer = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "useDeriveLambdaWeight")) {
				pEncConfig->useDeriveLambdaWeight =
					atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "dynamicMergeEnable")) {
				pEncConfig->dynamicMergeEnable = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "IndeSliceMode")) {
				pEncConfig->independSliceMode = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "IndeSliceArg")) {
				pEncConfig->independSliceModeArg = atoi(optarg);
			} else if (!strcmp(options[index].name,
					   "RateControl")) {
				pEncConfig->RcEnable = atoi(optarg);
			} else if (!strcmp(options[index].name, "maxIprop")) {
				pCviEc->u32MaxIprop = atoi(optarg);
			} else if (!strcmp(options[index].name, "RcInitQp")) {
				pCviEc->firstFrmstartQp = atoi(optarg);
			} else if (!strcmp(options[index].name, "RoiCfgType")) {
				pEncConfig->roi_cfg_type = atoi(optarg);
			} else if (!strcmp(options[index].name, "PIC_QP_Y")) {
				pEncConfig->picQpY = atoi(optarg);
#if CFG_MEM
			} else if (!strcmp(options[index].name, "code-addr")) {
				if (kstrtoll(optarg, 16, &dramCfg.pucCodeAddr) != 0) {
					pr_err("pucCodeAddr input error\n");
				}
				CVI_VC_TRACE("pucCodeAddr = 0x%lX\n",
					     dramCfg.pucCodeAddr);
			} else if (!strcmp(options[index].name, "code-size")) {
				dramCfg.iCodeSize = atoi(optarg);
				CVI_VC_TRACE("pucCodeSize = 0x%X\n",
					     dramCfg.iCodeSize);
			} else if (!strcmp(options[index].name,
					   "vpu-dram-addr")) {
				if (kstrtoll(optarg, 16, &dramCfg.pucVpuDramAddr) != 0) {
					pr_err("pucVpuDramAddr input error\n");
				}
				CVI_VC_TRACE("pucVpuDramAddr = 0x%lX\n",
					     dramCfg.pucVpuDramAddr);
			} else if (!strcmp(options[index].name,
					   "vpu-dram-size")) {
				if (kstrtoll(optarg, 16, &dramCfg.iVpuDramSize) != 0) {
					pr_err("iVpuDramSize input error\n");
				}
				CVI_VC_TRACE("pucVpuDramSize = 0x%X\n",
					     dramCfg.iVpuDramSize);
			} else if (!strcmp(options[index].name,
					   "src-yuv-addr")) {
				if (kstrtoll(optarg, 16, &dramCfg.pucSrcYuvAddr) != 0) {
					pr_err("pucSrcYuvAddr input error\n");
				}
				CVI_VC_TRACE("pucSrcYuvAddr = 0x%lX\n",
					     dramCfg.pucSrcYuvAddr);
			} else if (!strcmp(options[index].name,
					   "src-yuv-size")) {
				if (kstrtoll(optarg, 16, &dramCfg.iSrcYuvSize) != 0) {
					pr_err("iSrcYuvSize input error\n");
				}
				CVI_VC_TRACE("pucSrcYuvSize = 0x%X\n",
					     dramCfg.iSrcYuvSize);
#endif
			} else {
				CVI_VC_ERR("not exist pEncConfig = %s\n",
					   options[index].name);
				help(options_help, argv[0]);
				return 1;
			}
			break;
		default:
			help(options_help, argv[0]);
			return 1;
		}
	}

	return ret;
}
#endif

static int checkEncConfig(TestEncConfig *pEncConfig, Uint32 productId)
{
	int ret = 0;

	if (pEncConfig->mapType == TILED_FRAME_MB_RASTER_MAP ||
	    pEncConfig->mapType == TILED_FIELD_MB_RASTER_MAP)
		pEncConfig->cbcrInterleave = TRUE;

	if (pEncConfig->rotAngle > 0 || pEncConfig->mirDir > 0)
		pEncConfig->useRot = TRUE;

#if CFG_MEM
	if (checkDramCfg(&dramCfg)) {
		CVI_VC_ERR("checkDramCfg\n");
		return -1;
	}
#endif

	if (checkParamRestriction(productId, pEncConfig) == FALSE) {
		CVI_VC_ERR("checkParamRestriction\n");
		return 1;
	}

	return ret;
}

#ifdef VC_DRIVER_TEST
#if CFG_MEM
static int checkDramCfg(DRAM_CFG *pDramCfg)
{
	if (!pDramCfg->pucCodeAddr || !pDramCfg->iCodeSize ||
	    !pDramCfg->pucVpuDramAddr || !pDramCfg->iVpuDramSize ||
	    !pDramCfg->pucSrcYuvAddr || !pDramCfg->iSrcYuvSize) {
		CVI_VC_TRACE("dramCfg = 0\n");
		return -1;
	}
	return 0;
}
#endif

static int initEncConfigByArgv(int argc, char **argv, TestEncConfig *pEncConfig)
{
	int ret = 0;

	initDefaultEncConfig(pEncConfig);

	CVI_VC_TRACE("\n");

	pEncConfig->cviApiMode = API_MODE_DRIVER;
	pEncConfig->lineBufIntEn = 1;

	ret = parseArgs(argc, argv, pEncConfig);

	if (ret < 0) {
		CVI_VC_ERR("parseArgs\n");
		return ret;
	}

	// from BITMONET-68 in CnM Jira:
	// There are two bit-stream handling method such as 'ring-buffer' and 'line-bufer'.
	// Also these two method cannot be activated together.
	//assert(pEncConfig->lineBufIntEn == !pEncConfig->ringBufferEnable);

	return ret;
}

static int cviGetStream(stTestEncoder *pTestEnc)
{
	int ret = 0;
	EncOutputInfo *pOutputInfo = &pTestEnc->outputInfo;
	EncOpenParam *peop = &pTestEnc->encOP;

	ret = cviGetEncodedInfo(pTestEnc, TIME_BLOCK_MODE);

	if (ret == TE_STA_ENC_TIMEOUT) {
		return RET_VCODEC_TIMEOUT;
	} else if (ret) {
		CVI_VC_ERR("cviGetEncodedInfo, ret = %d\n", ret);
		return ret;
	}

	if (peop->ringBufferEnable == 0) {
		ret = cviCheckOutputInfo(pTestEnc);

		if (ret == TE_ERR_ENC_OPEN) {
			CVI_VC_ERR("cviCheckOutputInfo, ret = %d\n", ret);
			return ret;
		}

#ifdef SUPPORT_DONT_READ_STREAM
		updateBitstream(&pTestEnc->vbStream[0],
				pOutputInfo->bitstreamSize);
#else

		if (pOutputInfo->bitstreamSize) {
			ret = BitstreamReader_Act(
				      pTestEnc->bsReader,
				      pOutputInfo->bitstreamBuffer,
				      peop->bitstreamBufferSize,
				      pOutputInfo->bitstreamSize,
				      pTestEnc->comparatorBitStream);

			if (ret == FALSE) {
				CVI_VC_ERR("ES, ret = %d\n", ret);
				return TE_ERR_ENC_OPEN;
			}
		}

#endif
	}

	ret = cviCheckEncodeEnd(pTestEnc);
	cviVPU_ChangeState(pTestEnc->handle);

	return ret;
}


//extern char *optarg; /* argument associated with option */
int TestEncoder(TestEncConfig *pEncConfig)
{
	stTestEncoder *pTestEnc = osal_malloc(sizeof(stTestEncoder));
	int suc = 0;
	int ret;

	VDI_POWER_ON_DOING_JOB(pEncConfig->coreIdx, ret,
			       initEncOneFrame(pTestEnc, pEncConfig));

	if (ret == TE_ERR_ENC_INIT) {
		CVI_VC_ERR("ret %d\n", ret);
		goto ERR_ENC_INIT;
	} else if (ret == TE_ERR_ENC_OPEN) {
		CVI_VC_ERR("ret %d\n", ret);
		goto ERR_ENC_OPEN;
	}

	while (1) {
		vdi_set_clock_gate(pEncConfig->coreIdx, CLK_ENABLE);
		ret = cviEncodeOneFrame(pTestEnc);
		if (ret == TE_STA_ENC_BREAK) {
			CVI_VC_INFO("cviEncodeOneFrame, ret = %d\n", ret);
			vdi_set_clock_gate(pEncConfig->coreIdx, CLK_DISABLE);
			break;
		} else if (ret == TE_ERR_ENC_OPEN) {
			CVI_VC_INFO("cviEncodeOneFrame, ret = %d\n", ret);
			vdi_set_clock_gate(pEncConfig->coreIdx, CLK_DISABLE);
			goto ERR_ENC_OPEN;
		}

		ret = cviGetStream(pTestEnc);
		vdi_set_clock_gate(pEncConfig->coreIdx, CLK_DISABLE);

		if (ret == TE_STA_ENC_BREAK) {
			break;
		} else if (ret) {
			CVI_VC_ERR("cviGetStream, ret = %d\n", ret);
			goto ERR_ENC_OPEN;
		}
	}

	VDI_POWER_ON_DOING_JOB(pEncConfig->coreIdx, ret,
			       cviCheckAndCompareBitstream(pTestEnc));

	if (ret == TE_ERR_ENC_OPEN) {
		CVI_VC_ERR("cviCheckAndCompareBitstream\n");
		goto ERR_ENC_OPEN;
	}

	suc = 1;

ERR_ENC_OPEN:
	// Now that we are done with encoding, close the open instance.
	cviCloseVpuEnc(pTestEnc);
ERR_ENC_INIT:
	cviDeInitVpuEnc(pTestEnc);
	osal_free(pTestEnc);
	return suc;
}

static int FnEncoder(void *param)
{
	int ret;
	TestEncConfig *pEncConfig = (TestEncConfig *)param;
	//pthread_t tid = pthread_self();

	//SetTidToInstIdx(tid, pEncConfig->instNum);

	ret = TestEncoder(pEncConfig);

	if (ret)
		return ret;
	else
		return 0;
}

int cvitest_venc_main(int argc, char **argv)
{
	BOOL debugMode = FALSE;
	TestEncConfig *pEncConfig[MAX_NUM_INSTANCE];
	struct task_struct *thread_id[MAX_NUM_INSTANCE];
	int ret = 0;
	int i = 0;
	struct sched_param param = {
				.sched_priority = 80,
			};

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pEncConfig[i] = osal_malloc(sizeof(TestEncConfig));
	}
	ret = cviVcodecInit();

	if (ret < 0) {
		CVI_VC_INFO("cviVcodecInit, %d\n", ret);
	}

	ret = initEncConfigByArgv(argc, argv, pEncConfig[0]);

	if (ret < 0) {
		CVI_VC_ERR("initEncConfigByArgv\n");
		return ret;
	}

	VDI_POWER_ON_DOING_JOB(pEncConfig[0]->coreIdx, ret,
			       initMcuEnv(pEncConfig[0]));

	if (ret != INIT_TEST_ENCODER_OK) {
		CVI_VC_ERR("initMcuEnv, ret = %d\n", ret);
		return ret;
	}

	for (i = 1; i < MAX_NUM_INSTANCE; i++) {
		memcpy(pEncConfig[i], pEncConfig[0], sizeof(TestEncConfig));
		pEncConfig[i]->instNum = (i);
	}

	ret = 1;
	//assert(gNumInstance <= MAX_NUM_INSTANCE);

	for (i = 0; i < gNumInstance; i++) {
		thread_id[i] = kthread_run(FnEncoder,
					   pEncConfig[i], "FnEncoder_%d", i);

		if (IS_ERR(thread_id[i])) {
			CVI_VC_ERR("pthread_create error!\n");
			ret = 0;
			goto BAILOUT;
		}
		sched_setscheduler(thread_id[i], SCHED_RR, &param);
	}

	for (i = 0; i < gNumInstance; i++) {
		if (thread_id[i]) {
			kthread_stop(thread_id[i]);
		}

		thread_id[i] = NULL;
	}
BAILOUT:

	if (debugMode == TRUE) {
		VPU_DeInit(pEncConfig[0]->coreIdx);
	}

	vdi_release(pEncConfig[0]->coreIdx);
	VPU_SetCoreStatus(pEncConfig[0]->coreIdx, 0);

	if (pEncConfig[0]->pusBitCode) {
#ifndef FIRMWARE_H
		osal_free(pEncConfig[0]->pusBitCode);
#endif
		pEncConfig[0]->pusBitCode = NULL;
		pEncConfig[0]->sizeInWord = 0;
	}

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		osal_free(pEncConfig[i]);
	}

	return ret == 1 ? 0 : 1;
}

int cvi_venc_test(u_long arg)
{
#define MAX_ARG_CNT 30
	char buf[512];
	char *pArgv[MAX_ARG_CNT] = {0};
	char *save_ptr;
	unsigned int u32Argc = 0;
	char *pBuf;
	unsigned int __user *argp = (unsigned int __user *)arg;

	memset(buf, 0, 512);

	if (argp != NULL) {
		if (copy_from_user(buf, (char *)argp, 512))
			return -1;
	}

	pBuf = buf;

	while (NULL != (pArgv[u32Argc] = cvi_strtok_r(pBuf, " ", &save_ptr))) {
		u32Argc++;

		if (u32Argc >= MAX_ARG_CNT) {
			break;
		}

		pBuf = NULL;
	}

	return cvitest_venc_main(u32Argc, pArgv);
}
#endif
