#include "avbsf.h"
#include "cbuffer.h"
#include "mpeg4-hevc.h"
#include "avdtsinfer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define H265_NAL_VPS		32
#define H265_NAL_SPS		33
#define H265_NAL_PPS		34
#define H265_NAL_AUD		35 // Access unit delimiter
#define H265_NAL_SEI_PREFIX	39
#define H265_NAL_SEI_SUFFIX	40

static const uint8_t startcode[] = { 0x00, 0x00, 0x00, 0x01 };

struct h265bsf_t
{
	struct mpeg4_hevc_t hevc;
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

static int h265bsf_destroy(void** pp)
{
	struct h265bsf_t* bsf;
	if (pp && *pp)
	{
		bsf = (struct h265bsf_t*)*pp;
		cbuffer_free(&bsf->ptr);
		free(bsf);
		*pp = NULL;
	}
	return 0;
}

static void* h265bsf_create(const uint8_t* extra, int bytes, avbsf_onpacket onpacket, void* param)
{
	struct h265bsf_t* bsf;
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
	else if (0 == mpeg4_hevc_decoder_configuration_record_load(extra, bytes, &bsf->hevc))
	{
		bsf->extra_bytes = mpeg4_hevc_to_nalu(&bsf->hevc, bsf->extra, sizeof(bsf->extra));
		if (bsf->extra_bytes < 0)
			bsf->extra_bytes = 0;
	}

	bsf->onpacket = onpacket;
	bsf->param = param;
	return bsf;
}

static int h265bsf_input(void* param, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes)
{
	int r;
	uint8_t nalt;
	uint8_t irap;
	struct h265bsf_t* bsf;
	bsf = (struct h265bsf_t*)param;

	if (bytes > 3 && 0x00 == nalu[0] && 0x00 == nalu[1] && (0x01 == nalu[2] || (0x00 == nalu[2] && 0x01 == nalu[3])))
	{
		nalu += 0x01 == nalu[2] ? 3 : 4;
		bytes -= 0x01 == nalu[2] ? 3 : 4;
	}

	if (bsf->vcl && (dts != bsf->dts || 0 == bytes || h265_is_new_access_unit(nalu, bytes)))
	{
		bsf->dts = avdtsinfer_update(&bsf->infer, 1 == bsf->vcl ? 1 : 0, bsf->pts, pts);

		r = bsf->onpacket(bsf->param, bsf->pts, bsf->dts, bsf->ptr.ptr, (int)bsf->ptr.len, 1 == bsf->vcl ? 0x01 : 0);
		bsf->ptr.len = 0;
		bsf->vps_sps_pps_flag = 0;
		bsf->vcl = 0;
		if (0 != r)
			return r;
	}

	if (bytes < 1)
		return 0;

	nalt = (nalu[0] >> 1) & 0x3F;
	switch (nalt)
	{
	case H265_NAL_VPS:
	case H265_NAL_SPS:
	case H265_NAL_PPS:
		bsf->vps_sps_pps_flag = 1;
		break;

#if defined(H2645_FILTER_AUD)
	case H265_NAL_AUD:
		return 0; // ignore AUD
#endif

	case H265_NAL_SEI_PREFIX:
	case H265_NAL_SEI_SUFFIX:
		// fallthough

	default:
		irap = 16 <= nalt && nalt <= 23;
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
	if (nalt < H265_NAL_VPS)
		bsf->vcl = (16 <= nalt && nalt <= 23) ? 1 : 2;
	return 0;
}

struct avbsf_t* avbsf_h265(void)
{
	static struct avbsf_t bsf = {
		h265bsf_create,
		h265bsf_destroy,
		h265bsf_input,
	};
	return &bsf;
}
