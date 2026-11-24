//------------------------------------------------------------------------------
// File: main_helper.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef _MAIN_HELPER_H_
#define _MAIN_HELPER_H_

#include "config.h"
#include "vpuapifunc.h"
#include "vpuapi.h"
#include "vputypes.h"
#ifdef PLATFORM_QNX
#include <sys/stat.h>
#endif

#ifdef SUPPORT_ENCODE_CUSTOM_HEADER
#include "./misc/header_struct.h"
#endif

#define MATCH_OR_MISMATCH(_expected, _value, _ret)                             \
	((_ret = (_expected == _value)) ? "MATCH" : "MISMATCH")

#if defined(WIN32) || defined(WIN64)
/*
 ( _MSC_VER => 1200 )     6.0     vs6
 ( _MSC_VER => 1310 )     7.1     vs2003
 ( _MSC_VER => 1400 )     8.0     vs2005
 ( _MSC_VER => 1500 )     9.0     vs2008
 ( _MSC_VER => 1600 )    10.0     vs2010
 */
#if (_MSC_VER == 1200)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#else
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
#define inline _inline
#if (_MSC_VER == 1600)
#define strdup _strdup
#endif
#endif

#define MAX_GETOPT_OPTIONS 100
// extension of option struct in getopt
struct OptionExt {
	const char *name;
	int has_arg;
	int *flag;
	int val;
	const char *help;
};

#define MAX_FILE_PATH 256
#define MAX_PIC_SKIP_NUM 5
#ifdef SUPPORT_SRC_BUF_CONTROL
#define ENC_SRC_BUF_NUM 2000
#else
//!< case of GOPsize = 8 (IBBBBBBBP), max src buffer num  = 13
#define ENC_SRC_BUF_NUM 3
#endif

#define EXTRA_SRC_BUFFER_NUM 0
#define VPU_WAIT_TIME_OUT 10
/* should be less than normal decoding time to give a chance to fill
		 stream. if this value happens some problem. we should fix
		 VPU_WaitInterrupt function */
#define VPU_WAIT_TIME_OUT_CQ 1
#define PARALLEL_VPU_WAIT_TIME_OUT 1
/* the value of timeout is 1 means we want to keep a waiting time to
		 give a chance of an interrupt of the next core. */

#define NUM_OF_USER_DATA_BUF 4

extern char *productNameList[];

typedef union {
	struct {
		Uint32 ctu_force_mode : 2; //[ 1: 0]
		Uint32 ctu_coeff_drop : 1; //[    2]
		Uint32 reserved : 5; //[ 7: 3]
		Uint32 sub_ctu_qp_0 : 6; //[13: 8]
		Uint32 sub_ctu_qp_1 : 6; //[19:14]
		Uint32 sub_ctu_qp_2 : 6; //[25:20]
		Uint32 sub_ctu_qp_3 : 6; //[31:26]

		Uint32 lambda_sad_0 : 8; //[39:32]
		Uint32 lambda_sad_1 : 8; //[47:40]
		Uint32 lambda_sad_2 : 8; //[55:48]
		Uint32 lambda_sad_3 : 8; //[63:56]
	} field;
} EncCustomMap; // for wave520 custom map (1 CTU = 64bits)

typedef enum {
	MODE_YUV_LOAD = 0,
	MODE_COMP_JYUV,
	MODE_SAVE_JYUV,

	MODE_COMP_CONV_YUV,
	MODE_SAVE_CONV_YUV,

	MODE_SAVE_LOAD_YUV,

	MODE_COMP_RECON,
	MODE_SAVE_RECON,

	MODE_COMP_ENCODED,
	MODE_SAVE_ENCODED
} CompSaveMode;

typedef enum {
	CVI_CAVLC = 0,
	CVI_CABAC = 1,
} CviEntropyMode;

typedef struct {
	int picX;
	int picY;
	int internalBitDepth;
	int losslessEnable;
	int constIntraPredFlag;
	int gopSize;
	int numTemporalLayers;
	int decodingRefreshType;
	int intraQP;
	int intraPeriod;
	int frameRate;

	int confWinTop;
	int confWinBot;
	int confWinLeft;
	int confWinRight;

	int independSliceMode;
	int independSliceModeArg;
	int dependSliceMode;
	int dependSliceModeArg;
	int intraRefreshMode;
	int intraRefreshArg;

	int useRecommendEncParam;
	int scalingListEnable;
	int cuSizeMode;
	int tmvpEnable;
	int wppenable;
	int maxNumMerge;
	int dynamicMerge8x8Enable;
	int dynamicMerge16x16Enable;
	int dynamicMerge32x32Enable;
	int disableDeblk;
	int lfCrossSliceBoundaryEnable;
	int betaOffsetDiv2;
	int tcOffsetDiv2;
	int skipIntraTrans;
	int saoEnable;
	int intraInInterSliceEnable;
	int intraNxNEnable;
	int rcEnable;

	int bitRate;
	int intraQpOffset;
	int initBufLevelx8;
	int bitAllocMode;
	int fixedBitRatio[MAX_GOP_NUM];
	int cuLevelRCEnable;
	int hvsQPEnable;

	int hvsQpScaleEnable;
	int hvsQpScale;
	int minQp;
	int maxQp;
	int maxDeltaQp;

	int initDelay;

	int transRate;
	int gopPresetIdx;
	// CUSTOM_GOP
	CustomGopParam gopParam;

	// ROI / CTU mode
	HevcCtuOptParam ctuOptParam;
	char roiFileName[MAX_FILE_PATH];
	char ctuModeFileName[MAX_FILE_PATH];
	char ctuQpFileName[MAX_FILE_PATH];

	// VUI
	HevcVuiParam vuiParam;
	Uint32 numUnitsInTick;
	Uint32 timeScale;
	Uint32 numTicksPocDiffOne;

	int encAUD;
	int encEOS;
	int encEOB;

	int chromaCbQpOffset;
	int chromaCrQpOffset;

	int initialRcQp;

	Uint32 nrYEnable;
	Uint32 nrCbEnable;
	Uint32 nrCrEnable;
	Uint32 nrNoiseEstEnable;
	Uint32 nrNoiseSigmaY;
	Uint32 nrNoiseSigmaCb;
	Uint32 nrNoiseSigmaCr;

	Uint32 nrIntraWeightY;
	Uint32 nrIntraWeightCb;
	Uint32 nrIntraWeightCr;

	Uint32 nrInterWeightY;
	Uint32 nrInterWeightCb;
	Uint32 nrInterWeightCr;

	Uint32 intraMinQp;
	Uint32 intraMaxQp;

	Uint32 useAsLongtermPeriod;
	Uint32 refLongtermPeriod;
	Uint32 vuiDataEnable;
	Uint32 vuiDataSize;
	char vuiDataFileName[MAX_FILE_PATH];
	Uint32 hrdInVPS;
	Uint32 hrdInVUI;
	Uint32 hrdDataSize;
	char hrdDataFileName[MAX_FILE_PATH];
	Uint32 prefixSeiEnable;
	Uint32 prefixSeiDataSize;
	Uint32 prefixSeiTimingFlag;
	char prefixSeiDataFileName[MAX_FILE_PATH];
	Uint32 suffixSeiEnable;
	Uint32 suffixSeiDataSize;
	Uint32 suffixSeiTimingFlag;
	char suffixSeiDataFileName[MAX_FILE_PATH];
	int forcedIdrHeaderEnable;

	// newly added for WAVE520
#ifdef SUPPORT_HOST_RC_PARAM
	int hostPicRcEnable;
	char hostPicRcFileName[MAX_FILE_PATH];
#endif
} HEVC_ENC_CFG;

