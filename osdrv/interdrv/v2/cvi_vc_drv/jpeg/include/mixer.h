
#ifndef _MIXER_H_
#define _MIXER_H_

#include "jpuconfig.h"

typedef struct {
	int Format;
	int Index;
	jpu_buffer_t vbY;
	jpu_buffer_t vbCb;
	jpu_buffer_t vbCr;
	int strideY;
	int strideC;
} FRAME_BUF;

#define MAX_DISPLAY_WIDTH 1920
#define MAX_DISPLAY_HEIGHT 1088

#if defined(__cplusplus)
extern "C" {
#endif

int AllocateFrameBuffer(int instIdx, int format, int width, int height,
			int frameBufNum, int pack);
void FreeFrameBuffer(int instIdx);
FRAME_BUF *GetFrameBuffer(int instIdx, int index);
int GetFrameBufBase(int instIdx);
int GetFrameBufAllocSize(int instIdx);
void ClearFrameBuffer(int instIdx, int index);
FRAME_BUF *FindFrameBuffer(int instIdx, PhysicalAddress addrY);
#ifdef JPU_FPGA_PLATFORM
int SetMixerDecOutLayer(int instIdx, int index, int picX, int picY);
int SetMixerDecOutFrame(FRAME_BUF *pFrame, int width, int height);
void WaitMixerInt(void);
#endif
int sw_mixer_open(int instIdx, int width, int height);
int sw_mixer_draw(int instIdx, int x, int y, int width, int height,
		  int planar_format, int pack_format, int inteleave,
		  unsigned char *pbImage);
void sw_mixer_close(int instIdx);

#if defined(__cplusplus)
}
#endif

#endif //#ifndef _MIXER_H_
