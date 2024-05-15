#include "audio-encoder.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

//#define ULAW_ZEROTRAP       /* turn on the trap as per the MIL-STD */
#define ULAW_BIAS       0x84  /* Bias for linear code. */
#define ALAW_AMI_MASK   0x55

struct g711_encoder_t
{
    struct avpacket_t pkt;
    int capacity;
    int alaw;
};

static inline int top_bit(unsigned int bits) {
    int i;
    if (bits == 0) {
        return -1;
    }
    i = 0;
    if (bits & 0xFFFF0000) {
        bits &= 0xFFFF0000;
        i += 16;
    }
    if (bits & 0xFF00FF00) {
        bits &= 0xFF00FF00;
        i += 8;
    }
    if (bits & 0xF0F0F0F0) {
        bits &= 0xF0F0F0F0;
        i += 4;
    }
    if (bits & 0xCCCCCCCC) {
        bits &= 0xCCCCCCCC;
        i += 2;
    }
    if (bits & 0xAAAAAAAA) {
        bits &= 0xAAAAAAAA;
        i += 1;
    }
    return i;
}

static inline int bottom_bit(unsigned int bits) {
    int i;
    if (bits == 0) {
        return -1;
    }
    i = 32;
    if (bits & 0x0000FFFF) {
        bits &= 0x0000FFFF;
        i -= 16;
    }
    if (bits & 0x00FF00FF) {
        bits &= 0x00FF00FF;
        i -= 8;
    }
    if (bits & 0x0F0F0F0F) {
        bits &= 0x0F0F0F0F;
        i -= 4;
    }
    if (bits & 0x33333333) {
        bits &= 0x33333333;
        i -= 2;
    }
    if (bits & 0x55555555) {
        bits &= 0x55555555;
        i -= 1;
    }
    return i;
}

/*
 * A-law is basically as follows:
 *
 *      Linear Input Code        Compressed Code
 *      -----------------        ---------------
 *      0000000wxyza             000wxyz
 *      0000001wxyza             001wxyz
 *      000001wxyzab             010wxyz
 *      00001wxyzabc             011wxyz
 *      0001wxyzabcd             100wxyz
 *      001wxyzabcde             101wxyz
 *      01wxyzabcdef             110wxyz
 *      1wxyzabcdefg             111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */

static inline uint8_t linear_to_alaw(int linear) 
{
    int mask;
    int seg;
    if (linear >= 0) {
        /* Sign (bit 7) bit = 1 */
        mask = ALAW_AMI_MASK | 0x80;
    }
    else {
        /* Sign (bit 7) bit = 0 */
        mask = ALAW_AMI_MASK;
        /* WebRtc, tlegrand: Changed from -8 to -1 to get bitexact to reference
         * implementation */
        linear = -linear - 1;
    }
    /* Convert the scaled magnitude to segment number. */
    seg = top_bit(linear | 0xFF) - 7;
    if (seg >= 8) {
        if (linear >= 0) {
            /* Out of range. Return maximum value. */
            return (uint8_t)(0x7F ^ mask);
        }
        /* We must be just a tiny step below zero */
        return (uint8_t)(0x00 ^ mask);
    }
    /* Combine the sign, segment, and quantization bits. */
    return (uint8_t)(((seg << 4) | ((linear >> ((seg) ? (seg + 3) : 4)) & 0x0F)) ^ mask);
}

