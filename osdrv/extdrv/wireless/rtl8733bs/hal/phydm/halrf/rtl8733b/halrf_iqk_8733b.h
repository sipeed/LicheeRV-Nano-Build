/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#ifndef __HALRF_IQK_8733B_H__
#define __HALRF_IQK_8733B_H__

#if (RTL8733B_SUPPORT == 1)
/*--------------------------Define Parameters-------------------------------*/
#define MAC_REG_NUM_8733B 1
#define BB_REG_NUM_8733B 11
#define RF_REG_NUM_8733B 5
#define IQK_DELAY_8733B 2
#define IQK_STEP_8733B 3
#define RXK_STEP_8733B 6
#define IQK_CMD_8733B 0x8

#define TX_IQK 0
#define RX_IQK 1
#define RF_PATH_MAX_8733B 2

/*---------------------------End Define Parameters-------------------------------*/
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8733b(void *dm_void,
		  u8 delta_thermal_index,
		  u8 thermal_value,
		  u8 threshold);
#else
void do_iqk_8733b(void *dm_void,
		  u8 delta_thermal_index,
		  u8 thermal_value,
		  u8 threshold);
#endif

void phy_iq_calibrate_8733b(void *dm_void,
			    boolean is_recovery);
void _iqk_fill_iqk_xy_8733b(void *dm_void, u8 path);

#else /* (RTL8733B_SUPPORT == 0)*/

#define phy_iq_calibrate_8733b(_pdm_void, clear, segment_iqk)

#endif /* RTL8733B_SUPPORT */

#endif /*__HALRF_IQK_8733B_H__*/
