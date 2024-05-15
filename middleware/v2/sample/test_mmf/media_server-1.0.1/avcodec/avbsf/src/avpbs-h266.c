#include "avpbs.h"
#include "mpeg4-avc.h"
#include "mpeg4-vvc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define h266_STARTCODE_PADDING 32  // 3->4 startcode

struct avpbs_h266_t
{
	int avs;
	struct mpeg4_vvc_t vvc;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_h266_create_stream(struct avpbs_h266_t* bs)
{
	avstream_release(bs->stream);
	bs->stream = avstream_alloc((int)(bs->vvc.off + bs->vvc.numOfArrays * 2 + 64));
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	bs->stream->stream = bs->avs;
	bs->stream->codecid = AVCODEC_VIDEO_H265;
	bs->stream->width = bs->vvc.max_picture_width;
	bs->stream->height = bs->vvc.max_picture_height;
	bs->stream->bytes = mpeg4_vvc_decoder_configuration_record_save(&bs->vvc, bs->stream->extra, bs->stream->bytes);
	return bs->stream->bytes > 0 ? 0 : -1;
}

static int avpbs_h266_destroy(void** pp)
{
	struct avpbs_h266_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_h266_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_h266_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	int n;
	struct avpbs_h266_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	bs->onpacket = onpacket;
	bs->param = param;
	bs->avs = stream;

	// can be failure
	assert(AVCODEC_VIDEO_H266 == codec);
	n = mpeg4_h264_bitstream_format(extra, bytes);
	if (n >= 0) {
		h266_annexbtomp4(&bs->vvc, extra, bytes, NULL, 0, NULL, NULL);
	}
	else if (bytes >= 5 && 0xF8 == (0xF8 & extra[0])) { // '11111'b;
		mpeg4_vvc_decoder_configuration_record_load(extra, bytes, &bs->vvc);
	}

	if (bs->vvc.numOfArrays >= 3)
		avpbs_h266_create_stream(bs);
	return bs;
}

static int avpbs_h266_input(void* param, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes, int flags)
{
	int r, vcl, update;
	struct avpacket_t* pkt;
	struct avpbs_h266_t* bs;

	bs = (struct avpbs_h266_t*)param;
	pkt = avpacket_alloc(bytes + h266_STARTCODE_PADDING);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	r = h266_annexbtomp4(&bs->vvc, nalu, bytes, pkt->data, pkt->size, &vcl, &update);
	if (r < 1 || (update && bs->vvc.numOfArrays >= 3 && 0 != avpbs_h266_create_stream(bs)))
	{
		avpacket_release(pkt);
		return r < 0 ? r : (-(__ERROR__ + E2BIG)); // h266 data process failed
	}

	if (!bs->stream)
	{
		avpacket_release(pkt);
		return 0; // discard
	}

	pkt->size = r;
	pkt->pts = pts;
	pkt->dts = dts;
	pkt->flags = (flags & (~AVPACKET_FLAG_KEY)) | (1 == vcl ? AVPACKET_FLAG_KEY : 0);
	pkt->stream = bs->stream;
	avstream_addref(bs->stream);

	r = bs->onpacket(bs->param, bs->stream ? pkt : NULL);
	avpacket_release(pkt);
	return r;
}

struct avpbs_t* avpbs_h266(void)
{
	static struct avpbs_t bs = {
		avpbs_h266_create,
		avpbs_h266_destroy,
		avpbs_h266_input,
	};
	return &bs;
}
