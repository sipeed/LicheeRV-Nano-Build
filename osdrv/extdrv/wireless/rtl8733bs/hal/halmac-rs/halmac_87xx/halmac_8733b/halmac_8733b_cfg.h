/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
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

#ifndef _HALMAC_8733B_CFG_H_
#define _HALMAC_8733B_CFG_H_

#include "../../halmac_hw_cfg.h"
#include "../halmac_87xx_cfg.h"

#if HALMAC_8733B_SUPPORT

#define TX_FIFO_SIZE_8733B	32768
#define RX_FIFO_SIZE_8733B	16384
#define TRX_SHARE_SIZE0_8733B	8192
#define TRX_SHARE_SIZE1_8733B	8192
#define TRX_SHARE_SIZE2_8733B	8192

#define TX_FIFO_SIZE_LA_8733B	(TX_FIFO_SIZE_8733B >>  1)
#define TX_FIFO_SIZE_RX_EXPAND_1BLK_8733B	\
	(TX_FIFO_SIZE_8733B - TRX_SHARE_SIZE0_8733B)
#define RX_FIFO_SIZE_RX_EXPAND_1BLK_8733B	\
	(RX_FIFO_SIZE_8733B + TRX_SHARE_SIZE0_8733B)
#define TX_FIFO_SIZE_RX_EXPAND_2BLK_8733B	\
	(TX_FIFO_SIZE_8733B - TRX_SHARE_SIZE2_8733B)
#define RX_FIFO_SIZE_RX_EXPAND_2BLK_8733B	\
	(RX_FIFO_SIZE_8733B + TRX_SHARE_SIZE2_8733B)
#define TX_FIFO_SIZE_RX_EXPAND_3BLK_8733B	\
	(TX_FIFO_SIZE_8733B - TRX_SHARE_SIZE2_8733B - TRX_SHARE_SIZE0_8733B)
#define RX_FIFO_SIZE_RX_EXPAND_3BLK_8733B	\
	(RX_FIFO_SIZE_8733B + TRX_SHARE_SIZE2_8733B + TRX_SHARE_SIZE0_8733B)
#define TX_FIFO_SIZE_RX_EXPAND_4BLK_8733B	\
	(TX_FIFO_SIZE_8733B - (2 * TRX_SHARE_SIZE2_8733B))
#define RX_FIFO_SIZE_RX_EXPAND_4BLK_8733B	\
	(RX_FIFO_SIZE_8733B + (2 * TRX_SHARE_SIZE2_8733B))

#define EFUSE_SIZE_8733B	512
#define EEPROM_SIZE_8733B	768
//#define BT_EFUSE_SIZE_8733B	128
#define BT_EFUSE_SIZE_8733B	512  //jerry_zhou
#define PRTCT_EFUSE_SIZE_8733B	124

#define SEC_CAM_NUM_8733B	32

#define OQT_ENTRY_AC_8733B	32
#define OQT_ENTRY_NOAC_8733B	32
#define MACID_MAX_8733B		128

#define WLAN_FW_IRAM_MAX_SIZE_8733B	65536
#define WLAN_FW_DRAM_MAX_SIZE_8733B	65536
#define WLAN_FW_ERAM_MAX_SIZE_8733B	131072
#define WLAN_FW_MAX_SIZE_8733B		(WLAN_FW_IRAM_MAX_SIZE_8733B + \
	WLAN_FW_DRAM_MAX_SIZE_8733B + WLAN_FW_ERAM_MAX_SIZE_8733B)

#endif /* HALMAC_8733B_SUPPORT*/

#endif