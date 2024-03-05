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

/*8733B GAPK ver:0x04 20210107*/

void _txgapk_backup_rf_registers_8733b(struct dm_struct *dm,
				       u32 *rf_reg,
				       u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < GAPK_RF_REG_NUM_8733B; i++) {
		rf_reg_backup[i][RF_PATH_A] = odm_get_rf_reg(dm, RF_PATH_A,
							     rf_reg[i],
							     RFREG_MASK);
		rf_reg_backup[i][RF_PATH_B] = odm_get_rf_reg(dm, RF_PATH_B,
							     rf_reg[i],
							     RFREG_MASK);
#if (GAPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Backup RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Backup RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
}

void _txgapk_reload_rf_registers_8733b(struct dm_struct *dm,
				       u32 *rf_reg,
				       u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < GAPK_RF_REG_NUM_8733B; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_B]);
#if (GAPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Reload RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Reload RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
	odm_set_rf_reg(dm, RF_PATH_A, 0x5c, BIT(19), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0x5c, BIT(19), 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0x5e, BIT(19), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0x5e, BIT(19), 0x0);
}

void _txgapk_backup_bb_registers_8733b(struct dm_struct *dm,
				       u32 *reg,
				       u32 *reg_backup)
{
	u32 i;

	for (i = 0; i < GAPK_BB_REG_NUM_8733B; i++) {
		reg_backup[i] = odm_get_bb_reg(dm, reg[i], MASKDWORD);
#if (GAPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
}

void _txgapk_reload_bb_registers_8733b(struct dm_struct *dm,
				       u32 *reg,
				       u32 *reg_backup)

{
	u32 i;

	for (i = 0; i < GAPK_BB_REG_NUM_8733B; i++) {
		odm_set_bb_reg(dm, reg[i], MASKDWORD, reg_backup[i]);
#if (GAPK_REG_DBG_8733B)
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Reload BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
}

void _txgapk_dbg_diff_calculate_8733b(struct dm_struct *dm)
{
	u8 i, diff[10];

	/*Read Dout Results*/
	odm_set_bb_reg(dm, R_0x1bd4, BIT(22), 0);
	/*46: report = gapk_rpt*/
	odm_write_1byte(dm, R_0x1bd6, 0x2e);

	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x3);

	for (i = 0; i < 5; i++) {
		diff[i] = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x3f << (i * 6));
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][REPORT] READ D[%d]=0x%x\n", i, diff[i]);
	}

	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x4);
	for (i = 0; i < 5; i++) {
		diff[i + 5] = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x3f << (i * 6));
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][REPORT] READ D[%d]=0x%x\n",
		       i + 5, diff[i + 5]);
	}
}

boolean _txgapk_dbg_psd_pwr_8733b(struct dm_struct *dm)
{
	u32 psd_pwr[2];
	u8 psd_pwr_h8[2];

	odm_set_bb_reg(dm, R_0x1bd4, BIT(22), 0);
	odm_write_1byte(dm, R_0x1bd6, 0x2e);

	/*psd_pwr0[38:32]*/
	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x2);
	psd_pwr_h8[0] = ((u8)odm_get_bb_reg(dm, 0x1bfc, 0x7f) & 0x7f);
	/*psd_pwr1[38:32]*/
	psd_pwr_h8[1] = ((u8)odm_get_bb_reg(dm, 0x1bfc, 0x007f0000) & 0x7f);

	/*psd_pwr0[31:0]*/
	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x0);
	psd_pwr[0] = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK][PSD] psd_pwr0[38:32]=0x%x, psd_pwr0[31:0]=0x%x\n",
	       psd_pwr_h8[0], psd_pwr[0]);

	/*psd_pwr1[31:0]*/
	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x1);
	psd_pwr[1] = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK][PSD] psd_pwr1[38:32]=0x%x, psd_pwr1[31:0]=0x%x\n",
	       psd_pwr_h8[1], psd_pwr[1]);

	if (psd_pwr[0] < 0x1000 && psd_pwr[1] < 0x1000) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK][PSD] PSD fail !!!\n");
		return false;
	}

	if ((psd_pwr[1] / psd_pwr[0] >= 2) || (psd_pwr[0] / psd_pwr[1] >= 2)) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK][PSD] 2point diff over 3db!!!\n");
		return false;
	}
	return true;
}

