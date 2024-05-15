#include "avpkt2bs.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define N_PACKER_SIZE (32 * 1024 * 1024)

int avpkt2bs_create(struct avpkt2bs_t* bs)
{
	memset(bs, 0, sizeof(*bs));
	return 0;
}

int avpkt2bs_destroy(struct avpkt2bs_t* bs)
{
	if (bs->ptr)
	{
		assert(bs->cap > 0);
		free(bs->ptr);
		bs->ptr = NULL;
		bs->cap = 0;
	}
	return 0;
}

static int avpkt2bs_realloc(struct avpkt2bs_t* bs, int bytes)
{
	void* p;
	if (bytes > bs->cap)
	{
		if(bytes > N_PACKER_SIZE)
			return -(__ERROR__ + ENOMEM);
		p = realloc(bs->ptr, bytes);
		if (NULL == p)
			return -(__ERROR__ + ENOMEM);
		bs->ptr = (uint8_t*)p;
		bs->cap = bytes;
	}
	return 0;
}

static int avpkt2bs_aac_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	int r;
	if (0 == bs->a.aac.sampling_frequency)
	{
		r = mpeg4_aac_audio_specific_config_load((const uint8_t*)pkt->stream->extra, pkt->stream->bytes, &bs->a.aac);
		if (r < 0)
			return r;
	}

	r = avpkt2bs_realloc(bs, 7 + bs->a.aac.npce + pkt->size);
	if (0 != r)
		return r;

	r = mpeg4_aac_adts_save(&bs->a.aac, pkt->size, bs->ptr, bs->cap);
	if (r < 0 || r + pkt->size > bs->cap)
		return r < 0 ? r : (-(__ERROR__ + E2BIG));

	memmove(bs->ptr + r, pkt->data, pkt->size);
	return r + pkt->size;
}

static int avpkt2bs_h264_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	int r;
	if (0 == bs->v.avc.off)
	{
		r = mpeg4_avc_decoder_configuration_record_load((const uint8_t*)pkt->stream->extra, pkt->stream->bytes, &bs->v.avc);
		if (r < 0)
			return r;
	}

	r = avpkt2bs_realloc(bs, pkt->size + 16 + sizeof(bs->v.avc.data));
	if (0 != r)
		return r;

	return h264_mp4toannexb(&bs->v.avc, pkt->data, pkt->size, bs->ptr, bs->cap);
}

static int avpkt2bs_h265_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	int r;
	if (0 == bs->v.hevc.off)
	{
		r = mpeg4_hevc_decoder_configuration_record_load((const uint8_t*)pkt->stream->extra, pkt->stream->bytes, &bs->v.hevc);
		if (r < 0)
			return r;
	}

	r = avpkt2bs_realloc(bs, pkt->size + 16 + sizeof(bs->v.hevc.data));
	if (0 != r)
		return r;

	return h265_mp4toannexb(&bs->v.hevc, pkt->data, pkt->size, bs->ptr, bs->cap);
}

static int avpkt2bs_h266_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	int r;
	if (0 == bs->v.vvc.off)
	{
		r = mpeg4_vvc_decoder_configuration_record_load((const uint8_t*)pkt->stream->extra, pkt->stream->bytes, &bs->v.vvc);
		if (r < 0)
			return r;
	}

	r = avpkt2bs_realloc(bs, pkt->size + 16 + sizeof(bs->v.vvc.data));
	if (0 != r)
		return r;

	return h266_mp4toannexb(&bs->v.vvc, pkt->data, pkt->size, bs->ptr, bs->cap);
}

static int avpkt2bs_copy_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	int r;
	r = avpkt2bs_realloc(bs, pkt->size);
	if (0 != r)
		return r;

	memcpy(bs->ptr, pkt->data, pkt->size);
	return pkt->size;
}

int avpkt2bs_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt)
{
	switch (pkt->stream->codecid)
	{
	case AVCODEC_AUDIO_AAC:
		return avpkt2bs_aac_input(bs, pkt);

	case AVCODEC_AUDIO_MP3:
	case AVCODEC_AUDIO_OPUS:
	case AVCODEC_AUDIO_G711A:
	case AVCODEC_AUDIO_G711U:
		return avpkt2bs_copy_input(bs, pkt);

	case AVCODEC_VIDEO_H264:
		return avpkt2bs_h264_input(bs, pkt);

	case AVCODEC_VIDEO_H265:
		return avpkt2bs_h265_input(bs, pkt);

	case AVCODEC_VIDEO_H266:
		return avpkt2bs_h266_input(bs, pkt);

	case AVCODEC_VIDEO_AV1:
	case AVCODEC_VIDEO_VP8:
	case AVCODEC_VIDEO_VP9:
		return avpkt2bs_copy_input(bs, pkt);

	case AVCODEC_DATA_MP2P:
		return avpkt2bs_copy_input(bs, pkt);

	case AVCODEC_VIDEO_SVAC:
	case AVCODEC_AUDIO_SVAC:
		return avpkt2bs_copy_input(bs, pkt);

	default:
		return -(__ERROR__ + ENOPROTOOPT);
	}
}
