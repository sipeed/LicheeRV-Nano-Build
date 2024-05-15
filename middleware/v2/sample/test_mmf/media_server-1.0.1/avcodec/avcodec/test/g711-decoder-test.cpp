#include "audio-decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void g711a_decoder_test(const char* g711a, const char* pcm)
{
	FILE* fg711 = fopen(g711a, "rb");
	FILE* fpcm = fopen(pcm, "wb");

	void* dec = g711a_decoder()->create(PCM_SAMPLE_FMT_S16, 1, 8000, NULL, 0);

	static uint8_t buffer[8000];
	avpacket_t pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.data = buffer;

	while (fg711 && fpcm)
	{
		pkt.size = fread(pkt.data, 1, 640, fg711);
		if (pkt.size <= 0)
			break;

		if (g711a_decoder()->decode(dec, &pkt) < 0)
			break;

		avframe_t* frame;
		if (g711a_decoder()->getframe(dec, &frame) >= 0)
		{
			assert(!PCM_SAMPLE_PLANAR(frame->format));
			fwrite(frame->data[0], 1, frame->linesize[0], fpcm);
			avframe_release(frame);
		}
	}

	g711a_decoder()->destroy(dec);
	fclose(fpcm);
	fclose(fg711);
}

void g711u_decoder_test(const char* g711u, const char* pcm)
{
	FILE* fg711 = fopen(g711u, "rb");
	FILE* fpcm = fopen(pcm, "wb");

	void* dec = g711u_decoder()->create(PCM_SAMPLE_FMT_S16, 1, 8000, NULL, 0);

	static uint8_t buffer[8000];
	avpacket_t pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.data = buffer;

	while (fg711 && fpcm)
	{
		pkt.size = fread(pkt.data, 1, 640, fg711);
		if (pkt.size <= 0)
			break;

		if (g711u_decoder()->decode(dec, &pkt) < 0)
			break;

		avframe_t* frame;
		if (g711u_decoder()->getframe(dec, &frame) >= 0)
		{
			assert(!PCM_SAMPLE_PLANAR(frame->format));
			fwrite(frame->data[0], 1, frame->linesize[0], fpcm);
			avframe_release(frame);
		}
	}

	g711u_decoder()->destroy(dec);
	fclose(fpcm);
	fclose(fg711);
}
