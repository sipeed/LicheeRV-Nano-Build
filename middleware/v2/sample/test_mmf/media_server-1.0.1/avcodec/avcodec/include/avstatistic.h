#ifndef _avstatistic_h_
#define _avstatistic_h_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct avjitter_t {
	int duration;
	int count;
	int max;
	int min;
};

struct avbitrate_t
{
	uint32_t i; // internal use only
	uint32_t interval; // bucket interval(ms)
	uint32_t packets[6]; // N + 1
	uint64_t buckets[6]; // N + 1

	int64_t clock; // last clock
	uint64_t total; // total bytes
};

struct avstatistic_t
{
	int64_t clock;			// start time(UTC+0 ms)

	struct
	{
		int64_t pts; // last packet pts
		int64_t dts; // last packet dts
		uint64_t packets;
		struct avjitter_t jitter;
		struct avbitrate_t bitrate;

		int64_t first_pts;
		int64_t first_dts;
		int64_t first_recv; // first data byte
		int64_t first_packet; // first audio/video/data chunk/packet
	} streams[8];

//#define s_audio streams[0]
//#define s_video streams[1]
};

void avstatistic_init(struct avstatistic_t* stats, int64_t clock, int interval);

/// update stream stats, include bitrate/jitter
/// @return 0-ok, other-error
int avstatistic_input(struct avstatistic_t* stats, int64_t clock, int stream, int64_t pts, int64_t dts, uint64_t bytes);

/// @return stream frame rate(in (N-1) * interval)
double avstatistic_getfps(const struct avstatistic_t* stats, int stream);

/// @return total bitrate
uint64_t avstatistic_getbitrate(const struct avstatistic_t* stats);

/// avjitter_clear clear all data
void avjitter_clear(struct avjitter_t* jitter);

/// avjitter_input update jitter min/max
void avjitter_input(struct avjitter_t* jitter, int value);

/// avjitter_format output format jitter string
int avjitter_format(const struct avjitter_t* jitter, char* buf, int len);


/// avbitrate_clear clear all data
void avbitrate_clear(struct avbitrate_t* rate);

/// avbitrate_input [10 | 10 | 10 | 10 | 10 | 10] bytes bucket
void avbitrate_input(struct avbitrate_t* rate, int64_t clock, uint64_t bytes);

/// avbitrate_get bytes per second
uint64_t avbitrate_get(const struct avbitrate_t* rate);

#ifdef __cplusplus
}
#endif
#endif /* !_avstatistic_h_ */
