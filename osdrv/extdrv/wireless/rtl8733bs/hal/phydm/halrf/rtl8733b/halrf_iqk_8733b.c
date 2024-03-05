// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2018  Realtek Corporation.
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

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == 0x08)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8733B_SUPPORT == 1)

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8733b(void *dm_void,
		  u8 delta_thermal_index,
		  u8 thermal_value,
		  u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	//odm_reset_iqk_result(dm);
	dm->rf_calibrate_info.thermal_value_iqk = thermal_value;
	halrf_iqk_trigger(dm, false);
}
#else
void do_iqk_8733b(void *dm_void,
		  u8 delta_thermal_index,
		  u8 thermal_value,
		  u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean is_recovery = (boolean)delta_thermal_index;

	halrf_iqk_trigger(dm, false);
}
#endif

void _iqk_information_8733b(struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	u32  reg_rf18;

	if (odm_get_bb_reg(dm, R_0x1e7c, BIT(30)))
		iqk_info->is_tssi_mode = true;
	else
		iqk_info->is_tssi_mode = false;

	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);
	iqk_info->iqk_band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	iqk_info->iqk_ch = (u8)reg_rf18 & 0xff;
	iqk_info->iqk_bw = (u8)((reg_rf18 & 0x400) >> 10); /*3/2/1:20/40/80*/
/*
	RF_DBG(dm, DBG_RF_DPK, "[IQK] TSSI/ Band/ CH/ BW = %d / %s / %d / %s\n",
	       iqk_info->is_tssi_mode, iqk_info->iqk_band == 0 ? "2G" : "5G",
	       iqk_info->iqk_ch,
	       iqk_info->iqk_bw == 3 ? "20M" : (iqk_info->iqk_bw == 2 ? "40M" : "80M"));
*/
	RF_DBG(dm, DBG_RF_IQK, "[IQK] TSSI/ Band/ CH/ BW = %d / %s / %d / %s\n",
	       iqk_info->is_tssi_mode, iqk_info->iqk_band == 0 ? "2G" : "5G",
	       iqk_info->iqk_ch, iqk_info->iqk_bw == 0 ? "40MHz" : "20MHz");
}

void _iqk_set_gnt_wl_gnt_bt_8733b(struct dm_struct *dm, boolean beforeK)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
#if 0
	if (beforeK) {
		_iqk_set_gnt_wl_high_8822c(dm);
		_iqk_set_gnt_bt_low_8822c(dm);
	} else {
		_iqk_btc_write_indirect_reg_8822c(dm, 0x38, MASKDWORD, iqk_info->tmp_gntwl);
	}
#endif
}

u8 _iqk_switch_rf_path_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	//struct dm_iqk_info *iqk = &dm->IQK_info;
	u8 s;

	RF_DBG(dm, DBG_RF_IQK, "[IQK] ======>%s\n", __func__);

	s = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	if (s == 0)
		return RF_PATH_A;
	else
		return RF_PATH_B;
}

void _iqk_fill_iqk_xy_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk = &dm->IQK_info;
	u8 i;

	RF_DBG(dm, DBG_RF_IQK, "[IQK] ======>%s\n", __func__);
	switch (path) {
	case RF_PATH_A:
		odm_set_bb_reg(dm, 0x1b38, 0x3, 0x1);
		odm_set_bb_reg(dm, 0x1b38, 0x7FF00000, iqk->txxy[0][0]);
		odm_set_bb_reg(dm, 0x1b38, 0x0007FF00, iqk->txxy[0][1]);

		//lna=0x3
		odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x07);
		odm_set_bb_reg(dm, 0x1b3c, 0x7FF00000, iqk->rxxy[0][0][0]);
		odm_set_bb_reg(dm, 0x1b3c, 0x0007FF00, iqk->rxxy[0][1][0]);
		
		// lna=0x6
		odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x0D);
		odm_set_bb_reg(dm, 0x1b3c, 0x7FF00000, iqk->rxxy[0][0][1]);
		odm_set_bb_reg(dm, 0x1b3c, 0x0007FF00, iqk->rxxy[0][1][1]);
		odm_set_bb_reg(dm, 0x1b38, 0x3, 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] S0 0x1b38 = 0x%x, 0x1b3c = 0x%x\n",
			odm_get_bb_reg(dm, 0x1b38, MASKDWORD),
			odm_get_bb_reg(dm, 0x1b3C, MASKDWORD));
		break;
	case RF_PATH_B:
		odm_set_bb_reg(dm, 0x1b38, 0x3, 0x3);
		odm_set_bb_reg(dm, 0x1b38, 0x7FF00000, iqk->txxy[1][0]);
		odm_set_bb_reg(dm, 0x1b38, 0x0007FF00, iqk->txxy[1][1]);

		//lna=0x3
		odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x07);
		odm_set_bb_reg(dm, 0x1b3c, 0x7FF00000, iqk->rxxy[1][0][0]);
		odm_set_bb_reg(dm, 0x1b3c, 0x0007FF00, iqk->rxxy[1][1][0]);
		
		// lna=0x6
		odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x0D);
		odm_set_bb_reg(dm, 0x1b3c, 0x7FF00000, iqk->rxxy[1][0][1]);
		odm_set_bb_reg(dm, 0x1b3c, 0x0007FF00, iqk->rxxy[1][1][1]);

		odm_set_bb_reg(dm, 0x1b38, 0x3, 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] S1 0x1b38 = 0x%x, 0x1b3c = 0x%x\n",
			odm_get_bb_reg(dm, 0x1b38, MASKDWORD),
			odm_get_bb_reg(dm, 0x1b3C, MASKDWORD));
		break;
	default:
		break;
	}

}

#if 0
void _iqk_fail_count_8822c(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i;

	dm->n_iqk_cnt++;
	if (odm_get_rf_reg(dm, RF_PATH_A, RF_0x1bf0, BIT(16)) == 1)
		iqk_info->is_reload = true;
	else
		iqk_info->is_reload = false;

	if (!iqk_info->is_reload) {
		for (i = 0; i < 8; i++) {
			if (odm_get_bb_reg(dm, R_0x1bf0, BIT(i)) == 1)
				dm->n_iqk_fail_cnt++;
		}
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]All/Fail = %d %d\n", dm->n_iqk_cnt, dm->n_iqk_fail_cnt);
}

void _iqk_iqk_fail_report_8822c(struct dm_struct *dm)
{
	u32 tmp1bf0 = 0x0;
	u8 i;

	tmp1bf0 = odm_read_4byte(dm, 0x1bf0);

	for (i = 0; i < 4; i++) {
		if (tmp1bf0 & (0x1 << i))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			RF_DBG(dm, DBG_RF_IQK, "[IQK] please check S%d TXIQK\n", i);
#else
			panic_printk("[IQK] please check S%d TXIQK\n", i);
#endif
		if (tmp1bf0 & (0x1 << (i + 12)))
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
			RF_DBG(dm, DBG_RF_IQK, "[IQK] please check S%d RXIQK\n", i);
#else
			panic_printk("[IQK] please check S%d RXIQK\n", i);
#endif
	}
}
#endif
void _iqk_backup_mac_bb_8733b(struct dm_struct *dm,
			      u32 *MAC_backup,
			      u32 *BB_backup,
			      u32 *backup_mac_reg,
			      u32 *backup_bb_reg)
{
	u32 i;

	for (i = 0; i < MAC_REG_NUM_8733B; i++) {
		MAC_backup[i] = odm_read_4byte(dm, backup_mac_reg[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]Backup mac addr = %x, value =% x\n", backup_mac_reg[i], MAC_backup[i]);
	}
	for (i = 0; i < BB_REG_NUM_8733B; i++) {
		BB_backup[i] = odm_read_4byte(dm, backup_bb_reg[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]Backup bbaddr = %x, value =% x\n", backup_bb_reg[i], BB_backup[i]);
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]BackupMacBB Success!!!!\n");
}

void _iqk_backup_rf_8733b(struct dm_struct *dm,
			  u32 RF_backup[][RF_PATH_MAX_8733B],
			  u32 *backup_rf_reg)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 i;

	for (i = 0; i < RF_REG_NUM_8733B; i++) {
		RF_backup[i][RF_PATH_A] = odm_get_rf_reg(dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK);
		RF_backup[i][RF_PATH_B] = odm_get_rf_reg(dm, RF_PATH_B, backup_rf_reg[i], RFREGOFFSETMASK);
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]BackupRF Success!!!!\n");
}

void _iqk_restore_rf_8733b(struct dm_struct *dm,
			  u32 *rf_reg,
			  u32 temp[][RF_PATH_MAX_8733B])
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 i;

	for (i = 0; i < RF_REG_NUM_8733B; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, rf_reg[i],
			       0xfffff, temp[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, rf_reg[i],
			       0xfffff, temp[i][RF_PATH_B]);
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RestoreRF Success!!!!\n");
}

void _iqk_bb_for_dpk_setting_8733b(struct dm_struct *dm)
{
#if 0
	odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, BIT(28), 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, BIT(29), 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, BIT(30), 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, BIT(31), 0x0);
	//odm_set_bb_reg(dm, R_0x1c68, 0x0f000000, 0xf);
	odm_set_bb_reg(dm, 0x1d58, 0xff8, 0x1ff);
	odm_set_bb_reg(dm, 0x1864, BIT(31), 0x1);
	odm_set_bb_reg(dm, 0x4164, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x410c, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x186c, BIT(7), 0x1);
	odm_set_bb_reg(dm, 0x416c, BIT(7), 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x0); //S0 -3 wire
	odm_set_bb_reg(dm, R_0x410c, 0x3, 0x0); //S1 -3wire
	odm_set_bb_reg(dm, 0x1a00, BIT(1) | BIT(0), 0x2);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_bb_for_dpk_setting_8822c!!!!\n");
#endif
}