void _txgapk_psd_one_shot_8733b(struct dm_struct *dm, u8 point)
{
	btc_set_gnt_wl_bt_8733b(dm, true);

	/*PSD one shot*/
	odm_set_bb_reg(dm, R_0x1bf0, BIT(30), point); //psd_pwr_idx
	odm_set_bb_reg(dm, R_0x1b34, BIT(0), 1);
	odm_set_bb_reg(dm, R_0x1b34, BIT(0), 0);
	ODM_delay_us(10);

	btc_set_gnt_wl_bt_8733b(dm, false);
}

void _txgapk_enablek_one_shot_8733b(struct dm_struct *dm, u8 sel)
{
	/*d_clr;d_cal_en;ta_en*/
	u32 action[3] = {BIT(21), BIT(23), BIT(31)};

	btc_set_gnt_wl_bt_8733b(dm, true);

	odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 0x1);
	odm_set_bb_reg(dm, R_0x1bf0, action[sel], 0x1);
	odm_set_bb_reg(dm, R_0x1bf0, action[sel], 0x0);
	ODM_delay_us(10);
	odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 0x0);

	btc_set_gnt_wl_bt_8733b(dm, false);
#if 0
	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] sel = %d, action = 0x%x\n", sel, action[sel]);
#endif
}

void _txgapk_afe_setting_8733b(struct dm_struct *dm, boolean do_txgapk)
{
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);
	if (do_txgapk) {
		/*01 AFE 0N BB setting*/
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x0); /*r_path_en_en*/
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x1);
		odm_set_bb_reg(dm, R_0x824, 0x000f0000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0xf0000000, 0x7); /*IQK clk on*/
		/*Block CCA*/
		/*Prevent CCKCCA at sine PSD*/
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1); /*CCK CCA*/
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x1); /*OFDM CCA*/
		/*trx gating clk force on*/
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1);  /*go_through_iqk*/
		/*AFE ADDA both ON setting*/
		/*ADDA fifo force off*/
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0); /*force ADDA*/
		odm_set_bb_reg(dm, R_0x1860, 0xf0000000, 0xf); /*ADDA all on*/
		odm_set_bb_reg(dm, R_0x1860, 0x0ffff000, 0x0041);
		/*AD CLK rate:80M*/
		odm_set_bb_reg(dm, R_0x9f0, MASKLWORD, 0xbbbb);
		odm_set_bb_reg(dm, R_0x1d40, BIT(3), 0x1);
		odm_set_bb_reg(dm, R_0x1d40, 0x00000007, 0x3);
		/*DA CLK rate:160M*/
		odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x3);
		odm_set_bb_reg(dm, R_0x9b4, 0x00003800, 0x3);
		odm_set_bb_reg(dm, R_0x9b4, 0x0001C000, 0x3);
		odm_set_bb_reg(dm, R_0x9b4, 0x000E0000, 0x3);
		odm_set_bb_reg(dm, R_0x1c20, BIT(5), 0x1);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xFFFFFFFF);

		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]BB setting for AFE ON\n");
	} else {
		/*gapk restore*/
		odm_set_bb_reg(dm, R_0x1bd8, BIT(20), 0x0);
		odm_set_bb_reg(dm, R_0x1bd8, BIT(1) | BIT(0), 0x0);
		odm_set_bb_reg(dm, R_0x1bf0, BIT(1) | BIT(0), 0x0);
		/*AFE restore*/
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x9f0, MASKLWORD, 0xcccc);
		odm_set_bb_reg(dm, R_0x1d40, BIT(3), 0x0);
		odm_set_bb_reg(dm, R_0x1e24, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x0); /*CCK CCA*/
		odm_set_bb_reg(dm, R_0x1c68, BIT(24), 0x0); /*OFDM CCA*/
		odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x0);
		odm_set_bb_reg(dm, R_0x180c, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);

		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]Restore BB setting for AFE OFF\n");
	}
}

