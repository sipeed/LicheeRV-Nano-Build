#include "avpbs.h"
#include "opus-head.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct avpbs_opus_t
{
	int avs;
	struct opus_head_t opus;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_opus_destroy(void** pp)
{
	struct avpbs_opus_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_opus_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_opus_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_opus_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	// default
	bs->opus.input_sample_rate = 48000;

	// can be failure
	opus_head_load(extra, bytes, &bs->opus);
	assert(AVCODEC_AUDIO_OPUS == codec);
	bs->onpacket = onpacket;
	bs->param = param;
	bs->avs = stream;
	return bs;
}

static int avpbs_opus_create_stream(struct avpbs_opus_t* bs)
{
	avstream_release(bs->stream);
	bs->stream = avstream_alloc(0);
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	bs->stream->stream = bs->avs;
	bs->stream->codecid = AVCODEC_AUDIO_OPUS;
	bs->stream->channels = opus_head_channels(&bs->opus);
	bs->stream->sample_rate = bs->opus.input_sample_rate;
	bs->stream->sample_bits = 16;
	return 0;
}

static int avpbs_opus_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r;
	struct avpacket_t* pkt;
	struct avpbs_opus_t* bs;

	bs = (struct avpbs_opus_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	if (bytes > 8 && 0 == memcmp(data, "OpusHead", 8))
	{
		r = opus_head_load(data, bytes, &bs->opus);
		if (r < 0)
			return r;
		if (r >= bytes)
			return 0; // ignore
		
		data += r;
		bytes -= r;
	}

	if (!bs->stream || bs->stream->channels != opus_head_channels(&bs->opus)
		|| bs->stream->sample_rate != (int)bs->opus.input_sample_rate)
	{
		avpbs_opus_create_stream(bs);
	}

	memcpy(pkt->data, data, bytes);
	pkt->size = bytes;
	pkt->pts = pts;
	pkt->dts = dts;
	pkt->flags = flags;
	pkt->stream = bs->stream;
	avstream_addref(bs->stream);

	r = bs->onpacket(bs->param, bs->stream ? pkt : NULL);
	avpacket_release(pkt);
	return r;
}

struct avpbs_t* avpbs_opus(void)
{
	static struct avpbs_t bs = {
		avpbs_opus_create,
		avpbs_opus_destroy,
		avpbs_opus_input,
	};
	return &bs;
}
