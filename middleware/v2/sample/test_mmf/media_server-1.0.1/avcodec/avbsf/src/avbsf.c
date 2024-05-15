#include "avbsf.h"
#include <stdlib.h>

struct avbsf_t* avbsf_aac(void);
struct avbsf_t* avbsf_h264(void);
struct avbsf_t* avbsf_h265(void);
struct avbsf_t* avbsf_h266(void);

struct avbsf_t* avbsf_find(AVPACKET_CODEC_ID codec)
{
	switch (codec)
	{
		// video
	case AVCODEC_VIDEO_H264:
		return avbsf_h264();
	case AVCODEC_VIDEO_H265:
		return avbsf_h265();
	case AVCODEC_VIDEO_H266:
		return avbsf_h266();
	//case AVCODEC_VIDEO_AV1:
	//	return avbsf_av1();
	//case AVCODEC_VIDEO_VP8:
	//case AVCODEC_VIDEO_VP9:
	//	return avbsf_vpx();

		// audio
	case AVCODEC_AUDIO_AAC:
		return avbsf_aac();
	//case AVCODEC_AUDIO_MP3:
	//	return avbsf_mp3();
	//case AVCODEC_AUDIO_OPUS:
	//	return avbsf_opus();
	//case AVCODEC_AUDIO_G711A:
	//case AVCODEC_AUDIO_G711U:
	//case AVCODEC_AUDIO_G726:
	//case AVCODEC_AUDIO_G729:
	//	return avbsf_g7xx();

	//default:
	//	return avbsf_common();
	}

	return NULL;
}