void _iqk_afe_setting_8733b(struct dm_struct *dm,	boolean do_iqk)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	if (do_iqk) {
		/*01_8733B_AFE_ON_BB_settings.txt*/
		//--BB settings--//
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);//IQK&DPK KIP power on
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x0); //r_path_en_en
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000F, 0x1); //wire [3:0] path_en_seg0_sel
		odm_set_bb_reg(dm, R_0x824, 0x000F0000, 0x1); //wire [3:0] path_en_seg0_sel
		//IQK clk on
		odm_set_bb_reg(dm, R_0x1cd0, 0xF0000000, 0x7);
		//Block CCA
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1); //Prevent CCKCCA at sine PSD
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x1); //Prevent OFDM CCA
		//trx gating clk force on
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x1);
		//go through iqk
		odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);
		
		//wire r_iqk_IO_RFC_en
		//odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		
		//--AFE on settings (ADC/DAC both on)
		//ADDA fifo force off until clk is ready
		odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		//Anapar force mode
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0); //force ADDA
		odm_set_bb_reg(dm, R_0x1860, 0xF0000000, 0xf); //ADDA all on
		odm_set_bb_reg(dm, R_0x1860, 0x0FFFF000, 0x0041); //ADDA all on
		//AD CLK rate,160M
		//odm_set_bb_reg(dm, R_0x1830, 0x0000FFFF, 0xcccc);
		//odm_set_bb_reg(dm, 0x1d40, BIT(3), 0x1);
		//odm_set_bb_reg(dm, 0x1d40, 0x00000007, 0x4);
		//DAC clk rate,80M
		odm_set_bb_reg(dm, 0x09f0, 0x0000FFFF, 0xbbbb);
		odm_set_bb_reg(dm, 0x1d40, BIT(3), 0x1);
		odm_set_bb_reg(dm, 0x1d40, 0x00000007, 0x3);
		//DAC clk rate
		//DA: 160M
		odm_set_bb_reg(dm, 0x09b4, 0x00000700, 0x3);//[10:8]
		odm_set_bb_reg(dm, 0x09b4, 0x00003800, 0x3);//[13:11]
		odm_set_bb_reg(dm, 0x09b4, 0x0001C000, 0x3);//[16:14]
		odm_set_bb_reg(dm, 0x09b4, 0x000E0000, 0x3);//[19:17]
		odm_set_bb_reg(dm, R_0x1c20, BIT(5), 0x1);
		//Release ADDA fifo force off
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);

		//odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0xA2103C02);

		RF_DBG(dm, DBG_RF_IQK, "[IQK]AFE setting for IQK mode!!!!\n");
	} else {
		/*11_8733B_restore_AFE_BB_settings.txt*/
		// AFE Restore Settings
		//ADDA fifo force off until clk is ready
		//odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		//Anapar force mode
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
		//force Path en
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x1);
		//Block CCA
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x0);
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x0);

		//trx gating clk force on
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x0);
		//wire r_iqk_IO_RFC_en
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0);
		//Release ADDA fifo force off
		//check values before DPK
		

		RF_DBG(dm, DBG_RF_IQK, "[IQK]AFE restore for Normal mode!!\n");
	}
//#endif
}

void _iqk_preset_8733b(struct dm_struct *dm,	boolean do_iqk)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	if (do_iqk) {
		/*02_IQK_Preset.txt*/
		// RF does not control by bb
		odm_set_rf_reg(dm, RF_PATH_A, 0x05, BIT(0), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0x05, BIT(0), 0x0);
		//[7]cip_power_on
		//odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x00000080);
		//write SRAM
		//debug check performance  [1:0]=2'b10 IQK
		//odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x00000002);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]iqk_preset\n");

	} else {
		/*10_IQK_Reg_Restore.txt*/

		// RF Restore
		//0xEF[2]=WE_LUT_TX_LOK=1
		odm_set_rf_reg(dm, RF_PATH_A, 0x05, BIT(0), 0x1);
		odm_set_rf_reg(dm, RF_PATH_B, 0x05, BIT(0), 0x1);

		odm_set_rf_reg(dm, RF_PATH_A, 0xdf, BIT(12), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xdf, BIT(12), 0x0);

		odm_set_rf_reg(dm, RF_PATH_A, 0xef, BIT(2), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xef, BIT(2), 0x0);

		odm_set_rf_reg(dm, RF_PATH_A, 0xde, 0xFE000, 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xde, 0xFE000, 0x0);

		odm_set_rf_reg(dm, RF_PATH_A, 0xef, BIT(2), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xef, BIT(2), 0x0);

		// [7]cip_power_on
		odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x00000000);
		// [2]lna_sel
		odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x00);
		//[0]:dbg_WLS1_BB_SEL_mod
		odm_set_bb_reg(dm, 0x1b38, BIT(0), 0x0);
		// b8[20]= tst_iqk2set
		odm_set_bb_reg(dm, 0x1bb8, MASKDWORD, 0x00000000);
		// [5:0]=ItQt
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003F, 0x0);
		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]iqk_reg_restore\n");

		}

}

void _iqk_restore_mac_bb_8733b(struct dm_struct *dm,
			       u32 *MAC_backup,
			       u32 *BB_backup,
			       u32 *backup_mac_reg,
			       u32 *backup_bb_reg)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 i;

	for (i = 0; i < MAC_REG_NUM_8733B; i++) {
		odm_write_4byte(dm, backup_mac_reg[i], MAC_backup[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]restore mac = %x, value = %x\n",backup_mac_reg[i],MAC_backup[i]);
		}
	for (i = 0; i < BB_REG_NUM_8733B; i++) {
		odm_write_4byte(dm, backup_bb_reg[i], BB_backup[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]restore bb = %x, value = %x\n",backup_bb_reg[i],BB_backup[i]);
		}
	/*rx go throughput IQK*/
#if 0
	odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);
	odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);

	if (iqk_info->iqk_fail_report[0][0][RXIQK] == 1)
		odm_set_bb_reg(dm, 0x180c, BIT(31), 0x0);
	else
		odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);

	if (iqk_info->iqk_fail_report[0][1][RXIQK] == 1)
		odm_set_bb_reg(dm, 0x410c, BIT(31), 0x0);
	else
		odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);
#endif
	//odm_set_bb_reg(dm, 0x520c, BIT(31), 0x1);
	//odm_set_bb_reg(dm, 0x530c, BIT(31), 0x1);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RestoreMacBB Success!!!!\n");
}


void _iqk_backup_iqk_8733b(struct dm_struct *dm, u8 step, u8 path)
{
	struct dm_iqk_info *iqk = &dm->IQK_info;
	u8 i, j, k;
}

