#ifndef _audio_decoder_h_
#define _audio_decoder_h_

#include "audio-encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

struct audio_decoder_t
{
	void* (*create)(int format, int channels, int samplerate, const void* extradata, int extradata_size);
	void (*destroy)(void* audio);

	/// @return >0-got a packet, 0-need more data, <0-error
	int (*decode)(void* audio, const struct avpacket_t* pkt);

	/// Get decode audio frame
	/// @param[out] frame alloc memory internal, use avframe_release to free memory
	/// @return >0-got a packet, 0-no packet, <0-error
	int (*getframe)(void* audio, struct avframe_t** frame);
};

struct audio_decoder_t* fdk_decoder(void);
struct audio_decoder_t* faac_decoder(void);
struct audio_decoder_t* opus_decoder(void);
struct audio_decoder_t* g711a_decoder(void);
struct audio_decoder_t* g711u_decoder(void);

#ifdef __cplusplus
}
#endif
#endif /* !_audio_decoder_h_ */
