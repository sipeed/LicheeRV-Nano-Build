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
#include "coda9_vpuconfig.h"
#include "product.h"
#include "coda9_regdefine.h"
#include "coda9.h"
#include "main_helper.h"
#include "cvi_enc_internal.h"

#define SUPPORT_ASO_FMO 1
#define USE_SWAP_264_FW

#define UNREFERENCED_PARAM(x) ((void)(x))

static int Coda9VpuEncCalcPicQp(CodecInst *pCodecInst, EncParam *param,
				int *pRowMaxDqpMinus, int *pRowMaxDqpPlus,
				int *pOutQp, int *pTargetPicBit,
				int *pHrdBufLevel, int *pHrdBufSize);
static void Coda9VpuEncGetPicInfo(EncInfo *pEncInfo, int *p_pic_idx,
				  int *p_gop_num, int *p_slice_type);
static void cviPrintFrmRc(frm_rc_t *pFrmRc);

static void LoadBitCode(Uint32 coreIdx, PhysicalAddress codeBase,
			const Uint16 *codeWord, int codeSize)
{
#ifndef USE_SWAP_264_FW
	int i;
#ifdef CVI_VC_MSG_ENABLE
	int load, prevLoad = -1;
#endif
	BYTE code[8];

	for (i = 0; i < codeSize; i += 4) {
		// 2byte little endian variable to 1byte big endian buffer
		code[0] = (BYTE)(codeWord[i + 0] >> 8);
		code[1] = (BYTE)codeWord[i + 0];
		code[2] = (BYTE)(codeWord[i + 1] >> 8);
		code[3] = (BYTE)codeWord[i + 1];
		code[4] = (BYTE)(codeWord[i + 2] >> 8);
		code[5] = (BYTE)codeWord[i + 2];
		code[6] = (BYTE)(codeWord[i + 3] >> 8);
		code[7] = (BYTE)codeWord[i + 3];
		VpuWriteMem(coreIdx, codeBase + i * 2, (BYTE *)code, 8,
			    VDI_BIG_ENDIAN);

#ifdef CVI_VC_MSG_ENABLE
		load = i * 100 / codeSize;
		if (load != prevLoad) {
			printf("%d %%\r", load);
			prevLoad = load;
		}
#endif
	}
#else
	VpuWriteMem(coreIdx, codeBase, (BYTE *)codeWord, codeSize * 2,
		    VDI_128BIT_LITTLE_ENDIAN);
#endif

	CVI_VC_FLOW("\n");

	vdi_set_bit_firmware_to_pm(coreIdx, codeWord);
}

static RetCode BitLoadFirmware(Uint32 coreIdx, PhysicalAddress codeBase,
			       const Uint16 *codeWord, int codeSize)
{
	int i, len;

#ifdef SMALL_CODE_SIZE
	len = 10 * 1024;
#else
	len = 2048;
#endif

	LoadBitCode(coreIdx, codeBase, codeWord, codeSize);

	VpuWriteReg(coreIdx, BIT_INT_ENABLE, 0);
	VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);

#ifndef USE_SWAP_264_FW
#ifdef CVI_VC_MSG_ENABLE
	int load = -1, prevLoad = -1;
#endif
	Uint32 data;

	for (i = 0; i < len; ++i) {
		data = codeWord[i];
		VpuWriteReg(coreIdx, BIT_CODE_DOWN, (i << 16) | data);

#ifdef CVI_VC_MSG_ENABLE
		load = i * 100 / len;
		if (load != prevLoad) {
			printf("%d %%\r", load);
			prevLoad = load;
		}
#endif
	}
#else

	for (i = 0; i < len; i += 4) {
		uint64_t code = *(uint64_t *)&codeWord[i];

		VpuWriteReg(coreIdx, BIT_CODE_DOWN,
			    (i << 16) | ((code >> 48) & 0xFFFF));
		VpuWriteReg(coreIdx, BIT_CODE_DOWN,
			    ((i + 1) << 16) | ((code >> 32) & 0xFFFF));
		VpuWriteReg(coreIdx, BIT_CODE_DOWN,
			    ((i + 2) << 16) | ((code >> 16) & 0xFFFF));
		VpuWriteReg(coreIdx, BIT_CODE_DOWN,
			    ((i + 3) << 16) | (code & 0xFFFF));
	}

#endif
	CVI_VC_FLOW("\n");

	return RETCODE_SUCCESS;
}

static void SetEncFrameMemInfo(CodecInst *pCodecInst)
{
	Uint32 val;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;

	switch (pCodecInst->productId) {
	case PRODUCT_ID_980:
		val = (pEncInfo->openParam.bwbEnable << 15) |
		      (pEncInfo->linear2TiledMode << 13) |
		      (pEncInfo->mapType << 9);
		if (pEncInfo->openParam.EncStdParam.avcParam.chromaFormat400)
			val |= (FORMAT_400 << 6);
		else
			val |= (FORMAT_420 << 6);
		val |= ((pEncInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= pEncInfo->openParam.frameEndian;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);

		break;
#ifdef REDUNDENT_CODE
	case PRODUCT_ID_960:
		val = 0;
		if (pEncInfo->mapType) {
			if (pEncInfo->mapType == TILED_FRAME_MB_RASTER_MAP ||
			    pEncInfo->mapType == TILED_FIELD_MB_RASTER_MAP)
				val |= (pEncInfo->linear2TiledEnable << 11) |
				       (0x03 << 9) | (FORMAT_420 << 6);
			else
				val |= (pEncInfo->linear2TiledEnable << 11) |
				       (0x02 << 9) | (FORMAT_420 << 6);
		}
		val |= ((pEncInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= (pEncInfo->openParam.cbcrInterleave &
			pEncInfo->openParam.nv21)
		       << 3;
		val |= (pEncInfo->openParam.bwbEnable << 12);
		val |= pEncInfo->openParam.frameEndian;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);
		break;
#endif
	}
}

void Coda9BitIssueCommand(Uint32 coreIdx, CodecInst *inst, int cmd)
{
	int instIdx = 0;
	int cdcMode = 0;
	int auxMode = 0;

	if (inst != NULL) {
		// command is specific to instance
		instIdx = inst->instIndex;
		cdcMode = inst->codecMode;
		auxMode = inst->codecModeAux;
	}

	if (inst) {
		if (inst->codecMode < AVC_ENC) {
			VpuWriteReg(coreIdx, BIT_WORK_BUF_ADDR,
				    inst->CodecInfo->decInfo.vbWork.phys_addr);
		} else {
			VpuWriteReg(coreIdx, BIT_WORK_BUF_ADDR,
				    inst->CodecInfo->encInfo.vbWork.phys_addr);
		}
	}

	VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 1);
	VpuWriteReg(coreIdx, BIT_RUN_INDEX, instIdx);
	VpuWriteReg(coreIdx, BIT_RUN_COD_STD, cdcMode);
	VpuWriteReg(coreIdx, BIT_RUN_AUX_STD, auxMode);
#ifdef ENABLE_CNM_DEBUG_MSG
	if (inst && inst->loggingEnable)
		vdi_log(coreIdx, cmd, 1);
#endif
	VpuWriteReg(coreIdx, BIT_RUN_COMMAND, cmd);
}

static void SetupCoda9Properties(Uint32 coreIdx, Uint32 productId)
{
	VpuAttr *pAttr = &g_VpuCoreAttributes[coreIdx];
	Int32 val;
	char *pstr;

	/* Setup Attributes */
	pAttr = &g_VpuCoreAttributes[coreIdx];

	// Hardware version information
	val = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);
	if ((val & 0xff00) == 0x3200)
		val = 0x3200;
	val = VpuReadReg(coreIdx, DBG_CONFIG_REPORT_0);
	pstr = (char *)&val;
	pAttr->productName[0] = pstr[3];
	pAttr->productName[1] = pstr[2];
	pAttr->productName[2] = pstr[1];
	pAttr->productName[3] = pstr[0];
	pAttr->productName[4] = 0;

	pAttr->supportDecoders = (1 << STD_AVC) | (1 << STD_VC1) |
				 (1 << STD_MPEG2) | (1 << STD_MPEG4) |
				 (1 << STD_H263) | (1 << STD_AVS) |
				 (1 << STD_DIV3) | (1 << STD_RV) |
				 (1 << STD_THO) | (1 << STD_VP8);

	/* Encoder */
	pAttr->supportEncoders =
		(1 << STD_AVC) | (1 << STD_MPEG4) | (1 << STD_H263);
	CVI_VC_TRACE("supportEncoders = 0x%X\n", pAttr->supportEncoders);

	/* WTL */
	if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
		pAttr->supportWTL = 1;
	}
	/* Tiled2Linear */
	pAttr->supportTiled2Linear = 1;
	/* Maptypes */
	pAttr->supportMapTypes =
		(1 << LINEAR_FRAME_MAP) | (1 << TILED_FRAME_V_MAP) |
		(1 << TILED_FRAME_H_MAP) | (1 << TILED_FIELD_V_MAP) |
		(1 << TILED_MIXED_V_MAP) | (1 << TILED_FRAME_MB_RASTER_MAP) |
		(1 << TILED_FIELD_MB_RASTER_MAP);
	if (productId == PRODUCT_ID_980) {
		pAttr->supportMapTypes |= (1 << TILED_FRAME_NO_BANK_MAP) |
					  (1 << TILED_FIELD_NO_BANK_MAP);
	}
	/* Linear2Tiled */
	if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
		pAttr->supportLinear2Tiled = 1;
	}
	/* Framebuffer Cache */
#ifdef REDUNDENT_CODE0
	if (productId == PRODUCT_ID_960)
		pAttr->framebufferCacheType = FramebufCacheMaverickI;
	else
#endif
		if (productId == PRODUCT_ID_980)
		pAttr->framebufferCacheType = FramebufCacheMaverickII;
	else
		pAttr->framebufferCacheType = FramebufCacheNone;
	/* AXI 128bit Bus */
	pAttr->support128bitBus = FALSE;
	pAttr->supportEndianMask =
		(1 << VDI_LITTLE_ENDIAN) | (1 << VDI_BIG_ENDIAN) |
		(1 << VDI_32BIT_LITTLE_ENDIAN) | (1 << VDI_32BIT_BIG_ENDIAN);
	pAttr->supportBitstreamMode =
		(1 << BS_MODE_INTERRUPT) | (1 << BS_MODE_PIC_END);
	pAttr->bitstreamBufferMargin = VPU_GBU_SIZE;
	pAttr->numberOfMemProtectRgns = 6;
}

Uint32 Coda9VpuGetProductId(Uint32 coreIdx)
{
	Uint32 productId;
	Uint32 val;

	val = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);

#ifdef REDUNDENT_CODE
	if (val == BODA950_CODE)
		productId = PRODUCT_ID_950;
	else if (val == CODA960_CODE)
		productId = PRODUCT_ID_960;
	else
#endif
		if (val == CODA980_CODE)
		productId = PRODUCT_ID_980;
	else
		productId = PRODUCT_ID_NONE;

	if (productId != PRODUCT_ID_NONE)
		SetupCoda9Properties(coreIdx, productId);

	return productId;
}

RetCode Coda9VpuGetVersion(Uint32 coreIdx, Uint32 *versionInfo,
			   Uint32 *revision)
{
	/* Get Firmware version */
	VpuWriteReg(coreIdx, RET_FW_VER_NUM, 0);
	Coda9BitIssueCommand(coreIdx, NULL, FIRMWARE_GET);
	if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, BIT_BUSY_FLAG) == -1)
		return RETCODE_VPU_RESPONSE_TIMEOUT;

	if (versionInfo != NULL) {
		*versionInfo = VpuReadReg(coreIdx, RET_FW_VER_NUM);
	}
	if (revision != NULL) {
		*revision = VpuReadReg(coreIdx, RET_FW_CODE_REV);
	}

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuInit(Uint32 coreIdx, void *firmware, Uint32 size)
{
	Uint32 data;
	vpu_buffer_t vb;
	PhysicalAddress tempBuffer;
	PhysicalAddress paraBuffer;
	PhysicalAddress codeBuffer;
	Uint64 start_time, end_time;

	vdi_get_common_memory((unsigned long)coreIdx, &vb);

	codeBuffer = vb.phys_addr;
	tempBuffer = codeBuffer + CODE_BUF_SIZE;
	paraBuffer = tempBuffer + TEMP_BUF_SIZE;

	CVI_VC_MCU("VPU INIT Start\n");

	start_time = cviGetCurrentTime();

	BitLoadFirmware(coreIdx, codeBuffer, (const Uint16 *)firmware, size);

	/* Clear registers */
	if (vdi_read_register(coreIdx, BIT_CUR_PC) == 0) {
		Uint32 i;
		for (i = 0; i < 64; i++) {
			vdi_write_register(coreIdx, (i * 4) + 0x100, 0x0);
		}
	}
	end_time = cviGetCurrentTime();
	CVI_VC_PERF("Coda9 LoadFirmware = %llu us\n", end_time - start_time);

	VpuWriteReg(coreIdx, BIT_PARA_BUF_ADDR, paraBuffer);
	VpuWriteReg(coreIdx, BIT_CODE_BUF_ADDR, codeBuffer);
	VpuWriteReg(coreIdx, BIT_TEMP_BUF_ADDR, tempBuffer);

	VpuWriteReg(coreIdx, BIT_BIT_STREAM_CTRL, VPU_STREAM_ENDIAN);
	VpuWriteReg(coreIdx, BIT_FRAME_MEM_CTRL,
		    CBCR_INTERLEAVE << 2 | VPU_FRAME_ENDIAN); // Interleave bit
	// position is
	// modified
	VpuWriteReg(coreIdx, BIT_BIT_STREAM_PARAM, 0);

	VpuWriteReg(coreIdx, BIT_AXI_SRAM_USE, 0);
	VpuWriteReg(coreIdx, BIT_INT_ENABLE, 0);
	VpuWriteReg(coreIdx, BIT_ROLLBACK_STATUS, 0);

	data = (1 << INT_BIT_BIT_BUF_FULL);
	data |= (1 << INT_BIT_BIT_BUF_EMPTY);
	data |= (1 << INT_BIT_DEC_MB_ROWS);
	data |= (1 << INT_BIT_SEQ_INIT);
	data |= (1 << INT_BIT_DEC_FIELD);
	data |= (1 << INT_BIT_PIC_RUN);

	VpuWriteReg(coreIdx, BIT_INT_ENABLE, data);
	VpuWriteReg(coreIdx, BIT_INT_CLEAR, 0x1);
	VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 0x1);
	VpuWriteReg(coreIdx, BIT_CODE_RESET, 1);
	VpuWriteReg(coreIdx, BIT_CODE_RUN, 1);

	if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, BIT_BUSY_FLAG) ==
	    -1) {
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	return RETCODE_SUCCESS;
}

RetCode Coda9VpuReInit(Uint32 coreIdx, void *firmware, Uint32 size)
{
	vpu_buffer_t vb;
	PhysicalAddress tempBuffer;
	PhysicalAddress paraBuffer;
	PhysicalAddress codeBuffer;
	PhysicalAddress oldCodeBuffer;

	CVI_VC_TRACE("\n");

	vdi_get_common_memory((unsigned long)coreIdx, &vb);

	codeBuffer = vb.phys_addr;
	tempBuffer = codeBuffer + CODE_BUF_SIZE;
	paraBuffer = tempBuffer + TEMP_BUF_SIZE;

	oldCodeBuffer = VpuReadReg(coreIdx, BIT_CODE_BUF_ADDR);

	VpuWriteReg(coreIdx, BIT_PARA_BUF_ADDR, paraBuffer);
	VpuWriteReg(coreIdx, BIT_CODE_BUF_ADDR, codeBuffer);
	VpuWriteReg(coreIdx, BIT_TEMP_BUF_ADDR, tempBuffer);

	if (oldCodeBuffer != codeBuffer) {
		LoadBitCode(coreIdx, codeBuffer, (const Uint16 *)firmware,
			    size);
	}

	return RETCODE_SUCCESS;
}

Uint32 Coda9VpuIsInit(Uint32 coreIdx)
{
	Uint32 pc;

	pc = VpuReadReg(coreIdx, BIT_CUR_PC);

	CVI_VC_TRACE("cur pc = 0x%X\n", pc);
	return pc;
}

Int32 Coda9VpuIsBusy(Uint32 coreIdx)
{
	return VpuReadReg(coreIdx, BIT_BUSY_FLAG);
}

Int32 Coda9VpuWaitInterrupt(CodecInst *handle, Int32 timeout)
{
	Int32 reason = 0;

	reason = vdi_wait_interrupt(handle->coreIdx, timeout,
				    (uint64_t *)&handle->u64EndTime);

	if ((Uint32)reason != INTERRUPT_TIMEOUT_VALUE) {
		VpuWriteReg(handle->coreIdx, BIT_INT_CLEAR,
			    1); // clear HW signal
	}

	return reason;
}
RetCode Coda9VpuClearInterrupt(Uint32 coreIdx)
{
	VpuWriteReg(coreIdx, BIT_INT_REASON,
		    0); // tell to F/W that HOST received an interrupt.

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
	Uint32 cmd;
	Int32 productId = Coda9VpuGetProductId(coreIdx);

	CVI_VC_TRACE("\n");
	if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
		if (resetMode != SW_RESET_ON_BOOT) {
			cmd = VpuReadReg(coreIdx, BIT_RUN_COMMAND);
			if (cmd == DEC_SEQ_INIT || cmd == PIC_RUN) {
				if (VpuReadReg(coreIdx, BIT_BUSY_FLAG) ||
				    VpuReadReg(coreIdx, BIT_INT_REASON)) {
#define MBC_SET_SUBBLK_EN (MBC_BASE + 0xA0)
					// subblk_man_mode[20]
					// cr_subblk_man_en[19:0] stop all of
					// pipeline
					VpuWriteReg(coreIdx, MBC_SET_SUBBLK_EN,
						    ((1 << 20) | 0));

					// force to set the end of Bitstream to
					// be decoded.
					cmd = VpuReadReg(coreIdx,
							 BIT_BIT_STREAM_PARAM);
#ifndef PLATFORM_NON_OS
					cmd |= 1 << 2;
#endif
					VpuWriteReg(coreIdx,
						    BIT_BIT_STREAM_PARAM, cmd);

					cmd = VpuReadReg(coreIdx, BIT_RD_PTR);
					VpuWriteReg(coreIdx, BIT_WR_PTR, cmd);

					cmd = vdi_wait_interrupt(
						coreIdx, __VPU_BUSY_TIMEOUT,
						NULL);

					if (cmd != INTERRUPT_TIMEOUT_VALUE) {
						VpuWriteReg(coreIdx,
							    BIT_INT_REASON, 0);
						VpuWriteReg(coreIdx,
							    BIT_INT_CLEAR,
							    1); // clear HW
						// signal
					}
					// now all of hardwares would be stop.
				}
			}
		}

		// Waiting for completion of BWB transaction first
		if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT,
				      GDI_BWB_STATUS) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
			vdi_log(coreIdx, 0x10, 2);
#endif
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}

		// Waiting for completion of bus transaction
		// Step1 : No more request
		VpuWriteReg(coreIdx, GDI_BUS_CTRL,
			    0x11); // no more request
		// {3'b0,no_more_req_sec,3'b0,no_more_req}

		// Step2 : Waiting for completion of bus transaction
		if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT,
				      GDI_BUS_STATUS) == -1) {
			VpuWriteReg(coreIdx, GDI_BUS_CTRL, 0x00);
#ifdef ENABLE_CNM_DEBUG_MSG
			vdi_log(coreIdx, 0x10, 2);
#endif
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}

		cmd = 0;
		// Software Reset Trigger
		if (resetMode != SW_RESET_ON_BOOT)
			cmd = VPU_SW_RESET_BPU_CORE | VPU_SW_RESET_BPU_BUS;
		cmd |= VPU_SW_RESET_VCE_CORE | VPU_SW_RESET_VCE_BUS;
		if (resetMode == SW_RESET_ON_BOOT)
			cmd |= VPU_SW_RESET_GDI_CORE |
			       VPU_SW_RESET_GDI_BUS; // If you reset GDI, tiled
				// map should be
				// reconfigured

		VpuWriteReg(coreIdx, BIT_SW_RESET, cmd);

		// wait until reset is done
		if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT,
				      BIT_SW_RESET_STATUS) == -1) {
			VpuWriteReg(coreIdx, BIT_SW_RESET, 0x00);
			VpuWriteReg(coreIdx, GDI_BUS_CTRL, 0x00);
#ifdef ENABLE_CNM_DEBUG_MSG
			vdi_log(coreIdx, 0x10, 2);
#endif
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}

		VpuWriteReg(coreIdx, BIT_SW_RESET, 0);

		// Step3 : must clear GDI_BUS_CTRL after done SW_RESET
		VpuWriteReg(coreIdx, GDI_BUS_CTRL, 0x00);
	} else {
#ifdef ENABLE_CNM_DEBUG_MSG
		vdi_log(coreIdx, 0x10, 0);
#endif
		return RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuSleepWake_EX(Uint32 coreIdx, int iSleepWake, const Uint16 *code,
			     Uint32 size)
{
	static unsigned int regBk[64];
	int i = 0;
#ifdef FIRMWARE_H
	const Uint16 *bit_code = NULL;
	if (code && size > 0)
		bit_code = code;

	if (!bit_code) {
		CVI_VC_ERR("no bit_code\n");
		return RETCODE_INVALID_PARAM;
	}
#endif
	if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, BIT_BUSY_FLAG) ==
	    -1) {
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}

	if (iSleepWake == 1) {
		for (i = 0; i < 64; i++)
			regBk[i] =
				VpuReadReg(coreIdx, BIT_BASE + 0x100 + (i * 4));

		VpuWriteReg(coreIdx, BIT_INT_ENABLE, 0);
		VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);
	} else {
		VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);

		for (i = 0; i < 64; i++)
			VpuWriteReg(coreIdx, BIT_BASE + 0x100 + (i * 4),
				    regBk[i]);

		VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 1);
		VpuWriteReg(coreIdx, BIT_CODE_RESET, 1);
		VpuWriteReg(coreIdx, BIT_CODE_RUN, 1);

		if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT,
				      BIT_BUSY_FLAG) == -1) {
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}
	}

	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