boolean
_iqk_reload_iqk_8733b(struct dm_struct *dm,	boolean reset)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i;

	iqk_info->is_reload = false;
#if 0
	if (reset) {
		for (i = 0; i < 2; i++)
			iqk_info->iqk_channel[i] = 0x0;
	} else {
		iqk_info->rf_reg18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREGOFFSETMASK);

		for (i = 0; i < 2; i++) {
			if (iqk_info->rf_reg18 == iqk_info->iqk_channel[i]) {
				_iqk_reload_iqk_setting_8822c(dm, i, 2);
				_iqk_fill_iqk_report_8822c(dm, i);
				RF_DBG(dm, DBG_RF_IQK, "[IQK]reload IQK result before!!!!\n");
				iqk_info->is_reload = true;
			}
		}
	}
	/*report*/
	odm_set_bb_reg(dm, R_0x1bf0, BIT(16), (u8)iqk_info->is_reload);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xdf, BIT(4), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xdf, BIT(4), 0x0);
#endif

	return iqk_info->is_reload;
}

void _iqk_rfe_setting_8733b(struct dm_struct *dm, boolean ext_pa_on)
{
	/*TBD*/
	return;
#if 0
	if (ext_pa_on) {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77777777);
		odm_write_4byte(dm, 0xcb4, 0x00007777);
		odm_write_4byte(dm, 0xcbc, 0x0000083B);
		odm_write_4byte(dm, 0xeb0, 0x77777777);
		odm_write_4byte(dm, 0xeb4, 0x00007777);
		odm_write_4byte(dm, 0xebc, 0x0000083B);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		RF_DBG(dm, DBG_RF_IQK, "[IQK]external PA on!!!!\n");
	} else {
		/*RFE setting*/
		odm_write_4byte(dm, 0xcb0, 0x77777777);
		odm_write_4byte(dm, 0xcb4, 0x00007777);
		odm_write_4byte(dm, 0xcbc, 0x00000100);
		odm_write_4byte(dm, 0xeb0, 0x77777777);
		odm_write_4byte(dm, 0xeb4, 0x00007777);
		odm_write_4byte(dm, 0xebc, 0x00000100);
		/*odm_write_4byte(dm, 0x1990, 0x00000c30);*/
		/*RF_DBG(dm, DBG_RF_IQK, "[IQK]external PA off!!!!\n");*/
	}
#endif
}

void _iqk_reload_lok_setting_8733b(struct dm_struct *dm,	u8 path)
{
#if 0
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tmp;
	u8 idac_i, idac_q;
	u8 i;

	idac_i = (u8)((iqk_info->rf_reg58 & 0xfc000) >> 14);
	idac_q = (u8)((iqk_info->rf_reg58 & 0x3f00) >> 8);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf, BIT(4), 0x0);//W LOK table
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1);

	if (*dm->band_type == ODM_BAND_2_4G)
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x00);
	else
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x20);

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x08, 0xfc000, idac_i);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x08, 0x003f0, idac_q);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x0);// stop write

	tmp = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x58, 0xfffff);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d,reload 0x58 = 0x%x\n", path, tmp);
#endif
}

void _iqk_lok_for_rxk_setting_8733b(struct dm_struct *dm,	u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean is_NB_IQK = false;
#if 0
	if ((*dm->band_width == CHANNEL_WIDTH_5) || (*dm->band_width == CHANNEL_WIDTH_10))
		is_NB_IQK = true;

	//_iqk_cal_path_off_8822c(dm);
	odm_write_4byte(dm, 0x1b00, 0x8 | path << 1);
	odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
	odm_set_bb_reg(dm, 0x1b20, BIT(31) | BIT(30), 0x0);

	//force 0x53[0]=1, force PA on
	odm_set_rf_reg(dm, (enum rf_path)path, 0x53, BIT(0), 0x1);

	//LOK_RES Table
	if (*dm->band_type == ODM_BAND_2_4G) {
		odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0xf0000, 0x7);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x9e, BIT(5), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x9e, BIT(10), 0x1);
		odm_set_bb_reg(dm, 0x1b20, 0x3e0, 0x12);// 12dB
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xde, BIT(16), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x56, 0xfff, 020);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(2), 0x1);
		//odm_set_rf_reg(dm, (enum rf_path)path, RF_0x18, BIT(16), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, BIT(0), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x08, 0x70, 0x4);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(2), 0x0);
	} else {
		odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0xf0000, 0x7);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x9e, BIT(5), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x9e, BIT(10), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xde, BIT(16), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x56, 0xfff, 0x000);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(2), 0x1);
		//odm_set_rf_reg(dm, (enum rf_path)path, RF_0x18, BIT(16), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, BIT(0), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x08, 0x70, 0x4);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(2), 0x0);
	}
		odm_set_rf_reg(dm, (enum rf_path)path, 0x57, BIT(0), 0x0);

	//TX_LOK
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1);
	if (*dm->band_type == ODM_BAND_2_4G) {
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x00);
		odm_write_1byte(dm, 0x1bcc, 0x09);
	} else {
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x20);
		odm_write_1byte(dm, 0x1bcc, 0x09);
	}
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1); //LOK _Write_en
	odm_write_1byte(dm, 0x1b10, 0x0);
	odm_write_1byte(dm, 0x1bcc, 0x12);
	if (is_NB_IQK)
		odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x008);
	else
		odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x038);
#endif
}

boolean
_lok_load_default_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	u32 temp;
	u8 idac_i, idac_q;
	u8 i;
#if 0
	_iqk_cal_path_off_8733b(dm);
	odm_write_4byte(dm, 0x1b00, 0x8 | path << 1);

	temp = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x58, 0xfffff);
	//RF_DBG(dm, DBG_RF_IQK, "[IQK](1)setlut_0x58 = 0x%x\n", temp);
	idac_i = (u8)((temp & 0xfc000) >> 14);
	idac_q = (u8)((temp & 0x3f00) >> 8);

	if (!(idac_i == 0x0 || idac_i == 0x3f || idac_q == 0x0 || idac_q == 0x3f)) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]LOK 0x58 = 0x%x\n", temp);
		return false;
	}

	idac_i = 0x20;
	idac_q = 0x20;

	odm_set_rf_reg(dm, (enum rf_path)path, 0x57, BIT(0), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1);

	if (*dm->band_type == ODM_BAND_2_4G)
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x0);
	else
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33, 0x7f, 0x20);

	//_iqk_rf_direct_access_8822c(dm, (enum rf_path)path, false);

	odm_set_rf_reg(dm, (enum rf_path)path, 0x08, 0x003f0, idac_i);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x08, 0xfc000, idac_q);

	temp = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x08, 0xfffff);
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)setlut_0x08 = 0x%x\n", temp);

	temp = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x58, 0xfffff);
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)setlut_0x58 = 0x%x\n", temp);
	//_iqk_rf_direct_access_8822c(dm, (enum rf_path)path, true);

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x0);
#endif
	return true;
}

void _iqk_rxk_rf_setting_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tx_pi_data, rf_reg1F;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]rxk START : Set RF Setting!\n");
	odm_set_bb_reg(dm, R_0x1860, BIT(30), 0x0); // DAC off

	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000, 0x7);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000, 0x3);
	} else if (path == RF_PATH_B) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000, 0x7);  //0xC
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x88, 0x0000F, 0x3);
	}
	if (iqk_info->iqk_band == 0)
		odm_set_rf_reg(dm, path, RF_0x20, BIT(8), 0x1);
	else
		odm_set_rf_reg(dm, path, RF_0x20, BIT(7), 0x1);

	//IQKPLL_MODE_AG: 1F[17:16]  (same as 18[17:16])
	//IQKPLL_CH: 1F[9:0]	     (same as 18[9:0])
	//other 0x1F bit are reserved, so it is ok to copy 0x18 to 0x1f.
	rf_reg1F = odm_get_rf_reg(dm, path, RF_0x18, 0xFFFFF);
	odm_set_rf_reg(dm, path, RF_0x1f, 0xFFFFF, rf_reg1F);
	// 1E[5:0]=IQKPLL_FOS=6'h13 (4.25MHz)
	odm_set_rf_reg(dm, path, RF_0x1e, 0x0003F, 0x13);
	//1E[19]=POW_IQKPLL
	odm_set_rf_reg(dm, path, RF_0x1e, BIT(19), 0x0);
	odm_set_rf_reg(dm, path, RF_0x1e, BIT(19), 0x1);
	// IQKPLL's settling time needs 60us.
	ODM_delay_ms(1);
}

