#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <fcntl.h> /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "cvi_sys.h"
#include "cvi_ive.h"
#include "linux/cvi_ive_ioctl.h"

#define IVE_DEV_NODE "/dev/cvi-ive"
#define STRFY(s) #s

typedef void *IVE_HANDLE;

enum DIRECTION {
	VERTICAL = 0,
	HORIZON = 1,
	SLASH = 2,
	BACK_SLASH = 3,
	ZERO = 4,
};

struct IVE_HANDLE_CTX {
	int devfd;
	int used_time;
};

struct IVE_HANDLE_CTX handle_ctx;

const char *cviIveImgEnTypeStr[] = {
	STRFY(IVE_IMAGE_TYPE_U8C1),	    STRFY(IVE_IMAGE_TYPE_S8C1),
	STRFY(IVE_IMAGE_TYPE_YUV420SP),	    STRFY(IVE_IMAGE_TYPE_YUV422SP),
	STRFY(IVE_IMAGE_TYPE_YUV420P),	    STRFY(IVE_IMAGE_TYPE_YUV422P),
	STRFY(IVE_IMAGE_TYPE_S8C2_PACKAGE), STRFY(IVE_IMAGE_TYPE_S8C2_PLANAR),
	STRFY(IVE_IMAGE_TYPE_S16C1),	    STRFY(IVE_IMAGE_TYPE_U16C1),
	STRFY(IVE_IMAGE_TYPE_U8C3_PACKAGE), STRFY(IVE_IMAGE_TYPE_U8C3_PLANAR),
	STRFY(IVE_IMAGE_TYPE_S32C1),	    STRFY(IVE_IMAGE_TYPE_U32C1),
	STRFY(IVE_IMAGE_TYPE_S64C1),	    STRFY(IVE_IMAGE_TYPE_U64C1),
	STRFY(IVE_IMAGE_TYPE_BF16C1),	    STRFY(IVE_IMAGE_TYPE_FP32C1)
};

int open_ive_dev()
{
	if (handle_ctx.devfd != 0)
		return handle_ctx.devfd;

	int devfd;

	do {
		if (handle_ctx.devfd <= 0)
			devfd = open(IVE_DEV_NODE, O_RDWR);
		else
			devfd = handle_ctx.devfd;
		if (devfd < 0) {
			printf("Can't open %s\n", IVE_DEV_NODE);
			return ERR_IVE_OPEN_FILE;
		}
	} while (0);
	return devfd;
}

uint32_t WidthAlign(const uint32_t width, const uint32_t align)
{
	uint32_t stride = (uint32_t)(width / align) * align;
	if (stride < width) {
		stride += align;
	}
	return stride;
}

void dump_data_u8(const CVI_U8 *data, CVI_S32 count, const CVI_CHAR *desc)
{
	CVI_S32 i;

	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%02X ", data[i]);
		if ((i + 1) % 8 == 0 || i + 1 == count) {
			printf(" ");
			if ((i + 1) % 16 == 0) {
				printf("\n");
			} else if (i + 1 == count) {
				printf("\n");
			}
		}
	}
}

static CVI_S32 find_first_diff_u8(const CVI_U8 *A, const CVI_U8 *B,
				  CVI_S32 count)
{
	CVI_S32 i;
	CVI_S32 result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

static CVI_S32 find_first_diff_u16(const CVI_U16 *A, const CVI_U16 *B,
				   CVI_S32 count)
{
	CVI_S32 i;
	CVI_S32 result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u16(const CVI_U16 *data, CVI_S32 count, const CVI_CHAR *desc)
{
	CVI_S32 i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%04X ", data[i]);
		if ((i + 1) % 8 == 0 || i + 1 == count) {
			printf(" ");
			if ((i + 1) % 16 == 0) {
				printf("\n");
			} else if (i + 1 == count) {
				printf("\n");
			}
		}
	}
}

static CVI_S32 find_first_diff_u32(const CVI_U32 *A, const CVI_U32 *B,
				   CVI_S32 count)
{
	CVI_S32 i;
	CVI_S32 result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}
void dump_data_u32(const CVI_U32 *data, CVI_S32 count, const CVI_CHAR *desc)
{
	CVI_S32 i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%08X ", data[i]);
		if ((i + 1) % 4 == 0 || i + 1 == count) {
			printf(" ");
			if ((i + 1) % 8 == 0) {
				printf("\n");
			} else if (i + 1 == count) {
				printf("\n");
			}
		}
	}
}

static CVI_S32 find_first_diff_u64(const CVI_U64 *A, const CVI_U64 *B,
				   CVI_S32 count)
{
	CVI_S32 i;
	CVI_S32 result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u64(const CVI_U64 *data, CVI_S32 count, const CVI_CHAR *desc)
{
	CVI_S32 i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%016jX ", data[i]);
		if ((i + 1) % 2 == 0 || i + 1 == count) {
			printf(" ");
			if ((i + 1) % 4 == 0) {
				printf("\n");
			} else if (i + 1 == count) {
				printf("\n");
			}
		}
	}
}

void boundary_check(IVE_POINT_U16_S *point, int width, int height)
{
	if ((CVI_S16)point->u16X < 0) {
		point->u16X = 0;
	}
	if ((CVI_S16)point->u16Y < 0) {
		point->u16Y = 0;
	}
	if (point->u16X >= width) {
		point->u16X = width - 1;
	}
	if (point->u16Y >= height) {
		point->u16Y = height - 1;
	}
}

int do_hysteresis_wo_ang(unsigned char *pEdgeMap,
			 unsigned char *dst, int imw, int imh, int stride)
{
	IVE_POINT_U16_S p1, p2, p;
	unsigned char *visited =
		(unsigned char *)calloc(imh * stride, sizeof(unsigned char));
	unsigned char *pdst =
		(unsigned char *)calloc(imh * stride, sizeof(unsigned char));

	for (int i = 0; i < imh; i++) {
		for (int j = 0; j < imw; j++) {
			int index = i * stride + j;

			if (pEdgeMap[index] == 2) {
				pdst[index] = 255;
			} else {
				pdst[index] = 0;
			}
		}
	}

	for (int y = 1; y < imh - 1; y++) {
		for (int x = 1; x < imw - 1; x++) {
			// if pixel not in upper or had visited, skip
			if (pEdgeMap[y * stride + x] != 2 ||
				visited[y * stride + x] > 0)
				continue;
			p.u16X = x;
			p.u16Y = y;
			visited[p.u16Y * stride + p.u16X] = 255;
			pdst[p.u16Y * stride + p.u16X] = 255;
			for (unsigned char dir = 0; dir < 4; dir++) {
				if (dir == VERTICAL) {
					p1.u16X = p.u16X + 1;
					p1.u16Y = p.u16Y;
					p2.u16X = p.u16X - 1;
					p2.u16Y = p.u16Y;
				} else if (dir == HORIZON) {
					p1.u16X = p.u16X;
					p1.u16Y = p.u16Y + 1;
					p2.u16X = p.u16X;
					p2.u16Y = p.u16Y - 1;
				} else if (dir == SLASH) {
					p1.u16X = p.u16X + 1;
					p1.u16Y = p.u16Y - 1;
					p2.u16X = p.u16X - 1;
					p2.u16Y = p.u16Y + 1;
				} else if (dir == BACK_SLASH) {
					p1.u16X = p.u16X + 1;
					p1.u16Y = p.u16Y + 1;
					p2.u16X = p.u16X - 1;
					p2.u16Y = p.u16Y - 1;
				} else {
					p1.u16X = p.u16X;
					p1.u16Y = p.u16Y;
					p2.u16X = p.u16X;
					p2.u16Y = p.u16Y;
				}
				boundary_check(&p1, imw, imh);
				boundary_check(&p2, imw, imh);
				if (pEdgeMap[p1.u16Y * stride + p1.u16X] == 0 &&
					visited[p1.u16Y * stride + p1.u16X] == 0) {
					visited[p1.u16Y * stride + p1.u16X] = 255;
					pdst[p1.u16Y * stride + p1.u16X] = 255;
				}
				if (pEdgeMap[p2.u16Y * stride + p2.u16X] == 0 &&
					visited[p2.u16Y * stride + p2.u16X] == 0) {
					visited[p2.u16Y * stride + p2.u16X] = 255;
					pdst[p2.u16Y * stride + p2.u16X] = 255;
				}
			}
		}
	}

	memcpy(dst, pdst, stride * imh);
	free(visited);
	free(pdst);
	return 0;
}