void _txgapk_rf_gain_setting_8733b(struct dm_struct *dm, u8 band, u8 path)
{
	u8 i;
	u8 txagc[2] = {0x1c, 0x16};

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);
	if (band == 0x0) { /*2G*/
		for (i = 0; i < (path + 1); i++) {
			odm_set_rf_reg(dm, i, RF_0x5, BIT(0), 0x0);
			odm_set_rf_reg(dm, i, RF_0x00, RFREG_MASK, 0x50000);
			odm_set_rf_reg(dm, i, RF_0x1, 0xff, txagc[path]);
			/*ATT Gain 000/001/011/111=-31/-37/-43/-49dB*/
			odm_set_rf_reg(dm, i, RF_0x83, 0x00007, 0x0);
			/*R1 Gain 0x0/1/3/7/f =-27/-21/-13/-9/-3.5dB*/
			odm_set_rf_reg(dm, i, RF_0x83, 0x000f0, 0x7);
			/*TIA gain -6db*/
			odm_set_rf_reg(dm, i, RF_0xdf, BIT(12), 0x1);
			odm_set_rf_reg(dm, i, RF_0x9e, BIT(8), 0x1);
			/*PGA gain 2db/step*/
			odm_set_rf_reg(dm, i, RF_0x8f, BIT(1), 0x0);
			odm_set_rf_reg(dm, i, RF_0x8f, 0x0e000, 0x7);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK] S%d RF_0x5=0x%x, 0x1=0x%x, 0x5c=0x%x, 0x5e=0x%x, 0x8f=0x%x\n",
			       i,
			       odm_get_rf_reg(dm, (enum rf_path)i, RF_0x5, RFREG_MASK),
			       odm_get_rf_reg(dm, (enum rf_path)i, RF_0x1, RFREG_MASK),
			       odm_get_rf_reg(dm, (enum rf_path)i, RF_0x5c, RFREG_MASK),
			       odm_get_rf_reg(dm, (enum rf_path)i, RF_0x5e, RFREG_MASK),
			       odm_get_rf_reg(dm, (enum rf_path)i, RF_0x8f, RFREG_MASK));
		}
	} else { /*5G*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5, BIT(0), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path,
			       RF_0x00, RFREG_MASK, 0x50000);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1, 0xff, 0x19);
		/*ATT Gain 000~111=-27.3db ~-36.7dB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8c, 0x0e000, 0x0);
		/*R1 Gain 00/01/10/11 = 5/8/20/20dB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8c, 0x01800, 0x0);
		/*TIA gain -6db*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf, BIT(12), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x9e, BIT(8), 0x1);
		/*PGA gain 2db/step*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8f, BIT(1), 0x0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8f, 0x0e000, 0x7);
	}
	/*EN_PAD_GAPK*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, BIT(19), 0x1);
	/*EN_PA_GAPK*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, BIT(19), 0x1);
	/*PA_GAPK_INDEX,PA=001,011,110,111 //0x0,1F,28,2E*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, 0x00);
	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] S%d RF_0x5=0x%x, 0x1=0x%x, 0x5c=0x%x, 0x5e=0x%x, 0x8f=0x%x\n",
	       path,
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x5, RFREG_MASK),
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1, RFREG_MASK),
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x5c, RFREG_MASK),
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x5e, RFREG_MASK),
	       odm_get_rf_reg(dm, (enum rf_path)path, RF_0x8f, RFREG_MASK));
}

void _txgapk_enable_gapk_8733b(struct dm_struct *dm, u8 path)
{
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path); //s0:0x0, s1:0x1
	odm_set_bb_reg(dm, R_0x1bd8, BIT(1) | BIT(0), 0x2);
	odm_set_bb_reg(dm, R_0x1b0c, BIT(11) | BIT(10), 0x2); //SIPI FLOK
	/*RX_PI_DATA*/
	odm_set_bb_reg(dm, R_0x1b24, RFREG_MASK,
		       odm_get_rf_reg(dm, path, RF_0x0, RFREG_MASK));
	odm_set_bb_reg(dm, R_0x1b1c, 0x0001C000, 0x4); //Tx_P_avg
	/*TRX IQC set default value*/
	odm_set_bb_reg(dm, R_0x1b38, 0xFFFFFF00, 0x200000);
	odm_set_bb_reg(dm, R_0x1b3c, 0xFFFFFF00, 0x200000);
	//RXK
	odm_set_bb_reg(dm, R_0x1b18, 0x70000000, 0x4);
	odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x00010100);
	odm_set_bb_reg(dm, R_0x1bcc, BIT(31), 0x0);
	/*Rx_tone_idx=0x024 (index*0.0625 = 2.25MHz)*/
	odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x024);

	odm_write_1byte(dm, R_0x1bf4, 0x5c);
	odm_set_bb_reg(dm, R_0x1bf0, BIT(1) | BIT(0), 0x1);//[0]=Psd_Gapk_en
	/*D_clr*/
	_txgapk_enablek_one_shot_8733b(dm, D_CLR_8733B);
}

