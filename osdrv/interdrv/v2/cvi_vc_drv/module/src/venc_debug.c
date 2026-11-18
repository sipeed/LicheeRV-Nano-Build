#ifdef CLI_DEBUG_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tcli.h"
#include "cvi_venc.h"
#include "cvi_sys.h"
#include "venc.h"

#ifndef UNUSED
#define UNUSED(x) ((x) = (x))
#endif

extern venc_context *handle;
static int bVencCliReg;
static pthread_mutex_t vencCliMutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_RUN_CNT (3)

typedef struct _VENC_S_CLI_DBG {
	int dump_yuv_cnt;
	int bDumpBsEnable;
	CVI_S32 s32handlerState;
	CVI_S32 s32handerExitRetcode;
	CVI_U32 u32MaxBsSize;
	CVI_U32 u32RunCnt[MAX_RUN_CNT];

} VENC_S_CLI_DBG;

VENC_S_CLI_DBG __astVencCliDbgInfo[VENC_MAX_CHN_NUM];

static void getCodecTypeStr(PAYLOAD_TYPE_E enType, char *pcCodecType)
{
	switch (enType) {
	case PT_JPEG:
		strcpy(pcCodecType, "JPEG");
		break;
	case PT_MJPEG:
		strcpy(pcCodecType, "MJPEG");
		break;
	case PT_H264:
		strcpy(pcCodecType, "H264");
		break;
	case PT_H265:
		strcpy(pcCodecType, "H265");
		break;
	default:
		strcpy(pcCodecType, "N/A");
		break;
	}
}

static void getRcModeStr(VENC_RC_MODE_E enRcMode, char *pRcMode)
{
	switch (enRcMode) {
	case VENC_RC_MODE_H264CBR:
	case VENC_RC_MODE_H265CBR:
	case VENC_RC_MODE_MJPEGCBR:
		strcpy(pRcMode, "CBR");
		break;
	case VENC_RC_MODE_H264VBR:
	case VENC_RC_MODE_H265VBR:
	case VENC_RC_MODE_MJPEGVBR:
		strcpy(pRcMode, "VBR");
		break;
	case VENC_RC_MODE_H264AVBR:
	case VENC_RC_MODE_H265AVBR:
		strcpy(pRcMode, "AVBR");
		break;
	case VENC_RC_MODE_H264QVBR:
	case VENC_RC_MODE_H265QVBR:
		strcpy(pRcMode, "QVBR");
		break;
	case VENC_RC_MODE_H264FIXQP:
	case VENC_RC_MODE_H265FIXQP:
	case VENC_RC_MODE_MJPEGFIXQP:
		strcpy(pRcMode, "FIXQP");
		break;
	case VENC_RC_MODE_H264QPMAP:
	case VENC_RC_MODE_H265QPMAP:
		strcpy(pRcMode, "QPMAP");
		break;
	default:
		strcpy(pRcMode, "N/A");
		break;
	}
}

static void getGopModeStr(VENC_GOP_MODE_E enGopMode, char *pcGopMode)
{
	switch (enGopMode) {
	case VENC_GOPMODE_NORMALP:
		strcpy(pcGopMode, "NORMALP");
		break;
	case VENC_GOPMODE_DUALP:
		strcpy(pcGopMode, "DUALP");
		break;
	case VENC_GOPMODE_SMARTP:
		strcpy(pcGopMode, "SMARTP");
		break;
	case VENC_GOPMODE_ADVSMARTP:
		strcpy(pcGopMode, "ADVSMARTP");
		break;
	case VENC_GOPMODE_BIPREDB:
		strcpy(pcGopMode, "BIPREDB");
		break;
	case VENC_GOPMODE_LOWDELAYB:
		strcpy(pcGopMode, "LOWDELAYB");
		break;
	case VENC_GOPMODE_BUTT:
		strcpy(pcGopMode, "BUTT");
		break;
	default:
		strcpy(pcGopMode, "N/A");
		break;
	}
}

static void getPixelFormatStr(PIXEL_FORMAT_E enPixelFormat, char *pcPixelFormat)
{
	switch (enPixelFormat) {
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strcpy(pcPixelFormat, "YUV422");
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strcpy(pcPixelFormat, "YUV420");
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strcpy(pcPixelFormat, "YUV444");
		break;
	case PIXEL_FORMAT_NV12:
		strcpy(pcPixelFormat, "NV12");
		break;
	case PIXEL_FORMAT_NV21:
		strcpy(pcPixelFormat, "NV21");
		break;
	default:
		strcpy(pcPixelFormat, "N/A");
		break;
	}
}

