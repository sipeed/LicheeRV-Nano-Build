#ifndef _av_segment_h_
#define _av_segment_h_

#include <stdint.h>
#include "avpacket.h"

#ifdef __cplusplus
extern "C" {
#endif

struct avsegment_t
{
	struct
	{
		int64_t size; // bytes limit (A+V)
		int64_t packets; // packets limit (A+V)
		int64_t duration; // duration limit (ms)
	} limit;
	
	// current
	int64_t size; // bytes
	int64_t packets; // packets
	int64_t timeline; // first packet timestamp(dts/pts)
	struct avstream_t* streams[8]; // stream version

	int keyframe; // keyframe counter
	int video; // has video
	int audio; // has audio
};

/// @param[in] timeline first packet timestamp
void avsegment_reset(struct avsegment_t* seg, int64_t timeline);

/// @param[in] timeline timestamp return by timeline input32/input64
/// @return >0-should create new segment, 0-don't need, <0-error
int avsegment_check(struct avsegment_t* seg, const struct avpacket_t* pkt, int64_t timeline, int discontinuity);

/// @param[in] timeline timestamp return by timeline input32/input64
/// @return 0-ok, other-error
int avsegment_input(struct avsegment_t* seg, const struct avpacket_t* pkt, int64_t timeline);

#ifdef __cplusplus
}
#endif
#endif /* !_av_segment_h_ */
