#include "avframe.h"
#include "avbuffer.h"
#include <string.h>
#include <assert.h>

struct avframe_t* avframe_alloc(int size)
{
	struct avbuffer_t* buf;
	struct avframe_t* frame;
	buf = avbuffer_alloc(size + sizeof(struct avframe_t));
	if (buf)
	{
		frame = (struct avframe_t*)buf->data;
		memset(frame, 0, sizeof(struct avframe_t));
		frame->data[0] = (uint8_t*)(frame + 1);
		frame->opaque = buf;
		return frame;
	}
	return NULL;
}

int32_t avframe_addref(struct avframe_t* frame)
{
	struct avbuffer_t* buf;
	if (NULL == frame || NULL == frame->opaque)
		return -1;

	buf = (struct avbuffer_t*)frame->opaque;
	return avbuffer_addref(buf);
}

int32_t avframe_release(struct avframe_t* frame)
{
	struct avbuffer_t* buf;
	if (NULL == frame || NULL == frame->opaque)
		return -1;

	buf = (struct avbuffer_t*)frame->opaque;
	return avbuffer_release(buf);
}
