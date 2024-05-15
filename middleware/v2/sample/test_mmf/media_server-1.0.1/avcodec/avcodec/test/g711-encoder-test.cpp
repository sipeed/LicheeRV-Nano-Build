#include "audio-encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void g711a_encoder_test(const char* pcm, const char* g711a)
{
	FILE* fpcm = fopen(pcm, "rb");
	FILE* fg711 = fopen(g711a, "wb");

	audio_parameter_t param;
	param.format = PCM_SAMPLE_FMT_S16;
	param.channels = 1;
	param.samplerate = 8000;
	void* faac = g711a_encoder()->create(&param);

	static uint8_t buffer[8000];
	avframe_t frame;
	memset(&frame, 0, sizeof(frame));
	frame.data[0] = buffer;
	frame.format = param.format;
	frame.samples = 640;
	frame.channels = param.channels;
	frame.sample_rate = param.samplerate;
	frame.sample_bits = PCM_SAMPLE_BITS(param.format);

	avpacket_t pkt;
	while (fg711 && fpcm)
	{
		int r = fread(buffer, param.channels * PCM_SAMPLE_BITS(param.format) / 8, frame.samples, fpcm);
		if (r < frame.samples)
			break;

		if (g711a_encoder()->encode(faac, &frame) < 0)
			break;

		avpacket_t pkt2;
		if (g711a_encoder()->getpacket(faac, &pkt2) >= 0)
		{
			fwrite(pkt2.data, 1, pkt2.size, fg711);
		}
	}

	g711a_encoder()->destroy(faac);
	fclose(fpcm);
	fclose(fg711);
}

void g711u_encoder_test(const char* pcm, const char* g711u)
{
	FILE* fpcm = fopen(pcm, "rb");
	FILE* fg711 = fopen(g711u, "wb");

	audio_parameter_t param;
	param.format = PCM_SAMPLE_FMT_S16;
	param.channels = 1;
	param.samplerate = 8000;
	void* faac = g711u_encoder()->create(&param);

	static uint8_t buffer[8000];
	avframe_t frame;
	memset(&frame, 0, sizeof(frame));
	frame.data[0] = buffer;
	frame.format = param.format;
	frame.samples = 640;
	frame.channels = param.channels;
	frame.sample_rate = param.samplerate;
	frame.sample_bits = PCM_SAMPLE_BITS(param.format);

	avpacket_t pkt;
	while (fg711 && fpcm)
	{
		int r = fread(buffer, param.channels * PCM_SAMPLE_BITS(param.format) / 8, frame.samples, fpcm);
		if (r < frame.samples)
			break;

		if (g711u_encoder()->encode(faac, &frame) < 0)
			break;

		avpacket_t pkt2;
		if (g711u_encoder()->getpacket(faac, &pkt2) >= 0)
		{
			fwrite(pkt2.data, 1, pkt2.size, fg711);
		}
	}

	g711u_encoder()->destroy(faac);
	fclose(fpcm);
	fclose(fg711);
}