typedef struct {
	char SrcFileName[MAX_FILE_PATH];
	char BitStreamFileName[MAX_FILE_PATH];
	BOOL srcCbCrInterleave;
	int NumFrame;
	int PicX;
	int PicY;
	int FrameRate;

	// MPEG4 ONLY
	int VerId;
	int DataPartEn;
	int RevVlcEn;
	int ShortVideoHeader;
	int AnnexI;
	int AnnexJ;
	int AnnexK;
	int AnnexT;
	int IntraDcVlcThr;
	int VopQuant;

	int frameCroppingFlag;
	int frameCropLeft;
	int frameCropRight;
	int frameCropTop;
	int frameCropBottom;

	// H.264 ONLY
	int ConstIntraPredFlag;
	int DisableDeblk;
	int DeblkOffsetA;
	int DeblkOffsetB;
	int ChromaQpOffset;
	int PicQpY;
	// H.264 VUI information
	int VuiPresFlag;
	int VideoSignalTypePresFlag;
	char VideoFormat;
	char VideoFullRangeFlag;
	int ColourDescripPresFlag;
	char ColourPrimaries;
	char TransferCharacteristics;
	char MatrixCoeff;
	int NumReorderFrame;
	int MaxDecBuffering;
	int aud_en;
	int level;
	// COMMON
	int GopPicNum;
	int SliceMode;
	int SliceSizeMode;
	int SliceSizeNum;
	// COMMON - RC
	int RcEnable;
	int RcBitRate;
	int RcInitDelay;
	int RcBufSize;
	int IntraRefreshNum;
	int ConscIntraRefreshEnable;
	int CountIntraMbEnable;
	int FieldSeqIntraRefreshEnable;
	int frameSkipDisable;
	int ConstantIntraQPEnable;
	int MaxQpSetEnable;
	int MaxQp;
	// H.264 only
	int MaxDeltaQpSetEnable;
	int MaxDeltaQp;
	int MinQpSetEnable;
	int MinQp;
	int MinDeltaQpSetEnable;
	int MinDeltaQp;
	int intraCostWeight;

	// MP4 Only
	int RCIntraQP;
	int HecEnable;

	int GammaSetEnable;
	int Gamma;

	// NEW RC Scheme
	int rcIntervalMode;
	int RcMBInterval;
	int skipPicNums[MAX_PIC_SKIP_NUM];
	int RcMaxIntraSize;
	int SearchRangeX;
	int SearchRangeY;
	// H.264 ONLY
	int entropyCodingMode;
	int cabacInitIdc;
	int transform8x8Mode;
	int chroma_format_400;
	int field_flag;
	int field_ref_mode;
	int RcGopIQpOffsetEn;
	int RcGopIQpOffset;
	int coda9RoiEnable;
	char RoiFile[MAX_FILE_PATH];
	int RoiPicAvgQp;
	int set_dqp_pic_num;
	int HvsQpScaleDiv2;
	int EnHvsQp;
	int EnRowLevelRc;
	int RcInitialQp;
	int RcHvsMaxDeltaQp;

	gop_entry_t gop_entry[MAX_GOP_SIZE * 2];
	int GopPreset; // 0: user specified, 1: normal, 2, 3 svc-T layer coding

	int LongTermPeriod;
	int LongTermDeltaQp;
	int VirtualIPeriod;
	int dqp[8];

#ifdef ROI_MB_RC
	int roi_max_delta_qp_minus;
	int roi_max_delta_qp_plus;
#endif

	char pchChangeCfgFileName[MAX_FILE_PATH];
	int ChangeFrameNum;
	int paraEnable;

	int IntraMbRefreshNum;
	int NewGopNum;
	int NewIntraQp;
	int NewBitrate;
	int NewFrameRate;
	int NewIntraRefresh;
	MinMaxQpChangeParam minMaxQpParam;

	int interviewEn;
	int parasetRefreshEn;
	int prefixNalEn;
	int MeUseZeroPmv; // will be removed. must be 264 = 0, mpeg4 = 1 263 = 0
	int MeBlkModeEnable; // only api option
	int IDRInterval;
	int SrcBitDepth;

	HEVC_ENC_CFG hevcCfg;
#ifdef AUTO_FRM_SKIP_DROP
	int enAutoFrmSkip;
	int enAutoFrmDrop;
	int vbvThreshold;
	int qpThreshold;
	int maxContinuosFrameSkipNum;
	int maxContinuosFrameDropNum;
#endif
	int rcWeightFactor;
} ENC_CFG;

extern Uint32 randomSeed;

/* yuv & md5 */
#define NO_COMPARE 0
#define YUV_COMPARE 1
#define MD5_COMPARE 2
#define STREAM_COMPARE 3

//#define FIRMWARE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Performance report */
typedef void *PFCtx;

PFCtx PFMonitorSetup(Uint32 coreIndex, Uint32 instanceIndex,
		     Uint32 referenceClkInMHz, Uint32 fps, char *strLogDir);

void PFMonitorRelease(PFCtx context);

void PFMonitorUpdate(PFCtx context, Uint32 cycles);

#ifdef REDUNDENT_CODE
void PrepareDecoderTest(DecHandle decHandle);

void PreparationWorkForDecTest(DecHandle decHandle);

void PreparationWorkForEncTest(EncHandle handle);

void byte_swap(unsigned char *data, int len);
#endif

#ifdef FIRMWARE_H
Int32 LoadFirmwareH(Int32 productId, Uint8 **retFirmware,
		    Uint32 *retSizeInWord);
#else
Int32 LoadFirmware(Int32 productId, Uint8 **retFirmware, Uint32 *retSizeInWord,
		   const char *path);
#endif

void PrintDecSeqWarningMessages(Uint32 productId, DecInitialInfo *seqInfo);

void DisplayEncodedInformation(EncHandle handle, CodStd codec,
			       EncOutputInfo *encodedInfo, ...);

void PrintEncSppStatus(Uint32 coreIdx, Uint32 productId);

void WriteRegVCE(Uint32 core_idx, Uint32 vce_core_idx, Uint32 vce_addr,
		 Uint32 udata);

Uint32 ReadRegVCE(Uint32 core_idx, Uint32 vce_core_idx, Uint32 vce_addr);

/*
 * VPU Helper functions
 */
/************************************************************************/
/* Video                                                                */
/************************************************************************/

#define PUT_BYTE(_p, _b)                                                       \
	{                                                                   \
		*_p++ = (unsigned char)_b;                                     \
	}