RetCode Coda9VpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16 *code,
			  Uint32 size)
{
	static unsigned int regBk[64];
	int i = 0;
	const Uint16 *bit_code = NULL;
	if (code && size > 0)
		bit_code = code;

	if (!bit_code) {
		CVI_VC_ERR("no bit_code\n");
		return RETCODE_INVALID_PARAM;
	}

	if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, BIT_BUSY_FLAG) ==
	    -1) {
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}

	if (iSleepWake == 1) {
		for (i = 0; i < 64; i++)
			regBk[i] =
				VpuReadReg(coreIdx, BIT_BASE + 0x100 + (i * 4));
	} else {
		VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);

		for (i = 0; i < 64; i++)
			VpuWriteReg(coreIdx, BIT_BASE + 0x100 + (i * 4),
				    regBk[i]);

		VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 1);
		VpuWriteReg(coreIdx, BIT_CODE_RESET, 1);
		VpuWriteReg(coreIdx, BIT_CODE_RUN, 1);

		if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT,
				      BIT_BUSY_FLAG) == -1) {
			return RETCODE_VPU_RESPONSE_TIMEOUT;
		}
	}

	return RETCODE_SUCCESS;
}
#endif

static RetCode SetupDecCodecInstance(Int32 productId, CodecInst *pCodec)
{
	DecInfo *pDecInfo = &pCodec->CodecInfo->decInfo;
#ifndef REDUNDENT_CODE
	UNREFERENCED_PARAM(productId);
#endif

	pDecInfo->streamRdPtrRegAddr = BIT_RD_PTR;
	pDecInfo->streamWrPtrRegAddr = BIT_WR_PTR;
	pDecInfo->frameDisplayFlagRegAddr = BIT_FRM_DIS_FLG;
	pDecInfo->currentPC = BIT_CUR_PC;
	pDecInfo->busyFlagAddr = BIT_BUSY_FLAG;

#ifdef REDUNDENT_CODE
	if (productId == PRODUCT_ID_960) {
		pDecInfo->dramCfg.rasBit = EM_RAS;
		pDecInfo->dramCfg.casBit = EM_CAS;
		pDecInfo->dramCfg.bankBit = EM_BANK;
		pDecInfo->dramCfg.busBit = EM_WIDTH;
	}
#endif

	return RETCODE_SUCCESS;
}

static RetCode SetupEncCodecInstance(Int32 productId, CodecInst *pCodec)
{
	EncInfo *pEncInfo = &pCodec->CodecInfo->encInfo;
#ifndef REDUNDENT_CODE
	UNREFERENCED_PARAM(productId);
#endif

	pEncInfo->streamRdPtrRegAddr = BIT_RD_PTR;
	pEncInfo->streamWrPtrRegAddr = BIT_WR_PTR;
	pEncInfo->currentPC = BIT_CUR_PC;
	pEncInfo->busyFlagAddr = BIT_BUSY_FLAG;

#ifdef REDUNDENT_CODE
	if (productId == PRODUCT_ID_960) {
		pEncInfo->dramCfg.rasBit = EM_RAS;
		pEncInfo->dramCfg.casBit = EM_CAS;
		pEncInfo->dramCfg.bankBit = EM_BANK;
		pEncInfo->dramCfg.busBit = EM_WIDTH;
	}
#endif

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuBuildUpDecParam(CodecInst *pCodec, DecOpenParam *param)
{
	RetCode ret = RETCODE_SUCCESS;
	Uint32 coreIdx;
	Uint32 productId;
	DecInfo *pDecInfo = &pCodec->CodecInfo->decInfo;

	coreIdx = pCodec->coreIdx;
	productId = Coda9VpuGetProductId(coreIdx);

	ret = SetupDecCodecInstance(productId, pCodec);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (param->vbWork.size) {
		pDecInfo->vbWork = param->vbWork;
		pDecInfo->workBufferAllocExt = 1;
	} else {
		char ionName[MAX_VPU_ION_BUFFER_NAME];

		pDecInfo->vbWork.size = WORK_BUF_SIZE;
		if (pCodec->codecMode == AVC_DEC)
			pDecInfo->vbWork.size += PS_SAVE_SIZE;

		CVI_VC_MEM("vbWork.size = 0x%X\n", pDecInfo->vbWork.size);
		sprintf(ionName, "VDEC_%d_H264_WorkBuffer", pCodec->s32ChnNum);
		if (VDI_ALLOCATE_MEMORY(pCodec->coreIdx, &pDecInfo->vbWork, 0,
					ionName) < 0)
			return RETCODE_INSUFFICIENT_RESOURCE;

		param->vbWork = pDecInfo->vbWork;
		pDecInfo->workBufferAllocExt = 0;
	}

#ifdef REDUNDENT_CODE
	if (productId == PRODUCT_ID_960) {
		pDecInfo->dramCfg.bankBit = EM_BANK;
		pDecInfo->dramCfg.casBit = EM_CAS;
		pDecInfo->dramCfg.rasBit = EM_RAS;
		pDecInfo->dramCfg.busBit = EM_WIDTH;
	} else
#endif
	{
		/* CODA980 */
		pDecInfo->targetSubLayerId = AVC_MAX_SUB_LAYER_ID;
	}

	return ret;
}

RetCode Coda9VpuDecInitSeq(DecHandle handle)
{
	CodecInst *pCodecInst = (CodecInst *)handle;
	DecInfo *pDecInfo = &pCodecInst->CodecInfo->decInfo;
	Uint32 val = 0;

	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_BB_START,
		    pDecInfo->streamBufStartAddr);
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_BB_SIZE,
		    pDecInfo->streamBufSize >> 10); // size in KBytes

	if (pDecInfo->userDataEnable == TRUE) {
		val = 0;
		val |= (pDecInfo->userDataReportMode << 10);
		val |= (pDecInfo->userDataEnable << 5);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_USER_DATA_OPTION,
			    val);
		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_DEC_SEQ_USER_DATA_BASE_ADDR,
			    pDecInfo->userDataBufAddr);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_USER_DATA_BUF_SIZE,
			    pDecInfo->userDataBufSize);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_USER_DATA_OPTION,
			    0);
		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_DEC_SEQ_USER_DATA_BASE_ADDR, 0);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_USER_DATA_BUF_SIZE,
			    0);
	}
	val = 0;

	val |= (pDecInfo->reorderEnable << 1) & 0x2;

	val |= (pDecInfo->openParam.mp4DeblkEnable & 0x1);
	val |= (pDecInfo->avcErrorConcealMode << 2);
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_OPTION, val);

	switch (pCodecInst->codecMode) {
#ifdef REDUNDENT_CODE
	case VC1_DEC:
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_VC1_STREAM_FMT,
			    (0 << 3) & 0x08);
		break;
	case MP4_DEC:
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_MP4_ASP_CLASS,
			    (VPU_GMC_PROCESS_METHOD << 3) |
				    pDecInfo->openParam.mp4Class);
		break;
#endif
	case AVC_DEC:
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_X264_MV_EN,
			    VPU_AVC_X264_SUPPORT);
		break;
	}

	if (pCodecInst->codecMode == AVC_DEC)
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_SEQ_SPP_CHUNK_SIZE,
			    VPU_GBU_SIZE);

	VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamWrPtrRegAddr,
		    pDecInfo->streamWrPtr);
	VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr,
		    pDecInfo->streamRdPtr);

	if (pCodecInst->productId == PRODUCT_ID_980 ||
	    pCodecInst->productId == PRODUCT_ID_960) {
		pDecInfo->streamEndflag &= ~(3 << 3);
		if (pDecInfo->openParam.bitstreamMode == BS_MODE_PIC_END)
			pDecInfo->streamEndflag |= (2 << 3);
		else { // Interrupt Mode
			if (pDecInfo->seqInitEscape) {
				pDecInfo->streamEndflag |= (2 << 3);
			}
		}
	}
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM,
		    pDecInfo->streamEndflag);

	val = pDecInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (pCodecInst->productId == PRODUCT_ID_980) {
		val = 0;
		val |= (pDecInfo->openParam.bwbEnable << 15);
		val |= (pDecInfo->wtlMode << 17) |
		       (pDecInfo->tiled2LinearMode << 13) | (FORMAT_420 << 6);
		val |= ((pDecInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= pDecInfo->openParam.frameEndian;
		val |= pDecInfo->openParam.nv21 << 3;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);
#ifdef REDUNDENT_CODE
	} else if (pCodecInst->productId == PRODUCT_ID_960) {
		val = 0;
		val |= (pDecInfo->wtlEnable << 17);
		val |= (pDecInfo->openParam.bwbEnable << 12);
		val |= ((pDecInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= pDecInfo->openParam.frameEndian;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);
#endif
	} else {
		return RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	VpuWriteReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr, 0);
	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, DEC_SEQ_INIT);

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuFiniSeq(CodecInst *instance)
{
	Coda9BitIssueCommand(instance->coreIdx, instance, DEC_SEQ_END);
	if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuDecode(CodecInst *instance, DecParam *param)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	Uint32 rotMir;
	Int32 val;
	vpu_instance_pool_t *vip;

	pCodecInst = instance;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		return RETCODE_INVALID_HANDLE;
	}

	rotMir = 0;
	if (pDecInfo->rotationEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->rotationAngle) {
		case 0:
			rotMir |= 0x0;
			break;

		case 90:
			rotMir |= 0x1;
			break;

		case 180:
			rotMir |= 0x2;
			break;

		case 270:
			rotMir |= 0x3;
			break;
		}
	}

	if (pDecInfo->mirrorEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->mirrorDirection) {
		case MIRDIR_NONE:
			rotMir |= 0x0;
			break;

		case MIRDIR_VER:
			rotMir |= 0x4;
			break;

		case MIRDIR_HOR:
			rotMir |= 0x8;
			break;

		case MIRDIR_HOR_VER:
			rotMir |= 0xc;
			break;
		}
	}

	if (pDecInfo->tiled2LinearEnable) {
		rotMir |= 0x10;
	}

	if (pDecInfo->deringEnable) {
		rotMir |= 0x20; // Enable Dering Filter
	}

	if (rotMir && !pDecInfo->rotatorOutputValid) {
		return RETCODE_ROTATOR_OUTPUT_NOT_SET;
	}

	VpuWriteReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_LEFT_RIGHT,
		    0); // frame crop information(left, right)
	VpuWriteReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_TOP_BOTTOM,
		    0); // frame crop information(top, bottom)

#ifdef REDUNDENT_CODE
	if (pCodecInst->productId == PRODUCT_ID_960) {
		if (pDecInfo->mapType > LINEAR_FRAME_MAP &&
		    pDecInfo->mapType <= TILED_MIXED_V_MAP) {
			SetTiledFrameBase(pCodecInst->coreIdx,
					  pDecInfo->mapCfg.tiledBaseAddr);
		} else {
			SetTiledFrameBase(pCodecInst->coreIdx, 0);
		}
	}
#endif

	if (pDecInfo->mapType != LINEAR_FRAME_MAP &&
	    pDecInfo->mapType != LINEAR_FIELD_MAP) {
		val = SetTiledMapType(
			pCodecInst->coreIdx, &pDecInfo->mapCfg,
			pDecInfo->mapType,
			(pDecInfo->stride > pDecInfo->frameBufferHeight) ?
				      pDecInfo->stride :
				      pDecInfo->frameBufferHeight,
			pDecInfo->openParam.cbcrInterleave, &pDecInfo->dramCfg);
	} else {
		val = SetTiledMapType(pCodecInst->coreIdx, &pDecInfo->mapCfg,
				      pDecInfo->mapType, pDecInfo->stride,
				      pDecInfo->openParam.cbcrInterleave,
				      &pDecInfo->dramCfg);
	}
	if (val == 0) {
		return RETCODE_INVALID_PARAM;
	}

	if (rotMir & 0x30) { // rotator or dering or tiled2linear enabled
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_MODE, rotMir);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_INDEX,
			    pDecInfo->rotatorOutput.myIndex);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_ADDR_Y,
			    pDecInfo->rotatorOutput.bufY);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_ADDR_CB,
			    pDecInfo->rotatorOutput.bufCb);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_ADDR_CR,
			    pDecInfo->rotatorOutput.bufCr);
		if (pCodecInst->productId == PRODUCT_ID_980) {
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_DEC_PIC_ROT_BOTTOM_Y,
				    pDecInfo->rotatorOutput.bufYBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_DEC_PIC_ROT_BOTTOM_CB,
				    pDecInfo->rotatorOutput.bufCbBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_DEC_PIC_ROT_BOTTOM_CR,
				    pDecInfo->rotatorOutput.bufCrBot);
		}
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_STRIDE,
			    pDecInfo->rotatorStride);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_ROT_MODE, rotMir);
	}
	if (pDecInfo->userDataEnable) {
		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_DEC_PIC_USER_DATA_BASE_ADDR,
			    pDecInfo->userDataBufAddr);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_USER_DATA_BUF_SIZE,
			    pDecInfo->userDataBufSize);
	} else {
		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_DEC_PIC_USER_DATA_BASE_ADDR, 0);
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_USER_DATA_BUF_SIZE,
			    0);
	}

	val = 0;
	if (param->iframeSearchEnable == TRUE) { // if iframeSearch is Enable,
		// other bit is ignore;
		val |= (pDecInfo->userDataReportMode << 10);

		if (pCodecInst->codecMode == AVC_DEC ||
		    pCodecInst->codecMode == VC1_DEC) {
			if (param->iframeSearchEnable == 1)
				val |= (1 << 11) | (1 << 2);
			else if (param->iframeSearchEnable == 2)
				val |= (1 << 2);
		} else {
			val |= ((param->iframeSearchEnable & 0x1) << 2);
		}
	} else {
		val |= (pDecInfo->userDataReportMode << 10);
		val |= (pDecInfo->userDataEnable << 5);
		val |= (param->skipframeMode << 3);
	}

	if (pCodecInst->productId == PRODUCT_ID_980) {
		if (pCodecInst->codecMode == AVC_DEC &&
		    pDecInfo->lowDelayInfo.lowDelayEn) {
			val |= (pDecInfo->lowDelayInfo.lowDelayEn << 18);
		}
	}
	if (pCodecInst->codecMode == MP2_DEC) {
		val |= ((param->DecStdParam.mp2PicFlush & 1) << 15);
	}
	if (pCodecInst->codecMode == RV_DEC) {
		val |= ((param->DecStdParam.rvDbkMode & 0x0f) << 16);
	}

	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_OPTION, val);

	if (pCodecInst->productId == PRODUCT_ID_980) {
		if (pDecInfo->lowDelayInfo.lowDelayEn == TRUE) {
			VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_NUM_ROWS,
				    pDecInfo->lowDelayInfo.numRows);
		} else {
			VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_NUM_ROWS,
				    0);
		}
	}

	val = 0;

	val = ((pDecInfo->secAxiInfo.u.coda9.useBitEnable & 0x01) << 0 |
	       (pDecInfo->secAxiInfo.u.coda9.useIpEnable & 0x01) << 1 |
	       (pDecInfo->secAxiInfo.u.coda9.useDbkYEnable & 0x01) << 2 |
	       (pDecInfo->secAxiInfo.u.coda9.useDbkCEnable & 0x01) << 3 |
	       (pDecInfo->secAxiInfo.u.coda9.useOvlEnable & 0x01) << 4 |
	       (pDecInfo->secAxiInfo.u.coda9.useBtpEnable & 0x01) << 5 |
	       (pDecInfo->secAxiInfo.u.coda9.useBitEnable & 0x01) << 8 |
	       (pDecInfo->secAxiInfo.u.coda9.useIpEnable & 0x01) << 9 |
	       (pDecInfo->secAxiInfo.u.coda9.useDbkYEnable & 0x01) << 10 |
	       (pDecInfo->secAxiInfo.u.coda9.useDbkCEnable & 0x01) << 11 |
	       (pDecInfo->secAxiInfo.u.coda9.useOvlEnable & 0x01) << 12 |
	       (pDecInfo->secAxiInfo.u.coda9.useBtpEnable & 0x01) << 13);

	VpuWriteReg(pCodecInst->coreIdx, BIT_AXI_SRAM_USE, val);

	VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamWrPtrRegAddr,
		    pDecInfo->streamWrPtr);
	VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr,
		    pDecInfo->streamRdPtr);
	CVI_VC_BS("rdPtr = 0x%llX, wrPtr = 0x%llX\n", pDecInfo->streamRdPtr,
		  pDecInfo->streamWrPtr);

	pDecInfo->streamEndflag &= ~(3 << 3);
	if (pDecInfo->openParam.bitstreamMode == BS_MODE_PIC_END)
		pDecInfo->streamEndflag |= (2 << 3);

	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM,
		    pDecInfo->streamEndflag);

	if (pCodecInst->productId == PRODUCT_ID_980) {
		val = 0;
		val |= (pDecInfo->openParam.bwbEnable << 15);
		val |= (pDecInfo->wtlMode << 17) |
		       (pDecInfo->tiled2LinearMode << 13) |
		       (pDecInfo->mapType << 9) | (FORMAT_420 << 6);
		if (pDecInfo->openParam.cbcrInterleave == 1)
			val |= pDecInfo->openParam.nv21 << 3;
#ifdef REDUNDENT_CODE
	} else if (pCodecInst->productId == PRODUCT_ID_960) {
		val = 0;
		val |= (pDecInfo->wtlEnable << 17);
		val |= (pDecInfo->openParam.bwbEnable << 12);
		if (pDecInfo->mapType) {
			if (pDecInfo->mapType == TILED_FRAME_MB_RASTER_MAP ||
			    pDecInfo->mapType == TILED_FIELD_MB_RASTER_MAP)
				val |= (pDecInfo->tiled2LinearEnable << 11) |
				       (0x03 << 9) | (FORMAT_420 << 6);
			else
				val |= (pDecInfo->tiled2LinearEnable << 11) |
				       (0x02 << 9) | (FORMAT_420 << 6);
		}
#endif
	} else {
		return RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	val |= ((pDecInfo->openParam.cbcrInterleave) << 2); // Interleave bit
		// position is
		// modified
	val |= pDecInfo->openParam.frameEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);

	val = pDecInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (PRODUCT_ID_980 == pCodecInst->productId &&
	    AVC_DEC == pCodecInst->codecMode) {
		VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PIC_SVC_INFO,
			    pDecInfo->targetSubLayerId);
	}

	pCodecInst->u64StartTime = cviGetCurrentTime();

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, PIC_RUN);

	return RETCODE_SUCCESS;
}

Uint32 Coda9VpuGetFrameCycle(Uint32 coreIdx)
{
	return VpuReadReg(coreIdx, BIT_FRAME_CYCLE);
}

RetCode Coda9VpuDecGetResult(CodecInst *instance, DecOutputInfo *result)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	Uint32 val = 0;

	pCodecInst = instance;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, PIC_RUN, 0);
#endif

	result->warnInfo = 0;
	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SUCCESS);
	result->decodingSuccess = val;
	if (result->decodingSuccess & (1UL << 31)) {
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

	if (pCodecInst->codecMode == AVC_DEC) {
		result->notSufficientPsBuffer = (val >> 3) & 0x1;
		result->notSufficientSliceBuffer = (val >> 2) & 0x1;
		result->refMissingFrameFlag = (val >> 21) & 0x1;
	}

	result->chunkReuseRequired = 0;
	if (pDecInfo->openParam.bitstreamMode == BS_MODE_PIC_END) {
		switch (pCodecInst->codecMode) {
		case AVC_DEC:
			result->chunkReuseRequired =
				((val >> 16) & 0x01); // in case of NPF frame
			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_DECODED_IDX);
			if (val == (Uint32)-1) {
				result->chunkReuseRequired = TRUE;
			}
			break;
		default:
			break;
		}
	}

	result->indexFrameDecoded =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DECODED_IDX);
	result->indexFrameDisplay =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DISPLAY_IDX);

	if (pDecInfo->mapType == LINEAR_FRAME_MAP) {
		result->indexFrameDecodedForTiled = -1;
		result->indexFrameDisplayForTiled = -1;
	} else {
		result->indexFrameDecodedForTiled = result->indexFrameDecoded;
		result->indexFrameDisplayForTiled = result->indexFrameDisplay;
	}

	val = VpuReadReg(pCodecInst->coreIdx,
			 RET_DEC_PIC_SIZE); // decoding picture size
	result->decPicWidth = (val >> 16) & 0xFFFF;
	result->decPicHeight = (val)&0xFFFF;

	if (result->indexFrameDecoded >= 0 &&
	    result->indexFrameDecoded < MAX_GDI_IDX) {
		switch (pCodecInst->codecMode) {
#ifdef REDUNDENT_CODE
		case VPX_DEC:
			if (pCodecInst->codecModeAux == VPX_AUX_VP8) {
				// VP8 specific header information
				// h_scale[31:30] v_scale[29:28]
				// pic_width[27:14] pic_height[13:0]
				val = VpuReadReg(pCodecInst->coreIdx,
						 RET_DEC_PIC_VP8_SCALE_INFO);
				result->vp8ScaleInfo.hScaleFactor =
					(val >> 30) & 0x03;
				result->vp8ScaleInfo.vScaleFactor =
					(val >> 28) & 0x03;
				result->vp8ScaleInfo.picWidth =
					(val >> 14) & 0x3FFF;
				result->vp8ScaleInfo.picHeight =
					(val >> 0) & 0x3FFF;
				// ref_idx_gold[31:24], ref_idx_altr[23:16],
				// ref_idx_last[15: 8], version_number[3:1],
				// show_frame[0]
				val = VpuReadReg(pCodecInst->coreIdx,
						 RET_DEC_PIC_VP8_PIC_REPORT);
				result->vp8PicInfo.refIdxGold =
					(val >> 24) & 0x0FF;
				result->vp8PicInfo.refIdxAltr =
					(val >> 16) & 0x0FF;
				result->vp8PicInfo.refIdxLast =
					(val >> 8) & 0x0FF;
				result->vp8PicInfo.versionNumber =
					(val >> 1) & 0x07;
				result->vp8PicInfo.showFrame =
					(val >> 0) & 0x01;
			}
			break;
		case AVS_DEC:
#endif
		case AVC_DEC:
			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_CROP_LEFT_RIGHT); // frame
			// crop
			// information(left,
			// right)
			pDecInfo->initialInfo.picCropRect.left =
				(val >> 16) & 0xffff;
			pDecInfo->initialInfo.picCropRect.right =
				pDecInfo->initialInfo.picWidth - (val & 0xffff);
			val = VpuReadReg(
				pCodecInst->coreIdx,
				RET_DEC_PIC_CROP_TOP_BOTTOM); // frame crop
			// information(top,
			// bottom)
			pDecInfo->initialInfo.picCropRect.top =
				(val >> 16) & 0xffff;
			pDecInfo->initialInfo.picCropRect.bottom =
				pDecInfo->initialInfo.picHeight -
				(val & 0xffff);
			break;
#ifdef REDUNDENT_CODE
		case MP2_DEC:
			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_SEQ_MP2_BAR_LEFT_RIGHT);
			pDecInfo->initialInfo.mp2BardataInfo.barLeft =
				((val >> 16) & 0xFFFF);
			pDecInfo->initialInfo.mp2BardataInfo.barRight =
				(val & 0xFFFF);
			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_SEQ_MP2_BAR_TOP_BOTTOM);
			pDecInfo->initialInfo.mp2BardataInfo.barTop =
				((val >> 16) & 0xFFFF);
			pDecInfo->initialInfo.mp2BardataInfo.barBottom =
				(val & 0xFFFF);
			result->mp2BardataInfo =
				pDecInfo->initialInfo.mp2BardataInfo;

			result->mp2PicDispExtInfo.offsetNum =
				VpuReadReg(pCodecInst->coreIdx,
					   RET_DEC_PIC_MP2_OFFSET_NUM);

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_MP2_OFFSET1);
			result->mp2PicDispExtInfo.horizontalOffset1 =
				(Int16)(val >> 16) & 0xFFFF;
			result->mp2PicDispExtInfo.verticalOffset1 =
				(Int16)(val & 0xFFFF);

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_MP2_OFFSET2);
			result->mp2PicDispExtInfo.horizontalOffset2 =
				(Int16)(val >> 16) & 0xFFFF;
			result->mp2PicDispExtInfo.verticalOffset2 =
				(Int16)(val & 0xFFFF);

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_MP2_OFFSET3);
			result->mp2PicDispExtInfo.horizontalOffset3 =
				(Int16)(val >> 16) & 0xFFFF;
			result->mp2PicDispExtInfo.verticalOffset3 =
				(Int16)(val & 0xFFFF);
			break;
