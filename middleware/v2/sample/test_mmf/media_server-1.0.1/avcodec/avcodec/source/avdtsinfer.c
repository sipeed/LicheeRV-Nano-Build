#include "avdtsinfer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

void avdtsinfer_reset(struct avdtsinfer_t* infer)
{
	memset(infer, 0, sizeof(*infer));
	infer->fps1000 = 1000;
}

int64_t avdtsinfer_update(struct avdtsinfer_t* infer, int idr, int64_t pts, int64_t next_pts)
{
	int64_t dts;

	assert(pts >= 0 && next_pts >= 0); // can't be negative value
	if (infer->count > 0 && pts < infer->max_pts)
	{
		// B Frames
		infer->bframes++;
		infer->max_bframes = infer->max_bframes > infer->bframes ? infer->max_bframes : infer->bframes;
	}
	else
	{
		// I/P frame
		if (infer->bframes > 0 && infer->consecutive_prev_max_pts < infer->max_pts)
		{
			// calc fps between two consecutive I/P frames
			infer->fps1000 = (int)((infer->max_pts - infer->consecutive_prev_max_pts) * 1000 / (infer->bframes + 1));
			//printf("fps: (%lld - %lld) / %d = %.1f\n", infer->max_pts, infer->consecutive_prev_max_pts, infer->bframes + 1, infer->fps1000 / 1000.0);
		}

		infer->consecutive_prev_max_pts = infer->count > 0 ? infer->max_pts : pts;
		infer->max_pts = pts;
		infer->bframes = 0;
	}

	// update fps on IDR frame
	if (1 == idr && pts > infer->consecutive_prev_max_pts && (infer->max_bframes + 1) * infer->fps1000 / 1000 < pts - infer->consecutive_prev_max_pts)
	{
		// key frame dts offset
		infer->last_dts += pts - infer->consecutive_prev_max_pts - (infer->max_bframes + 1) * infer->fps1000 / 1000;
		//infer->last_dts = (infer->last_dts < dts || 0 == infer->count) ? dts : (infer->last_dts + 1); // monotone increasing
	}
	else
	{
		if (0 == infer->max_bframes && next_pts >= pts)
		{
			infer->last_dts = pts; // non-B-frames: dts = pts
		}
		else
		{
			dts = infer->last_dts + infer->fps1000 / 1000;
			infer->residual += infer->fps1000 % 1000;
			if (infer->residual >= 1000)
			{
				dts += infer->residual / 1000;
				infer->residual %= 1000;
			}
			// keep dts <= pts (for ffmpeg)
			infer->last_dts = dts > pts ? (infer->last_dts + 1) : dts;
		}
	}

	infer->count++;
	return infer->last_dts;
}

#if defined(_DEBUG) || defined(DEBUG)
void avdtsinfer_test(void)
{
	// pts from Elecard_about_Tomsk_part2_HEVC_720p.mp4
	const int idr[]     = { 1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    0, };
	const int64_t pts[] = { 160, 426, 293, 226, 193, 260, 360, 326, 393, 693, 560, 493, 460, 526, 626, 593, 660, 960, 826, 760, 726, 793, 893, 860, 926, 1286, 1126, 1046, 1006, 1086, 1206, 1166, 1246, 1606, 1446, 1366, 1326, 1406, 1526, 1486, 1566, 1926, 1766, 1686, 1646, 1726, 1806, 1846, 1886, 2246, 2086, 2006, 1966, 2046, 2166, 2126, 2206, 2566, 2406, 2326 };
	const int64_t dts[] = { 0,   33,  66,  100, 133, 166, 200, 233, 266, 300, 333, 366, 400, 433, 466, 500, 533, 566, 613, 646, 679, 714, 746, 779, 813, 846,  886,  926,  966,  1006, 1046, 1086, 1126, 1166, 1206, 1246, 1286, 1326, 1366, 1406 };

	int i;
	int64_t v, v0;
	struct avdtsinfer_t infer;

	avdtsinfer_reset(&infer);
	//infer.fps1000 = 33333; // 33.3

	v0 = 0;
	for (i = 0; i < sizeof(pts) / sizeof(pts[0]) - 1; i++)
	{
		v = avdtsinfer_update(&infer, idr[i], pts[i], pts[i+1]);
		printf("pts -> dts: %"PRId64" -> %"PRId64"(%"PRId64") %s %s\n", pts[i], v, v-v0, idr[i] ? "[IDR]" : "", v > pts[i] ? "[-]" : "");
		v0 = v;
	}
}
#endif
