#include "cvi_misc.h"
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>


#ifdef ENABLE_TRACE
static int sTraceFD = -1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

static inline void init(void)
{

#ifdef ENABLE_TRACE
	const char *traceFileName = "/sys/kernel/debug/tracing/trace_marker";

	sTraceFD = open(traceFileName, O_WRONLY);
	if (sTraceFD == -1) {
		printf("error opening trace file, errno(%d), %s\n", errno, strerror(errno));
		// sEnabledTags remains zero indicating that no tracing can occur
	}
#endif
}

void CVI_SYS_TraceBegin(const char *name)
{

#ifdef ENABLE_TRACE
	pthread_mutex_lock(&lock);
	if (sTraceFD == -1) {
		init();
	}
	pthread_mutex_unlock(&lock);

	char buf[1024] = {};
	size_t len = snprintf(buf, 1024, "B|%d|%s", getpid(), name);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(sTraceFD, buf, len);
#pragma GCC diagnostic pop
#endif

}

void CVI_SYS_TraceCounter(const char *name, signed int value)
{

#ifdef ENABLE_TRACE
	char buf[1024] = {};

	snprintf(buf, 1024, "C|%d|%s|%d", getpid(), name, value);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(sTraceFD, buf, strlen(buf));
#pragma GCC diagnostic pop
#endif
}

void CVI_SYS_TraceEnd(void)
{

#ifdef ENABLE_TRACE
	char buf = 'E';
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
	write(sTraceFD, &buf, 1);
#pragma GCC diagnostic pop
#endif

}

#ifndef ENABLE_TRACE
#pragma GCC diagnostic pop
#endif
