#include "avpbs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

// g711/g726/729
struct avpbs_g7xx_t
{
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_g7xx_destroy(void** pp)
{
	struct avpbs_g7xx_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_g7xx_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_g7xx_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_g7xx_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	bs->stream = avstream_alloc(0);
	if (bs->stream)
	{
		bs->stream->stream = stream;
		switch (codec)
		{
		case AVCODEC_AUDIO_G711A:
		case AVCODEC_AUDIO_G711U:
		case AVCODEC_AUDIO_G726:
		case AVCODEC_AUDIO_G729:
			bs->stream->codecid = codec;
			bs->stream->channels = 1;
			bs->stream->sample_rate = 8000;
			bs->stream->sample_bits = 16;
			break;

		default:
			assert(0);
		}
	}

	(void)extra, (void)bytes; // ignore
	bs->onpacket = onpacket;
	bs->param = param;
	return bs;
}

static int avpbs_g7xx_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r;
	struct avpacket_t* pkt;
	struct avpbs_g7xx_t* bs;

	bs = (struct avpbs_g7xx_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	memcpy(pkt->data, data, bytes);
	pkt->pts = pts;
	pkt->dts = dts;
	pkt->flags = flags;
	pkt->stream = bs->stream;
	avstream_addref(bs->stream);

	r = bs->onpacket(bs->param, bs->stream ? pkt : NULL);
	avpacket_release(pkt);
	return r;
}

struct avpbs_t* avpbs_g7xx(void)
{
	static struct avpbs_t bs = {
		avpbs_g7xx_create,
		avpbs_g7xx_destroy,
		avpbs_g7xx_input,
	};
	return &bs;
}
