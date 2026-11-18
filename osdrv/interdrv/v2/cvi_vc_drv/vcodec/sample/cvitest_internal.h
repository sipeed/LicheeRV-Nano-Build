/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: Dec, 2019
 */
#ifndef __CVITEST_INTERNAL_H__
#define __CVITEST_INTERNAL_H__

#include <linux/completion.h>

#include "main_helper.h"
#include "vpuapi.h"
#include "cvi_h265_interface.h"
#include "vdi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_CTU_NUM 0x4000 // CTU num for max resolution = 8192x8192/(64x64)
#define NUM_OF_BS_BUF 1

#define CACHE_ENCODE_HEADER 1

//is bitstreambuff is not enough, drop frame
//or alloc ion to store it
#define DROP_FRAME 1

typedef struct _stTestEncoder_ {
	EncHandle handle;
	EncOpenParam encOP;
	EncParam encParam;
	EncInitialInfo initialInfo;
	EncOutputInfo outputInfo;
	SecAxiUse secAxiUse;
	vpu_buffer_t vbStream[NUM_OF_BS_BUF];
	Uint32 bsBufferCount;
	Uint32 bsQueueIndex;
	vpu_buffer_t vbReport;
	vpu_buffer_t vbReconFrameBuf[MAX_REG_FRAME];
	vpu_buffer_t vbSourceFrameBuf[MAX_REG_FRAME];
	FrameBuffer fbSrc[ENC_SRC_BUF_NUM];
#ifdef SUPPORT_DIRECT_YUV
	FrameBuffer fbSrcFixed;
#endif
	FrameBufferAllocInfo fbAllocInfo;
	int ret;
	EncHeaderParam encHeaderParam;
#if CACHE_ENCODE_HEADER
	int bEncHeader;
	EncHeaderParam headerBackup[8];
#endif
	int srcFrameIdx;
	Uint32 frameIdx;
	int srcFrameWidth, srcFrameHeight, srcFrameStride;
	int framebufStride, framebufWidth, framebufHeight;
	int suc;
	int regFrameBufCount;
	int coreIdx;
	TestEncConfig encConfig;
	Int32 productId;
	Comparator comparatorBitStream;
	BitstreamReader bsReader;
	FrameBuffer fbRecon[MAX_REG_FRAME];
	vpu_buffer_t vbRoi[MAX_REG_FRAME];
	vpu_buffer_t vbRoiBin[MAX_REG_FRAME];
	vpu_buffer_t vbCtuQp[MAX_REG_FRAME];
	Uint8 ctuQpMapBuf[MAX_CTU_NUM];
	// for encode host rbsp header
	vpu_buffer_t vbPrefixSeiNal[MAX_REG_FRAME];
	vpu_buffer_t vbSuffixSeiNal[MAX_REG_FRAME];
	vpu_buffer_t vbHrdRbsp;
	vpu_buffer_t vbVuiRbsp;
#ifdef TEST_ENCODE_CUSTOM_HEADER
	vpu_buffer_t vbSeiNal[MAX_REG_FRAME];
	vpu_buffer_t vbCustomHeader;
#endif

	Uint8 roiMapBuf[MAX_CTU_NUM];
	void *yuvFeeder;
	YuvInfo yuvFeederInfo;
	Uint32 interruptTimeout;
#ifdef TEST_ENCODE_CUSTOM_HEADER
	hrd_t hrd;
#endif
	feederYuvaddr yuvAddr;

	stStreamPack streamPack;
	Uint8 bIsEncoderInited;

	/* for handle BIT_BUF_FULL interrupt */
	Uint8 *pu8PreStream;
	Uint32 u32PreStreamLen;
	Uint32 u32BsRotateOffset;
#ifdef DROP_FRAME
	Uint8 bDrop; // Drop bitstream
	int originBitrate; // srore rc origin bitrate
#endif
	struct task_struct *tPthreadId;
	struct completion semSendEncCmd;
	struct completion semGetStreamCmd;
	struct completion semEncDoneCmd;

	Uint8 tPthreadRunFlag;
	cviVEncStreamInfo tStreamInfo;
	Uint64 u64Pts;
	cviVencSbSetting SbSetting;
} stTestEncoder;

int initEncoder(stTestEncoder *pTestEnc, TestEncConfig *pEncConfig);
void initDefaultEncConfig(TestEncConfig *pEncConfig);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
