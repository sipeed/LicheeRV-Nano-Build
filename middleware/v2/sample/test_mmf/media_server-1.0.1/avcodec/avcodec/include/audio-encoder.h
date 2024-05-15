#ifndef _audio_encoder_h_
#define _audio_encoder_h_

#include "avframe.h"
#include "avpacket.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    AUDIO_BITRATE_CBR = 0,
    AUDIO_BITRATE_VBR,
};

struct audio_parameter_t
{
	int format; // enum pcm_sample_format
	int channels; // 1, 2
	int samplerate; // 8000, 16000, 24000, 32000, 44100, 48000

	int profile;
	int level;

	int bitrate;
	int bitrate_mode; // CBR/VBR

    int min_bitrate_kbps; // vbr only
    int max_bitrate_kbps; // vbr only
};

struct audio_encoder_t
{
	void* (*create)(const struct audio_parameter_t* param);
	void (*destroy)(void* audio);

	/// pic->flags & AVPACKET_FLAG_KEY => force IDR
	/// @return >0-got a packet, 0-need more data, <0-error
	int (*encode)(void* audio, const struct avframe_t* frame);

	/// @return >0-got a packet, 0-no packet, <0-error
	int (*getpacket)(void* audio, struct avpacket_t* pkt);
};

struct audio_encoder_t* faac_encoder(void);
struct audio_encoder_t* opus_encoder(void);
struct audio_encoder_t* mp2lame_encoder(void);
struct audio_encoder_t* g711a_encoder(void);
struct audio_encoder_t* g711u_encoder(void);

#ifdef __cplusplus
}
#endif
#endif /* !_audio_encoder_h_ */
