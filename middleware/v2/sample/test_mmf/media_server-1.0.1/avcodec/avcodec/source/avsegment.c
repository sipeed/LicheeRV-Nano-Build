#include "avsegment.h"
#include <assert.h>

void avsegment_reset(struct avsegment_t* seg, int64_t timeline)
{
	int i;
	for (i = 0; i < sizeof(seg->streams) / sizeof(seg->streams[0]); i++)
	{
		//seg->streams[i].first = 0;
		//seg->streams[i].last = 0;
		//seg->streams[i].duratoin = 0; // clear duation
		seg->streams[i] = (struct avstream_t*)0;
	}
	
	seg->size = 0;
	seg->packets = 0;
	seg->timeline = timeline;

	seg->keyframe = 0;
	seg->audio = 0;
	seg->video = 0;
}

int avsegment_check(struct avsegment_t* seg, const struct avpacket_t* pkt, int64_t timeline, int discontinuity)
{
	int keyframe;
	int64_t duration;
	if (pkt->stream->stream < 0 || pkt->stream->stream >= sizeof(seg->streams) / sizeof(seg->streams[0]))
	{
		assert(0);
		return -1;
	}

	if (seg->packets < 1 || !seg->streams[pkt->stream->stream])
	{
		assert(!seg->streams[pkt->stream->stream]);
		return 0;
	}

	keyframe = (AVSTREAM_VIDEO == avstream_type(pkt->stream) && (pkt->flags & AVPACKET_FLAG_KEY)) ? 1 : 0;

	// fix: ffmpeg concat failed without a/v packets
	// fix: ffmpeg sps/sps not found without key frame
	if (discontinuity && keyframe && seg->audio && seg->video)
		return 1;

	// stream changed
	if (seg->streams[pkt->stream->stream] && seg->streams[pkt->stream->stream] != pkt->stream)
		return 2;

	duration = timeline - seg->timeline;
	if (keyframe)
	{
		// TODO: gop length check
		if(duration >= seg->limit.duration)
			return 3;
	}
	else
	{
		// audio only
		if(!seg->video && (duration >= seg->limit.duration || seg->size + pkt->size >= seg->limit.size))
			return 4;

		// force segment by duration
		if (duration >= seg->limit.duration && duration >= 2 * 60 * 1000)
			return 5;
	}

	// force segment by size
	if (seg->size + pkt->size >= seg->limit.size)
		return 6;

	// force segment by packets
	if (seg->packets + 1 >= seg->limit.packets)
		return 7;

	return 0;
}

int avsegment_input(struct avsegment_t* seg, const struct avpacket_t* pkt, int64_t timeline)
{
	if (pkt->stream->stream < 0 || pkt->stream->stream >= sizeof(seg->streams) / sizeof(seg->streams[0]))
	{
		assert(0);
		return -1;
	}

	if (!seg->streams[pkt->stream->stream] || seg->streams[pkt->stream->stream] != pkt->stream)
	{
		//seg->streams[pkt->stream->stream].first = timestamp;
		seg->streams[pkt->stream->stream] = pkt->stream;
	}
	//seg->streams[pkt->stream->stream].last = timestamp;
	//seg->streams[pkt->stream->stream].duratoin = timestamp - seg->streams[pkt->stream->stream].first;

	seg->size += pkt->size;
	seg->packets++;
	
	switch (avstream_type(pkt->stream))
	{
	case AVSTREAM_VIDEO:
		seg->video = 1;
		if (pkt->flags & AVPACKET_FLAG_KEY)
			seg->keyframe++;
		break;

	case AVSTREAM_AUDIO:
		seg->audio = 1;
		break;

	default:
		// nothing to do
		break;
	}

	return 0;
}
