
#ifndef JPUAPI_H_INCLUDED
#define JPUAPI_H_INCLUDED

#include "jpuconfig.h"
#include "../jdi/jdi.h"
#include "rcKernel/cvi_rc_kernel.h"

#ifndef MAX
#define MAX(A, B) ((A >= B) ? A : B)
#endif

#ifndef MIN
#define MIN(A, B) ((A <= B) ? A : B)
#endif

#ifndef CLIP
#define CLIP(L, H, X) (MAX(L, MIN(X, H)))
#endif

#define MAX_FPS 60
#define MAX_ABS_BIT_ERR (1 << 30)

typedef struct {
	int winSize;
	int total;
	int stats[MAX_FPS];
	int ptrIdx;
} stSlideWinStats;

typedef struct _stRcCfg_ {
	int targetBitrate; // in kByte
	//int minPicBit; // in Byte
	int fps;
	int width;
	int height;
	int minQ;
	int maxQ;
	int qClipRange;
	int maxBitrateLimit; // in kByte
	bool cviRcEn;
} stRcCfg;

typedef struct _stRcInfo_ {
	int targetBitrate; // in BYTE
	//int minPicBit; // in Byte
	int fps;
	int picPelNum;
	int picAvgBit;
	int picTargetBit;
	int bitErr;
	int errCompSize;
	int minQ;
	int maxQ;
	int minQs;
	int maxQs;
	int qClipRange;
	int lastPicQ;
	unsigned int picIdx;
	stSlideWinStats stBitRateStats;
	int maxBitrateLimit; // in kByte

	stRcKernelInfo rcKerInfo;
	stRcKernelPicOut rcPicOut;
	bool cviRcEn;

} stRcInfo;

#ifdef LIBCVIJPULITE
#include "jpu_lib.h"

//------------------------------------------------------------------------------
// common struct and definition
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// decode struct and definition
//------------------------------------------------------------------------------
typedef void *JpgDecHandle;

typedef DecOpenParam JpgDecOpenParam;
typedef DecInitialInfo JpgDecInitialInfo;
typedef DecParam JpgDecParam;
typedef DecOutputInfo JpgDecOutputInfo;

//------------------------------------------------------------------------------
// encode struct and definition
//------------------------------------------------------------------------------
typedef void *JpgEncHandle;

typedef EncOpenParam JpgEncOpenParam;
typedef EncInitialInfo JpgEncInitialInfo;
typedef EncParam JpgEncParam;
typedef EncOutputInfo JpgEncOutputInfo;
typedef EncParamSet JpgEncParamSet;
#else

//------------------------------------------------------------------------------
// common struct and definition
//------------------------------------------------------------------------------

typedef enum {
	ENABLE_JPG_ROTATION,
	DISABLE_JPG_ROTATION,
	ENABLE_JPG_MIRRORING,
	DISABLE_JPG_MIRRORING,
	SET_JPG_MIRROR_DIRECTION,
	SET_JPG_ROTATION_ANGLE,
	SET_JPG_ROTATOR_OUTPUT,
	SET_JPG_ROTATOR_STRIDE,
	SET_JPG_SCALE_HOR,
	SET_JPG_SCALE_VER,
	SET_JPG_USE_PARTIAL_MODE,
	SET_JPG_PARTIAL_FRAME_NUM,
	SET_JPG_PARTIAL_LINE_NUM,
	SET_JPG_ENCODE_NEXT_LINE,
	SET_JPG_USE_STUFFING_BYTE_FF,
	ENC_JPG_GET_HEADER,
	ENABLE_LOGGING,
	DISABLE_LOGGING,
	JPG_CMD_END
} JpgCommand;

typedef enum {
	JPG_RET_SUCCESS,
	JPG_RET_FAILURE,
	JPG_RET_BIT_EMPTY,
	JPG_RET_EOS,
	JPG_RET_INVALID_HANDLE,
	JPG_RET_INVALID_PARAM,
	JPG_RET_INVALID_COMMAND,
	JPG_RET_ROTATOR_OUTPUT_NOT_SET,
	JPG_RET_ROTATOR_STRIDE_NOT_SET,
	JPG_RET_FRAME_NOT_COMPLETE,
	JPG_RET_INVALID_FRAME_BUFFER,
	JPG_RET_INSUFFICIENT_FRAME_BUFFERS,
	JPG_RET_INVALID_STRIDE,
	JPG_RET_WRONG_CALL_SEQUENCE,
	JPG_RET_CALLED_BEFORE,
	JPG_RET_NOT_INITIALIZED,
	JPG_RET_HWRESET_SUCCESS, // interruption timeout happened and need reset
	JPG_RET_HWRESET_FAILURE,
	JPG_RET_ENC_TIMEOUT,
	JPG_RET_BREAK,
	JPG_RET_GOTO
} JpgRet;

