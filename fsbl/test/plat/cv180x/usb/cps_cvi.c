// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dps.h>

/* see dps.h */
uint32_t DWC2_ReadReg32(volatile uint32_t *address)
{
	return *address;
}

/* see dps.h */
void DWC2_WriteReg32(uint32_t value, volatile uint32_t *address)
{
	*address = value;
}

/* see dps.h */
uint8_t DWC2_UncachedRead8(volatile uint8_t *address)
{
	return *address;
}

/* see dps.h */
uint16_t DWC2_UncachedRead16(volatile uint16_t *address)
{
	return *address;
}

/* see dps.h */
uint32_t DWC2_UncachedRead32(volatile uint32_t *address)
{
	return *address;
}

/* see dps.h */
void DWC2_UncachedWrite8(uint8_t value, volatile uint8_t *address)
{
	*address = value;
}

/* see dps.h */
void DWC2_UncachedWrite16(uint16_t value, volatile uint16_t *address)
{
	*address = value;
}

/* see dps.h */
void DWC2_UncachedWrite32(uint32_t value, volatile uint32_t *address)
{
	*address = value;
}

/* see dps.h */
void DWC2_WritePhysAddress32(uint32_t addrValue, volatile uint32_t *location)
{
	*location = addrValue;
}

/* see dps.h */
void DWC2_BufferCopy(volatile uint8_t *dst, volatile uint8_t *src, uint32_t size)
{
	memcpy((void *)dst, (void *)src, size);
}
