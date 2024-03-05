/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef DPS_H
#define DPS_H

#include "dwc2_stdtypes.h"
/****************************************************************************
 * Prototypes
 ***************************************************************************/

/**
 * Read a (32-bit) word
 * @param[in] address the address
 * @return the word at the given address
 */
extern uint32_t DWC2_ReadReg32(volatile uint32_t *address);

/**
 * Write a (32-bit) word to memory
 * @param[in] address the address
 * @param[in] value the word to write
 */
extern void DWC2_WriteReg32(uint32_t value, volatile uint32_t *address);
/**
 * Read a byte, bypassing the cache
 * @param[in] address the address
 * @return the byte at the given address
 */
extern uint8_t DWC2_UncachedRead8(volatile uint8_t *address);

/**
 * Read a short, bypassing the cache
 * @param[in] address the address
 * @return the short at the given address
 */
extern uint16_t DWC2_UncachedRead16(volatile uint16_t *address);

/**
 * Read a (32-bit) word, bypassing the cache
 * @param[in] address the address
 * @return the word at the given address
 */
extern uint32_t DWC2_UncachedRead32(volatile uint32_t *address);

/**
 * Write a byte to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the byte to write
 */
extern void DWC2_UncachedWrite8(uint8_t value, volatile uint8_t *address);

/**
 * Write a short to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the short to write
 */
extern void DWC2_UncachedWrite16(uint16_t value, volatile uint16_t *address);
/**
 * Write a (32-bit) word to memory, bypassing the cache
 * @param[in] address the address
 * @param[in] value the word to write
 */
extern void DWC2_UncachedWrite32(uint32_t value, volatile uint32_t *address);
/**
 * Write a (32-bit) address value to memory, bypassing the cache.
 * This function is for writing an address value, i.e. something that
 * will be treated as an address by hardware, and therefore might need
 * to be translated to a physical bus address.
 * @param[in] location the (CPU) location where to write the address value
 * @param[in] addrValue the address value to write
 */
extern void DWC2_WritePhysAddress32(uint32_t addrValue, volatile uint32_t *location);

/**
 * Hardware specific memcpy.
 * @param[in] src  src address
 * @param[in] dst  destination address
 * @param[in] size: size of the copy
 */
extern void DWC2_BufferCopy(volatile uint8_t *dst, volatile uint8_t *src, uint32_t size);

#endif /* DPS_H */
