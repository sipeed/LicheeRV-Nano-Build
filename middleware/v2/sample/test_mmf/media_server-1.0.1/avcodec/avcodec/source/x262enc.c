#include "h264-encoder.h"
#if defined(_AVCODEC_X262_)
#include "x264/x264.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct x262_encoder_t
{
    x264_t* x264;
    x264_param_t params;
    x264_picture_t pic;
    x264_picture_t out;
    x264_nal_t *nal;
    int nnal;
    size_t bytes;
};

//"baseline", "main", "high", "high10", "high422", "high444"
static const char* x264enc_profile(const struct h264_parameter_t* param)
{
    switch (param->profile)
    {
    case H264_PROFILE_BASELINE: return x264_profile_names[0];
    case H264_PROFILE_MAIN: return x264_profile_names[1];
    case H264_PROFILE_HIGH: return x264_profile_names[2];
    case H264_PROFILE_HIGH10: return x264_profile_names[3];
    case H264_PROFILE_HIGH422: return x264_profile_names[4];
    case H264_PROFILE_HIGH444: return x264_profile_names[5];
    default:
        return x264_profile_names[0];
    }
}

static int x264enc_level(struct h264_parameter_t* param)
{
    switch (param->level)
    {
    case H264_LEVEL_1_0: return 10;
    case H264_LEVEL_1_B: return 9;
    case H264_LEVEL_1_1: return 11;
    case H264_LEVEL_1_2: return 12;
    case H264_LEVEL_1_3: return 13;
    case H264_LEVEL_2_0: return 20;
    case H264_LEVEL_3_0: return 30;
    case H264_LEVEL_3_1: return 31;
    case H264_LEVEL_3_2: return 32;
    case H264_LEVEL_4_0: return 40;
    case H264_LEVEL_4_1: return 41;
    case H264_LEVEL_4_2: return 42;
    case H264_LEVEL_5_0: return 50;
    case H264_LEVEL_5_1: return 51;
    case H264_LEVEL_5_2: return 52;
    default: return 30;
    }
}
static void x264enc_parameter_set(x264_param_t* x264, const struct h264_parameter_t* param)
{
    x264->b_mpeg2 = 1;
    x264_param_default_mpeg2(x264);

    x264_param_default_preset(x264, "fast", "zerolatency");
    x264_param_apply_profile(x264, x264enc_profile(param));
    //	x264->i_level_idc = x264enc_level(param);
    x264->i_csp = X264_CSP_I420;
    x264->i_width = param->width;
    x264->i_height = param->height;
    x264->i_bframe = 0;
    x264->b_cabac = 0; // disable CABAC
                       //	x264->b_deblocking_filter = 0;
    x264->i_threads = 1;
    x264->i_frame_reference = 1;
    x264->b_repeat_headers = 1;		// repeat sps/pps
                                    //	x264->i_slice_count = ;
                                    //	x264->i_bframe = 0;
    x264->i_keyint_max = param->frame_rate / 1000 * 2; // GOP size(2sec)
    x264->i_fps_num = param->frame_rate / 1000;
    x264->i_fps_den = 1;

    // 码率控制
    x264->rc.i_rc_method = X264_RC_ABR;//CQP(恒定质量)，CRF(恒定码率)，ABR(平均码率)
    x264->rc.i_bitrate = param->bitrate / 1000;
    x264->rc.i_vbv_max_bitrate = (int)((param->bitrate*1.2) / 1000);

    //x264->rc.i_qp_constant = ;
    //x264->rc.i_qp_min = ;
    //x264->rc.i_qp_max = ;
    //x264->rc.f_rate_tolerance = 1.0;

    // 图像质量控制, 越大图像越花, 越小越清晰
    x264->rc.f_rf_constant = 25;
    x264->rc.f_rf_constant_max = 45;

    x264->analyse.i_me_method = X264_ME_DIA; // 菱形搜索
}

static void* x262enc_create(const struct h264_parameter_t* param)
{
    struct x262_encoder_t* p;
    p = (struct x262_encoder_t*)malloc(sizeof(struct x262_encoder_t));
    if (NULL == p)
        return NULL;

    x264enc_parameter_set(&p->params, param);
    //if (0 != x264_picture_alloc(&p->pic, p->params.i_csp, p->params.i_width, p->params.i_height))
    //{
    //	free(p);
    //	return NULL;
    //}

    p->x264 = x264_encoder_open(&p->params);
    if (!p->x264)
    {
        //		x264_picture_clean(&p->pic);
        free(p);
        return NULL;
    }

    return p;
}

static void x262enc_destroy(void* h264)
{
    struct x262_encoder_t* p;
    p = (struct x262_encoder_t*)h264;
    x264_encoder_close(p->x264);
    //	x264_picture_clean(&p->pic);
}

static int x262enc_input(void* h264, const struct avframe_t* pic)
{
    int i, ret;
    struct x262_encoder_t* p;
    p = (struct x262_encoder_t*)h264;
    p->nnal = 0; // clear picture flag

    x264_picture_init(&p->pic);
    if (pic)
    {
        p->pic.img.i_csp = p->params.i_csp;
        //if (x264_bit_depth > 8)
        //	p->pic.img.i_csp |= X264_CSP_HIGH_DEPTH;

        p->pic.img.i_plane = 3;
        for (i = 0; i < p->pic.img.i_plane; i++) {
            p->pic.img.plane[i] = pic->data[i];
            p->pic.img.i_stride[i] = pic->linesize[i];
        }

        p->pic.i_pts = pic->pts;
        p->pic.i_type = (pic->flags & AVPACKET_FLAG_KEY) ? X264_TYPE_IDR : X264_TYPE_AUTO; // force IDR

                                                                                           // SEI
                                                                                           // x4->pic.extra_sei
    }

    if (pic || x264_encoder_delayed_frames(p->x264))
    {
        ret = x264_encoder_encode(p->x264, &p->nal, &p->nnal, pic ? &p->pic : NULL, &p->out);
        if (ret <= 0)
            return ret;

        p->bytes = ret;
    }
    return 1;
}

static int x262enc_getpacket(void* h264, struct avpacket_t* pkt)
{
    struct x262_encoder_t* p;
    p = (struct x262_encoder_t*)h264;
    //pkt->codecid = AVCODEC_VIDEO_H264;
    pkt->size = p->bytes;
    pkt->data = p->nal->p_payload; // the payloads of all output NALs are guaranteed to be sequential in memory.
    pkt->pts = p->out.i_pts;
    pkt->dts = p->out.i_dts;

    switch (p->out.i_type) {
    case X264_TYPE_IDR:
    case X264_TYPE_I:
        pkt->flags = AVPACKET_FLAG_KEY;
        break;

    case X264_TYPE_P:
    case X264_TYPE_B:
    case X264_TYPE_BREF:
    default:
        break;
    }

    return p->nnal;
}

struct h264_encoder_t* x262_encoder(void)
{
    static struct h264_encoder_t s_encoder = {
        x262enc_create,
        x262enc_destroy,
        x262enc_input,
        x262enc_getpacket,
    };
    return &s_encoder;
}

#endif /* _AVCODEC_X262_ */
