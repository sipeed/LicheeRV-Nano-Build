#include "avtimeline.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static inline int64_t max64(int64_t t1, int64_t t2)
{
	return t1 > t2 ? t1 : t2;
}

void avtimeline_init(struct avtimeline_t* t, uint32_t gap, int64_t t0)
{
	memset(t, 0, sizeof(*t));
	t->t0 = t0;
	t->max = 0;
	t->gap = gap;
}

void avtimeline_reset(struct avtimeline_t* t)
{
	t->max = 0;
	memset(t->streams, 0, sizeof(t->streams));
}

static int64_t avtimeline_map32(struct avtimeline_t* t, int stream, uint32_t dts)
{
	assert(stream >= 0 && stream <= sizeof(t->streams) / sizeof(t->streams[0]));
	return t->streams[stream].t + (int64_t)(uint32_t)(dts - (uint32_t)(t->streams[stream].dts));
}

static int64_t avtimeline_map64(struct avtimeline_t* t, int stream, int64_t dts)
{
	assert(stream >= 0 && stream <= sizeof(t->streams) / sizeof(t->streams[0]));
	return t->streams[stream].t + (dts - t->streams[stream].dts);
}

static int64_t avtimeline_max(struct avtimeline_t* t)
{
	size_t i;
	int64_t timestamp;

	timestamp = t->t0;
	for (i = 0; i < sizeof(t->streams) / sizeof(t->streams[0]); i++)
	{
		if (0 == t->streams[i].init)
			continue;
		if (t->streams[i].tmax > timestamp)
			timestamp = t->streams[i].tmax;
	}

	return timestamp;
}

static void avtimeline_rebuild(struct avtimeline_t* t, int stream, int64_t dts)
{
	int64_t timestamp;
	timestamp = avtimeline_max(t);
	//avtimeline_reset(t);

	t->streams[stream].t = timestamp + 1; // monotone increasing timestamp
	t->streams[stream].tmax = t->streams[stream].t;
	t->streams[stream].dts = dts;
	t->streams[stream].init = 1;
	t->streams[stream].repeat = 0;
}

static void avtimeline_stream_init(struct avtimeline_t* t, int stream, int64_t dts)
{
	int i;
	int32_t diff;

	for (i = 0; i < sizeof(t->streams) / sizeof(t->streams[0]); i++)
	{
		if (!t->streams[i].init)
			continue;

		diff = (int32_t)(dts - t->streams[i].dts);
		if (((diff > 0 && diff < (int32_t)t->gap /*gap*/) || (diff < 0 && -diff < (int32_t)t->gap)) 
			&& t->streams[i].t + diff >= 0) // fix stream base timestamp < 0
		{
			t->streams[stream].init = 1;
			t->streams[stream].t = t->streams[i].t + diff;
			t->streams[stream].tmax = t->streams[i].t;
			t->streams[stream].dts = dts;
			return;
		}
	}

	t->streams[stream].init = 1;
	t->streams[stream].t = avtimeline_max(t);
	t->streams[stream].tmax = t->streams[stream].t;
	t->streams[stream].dts = dts;
}

int64_t avtimeline_input32(struct avtimeline_t* t, int stream, uint32_t dts, uint32_t pts, int *discontinuity)
{
	int init;
	int32_t diff;
	uint32_t last;
	int64_t timestamp;

	if (stream < 0 || stream >= sizeof(t->streams) / sizeof(t->streams[0]))
		return 0;

	init = t->streams[stream].init;
	if (!t->streams[stream].init)
	{
		avtimeline_stream_init(t, stream, dts);
	}

	*discontinuity = 0;
	timestamp = t->streams[stream].t;
	last = (uint32_t)(t->streams[stream].dts);
	
	diff = (int32_t)(dts - last);
	if ( diff > (int32_t)t->gap /*gap*/ || (diff < 0 && last - dts > t->gap /*rewind*/ ) )
	{
		// reset timestamp
		avtimeline_rebuild(t, stream, dts);
		*discontinuity = 1;

		timestamp = avtimeline_map32(t, stream, dts);
	}
	else if (diff > t->streams[stream].repeat)
	{
		timestamp = avtimeline_map32(t, stream, dts);

		t->streams[stream].dts = dts; // update last dts
	}
	else
	{
		// fix: don't change t, for timestamp inc
	}

	if (t->max < timestamp)
	{
		t->max = timestamp;
	}
	else if (timestamp + t->gap < t->max)
	{
		// a/v timeline diff > gap
		t->streams[stream].t = avtimeline_max(t);
		t->streams[stream].dts = dts;
		timestamp = avtimeline_map32(t, stream, dts);
	}

	t->streams[stream].t = timestamp;

	t->streams[stream].repeat = (0 == *discontinuity && diff <= t->streams[stream].repeat && init) ? t->streams[stream].repeat + 1 : 0;
	timestamp += t->streams[stream].repeat; // monotone increasing timestamp

	t->streams[stream].tmax = max64(t->streams[stream].tmax, max64(timestamp, (int64_t)(pts - dts) + timestamp));
	return timestamp;
}

