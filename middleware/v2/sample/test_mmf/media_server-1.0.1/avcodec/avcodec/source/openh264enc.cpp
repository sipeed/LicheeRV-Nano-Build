#include "h264-encoder.h"
#if defined(_AVCODEC_OPENH264_)
#include "openh264/codec_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct openh264_encoder_t
{
	ISVCEncoder* h264;
	SEncParamExt param;
	SFrameBSInfo out;
	int skipped;
};

static void* openh264enc_create(const struct h264_parameter_t* param)
{
	int ret;
	struct openh264_encoder_t* p;
	p = (struct openh264_encoder_t*)malloc(sizeof(*p));
	if (NULL == p)
		return NULL;

	memset(p, 0, sizeof(*p));
	ret = WelsCreateSVCEncoder(&p->h264);
	if (0 != ret)
	{
		free(p);
		return NULL;
	}

	//SLevelInfo level;
	//level.iLayer = SPATIAL_LAYER_0;
	//level.uiLevelIdc = (ELevelIdc)param->level;
	//SProfileInfo profile;
	//profile.iLayer = SPATIAL_LAYER_0;
	//profile.uiProfileIdc = (EProfileIdc)param->profile;
	//p->h264->SetOption(ENCODER_OPTION_PROFILE, &profile);
	//p->h264->SetOption(ENCODER_OPTION_LEVEL, &level);

	p->h264->GetDefaultParams(&p->param);
	//p->param.bEnableDenoise = denoise;
	//p->param.iSpatialLayerNum = layers;
	p->param.iUsageType = CAMERA_VIDEO_REAL_TIME;
	p->param.fMaxFrameRate = param->frame_rate / 1000.0f;
	p->param.iPicWidth = param->width;
	p->param.iPicHeight = param->height;
	p->param.iTargetBitrate = param->bitrate;
	p->param.iRCMode = RC_BITRATE_MODE;

	p->param.iTemporalLayerNum = 1;
	p->param.iSpatialLayerNum = 1;
	p->param.bEnableDenoise = 0;
	p->param.bEnableBackgroundDetection = 1;
	p->param.bEnableAdaptiveQuant = 1;
//	p->param.bEnableFrameSkip = false;
	p->param.iLtrMarkPeriod = 30;
	p->param.bEnableLongTermReference = 0;
	p->param.uiIntraPeriod = param->gop_size;
	p->param.iNumRefFrame = 1;
//	p->param.bEnableSpsPpsIdAddition = 1;
	p->param.bPrefixNalAddingCtrl = false; // 14-H264_NAL_PREFIX
//	p->param.iMultipleThreadIdc = avctx->thread_count;
	p->param.iEntropyCodingModeFlag = 0; // 0:CAVLC  1:CABAC.
	p->param.bEnableLongTermReference = false;

	p->param.sSpatialLayers[0].iVideoWidth = p->param.iPicWidth;
	p->param.sSpatialLayers[0].iVideoHeight = p->param.iPicHeight;
	p->param.sSpatialLayers[0].fFrameRate = p->param.fMaxFrameRate;
	p->param.sSpatialLayers[0].iSpatialBitrate = p->param.iTargetBitrate;
	p->param.sSpatialLayers[0].iMaxSpatialBitrate = p->param.iMaxBitrate;
	p->param.sSpatialLayers[0].uiProfileIdc = (EProfileIdc)param->profile;
	p->param.sSpatialLayers[0].uiLevelIdc = (ELevelIdc)param->level;

	ret = p->h264->InitializeExt(&p->param);
	if (0 != ret)
	{
		WelsDestroySVCEncoder(p->h264);
		free(p);
		return NULL;
	}

	return p;
}

static void openh264enc_destroy(void* h264)
{
	struct openh264_encoder_t* p;
	p = (struct openh264_encoder_t*)h264;

	if(p->h264)
	{
		p->h264->Uninitialize();
		WelsDestroySVCEncoder(p->h264);
		assert(0 == p->skipped);
	}

	free(p);
}