void _iqk_txk_rf_setting_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tx_pi_data;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]txk START : Set RF Setting!\n");
	// ----- START : Set RF Setting -----
	// 0xDE[14]=DEBUG_LUT_TX_TRACK=1
	// 0xDE[13]=DEBUG_LUT_TX_POWER=1
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xde, 0xFE000, 0x3f);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xde, 0xFE000, 0x3f);
	if (path == RF_PATH_A) {
		// Gmode : {0x51[19],0x51[11],0x52[11]}=Att_SMXR
		// Amode : 0x60[2:0]=Att_SMXR
		odm_set_rf_reg(dm, path, RF_0x60, 0x00007, 0x7);
		if (iqk_info->iqk_band == 0) {//G mode
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x51, BIT(19), 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x51, BIT(11), 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x52, BIT(11), 0x0);
		} 
	} else if (path == RF_PATH_B) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x51, BIT(19), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x51, BIT(11), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x52, BIT(11), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x51, BIT(19), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x51, BIT(11), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x52, BIT(11), 0x0);
	}
	// for LOK Setting
	//0x55[0]=EN_TXGAIN_FOR_LOK=0
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x55, BIT(0), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x55, BIT(0), 0x0);
	// 0xEF[2]=WE_LUT_TX_LOK=1
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, BIT(2), 0x1);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xef, BIT(2), 0x1);
	//0xDF[2]=DEBUG_LUT_TX_LOK
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xdf, BIT(2), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xdf, BIT(2), 0x0);
	// LOK WA_1 Gmode
	if (iqk_info->iqk_band == 0) {//G mode
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, 0x003FF, 0x000);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x33, 0x003FF, 0x000);
	} else {		    //A mode
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, 0x003FF, 0x100);
	}
	// default IDAC
	/*
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x08, 0xFFFFF, 0x80200);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x09, 0xFFFFF, 0x80200);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x08, 0xFFFFF, 0x80200);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x09, 0xFFFFF, 0x80200);
	*/
	// set RFmode[19:16] and RXBB[9:5],0x00[1:0] remain original Value
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0xFFFF0, 0x403E);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0xFFFF0, 0x403E);
	//odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0xFFFF0, 0x403E);

	//Tx Gain
	// Gmode : 0x56[15:13]=MOD, 0x56[12:10]=PA, 0x56[8:5]=PAD, 0x56[4:0]=TxBB
	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x56, 0x0FFFF, 0xe0e4);
	} else if (path == RF_PATH_B) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x56, 0x0FFFF, 0xe0e4);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x56, 0x0FFFF, 0xe0e4);
	}
	// ----- END : Set RF Setting -----
	// TX_PI_DATA[19:0] is same as RF0x00
	tx_pi_data = odm_get_rf_reg(dm, path, RF_0x00, 0xFFFFF);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]TX_PI_DATA = 0x%x\n", tx_pi_data);
	odm_set_bb_reg(dm, R_0x1b20, 0x000FFFFF, tx_pi_data);
	odm_set_bb_reg(dm, R_0x1b20, 0x0F000000, 0x0); // disable DPD
	odm_set_bb_reg(dm, R_0x1bbc, 0x30000000, 0x0); // disable DPD
	odm_set_bb_reg(dm, R_0x1b1c, 0x0001C000, 0x0); // [16:14]=TX_P_Avg
	RF_DBG(dm, DBG_RF_IQK, "[IQK]1b20= 0x%x\n",
	       odm_get_bb_reg(dm, R_0x1b20, MASKDWORD));
	//odm_set_bb_reg(dm, 0x1b1c, 0x0001C000, 0x4); //Tx_P_avg
	odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 0x0); // r_tst_iqk2set = 0x0
}

void _iqk_lok_by_path_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i = 0, idac_ic = 0, idac_qc = 0, idac_if = 0, idac_qf = 0;
	u32 reg_1bfc;
	u8 KFAIL = 1;
	/*
	u8 idac_i[6] = { 0x08, 0x18, 0x10, 0x10, 0x18, 0x10};
	u8 idac_q[6] = { 0x10, 0x10, 0x08, 0x18, 0x18, 0x10};
	s32 psd_a[6], psd_b[6];
	s32 psd_pwr[6];
	s32 a[5], y0[2], n = 0x10, p = 0xA0;
	u64 b[5], s[3], x[3], y[2], y10 = 10;
	u16 idaci = 0, idacq = 0, m[2];
	u8 nega[5], negs[3], negx[3], negy[2];
	*/

	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d LOK!\n", path);
	// ============START : FLoK_coarse ========
	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xf5, BIT(17), 0x1);  // clock gating
	} else {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xf5, BIT(17), 0x1);  // clock gating
		odm_set_rf_reg(dm, RF_PATH_B, RF_0xf5, BIT(17), 0x1);  // clock gating
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF_0xf5[17] = 0x%x\n", odm_get_rf_reg(dm, path, RF_0xf5, BIT(17)));
	
	odm_set_bb_reg(dm, R_0x1b10, 0x000000FF, 0x00); //0x8010[7:0]-->0x2d9c[7:0]  @ 23F
	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1b00, 0xFFFF0000, 0x3c00); //rfc_base_address[15:0]
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S0 RF 0x0 = 0x%x!\n", odm_get_rf_reg(dm, path, RF_0x0, 0xFFFFF));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]1b0c[11:10] = 0x%x!\n", odm_get_bb_reg(dm, R_0x1b0c, 0x00000C00));
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x1); //wire r_iqk_IO_RFC_en 
		// ====START : NCTL=====
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x9); // ItQt
		odm_set_bb_reg(dm, R_0x1b2c, MASKDWORD, 0x00240024); // Tx_tone_idx=0x009 (2.25MHz)
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x018); //set cal_path, process id
	} else {
		odm_set_bb_reg(dm, R_0x1b00, 0xFFFF0000, 0x4c00); //rfc_base_address[15:0]
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S1 RF 0x0 = 0x%x!\n", odm_get_rf_reg(dm, path, RF_0x0, 0xFFFFF));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]1b0c[11:10] = 0x%x!\n", odm_get_bb_reg(dm, R_0x1b0c, 0x00000C00));
		odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x1); //wire r_iqk_IO_RFC_en 
		// ====START : NCTL=====
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x9); // ItQt
		//odm_set_bb_reg(dm, R_0x1b2c, MASKDWORD, 0x00240024); // Tx_tone_idx=0x009 (2.25MHz)
		odm_set_bb_reg(dm, R_0x1b2c, 0x00000FFF, 0x024);
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x028); //set cal_path, process id
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]1b00[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x1b00, MASKDWORD));
	if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x0) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXLOK NCTL(one shot) corse!\n");
		odm_set_bb_reg(dm, R_0x1b00, BIT(0), 0x1);//one shot
		while (i < 10) {
			i++;
			ODM_delay_ms(1);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x55) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL FLOK corse done, delaytime = %d ms!\n", i);
				break;
			} else {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
				continue;
			}
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
	}
	odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0); //wire r_iqk_IO_RFC_en
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002c0001); // Tx_tone_idx=0x009 (2.25MHz)
	//ODM_delay_ms(1);

	reg_1bfc = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]BBreg_1bfc= 0x%x\n", reg_1bfc);
	//IDACI = 0x1bfc[29:25], IDACQ = 0x1bfc[9:5]
	idac_ic = (u8)((reg_1bfc >> 25) & 0x1F);
	idac_qc = (u8)((reg_1bfc >> 5) & 0x1F);
	if (odm_get_bb_reg(dm, 0x1bfc, BIT(24)) == 1)
		idac_ic += 1;
	if (odm_get_bb_reg(dm, 0x1bfc, BIT(4)) == 1)
		idac_qc += 1;

	odm_set_rf_reg(dm, path, RF_0x08, 0xF8000, idac_ic);
	odm_set_rf_reg(dm, path, RF_0x08, 0x003E0, idac_qc);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_ic= 0x%x\n", idac_ic);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_qc= 0x%x\n", idac_qc);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF s%d 08 = 0x%x\n", path,
	       odm_get_rf_reg(dm, path, RF_0x08, 0xFFFFF));