static void getFrameRate(venc_proc_info_t *ptVencProcInfo,
			 CVI_U32 *pu32SrcFrameRate, CVI_FR32 *pfr32DstFrameRate)
{
	switch (ptVencProcInfo->chnAttr.stRcAttr.enRcMode) {
	case VENC_RC_MODE_H264CBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Cbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265CBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Cbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGCBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegCbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264VBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264Vbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265VBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265Vbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stMjpegVbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264FIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264FixQp
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					     .stH264FixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265FIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265FixQp
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					     .stH265FixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_MJPEGFIXQP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					    .stMjpegFixQp.u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					     .stMjpegFixQp.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264AVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264AVbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265AVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265AVbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264QVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QVbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265QVBR:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QVbr
					     .fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H264QPMAP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH264QpMap
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					     .stH264QpMap.fr32DstFrameRate;
		break;
	case VENC_RC_MODE_H265QPMAP:
		*pu32SrcFrameRate = ptVencProcInfo->chnAttr.stRcAttr.stH265QpMap
					    .u32SrcFrameRate;
		*pfr32DstFrameRate = ptVencProcInfo->chnAttr.stRcAttr
					     .stH265QpMap.fr32DstFrameRate;
		break;
	default:
		break;
	}
}

extern void *venc_get_share_mem(void);

static void cliVencInfoShow(int idx)
{
	venc_proc_info_t *pProcInfoShareMem =
		(venc_proc_info_t *)venc_get_share_mem();
	if (!pProcInfoShareMem) {
		tcli_print("no venc channel has been created.\n");
		return;
	}

	int roiIdx = 0;
	venc_proc_info_t *ptVencProcInfo = (pProcInfoShareMem + idx);

	if (ptVencProcInfo->u8ChnUsed == 1) {
		char cCodecType[6] = { '\0' };
		char cRcMode[6] = { '\0' };
		char cGopMode[10] = { '\0' };
		char cPixelFormat[8] = { '\0' };
		CVI_U32 u32SrcFrameRate = 0;
		CVI_FR32 fr32DstFrameRate = 0;

		getCodecTypeStr(ptVencProcInfo->chnAttr.stVencAttr.enType,
				cCodecType);
		getRcModeStr(ptVencProcInfo->chnAttr.stRcAttr.enRcMode,
			     cRcMode);
		getGopModeStr(ptVencProcInfo->chnAttr.stGopAttr.enGopMode,
			      cGopMode);
		getPixelFormatStr(ptVencProcInfo->stFrame.enPixelFormat,
				  cPixelFormat);
		getFrameRate(ptVencProcInfo, &u32SrcFrameRate,
			     &fr32DstFrameRate);

		tcli_print(
			"-----VENC CHN ATTR 1---------------------------------------------\n");
		tcli_print(
			"ID: %d\t Width: %u\t Height: %u\t Type: %s\t RcMode: %s",
			idx, ptVencProcInfo->chnAttr.stVencAttr.u32PicWidth,
			ptVencProcInfo->chnAttr.stVencAttr.u32PicHeight,
			cCodecType, cRcMode);
		tcli_print(
			"\t ByFrame: %s\t Sequence: %u\t LeftBytes: %u\t LeftFrm: %u",
			ptVencProcInfo->chnAttr.stVencAttr.bByFrame ? "Y" : "N",
			ptVencProcInfo->stStream.u32Seq,
			ptVencProcInfo->chnStatus.u32LeftStreamBytes,
			ptVencProcInfo->chnStatus.u32LeftStreamFrames);
		tcli_print("\t CurPacks: %u\t GopMode: %s\t Prio: %d\n",
			   ptVencProcInfo->chnStatus.u32CurPacks, cGopMode,
			   ptVencProcInfo->stChnParam.u32Priority);

		tcli_print(
			"-----VENC CHN ATTR 2-----------------------------------------------\n");
		tcli_print(
			"VeStr: Y\t SrcFr: %u\t TarFr: %u\t Timeref: %u\t PixFmt: %s",
			u32SrcFrameRate, fr32DstFrameRate,
			ptVencProcInfo->stFrame.u32TimeRef, cPixelFormat);
		tcli_print("\t PicAddr: 0x%llx\t WakeUpFrmCnt: %u\n",
			   ptVencProcInfo->stFrame.u64PhyAddr[0],
			   ptVencProcInfo->stChnParam.u32PollWakeUpFrmCnt);

		tcli_print(
			"-----VENC CROP INFO------------------------------------------------\n");
		tcli_print(
			"ID: %d\t CropEn: %s\t StartX: %d\t StartY: %d\t Width: %u\t Height: %u\n",
			idx,
			ptVencProcInfo->stChnParam.stCropCfg.bEnable ? "Y" :
									     "N",
			ptVencProcInfo->stChnParam.stCropCfg.stRect.s32X,
			ptVencProcInfo->stChnParam.stCropCfg.stRect.s32Y,
			ptVencProcInfo->stChnParam.stCropCfg.stRect.u32Width,
			ptVencProcInfo->stChnParam.stCropCfg.stRect.u32Height);

		tcli_print(
			"-----ROI INFO-----------------------------------------------------\n");
		for (roiIdx = 0; roiIdx < 8; roiIdx++) {
			if (ptVencProcInfo->stRoiAttr[roiIdx].bEnable) {
				tcli_print(
					"ID: %d\t Index: %u\t bRoiEn: %s\t bAbsQp: %s\t Qp: %d",
					idx,
					ptVencProcInfo->stRoiAttr[roiIdx]
						.u32Index,
					ptVencProcInfo->stRoiAttr[roiIdx]
							.bEnable ?
						      "Y" :
						      "N",
					ptVencProcInfo->stRoiAttr[roiIdx].bAbsQp ?
						      "Y" :
						      "N",
					ptVencProcInfo->stRoiAttr[roiIdx].s32Qp);
				tcli_print(
					"\t Width: %u\t Height: %u\t StartX: %d\t StartY: %d\n",
					ptVencProcInfo->stRoiAttr[roiIdx]
						.stRect.u32Width,
					ptVencProcInfo->stRoiAttr[roiIdx]
						.stRect.u32Height,
					ptVencProcInfo->stRoiAttr[roiIdx]
						.stRect.s32X,
					ptVencProcInfo->stRoiAttr[roiIdx]
						.stRect.s32Y);
			}
		}

		tcli_print(
			"-----VENC PTS STATE------------------------------------------------\n");
		tcli_print("ID: %d\t RcvFirstFrmPts: %llu\t RcvFrmPts: %llu\n",
			   idx, 0LL, ptVencProcInfo->stFrame.u64PTS);

		tcli_print(
			"-----VENC CHN PERFORMANCE------------------------------------------------\n");
		tcli_print(
			"ID: %d\t InFPS: %u\t OutFPS: %u\t HwEncTime: %llu us\n\n",
			idx, ptVencProcInfo->stFPS.u32InFPS,
			ptVencProcInfo->stFPS.u32OutFPS,
			ptVencProcInfo->stFPS.u64EncHwTime);
	}
}