#endif
		}
	}

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_TYPE);
	result->interlacedFrame = (val >> 18) & 0x1;
	result->topFieldFirst = (val >> 21) & 0x0001; // TopFieldFirst[21]
	if (result->interlacedFrame) {
		result->picTypeFirst = (val & 0x38) >> 3; // pic_type of 1st
			// field
		result->picType = val & 7; // pic_type of 2nd field
	} else {
		result->picTypeFirst = PIC_TYPE_MAX; // no meaning
		result->picType = val & 7;
	}

	result->pictureStructure = (val >> 19) & 0x0003; // MbAffFlag[17],
		// FieldPicFlag[16]
	result->repeatFirstField = (val >> 22) & 0x0001;
	result->progressiveFrame = (val >> 23) & 0x0003;

	if (pCodecInst->codecMode == AVC_DEC) {
		result->decFrameInfo = (val >> 15) & 0x0001;
		result->picStrPresent = (val >> 27) & 0x0001;
		result->picTimingStruct = (val >> 28) & 0x000f;
		// update picture type when IDR frame
		if (val & 0x40) { // 6th bit
			if (result->interlacedFrame)
				result->picTypeFirst = PIC_TYPE_IDR;
			else
				result->picType = PIC_TYPE_IDR;
		}
		result->decFrameInfo = (val >> 16) & 0x0003;
		if (result->indexFrameDisplay >= 0) {
			if (result->indexFrameDisplay ==
			    result->indexFrameDecoded)
				result->avcNpfFieldInfo = result->decFrameInfo;
			else
				result->avcNpfFieldInfo =
					pDecInfo->decOutInfo
						[result->indexFrameDisplay]
							.decFrameInfo;
		}
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_HRD_INFO);
		result->avcHrdInfo.cpbMinus1 = val >> 2;
		result->avcHrdInfo.vclHrdParamFlag = (val >> 1) & 1;
		result->avcHrdInfo.nalHrdParamFlag = val & 1;

		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_INFO);
		result->avcVuiInfo.fixedFrameRateFlag = val & 1;
		result->avcVuiInfo.timingInfoPresent = (val >> 1) & 0x01;
		result->avcVuiInfo.chromaLocBotField = (val >> 2) & 0x07;
		result->avcVuiInfo.chromaLocTopField = (val >> 5) & 0x07;
		result->avcVuiInfo.chromaLocInfoPresent = (val >> 8) & 0x01;
		result->avcVuiInfo.colorPrimaries = (val >> 16) & 0xff;
		result->avcVuiInfo.colorDescPresent = (val >> 24) & 0x01;
		result->avcVuiInfo.isExtSAR = (val >> 25) & 0x01;
		result->avcVuiInfo.vidFullRange = (val >> 26) & 0x01;
		result->avcVuiInfo.vidFormat = (val >> 27) & 0x07;
		result->avcVuiInfo.vidSigTypePresent = (val >> 30) & 0x01;
		result->avcVuiInfo.vuiParamPresent = (val >> 31) & 0x01;
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_PIC_VUI_PIC_STRUCT);
		result->avcVuiInfo.vuiPicStructPresent = (val & 0x1);
		result->avcVuiInfo.vuiPicStruct = (val >> 1);
	}

#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == MP2_DEC) {
		result->fieldSequence = (val >> 25) & 0x0007;
		result->frameDct = (val >> 28) & 0x0001;
		result->progressiveSequence = (val >> 29) & 0x0001;
	}
#endif

	result->fRateNumerator =
		VpuReadReg(pCodecInst->coreIdx,
			   RET_DEC_PIC_FRATE_NR); // Frame rate, Aspect ratio
	// can be changed frame by
	// frame.
	result->fRateDenominator =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_DR);
	if (pCodecInst->codecMode == AVC_DEC && result->fRateDenominator > 0)
		result->fRateDenominator *= 2;
#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == MP4_DEC) {
		result->mp4ModuloTimeBase = VpuReadReg(
			pCodecInst->coreIdx, RET_DEC_PIC_MODULO_TIME_BASE);
		result->mp4TimeIncrement = VpuReadReg(
			pCodecInst->coreIdx, RET_DEC_PIC_VOP_TIME_INCREMENT);
	}

	if (pCodecInst->codecMode == RV_DEC) {
		result->rvTr =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_RV_TR);
		result->rvTrB = VpuReadReg(pCodecInst->coreIdx,
					   RET_DEC_PIC_RV_TR_BFRAME);
	}

	if (pCodecInst->codecMode == VPX_DEC) {
		result->aspectRateInfo = 0;
	} else
#endif
	{
		result->aspectRateInfo =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ASPECT);
	}

	// User Data
	if (pDecInfo->userDataEnable) {
		int userDataNum;
		int userDataSize;
		BYTE tempBuf[8] = {
			0,
		};

		VpuReadMem(pCodecInst->coreIdx, pDecInfo->userDataBufAddr + 0,
			   tempBuf, 8, VPU_USER_DATA_ENDIAN);

		val = ((tempBuf[0] << 24) & 0xFF000000) |
		      ((tempBuf[1] << 16) & 0x00FF0000) |
		      ((tempBuf[2] << 8) & 0x0000FF00) |
		      ((tempBuf[3] << 0) & 0x000000FF);

		userDataNum = (val >> 16) & 0xFFFF;
		userDataSize = (val >> 0) & 0xFFFF;
		if (userDataNum == 0)
			userDataSize = 0;

		result->decOutputExtData.userDataNum = userDataNum;
		result->decOutputExtData.userDataSize = userDataSize;

		val = ((tempBuf[4] << 24) & 0xFF000000) |
		      ((tempBuf[5] << 16) & 0x00FF0000) |
		      ((tempBuf[6] << 8) & 0x0000FF00) |
		      ((tempBuf[7] << 0) & 0x000000FF);

		if (userDataNum == 0)
			result->decOutputExtData.userDataBufFull = 0;
		else
			result->decOutputExtData.userDataBufFull =
				(val >> 16) & 0xFFFF;

		result->decOutputExtData.activeFormat =
			VpuReadReg(pCodecInst->coreIdx,
				   RET_DEC_PIC_ATSC_USER_DATA_INFO) &
			0xf;
	}

	result->numOfErrMBs =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ERR_MB);
	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SUCCESS);
	result->sequenceChanged = ((val >> 20) & 0x1);
	result->streamEndFlag = ((pDecInfo->streamEndflag >> 2) & 0x01);

#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == VC1_DEC &&
	    result->indexFrameDisplay != -3) {
		if (pDecInfo->vc1BframeDisplayValid == 0) {
			if (result->picType == 2) {
				result->indexFrameDisplay = -3;
			} else {
				pDecInfo->vc1BframeDisplayValid = 1;
			}
		}
	}
#endif
	if (pCodecInst->codecMode == AVC_DEC &&
	    pCodecInst->codecModeAux == AVC_AUX_MVC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_MVC_REPORT);
		result->mvcPicInfo.viewIdxDisplay = (val >> 0) & 1;
		result->mvcPicInfo.viewIdxDecoded = (val >> 1) & 1;
	}

	if (pCodecInst->codecMode == AVC_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI0);

		if ((int)val < 0) {
			result->avcFpaSei.exist = 0;
		} else {
			result->avcFpaSei.exist = 1;
			result->avcFpaSei.framePackingArrangementId = val;

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_AVC_FPA_SEI1);
			result->avcFpaSei.contentInterpretationType =
				val & 0x3F; // [5:0]
			result->avcFpaSei.framePackingArrangementType =
				(val >> 6) & 0x7F; // [12:6]
			result->avcFpaSei.framePackingArrangementExtensionFlag =
				(val >> 13) & 0x01; // [13]
			result->avcFpaSei.frame1SelfContainedFlag =
				(val >> 14) & 0x01; // [14]
			result->avcFpaSei.frame0SelfContainedFlag =
				(val >> 15) & 0x01; // [15]
			result->avcFpaSei.currentFrameIsFrame0Flag =
				(val >> 16) & 0x01; // [16]
			result->avcFpaSei.fieldViewsFlag =
				(val >> 17) & 0x01; // [17]
			result->avcFpaSei.frame0FlippedFlag =
				(val >> 18) & 0x01; // [18]
			result->avcFpaSei.spatialFlippingFlag =
				(val >> 19) & 0x01; // [19]
			result->avcFpaSei.quincunxSamplingFlag =
				(val >> 20) & 0x01; // [20]
			result->avcFpaSei.framePackingArrangementCancelFlag =
				(val >> 21) & 0x01; // [21]

			val = VpuReadReg(pCodecInst->coreIdx,
					 RET_DEC_PIC_AVC_FPA_SEI2);
			result->avcFpaSei
				.framePackingArrangementRepetitionPeriod =
				val & 0x7FFF; // [14:0]
			result->avcFpaSei.frame1GridPositionY =
				(val >> 16) & 0x0F; // [19:16]
			result->avcFpaSei.frame1GridPositionX =
				(val >> 20) & 0x0F; // [23:20]
			result->avcFpaSei.frame0GridPositionY =
				(val >> 24) & 0x0F; // [27:24]
			result->avcFpaSei.frame0GridPositionX =
				(val >> 28) & 0x0F; // [31:28]
		}

		result->avcPocTop =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_TOP);
		result->avcPocBot =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_BOT);

		if (result->interlacedFrame) {
			if (result->avcPocTop > result->avcPocBot) {
				result->avcPocPic = result->avcPocBot;
			} else {
				result->avcPocPic = result->avcPocTop;
			}
		} else
			result->avcPocPic = VpuReadReg(pCodecInst->coreIdx,
						       RET_DEC_PIC_POC);

		result->avcTemporalId =
			VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_TID_INFO);
	}

	if (pCodecInst->codecMode == AVC_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_PIC_AVC_SEI_RP_INFO);

		if ((int)val < 0) {
			result->avcRpSei.exist = 0;
		} else {
			result->avcRpSei.exist = 1;
			result->avcRpSei.changingSliceGroupIdc =
				val & 0x3; // [1:0]
			result->avcRpSei.brokenLinkFlag =
				(val >> 2) & 0x01; // [2]
			result->avcRpSei.exactMatchFlag =
				(val >> 3) & 0x01; // [3]
			result->avcRpSei.recoveryFrameCnt =
				(val >> 4) & 0x3F; // [9:4]
		}
	}

	result->bytePosFrameStart =
		VpuReadReg(pCodecInst->coreIdx, BIT_BYTE_POS_FRAME_START);
	result->bytePosFrameEnd =
		VpuReadReg(pCodecInst->coreIdx, BIT_BYTE_POS_FRAME_END);

	if (result->indexFrameDecoded >= 0 &&
	    result->indexFrameDecoded < MAX_GDI_IDX)
		pDecInfo->decOutInfo[result->indexFrameDecoded] = *result;

	result->frameDisplayFlag = pDecInfo->frameDisplayFlag;
	result->frameCycle = VpuReadReg(pCodecInst->coreIdx, BIT_FRAME_CYCLE);

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuDecSetBitstreamFlag(CodecInst *instance, BOOL running, BOOL eos)
{
	Uint32 val;
	DecInfo *pDecInfo;

	pDecInfo = &instance->CodecInfo->decInfo;

	if (eos & 0x01) {
		val = VpuReadReg(instance->coreIdx, BIT_BIT_STREAM_PARAM);
		val |= 1 << 2;
		pDecInfo->streamEndflag = val;
		if (running == TRUE)
			VpuWriteReg(instance->coreIdx, BIT_BIT_STREAM_PARAM,
				    val);
	} else {
		val = VpuReadReg(instance->coreIdx, BIT_BIT_STREAM_PARAM);
		val &= ~(1 << 2);
		pDecInfo->streamEndflag = val;
		if (running == TRUE)
			VpuWriteReg(instance->coreIdx, BIT_BIT_STREAM_PARAM,
				    val);
	}
	return RETCODE_SUCCESS;
}

#ifdef REDUNDENT_CODE
RetCode Coda9VpuDecCpbFlush(CodecInst *instance)
{
	Uint32 val;
	DecInfo *pDecInfo;

	pDecInfo = &instance->CodecInfo->decInfo;

	if (pDecInfo->openParam.bitstreamMode != BS_MODE_INTERRUPT) {
		return RETCODE_INVALID_COMMAND;
	}

	val = VpuReadReg(instance->coreIdx, BIT_BIT_STREAM_PARAM);
	val &= ~(3 << 3);
	val |= (2 << 3); // set to pic_end mode
	VpuWriteReg(instance->coreIdx, BIT_BIT_STREAM_PARAM, val);

	return RETCODE_SUCCESS;
}
#endif

RetCode Coda9VpuDecGetSeqInfo(CodecInst *instance, DecInitialInfo *info)
{
	CodecInst *pCodecInst = NULL;
	DecInfo *pDecInfo = NULL;
	Uint32 val, val2;

	pCodecInst = instance;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable) {
		vdi_log(pCodecInst->coreIdx, DEC_SEQ_INIT, 0);
	}
#endif

	info->warnInfo = 0;
	if (pDecInfo->openParam.bitstreamMode == BS_MODE_INTERRUPT &&
	    pDecInfo->seqInitEscape) {
		pDecInfo->streamEndflag &= ~(3 << 3);
		VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM,
			    pDecInfo->streamEndflag);
		pDecInfo->seqInitEscape = 0;
	}
	pDecInfo->streamRdPtr =
		VpuReadReg(instance->coreIdx, pDecInfo->streamRdPtrRegAddr);

	pDecInfo->frameDisplayFlag = VpuReadReg(
		pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
	CVI_VC_DISP("frameDisplayFlag = 0x%X\n", pDecInfo->frameDisplayFlag);

	pDecInfo->streamEndflag =
		VpuReadReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM);

	info->seqInitErrReason = 0;
	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_SUCCESS);
	if (val & (1UL << 31)) {
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

	if (pDecInfo->openParam.bitstreamMode == BS_MODE_PIC_END) {
		if (val & (1 << 4)) {
			info->seqInitErrReason =
				(VpuReadReg(pCodecInst->coreIdx,
					    RET_DEC_SEQ_SEQ_ERR_REASON));
			return RETCODE_FAILURE;
		}
	}

	if (val == 0) {
		info->seqInitErrReason = VpuReadReg(pCodecInst->coreIdx,
						    RET_DEC_SEQ_SEQ_ERR_REASON);
		CVI_VC_FLOW("seqInitErrReason = 0x%X\n",
			    info->seqInitErrReason);
		return RETCODE_FAILURE;
	}

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_SRC_SIZE);
	info->picWidth = ((val >> 16) & 0xffff);
	info->picHeight = (val & 0xffff);
	CVI_VC_MEM("picWidth = %d, picHeight = %d\n", info->picWidth,
		   info->picHeight);

	info->lumaBitdepth = 8;
	info->chromaBitdepth = 8;
	info->fRateNumerator =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_FRATE_NR);
	info->fRateDenominator =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_FRATE_DR);
	if (pCodecInst->codecMode == AVC_DEC && info->fRateDenominator > 0) {
		info->fRateDenominator *= 2;
	}

#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == MP4_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_INFO);
		info->mp4ShortVideoHeader = (val >> 2) & 1;
		info->mp4DataPartitionEnable = val & 1;
		info->mp4ReversibleVlcEnable =
			info->mp4DataPartitionEnable ? ((val >> 1) & 1) : 0;
		info->h263AnnexJEnable = (val >> 3) & 1;
	} else if (pCodecInst->codecMode == VPX_DEC &&
		   pCodecInst->codecModeAux == VPX_AUX_VP8) {
		// h_scale[31:30] v_scale[29:28] pic_width[27:14]
		// pic_height[13:0]
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_SEQ_VP8_SCALE_INFO);
		info->vp8ScaleInfo.hScaleFactor = (val >> 30) & 0x03;
		info->vp8ScaleInfo.vScaleFactor = (val >> 28) & 0x03;
		info->vp8ScaleInfo.picWidth = (val >> 14) & 0x3FFF;
		info->vp8ScaleInfo.picHeight = (val >> 0) & 0x3FFF;
	}
#endif

	info->minFrameBufferCount =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_FRAME_NEED);
	CVI_VC_FLOW("minFrameBufferCount = %d\n", info->minFrameBufferCount);

	info->frameBufDelay =
		VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_FRAME_DELAY);

	if (pCodecInst->codecMode == AVC_DEC ||
	    pCodecInst->codecMode == MP2_DEC ||
	    pCodecInst->codecMode == AVS_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_SEQ_CROP_LEFT_RIGHT);
		val2 = VpuReadReg(pCodecInst->coreIdx,
				  RET_DEC_SEQ_CROP_TOP_BOTTOM);

		info->picCropRect.left = ((val >> 16) & 0xFFFF);
		info->picCropRect.right = info->picWidth - (val & 0xFFFF);
		;
		info->picCropRect.top = ((val2 >> 16) & 0xFFFF);
		info->picCropRect.bottom = info->picHeight - (val2 & 0xFFFF);

		val = ((info->picWidth * info->picHeight * 3) >> 1) >> 10;
		info->normalSliceSize = val >> 2;
		info->worstSliceSize = val >> 1;
	}
#ifdef REDUNDENT_CODE
	else {
		info->picCropRect.left = 0;
		info->picCropRect.right = info->picWidth;
		info->picCropRect.top = 0;
		info->picCropRect.bottom = info->picHeight;
	}

	if (pCodecInst->codecMode == MP2_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_SEQ_MP2_BAR_LEFT_RIGHT);
		val2 = VpuReadReg(pCodecInst->coreIdx,
				  RET_DEC_SEQ_MP2_BAR_TOP_BOTTOM);

		info->mp2BardataInfo.barLeft = ((val >> 16) & 0xFFFF);
		info->mp2BardataInfo.barRight = (val & 0xFFFF);
		info->mp2BardataInfo.barTop = ((val2 >> 16) & 0xFFFF);
		info->mp2BardataInfo.barBottom = (val2 & 0xFFFF);
	}
#endif

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_HEADER_REPORT);
	info->profile = (val >> 0) & 0xFF;
	info->level = (val >> 8) & 0xFF;
	info->interlace = (val >> 16) & 0x01;
	info->direct8x8Flag = (val >> 17) & 0x01;
	info->vc1Psf = (val >> 18) & 0x01;
	info->constraint_set_flag[0] = (val >> 19) & 0x01;
	info->constraint_set_flag[1] = (val >> 20) & 0x01;
	info->constraint_set_flag[2] = (val >> 21) & 0x01;
	info->constraint_set_flag[3] = (val >> 22) & 0x01;
	info->chromaFormatIDC = (val >> 23) & 0x03;
	info->isExtSAR = (val >> 25) & 0x01;
	info->maxNumRefFrm = (val >> 27) & 0x0f;
	info->maxNumRefFrmFlag = (val >> 31) & 0x01;
	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_ASPECT);
	info->aspectRateInfo = val;

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_BIT_RATE);
	info->bitRate = val;

	if (pCodecInst->codecMode == AVC_DEC) {
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_VUI_INFO);
		info->avcVuiInfo.fixedFrameRateFlag = val & 1;
		info->avcVuiInfo.timingInfoPresent = (val >> 1) & 0x01;
		info->avcVuiInfo.chromaLocBotField = (val >> 2) & 0x07;
		info->avcVuiInfo.chromaLocTopField = (val >> 5) & 0x07;
		info->avcVuiInfo.chromaLocInfoPresent = (val >> 8) & 0x01;
		info->avcVuiInfo.colorPrimaries = (val >> 16) & 0xff;
		info->avcVuiInfo.colorDescPresent = (val >> 24) & 0x01;
		info->avcVuiInfo.isExtSAR = (val >> 25) & 0x01;
		info->avcVuiInfo.vidFullRange = (val >> 26) & 0x01;
		info->avcVuiInfo.vidFormat = (val >> 27) & 0x07;
		info->avcVuiInfo.vidSigTypePresent = (val >> 30) & 0x01;
		info->avcVuiInfo.vuiParamPresent = (val >> 31) & 0x01;

		val = VpuReadReg(pCodecInst->coreIdx,
				 RET_DEC_SEQ_VUI_PIC_STRUCT);
		info->avcVuiInfo.vuiPicStructPresent = (val & 0x1);
		info->avcVuiInfo.vuiPicStruct = (val >> 1);
	}

#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == MP2_DEC) {
		// seq_ext info
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_SEQ_EXT_INFO);
		info->mp2LowDelay = val & 1;
		info->mp2DispVerSize = (val >> 1) & 0x3fff;
		info->mp2DispHorSize = (val >> 15) & 0x3fff;

		if (pDecInfo->userDataEnable) {
			Uint32 userDataNum = 0;
			Uint32 userDataSize = 0;
			BYTE tempBuf[8] = {
				0,
			};

			// user data
			VpuReadMem(pCodecInst->coreIdx,
				   pDecInfo->userDataBufAddr, tempBuf, 8,
				   VPU_USER_DATA_ENDIAN);

			val = ((tempBuf[0] << 24) & 0xFF000000) |
			      ((tempBuf[1] << 16) & 0x00FF0000) |
			      ((tempBuf[2] << 8) & 0x0000FF00) |
			      ((tempBuf[3] << 0) & 0x000000FF);

			userDataNum = (val >> 16) & 0xFFFF;
			userDataSize = (val >> 0) & 0xFFFF;
			if (userDataNum == 0) {
				userDataSize = 0;
			}

			info->userDataNum = userDataNum;
			info->userDataSize = userDataSize;

			val = ((tempBuf[4] << 24) & 0xFF000000) |
			      ((tempBuf[5] << 16) & 0x00FF0000) |
			      ((tempBuf[6] << 8) & 0x0000FF00) |
			      ((tempBuf[7] << 0) & 0x000000FF);

			if (userDataNum == 0) {
				info->userDataBufFull = 0;
			} else {
				info->userDataBufFull = (val >> 16) & 0xFFFF;
			}
		}
	}