#if 0
	odm_set_bb_reg(dm, R_0x1b18, 0x00000FFF, 0x01);
	odm_set_bb_reg(dm, R_0x1b14, BIT(30), 0x1);
	odm_set_bb_reg(dm, R_0x1b14, BIT(8), 0x0);

	for (i = 0; i < 6; i++)	 {
		odm_set_rf_reg(dm, path, RF_0x08, 0xF8000, idac_i[i]);
		odm_set_rf_reg(dm, path, RF_0x08, 0x003E0, idac_q[i]);
		odm_set_rf_reg(dm, path, RF_0x09, 0xFFFFF, 0x80200);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x09);
		// one shot psd
		odm_set_bb_reg(dm, R_0x1b34, BIT(0), 0x1);
		odm_set_bb_reg(dm, R_0x1b34, BIT(0), 0x0);
		ODM_delay_ms(2);
		// PSD PWR
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00210001);
		psd_a[i] = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00220001);
		psd_b[i] = odm_get_bb_reg(dm, 0x1bfc,  MASKDWORD);

		if ((psd_a[i] & 0x00FF0000) == 0x0) {
			psd_pwr[i] = psd_b[i];
		} else {
			psd_pwr[i] = ((psd_a[i] & 0x00FF0000) << 16) + psd_b[i];
			//psd_pwr[i] = psd_b[i];
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK] LOK psd_pwr[%d]= 0x%x\n", i, psd_pwr[i]);
	}

	a[0] = psd_pwr[0] + psd_pwr[1] - 2 * psd_pwr[5];
	a[1] = psd_pwr[2] + psd_pwr[3] - 2 * psd_pwr[5];
	a[2] = 2 * (psd_pwr[4] + psd_pwr[5] - psd_pwr[1] - psd_pwr[3]);
	a[3] = 8 * (psd_pwr[1] - psd_pwr[0]);
	a[4] = 8 * (psd_pwr[3] - psd_pwr[2]);

	for (i = 0; i < 5; i++) {  //change s32 a[i] to u32 b[i]+nega[i]
		if(a[i] < 0) {
			nega[i] = 1;
			b[i] = (u64)(~a[i] + 1);
		} else {
			nega[i] = 0;
			b[i] = (u64)a[i];
		}
	}

	negs[0] = (nega[1] + nega[3] + nega[2]) % 2; //s[0]
	negs[1] = (nega[0] + nega[4] + nega[2]) % 2; //s[1]
	negs[2] = (nega[0] + nega[1] + nega[2]) % 2; //s[2]

	s[0]= phydm_division64(b[1] * b[3], b[2]);
	s[1]= phydm_division64(b[0] * b[4], b[2]);
	s[2]= phydm_division64(b[0] * b[1], b[2]);

	//x1
	if((nega[4] + negs[0]) % 2 == 0 ) {  //same sign
		if(b[4] >= 2 * s[0]) { 
			x[0] = b[4] - 2 * s[0];
			negx[0] = nega[4];
		} else {
			x[0] = 2 * s[0] - b[4];
			negx[0] = 1 - nega[4];		
		}
	} else {  //opposite sign
		x[0] = b[4] + 2 * s[0];
		negx[0] = nega[4];		
	}
	//x2
	if((nega[3] + negs[1]) % 2 == 0 ) {  //same sign
		if(b[3] >= 2 * s[1]) { 
			x[1] = b[3] - 2 * s[1];
			negx[1] = nega[3];
		} else {
			x[1] = 2 * s[1] - b[3];
			negx[1] = 1 - nega[3];		
		}
	} else {  //opposite sign
		x[1] = b[3] + 2 * s[1];
		negx[1] = nega[3];		
	}
	//x3
	if((nega[2] + negs[2]) % 2 == 0 ) {  //same sign
		if(4 * s[2] >= b[2]) { 
			x[2] = 4 * s[2] - b[2];
			negx[2] = negs[2];
		} else {
			x[2] = b[2] - 4 * s[2];
			negx[2] = 1 - negs[2];		
		}
	} else {  //opposite sign
		x[2] = b[4] + 4 * s[2];
		negx[2] = negs[2];		
	}
#if 0

	for (i = 0; i < 2; i++) { //floor
		y[i] = phydm_division64(x[i], x[2]);
		negy[i] = (negx[i] + negx[2]) % 2;
		if(negy[i] == 0)
			y0[i] = n + (u16)y[i];
		else
			y0[i] = n - (u16)y[i];
	}
#endif
#if 1
	for (i = 0; i < 2; i++) {
		x[i] = (u64)(x[i] * 10);
		y[i] = phydm_division64(x[i], x[2]);
		negy[i] = (negx[i] + negx[2]) % 2;
		m[i] = (u16)(y[i] & 0xffff) % 10; 

		if(negy[i] == 0) {
			y0[i] = n + (s32)phydm_division64(y[i], y10);
			if(m[i] >= 5)
				y0[i] = y0[i] + 1;
		} else {
			y0[i] = n - (s32)phydm_division64(y[i], y10);
			if(m[i] >= 5)
				y0[i] = y0[i] - 1;
		}
	}
#endif
	if (y0[0] < 0)
		idaci = 0x0;
	else if (y0[0] >= 0x20)
		idaci = 0x1F;
	else
		idaci = (u16)y0[0];

	if (y0[1] < 0)
		idacq = 0x0;
	else if (y0[1] >= 0x20)
		idacq = 0x1F;
	else
		idacq = (u16)y0[1];

	for (i = 0; i < 5; i++) 
		RF_DBG(dm, DBG_RF_IQK, "[IQK] a[%d]= 0x%x\n", i, a[i]);
	for (i = 0; i < 5; i++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK] nega[%d]= %d\n", i, nega[i]);
	for (i = 0; i < 5; i++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK] b[%d]= 0x%x\n", i, b[i]);

	for (i = 0; i < 3; i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK] s[%d]= 0x%x\n", i, s[i]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] negs[%d]= %d\n", i, negs[i]);
	}
	for (i = 0; i < 3; i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK] x[%d]= 0x%x\n", i, x[i]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] negx[%d]= %d\n", i, negx[i]);
	}
	for (i = 0; i < 2; i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK] y[%d]= 0x%x\n", i, y[i]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] negy[%d]= %d\n", i, negy[i]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] y0[%d]= 0x%x\n", i, y0[i]);
	}
	odm_set_rf_reg(dm, path, RF_0x08, 0xF8000, idaci);
	odm_set_rf_reg(dm, path, RF_0x08, 0x003E0, idacq);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]idaci= 0x%x\n", idaci);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]idacq= 0x%x\n", idacq);