IVE_HANDLE CVI_IVE_CreateHandle()
{
	handle_ctx.devfd = open_ive_dev();
	handle_ctx.used_time++;
	return &handle_ctx;
}

CVI_S32 CVI_IVE_DestroyHandle(IVE_HANDLE pIveHandle)
{
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	p->used_time--;
	if (p->used_time <= 0 && p->devfd > 0) {
		close(p->devfd);
		p->devfd = 0;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CompareIveData(IVE_DATA_S *pstData1, IVE_DATA_S *pstData2)
{
	int n;
	CVI_U32 y;

	if (pstData1->u32Width != pstData2->u32Width) {
		printf("Not same u32Width, %d vs %d\n", pstData1->u32Width,
			   pstData2->u32Width);
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	if (pstData1->u32Height != pstData2->u32Height) {
		printf("Not same u32Height, %d vs %d\n", pstData1->u32Height,
			   pstData2->u32Height);
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	printf("compare A at %p, B at %p\n",
		   (char *)(uintptr_t)pstData1->u64VirAddr,
		   (char *)(uintptr_t)pstData2->u64VirAddr);


	for (y = 0; y < pstData1->u32Height; y++) {
		n = memcmp((char *)(uintptr_t)pstData1->u64VirAddr + y * pstData1->u32Stride,
		(char *)(uintptr_t)pstData2->u64VirAddr + y * pstData1->u32Stride,
		pstData1->u32Width);
		if (n != 0) {
			printf("Not same content\n");
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CompareIveMem(IVE_MEM_INFO_S *pstMem1, IVE_MEM_INFO_S *pstMem2)
{
	int n;

	if (pstMem1->u32Size != pstMem2->u32Size || pstMem1->u32Size == 0) {
		printf("Not same u32Size, %d vs %d\n", pstMem1->u32Size,
			   pstMem2->u32Size);
		return CVI_ERR_IVE_INVALID_DEVID;
	}

	if ((char *)(uintptr_t)pstMem1->u64VirAddr == NULL ||
		(char *)(uintptr_t)pstMem2->u64VirAddr == NULL) {
		printf("invalid address\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}

	printf("compare A at %p, B at %p\n",
		   (char *)(uintptr_t)pstMem1->u64VirAddr,
		   (char *)(uintptr_t)pstMem2->u64VirAddr);
	n = memcmp((char *)(uintptr_t)pstMem1->u64VirAddr,
		   (char *)(uintptr_t)pstMem2->u64VirAddr, pstMem1->u32Size);
	if (n != 0) {
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CompareIveImage(IVE_IMAGE_S *pstImage1, IVE_IMAGE_S *pstImage2)
{
	CVI_U32 u32Stride;
	CVI_S32 s32Succ;
	IVE_IMAGE_TYPE_E enType;
	CVI_U32 u32Width;
	CVI_U32 u32Height;
	CVI_U8 *pData1;
	CVI_U8 *pData2;
	CVI_U16 *pData1_U16;
	CVI_U16 *pData2_U16;
	CVI_U32 *pData1_U32;
	CVI_U32 *pData2_U32;
	CVI_U64 *pData1_U64;
	CVI_U64 *pData2_U64;
	CVI_U16 y;
	int n;

	if (pstImage1->enType != pstImage2->enType) {
		printf("Not same IMAGE_TYPE, %d vs %d\n", pstImage1->enType,
			   pstImage2->enType);
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	if (pstImage1->u32Width != pstImage2->u32Width) {
		printf("Not same u32Width, %d vs %d\n", pstImage1->u32Width,
			   pstImage2->u32Width);
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	if (pstImage1->u32Height != pstImage2->u32Height) {
		printf("Not same u32Height, %d vs %d\n", pstImage1->u32Height,
			   pstImage2->u32Height);
		return CVI_ERR_IVE_INVALID_DEVID;
	}

	enType = pstImage1->enType;
	u32Width = pstImage1->u32Width;
	u32Height = pstImage1->u32Height;

	//u32Stride = CVI_CalcStride(u32Width, CVI_IVE2_STRIDE_ALIGN);
	u32Stride = pstImage1->u32Stride[0];//WidthAlign(u32Width, DEFAULT_ALIGN);
	s32Succ = CVI_SUCCESS;

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compare A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width, "A");
				dump_data_u8(pData2, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_YUV420SP: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compareY A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare Y failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width, "A");
				dump_data_u8(pData2, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}

		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[1];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[1];
		printf("compareUV A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height / 2;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare UV failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width, "A");
				dump_data_u8(pData2, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_YUV422SP: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compareY A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare Y failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width, "A");
				dump_data_u8(pData2, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}

		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[1];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[1];
		printf("compareUV A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare UV failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width, "A");
				dump_data_u8(pData2, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
	case IVE_IMAGE_TYPE_S8C2_PLANAR: {
		printf("Unsupported IMAGE_TYPE %d\n", enType);
		s32Succ = CVI_FAILURE;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compare U8C3_PACKAGE A at %p, B at %p\n", pData1,
			   pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width * 3);
			//dump_data_u8(pData1, u32Width, "dst");
			//dump_data_u8(pData2, u32Width, "ref");
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32Width * 3);
				CVI_ASSERT(idx >= 0);
				printf("Compare U8C3_PACKAGE failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32Width * 3, "A");
				dump_data_u8(pData2, u32Width * 3, "B");
				CVI_IVE_WriteImg(0, "sample_CSC_YUV4442RGB_1.yuv", pstImage1);
				CVI_IVE_WriteImg(0, "sample_CSC_YUV4442RGB_2.yuv", pstImage2);
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		for (int i = 0; i < 3; i++) {
			char YUVStr[4] = { 'Y', 'U', 'V' };
			pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[i];
			pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[i];
			printf("compare U8C3_PLANAR[%c] A at %p, B at %p\n",
				   YUVStr[i], pData1, pData2);
			for (y = 0; y < u32Height;
				 y++, pData1 += u32Stride, pData2 += u32Stride) {
				n = memcmp(pData1, pData2, u32Width);
				if (n != 0) {
					int idx = find_first_diff_u8(
						pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare %c failed, at y = %d, x = %d, n = %d\n",
						   YUVStr[i], y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n",
						   pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
	} break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compare A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width * 2);
			if (n != 0) {
				pData1_U16 = (CVI_U16 *)pData1;
				pData2_U16 = (CVI_U16 *)pData2;
				int idx = find_first_diff_u16(
					pData1_U16, pData2_U16, u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%04X, B = 0x%04X\n",
					   pData1_U16[idx], pData2_U16[idx]);
				dump_data_u16(pData1_U16, u32Width, "A");
				dump_data_u16(pData2_U16, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compareU32C1 A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			n = memcmp(pData1, pData2, u32Width * 4);
			if (n != 0) {
				pData1_U32 = (CVI_U32 *)pData1;
				pData2_U32 = (CVI_U32 *)pData2;
				int idx = find_first_diff_u32(
					pData1_U32, pData2_U32, u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%08X, B = 0x%08X\n",
					   pData1_U32[idx], pData2_U32[idx]);
				dump_data_u32(pData1_U32, u32Width, "A");
				dump_data_u32(pData2_U32, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compareU64C1 A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32Height;
			 y++, pData1 += u32Stride, pData2 += u32Stride) {
			int m = memcmp(pData1, pData2, u32Width * 4);

			n = memcmp(pData1 + u32Width * 4, pData2 + u32Width * 4,
				   u32Width * 4);
			if (n != 0 || m != 0) {
				pData1_U64 = (CVI_U64 *)pData1;
				pData2_U64 = (CVI_U64 *)pData2;
				int idx = find_first_diff_u64(
					pData1_U64, pData2_U64, u32Width);
				CVI_ASSERT(idx >= 0);
				printf("Compare failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%016jX, B = 0x%016jX\n",
					   pData1_U64[idx], pData2_U64[idx]);
				dump_data_u64(pData1_U64, u32Width, "A");
				dump_data_u64(pData2_U64, u32Width, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	default: {
		printf("Unknown IMAGE_TYPE %d\n", enType);
		s32Succ = CVI_ERR_IVE_ILLEGAL_PARAM;
	} break;
	}
	return s32Succ;
}

CVI_S32 CVI_IVE_CompareSADImage(IVE_IMAGE_S *pstImage1, IVE_IMAGE_S *pstImage2,
				IVE_SAD_MODE_E mode, CVI_BOOL isDMAhalf)
{
	CVI_U32 u32Stride;
	CVI_S32 s32Succ;
	IVE_IMAGE_TYPE_E enType;
	CVI_U32 u32Width, u32SADWidth;
	CVI_U32 u32Height, u32SADHeight;
	CVI_U32 u32SADStride;
	CVI_U32 u32ByteSize;
	CVI_U8 *pData1;
	CVI_U8 *pData2;
	CVI_U16 y;
	int n;

	if (pstImage1->enType != pstImage2->enType) {
		printf("Not same IMAGE_TYPE, %d vs %d\n", pstImage1->enType,
			   pstImage2->enType);
		return CVI_FAILURE;
	}
	if (pstImage1->u32Width != pstImage2->u32Width) {
		printf("Not same u32Width, %d vs %d\n", pstImage1->u32Width,
			   pstImage2->u32Width);
		return CVI_FAILURE;
	}
	if (pstImage1->u32Height != pstImage2->u32Height) {
		printf("Not same u32Height, %d vs %d\n", pstImage1->u32Height,
			   pstImage2->u32Height);
		return CVI_FAILURE;
	}

	if (pstImage1->u32Stride[0] != pstImage2->u32Stride[0]) {
		printf("Not same u32Stride, %d vs %d\n", pstImage1->u32Stride[0],
			   pstImage2->u32Stride[0]);
		return CVI_FAILURE;
	}

	enType = pstImage1->enType;
	u32Width = pstImage1->u32Width;
	u32Height = pstImage1->u32Height;

	u32Stride = pstImage1->u32Stride[0];
	s32Succ = CVI_SUCCESS;
	switch (enType) {
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		u32ByteSize = 2;
		break;
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		u32ByteSize = 1;
		break;
	default:
		printf("not support output type %d, return\n", enType);
		return CVI_FAILURE;
	}
	switch (mode) {
	case IVE_SAD_MODE_MB_4X4:
		u32SADWidth = u32Width * u32ByteSize / 4;
		u32SADHeight = u32Height / 4;
		break;
	case IVE_SAD_MODE_MB_8X8:
		u32SADWidth = u32Width * u32ByteSize / 8;
		u32SADHeight = u32Height / 8;
		break;
	case IVE_SAD_MODE_MB_16X16:
		u32SADWidth = u32Width * u32ByteSize / 16;
		u32SADHeight = u32Height / 16;
		break;
	default:
		printf("not support mode type %d, return\n", mode);
		return CVI_FAILURE;
	}
	u32SADStride = (isDMAhalf) ? pstImage1->u32Stride[0] / 2 : pstImage1->u32Stride[0];
	printf("SAD Width %u Height %u Stride %u %u\n", u32SADWidth,
		   u32SADHeight, u32SADStride, u32Stride);

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1: {
		pData1 = (CVI_U8 *)(uintptr_t)pstImage1->u64VirAddr[0];
		pData2 = (CVI_U8 *)(uintptr_t)pstImage2->u64VirAddr[0];
		printf("compareSAD A at %p, B at %p\n", pData1, pData2);
		for (y = 0; y < u32SADHeight;
			 y++, pData1 += u32SADStride, pData2 += u32Stride) { //dst=96 ref=384
			n = memcmp(pData1, pData2, u32SADWidth);
			if (n != 0) {
				int idx = find_first_diff_u8(pData1, pData2,
								 u32SADWidth);
				CVI_ASSERT(idx >= 0);
				printf("Compare failed, at y = %d, x = %d, n = %d\n",
					   y, idx, n);
				printf("  A = 0x%02X, B = 0x%02X\n",
					   pData1[idx], pData2[idx]);
				dump_data_u8(pData1, u32SADWidth, "A");
				dump_data_u8(pData2, u32SADWidth, "B");
				s32Succ = CVI_FAILURE;
				break;
			}
		}
	} break;
	default: {
		printf("Unknown IMAGE_TYPE %d\n", enType);
		s32Succ = CVI_ERR_IVE_ILLEGAL_PARAM;
	} break;
	}

	return s32Succ;
}

CVI_S32 CVI_IVE_VideoFrameInfo2Image(VIDEO_FRAME_INFO_S *pstVFISrc, IVE_IMAGE_S *pstIIDst)
{
	CVI_U32 u32Channel = 1;
	VIDEO_FRAME_S *pstVFSrc = &pstVFISrc->stVFrame;
	IVE_IMAGE_TYPE_E img_type = IVE_IMAGE_TYPE_U8C1;

	switch (pstVFSrc->enPixelFormat) {
	case PIXEL_FORMAT_YUV_400: {
		img_type = IVE_IMAGE_TYPE_U8C1;
	} break;
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV12: {
		img_type = IVE_IMAGE_TYPE_YUV420SP;
		u32Channel = 2;
	} break;
	case PIXEL_FORMAT_YUV_PLANAR_420: {
		img_type = IVE_IMAGE_TYPE_YUV420P;
		u32Channel = 3;
	} break;
	case PIXEL_FORMAT_YUV_PLANAR_422: {
		img_type = IVE_IMAGE_TYPE_YUV422P;
		u32Channel = 3;
	} break;
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888: {
		img_type = IVE_IMAGE_TYPE_U8C3_PACKAGE;
	} break;
	case PIXEL_FORMAT_RGB_888_PLANAR: {
		img_type = IVE_IMAGE_TYPE_U8C3_PLANAR;
		u32Channel = 3;
	} break;
	case PIXEL_FORMAT_INT16_C1: {
		img_type = IVE_IMAGE_TYPE_S16C1;
	} break;
	case PIXEL_FORMAT_UINT16_C1: {
		img_type = IVE_IMAGE_TYPE_U16C1;
	} break;
	default: {
		printf("Unsupported conversion type: %u.\n", pstVFSrc->enPixelFormat);
		return CVI_FAILURE;
	}
	}

	for (CVI_U32 i = 0; i < u32Channel; i++) {
		pstIIDst->u64VirAddr[i] = (CVI_U64)(uintptr_t)pstVFSrc->pu8VirAddr[i];
		pstIIDst->u64PhyAddr[i] = pstVFSrc->u64PhyAddr[i];
		pstIIDst->u32Stride[i] = pstVFSrc->u32Stride[i];
	}
	pstIIDst->enType = img_type;
	pstIIDst->u32Width = pstVFSrc->u32Width;
	pstIIDst->u32Height = pstVFSrc->u32Height;
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Image2VideoFrameInfo(IVE_IMAGE_S *pstIISrc, VIDEO_FRAME_INFO_S *pstVFIDst)
		//, CVI_BOOL invertPackage)
{
	CVI_U32 u32Channel = 1;
	VIDEO_FRAME_S *pstVFSrc = &pstVFIDst->stVFrame;
	PIXEL_FORMAT_E img_type = PIXEL_FORMAT_YUV_400;

	pstVFIDst->u32PoolId = -1;
	switch (pstIISrc->enType) {
	case IVE_IMAGE_TYPE_U8C1: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		img_type = PIXEL_FORMAT_YUV_400;
	} break;
	case IVE_IMAGE_TYPE_YUV420SP: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[1] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		img_type = PIXEL_FORMAT_NV21;
		u32Channel = 2;
	} break;
	case IVE_IMAGE_TYPE_YUV420P: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[1] = WidthAlign(pstIISrc->u32Width >> 1, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[2] = pstVFSrc->u32Stride[1];
		img_type = PIXEL_FORMAT_YUV_PLANAR_420;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_YUV422P: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[1] = WidthAlign(pstIISrc->u32Width >> 1, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[2] = pstVFSrc->u32Stride[1];
		img_type = PIXEL_FORMAT_YUV_PLANAR_422;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width * 3, DEFAULT_ALIGN);
		img_type = PIXEL_FORMAT_RGB_888;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		pstVFSrc->u32Stride[0] = WidthAlign(pstIISrc->u32Width, DEFAULT_ALIGN);
		pstVFSrc->u32Stride[1] = pstVFSrc->u32Stride[0];
		pstVFSrc->u32Stride[2] = pstVFSrc->u32Stride[0];
		img_type = PIXEL_FORMAT_RGB_888_PLANAR;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_S16C1: {
		pstVFSrc->u32Stride[0] =
			WidthAlign(pstIISrc->u32Width * sizeof(int16_t), DEFAULT_ALIGN);
		img_type = PIXEL_FORMAT_INT16_C1;
	} break;
	case IVE_IMAGE_TYPE_U16C1: {
		pstVFSrc->u32Stride[0] =
			WidthAlign(pstIISrc->u32Width * sizeof(uint16_t), DEFAULT_ALIGN);
		img_type = PIXEL_FORMAT_UINT16_C1;
	} break;
	default: {
		printf("Unsupported conversion type: %u.\n", pstIISrc->enType);
		return CVI_FAILURE;
	}
	}

	for (CVI_U32 i = 0; i < u32Channel; i++) {
		pstVFSrc->pu8VirAddr[i] = (CVI_U8 *)(uintptr_t)pstIISrc->u64VirAddr[i];
		pstVFSrc->u64PhyAddr[i] = pstIISrc->u64PhyAddr[i];
		pstVFSrc->u32Length[i] = pstIISrc->u32Height * pstIISrc->u32Stride[i];
	}
	for (CVI_U32 i = u32Channel; i < 3; i++) {
		pstVFSrc->pu8VirAddr[i] = 0;
		pstVFSrc->u64PhyAddr[i] = 0;
		pstVFSrc->u32Stride[i] = 0;
		pstVFSrc->u32Length[i] = 0;
	}
	pstVFSrc->enPixelFormat = img_type;
	pstVFSrc->u32Width = pstIISrc->u32Width;
	pstVFSrc->u32Height = pstIISrc->u32Height;

	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_ReadData(IVE_HANDLE pIveHandle, IVE_DATA_S *pstData,
			 const char *filename, CVI_U16 u32Width,
			 CVI_U16 u32Height)
{
	CVI_U16 u32Stride = WidthAlign(u32Width, DEFAULT_ALIGN);
	int uSize = u32Stride * u32Height;
	CVI_S32 s32Succ = CVI_SUCCESS;
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Can't open %s\n", filename);
		return ERR_IVE_OPEN_FILE;
	}

	s32Succ = CVI_IVE_CreateDataInfo(pIveHandle, pstData, u32Width,
					 u32Height);
	if (s32Succ) {
		printf("Ion alloc failed\n");
		return s32Succ;
	}

	int readCnt =
		fread((char *)(uintptr_t)pstData->u64VirAddr, 1, uSize, fp);

	if (readCnt == 0) {
		printf("Image %s read failed.\n", filename);
		return ERR_IVE_READ_FILE;
	}
	fclose(fp);
	return s32Succ;
}

CVI_S32 CVI_IVE_ReadDataArray(IVE_HANDLE pIveHandle, IVE_DATA_S *pstData,
				  char *pBuffer, CVI_U16 u32Width, CVI_U16 u32Height)
{
	char *ptr = NULL;
	CVI_S32 s32Succ = CVI_SUCCESS;

	s32Succ = CVI_IVE_CreateDataInfo(pIveHandle, pstData, u32Width,
					 u32Height);
	if (s32Succ) {
		printf("Ion alloc failed\n");
		return s32Succ;
	}
	printf("u32Stride = %d\n", pstData->u32Stride);
	ptr = pBuffer;
	for (size_t j = 0; j < (size_t)u32Height; j++) {
		memcpy((char *)(uintptr_t)(pstData->u64VirAddr +
				(j * pstData->u32Stride)), ptr, u32Width);
		ptr += u32Width;
	}
	return s32Succ;
}

CVI_S32 CVI_IVE_ReadMem(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMem,
			const char *filename, CVI_U32 uSize)
{
	CVI_S32 s32Succ = CVI_SUCCESS;
	FILE *fp;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("Can't open %s\n", filename);
		return ERR_IVE_OPEN_FILE;
	}

	s32Succ = CVI_IVE_CreateMemInfo(pIveHandle, pstMem, uSize);
	if (s32Succ) {
		printf("Ion alloc failed\n");
		return s32Succ;
	}

	int readCnt =
		fread((char *)(uintptr_t)pstMem->u64VirAddr, 1, uSize, fp);

	if (readCnt == 0) {
		printf("Image %s read failed.\n", filename);
		return ERR_IVE_READ_FILE;
	}
	fclose(fp);
	return s32Succ;
}

CVI_S32 CVI_IVE_ReadMemArray(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMem,
				 char *pBuffer, CVI_U32 uSize)
{
	CVI_S32 s32Succ = CVI_SUCCESS;

	s32Succ = CVI_IVE_CreateMemInfo(pIveHandle, pstMem, uSize);
	if (s32Succ) {
		printf("Ion alloc failed\n");
		return s32Succ;
	}
	memcpy((char *)(uintptr_t)pstMem->u64VirAddr, (void *)pBuffer, uSize);
	return s32Succ;
}

CVI_S32 CVI_IVE_ReadImageArray(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg,
				   char *pBuffer, IVE_IMAGE_TYPE_E enType,
				   CVI_U16 u32Width, CVI_U16 u32Height)
{
	char *ptr = NULL;

	memset(pstImg, 0, sizeof(IVE_IMAGE_S));

	CVI_IVE_CreateImage(pIveHandle, pstImg, enType, u32Width, u32Height);

	if (enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
		ptr = pBuffer;
		for (size_t j = 0; j < (size_t)u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[0] +
						   (j * pstImg->u32Stride[0])),
				   ptr, u32Width);
			ptr += u32Width;
		}
		for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[1] +
						   (j * pstImg->u32Stride[1])),
				   ptr, u32Width);
			ptr += u32Width;
		}
		for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[2] +
						   (j * pstImg->u32Stride[2])),
				   ptr, u32Width);
			ptr += u32Width;
		}
	} else if (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
		// yyy... uuu... vvv... to yyy... uuu... vvv...
		ptr = pBuffer;
		for (size_t i = 0; i < (size_t)u32Height; i++) {
			uint32_t stb_stride = i * u32Width * 3;
			uint32_t image_stride = (i * pstImg->u32Stride[0]);

			for (size_t j = 0; j < (size_t)u32Width; j++) {
				uint32_t buf_idx = stb_stride + (j * 3);
				uint32_t img_idx = image_stride + (j * 3);
				((char *)(uintptr_t)
					 pstImg->u64VirAddr[0])[img_idx] =
					ptr[buf_idx];
				((char *)(uintptr_t)
					 pstImg->u64VirAddr[0])[img_idx + 1] =
					ptr[buf_idx + 1];
				((char *)(uintptr_t)
					 pstImg->u64VirAddr[0])[img_idx + 2] =
					ptr[buf_idx + 2];
			}
		}
	} else if (enType == IVE_IMAGE_TYPE_YUV420SP) {
		ptr = pBuffer;
		for (size_t j = 0; j < (size_t)u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[0] +
						   (j * pstImg->u32Stride[0])),
				   ptr, u32Width);
			ptr += u32Width;
		}
		for (size_t j = 0; j < (size_t)pstImg->u32Height / 2; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[1] +
						   (j * pstImg->u32Stride[1])),
				   ptr, u32Width);
			ptr += u32Width;
		}
	} else if (enType == IVE_IMAGE_TYPE_YUV422SP) {
		ptr = pBuffer;
		for (size_t j = 0; j < (size_t)u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[0] +
						   (j * pstImg->u32Stride[0])),
				   ptr, u32Width);
			ptr += u32Width;
		}
		for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[1] +
						   (j * pstImg->u32Stride[1])),
				   ptr, u32Width);
			ptr += u32Width;
		}
	} else if (enType == IVE_IMAGE_TYPE_U16C1 ||
		   enType == IVE_IMAGE_TYPE_S16C1) {
		ptr = pBuffer;
		for (size_t j = 0; j < (size_t)u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[0] +
						   (j * pstImg->u32Stride[0])),
				   ptr, u32Width * (sizeof(uint16_t)));
			ptr += u32Width * (sizeof(uint16_t));
		}
	} else {
		ptr = pBuffer;
		for (size_t j = 0; j < (size_t)u32Height; j++) {
			memcpy((char *)(uintptr_t)(pstImg->u64VirAddr[0] +
						   (j * pstImg->u32Stride[0])),
				   ptr, u32Width);
			ptr += u32Width;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_ReadRawImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg,
			  const char *filename, IVE_IMAGE_TYPE_E enType,
			  CVI_U16 u32Width, CVI_U16 u32Height)
{
	float desiredNChannels = -1;

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		desiredNChannels = 1;
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		desiredNChannels = 1.5;
		break;
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_YUV422SP:
		desiredNChannels = 2;
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		desiredNChannels = 3;
		break;
	default:
		printf("Not support channel %s.\n", cviIveImgEnTypeStr[enType]);
		return CVI_ERR_IVE_ILLEGAL_PARAM;
	}

	if (desiredNChannels > 0) {
		int buf_size = (int)((float)u32Width * (float)u32Height *
					 (float)desiredNChannels);
		char buffer[buf_size];
		FILE *fp;

		fp = fopen(filename, "r");
		int readCnt = fread(buffer, 1, buf_size, fp);

		if (readCnt == 0) {
			printf("Image %s read failed.\n", filename);
			return ERR_IVE_READ_FILE;
		}
		fclose(fp);

		CVI_IVE_ReadImageArray(pIveHandle, pstImg, buffer, enType,
					   u32Width, u32Height);
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

IVE_IMAGE_S CVI_IVE_ReadImage(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_TYPE_E enType)
{
	int desiredNChannels = -1;
	CVI_BOOL invertPackage = false;

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
		desiredNChannels = STBI_grey;
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		desiredNChannels = STBI_rgb;
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		desiredNChannels = STBI_rgb;
		break;
	default:
		printf("Not support channel %s.\n", cviIveImgEnTypeStr[enType]);
		break;
	}
	IVE_IMAGE_S img;

	memset(&img, 0, sizeof(IVE_IMAGE_S));
	if (desiredNChannels >= 0) {
		int width, height, nChannels;
		stbi_uc *stbi_data = stbi_load(filename, &width, &height, &nChannels, desiredNChannels);

		if (stbi_data == NULL) {
			printf("Image %s read failed.\n", filename);
			return img;
		}
		CVI_IVE_CreateImage(pIveHandle, &img, enType, width, height);
		//printf("desiredNChannels, width, height: %d %d %d\n", desiredNChannels, width, height);
		if (enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
			for (size_t i = 0; i < (size_t)height; i++) {
				for (size_t j = 0; j < (size_t)width; j++) {
					size_t stb_idx = (i * width + j) * 3;
					size_t img_idx = (i * img.u32Stride[0] + j);
					((CVI_U8 *)(uintptr_t)img.u64VirAddr[0])[img_idx] = stbi_data[stb_idx];
					((CVI_U8 *)(uintptr_t)img.u64VirAddr[1])[img_idx] = stbi_data[stb_idx + 1];
					((CVI_U8 *)(uintptr_t)img.u64VirAddr[2])[img_idx] = stbi_data[stb_idx + 2];
				}
			}
		} else {
			if (invertPackage && enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
				for (size_t i = 0; i < (size_t)height; i++) {
					uint32_t stb_stride = i * width * 3;
					uint32_t image_stride = (i * img.u32Stride[0]);

					for (size_t j = 0; j < (size_t)width; j++) {
						uint32_t stb_idx = stb_stride + (j * 3);
						uint32_t img_idx = image_stride + (j * 3);

						((CVI_U8 *)(uintptr_t)img.u64VirAddr[0])[img_idx] =
						stbi_data[stb_idx + 2];
						((CVI_U8 *)(uintptr_t)img.u64VirAddr[0])[img_idx + 1] =
						stbi_data[stb_idx + 1];
						((CVI_U8 *)(uintptr_t)img.u64VirAddr[0])[img_idx + 2] =
						stbi_data[stb_idx];
					}
				}
			} else {
				stbi_uc *ptr = stbi_data;

				for (size_t j = 0; j < (size_t)height; j++) {
					memcpy((void *)(uintptr_t)img.u64VirAddr[0] + (j * img.u32Stride[0]),
							ptr, width * desiredNChannels);
					ptr += width * desiredNChannels;
				}
			}
		}
		stbi_image_free(stbi_data);
	}
	return img;
}

CVI_S32 CVI_IVE_WriteImage(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_S *pstImg)
{
	UNUSED(pIveHandle);
	int desiredNChannels = -1;
	int stride = 1;
	uint8_t *arr = NULL;
	bool remove_buffer = false;

	switch (pstImg->enType) {
	case IVE_IMAGE_TYPE_U8C1:
		desiredNChannels = STBI_grey;
		arr = (CVI_U8 *)(uintptr_t)pstImg->u64VirAddr[0];
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		desiredNChannels = STBI_rgb;
		stride = 1;
		arr = (uint8_t *)calloc((size_t)pstImg->u32Stride[0] * pstImg->u32Height * desiredNChannels,
								(size_t)sizeof(uint8_t));
		size_t image_total = pstImg->u32Stride[0] * pstImg->u32Height;

		for (size_t i = 0; i < image_total; i++) {
			size_t stb_idx = i * 3;

			arr[stb_idx] = ((CVI_U8 *)(uintptr_t)pstImg->u64VirAddr[0])[i];
			arr[stb_idx + 1] = ((CVI_U8 *)(uintptr_t)pstImg->u64VirAddr[1])[i];
			arr[stb_idx + 2] = ((CVI_U8 *)(uintptr_t)pstImg->u64VirAddr[2])[i];
		}
		stride = 3;
		remove_buffer = true;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		desiredNChannels = STBI_rgb;
		arr = (CVI_U8 *)(uintptr_t)pstImg->u64VirAddr[0];
		stride = 1;
		break;
	default:
		printf("Not supported channel %s.", cviIveImgEnTypeStr[pstImg->enType]);
		return CVI_FAILURE;
	}
	stbi_write_png(filename, pstImg->u32Width, pstImg->u32Height, desiredNChannels, arr,
					pstImg->u32Stride[0] * stride);
	if (remove_buffer) {
		free(arr);
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_ResetImage(IVE_IMAGE_S *pstImage, CVI_U8 val)
{
	CVI_U32 u32Stride;
	CVI_S32 s32Succ;
	IVE_IMAGE_TYPE_E enType;
	CVI_U32 u32Width;
	CVI_U32 u32Height;
	CVI_U8 *pData;
	CVI_U16 y;

	enType = pstImage->enType;
	u32Width = pstImage->u32Width;
	u32Height = pstImage->u32Height;
	u32Stride = WidthAlign(u32Width, DEFAULT_ALIGN);
	s32Succ = CVI_SUCCESS;

	switch (enType) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1: {
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[0];
		for (y = 0; y < u32Height; y++, pData += u32Stride) {
			memset(pData, val, u32Width);
		}
	} break;
	case IVE_IMAGE_TYPE_YUV420SP: {
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[0];
		for (y = 0; y < u32Height; y++, pData += u32Stride) {
			memset(pData, val, u32Width);
		}
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[1];
		for (y = 0; y < u32Height / 2; y++, pData += u32Stride) {
			memset(pData, val, u32Width);
		}
	} break;
	case IVE_IMAGE_TYPE_YUV422SP: {
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[0];
		for (y = 0; y < u32Height; y++, pData += u32Stride) {
			memset(pData, val, u32Width);
		}
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[1];
		for (y = 0; y < u32Height; y++, pData += u32Stride) {
			memset(pData, val, u32Width);
		}
	} break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
		pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[0];
		for (y = 0; y < pstImage->u32Height;
			 y++, pData += pstImage->u32Stride[0] * 3) {
			memset(pData, val, pstImage->u32Width * 3);
		}
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		for (int i = 0; i < 3; i++) {
			pData = (CVI_U8 *)(uintptr_t)pstImage->u64VirAddr[i];
			for (y = 0; y < pstImage->u32Height;
				 y++, pData += pstImage->u32Stride[i]) {
				memset(pData, val, pstImage->u32Width);
			}
		}

	} break;
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1: {
		printf("Unsupported IMAGE_TYPE %d\n", enType);
		s32Succ = CVI_ERR_IVE_ILLEGAL_PARAM;
	} break;
	default: {
		printf("Unknown IMAGE_TYPE %d\n", enType);
		s32Succ = CVI_ERR_IVE_ILLEGAL_PARAM;
	} break;
	}

	return s32Succ;
}

CVI_S32 CVI_IVE_CreateMemInfo(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMemInfo,
				  CVI_U32 u32ByteSize)
{
	UNUSED(pIveHandle);
	int s32Ret = CVI_SYS_IonAlloc((CVI_U64 *)&pstMemInfo->u64PhyAddr,
					  (CVI_VOID **)&pstMemInfo->u64VirAddr,
					  "ive_mesh", u32ByteSize);

	memset((char *)(uintptr_t)pstMemInfo->u64VirAddr, 0, u32ByteSize);
	pstMemInfo->u32Size = u32ByteSize;
	return s32Ret;
}

CVI_S32 CVI_IVE_CreateDataInfo(IVE_HANDLE pIveHandle, IVE_DATA_S *pstDataInfo,
				   CVI_U16 u32Width, CVI_U16 u32Height)
{
	UNUSED(pIveHandle);
	pstDataInfo->u32Stride = WidthAlign(u32Width, LUMA_PHY_ALIGN);
	int s32Ret = CVI_SYS_IonAlloc((CVI_U64 *)&pstDataInfo->u64PhyAddr,
					  (CVI_VOID **)&pstDataInfo->u64VirAddr,
					  "ive_mesh",
					  pstDataInfo->u32Stride * u32Height);

	memset((char *)(uintptr_t)pstDataInfo->u64VirAddr, 0,
		   pstDataInfo->u32Stride * u32Height);
	pstDataInfo->u32Width = u32Width;
	pstDataInfo->u32Height = u32Height;
	return s32Ret;
}

CVI_S32 _CVI_IVE_CreateImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg,
				IVE_IMAGE_TYPE_E enType, uint16_t u32Width,
				uint16_t u32Height, CVI_BOOL bCached)
{
	UNUSED(pIveHandle);
	CVI_S32 s32Ret;
	CVI_U32 u32Len = 0, u32Channel = 1;
	CVI_U32 u32Coffset[3] = { 0 };

	if (u32Width == 0 || u32Height == 0) {
		printf("Image width or height cannot be 0.\n");
		pstImg->enType = enType;
		pstImg->u32Width = 0;
		pstImg->u32Height = 0;
		pstImg->u32Reserved = 0;
		for (size_t i = 0; i < 3; i++) {
			pstImg->u64VirAddr[i] = 0;
			pstImg->u64PhyAddr[i] = 0;
			pstImg->u32Stride[i] = 0;
		}
		return CVI_FAILURE;
	}

	switch (enType) {
	case IVE_IMAGE_TYPE_S8C1: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_U8C1: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_YUV420SP: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Coffset[1] = u32Len;
		pstImg->u32Stride[1] = WidthAlign(u32Width, DEFAULT_ALIGN);
		u32Len += pstImg->u32Stride[0] * u32Height >> 1;
		u32Channel = 2;
	} break;
	case IVE_IMAGE_TYPE_YUV420P: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Coffset[1] = u32Len;
		pstImg->u32Stride[1] =
			WidthAlign(u32Width >> 1, DEFAULT_ALIGN);
		pstImg->u32Stride[2] = pstImg->u32Stride[1];
		u32Len += pstImg->u32Stride[1] * u32Height >> 1;
		u32Coffset[2] = u32Len;
		u32Len += pstImg->u32Stride[1] * u32Height >> 1;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_YUV422SP: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		pstImg->u32Stride[1] = pstImg->u32Stride[0];
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Coffset[1] = u32Len;
		u32Len += pstImg->u32Stride[0] * u32Height;
		u32Channel = 2;
	} break;
	case IVE_IMAGE_TYPE_YUV422P: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		pstImg->u32Stride[1] =
			WidthAlign(u32Width >> 1, DEFAULT_ALIGN);
		pstImg->u32Stride[2] = pstImg->u32Stride[1];
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Coffset[1] = u32Len;
		u32Len += pstImg->u32Stride[1] * u32Height;
		u32Coffset[2] = u32Len;
		u32Len += pstImg->u32Stride[1] * u32Height;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
		pstImg->u32Stride[0] = WidthAlign(u32Width * 3, DEFAULT_ALIGN);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		pstImg->u32Stride[0] = WidthAlign(u32Width, DEFAULT_ALIGN);
		pstImg->u32Stride[1] = pstImg->u32Stride[0];
		pstImg->u32Stride[2] = pstImg->u32Stride[0];
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Coffset[1] = u32Len;
		u32Len += pstImg->u32Stride[0] * u32Height;
		u32Coffset[2] = u32Len;
		u32Len += pstImg->u32Stride[0] * u32Height;
		u32Channel = 3;
	} break;
	case IVE_IMAGE_TYPE_BF16C1: {
		pstImg->u32Stride[0] =
			WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(int16_t);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_U16C1: {
		pstImg->u32Stride[0] =
			WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_S16C1: {
		pstImg->u32Stride[0] =
			WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_U32C1: {
		pstImg->u32Stride[0] =
			WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint32_t);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_FP32C1: {
		pstImg->u32Stride[0] =
			WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(float);
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	default:
		printf("Not supported enType %s.\n",
			   cviIveImgEnTypeStr[enType]);
		return CVI_ERR_IVE_ILLEGAL_PARAM;
		break;
	}
	pstImg->enType = enType;
	if (pstImg->u32Stride[0] == 0 || u32Len == 0) {
		printf("[DEV] Stride not set.\n");
		return CVI_ERR_IVE_ILLEGAL_PARAM;
	}
	pstImg->u32Width = u32Width;
	pstImg->u32Height = u32Height;
	if (bCached) {
		s32Ret = CVI_SYS_IonAlloc_Cached(&pstImg->u64PhyAddr[0],
				  (CVI_VOID **)&pstImg->u64VirAddr[0],
				  "ive_mesh", u32Len);
	} else {
		s32Ret = CVI_SYS_IonAlloc(&pstImg->u64PhyAddr[0],
				  (CVI_VOID **)&pstImg->u64VirAddr[0],
				  "ive_mesh", u32Len);
	}
	memset((char *)(uintptr_t)pstImg->u64VirAddr[0], 0, u32Len);
	if (s32Ret) {
		printf("CVI_SYS_IonAlloc failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	for (size_t i = 1; i < u32Channel; i++) {
		pstImg->u64VirAddr[i] = pstImg->u64VirAddr[0] + u32Coffset[i];
		pstImg->u64PhyAddr[i] = pstImg->u64PhyAddr[0] + u32Coffset[i];
	}

	for (size_t i = u32Channel; i < 3; i++) {
		pstImg->u64VirAddr[i] = 0;
		pstImg->u64PhyAddr[i] = -1;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CreateImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg,
				IVE_IMAGE_TYPE_E enType, uint16_t u32Width,
				uint16_t u32Height)
{
	return _CVI_IVE_CreateImage(pIveHandle, pstImg, enType, u32Width, u32Height, false);
}

CVI_S32 CVI_IVE_CreateImage_Cached(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg,
				IVE_IMAGE_TYPE_E enType, uint16_t u32Width,
				uint16_t u32Height)
{
	return _CVI_IVE_CreateImage(pIveHandle, pstImg, enType, u32Width, u32Height, true);
}

CVI_S32 _CVI_IVE_BufFlush_Request(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, CVI_BOOL bFlush)
{
	UNUSED(pIveHandle);
	CVI_U32 u32Len = 0;
	CVI_U32 u32Height = pstImg->u32Height;

	switch (pstImg->enType) {
	case IVE_IMAGE_TYPE_S8C1:
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
	case IVE_IMAGE_TYPE_BF16C1:
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U32C1:
	case IVE_IMAGE_TYPE_FP32C1: {
		u32Len = pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_YUV420SP: {
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[0] * u32Height >> 1;
	} break;
	case IVE_IMAGE_TYPE_YUV420P: {
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[1] * u32Height >> 1;
		u32Len += pstImg->u32Stride[1] * u32Height >> 1;
	} break;
	case IVE_IMAGE_TYPE_YUV422SP: {
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[0] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_YUV422P: {
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[1] * u32Height;
		u32Len += pstImg->u32Stride[1] * u32Height;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		u32Len = pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[0] * u32Height;
		u32Len += pstImg->u32Stride[0] * u32Height;
	} break;
	default:
		printf("Not supported enType %s.\n",
			   cviIveImgEnTypeStr[pstImg->enType]);
		return CVI_ERR_IVE_ILLEGAL_PARAM;
	}
	if (u32Len == 0) {
		printf("[DEV] BufFlush Stride not set.\n");
		return CVI_ERR_IVE_ILLEGAL_PARAM;
	}
	if (bFlush)
		return CVI_SYS_IonFlushCache(pstImg->u64PhyAddr[0], (char *)(uintptr_t)pstImg->u64VirAddr[0], u32Len);
	return CVI_SYS_IonInvalidateCache(pstImg->u64PhyAddr[0], (char *)(uintptr_t)pstImg->u64VirAddr[0], u32Len);
}

CVI_S32 CVI_IVE_BufFlush(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg)
{
	return _CVI_IVE_BufFlush_Request(pIveHandle, pstImg, true);
}

CVI_S32 CVI_IVE_BufRequest(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg)
{
	return _CVI_IVE_BufFlush_Request(pIveHandle, pstImg, false);
}

CVI_S32 _CVI_IVE_Write(const char *filename, CVI_U64 u64VirAddr, CVI_U32 len)
{
	FILE *fp;
	int readCnt = 0;

	fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Can't open %s\n", filename);
		return ERR_IVE_OPEN_FILE;
	}
	while (len) {
		readCnt = fwrite((char *)(uintptr_t)(u64VirAddr + readCnt), 1,
				 len, fp);
		len -= readCnt;
		if (readCnt == 0) {
			printf("Image %s write failed.\n", filename);
			return ERR_IVE_WRITE_FILE;
		}
	}

	fclose(fp);
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_WriteData(IVE_HANDLE pIveHandle, const char *filename,
			  IVE_DATA_S *pstData)
{
	UNUSED(pIveHandle);
	return _CVI_IVE_Write(filename, pstData->u64VirAddr,
				  pstData->u32Stride * pstData->u32Height);
}

CVI_S32 CVI_IVE_WriteMem(IVE_HANDLE pIveHandle, const char *filename,
			 IVE_MEM_INFO_S *pstMem)
{
	UNUSED(pIveHandle);
	return _CVI_IVE_Write(filename, pstMem->u64VirAddr, pstMem->u32Size);
}

CVI_S32 CVI_IVE_WriteImg(IVE_HANDLE pIveHandle, const char *filename,
			 IVE_IMAGE_S *pstImg)
{
	UNUSED(pIveHandle);
	float desiredNChannels = 1.0;

	switch (pstImg->enType) {
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
	case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
		desiredNChannels = 1;
	} break;
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV420SP: {
		desiredNChannels = 1.5;
	} break;
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_YUV422SP: {
		desiredNChannels = 2;
	} break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR: {
		desiredNChannels = 3;
	} break;
	default: {
		printf("Unsupported conversion type: %u.\n", pstImg->enType);
		return CVI_ERR_IVE_ILLEGAL_PARAM;
	} break;
	}
	int len = (int)((float)pstImg->u32Stride[0] * (float)pstImg->u32Height *
			desiredNChannels);

	return _CVI_IVE_Write(filename, pstImg->u64VirAddr[0], len);
}

CVI_S32 CVI_SYS_FreeM(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMem)
{
	UNUSED(pIveHandle);
	return CVI_SYS_IonFree(pstMem->u64PhyAddr,
				   (char *)(uintptr_t)pstMem->u64VirAddr);
}

CVI_S32 CVI_SYS_FreeI(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg)
{
	UNUSED(pIveHandle);
	return CVI_SYS_IonFree(pstImg->u64PhyAddr[0],
				   (char *)(uintptr_t)pstImg->u64VirAddr[0]);
}

CVI_S32 CVI_SYS_FreeD(IVE_HANDLE pIveHandle, IVE_DATA_S *pstData)
{
	UNUSED(pIveHandle);
	return CVI_SYS_IonFree(pstData->u64PhyAddr,
				   (char *)(uintptr_t)pstData->u64VirAddr);
}

CVI_S32 CVI_IVE_DiffFg_Split(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstDiffFg,
				 IVE_DST_IMAGE_S *pstBgDiffFg,
				 IVE_DST_IMAGE_S *pstFrmDiffFg)
{
	UNUSED(pIveHandle);
	int i = 0;
	int size = (pstDiffFg->u32Stride[0] / sizeof(uint16_t)) *
		   pstDiffFg->u32Height;

	for (i = 0; i < size; i++) {
		((char *)(uintptr_t)pstBgDiffFg->u64VirAddr[0])[i] =
			((char *)(uintptr_t)pstDiffFg->u64VirAddr[0])[i * 2];
		((char *)(uintptr_t)pstFrmDiffFg->u64VirAddr[0])[i] =
			((char *)(uintptr_t)pstDiffFg->u64VirAddr[0])[i * 2 + 1];
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_ChgSta_Split(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstChgSta,
				 IVE_DST_IMAGE_S *pstChgStaImg,
				 IVE_DST_IMAGE_S *pstChgStaFg,
				 IVE_DST_IMAGE_S *pstChStaLift)
{
	UNUSED(pIveHandle);
	int i = 0;
	int size = (pstChgSta->u32Stride[0] / sizeof(uint32_t)) *
		   pstChgSta->u32Height;

	for (i = 0; i < size; i++) {
		((char *)(uintptr_t)pstChgStaImg->u64VirAddr[0])[i] =
			((char *)(uintptr_t)pstChgSta->u64VirAddr[0])[i * 4];//0
		((char *)(uintptr_t)pstChgStaFg->u64VirAddr[0])[i] =
			((char *)(uintptr_t)pstChgSta->u64VirAddr[0])[i * 4 + 1];//1
		((char *)(uintptr_t)pstChStaLift->u64VirAddr[0])[i * 2] =
			((char *)(uintptr_t)pstChgSta->u64VirAddr[0])[i * 4 + 2];//2
		((char *)(uintptr_t)pstChStaLift->u64VirAddr[0])[i * 2 + 1] =
			((char *)(uintptr_t)pstChgSta->u64VirAddr[0])[i * 4 + 3];//3
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_DUMP(IVE_HANDLE pIveHandle)
{
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ioctl(p->devfd, CVI_IVE_IOC_DUMP);
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_RESET(IVE_HANDLE pIveHandle, int s)
{
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ioctl(p->devfd, CVI_IVE_IOC_RESET, &s);
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_QUERY(IVE_HANDLE pIveHandle, CVI_BOOL *pbFinish,
			  CVI_BOOL bBlock)
{
	struct cvi_ive_query_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.pbFinish = pbFinish;
	ive_arg.bBlock = bBlock;
	ioctl(p->devfd, CVI_IVE_IOC_QUERY, &ive_arg);
	return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_DMA(IVE_HANDLE pIveHandle, IVE_DATA_S *pstSrc,
			IVE_DST_DATA_S *pstDst, IVE_DMA_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_dma_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_DMA, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_And(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_and_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_And, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Or(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
		   IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
		   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_or_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Or, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Xor(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_xor_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Xor, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Add(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_ADD_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_add_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.pstCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	if (ioctl(p->devfd, CVI_IVE_IOC_Add, &ive_arg) < 0) {
		fprintf(stderr, "SYS_IOC_S_CTRL - %s NG\n", __func__);
		return -1;
	}
	return 0;
}

CVI_S32 CVI_IVE_Sub(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst,
			IVE_SUB_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_sub_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Sub, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Erode(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDst, IVE_ERODE_CTRL_S *pstCtrl,
			  CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_erode_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Erode, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Dilate(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, IVE_DILATE_CTRL_S *pstctrl,
			   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_dilate_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstctrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Dilate, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Thresh(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, IVE_THRESH_CTRL_S *pstCtrl,
			   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_thresh_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Thresh, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_MatchBgModel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstCurImg,
				 IVE_DATA_S *pstBgModel, IVE_IMAGE_S *pstFgFlag,
				 IVE_DST_IMAGE_S *pstDiffFg,
				 IVE_DST_MEM_INFO_S *pstStatData,
				 IVE_MATCH_BG_MODEL_CTRL_S *pstCtrl,
				 CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_match_bgmodel_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stCurImg = *pstCurImg;
	ive_arg.stBgModel = *pstBgModel;
	ive_arg.stFgFlag = *pstFgFlag;
	ive_arg.stDiffFg = *pstDiffFg;
	ive_arg.stStatData = *pstStatData;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_MatchBgModel, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_UpdateBgModel(IVE_HANDLE pIveHandle, IVE_DATA_S *pstBgModel,
				  IVE_IMAGE_S *pstFgFlag, IVE_DST_IMAGE_S *pstBgImg,
				  IVE_DST_IMAGE_S *pstChgSta,
				  IVE_DST_MEM_INFO_S *pstStatData,
				  IVE_UPDATE_BG_MODEL_CTRL_S *pstCtrl,
				  CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_update_bgmodel_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stBgModel = *pstBgModel;
	ive_arg.stFgFlag = *pstFgFlag;
	ive_arg.stBgImg = *pstBgImg;
	ive_arg.stChgSta = *pstChgSta;
	ive_arg.stStatData = *pstStatData;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_UpdateBgModel, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_GMM(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstFg, IVE_DST_IMAGE_S *pstBg,
			IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_gmm_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stFg = *pstFg;
	ive_arg.stBg = *pstBg;
	ive_arg.stModel = *pstModel;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_GMM, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_GMM2(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_SRC_IMAGE_S *pstFactor, IVE_DST_IMAGE_S *pstFg,
			 IVE_DST_IMAGE_S *pstBg, IVE_DST_IMAGE_S *pstMatchModelInfo,
			 IVE_MEM_INFO_S *pstModel, IVE_GMM2_CTRL_S *pstCtrl,
			 CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_gmm2_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stFactor = *pstFactor;
	ive_arg.stFg = *pstFg;
	ive_arg.stBg = *pstBg;
	ive_arg.stInfo = *pstMatchModelInfo;
	ive_arg.stModel = *pstModel;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_GMM2, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Bernsen(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_BERNSEN_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_bernsen_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Bernsen, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Filter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstCtrl,
			   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_filter_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Filter, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Sobel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			  IVE_SOBEL_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_sobel_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	if (pstDstH != CVI_NULL)
		ive_arg.stDstH = *pstDstH;
	if (pstDstV != CVI_NULL)
		ive_arg.stDstV = *pstDstV;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Sobel, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_MagAndAng(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_IMAGE_S *pstDstMag,
			  IVE_DST_IMAGE_S *pstDstAng,
			  IVE_MAG_AND_ANG_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_maganang_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	if (pstDstMag != CVI_NULL)
		ive_arg.stDstMag = *pstDstMag;
	if (pstDstAng != CVI_NULL)
		ive_arg.stDstAng = *pstDstAng;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_MagAndAng, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_CSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_CSC_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_csc_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_CSC, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_FilterAndCSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				 IVE_DST_IMAGE_S *pstDst,
				 IVE_FILTER_AND_CSC_CTRL_S *pstCtrl,
				 CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_filter_and_csc_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_FilterAndCSC, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Hist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_MEM_INFO_S *pstDst, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_hist_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Hist, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Map(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_SRC_MEM_INFO_S *pstMap, IVE_DST_IMAGE_S *pstDst,
			IVE_MAP_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_map_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stMap = *pstMap;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Map, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_NCC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_MEM_INFO_S *pstDst,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_ncc_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_NCC, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Integ(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			  IVE_DST_MEM_INFO_S *pstDst, IVE_INTEG_CTRL_S *pstCtrl,
			  CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_integ_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Integ, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_LBP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			IVE_DST_IMAGE_S *pstDst, IVE_LBP_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_lbp_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = p;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;
	ioctl(p->devfd, CVI_IVE_IOC_LBP, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Thresh_S16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_S16_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_thresh_s16_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Thresh_S16, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Thresh_U16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			   IVE_DST_IMAGE_S *pstDst,
			   IVE_THRESH_U16_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_thres_su16_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Thresh_U16, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_16BitTo8Bit(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				IVE_DST_IMAGE_S *pstDst,
				IVE_16BIT_TO_8BIT_CTRL_S *pstCtrl,
				CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_16bit_to_8bit_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_16BitTo8Bit, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_OrdStatFilter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				  IVE_DST_IMAGE_S *pstDst,
				  IVE_ORD_STAT_FILTER_CTRL_S *pstCtrl,
				  CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_ord_stat_filter_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_OrdStatFilter, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_CannyEdge(IVE_IMAGE_S *pstEdge, IVE_MEM_INFO_S *pstStack)
{
	UNUSED(pstStack);
	do_hysteresis_wo_ang((unsigned char *)(uintptr_t)pstEdge->u64VirAddr[0],
				 (unsigned char *)(uintptr_t)pstEdge->u64VirAddr[0],
				 pstEdge->u32Width, pstEdge->u32Height, pstEdge->u32Stride[0]);

	return 0;
}

CVI_S32 CVI_IVE_CannyHysEdge(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				 IVE_DST_IMAGE_S *pstEdge,
				 IVE_DST_MEM_INFO_S *pstStack,
				 IVE_CANNY_HYS_EDGE_CTRL_S *pstCtrl,
				 CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_canny_hys_edge_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstEdge;
	ive_arg.stStack = *pstStack;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_CannyHysEdge, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_NormGrad(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
			 IVE_DST_IMAGE_S *pstDstH, IVE_DST_IMAGE_S *pstDstV,
			 IVE_DST_IMAGE_S *pstDstHV,
			 IVE_NORM_GRAD_CTRL_S *pstCtrl, CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_norm_grad_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	if (pstDstH != CVI_NULL)
		ive_arg.stDstH = *pstDstH;
	if (pstDstV != CVI_NULL)
		ive_arg.stDstV = *pstDstV;
	if (pstDstHV != CVI_NULL)
		ive_arg.stDstHV = *pstDstHV;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_NormGrad, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_GradFg(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstBgDiffFg,
			   IVE_SRC_IMAGE_S *pstCurGrad, IVE_SRC_IMAGE_S *pstBgGrad,
			   IVE_DST_IMAGE_S *pstGradFg, IVE_GRAD_FG_CTRL_S *pstCtrl,
			   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_grad_fg_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	if (pstBgDiffFg != CVI_NULL)
		ive_arg.stBgDiffFg = *pstBgDiffFg;
	if (pstCurGrad != CVI_NULL)
		ive_arg.stCurGrad = *pstCurGrad;
	ive_arg.stBgGrad = *pstBgGrad;
	ive_arg.stGradFg = *pstGradFg;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_GradFg, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_SAD(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
			IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstSad,
			IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstCtrl,
			CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_sad_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stSad = *pstSad;
	ive_arg.stThr = *pstThr;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_SAD, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_Resize(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S astSrc[],
			   IVE_DST_IMAGE_S astDst[], IVE_RESIZE_CTRL_S *pstCtrl,
			   CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_resize_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.astSrc = astSrc;
	ive_arg.astDst = astDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_Resize, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_imgInToOdma(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				IVE_DST_IMAGE_S *pstDst, IVE_FILTER_CTRL_S *pstCtrl,
				CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_filter_arg ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_imgInToOdma, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_rgbPToYuvToErodeToDilate(IVE_HANDLE pIveHandle,
					 IVE_SRC_IMAGE_S *pstSrc,
					 IVE_DST_IMAGE_S *pstDst1,
					 IVE_DST_IMAGE_S *pstDst2,
					 IVE_FILTER_CTRL_S *pstCtrl,
					 CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_rgbPToYuvToErodeToDilate ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst1 = *pstDst1;
	ive_arg.stDst2 = *pstDst2;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_rgbPToYuvToErodeToDilate, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_STCandiCorner(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
				  IVE_DST_IMAGE_S *pstDst,
				  IVE_ST_CANDI_CORNER_CTRL_S *pstCtrl,
				  CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_stcandicorner ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc = *pstSrc;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_STCandiCorner, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_FrameDiffMotion(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
				IVE_SRC_IMAGE_S *pstSrc2,
				IVE_DST_IMAGE_S *pstDst,
				IVE_FRAME_DIFF_MOTION_CTRL_S *pstCtrl,
				CVI_BOOL bInstant)
{
	struct cvi_ive_ioctl_md ive_arg;
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}
	ive_arg.pIveHandle = pIveHandle;
	ive_arg.stSrc1 = *pstSrc1;
	ive_arg.stSrc2 = *pstSrc2;
	ive_arg.stDst = *pstDst;
	ive_arg.stCtrl = *pstCtrl;
	ive_arg.bInstant = bInstant;

	ioctl(p->devfd, CVI_IVE_IOC_MD, &ive_arg);
	return 0;
}

CVI_S32 CVI_IVE_CMDQ(IVE_HANDLE pIveHandle)
{
	struct IVE_HANDLE_CTX *p = (struct IVE_HANDLE_CTX *)pIveHandle;

	if (p->devfd <= 0) {
		printf("Device ive is not open, please check it\n");
		return CVI_ERR_IVE_INVALID_DEVID;
	}

	ioctl(p->devfd, CVI_IVE_IOC_CMDQ, NULL);
	return 0;
}