#endif

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuDecRegisterFramebuffer(CodecInst *instance)
{
	CodecInst *pCodecInst;
	DecInfo *pDecInfo;
	PhysicalAddress paraBuffer;
	vpu_buffer_t vb;
	Uint32 val;
	int i;
	BYTE frameAddr[MAX_DEC_FRAME_NUM][3][4];
	BYTE colMvAddr[MAX_DEC_FRAME_NUM][4];
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	pCodecInst = instance;
	pDecInfo = &pCodecInst->CodecInfo->decInfo;

	vdi_get_common_memory(pCodecInst->coreIdx, &vb);
	paraBuffer = vb.phys_addr + CODE_BUF_SIZE + TEMP_BUF_SIZE;

	pDecInfo->mapCfg.productId = pCodecInst->productId;

	if (pDecInfo->mapType != LINEAR_FRAME_MAP &&
	    pDecInfo->mapType != LINEAR_FIELD_MAP) {
		val = SetTiledMapType(
			pCodecInst->coreIdx, &pDecInfo->mapCfg,
			pDecInfo->mapType,
			(pDecInfo->stride > pDecInfo->frameBufferHeight) ?
				      pDecInfo->stride :
				      pDecInfo->frameBufferHeight,
			pDecInfo->openParam.cbcrInterleave, &pDecInfo->dramCfg);
	} else {
		val = SetTiledMapType(pCodecInst->coreIdx, &pDecInfo->mapCfg,
				      pDecInfo->mapType, pDecInfo->stride,
				      pDecInfo->openParam.cbcrInterleave,
				      &pDecInfo->dramCfg);
	}

	if (val == 0) {
		return RETCODE_INVALID_PARAM;
	}

	// Allocate frame buffer
	for (i = 0; i < pDecInfo->numFbsForDecoding; i++) {
		frameAddr[i][0][0] =
			(pDecInfo->frameBufPool[i].bufY >> 24) & 0xFF;
		frameAddr[i][0][1] =
			(pDecInfo->frameBufPool[i].bufY >> 16) & 0xFF;
		frameAddr[i][0][2] =
			(pDecInfo->frameBufPool[i].bufY >> 8) & 0xFF;
		frameAddr[i][0][3] =
			(pDecInfo->frameBufPool[i].bufY >> 0) & 0xFF;
		if (pDecInfo->openParam.cbcrOrder == CBCR_ORDER_NORMAL) {
			frameAddr[i][1][0] =
				(pDecInfo->frameBufPool[i].bufCb >> 24) & 0xFF;
			frameAddr[i][1][1] =
				(pDecInfo->frameBufPool[i].bufCb >> 16) & 0xFF;
			frameAddr[i][1][2] =
				(pDecInfo->frameBufPool[i].bufCb >> 8) & 0xFF;
			frameAddr[i][1][3] =
				(pDecInfo->frameBufPool[i].bufCb >> 0) & 0xFF;
			frameAddr[i][2][0] =
				(pDecInfo->frameBufPool[i].bufCr >> 24) & 0xFF;
			frameAddr[i][2][1] =
				(pDecInfo->frameBufPool[i].bufCr >> 16) & 0xFF;
			frameAddr[i][2][2] =
				(pDecInfo->frameBufPool[i].bufCr >> 8) & 0xFF;
			frameAddr[i][2][3] =
				(pDecInfo->frameBufPool[i].bufCr >> 0) & 0xFF;
		} else {
			frameAddr[i][2][0] =
				(pDecInfo->frameBufPool[i].bufCb >> 24) & 0xFF;
			frameAddr[i][2][1] =
				(pDecInfo->frameBufPool[i].bufCb >> 16) & 0xFF;
			frameAddr[i][2][2] =
				(pDecInfo->frameBufPool[i].bufCb >> 8) & 0xFF;
			frameAddr[i][2][3] =
				(pDecInfo->frameBufPool[i].bufCb >> 0) & 0xFF;
			frameAddr[i][1][0] =
				(pDecInfo->frameBufPool[i].bufCr >> 24) & 0xFF;
			frameAddr[i][1][1] =
				(pDecInfo->frameBufPool[i].bufCr >> 16) & 0xFF;
			frameAddr[i][1][2] =
				(pDecInfo->frameBufPool[i].bufCr >> 8) & 0xFF;
			frameAddr[i][1][3] =
				(pDecInfo->frameBufPool[i].bufCr >> 0) & 0xFF;
		}
	}

	VpuWriteMem(pCodecInst->coreIdx, paraBuffer, (BYTE *)frameAddr,
		    sizeof(frameAddr), VDI_BIG_ENDIAN);

	// MV allocation and register
	if (pCodecInst->codecMode == AVC_DEC ||
	    pCodecInst->codecMode == VC1_DEC ||
	    pCodecInst->codecMode == MP4_DEC ||
	    pCodecInst->codecMode == RV_DEC ||
	    pCodecInst->codecMode == AVS_DEC) {
		int size_mvcolbuf;
		vpu_buffer_t vbBuffer;
		size_mvcolbuf = ((pDecInfo->initialInfo.picWidth + 31) & ~31) *
				((pDecInfo->initialInfo.picHeight + 31) & ~31);
		size_mvcolbuf = (size_mvcolbuf * 3) >> 1;
		size_mvcolbuf = (size_mvcolbuf + 4) / 5;
		size_mvcolbuf = ((size_mvcolbuf + 7) >> 3) << 3;
		vbBuffer.size = VPU_ALIGN16384(size_mvcolbuf);
		vbBuffer.phys_addr = 0;
		for (i = 0; i < pDecInfo->numFbsForDecoding; i++) {
			CVI_VC_MEM("[%d] size_mvcolbuf = 0x%X\n", i,
				   vbBuffer.size);
			sprintf(ionName, "VDEC_%d_H264_Mvcolbuf",
				pCodecInst->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx, &vbBuffer,
						0, ionName) < 0) {
				return RETCODE_FAILURE;
			}
			pDecInfo->vbMV[i] = vbBuffer;
		}
		if (pCodecInst->codecMode == AVC_DEC) {
			for (i = 0; i < pDecInfo->numFbsForDecoding; i++) {
				colMvAddr[i][0] =
					(pDecInfo->vbMV[i].phys_addr >> 24) &
					0xFF;
				colMvAddr[i][1] =
					(pDecInfo->vbMV[i].phys_addr >> 16) &
					0xFF;
				colMvAddr[i][2] =
					(pDecInfo->vbMV[i].phys_addr >> 8) &
					0xFF;
				colMvAddr[i][3] =
					(pDecInfo->vbMV[i].phys_addr >> 0) &
					0xFF;
			}
		}
#ifdef REDUNDENT_CODE
		else {
			colMvAddr[0][0] =
				(pDecInfo->vbMV[0].phys_addr >> 24) & 0xFF;
			colMvAddr[0][1] =
				(pDecInfo->vbMV[0].phys_addr >> 16) & 0xFF;
			colMvAddr[0][2] =
				(pDecInfo->vbMV[0].phys_addr >> 8) & 0xFF;
			colMvAddr[0][3] =
				(pDecInfo->vbMV[0].phys_addr >> 0) & 0xFF;
		}
#endif
		VpuWriteMem(pCodecInst->coreIdx, paraBuffer + 384,
			    (BYTE *)colMvAddr, sizeof(colMvAddr),
			    VDI_BIG_ENDIAN);
	}

	if (pCodecInst->productId == PRODUCT_ID_980) {
		for (i = 0; i < pDecInfo->numFbsForDecoding; i++) {
			frameAddr[i][0][0] =
				(pDecInfo->frameBufPool[i].bufYBot >> 24) &
				0xFF;
			frameAddr[i][0][1] =
				(pDecInfo->frameBufPool[i].bufYBot >> 16) &
				0xFF;
			frameAddr[i][0][2] =
				(pDecInfo->frameBufPool[i].bufYBot >> 8) & 0xFF;
			frameAddr[i][0][3] =
				(pDecInfo->frameBufPool[i].bufYBot >> 0) & 0xFF;
			if (pDecInfo->openParam.cbcrOrder ==
			    CBCR_ORDER_NORMAL) {
				frameAddr[i][1][0] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 24) &
					0xFF;
				frameAddr[i][1][1] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 16) &
					0xFF;
				frameAddr[i][1][2] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 8) &
					0xFF;
				frameAddr[i][1][3] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 0) &
					0xFF;
				frameAddr[i][2][0] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 24) &
					0xFF;
				frameAddr[i][2][1] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 16) &
					0xFF;
				frameAddr[i][2][2] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 8) &
					0xFF;
				frameAddr[i][2][3] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 0) &
					0xFF;
			} else {
				frameAddr[i][2][0] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 24) &
					0xFF;
				frameAddr[i][2][1] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 16) &
					0xFF;
				frameAddr[i][2][2] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 8) &
					0xFF;
				frameAddr[i][2][3] =
					(pDecInfo->frameBufPool[i].bufCbBot >>
					 0) &
					0xFF;
				frameAddr[i][1][0] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 24) &
					0xFF;
				frameAddr[i][1][1] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 16) &
					0xFF;
				frameAddr[i][1][2] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 8) &
					0xFF;
				frameAddr[i][1][3] =
					(pDecInfo->frameBufPool[i].bufCrBot >>
					 0) &
					0xFF;
			}
		}

		VpuWriteMem(pCodecInst->coreIdx, paraBuffer + 384 + 128,
			    (BYTE *)frameAddr, sizeof(frameAddr),
			    VDI_BIG_ENDIAN);

		if (pDecInfo->wtlEnable) {
			int num = pDecInfo->numFbsForDecoding; /* start index of
								  WTL fb array
								*/
			int end = pDecInfo->numFrameBuffers;
			for (i = num; i < end; i++) {
				frameAddr[i - num][0][0] =
					(pDecInfo->frameBufPool[i].bufY >> 24) &
					0xFF;
				frameAddr[i - num][0][1] =
					(pDecInfo->frameBufPool[i].bufY >> 16) &
					0xFF;
				frameAddr[i - num][0][2] =
					(pDecInfo->frameBufPool[i].bufY >> 8) &
					0xFF;
				frameAddr[i - num][0][3] =
					(pDecInfo->frameBufPool[i].bufY >> 0) &
					0xFF;
				if (pDecInfo->openParam.cbcrOrder ==
				    CBCR_ORDER_NORMAL) {
					frameAddr[i - num][1][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 24) &
						0xFF;
					frameAddr[i - num][1][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 16) &
						0xFF;
					frameAddr[i - num][1][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 8) &
						0xFF;
					frameAddr[i - num][1][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 0) &
						0xFF;
					frameAddr[i - num][2][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 24) &
						0xFF;
					frameAddr[i - num][2][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 16) &
						0xFF;
					frameAddr[i - num][2][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 8) &
						0xFF;
					frameAddr[i - num][2][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 0) &
						0xFF;
				} else {
					frameAddr[i - num][2][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 24) &
						0xFF;
					frameAddr[i - num][2][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 16) &
						0xFF;
					frameAddr[i - num][2][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 8) &
						0xFF;
					frameAddr[i - num][2][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 0) &
						0xFF;
					frameAddr[i - num][1][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 24) &
						0xFF;
					frameAddr[i - num][1][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 16) &
						0xFF;
					frameAddr[i - num][1][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 8) &
						0xFF;
					frameAddr[i - num][1][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 0) &
						0xFF;
				}
			}

			VpuWriteMem(pCodecInst->coreIdx,
				    paraBuffer + 384 + 128 + 384,
				    (BYTE *)frameAddr, sizeof(frameAddr),
				    VDI_BIG_ENDIAN);

			if (pDecInfo->wtlMode == FF_FIELD) {
				for (i = num; i < num * 2; i++) {
					frameAddr[i - num][0][0] =
						(pDecInfo->frameBufPool[i]
							 .bufYBot >>
						 24) &
						0xFF;
					frameAddr[i - num][0][1] =
						(pDecInfo->frameBufPool[i]
							 .bufYBot >>
						 16) &
						0xFF;
					frameAddr[i - num][0][2] =
						(pDecInfo->frameBufPool[i]
							 .bufYBot >>
						 8) &
						0xFF;
					frameAddr[i - num][0][3] =
						(pDecInfo->frameBufPool[i]
							 .bufYBot >>
						 0) &
						0xFF;
					if (pDecInfo->openParam.cbcrOrder ==
					    CBCR_ORDER_NORMAL) {
						frameAddr[i - num][1][0] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 24) &
							0xFF;
						frameAddr[i - num][1][1] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 16) &
							0xFF;
						frameAddr[i - num][1][2] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 8) &
							0xFF;
						frameAddr[i - num][1][3] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 0) &
							0xFF;
						frameAddr[i - num][2][0] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 24) &
							0xFF;
						frameAddr[i - num][2][1] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 16) &
							0xFF;
						frameAddr[i - num][2][2] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 8) &
							0xFF;
						frameAddr[i - num][2][3] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 0) &
							0xFF;
					} else {
						frameAddr[i - num][2][0] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 24) &
							0xFF;
						frameAddr[i - num][2][1] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 16) &
							0xFF;
						frameAddr[i - num][2][2] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 8) &
							0xFF;
						frameAddr[i - num][2][3] =
							(pDecInfo->frameBufPool[i]
								 .bufCbBot >>
							 0) &
							0xFF;
						frameAddr[i - num][1][0] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 24) &
							0xFF;
						frameAddr[i - num][1][1] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 16) &
							0xFF;
						frameAddr[i - num][1][2] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 8) &
							0xFF;
						frameAddr[i - num][1][3] =
							(pDecInfo->frameBufPool[i]
								 .bufCrBot >>
							 0) &
							0xFF;
					}
				}
				VpuWriteMem(pCodecInst->coreIdx,
					    paraBuffer + 384 + 128 + 384 + 384,
					    (BYTE *)frameAddr,
					    sizeof(frameAddr), VDI_BIG_ENDIAN);
			}
		}
	}
#ifdef REDUNDENT_CODE
	else {
		if (pDecInfo->wtlEnable) {
			int num = pDecInfo->numFbsForDecoding; /* start index of
								  WTL fb array
								*/
			int end = pDecInfo->numFrameBuffers;
			for (i = num; i < end; i++) {
				frameAddr[i - num][0][0] =
					(pDecInfo->frameBufPool[i].bufY >> 24) &
					0xFF;
				frameAddr[i - num][0][1] =
					(pDecInfo->frameBufPool[i].bufY >> 16) &
					0xFF;
				frameAddr[i - num][0][2] =
					(pDecInfo->frameBufPool[i].bufY >> 8) &
					0xFF;
				frameAddr[i - num][0][3] =
					(pDecInfo->frameBufPool[i].bufY >> 0) &
					0xFF;
				if (pDecInfo->openParam.cbcrOrder ==
				    CBCR_ORDER_NORMAL) {
					frameAddr[i - num][1][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 24) &
						0xFF;
					frameAddr[i - num][1][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 16) &
						0xFF;
					frameAddr[i - num][1][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 8) &
						0xFF;
					frameAddr[i - num][1][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 0) &
						0xFF;
					frameAddr[i - num][2][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 24) &
						0xFF;
					frameAddr[i - num][2][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 16) &
						0xFF;
					frameAddr[i - num][2][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 8) &
						0xFF;
					frameAddr[i - num][2][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 0) &
						0xFF;
				} else {
					frameAddr[i - num][2][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 24) &
						0xFF;
					frameAddr[i - num][2][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 16) &
						0xFF;
					frameAddr[i - num][2][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 8) &
						0xFF;
					frameAddr[i - num][2][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCb >>
						 0) &
						0xFF;
					frameAddr[i - num][1][0] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 24) &
						0xFF;
					frameAddr[i - num][1][1] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 16) &
						0xFF;
					frameAddr[i - num][1][2] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 8) &
						0xFF;
					frameAddr[i - num][1][3] =
						(pDecInfo->frameBufPool[i]
							 .bufCr >>
						 0) &
						0xFF;
				}
			}

			VpuWriteMem(pCodecInst->coreIdx,
				    paraBuffer + 384 + 128 + 384,
				    (BYTE *)frameAddr, sizeof(frameAddr),
				    VDI_BIG_ENDIAN);
		}
	}
#endif

	if (!ConfigSecAXICoda9(pCodecInst->coreIdx, pCodecInst->codecMode,
			       &pDecInfo->secAxiInfo, pDecInfo->stride,
			       pDecInfo->initialInfo.profile & 0xff,
			       pDecInfo->initialInfo.interlace)) {
		SecAxiUse sau;
		sau.u.coda9.useBitEnable = 0;
		sau.u.coda9.useIpEnable = 0;
		sau.u.coda9.useDbkYEnable = 0;
		sau.u.coda9.useDbkCEnable = 0;
		sau.u.coda9.useOvlEnable = 0;
		sau.u.coda9.useBtpEnable = 0;
		VPU_DecGiveCommand((DecHandle)pCodecInst, SET_SEC_AXI, &sau);

		CVI_VC_ERR("ConfigSecAXICoda9 Fail, Don't use Sec AXI\n");

		pDecInfo->secAxiInfo.u.coda9.bufBitUse = 0;
		pDecInfo->secAxiInfo.u.coda9.bufIpAcDcUse = 0;
		pDecInfo->secAxiInfo.u.coda9.bufDbkYUse = 0;
		pDecInfo->secAxiInfo.u.coda9.bufDbkCUse = 0;
		pDecInfo->secAxiInfo.u.coda9.bufOvlUse = 0;
		pDecInfo->secAxiInfo.u.coda9.bufBtpUse = 0;
	}

	for (i = 0; i < pDecInfo->numFrameBuffers; i++) {
		pDecInfo->frameBufPool[i].nv21 =
			pDecInfo->openParam.nv21 &
			pDecInfo->openParam.cbcrInterleave;
	}

	// Tell the decoder how much frame buffers were allocated.
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_BUF_NUM,
		    pDecInfo->numFrameBuffers);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_BUF_STRIDE,
		    pDecInfo->stride);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_BIT_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufBitUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_IPACDC_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufIpAcDcUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_DBKY_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufDbkYUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_DBKC_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufDbkCUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_OVL_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufOvlUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_BTP_ADDR,
		    pDecInfo->secAxiInfo.u.coda9.bufBtpUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_DELAY,
		    pDecInfo->frameDelay);

	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_CACHE_CONFIG,
		    pDecInfo->cacheConfig.type2.CacheMode);

#ifdef REDUNDENT_CODE
	if (pCodecInst->codecMode == VPX_DEC) {
		vpu_buffer_t *pvbSlice = &pDecInfo->vbSlice;
		if (pvbSlice->size == 0) {
			pvbSlice->size = VP8_MB_SAVE_SIZE;
			if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx, pvbSlice,
						0) < 0) {
				return RETCODE_INSUFFICIENT_RESOURCE;
			}
		}
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_MB_BUF_BASE,
			    pvbSlice->phys_addr);
	}
#endif
#if SUPPORT_ASO_FMO
	if ((pCodecInst->codecMode == AVC_DEC) &&
	    (pDecInfo->initialInfo.profile == 66)) {
		vpu_buffer_t *pvbSlice = &pDecInfo->vbSlice;

		if (pvbSlice->size == 0) {
			// default pvbSlice->size = SLICE_SAVE_SIZE, we change to real frame size
			pvbSlice->size =
				((pDecInfo->initialInfo.picHeight *
				  pDecInfo->initialInfo.picWidth * 3) >>
				 2);
			CVI_VC_MEM("pvbSlice->size = 0x%X\n", pvbSlice->size);

			sprintf(ionName, "VDEC_%d_H264_SliceBuf",
				pCodecInst->s32ChnNum);
			if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx, pvbSlice,
						0, ionName) < 0) {
				return RETCODE_INSUFFICIENT_RESOURCE;
			}
		}
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_SLICE_BB_START,
			    pvbSlice->phys_addr);
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_SLICE_BB_SIZE,
			    (pvbSlice->size >> 10));
	}
#endif
	if (pCodecInst->productId == PRODUCT_ID_980) {
		val = 0;
		val |= (pDecInfo->openParam.bwbEnable << 15);
		val |= (pDecInfo->wtlMode << 17) |
		       (pDecInfo->tiled2LinearMode << 13) |
		       (pDecInfo->mapType << 9) | (FORMAT_420 << 6);
		val |= ((pDecInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= pDecInfo->openParam.frameEndian;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);
#ifdef REDUNDENT_CODE
	} else if (pCodecInst->productId == PRODUCT_ID_960) {
		val = 0;
		val |= (pDecInfo->wtlEnable << 17);
		val |= (pDecInfo->openParam.bwbEnable << 12);
		if (pDecInfo->mapType) {
			if (pDecInfo->mapType == TILED_FRAME_MB_RASTER_MAP ||
			    pDecInfo->mapType == TILED_FIELD_MB_RASTER_MAP)
				val |= (pDecInfo->tiled2LinearEnable << 11) |
				       (0x03 << 9) | (FORMAT_420 << 6);
			else
				val |= (pDecInfo->tiled2LinearEnable << 11) |
				       (0x02 << 9) | (FORMAT_420 << 6);
		}
		val |= ((pDecInfo->openParam.cbcrInterleave)
			<< 2); // Interleave
		// bit
		// position
		// is
		// modified
		val |= pDecInfo->openParam.frameEndian;
		VpuWriteReg(pCodecInst->coreIdx, BIT_FRAME_MEM_CTRL, val);
#endif
	} else {
		return RETCODE_NOT_FOUND_VPU_DEVICE;
	}

	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_MAX_DEC_SIZE,
		    0); // Must set to zero at API 2.1.5 version
	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, SET_FRAME_BUF);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, SET_FRAME_BUF, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, SET_FRAME_BUF, 0);
#endif

	if (VpuReadReg(pCodecInst->coreIdx, RET_SET_FRAME_SUCCESS) &
	    (1UL << 31)) {
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuDecFlush(CodecInst *instance,
			 FramebufferIndex *framebufferIndexes, Uint32 size)
{
	Uint32 i;
	DecInfo *pDecInfo = &instance->CodecInfo->decInfo;

	Coda9BitIssueCommand(instance->coreIdx, instance, DEC_BUF_FLUSH);
	if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}

	pDecInfo->frameDisplayFlag = 0;

	if (framebufferIndexes != NULL) {
		for (i = 0; i < size; i++) {
			framebufferIndexes[i].linearIndex = -2;
			framebufferIndexes[i].tiledIndex = -2;
		}
	}

	return RETCODE_SUCCESS;
}

