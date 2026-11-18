
#ifndef JPUAPI_UTIL_H_INCLUDED
#define JPUAPI_UTIL_H_INCLUDED

#include "jpuapi.h"
#ifndef LIBCVIJPULITE
#include "mixer.h"
#endif
#include "regdefine.h"

#define DC_TABLE_INDEX0 0
#define AC_TABLE_INDEX0 1
#define DC_TABLE_INDEX1 2
#define AC_TABLE_INDEX1 3

#define Q_COMPONENT0 0
#define Q_COMPONENT1 0x40
#define Q_COMPONENT2 0x80

#define USER_DATA_BUF_SIZE 1024
#define USER_DATA_MARKER 0xFFEF // Application segment 15
#define USER_DATA_MARKER_CODE_SIZE 2 // 2 bytes for segment marker, 0xffef
#define USER_DATA_LENGTH_CODE_SIZE 2 // 2 bytes encoding segment length

#define JPG_MARKER_ORDER_CNT 16
#define JPG_MARKER_ORDER_BUF_SIZE (sizeof(int) * JPG_MARKER_ORDER_CNT)

#ifdef MJPEG_INTERFACE_API
#define NUM_FRAME_BUF MAX_FRAME
#endif /* MJPEG_INTERFACE_API */

//#define DUMP_JPU_REGS
typedef enum {
	JPG_START_PIC = 0,
	JPG_START_INIT,
	JPG_START_STOP,
	JPG_START_PARTIAL
} JpgStartCmd;

typedef struct {
	UINT tag;
	UINT type;
	int count;
	int offset;
} TAG;

enum { JFIF = 0, JFXX_JPG = 1, JFXX_PAL = 2, JFXX_RAW = 3, EXIF_JPG = 4 };

typedef enum {
	JPG_SOI = 1,
	JPG_DQT,
	JPG_DQT_MERGE,
	JPG_DHT,
	JPG_DHT_MERGE,
	JPG_DRI,
	JPG_DRI_OPT,
	JPG_SOF0,
	JPG_JFIF,
	JPG_FRAME_INDEX,
	JPG_ADOBE,
	JPG_USER_DATA,
} JpgMarker;

typedef struct {
	int PicX;
	int PicY;
	int BitPerSample[3];
	int Compression; // 1 for uncompressed / 6 for compressed(jpeg)
	int PixelComposition; // 2 for RGB / 6 for YCbCr
	int SamplePerPixel;
	int PlanrConfig; // 1 for chunky / 2 for planar
	int YCbCrSubSample; // 00020002 for YCbCr 4:2:0 / 00020001 for YCbCr
		// 4:2:2
	UINT JpegOffset;
	UINT JpegThumbSize;
} EXIF_INFO;

typedef struct {
	BYTE *buffer;
	int index;
	int size;
} jpu_getbit_context_t;

#define init_get_bits(CTX, BUFFER, SIZE) JpuGbuInit(CTX, BUFFER, SIZE)
#define show_bits(CTX, NUM) JpuGguShowBit(CTX, NUM)
#define get_bits(CTX, NUM) JpuGbuGetBit(CTX, NUM)
#define get_bits_left(CTX) JpuGbuGetLeftBitCount(CTX)
#define get_bits_count(CTX) JpuGbuGetUsedBitCount(CTX)

typedef struct {
	PhysicalAddress streamWrPtr;
	PhysicalAddress streamRdPtr;
	int streamEndflag;
	PhysicalAddress streamBufStartAddr;
	PhysicalAddress streamBufEndAddr;
	int streamBufSize;
	BYTE *pBitStream;

	int frameOffset;
	int consumeByte;
	int nextOffset;
	int currOffset;

	FrameBuffer *frameBufPool;
	int numFrameBuffers;
	int stride;
	int rotationEnable;
	int mirrorEnable;
	int mirrorDirection;
	int rotationAngle;
	FrameBuffer rotatorOutput;
	int rotatorStride;
	int rotatorOutputValid;
	int initialInfoObtained;
	int minFrameBufferNum;
	int streamEndian;
	int frameEndian;
	int chromaInterleave;

	int picWidth;
	int picHeight;
	int alignedWidth;
	int alignedHeight;
	int headerSize;
	int ecsPtr;
	int pagePtr;
	int wordPtr;
	int bitPtr;
	int format;
	int rstIntval;

	int userHuffTab;

	int huffDcIdx;
	int huffAcIdx;
	int Qidx;

	BYTE huffVal[4][162];
	BYTE huffBits[4][256];
	BYTE cInfoTab[4][6];
	BYTE qMatTab[4][64];

	Uint32 huffMin[4][16];
	Uint32 huffMax[4][16];
	BYTE huffPtr[4][16];

	// partial
	int usePartial;
	int lineNum;
	int bufNum;

	int busReqNum;
	int compNum;
	int mcuBlockNum;
	int compInfo[3];
	int frameIdx;
	int bitEmpty;
	int iHorScaleMode;
	int iVerScaleMode;
	int mcuWidth;
	int mcuHeight;
	jpu_getbit_context_t gbc;

#ifdef MJPEG_ERROR_CONCEAL
	// error conceal
	struct {
		int bError;
		int rstMarker;
		int errPosX;
		int errPosY;
	} errInfo;

	int curRstIdx;
	int nextRstIdx;
	int setPosX;
	int setPosY;
	int gbuStartPtr; // entry point in stream buffer before pic_run

	int numRstMakerRounding;
#endif

	// ROI
	int roiEnable;
	int roiOffsetX;
	int roiOffsetY;
	int roiWidth;
	int roiHeight;
	int roiMcuOffsetX;
	int roiMcuOffsetY;
	int roiMcuWidth;
	int roiMcuHeight;
	int packedFormat;
	int dst_type;
	FrameBuffer dst_info;

#ifdef MJPEG_INTERFACE_API
	FrameBuffer frameBuf[MAX_FRAME_JPU];
	FRAME_BUF *pFrame[MAX_FRAME_JPU];
	// Uint32 framebufStride;
#endif /* MJPEG_INTERFACE_API */

} JpgDecInfo;

