#include "audio-encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// https://trac.ffmpeg.org/wiki/Encode/AAC
// https://developer.apple.com/library/content/technotes/tn2258/_index.html
#define FRAME_LEN 1024

extern "C" struct audio_encoder_t* faac_encoder();

static uint8_t buffer[8 * FRAME_LEN];
void faac_encoder_test(const char* pcm, const char* adts)
{
	FILE* fpcm = fopen(pcm, "rb");
	FILE* fadts = fopen(adts, "wb");
	
	audio_parameter_t param;
	param.format = PCM_SAMPLE_FMT_S16;
	param.channels = 1;
	param.samplerate = 8000;
	void* faac = faac_encoder()->create(&param);

	avframe_t frame;
	memset(&frame, 0, sizeof(frame));
	frame.data[0] = buffer;
	frame.format = param.format;
	frame.samples = FRAME_LEN;
	frame.channels = param.channels;
	frame.sample_rate = param.samplerate;
	frame.sample_bits = PCM_SAMPLE_BITS(param.format);

	avpacket_t pkt;
	while (fadts && fpcm)
	{
		int r = fread(buffer, param.channels * PCM_SAMPLE_BITS(param.format) / 8, frame.samples, fpcm);
		if (r < frame.samples)
			break;

		if (faac_encoder()->encode(faac, &frame) < 0)
			break;

		avpacket_t pkt2;
		if (faac_encoder()->getpacket(faac, &pkt2) > 0)
		{
			fwrite(pkt2.data, 1, pkt2.size, fadts);
		}
	}

	frame.data[0] = NULL;
	frame.samples = 0;
	while (faac_encoder()->encode(faac, &frame) >= 0
		&& faac_encoder()->getpacket(faac, &pkt) >= 0)
	{
		fwrite(pkt.data, 1, pkt.size, fadts);
	}

	faac_encoder()->destroy(faac);
	fclose(fpcm);
	fclose(fadts);
}