/************************************************************************/
/* Encoder                                                              */
/************************************************************************/

RetCode Coda9VpuBuildUpEncParam(CodecInst *pCodec, EncOpenParam *param)
{
	RetCode ret = RETCODE_SUCCESS;
	Uint32 coreIdx;
	Int32 productId;
	EncInfo *pEncInfo = &pCodec->CodecInfo->encInfo;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	coreIdx = pCodec->coreIdx;
	productId = Coda9VpuGetProductId(coreIdx);

	ret = SetupEncCodecInstance(productId, pCodec);
	if (ret != RETCODE_SUCCESS)
		return ret;

#ifdef REDUNDENT_CODE
	if (param->bitstreamFormat == STD_MPEG4 ||
	    param->bitstreamFormat == STD_H263)
		pCodec->codecMode = MP4_ENC;
	else
#endif
		if (param->bitstreamFormat == STD_AVC)
		pCodec->codecMode = AVC_ENC;

	if (param->bitstreamFormat == STD_AVC &&
	    param->EncStdParam.avcParam.mvcExtension)
		pCodec->codecModeAux = AVC_AUX_MVC;
	else
		pCodec->codecModeAux = 0;

	if (param->bitstreamFormat == STD_AVC &&
	    param->EncStdParam.avcParam.svcExtension)
		pCodec->codecModeAux = AVC_AUX_SVC;

	if (productId == PRODUCT_ID_980) {
		pEncInfo->ActivePPSIdx = 0;
		pEncInfo->frameIdx = 0;
		pEncInfo->fieldDone = 0;
	}

	pEncInfo->vbWork.size = WORK_BUF_SIZE;
	CVI_VC_MEM("vbWork.size = 0x%X\n", pEncInfo->vbWork.size);

	sprintf(ionName, "VENC_%d_H264_WorkBuffer", pCodec->s32ChnNum);
	if (VDI_ALLOCATE_MEMORY(pCodec->coreIdx, &pEncInfo->vbWork, 0,
				ionName) < 0)
		return RETCODE_INSUFFICIENT_RESOURCE;

	pEncInfo->streamRdPtr = param->bitstreamBuffer;
	pEncInfo->streamWrPtr = param->bitstreamBuffer;
	CVI_VC_TRACE("bitstreamBuffer = 0x%llX\n", param->bitstreamBuffer);
	CVI_VC_TRACE("streamRdPtr = 0x%llX, streamWtPtr = 0x%llX\n",
		     pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);
	pEncInfo->lineBufIntEn = param->lineBufIntEn;
	pEncInfo->bEsBufQueueEn = param->bEsBufQueueEn;
	pEncInfo->streamBufStartAddr = param->bitstreamBuffer;
	pEncInfo->streamBufSize = param->bitstreamBufferSize;
	pEncInfo->streamBufEndAddr =
		param->bitstreamBuffer + param->bitstreamBufferSize;
	pEncInfo->stride = 0;
	pEncInfo->vbFrame.size = 0;
	pEncInfo->vbPPU.size = 0;
	pEncInfo->frameAllocExt = 0;
	pEncInfo->ppuAllocExt = 0;
	pEncInfo->secAxiInfo.u.coda9.useBitEnable = 0;
	pEncInfo->secAxiInfo.u.coda9.useIpEnable = 0;
	pEncInfo->secAxiInfo.u.coda9.useDbkYEnable = 0;
	pEncInfo->secAxiInfo.u.coda9.useDbkCEnable = 0;
	pEncInfo->secAxiInfo.u.coda9.useOvlEnable = 0;
	pEncInfo->rotationEnable = 0;
	pEncInfo->mirrorEnable = 0;
	pEncInfo->mirrorDirection = MIRDIR_NONE;
	pEncInfo->rotationAngle = 0;
	pEncInfo->initialInfoObtained = 0;
	pEncInfo->ringBufferEnable = param->ringBufferEnable;
	pEncInfo->linear2TiledEnable = param->linear2TiledEnable;
	pEncInfo->linear2TiledMode = param->linear2TiledMode; // coda980 only

	if (pEncInfo->linear2TiledEnable == TRUE)
		pEncInfo->mapType = TILED_FRAME_V_MAP;

	if (!pEncInfo->linear2TiledEnable)
		pEncInfo->linear2TiledMode = 0;

	/* Maverick Cache I */
	osal_memset((void *)&pEncInfo->cacheConfig, 0x00,
		    sizeof(MaverickCacheConfig));

#ifdef REDUNDENT_CODE
	if (productId == PRODUCT_ID_960) {
		pEncInfo->dramCfg.bankBit = EM_BANK;
		pEncInfo->dramCfg.casBit = EM_CAS;
		pEncInfo->dramCfg.rasBit = EM_RAS;
		pEncInfo->dramCfg.busBit = EM_WIDTH;
	}
#endif

	return ret;
}

#ifdef SUPPORT_980_ROI_RC_LIB
static void set_rps(gop_entry_t *gop, int gop_size,
		    // OUT
		    rps_t rps[MAX_GOP_SIZE])
{
	int curr;

	for (curr = 0; curr < gop_size; curr++)
		rps[curr].num_poc = 0;

	for (curr = 0; curr < gop_size; curr++) {
		int next;
		int last_idx = -1;
		int curr_poc = gop[curr].curr_poc;

		// find the last frame that uses the current frame as reference
		// in encoding order
		for (next = curr + 1; next < 3 * gop_size; next++) {
			gop_entry_t *g = &gop[next % gop_size];
			int gop_poc = next / gop_size * gop_size;

			if (g->ref_poc + gop_poc == curr_poc) // ref0
				last_idx = next;
		}

		// set nal_ref_idc
		rps[curr].used_for_ref = (last_idx != -1);

		// set rps
		for (next = curr + 1; next <= last_idx; next++) {
			rps_t *r = &rps[next % gop_size];
			int gop_poc = next / gop_size * gop_size;

			r->poc[r->num_poc++] = curr_poc - gop_poc;
		}
	}
}
static void set_extra_gop_entry(gop_entry_t *gop, int gop_size)
{
	int i;
	gop_entry_t *src = gop;
	gop_entry_t *des = gop + gop_size;

	for (i = 0; i < gop_size; i++) {
		des->curr_poc = i + 1;
		des->ref_poc = i;
		des->qp_offset = src->qp_offset;
		des++;
		src++;
	}
}

static int get_num_ref_frames(rps_t *rps, int gop_size, int use_long_term)
{
	int i;
	int num_ref_frames = 0;

	for (i = 0; i < gop_size; i++) {
		num_ref_frames = MAX(rps[i].num_poc, num_ref_frames);
	}
	if (use_long_term)
		num_ref_frames += 1;

	return num_ref_frames;
}
static void get_num_reorder_frames(gop_entry_t *gop, int gop_size,
				   int *max_num_reorder_pic,
				   int32_t *src_latency)
{
	int curr;
	int max_num_reorder = 0;
	int enc_order, latency;
	int max_latency_pic = 0;

	for (curr = 0; curr < gop_size; curr++) {
		int prev;
		int num_reorder = 0;
		for (prev = 0; prev < curr; prev++) // for all frames that
		// precedes frame i in
		// encoding order
		{
			if (gop[prev].curr_poc > gop[curr].curr_poc) // follows
				// frame i
				// in output
				// order
				num_reorder++;
		}
		enc_order = curr + 1;
		latency = gop[curr].curr_poc - enc_order;

		max_num_reorder = MAX(num_reorder, max_num_reorder);
		max_latency_pic = MAX(latency, max_latency_pic);
	}
	*src_latency = max_latency_pic;
	*max_num_reorder_pic = max_num_reorder;
}

static int get_max_dec_buffering(gop_entry_t gop[MAX_GOP_SIZE],
				 rps_t rps[MAX_GOP_SIZE], int gop_size,
				 int use_long_term)
{
	int curr;
	int max_dec_buffering = 0;

	for (curr = 0; curr < gop_size; curr++) {
		int prev;
		int dec_buffering = rps[curr].num_poc;

		// find reordered frame that is not used for reference
		for (prev = 0; prev < curr; prev++) {
			int is_ref_frm = 0;

			if (gop[prev].curr_poc > gop[curr].curr_poc) // it is
			// reordered
			// frame
			{
				int i;
				for (i = 0; i < rps[curr].num_poc; i++) {
					if (gop[prev].curr_poc ==
					    rps[curr].poc[i])
						is_ref_frm = 1;
				}
				if (!is_ref_frm) // not used for reference
					dec_buffering++;
			}
		}
		max_dec_buffering = MAX(dec_buffering, max_dec_buffering);
	}
	if (use_long_term)
		max_dec_buffering += 1;

	return max_dec_buffering;
}

// when temporal scalability is enabled,
// decoder needs more num_ref_frames than encoder due to non-existing frames.
static int get_num_ref_frames_for_decoder(
	// dpb_t *dpb,
	EncInfo *avc, int use_long_term)
{
	int num_ref_frames_decoder = avc->num_ref_frame;

	//  if (avc->max_temporal_id > 0)
	{
		int enc_idx;
		int frmnum = 0;
		uint8_t poc_to_frmnum[2 * MAX_GOP_SIZE + 1];

		poc_to_frmnum[0] = 0;

		for (enc_idx = 0; enc_idx < 2 * avc->gop_size; enc_idx++) {
			int i;
			int enc_idx_mod = enc_idx % avc->gop_size;
			int gop_poc = enc_idx / avc->gop_size * avc->gop_size;
			int poc =
				avc->gop_entry[enc_idx_mod].curr_poc + gop_poc;
			rps_t *rps = &avc->rps[enc_idx_mod];

			if (rps->used_for_ref)
				frmnum++;

			// set table for poc to frame_num
			// assert(poc < sizeof(poc_to_frmnum));
			poc_to_frmnum[poc] = frmnum;

			// calculate difference of frame_num between current
			// frame and ref frame
			for (i = 0; i < rps->num_poc; i++) {
				int ref_poc = gop_poc + rps->poc[i];
				if (ref_poc >= 0) {
					int frmnum_diff;
					// assert(ref_poc <
					// sizeof(poc_to_frmnum)); +1 for non_ref
					//frame because frmnum itself is ref
					//frame
					frmnum_diff = frmnum -
						      poc_to_frmnum[ref_poc] +
						      !rps->used_for_ref;
					num_ref_frames_decoder = MAX(
						num_ref_frames_decoder,
						frmnum_diff + use_long_term);
				}
			}
		}
	}

	return num_ref_frames_decoder;
}

static int get_max_dec_buffering_for_decoder(
	// dpb_t *dpb)
	EncInfo *avc)
{
	int extra_num_ref_frames =
		avc->num_ref_frames_decoder - avc->num_ref_frame;
	int max_dec_buffering_decoder =
		avc->max_dec_buffering + extra_num_ref_frames;

	return max_dec_buffering_decoder;
}

static int calc_max_temporal_id(
	// dpb_t *dpb)
	EncInfo *avc)
{
	int i;
	int max_temporal_id = 0;

	if (avc->gop_size != 0) {
		for (i = 0; i < avc->gop_size; i++)
			max_temporal_id = MAX(max_temporal_id,
					      avc->gop_entry[i].temporal_id);
	}

	return max_temporal_id;
}

static void enc_init_header(EncInfo *pEncInfo, gop_entry_t *gop_entry_in,
			    int gopSize, int setAsLongTermPeriod,
			    int set_dqp_pic_num)
{
	int i;

	osal_memcpy(pEncInfo->gop_entry, gop_entry_in,
		    sizeof(gop_entry_t) * MAX_GOP_SIZE);

	pEncInfo->prev_idx = -1;
	pEncInfo->gop_size = set_dqp_pic_num;
	pEncInfo->enc_idx_modulo = -1;
	pEncInfo->src_idx = 0;
	pEncInfo->enc_idx_gop = 0;
	pEncInfo->src_idx_last_idr = 0;
	pEncInfo->prev_frame_num = 0;
	pEncInfo->use_long_term_seq = setAsLongTermPeriod > 0 ? 1 : 0;
	pEncInfo->curr_long_term = 0;
	pEncInfo->longterm_frmidx = -1;
	pEncInfo->ref_long_term = 0;
	pEncInfo->Idr_picId = -1;
	pEncInfo->long_term_period = pEncInfo->openParam.LongTermPeriod;
	pEncInfo->virtual_i_period = pEncInfo->openParam.VirtualIPeriod;

	pEncInfo->Idr_cnt = 0;
	pEncInfo->frm_cnt = 0;
	pEncInfo->enc_long_term_cnt = 0;

	for (i = 0; i < 31; i++)
		pEncInfo->frm[i].used_for_ref = 0;

	if (gopSize == 1) { // all-Intra
		pEncInfo->num_ref_frame = 0;
		pEncInfo->max_dec_buffering = 1;
		pEncInfo->gop_size = 1;
		pEncInfo->rps[0].used_for_ref = 1;
		pEncInfo->rps[0].num_poc = 1;
		pEncInfo->gop_entry[0].curr_poc = 1;
		pEncInfo->gop_entry[0].temporal_id = 0;
	} else {
		set_extra_gop_entry(pEncInfo->gop_entry, pEncInfo->gop_size);
		set_rps(pEncInfo->gop_entry, pEncInfo->gop_size, pEncInfo->rps);
		set_rps(&pEncInfo->gop_entry[pEncInfo->gop_size],
			pEncInfo->gop_size, &pEncInfo->rps[pEncInfo->gop_size]);
		pEncInfo->num_ref_frame =
			get_num_ref_frames(pEncInfo->rps, pEncInfo->gop_size,
					   pEncInfo->use_long_term_seq);
		get_num_reorder_frames(pEncInfo->gop_entry, pEncInfo->gop_size,
				       &pEncInfo->num_reorder_frames,
				       &pEncInfo->max_latency_pictures);
		pEncInfo->max_dec_buffering =
			get_max_dec_buffering(pEncInfo->gop_entry,
					      pEncInfo->rps, pEncInfo->gop_size,
					      pEncInfo->use_long_term_seq);
	}
	pEncInfo->max_temporal_id = calc_max_temporal_id(pEncInfo);
	pEncInfo->num_ref_frames_decoder = get_num_ref_frames_for_decoder(
		pEncInfo, pEncInfo->use_long_term_seq);
	pEncInfo->max_dec_buffering_decoder =
		get_max_dec_buffering_for_decoder(pEncInfo);

	pEncInfo->num_total_frames = pEncInfo->num_ref_frame + 1;
}
#endif /* SUPPORT_980_ROI_RC_LIB */

RetCode Coda9VpuEncSetup(CodecInst *pCodecInst)
{
	Int32 picWidth, picHeight;
	Int32 data, val;
	Int32 productId, rcEnable;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;

	rcEnable = pEncInfo->openParam.rcEnable & 0xf;

	//=====================================================================
	// using RC library,
	//   1. calculate non-roi QP considering target bitrate
	//		when rc mode == 4 && roi enabled.
	//   2. calculate picQp (rc mode==4). if rc mode is 4(picture level RC)
	//   && roi disabled
	//=====================================================================

	// to calculate real encoded bit when ringbuffer enabled.
	pEncInfo->prevWrPtr = pEncInfo->streamBufStartAddr;

	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		rcLibSetupRc(pCodecInst);
	}

	if (pEncInfo->openParam.bitstreamFormat == STD_AVC &&
	    pCodecInst->codecModeAux == AVC_AUX_SVC)
#ifdef SUPPORT_980_ROI_RC_LIB
		enc_init_header(pEncInfo, pEncInfo->openParam.gopEntry,
				pEncInfo->openParam.gopSize,
				pEncInfo->openParam.LongTermPeriod,
				pEncInfo->openParam.set_dqp_pic_num);
#endif

	productId = pCodecInst->productId;
	picWidth = pEncInfo->openParam.picWidth;
	picHeight = pEncInfo->openParam.picHeight;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_BB_START,
		    pEncInfo->streamBufStartAddr);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_BB_SIZE,
		    pEncInfo->streamBufSize >> 10); // size in KB

	// Rotation Left 90 or 270 case : Swap XY resolution for VPU internal
	// usage
	if (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270)
		data = (picHeight << 16) | picWidth;
	else
		data = (picWidth << 16) | picHeight;

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_SRC_SIZE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_SRC_F_RATE,
		    pEncInfo->openParam.frameRateInfo);

#ifdef REDUNDENT_CODE
	if (pEncInfo->openParam.bitstreamFormat == STD_MPEG4) {
		CVI_VC_TRACE("bitstreamFormat = 0x%X\n",
			     pEncInfo->openParam.bitstreamFormat);

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_COD_STD, 3);
		data = pEncInfo->openParam.EncStdParam.mp4Param.mp4IntraDcVlcThr
			       << 2 |
		       pEncInfo->openParam.EncStdParam.mp4Param
				       .mp4ReversibleVlcEnable
			       << 1 |
		       pEncInfo->openParam.EncStdParam.mp4Param
			       .mp4DataPartitionEnable;

		data |= ((pEncInfo->openParam.EncStdParam.mp4Param.mp4HecEnable >
			  0) ?
				       1 :
				       0)
			<< 5;
		data |= ((pEncInfo->openParam.EncStdParam.mp4Param.mp4Verid ==
			  2) ?
				       0 :
				       1)
			<< 6;

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_MP4_PARA, data);

		if (productId == PRODUCT_ID_980)
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				    (VPU_ME_LINEBUFFER_MODE << 9) |
					    (pEncInfo->openParam.meBlkMode
					     << 5) |
					    (pEncInfo->openParam.MEUseZeroPmv
					     << 4) |
					    (pEncInfo->openParam.MESearchRangeY
					     << 2) |
					    pEncInfo->openParam.MESearchRangeX);
		else
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				    (pEncInfo->openParam.meBlkMode << 3) |
					    (pEncInfo->openParam.MEUseZeroPmv
					     << 2) |
					    pEncInfo->openParam.MESearchRange);
	} else if (pEncInfo->openParam.bitstreamFormat == STD_H263) {
		CVI_VC_TRACE("bitstreamFormat = 0x%X\n",
			     pEncInfo->openParam.bitstreamFormat);

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_COD_STD, 11);
		data = pEncInfo->openParam.EncStdParam.h263Param.h263AnnexIEnable
			       << 3 |
		       pEncInfo->openParam.EncStdParam.h263Param.h263AnnexJEnable
			       << 2 |
		       pEncInfo->openParam.EncStdParam.h263Param.h263AnnexKEnable
			       << 1 |
		       pEncInfo->openParam.EncStdParam.h263Param
			       .h263AnnexTEnable;
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_263_PARA, data);
		if (productId == PRODUCT_ID_980)
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				    (VPU_ME_LINEBUFFER_MODE << 9) |
					    (pEncInfo->openParam.meBlkMode
					     << 5) |
					    (pEncInfo->openParam.MEUseZeroPmv
					     << 4) |
					    (pEncInfo->openParam.MESearchRangeY
					     << 2) |
					    pEncInfo->openParam.MESearchRangeX);
		else
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				    (pEncInfo->openParam.meBlkMode << 3) |
					    (pEncInfo->openParam.MEUseZeroPmv
					     << 2) |
					    pEncInfo->openParam.MESearchRange);
	} else
#endif
		if (pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		CVI_VC_CFG("bitstreamFormat = 0x%X\n",
			   pEncInfo->openParam.bitstreamFormat);

		if (productId == PRODUCT_ID_980) {
			int SliceNum = 0;
			AvcPpsParam *ActivePPS =
				&pEncInfo->openParam.EncStdParam.avcParam
					 .ppsParam[pEncInfo->ActivePPSIdx];
			if (ActivePPS->transform8x8Mode == 1 ||
			    pEncInfo->openParam.EncStdParam.avcParam
					    .chromaFormat400 == 1)
				pEncInfo->openParam.EncStdParam.avcParam
					.profile = CVI_H264E_PROFILE_HIGH;
			else if (ActivePPS->entropyCodingMode != 0 ||
				 pEncInfo->openParam.EncStdParam.avcParam
						 .fieldFlag == 1)
				pEncInfo->openParam.EncStdParam.avcParam
					.profile = CVI_H264E_PROFILE_MAIN;
			else
				pEncInfo->openParam.EncStdParam.avcParam
					.profile = CVI_H264E_PROFILE_BASELINE;

			if (pEncInfo->openParam.sliceMode.sliceMode == 1 &&
			    pEncInfo->openParam.sliceMode.sliceSizeMode == 1)
				SliceNum =
					pEncInfo->openParam.sliceMode.sliceSize;

			if (!pEncInfo->openParam.EncStdParam.avcParam.level) {
				if (pEncInfo->openParam.EncStdParam.avcParam
					    .fieldFlag)
					pEncInfo->openParam.EncStdParam.avcParam
						.level = LevelCalculation(
						picWidth >> 4,
						(picHeight + 31) >> 5,
						pEncInfo->openParam
							.frameRateInfo,
						1, pEncInfo->openParam.bitRate,
						SliceNum);
				else
					pEncInfo->openParam.EncStdParam.avcParam
						.level = LevelCalculation(
						picWidth >> 4, picHeight >> 4,
						pEncInfo->openParam
							.frameRateInfo,
						0, pEncInfo->openParam.bitRate,
						SliceNum);
				if (pEncInfo->openParam.EncStdParam.avcParam
					    .level < 0) {
					CVI_VC_ERR(
						"avcParam.level %d not support\n",
						pEncInfo->openParam.EncStdParam
							.avcParam.level);
					return RETCODE_INVALID_PARAM;
				}
			}

			VpuWriteReg(
				pCodecInst->coreIdx, CMD_ENC_SEQ_COD_STD,
				(pEncInfo->openParam.EncStdParam.avcParam.profile
				 << 4) | 0x0);

		} else {
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_COD_STD,
				    0x0);
		}

		data = (pEncInfo->openParam.EncStdParam.avcParam
				.deblkFilterOffsetBeta &
			15) << 12 |
		       (pEncInfo->openParam.EncStdParam.avcParam
				.deblkFilterOffsetAlpha &
			15) << 8 |
		       pEncInfo->openParam.EncStdParam.avcParam.disableDeblk
			       << 6 |
		       pEncInfo->openParam.EncStdParam.avcParam
				       .constrainedIntraPredFlag
			       << 5 |
		       (pEncInfo->openParam.EncStdParam.avcParam.chromaQpOffset &
			31);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_264_PARA, data);

