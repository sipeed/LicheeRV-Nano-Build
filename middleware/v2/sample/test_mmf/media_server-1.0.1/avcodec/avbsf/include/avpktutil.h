#ifndef _avpktutil_h_
#define _avpktutil_h_

#include "avpacket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

struct avpktutil_t
{
    struct avstream_t* streams[8];
    int count;
};

static inline struct avstream_t* avpktutil_addvideo(struct avpktutil_t* s, int stream, AVPACKET_CODEC_ID codec, int width, int height, const void* extra, size_t bytes)
{
    struct avstream_t* video;
    if((size_t)s->count >= sizeof(s->streams)/sizeof(s->streams[0]))
        return NULL;
    
    video = avstream_alloc((int)bytes);
    if(!video) return NULL;
    video->stream = stream;
    video->codecid = codec;
    video->width = width;
    video->height = height;
    memcpy(video->extra, extra, bytes);
    s->streams[s->count++] = video;
    return video;
}

static inline struct avstream_t* avpktutil_addaudio(struct avpktutil_t* s, int stream, AVPACKET_CODEC_ID codec, int channel_count, int bit_per_sample, int sample_rate, const void* extra, size_t bytes)
{
    struct avstream_t* audio;
    if((size_t)s->count >= sizeof(s->streams)/sizeof(s->streams[0]))
        return NULL;
    
    audio = avstream_alloc((int)bytes);
    if(!audio) return NULL;
    audio->stream = stream;
    audio->codecid = codec;
    audio->channels = channel_count;
    audio->sample_rate = sample_rate;
    audio->sample_bits = bit_per_sample;
    memcpy(audio->extra, extra, bytes);
    s->streams[s->count++] = audio;
    return audio;
}

static inline struct avstream_t* avpktutil_addsubtitle(struct avpktutil_t* s, int stream, AVPACKET_CODEC_ID codec, const void* extra, size_t bytes)
{
    struct avstream_t* subtitle;
    if((size_t)s->count >= sizeof(s->streams)/sizeof(s->streams[0]))
        return NULL;
    
    subtitle = avstream_alloc((int)bytes);
    if(!subtitle) return NULL;
    subtitle->stream = stream;
    subtitle->codecid = codec;
    memcpy(subtitle->extra, extra, bytes);
    s->streams[s->count++] = subtitle;
    return subtitle;
}

static inline int avpktutil_input(struct avpktutil_t* s, struct avstream_t* stream, const void* buffer, size_t bytes, int64_t pts, int64_t dts, int flags, struct avpacket_t** pkt0)
{
    struct avpacket_t* pkt;
    (void)s; //ignore

    //app_log(LOG_DEBUG, "track %u pts: %lld, dts: %lld, size: %u, key: %d\n", stream->stream, pts, dts, bytes, flags ? 1 : 0);
    pkt = avpacket_alloc((int)bytes);
    if (!pkt) return -(__ERROR__ + ENOMEM);
    
    avstream_addref(stream);
    memmove(pkt->data, buffer, bytes);
    pkt->pts = pts;
    pkt->dts = dts;
    pkt->flags = flags;
    pkt->stream = stream;
    *pkt0 = pkt;
    return 0;
}

static inline int avpktutil_destroy(struct avpktutil_t* s)
{
    int i;
    for(i = 0; i < s->count; i++)
    {
        avstream_release(s->streams[i]);
        s->streams[i] = NULL;
    }
    return 0;
}

#endif /* _avpktutil_h_ */
