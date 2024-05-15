#include "avpbs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

static struct avpbs_t* avpbs_common(void);

struct avpbs_t* avpbs_h264(void);
struct avpbs_t* avpbs_h265(void);
struct avpbs_t* avpbs_h266(void);
struct avpbs_t* avpbs_av1(void);
struct avpbs_t* avpbs_vpx(void);

struct avpbs_t* avpbs_aac(void);
struct avpbs_t* avpbs_mp3(void);
struct avpbs_t* avpbs_opus(void);
struct avpbs_t* avpbs_g7xx(void);

struct avpbs_t* avpbs_find(AVPACKET_CODEC_ID codec)
{
	switch (codec)
	{
		// video
	case AVCODEC_VIDEO_H264:
		return avpbs_h264();
	case AVCODEC_VIDEO_H265:
		return avpbs_h265();
	case AVCODEC_VIDEO_H266:
		return avpbs_h266();
	case AVCODEC_VIDEO_AV1:
		return avpbs_av1();
	case AVCODEC_VIDEO_VP8:
	case AVCODEC_VIDEO_VP9:
		return avpbs_vpx();

		// audio
	case AVCODEC_AUDIO_AAC:
		return avpbs_aac();
	case AVCODEC_AUDIO_MP3:
		return avpbs_mp3();
	case AVCODEC_AUDIO_OPUS:
		return avpbs_opus();
	case AVCODEC_AUDIO_G711A:
	case AVCODEC_AUDIO_G711U:
	case AVCODEC_AUDIO_G726:
	case AVCODEC_AUDIO_G729:
		return avpbs_g7xx();

	default:
		return avpbs_common();
	}
}

struct avpbs_common_t
{
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_common_destroy(void** pp)
{
	struct avpbs_common_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_common_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_common_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_common_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	bs->stream = avstream_alloc(bytes);
	if (bs->stream)
	{
		bs->stream->stream = stream;
		bs->stream->codecid = codec;
		memcpy(bs->stream->extra, extra, bytes);
	}

	bs->onpacket = onpacket;
	bs->param = param;
	return bs;
}

static int avpbs_common_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r;
	struct avpacket_t* pkt;
	struct avpbs_common_t* bs;

	bs = (struct avpbs_common_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	memcpy(pkt->data, data, bytes);
	pkt->pts = pts;
	pkt->dts = dts;
	pkt->flags = flags;
	pkt->stream = bs->stream;
	avstream_addref(bs->stream);

	r = bs->onpacket(bs->param, pkt);
	avpacket_release(pkt);
	return r;
}

static struct avpbs_t* avpbs_common(void)
{
	static struct avpbs_t bs = {
		   avpbs_common_create,
		   avpbs_common_destroy,
		   avpbs_common_input,
	};
	return &bs;
}
