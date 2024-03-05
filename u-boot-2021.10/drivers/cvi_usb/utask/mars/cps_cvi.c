// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 bitmain
 */

#include <common.h>
#include <linux/types.h>
#include <stdlib.h>
#include <cpu_func.h>

/* see dps.h */
u32 cvi_uncached_read32(u32 *address)
{
	return *address;
}

/* see dps.h */
void cvi_uncached_write32(u32 value, u32 *address)
{
	*address = value;
}

/* see dps.h */
void cvi_buffer_copy(u8 *dst, u8 *src, u32 size)
{
	memcpy((void *)dst, (void *)src, size);
}

/* Since this is a bare-metal system, with no MMU in place, we expect that there will be no cache enabled */

void cvi_cache_invalidate(uintptr_t address, size_t size)
{
#ifdef TENSILICA
	xthal_dcache_region_invalidate(address, size);
#else
	invalidate_dcache_range(address, address + ROUND(size, CONFIG_SYS_CACHELINE_SIZE));
#endif
}

void cvi_cache_flush(uintptr_t address, size_t size)
{
#ifdef TENSILICA
	xthal_dcache_region_writeback(address, size);
#else
	flush_dcache_range(address, address + ROUND(size, CONFIG_SYS_CACHELINE_SIZE));
#endif
}

void cvi_delay_ns(u32 ns)
{
}