void _txgapk_clear_gain_table_8733b(struct dm_struct *dm, u8 path)
{
	u8 i, idx;

	/*clear track table*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(15), 0x1);
	for (i = 0; i < 0xa; i++) {
		idx = 0x3 + i * 6;
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, 0x3f800, idx);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f, 0x0003f, 0x0);
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][Track] idx=0x%2x, ori 0x56=0x%x\n",
		       idx, odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));
	}
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(15), 0x0);

	/*clear power table*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(18), 0x1);
	for (i = 0; i < 0xa; i++) {
		idx = 0x1 + i * 3;
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, idx);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f, 0x0003f, 0x0);
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][Power] idx=0x%2x, ori 0x56=0x%x\n",
		       idx, odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));
	}
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(18), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, 0x00);
}

void _txgapk_write_gain_table_8733b(struct dm_struct *dm,
				    u8 path,
				    u8 table_sel)
{
	u8 idx, i, ta[10] = {0};

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	/*cal_Ta_en*/
	_txgapk_enablek_one_shot_8733b(dm, TAK_EN_8733B);

	_txgapk_dbg_diff_calculate_8733b(dm);

	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x8);
	for (i = 0; i < 5; i++) {
		ta[i] = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x3f << (i * 6));
	}
	odm_set_bb_reg(dm, R_0x1bf4, 0x00000F00, 0x9);
	for (i = 0; i < 5; i++) {
		ta[i + 5] = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x3f << (i * 6));
	}

	btc_set_gnt_wl_bt_8733b(dm, true);

	/*Write GapK Results*/
	if (table_sel == GAPK_TRACK_TABLE_8733B) {
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(15), 0x1);
		for (i = 0; i < 0xa; i++) {
			idx = 0x3 + i * 6;
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, 0x3f800, idx);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f, 0x0003f, ta[i]);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Track] Write TA[%d]=0x%2x, After 0x56=0x%x\n",
			       i, ta[i],
			       odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));
		}
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(15), 0x0);
	} else if (table_sel == GAPK_POWER_TABLE_8733B) {
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(18), 0x1);
		for (i = 0; i < 0xa; i++) {
			idx = 0x1 + i * 3;
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, idx);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f, 0x0003f, ta[i]);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Power] Write TA[%d]=0x%2x, After 0x56=0x%x\n",
			       i, ta[i],
			       odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));
		}
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xee, BIT(18), 0x0);
	}

	btc_set_gnt_wl_bt_8733b(dm, false);
}

