/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef BYTEORDER_LENDIAN_H
#define BYTEORDER_LENDIAN_H

#include "swap.h"

#define swap_cpu_to_le32(x) (x)
#define swap_le32_to_cpu(x) (x)
#define swap_cpu_to_le16(x) ((uint16_t)(x))
#define swap_le16_to_cpu(x) ((uint16_t)(x))

#define swap_cpu_to_be32(x) ((uint32_t)swap32(x))
#define swap_be32_to_cpu(x) ((uint32_t)swap32(x))
#define swap_cpu_to_be16(x) ((uint16_t)swap16(x))
#define swap_be16_to_cpu(x) ((uint16_t)swap16(x))

/**
 * Macros used for reading 16-bits and 32-bits data from memory which
 * starting address could be unaligned.
 */
#define ptr_to_word(ptr) ((*(uint8_t *)(ptr) << 8) | (*(uint8_t *)(ptr + 1)))
#define ptr_to_dword(ptr) ((*(uint8_t *)(ptr) << 24) | ((*(uint8_t *)(ptr + 1)) << 16) | \
	(*((uint8_t *)(ptr + 2)) << 8) | (*((uint8_t *)(ptr + 3))))

#endif /* BYTEORDER_LENDIAN_H */

