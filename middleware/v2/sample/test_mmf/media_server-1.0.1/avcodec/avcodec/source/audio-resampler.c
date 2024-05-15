#include "audio-resampler.h"

#if defined(_AVCODEC_SOXR_)

#include "soxr.h"
#include <assert.h>

audio_resampler_t audio_resampler_create(int channels, int input_rate, enum audio_resampler_format_t input_format, int output_rate, enum audio_resampler_format_t output_format)
{
	soxr_t soxr;
	soxr_error_t err;
	soxr_io_spec_t spec;
	soxr_quality_spec_t quality;
	soxr_runtime_spec_t runtime;

	spec = soxr_io_spec(input_format, output_format);
	quality = soxr_quality_spec(SOXR_HQ, 0);
	runtime = soxr_runtime_spec(1);

	soxr = soxr_create(input_rate, output_rate, channels, &err, &spec, &quality, &runtime);
	return 0 == err ? soxr : NULL;
}

int audio_resampler_destroy(audio_resampler_t resampler)
{
	if (resampler)
		soxr_delete(resampler);
	return 0;
}

int audio_resampler_process(audio_resampler_t resampler, const void* data, int samples, void* out, int len)
{
	size_t n[2];
	soxr_error_t err;
	err = soxr_process(resampler, data, samples, &n[0], out, len, &n[1]);
	if (err)
		return -1;
	assert(samples == n[0]);
	return (int)n[1];
}

#endif /* _AVCODEC_SOXR_ */
