#include "avpbs.h"
#include "h265-sps.h"
#include "mpeg4-avc.h"
#include "mpeg4-hevc.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define H265_NAL_SPS		33

#define h265_STARTCODE_PADDING 32  // 3->4 startcode

struct avpbs_h265_t
{
	int avs;
	struct mpeg4_hevc_t hevc;
	struct avstream_t* stream;
	avpbs_onpacket onpacket;
	void* param;
};

static int avpbs_h265_create_stream(struct avpbs_h265_t* bs)
{
	int x, y;
	struct h265_sps_t sps;

	avstream_release(bs->stream);
	bs->stream = avstream_alloc((int)(bs->hevc.off + bs->hevc.numOfArrays * 2 + 64));
	if (!bs->stream)
		return -(__ERROR__ + ENOMEM);

	for (x = 0; x < bs->hevc.numOfArrays; x++)
	{
		if (H265_NAL_SPS != bs->hevc.nalu[x].type)
			continue;

		memset(&sps, 0, sizeof(sps));
		h265_sps_parse(bs->hevc.nalu[x].data, bs->hevc.nalu[x].bytes, &sps);
		h265_display_rect(&sps, &x, &y, &bs->stream->width, &bs->stream->height);
		break;
	}

	bs->stream->stream = bs->avs;
	bs->stream->codecid = AVCODEC_VIDEO_H265;
	bs->stream->bytes = mpeg4_hevc_decoder_configuration_record_save(&bs->hevc, bs->stream->extra, bs->stream->bytes);
	return bs->stream->bytes > 0 ? 0 : -1;
}

static int avpbs_h265_destroy(void** pp)
{
	struct avpbs_h265_t* bs;
	if (pp && *pp)
	{
		bs = (struct avpbs_h265_t*)*pp;
		avstream_release(bs->stream);
		free(bs);
		*pp = NULL;
	}
	return 0;
}

static void* avpbs_h265_create(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param)
{
	int n;
	struct avpbs_h265_t* bs;
	bs = calloc(1, sizeof(*bs));
	if (!bs) return NULL;

	bs->onpacket = onpacket;
	bs->param = param;
	bs->avs = stream;

	// can be failure
	assert(AVCODEC_VIDEO_H265 == codec);
	n = mpeg4_h264_bitstream_format(extra, bytes);
	if (n >= 0) {
		h265_annexbtomp4(&bs->hevc, extra, bytes, NULL, 0, NULL, NULL);
	} else if (bytes >= 23 && 0x01 == extra[0]) {
		mpeg4_hevc_decoder_configuration_record_load(extra, bytes, &bs->hevc);
	}

	if (bs->hevc.numOfArrays >= 3)
		avpbs_h265_create_stream(bs);
	return bs;
}

static int avpbs_h265_input(void* param, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes, int flags)
{
	int r, vcl, update;
	struct avpacket_t* pkt;
	struct avpbs_h265_t* bs;	

	bs = (struct avpbs_h265_t*)param;
	pkt = avpacket_alloc(bytes + h265_STARTCODE_PADDING);
	if (!pkt) return -(__ERROR__ + ENOMEM);

	r = h265_annexbtomp4(&bs->hevc, nalu, bytes, pkt->data, pkt->size, &vcl, &update);
	if (r < 1 || (update && bs->hevc.numOfArrays >= 3 && 0 != avpbs_h265_create_stream(bs)))
	{
		avpacket_release(pkt);
		return r < 0 ? r : (-(__ERROR__ + E2BIG)); // h265 data process failed
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

struct avpbs_t* avpbs_h265(void)
{
	static struct avpbs_t bs = {
		avpbs_h265_create,
		avpbs_h265_destroy,
		avpbs_h265_input,
	};
	return &bs;
}