static int __cliVencInfoShow(int32_t argc, char *argv[])
{
	int idx = 0;

	if (argc == 1) {
		for (idx = 0; idx < VENC_MAX_CHN_NUM; idx++)
			cliVencInfoShow(idx);
		return 0;
	}

	if (argc == 2) {
		idx = atoi(argv[1]);
		if (idx < 0 || idx >= VENC_MAX_CHN_NUM) {
			tcli_print("usage:%s chn\n", argv[0]);
			tcli_print("chn:[0 ~ %d]\n", VENC_MAX_CHN_NUM - 1);
			return -1;
		}
		cliVencInfoShow(idx);
	}

	return 0;
}

void venc_set_handler_state(VENC_CHN VeChn, int state)
{
	__astVencCliDbgInfo[VeChn].s32handlerState = state;
}

void venc_handler_exit_retcode(VENC_CHN VeChn, int retcode)
{
	__astVencCliDbgInfo[VeChn].s32handerExitRetcode = retcode;
}

void venc_channel_run_status_addself(VENC_CHN VeChn, int index)
{
	if (index >= MAX_RUN_CNT) {
		return;
	}
	__astVencCliDbgInfo[VeChn].u32RunCnt[index]++;
}

static int cliShowVencDbgInfo(int32_t argc, char *argv[])
{
	int chnIdx = 0;

	if (argc != 2) {
		tcli_print("usage:%s chn\n", argv[0]);
		return 0;
	}

	chnIdx = atoi(argv[1]);
	if (chnIdx > VENC_MAX_CHN_NUM) {
		tcli_print("channle range [0:%d]\n", VENC_MAX_CHN_NUM);
		return -1;
	}

	tcli_print("chnIdx = %d\n", chnIdx);
	tcli_print("dump_yuv_cnt = %d\n",
		   __astVencCliDbgInfo[chnIdx].dump_yuv_cnt);
	tcli_print("s32handlerState = %d\n",
		   __astVencCliDbgInfo[chnIdx].s32handlerState);
	tcli_print("s32handerExitRet = %d\n",
		   __astVencCliDbgInfo[chnIdx].s32handerExitRetcode);
	for (int i = 0; i < MAX_RUN_CNT; i++) {
		tcli_print("RunCnt[%d]: %d\n", i,
			   __astVencCliDbgInfo[chnIdx].u32RunCnt[i]);
	}

	return 0;
}

