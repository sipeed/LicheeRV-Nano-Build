#ifndef _h264_encoder_h_
#define _h264_encoder_h_

#include "avframe.h"
#include "avpacket.h"
#include "h264-parameter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct h264_encoder_t
{
	void* (*create)(const struct h264_parameter_t* param);
	void (*destroy)(void* h264);

	/// pic->flags & AVPACKET_FLAG_KEY => force IDR
	/// @return >0-ok, other-error
	int (*input)(void* h264, const struct avframe_t* pic);

	/// @return >=0-got packet, <0-error
	int (*getpacket)(void* h264, struct avpacket_t* pkt);
};

struct h264_encoder_t* x264_encoder(void);
struct h264_encoder_t* x262_encoder(void); // MPEG-2 Video
struct h264_encoder_t* openh264_encoder(void);

#ifdef __cplusplus
}
#endif
#endif /* !_h264_encoder_h_ */
