#include "avframe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern "C" void interpolation_bilinear(struct avframe_t* dst, const struct avframe_t* src);

static inline void interpolation_test1()
{
	static uint8_t s_yuv1080p[1920 * 1080 * 3];
	static uint8_t s_yuv720p[1280 * 720 * 3];

	struct avframe_t src;
	memset(&src, 0, sizeof(struct avframe_t));
	src.format = PICTURE_YUV420;
	src.width = 1920;
	src.height = 1080;
	src.data[0] = s_yuv1080p;
	src.data[1] = src.data[0] + src.width * src.height;
	src.data[2] = src.data[1] + src.width * src.height / 4;
	src.linesize[0] = src.width;
	src.linesize[1] = src.linesize[2] = src.width / 2;

	struct avframe_t dst;
	memset(&dst, 0, sizeof(struct avframe_t));
	dst.format = PICTURE_YUV420;
	dst.width = 960;
	dst.height = 540;
	dst.data[0] = s_yuv720p;
	dst.data[1] = dst.data[0] + dst.width * dst.height;
	dst.data[2] = dst.data[1] + dst.width * dst.height / 4;
	dst.linesize[0] = dst.width;
	dst.linesize[1] = dst.linesize[2] = dst.width / 2;

	FILE* fp = fopen("E:\\video\\yuv-rgb\\1.yuv", "wb");
	FILE* dfp = fopen("E:\\video\\yuv-rgb\\jiangsu-YV12-1920x1080.yuv", "rb");
	for (int i = 0; i < 100; i++)
	{
		if ((int)fread(src.data[0], 1, src.width * src.height * 3 / 2, dfp) != src.width * src.height * 3 / 2)
			break;

		interpolation_bilinear(&dst, &src);
		fwrite(dst.data[0], 1, dst.width * dst.height * 3 / 2, fp);
	}
	fclose(dfp);
	fclose(fp);
}

static inline void interpolation_test2()
{
	static uint8_t s_yuv1080p[1920 * 1080 * 3];
	static uint8_t s_yuv720p[1280 * 720 * 3];

	struct avframe_t src;
	memset(&src, 0, sizeof(struct avframe_t));
	src.format = PICTURE_YUV420;
	src.width = 352;
	src.height = 288;
	src.data[0] = s_yuv720p;
	src.data[1] = src.data[0] + src.width * src.height;
	src.data[2] = src.data[1] + src.width * src.height / 4;
	src.linesize[0] = src.width;
	src.linesize[1] = src.linesize[2] = src.width / 2;

	struct avframe_t dst;
	memset(&dst, 0, sizeof(struct avframe_t));
	dst.format = PICTURE_YUV420;
	dst.width = 704;
	dst.height = 576;
	dst.data[0] = s_yuv1080p;
	dst.data[1] = dst.data[0] + dst.width * dst.height;
	dst.data[2] = dst.data[1] + dst.width * dst.height / 4;
	dst.linesize[0] = dst.width;
	dst.linesize[1] = dst.linesize[2] = dst.width / 2;

	FILE* fp = fopen("E:\\video\\yuv-rgb\\2.yuv", "wb");
	for (int i = 0; i < 100; i++)
	{
		snprintf((char*)s_yuv720p, sizeof(s_yuv720p), "E:\\video\\yuv-rgb\\foreman-YV12-352x288\\foreman%03d.yuv", i);
		FILE* sfp = fopen((char*)s_yuv720p, "rb");
		int n = fread(src.data[0], 1, src.width * src.height * 3 / 2, sfp);
		fclose(sfp);
		assert(n == src.width * src.height * 3 / 2);
		if (n != src.width * src.height * 3 / 2)
			break;

		interpolation_bilinear(&dst, &src);
		fwrite(dst.data[0], 1, dst.width * dst.height * 3 / 2, fp);
	}
	fclose(fp);
}

void interpolation_test()
{
	//interpolation_test1();
	interpolation_test2();
}
