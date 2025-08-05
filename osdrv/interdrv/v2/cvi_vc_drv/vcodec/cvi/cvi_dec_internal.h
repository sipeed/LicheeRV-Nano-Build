/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: Aug, 2020
 */

#ifndef __CVI_DEC_INTERNAL_H__
#define __CVI_DEC_INTERNAL_H__
#include "main_helper.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PPU_FB_COUNT 2
#define MAX_COUNT_NO_RESPONSE 3 // For integration test.
#define STREAM_BUF_SIZE 0x700000 // max bitstream size
#define EXTRA_FRAME_BUFFER_NUM 1
#define MAX_SEQUENCE_MEM_COUNT 16
#define HEVC_OUTPUT_FP_NUMBER 4
#define MAX_NUM_BS_BUFFER 2

typedef enum _CVI_VB_SOURCE_ {
	E_CVI_VB_SRC_COMMON = 0,
	E_CVI_VB_SRC_MODULE = 1,
	E_CVI_VB_SRC_PRIVATE = 2,
	E_CVI_VB_SRC_USER = 3,
	E_CVI_VB_SRC_BUTT
} E_CVI_VB_SOURCE;

typedef enum _CVI_DEC_STATUS_ {
	CVI_ERR_DECODE_END = -100,
	CVI_ERR_DEC_INIT,
	CVI_ERR_DEC_ARGV,
	CVI_ERR_DEC_MCU,
	CVI_ERR_ALLOC_VDEC,
	CVI_ERR_WRITE_ERR,
	CVI_ERR_VIR_ADDR,
	CVI_INIT_DECODER_OK = 30,
	CVI_INIT_SEQ_OK,
	CVI_DECODE_ONE_FRAME_OK,
	CVI_WAIT_INT_OK,
	CVI_DECODE_DATA_OK,
	CVI_DECODE_CONTINUE,
	CVI_DECODE_BREAK,
	CVI_DECODE_NO_FB,
	CVI_DISP_LAST_FRM,
	CVI_SEQ_CHANGE,
	CVI_SEQ_CHG_FLUSH,
	CVI_SEQ_CHG_WAIT_BUF_FREE,
	CVI_DECODE_NO_FB_WITH_DISP,
} CVI_DEC_STATUS;

typedef struct cviDecConfig_struct {
	Uint32 magicNumber;
	char outputPath[MAX_FILE_PATH];
	char inputPath[MAX_FILE_PATH];
	Int32 forceOutNum;
	Int32 bitFormat;
	Int32 reorder;
	TiledMapType mapType;
	Int32 bitstreamMode;
	FeedingMethod feedingMode;
	BOOL enableWTL;
	FrameFlag wtlMode;
	FrameBufferFormat wtlFormat;
	Int32 coreIdx;
	Int32 instIdx;
	BOOL enableCrop; //!<< option for saving yuv
	Uint32 loopCount;
	BOOL cbcrInterleave; //!<< 0: None, 1: NV12, 2: NV21
	BOOL nv21; //!<< FALSE: NV12, TRUE: NV21,
		//!<< This variable is valid when cbcrInterleave is TRUE
	EndianMode streamEndian;
	EndianMode frameEndian;
	Int32 secondaryAXI;
	Int32 compareType;
	char md5Path[MAX_FILE_PATH];
	char fwPath[MAX_FILE_PATH];
	char refYuvPath[MAX_FILE_PATH];
	RenderDeviceType renderType;
	BOOL thumbnailMode;
	Int32 skipMode;
	size_t bsSize;
	struct {
		BOOL enableMvc; //!<< H.264 MVC
		BOOL enableSvc;
		BOOL enableTiled2Linear;
		FrameFlag tiled2LinearMode;
		BOOL enableBWB;
		Uint32 rotate; //!<< 0, 90, 180, 270
		Uint32 mirror;
		BOOL enableDering; //!<< MPEG-2/4
		BOOL enableDeblock; //!<< MPEG-2/4
		Uint32 mp4class; //!<< MPEG_4
		Uint32 frameCacheBypass;
		Uint32 frameCacheBurst;
		Uint32 frameCacheMerge;
		Uint32 frameCacheWayShape;
		LowDelayInfo lowDelay; //!<< H.264
	} coda9;
	struct {
		Uint32 numVCores; //!<< This numVCores is valid on
			//!<PRODUCT_ID_4102 multi-core version
		Uint32 fbcMode;
		BOOL bwOptimization; //!<< On/Off bandwidth optimization
			//!<function
		BOOL craAsBla;
		BOOL dualDisplay;
	} wave4;
	BOOL enableUserData;
	Uint32 testEnvOptions; /*!<< See debug.h */
	TestMachine *testMachine;
	Uint32 tid; /*!<< Target temporal id for AVC and HEVC */
	// FW usage
	Uint32 sizeInWord;
	Uint16 *pusBitCode;
	Int32 reorderEnable;
} cviDecConfig;

