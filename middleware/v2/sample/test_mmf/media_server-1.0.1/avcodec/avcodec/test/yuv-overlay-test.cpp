#include "yuv-overlay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void yuv_overlay_test(const char* output, const char* input, const char* overlay)
{
	static uint8_t s_yuv1080p[1920 * 1080 * 3];
	static uint8_t s_yuv720p[1280 * 720 * 3];

	overlay_t param;
	memset(&param, 0, sizeof(param));
	param.x = 500;
	param.y = 150;
	param.alpha = 100;

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
	dst.width = 1920;
	dst.height = 1080;
	dst.data[0] = s_yuv1080p;
	dst.data[1] = dst.data[0] + dst.width * dst.height;
	dst.data[2] = dst.data[1] + dst.width * dst.height / 4;
	dst.linesize[0] = dst.width;
	dst.linesize[1] = dst.linesize[2] = dst.width / 2;

	FILE* fp = fopen(output, "wb");
	FILE* dfp = fopen(input, "rb");
	for (int i = 0; i < 100; i++)
	{
		snprintf((char*)s_yuv720p, sizeof(s_yuv720p), "%s%03d.yuv", overlay, i);
		FILE* sfp = fopen((char*)s_yuv720p, "rb");
		int n = fread(s_yuv720p, 1, sizeof(s_yuv720p), sfp);
		assert(n == src.width * src.height * 3 / 2);
		fclose(sfp);

		if ((int)fread(s_yuv1080p, 1, dst.width * dst.height * 3 / 2, dfp) != dst.width * dst.height * 3 / 2)
			break;

		yuv_overlay(&dst, &src, &param);
		fwrite(s_yuv1080p, 1, dst.width * dst.height * 3 / 2, fp);
	}
	fclose(dfp);
	fclose(fp);
}