#if ENABLE_RC_LIB
		if (productId == PRODUCT_ID_980) {
			VpuWriteReg(
				pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				(VPU_ME_LINEBUFFER_MODE << 9) |
					(pEncInfo->openParam.meBlkMode << 5) |
					(pEncInfo->openParam.MEUseZeroPmv
					 << 4) |
#ifdef CVI_SEARCH_RANGE
					(pEncInfo->openParam.MESearchRangeY
					 << 2) |
					pEncInfo->openParam.MESearchRangeX
#else
					(2 << 2) | 3
#endif
			);

			CVI_VC_CFG("CMD_ENC_SEQ_ME_OPTION = 0x%X\n",
				   VpuReadReg(pCodecInst->coreIdx,
					      CMD_ENC_SEQ_ME_OPTION));
		} else {
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_ME_OPTION,
				    (pEncInfo->openParam.meBlkMode << 3) |
					    (pEncInfo->openParam.MEUseZeroPmv
					     << 2) |
					    pEncInfo->openParam.MESearchRange);
		}
#endif
	}

	if (productId == PRODUCT_ID_980) {
		data = 0;
		if (pEncInfo->openParam.sliceMode.sliceMode != 0) {
			data = pEncInfo->openParam.sliceMode.sliceSize << 2 |
			       (pEncInfo->openParam.sliceMode.sliceSizeMode +
				1); // encoding mode 0,1,2
		}
	}
#ifdef REDUNDENT_CODE
	else {
		data = pEncInfo->openParam.sliceMode.sliceSize << 2 |
		       pEncInfo->openParam.sliceMode.sliceSizeMode << 1 |
		       pEncInfo->openParam.sliceMode.sliceMode;
	}
#endif
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_SLICE_MODE, data);
	CVI_VC_CFG("rcEnable = %d\n", rcEnable);

	if (rcEnable) { // rate control enabled
		if (productId == PRODUCT_ID_980) {
			if (pEncInfo->openParam.bitstreamFormat == STD_AVC) {
				int MinDeltaQp, MaxDeltaQp, QpMinI, QpMaxI,
					QpMinP, QpMaxP;

				data = (pEncInfo->openParam.idrInterval << 21) |
				       (pEncInfo->openParam.rcGopIQpOffsetEn
					<< 20) |
				       ((pEncInfo->openParam.rcGopIQpOffset &
					 0xF)
					<< 16) |
				       pEncInfo->openParam.gopSize;

				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_GOP_NUM, data);

				data = (pEncInfo->openParam.frameSkipDisable
					<< 31) |
				       (pEncInfo->openParam.rcInitDelay << 16) |
				       (pEncInfo->openParam.RcHvsMaxDeltaQp);

				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_RC_PARA, data);

				data = (pEncInfo->openParam.HvsQpScaleDiv2
					<< 25) |
				       (pEncInfo->openParam.EnHvsQp << 24) |
				       (pEncInfo->openParam.EnRowLevelRc
					<< 23) |
				       (pRcInfo->targetBitrate << 4) |
				       (rcEnable);
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_RC_PARA2, data);

				data = 0;
				if (pEncInfo->openParam.maxIntraSize > 0) {
					data = (1 << 16) |
					       (pEncInfo->openParam.maxIntraSize &
						0xFFFF);
				}

				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_RC_MAX_INTRA_SIZE,
					    data);

				if (pEncInfo->openParam.userQpMinI < 0)
					QpMinI = 0;
				else
					QpMinI = (1 << 6) |
						 pEncInfo->openParam.userQpMinI;

				if (pEncInfo->openParam.userQpMaxI < 0)
					QpMaxI = 0;
				else
					QpMaxI = (1 << 6) |
						 pEncInfo->openParam.userQpMaxI;

				if (pEncInfo->openParam.userQpMinP < 0)
					QpMinP = 0;
				else
					QpMinP = (1 << 6) |
						 pEncInfo->openParam.userQpMinP;

				if (pEncInfo->openParam.userQpMaxP < 0)
					QpMaxP = 0;
				else
					QpMaxP = (1 << 6) |
						 pEncInfo->openParam.userQpMaxP;

				data = QpMinP << 24 | QpMaxP << 16 |
				       QpMinI << 8 | QpMaxI;
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_QP_RANGE_SET, data);

				if (pEncInfo->openParam.userMinDeltaQp < 0)
					MinDeltaQp = 0;
				else
					MinDeltaQp = (1 << 6) |
						     pEncInfo->openParam
							     .userMinDeltaQp;

				if (pEncInfo->openParam.userMaxDeltaQp < 0)
					MaxDeltaQp = 0;
				else
					MaxDeltaQp = (1 << 6) |
						     pEncInfo->openParam
							     .userMaxDeltaQp;

				data = MinDeltaQp << 8 | MaxDeltaQp;
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_DELTA_QP, data);
			}
#ifdef REDUNDENT_CODE
			else { // MP4
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_GOP_NUM,
					    pEncInfo->openParam.gopSize);
				data = (pEncInfo->openParam.frameSkipDisable)
					       << 31 |
				       pEncInfo->openParam.rcInitDelay << 16;
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_RC_PARA, data);
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_QP_RANGE_SET, 0);

				data = (pEncInfo->openParam.bitRate << 4) | 1;
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_SEQ_RC_PARA2, data);
			}
#endif
		}
#ifdef REDUNDENT_CODE
		else {
			/* coda960 ENCODER */
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_GOP_NUM,
				    pEncInfo->openParam.gopSize);

			data = (pEncInfo->openParam.frameSkipDisable) << 31 |
			       pEncInfo->openParam.rcInitDelay << 16 |
			       pEncInfo->openParam.bitRate << 1 | 1;
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_PARA,
				    data);
		}
#endif
	} else {
		if (pEncInfo->openParam.bitstreamFormat == STD_AVC) {
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_GOP_NUM,
				    (pEncInfo->openParam.idrInterval << 21) |
					    pEncInfo->openParam.gopSize);
		} else
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_GOP_NUM,
				    pEncInfo->openParam.gopSize);

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_PARA, 0);

		if (productId == PRODUCT_ID_980) {
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_SEQ_QP_RANGE_SET, 0);
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_PARA2,
				    0);
		}
	}

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_BUF_SIZE,
		    pEncInfo->openParam.vbvBufferSize);
	data = pEncInfo->openParam.intraRefreshNum |
	       pEncInfo->openParam.ConscIntraRefreshEnable << 16 |
	       pEncInfo->openParam.CountIntraMbEnable << 17 |
	       pEncInfo->openParam.FieldSeqIntraRefreshEnable << 18;
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_INTRA_REFRESH, data);

	data = 0;
	if (pEncInfo->openParam.rcIntraQp >= 0) {
		data = (1 << 5);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_INTRA_QP,
			    pEncInfo->openParam.rcIntraQp);
	} else {
		data = 0;
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_INTRA_QP,
			    (Uint32)-1);
	}

	if (pEncInfo->openParam.userQpMax >= 0) {
		data |= (1 << 6);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_QP_MAX,
			    pEncInfo->openParam.userQpMax);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_QP_MAX, 0);
	}

	if (pEncInfo->openParam.userGamma >= 0) {
		data |= (1 << 7);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_GAMMA,
			    pEncInfo->openParam.userGamma);
	} else {
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_GAMMA, 0);
	}

	if (pCodecInst->codecMode == AVC_ENC) {
		data |= (pEncInfo->openParam.EncStdParam.avcParam.audEnable
			 << 2);

		if (pCodecInst->codecModeAux == AVC_AUX_MVC) {
			data |= (pEncInfo->openParam.EncStdParam.avcParam
					 .interviewEn
				 << 4);
			data |= (pEncInfo->openParam.EncStdParam.avcParam
					 .parasetRefreshEn
				 << 8);
			data |= (pEncInfo->openParam.EncStdParam.avcParam
					 .prefixNalEn
				 << 9);
		}

		if (productId == PRODUCT_ID_980) {
			data |= pEncInfo->openParam.EncStdParam.avcParam
					.fieldFlag
				<< 10;
			data |= pEncInfo->openParam.EncStdParam.avcParam
					.fieldRefMode
				<< 11;
		}
	}

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_OPTION, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_RC_INTERVAL_MODE,
		    (pEncInfo->openParam.mbInterval << 2) |
			    pEncInfo->openParam.rcIntervalMode);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_INTRA_WEIGHT,
		    pEncInfo->openParam.intraCostWeight);

	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
		    pEncInfo->streamWrPtr);
	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
		    pEncInfo->streamRdPtr);
	CVI_VC_TRACE("streamRdPtr = 0x%llX, streamWtPtr = 0x%llX\n",
		     pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);

	SetEncFrameMemInfo(pCodecInst);

	val = 0;
	if (pEncInfo->ringBufferEnable == 0) {
		if (pEncInfo->lineBufIntEn)
			val |= (0x1 << 6);
#ifndef PLATFORM_NON_OS
		val |= (0x1 << 5);
		val |= (0x1 << 4);
#endif
	} else {
		val |= (0x1 << 3);
	}
	val |= pEncInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENC_SEQ_INIT);

	if (vdi_wait_interrupt(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT, NULL) ==
	    -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENC_SEQ_INIT, 2);
#endif
		CVI_VC_ERR("ENC_SEQ_INIT RESPONSE_TIMEOUT\n");
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	VpuWriteReg(pCodecInst->coreIdx, BIT_INT_CLEAR,
		    1); // that is OK. HW signal already is clear by device
	// driver
	VpuWriteReg(pCodecInst->coreIdx, BIT_INT_REASON, 0);

#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENC_SEQ_INIT, 0);
#endif

	if (VpuReadReg(pCodecInst->coreIdx, RET_ENC_SEQ_END_SUCCESS) &
	    (1UL << 31)) {
		CVI_VC_ERR("MEMORY_ACCESS_VIOLATION\n");
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

	if (VpuReadReg(pCodecInst->coreIdx, RET_ENC_SEQ_END_SUCCESS) == 0) {
		CVI_VC_ERR("RET_ENC_SEQ_END_SUCCESS fail\n");
		return RETCODE_FAILURE;
	}

	pEncInfo->streamWrPtr =
		VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
	pEncInfo->streamEndflag =
		VpuReadReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM);
	CVI_VC_TRACE("streamRdPtr = 0x%llX, streamWtPtr = 0x%llX\n",
		     pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);

	return RETCODE_SUCCESS;
}

void rcLibSetupRc(CodecInst *pCodecInst)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;
	int gamma = pEncInfo->openParam.userGamma;
#ifdef CLIP_PIC_DELTA_QP
	int max_delta_qp_minus = pEncInfo->openParam.userMinDeltaQp;
	int max_delta_qp_plus = pEncInfo->openParam.userMaxDeltaQp;
#endif
#if ENABLE_RC_LIB
	int is_first_pic = (pEncInfo->frameIdx == 0);
	int gop_size = pEncInfo->openParam.gopSize;
#if 1
	int buf_size_ms = pEncInfo->openParam.rcInitDelay; //  * bps;
#else
	int buf_size_ms = pEncInfo->openParam.statTime * 1000; //  * bps;
#endif
	int bps = pRcInfo->targetBitrate * 1000;
	int frame_rate = (pEncInfo->openParam.frameRateInfo & 0xffff) /
			 ((pEncInfo->openParam.frameRateInfo >> 16) + 1);
	int pic_width, pic_height;
	Int32 rcEnable;

	UNUSED(pRcInfo);
	UNUSED(is_first_pic);
	UNUSED(buf_size_ms);
	UNUSED(bps);
	if (pCodecInst->codecMode == HEVC_ENC) {
		/*
		For current HEVC, FW does all the RC-releated jobs.
		Some features are not supported, like maxIprop.
		By using LLRC, user can develop thier own RC algorithm.
		We open LLRC interface as UBR mode. User needs to set frame Qp & target
		bits of each frame by using UBR API.
		However, if bTestUbrEn is enabled, this means we'd like to use rcLib to
		do CBR, and send Qp & target bits to test UBR API.
		By using LLRC, We also can replace original CBR with new developed CBR.
		In this way, maxIprop could be supported.
		*/
		rcEnable = 4;
	} else {
		rcEnable = pEncInfo->openParam.rcEnable & 0xf;
	}

	CVI_VC_CFG("rcEnable = %d, bps = %d, frame_rate = %d\n", rcEnable, bps,
		   frame_rate);

	if (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270) {
		pic_width = pEncInfo->openParam.picHeight;
		pic_height = pEncInfo->openParam.picWidth;
	} else {
		pic_width = pEncInfo->openParam.picWidth;
		pic_height = pEncInfo->openParam.picHeight;
	}

	if (pEncInfo->openParam.EncStdParam.avcParam.fieldFlag) {
		pic_height = pic_height >> 1;
		frame_rate = frame_rate << 1;
		if (gop_size != 1)
			gop_size = pEncInfo->openParam.gopSize << 1;
	}
#endif

	if (gamma == -1)
		gamma = 1;

#ifdef CLIP_PIC_DELTA_QP
	if (max_delta_qp_minus == -1)
		max_delta_qp_minus = 51;
	if (max_delta_qp_plus == -1)
		max_delta_qp_plus = 51;
#endif

#if ENABLE_RC_LIB
	CVI_VC_RC("bps = %d, buf_size_ms = %d\n", bps, buf_size_ms);
	CVI_VC_RC("frame_rate = %d, pic_width = %d, pic_height = %d\n",
		  frame_rate, pic_width, pic_height);
	CVI_VC_RC("gop_size = %d, rcEnable = %d, set_dqp_pic_num = %d\n",
		  gop_size, rcEnable, pEncInfo->openParam.set_dqp_pic_num);
	CVI_VC_RC(
		"LongTermDeltaQp = %d, RcInitialQp = %d, is_first_pic = %d, gamma = %d\n",
		pEncInfo->openParam.LongTermDeltaQp,
		pEncInfo->openParam.RcInitialQp, is_first_pic, gamma);
	CVI_VC_RC(
		"rcWeightFactor = %d, max_delta_qp_minus = %d, max_delta_qp_plus = %d\n",
		pEncInfo->openParam.rcWeightFactor, max_delta_qp_minus,
		max_delta_qp_plus);

	CVI_FUNC_COND(CVI_MASK_RQ, frm_rc_print(&pEncInfo->frm_rc));
#endif

#ifdef AUTO_FRM_SKIP_DROP
	CVI_VC_RC(
		"enAutoFrmSkip = %d, enAutoFrmDrop = %d, vbvThreshold = %d, qpThreshold = %d\n",
		pEncInfo->openParam.enAutoFrmSkip,
		pEncInfo->openParam.enAutoFrmDrop,
		pEncInfo->openParam.vbvThreshold,
		pEncInfo->openParam.qpThreshold);

	CVI_FUNC_COND(CVI_MASK_RQ, frm_rc_print(&pEncInfo->frm_rc));
#endif
}

RetCode Coda9VpuEncRegisterFramebuffer(CodecInst *instance)
{
	CodecInst *pCodecInst = instance;
	EncInfo *pEncInfo;
	Int32 i, val;
#ifdef REDUNDENT_CODE
	RetCode ret;
#endif
	PhysicalAddress paraBuffer;
	BYTE frameAddr[MAX_FRAMEBUFFER_COUNT + 1][3][4] = { 0 };
#ifdef REDUNDENT_CODE
	Int32 stride, height, mapType, num;
#else
	Int32 stride, height, mapType;
#endif
	VpuAttr *pAttr = &g_VpuCoreAttributes[instance->coreIdx];

	pEncInfo = &instance->CodecInfo->encInfo;
	stride = pEncInfo->stride;
	height = pEncInfo->frameBufferHeight;
	mapType = pEncInfo->mapType;

	CVI_VC_TRACE("mapType = 0x%X\n", mapType);

#ifdef REDUNDENT_CODE
	if (pCodecInst->productId == PRODUCT_ID_960) {
		pEncInfo->mapCfg.tiledBaseAddr = pEncInfo->vbFrame.phys_addr;
	}
#endif

	if (!ConfigSecAXICoda9(pCodecInst->coreIdx, instance->codecMode,
			       &pEncInfo->secAxiInfo, stride, 0, 0))
		return RETCODE_INSUFFICIENT_RESOURCE;

#ifdef REDUNDENT_CODE
	if (pCodecInst->productId == PRODUCT_ID_960) {
		val = SetTiledMapType(pCodecInst->coreIdx, &pEncInfo->mapCfg,
				      mapType, stride,
				      pEncInfo->openParam.cbcrInterleave,
				      &pEncInfo->dramCfg);
	} else
#endif
	{
		if (mapType != LINEAR_FRAME_MAP && mapType != LINEAR_FIELD_MAP)
			val = SetTiledMapType(
				pCodecInst->coreIdx, &pEncInfo->mapCfg, mapType,
				(stride > height) ? stride : height,
				pEncInfo->openParam.cbcrInterleave,
				&pEncInfo->dramCfg);
		else
			val = SetTiledMapType(
				pCodecInst->coreIdx, &pEncInfo->mapCfg, mapType,
				stride, pEncInfo->openParam.cbcrInterleave,
				&pEncInfo->dramCfg);
	}

	if (val == 0)
		return RETCODE_INVALID_PARAM;

	SetEncFrameMemInfo(pCodecInst);

	paraBuffer = vdi_remap_memory_address(pCodecInst->coreIdx,
					      VpuReadReg(pCodecInst->coreIdx,
							 BIT_PARA_BUF_ADDR));

	if (pEncInfo->addrRemapEn) {
		cviInitAddrRemapFb(pCodecInst);
	}

	if (pEncInfo->numFrameBuffers > MAX_FRAMEBUFFER_COUNT) {
		CVI_VC_ERR("numFrameBuffers %d out of spec\n",
			   pEncInfo->numFrameBuffers);
		return RETCODE_INVALID_PARAM;
	}

	// Let the decoder know the addresses of the frame buffers.
	for (i = 0; i < pEncInfo->numFrameBuffers; i++) {
		CVI_VC_TRACE("bufY = 0x%llX, Cb = 0x%llX, Cr = 0x%llX\n",
			     pEncInfo->frameBufPool[i].bufY,
			     pEncInfo->frameBufPool[i].bufCb,
			     pEncInfo->frameBufPool[i].bufCr);
		frameAddr[i][0][0] =
			(pEncInfo->frameBufPool[i].bufY >> 24) & 0xFF;
		frameAddr[i][0][1] =
			(pEncInfo->frameBufPool[i].bufY >> 16) & 0xFF;
		frameAddr[i][0][2] =
			(pEncInfo->frameBufPool[i].bufY >> 8) & 0xFF;
		frameAddr[i][0][3] =
			(pEncInfo->frameBufPool[i].bufY >> 0) & 0xFF;
		frameAddr[i][1][0] =
			(pEncInfo->frameBufPool[i].bufCb >> 24) & 0xFF;
		frameAddr[i][1][1] =
			(pEncInfo->frameBufPool[i].bufCb >> 16) & 0xFF;
		frameAddr[i][1][2] =
			(pEncInfo->frameBufPool[i].bufCb >> 8) & 0xFF;
		frameAddr[i][1][3] =
			(pEncInfo->frameBufPool[i].bufCb >> 0) & 0xFF;
		frameAddr[i][2][0] =
			(pEncInfo->frameBufPool[i].bufCr >> 24) & 0xFF;
		frameAddr[i][2][1] =
			(pEncInfo->frameBufPool[i].bufCr >> 16) & 0xFF;
		frameAddr[i][2][2] =
			(pEncInfo->frameBufPool[i].bufCr >> 8) & 0xFF;
		frameAddr[i][2][3] =
			(pEncInfo->frameBufPool[i].bufCr >> 0) & 0xFF;
	}
	VpuWriteMem(pCodecInst->coreIdx, paraBuffer, (BYTE *)frameAddr,
		    sizeof(frameAddr), VDI_BIG_ENDIAN);

	if (pCodecInst->productId == PRODUCT_ID_980) {
		for (i = 0; i < pEncInfo->numFrameBuffers; i++) {
			frameAddr[i][0][0] =
				(pEncInfo->frameBufPool[i].bufYBot >> 24) &
				0xFF;
			frameAddr[i][0][1] =
				(pEncInfo->frameBufPool[i].bufYBot >> 16) &
				0xFF;
			frameAddr[i][0][2] =
				(pEncInfo->frameBufPool[i].bufYBot >> 8) & 0xFF;
			frameAddr[i][0][3] =
				(pEncInfo->frameBufPool[i].bufYBot >> 0) & 0xFF;
			frameAddr[i][1][0] =
				(pEncInfo->frameBufPool[i].bufCbBot >> 24) &
				0xFF;
			frameAddr[i][1][1] =
				(pEncInfo->frameBufPool[i].bufCbBot >> 16) &
				0xFF;
			frameAddr[i][1][2] =
				(pEncInfo->frameBufPool[i].bufCbBot >> 8) &
				0xFF;
			frameAddr[i][1][3] =
				(pEncInfo->frameBufPool[i].bufCbBot >> 0) &
				0xFF;
			frameAddr[i][2][0] =
				(pEncInfo->frameBufPool[i].bufCrBot >> 24) &
				0xFF;
			frameAddr[i][2][1] =
				(pEncInfo->frameBufPool[i].bufCrBot >> 16) &
				0xFF;
			frameAddr[i][2][2] =
				(pEncInfo->frameBufPool[i].bufCrBot >> 8) &
				0xFF;
			frameAddr[i][2][3] =
				(pEncInfo->frameBufPool[i].bufCrBot >> 0) &
				0xFF;
		}
		VpuWriteMem(pCodecInst->coreIdx, paraBuffer + 384 + 128,
			    (BYTE *)frameAddr, sizeof(frameAddr),
			    VDI_BIG_ENDIAN);
	}

	// Tell the codec how much frame buffers were allocated.
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_BUF_NUM,
		    pEncInfo->numFrameBuffers);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_BUF_STRIDE, stride);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_BIT_ADDR,
		    pEncInfo->secAxiInfo.u.coda9.bufBitUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_IPACDC_ADDR,
		    pEncInfo->secAxiInfo.u.coda9.bufIpAcDcUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_DBKY_ADDR,
		    pEncInfo->secAxiInfo.u.coda9.bufDbkYUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_DBKC_ADDR,
		    pEncInfo->secAxiInfo.u.coda9.bufDbkCUse);
	VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_AXI_OVL_ADDR,
		    pEncInfo->secAxiInfo.u.coda9.bufOvlUse);

	if (pAttr->framebufferCacheType == FramebufCacheMaverickII) {
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_CACHE_CONFIG,
			    pEncInfo->cacheConfig.type2.CacheMode);
	} else if (pAttr->framebufferCacheType == FramebufCacheMaverickI) {
		// Maverick Cache Configuration
		val = (pEncInfo->cacheConfig.type1.luma.cfg.PageSizeX << 28) |
		      (pEncInfo->cacheConfig.type1.luma.cfg.PageSizeY << 24) |
		      (pEncInfo->cacheConfig.type1.luma.cfg.CacheSizeX << 20) |
		      (pEncInfo->cacheConfig.type1.luma.cfg.CacheSizeY << 16) |
		      (pEncInfo->cacheConfig.type1.chroma.cfg.PageSizeX << 12) |
		      (pEncInfo->cacheConfig.type1.chroma.cfg.PageSizeY << 8) |
		      (pEncInfo->cacheConfig.type1.chroma.cfg.CacheSizeX << 4) |
		      (pEncInfo->cacheConfig.type1.chroma.cfg.CacheSizeY << 0);

		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_CACHE_SIZE, val);

		val = (pEncInfo->cacheConfig.type1.Bypass << 4) |
		      (pEncInfo->cacheConfig.type1.DualConf << 2) |
		      (pEncInfo->cacheConfig.type1.PageMerge << 0);
		val = val << 24;
		val |= (pEncInfo->cacheConfig.type1.luma.cfg.BufferSize << 16) |
		       (pEncInfo->cacheConfig.type1.chroma.cfg.BufferSize
			<< 8) |
		       (pEncInfo->cacheConfig.type1.chroma.cfg.BufferSize << 8);

		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_CACHE_CONFIG,
			    val);
	}

