#include "bitmap.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

size_t bitmap_load(const char* file, BITMAPINFOHEADER* bi, void* data, size_t bytes)
{
	size_t r;
	FILE* fp;
	BITMAPFILEHEADER h;

	fp = fopen(file, "rb");
	if (!fp)
		return 0;

	memset(&h, 0, sizeof(h));
	if (sizeof(h) != fread(&h, 1, sizeof(h), fp))
	{
		r = 0;
		goto CLEANUP;
	}

	r = h.bfSize - h.bfOffBits;
	if (bytes < r)
	{
		r = 0;
		goto CLEANUP;
	}

	assert(bi->biSize == sizeof(BITMAPINFOHEADER));
	if (bi->biSize != fread(bi, 1, bi->biSize, fp))
	{
		r = 0;
		goto CLEANUP;
	}

	assert((long)h.bfOffBits == ftell(fp));
	if (r != fread(data, 1, h.bfSize - h.bfOffBits, fp))
	{
		r = 0;
		goto CLEANUP;
	}

CLEANUP:
	fclose(fp);
	return r;
}

int bitmap_save(const char* file, const BITMAPINFOHEADER* bi, const void* data)
{
	FILE* fp;
	BITMAPFILEHEADER h;

	fp = fopen(file, "wb");
	if (!fp)
		return -errno;

	memset(&h, 0, sizeof(h));
	h.bfType = 0x4D42; // BM
	h.bfSize = (bi->biWidth * abs(bi->biHeight) * bi->biBitCount + 7) / 8 + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	h.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	fwrite(&h, 1, sizeof(h), fp);
	fwrite(bi, 1, bi->biSize, fp);
	fwrite(data, 1, h.bfSize - h.bfOffBits, fp);

	fclose(fp);
	return 0;
}
