#ifndef _avtimeline_h_
#define _avtimeline_h_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct avtimeline_t
{
	uint32_t gap;

	int64_t t0; // init timestamp
	int64_t max;
	
	struct
	{
		int init;
		int repeat; // same dts repeat times
		int64_t t; // last mapped dts timestamp
		int64_t dts; // last dts

		int64_t tmax; // mapped max pts timestamp
	} streams[8];
};

void avtimeline_init(struct avtimeline_t* t, uint32_t gap, int64_t t0);

void avtimeline_reset(struct avtimeline_t* t);

/// @return mapped pts
int64_t avtimeline_input32(struct avtimeline_t* t, int stream, uint32_t dts, uint32_t pts, int* discontinuity);

/// @return mapped pts
int64_t avtimeline_input64(struct avtimeline_t* t, int stream, int64_t dts, int64_t pts, int* discontinuity);

#ifdef __cplusplus
}
#endif
#endif /* !_avtimeline_h_ */
