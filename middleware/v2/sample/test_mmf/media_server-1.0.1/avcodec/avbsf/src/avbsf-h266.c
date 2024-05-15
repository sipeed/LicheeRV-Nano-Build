#include "avbsf.h"
#include "cbuffer.h"
#include "mpeg4-vvc.h"
#include "avdtsinfer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define H266_NAL_IDR_W_RADL	7
#define H266_NAL_RSV_IRAP	11
#define H266_NAL_OPI		12
#define H266_NAL_DCI		13
#define H266_NAL_VPS		14
#define H266_NAL_SPS		15
#define H266_NAL_PPS		16
#define H266_NAL_PREFIX_APS	17
#define H266_NAL_SUFFIX_APS	18
#define H266_NAL_PH			19
#define H266_NAL_AUD		20
#define H266_NAL_PREFIX_SEI 23
#define H266_NAL_SUFFIX_SEI 24

static const uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };

struct h266bsf_t
{
	struct mpeg4_vvc_t vvc;
	struct cbuffer_t ptr;
	int vps_sps_pps_flag;
	int vcl;
	int64_t pts;
	int64_t dts;
	struct avdtsinfer_t infer;

	uint8_t extra[4 * 1024];
	int extra_bytes;

	avbsf_onpacket onpacket;
	void* param;
};

static int h266bsf_destroy(void** pp)
{
	struct h266bsf_t* bsf;
	if (pp && *pp)
	{
		bsf = (struct h266bsf_t*)*pp;
		cbuffer_free(&bsf->ptr);
		free(bsf);
		*pp = NULL;
	}
	return 0;
}

static void* h266bsf_create(const uint8_t* extra, int bytes, avbsf_onpacket onpacket, void* param)
{
	struct h266bsf_t* bsf;
	bsf = calloc(1, sizeof(*bsf));
	if (!bsf) return NULL;

	avdtsinfer_reset(&bsf->infer);
	cbuffer_init(&bsf->ptr);
	bsf->vps_sps_pps_flag = 0;

	if (bytes > 5 && bytes <= sizeof(bsf->extra) && 0x00 == extra[0] && 0x00 == extra[1] && (0x01 == extra[2] || (0x00 == extra[2] && 0x01 == extra[3])))
	{
		memcpy(bsf->extra, extra, bytes);
		bsf->extra_bytes = bytes;
	}
	else if (0 == mpeg4_vvc_decoder_configuration_record_load(extra, bytes, &bsf->vvc))
	{
		bsf->extra_bytes = mpeg4_vvc_to_nalu(&bsf->vvc, bsf->extra, sizeof(bsf->extra));
		if (bsf->extra_bytes < 0)
			bsf->extra_bytes = 0;
	}

	bsf->onpacket = onpacket;
	bsf->param = param;
	return bsf;
}

static int h266bsf_input(void* param, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes)
{
	int r;
	uint8_t nalt;
	uint8_t irap;
	struct h266bsf_t* bsf;
	bsf = (struct h266bsf_t*)param;

	if (bytes > 3 && 0x00 == nalu[0] && 0x00 == nalu[1] && (0x01 == nalu[2] || (0x00 == nalu[2] && 0x01 == nalu[3])))
	{
		nalu += 0x01 == nalu[2] ? 3 : 4;
		bytes -= 0x01 == nalu[2] ? 3 : 4;
	}

	if (bsf->vcl && (dts != bsf->dts || 0 == bytes || h266_is_new_access_unit(nalu, bytes)))
	{
		bsf->dts = avdtsinfer_update(&bsf->infer, 1 == bsf->vcl ? 1 : 0, bsf->pts, pts);

		r = bsf->onpacket(bsf->param, bsf->pts, bsf->dts, bsf->ptr.ptr, (int)bsf->ptr.len, 1 == bsf->vcl ? 0x01 : 0);
		bsf->ptr.len = 0;
		bsf->vps_sps_pps_flag = 0;
		bsf->vcl = 0;
		if (0 != r)
			return r;
	}

	if (bytes < 2)
		return 0;

	nalt = (nalu[1] >> 3) & 0x1F;
	switch (nalt)
	{
	case H266_NAL_VPS:
	case H266_NAL_SPS:
	case H266_NAL_PPS:
		bsf->vps_sps_pps_flag = 1;
		break;

#if defined(H2645_FILTER_AUD)
	case H266_NAL_AUD:
		return 0; // ignore AUD
#endif

	case H266_NAL_PREFIX_SEI:
	case H266_NAL_SUFFIX_SEI:
		// fallthough

	default:
		irap = H266_NAL_IDR_W_RADL <= nalt && nalt <= H266_NAL_RSV_IRAP;
		if (irap && 0 == bsf->vps_sps_pps_flag)
		{
			bsf->vps_sps_pps_flag = 1; // don't insert more than one-times
			// write vps/sps/pps at first
			cbuffer_insert(&bsf->ptr, 0, bsf->extra, bsf->extra_bytes);
		}
	}

	if (cbuffer_append(&bsf->ptr, startcode, sizeof(startcode)) <= 0
		|| cbuffer_append(&bsf->ptr, nalu, bytes) <= 0)
		return -(__ERROR__ + ENOMEM);

	bsf->pts = pts;
	bsf->dts = dts;
	if (nalt < H266_NAL_OPI)
		bsf->vcl = (H266_NAL_IDR_W_RADL <= nalt && nalt <= H266_NAL_RSV_IRAP) ? 1 : 2;
	return 0;
}

struct avbsf_t* avbsf_h266(void)
{
	static struct avbsf_t bsf = {
		h266bsf_create,
		h266bsf_destroy,
		h266bsf_input,
	};
	return &bsf;
}
