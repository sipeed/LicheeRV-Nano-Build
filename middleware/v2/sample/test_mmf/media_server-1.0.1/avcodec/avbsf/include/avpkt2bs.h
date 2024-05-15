#ifndef _avpkt2bs_h_
#define _avpkt2bs_h_

#include <stddef.h>
#include <stdint.h>
#include "avpacket.h"
#include "mpeg4-aac.h"
#include "mpeg4-avc.h"
#include "mpeg4-vvc.h"
#include "mpeg4-hevc.h"
#include "mp3-header.h"
#include "opus-head.h"
#include "aom-av1.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct avpkt2bs_t
{
	union
	{
		struct mpeg4_aac_t aac;
		struct opus_head_t opus;
	} a;

	union
	{
		struct aom_av1_t av1;
		struct mpeg4_avc_t avc;
		struct mpeg4_hevc_t hevc;
		struct mpeg4_vvc_t vvc;
	} v;

	uint8_t* ptr;
	int cap;
};


int avpkt2bs_create(struct avpkt2bs_t* bs);
int avpkt2bs_destroy(struct avpkt2bs_t* bs);

/// @return <0-error, >0-bytes
int avpkt2bs_input(struct avpkt2bs_t* bs, const struct avpacket_t* pkt);

#if defined(__cplusplus)

struct AVPacket2BitStream
{
public:
	AVPacket2BitStream()
	{
		avpkt2bs_create(&m_bs);
	}

	~AVPacket2BitStream()
	{ 
		avpkt2bs_destroy(&m_bs);
	}

public:
	int Input(const struct avpacket_t* pkt)
	{
		return avpkt2bs_input(&m_bs, pkt);
	}

	// get ptr
	operator const void* () const {
		return m_bs.ptr;
	}

private:
	AVPacket2BitStream(AVPacket2BitStream&) {}
	AVPacket2BitStream& operator= (AVPacket2BitStream&) { return *this; }

private:
	avpkt2bs_t m_bs;
};
}
#endif
#endif /* !_avpkt2bs_h_ */
