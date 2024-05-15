#include "audio-decoder.h"
//#define _AVCODEC_FDK_ 1
#if defined(_AVCODEC_FDK_)
#include "aacdecoder_lib.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

struct fdk_decoder_t
{
	HANDLE_AACDECODER fdk;
	CStreamInfo info;
	struct avframe_t frame;
	INT_PCM* samples;
	unsigned long bytes;

	int profile, frequency, channel;
};

static void fdk_destroy(void* audio)
{
	struct fdk_decoder_t* dec;
	dec = (struct fdk_decoder_t*)audio;
	if (dec->fdk)
	{
		aacDecoder_Close(dec->fdk);
		dec->fdk = NULL;
	}
	free(dec);
}

static void* fdk_create(int format, int channels, int frequency, const void* extradata, int extradata_size)
{
	struct fdk_decoder_t* dec;

	(void)extradata, (void)extradata_size;
	dec = (struct fdk_decoder_t*)malloc(sizeof(*dec) + sizeof(INT_PCM) * 1024 /*frame*/ * 2 /*SBR*/ * 8 /*channel*/ );
	if (NULL == dec)
		return NULL;

	memset(dec, 0, sizeof(*dec));
	dec->samples = (INT_PCM*)(dec + 1);
	dec->bytes = sizeof(INT_PCM) * 1024 /*frame*/ * 2 /*SBR*/ * 8 /*channel*/;
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

static int fdk_open(struct fdk_decoder_t* dec, const struct avpacket_t* pkt)
{
	unsigned char ch = 0;
	unsigned long samplerate = 0;
	UCHAR* config[1];
	UINT bufferSize[1];
	HANDLE_AACDECODER fdk;
	fdk = aacDecoder_Open(TT_MP4_ADTS, 1);
	if (NULL == fdk)
		return -1;

	config[0] = pkt->stream ? pkt->stream->extra : NULL;
	bufferSize[0] = pkt->stream ? pkt->stream->bytes : 0;
	if (pkt->stream && AAC_DEC_OK != aacDecoder_ConfigRaw(fdk, config, bufferSize))
	{
		aacDecoder_Close(fdk);
		return -1;
	}

	dec->fdk = fdk;
	dec->frame.channels = ch;
	dec->frame.sample_rate = (int)samplerate;
	dec->frame.sample_bits = PCM_SAMPLE_BITS(dec->frame.format);
	return 0;
}

static void aac_check(struct fdk_decoder_t* dec, const struct avpacket_t* pkt)
{
	int profile, frequency, channel;
	if (0 != aac_adts_parse(pkt->data, pkt->size, &profile, &frequency, &channel))
		return;

	if (profile != dec->profile || frequency != dec->frequency || channel != dec->channel)
	{
		if (dec->fdk)
		{
			aacDecoder_Close(dec->fdk);
			dec->fdk = NULL;
		}

		if (0 == fdk_open(dec, pkt))
		{
			dec->profile = profile;
			dec->frequency = frequency;
			dec->channel = channel;
		}
	}
}

static int fdk_input(void* audio, const struct avpacket_t* pkt)
{
	UCHAR* buffers[1];
	UINT bufferSize[1];
	UINT bytesValid[1];
	CStreamInfo* info;
	AAC_DECODER_ERROR ret;
	struct fdk_decoder_t* dec;
	dec = (struct fdk_decoder_t*)audio;
	aac_check(dec, pkt);
	if (!dec->fdk)
		return -1;

	buffers[0] = pkt->data;
	bufferSize[0] = pkt->size;
	bytesValid[0] = pkt->size;
	ret = aacDecoder_Fill(dec->fdk, buffers, bufferSize, bytesValid);
	if (AAC_DEC_OK != ret)
	{
		printf("[AAC] FDK fill error: %d\n", (int)ret);
		return ret > 0 ? -ret : ret;
	}

	assert(bytesValid[0] == 0); // all consumed
	
	ret = aacDecoder_DecodeFrame(dec->fdk, dec->samples, dec->bytes / sizeof(INT_PCM), 0);
	if (AAC_DEC_OK == ret)
	{
		info = aacDecoder_GetStreamInfo(dec->fdk);
		dec->frame.samples = info->frameSize;
		dec->frame.channels = info->aacNumChannels;
		dec->frame.sample_rate = info->aacSampleRate;
		dec->frame.pts = pkt->pts;
		dec->frame.dts = pkt->dts;
		dec->frame.linesize[0] = dec->frame.samples * dec->frame.channels * dec->frame.sample_bits / 8;
	}

	return ret > 0 ? -ret : ret;
}

static int fdk_getframe(void* audio, struct avframe_t** frame)
{
	struct fdk_decoder_t* dec;
	dec = (struct fdk_decoder_t*)audio;
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

struct audio_decoder_t* fdk_decoder(void)
{
	static struct audio_decoder_t s_decoder = {
		fdk_create,
		fdk_destroy,
		fdk_input,
		fdk_getframe,
	};
	return &s_decoder;
}

#endif /* _AVCODEC_FDK_ */
