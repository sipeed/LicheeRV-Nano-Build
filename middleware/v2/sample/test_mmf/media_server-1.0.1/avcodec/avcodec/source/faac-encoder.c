#include "audio-encoder.h"
#if defined(_AVCODEC_FAAC_)
#include "faac.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct faac_encoder_t
{
	faacEncHandle* faac;
	struct avpacket_t pkt;
	size_t capacity;
	uint8_t* sample;
};

static unsigned int pcm_format_to_faac_format(int format)
{
	switch (format)
	{
	case PCM_SAMPLE_FMT_S16: return FAAC_INPUT_16BIT;
	case PCM_SAMPLE_FMT_S32: return FAAC_INPUT_32BIT;
	case PCM_SAMPLE_FMT_FLOAT: return FAAC_INPUT_FLOAT;
	default: return FAAC_INPUT_NULL;
	}
}

static int faac_apply_format(faacEncHandle* faac, int format)
{
	unsigned int fmt;
	faacEncConfigurationPtr cfg;

	fmt = pcm_format_to_faac_format(format);
	if (FAAC_INPUT_NULL == fmt)
		return -1;

	cfg = faacEncGetCurrentConfiguration(faac);
	cfg->inputFormat = fmt;
	cfg->outputFormat = 1; // ADTS
	cfg->useTns = 0;
	cfg->allowMidside = 1;
	//cfg->bitRate = bitrate / channels;
	//cfg->bandWidth = cutoff;
	cfg->aacObjectType = LOW;
	cfg->mpegVersion = MPEG4;
	return faacEncSetConfiguration(faac, cfg) ? 0 : -1;
}

static void faac_destroy(void* audio)
{
	struct faac_encoder_t* enc;
	enc = (struct faac_encoder_t*)audio;
	if (enc->faac)
	{
		faacEncClose(enc->faac);
		enc->faac = NULL;
	}
	free(enc);
}

static void* faac_create(const struct audio_parameter_t* param)
{
	unsigned long bytes = 0;
	unsigned long samples = 0;
	faacEncHandle* faac = NULL;
	struct faac_encoder_t* enc;
	
	if (param->format != PCM_SAMPLE_FMT_S16)
		return NULL; // faac only support FAAC_INPUT_16BIT ???

	faac = faacEncOpen(param->samplerate, param->channels, &samples, &bytes);
	bytes *= 2;
	if (NULL == faac)
		return NULL;

	if (0 != faac_apply_format(faac, param->format))
	{
		faacEncClose(faac);
		return NULL;
	}

	enc = (struct faac_encoder_t*)malloc(sizeof(*enc) + bytes + samples * 4);
	if (NULL == enc)
	{
		faacEncClose(faac);
		return NULL;
	}

	memset(enc, 0, sizeof(*enc));
	//enc->pkt.codecid = AVCODEC_AUDIO_AAC; // mpeg4-aac
	enc->pkt.data = (uint8_t*)(enc + 1);
	enc->sample = enc->pkt.data + bytes;
	enc->capacity = bytes;
	enc->faac = faac;
	return enc;
}

static int faac_input(void* audio, const struct avframe_t* pic)
{
	struct faac_encoder_t* enc;
	enc = (struct faac_encoder_t*)audio;
	enc->pkt.size = faacEncEncode(enc->faac, (int32_t*)pic->data[0], pic->samples * pic->channels, enc->pkt.data, (int)enc->capacity);
	if (enc->pkt.size > 0)
	{
		enc->pkt.flags = 0;
		enc->pkt.pts = pic->pts;
		enc->pkt.dts = pic->dts;
	}
	return enc->pkt.size;
}

static int faac_getpacket(void* audio, struct avpacket_t* pkt)
{
	struct faac_encoder_t* enc;
	enc = (struct faac_encoder_t*)audio;
	if (enc->pkt.size > 0)
	{
		memcpy(pkt, &enc->pkt, sizeof(*pkt));
		enc->pkt.size = 0; // clear
		return 1;
	}
	return 0;
}

struct audio_encoder_t* faac_encoder(void)
{
	static struct audio_encoder_t s_encoder = {
		faac_create,
		faac_destroy,
		faac_input,
		faac_getpacket,
	};
	return &s_encoder;
}

#endif /* _AVCODEC_FAAC_ */