#ifdef REDUNDENT_CODE
	num = pEncInfo->numFrameBuffers;
	if (pCodecInst->productId == PRODUCT_ID_960) {
		Uint32 subsampleLumaSize = stride * height;
		Uint32 subsampleChromaSize =
			(stride * height) >> 2; // FORMAT_420
		vpu_buffer_t vbBuf;
		FrameBuffer *pFb;

		vbBuf.size = subsampleLumaSize + 2 * subsampleChromaSize;
		vbBuf.phys_addr = (PhysicalAddress)0;
		if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx, &vbBuf, 0) < 0) {
			pEncInfo->vbSubSampFrame.size = 0;
			pEncInfo->vbSubSampFrame.phys_addr = 0;
			return RETCODE_INSUFFICIENT_RESOURCE;
		}
		pFb = &pEncInfo->frameBufPool[num];
		pFb->bufY = vbBuf.phys_addr;
		pFb->bufCb = (PhysicalAddress)-1;
		pFb->bufCr = (PhysicalAddress)-1;
		pFb->updateFbInfo = TRUE;
		ret = AllocateLinearFrameBuffer(LINEAR_FRAME_MAP, pFb, 1,
						subsampleLumaSize,
						subsampleChromaSize);
		if (ret != RETCODE_SUCCESS) {
			pEncInfo->vbSubSampFrame.size = 0;
			pEncInfo->vbSubSampFrame.phys_addr = 0;
			return RETCODE_INSUFFICIENT_RESOURCE;
		}
		pEncInfo->vbSubSampFrame = vbBuf;
		num++;

		// Set Sub-Sampling buffer for ME-Reference and
		// DBK-Reconstruction BPU will swap below two buffer internally
		// every pic by pic
		val = GetXY2AXIAddr(&pEncInfo->mapCfg, 0, 0, 0, stride, pFb);
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_SUBSAMP_A, val);
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_SUBSAMP_B,
			    val + ((stride * height) >> 1));

		if (pCodecInst->codecMode == AVC_ENC &&
		    pCodecInst->codecModeAux == AVC_AUX_MVC) {
			vbBuf.size =
				subsampleLumaSize + 2 * subsampleChromaSize;
			vbBuf.phys_addr = (PhysicalAddress)0;
			if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx, &vbBuf,
						0) < 0) {
				pEncInfo->vbSubSampFrame.size = 0;
				pEncInfo->vbSubSampFrame.phys_addr = 0;
				return RETCODE_INSUFFICIENT_RESOURCE;
			}
			pFb = &pEncInfo->frameBufPool[num];
			pFb->bufY = vbBuf.phys_addr;
			pFb->bufCb = (PhysicalAddress)-1;
			pFb->bufCr = (PhysicalAddress)-1;
			pFb->updateFbInfo = TRUE;
			ret = AllocateLinearFrameBuffer(LINEAR_FRAME_MAP, pFb,
							1, subsampleLumaSize,
							subsampleChromaSize);
			if (ret != RETCODE_SUCCESS) {
				pEncInfo->vbMvcSubSampFrame.size = 0;
				pEncInfo->vbMvcSubSampFrame.phys_addr = 0;
				return RETCODE_INSUFFICIENT_RESOURCE;
			}
			pEncInfo->vbMvcSubSampFrame = vbBuf;

			val = GetXY2AXIAddr(&pEncInfo->mapCfg, 0, 0, 0, stride,
					    pFb);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_SET_FRAME_SUBSAMP_A_MVC, val);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_SET_FRAME_SUBSAMP_B_MVC,
				    val + ((stride * height) >> 1));
		}
	}

	if (pCodecInst->codecMode == MP4_ENC) {
		// MPEG4 Encoder Data-Partitioned bitstream temporal buffer
		pEncInfo->vbScratch.size = SIZE_MP4ENC_DATA_PARTITION;
		if (VDI_ALLOCATE_MEMORY(pCodecInst->coreIdx,
					&pEncInfo->vbScratch, 0) < 0)
			return RETCODE_INSUFFICIENT_RESOURCE;
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_DP_BUF_BASE,
			    pEncInfo->vbScratch.phys_addr);
		VpuWriteReg(pCodecInst->coreIdx, CMD_SET_FRAME_DP_BUF_SIZE,
			    pEncInfo->vbScratch.size >> 10);
	}
#endif

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, SET_FRAME_BUF);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, SET_FRAME_BUF, 2);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, SET_FRAME_BUF, 0);
#endif

	if (VpuReadReg(pCodecInst->coreIdx, RET_SET_FRAME_SUCCESS) &
	    (1UL << 31)) {
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

	return RETCODE_SUCCESS;
}

static RetCode Coda9PicTypeEntropyCoding(CodecInst *pCodecInst, EncInfo *pEncInfo)
{
	int pic_idx;
	int gop_num;
	int slice_type; // 0 = Intra, 1 =inter

	Coda9VpuEncGetPicInfo(pEncInfo, &pic_idx,
			      &gop_num, &slice_type);

	CVI_VC_CFG("slice_type = %d\n", slice_type);

	if (pEncInfo->openParam.EncStdParam.avcParam
		    .mvcExtension) {
		if (pEncInfo->openParam.EncStdParam
			    .avcParam.interviewEn) {
			if (slice_type == 0 &&
			    (pic_idx % 2) == 0) {
				// change_enable. pps_id, entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					(1 << 11) +
						(1
						 << 7));

				// pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					0);

				// cabac_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					0);
			} else if (slice_type != 0 &&
				   (pic_idx % 2) == 0) {
				// change_enable. pps_id, entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					(1 << 11) +
						(1
						 << 7));
				// pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1);
				// cabac_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					1);
			}

			else //(slice_type==0 &&
			//pic_idx%2 !=0)
			{
				// change_enable. pps_id, entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					(1 << 11) +
						(1
						 << 7));
				// pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1);
				// cabac_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					3);
			}
		} else {
			if (slice_type == 0 &&
			    (pic_idx % 2) == 0) {
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					0x880); // change_enable.
				// pps_id,
				// entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					0); // pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					0); // cabac_mode
			}

			else if (slice_type != 0 &&
				 (pic_idx % 2) == 0) {
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					0x880); // change_enable.
				// pps_id,
				// entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1); // pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					1); // cabac_mode
			}

			else if (slice_type == 0 &&
				 (pic_idx % 2) != 0) {
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					0x880); // change_enable.
				// pps_id,
				// entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					0); // pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					2); // cabac_mode
			}

			else // if(slice_type!=0 &&
			// pic_idx%2 !=0)
			{
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_ENABLE,
					0x880); // change_enable.
				// pps_id,
				// entropy_coding_mode
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1); // pps-id
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					3); // cabac_mode
			}
		}
	} else {
		// change_enable.
		// ppsId,
		// entropyCodingMode
		// default set by I (ID=0, mode=0)
		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_ENC_PARAM_CHANGE_ENABLE,
			    0x880);

		VpuWriteReg(
			pCodecInst->coreIdx,
			CMD_ENC_PARAM_CHANGE_CABAC_MODE,
			0);

		VpuWriteReg(pCodecInst->coreIdx,
			    CMD_ENC_PARAM_CHANGE_PPS_ID,
			    0);

		// only first I
		if (gop_num == 0) {
			if (slice_type != 0) {
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1);

				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					1);
			}
		} else if (gop_num != 1) {
			// not All I
			if (slice_type != 0) {
				// (encoded_frames_in_gop > 0)
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_CABAC_MODE,
					1);
				VpuWriteReg(
					pCodecInst
						->coreIdx,
					CMD_ENC_PARAM_CHANGE_PPS_ID,
					1);
			}
		}
	}

	CVI_VC_CFG("gop_num = %d, slice_type = %d\n",
		   gop_num, slice_type);

	Coda9BitIssueCommand(pCodecInst->coreIdx,
			     pCodecInst,
			     RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx,
			      __VPU_BUSY_TIMEOUT,
			      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx,
				RC_CHANGE_PARAMETER, 0);
#endif
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx,
			RC_CHANGE_PARAMETER, 0);
#endif

	return RETCODE_SUCCESS;
}

RetCode Coda9VpuEncode(CodecInst *pCodecInst, EncParam *param)
{
	EncInfo *pEncInfo;
	FrameBuffer *pSrcFrame;
	Uint32 rotMirMode;
	Uint32 val;
	vpu_instance_pool_t *vip;
	int outQp = 0;
	int target_pic_bit = 0;
	int hrd_buf_level = 0;
	int hrd_buf_size = 0;
#ifdef CLIP_PIC_DELTA_QP
	int row_max_dqp_minus = 0;
	int row_max_dqp_plus = 0;
#endif
	int ret;

	pEncInfo = &pCodecInst->CodecInfo->encInfo;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip) {
		return RETCODE_INVALID_HANDLE;
	}

	pSrcFrame = param->sourceFrame;
	rotMirMode = 0;
	if (pEncInfo->rotationEnable == TRUE) {

		switch (pEncInfo->rotationAngle) {
		case 0:
			rotMirMode |= 0x0;
			break;
		case 90:
			rotMirMode |= 0x1;
			break;
		case 180:
			rotMirMode |= 0x2;
			break;
		case 270:
			rotMirMode |= 0x3;
			break;
		}
	}

	if (pEncInfo->mirrorEnable == TRUE) {

		switch (pEncInfo->mirrorDirection) {
		case MIRDIR_NONE:
			rotMirMode |= 0x0;
			break;
		case MIRDIR_VER:
			rotMirMode |= 0x4;
			break;
		case MIRDIR_HOR:
			rotMirMode |= 0x8;
			break;
		case MIRDIR_HOR_VER:
			rotMirMode |= 0xc;
			break;
		}
	}

	//====================================
	// @ Step 1 :
	//      Get Qp for non-roi region
	//====================================
	ret = Coda9VpuEncCalcPicQp(pCodecInst, param, &row_max_dqp_minus,
				   &row_max_dqp_plus, &outQp, &target_pic_bit,
				   &hrd_buf_level, &hrd_buf_size);
	if (ret != RETCODE_SUCCESS) {
		CVI_VC_ERR("ret = %d\n", ret);
		return ret;
	}

	if (pCodecInst->productId == PRODUCT_ID_980) {
		rotMirMode |= ((pSrcFrame->endian & 0x03) << 16);
		rotMirMode |= ((pSrcFrame->cbcrInterleave & 0x01) << 18);
		rotMirMode |= ((pSrcFrame->sourceLBurstEn & 0x01) << 4);
	}
#ifdef REDUNDENT_CODE
	else {
		rotMirMode |= ((pSrcFrame->sourceLBurstEn & 0x01) << 4);
		rotMirMode |= ((pSrcFrame->cbcrInterleave & 0x01) << 18);
		rotMirMode |= pEncInfo->openParam.nv21 << 21;
	}
#endif

#ifdef PLATFORM_NON_OS
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_ROT_MODE, rotMirMode);
#endif
	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC) {
		if (pCodecInst->cviApiMode != API_MODE_DRIVER)
			cviSetCodaRoiBySdk(param, &pEncInfo->openParam, outQp);

		// ROI command
		if (param->coda9RoiEnable) {
#ifdef ROI_MB_RC
			int roi_number = param->setROI.number;
			int i;
			int data = 0;
			int qp = 0;

			VpuWriteReg(
				pCodecInst->coreIdx, CMD_ENC_ROI_INFO,
				param->nonRoiQp << 7 |
					param->coda9RoiPicAvgQp << 1 |
					param->coda9RoiEnable); // currently,
			// only mode
			// 0 can be
			// supported
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_ROI_NUM,
				    param->setROI.number);

			for (i = 0; i < MAX_CODA980_ROI_NUMBER; i++) {
				VpuRect *rect = &param->setROI.region[i];

				/*
		if (pEncInfo->openParam.EncStdParam.avcParam.fieldFlag)
		    data = ((((rect->bottom+1)/2)&0xff)<<24)|
		((((rect->top+1)/2)&0xff)<<16) | ((((rect->right+1)/2)&0xff)<<8)
		| (((rect->left+1)/2)&0xff); else*/
				if (i < roi_number) {
					data = (((rect->bottom) & 0xff) << 24) |
					       (((rect->top) & 0xff) << 16) |
					       (((rect->right) & 0xff) << 8) |
					       ((rect->left) & 0xff);
					qp = param->setROI.qp[i];
				}
				//===============================================
				// @ Step 2 :
				if (i != 7)
					VpuWriteReg(pCodecInst->coreIdx,
						    CMD_ENC_ROI_POS_0 + i * 8,
						    data);
				else
					VpuWriteReg(pCodecInst->coreIdx,
						    CMD_ENC_ROI_POS_7, data);
				//    generate roi qp for each region by using
				//    roi_delta_qp/roi_level and picQp (roi qp =
				//    picQp - (roi_delta_qp*roi_level)
				VpuWriteReg(pCodecInst->coreIdx,
					    CMD_ENC_ROI_QP_0 + i * 8, qp);
				//===============================================
			}

#else
			VpuWriteReg(
				pCodecInst->coreIdx, CMD_ENC_ROI_INFO,
				param->coda9RoiPicAvgQp << 1 |
					param->coda9RoiEnable); // currently,
			// only mode
			// 0 can be
			// supported
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_ROI_QP_MAP_ADDR,
				    param->roiQpMapAddr);
#endif

			Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst,
					     ENC_ROI_INIT);
			if (vdi_wait_vpu_busy(pCodecInst->coreIdx,
					      __VPU_BUSY_TIMEOUT,
					      BIT_BUSY_FLAG) == -1) {
#ifdef ENABLE_CNM_DEBUG_MSG
				if (pCodecInst->loggingEnable)
					vdi_log(pCodecInst->coreIdx,
						ENC_ROI_INIT, 0);
#endif
				return RETCODE_VPU_RESPONSE_TIMEOUT;
			}
#ifdef ENABLE_CNM_DEBUG_MSG
			if (pCodecInst->loggingEnable)
				vdi_log(pCodecInst->coreIdx, ENC_ROI_INIT, 0);
#endif
			if (!VpuReadReg(pCodecInst->coreIdx,
					RET_ENC_ROI_SUCCESS)) {
				return RETCODE_FAILURE;
			}
		} // if (param->coda9RoiEnable)

		{
			int ActivePPSIdx = pEncInfo->ActivePPSIdx;
			AvcPpsParam *ActvePPS =
				&pEncInfo->openParam.EncStdParam.avcParam
					 .ppsParam[ActivePPSIdx];
			if (pEncInfo->frameIdx == 0)
				pEncInfo->encoded_frames_in_gop = 0;

			CVI_VC_TRACE("ActivePPSIdx = %d, frameIdx = %d, encoded_frames_in_gop = %d\n",
				     ActivePPSIdx, pEncInfo->frameIdx,
				     pEncInfo->encoded_frames_in_gop);

			if (ActvePPS->entropyCodingMode == 2) {
				ret = Coda9PicTypeEntropyCoding(pCodecInst, pEncInfo);
				if (ret != RETCODE_SUCCESS) {
					CVI_VC_ERR("Coda9PicTypeEntropyCoding = %d\n", ret);
					return ret;
				}
			} else {
				int pic_idx;
				int gop_num;
				int slice_type; // 0 = Intra, 1 =inter

				Coda9VpuEncGetPicInfo(pEncInfo, &pic_idx,
						      &gop_num, &slice_type);
			}
		}
	}

#ifdef SUPPORT_980_ROI_RC_LIB
	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC &&
	    pCodecInst->codecModeAux == AVC_AUX_SVC) {
		pEncInfo->is_skip_picture = param->skipPicture;

		// Write register
		// Slice Header info
		{
			int long_term_ref_flag = (pEncInfo->ref_pic_list ==
						  pEncInfo->longterm_frmidx);
			int diff =
				pEncInfo->frm[pEncInfo->ref_pic_list].frame_num -
				pEncInfo->curr_frm->frame_num;
			int diff_sign = (diff > 0) ? 1 : 0;
			int abs_diff = (abs(diff) - 1) &
				       ((1 << 4) - 1); //  LOG2_MAX_FRM_NUM
			//  4
			int longterm_frmidx_flag =
				(pEncInfo->longterm_frmidx == -1) ? 1 : 0;
			int curr_poc =
				pEncInfo->curr_frm->poc & ((1 << 16) - 1);
			int data;

			data = (pEncInfo->curr_frm->frame_num << 24 |
				pEncInfo->temporal_id << 21 |
				pEncInfo->prev_idx << 18 |
				pEncInfo->ref_pic_list << 15 |
				longterm_frmidx_flag << 14 |
				pEncInfo->curr_long_term << 13 |
				diff_sign << 12 | abs_diff << 8 |
				long_term_ref_flag << 7 |
				pEncInfo->nal_ref_idc << 5);

			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_DPB_INFO,
				    data);
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_POC,
				    curr_poc);
		}
	}
#endif

#ifdef REDUNDENT_CODE
	if (pCodecInst->productId == PRODUCT_ID_960) {
		if (pEncInfo->mapType > LINEAR_FRAME_MAP &&
		    pEncInfo->mapType <= TILED_MIXED_V_MAP) {
			SetTiledFrameBase(pCodecInst->coreIdx,
					  pEncInfo->vbFrame.phys_addr);
		} else {
			SetTiledFrameBase(pCodecInst->coreIdx, 0);
		}
	}
#endif
	if (pEncInfo->mapType != LINEAR_FRAME_MAP &&
	    pEncInfo->mapType != LINEAR_FIELD_MAP) {
		if (pEncInfo->stride > pEncInfo->frameBufferHeight)
			val = SetTiledMapType(
				pCodecInst->coreIdx, &pEncInfo->mapCfg,
				pEncInfo->mapType, pEncInfo->stride,
				pEncInfo->openParam.cbcrInterleave,
				&pEncInfo->dramCfg);
		else
			val = SetTiledMapType(
				pCodecInst->coreIdx, &pEncInfo->mapCfg,
				pEncInfo->mapType, pEncInfo->frameBufferHeight,
				pEncInfo->openParam.cbcrInterleave,
				&pEncInfo->dramCfg);
	} else {
		val = SetTiledMapType(pCodecInst->coreIdx, &pEncInfo->mapCfg,
				      pEncInfo->mapType, pEncInfo->stride,
				      pEncInfo->openParam.cbcrInterleave,
				      &pEncInfo->dramCfg);
	}

	CVI_VC_TRACE("mapType = %d, stride = %d, cbcrInterleave = %d\n",
		     pEncInfo->mapType, pEncInfo->stride,
		     pEncInfo->openParam.cbcrInterleave);

	if (val == 0) {
		return RETCODE_INVALID_PARAM;
	}

#ifndef PLATFORM_NON_OS
	rotMirMode |= pEncInfo->openParam.nv21 << 21;
	rotMirMode |= (1 << 4);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_ROT_MODE, rotMirMode);

#endif

	//=======================================================================
	// @ Step 3 :
	//    set Qp for non-roi region or Qp for picture.
	//=======================================================================
	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC) {
		CVI_VC_TRACE("outQp = %d, hrd_buf_level = %d, hrd_buf_size = %d\n",
				outQp, hrd_buf_level, hrd_buf_size);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_QS, outQp);

	} else
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_QS,
			    param->quantParam);

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_TARGET_BIT,
		    target_pic_bit);

#ifdef ROI_MB_RC
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_BUF_LEVEL,
		    hrd_buf_level);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_BUF_SIZE, hrd_buf_size);

	CVI_FUNC_COND(CVI_MASK_CFG,
		      fprintf(fpCfg, "hrd_buf_level = %d, hrd_buf_size = %d\n",
			      hrd_buf_level, hrd_buf_size));

#ifdef AUTO_FRM_SKIP_DROP
	pEncInfo->frm_rc.rc_pic.frame_skip |= param->skipPicture;
#ifdef CLIP_PIC_DELTA_QP
	if (param->coda9RoiEnable) {
		row_max_dqp_plus = pEncInfo->openParam.roi_max_delta_qp_plus;
		row_max_dqp_minus = pEncInfo->openParam.roi_max_delta_qp_minus;
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_QP,
		    (row_max_dqp_minus << 26) | (row_max_dqp_plus << 20) |
			    (pEncInfo->frm_rc.rc_pic.frame_drop << 19) |
			    (pEncInfo->frm_rc.rc_pic.frame_skip << 18) |
			    (outQp << 12) |
			    (pEncInfo->openParam.roi_max_delta_qp_plus << 6) |
			    pEncInfo->openParam.roi_max_delta_qp_minus);
#else
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_QP,
		    (pEncInfo->frm_rc.rc_pic.frame_drop << 19) |
			    (pEncInfo->frm_rc.rc_pic.frame_skip << 18) |
			    (outQp << 12) |
			    (pEncInfo->openParam.roi_max_delta_qp_plus << 6) |
			    pEncInfo->openParam.roi_max_delta_qp_minus);
#endif

#else
#ifdef CLIP_PIC_DELTA_QP
	if (param->coda9RoiEnable) {
		row_max_dqp_plus = pEncInfo->openParam.roi_max_delta_qp_plus;
		row_max_dqp_minus = pEncInfo->openParam.roi_max_delta_qp_minus;
	}

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_QP,
		    (row_max_dqp_minus << 26) | (row_max_dqp_plus << 20) |
			    0 << 18 | (outQp << 12) |
			    (pEncInfo->openParam.roi_max_delta_qp_plus << 6) |
			    pEncInfo->openParam.roi_max_delta_qp_minus);
#else
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_RC_QP,
		    0 << 18 | (outQp << 12) |
			    (pEncInfo->openParam.roi_max_delta_qp_plus << 6) |
			    pEncInfo->openParam.roi_max_delta_qp_minus);
