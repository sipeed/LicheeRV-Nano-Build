#ifndef _flv2pkt_h_
#define _flv2pkt_h_

// EXPORT API:
// 1. int flv2avpkt_init(struct flv2avpkt_t* ctx)
// 2. int flv2avpkt_destroy(struct flv2avpkt_t* ctx)
// 3. int flv2avpkt_input(struct flv2avpkt_t* ctx, int avtype, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags, flv2avpkt_onpacket onpacket, void* param)

#include "avpbs.h"
#include "flv-proto.h"
#include "flv-header.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct flv2avpkt_t
{
	struct
	{
		avpbs_t* bs;
		void* ptr;
	} streams[3]; // 0-video, 1-audio, 2-metadata

	uint16_t video, audio;
};

typedef int (flv2avpkt_onpacket)(void* param, struct avpacket_t* pkt);

static inline int flv2avpkt_init(struct flv2avpkt_t* ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	return 0;
}

static inline int flv2avpkt_destroy(struct flv2avpkt_t* ctx)
{
	size_t i;
	for (i = 0; i < sizeof(ctx->streams) / sizeof(ctx->streams[0]); i++)
	{
		if (ctx->streams[i].bs && ctx->streams[i].ptr)
			ctx->streams[i].bs->destroy(&ctx->streams[i].ptr);
	}
	memset(ctx, 0, sizeof(*ctx));
	return 0;
}

static int flv2avpkt_find(struct flv2avpkt_t* ctx, int avtype, const void* data, int bytes, flv2avpkt_onpacket onpacket, void* param)
{
	void* ptr;
	avpbs_t* bs;
	AVPACKET_CODEC_ID codec;
	int stream;

	stream = FLV_SCRIPT_METADATA == avtype ? 2 : ((avtype & 0xFF) <= 0x0F ? 0 : 1);
	assert(stream >= 0 && stream < (int)(sizeof(ctx->streams) / sizeof(ctx->streams[0])));
	if (ctx->streams[stream].ptr)
	{
		if (0 == stream && (ctx->video & (1 << (avtype & 0x0F))))
			return 0;
		else if (1 == stream && (ctx->audio & (1 << ((avtype >> 4) & 0x0F))))
			return 1;
		else if (2 == stream)
			return 2;
	}

	switch (avtype)
	{
	case FLV_AUDIO_ASC:
		codec = AVCODEC_AUDIO_AAC;
		break;

	case FLV_AUDIO_OPUS_HEAD:
		codec = AVCODEC_AUDIO_OPUS;
		break;

	case FLV_VIDEO_AVCC:
		codec = AVCODEC_VIDEO_H264;
		break;

	case FLV_VIDEO_HVCC:
		codec = AVCODEC_VIDEO_H265;
		break;

	case FLV_VIDEO_VVCC:
		codec = AVCODEC_VIDEO_H266;
		break;

	case FLV_VIDEO_AV1C:
		codec = AVCODEC_VIDEO_AV1;
		break;

	case FLV_AUDIO_MP3:
	case FLV_AUDIO_MP3_8K:
		codec = AVCODEC_AUDIO_MP3;
		break;

	case FLV_AUDIO_G711A:
		codec = AVCODEC_AUDIO_G711A;
		break;

	case FLV_AUDIO_G711U:
		codec = AVCODEC_AUDIO_G711U;
		break;

	case FLV_SCRIPT_METADATA:
		codec = AVCODEC_DATA_RAW;
		break;

	default:
		return -1;
	}

	bs = avpbs_find(codec);
	if (!bs)
		return -1;

	ptr = bs->create(stream, codec, (const uint8_t*)data, bytes, onpacket, param);
	if (!ptr)
		return -1;

	if (0 == stream)
		ctx->video = 1 << (avtype & 0x0F);
	else if(1 == stream)
		ctx->audio = 1 << ((avtype >> 4) & 0x0F);

	if (ctx->streams[stream].bs && ctx->streams[stream].ptr)
		ctx->streams[stream].bs->destroy(&ctx->streams[stream].ptr);
	ctx->streams[stream].bs = bs;
	ctx->streams[stream].ptr = ptr;
	return stream;
}

static inline int flv2avpkt_input(struct flv2avpkt_t* ctx, int avtype, const void* data, size_t bytes, uint32_t pts, uint32_t dts, int flags, flv2avpkt_onpacket onpacket, void* param)
{
	int stream;

	stream = flv2avpkt_find(ctx, avtype, data, (int)bytes, onpacket, param);
	if (stream < 0 || stream >= (int)(sizeof(ctx->streams) / sizeof(ctx->streams[0])))
		return -1;

	if (avtype >= FLV_AUDIO_ASC)
		return 0;
	return ctx->streams[stream].bs->input(ctx->streams[stream].ptr, pts, dts, (const uint8_t*)data, (int)bytes, flags);
}

#if defined(__cplusplus)
}
#endif
#endif /* !_flv2pkt_h_ */
