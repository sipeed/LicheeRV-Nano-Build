#ifndef _avpktutil2_h_
#define _avpktutil2_h_

#include "avpktutil.h"
#include "mpeg4-aac.h"
#include "mp3-header.h"
#include "webm-vpx.h"

// avpktutil2_t is deprecated, should use avpbs_find(@avbps.h) or AVPBitStream(@avbps.hpp)
#pragma deprecated(avpktutil2_t)
struct avpktutil2_t
{
    struct avpktutil_t pkt;
    struct avstream_t* video;
    struct avstream_t* audio;
    
    union {
        struct mpeg4_aac_t aac;
        struct mp3_header_t mp3;
    } a;
};

static inline struct avstream_t* avpktutil2_findstream(struct avpktutil2_t* s, int stream, AVPACKET_CODEC_ID codec, const void* data, size_t bytes)
{
    int r;
    (void)stream; // ignore

    switch (codec)
    {
    case AVCODEC_VIDEO_H264:
    case AVCODEC_VIDEO_H265:
    case AVCODEC_VIDEO_H266:
        if (!s->video)
            s->video = avpktutil_addvideo(&s->pkt, s->pkt.count, codec, 0, 0, 0, 0);
        return s->video;

    case AVCODEC_VIDEO_VP8:
        if (!s->video)
        {
            int width = 0, height = 0;
            struct webm_vpx_t vpx;
            const unsigned char src[] = { 0x00, 0x1f, 0x80, 0x02, 0x02, 0x02, 0x00, 0x00 };
            if(0 == webm_vpx_codec_configuration_record_from_vp8(&vpx, &width, &height, data, bytes))
                s->video = avpktutil_addvideo(&s->pkt, s->pkt.count, codec, width, height, src, sizeof(src));
        }
        return s->video;

    case AVCODEC_VIDEO_VP9:
        if (!s->video)
        {
            const unsigned char src[] = { 0x00, 0x1f, 0x80, 0x02, 0x02, 0x02, 0x00, 0x00 };
            s->video = avpktutil_addvideo(&s->pkt, s->pkt.count, codec, 0, 0, src, sizeof(src));
        }
        return s->video;

    case AVCODEC_AUDIO_AAC:
        if(!s->audio)
        {
            r = mpeg4_aac_adts_load((const uint8_t*)data, bytes, &s->a.aac);
            if(r > 0)
                s->audio = avpktutil_addaudio(&s->pkt, s->pkt.count, AVCODEC_AUDIO_AAC, s->a.aac.channel_configuration, 16, s->a.aac.sampling_frequency, 0, 0);
        }
        return s->audio;

    case AVCODEC_AUDIO_MP3:
        if(!s->audio)
        {
            // FIXME: mp3 sample bits
            r = mp3_header_load(&s->a.mp3, data, (int)bytes);
            if(r > 0)
                s->audio = avpktutil_addaudio(&s->pkt, s->pkt.count, AVCODEC_AUDIO_MP3, mp3_get_channel(&s->a.mp3), 16, mp3_get_frequency(&s->a.mp3), NULL, 0);
        }
        return s->audio;

    case AVCODEC_AUDIO_OPUS:
        if (!s->audio)
            s->audio = avpktutil_addaudio(&s->pkt, s->pkt.count, AVCODEC_AUDIO_OPUS, 2, 16, 48000, 0, 0);
        return s->audio;

    case AVCODEC_AUDIO_G711A:
    case AVCODEC_AUDIO_G711U:
        if (!s->audio)
            s->audio = avpktutil_addaudio(&s->pkt, s->pkt.count, codec, 1, 16, 8000, 0, 0);
        return s->audio;

    default:
        assert(0);
        return NULL;
    }
}

static inline int avpktutil2_input(struct avpktutil2_t* s, int stream, AVPACKET_CODEC_ID codec, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags, struct avpacket_t** pkt0)
{
    struct avstream_t* av;
    av = avpktutil2_findstream(s, stream, codec, data, bytes);
    if(!av)
        return -(__ERROR__ + ENOENT);
    
    return avpktutil_input(&s->pkt, av, data, bytes, pts, dts, flags, pkt0);
}

static inline int avpktutil2_destroy(struct avpktutil2_t* s)
{
    return avpktutil_destroy(&s->pkt);
}

#endif /* _avpktutil2_h_ */