#endif
	//============END : FLoK_coarse ========
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF_0xf5[17] = 0x%x\n", odm_get_rf_reg(dm, path, RF_0xf5, BIT(17)));

	//============START : FLoK_Fine ========
	odm_set_bb_reg(dm, R_0x1b10, 0x000000FF, 0x00); //0x8010[7:0]-->0x2d9c[7:0]  @ 23F
	odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x1); //wire r_iqk_IO_RFC_en
	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d RF 0x0 = 0x%x!\n", path, odm_get_rf_reg(dm, path, RF_0x0, 0xFFFFF));
	RF_DBG(dm, DBG_RF_IQK, "[IQK]1b0c[11:10] = 0x%x!\n", odm_get_bb_reg(dm, R_0x1b0c, 0x00000C00));
	// ====START : NCTL=====
	odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x9); // ItQt
	odm_set_bb_reg(dm, R_0x1b2c, MASKDWORD, 0x00240024); // Tx_tone_idx=0x009 (2.25MHz)

	if (path == RF_PATH_A) 
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x118); //set cal_path, process id
	else 
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x128); //set cal_path, process id

	i = 0;

	if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x0) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXLOK NCTL(one shot) fine!\n");
		odm_set_bb_reg(dm, 0x1b00, BIT(0), 0x1);//one shot
		while (i < 10) {
			i++;
			ODM_delay_ms(1);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x55) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL FLOK fine done, delaytime = %d ms!\n", i);
				break;
			} else {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
				continue;
			}
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
	}

	odm_set_bb_reg(dm, R_0x1880, BIT(21), 0x0); //wire r_iqk_IO_RFC_en
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002c0001); // Tx_tone_idx=0x009 (2.25MHz)
	ODM_delay_ms(5);
	reg_1bfc = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]BBreg_1bfc= 0x%x\n", reg_1bfc);
	
	//IDACI = 0x1bfc[29:26], IDACQ = 0x1bfc[9:6]
	idac_if = (u8)((reg_1bfc >> 26) & 0xF);
	idac_qf = (u8)((reg_1bfc >> 6) & 0xF);
	if (odm_get_bb_reg(dm, 0x1bfc, BIT(25)) == 1)
		idac_if += 1;
	if (odm_get_bb_reg(dm, 0x1bfc, BIT(5)) == 1)
		idac_qf += 1;
	odm_set_rf_reg(dm, path, RF_0x09, 0xF0000, idac_if);
	odm_set_rf_reg(dm, path, RF_0x09, 0x003C0, idac_qf);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF after fine LOK s%d 08 = 0x%x\n", path,
	       odm_get_rf_reg(dm, path, RF_0x08, 0xFFFFF));

	odm_set_rf_reg(dm, path, RF_0x08, 0xF8000, idac_ic);
	odm_set_rf_reg(dm, path, RF_0x08, 0x003E0, idac_qc);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_ic= 0x%x\n", idac_ic);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_qc= 0x%x\n", idac_qc);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF fill s%d 08 = 0x%x\n", path,
	       odm_get_rf_reg(dm, path, RF_0x08, 0xFFFFF));

	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_if= 0x%x\n", idac_if);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]idac_qf= 0x%x\n", idac_qf);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF s%d 09 = 0x%x\n", path,
	       odm_get_rf_reg(dm, path, RF_0x09, 0xFFFFF));
	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xf5, BIT(17), 0x0);  // clock gating
	} else {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xf5, BIT(17), 0x0);  // clock gating
		odm_set_rf_reg(dm, RF_PATH_B, RF_0xf5, BIT(17), 0x0);  // clock gating
	}
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RF_0xf5[17] = 0x%x\n", odm_get_rf_reg(dm, path, RF_0xf5, BIT(17)));
}

void _iqk_txk_by_path_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 KFAIL = 1;
	u8 i = 0x0;
	u32 reg_1bfc;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d TXIQK!\n", path);
	// ====== START : NB TXIQK =====
	// ====START : NCTL=====
	// NCTL done for driver
	odm_set_bb_reg(dm, R_0x1b10, 0x000000FF, 0x00);//0x2D9C [7:0]=0x0
	//odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x1);   // r_tst_iqk2set = 0x1
	if (path == RF_PATH_A) {
		//[11:10]:TX_Y_Step_Size, [13:12]: TX_X_Step_Size_Step_Size
		//odm_set_bb_reg(dm, 0x1b18, 0x00003C00, 0xa);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003F, 0x09);
		// Tx_tone_idx=0x024 (2.25MHz)
		odm_set_bb_reg(dm, 0x1b2c, MASKDWORD, 0x00240024);
		//set cal_path, process id
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x218);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXIQK NCTL(one shot) pathA!\n");

	} else if (path == RF_PATH_B) {
		//odm_set_bb_reg(dm, 0x1b18, 0x00003C00, 0xf);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003F, 0x09);
		// Tx_tone_idx=0x024 (2.25MHz)
		odm_set_bb_reg(dm, 0x1b2c, MASKDWORD, 0x00240024);
		//set cal_path, process id
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, 0x228);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXIQK NCTL(one shot) pathB!\n");
	}

	if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x0) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXIQK NCTL(one shot)!\n");
		odm_set_bb_reg(dm, 0x1b00, BIT(0), 0x1);//one shot
		while (i < 10) {
			i++;
			ODM_delay_ms(1);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x55) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL FRxK done, delaytime = %d ms!\n", i * 1);
				break;
			} else {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
				continue;
			}
			RF_DBG(dm, DBG_RF_IQK, "[IQK]2d9c[7:0] = 0x%x!\n", odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF));
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]END : NB TXIQK NCTL!\n");

		KFAIL = (u8)odm_get_bb_reg(dm, 0x1b08, BIT(26));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXIQK 1b08[26]= 0x%x!\n", odm_get_bb_reg(dm, 0x1b08, BIT(26)));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]TXIQK PathA %s!\n", (KFAIL == 0 ? "success" : "fail"));

		//odm_set_bb_reg(dm, 0x1bd4, 0x007F0000, 0x15);
		//odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002c0001); 
		//reg_1bfc = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]BBreg_1bfc= 0x%x\n", reg_1bfc);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]BBreg_1b38= 0x%x\n", odm_get_bb_reg(dm, 0x1b38, MASKDWORD));
		if (!KFAIL) {
			//odm_set_bb_reg(dm, 0x1b38, 0x7FF00000, ((reg_1bfc & 0x07FF0000) >> 16));
			//odm_set_bb_reg(dm, 0x1b38, 0x0007FF00, (reg_1bfc & 0x000007FF));
			//iqk_info->txxy[path][0] = (reg_1bfc & 0x07FF0000) >> 16;
			//iqk_info->txxy[path][1] = reg_1bfc & 0x000007FF;
			iqk_info->txxy[path][0] = odm_get_bb_reg(dm, R_0x1b38, 0x7FF00000);
			iqk_info->txxy[path][1] = odm_get_bb_reg(dm, R_0x1b38, 0x0007FF00);
			iqk_info->iqk_fail_report[0][path][0] = true;
		} else {
			iqk_info->iqk_fail_report[0][path][0] = false; //[TX][path][tx]
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]txxy[%d][0]= 0x%x\n", path, iqk_info->txxy[path][0]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]txxy[%d][1]= 0x%x\n", path, iqk_info->txxy[path][1]);
		
	} else {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Jump : NB TXIQK NCTL not ready!\n");
	}
}

