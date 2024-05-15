#include "audio-mixer.h"

void audio_mixer_s16(int16_t* dst, const int16_t* src, float mul, int len)
{
    int i;
    for (i = 0; i < len; i++)
        dst[i] += (int16_t)((float)src[i] * mul);
}

void audio_mixer_float(float* dst, const float* src, float mul, int len)
{
    int i;
    for (i = 0; i < len; i++)
        dst[i] += src[i] * mul;
}

void audio_mixer_double(double* dst, const double* src, double mul, int len)
{
    int i;
    for (i = 0; i < len; i++)
        dst[i] += src[i] * mul;
}
