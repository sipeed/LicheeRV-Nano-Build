#ifndef _pkt2flv_h_
#define _pkt2flv_h_

// EXPORT API:
// 1. int avpkt2flv_reset(struct avpkt2flv_t* flv)
// 2. int avpkt2flv_input(struct avpkt2flv_t* flv, const struct avpacket_t* pkt, flv_muxer2_handler handler, void* param)

#include "avpacket.h"
#include "avstream.h"
#include "avcodecid.h"
#include "flv-proto.h"
#include "flv-header.h"
#include "flv-writer.h"
#include "rtsp-payloads.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct avpkt2flv_t
{
	int ash; // audio sequence header
	int vsh; // video sequence header
};

typedef int (*avpkt2flv_handler)(void* param, int type, const struct flv_vec_t* vec, int num, uint32_t timestamp);

static inline int avpkt2flv_reset(struct avpkt2flv_t* flv)
{
	memset(flv, 0, sizeof(*flv));
	return 0;
}

static int avpkt2flv_audio(struct avpkt2flv_t* flv, const struct avpacket_t* pkt, uint8_t codec, avpkt2flv_handler handler, void* param)
{
	int r, n;
	uint8_t header[2][2];
	struct flv_vec_t vec[4];
	struct flv_audio_tag_header_t tag;

	n = 0;
	tag.codecid = codec;

	if (AVCODEC_AUDIO_AAC == pkt->stream->codecid || AVCODEC_AUDIO_OPUS == pkt->stream->codecid)
	{
		tag.rate = FLV_SOUND_RATE_44100; // 44k-SoundRate
		tag.bits = FLV_SOUND_BIT_16; // 16-bit samples
		tag.channels = FLV_SOUND_CHANNEL_STEREO; // Stereo sound
	}
	else
	{
		switch (pkt->stream->sample_rate)
		{
		case 5500: tag.rate = FLV_SOUND_RATE_5500; break;
		case 11025: tag.rate = FLV_SOUND_RATE_11025; break;
		case 22050: tag.rate = FLV_SOUND_RATE_22050; break;
		case 44100: tag.rate = FLV_SOUND_RATE_44100; break;
		default: tag.rate = FLV_SOUND_RATE_44100;
		}

		tag.bits = (uint8_t)(8 == pkt->stream->sample_bits ? FLV_SOUND_BIT_8 : FLV_SOUND_BIT_16);
		tag.channels = (uint8_t)(pkt->stream->channels < 2 ? FLV_SOUND_CHANNEL_MONO : FLV_SOUND_CHANNEL_STEREO);
	}

	if (!flv->ash)
	{
		flv->ash = 1;
		tag.avpacket = FLV_SEQUENCE_HEADER;
		vec[n].len = flv_audio_tag_header_write(&tag, header[0], sizeof(header[0]));
		vec[n].ptr = header[0];
		vec[n + 1].ptr = pkt->stream->extra;
		vec[n + 1].len = pkt->stream->bytes;
		assert(vec[0].len > 0);
		r = handler(param, FLV_TYPE_AUDIO, vec, n + 2, (uint32_t)pkt->dts);
		if (0 != r) return r;
		//n += 2;
	}

	tag.avpacket = FLV_AVPACKET;
	vec[n].len = flv_audio_tag_header_write(&tag, header[1], sizeof(header[1]));
	vec[n].ptr = header[1];
	vec[n + 1].ptr = pkt->data;
	vec[n + 1].len = pkt->size;
	assert(vec[n].len > 0);
	return handler(param, FLV_TYPE_AUDIO, vec, n + 2, (uint32_t)pkt->dts);
}

static int avpkt2flv_video(struct avpkt2flv_t* flv, const struct avpacket_t* pkt, uint8_t codec, avpkt2flv_handler handler, void* param)
{
	int r, n;
	uint8_t header[2][5];
	struct flv_vec_t vec[4];
	struct flv_video_tag_header_t tag;

	n = 0;
	tag.codecid = codec;

	if (!flv->vsh)
	{
		flv->vsh = 1;
		tag.cts = 0;
		tag.keyframe = 1; // keyframe
		tag.avpacket = FLV_SEQUENCE_HEADER;
		vec[n].len = flv_video_tag_header_write(&tag, header[0], sizeof(header[0]));
		vec[n].ptr = header[0];
		vec[n + 1].ptr = pkt->stream->extra;
		vec[n + 1].len = pkt->stream->bytes;
		assert(vec[n].len > 0);
		r = handler(param, FLV_TYPE_VIDEO, vec, n + 2, (uint32_t)pkt->dts);
		if (0 != r) return r;
		//n += 2;
	}

	tag.cts = (int32_t)((uint32_t)pkt->pts - (uint32_t)pkt->dts);
	tag.keyframe = (uint8_t)((pkt->flags & AVPACKET_FLAG_KEY) ? FLV_VIDEO_KEY_FRAME : FLV_VIDEO_INTER_FRAME);
	tag.avpacket = FLV_AVPACKET;
	vec[n].len = flv_video_tag_header_write(&tag, header[1], sizeof(header[1]));
	vec[n].ptr = header[1];
	vec[n + 1].ptr = pkt->data;
	vec[n + 1].len = pkt->size;
	assert(vec[n].len > 0);
	return handler(param, FLV_TYPE_VIDEO, vec, n + 2, (uint32_t)pkt->dts);
}

static int avpkt2flv_script(const struct avpacket_t* pkt, avpkt2flv_handler handler, void* param)
{
	struct flv_vec_t vec[1];
	vec[0].ptr = pkt->data;
	vec[0].len = pkt->size;
	return handler(param, FLV_TYPE_SCRIPT, vec, 1, (uint32_t)pkt->dts);
}

static inline int avpkt2flv_input(struct avpkt2flv_t* flv, const struct avpacket_t* pkt, avpkt2flv_handler handler, void* param)
{
	int i;
	i = avcodecid_find_by_codecid(pkt->stream->codecid);
	if (-1 == i)
		return -1; // not found

	switch (avstream_type(pkt->stream))
	{
	case AVSTREAM_AUDIO:
		return avpkt2flv_audio(flv, pkt, (uint8_t)s_payloads[i].flv, handler, param);
	case AVSTREAM_VIDEO:
		return avpkt2flv_video(flv, pkt, (uint8_t)s_payloads[i].flv, handler, param);
	case AVSTREAM_DATA:
		return avpkt2flv_script(pkt, handler, param);
	default:
		assert(0);
		return -1;
	}
}

#if defined(__cplusplus)
}
#endif
#endif /* !_pkt2flv_h_ */
