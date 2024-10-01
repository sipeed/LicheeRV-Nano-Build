#ifdef CLI_DEBUG_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tcli.h"
#include "../vdi/vdi.h"
#include "../vdi/vdi_osal.h"
#include "../vpuapi/vpuapifunc.h"
#include "../sample/cvitest_internal.h"

#ifndef UNUSED
#define UNUSED(x) //((void)(x))
#endif

static int bVcodecCliReg;
static pthread_mutex_t vcodecCliMutex = PTHREAD_MUTEX_INITIALIZER;

static int showVpuReg(int32_t argc, char *argv[])
{
	int coreIdx = 0;
	int idx = 0;

	if (argc != 2) {
		tcli_print("usage:%s num\n", argv[0]);
		return -1;
	}

	coreIdx = atoi(argv[1]) ? 1 : 0;
	tcli_print("show coreid = %d regs from hw:\n", coreIdx);
	tcli_print("0x%04X  %llX\n", idx, VpuReadReg(coreIdx, idx + 0));
	tcli_print("0x%04X  %llX\n", idx + 0x4, VpuReadReg(coreIdx, idx + 0x4));
	tcli_print("0x%04X  %llX\n", idx + 0x8, VpuReadReg(coreIdx, idx + 0x8));
	tcli_print("0x%04X  %llX\n", idx + 0xC, VpuReadReg(coreIdx, idx + 0xC));
	tcli_print("0x%04X  %llX\n", idx + 0x010,
		   VpuReadReg(coreIdx, idx + 0x010));
	tcli_print("0x%04X  %llX\n", idx + 0x014,
		   VpuReadReg(coreIdx, idx + 0x014));
	tcli_print("0x%04X  %llX\n", idx + 0x018,
		   VpuReadReg(coreIdx, idx + 0x018));
	tcli_print("0x%04X  %llX\n", idx + 0x024,
		   VpuReadReg(coreIdx, idx + 0x024));
	tcli_print("0x%04X  %llX\n", idx + 0x034,
		   VpuReadReg(coreIdx, idx + 0x034));

	for (idx = 0x100; idx <= 0x200; idx += 0x10) {
		tcli_print("0x%04X  %011llX  %011llX  %011llX\t  %011llX\n",
			   idx, VpuReadReg(coreIdx, idx),
			   VpuReadReg(coreIdx, idx + 0x4),
			   VpuReadReg(coreIdx, idx + 0x8),
			   VpuReadReg(coreIdx, idx + 0xC));
	}

	return 0;
}

static int vcodecModMaskDebug(int32_t argc, char *argv[])
{
	int dbg_mask_tmp = 0;

	if (argc != 2) {
		tcli_print("usage:%s mask_value\n", argv[0]);
		tcli_print("current dbg_mask=%#x\n", dbg_mask);
		return -1;
	}

	dbg_mask_tmp = (int)atoi(argv[1]);
	dbg_mask = dbg_mask_tmp;
	tcli_print("set suc,current dbg_mask=%#x\n", dbg_mask);

	return 0;
}