/*
 * Mu-law is basically as follows:
 *
 *      Biased Linear Input Code        Compressed Code
 *      ------------------------        ---------------
 *      00000001wxyza                   000wxyz
 *      0000001wxyzab                   001wxyz
 *      000001wxyzabc                   010wxyz
 *      00001wxyzabcd                   011wxyz
 *      0001wxyzabcde                   100wxyz
 *      001wxyzabcdef                   101wxyz
 *      01wxyzabcdefg                   110wxyz
 *      1wxyzabcdefgh                   111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
*/
static inline uint8_t linear_to_ulaw(int linear) 
{
    uint8_t u_val;
    int mask;
    int seg;
    /* Get the sign and the magnitude of the value. */
    if (linear < 0) {
        /* WebRtc, tlegrand: -1 added to get bitexact to reference implementation */
        linear = ULAW_BIAS - linear - 1;
        mask = 0x7F;
    }
    else {
        linear = ULAW_BIAS + linear;
        mask = 0xFF;
    }
    seg = top_bit(linear | 0xFF) - 7;
    /*
     * Combine the sign, segment, quantization bits,
     * and complement the code word.
     */
    if (seg >= 8)
        u_val = (uint8_t)(0x7F ^ mask);
    else
        u_val = (uint8_t)(((seg << 4) | ((linear >> (seg + 3)) & 0xF)) ^ mask);
#ifdef ULAW_ZEROTRAP
    /* Optional ITU trap */
    if (u_val == 0)
        u_val = 0x02;
#endif
    return u_val;
}

static void g711_destroy(void* audio)
{
    struct g711_encoder_t* enc;
    enc = (struct g711_encoder_t*)audio;
    if (enc->pkt.data)
    {
        free(enc->pkt.data);
        enc->pkt.data = NULL;
    }
    free(enc);
}

static void* g711a_create(const struct audio_parameter_t* param)
{
    unsigned long bytes = 0;
    unsigned long samples = 0;
    struct g711_encoder_t* enc;

    if (param->format != PCM_SAMPLE_FMT_S16)
        return NULL; // g711 only support PCM_SAMPLE_FMT_S16 ???

    enc = (struct g711_encoder_t*)malloc(sizeof(*enc) + bytes + samples * 4);
    if (NULL == enc)
        return NULL;

    memset(enc, 0, sizeof(*enc));
    //enc->pkt.codecid = AVCODEC_AUDIO_AAC; // mpeg4-aac
    enc->alaw = 1;
    return enc;
}

static void* g711u_create(const struct audio_parameter_t* param)
{
    struct g711_encoder_t* enc;
    enc = (struct g711_encoder_t*)g711a_create(param);
    if (enc)
        enc->alaw = 0;
    return enc;
}

static int g711_encode(void* audio, const struct avframe_t* pic)
{
    int i;
    void* p;
    struct g711_encoder_t* enc;
    enc = (struct g711_encoder_t*)audio;

    assert(PCM_SAMPLE_FMT_S16 == pic->format && 1 == pic->channels && 16 == pic->sample_bits && 8000 == pic->sample_rate);

    if (enc->capacity < pic->samples)
    {
        p = realloc(enc->pkt.data, pic->samples);
        if (!p)
            return -ENOMEM;
        enc->pkt.data = (uint8_t*)p;
        enc->capacity = pic->samples;
    }

    if (enc->alaw)
    {
        for (i = 0; i < pic->samples; i++)
            enc->pkt.data[i] = linear_to_alaw(((int16_t*)pic->data[0])[i]);
    }
    else
    {
        for (i = 0; i < pic->samples; i++)
            enc->pkt.data[i] = linear_to_ulaw(((int16_t*)pic->data[0])[i]);
    }

    enc->pkt.pts = pic->pts;
    enc->pkt.dts = pic->dts;
    enc->pkt.size = pic->samples;
    enc->pkt.flags = 0;
    return enc->pkt.size;
}

static int g711_getpacket(void* audio, struct avpacket_t* pkt)
{
    struct g711_encoder_t* enc;
    enc = (struct g711_encoder_t*)audio;
    if (enc->pkt.size > 0)
    {
        memcpy(pkt, &enc->pkt, sizeof(*pkt));
        enc->pkt.size = 0; // clear
        return 1;
    }
    return 0;
}

struct audio_encoder_t* g711a_encoder(void)
{
    static struct audio_encoder_t s_encoder = {
        g711a_create,
        g711_destroy,
        g711_encode,
        g711_getpacket,
    };
    return &s_encoder;
}

struct audio_encoder_t* g711u_encoder(void)
{
    static struct audio_encoder_t s_encoder = {
        g711u_create,
        g711_destroy,
        g711_encode,
        g711_getpacket,
    };
    return &s_encoder;
}