typedef struct _cviSdkParam_ {
	vpu_buffer_t bsBuf;
} cviSdkParam;

typedef struct {
	DecGetFramebufInfo fbInfo;
	vpu_buffer_t allocFbMem[MAX_REG_FRAME];
} SequenceMemInfo;

typedef struct _cviVideoDecoder_ {
	Int32 cviApiMode;
	cviSdkParam sdkParam;
	Int32 framebufHeight;
	BOOL doDumpImage;
	Int32 fbCount;
	Int32 ppuFbCount;
	Int32 decodedIdx;
	Int32 frameNum;

	Uint32 dispIdx;
	Uint32 noFbCount;
	BOOL assertedFieldDoneInt;
	Uint32 noResponseCount;
	Int32 seqChangedStreamEndFlag;
	Int32 seqChangedRdPtr;
	Int32 seqChangedWrPtr;
	Int32 nWritten;
	Int32 productId;
	Int32 prevFbIndex;
	BOOL seqInited;
	BOOL waitPostProcessing;
	BOOL needStream;
	BOOL seqChangeRequest;
	BOOL enablePPU;
	BOOL success;
	FrameBuffer *ppuFb;
	Comparator comparator;
	BSFeeder feeder;
	Renderer renderer;
	DecHandle decHandle;
	Queue *ppuQ;
	VpuRect rcPpu;
	MaverickCacheConfig cacheCfg;
	cviDecConfig decConfig;
	DecInitialInfo sequenceInfo;
	DecOutputInfo outputInfo;
	DecParam decParam;
	vpu_buffer_t vbStream[MAX_NUM_BS_BUFFER];
	SecAxiUse secAxiUse;
	VpuReportConfig_t decReportCfg;
	FrameBufferAllocInfo fbAllocInfo;
	vpu_buffer_t fbMem[MAX_REG_FRAME];
	vpu_buffer_t PPUFbMem[MAX_REG_FRAME];
	FrameBuffer PPUFrame[MAX_REG_FRAME];
	FrameBuffer Frame[MAX_REG_FRAME];
	// h265 only
	Queue *displayQ;
	Queue *sequenceQ;
	SequenceMemInfo seqMemInfo[MAX_SEQUENCE_MEM_COUNT];
	Uint32 bsBufferCount; //!<< In PICEND mode, this variable is greater than 1.
	Uint32 bsQueueIndex;
	vpu_buffer_t vbUserData;
	Uint32 sequenceChangeFlag;
	BOOL bStreamOfEnd;
	E_CVI_VB_SOURCE eVdecVBSource;
	vpu_buffer_t vbFrame[MAX_REG_FRAME];
	Uint32 nVbFrameNum;
	Int32 chnNum;
	BOOL waitFrmBufFree;
	Int32 ReorderEnable;
} cviVideoDecoder;

cviVideoDecoder *cviAllocVideoDecoder(void);
void cviFreeVideoDecoder(cviVideoDecoder **ppvdec);
int cviInitDecMcuEnv(cviDecConfig *pdeccfg);
void cviFreeDecMcuEnv(cviVideoDecoder *pvdec);
int cviInitDecoder(cviVideoDecoder *pvdec);
int cviInitSeqSetting(cviVideoDecoder *pvdec);
int cviDecodeOneFrame(cviVideoDecoder *pvdec);
int cviWaitInterrupt(cviVideoDecoder *pvdec);
int cviGetDecodedData(cviVideoDecoder *pvdec);
void cviCloseDecoder(cviVideoDecoder *pvdec);
void cviDeInitDecoder(cviVideoDecoder *pvdec);
void initDefaultcDecConfig(cviVideoDecoder *pvdec);

int parseVdecArgs(int argc, char **argv, cviDecConfig *pdeccfg);

void checkDecConfig(cviDecConfig *pdeccfg);

int cviAttachFrmBuf(cviVideoDecoder *pvdec, cviBufInfo *pFrmBufArray,
		    int nFrmNum);

typedef int (*CVI_VDEC_CALLBACK)(unsigned int nChn, unsigned int nCbType,
				 void *pArg);

void cviDecAttachCallBack(CVI_VDEC_CALLBACK pCbFunc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
