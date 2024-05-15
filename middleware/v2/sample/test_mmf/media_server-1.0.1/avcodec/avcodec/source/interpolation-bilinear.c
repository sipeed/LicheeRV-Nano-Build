// https://en.wikipedia.org/wiki/Bilinear_interpolation
// http://www.cnblogs.com/acloud/archive/2011/10/29/sws_scale.html

#include "avframe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void yuv_interpolation_bilinear(struct avframe_t* dst, const struct avframe_t* src)
{
	uint8_t v[2][2], *dp;
	int row, col, planar, samples[3];
	int x0, x1, y0, y1, sw, sh, dw, dh;
	double x, y, xscale, yscale, f0, f1;

	samples[0] = 0;
	samples[1] = samples[2] = 1; 
	xscale = 1.0 * src->width / dst->width;
	yscale = 1.0 * src->height / dst->height;
	
	for (planar = 0; planar < 3; planar++)
	{
		dp = dst->data[planar];
		dw = dst->width >> samples[planar];
		dh = dst->height >> samples[planar];
		sw = src->width >> samples[planar];
		sh = src->height >> samples[planar];

		for (row = 0; row < dh; row++)
		{
			y = row * yscale;
			//assert(y < (double)(sh - 1));
			y0 = (int)y;
			y1 = (y0 + 1 >= sh) ? y0 : (y0 + 1);
			assert(y0 < sh && y1 < sh);
			
			for (col = 0; col < dw; col++)
			{
				x = col * xscale;
				//assert(x < (double)(sw - 1));
				x0 = (int)x;
				x1 = (x0 + 1 >= sw) ? x0 : (x0 + 1);
				assert(x0 < sw && x1 < sw);

				v[0][0] = *(src->data[planar] + y0 * src->linesize[planar] + x0);
				v[0][1] = *(src->data[planar] + y0 * src->linesize[planar] + x1);
				v[1][0] = *(src->data[planar] + y1 * src->linesize[planar] + x0);
				v[1][1] = *(src->data[planar] + y1 * src->linesize[planar] + x1);

				f0 = ((double)x1 - x) * v[0][0] + (x - (double)x0) * v[0][1];
				f1 = ((double)x1 - x) * v[1][0] + (x - (double)x0) * v[1][1];

				dp[col] = (uint8_t)(((double)y1 - y) * f0 + (y - (double)y0) * f1);
				assert(((double)y1 - y) * f0 + (y - (double)y0) * f1 <= (double)255);
			}

			dp += dst->linesize[planar];
		}
	}
}

void interpolation_bilinear(struct avframe_t* dst, const struct avframe_t* src)
{
	switch (src->format)
	{
	case PICTURE_YUV420:
	case PICTURE_YUV422:
	case PICTURE_YUV444:
		yuv_interpolation_bilinear(dst, src);
		break;

	default:
		printf("%s unknown format: %d\n", __FUNCTION__, src->format);
		break;
	}
}