void showCodecInstPoolInfo(CodecInst *pCodecInst)
{
	if (pCodecInst && pCodecInst->inUse) {
		tcli_print("================start=================\n");
		tcli_print("inUse:%d\n", pCodecInst->inUse);
		tcli_print("instIndex:%d\n", pCodecInst->instIndex);
		tcli_print("coreIdx:%d\n", pCodecInst->coreIdx);
		tcli_print("codecMode:%d\n", pCodecInst->codecMode);
		tcli_print("codecModeAux:%d\n", pCodecInst->codecModeAux);
		tcli_print("productId:%d\n", pCodecInst->productId);
#ifdef ENABLE_CNM_DEBUG_MSG
		tcli_print("loggingEnable:%d\n", pCodecInst->loggingEnable);
#endif
		tcli_print("isDecoder:%s\n",
			   pCodecInst->isDecoder ? "decode" : "encode");
		tcli_print("state:%d\n", pCodecInst->state);

		if (!pCodecInst->isDecoder) { //enc
			EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;

			tcli_print("encInfo:\n");
			tcli_print(" bitRate:%d\n",
				   pEncInfo->openParam.bitRate);
			tcli_print(" bitstreamBufferSize:%d\n",
				   pEncInfo->openParam.bitstreamBufferSize);
			tcli_print(" picWidth:%d\n",
				   pEncInfo->openParam.picWidth);
			tcli_print(" picHeight:%d\n",
				   pEncInfo->openParam.picHeight);
			tcli_print(" frameSkipDisable:%d\n",
				   pEncInfo->openParam.frameSkipDisable);
			tcli_print(" ringBufferEnable:%d\n",
				   pEncInfo->openParam.ringBufferEnable);
			tcli_print(" frameRateInfo:%d\n",
				   pEncInfo->openParam.frameRateInfo);
			tcli_print(" gopSize:%d\n",
				   pEncInfo->openParam.gopSize);
			tcli_print(" initialDelay:%d\n",
				   pEncInfo->openParam.initialDelay);
			tcli_print(" changePos:%d\n",
				   pEncInfo->openParam.changePos);

			tcli_print(" minFrameBufferCount:%d\n",
				   pEncInfo->initialInfo.minFrameBufferCount);
			tcli_print(" minSrcFrameCount:%d\n",
				   pEncInfo->initialInfo.minSrcFrameCount);

			tcli_print(" streamBufSize:%d\n",
				   pEncInfo->streamBufSize);
			tcli_print(" linear2TiledEnable:%d\n",
				   pEncInfo->linear2TiledEnable);
			tcli_print(" linear2TiledMode:%d\n",
				   pEncInfo->linear2TiledMode);
			tcli_print(" mapType:%d\n", pEncInfo->mapType);
			tcli_print(" stride:%d\n",
				   pEncInfo->frameBufPool[0].stride);
			tcli_print(" myIndex:%d\n",
				   pEncInfo->frameBufPool[0].myIndex);
			tcli_print(" width:%d\n",
				   pEncInfo->frameBufPool[0].width);
			tcli_print(" height:%d\n",
				   pEncInfo->frameBufPool[0].height);
			tcli_print(" size:%d\n",
				   pEncInfo->frameBufPool[0].size);
			tcli_print(" format:%d\n",
				   pEncInfo->frameBufPool[0].format);
			tcli_print(" vbFrame_size:%d\n",
				   pEncInfo->vbFrame.size);
			tcli_print(" vbPPU_size:%d\n", pEncInfo->vbPPU.size);
			tcli_print(" streamBufSize:%d\n",
				   pEncInfo->frameAllocExt);
			tcli_print(" ppuAllocExt:%d\n", pEncInfo->ppuAllocExt);
			tcli_print(" numFrameBuffers:%d\n",
				   pEncInfo->numFrameBuffers);
			tcli_print(" ActivePPSIdx:%d\n",
				   pEncInfo->ActivePPSIdx);
			tcli_print(" frameIdx:%d\n", pEncInfo->frameIdx);

			if (pCodecInst->coreIdx) {
				int ActivePPSIdx = pEncInfo->ActivePPSIdx;
				EncAvcParam *pAvcParam =
					&pEncInfo->openParam.EncStdParam
						 .avcParam;
				AvcPpsParam *ActivePPS =
					&pAvcParam->ppsParam[ActivePPSIdx];

				tcli_print(" entropyCodingMode:%d\n",
					   ActivePPS->entropyCodingMode);
				tcli_print(" transform8x8Mode:%d\n",
					   ActivePPS->transform8x8Mode);
				tcli_print(" profile:%d\n", pAvcParam->profile);
				tcli_print(" fieldFlag:%d\n",
					   pAvcParam->fieldFlag);
				tcli_print(" chromaFormat400:%d\n",
					   pAvcParam->chromaFormat400);
			}

			tcli_print(" numFrameBuffers:%d\n",
				   pEncInfo->numFrameBuffers);
			tcli_print(" stride:%d\n", pEncInfo->stride);
			tcli_print(" frameBufferHeight:%d\n",
				   pEncInfo->frameBufferHeight);
			tcli_print(" rotationEnable:%d\n",
				   pEncInfo->rotationEnable);
			tcli_print(" mirrorEnable:%d\n",
				   pEncInfo->mirrorEnable);
			tcli_print(" mirrorDirection:%d\n",
				   pEncInfo->mirrorDirection);
			tcli_print(" rotationAngle:%d\n",
				   pEncInfo->rotationAngle);
			tcli_print(" initialInfoObtained:%d\n",
				   pEncInfo->initialInfoObtained);
			tcli_print(" ringBufferEnable:%d\n",
				   pEncInfo->ringBufferEnable);
			tcli_print(" mirrorEnable:%d\n",
				   pEncInfo->mirrorEnable);
			tcli_print(" encoded_frames_in_gop:%d\n",
				   pEncInfo->encoded_frames_in_gop);

			tcli_print(" lineBufIntEn:%d\n",
				   pEncInfo->lineBufIntEn);
			tcli_print(" bEsBufQueueEn:%d\n",
				   pEncInfo->bEsBufQueueEn);
			tcli_print(" vbWork_size:%d\n", pEncInfo->vbWork.size);
			tcli_print(" vbScratch_size:%d\n",
				   pEncInfo->vbScratch.size);
			tcli_print(" vbTemp_size:%d\n", pEncInfo->vbTemp.size);
			tcli_print(" vbMV_size:%d\n", pEncInfo->vbMV.size);
			tcli_print(" vbFbcYTbl_size:%d\n",
				   pEncInfo->vbFbcYTbl.size);
			tcli_print(" vbFbcCTbl_size:%d\n",
				   pEncInfo->vbFbcCTbl.size);
			tcli_print(" vbSubSamBuf_size:%d\n",
				   pEncInfo->vbSubSamBuf.size);
			tcli_print(" errorReasonCode:%d\n",
				   pEncInfo->errorReasonCode);
			tcli_print(" curPTS:%llu\n", pEncInfo->curPTS);
			tcli_print(" instanceQueueCount:%d\n",
				   pEncInfo->instanceQueueCount);
#ifdef SUPPORT_980_ROI_RC_LIB
			tcli_print(" gop_size:%d\n", pEncInfo->gop_size);
			tcli_print(" max_latency_pictures:%d\n",
				   pEncInfo->max_latency_pictures);
#endif
			tcli_print(" force_as_long_term_ref:%d\n",
				   pEncInfo->force_as_long_term_ref);
			tcli_print(" pic_ctu_avg_qp:%d\n",
				   pEncInfo->pic_ctu_avg_qp);
		} else { //dec
			DecInfo *pDecInfo = &pCodecInst->CodecInfo->decInfo;

			tcli_print("decInfo:\n");
			tcli_print(" bitstreamBufferSize:%d\n",
				   pDecInfo->openParam.bitstreamBufferSize);
			tcli_print(" streamBufSize:%d\n",
				   pDecInfo->streamBufSize);
			tcli_print(" numFbsForDecoding:%d\n",
				   pDecInfo->numFbsForDecoding);
			tcli_print(" wtlEnable:%d\n", pDecInfo->wtlEnable);
			tcli_print(" picWidth:%d\n",
				   pDecInfo->initialInfo.picWidth);
			tcli_print(" picHeight:%d\n",
				   pDecInfo->initialInfo.picHeight);
			tcli_print(" wtlEnable:%d\n", pDecInfo->wtlEnable);
			tcli_print(" frameBufPool_size:%d\n",
				   pDecInfo->frameBufPool[0].size);
			tcli_print(" frameBufPool_width:%d\n",
				   pDecInfo->frameBufPool[0].width);
			tcli_print(" frameBufPool_height:%d\n",
				   pDecInfo->frameBufPool[0].height);
			tcli_print(" frameBufPool_format:%d\n",
				   pDecInfo->frameBufPool[0].format);
		}

		tcli_print("rcInfo begin:\n");
		tcli_print(" rcEnable:%d\n", pCodecInst->rcInfo.rcEnable);
		tcli_print(" targetBitrate:%d\n",
			   pCodecInst->rcInfo.targetBitrate);
		tcli_print(" picAvgBit:%d\n", pCodecInst->rcInfo.picAvgBit);
		tcli_print(" maximumBitrate:%d\n",
			   pCodecInst->rcInfo.maximumBitrate);
		tcli_print(" bitrateBuffer:%d\n",
			   pCodecInst->rcInfo.bitrateBuffer);
		tcli_print(" frameSkipBufThr:%d\n",
			   pCodecInst->rcInfo.frameSkipBufThr);
		tcli_print(" frameSkipCnt:%d\n",
			   pCodecInst->rcInfo.frameSkipCnt);
		tcli_print(" contiSkipNum:%d\n",
			   pCodecInst->rcInfo.contiSkipNum);
		tcli_print(" convergStateBufThr:%d\n",
			   pCodecInst->rcInfo.convergStateBufThr);
		tcli_print(" ConvergenceState:%d\n",
			   pCodecInst->rcInfo.ConvergenceState);
		tcli_print(" codec:%d\n", pCodecInst->rcInfo.codec);
		tcli_print(" rcMode:%d\n", pCodecInst->rcInfo.rcMode);
		tcli_print(" picIMinQp:%d\n", pCodecInst->rcInfo.picIMinQp);
		tcli_print(" picIMaxQp:%d\n", pCodecInst->rcInfo.picIMaxQp);
		tcli_print(" picPMinQp:%d\n", pCodecInst->rcInfo.picPMinQp);
		tcli_print(" picPMaxQp:%d\n", pCodecInst->rcInfo.picPMaxQp);

		tcli_print(" picMinMaxQpClipRange:%d\n",
			   pCodecInst->rcInfo.picMinMaxQpClipRange);
		tcli_print(" lastPicQp:%d\n", pCodecInst->rcInfo.lastPicQp);
		tcli_print(" rcState:%d\n", pCodecInst->rcInfo.rcState);
		tcli_print(" framerate:%d\n", pCodecInst->rcInfo.framerate);
		tcli_print(" maxIPicBit:%d\n", pCodecInst->rcInfo.maxIPicBit);
		tcli_print("cviApiMode:%d\n", pCodecInst->cviApiMode);
		tcli_print("u64StartTime:%d\n", pCodecInst->u64StartTime);
		tcli_print("u64EndTime:%d\n", pCodecInst->u64EndTime);
		tcli_print("================end=================\n");
		tcli_print("================memory info=================\n");
		uint32_t total_size = 0;
		vpu_buffer_t vbu_comm_bf;

		vdi_get_common_memory(pCodecInst->coreIdx, &vbu_comm_bf);
		tcli_print("dts common_size:%d\n", vbu_comm_bf.size);
		if (!pCodecInst->isDecoder) {
			tcli_print("ion bitstreamBufferSize:%d\n",
				   pCodecInst->CodecInfo->encInfo.openParam
					   .bitstreamBufferSize);

			tcli_print("ion work_buf_size:%d\n",
				   pCodecInst->CodecInfo->encInfo.vbWork.size);
			tcli_print("TestEncConfig_size:%d\n",
				   sizeof(TestEncConfig));
			tcli_print("ion reconFBsize:%d\n",
				   pCodecInst->CodecInfo->encInfo
					   .frameBufPool[0]
					   .size);
			tcli_print(
				"numFrameBuffers:%d\n",
				pCodecInst->CodecInfo->encInfo.numFrameBuffers);
			tcli_print(
				"ion vbScratch_size:%d\n",
				pCodecInst->CodecInfo->encInfo.vbScratch.size);
			tcli_print("sram vbTemp_size:%d\n",
				   pCodecInst->CodecInfo->encInfo.vbTemp.size);
			tcli_print("ion vbMV_size:%d\n",
				   pCodecInst->CodecInfo->encInfo.vbMV.size);
			tcli_print(
				"ion vbFbcYTbl_size:%d\n",
				pCodecInst->CodecInfo->encInfo.vbFbcYTbl.size);
			tcli_print(
				"ion vbFbcCTbl_size:%d\n",
				pCodecInst->CodecInfo->encInfo.vbFbcCTbl.size);
			tcli_print(
				"ion vbSubSamBuf_size:%d\n",
				pCodecInst->CodecInfo->encInfo.vbSubSamBuf.size);
			total_size =
				pCodecInst->CodecInfo->encInfo.openParam
					.bitstreamBufferSize +
				pCodecInst->CodecInfo->encInfo.vbWork.size +
				sizeof(TestEncConfig) +
				pCodecInst->CodecInfo->encInfo.numFrameBuffers *
					pCodecInst->CodecInfo->encInfo
						.frameBufPool[0]
						.size +
				pCodecInst->CodecInfo->encInfo.vbScratch.size
				// + pCodecInst->CodecInfo->encInfo.vbTemp.size
				+
				pCodecInst->CodecInfo->encInfo.vbFbcYTbl.size +
				pCodecInst->CodecInfo->encInfo.vbFbcCTbl.size +
				pCodecInst->CodecInfo->encInfo.vbSubSamBuf.size;
			tcli_print(
				"currnet enc instance's total_size:%d (%d KB)\n",
				total_size, total_size / 1000);
		} else {
			DecInfo *pDecInfo = &pCodecInst->CodecInfo->decInfo;

			tcli_print("ion bitstreamBufferSize:%d\n",
				   pDecInfo->openParam.bitstreamBufferSize);
			tcli_print("work_buf_size:%d\n", pDecInfo->vbWork.size);
			tcli_print("DecInfo_size:%d\n", sizeof(DecInfo));
			tcli_print("FrameBuffer_size:%d\n",
				   pDecInfo->initialInfo.picWidth *
					   pDecInfo->initialInfo.picHeight *
					   pDecInfo->numFbsForDecoding);

			total_size = pDecInfo->openParam.bitstreamBufferSize +
				     pDecInfo->vbWork.size + sizeof(DecInfo) +
				     pDecInfo->initialInfo.picWidth *
					     pDecInfo->initialInfo.picHeight *
					     pDecInfo->numFbsForDecoding;
			tcli_print(
				"currnet dec instance's total_size:%d (%d KB)\n",
				total_size, total_size / 1000);
		}
	}
}

