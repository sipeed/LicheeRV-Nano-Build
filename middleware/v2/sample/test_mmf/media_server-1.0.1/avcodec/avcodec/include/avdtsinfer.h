#ifndef _avdtsinfer_h_
#define _avdtsinfer_h_

#include <inttypes.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct avdtsinfer_t
{
	int64_t count;
	int64_t last_dts; // last I/P frame dts
	
	int64_t consecutive_prev_max_pts;
	int64_t max_pts; // maximum frame pts
	int bframes; // consecutive b-frames
	int max_bframes; // max consecutive b-frames

	int fps1000; // 1/fps * 1000
	int residual;
};

void avdtsinfer_reset(struct avdtsinfer_t* infer);

/// infer current frame dts from next frame pts
/// @param[in] idr current frame type, 1-h.264/h.265 idr frame, other-B/P frame
/// @param[in] pts current frame pts
/// @param[in] next_pts next frame pts
/// @return current frame dts
int64_t avdtsinfer_update(struct avdtsinfer_t* infer, int idr, int64_t pts, int64_t next_pts);

#ifdef __cplusplus
}
#endif
#endif /* !_avdtsinfer_h_ */
