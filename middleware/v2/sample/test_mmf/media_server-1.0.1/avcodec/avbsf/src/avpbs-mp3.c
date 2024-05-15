#include "avpbs.h"
#include "mp3-header.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct avpbs_mp3_t
{
	int avs;
	struct mp3_header_t mp3;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_mp3_destroy(void** pp)
{
	struct avpbs_mp3_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_mp3_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_mp3_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_mp3_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	// can be failure
	mp3_header_load(&bs->mp3, extra, bytes);
	assert(AVCODEC_AUDIO_MP3 == codec);
	bs->onpacket = onpacket;
	bs->param = param;
	bs->avs = stream;
	return bs;
}

static int avpbs_mp3_create_stream(struct avpbs_mp3_t* bs)
{
	avstream_release(bs->stream);
	bs->stream = avstream_alloc(0);
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	bs->stream->stream = bs->avs;
	bs->stream->codecid = AVCODEC_AUDIO_MP3;
	bs->stream->channels = mp3_get_channel(&bs->mp3);
	bs->stream->sample_rate = mp3_get_frequency(&bs->mp3);
	bs->stream->sample_bits = 16;
	return 0;
}

static int avpbs_mp3_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r;
	struct avpacket_t* pkt;
	struct avpbs_mp3_t* bs;

	bs = (struct avpbs_mp3_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	r = mp3_header_load(&bs->mp3, data, bytes);
	if (r < 0)
	{
		assert(0);
		return r;
	}

	if (!bs->stream || bs->stream->channels != mp3_get_channel(&bs->mp3)
		|| bs->stream->sample_rate != mp3_get_frequency(&bs->mp3))
	{
		avpbs_mp3_create_stream(bs);
	}
	
	memcpy(pkt->data, data, bytes);
	pkt->size = bytes - r;
	pkt->pts = pts;
	pkt->dts = dts;
	pkt->flags = flags;
	pkt->stream = bs->stream;
	avstream_addref(bs->stream);

	r = bs->onpacket(bs->param, bs->stream ? pkt : NULL);
	avpacket_release(pkt);
	return r;
}

struct avpbs_t* avpbs_mp3(void)
{
	static struct avpbs_t bs = {
		avpbs_mp3_create,
		avpbs_mp3_destroy,
		avpbs_mp3_input,
	};
	return &bs;
}
