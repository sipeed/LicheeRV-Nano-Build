#include "avstream.h"
#include "avbuffer.h"
#include <string.h>
#include <assert.h>

struct avstream_t* avstream_alloc(int size)
{
	struct avbuffer_t* buf;
	struct avstream_t* stream;
	buf = avbuffer_alloc(size + sizeof(struct avstream_t));
	if (buf)
	{
		stream = (struct avstream_t*)buf->data;
		memset(stream, 0, sizeof(struct avstream_t));
		stream->timebase.num = 1;
		stream->timebase.den = 1000; // ms
		stream->extra = (uint8_t*)(stream + 1);
		stream->bytes = size;
		stream->opaque = buf;
		return stream;
	}
	return NULL;
}

int32_t avstream_addref(struct avstream_t* stream)
{
	struct avbuffer_t* buf;
	if (NULL == stream || NULL == stream->opaque)
		return -1;

	buf = (struct avbuffer_t*)stream->opaque;
	return avbuffer_addref(buf);
}

int32_t avstream_release(struct avstream_t* stream)
{
	struct avbuffer_t* buf;
	if (NULL == stream || NULL == stream->opaque)
		return -1;

	buf = (struct avbuffer_t*)stream->opaque;
	return avbuffer_release(buf);
}

AVSTREAM_TYPE avstream_type(const struct avstream_t* stream)
{
	if (stream->codecid > 0 && stream->codecid < 0x10000)
		return AVSTREAM_VIDEO;
	else if (stream->codecid >= 0x10000 && stream->codecid < 0x20000)
		return AVSTREAM_AUDIO;
	else if (stream->codecid >= 0x20000 && stream->codecid < 0x30000)
		return AVSTREAM_SUBTITLE;
	else if (stream->codecid >= 0x30000 && stream->codecid < 0x40000)
		return AVSTREAM_DATA;
	else
		return AVSTREAM_UNKNOWN;
}
