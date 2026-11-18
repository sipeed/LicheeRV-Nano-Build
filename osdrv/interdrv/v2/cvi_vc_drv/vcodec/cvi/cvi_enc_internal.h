/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: May, 2020
 */

#ifndef __CVI_ENC_INTERNAL_H__
#define __CVI_ENC_INTERNAL_H__

#include "cvitest_internal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VC_FAB_MMU_CTRL			0x40
#define H265_MAP_REG_OFFSET		0x200

// cvi_enc_internal.c
int cviCheckIdrPeriod(stTestEncoder *pTestEnc);
int cviEncode264Header(stTestEncoder *pTestEnc);
int cviEncode265Header(stTestEncoder *pTestEnc);
int cviInsertUserData(stTestEncoder *pTestEnc);
int cviPutEsInPack(stTestEncoder *pTestEnc, PhysicalAddress paBsBufStart,
		   PhysicalAddress paBsBufEnd, Uint32 esSize, Int32 cviNalType);
void cviPrintRc(stTestEncoder *pTestEnc);
int cviSetupPicConfig(stTestEncoder *pTestEnc);
int cviInitAddrRemap(stTestEncoder *pTestEnc);

// vpuapi.c
RetCode cviConfigEncParam(CodecInst *pCodec, EncOpenParam *param);

// coda9.c / wave4.c
int cviInitAddrRemapFb(CodecInst *pCodecInst);
void cviSetApiMode(EncHandle pHandle, int cviApiMode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
