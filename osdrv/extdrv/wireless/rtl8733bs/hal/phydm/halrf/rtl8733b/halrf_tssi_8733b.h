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

#ifndef __HALRF_TSSI_8733b_H__
#define __HALRF_TSSI_8733b_H__

#if (RTL8733B_SUPPORT == 1)

/*--------------------------Define Parameters-------------------------------*/

/*---------------------------End Define Parameters----------------------------*/

void halrf_tssi_set_de_for_tx_verify_8733b(void *dm_void, u32 tssi_de, u8 path);

void halrf_tssi_dck_8733b(void *dm_void);

void halrf_tssi_clean_de_8733b(void *dm_void);

u32 halrf_get_online_tssi_de_8733b(void *dm_void, u8 path, s32 pout);

void halrf_tssi_get_efuse_8733b(void *dm_void);

u32 halrf_tssi_set_de_8733b(void *dm_void, u32 tssi_de);

void halrf_enable_tssi_8733b(void *dm_void);

void halrf_disable_tssi_8733b(void *dm_void);

void halrf_do_tssi_8733b(void *dm_void);

void _halrf_tssi_8733b(void *dm_void, u8 path);

void _halrf_tssi_set_powerlevel_8733b(void *dm_void, s16 power_offset, u8 path);

u32 halrf_tssi_set_powerbyrate_pout_8733b(void *dm_void, s16 power_offset, u8 path);

#endif /* RTL8733b_SUPPORT */
#endif /*#ifndef __HALRF_TSSI_8733b_H__*/
