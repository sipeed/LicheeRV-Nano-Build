#include "audio-encoder.h"
#if defined(_AVCODEC_TWOLAME_)
#include "twolame.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct mp2lame_encoder_t
{
    int format;
    twolame_options* opts;
    struct avpacket_t pkt;
    size_t capacity;
};

static void mp2lame_destroy(void* audio)
{
    struct mp2lame_encoder_t* enc;
    enc = (struct mp2lame_encoder_t*)audio;
    twolame_close(&enc->opts);
    free(enc);
}

static void* mp2lame_create(const struct audio_parameter_t* param)
{
    int r = 0;
    struct mp2lame_encoder_t* enc;

    // check channels and format
    if (param->channels > 2 || (PCM_SAMPLE_FMT_S16 != param->format && PCM_SAMPLE_FMT_S16P != param->format && PCM_SAMPLE_FMT_FLOAT != param->format && PCM_SAMPLE_FMT_FLTP != param->format))
        return NULL;

    // TWOLAME_SAMPLES_PER_FRAME
    // samples/1152 * sizeof(frame) < mp2buffer_size
    r = param->samplerate / 2 * param->channels * PCM_SAMPLE_BITS(param->format) / 8; // 500ms
    enc = (struct mp2lame_encoder_t*)malloc(sizeof(*enc) + r);
    if (NULL == enc)
        return NULL;

    memset(enc, 0, sizeof(*enc));
    enc->format = param->format;
    //enc->pkt.codecid = AVCODEC_AUDIO_MP2;
    enc->pkt.data = (uint8_t*)(enc + 1);
    enc->capacity = r;
    enc->opts = twolame_init();

    //if (!param->bitrate)
    //    param->bitrate = param->samplerate < 28000 ? 160000 : 384000;

//  twolame_set_verbosity(enc->opts, 0);
//  twolame_set_version(enc->opts, TWOLAME_MPEG2);
//  twolame_set_bitrate(enc->opts, param->bitrate/1000); // default 192
    twolame_set_in_samplerate(enc->opts, param->samplerate);
//  twolame_set_out_samplerate(enc->opts, param->samplerate); // use input samplerate
    twolame_set_num_channels(enc->opts, param->channels); // default 2
    twolame_set_VBR(enc->opts, AUDIO_BITRATE_VBR == param->bitrate_mode);

    r = twolame_init_params(enc->opts);
    if (0 != r)
    {
        mp2lame_destroy(enc);
        return NULL;
    }
    return enc;
}

static int mp2lame_input(void* audio, const struct avframe_t* pic)
{
    struct mp2lame_encoder_t* enc;
    enc = (struct mp2lame_encoder_t*)audio;
    switch (enc->format)
    {
    case PCM_SAMPLE_FMT_S16:
        enc->pkt.size = twolame_encode_buffer_interleaved(enc->opts, (const short int*)pic->data[0], pic->samples, enc->pkt.data, (int)enc->capacity);
        break;
    case PCM_SAMPLE_FMT_S16P:
        enc->pkt.size = twolame_encode_buffer(enc->opts, (const short int*)pic->data[0], (const short int*)pic->data[1], pic->samples, enc->pkt.data, (int)enc->capacity);
        break;
    case PCM_SAMPLE_FMT_FLOAT:
        enc->pkt.size = twolame_encode_buffer_float32_interleaved(enc->opts, (const float*)pic->data[0], pic->samples, enc->pkt.data, (int)enc->capacity);
        break;
    case PCM_SAMPLE_FMT_FLTP:
        enc->pkt.size = twolame_encode_buffer_float32(enc->opts, (const float*)pic->data[0], (const float*)pic->data[1], pic->samples, enc->pkt.data, (int)enc->capacity);
        break;
    default:
        assert(0);
        return -1;
    }

    if (enc->pkt.size > 0)
    {
        enc->pkt.flags = 0;
        enc->pkt.pts = pic->pts;
        enc->pkt.dts = pic->dts;
    }
    return enc->pkt.size;
}

static int mp2lame_getpacket(void* audio, struct avpacket_t* pkt)
{
    struct mp2lame_encoder_t* enc;
    enc = (struct mp2lame_encoder_t*)audio;
    if (enc->pkt.size > 0)
    {
        memcpy(pkt, &enc->pkt, sizeof(*pkt));
        enc->pkt.size = 0; // clear
        return 0;
    }
    return -1;
}

struct audio_encoder_t* mp2lame_encoder(void)
{
    static struct audio_encoder_t s_encoder = {
        mp2lame_create,
        mp2lame_destroy,
        mp2lame_input,
        mp2lame_getpacket,
    };
    return &s_encoder;
}

#endif /* _AVCODEC_TWOLAME_ */