int64_t avtimeline_input64(struct avtimeline_t* t, int stream, int64_t dts, int64_t pts, int* discontinuity)
{
	int init;
	int64_t diff;
	int64_t last;
	int64_t timestamp;

	if (stream < 0 || stream >= sizeof(t->streams) / sizeof(t->streams[0]))
		return 0;

	init = t->streams[stream].init;
	if (!t->streams[stream].init)
	{
		avtimeline_stream_init(t, stream, dts);
	}

	last = t->streams[stream].dts;
	timestamp = t->streams[stream].t;

	diff = dts - last;
	if (diff > t->gap /*gap*/ || (diff < 0 && last - dts > t->gap /*rewind*/ ))
	{
		avtimeline_rebuild(t, stream, dts);
		*discontinuity = 1;

		timestamp = avtimeline_map64(t, stream, dts);
	}
	else if (diff > t->streams[stream].repeat)
	{
		timestamp = avtimeline_map64(t, stream, dts);

		t->streams[stream].dts = dts; // update last dts
	}
	else
	{
		// fix: don't change t, for timestamp inc
	}

	if (t->max < timestamp)
	{
		t->max = timestamp;
	}
	else if (timestamp + t->gap < t->max)
	{
		t->streams[stream].t = avtimeline_max(t);
		t->streams[stream].dts = dts;
		timestamp = avtimeline_map64(t, stream, dts);
	}

	t->streams[stream].t = timestamp;

	t->streams[stream].repeat = (0 == *discontinuity && diff <= t->streams[stream].repeat && init) ? t->streams[stream].repeat + 1 : 0;
	timestamp += t->streams[stream].repeat; // monotone increasing timestamp

	t->streams[stream].tmax = max64(t->streams[stream].tmax, max64(timestamp, (int64_t)(pts - dts) + timestamp));
	return timestamp;
}


#if defined(_DEBUG) || defined(DEBUG)
#include <time.h>

static void avtimeline_test1(void)
{
	//static const int videoStep = 33; // 30fps
	//static const int audioStep = 23; // 1024/44100Hz
	static const int videoStep = 33000000; // 30fps
	static const int audioStep = 23000000; // 1024/44100Hz

	struct avtimeline_t line;
	uint64_t atotal, vtotal;
	uint32_t adts, vdts;
	int64_t t, at, vt;
	uint64_t i;
	int discontinuity;
	
	vdts = adts = rand();
	//adts = 0xFFFFFF00;
	//vdts = 0xFFFF0000;
	
	avtimeline_init(&line, 50000000, vdts);
	atotal = vtotal = at = vt = 0;
	//while (atotal < 0x20000)
	for(i = 0; atotal < 8 * (uint64_t)(0xFFFFFFFF) + 100; i++)
	{
		if(atotal < vtotal) 
		{
			t = avtimeline_input32(&line, 0, adts, adts, &discontinuity);
			assert(0 == at || t == at + audioStep);
			//printf("[A] dts: %u, timeline: %u\n", adts, t);
			at = t;
			adts += audioStep;
			atotal += audioStep;
		}
		else 
		{
			t = avtimeline_input32(&line, 1, vdts, vdts, &discontinuity);
			assert(0 == vt || t == vt + videoStep);
			//printf("[V] dts: %u, timeline: %u\n", vdts, t);
			vt = t;
			vdts += videoStep;
			vtotal += videoStep;
		}
	}
}