void cviCliDumpSrcYuv(int chn, const VIDEO_FRAME_INFO_S *pstFrame)
{
	FILE *fp = NULL;
	const VIDEO_FRAME_S *pstVFrame;
	CVI_VOID *vir_addr;
	CVI_U32 u32LumaSize;
	CVI_U32 u32ChrmSize;
	CVI_U8 u8PlaneNum;
	char filename[64] = { 0 };

	if (__astVencCliDbgInfo[chn].dump_yuv_cnt <= 0) {
		return;
	}

	snprintf(filename, 64, "/tmp/venc_ch%d_src.yuv", chn);

	pstVFrame = &pstFrame->stVFrame;
	u32LumaSize = pstVFrame->u32Stride[0] * pstVFrame->u32Height;
	if ((pstVFrame->enPixelFormat == PIXEL_FORMAT_NV12) ||
	    (pstVFrame->enPixelFormat == PIXEL_FORMAT_NV21)) {
		u32ChrmSize = pstVFrame->u32Stride[1] * pstVFrame->u32Height;
		u8PlaneNum = 2;
	} else { //default yuv420
		u32ChrmSize =
			pstVFrame->u32Stride[1] * pstVFrame->u32Height >> 1;
		u8PlaneNum = 3;
	}

	//warning: u_stride must be half of the y_stride,not align by the half of pic_width

	if (!u32LumaSize || !u32ChrmSize) {
		CVI_VENC_ERR(
			"dump chn:%d,[%u,%u] u32LumaSize:%d,u32ChrmSize:%d\n",
			chn, pstVFrame->u32Width, pstVFrame->u32Height,
			u32LumaSize, u32ChrmSize);
		return;
	}

	fp = fopen(filename, "ab+");
	if (fp == NULL) {
		CVI_VENC_ERR("open %s failed\n", filename);
		return;
	}

	for (int i = 0; i < u8PlaneNum; i++) {
		CVI_U32 u32len = i == 0 ? u32LumaSize : u32ChrmSize;
		if (!pstVFrame->pu8VirAddr[i] &&
		    pstVFrame->u64PhyAddr[i]) { //bind mode
			vir_addr =
				CVI_SYS_Mmap(pstVFrame->u64PhyAddr[i], u32len);
			CVI_SYS_IonInvalidateCache(pstVFrame->u64PhyAddr[i],
						   vir_addr, u32len);
			fwrite(vir_addr, u32len, 1, fp);
			CVI_SYS_Munmap(vir_addr, u32len);
		} else {
			fwrite((void *)pstVFrame->pu8VirAddr[i], 1, u32len, fp);
		}
	}

	fflush(fp);
	fclose(fp);

	__astVencCliDbgInfo[chn].dump_yuv_cnt--;
}

CVI_S32 cviDumpVencBitstream(VENC_CHN VeChn, VENC_STREAM_S *pstStream,
			     PAYLOAD_TYPE_E enType)
{
	char filename[64] = { 0 };
	VENC_PACK_S *ppack;
	FILE *fp = NULL;

	if (!__astVencCliDbgInfo[VeChn].bDumpBsEnable) {
		return -1;
	}

	snprintf(filename, 64, "/tmp/chn%d.%s", VeChn,
		 enType == PT_H264   ? "264" :
		 (enType == PT_H265) ? "265" :
					     "mjpg");

	fp = fopen(filename, "ab+");
	if (fp == NULL) {
		CVI_VENC_ERR("open %s failed\n", filename);
		return -1;
	}

	for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
		ppack = &pstStream->pstPack[i];
		fwrite(ppack->pu8Addr + ppack->u32Offset,
		       ppack->u32Len - ppack->u32Offset, 1, fp);
	}

	fflush(fp);
	fclose(fp);

	return 0;
}

static int __cliDumpVencBs(int32_t argc, char *argv[])
{
	int VencChn = 0;

	if (argc != 3) {
		tcli_print("usage:dumpbitstream chn switch\n");
		tcli_print("chn:[0,%d]\n", VENC_MAX_CHN_NUM);
		tcli_print("siwtch:[0,1]\n");
		return -1;
	}

	VencChn = atoi(argv[1]);
	if (VencChn < 0 || VencChn >= VENC_MAX_CHN_NUM) {
		tcli_print("channel range[0,%d]\n", VENC_MAX_CHN_NUM);
		return -1;
	}
	__astVencCliDbgInfo[VencChn].bDumpBsEnable = atoi(argv[2]) ? 1 : 0;

	tcli_print("dumpbitstream chn:%d switch:%d\n", VencChn,
		   __astVencCliDbgInfo[VencChn].bDumpBsEnable);

	return 0;
}

static int __cliDumpVencSrcYUV(int32_t argc, char *argv[])
{
	int VencChn = 0;

	if (argc != 3) {
		tcli_print("usage:dumpyuv chn yuv_count\n");
		return -1;
	}

	VencChn = atoi(argv[1]);
	if (VencChn < 0 || VencChn >= VENC_MAX_CHN_NUM) {
		tcli_print("channel range[0,%d]\n", VENC_MAX_CHN_NUM);
		return -1;
	}
	__astVencCliDbgInfo[VencChn].dump_yuv_cnt = atoi(argv[2]);

	tcli_print("dumpyuv chn:%d cnt:%d\n", VencChn,
		   __astVencCliDbgInfo[VencChn].dump_yuv_cnt);

	return 0;
}

void cviGetMaxBitstreamSize(VENC_CHN VeChn, VENC_STREAM_S *pstStream)
{
	if (!pstStream || VeChn < 0 || VeChn >= VENC_MAX_CHN_NUM) {
		return;
	}

	for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
		VENC_PACK_S *ppack = &pstStream->pstPack[i];
		if (ppack &&
		    (ppack->u32Len > __astVencCliDbgInfo[VeChn].u32MaxBsSize)) {
			__astVencCliDbgInfo[VeChn].u32MaxBsSize = ppack->u32Len;
		}
	}
}

