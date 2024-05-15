#include "audio-decoder.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

// webrtc/modules/audio_coding/codecs/g711/g711.h

#define ALAW_AMI_MASK	0x55
#define ULAW_BIAS		0x84  /* Bias for linear code. */

struct g711_decoder_t
{
	struct avframe_t frame;
	int16_t* samples;
	unsigned long bytes;
	int alaw;
};

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

static inline int16_t alaw_to_linear(uint8_t alaw) {
	int i;
	int seg;
	alaw ^= ALAW_AMI_MASK;
	i = ((alaw & 0x0F) << 4);
	seg = (((int)alaw & 0x70) >> 4);
	if (seg)
		i = (i + 0x108) << (seg - 1);
	else
		i += 8;
	return (int16_t)((alaw & 0x80) ? i : -i);
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

static inline int16_t ulaw_to_linear(uint8_t ulaw) 
{
	int t;
	/* Complement to obtain normal u-law value. */
	ulaw = ~ulaw;
	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = (((ulaw & 0x0F) << 3) + ULAW_BIAS) << (((int)ulaw & 0x70) >> 4);
	return (int16_t)((ulaw & 0x80) ? (ULAW_BIAS - t) : (t - ULAW_BIAS));
}

static void* g711a_create(int format, int channels, int samplerate, const void* extradata, int extradata_size)
{
	struct g711_decoder_t* dec;

	assert(PCM_SAMPLE_FMT_S16 == format && 1 == channels && 8000 == samplerate);
	(void)extradata, (void)extradata_size;
	dec = (struct g711_decoder_t*)malloc(sizeof(*dec));
	if (NULL == dec)
		return NULL;

	memset(dec, 0, sizeof(*dec));
	dec->alaw = 1;
	dec->frame.format = PCM_SAMPLE_FMT_S16;
	dec->frame.channels = 1;
	dec->frame.sample_rate = 8000;
	dec->frame.sample_bits = PCM_SAMPLE_BITS(dec->frame.format);
	return dec;
}

static void* g711u_create(int format, int channels, int samplerate, const void* extradata, int extradata_size)
{
	struct g711_decoder_t* dec;
	dec = (struct g711_decoder_t*)g711a_create(format, channels, samplerate, extradata, extradata_size);
	if (dec)
		dec->alaw = 0;
	return dec;
}

static void g711_destroy(void* audio)
{
	struct g711_decoder_t* dec;
	dec = (struct g711_decoder_t*)audio;
	if (dec->samples)
	{
		free(dec->samples);
		dec->samples = NULL;
	}
	free(dec);
}

/// @return >0-got a packet, 0-need more data, <0-error
static int g711_decode(void* audio, const struct avpacket_t* pkt)
{
	int i;
	void* p;
	struct g711_decoder_t* dec;
	dec = (struct g711_decoder_t*)audio;

	if (dec->bytes < pkt->size * sizeof(int16_t))
	{
		p = realloc(dec->samples, pkt->size * sizeof(int16_t));
		if (!p)
			return -ENOMEM;
		dec->samples = (int16_t*)p;
		dec->bytes = pkt->size * sizeof(int16_t);
	}

	if (dec->alaw)
	{
		for (i = 0; i < pkt->size; i++)
			dec->samples[i] = alaw_to_linear(pkt->data[i]);
	}
	else
	{
		for (i = 0; i < pkt->size; i++)
			dec->samples[i] = ulaw_to_linear(pkt->data[i]);
	}

	dec->frame.pts = pkt->pts;
	dec->frame.dts = pkt->dts;
	dec->frame.samples = pkt->size;
	dec->frame.linesize[0] = dec->frame.samples * dec->frame.channels * dec->frame.sample_bits / 8;
	return 1;
}

/// Get decode audio frame
/// @param[out] frame alloc memory internal, use avframe_release to free memory
/// @return >0-got a packet, 0-no packet, <0-error
static int g711_getframe(void* audio, struct avframe_t** frame)
{
	struct g711_decoder_t* dec;
	dec = (struct g711_decoder_t*)audio;
	if (dec->frame.samples > 0)
	{
		*frame = avframe_alloc(dec->frame.linesize[0]);
		if (NULL == *frame)
			return -ENOMEM;

		// audio interleave samples
		memcpy((*frame)->data[0], dec->samples, dec->frame.linesize[0]);
		(*frame)->linesize[0] = dec->frame.linesize[0];
		(*frame)->samples = dec->frame.samples;
		(*frame)->pts = dec->frame.pts;
		(*frame)->dts = dec->frame.dts;
		(*frame)->format = dec->frame.format;
		(*frame)->channels = dec->frame.channels;
		(*frame)->sample_bits = dec->frame.sample_bits;
		(*frame)->sample_rate = dec->frame.sample_rate;

		dec->frame.samples = 0; // clear
		return 1;
	}
	return 0;
}

struct audio_decoder_t* g711a_decoder(void)
{
	static struct audio_decoder_t s_decoder = {
		g711a_create,
		g711_destroy,
		g711_decode,
		g711_getframe,
	};
	return &s_decoder;
}

struct audio_decoder_t* g711u_decoder(void)
{
	static struct audio_decoder_t s_decoder = {
		g711u_create,
		g711_destroy,
		g711_decode,
		g711_getframe,
	};
	return &s_decoder;
}