static int vcodecH26xInstaccePoolInfo(int32_t argc, char *argv[])
{
	int coreIdx = 0;

	if (argc != 2) {
		tcli_print("usage:%s idx\n", argv[0]);
		tcli_print("0: 265,1: 264", argv[0]);
		return -1;
	}

	coreIdx = (int)atoi(argv[1]);

	if (coreIdx > 1 || coreIdx < 0) {
		tcli_print("0: 265,1: 264", argv[0]);
		return -1;
	}

	vdi_show_vdi_info(coreIdx);

	int i;
	vpu_instance_pool_t *vip;
	CodecInst *pCodecInst;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);

	if (!vip) {
		tcli_print("no resource in vdi instance pool\n");
		return -1;
	}

	tcli_print("show inst pool info:\n");

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pCodecInst = (CodecInst *)vip->codecInstPool[i];

		showCodecInstPoolInfo(pCodecInst);
	}

	return 0;
}

static uint32_t getVcodecMemoryInfo(int coreIdx)
{
	int i;
	vpu_instance_pool_t *vip;
	CodecInst *pCodecInst;
	uint32_t total_size = 0;
	uint32_t all_total_size = 0;
	vpu_buffer_t vbu_comm_bf;
	int bAddStreamSize = 0;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);

	if (!vip) {
		tcli_print("no resource in vdi instance pool\n");
		return -1;
	}

	tcli_print("key infomation:\n");
	tcli_print("is_single_es_buf:%d\n", vdi_get_is_single_es_buf(coreIdx));

	vdi_get_common_memory(coreIdx, &vbu_comm_bf);
	tcli_print("dts common_size:%d\n", vbu_comm_bf.size);
	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pCodecInst = (CodecInst *)vip->codecInstPool[i];

		if (pCodecInst && pCodecInst->inUse) {
			tcli_print("================%s=================\n",
				   pCodecInst->isDecoder ? "decode" : "encode");

			if (!pCodecInst->isDecoder) {
				EncInfo *pEncInfo =
					&pCodecInst->CodecInfo->encInfo;

				tcli_print(
					"ion bitstreamBufferSize:%d\n",
					pEncInfo->openParam.bitstreamBufferSize);

				tcli_print("ion work_buf_size:%d\n",
					   pEncInfo->vbWork.size);
				tcli_print("mem stTestEncoder_size:%d\n",
					   sizeof(stTestEncoder));
				tcli_print("ion reconFBsize:%d\n",
					   pEncInfo->frameBufPool[0].size);
				tcli_print("numFrameBuffers:%d\n",
					   pEncInfo->numFrameBuffers);
				tcli_print("ion vbScratch_size:%d\n",
					   pEncInfo->vbScratch.size);
				tcli_print("sram vbTemp_size:%d\n",
					   pEncInfo->vbTemp.size);
				tcli_print("ion vbMV_size:%d\n",
					   pEncInfo->vbMV.size);
				tcli_print("ion vbFbcYTbl_size:%d\n",
					   pEncInfo->vbFbcYTbl.size);
				tcli_print("ion vbFbcCTbl_size:%d\n",
					   pEncInfo->vbFbcCTbl.size);
				tcli_print("ion vbSubSamBuf_size:%d\n",
					   pEncInfo->vbSubSamBuf.size);
				total_size =
					pEncInfo->vbWork.size +
					sizeof(stTestEncoder) +
					pEncInfo->numFrameBuffers *
						pEncInfo->frameBufPool[0].size +
					pEncInfo->vbScratch.size +
					pEncInfo->vbMV.size +
					pEncInfo->vbFbcYTbl.size +
					pEncInfo->vbFbcCTbl.size +
					pEncInfo->vbSubSamBuf.size;
				if (vdi_get_is_single_es_buf(coreIdx)) {
					if (!bAddStreamSize) {
						total_size +=
							pEncInfo->openParam
								.bitstreamBufferSize;
						bAddStreamSize = 1;
					}
				} else {
					total_size +=
						pEncInfo->openParam
							.bitstreamBufferSize;
				}

				tcli_print(
					"currnet enc instance's total_size:%d (%d KB)\n",
					total_size, total_size / 1024);
			} else {
				DecInfo *pDecInfo =
					&pCodecInst->CodecInfo->decInfo;

				tcli_print(
					"bitstreamBufferSize:%d\n",
					pDecInfo->openParam.bitstreamBufferSize);
				tcli_print("common_size:%d\n",
					   vbu_comm_bf.size);
				tcli_print("work_buf_size:%d\n",
					   pDecInfo->vbWork.size);
				tcli_print("DecInfo_size:%d\n",
					   sizeof(DecInfo));
				tcli_print(
					"FrameBuffer_size:%d\n",
					pDecInfo->initialInfo.picWidth *
						pDecInfo->initialInfo.picHeight *
						pDecInfo->numFbsForDecoding);

				total_size =
					pDecInfo->openParam.bitstreamBufferSize +
					pDecInfo->vbWork.size +
					sizeof(DecInfo) +
					pDecInfo->initialInfo.picWidth *
						pDecInfo->initialInfo.picHeight *
						pDecInfo->numFbsForDecoding;
				tcli_print(
					"currnet dec instance's total_size:%d (%d KB)\n",
					total_size, total_size / 1024);
			}

			all_total_size += total_size;
			tcli_print("================end=================\n");
		}
	}

	all_total_size += vbu_comm_bf.size;
	tcli_print("all total_size:%d KB\n", all_total_size / 1024);

	return all_total_size;
}