void _iqk_rxk_by_path_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 KFAIL = 1;
	u8 i = 0;
	u32 rx_pi_data, reg_1b3c;
	u32 rf_reg1F;
	u16 reg1b00 = 0x418;

	if (path == RF_PATH_A) {
		reg1b00 = 0x418;
	} else if (path == RF_PATH_B) {
		reg1b00 = 0x428;
	}
	//++++++ lna small ++++++++++++++++++++
	if (iqk_info->iqk_band == 0) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0x03FF0, 0x1cc);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0x03FF0, 0x1cc);

		odm_set_rf_reg(dm, path, RF_0x83, 0x00300, 0x2);
		odm_set_rf_reg(dm, path, RF_0x83, 0x1FC00, 0x79);
	} else {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0x03FF0, 0x1c8);

		odm_set_rf_reg(dm, path, RF_0x8c, 0x00180, 0x1);
		odm_set_rf_reg(dm, path, RF_0x8c, 0x0007F, 0x07);
	}
	// [19:0]=RX_PI_DATA
	// [19:16]=RF_Mode
	// [13:11]=LNA, [10]=TIA, [9:5]=RxBB
	// RX_PI_DATA[19:0] is same as RF0x00
	rx_pi_data = odm_get_rf_reg(dm, path, RF_0x00, 0xFFFFF);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RX_PI_DATA = 0x%x\n", rx_pi_data);
	odm_set_bb_reg(dm, R_0x1b24, 0x000FFFFF, rx_pi_data);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]1b24= 0x%x\n",
	odm_get_bb_reg(dm, 0x1b24, MASKDWORD));
	// ====START : NCTL=====
	odm_set_bb_reg(dm, 0x1b10, 0x000000FF, 0x00);
	//lna=0x3
	odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x07);
	//r_tst_iqk2set = 0x1
	odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x1);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003F, 0x3f);
	// Rx_tone_idx=0x044 (4.25MHz)
	odm_set_bb_reg(dm, 0x1b2c, 0x0FFF0000, 0x044);

	if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x0) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB TXIQK NCTL(one shot)!\n");
		//set cal_path, process id
		odm_set_bb_reg(dm, R_0x1b00, 0x00001FFF, reg1b00);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]reg 1b00 = 0x%x!\n",
		       odm_get_bb_reg(dm, R_0x1b00, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1b00, BIT(0), 0x1);//one shot
		while (i < 10) {
			i++;
			ODM_delay_ms(2);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x55) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL FRxK done, delaytime = %d ms!\n", i * 2);
			break;
			}
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]END : NB RXIQK NCTL!\n");
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXIQK 1b08[26]= 0x%x!\n", odm_get_bb_reg(dm, 0x1b08, BIT(26)));
		KFAIL = (u8)odm_get_bb_reg(dm, 0x1b08, BIT(26));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXIQK small LNA %s!\n", (KFAIL == 0 ? "success" : "fail"));
		//1b3c auto write!
		if (!KFAIL) {
			reg_1b3c = odm_get_bb_reg(dm, 0x1b3c, MASKDWORD);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]small LNA RXIQK, BBreg_1b3c= 0x%x\n", reg_1b3c);
			iqk_info->rxxy[path][0][0] = (reg_1b3c & 0x7FF00000) >> 20;  //rx_x
			iqk_info->rxxy[path][1][0] = (reg_1b3c & 0x0007FF00) >> 8 ;  //rx_y
			iqk_info->iqk_fail_report[1][path][0] = true;
		} else {
			iqk_info->iqk_fail_report[1][path][0] = false; //[RX][path][rxs]
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]rxxy[%d][0][0]= 0x%x\n", path, iqk_info->rxxy[path][0][0]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]rxxy[%d][1][0]= 0x%x\n", path, iqk_info->rxxy[path][1][0]);
	} else {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Jump : NB RXIQK NCTL not ready!\n");
	}
		//++++++ lna large ++++++++++++++++++++
	if (iqk_info->iqk_band == 0) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0x03FF0, 0x342);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0x03FF0, 0x342);

		odm_set_rf_reg(dm, path, RF_0x83, 0x00300, 0x2);
		odm_set_rf_reg(dm, path, RF_0x83, 0x1FC00, 0x7e);
	} else {

		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0x03FF0, 0x348);

		odm_set_rf_reg(dm, path, RF_0x8c, 0x00180, 0x1);
		odm_set_rf_reg(dm, path, RF_0x8c, 0x0007F, 0x07);
	}
	// ====START : NCTL=====
	odm_set_bb_reg(dm, 0x1b10, 0x000000FF, 0x00);
	odm_set_bb_reg(dm, 0x1b34, 0x0000007C, 0x0D); // lna=0x6
	odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x1); // r_tst_iqk2set = 0x1
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003F, 0x3f);
	odm_set_bb_reg(dm, 0x1b2c, 0x0FFF0000, 0x044);// Rx_tone_idx=0x044 (4.25MHz)
	if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x0) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]START : NB RXIQK NCTL(one shot)!\n");
		odm_set_bb_reg(dm, 0x1b00, 0x00001FFF, reg1b00);//set cal_path, process id
		odm_set_bb_reg(dm, 0x1b00, BIT(0), 0x1);//one shot
		while (i < 10) {
			i++;
			ODM_delay_ms(2);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000FF) == 0x55) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL FRxK done, delaytime = %d ms!\n", i * 2);
				break;
			}
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]END : NB RXIQK NCTL!\n");
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXIQK 1b08[26]= 0x%x!\n", odm_get_bb_reg(dm, 0x1b08, BIT(26)));
		KFAIL = (u8)odm_get_bb_reg(dm, 0x1b08, BIT(26));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]RXIQK large LNA %s!\n", (KFAIL == 0 ? "success" : "fail"));
		//1b3c auto write!
		if (!KFAIL) {
			reg_1b3c = odm_get_bb_reg(dm, 0x1b3c, MASKDWORD);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]large LNA RXIQK,BBreg_1b3c= 0x%x\n", reg_1b3c);
			iqk_info->rxxy[path][0][1] = (reg_1b3c & 0x7FF00000) >> 20;  //rx_x
			iqk_info->rxxy[path][1][1] = (reg_1b3c & 0x0007FF00) >> 8 ;  //rx_y
			iqk_info->iqk_fail_report[1][path][1] = true; //[RX][path][rxl]
		} else {
			iqk_info->iqk_fail_report[1][path][1] = false; //[RX][path][rxl]
		}
		RF_DBG(dm, DBG_RF_IQK, "[IQK]rxxy[%d][0][1]= 0x%x\n", path, iqk_info->rxxy[path][0][1]);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]rxxy[%d][1][1]= 0x%x\n", path, iqk_info->rxxy[path][1][1]);
	} else {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Jump : NB RXIQK NCTL not ready!\n");
	}
	// =====Disable RXIQKPLL =====
	if (iqk_info->iqk_band == 0) 
		odm_set_rf_reg(dm, path, RF_0x20, BIT(8), 0x0);
	else
		odm_set_rf_reg(dm, path, RF_0x20, BIT(7), 0x0);
		//1E[19]=POW_IQKPLL
	odm_set_rf_reg(dm, path, RF_0x1e, BIT(19), 0x0);
	
	odm_set_bb_reg(dm, R_0x1860, BIT(30), 0x1); // DAC on
}

void _iqk_iqk_by_step_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 counter = 0x0;

	RF_DBG(dm, DBG_RF_IQK, "iqk_info->iqk_step = %d\n", iqk_info->iqk_step);

	switch (iqk_info->iqk_step) {
	case 0: /*S0 LOK*/
		//counter = 0x0;
		_iqk_txk_rf_setting_8733b(dm, path);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]bypass LOK! not ready!\n");
		_iqk_lok_by_path_8733b(dm, path);
		iqk_info->iqk_step++;
		break;
	case 1:	/*S0 TXIQK*/
		_iqk_txk_by_path_8733b(dm, path);
		iqk_info->iqk_step++;
		break;
	case 2: /*S0 RXIQK*/
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d RXIQK!\n", path);
		_iqk_rxk_rf_setting_8733b(dm, path);
		_iqk_rxk_by_path_8733b(dm, path);
		iqk_info->iqk_step++;
		break;
	default:
		iqk_info->iqk_step = IQK_STEP_8733B;
		break;
	}

	return;
}

void _iqk_start_iqk_8733b(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i = 0;
	u32 patha_mode, pathb_mode;

	/*backup RF mode*/
	patha_mode = odm_get_rf_reg(dm, RF_PATH_A, 0x0, 0xF0000);
	pathb_mode = odm_get_rf_reg(dm, RF_PATH_B, 0x0, 0xF0000);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]backup pathA rf0[19:16] = 0x%x,pathB rf0[19:16] = 0x%x\n", patha_mode, pathb_mode);

	_iqk_preset_8733b(dm, true);

	btc_set_gnt_wl_bt_8733b(dm, true);

	while (i < 10) {
		_iqk_iqk_by_step_8733b(dm, path);
		if (iqk_info->iqk_step == IQK_STEP_8733B)
			break;
		i++;
	}

	btc_set_gnt_wl_bt_8733b(dm, false);

	_iqk_preset_8733b(dm, false);

	/* restore RF mode*/
	odm_set_rf_reg(dm, RF_PATH_A, 0x0, 0xF0000, patha_mode);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]restore pathA rf0[19:16] = 0x%x\n"
		, odm_get_rf_reg(dm, RF_PATH_A, 0x0, 0xF0000));
	odm_set_rf_reg(dm, RF_PATH_B, 0x0, 0xF0000, pathb_mode);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]restore pathB rf0[19:16] = 0x%x\n"
	, odm_get_rf_reg(dm, RF_PATH_B, 0x0, 0xF0000));
}

