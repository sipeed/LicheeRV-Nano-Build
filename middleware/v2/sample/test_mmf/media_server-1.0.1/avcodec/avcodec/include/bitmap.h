#ifndef _bitmap_h_
#define _bitmap_h_

#if defined(OS_WINDOWS)
#include <Windows.h>
#else
#include <stdint.h>

#pragma pack(push)
#pragma pack(2)

typedef struct tagBITMAPFILEHEADER {
	uint16_t    bfType;
	uint32_t	bfSize;
	uint16_t    bfReserved1;
	uint16_t    bfReserved2;
	uint32_t	bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	uint32_t      biSize;
	int32_t       biWidth;
	int32_t       biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	int32_t       biXPelsPerMeter;
	int32_t       biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;
} BITMAPINFOHEADER;

#pragma pack(pop)
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @param[out] bi bi->biSize MUST be set
/// @return >0-rgb size, =0-error
size_t bitmap_load(const char* file, BITMAPINFOHEADER* bi, void* data, size_t bytes);

/// @return 0-ok, other-error
int bitmap_save(const char* file, const BITMAPINFOHEADER* bi, const void* data);

#ifdef __cplusplus
}
#endif
#endif /* !_bitmap_h_ */
