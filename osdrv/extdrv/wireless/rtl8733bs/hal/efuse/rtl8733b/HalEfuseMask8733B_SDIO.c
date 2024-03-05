/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#include <drv_types.h>
#include "HalEfuseMask8733B_SDIO.h"

/******************************************************************************
*                           MSDIO.TXT
******************************************************************************/
u8 Array_MP_8733B_MSDIO_BT[] = {
	0x00,
	0x41,
	0x00,
	0x70,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xCF,
	0xFF,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

u8 Array_MP_8733B_MSDIO[] = {
	0xFF,
	0xF3,
	0xEF,
	0x0E,
	0x70,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x03,
	0xF3,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0x30,
	0xFF,
	0xFF,
	0xFF,
	0x10,
	0x10,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0xFF,
	0x30,
	0xFF,
	0x10,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

/*	BT eFuse Mask  */
u16 EFUSE_GetBTArrayLen_MP_8733B_MSDIO(void)
{
	return sizeof(Array_MP_8733B_MSDIO_BT) / sizeof(u8);
}

void EFUSE_GetBTMaskArray_MP_8733B_MSDIO(u8 *Array)
{
	u16 len = EFUSE_GetBTArrayLen_MP_8733B_MSDIO(), i = 0;

	for (i = 0; i < len; ++i)
		Array[i] = Array_MP_8733B_MSDIO_BT[i];
}

BOOLEAN EFUSE_IsBTAddressMasked_MP_8733B_MSDIO(u16 Offset)
{
	int r = Offset / 16;
	int c = (Offset % 16) / 2;
	int result = 0;

	if (c < 4) /*Upper double word*/
		result = (Array_MP_8733B_MSDIO_BT[r] & (0x10 << c));
	else
		result = (Array_MP_8733B_MSDIO_BT[r] & (0x01 << (c - 4)));

	return (result > 0) ? 0 : 1;
}


/*	WiFi eFuse Mask  */
u16
EFUSE_GetArrayLen_MP_8733B_MSDIO(void)
{
	return sizeof(Array_MP_8733B_MSDIO) / sizeof(u8);
}

void
EFUSE_GetMaskArray_MP_8733B_MSDIO(
		u8 *Array
)
{
	u16 len = EFUSE_GetArrayLen_MP_8733B_MSDIO(), i = 0;

	for (i = 0; i < len; ++i)
		Array[i] = Array_MP_8733B_MSDIO[i];
}

BOOLEAN
EFUSE_IsAddressMasked_MP_8733B_MSDIO(
		u16 Offset
)
{
	int r = Offset / 16;
	int c = (Offset % 16) / 2;
	int result = 0;
	/* Upper double word */
	if (c < 4)
		result = (Array_MP_8733B_MSDIO[r] & (0x10 << c));
	else
		result = (Array_MP_8733B_MSDIO[r] & (0x01 << (c - 4)));

	return (result > 0) ? 0 : 1;
}