void _txgapk_track_gap_search_8733b(struct dm_struct *dm, u8 path)
{
	u8 track_idx = 0, i, itqt = 0x1b;
	u32 g1, g2;

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ======>/****** PAD GAPK START ******/<======\n");

	for (i = 0; i < 0xa; i++) {
		track_idx = 0x3 + i * 6;
		if (i > 0x3)
			itqt = 0x2d;
		//odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 0);
		odm_set_rf_reg(dm, path, RF_0x5c, 0x3f800, track_idx);
		g1 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x56, 0x0ffe0);
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][Track] D%d: G1=0x%x, Before 0x56=0x%x\n",
		       i, g1, odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));

		odm_set_rf_reg(dm, path, RF_0x5c, 0x3f800, track_idx + 2);
		g2 = odm_get_rf_reg(dm, path, RF_0x56, 0x0ffe0);

		if (g1 != g2) {
			//odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, 0x3f800, track_idx);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, 0x0003f, 0x0);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Track] D%d: track_idx=0x%x, 0x56=0x%x\n",
			       i, odm_get_rf_reg(dm, path, RF_0x5c, 0x3f800),
			       odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));

			odm_set_bb_reg(dm, R_0x1bf0, 0x1f000000, i); //i=D_idx
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, itqt);
			/*point0 PSD one shot*/
			_txgapk_psd_one_shot_8733b(dm, 0);

			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, 0x3f800, track_idx + 2);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Track] D%d: track_idx+2=0x%x, 0x56=0x%x\n",
			       i, odm_get_rf_reg(dm, path, RF_0x5c, 0x3f800),
			       odm_get_rf_reg(dm, path, RF_0x56, 0xfffff));
			/*point1 PSD one shot*/
			_txgapk_psd_one_shot_8733b(dm, 1);

			if (_txgapk_dbg_psd_pwr_8733b(dm))
				_txgapk_enablek_one_shot_8733b(dm, DIFFK_EN_8733B);

		} else {
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Track] D%d: G1=G2, PAD no switch!!!\n",
			       i);
		}
	}
}

void _txgapk_power_gap_search_8733b(struct dm_struct *dm, u8 path)
{
	u8 power_idx, i, itqt = 0x1b;
	u32 g1, g2;

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ======>/****** PA GAPK START ******/<======\n");

	/*disable_PAD_GAPK*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5c, BIT(19), 0x0);
	ODM_delay_ms(1);
	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK][Power] 0x5c=0x%x, 0x5e=0x%x\n",
	       odm_get_rf_reg(dm, path, RF_0x5c, RFREG_MASK),
	       odm_get_rf_reg(dm, path, RF_0x5e, RFREG_MASK));

	odm_write_1byte(dm, R_0x1bf4, 0x5e);

	/*D_clr*/
	_txgapk_enablek_one_shot_8733b(dm, D_CLR_8733B);

	for (i = 0; i < 0xa; i++) {
		power_idx = 0x1 + i * 3;
		if (i > 0x3)
			itqt = 0x2d;
		//odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 0);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, power_idx);
		g1 = odm_get_rf_reg(dm, path, RF_0x56, 0x01c00);
		RF_DBG(dm, DBG_RF_TXGAPK,
		       "[TXGAPK][Power] D%d: G1=0x%x, Before 0x56=0x%x\n",
		       i, g1, odm_get_rf_reg(dm, path, RF_0x56, RFREG_MASK));
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, power_idx + 1);
		g2 = odm_get_rf_reg(dm, path, RF_0x56, 0x01c00);

		if (g1 != g2) {
			//odm_set_bb_reg(dm, R_0x1bb8, BIT(20), 1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, power_idx);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x0003f, 0x0);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Power] D%d: power_idx=0x%x, 0x56=0x%x\n",
			       i, odm_get_rf_reg(dm, path, RF_0x5e, 0x3f000),
			       odm_get_rf_reg(dm, path, RF_0x56, RFREG_MASK));

			odm_set_bb_reg(dm, R_0x1bf0, 0x1f000000, i); //i=D_idx
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, itqt);
			/*pwr0 PSD one shot*/
			_txgapk_psd_one_shot_8733b(dm, 0);

			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x5e, 0x3f000, power_idx + 1);
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Power] D%d: power_idx+1=0x%x, 0x56=0x%x\n",
			       i, odm_get_rf_reg(dm, path, RF_0x5e, 0x3f000),
			       odm_get_rf_reg(dm, path, RF_0x56, RFREG_MASK));
			/*pwr1 PSD one shot*/
			_txgapk_psd_one_shot_8733b(dm, 1);

			if (_txgapk_dbg_psd_pwr_8733b(dm))
				_txgapk_enablek_one_shot_8733b(dm, DIFFK_EN_8733B);

		} else {
			RF_DBG(dm, DBG_RF_TXGAPK,
			       "[TXGAPK][Power] D%d: G1=G2, PAD no switch!!!\n",
			       i);
		}
	}
}