typedef enum {
	MIRDIR_NONE,
	MIRDIR_VER,
	MIRDIR_HOR,
	MIRDIR_HOR_VER
} JpgMirrorDirection;

typedef enum {
	FORMAT_420 = 0,
	FORMAT_422 = 1,
	FORMAT_224 = 2,
	FORMAT_444 = 3,
	FORMAT_400 = 4
} FrameFormat;

typedef enum { CBCR_ORDER_NORMAL, CBCR_ORDER_REVERSED } CbCrOrder;

typedef enum {
	CBCR_SEPARATED = 0,
	CBCR_INTERLEAVE,
	CRCB_INTERLEAVE
} CbCrInterLeave;

typedef enum {
	PACKED_FORMAT_NONE,
	PACKED_FORMAT_422_YUYV,
	PACKED_FORMAT_422_UYVY,
	PACKED_FORMAT_422_YVYU,
	PACKED_FORMAT_422_VYUY,
	PACKED_FORMAT_444,
	PACKED_FORMAT_444_RGB
} PackedOutputFormat;

typedef enum {
	INT_JPU_DONE = 0,
	INT_JPU_ERROR = 1,
	INT_JPU_BIT_BUF_EMPTY = 2,
	INT_JPU_BIT_BUF_FULL = 2,
	INT_JPU_PARIAL_OVERFLOW = 3,
	INT_JPU_PARIAL_BUF0_EMPTY = 4,
	INT_JPU_PARIAL_BUF1_EMPTY,
	INT_JPU_PARIAL_BUF2_EMPTY,
	INT_JPU_PARIAL_BUF3_EMPTY,
	INT_JPU_BIT_BUF_STOP
} InterruptJpu;

typedef enum { JPG_TBL_NORMAL, JPG_TBL_MERGE } JpgTableMode;

typedef enum {
	ENC_HEADER_MODE_NORMAL,
	ENC_HEADER_MODE_SOS_ONLY
} JpgEncHeaderMode;

typedef struct {
	PhysicalAddress bufY;
	PhysicalAddress bufCb;
	PhysicalAddress bufCr;
	int stride;
} FrameBuffer;

struct JpgInst;

//------------------------------------------------------------------------------
// decode struct and definition
//------------------------------------------------------------------------------

typedef struct JpgInst JpgDecInst;
typedef JpgDecInst * JpgDecHandle;

typedef struct {
	PhysicalAddress bitstreamBuffer;
	int bitstreamBufferSize;
	BYTE *pBitStream;
	int streamEndian;
	int frameEndian;
	CbCrInterLeave chromaInterleave;
	int thumbNailEn;
	PackedOutputFormat packedFormat;
	int roiEnable;
	int roiOffsetX;
	int roiOffsetY;
	int roiWidth;
	int roiHeight;
	int dst_type;
	FrameBuffer dst_info;
} JpgDecOpenParam;

typedef struct {
	int picWidth;
	int picHeight;
	int minFrameBufferCount;
	int sourceFormat;
	int ecsPtr;
	int roiFrameWidth;
	int roiFrameHeight;
	int roiFrameOffsetX;
	int roiFrameOffsetY;
	int roiMCUSize;
	int colorComponents;
} JpgDecInitialInfo;

typedef struct {
	int scaleDownRatioWidth;
	int scaleDownRatioHeight;
} JpgDecParam;

typedef struct {
	int indexFrameDisplay;
	int numOfErrMBs;
	int decodingSuccess;
	int decPicHeight;
	int decPicWidth;
	int consumedByte;
	int bytePosFrameStart;
	int ecsPtr;
} JpgDecOutputInfo;

//------------------------------------------------------------------------------
// encode struct and definition
//------------------------------------------------------------------------------

typedef struct JpgInst JpgEncInst;
typedef JpgEncInst * JpgEncHandle;

