/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef DPS_H
#define DPS_H

#include "cvi_stdtypes.h"

/****************************************************************************
 * Prototypes
 ***************************************************************************/

/**
 * Read a (32-bit) word
 * @param[in] address the address
 * @return the word at the given address
 */
extern u32 cvi_read32(volatile u32 *address);

/**
 * Write a (32-bit) word to memory
 * @param[in] address the address
 * @param[in] value the word to write
 */
extern void cvi_write32(u32 value, volatile u32 *address);
/**
 * Read a byte, bypassing the cache
 * @param[in] address the address
 * @return the byte at the given address
 */
extern u8 cvi_uncached_read8(volatile u8 *address);

/**
 * Read a short, bypassing the cache
 * @param[in] address the address
 * @return the short at the given address
 */
extern u16 cvi_uncached_read16(volatile u16 *address);

/**
 * Read a (32-bit) word, bypassing the cache
 * @param[in] address the address
 * @return the word at the given address
 */
extern u32 cvi_uncached_read32(volatile u32 *address);

/**
 * Write a byte to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the byte to write
 */
extern void cvi_uncached_write8(u8 value, volatile u8 *address);

/**
 * Write a short to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the short to write
 */
extern void cvi_uncached_write16(u16 value, volatile u16 *address);
/**
 * Write a (32-bit) word to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the word to write
 */
extern void cvi_uncached_write32(u32 value, volatile u32 *address);
/**
 * Write a (32-bit) address value to memory, bypassing the cache.
 * This function is for writing an address value, i.e. something that
 * will be treated as an address by hardware, and therefore might need
 * to be translated to a physical bus address.
 * @param[in] location the (CPU) location where to write the address value
 * @param[in] value the address value to write
 */
extern void cvi_write_phys32(u32 value, volatile u32 *location);

/**
 * Hardware specific memcpy.
 * @param[in] src  src address
 * @param[in] dst  destination address
 * @param[in] size size of the copy
 */
extern void cvi_buffer_copy(volatile u8 *dst, volatile u8 *src, u32 size);

/**
 * Invalidate the cache for the specified memory region.
 * This function may be stubbed out if caching is disabled for memory regions
 * as described in the driver documentation, or if the driver configuration does
 * not require this function.
 * @param[in] address Virtual address of memory region. (If an MMU is not in use,
 * this will be equivalent to the physical address.) This address should be
 * rounded down to the nearest cache line boundary.
 * @param[in] size  size of memory in bytes.  This size should be rounded up to
 * the nearest cache line boundary.  Use size UINTPTR_MAX to invalidate all
 * memory cache.  A size of 0 should be ignored and the function should return
 * immediately with no effect.
 * @param[in] devInfo   This parameter can be used to pass implementation specific
 * data to this function.  The content and use of this parameter is up to the
 * implementor of this function to determine, and if not required it may be ignored.
 *  For example, under Linux it can be used to pass a pointer to
 * the device struct to be used in a call to dma_sync_single_for_device().  If
 * used, the parameter should be passed to the core driver at initialisation as
 * part of the configurationInfo struct.  Please
 * see the core driver documentation for details of how to do this.
 */
extern void cvi_cache_invalidate(uintptr_t address, size_t size);

/**
 * Flush the cache for the specified memory region
 * This function may be stubbed out if caching is disabled for memory regions
 * as described in the driver documentation, or if the driver configuration does
 * not require this function.
 * @param[in] address Virtual address of memory region. (If an MMU is not in use,
 * this will be equivalent to the physical address.) This address should be
 * rounded down to the nearest cache line boundary.
 * @param[in] size  size of memory in bytes.  This size should be rounded up to
 * the nearest cache line boundary.  Use size UINTPTR_MAX to flush all
 * memory cache.  A size of 0 should be ignored and the function should return
 * immediately with no effect.
 * @param[in] devInfo   This parameter can be used to pass implementation specific
 * data to this function.  The content and use of this parameter is up to the
 * implementor of this function to determine, and if not required it may be ignored.
 *  For example, under Linux it can be used to pass a pointer to
 * the device struct to be used in a call to dma_sync_single_for_device().  If
 * used, the parameter should be passed to the core driver at initialisation as
 * part of the configurationInfo struct.  Please
 * see the core driver documentation for details of how to do this.
 */
extern void cvi_cache_flush(uintptr_t address, size_t size);

/**
 * Delay software execution by a number of nanoseconds
 * @param[in] ns number of nanoseconds to delay software execution
 */
extern void cvi_delay_ns(u32 ns);

#endif /* DPS_H */
