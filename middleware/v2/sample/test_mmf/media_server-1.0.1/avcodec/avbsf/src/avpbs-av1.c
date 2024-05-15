#include "avpbs.h"
#include "aom-av1.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct avpbs_av1_t
{
	struct aom_av1_t av1;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_av1_destroy(void** pp)
{
	struct avpbs_av1_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_av1_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static int avpbs_av1_create_stream(struct avpbs_av1_t* bs, int stream, const uint8_t* extra, int bytes)
{
	avstream_release(bs->stream);
	bs->stream = avstream_alloc(bytes);
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	bs->stream->stream = stream;
	bs->stream->codecid = AVCODEC_VIDEO_AV1;
	memcpy(bs->stream->extra, extra, bytes);
	return bs->stream->bytes > 0 ? 0 : -1;
}

static void* avpbs_av1_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_av1_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	if (bytes > 0 && aom_av1_codec_configuration_record_load(extra, bytes, &bs->av1) < 0)
	{
		avpbs_av1_destroy((void**)&bs);
		return NULL;
	}

	avpbs_av1_create_stream(bs, stream, extra, bytes);
	assert(AVCODEC_VIDEO_AV1 == codec);
	bs->onpacket = onpacket;
	bs->param = param;
	return bs;
}

static int avpbs_av1_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r;
	struct avpacket_t* pkt;
	struct avpbs_av1_t* bs;

	bs = (struct avpbs_av1_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	// TODO: stream width/height

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

struct avpbs_t* avpbs_av1(void)
{
	static struct avpbs_t bs = {
		avpbs_av1_create,
		avpbs_av1_destroy,
		avpbs_av1_input,
	};
	return &bs;
}
