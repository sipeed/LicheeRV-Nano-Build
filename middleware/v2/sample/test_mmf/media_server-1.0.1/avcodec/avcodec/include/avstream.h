#ifndef _avstream_h_
#define _avstream_h_

#include <stdint.h>
#include "avcodecid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AVSTREAM_TYPE {
	AVSTREAM_UNKNOWN,
	AVSTREAM_AUDIO,
	AVSTREAM_VIDEO,
	AVSTREAM_SUBTITLE,
	AVSTREAM_DATA,
} AVSTREAM_TYPE;

struct avtimebase_t
{
	int num; // numerator
	int den; // denominator
};

struct avstream_t
{
	int stream; // stream index
	enum AVPACKET_CODEC_ID codecid;
	struct avtimebase_t timebase;

	// video only
	int width;
	int height;
	double fps; // optional

	// audio only
	int channels;	 ///< number of audio channels
	int sample_bits; ///< bits per sample
	int sample_rate; ///< samples per second(frequency)

	// stream extra data(codec data)
	void* extra;
	int bytes;

	void* opaque; // internal use only
};

///@param[in] size alloc packet data size, don't include sizeof(struct avpacket_t)
///@return alloc new avpacket_t, use avpacket_release to free memory
struct avstream_t* avstream_alloc(int size);
int32_t avstream_addref(struct avstream_t* stream);
int32_t avstream_release(struct avstream_t* stream);

AVSTREAM_TYPE avstream_type(const struct avstream_t* stream);

#ifdef __cplusplus
}
#endif
#endif /* !_avstream_h_ */
