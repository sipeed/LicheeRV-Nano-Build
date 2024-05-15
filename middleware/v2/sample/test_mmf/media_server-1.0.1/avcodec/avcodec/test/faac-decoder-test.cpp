#include "audio-decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint8_t buffer[2 * 1024 * 1024];
static int adts_file_reader(FILE* fp, uint8_t* adts, int size)
{
	if (7 != fread(adts, 1, 7, fp))
		return 0; // eof

	if (0xFF != adts[0] || 0xF0 != (adts[1] & 0xF0))
		return -1; // check ADTS syncword failed

	int len = ((adts[3] & 0x03) << 11) | (adts[4] << 3) | (adts[5] >> 5);
	if (size < len)
		return -1;

	if (len - 7 != fread(adts + 7, 1, len - 7, fp))
		return -1;

	return len;
}

void faac_decoder_test(const char* adts, const char* pcm)
{
	FILE* fadts = fopen(adts, "rb");
	FILE* fpcm = fopen(pcm, "wb");

	void* faad = faac_decoder()->create(PCM_SAMPLE_FMT_S16, 2, 48000, NULL, 0);

	avpacket_t pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.data = buffer;

	while (fadts && fpcm)
	{
		pkt.size = adts_file_reader(fadts, buffer, sizeof(buffer));
		if (pkt.size <= 0)
			break;

		if (faac_decoder()->decode(faad, &pkt) < 0)
			break;

		avframe_t* frame;
		if (faac_decoder()->getframe(faad, &frame) >= 0)
		{
			assert(!PCM_SAMPLE_PLANAR(frame->format));
			fwrite(frame->data[0], 1, frame->linesize[0], fpcm);
			avframe_release(frame);
		}
	}

	faac_decoder()->destroy(faad);
	fclose(fpcm);
	fclose(fadts);
}