#endif
#endif
#else
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_QP, outQp);
#endif

	if (param->skipPicture) {
		if (param->fieldRun) { // not support field + skipPicture
			return RETCODE_INVALID_PARAM;
		}
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_OPTION,
			    (param->fieldRun << 8) | 1);
	} else {
		// Registering Source Frame Buffer information
		// Hide GDI IF under FW level
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_SRC_INDEX,
			    pSrcFrame->myIndex);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_SRC_STRIDE,
			    pSrcFrame->stride);

		if (pEncInfo->openParam.cbcrOrder == CBCR_ORDER_NORMAL) {
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_SRC_ADDR_Y,
				    pSrcFrame->bufY);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_ADDR_CB, pSrcFrame->bufCb);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_ADDR_CR, pSrcFrame->bufCr);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_Y,
				    pSrcFrame->bufYBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_CB,
				    pSrcFrame->bufCbBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_CR,
				    pSrcFrame->bufCrBot);
		} else { // CBCR_ORDER_REVERSED (YV12)
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_SRC_ADDR_Y,
				    pSrcFrame->bufY);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_ADDR_CB, pSrcFrame->bufCr);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_ADDR_CR, pSrcFrame->bufCb);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_Y,
				    pSrcFrame->bufYBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_CB,
				    pSrcFrame->bufCrBot);
			VpuWriteReg(pCodecInst->coreIdx,
				    CMD_ENC_PIC_SRC_BOTTOM_CR,
				    pSrcFrame->bufCbBot);
		}

		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PIC_OPTION,
			    (param->fieldRun << 8) |
				    (param->forceIPicture << 1 & 0x2));
	}

	CVI_VC_INFO("CMD_ENC_PIC_OPTION = 0x%X\n",
		    VpuReadReg(pCodecInst->coreIdx, CMD_ENC_PIC_OPTION));

	if (pEncInfo->ringBufferEnable == 0) {
		SetEncBitStreamInfo(pCodecInst, NULL, param);
	}

	val = 0;
	val = ((pEncInfo->secAxiInfo.u.coda9.useBitEnable & 0x01) << 0 |
	       (pEncInfo->secAxiInfo.u.coda9.useIpEnable & 0x01) << 1 |
	       (pEncInfo->secAxiInfo.u.coda9.useDbkYEnable & 0x01) << 2 |
	       (pEncInfo->secAxiInfo.u.coda9.useDbkCEnable & 0x01) << 3 |
	       (pEncInfo->secAxiInfo.u.coda9.useOvlEnable & 0x01) << 4 |
	       (pEncInfo->secAxiInfo.u.coda9.useBtpEnable & 0x01) << 5 |
	       (pEncInfo->secAxiInfo.u.coda9.useBitEnable & 0x01) << 8 |
	       (pEncInfo->secAxiInfo.u.coda9.useIpEnable & 0x01) << 9 |
	       (pEncInfo->secAxiInfo.u.coda9.useDbkYEnable & 0x01) << 10 |
	       (pEncInfo->secAxiInfo.u.coda9.useDbkCEnable & 0x01) << 11 |
	       (pEncInfo->secAxiInfo.u.coda9.useOvlEnable & 0x01) << 12 |
	       (pEncInfo->secAxiInfo.u.coda9.useBtpEnable & 0x01) << 13);

	VpuWriteReg(pCodecInst->coreIdx, BIT_AXI_SRAM_USE, val);

	CVI_VC_TRACE("streamRdPtr = 0x%llX, streamWtPtr = 0x%llX\n",
		     pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);
	CVI_VC_TRACE("streamEndflag = %d\n", pEncInfo->streamEndflag);

	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr,
		    pEncInfo->streamWrPtr);
	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr,
		    pEncInfo->streamRdPtr);
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM,
		    pEncInfo->streamEndflag);

	SetEncFrameMemInfo(pCodecInst);

	val = 0;
	if (pEncInfo->ringBufferEnable == 0) {
		if (pEncInfo->lineBufIntEn)
			val |= (0x1 << 6);
#ifndef PLATFORM_NON_OS
		val |= (0x1 << 5);
		val |= (0x1 << 4);
#endif
	} else {
		val |= (0x1 << 3);
	}
	val |= pEncInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (pCodecInst->productId == PRODUCT_ID_980)
		VpuWriteReg(pCodecInst->coreIdx, BIT_ME_LINEBUFFER_MODE,
			    VPU_ME_LINEBUFFER_MODE); // default
	CVI_VC_FLOW("PIC_RUN\n");
#ifdef DBG_CNM
	printf_gdi_info(pCodecInst->coreIdx, 32);
#endif

	pCodecInst->u64StartTime = cviGetCurrentTime();

	Coda9BitIssueCommand(pCodecInst->coreIdx, pCodecInst, PIC_RUN);

	return RETCODE_SUCCESS;
}

static int Coda9VpuEncCalcPicQp(CodecInst *pCodecInst, EncParam *param,
				int *pRowMaxDqpMinus, int *pRowMaxDqpPlus,
				int *pOutQp, int *pTargetPicBit,
				int *pHrdBufLevel, int *pHrdBufSize)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;
	int ret = RETCODE_SUCCESS;

	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC) {
		if (pEncInfo->openParam.rcEnable == 4) {
			if (pEncInfo->cviRcEn || pRcInfo->rcMode == RC_MODE_UBR) {
				*pOutQp = param->u32FrameQp;
				*pRowMaxDqpMinus = param->u32FrameQp;
				*pRowMaxDqpPlus = 51 - param->u32FrameQp;
				*pTargetPicBit = param->u32FrameBits;
				*pHrdBufSize = pRcInfo->targetBitrate *
					       pEncInfo->openParam.rcInitDelay;

				if (param->s32HrdBufLevel <= 0)
					*pHrdBufLevel = 0;
				else
					*pHrdBufLevel = param->s32HrdBufLevel;

				CVI_VC_UBR(
					"u32FrameBits = %d, u32FrameQp = %d, s32HrdBufLevel = %d\n",
					param->u32FrameBits, param->u32FrameQp,
					param->s32HrdBufLevel);
			} else {
				ret = rcLibCalcPicQp(pCodecInst,
						     pRowMaxDqpMinus,
						     pRowMaxDqpPlus, pOutQp,
						     pTargetPicBit,
						     pHrdBufLevel, pHrdBufSize);
				if (ret != RETCODE_SUCCESS) {
					CVI_VC_INFO("ret = %d\n", ret);
					return ret;
				}
			}
		} else {
			*pOutQp = param->quantParam;
		}
	} else {
		*pOutQp = param->quantParam;
	}

	return RETCODE_SUCCESS;
}

int rcLibCalcPicQp(CodecInst *pCodecInst, int *pRowMaxDqpMinus,
		   int *pRowMaxDqpPlus, int *pOutQp, int *pTargetPicBit,
		   int *pHrdBufLevel, int *pHrdBufSize)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;

	// outQp will be returned a QP for non-roi region when rc
	// mode==4 && roi enabled. outQp will be returned a QP for one
	// picture when rc mode==4 && roi disabled.
	int min_qp;
	int max_qp;
	int pic_idx;
	int gop_num;
	int slice_type; // 0 = Intra, 1 =inter

	Coda9VpuEncGetPicInfo(pEncInfo, &pic_idx, &gop_num, &slice_type);

	min_qp = slice_type == 0 ? pRcInfo->picIMinQp : pRcInfo->picPMinQp;
	max_qp = slice_type == 0 ? pRcInfo->picIMaxQp : pRcInfo->picPMaxQp;

	CVI_VC_RC("frameIdx = %d, maxIPicBit = %d\n", pEncInfo->frameIdx,
		  pRcInfo->maxIPicBit);
	CVI_VC_RC(
		"slice_type = %d, min_qp = %d, max_qp = %d, targetBitrate = %d\n",
		slice_type, min_qp, max_qp, pRcInfo->targetBitrate);
	CVI_VC_RC("curr_long_term = %d\n", pEncInfo->curr_long_term);

	CVI_FUNC_COND(CVI_MASK_RQ, frm_rc_print(&pEncInfo->frm_rc));

#ifdef AUTO_FRM_SKIP_DROP
	// picture qp adjustment
	if ((pRcInfo->isReEncodeIdr) ||
	    (slice_type == 0 && pEncInfo->frameIdx > 0)) {
		int constrain_min_i_qp = -1;

		CVI_VC_RC("isReEncodeIdr = %d, rcGopIQpOffset = %d\n",
			  pRcInfo->isReEncodeIdr,
			  pEncInfo->openParam.rcGopIQpOffset);

		*pOutQp -= pEncInfo->openParam.rcGopIQpOffset;
		if (pRcInfo->maxIPicBit > 0) {
			*pTargetPicBit =
				MIN(*pTargetPicBit, pRcInfo->maxIPicBit);

			if (pRcInfo->isReEncodeIdr) {
				*pTargetPicBit =
					MIN(*pTargetPicBit,
					    pRcInfo->s32SuperFrmBitsThr);
			}

			CVI_VC_RC(
				"maxIPicBit = %d, constrain_min_i_qp = %d, outQp = %d\n",
				pRcInfo->maxIPicBit, constrain_min_i_qp,
				*pOutQp);
		}

		if (constrain_min_i_qp > *pOutQp) {
			int weight = CLIP3(1, 4, pEncInfo->frameIdx / gop_num);
			*pOutQp = ((weight * constrain_min_i_qp) +
				   ((4 - weight) * (*pOutQp))) >>
				  2;
		}
	} else if (slice_type == 1 && pEncInfo->ref_long_term == 1) {
		// virtual I frame
		*pOutQp -= pEncInfo->openParam.LongTermDeltaQp;
	} else if (pRcInfo->rcMode == RC_MODE_VBR &&
		   pRcInfo->rcState == STEADY) {
		// picture Qp clipping for VBR
		if (slice_type == 1) {
			*pOutQp = CLIP3(pRcInfo->lastPicQp - 1,
					pRcInfo->lastPicQp + 1, *pOutQp);
		}
		pRcInfo->lastPicQp = *pOutQp;
	}

	if (pRcInfo->rcMode == RC_MODE_AVBR) {
		// weird workaround here. Appoligized..
		pRcInfo->lastPicQp = *pOutQp;
	}

	cviPrintFrmRc(&pEncInfo->frm_rc);
	CVI_VC_RC("slice_type = %d, min_qp = %d, max_qp = %d\n", slice_type,
		  min_qp, max_qp);
	CVI_VC_RC("row_max_dqp_minus = %d, curr_long_term = %d\n",
		  *pRowMaxDqpMinus, pEncInfo->curr_long_term);
	CVI_VC_RC(
		"outQp = %d, TargetPicBit = %d, HrdBufLevel = %d, HrdBufSize = %d\n",
		*pOutQp, *pTargetPicBit, *pHrdBufLevel, *pHrdBufSize);

	if (pEncInfo->frm_rc.rc_pic.frame_drop) {
		CVI_VC_INFO("RETCODE_FRAME_DROP\n");
		return RETCODE_FRAME_DROP;
	}
#else
#endif

	return RETCODE_SUCCESS;
}

static void Coda9VpuEncGetPicInfo(EncInfo *pEncInfo, int *p_pic_idx,
				  int *p_gop_num, int *p_slice_type)
{
	int pic_idx;
	int gop_num;
	int encoded_frames;
	int slice_type; // 0 = Intra, 1 =inter
	int is_all_i = (pEncInfo->openParam.gopSize == 1);

	int mvc_second_view;

	pic_idx = pEncInfo->openParam.EncStdParam.avcParam.fieldFlag ?
				2 * pEncInfo->frameIdx + pEncInfo->fieldDone :
				pEncInfo->frameIdx;
	gop_num = pEncInfo->openParam.EncStdParam.avcParam.fieldFlag ?
				2 * pEncInfo->openParam.gopSize :
				pEncInfo->openParam.gopSize;
	encoded_frames = pEncInfo->openParam.EncStdParam.avcParam.fieldFlag ?
				       2 * pEncInfo->encoded_frames_in_gop +
					 pEncInfo->fieldDone :
				       pEncInfo->encoded_frames_in_gop;

	mvc_second_view =
		pEncInfo->openParam.EncStdParam.avcParam.mvcExtension &&
		(!pEncInfo->openParam.EncStdParam.avcParam.interviewEn);

	if (pEncInfo->openParam.EncStdParam.avcParam.mvcExtension)
		gop_num *= 2;

	UNREFERENCED_PARAMETER(mvc_second_view);

	CVI_VC_RC(
		"gop_num = %d, encoded_frames = %d, encoded_frames_in_gop = %d\n",
		gop_num, encoded_frames, pEncInfo->encoded_frames_in_gop);

	if (gop_num == 0) {
		// Only first I
		if (pic_idx == 0 || (pic_idx == 1 && mvc_second_view == 1))
			slice_type = 0;
		else
			slice_type = 1;
	} else if (is_all_i) {
		// All I
		encoded_frames = 0;
		pEncInfo->encoded_frames_in_gop = 0;
		slice_type = 0;
	} else {
		if (encoded_frames >= gop_num) {
			// I frame
			encoded_frames = 0;
			pEncInfo->encoded_frames_in_gop = 0;
		}
		if (encoded_frames == 0 ||
		    (encoded_frames == 1 && mvc_second_view == 1))
			slice_type = 0;
		else // P frame
			slice_type = 1;
	}

	*p_pic_idx = pic_idx;
	*p_gop_num = gop_num;
	*p_slice_type = slice_type;
}

static void cviPrintFrmRc(frm_rc_t *pFrmRc)
{
	CVI_VC_RC(
		"num_pixel_in_pic = %d, mb_width = %d, mb_height = %d, gop_size = %d\n",
		pFrmRc->rc_pic.num_pixel_in_pic, pFrmRc->rc_pic.mb_width,
		pFrmRc->rc_pic.mb_height, pFrmRc->rc_pic.gop_size);
	CVI_VC_RC("intra_period = %d, bit_alloc_mode = %d, mode = %d\n",
		  pFrmRc->rc_pic.intra_period, pFrmRc->rc_pic.bit_alloc_mode,
		  pFrmRc->rc_pic.mode);
	CVI_VC_RC(" avg_pic_bit = %d, xmit_pic_bit = %d\n",
		  pFrmRc->rc_pic.avg_pic_bit, pFrmRc->rc_pic.xmit_pic_bit);
}

RetCode Coda9VpuEncGetResult(CodecInst *pCodecInst, EncOutputInfo *info)
{
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	Uint32 pic_enc_result;

#ifdef ENABLE_CNM_DEBUG_MSG
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, PIC_RUN, 0);
#endif

	pic_enc_result = VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SUCCESS);
	if (pic_enc_result & (1UL << 31)) {
		return RETCODE_MEMORY_ACCESS_VIOLATION;
	}

#ifdef REPORT_PIC_SUM_VAR
	info->picVariance =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SUM_VAR);
	CVI_VC_RC("sumPicVar = %d\n", info->picVariance);
#endif

#ifdef AUTO_FRM_SKIP_DROP
	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC &&
	    pEncInfo->openParam.rcEnable == 4) {
		if (pEncInfo->frm_rc.rc_pic.frame_drop) {
			if (pRcInfo->rcMode != RC_MODE_UBR ||
			    pRcInfo->bTestUbrEn)
				rcLibUpdateRc(pCodecInst, info, 0, 0);
			else {
				CVI_VC_WARN("UBR should not drop frames\n");
			}
			return RETCODE_SUCCESS;
		}
		info->picDropped = 0;
	}
#endif
	if (pCodecInst->productId == PRODUCT_ID_980) {
		if (pic_enc_result & 2) { // top field coding done
			if (!pEncInfo->fieldDone)
				pEncInfo->fieldDone = 1;
		} else {
			pEncInfo->frameIdx = VpuReadReg(pCodecInst->coreIdx,
							RET_ENC_PIC_FRAME_NUM);
			info->encPicCnt = pEncInfo->frameIdx;
			pEncInfo->fieldDone = 0;
		}
	}

	info->picType = VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_TYPE);

	if (pEncInfo->ringBufferEnable == 0) {
		rdPtr = VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamRdPtrRegAddr);
		wrPtr = VpuReadReg(pCodecInst->coreIdx,
				   pEncInfo->streamWrPtrRegAddr);
		info->bitstreamBuffer = rdPtr;
		info->bitstreamSize = wrPtr - rdPtr;
		CVI_VC_TRACE("rdPtr = 0x%llX, wrPtr = 0x%llX, size = 0x%X\n",
			     rdPtr, wrPtr, info->bitstreamSize);
	}

	info->numOfSlices =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SLICE_NUM);
	info->bitstreamWrapAround =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_FLAG);
	info->reconFrameIndex =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_FRAME_IDX);
	if (info->reconFrameIndex < MAX_REG_FRAME) {
		info->reconFrame =
			pEncInfo->frameBufPool[info->reconFrameIndex];
	}
	info->encSrcIdx = info->reconFrameIndex;

	pEncInfo->streamWrPtr = vdi_remap_memory_address(
		pCodecInst->coreIdx,
		VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr));
	pEncInfo->streamEndflag =
		VpuReadReg(pCodecInst->coreIdx, BIT_BIT_STREAM_PARAM);

#ifdef SUPPORT_PIC_INFO_REPORT
	info->u32SumQp = VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SUM_QP);
	info->u32MeanQp =
		info->u32SumQp * 256 / (pEncInfo->openParam.picWidth *
				   pEncInfo->openParam.picHeight);
	CVI_VC_TRACE("u32SumQp = %d, u32MeanQp = %d\n", info->u32SumQp, info->u32MeanQp);
#if 0
	info->intraMbNum =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_INTRA_MB_NUM);
	info->skippedMbNum =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SKPPED_MB_NUM);
	info->sadSumLuma =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SAD_LUMA);
	info->sadSumChroma =
		VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_SAD_CHROMA);
#endif
#endif
	info->frameCycle = VpuReadReg(pCodecInst->coreIdx, BIT_FRAME_CYCLE);
	info->rdPtr = pEncInfo->streamRdPtr;
	info->wrPtr = pEncInfo->streamWrPtr;
	CVI_VC_FLOW("streamRdPtr = 0x%llX, streamWtPtr = 0x%llX\n",
		    pEncInfo->streamRdPtr, pEncInfo->streamWrPtr);
	//===================================================
	// @ Step 3 :
	//    set Qp for non-roi region or Qp for picture.
	//===================================================
	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC &&
	    pEncInfo->openParam.rcEnable == 4) {
#if ENABLE_RC_LIB
		Uint32 real_pic_bit;
		int avg_qp;

		avg_qp = VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_RC);
		avg_qp = (avg_qp & 0x7E) >> 1;
		real_pic_bit =
			VpuReadReg(pCodecInst->coreIdx, RET_ENC_PIC_USED_BIT);

		if (!pRcInfo->cviRcEn) {
			if (pRcInfo->rcMode != RC_MODE_UBR || pRcInfo->bTestUbrEn)
				rcLibUpdateRc(pCodecInst, info, avg_qp, real_pic_bit);
		}

		info->encPicByte = (real_pic_bit + 7) >> 3;
#endif
		pEncInfo->prevWrPtr = info->wrPtr;
	}

	if (pCodecInst->productId == PRODUCT_ID_980 &&
	    pCodecInst->codecMode == AVC_ENC &&
	    pCodecInst->codecModeAux == AVC_AUX_SVC) {
		int i;
		for (i = 0; i < pEncInfo->num_mmco1; i++)
			pEncInfo->frm[pEncInfo->mmco1_frmidx[i]].used_for_ref =
				0;

		if (pEncInfo->is_skip_picture) {
			if (pEncInfo->curr_long_term) {
				if (pEncInfo->longterm_frmidx >= 0)
					pEncInfo->curr_frm->used_for_ref = 0;

				pEncInfo->longterm_frmidx =
					pEncInfo->curr_frm - pEncInfo->frm;
			}

			pEncInfo->frm[pEncInfo->ref_pic_list].frame_num =
				pEncInfo->curr_frm->frame_num;
			pEncInfo->frm[pEncInfo->ref_pic_list].poc =
				pEncInfo->curr_frm->poc;
			pEncInfo->frm[pEncInfo->ref_pic_list].slice_type =
				pEncInfo->curr_frm->slice_type;
			pEncInfo->frm[pEncInfo->ref_pic_list].used_for_ref =
				pEncInfo->curr_frm->used_for_ref;

			pEncInfo->curr_frm->used_for_ref = 0;
		} else {
			if (pEncInfo->curr_long_term) {
				if (!pEncInfo->singleLumaBuf) {
					if (pEncInfo->longterm_frmidx >= 0)
						pEncInfo->frm
							[pEncInfo->longterm_frmidx]
								.used_for_ref =
							0;
				}

				pEncInfo->longterm_frmidx =
					pEncInfo->curr_frm - pEncInfo->frm;
			}
		}

		pEncInfo->enc_idx_modulo++;
		if (pEncInfo->enc_idx_modulo == pEncInfo->gop_size) {
			pEncInfo->enc_idx_modulo = 0;
			pEncInfo->enc_idx_gop += pEncInfo->gop_size;
		}
	}

	if (pEncInfo->fieldDone == FALSE)
		pEncInfo->encoded_frames_in_gop++;

	return RETCODE_SUCCESS;
}

void rcLibUpdateRc(CodecInst *pCodecInst, EncOutputInfo *info, int avg_qp,
		   int real_pic_bit)
{
	EncInfo *pEncInfo = &pCodecInst->CodecInfo->encInfo;
	stRcInfo *pRcInfo = &pCodecInst->rcInfo;
#ifdef AUTO_FRM_SKIP_DROP
	int frame_skip;
	frame_skip = pEncInfo->frm_rc.rc_pic.frame_skip;
#else
	frame_skip = 0;
#endif

	CVI_VC_RC("frame_drop = %d\n", pEncInfo->frm_rc.rc_pic.frame_drop);

	if (pEncInfo->frm_rc.rc_pic.frame_drop) {
		info->picDropped = 1;
		return;
	}

	CVI_VC_RC("avg_qp = %d, real_pic_bit = %d, frame_skip = %d\n", avg_qp,
		  real_pic_bit, pEncInfo->frm_rc.rc_pic.frame_skip);

	// I frame RQ model update
	if ((info->picType == PIC_TYPE_I || info->picType == PIC_TYPE_IDR) &&
	    (pRcInfo->maxIPicBit > 0)) {
	}
}

#ifdef REDUNDENT_CODE
RetCode Coda9VpuEncGiveCommand(CodecCommand cmd, void *param)
{
	RetCode ret = RETCODE_SUCCESS;

	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(param);

	switch (cmd) {
	default:
		ret = RETCODE_NOT_SUPPORTED_FEATURE;
	}

	return ret;
}
#endif
