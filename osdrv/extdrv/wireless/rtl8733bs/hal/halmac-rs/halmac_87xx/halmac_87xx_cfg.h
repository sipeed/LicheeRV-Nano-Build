/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#ifndef _HALMAC_87XX_CFG_H_
#define _HALMAC_87XX_CFG_H_

#include "../halmac_api.h"

#if HALMAC_87XX_SUPPORT

#define TX_PAGE_SIZE_87XX		128
#define TX_PAGE_SIZE_SHIFT_87XX		7 /* 128 = 2^7 */
#define TX_ALIGN_SIZE_87XX		8
#define SDIO_TX_MAX_SIZE_87XX		31744
#define RX_BUF_FW_87XX			12288

#define TX_DESC_SIZE_87XX		40
#define RX_DESC_SIZE_87XX		24

#define H2C_PKT_SIZE_87XX		32 /* Only support 32 byte packet now */
#define H2C_PKT_HDR_SIZE_87XX		8
#define C2H_DATA_OFFSET_87XX		10
#define C2H_PKT_BUF_87XX		256

/* HW memory address */
#if HALMAC_8733B_SUPPORT
#define OCPBASE_TXBUF_87XX		0x18780000
#define OCPBASE_DMEM_87XX		0x14200000
#define OCPBASE_EMEM_87XX		0x14100000
#else
#define OCPBASE_TXBUF_87XX		0x18780000
#define OCPBASE_DMEM_87XX		0x00200000
#define OCPBASE_EMEM_87XX		0x00100000
#endif

#endif /* HALMAC_87XX_SUPPORT */

#endif