typedef struct {
	PhysicalAddress bitstreamBuffer;
	Uint32 bitstreamBufferSize;
	int picWidth;
	int picHeight;
	FrameFormat sourceFormat;
	int restartInterval;
	int streamEndian;
	int frameEndian;
	CbCrInterLeave chromaInterleave;
	BYTE huffVal[4][162];
	BYTE huffBits[4][256];
	BYTE qMatTab[4][64];
	PackedOutputFormat packedFormat;
	int rgbPacked;
	//rate control
	int quality;
	int bitrate;
	int framerate;
	stRcInfo RcInfo;
	stRcCfg RcCfg;
} JpgEncOpenParam;

typedef struct {
	int minFrameBufferCount;
	int colorComponents;
} JpgEncInitialInfo;

typedef struct {
	FrameBuffer *sourceFrame;
} JpgEncParam;

typedef struct {
	PhysicalAddress bitstreamBuffer;
#ifdef MJPEG_INTERFACE_API
	__u8 *bitstreamBuf_virt;
#endif /* MJPEG_INTERFACE_API */
	Uint32 bitstreamSize;
} JpgEncOutputInfo;

typedef struct {
	PhysicalAddress paraSet;
	BYTE *pParaSet;
	int size;
	int headerMode;
	int quantMode;
	int huffMode;
	int disableAPPMarker;
	int rgbPackd;
} JpgEncParamSet;

#endif

#ifdef __cplusplus
extern "C" {
#endif
int JPU_IsBusy(void);
Uint32 JPU_GetStatus(void);
void JPU_ClrStatus(Uint32 val);
Uint32 JPU_IsInit(void);
Uint32 JPU_WaitInterrupt(int timeout);

JpgRet JPU_Init(void);
void JPU_DeInit(void);
int JPU_GetOpenInstanceNum(void);
JpgRet JPU_GetVersionInfo(Uint32 *versionInfo);

// function for decode
JpgRet JPU_DecOpen(JpgDecHandle *pHandle, JpgDecOpenParam *pop);
JpgRet JPU_DecClose(JpgDecHandle handle);
JpgRet JPU_DecGetInitialInfo(JpgDecHandle handle, JpgDecInitialInfo *info);

JpgRet JPU_DecSetRdPtr(JpgDecHandle handle, PhysicalAddress addr,
		       int updateWrPtr);

JpgRet JPU_DecRegisterFrameBuffer(JpgDecHandle handle, FrameBuffer *bufArray,
				  int num, int stride);
JpgRet JPU_DecGetBitstreamBuffer(JpgDecHandle handle, PhysicalAddress *prdPrt,
				 PhysicalAddress *pwrPtr, int *size);
JpgRet JPU_DecUpdateBitstreamBuffer(JpgDecHandle handle, int size);
JpgRet JPU_HWReset(void);
JpgRet JPU_SWReset(void);
JpgRet JPU_DecStartOneFrame(JpgDecHandle handle, JpgDecParam *param);
JpgRet JPU_DecGetOutputInfo(JpgDecHandle handle, JpgDecOutputInfo *info);
JpgRet JPU_DecIssueStop(JpgDecHandle handle);
JpgRet JPU_DecCompleteStop(JpgDecHandle handle);
JpgRet JPU_DecGiveCommand(JpgDecHandle handle, JpgCommand cmd, void *parameter);

// function for encode
JpgRet JPU_EncOpen(JpgEncHandle *pHandle, JpgEncOpenParam *pop);
JpgRet JPU_EncClose(JpgEncHandle handle);
JpgRet JPU_EncGetInitialInfo(JpgEncHandle handle, JpgEncInitialInfo *info);
JpgRet JPU_EncGetBitstreamBuffer(JpgEncHandle handle, PhysicalAddress *prdPrt,
				 PhysicalAddress *pwrPtr, int *size);
JpgRet JPU_EncUpdateBitstreamBuffer(JpgEncHandle handle, int size);
JpgRet JPU_EncStartOneFrame(JpgEncHandle handle, JpgEncParam *param);
JpgRet JPU_EncGetOutputInfo(JpgEncHandle handle, JpgEncOutputInfo *info);
JpgRet JPU_EncIssueStop(JpgDecHandle handle);
JpgRet JPU_EncCompleteStop(JpgDecHandle handle);
JpgRet JPU_EncGiveCommand(JpgEncHandle handle, JpgCommand cmd, void *parameter);
void JPU_EncSetHostParaAddr(PhysicalAddress baseAddr, PhysicalAddress paraAddr);

#ifdef __cplusplus
}
#endif

#endif
