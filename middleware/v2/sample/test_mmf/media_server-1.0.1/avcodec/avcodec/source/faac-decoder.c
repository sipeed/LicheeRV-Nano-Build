#include "audio-decoder.h"
#if defined(_AVCODEC_FAAC_)
#include "neaacdec.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct faad_decoder_t
{
	NeAACDecHandle* faad;
	NeAACDecFrameInfo info;
	struct avframe_t frame;
	void* samples;
	unsigned long bytes;

	int profile, frequency, channel;
};

static void faad_destroy(void* audio)
{
	struct faad_decoder_t* dec;
	dec = (struct faad_decoder_t*)audio;
	if (dec->faad)
	{
		NeAACDecClose(dec->faad);
		dec->faad = NULL;
	}
	free(dec);
}

static void* faad_create(int format, int channels, int frequency, const void* extradata, int extradata_size)
{
	struct faad_decoder_t* dec;

	(void)extradata, (void)extradata_size;
	dec = (struct faad_decoder_t*)malloc(sizeof(*dec));
	if (NULL == dec)
		return NULL;

	memset(dec, 0, sizeof(*dec));
	dec->frame.format = format;
	dec->frame.channels = channels;
	dec->frame.sample_rate = frequency;
	dec->frame.sample_bits = PCM_SAMPLE_BITS(format);
	return dec;
}

static int aac_adts_parse(const uint8_t* data, int bytes, int* profile, int* frequency, int* channel)
{
	if (bytes >= 7 && 0xFF == data[0] && 0xF0 == (data[1] & 0xF0)) /* syncword */
	{
		*profile = ((data[2] >> 6) & 0x03) + 1; // 2 bits: the MPEG-2 Audio Object Type add 1
		*frequency = (data[2] >> 2) & 0x0F; // 4 bits: MPEG-4 Sampling Frequency Index (15 is forbidden)
		*channel = ((data[2] & 0x01) << 2) | ((data[3] >> 6) & 0x03); // 3 bits: MPEG-4 Channel Configuration 
		return 0;
	}
	return -1;
}

static unsigned char pcm_format_to_faad_format(int format)
{
	switch (format)
	{
	case PCM_SAMPLE_FMT_S16: return FAAD_FMT_16BIT;
	case PCM_SAMPLE_FMT_S32: return FAAD_FMT_32BIT;
	case PCM_SAMPLE_FMT_FLOAT: return FAAD_FMT_FLOAT;
	case PCM_SAMPLE_FMT_DOUBLE: return FAAD_FMT_DOUBLE;
	default: return 0;
	}
}

static int faad_format_to_pcm_format(unsigned char format)
{
	switch (format)
	{
	case FAAD_FMT_16BIT: return PCM_SAMPLE_FMT_S16;
	case FAAD_FMT_32BIT: return PCM_SAMPLE_FMT_S32;
	case FAAD_FMT_FLOAT: return PCM_SAMPLE_FMT_FLOAT;
	case FAAD_FMT_DOUBLE: return PCM_SAMPLE_FMT_DOUBLE;
	default: return PCM_SAMPLE_FMT_NONE;
	}
}

static int faad_apply_format(NeAACDecHandle* faad, struct faad_decoder_t* dec)
{
	unsigned char fmt;
	NeAACDecConfigurationPtr cfg;

	cfg = NeAACDecGetCurrentConfiguration(faad);
	if (0 == dec->frame.format)
		dec->frame.format = faad_format_to_pcm_format(cfg->outputFormat); // default

	fmt = pcm_format_to_faad_format(dec->frame.format);
	if (0 == fmt)
	{
		// default to S16
		fmt = FAAD_FMT_16BIT;
		dec->frame.format = PCM_SAMPLE_FMT_S16;
	}
	cfg->outputFormat = fmt;
	return NeAACDecSetConfiguration(faad, cfg) ? 0 : -1;
}

static int faad_open(struct faad_decoder_t* dec, const struct avpacket_t* pkt)
{
	unsigned char ch = 0;
	unsigned long samplerate = 0;
	NeAACDecHandle* faad;
	faad = NeAACDecOpen();
	if (NULL == faad)
		return -1;

	if (0 != faad_apply_format(faad, dec) || NeAACDecInit(faad, pkt->data, pkt->size, &samplerate, &ch) < 0)
	{
		NeAACDecClose(faad);
		return -1;
	}

	dec->faad = faad;
	dec->frame.channels = ch;
	dec->frame.sample_rate = (int)samplerate;
	dec->frame.sample_bits = PCM_SAMPLE_BITS(dec->frame.format);
	return 0;
}

static void aac_check(struct faad_decoder_t* dec, const struct avpacket_t* pkt)
{
	int profile, frequency, channel;
	if (0 != aac_adts_parse(pkt->data, pkt->size, &profile, &frequency, &channel))
		return;

	if (profile != dec->profile || frequency != dec->frequency || channel != dec->channel)
	{
		if (dec->faad)
		{
			NeAACDecClose(dec->faad);
			dec->faad = NULL;
		}

		if (0 == faad_open(dec, pkt))
		{
			dec->profile = profile;
			dec->frequency = frequency;
			dec->channel = channel;
		}
	}
}

static int faad_input(void* audio, const struct avpacket_t* pkt)
{
	struct faad_decoder_t* dec;
	dec = (struct faad_decoder_t*)audio;
	aac_check(dec, pkt);
	if (!dec->faad)
		return -1;

	dec->samples = NeAACDecDecode(dec->faad, &dec->info, pkt->data, pkt->size);
	dec->frame.samples = (int)(dec->info.samples / dec->info.channels); // convert to per channel samples
	if (dec->frame.samples > 0)
	{
		assert(dec->frame.channels == dec->info.channels);
		assert((unsigned long)dec->frame.sample_rate == dec->info.samplerate);
		dec->frame.pts = pkt->pts;
		dec->frame.dts = pkt->dts;
		dec->frame.linesize[0] = dec->frame.samples *  dec->frame.channels * dec->frame.sample_bits / 8;
	}
	return dec->frame.samples;
}

static int faad_getframe(void* audio, struct avframe_t** frame)
{
	struct faad_decoder_t* dec;
	dec = (struct faad_decoder_t*)audio;
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

struct audio_decoder_t* faac_decoder(void)
{
	static struct audio_decoder_t s_decoder = {
		faad_create,
		faad_destroy,
		faad_input,
		faad_getframe,
	};
	return &s_decoder;
}

#endif /* _AVCODEC_FAAC_ */
