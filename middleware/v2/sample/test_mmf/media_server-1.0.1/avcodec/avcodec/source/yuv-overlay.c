#include "yuv-overlay.h"
#include "colorspace.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// divide by 255 and round to nearest
// apply a fast variant: (X+127)/255 = ((X+127)*257+257)>>16 = ((X+128)*257)>>16
#define FAST_DIV255(x) ((((x) + 128) * 257) >> 16)

// calculate the unpremultiplied alpha, applying the general equation:
// alpha = alpha_overlay / ( (alpha_main + alpha_overlay) - (alpha_main * alpha_overlay) )
// (((x) << 16) - ((x) << 9) + (x)) is a faster version of: 255 * 255 * x
// ((((x) + (y)) << 8) - ((x) + (y)) - (y) * (x)) is a faster version of: 255 * (x + y)
#define UNPREMULTIPLY_ALPHA(x, y) ((((x) << 16) - ((x) << 9) + (x)) / ((((x) + (y)) << 8) - ((x) + (y)) - (y) * (x)))


// /ffmpeg/libavfilter/vf_overlay.c:blend_image
static void yuv_blend(struct avframe_t* yv12, const struct avframe_t* pic, const overlay_t* overlay)
{
	int hsample, vsample;
	int planar, row, col, rowmax, colmax;
	int x, y, srcw, srch, dstw, dsth;
	const uint8_t* sp, *ap;
	uint8_t* dp, alpha;

	for (planar = 0; planar < 3; planar++)
	{
		hsample = planar ? 1 : 0; // YUV420
		vsample = planar ? 1 : 0; // YUV420
		x = overlay->x >> hsample;
		y = overlay->y >> vsample;
		srcw = pic->width >> hsample;
		srch = pic->height >> vsample;
		dstw = yv12->width >> hsample;
		dsth = yv12->height >> vsample;

		row = MAX(-y, 0);
		ap = pic->data[3] + (row << vsample) * pic->linesize[3];
		sp = pic->data[planar] + row * pic->linesize[planar];
		dp = yv12->data[planar] + (y + row) * yv12->linesize[planar];

		for (rowmax = MIN(-y + dsth, srch); row < rowmax; row++)
		{
			col = MAX(-x, 0);
			for (colmax = MIN(-x + dstw, srcw); col < colmax; col++)
			{
				//alpha = overlay->src_alpha ? ap[col << samples[planar]] : overlay->alpha;
				if (overlay->src_alpha)
				{
					const uint8_t* a = &ap[col * (1 << hsample)];
					if (hsample && vsample && col + 1 < colmax && row + 1 < rowmax)
						alpha = (a[0] + a[1] + a[pic->linesize[3]] + a[pic->linesize[3] + 1]) >> 2; // (2-hsample + 2-vsample)
					else if (hsample || vsample)
						alpha = (((hsample && col + 1 < colmax) ? ((a[0] + a[1]) >> 1) : a[0]) + ((vsample && row + 1 < rowmax) ? ((a[0] + a[pic->linesize[3]]) >> 1) : a[0])) >> 1;
					else
						alpha = a[0];
				}
				else
				{
					alpha = overlay->alpha;
				}

				dp[col + x] = (uint8_t)FAST_DIV255(dp[col + x] * alpha + sp[col] * (255 - alpha));
			}

			ap += (1 << vsample) * pic->linesize[3];
			sp += pic->linesize[planar];
			dp += yv12->linesize[planar];
		}
	}
}

static int yv12_overlay_rgb(struct avframe_t* yv12, const struct avframe_t* pic, const overlay_t* overlay)
{
	struct avframe_t src;
	memcpy(&src, pic, sizeof(struct avframe_t));
	src.data[0] = (uint8_t*)malloc(pic->width * pic->height * 3 / 2);
	if (NULL == src.data[0])
		return -ENOMEM;
	src.data[1] = src.data[0] + pic->width * pic->height;
	src.data[2] = src.data[1] + pic->width * pic->height / 4;
	src.linesize[0] = pic->width;
	src.linesize[1] = src.linesize[2] = pic->width / 2;
	src.format = PICTURE_YUV420;

	rgb24_yv12(pic->data[0], pic->width, pic->height, pic->linesize[0], src.data[0], src.data[1], src.data[2]);

	yuv_blend(yv12, &src, overlay);

	free(src.data[0]);
	return 0;
}

static void rgba_alpha(const uint8_t* rgba, int width, int height, int stride, uint8_t* alpha, int linesize)
{
	int i, j;
	uint8_t* d;
	const uint8_t* s;
	for (i = 0; i < height; i++)
	{
		s = rgba + i * stride;
		d = alpha + i * linesize;

		for (j = 0; j < width; j++)
			d[j] = 255 - s[3 + 4 * j];
	}
}

static int yv12_overlay_rgba(struct avframe_t* yv12, const struct avframe_t* pic, const overlay_t* overlay)
{
	struct avframe_t src;
	overlay_t param;
	memcpy(&src, pic, sizeof(struct avframe_t));
	src.data[0] = (uint8_t*)malloc(pic->width * pic->height * 5 / 2);
	if (NULL == src.data[0])
		return -ENOMEM;

	src.data[1] = src.data[0] + pic->width * pic->height;
	src.data[2] = src.data[1] + pic->width * pic->height / 4;
	src.data[3] = src.data[2] + pic->width * pic->height / 4;
	src.linesize[0] = src.linesize[3] = pic->width;
	src.linesize[1] = src.linesize[2] = pic->width / 2;
	src.format = PICTURE_YUV420;

	rgb32_yv12(pic->data[0], pic->width, pic->height, pic->linesize[0], src.data[0], src.data[1], src.data[2]);
	rgba_alpha(pic->data[0], pic->width, pic->height, pic->linesize[0], src.data[3], src.linesize[3]);

	memcpy(&param, overlay, sizeof(overlay_t));
	param.src_alpha = 1;
	yuv_blend(yv12, &src, &param);

	free(src.data[0]);
	return 0;
}

static int yuv_overlay2(struct avframe_t* yv12, const struct avframe_t* pic, const overlay_t* overlay)
{
	switch (pic->format)
	{
	case PICTURE_YUV420:
		yuv_blend(yv12, pic, overlay);
		return 0;

	case PICTURE_RGB24:
		return yv12_overlay_rgb(yv12, pic, overlay);

	case PICTURE_RGBA:
		return yv12_overlay_rgba(yv12, pic, overlay);

	default:
		return -1;
	}
}

int yuv_overlay(struct avframe_t* dst, const struct avframe_t* src, const overlay_t* overlay)
{
	assert(overlay->alpha >= 0 && overlay->alpha <= 255);
	switch (dst->format)
	{
	case PICTURE_YUV420:
	case PICTURE_YUV422:
	case PICTURE_YUV444:
		return yuv_overlay2(dst, src, overlay);

	default:
		printf("%s: unknown picture format %d\n", __FUNCTION__, dst->format);
		return -1;
	}
}
