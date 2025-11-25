/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_jpg_interface.h
 * Description:
 *   Jpeg Codec interface
 */
#ifndef CVI_JPG_INTERFACE_H
#define CVI_JPG_INTERFACE_H
#include <linux/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CVI_JPG_MARKER_ORDER_CNT 16
#define CVI_JPG_MARKER_ORDER_BUF_SIZE (sizeof(int) * CVI_JPG_MARKER_ORDER_CNT)

/* enum define */
typedef enum {
	CVIJPGCOD_UNKNOWN = 0,
	CVIJPGCOD_DEC = 1,
	CVIJPGCOD_ENC = 2
} CVIJpgCodType;

typedef enum {
	CVI_FORMAT_420 = 0,
	CVI_FORMAT_422 = 1,
	CVI_FORMAT_224 = 2,
	CVI_FORMAT_444 = 3,
	CVI_FORMAT_400 = 4
} CVIFrameFormat;

typedef enum {
	CVI_CBCR_SEPARATED = 0,
	CVI_CBCR_INTERLEAVE,
	CVI_CRCB_INTERLEAVE
} CVICbCrInterLeave;

typedef enum {
	CVI_PACKED_FORMAT_NONE,
	CVI_PACKED_FORMAT_422_YUYV,
	CVI_PACKED_FORMAT_422_UYVY,
	CVI_PACKED_FORMAT_422_YVYU,
	CVI_PACKED_FORMAT_422_VYUY,
	CVI_PACKED_FORMAT_444,
	CVI_PACKED_FORMAT_444_RGB
} CVIPackedFormat;

typedef enum {
	JPEG_MEM_MODULE = 2,
	JPEG_MEM_EXTERNAL = 3,
} JPEG_MEM_TYPE;

#define CVI_JPEG_OP_BASE 0x10000
#define CVI_JPEG_OP_MASK 0xFF0000
#define CVI_JPEG_OP_SHIFT 16

typedef enum _CVI_JPEG_OP_ {
	CVI_JPEG_OP_NONE = 0,
	CVI_JPEG_OP_SET_QUALITY = (1 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_SET_CHN_ATTR = (2 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_SET_MCUPerECS = (3 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_RESET_CHN = (4 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_SET_USER_DATA = (5 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_SHOW_CHN_INFO = (6 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_START = (7 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_SET_SBM_ENABLE = (8 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_WAIT_FRAME_DONE = (9 << CVI_JPEG_OP_SHIFT),
	CVI_JPEG_OP_MAX = (10 << CVI_JPEG_OP_SHIFT),
} CVI_JPEG_OP;

/* struct define */

/* Frame Buffer */

typedef struct {
	unsigned int size;
	__u64 phys_addr;
	__u64 base;
	__u8 *virt_addr;
} CVIBUF;

typedef struct {
	CVIFrameFormat format;
	CVIPackedFormat packedFormat;
	CVICbCrInterLeave chromaInterleave;
	CVIBUF vbY;
	CVIBUF vbCb;
	CVIBUF vbCr;
	int width;
	int height;
	int strideY;
	int strideC;
} CVIFRAMEBUF;

/* encode config param */
typedef struct {
	int picWidth;
	int picHeight;
	int rotAngle;
	int mirDir;
	CVIFrameFormat sourceFormat;
	int outNum;
	CVICbCrInterLeave chromaInterleave;
	int bEnStuffByte;
	int encHeaderMode;
	int RandRotMode;
	CVIPackedFormat packedFormat;
	int quality;
	int bitrate;
	int framerate;
	int src_type;
	int bitstreamBufSize;
	int singleEsBuffer;
	int jpgMarkerOrder[CVI_JPG_MARKER_ORDER_CNT];
} CVIEncConfigParam;

/* decode config param */
typedef struct {
	/* ROI param */
	int roiEnable;
	int roiWidth;
	int roiHeight;
	int roiOffsetX;
	int roiOffsetY;
	/* Frame Partial Mode (DON'T SUPPORT)*/
	int usePartialMode;
	/* Rotation Angle (0, 90, 180, 270) */
	int rotAngle;
	/* mirror direction (0-no mirror, 1-vertical, 2-horizontal, 3-both) */
	int mirDir;
	/* Scale Mode */
	int iHorScaleMode;
	int iVerScaleMode;
	/* stream data length */
	int iDataLen;
	int dst_type;
	CVIFRAMEBUF dec_buf;
} CVIDecConfigParam;

/* jpu config param */
typedef struct {
	CVIJpgCodType type;
	union {
		CVIEncConfigParam enc;
		CVIDecConfigParam dec;
	} u;
	int s32ChnNum;
} CVIJpgConfig;

typedef struct _cviJpegChnAttr_ {
	unsigned int
		u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	unsigned int
		fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	unsigned int picWidth; ///< width of a picture to be encoded
	unsigned int picHeight; ///< height of a picture to be encoded
} cviJpegChnAttr;

typedef struct _cviJpegUserData_ {
	unsigned char *userData;
	unsigned int len;
} cviJpegUserData;

/* JPU CODEC HANDLE */
typedef void *CVIJpgHandle;

void cviJpgGetVersion(void);

/* initial jpu core */
int CVIJpgInit(void);
/* uninitial jpu core */
void CVIJpgUninit(void);
/* alloc a jpu handle for dcoder or encoder */
CVIJpgHandle CVIJpgOpen(CVIJpgConfig config);
/* jpu decode and encode capacity */
int CVIJpgGetCaps(CVIJpgHandle jpgHandle);
/* close and free alloced jpu handle */
int CVIJpgClose(CVIJpgHandle jpgHandle);
/* reset jpu core */
int CVIJpgReset(CVIJpgHandle jpgHandle);
/* flush data */
int CVIJpgFlush(CVIJpgHandle jpgHandle);
/* send jpu data to decode or encode */
int CVIJpgSendFrameData(CVIJpgHandle jpgHandle, void *data, int length,
			int s32TimeOut);
/* after decoded or encoded, get data from jpu */
int CVIJpgGetFrameData(CVIJpgHandle jpgHandle, void *data, int length,
		       unsigned long int *pu64HwTime);
/* get jpu encoder input data buffer */
int CVIJpgGetInputDataBuf(CVIJpgHandle jpgHandle, void *data, int length);
/* release stream buffer */
int CVIJpgReleaseFrameData(CVIJpgHandle jpgHandle);
int cviJpegIoctl(void *handle, int op, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* CVI_JPG_INTERFACE_H
*/