void _txgapk_track_table_offset_8733b(struct dm_struct *dm, u8 path)
{
	u8 track_idx, i, ta[10];

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	_txgapk_track_gap_search_8733b(dm, path);

	/*Read & Write GapK Results*/
	_txgapk_write_gain_table_8733b(dm, path, GAPK_TRACK_TABLE_8733B);

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ======>/****** PAD GAPK END ******/<======\n");
}

void _txgapk_power_table_offset_8733b(struct dm_struct *dm, u8 path)
{
	u8 power_idx, i, ta[10];

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	_txgapk_power_gap_search_8733b(dm, path);

	/*Read & Write GapK Results*/
	_txgapk_write_gain_table_8733b(dm, path, GAPK_POWER_TABLE_8733B);

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ======>/****** PA GAPK END ******/<======\n");
}

void _txgapk_config_offset_table_8733b(struct dm_struct *dm)
{
	u32 reg_rf18;
	u8 band, ch, bw, current_path;

	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);
	current_path = (u8)odm_get_bb_reg(dm, R_0x1884, BIT(20));

	band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	ch = (u8)reg_rf18 & 0xff;
	bw = (u8)((reg_rf18 & BIT(10)) >> 10); /*1/0:20/40*/

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] Path/ Band/ CH/ BW = S%d / %s / %d / %s\n",
	       current_path,
	       band == 0 ? "2G" : "5G", ch,
	       bw == 1 ? "20M" : "40M");

	_txgapk_rf_gain_setting_8733b(dm, band, current_path);

	_txgapk_clear_gain_table_8733b(dm, current_path);

	_txgapk_enable_gapk_8733b(dm, current_path);

	_txgapk_track_table_offset_8733b(dm, current_path);
#if (GAPK_POWER_TABLE_8733B)
	_txgapk_power_table_offset_8733b(dm, current_path);
#endif
}

void halrf_txgapk_8733b(struct dm_struct *dm)
{
	u8 path;
	u32 bb_reg_backup[GAPK_BB_REG_NUM_8733B];
	u32 rf_reg_backup[GAPK_RF_REG_NUM_8733B][GAPK_RF_PATH_NUM_8733B];

	u32 bb_reg[GAPK_BB_REG_NUM_8733B] = {
		R_0x1b00, R_0x1b14, R_0x1b24, R_0x1b38, R_0x1b3c, R_0x1bcc};
	u32 rf_reg[GAPK_RF_REG_NUM_8733B] = {
		RF_0x00, RF_0x1, RF_0x83, RF_0x8c, RF_0x8f,
		RF_0x9e, RF_0xdf, RF_0x5};

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ************* TXGAPK Start *************\n");

	_txgapk_backup_bb_registers_8733b(dm, bb_reg, bb_reg_backup);
	_txgapk_backup_rf_registers_8733b(dm, rf_reg, rf_reg_backup);
	_txgapk_afe_setting_8733b(dm, true);
	_txgapk_config_offset_table_8733b(dm);
	_txgapk_afe_setting_8733b(dm, false);
	_txgapk_reload_rf_registers_8733b(dm, rf_reg, rf_reg_backup);
	_txgapk_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup);

	RF_DBG(dm, DBG_RF_TXGAPK,
	       "[TXGAPK] ************* TXGAPK END *************\n");
}

#endif
