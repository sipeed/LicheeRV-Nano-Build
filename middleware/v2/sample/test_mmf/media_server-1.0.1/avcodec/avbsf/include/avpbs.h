#ifndef _avpbs_h_
#define _avpbs_h_

#include <stdint.h>
#include "avpacket.h"
#include "avcodecid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*avpbs_onpacket)(void* param, struct avpacket_t* pkt);

// avpacket bitstream helper
struct avpbs_t
{
	void* (*create)(int stream, AVPACKET_CODEC_ID codec, const uint8_t* extra, int bytes, avpbs_onpacket onpacket, void* param);
	int (*destroy)(void** ppbs);

	int (*input)(void* pbs, int64_t pts, int64_t dts, const uint8_t* data, int bytes, int flags);
};

struct avpbs_t* avpbs_find(AVPACKET_CODEC_ID codec);

#ifdef __cplusplus
}

// AVPacket Bitstream
class AVPBitStream
{
public:
	static AVPBitStream* Create(int stream, AVPACKET_CODEC_ID codec, const void* extra, int bytes, avpbs_onpacket onpacket, void* param)
	{
		avpbs_t* bs = avpbs_find(codec);
		if (!bs)
			return NULL;

		void* ptr = bs->create(stream, codec, (const uint8_t*)extra, bytes, onpacket, param);
		if (!ptr)
			return NULL;

		return new AVPBitStream(bs, ptr);
	}

public:
	~AVPBitStream()
	{
		if (m_bs && m_stream)
		{
			m_bs->destroy(&m_stream);
			m_bs = NULL;
		}
	}

	int Input(int64_t pts, int64_t dts, const void* data, int bytes, int flags)
	{
		return m_bs->input(m_stream, pts, dts, (const uint8_t*)data, bytes, flags);
	}

private:
	AVPBitStream(avpbs_t* bs, void* stream) : m_bs(bs), m_stream(stream) {}
	AVPBitStream(AVPBitStream&) {}
	AVPBitStream& operator= (AVPBitStream&) { return *this; }

private:
	avpbs_t* m_bs;
	void* m_stream;
};

#endif
#endif /* !_avpbs_h_ */