#define PUT_BUFFER(_p, _buf, _len)                                             \
	do {                                                                   \
		osal_memcpy(_p, _buf, _len);                                   \
		_p += _len;                                                    \
		while (0)

#define PUT_LE32(_p, _var)                                                     \
	do {                                                                   \
		*_p++ = (unsigned char)((_var) >> 0);                          \
		*_p++ = (unsigned char)((_var) >> 8);                          \
		*_p++ = (unsigned char)((_var) >> 16);                         \
		*_p++ = (unsigned char)((_var) >> 24);                         \
		while (0)

#define PUT_BE32(_p, _var)                                                     \
	do {                                                                   \
		*_p++ = (unsigned char)((_var) >> 24);                         \
		*_p++ = (unsigned char)((_var) >> 16);                         \
		*_p++ = (unsigned char)((_var) >> 8);                          \
		*_p++ = (unsigned char)((_var) >> 0);                          \
		while (0)

#define PUT_LE16(_p, _var)                                                     \
	do {                                                                   \
		*_p++ = (unsigned char)((_var) >> 0);                          \
		*_p++ = (unsigned char)((_var) >> 8);                          \
		while (0)

#define PUT_BE16(_p, _var)                                                     \
	do {                                                                   \
		*_p++ = (unsigned char)((_var) >> 8);                          \
		*_p++ = (unsigned char)((_var) >> 0);                          \
		while (0)

Int32 ConvFOURCCToMp4Class(Int32 fourcc);

Int32 ConvFOURCCToCodStd(Uint32 fourcc);

Int32 ConvCodecIdToMp4Class(Uint32 codecId);

Int32 ConvCodecIdToCodStd(Int32 codecId);

Int32 ConvCodecIdToFourcc(Int32 codecId);

/*!
 * \brief       wrapper function of StoreYuvImageBurstFormat()
 */
Uint8 *GetYUVFromFrameBuffer(DecHandle decHandle, FrameBuffer *fb,
			     VpuRect rcFrame, Uint32 *retWidth,
			     Uint32 *retHeight, Uint32 *retBpp,
			     size_t *retSize);

/************************************************************************/
/* VpuMutex                                                                */
/************************************************************************/
typedef void *VpuMutex;

VpuMutex VpuMutex_Create(void);

void VpuMutex_Destroy(VpuMutex handle);

BOOL VpuMutex_Lock(VpuMutex handle);

BOOL VpuMutex_Unlock(VpuMutex handle);

/************************************************************************/
/* Queue                                                                */
/************************************************************************/
typedef struct {
	void *data;
} QueueData;
typedef struct {
	Uint8 *buffer;
	Uint32 size;
	Uint32 itemSize;
	Uint32 count;
	Uint32 front;
	Uint32 rear;
	VpuMutex lock;
} Queue;

Queue *Queue_Create(Uint32 itemCount, Uint32 itemSize);

#ifdef REDUNDENT_CODE
Queue *Queue_Create_With_Lock(Uint32 itemCount, Uint32 itemSize);
#endif

void Queue_Destroy(Queue *queue);

/**
 * \brief       Enqueue with deep copy
 */
BOOL Queue_Enqueue(Queue *queue, void *data);

/**
 * \brief       Caller has responsibility for releasing the returned data
 */
void *Queue_Dequeue(Queue *queue);

#ifdef REDUNDENT_CODE
void Queue_Flush(Queue *queue);
#endif

void *Queue_Peek(Queue *queue);

#ifdef REDUNDENT_CODE
Uint32 Queue_Get_Cnt(Queue *queue);

/**
 * \brief       @dstQ is NULL, it allocates Queue structure and then copy from
 * @srcQ.
 */
Queue *Queue_Copy(Queue *dstQ, Queue *srcQ);
#endif

/************************************************************************/
/* VpuThread                                                               */
/************************************************************************/
typedef void *VpuThread;
typedef void (*VpuThreadRunner)(void *);

VpuThread VpuThread_Create(VpuThreadRunner func, void *arg);

BOOL VpuThread_Join(VpuThread thread);

/*!
 * \brief           millisecond sleep
 */
void MSleep(Uint32 ms //!<< millisecond
);

/************************************************************************/
/* ETC                                                                  */
/************************************************************************/
#ifdef REDUNDENT_CODE
Uint32 GetRandom(Uint32 start, Uint32 end);
#endif

#ifdef PLATFORM_WIN32
struct timezone {
	Int32 tz_minuteswest; /* minutes W of Greenwich */
	BOOL tz_dsttime; /* type of dst correction */
};

Int32 gettimeofday(struct timeval *tv, struct timezone *tz);
#endif
/************************************************************************/
/* MD5                                                                  */
/************************************************************************/
#ifdef REDUNDENT_CODE
typedef struct MD5state_st {
	Uint32 A, B, C, D;
	Uint32 Nl, Nh;
	Uint32 data[16];
	Uint32 num;
} MD5_CTX;

Int32 MD5_Init(MD5_CTX *c);

Int32 MD5_Update(MD5_CTX *c, const void *data, size_t len);

Int32 MD5_Final(Uint8 *md, MD5_CTX *c);

Uint8 *MD5(const Uint8 *d, size_t n, Uint8 *md);
void plane_md5(MD5_CTX *md5_ctx, Uint8 *src, int src_x, int src_y, int out_x,
	       int out_y, int stride, int bpp, Uint16 zero);

/************************************************************************/
/* Comparator                                                           */
/************************************************************************/
#define COMPARATOR_SKIP 0xF0F0F0F0
typedef enum {
	COMPARATOR_CONF_SET_GOLDEN_DATA_SIZE,
	COMPARATOR_CONF_SKIP_GOLDEN_DATA, /*!<< 2nd parameter pointer of Queue
						  containing skip command */
	COMPARATOR_CONF_SET_PICINFO, //!<< This command is followed by YUVInfo
	//!<structure.
} ComparatorConfType;
#endif
typedef void *Comparator;
#ifdef REDUNDENT_CODE
typedef struct ComparatorImpl {
	void *context;
	char *filename;
	Uint32 curIndex;
	Uint32 numOfFrames;
	BOOL (*Create)(struct ComparatorImpl *impl, char *path);
	BOOL (*Destroy)(struct ComparatorImpl *impl);
	BOOL (*Compare)(struct ComparatorImpl *impl, void *data, Uint32 size);
	BOOL(*Configure)
	(struct ComparatorImpl *impl, ComparatorConfType type, void *val);
	BOOL (*Rewind)(struct ComparatorImpl *impl);
	BOOL eof;
	BOOL enableScanMode;
	BOOL usePrevDataOneTime;
} ComparatorImpl;

typedef struct {
	Uint32 totalFrames;
	ComparatorImpl *impl;
} AbstractComparator;

// YUV Comparator
typedef struct {
	Uint32 width;
	Uint32 height;
	FrameBufferFormat format;
	BOOL cbcrInterleave;
	BOOL isVp9;
} PictureInfo;

Comparator Comparator_Create(Uint32 type, //!<<   1: yuv
			     char *goldenPath, ...);

BOOL Comparator_Destroy(Comparator comp);

BOOL Comparator_Act(Comparator comp, void *data, Uint32 size);

BOOL Comparator_CheckFrameCount(Comparator comp);

BOOL Comparator_SetScanMode(Comparator comp, BOOL enable);

BOOL Comparator_Rewind(Comparator comp);

BOOL Comparator_CheckEOF(Comparator comp);

Queue *Comprator_MakeSkipData(Comparator comp, const char *skipCmd);

BOOL IsEndOfFile(FILE *fp);
#endif

/************************************************************************/
/* Bitstream Feeder                                                     */
/************************************************************************/
typedef enum {
	FEEDING_METHOD_FIXED_SIZE,
	FEEDING_METHOD_FRAME_SIZE,
	FEEDING_METHOD_SIZE_PLUS_ES,
	FEEDING_METHOD_ES_IN,
	FEEDING_METHOD_MAX
} FeedingMethod;

typedef struct {
	void *data;
	Uint32 size;
	BOOL eos; //!<< End of stream
} BSChunk;

typedef void *BSFeeder;

typedef void (*BSFeederHook)(BSFeeder feeder, void *data, Uint32 size,
			     void *arg);

/**
 * \brief           BitstreamFeeder consumes bitstream and updates information
 * of bitstream buffer of VPU. \param handle    handle of decoder \param path
 * bitstream path \param method    feeding method. see FeedingMethod. \param
 * loopCount If @loopCount is greater than 1 then BistreamFeeder reads the start
 * of bitstream again when it encounters the end of stream @loopCount times.
 * \param ...       FEEDING_METHOD_FIXED_SIZE:
 *                      This value of parameter is size of chunk at a time.
 *                      If the size of chunk is equal to zero than the
 * BitstreamFeeder reads bistream in random size.(1Byte ~ 4MB) \return It
 * returns the pointer of handle containing the context of the BitstreamFeeder.
 */
void *BitstreamFeeder_Create(const char *path, FeedingMethod method,
			     PhysicalAddress base, Uint32 size, ...);

/**
 * \brief           This is helper function set to simplify the flow that update
 * bit-stream to the VPU.
 */
Uint32 BitstreamFeeder_Act(DecHandle decHandle, BSFeeder feeder,
			   EndianMode endian, vpu_buffer_t *buffer);

/**
 * \brief           Set filling bitstream as ringbuffer mode or linebuffer mode.
 * \param   mode    0 : auto
 *                  1 : ringbuffer
 *                  2 : linebuffer.
 */
#define BSF_FILLING_AUTO 0
#define BSF_FILLING_RINGBUFFER 1
#define BSF_FILLING_LINEBUFFER 2
/* BSF_FILLING_RINBGUFFER_WITH_ENDFLAG:
 * Scenario:
 * - Application writes 1 ~ 10 frames into bitstream buffer.
 * - Set stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, 0).
 * - Application clears stream end flag by using
 * VPU_DecUpdateBitstreamBuffer(handle, -1). when indexFrameDisplay is equal to
 * -1. NOTE:
 * - Last frame cannot be a complete frame.
 */
#define BSF_FILLING_RINGBUFFER_WITH_ENDFLAG 3
void BitstreamFeeder_SetFillMode(BSFeeder feeder, Uint32 mode);

#ifdef REDUNDENT_CODE
BOOL BitstreamFeeder_IsEos(BSFeeder feeder);
#endif

BOOL BitstreamFeeder_Destroy(BSFeeder feeder);

#ifdef REDUNDENT_CODE
BOOL BitstreamFeeder_Rewind(BSFeeder feeder);

BOOL BitstreamFeeder_SetHook(BSFeeder feeder, BSFeederHook hookFunc, void *arg);

BOOL BitstreamFeeder_SetAutoUpdate(BSFeeder feeder, BOOL onoff);
#endif

/************************************************************************/
/* YUV Feeder                                                           */
/************************************************************************/

#if 1
typedef enum {
	SOURCE_YUV = 0,
	SOURCE_YUV_JYUV_CONV = 1,
	SOURCE_YUV_WITH_LOADER = 2,
	SOURCE_YUV_FPGA = 3,
	SOURCE_YUV_ADDR = 4,
} SOURCE_YUV_MODE;
#else
#define SOURCE_YUV 0
#define SOURCE_YUV_WITH_LOADER 2
#define SOURCE_YUV_FPGA 3
#define SOURCE_YUV_ADDR 4
#endif

#define JPEG_CREATE 0x01 //!<< This command is followed by yuvInfo structure.
typedef struct {
	Uint32 cbcrInterleave;
	Uint32 nv21;
	Uint32 packedFormat;
	Uint32 srcFormat;
	Uint32 srcPlanar;
	Uint32 srcStride;
	Uint32 srcHeight;
} YuvInfo;

typedef struct YuvFeederImpl {
	void *context;
	BOOL(*Create)
	(struct YuvFeederImpl *impl, const char *path, Uint32 packed,
	 Uint32 fbStride, Uint32 fbHeight);
	BOOL(*Feed)
	(struct YuvFeederImpl *impl, Int32 coreIdx, FrameBuffer *fb,
	 size_t picWidth, size_t picHeight, void *arg);
	BOOL (*Destroy)(struct YuvFeederImpl *impl);
	BOOL (*Configure)(struct YuvFeederImpl *impl, Uint32 cmd, YuvInfo yuv);
} YuvFeederImpl;

typedef struct {
	YuvFeederImpl *impl;
	Uint8 pYuv;
} AbstractYuvFeeder;

typedef struct {
	void *addrY;
	void *addrCb;
	void *addrCr;
	PhysicalAddress phyAddrY;
	PhysicalAddress phyAddrCb;
	PhysicalAddress phyAddrCr;
} feederYuvaddr;

#ifdef PLATFORM_NON_OS
typedef AbstractYuvFeeder * YuvFeeder;
#else
typedef void *YuvFeeder;
#endif

typedef struct {
	osal_file_t *fp;
	Uint8 *pYuv;
	size_t fbStride;
	size_t cbcrInterleave;
	BOOL srcPlanar;
} yuvContext;

YuvFeeder YuvFeeder_Create(Uint32 type, const char *srcFilePath,
			   YuvInfo yuvInfo);

BOOL YuvFeeder_Feed(YuvFeeder feeder, Uint32 coreIdx, FrameBuffer *fb,
		    size_t picWidth, size_t picHeight, void *arg);

BOOL YuvFeeder_Destroy(YuvFeeder feeder);

/************************************************************************/
/* CNM video helper                                                    */
/************************************************************************/
/**
 *  \param  convertCbcrIntl     If this value is TRUE, it stores YUV as NV12 or
 * NV21 to @fb
 */
BOOL LoadYuvImageBurstFormat(Uint32 coreIdx, Uint8 *src, size_t picWidth,
			     size_t picHeight, FrameBuffer *fb,
			     BOOL convertCbcrIntl);
BOOL LoadYuvImageBurstFormat3(Uint32 coreIdx, Uint8 *srcY, Uint8 *srcCb,
			      Uint8 *srcCr, size_t picWidth, size_t picHeight,
			      FrameBuffer *fb, BOOL convertCbcrIntl);

typedef enum {
	EN_PARA_CHANGE_NONE = 0,
	EN_GOP_NUM = 1,
	EN_RC_INTRA_QP = 2,
	EN_RC_BIT_RATE = 4,
	EN_RC_FRAME_RATE = 8,
	EN_INTRA_REFRESH = 16,
	EN_RC_MIN_MAX_QP_CHANGE = 32
#ifdef RC_PIC_PARACHANGE
	,
	EN_PIC_PARA_CHANGE = 0x4000
#endif
} PARA_CHANGE_ENABLE;

int ProcessEncodedBitstreamBurst(Uint32 core_idx, osal_file_t fp,
				 int targetAddr, PhysicalAddress bsBufStartAddr,
				 PhysicalAddress bsBufEndAddr, int size,
				 int endian, Comparator comparator);

BOOL LoadTiledImageYuvBurst(Uint32 coreIdx, BYTE *pYuv, size_t picWidth,
			    size_t picHeight, FrameBuffer *fb,
			    TiledMapConfig mapCfg);

Uint32 StoreYuvImageBurstFormat(Uint32 coreIndex, FrameBuffer *fbSrc,
				TiledMapConfig mapCfg, Uint8 *pDst,
				VpuRect cropRect, BOOL enableCrop);

Uint32 seiEncode(CodStd format, Uint8 *pSrc, Uint32 srcLen, Uint8 *pBuffer,
		 Uint32 bufferSize);

BOOL H264SpsAddVui(cviH264Vui *pVui, void **ppBuffer, Int32 *pBufferSize);
BOOL H265SpsAddVui(cviH265Vui *pVui, void **ppBuffer, Int32 *pBufferSize);

/************************************************************************/
/* Bit Reader                                                           */
/************************************************************************/
#define BS_RESET_BUF 0
#define BS_RING_BUF 1
#define BUFFER_MODE_TYPE_LINEBUFFER 0
#define BUFFER_MODE_TYPE_RINGBUFFER 1
typedef void *BitstreamReader;
typedef struct BitstreamReaderImpl {
	void *context;
	vpu_buffer_t *streamVb;
	BOOL (*Create)(struct BitstreamReaderImpl *impl, const char *path);
	Uint32 (*Act)(struct BitstreamReaderImpl *impl, Int32 coreIdx,
		      PhysicalAddress bitstreamBuffer,
		      Uint32 bitstreamBufferSize, int endian,
		      Comparator comparator);
	BOOL (*Destroy)(struct BitstreamReaderImpl *impl);
	BOOL(*Configure)
	(struct BitstreamReaderImpl *impl, Uint32 cmd, void *val);
} BitstreamReaderImpl;

/*!
 * \param   type                0: Linebuffer, 1: Ringbuffer
 * \param   path                output filepath.
 * \param   endian              Endianness of bitstream buffer
 * \param   handle              Pointer of encoder handle
 */
BitstreamReader BitstreamReader_Create(Uint32 type, char *path,
				       EndianMode endian, EncHandle *handle);

BOOL BitstreamReader_SetVbStream(BitstreamReader reader, vpu_buffer_t *vb);

/*!
 * \param   bitstreamBuffer     base address of bitstream buffer
 * \param   bitstreamBufferSize size of bitstream buffer
 */
BOOL BitstreamReader_Act(BitstreamReader reader,
			 PhysicalAddress bitstreamBuffer,
			 Uint32 bitstreamBufferSize, Uint32 defaultsize,
			 Comparator comparator);

int cviCopyStreamToBuf(BitstreamReader reader, Int32 *pEsCopiedSize,
		       Uint8 **ppOutputEsBuf, Uint32 userEsReadSize,
		       int *pEsBufValidSize, PhysicalAddress paBsBufStart,
		       PhysicalAddress paBsBufEnd, BOOL *bSkipCopy, Int32 cviNalType);

BOOL BitstreamReader_Destroy(BitstreamReader reader);

/************************************************************************/
/* Binary Reader                                                           */
/************************************************************************/

/************************************************************************/
/* Simple Renderer                                                      */
/************************************************************************/
typedef void *Renderer;

typedef enum {
	RENDER_DEVICE_NULL,
	RENDER_DEVICE_FBDEV,
	RENDER_DEVICE_HDMI,
	RENDER_DEVICE_SDK,
	RENDER_DEVICE_MAX
} RenderDeviceType;

typedef struct RenderDevice {
	void *context;
	DecHandle decHandle;
	BOOL (*Open)(struct RenderDevice *device);
	void (*Render)(struct RenderDevice *device, DecOutputInfo *fbInfo,
		       Uint8 *yuv, Uint32 width, Uint32 height);
	BOOL (*Close)(struct RenderDevice *device);
} RenderDevice;

Renderer SimpleRenderer_Create(DecHandle decHandle, RenderDeviceType deviceType,
			       const char *yuvPath //!<< path to store yuv
			       //!<iamge.
);

Uint32 SimpleRenderer_Act(Renderer renderer, DecOutputInfo *fbInfo, Uint8 *pYuv,
			  Uint32 width, Uint32 height);

void *SimpleRenderer_GetFreeFrameInfo(Renderer renderer);

/* \brief       Flush display queues and clear display indexes
 */
void SimpleRenderer_Flush(Renderer renderer);

BOOL SimpleRenderer_Destroy(Renderer renderer);

#ifdef REDUNDENT_CODE
BOOL SimpleRenderer_SetFrameRate(Renderer renderer, Uint32 fps);
#endif

void *cviGetDispFrame(Renderer renderer, DecOutputInfo *poi);
void cviReleaseDispFrame(Renderer renderer, void *pDispInfo);
void cviReleaseDispFrameSDK(Renderer renderer, void *pDispInfo);
/************************************************************************/
/* Etc                                                                  */
/************************************************************************/
#ifdef REDUNDENT_CODE
typedef struct {
	DecOutputInfo *buffer;
	int size;
	int count;
	int front;
	int rear;
} frame_queue_item_t;

frame_queue_item_t *frame_queue_init(Int32 count);

void frame_queue_deinit(frame_queue_item_t *queue);

Int32 frame_queue_enqueue(frame_queue_item_t *queue, DecOutputInfo data);

Int32 frame_queue_dequeue(frame_queue_item_t *queue, DecOutputInfo *data);

Int32 frame_queue_dequeue_all(frame_queue_item_t *queue);

Int32 frame_queue_peekqueue(frame_queue_item_t *queue, DecOutputInfo *data);

Int32 frame_queue_count(frame_queue_item_t *queue);

Int32 frame_queue_check_in_queue(frame_queue_item_t *queue, Int32 index);
#endif

/*******************************************************************************
 * DATATYPES AND FUNCTIONS RELATED TO REPORT
 *******************************************************************************/
typedef struct {
	osal_file_t fpPicDispInfoLogfile;
	osal_file_t fpPicTypeLogfile;
	osal_file_t fpSeqDispInfoLogfile;
	osal_file_t fpUserDataLogfile;
	osal_file_t fpSeqUserDataLogfile;

	// encoder report file
	osal_file_t fpEncSliceBndInfo;
	osal_file_t fpEncQpInfo;
	osal_file_t fpEncmvInfo;
	osal_file_t fpEncsliceInfo;

	// Report Information
	BOOL reportOpened;
	Int32 decIndex;
	vpu_buffer_t vb_rpt;
	BOOL userDataEnable;
	BOOL userDataReportMode;

	Int32 profile;
	Int32 level;
} vpu_rpt_info_t;

typedef struct VpuReportConfig_t {
	PhysicalAddress userDataBufAddr;
	BOOL userDataEnable;
	Int32 userDataReportMode; // (0 : Int32errupt mode, 1 Int32errupt
		// disable mode)
	Int32 userDataBufSize;

} VpuReportConfig_t;

#ifdef REDUNDENT_CODE
void OpenDecReport(Uint32 core_idx, VpuReportConfig_t *cfg);

void CloseDecReport(Uint32 core_idx);

void ConfigDecReport(Uint32 core_idx, DecHandle handle);

void SaveDecReport(Uint32 core_idx, DecOutputInfo *pDecInfo,
		   CodStd bitstreamFormat);

void CheckUserDataInterrupt(Uint32 core_idx, Int32 decodeIdx,
			    CodStd bitstreamFormat, Int32 int_reason);
#endif

#define MAX_CFG (217)
#define MAX_ROI_LEVEL (8)
#define LOG2_CTB_SIZE (6)
#define CTB_SIZE (1 << LOG2_CTB_SIZE)
#define LAMBDA_SCALE_FACTOR (100000)
#define FLOATING_POINT_LAMBDA (1)
#define TEMP_SCALABLE_RC (1)
#define UI16_MAX (0xFFFF)
#ifndef INT_MAX
#define INT_MAX (2147483647)
#endif

typedef enum {
	INPUT_FILE = 0,
	SOURCE_WIDTH,
	SOURCE_HEIGHT,
	INPUT_BIT_DEPTH,
	FRAME_RATE, // 5
	FRAME_SKIP,
	FRAMES_TO_BE_ENCODED,
	INTRA_PERIOD,
	DECODING_REFRESH_TYPE,
	DERIVE_LAMBDA_WEIGHT, // 10
	GOP_SIZE,
	EN_INTRA_IN_INTER_SLICE,
	INTRA_NXN,
	EN_CU_8X8,
	EN_CU_16X16, // 15
	EN_CU_32X32,
	INTRA_TRANS_SKIP,
	CONSTRAINED_INTRA_PRED,
	INTRA_CTU_REFRESH_MODE,
	INTRA_CTU_REFRESH_ARG, // 20
	MAX_NUM_MERGE,
	EN_DYN_MERGE,
	EN_TEMPORAL_MVP,
	SCALING_LIST,
	INDE_SLICE_MODE, // 25
	INDE_SLICE_ARG,
	DE_SLICE_MODE,
	DE_SLICE_ARG,
	EN_DBK,
	EN_SAO, // 30
	LF_CROSS_SLICE_BOUNDARY_FLAG,
	BETA_OFFSET_DIV2,
	TC_OFFSET_DIV2,
	WAVE_FRONT_SYNCHRO,
	LOSSLESS_CODING, // 35
	USE_PRESENT_ENC_TOOLS,
	NUM_TEMPORAL_LAYERS,
	GOP_PRESET,
	RATE_CONTROL,
	ENC_BITRATE, // 40
	TRANS_BITRATE,
	INITIAL_DELAY,
	EN_HVS_QP,
	CU_LEVEL_RATE_CONTROL,
	CONF_WIND_SIZE_TOP, // 45
	CONF_WIND_SIZE_BOT,
	CONF_WIND_SIZE_RIGHT,
	CONF_WIND_SIZE_LEFT,
	HVS_QP_SCALE_DIV2,
	MIN_QP, // 50
	MAX_QP,
	MAX_DELTA_QP,
	NUM_ROI,
	INTRA_QP,
	ROI_DELTA_QP, // 55
	INTRA_QP_OFFSET,
	INIT_BUF_LEVELx8,
	BIT_ALLOC_MODE,
	FIXED_BIT_RATIO,
	INTERNAL_BITDEPTH, // 60
	EN_USER_DATA,
	USER_DATA_ENC_ORDER,
	USER_DATA_SIZE,
	USER_DATA_SUFFIX_FLAG,
	ROI_ENABLE, // 65
	VUI_PARAM_FLAG,
	VUI_ASPECT_RATIO_IDC,
	VUI_SAR_SIZE,
	VUI_OVERSCAN_APPROPRIATE,
	VIDEO_SIGNAL, // 70
	VUI_CHROMA_SAMPLE_LOC,
	VUI_DISP_WIN_LEFT_RIGHT,
	VUI_DISP_WIN_TOP_BOTTOM,
	NUM_UNITS_IN_TICK,
	TIME_SCALE, // 75
	NUM_TICKS_POC_DIFF_ONE,
	ENC_AUD,
	ENC_EOS,
	ENC_EOB,
	CB_QP_OFFSET, // 80
	CR_QP_OFFSET,
	RC_INIT_QP,
	EN_NR_Y,
	EN_NR_CB,
	EN_NR_CR, // 85
	EN_NOISE_EST,
	NOISE_SIGMA_Y,
	NOISE_SIGMA_CB,
	NOISE_SIGMA_CR,
	INTRA_NOISE_WEIGHT_Y, // 90
	INTRA_NOISE_WEIGHT_CB,
	INTRA_NOISE_WEIGHT_CR,
	INTER_NOISE_WEIGHT_Y,
	INTER_NOISE_WEIGHT_CB,
	INTER_NOISE_WEIGHT_CR, // 95
	INTRA_MIN_QP,
	INTRA_MAX_QP,
	MDFLAG0,
	MDFLAG1,
	MDFLAG2, // 100
	EN_SMART_BG,
	THRPIXELNUMCNT,
	THRMEAN0,
	THRMEAN1,
	THRMEAN2, // 105
	THRMEAN3,
	MDQPY,
	MDQPC,
	THRDCY0,
	THRDCC0, // 110
	THRDCY1,
	THRDCC1,
	THRDCY2,
	THRDCC2,
	THRACNUMY0, // 115
	THRACNUMC0,
	THRACNUMY1,
	THRACNUMC1,
	THRACNUMY2,
	THRACNUMC2, // 120
	USE_LONGTERM_PRRIOD,
	REF_LONGTERM_PERIOD,
	EN_CTU_MODE,
	EN_CTU_QP,
	CROP_X_POS, // 125
	CROP_Y_POS,
	CROP_X_SIZE,
	CROP_Y_SIZE,
	EN_VUI_DATA,
	VUI_DATA_SIZE, // 130
	EN_HRD_IN_VPS,
	EN_HRD_IN_VUI,
	HRD_DATA_SIZE,
	EN_PREFIX_SEI_DATA,
	PREFIX_SEI_DATA_SIZE, // 135
	PREFIX_SEI_TIMING_FLAG,
	EN_SUFFIX_SEI_DATA,
	SUFFIX_SEI_DATA_SIZE,
	SUFFIX_SEI_TIMING_FLAG,
	EN_REPORT_MV_COL, // 140
	EN_REPORT_DIST_MAP,
	EN_REPORT_BIT_INFO,
	EN_REPORT_FRAME_DIST,
	EN_REPORT_QP_HISTO,
	BITSTREAM_FILE, // 145
	EN_CUSTOM_VPS,
	EN_CUSTOM_SPS,
	EN_CUSTOM_PPS,
	CUSTOM_VPS_PSID,
	CUSTOM_SPS_PSID, // 150
	CUSTOM_SPS_ACTIVE_VPSID,
	CUSTOM_PPS_ACTIVE_SPSID,
	CUSTOM_VPS_INTFLAG,
	CUSTOM_VPS_AVAILFLAG,
	CUSTOM_VPS_MAXLAYER_MINUS1, // 155
	CUSTOM_VPS_MAXSUBLAYER_MINUS1,
	CUSTOM_VPS_TEMPID_NESTFLAG,
	CUSTOM_VPS_MAXLAYER_ID,
	CUSTOM_VPS_NUMLAYER_SETMINUS1,
	CUSTOM_VPS_EXTFLAG, // 160
	CUSTOM_VPS_EXTDATAFLAG,
	CUSTOM_VPS_SUBORDER_INFOFLAG,
	CUSTOM_SPS_SUBORDER_INFOFLAG,
	CUSTOM_VPS_LAYERID_0,
	CUSTOM_VPS_LAYERID_1, // 165
	CUSTOM_SPS_LOG2_MAXPOC_MINUS4,
	EN_FORCED_IDR_HEADER,
	// newly added for WAVE520
	EN_MONOCHROME,
	EN_STRONG_INTRA_SMOOTH,
	ROI_AVGQP, // 170
	EN_WEIGHTED_PRED,
	EN_BG_DETECT,
	BG_TH_DIFF,
	BG_TH_MEAN_DIFF,
	BG_LAMBDA_QP, // 175
	BG_DELTA_QP,
	TILE_NUM_COLUMNS,
	TILE_NUM_ROWS,
	TILE_UNIFORM_SPACE,
	EN_LAMBDA_MAP, // 180
	EN_CUSTOM_LAMBDA,
	EN_CUSTOM_MD,
	PU04_DELTA_RATE,
	PU08_DELTA_RATE,
	PU16_DELTA_RATE, // 185
	PU32_DELTA_RATE,
	PU04_INTRA_PLANAR_DELTA_RATE,
	PU04_INTRA_DC_DELTA_RATE,
	PU04_INTRA_ANGLE_DELTA_RATE,
	PU08_INTRA_PLANAR_DELTA_RATE, // 190
	PU08_INTRA_DC_DELTA_RATE,
	PU08_INTRA_ANGLE_DELTA_RATE,
	PU16_INTRA_PLANAR_DELTA_RATE,
	PU16_INTRA_DC_DELTA_RATE,
	PU16_INTRA_ANGLE_DELTA_RATE, // 195
	PU32_INTRA_PLANAR_DELTA_RATE,
	PU32_INTRA_DC_DELTA_RATE,
	PU32_INTRA_ANGLE_DELTA_RATE,
	CU08_INTRA_DELTA_RATE,
	CU08_INTER_DELTA_RATE, // 200
	CU08_MERGE_DELTA_RATE,
	CU16_INTRA_DELTA_RATE,
	CU16_INTER_DELTA_RATE,
	CU16_MERGE_DELTA_RATE,
	CU32_INTRA_DELTA_RATE, // 205
	CU32_INTER_DELTA_RATE,
	CU32_MERGE_DELTA_RATE,
	DISABLE_COEF_CLEAR,
	EN_CUSTOM_MODE_MAP,
	EN_TEMPORAL_LAYER_QP,
	TID_0_QP,
	TID_1_QP,
	TID_2_QP,
	TID_0_PERIOD,
#ifdef SUPPORT_HOST_RC_PARAM
	EN_HOST_PIC_RC,
#endif
} HevcCfgName;

typedef struct {
	char *name;
	int min;
	int max;
	int def;
} HevcCfgInfo;

#ifdef REDUNDENT_CODE
Int32 GetEncOpenParamChange(EncOpenParam *pEncOP, char *cfgFileName,
			    ENC_CFG * pEncCfg);
#endif

void PrintVpuVersionInfo(Uint32 coreIdx);

#ifdef REDUNDENT_CODE
void ChangePathStyle(char *str);
#endif

BOOL CalcYuvSize(Int32 format, Int32 picWidth, Int32 picHeight,
		 Int32 cbcrInterleave, size_t *lumaSize, size_t *chromaSize,
		 size_t *frameSize, Int32 *bitDepth, Int32 *packedFormat,
		 Int32 *yuv3p4b);

FrameBufferFormat GetPackedFormat(int srcBitDepth, PackedFormatNum packedType,
				  int p10bits, int msb);

#ifdef REDUNDENT_CODE
char *GetDirname(const char *path);

char *GetBasename(const char *pathname);

char *GetFileExtension(const char *filename);
#endif

void set_gop_info(ENC_CFG *pEncCfg);

int parseAvcCfgFile(ENC_CFG *pEncCfg, char *filename);

#ifdef REDUNDENT_CODE
int parseMp4CfgFile(ENC_CFG *pEncCfg, char *filename);
#endif

int parseHevcCfgFile(ENC_CFG *pEncCfg, char *FileName);

int parseRoiCtuModeParam(char *lineStr, VpuRect *roiRegion, int *roiLevel,
			 int picX, int picY);

#ifdef REDUNDENT_CODE
int ParseChangeParamCfgFile(ENC_CFG *pEncCfg, char *FileName);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

typedef struct ObserverStruct {
	void *ctx;
	void (*construct)(struct ObserverStruct *, void *);
	BOOL (*update)(struct ObserverStruct *ctx, void *data);
	void (*destruct)(struct ObserverStruct *);
} Listener;

#define MAX_OBSERVERS 100
typedef struct TestMachine_struct {
	Uint32 coreIdx;
	Uint32 testEnvOptions; /*!<< See debug.h */
	BOOL reset;
	Listener observers[MAX_OBSERVERS];
	Uint32 numObservers;
} TestMachine;

/************************************************************************/
/* Structure                                                            */
/************************************************************************/
typedef struct TestDecConfig_struct {
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
} TestDecConfig;

extern Listener decOutputInformation;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef REDUNDENT_CODE
BOOL TestMachineSetup(TestMachine *machine);

void TestMachineCleanUp(TestMachine *machine);

struct option *ConvertOptions(struct OptionExt *cnmOpt, Uint32 nItems);
#endif

void DisplayDecodedInformation(DecHandle handle, CodStd codec, Uint32 frameNo,
			       DecOutputInfo *decodedInfo);

void ReleaseVideoMemory(Uint32 coreIndex, vpu_buffer_t *memoryArr,
			Uint32 count);

#ifdef REDUNDENT_CODE
BOOL AllocateDecFrameBuffer(DecHandle decHandle, TestDecConfig *config,
			    Uint32 tiledFbCount, Uint32 linearFbCount,
			    FrameBuffer *retFbArray, vpu_buffer_t *retFbAddrs,
			    Uint32 *retStride);
#endif

#define OUTPUT_FP_NUMBER 4

#ifdef SUPPORT_SAVE_PIC_INFO_TO_XML
FILE *CreatePicInfoXmlFile(const char *yuvPath);

void ClosePicInfoXMLFile(FILE *fpXML);

void SavePicInfoToXml(FILE *fpXml, DecOutputInfo *fbInfo);
#endif /* SUPPORT_SAVE_PIC_INFO_TO_XML */

#ifdef __cplusplus
}
#endif /* __cplusplus */

typedef struct _cviEncRoiCfg_ {
	BOOL roi_enable;
	BOOL roi_qp_mode; // bAbsQp
	int roi_qp;
	int roi_rect_x; // pixel position
	int roi_rect_y; // pixel position
	unsigned int roi_rect_width; // pixel width
	unsigned int roi_rect_height; // pixel width
} cviEncRoiCfg;

typedef struct _cviEncCfg_ {
	unsigned int u32Profile;
	int rcMode;
	int s32IPQpDelta;
	int s32BgQpDelta;
	int s32ViQpDelta;
	int iqp;
	int pqp;
	int gop;
	int bitrate;
	int firstFrmstartQp;
	int framerate;
	unsigned int u32MaxIprop;
	unsigned int u32MinIprop;
	unsigned int u32MaxQp;
	unsigned int u32MinQp;
	unsigned int u32MaxIQp;
	unsigned int u32MinIQp;
	int maxbitrate;
	int initialDelay;
	// AVBR
	int s32MinStillPercent;
	int u32MaxStillQP;
	int u32MotionSensitivity;
	int s32AvbrFrmLostOpen;
	int s32AvbrFrmGap;
	int s32AvbrPureStillThr;
	int statTime;
	unsigned int bitstreamBufferSize;
	int singleLumaBuf;
	int bSingleEsBuf;
	BOOL roi_request;
	cviEncRoiCfg cviRoi[8];
	int virtualIPeriod;
	int frmLostBpsThr;
	int encFrmGaps;
	int s32ChangePos;
	cviVbVencBufConfig VbBufCfg;
	unsigned int u32IntraCost;
	unsigned int u32ThrdLv;
	unsigned int bBgEnhanceEn;
	int s32BgDeltaQp;
	cviH264Entropy h264Entropy;
	cviH264Trans h264Trans;
	cviH265Trans h265Trans;
	cviH264Split h264Split;
	cviH265Split h265Split;
	cviH264IntraPred h264IntraPred;
	union {
		cviH264Vui h264Vui;
		cviH265Vui h265Vui;
	};
	unsigned int u32RowQpDelta;
	Uint8 *pu8QpMap;
	BOOL bQpMapValid;
	BOOL bRoiBinValid;
	int roideltaqp;
	unsigned int enSuperFrmMode;
	unsigned int u32SuperIFrmBitsThr;
	unsigned int u32SuperPFrmBitsThr;
	int s32MaxReEncodeTimes;
	unsigned int originPicType;
	bool cviRcEn;
	bool addrRemapEn;
	union {
		cviH264Dblk h264Dblk;
		cviH265Dblk h265Dblk;
	};
	BOOL svcEnable;
	cviSvcParam svcParam;
} cviEncCfg;

typedef struct UserDataList_struct {
	struct list_head list;
	Uint8 *userDataBuf;
	Uint32 userDataLen;
} UserDataList;

typedef struct TestEncConfig_struct {
	char yuvSourceBaseDir[MAX_FILE_PATH];
	char yuvFileName[MAX_FILE_PATH];
	char cmdFileName[MAX_FILE_PATH];
	char bitstreamFileName[MAX_FILE_PATH];
	char huffFileName[MAX_FILE_PATH];
	char cInfoFileName[MAX_FILE_PATH];
	char qMatFileName[MAX_FILE_PATH];
	char qpFileName[MAX_FILE_PATH];
	char cfgFileName[MAX_FILE_PATH];
	CodStd stdMode;
	int picWidth;
	int picHeight;
	int kbps;
	int rcMode;
	int changePos;
	int frmLostOpen;
	int rotAngle;
	int mirDir;
	int useRot;
	int qpReport;
	int ringBufferEnable;
	int rcIntraQp;
	int outNum;
	int skipPicNums[MAX_PIC_SKIP_NUM];
	int instNum;
	int coreIdx;
	int mapType;
	int s32ChnNum;
	// 2D cache option

	int lineBufIntEn;
	int bEsBufQueueEn;
	int en_container; // enable container
	int container_frame_rate; // framerate for container
	int picQpY;

	int cbcrInterleave;
	int nv21;
	BOOL needSourceConvert; //!<< If the format of YUV file is YUV planar
		//!<mode and EncOpenParam::cbcrInterleave or
		//!<EncOpenParam::nv21 is true < the value of
		//!<needSourceConvert should be true.
	PackedFormatNum packedFormat;
	FrameBufferFormat srcFormat;
	int srcFormat3p4b;
	int decodingRefreshType;
	int gopSize;
	int tempLayer;
	int useDeriveLambdaWeight;
	int dynamicMergeEnable;
	int independSliceMode;
	int independSliceModeArg;
	int RcEnable;
	int bitdepth;
	int secondary_axi;
	int stream_endian;
	int frame_endian;
	int source_endian;

	int compare_type;
#define YUV_MODE_YUV 0
#define YUV_MODE_JYUV_CONV 1
#define YUV_MODE_YUV_LOADER 2
	int yuv_mode;
	char ref_stream_path[MAX_FILE_PATH];
	int loopCount;
	char ref_recon_md5_path[MAX_FILE_PATH];
#if defined(SUPPORT_W5ENC_BW_REPORT) || defined(CNM_FPGA_PLATFORM)
	BOOL performance;
#endif

	char roi_file_name[MAX_FILE_PATH];
	int roi_enable;
	int roi_delta_qp;

	char ctumode_file_name[MAX_FILE_PATH];
	int ctu_mode_enable;
	int ctuMode[MAX_ROI_NUMBER];

	char ctuqp_file_name[MAX_FILE_PATH];

	int ctu_qpMap_enable;
	int ctu_roiLevel_enable;
	int ctuQp[MAX_ROI_NUMBER];

	// char user_data_file_name[MAX_FILE_PATH];
	// FILE *user_data_fp;
	HevcSEIDataEnc seiDataEnc;

	char hrd_rbsp_file_name[MAX_FILE_PATH];
	char vui_rbsp_file_name[MAX_FILE_PATH];
	char prefix_sei_nal_file_name[MAX_FILE_PATH];
	char suffix_sei_nal_file_name[MAX_FILE_PATH];

	int encAUD;
	int encEOS;
	int encEOB;
	struct {
		BOOL enableMvc;
		BOOL enableSvc;
		BOOL enableLinear2Tiled;
		FrameFlag linear2TiledMode;
	} coda9;

	int actRegNum;
	VpuRect region[MAX_ROI_NUMBER]; /**< The size of the ROI region for
					   H.265 (start X/Y in CTU, end X/Y int
					   CTU)  */
	int roiLevel[MAX_ROI_NUMBER]; /**< An importance level for the given ROI
					 region for H.265. The higher an ROI
					 level is, the more important the region
					 is with a lower QP.  */

	int useAsLongtermPeriod;
	int refLongtermPeriod;
	Uint32 testEnvOptions; /*!<< See debug.h */

	// newly added for WAVE520
#ifdef SUPPORT_HOST_RC_PARAM


	char host_pic_rc_file_name[MAX_FILE_PATH];

	int host_pic_rc_enable;
#endif

	// CVITEK
	int cviApiMode;
	cviEncCfg cviEc;

	// FW usage
	Uint32 sizeInWord;
	Uint16 *pusBitCode;

	// user data
	Uint32 userDataBufSize;
	struct list_head userdataList;
#ifdef DUMP_MOTIONMAP
	struct file *filp;
	loff_t pos;
#endif
	int bIsoSendFrmEn;
} TestEncConfig;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Int32 GetEncOpenParam(EncOpenParam *pEncOP, TestEncConfig *pEncConfig,
		      ENC_CFG *pEncCfg, EncParam *encParam);

Int32 GetEncOpenParamDefault(EncOpenParam *pEncOP, TestEncConfig *pEncConfig);

void GenRegionToMap(VpuRect *region, int *roiLevel, int num, Uint32 mapWidth,
		   Uint32 mapHeight, Uint8 *roiCtuMap);
#ifdef REDUNDENT_CODE
#ifdef SUPPORT_980_ROI_RC_LIB
void GenRegionToMap980(VpuRect *region, /**< The size of the ROI region (start
					   X/Y in pixel, end X/Y in pixel)  */
		       int *roiLevel, int num, Uint32 mapWidth,
		       Uint32 mapHeight, Uint8 *roiCtuMap);
#endif
#endif

int cviSetCtuRoiLevelMap(int coreIdx, TestEncConfig *pEncConfig,
			EncOpenParam *pEncOP, PhysicalAddress addrRoiMap,
			Uint8 *roiMapBuf, EncParam *encParam,
			int maxCtuNum);

void setRoiMapFromMap(int coreIdx, TestEncConfig *pEncConfig,
		      EncOpenParam *pEncOP, PhysicalAddress addrRoiMap,
		      Uint8 *roiMapBuf, EncParam *encParam, int frmNum);

#define VUI_HRD_RBSP_BUF_SIZE 0x4000
#define SEI_NAL_DATA_BUF_SIZE 0x4000
Int32 writeVuiRbsp(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   vpu_buffer_t *vbVuiRbsp);
Int32 writeHrdRbsp(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   vpu_buffer_t *vbHrdRbsp);

#ifdef TEST_ENCODE_CUSTOM_HEADER
Int32 writeCustomHeader(int coreIdx, EncOpenParam *pEncOP,
			vpu_buffer_t *vbVuiRbsp, hrd_t *hrd);
Int32 allocateSeiNalDataBuf(int coreIdx, vpu_buffer_t *vbSeiNal, int srcFbNum);
Int32 writeSeiNalData(EncHandle handle, int streamEndian,
		      PhysicalAddress prefixSeiNalAddr, hrd_t *hrd);
#endif
void setEncBgMode(EncParam *encParam, TestEncConfig encConfig);

void GenRegionToQpMap(
	VpuRect *region, /**< The size of the ROI region for H.265 (start X/Y in
			    CTU, end X/Y int CTU)  */
	int *roiLevel, int num, int initQp, Uint32 mapWidth, Uint32 mapHeight,
	Uint8 *roiCtuMap);

int checkParamRestriction(Uint32 productId, TestEncConfig *pEncConfig);

void cviSetCodaRoiBySdk(EncParam *param, EncOpenParam *pEncOP, Int32 base_qp);
void GenRoiLevelMap(TestEncConfig *pEncConfig, EncParam *param,
		   EncOpenParam *pEncOP, Int32 base_qp, Uint8 *roiCtuMap);
void GenQpMap(TestEncConfig *pEncConfig, EncParam *param,
	     EncOpenParam *pEncOP,  Uint8 *QpMapBuf);

int allocateRoiMapBuf(int coreIdx, TestEncConfig *pEncConfig,
		      vpu_buffer_t *vbROi, int srcFbNum, int ctuNum);

#ifdef REDUNDENT_CODE
int openCtuModeMapFile(TestEncConfig *pEncConfig);
int allocateCtuModeMapBuf(int coreIdx, TestEncConfig encConfig,
			  vpu_buffer_t *vbCtuMode, int srcFbNum, int ctuNum);
int setCtuModeMap(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		  PhysicalAddress addrCtuModeMap, Uint8 *ctuModeMapBuf,
		  int srcFrameWidth, int srcFrameHeight, EncParam *encParam,
		  int maxCtuNum);
#endif
int allocateCtuQpMapBuf(int coreIdx, TestEncConfig *pEncConfig,
			vpu_buffer_t *vbCtuQp, int srcfbNum, int ctuNum);

int cviSetCtuQpMap(int coreIdx, TestEncConfig *pEncConfig, EncOpenParam *pEncOP,
		   PhysicalAddress addrCtuQpMap, Uint8 *ctuQpMapBuf,
		   EncParam *encParam, int maxCtuNum);

int cviSmartRoiLevelCfg(int coreIdx, TestEncConfig *pEncConfig,
			EncParam *pEncParam,  EncOpenParam *pEncOP,
			PhysicalAddress addrRoiMap, Uint8 *roiMapBuf,
			int frmNum);

#ifdef SUPPORT_HOST_RC_PARAM
#ifdef REDUNDENT_CODE
int openHostPicRcFile(TestEncConfig *pEncConfig);
int setHostPicRc(TestEncConfig *pEncConfig, EncParam *encParam);
#endif
#endif

/************************************************************************/
/* User Parameters (WAVE520)                                            */
/************************************************************************/
// user scaling list
#define SL_NUM_MATRIX (6)

typedef struct {
	Uint8 s4[SL_NUM_MATRIX][16]; // [INTRA_Y/U/V,INTER_Y/U/V][NUM_COEFF]
	Uint8 s8[SL_NUM_MATRIX][64];
	Uint8 s16[SL_NUM_MATRIX][64];
	Uint8 s32[SL_NUM_MATRIX][64];
} UserScalingList;

enum ScalingListSize {
	SCALING_LIST_4x4 = 0,
	SCALING_LIST_8x8,
	SCALING_LIST_16x16,
	SCALING_LIST_32x32,
	SCALING_LIST_SIZE_NUM
};

void dpb_pic_init(EncInfo *pAvcInfo);

#ifdef REDUNDENT_CODE
int parse_user_scaling_list(UserScalingList *sl, FILE *fp_sl);
// custom lambda
#define NUM_CUSTOM_LAMBDA (2 * 52)
int parse_custom_lambda(Uint32 buf[NUM_CUSTOM_LAMBDA], FILE *fp);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MAIN_HELPER_H_ */