typedef struct {
	JpgEncOpenParam openParam;
	JpgEncInitialInfo initialInfo;
	PhysicalAddress streamRdPtr;
	PhysicalAddress streamWrPtr;
	PhysicalAddress streamBufStartAddr;
	PhysicalAddress streamBufEndAddr;
	int streamBufSize;
	BYTE *pBitStream;

	FrameBuffer *frameBufPool;
	int numFrameBuffers;
	int stride;
	int rotationEnable;
	int mirrorEnable;
	int mirrorDirection;
	int rotationAngle;
	int initialInfoObtained;

	int picWidth;
	int picHeight;
	int alignedWidth;
	int alignedHeight;
	int seqInited;
	int frameIdx;
	FrameFormat sourceFormat;

	int streamEndian;
	int frameEndian;
	CbCrInterLeave chromaInterleave;

	int rstIntval;
	int busReqNum;
	int mcuBlockNum;
	int compNum;
	int compInfo[3];

	// give command
	int disableAPPMarker;
	int quantMode;
	int stuffByteEnable;

	Uint32 huffCode[4][256];
	Uint32 huffSize[4][256];
	BYTE *pHuffVal[4];
	BYTE *pHuffBits[4];
	BYTE *pCInfoTab[4];
	BYTE *pQMatTab[4];
	// partial
	int usePartial;
	int partiallineNum;
	int partialBufNum;
	PackedOutputFormat packedFormat;

	JpgEncParamSet *paraSet;

#ifdef MJPEG_INTERFACE_API
	FrameBuffer frameBuf[MAX_FRAME_JPU];
	FRAME_BUF *pFrame[MAX_FRAME_JPU];
	Uint32 framebufStride;
	int encHeaderMode;
	int rgbPacked;
#endif /* MJPEG_INTERFACE_API */

	Uint32 userDataLen;
	BYTE userData[USER_DATA_BUF_SIZE];

	JpgMarker jpgMarkerOrder[JPG_MARKER_ORDER_CNT];

	BYTE *pPreStream;
	Uint32 preStreamLen;
	BYTE *pFinalStream;

	bool bIsoSendFrmEn;

	struct task_struct *tPthreadId;
	struct completion semSendEncCmd;
	struct completion semGetStreamCmd;
	jpu_buffer_t tEncBitstreamData;

	bool bSbmEn;
} JpgEncInfo;

typedef struct JpgInst {
	int inUse;
	int instIndex;
	int loggingEnable;
#ifdef MJPEG_INTERFACE_API
	/* 0 -- CVIJPGCOD_UNKNOWN */
	/* 1 -- CVIJPGCOD_DEC     */
	/* 2 -- CVIJPGCOD_ENC     */
	int type;
#endif /* MJPEG_INTERFACE_API */
	union {
		JpgEncInfo encInfo;
		JpgDecInfo decInfo;
	} JpgInfo;
	Uint64 u64StartTime;
	Uint64 u64EndTime;
	int s32ChnNum;
} JpgInst;

extern unsigned char sJpuCompInfoTable[5][24];

#ifdef __cplusplus
extern "C" {
#endif

JpgRet InitJpgInstancePool(void);
JpgRet GetJpgInstance(JpgInst **ppInst);
void FreeJpgInstance(JpgInst *pJpgInst);
JpgRet CheckJpgInstValidity(JpgInst *pci);
JpgRet CheckJpgDecOpenParam(JpgDecOpenParam *pop);

int JpuGbuInit(jpu_getbit_context_t *ctx, BYTE *buffer, int size);
int JpuGbuGetUsedBitCount(jpu_getbit_context_t *ctx);
int JpuGbuGetLeftBitCount(jpu_getbit_context_t *ctx);
unsigned int JpuGbuGetBit(jpu_getbit_context_t *ctx, int bit_num);
unsigned int JpuGguShowBit(jpu_getbit_context_t *ctx, int bit_num);

int JpegDecodeHeader(JpgDecInfo *jpg);
int JpgDecQMatTabSetUp(JpgDecInfo *jpg);
int JpgDecHuffTabSetUp(JpgDecInfo *jpg);
void JpgDecGramSetup(JpgDecInfo *jpg);

JpgRet CheckJpgEncOpenParam(JpgEncOpenParam *pop);
JpgRet CheckJpgEncParam(JpgEncHandle handle, JpgEncParam *param);
int JpgEncLoadHuffTab(JpgEncInfo *pJpgEncInfo);
int JpgEncLoadQMatTab(JpgEncInfo *pJpgEncInfo);
int JpgEncEncodeHeader(JpgEncHandle handle, JpgEncParamSet *para);

JpgRet JpgEnterLock(void);
JpgRet JpgEnterTryLock(void);
JpgRet JpgEnterTimeLock(int timout_ms);
JpgRet JpgLeaveLock(void);
JpgRet JpgSetClockGate(Uint32 on);

void SetJpgPendingInst(JpgInst *inst);
void ClearJpgPendingInst(void);
JpgInst *GetJpgPendingInst(void);

//void *JpuProcessEnterLock(void);
//void JpuProcessLeaveLock(void *lock);
#ifdef MJPEG_ERROR_CONCEAL
int JpegDecodeConcealError(JpgDecInfo *jpg);
#endif

#ifdef __cplusplus
}
#endif

#endif /* JPUAPI_UTIL_H_INCLUDED */