static int vcodecMemoryInfo(int32_t argc, char *argv[])
{
	int coreIdx = 0;

	if (argc > 2) {
		tcli_print("usage:%s idx\n", argv[0]);
		tcli_print("usage:%s\n", argv[0]);
		tcli_print("0: 265,1: 264", argv[0]);
		return -1;
	}

	if (argc == 2) {
		coreIdx = (int)atoi(argv[1]);

		if (coreIdx > 1 || coreIdx < 0) {
			tcli_print("0: 265,1: 264", argv[0]);
			return -1;
		}
		getVcodecMemoryInfo(coreIdx);
		return 0;
	}

	uint32_t all_size = 0;

	all_size = getVcodecMemoryInfo(0) + getVcodecMemoryInfo(1);

	tcli_print("h264 and h265 use total_size:%d KB\n", all_size / 1000);
	return 0;
}

static TELNET_S_CLICMD vcodec_cli_cmd_list_subSecond[] = {
	DECLARE_CLI_CMD_MACRO(vpuregs, NULL, showVpuReg, show vcodec regs, 0),

	DECLARE_CLI_CMD_MACRO(vcdbgmask, NULL, vcodecModMaskDebug, get or set dbg_mask for vcodec, 0),
		DECLARE_CLI_CMD_MACRO(instPoolInfo, NULL, vcodecH26xInstaccePoolInfo, get vcodec instance pool, 0),
		DECLARE_CLI_CMD_MACRO(meminfo, NULL, vcodecMemoryInfo, get vcodec mem info, 0),
		DECLARE_CLI_CMD_MACRO_END()
	};

static TELNET_CLI_S_COMMAND vcodec_cli_cmd_list[] = {
	DECLARE_CLI_CMD_MACRO(vcodec, vcodec_cli_cmd_list_subSecond, NULL,
			      vcodec commands, 0),
	DECLARE_CLI_CMD_MACRO_END()
};

void vcodec_register_cmd(void)
{
	int enRet = 0;

	pthread_mutex_lock(&vcodecCliMutex);

	if (bVcodecCliReg) {
		pthread_mutex_unlock(&vcodecCliMutex);
		return;
	}

	bVcodecCliReg = 1;

	enRet = RegisterCliCommand((void *)vcodec_cli_cmd_list);

	if (enRet) {
		printf("<%s,%d>:cmd register failed,enRet=%#x\r\n", __func__,
		       __LINE__, enRet);
		pthread_mutex_unlock(&vcodecCliMutex);
		return;
	}

	printf("register vcodec_cli_cmd_list suc.\n");
	pthread_mutex_unlock(&vcodecCliMutex);
}
#endif