void _iq_calibrate_8733b_init(struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, j, k, m;
	static boolean firstrun = true;

	if (firstrun) {
		//odm_read_and_config_mp_8733b_cal_init(dm);
		//firstrun = false;
		RF_DBG(dm, DBG_RF_IQK, "[IQK]=====>PHY_IQCalibrate_8733B_Init\n");

		for (i = 0; i < RF_PATH_MAX_8733B; i++) {
			for (j = 0; j < 2; j++) {
				iqk_info->lok_fail[i] = true;
				iqk_info->iqk_fail[j][i] = true;
			
				//iqk_info->iqc_matrix[j][i] = 0x20000000;
			}
		iqk_info->txxy[i][0] = 0x200;
		iqk_info->txxy[i][1] = 0x000;
		iqk_info->rxxy[i][0][0] = 0x200;
		iqk_info->rxxy[i][1][0] = 0x000;
		iqk_info->rxxy[i][0][1] = 0x200;
		iqk_info->rxxy[i][1][1] = 0x000;
		}

		for (i = 0; i < 2; i++) { //i:band,j:path
			iqk_info->iqk_channel[i] = 0x0;

			for (j = 0; j < RF_PATH_MAX_8733B; j++) {
				iqk_info->lok_idac[i][j] = 0x0;
				iqk_info->rxiqk_agc[i][j] = 0x0;
				iqk_info->bypass_iqk[i][j] = 0x0;

				for (k = 0; k < 2; k++) {
					iqk_info->iqk_fail_report[i][j][k] = true;//i:TX/RX,j:path,k:Rxs/RXl
					}

				for (k = 0; k < 2; k++) {//i:band,j:path,k=tx/rx
					iqk_info->retry_count[i][j][k] = 0x0;
				}
			}
		}
	}

}

void _phy_iq_calibrate_8733b(struct dm_struct *dm,
			     u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 MAC_backup[MAC_REG_NUM_8733B], BB_backup[BB_REG_NUM_8733B], RF_backup[RF_REG_NUM_8733B][RF_PATH_MAX_8733B];
	u32 backup_mac_reg[MAC_REG_NUM_8733B] = {0x520};
	u32 backup_bb_reg[BB_REG_NUM_8733B] = {0x09f0, 0x09b4, 0x1c38, 0x1860, 0x1cd0, 0x824, 0x2a24, 0x1d40, 0x1c20, 0x1880, 0x180c}; //?not sure
	u32 backup_rf_reg[RF_REG_NUM_8733B] = {0x5, 0xde, 0xdf, 0xef, 0x1f};//0x0, 0x8f
	u32 temp0;
	boolean is_mp = false;
	boolean Kpass = true, Kpass_ab[2] = {true, true};
	u8 i = 0, j = 0, k = 0;
	u8 ab = 0;

	if (*dm->mp_mode)
		is_mp = true;
#if 0
	if (!is_mp)
		if (_iqk_reload_iqk_8733b(dm, reset))
			return;
#endif

	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========IQK strat!!!!!==========\n");
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]Interface = %d, Cv = %x\n", dm->support_interface, dm->cut_version);
	RF_DBG(dm, DBG_RF_IQK, "[IQK] Test V02\n");

	/*ab = (path << 1) | 0x1;
	odm_set_bb_reg(dm, 0x1b38, 0x3, ab);
	temp0 = odm_get_bb_reg(dm, 0x1b38, MASKDWORD);
	odm_set_bb_reg(dm, 0x1b38, BIT(0), 0x0);*/

	iqk_info->iqk_times++;
	iqk_info->kcount = 0;
	_iqk_information_8733b(dm);
	//_iqk_backup_iqk_8733b(dm, 0x0, 0x0);
	_iqk_backup_mac_bb_8733b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
	_iqk_backup_rf_8733b(dm, RF_backup, backup_rf_reg);

	while (i < 3) {
		i++;
		dm->n_iqk_cnt++;
		iqk_info->kcount = i;
		iqk_info->iqk_step = 0;
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Kcount = %d\n", iqk_info->kcount);
#if 1
		if (dm->rfe_type <= 2 || dm->rfe_type == 4 || dm->rfe_type == 9) {//only pathA or pathB
			_iqk_afe_setting_8733b(dm, true);
			_iqk_start_iqk_8733b(dm, path);
			_iqk_afe_setting_8733b(dm, false);
		} else {
			for (j = RF_PATH_A; j < RF_PATH_MAX_8733B; j++) {
				//path0 IQK
				odm_set_bb_reg(dm, 0x1884, BIT(20), j);
				_iqk_afe_setting_8733b(dm, true);
				_iqk_start_iqk_8733b(dm, j);
				_iqk_afe_setting_8733b(dm, false);
				//path1 IQK
				if (iqk_info->iqk_band == 1) {
					iqk_info->iqk_step = 3;
					break;
				} else {
					iqk_info->iqk_step = 0;
				}
			}
		}
#endif
		halrf_delay_10us(1);
		
		for(k = 0; k < 2; k++) { //[TXK/RXK][path][tx/rxs/rxl]
			Kpass_ab[k] = ((iqk_info->iqk_fail_report[0][k][0]) &
						   (iqk_info->iqk_fail_report[1][k][0]) &
						   (iqk_info->iqk_fail_report[1][k][1]));
		}
		Kpass = Kpass_ab[0] & Kpass_ab[1];
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Kpass = %s\n", (Kpass == true) ? "true" : "false");
		if (Kpass == true) {
			dm->n_iqk_ok_cnt++;
			//_iqk_fill_iqk_xy_8733b(dm);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]Kpass! break!\n");
			break;
		} else {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]fill old IQK value!\n");
			_iqk_fill_iqk_xy_8733b(dm, RF_PATH_A);
			_iqk_fill_iqk_xy_8733b(dm, RF_PATH_B);
			dm->n_iqk_fail_cnt++;
		}
	}

	_iqk_restore_rf_8733b(dm, backup_rf_reg, RF_backup);
	_iqk_restore_mac_bb_8733b(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);

	RF_DBG(dm, DBG_RF_IQK, "[IQK]K %s !\n", ((Kpass_ab[path] == true) ? "PASS" : "FAIL"));
	/*odm_set_bb_reg(dm, 0x1b38, BIT(0), 0x0); //[0]:dbg_WLS1_BB_SEL_mod
	odm_set_bb_reg(dm, 0x1b38, BIT(1), path);*/
	odm_set_bb_reg(dm, 0x1884, BIT(20), path);
	odm_set_bb_reg(dm, R_0x180c, BIT(31), 0x1);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========IQK end!!!!!==========\n");
}

void _check_fwiqk_done_8733b(struct dm_struct *dm)
{
	u32 counter = 0x0;
#if 1
	while (1) {
		if (odm_read_1byte(dm, 0x2d9c) == 0xaa  || counter > 300)
			break;
		counter++;
		halrf_delay_10us(100);
	};
	odm_write_1byte(dm, 0x1b10, 0x0);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]counter = %d\n", counter);
#else
	ODM_delay_ms(50);
	RF_DBG(dm, DBG_RF_IQK, "[IQK] delay 50ms\n");

#endif
}


/*IQK_version:0x1, NCTL:0x1*/
/*1.max tx pause while IQK*/
/*2.CCK off while IQK*/
void phy_iq_calibrate_8733b(void *dm_void,
	boolean is_recovery)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	u8 path;

	if (!(rf->rf_supportability & HAL_RF_IQK))
		return;
/*
	if (dm->mp_mode)
		if (*dm->mp_mode)
			halrf_iqk_hwtx_check(dm, true);

	if (!(*dm->mp_mode))
		_iqk_check_coex_status(dm, true);
*/

	dm->rf_calibrate_info.is_iqk_in_progress = true;
	/*FW IQK*/
#if 0
	if (dm->fw_offload_ability & PHYDM_RF_IQK_OFFLOAD) {
		_iqk_check_if_reload(dm);
		RF_DBG(dm, DBG_RF_IQK, "!!!!!  FW IQK   !!!!!\n");
	} else {
	}
#endif
	path = _iqk_switch_rf_path_8733b(dm);
	//odm_set_bb_reg(dm, R_0x180c, BIT(31), 0x0);

	_iq_calibrate_8733b_init(dm);

	_phy_iq_calibrate_8733b(dm, path);
	//odm_set_bb_reg(dm, R_0x180c, BIT(31), 0x1);

/*
	if (dm->mp_mode)
		if (*dm->mp_mode)
			halrf_iqk_hwtx_check(dm, false);
*/
	//halrf_iqk_dbg(dm);
	dm->rf_calibrate_info.is_iqk_in_progress = false;
}

#endif
