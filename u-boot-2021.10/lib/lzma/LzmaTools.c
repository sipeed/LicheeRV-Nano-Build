// SPDX-License-Identifier: GPL-2.0+
/*
 * Usefuls routines based on the LzmaTest.c file from LZMA SDK 4.65
 *
 * Copyright (C) 2007-2009 Industrie Dial Face S.p.A.
 * Luigi 'Comio' Mantellini (luigi.mantellini@idf-hit.com)
 *
 * Copyright (C) 1999-2005 Igor Pavlov
 */

/*
 * LZMA_Alone stream format:
 *
 * uchar   Properties[5]
 * uint64  Uncompressed size
 * uchar   data[*]
 *
 */

#include <config.h>
#include <common.h>
#include <log.h>
#include <watchdog.h>

#ifdef CONFIG_LZMA

#define LZMA_PROPERTIES_OFFSET 0
#define LZMA_SIZE_OFFSET       LZMA_PROPS_SIZE
#define LZMA_DATA_OFFSET       LZMA_SIZE_OFFSET + sizeof(uint64_t)

#include "LzmaTools.h"
#include "LzmaDec.h"

#include <linux/string.h>
#include <malloc.h>

static void *sz_alloc(void *p, size_t size) { return malloc(size); }
static void sz_free(void *p, void *address) { free(address); }

int lzma_buff_to_buff_decompress(unsigned char *out_stream, size_t *uncompressed_size,
				 unsigned char *in_stream, size_t length)
{
	int res = SZ_ERROR_DATA;
	int i;
	i_sz_alloc g_alloc;

	size_t out_size_full = 0xFFFFFFFF; /* 4GBytes limit */
	size_t out_processed;
	size_t out_size;
	size_t out_size_high;
	e_lzma_status state;
	size_t compressed_size = (size_t)(length - LZMA_PROPS_SIZE);

	debug("LZMA: Image address............... 0x%p\n", in_stream);
	debug("LZMA: Properties address.......... 0x%p\n", in_stream + LZMA_PROPERTIES_OFFSET);
	debug("LZMA: Uncompressed size address... 0x%p\n", in_stream + LZMA_SIZE_OFFSET);
	debug("LZMA: Compressed data address..... 0x%p\n", in_stream + LZMA_DATA_OFFSET);
	debug("LZMA: Destination address......... 0x%p\n", out_stream);

	memset(&state, 0, sizeof(state));

	out_size = 0;
	out_size_high = 0;
	/* Read the uncompressed size */
	for (i = 0; i < 8; i++) {
		unsigned char b = in_stream[LZMA_SIZE_OFFSET + i];

		if (i < 4)
			out_size     += (uint32_t)(b) << (i * 8);
		else
			out_size_high += (uint32_t)(b) << ((i - 4) * 8);
	}

	out_size_full = (size_t)out_size;
	if (sizeof(size_t) >= 8) {
		/*
		 * size_t is a 64 bit uint => We can manage files larger than 4GB!
		 *
		 */
		out_size_full |= (((size_t)out_size_high << 16) << 16);
	} else if (out_size_high != 0 || (uint32_t)(size_t)out_size != out_size) {
		/*
		 * size_t is a 32 bit uint => We cannot manage files larger than
		 * 4GB!  Assume however that all 0xf values is "unknown size" and
		 * not actually a file of 2^64 bits.
		 *
		 */
		if (out_size_high != (size_t)-1 || out_size != (size_t)-1) {
			debug("LZMA: 64bit support not enabled.\n");
			return SZ_ERROR_DATA;
		}
	}

	debug("LZMA: Uncompresed size............ 0x%lx\n", out_size_full);
	debug("LZMA: Compresed size.............. 0x%lx\n", compressed_size);

	g_alloc.alloc = sz_alloc;
	g_alloc.free = sz_free;

	/* Short-circuit early if we know the buffer can't hold the results. */
	if (out_size_full != (size_t)-1 && *uncompressed_size < out_size_full)
		return SZ_ERROR_OUTPUT_EOF;

	/* Decompress */
	out_processed = min(out_size_full, *uncompressed_size);

	WATCHDOG_RESET();

	res = lzma_decode(out_stream, &out_processed,
			  in_stream + LZMA_DATA_OFFSET, &compressed_size,
			  in_stream, LZMA_PROPS_SIZE, LZMA_FINISH_END, &state, &g_alloc);
	*uncompressed_size = out_processed;

	debug("LZMA: Uncompressed ............... 0x%lx\n", out_processed);

	if (res != SZ_OK)
		return res;

	return res;
}

#endif