static int openh264enc_input(void* h264, const struct avframe_t* pic)
{
	int i, ret;
	SSourcePicture src;
	struct openh264_encoder_t* p;
	p = (struct openh264_encoder_t*)h264;

	memset(&p->out, 0, sizeof(p->out));
	memset(&src, 0, sizeof(SSourcePicture));
	src.iPicWidth = pic->width;
	src.iPicHeight = pic->height;
	src.iColorFormat = videoFormatI420;
	src.uiTimeStamp = pic->pts;
	for (i = 0; i < 3; i++) {
		src.pData[i] = pic->data[i];
		src.iStride[i] = pic->linesize[i];
	}

	if (pic->flags & AVPACKET_FLAG_KEY)
		p->h264->ForceIntraFrame(true);

	ret = p->h264->EncodeFrame(&src, &p->out);
	if (cmResultSuccess != ret)
	{
		printf("%s: EncodeFrame => %d\n", __FUNCTION__, ret);
		return -1;
	}

	return 1;
}

static int openh264enc_getpacket(void* h264, avpacket_t* pkt)
{
	int i, layer;
	int layer_size[MAX_LAYER_NUM_OF_FRAME];
	struct openh264_encoder_t* p;
	p = (struct openh264_encoder_t*)h264;

	switch (p->out.eFrameType) {
	case videoFrameTypeIDR:
	case videoFrameTypeI:
		pkt->flags = AVPACKET_FLAG_KEY;
		break;
	case videoFrameTypeP:
		break;
	//case videoFrameTypeB:
	//	pkt->pic_type = PICTURE_TYPE_B;
	//	break;
	case videoFrameTypeSkip:
		printf("%s: frame skipped\n", __FUNCTION__);
		p->skipped++;
		return 0;

	default:
		printf("%s: unknown frame type\n", __FUNCTION__);
		return 0;
	}

	// Normal frames are returned with one single layer, while IDR
	// frames have two layers, where the first layer contains the SPS/PPS.
	// If using global headers, don't include the SPS/PPS in the returned
	// packet - thus, only return one layer.
	//first_layer = 0;
	//if (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER)
	//	first_layer = fbi.iLayerNum - 1;

	pkt->size = 0;
	memset(layer_size, 0, sizeof(layer_size));
	for (layer = 0; layer < p->out.iLayerNum; layer++) {
		for (i = 0; i < p->out.sLayerInfo[layer].iNalCount; i++)
			layer_size[layer] += p->out.sLayerInfo[layer].pNalLengthInByte[i];
		assert(p->out.sLayerInfo[layer].pBsBuf == p->out.sLayerInfo[0].pBsBuf + pkt->size);
		pkt->size += layer_size[layer];
	}
	//printf("%s: %d slices\n", __FUNCTION__, p->out.sLayerInfo[p->out.iLayerNum - 1].iNalCount);

	pkt->data = p->out.sLayerInfo[0].pBsBuf;
	//pkt->data = (uint8_t*)malloc(pkt->bytes);
	//if (NULL == pkt->data)
	//	return -1;

	//size = 0;
	//for (layer = 0; layer < p->out.iLayerNum; layer++) {
	//	memcpy(pkt->data + size, p->out.sLayerInfo[layer].pBsBuf, layer_size[layer]);
	//	size += layer_size[layer];
	//}

	pkt->dts = pkt->pts = p->out.uiTimeStamp;
	//pkt->codecid = AVCODEC_VIDEO_H264;
	return 1;
}

struct h264_encoder_t* openh264_encoder(void)
{
	static struct h264_encoder_t s_encoder = {
		openh264enc_create,
		openh264enc_destroy,
		openh264enc_input,
		openh264enc_getpacket,
	};

	return &s_encoder;
}

#endif /* _AVCODEC_OPENH264_ */
