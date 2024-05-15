#include "avbsf.h"
#include "mpeg4-aac.h"
#include "cbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct aacbsf_t
{
	struct mpeg4_aac_t aac;
	struct cbuffer_t ptr;

	avbsf_onpacket onpacket;
	void* param;
};

static int aacbsf_destroy(void** pp)
{
	struct aacbsf_t* bsf;
	if (pp && *pp)
	{
		bsf = (struct aacbsf_t*)*pp;
		cbuffer_free(&bsf->ptr);
		free(bsf);
		*pp = NULL;
	}
	return 0;
}

static void* aacbsf_create(const uint8_t* extra, int bytes, avbsf_onpacket onpacket, void* param)
{
	struct aacbsf_t* bsf;
	bsf = calloc(1, sizeof(*bsf));
	if (!bsf) return NULL;

	cbuffer_init(&bsf->ptr);
	if (mpeg4_aac_audio_specific_config_load(extra, bytes, &bsf->aac) < 0)
	{
		aacbsf_destroy((void**)&bsf);
		return NULL;
	}

	bsf->onpacket = onpacket;
	bsf->param = param;
	return bsf;
}

static int aacbsf_input(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes)
{
	int r;
	struct aacbsf_t* bsf;
	bsf = (struct aacbsf_t*)param;
	cbuffer_resize(&bsf->ptr, bytes + 128);
	r = mpeg4_aac_adts_save(&bsf->aac, bytes, bsf->ptr.ptr, bsf->ptr.cap);
	if (r < 0)
		return r;

	bsf->ptr.len = r;
	r = cbuffer_append(&bsf->ptr, data, bytes);
	if (r < 0)
		return r;

	return bsf->onpacket(bsf->param, pts, dts, bsf->ptr.ptr, (int)bsf->ptr.len, 0);
}

struct avbsf_t* avbsf_aac(void)
{
	static struct avbsf_t bsf = {
		aacbsf_create,
		aacbsf_destroy,
		aacbsf_input,
	};
	return &bsf;
}
