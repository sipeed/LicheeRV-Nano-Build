#include "h264-encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void x264_encode_test(const char* output, const char* input, int width, int height, int stride, int bitrate)
{
	static unsigned char s_yuv[1920*1080*3/2];
	
	h264_parameter_t param;
//	memset(&param, 0, sizeof(param));
	param.profile = H264_PROFILE_BASELINE;
	param.level = H264_LEVEL_3_0;
	param.width = width;
	param.height = height;
	param.format = PICTURE_YUV420;
	param.frame_rate = 25000;
	param.gop_size = 50;
	param.bitrate = bitrate;

	h264_encoder_t* api = x264_encoder();
//	h264_encoder_t* api = openh264_encoder();
	void* h264 = api->create(&param);

	FILE* wfp = fopen(output, "wb");
	FILE* rfp = fopen(input, "rb");
	for (int i = 0; fread(s_yuv, 1, stride*height * 3 / 2, rfp) == stride*height * 3 / 2; i++)
	{
		struct avframe_t pic;
		memset(&pic, 0, sizeof(pic));
		pic.format = PICTURE_YUV420;
		pic.width = param.width;
		pic.height = param.height;
		pic.flags = 0;
		pic.pts = i * 400;
		pic.dts = i * 400;
		pic.data[0] = s_yuv;
		pic.linesize[0] = stride;
		pic.data[1] = pic.data[0] + pic.linesize[0] * pic.height;
		pic.linesize[1] = stride / 2;
		pic.data[2] = pic.data[1] + pic.linesize[1] * pic.height / 2;
		pic.linesize[2] = stride / 2;

		assert(api->input(h264, &pic) > 0);

		avpacket_t pkt;
		memset(&pkt, 0, sizeof(pkt));
		assert(api->getpacket(h264, &pkt) > 0);
		assert(pkt.size == fwrite(pkt.data, 1, pkt.size, wfp));
	}
	fclose(rfp);
	fclose(wfp);

	api->destroy(h264);	
}
