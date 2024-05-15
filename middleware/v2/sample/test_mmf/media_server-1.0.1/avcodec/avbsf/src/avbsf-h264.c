#include "avbsf.h"
#include "cbuffer.h"
#include "mpeg4-avc.h"
#include "avdtsinfer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define H264_NAL_IDR		5 // Coded slice of an IDR picture
#define H264_NAL_SPS		7 // Sequence parameter set
#define H264_NAL_PPS		8 // Picture parameter set
#define H264_NAL_AUD		9 // Access unit delimiter

static const uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };

struct h264bsf_t
{
	struct mpeg4_avc_t avc;
	struct cbuffer_t ptr;
	int sps_pps_flag;
	int vcl;
	int64_t pts;
	int64_t dts;
	struct avdtsinfer_t infer;

	uint8_t extra[4 * 1024];
	int extra_bytes;

	avbsf_onpacket onpacket;
	void* param;
};

static int h264bsf_destroy(void** pp)
{
	struct h264bsf_t* bsf;
	if (pp && *pp)
	{
		bsf = (struct h264bsf_t*)*pp;
		cbuffer_free(&bsf->ptr);
		free(bsf);
		*pp = NULL;
	}
	return 0;
}

static void* h264bsf_create(const uint8_t* extra, int bytes, avbsf_onpacket onpacket, void* param)
{
	struct h264bsf_t* bsf;
	bsf = calloc(1, sizeof(*bsf));
	if (!bsf) return NULL;

	avdtsinfer_reset(&bsf->infer);
	cbuffer_init(&bsf->ptr);
	bsf->sps_pps_flag = 0;
	
	if (bytes > 5 && bytes <= sizeof(bsf->extra) && 0x00 == extra[0] && 0x00 == extra[1] && (0x01 == extra[2] || (0x00 == extra[2] && 0x01 == extra[3])))
	{
		memcpy(bsf->extra, extra, bytes);
		bsf->extra_bytes = bytes;
	}
	else if (0 == mpeg4_avc_decoder_configuration_record_load(extra, bytes, &bsf->avc))
	{
		bsf->extra_bytes = mpeg4_avc_to_nalu(&bsf->avc, bsf->extra, sizeof(bsf->extra));
		if (bsf->extra_bytes < 0)
			bsf->extra_bytes = 0;
	}

	bsf->onpacket = onpacket;
	bsf->param = param;
	return bsf;
}

static int h264bsf_input(void* param, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes)
{
	int r;
	uint8_t nalt;
	struct h264bsf_t* bsf;
	bsf = (struct h264bsf_t*)param;

	if (bytes > 3 && 0x00 == nalu[0] && 0x00 == nalu[1] && (0x01 == nalu[2] || (0x00 == nalu[2] && 0x01 == nalu[3])))
	{
		nalu += 0x01 == nalu[2] ? 3 : 4;
		bytes -= 0x01 == nalu[2] ? 3 : 4;
	}

	if (bsf->vcl && (dts != bsf->dts || 0 == bytes || h264_is_new_access_unit(nalu, bytes)))
	{
		bsf->dts = avdtsinfer_update(&bsf->infer, 1 == bsf->vcl ? 1 : 0, bsf->pts, pts);

		r = bsf->onpacket(bsf->param, bsf->pts, bsf->dts, bsf->ptr.ptr, (int)bsf->ptr.len, 1==bsf->vcl ? 0x01 : 0);
		bsf->ptr.len = 0;
		bsf->sps_pps_flag = 0;
		bsf->vcl = 0;
		if (0 != r)
			return r;
	}
	
	if (bytes < 1)
		return 0;

	nalt = nalu[0] & 0x1f;
	switch (nalt)
	{
	case H264_NAL_SPS:
	case H264_NAL_PPS:
		//data[k++] = 0; // SPS/PPS add zero_byte(ITU H.264 B.1.2 Byte stream NAL unit semantics)
		bsf->sps_pps_flag = 1;
		break;

	case H264_NAL_IDR:
		// insert SPS/PPS before IDR frame
		if (0 == bsf->sps_pps_flag)
		{
			bsf->sps_pps_flag = 1; // don't insert more than one-times
			// write sps/pps at first
			cbuffer_insert(&bsf->ptr, 0, bsf->extra, bsf->extra_bytes);
		}
		bsf->vcl = 1;
		break;

#if defined(H2645_FILTER_AUD)
	case H264_NAL_AUD:
		return 0; // filter AUD
#endif

	default:
		if (nalt > 0 && nalt < H264_NAL_IDR)
		{
			bsf->vcl = 2;
			bsf->sps_pps_flag = 0; // clear sps/pps flags on VCL NAL
		}
	}

	if (cbuffer_append(&bsf->ptr, startcode, sizeof(startcode)) <= 0
		|| cbuffer_append(&bsf->ptr, nalu, bytes) <= 0 )
		return -(__ERROR__ + ENOMEM);

	bsf->pts = pts;
	bsf->dts = dts;
	return 0;
}

struct avbsf_t* avbsf_h264(void)
{
	static struct avbsf_t bsf = {
		h264bsf_create,
		h264bsf_destroy,
		h264bsf_input,
	};
	return &bsf;
}