static int __cliGetmaxBsSize(int argc, char *argv[])
{
	int VencChn = 0;

	if (argc != 2) {
		tcli_print("usage:getmaxBsSize chn\n");
		return -1;
	}

	VencChn = atoi(argv[1]);
	if (VencChn < 0 || VencChn >= VENC_MAX_CHN_NUM) {
		tcli_print("channel range[0,%d]\n", VENC_MAX_CHN_NUM);
		return -1;
	}

	tcli_print("venc chn[%d]'s max bitstream size:%d\n", VencChn,
		   __astVencCliDbgInfo[VencChn].u32MaxBsSize);

	return 0;
}

static void cviDumpChnPram(int chnIdx)
{
	venc_chn_context *pChnHandle = handle->chn_handle[chnIdx];

	if (pChnHandle == NULL)
		return;

	tcli_print(
		"---------------------------------------------------------\n");
	tcli_print("VeChn:\t\t\t%d\n", pChnHandle->VeChn);

	tcli_print("enType:\t\t\t%d\n",
		   pChnHandle->pChnAttr->stVencAttr.enType);
	tcli_print("u32Profile:\t\t%d\n",
		   pChnHandle->pChnAttr->stVencAttr.u32Profile);
	tcli_print("u32BufSize:\t\t%d\n",
		   pChnHandle->pChnAttr->stVencAttr.u32BufSize);
	tcli_print("u32PicWidth:\t\t%d\n",
		   pChnHandle->pChnAttr->stVencAttr.u32PicWidth);
	tcli_print("u32PicHeight:\t\t%d\n",
		   pChnHandle->pChnAttr->stVencAttr.u32PicHeight);

	if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
		tcli_print("bSingleLumaBuf:\t\t%d\n",
			   pChnHandle->pChnAttr->stVencAttr.stAttrH264e
				   .bSingleLumaBuf);
		tcli_print("\n");
		tcli_print("enH264eVBSource:\t%d\n",
			   handle->ModParam.stH264eModParam.enH264eVBSource);
		tcli_print("bSingleEsBuf:\t\t%d\n",
			   handle->ModParam.stH264eModParam.bSingleEsBuf);
		tcli_print("u32SingleEsBufSize:\t%d\n",
			   handle->ModParam.stH264eModParam.u32SingleEsBufSize);
	} else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
		tcli_print("\n");
		tcli_print("enH264eVBSource:\t%d\n",
			   handle->ModParam.stH265eModParam.enH265eVBSource);
		tcli_print("bSingleEsBuf:\t\t%d\n",
			   handle->ModParam.stH265eModParam.bSingleEsBuf);
		tcli_print("u32SingleEsBufSize:\t%d\n",
			   handle->ModParam.stH265eModParam.u32SingleEsBufSize);
		tcli_print("enRefreshType:\t\t%d\n",
			   handle->ModParam.stH265eModParam.enRefreshType);
	} else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_JPEG) {
		tcli_print("\n");
		tcli_print("bSingleEsBuf:\t\t%d\n",
			   handle->ModParam.stJpegeModParam.bSingleEsBuf);
		tcli_print("u32SingleEsBufSize:\t%d\n",
			   handle->ModParam.stJpegeModParam.u32SingleEsBufSize);
		tcli_print("enJpegeFormat:\t\t%d\n",
			   handle->ModParam.stJpegeModParam.enJpegeFormat);
	}

	tcli_print("\n");
	tcli_print("enRcMode:\t\t%d\n",
		   pChnHandle->pChnAttr->stRcAttr.enRcMode);

	if (pChnHandle->pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_H264CBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Cbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH264Cbr.u32StatTime);
		tcli_print("u32BitRate:\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Cbr.u32BitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Cbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Cbr
				   .fr32DstFrameRate);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Cbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t%d\n",
			pChnHandle->rcParam.stParamH264Cbr.s32MaxReEncodeTimes);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H264VBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Vbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH264Vbr.u32StatTime);
		tcli_print(
			"u32MaxBitRate:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH264Vbr.u32MaxBitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Vbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264Vbr
				   .fr32DstFrameRate);
		tcli_print("s32ChangePos:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.s32ChangePos);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264Vbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t%d\n",
			pChnHandle->rcParam.stParamH264Vbr.s32MaxReEncodeTimes);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H264AVBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264AVbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH264Vbr.u32StatTime);
		tcli_print(
			"u32MaxBitRate:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH264AVbr.u32MaxBitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264AVbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264AVbr
				   .fr32DstFrameRate);
		tcli_print("s32ChangePos:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.s32ChangePos);
		tcli_print(
			"s32MinStillPercent:\t\t%d\n",
			pChnHandle->rcParam.stParamH264AVbr.s32MinStillPercent);
		tcli_print("u32MaxStillQP:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MaxStillQP);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t\t%d\n",
			pChnHandle->rcParam.stParamH264AVbr.s32MaxReEncodeTimes);
		tcli_print("u32MinQpDelta:\t\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr.u32MinQpDelta);
		tcli_print("u32MotionSensitivity:\t%d\n",
			   pChnHandle->rcParam.stParamH264AVbr
				   .u32MotionSensitivity);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H264FIXQP) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp.u32Gop);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp
				   .fr32DstFrameRate);
		tcli_print("u32IQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp.u32IQp);
		tcli_print("u32PQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp.u32PQp);
		tcli_print("u32BQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH264FixQp.u32BQp);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H265CBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Cbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH265Cbr.u32StatTime);
		tcli_print("u32BitRate:\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Cbr.u32BitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Cbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Cbr
				   .fr32DstFrameRate);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Cbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t%d\n",
			pChnHandle->rcParam.stParamH265Cbr.s32MaxReEncodeTimes);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H265VBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Vbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH265Vbr.u32StatTime);
		tcli_print(
			"u32MaxBitRate:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH265Vbr.u32MaxBitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Vbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265Vbr
				   .fr32DstFrameRate);
		tcli_print("s32ChangePos:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.s32ChangePos);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265Vbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t%d\n",
			pChnHandle->rcParam.stParamH265Vbr.s32MaxReEncodeTimes);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H265AVBR) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265AVbr.u32Gop);
		tcli_print(
			"u32StatTime:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH265AVbr.u32StatTime);
		tcli_print(
			"u32MaxBitRate:\t\t%d\n",
			pChnHandle->pChnAttr->stRcAttr.stH265AVbr.u32MaxBitRate);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265AVbr
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265AVbr
				   .fr32DstFrameRate);
		tcli_print("s32ChangePos:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.s32ChangePos);
		tcli_print(
			"s32MinStillPercent:\t\t%d\n",
			pChnHandle->rcParam.stParamH265AVbr.s32MinStillPercent);
		tcli_print("u32MaxStillQP:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MaxStillQP);
		tcli_print("u32MaxIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MaxIQp);
		tcli_print("u32MinIQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MinIQp);
		tcli_print("u32MaxQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MaxQp);
		tcli_print("u32MinQp:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MinQp);
		tcli_print("u32MaxIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MaxIprop);
		tcli_print("u32MinIprop:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MinIprop);
		tcli_print(
			"s32MaxReEncodeTimes:\t\t%d\n",
			pChnHandle->rcParam.stParamH265AVbr.s32MaxReEncodeTimes);
		tcli_print("u32MinQpDelta:\t\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr.u32MinQpDelta);
		tcli_print("u32MotionSensitivity:\t%d\n",
			   pChnHandle->rcParam.stParamH265AVbr
				   .u32MotionSensitivity);
	} else if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
		   VENC_RC_MODE_H265FIXQP) {
		tcli_print("u32Gop:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp.u32Gop);
		tcli_print("u32SrcFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp
				   .u32SrcFrameRate);
		tcli_print("fr32DstFrameRate:\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp
				   .fr32DstFrameRate);
		tcli_print("u32IQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp.u32IQp);
		tcli_print("u32PQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp.u32PQp);
		tcli_print("u32BQp:\t\t\t%d\n",
			   pChnHandle->pChnAttr->stRcAttr.stH265FixQp.u32BQp);
	}

	tcli_print("s32FirstFrameStartQp:\t%d\n",
		   pChnHandle->rcParam.s32FirstFrameStartQp);
	tcli_print("s32InitialDelay:\t%d\n",
		   pChnHandle->rcParam.s32InitialDelay);
	tcli_print("u32RowQpDelta:\t\t%d\n", pChnHandle->rcParam.u32RowQpDelta);

	tcli_print("\n");
	tcli_print("enGopMode:\t\t%d\n",
		   pChnHandle->pChnAttr->stGopAttr.enGopMode);

	if (pChnHandle->pChnAttr->stGopAttr.enGopMode == VENC_GOPMODE_NORMALP) {
		tcli_print(
			"s32IPQpDelta:\t\t%d\n",
			pChnHandle->pChnAttr->stGopAttr.stNormalP.s32IPQpDelta);
	} else if (pChnHandle->pChnAttr->stGopAttr.enGopMode ==
		   VENC_GOPMODE_SMARTP) {
		tcli_print(
			"s32IPQpDelta:\t\t%d\n",
			pChnHandle->pChnAttr->stGopAttr.stSmartP.s32BgQpDelta);
		tcli_print(
			"u32BgInterval:\t\t%d\n",
			pChnHandle->pChnAttr->stGopAttr.stSmartP.u32BgInterval);
		tcli_print(
			"s32ViQpDelta:\t\t%d\n",
			pChnHandle->pChnAttr->stGopAttr.stSmartP.s32ViQpDelta);
	}

	tcli_print("\n");
	tcli_print("currPTS:\t\t%llu\n", pChnHandle->pChnVars->currPTS);

	tcli_print("\n");

	if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
		tcli_print("chroma_qp_index_offset:\t%d\n",
			   pChnHandle->h264Trans.chroma_qp_index_offset);
	} else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
		tcli_print("cb_qp_offset:\t\t%d\n",
			   pChnHandle->h265Trans.cb_qp_offset);
		tcli_print("cr_qp_offset:\t\t%d\n",
			   pChnHandle->h265Trans.cr_qp_offset);
	}

	tcli_print("\n");
	tcli_print("bFrmLostOpen:\t\t%d\n", pChnHandle->frameLost.bFrmLostOpen);
	tcli_print("enFrmLostMode:\t\t%d\n",
		   pChnHandle->frameLost.enFrmLostMode);
	tcli_print("u32EncFrmGaps:\t\t%d\n",
		   pChnHandle->frameLost.u32EncFrmGaps);
	tcli_print("u32FrmLostBpsThr:\t%d\n",
		   pChnHandle->frameLost.u32FrmLostBpsThr);

	venc_chn_vars *pChnVars = pChnHandle->pChnVars;

	if (pChnVars) {
		tcli_print("venc_chn_vars info:\n");
		tcli_print("u64TimeOfSendFrame:%llu\n",
			   pChnVars->u64TimeOfSendFrame);
		tcli_print("u64LastGetStreamTimeStamp:%llu\n",
			   pChnVars->u64LastGetStreamTimeStamp);
		tcli_print("u64LastSendFrameTimeStamp:%llu\n",
			   pChnVars->u64LastSendFrameTimeStamp);
		tcli_print("currPTS:%llu\n", pChnVars->currPTS);
		tcli_print("totalTime:%llu\n", pChnVars->totalTime);
		tcli_print("frameIdx:%d\n", pChnVars->frameIdx);
		tcli_print("s32RecvPicNum:%d\n", pChnVars->s32RecvPicNum);
		tcli_print("bFrcEnable:%d\n", pChnVars->frc.bFrcEnable);
		tcli_print("u32Seq:%u\n", pChnVars->stStream.u32Seq);
		tcli_print("chnState:%d\n", pChnVars->chnState);
		tcli_print("bAttrChang:%d\n", pChnVars->bAttrChange);
		tcli_print("bHasVbPool:%d\n", pChnVars->bHasVbPool);
		tcli_print("FrmNum:%u\n", pChnVars->FrmNum);
		tcli_print("u32SendFrameCnt:%d\n", pChnVars->u32SendFrameCnt);
		tcli_print("u32GetStreamCnt:%u\n", pChnVars->u32GetStreamCnt);
		tcli_print("s32BindModeGetStreamRet:%d\n",
			   pChnVars->s32BindModeGetStreamRet);

		tcli_print("stChnParam_bColor2Grey:%d\n",
			   pChnVars->stChnParam.bColor2Grey);
		tcli_print("stChnParam_u32Priority:%d\n",
			   pChnVars->stChnParam.u32Priority);
		tcli_print("stChnParam_u32MaxStrmCnt:%d\n",
			   pChnVars->stChnParam.u32MaxStrmCnt);
		tcli_print("stChnParam_u32Priority:%d\n",
			   pChnVars->stChnParam.u32Priority);
		tcli_print("stChnParam_u32PollWakeUpFrmCnt:%d\n",
			   pChnVars->stChnParam.u32PollWakeUpFrmCnt);
		tcli_print("stChnParam_s32SrcFrmRate:%d\n",
			   pChnVars->stChnParam.stFrameRate.s32SrcFrmRate);
		tcli_print("s32DstFrmRate:%d\n",
			   pChnVars->stChnParam.stFrameRate.s32DstFrmRate);
		tcli_print("stCropCfg_bEnable:%d\n",
			   pChnVars->stChnParam.stCropCfg.bEnable);
		tcli_print("stCropCfg:x(%d) y(%d) width(%d) height(%d)\n",
			   pChnVars->stChnParam.stCropCfg.stRect.s32X,
			   pChnVars->stChnParam.stCropCfg.stRect.s32Y,
			   pChnVars->stChnParam.stCropCfg.stRect.u32Width,
			   pChnVars->stChnParam.stCropCfg.stRect.u32Height);

		tcli_print("chnStatus_u32LeftPics:%d\n",
			   pChnVars->chnStatus.u32LeftPics);
		tcli_print("chnStatus_u32LeftStreamBytes:%d\n",
			   pChnVars->chnStatus.u32LeftStreamBytes);
		tcli_print("chnStatus_u32LeftStreamFrames:%d\n",
			   pChnVars->chnStatus.u32LeftStreamFrames);
		tcli_print("chnStatus_u32CurPacks:%d\n",
			   pChnVars->chnStatus.u32CurPacks);
		tcli_print("chnStatus_u32LeftRecvPics:%d\n",
			   pChnVars->chnStatus.u32LeftRecvPics);
		tcli_print("chnStatus_u32LeftEncPics:%d\n",
			   pChnVars->chnStatus.u32LeftEncPics);
		tcli_print("chnStatus_bJpegSnapEnd:%d\n",
			   pChnVars->chnStatus.bJpegSnapEnd);
		tcli_print("u32Stride [0]:%d [1]:%d [2]:%d\n",
			   pChnVars->u32Stride[0], pChnVars->u32Stride[1],
			   pChnVars->u32Stride[2]);
	}

	venc_enc_ctx *pEncCtx;

	pEncCtx = &pChnHandle->encCtx;
	PAYLOAD_TYPE_E enType = pChnHandle->pChnAttr->stVencAttr.enType;

	tcli_print(
		"-------------------------ipinfo---------------------------\n");
	if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
		pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_CHN_INFO, NULL);
	} else if (pEncCtx->base.ioctl &&
		   (enType == PT_JPEG || enType == PT_MJPEG)) {
		pEncCtx->base.ioctl(pEncCtx, CVI_JPEG_OP_SHOW_CHN_INFO, NULL);
	}
	tcli_print(
		"---------------------------------------------------------\n");
}

static int showVencParam(int32_t argc, char *argv[])
{
	int chnIdx = 0;

	if (argc != 2) {
		for (chnIdx = 0; chnIdx < VENC_MAX_CHN_NUM; chnIdx++)
			cviDumpChnPram(chnIdx);

		return 0;
	}

	chnIdx = atoi(argv[1]);
	tcli_print("show chnIdx = %d\n", chnIdx);
	cviDumpChnPram(chnIdx);

	return 0;
}

static int setIpQpDeltaParam(int32_t argc, char *argv[])
{
	int VencChn = 0;
	int delta = 0;
	int s32Ret = 0;

	if (argc != 3) {
		tcli_print("usage:setipqpdelta chn delta\n");

		return -1;
	}

	VencChn = atoi(argv[1]);
	delta = atoi(argv[2]);

	VENC_CHN_ATTR_S stVencChnAttr;

	s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		tcli_print("CVI_VENC_GetChnAttr, VencChn = %d, s32Ret = %d\n",
			   VencChn, s32Ret);
		return -1;
	}

	stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = delta;

	if (CVI_VENC_SetChnAttr(VencChn, &stVencChnAttr)) {
		tcli_print("CVI_VENC_SetChnAttr failed\n");
		return -1;
	}

	tcli_print("set suc.\n");

	return 0;
}

static int getIpQpDeltaParam(int32_t argc, char *argv[])
{
	int VencChn = 0;
	int delta = 0;
	int s32Ret = 0;

	if (argc != 2) {
		tcli_print("usage:getipqpdelta chn\n");

		return -1;
	}

	VencChn = atoi(argv[1]);

	VENC_CHN_ATTR_S stVencChnAttr;

	s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		tcli_print("CVI_VENC_GetChnAttr, VencChn = %d, s32Ret = %d\n",
			   VencChn, s32Ret);
		return -1;
	}

	delta = stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta;

	tcli_print("enGopMode:%d,delta = %d\n",
		   stVencChnAttr.stGopAttr.enGopMode, delta);

	return 0;
}

static TELNET_CLI_S_COMMAND venc_cmd_list[] = {
	DECLARE_CLI_CMD_MACRO(channelinfo, NULL, showVencParam,
			      show venc channel info, 0),
	DECLARE_CLI_CMD_MACRO(getipqpdelta, NULL, getIpQpDeltaParam,
			      get ipqpdelta, 0),
	DECLARE_CLI_CMD_MACRO(setipqpdelta, NULL, setIpQpDeltaParam,
			      set ipqpdelta, 0),
	DECLARE_CLI_CMD_MACRO(dumpyuv, NULL, __cliDumpVencSrcYUV, dump venc yuv,
			      0),
	DECLARE_CLI_CMD_MACRO(dbginfo, NULL, cliShowVencDbgInfo,
			      show chn debug info, 0),
	DECLARE_CLI_CMD_MACRO(dumpbitstream, NULL, __cliDumpVencBs,
			      dump venc bitstream, 0),
	DECLARE_CLI_CMD_MACRO(vencinfo, NULL, __cliVencInfoShow,
			      dump venc bitstream, 0),
	DECLARE_CLI_CMD_MACRO(getmaxBsSize, NULL, __cliGetmaxBsSize,
			      get max bs size, 0),
	DECLARE_CLI_CMD_MACRO_END()
};

static TELNET_CLI_S_COMMAND vcodec_cli_cmd_list[] = {
	DECLARE_CLI_CMD_MACRO(venc, venc_cmd_list, NULL, venc cmd list, 0),
	DECLARE_CLI_CMD_MACRO_END()
};

void venc_register_cmd(void)
{
	int enRet = 0;

	pthread_mutex_lock(&vencCliMutex);

	if (bVencCliReg) {
		pthread_mutex_unlock(&vencCliMutex);
		return;
	}
	memset(__astVencCliDbgInfo, 0, sizeof(__astVencCliDbgInfo));

	bVencCliReg = 1;
	enRet = RegisterCliCommand((void *)vcodec_cli_cmd_list);

	if (enRet) {
		printf("<%s,%d>:cmd register failed,enRet=%#x\r\n", __func__,
		       __LINE__, enRet);
		pthread_mutex_unlock(&vencCliMutex);
		return;
	}

	pthread_mutex_unlock(&vencCliMutex);
}

#endif
