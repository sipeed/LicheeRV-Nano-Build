#ifndef _audio_resampler_h_
#define _audio_resampler_h_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* audio_resampler_t;

enum audio_resampler_format_t
{ 
	// interleaved samples: [ L|R L|R L|R ...]
	AUDIO_RESAMPLE_AUDIO_INTERLEAVED_FLOAT32 = 0, 
	AUDIO_RESAMPLE_AUDIO_INTERLEAVED_FLOAT64,
	AUDIO_RESAMPLE_AUDIO_INTERLEAVED_INT32,
	AUDIO_RESAMPLE_AUDIO_INTERLEAVED_INT16,

	// split/planar samples: [ LLL... | RRR... ]
	AUDIO_RESAMPLE_AUDIO_PLANAR_FLOAT32 = 4,
	AUDIO_RESAMPLE_AUDIO_PLANAR_FLOAT64,
	AUDIO_RESAMPLE_AUDIO_PLANAR_INT32,
	AUDIO_RESAMPLE_AUDIO_PLANAR_INT16,
};

audio_resampler_t audio_resampler_create(int channels, int input_rate, enum audio_resampler_format_t input_format, int output_rate, enum audio_resampler_format_t output_format);
int audio_resampler_destroy(audio_resampler_t resampler);

/// @param[in] samples input data length(samples per channel)
/// @param[in] len output samples length(samples per channel)
/// @return >0-output samples length, <0-error, =0-reserved
int audio_resampler_process(audio_resampler_t resampler, const void* data, int samples, void* out, int len);

#ifdef __cplusplus
}
#endif
#endif /* !_audio_resampler_h_ */
