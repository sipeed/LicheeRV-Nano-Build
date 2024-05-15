#ifndef _avpacket_h_
#define _avpacket_h_

#include <stdint.h>
#include "avstream.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AVPACKET_FLAG_KEY		0x0001
#define AVPACKET_FLAG_LOST		0x0100 // some packets lost before the packet
#define AVPACKET_FLAG_CORRUPT	0x0200 // the packet data is corrupt

struct avpacket_t
{
	uint8_t* data;
	int size;

	int64_t pts;
	int64_t dts;

	struct avstream_t* stream;
	int flags; // AVPACKET_FLAG_XXX

	void* opaque; // internal use only
};

///@param[in] size alloc packet data size, don't include sizeof(struct avpacket_t)
///@return alloc new avpacket_t, use avpacket_release to free memory
struct avpacket_t* avpacket_alloc(int size);
int32_t avpacket_addref(struct avpacket_t* pkt);
int32_t avpacket_release(struct avpacket_t* pkt);

#ifdef __cplusplus
}
#endif
#endif /* !_avpacket_h_ */
