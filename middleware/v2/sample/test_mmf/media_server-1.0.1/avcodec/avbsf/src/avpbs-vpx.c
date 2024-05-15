#include "avpbs.h"
#include "webm-vpx.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct avpbs_vpx_t
{
	int avs;
	struct webm_vpx_t vpx;
	AVPACKET_CODEC_ID codec;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_vpx_destroy(void** pp)
{
	struct avpbs_vpx_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_vpx_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static int avpbs_vpx_create_stream(struct avpbs_vpx_t* bs, int width, int height, const uint8_t* extra, int bytes)
{
	avstream_release(bs->stream);
	bs->stream = avstream_alloc(bytes);
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	bs->stream->width = width;
	bs->stream->height = height;
	bs->stream->stream = bs->avs;
	bs->stream->codecid = bs->codec;
	memcpy(bs->stream->extra, extra, bytes);
	return bs->stream->bytes > 0 ? 0 : -1;
}

static void* avpbs_vpx_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	struct avpbs_vpx_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	bs->avs = stream;
	bs->codec = codec;
	if(webm_vpx_codec_configuration_record_load(extra, bytes, &bs->vpx) > 0)
		avpbs_vpx_create_stream(bs, 0, 0, extra, bytes);
	
	assert(AVCODEC_VIDEO_VP8 == codec || AVCODEC_VIDEO_VP9 == codec);
	bs->onpacket = onpacket;
	bs->param = param;
	return bs;
}

static int avpbs_vpx_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags)
{
	int r, width, height;
	struct avpacket_t* pkt;
	struct avpbs_vpx_t* bs;

	bs = (struct avpbs_vpx_t*)param;
	pkt = avpacket_alloc(bytes);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	flags &= ~AVPACKET_FLAG_KEY;
	if ( (AVCODEC_VIDEO_VP8 == bs->codec && webm_vpx_codec_configuration_record_from_vp8(&bs->vpx, &width, &height, data, bytes) >= 0)
		|| (AVCODEC_VIDEO_VP9 == bs->codec && webm_vpx_codec_configuration_record_from_vp9(&bs->vpx, &width, &height, data, bytes) >= 0))
	{
		flags = AVPACKET_FLAG_KEY;
		if (!bs->stream || bs->stream->width != width || bs->stream->height != height)
		{
			uint8_t extra[32];
			r = webm_vpx_codec_configuration_record_save(&bs->vpx, extra, sizeof(extra));
			avpbs_vpx_create_stream(bs, width, height, extra, r);
		}
	}

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

struct avpbs_t* avpbs_vpx(void)
{
	static struct avpbs_t bs = {
		avpbs_vpx_create,
		avpbs_vpx_destroy,
		avpbs_vpx_input,
	};
	return &bs;
}
