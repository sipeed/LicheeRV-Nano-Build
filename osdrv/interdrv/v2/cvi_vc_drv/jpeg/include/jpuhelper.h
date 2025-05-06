#ifndef _JPU_HELPER_H_
#define _JPU_HELPER_H_

#include "jpurun.h"
#include "../jpuapi/jpuapi.h"

typedef struct {
	char SrcFileName[256];
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

	// H.264 ONLY
	int ConstIntraPredFlag;
	int DisableDeblk;
	int DeblkOffsetA;
	int DeblkOffsetB;
	int ChromaQpOffset;
	int PicQpY;

	// MJPEG ONLY
	char HuffTabName[256];
	char QMatTabName[256];
	int VersionID;
	int FrmFormat;
	int SrcFormat;
	int RstIntval;
	int ThumbEnable;
	int ThumbSizeX;
	int ThumbSizeY;

	// COMMON
	int GopPicNum;
	int SliceMode;
	int SliceSizeMode;
	int SliceSizeNum;

	int IntraRefreshNum;

	int ConstantIntraQPEnable;
	int RCIntraQP;
	int MaxQpSetEnable;
	int MaxQp;
	int GammaSetEnable;
	int Gamma;
	int HecEnable;

	// RC
	int RcEnable;
	int RcBitRate;
	int RcInitDelay;
	int RcBufSize;

	// NEW RC Scheme
	int RcIntervalMode;
	int RcMBInterval;
	int IntraCostWeight;
	int SearchRange;
	int MeUseZeroPmv;
	int MeBlkModeEnable;

} ENC_CFG;

typedef struct {
	FrameFormat sourceFormat;
	int restartInterval;
	BYTE huffVal[4][162];
	BYTE huffBits[4][256];
	BYTE qMatTab[4][64];
} EncMjpgParam;

#if defined(__cplusplus)
extern "C" {
#endif

int jpgGetHuffTable(char *huffFileName, EncMjpgParam *param);
int jpgGetQMatrix(char *qMatFileName, EncMjpgParam *param);

int getJpgEncOpenParamDefault(JpgEncOpenParam *pEncOP,
			      EncConfigParam *pEncConfig);

JpgRet WriteJpgBsBufHelper(JpgDecHandle handle, BufInfo *pBufInfo,
			   PhysicalAddress paBsBufStart,
			   PhysicalAddress paBsBufEnd, int defaultsize,
			   int checkeos, int *pstreameos, int endian);

#ifdef REDUNDENT_CODE
JpgRet ReadJpgBsBufHelper(JpgEncHandle handle, FILE *bsFp,
			  PhysicalAddress paBsBufStart,
			  PhysicalAddress paBsBufEnd, int encHeaderSize,
			  int endian);
#endif

#ifdef REDUNDENT_CODE
int LoadYuvPartialImageHelperFormat(
	FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY, PhysicalAddress addrCb,
	PhysicalAddress addrCr, int picWidth, int picHeight,
	int picHeightPartial, int stride, int interleave, int format,
	int endian, int partPosIdx, int frameIdx, int packed);

int SaveYuvImageHelperFormat(FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY,
			     PhysicalAddress addrCb, PhysicalAddress addrCr,
			     int picWidth, int picHeight, int stride,
			     int interLeave, int format, int endian, int packed,
			     int *pFrameSize);

int SaveYuvPartialImageHelperFormat(
	FILE *yuvFp, Uint8 *pYuv, PhysicalAddress addrY, PhysicalAddress addrCb,
	PhysicalAddress addrCr, int picWidth, int picHeight,
	int picHeightPartial, int stride, int interLeave, int format,
	int endian, int partPosIdx, int frameIdx, int packed);
#endif

unsigned int GetFrameBufSize(int framebufFormat, int picWidth, int picHeight);
#ifdef REDUNDENT_CODE
void GetMcuUnitSize(int format, int *mcuWidth, int *mcuHeight);
#endif

typedef enum {
	YUV444,
	YUV422,
	YUV420,
	NV12,
	NV21,
	YUV400,
	YUYV,
	YVYU,
	UYVY,
	VYUY,
	YYY,
	RGB_PLANAR,
	RGB32,
	RGB24,
	RGB16
} yuv2rgb_color_format;
void jpu_yuv2rgb(int width, int height, yuv2rgb_color_format format,
		 unsigned char *src, unsigned char *rgba, int chroma_reverse);
yuv2rgb_color_format
convert_jpuapi_format_to_yuv2rgb_color_format(int planar_format,
					      int pack_format, int interleave);
Uint64 jpgGetCurrentTime(void);
#if defined(__cplusplus)
}
#endif

#endif //#ifndef _JPU_HELPER_H_
