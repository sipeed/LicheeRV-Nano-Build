#ifndef _avbsf_h_
#define _avbsf_h_

#include <stdint.h>
#include "avcodecid.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @param[in] flags 0x01-keyframe, other-undefined
/// @return 0-ok, other-error
typedef int (*avbsf_onpacket)(void* param, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags);

struct avbsf_t
{
	void* (*create)(const uint8_t* extra, int bytes, avbsf_onpacket onpacket, void* param);
	int (*destroy)(void** pbsf);

	int (*input)(void* bsf, int64_t pts, int64_t dts, const uint8_t* nalu, int bytes);
};

struct avbsf_t* avbsf_find(AVPACKET_CODEC_ID codec);

#ifdef __cplusplus
}

// AV Bitstream Filter
class AVBitStreamFilter
{
public:
	static AVBitStreamFilter* Create(AVPACKET_CODEC_ID codec, const void* extra, int bytes, avbsf_onpacket onpacket, void* param)
	{
		avbsf_t* bsf = avbsf_find(codec);
		if (!bsf)
			return NULL;

		void* ptr = bsf->create((const uint8_t*)extra, bytes, onpacket, param);
		if (!ptr)
			return NULL;

		return new AVBitStreamFilter(bsf, ptr);
	}

public:
	~AVBitStreamFilter()
	{
		if (m_bsf && m_ptr)
		{
			m_bsf->destroy(&m_ptr);
			m_bsf = NULL;
		}
	}

	int Input(int64_t pts, int64_t dts, const uint8_t* nalu, int bytes)
	{
		return m_bsf->input(m_ptr, pts, dts, nalu, bytes);
	}

private:
	AVBitStreamFilter(avbsf_t* bsf, void* ptr) : m_bsf(bsf), m_ptr(ptr) {}
	AVBitStreamFilter(AVBitStreamFilter&) {}
	AVBitStreamFilter& operator= (AVBitStreamFilter&) { return *this; }

private:
	avbsf_t* m_bsf;
	void* m_ptr;
};
#endif
#endif /* !_avbsf_h_ */