static void avtimeline_gap_test(void)
{
	int ad, vd;
	uint32_t adts, vdts;
	int64_t at[10], vt[10];
	struct avtimeline_t line;

	vdts = adts = rand();
	avtimeline_init(&line, 5000, adts);

	at[0] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[0] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[0] == adts);

	adts += 100;
	vdts += 500;
	at[1] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[1] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[1] - at[0] == 100 && vt[1] - vt[0] == 500);

	adts += 2 * 1000;
	vdts += 500;
	at[2] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[2] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[2] - at[1] == 2*1000 && vt[2] - vt[1] == 500);

	adts += 3 * 1000;
	vdts += 6 * 1000;
	at[3] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[3] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && vd && at[3] - at[2] == 3 * 1000 && vt[3] - at[3] == 1);

	adts += 1;
	vdts += 500;
	at[4] = avtimeline_input32(&line, 0, adts, adts, &ad); //rebuild
	vt[4] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[4] == vt[3] && vt[4] - vt[3] == 500);

	adts += 3 * 1000;
	vdts += 6 * 1000;
	at[5] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[5] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && vd && at[5] - at[4] == 3 * 1000 && vt[5] - at[5] == 1);

	adts += 1;
	vdts += 500;
	at[6] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[6] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[6] == vt[5] && vt[6] - vt[5] == 500);

	adts += 6 * 1000;
	vdts += 6 * 1000;
	at[7] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[7] = avtimeline_input32(&line, 1, vdts, vdts, &vd); // init
	assert(ad && vd && at[7] - vt[6] == 1 && vt[7] == at[7]+1);

	adts += 100;
	vdts += 500;
	at[8] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[8] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[8] - at[7] == 100 && vt[8] - vt[7] == 500);

	adts += 100;
	vdts += 500;
	at[9] = avtimeline_input32(&line, 0, adts, adts, &ad);
	vt[9] = avtimeline_input32(&line, 1, vdts, vdts, &vd);
	assert(!ad && !vd && at[9] - at[8] == 100 && vt[9] - vt[8] == 500);
}

static void avtimeline_monotone_increment_test(void)
{
	uint32_t dts;
	int64_t timestamp;
	int discontinuity;
	struct avtimeline_t line;
	
	dts = rand();
	avtimeline_init(&line, 5000, dts);
	timestamp = avtimeline_input32(&line, 0, dts, dts, &discontinuity);
	assert(timestamp + 1 == avtimeline_input32(&line, 0, dts, dts, &discontinuity));
	assert(timestamp + 2 == avtimeline_input32(&line, 0, dts, dts, &discontinuity));
	assert(timestamp + 3 == avtimeline_input32(&line, 0, dts, dts, &discontinuity));
	assert(timestamp + 4 == avtimeline_input32(&line, 0, dts, dts, &discontinuity));
	assert(timestamp + 5 == avtimeline_input32(&line, 0, dts+3, dts+3, &discontinuity));
	assert(timestamp + 10 == avtimeline_input32(&line, 0, dts + 10, dts + 10, &discontinuity));
	assert(timestamp + 11 == avtimeline_input32(&line, 0, dts + 10, dts + 10, &discontinuity));
	assert(timestamp + 12 == avtimeline_input32(&line, 0, dts + 10, dts + 10, &discontinuity));
}

static void avtimeline_dts_revert_test(void)
{
	int discontinuity;
	struct avtimeline_t line;

	avtimeline_init(&line, 5000, 334);
	assert(334 == avtimeline_input32(&line, 0, 8143402, 8143402, &discontinuity));
	assert(368 == avtimeline_input32(&line, 0, 8143436, 8143436, &discontinuity));
	assert(501 == avtimeline_input32(&line, 0, 8143569, 8143569, &discontinuity));
	assert(502 == avtimeline_input32(&line, 0, 8143502, 8143502, &discontinuity));
	assert(503 == avtimeline_input32(&line, 0, 8143536, 8143536, &discontinuity));
	assert(601 == avtimeline_input32(&line, 0, 8143669, 8143669, &discontinuity));
}

static void avtimeline_diff_start_test(void)
{
	int discontinuity;
	struct avtimeline_t line;

	avtimeline_init(&line, 5000, 0);
	assert(0 == avtimeline_input32(&line, 1, 0, 0, &discontinuity));
	assert(10 == avtimeline_input32(&line, 1, 10, 10, &discontinuity));
	assert(300 == avtimeline_input32(&line, 1, 300, 300, &discontinuity));
	assert(320 == avtimeline_input32(&line, 0, 320, 320, &discontinuity));
	assert(290 == avtimeline_input32(&line, 2, 290, 290, &discontinuity));
}

static void avtimeline_diff_start_test2(void)
{
	int discontinuity;
	struct avtimeline_t line;

	avtimeline_init(&line, 5000, 0);
	avtimeline_input32(&line, 1, 65368, 65408, &discontinuity);
	avtimeline_input32(&line, 1, 65402, 65442, &discontinuity);
	avtimeline_input32(&line, 1, 65435, 65475, &discontinuity);
	avtimeline_input32(&line, 1, 95443684, 7, &discontinuity);
	avtimeline_input32(&line, 1, 0, 40, &discontinuity);
	avtimeline_input32(&line, 1, 33, 73, &discontinuity);
}

void avtimeline_test(void)
{
	unsigned int seed;
	seed = (unsigned int)time(NULL);
	srand(seed);

	avtimeline_test1();
	avtimeline_gap_test();
	avtimeline_monotone_increment_test();
	avtimeline_dts_revert_test();
	avtimeline_diff_start_test();
	avtimeline_diff_start_test2();
}

#endif
