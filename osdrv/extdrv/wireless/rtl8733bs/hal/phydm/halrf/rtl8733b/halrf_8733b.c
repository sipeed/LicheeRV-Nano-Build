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
void backup_bb_register_8733b(struct dm_struct *dm, u32 *bb_backup, u32 *backup_bb_reg, u32 counter)
{
	u32 i;

	for (i = 0; i < counter; i++)
		bb_backup[i] = odm_get_bb_reg(dm, backup_bb_reg[i], MASKDWORD);
}

void restore_bb_register_8733b(struct dm_struct *dm, u32 *bb_backup, u32 *backup_bb_reg, u32 counter)
{
	u32 i;

	for (i = 0; i < counter; i++)
		odm_set_bb_reg(dm, backup_bb_reg[i], MASKDWORD, bb_backup[i]);
}

void halrf_rf_lna_setting_8733b(struct dm_struct *dm_void,
				enum halrf_lna_set type)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path = 0x0;
#if 0
	for (path = 0x0; path < 2; path++)
		if (type == HALRF_LNA_DISABLE) {
			/*S0*/
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33,
				       RFREGOFFSETMASK, 0x00003);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3e,
				       RFREGOFFSETMASK, 0x00064);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f,
				       RFREGOFFSETMASK, 0x0afce);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x0);
		} else if (type == HALRF_LNA_ENABLE) {
			/*S0*/
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33,
				       RFREGOFFSETMASK, 0x00003);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3e,
				       RFREGOFFSETMASK, 0x00064);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f,
				       RFREGOFFSETMASK, 0x1afce);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x0);
		}
#endif
}

void odm_tx_pwr_track_set_pwr8733b(void *dm_void, enum pwrtrack_method method,
				   u8 rf_path, u8 channel_mapped_index)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u32 bitmask_6_0 = BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0);

	//[TBD]
	return;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "pRF->absolute_ofdm_swing_idx=%d   pRF->remnant_ofdm_swing_idx=%d   pRF->absolute_cck_swing_idx=%d   pRF->remnant_cck_swing_idx=%d   rf_path=%d\n",
	       cali_info->absolute_ofdm_swing_idx[rf_path],
	       cali_info->remnant_ofdm_swing_idx[rf_path],
	       cali_info->absolute_cck_swing_idx[rf_path],
	       cali_info->remnant_cck_swing_idx, rf_path);

	/*use for mp driver clean power tracking status*/
	if (method == CLEAN_MODE) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===> %s method=%d clear power tracking rf_path=%d\n",
		       __func__, method, rf_path);
		tssi->tssi_trk_txagc_offset[rf_path] = 0;

		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00002, 0x0);
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00100, 0x0);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0,
			       odm_get_bb_reg(dm, R_0x18a0, bitmask_6_0));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00002, 0x0);
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00100, 0x0);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0,
			       odm_get_bb_reg(dm, R_0x41a0, bitmask_6_0));
			break;
		default:
			break;
		}
	} else if (method == BBSWING) {
		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0,
			       odm_get_bb_reg(dm, R_0x18a0, bitmask_6_0));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0,
			       odm_get_bb_reg(dm, R_0x41a0, bitmask_6_0));
			break;
		default:
			break;
		}
	} else if (method == MIX_MODE) {
		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0,
			       odm_get_bb_reg(dm, R_0x18a0, bitmask_6_0));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, bitmask_6_0, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0x7f));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0,
			       odm_get_bb_reg(dm, R_0x41a0, bitmask_6_0));
			break;
		default:
			break;
		}
	}

}

void get_delta_swing_table_8733b(void *dm_void,
				 u8 **temperature_up_a,
				 u8 **temperature_down_a,
				 u8 **temperature_up_b,
				 u8 **temperature_down_b)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	u8 channel = *dm->channel;
	u8 tx_rate = phydm_get_tx_rate(dm);

	if (channel >= 1 && channel <= 14) {
		if (IS_CCK_RATE(tx_rate)) {
			*temperature_up_a = cali_info->delta_swing_table_idx_2g_cck_a_p;
			*temperature_down_a = cali_info->delta_swing_table_idx_2g_cck_a_n;
			*temperature_up_b = cali_info->delta_swing_table_idx_2g_cck_b_p;
			*temperature_down_b = cali_info->delta_swing_table_idx_2g_cck_b_n;
		} else {
			*temperature_up_a = cali_info->delta_swing_table_idx_2ga_p;
			*temperature_down_a = cali_info->delta_swing_table_idx_2ga_n;
			*temperature_up_b = cali_info->delta_swing_table_idx_2gb_p;
			*temperature_down_b = cali_info->delta_swing_table_idx_2gb_n;
		}
	}

	if (channel >= 36 && channel <= 64) {
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[0];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[0];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[0];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[0];
	} else if (channel >= 100 && channel <= 144) {
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[1];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[1];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[1];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[1];
	} else if (channel >= 149 && channel <= 177) {
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[2];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[2];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[2];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[2];
	}
}

void _phy_aac_calibrate_8733b(struct dm_struct *dm)
{
#if 0
	u32 cnt = 0;

	RF_DBG(dm, DBG_RF_LCK, "[AACK]AACK start!!!!!!!\n");
	//odm_set_rf_reg(dm, RF_PATH_A, 0xbb, RFREGOFFSETMASK, 0x80010);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb0, RFREGOFFSETMASK, 0x1F0FA);
	ODM_delay_ms(1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xca, RFREGOFFSETMASK, 0x80000);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xc9, RFREGOFFSETMASK, 0x80001);
	for (cnt = 0; cnt < 100; cnt++) {
		ODM_delay_ms(1);
		if (odm_get_rf_reg(dm, RF_PATH_A, RF_0xca, 0x1000) != 0x1)
			break;
	}

	odm_set_rf_reg(dm, RF_PATH_A, RF_0xb0, RFREGOFFSETMASK, 0x1F0F8);
	//odm_set_rf_reg(dm, RF_PATH_B, 0xbb, RFREGOFFSETMASK, 0x80010);

	RF_DBG(dm, DBG_RF_IQK, "[AACK]AACK end!!!!!!!\n");
#endif
}

void halrf_spur_compensation_2G_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_psd_data *psd = &rf->halrf_psd_data;
	u8 i, j, k, t = 0;
	u8 channel = *dm->channel, bandwidth = *dm->band_width, band = *dm->band_type;
	s32 ch_psd_bin[2][14] = { {0, 0, 0, 0, 0x80, 0x30, 0xFE0, 0xF90, 0, 0, 0, 0, 0x80, 0xFC0},
				  {0, 0, 0x120, 0xD0, 0x80, 0x30, 0xFE0, 0xF90, 0xF40, 0xEF0, 0x120, 0x0, 0x0, 0x0} };
	u32 psd_bin[3];
	s32 loc_diff[3] = {-1, 0, 1};
	u64 data[3], datamax[6], mindata, spur_level;

	RF_DBG(dm, DBG_RF_LCK, "[RF]spur compensation 2G!\n");

	if (ch_psd_bin[bandwidth][channel - 1] == 0) {
		RF_DBG(dm, DBG_RF_LCK, "[RF]Channel = %d,bandwidth = %d, spur return!\n", channel, bandwidth);
		return;
	}
	//rf 0?
	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x0, 0xf0000, 0x3);
	} else {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x0, 0xf0000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x0, 0xf0000, 0x3);

	}
	RF_DBG(dm, DBG_RF_LCK, "[RF]RF S%d reg0x0 = 0x%x\n", path, odm_get_rf_reg(dm, path, RF_0x0, 0xfffff));
#if 0
	RF_DBG(dm, DBG_RF_LCK, "[RF]cut_version = %d!\n", dm->cut_version);
	if (dm->cut_version == 0 || dm->cut_version == 1) {
		//RFC path A 0xDD[0]=1
		odm_set_rf_reg(dm, RF_PATH_A, 0xdd, BIT(0), 0x1);
		if (path == RF_PATH_B)
			odm_set_rf_reg(dm, RF_PATH_B, 0xdd, BIT(0), 0x1);
	}
#endif
		odm_set_bb_reg(dm, R_0x1040, BIT(10), 0);
	for (i = 0; i < 6; i++) {
		odm_set_bb_reg(dm, R_0x1064, 0x7000, i);
		_halrf_iqk_psd_init_8733b(dm, true);
		//delay_ms(1);
		odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0xA21FFC00);
		for (k = 0; k < 3; k++) {
			psd_bin[k] = (u32)(ch_psd_bin[bandwidth][channel - 1] + loc_diff[k]);
			data[k] = halrf_get_iqk_psd_data(dm, psd_bin[k]);
			data[k] &= 0xFFFFFFFFFF;
		}
		datamax[i] = MAX_2(MAX_2(data[0], data[1]), data[2]);
		_halrf_iqk_psd_init_8733b(dm, false);
	}

	mindata = datamax[1];
	for (i = 1; i < 6; i++)
		mindata = MIN_2(mindata, datamax[i]);

	for (i = 1; i < 6; i++) {
		if (mindata == datamax[i]) {
			t = i;
			RF_DBG(dm, DBG_RF_LCK, "[RF]t= %x\n", t);
		}
	}
	odm_set_bb_reg(dm, R_0x1064, 0x7000, t);
#if 0
	RF_DBG(dm, DBG_RF_LCK, "[RF]ch_psd_bin[%d][%d] = 0x%x!\n", bandwidth, (channel - 1), ch_psd_bin[bandwidth][channel - 1]);
	for (k = 1; k < 6; k++)
		RF_DBG(dm, DBG_RF_LCK, "[RF]datamax[%d] = 0x%x\n", k, datamax[k]);
#endif
	//10*LOG10(15848932)=72db
	if (mindata >= 0xF1D5E4)
		halrf_spur_cancellation_2G_8733b(dm);
}

void halrf_spur_compensation_5G_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_psd_data *psd = &rf->halrf_psd_data;
	u8 i, j, k, t = 0;
	u8 channel = *dm->channel, bandwidth = *dm->band_width, band = *dm->band_type;
	u8 do_spur = 0;
	s32 ch_psd_bin[2] = {0xFB0, 0x50};//location at diff ch&bw
	s32 psd_bin[3];
	s32 loc_diff[3] = {-1, 0, 1};
	u64 data[3], datamax[5], mindata;
	u64 spur_level; //15848932

	RF_DBG(dm, DBG_RF_LCK, "[RF]spur compensation 5G!\n");
	if ((channel == 153 && bandwidth == 0) || (channel == 151 && bandwidth == 1))
		do_spur = 1;
	if (do_spur == 0) {
		RF_DBG(dm, DBG_RF_LCK, "[RF]Channel = %d,bandwidth = %d, return!\n", channel, bandwidth);
		return;
	}
#if 0
	//rf 0?
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x0, 0xf0000, 0x3);
	RF_DBG(dm, DBG_RF_LCK, "[RF]RF reg0x0 = 0x%x\n", odm_get_rf_reg(dm, RF_PATH_A, RF_0x0, 0xfffff));

	RF_DBG(dm, DBG_RF_LCK, "[RF]cut_version = %d!\n", dm->cut_version);
	if (dm->cut_version == 0 || dm->cut_version == 1) {
		//RFC path A 0xDD[0]=1
		odm_set_rf_reg(dm, RF_PATH_A, 0xdd, BIT(0), 0x1);
	}
	for (i = 1; i < 6; i++) {
		odm_set_bb_reg(dm, R_0x1040, BIT(10), 0);
		odm_set_bb_reg(dm, R_0x1064, 0x7000, i);
		_halrf_iqk_psd_init_8733b(dm, true);
		odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0xA21FFC00);
		for (k = 0; k < 3; k++) {
			psd_bin[i] = ch_psd_bin[bandwidth] + loc_diff[k];
			data[k] = halrf_get_iqk_psd_data(dm, psd_bin[i]);
			data[k] &= 0xFFFFFFFFFF;
			//RF_DBG(dm, DBG_RF_LCK, "[RF]data[%d] = 0x%x\n", k, data[k]);
		}
		datamax[i - 1] = MAX_2(MAX_2(data[0], data[1]), data[2]);
		_halrf_iqk_psd_init_8733b(dm, false);
	}
	mindata = datamax[0];
	for (i = 0; i < 5; i++)
		mindata = MIN_2(mindata, datamax[i]);

	for (i = 0; i < 5; i++) {
		if (mindata == datamax[i]) {
			t = i + 1;
			RF_DBG(dm, DBG_RF_LCK, "[RF]t= %x\n", t);
		}
	}
	odm_set_bb_reg(dm, R_0x1064, 0x7000, t);

	for (k = 0; k < 5; k++)
		RF_DBG(dm, DBG_RF_LCK, "[RF]datamax[%d] = 0x%x\n", k, datamax[k]);

	//10*LOG10(15848932)=72db
	if (mindata >= 0xF1D5E4) //do NBI/CSI
#endif
		halrf_spur_cancellation_5G_8733b(dm);
}

void halrf_spur_cancellation_2G_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 channel = *dm->channel, bandwidth = *dm->band_width;
	s32 tone_idx[2][14] = { {0, 0, 0, 0, 0x1A, 0xA, 0x7A, 0x6A, 0, 0, 0, 0, 0x1A, 0x73},
				{0, 0, 0x3A, 0x2A, 0x1A, 0xA, 0x7A, 0x6A, 0x5A, 0x4A, 0x3A, 0x0, 0x0, 0x0} };

	RF_DBG(dm, DBG_RF_LCK, "[RF]spur NBI/CSI start!!!!!!!\n");
	odm_set_bb_reg(dm, R_0x1944, 0x001FF000, tone_idx[bandwidth][channel - 1]);
	RF_DBG(dm, DBG_RF_LCK, "[RF][NBI]tone_idx(1944) = 0x%x\n", odm_get_bb_reg(dm, R_0x1944, 0x001FF000));
	odm_set_bb_reg(dm, R_0x818, BIT(11), 0);
	odm_set_bb_reg(dm, R_0x818, BIT(11), 1);
	odm_set_bb_reg(dm, R_0x1940, BIT(31), 0);
	odm_set_bb_reg(dm, R_0x1940, BIT(31), 1);
	odm_set_bb_reg(dm, R_0x1ce8, BIT(28), 0);
	odm_set_bb_reg(dm, R_0x1ce8, BIT(28), 1);

	odm_set_bb_reg(dm, R_0xdb4, 0xFE, tone_idx[bandwidth][channel - 1]);
	RF_DBG(dm, DBG_RF_LCK, "[RF][CSI]tone_idx(db4) = 0x%x\n", odm_get_bb_reg(dm, R_0xdb4, 0xFE));
	odm_set_bb_reg(dm, R_0xdb0, MASKDWORD, 0x33221100);
	odm_set_bb_reg(dm, R_0xdb4, BIT(0), 1);
}

void halrf_spur_cancellation_5G_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 channel = *dm->channel, bandwidth = *dm->band_width;
	s32 tone_idx[2] = {0x70, 0x10};//location at diff ch&bw

	RF_DBG(dm, DBG_RF_LCK, "[RF]spur NBI/CSI start!!!!!!!\n");
	odm_set_bb_reg(dm, R_0x1944, 0x001FF000, tone_idx[bandwidth]);
	RF_DBG(dm, DBG_RF_LCK, "[RF][NBI]tone_idx(1944) = 0x%x\n", odm_get_bb_reg(dm, R_0x1944, 0x001FF000));
	odm_set_bb_reg(dm, R_0x818, BIT(11), 0);
	odm_set_bb_reg(dm, R_0x818, BIT(11), 1);
	odm_set_bb_reg(dm, R_0x1940, BIT(31), 0);
	odm_set_bb_reg(dm, R_0x1940, BIT(31), 1);
	odm_set_bb_reg(dm, R_0x1ce8, BIT(28), 0);
	odm_set_bb_reg(dm, R_0x1ce8, BIT(28), 1);

	odm_set_bb_reg(dm, R_0xdb4, 0xFE, tone_idx[bandwidth]);
	RF_DBG(dm, DBG_RF_LCK, "[RF][CSI]tone_idx(db4) = 0x%x\n", odm_get_bb_reg(dm, R_0xdb4, 0xFE));
	odm_set_bb_reg(dm, R_0xdb0, MASKDWORD, 0x33221100);
	odm_set_bb_reg(dm, R_0xdb4, BIT(0), 1);
}

void halrf_spur_compensation_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 band = *dm->band_type, path;
	u32 reg_rf0A, reg_rf0B, reg_rf5;
	u32 bb_backup[10];
	u32 backup_bb_reg[10] = {0x09f0, 0x09b4, 0x1c38, 0x1860, 0x1cd0,
				 0x824, 0x2a24, 0x1d40, 0x1c20, 0x1b1c};

	RF_DBG(dm, DBG_RF_LCK, "[RF]spur compensation start!!!!!!!\n");
	path = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	reg_rf0A = odm_get_rf_reg(dm, RF_PATH_A, RF_0x0, 0xfffff);
	reg_rf0B = odm_get_rf_reg(dm, RF_PATH_B, RF_0x0, 0xfffff);
	RF_DBG(dm, DBG_RF_LCK, "[RF][spur]before rf_reg pathA 0x0 = 0x%x, pathB 0x0 = 0x%x\n",
		   reg_rf0A, reg_rf0B);
	reg_rf5 = odm_get_rf_reg(dm, path, RF_0x5, 0xfffff);
	//0x818[11]=0x1940[31]=0x1CE8[28]=0xDB4[0]=0,Turn off NBI/CSI
	RF_DBG(dm, DBG_RF_LCK, "[RF][spur]Turn off NBI/CSI!!!!!!!\n");
	odm_set_bb_reg(dm, R_0x818, BIT(11), 0);
	odm_set_bb_reg(dm, R_0x1940, BIT(31), 0);
	odm_set_bb_reg(dm, R_0x1ce8, BIT(28), 0);
	odm_set_bb_reg(dm, R_0xdb4, BIT(0), 0);
	backup_bb_register_8733b(dm, bb_backup, backup_bb_reg, 10);
	switch (band) {
	case 0:  //2G
		if (dm->rfe_type <= 2 || dm->rfe_type == 4 || dm->rfe_type == 9) {//only pathA or pathB
			odm_set_bb_reg(dm, 0x1884, BIT(20), path);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x5, BIT(0), 0x0);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x5, BIT(0), 0x0);
			halrf_spur_compensation_2G_8733b(dm, path);
		} else {
			odm_set_bb_reg(dm, 0x1884, BIT(20), 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x5, BIT(0), 0x0);
			halrf_spur_compensation_2G_8733b(dm, RF_PATH_A);
			odm_set_bb_reg(dm, 0x1884, BIT(20), 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x5, BIT(0), 0x0);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x5, BIT(0), 0x0);
			halrf_spur_compensation_2G_8733b(dm, RF_PATH_B);
		}
		break;
	case 1:  //5G
		halrf_spur_compensation_5G_8733b(dm);
		break;
	default:
		RF_DBG(dm, DBG_RF_LCK, "[RF]invalid band!!!!!!!\n");
		break;
	}
	restore_bb_register_8733b(dm, bb_backup, backup_bb_reg, 10);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x0, 0xfffff, reg_rf0A);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x0, 0xfffff, reg_rf0B);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x5, 0xfffff, reg_rf5);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x5, 0xfffff, reg_rf5);
	RF_DBG(dm, DBG_RF_LCK, "[RF][spur]rf_reg pathA 0x5 = 0x%x, pathB 0x5 = 0x%x\n",
		   odm_get_rf_reg(dm, RF_PATH_A, RF_0x5, 0xfffff),
		   odm_get_rf_reg(dm, RF_PATH_B, RF_0x5, 0xfffff));
	RF_DBG(dm, DBG_RF_LCK, "[RF][spur]after rf_reg pathA 0x0 = 0x%x, pathB 0x0 = 0x%x\n",
		   odm_get_rf_reg(dm, RF_PATH_A, RF_0x0, 0xfffff),
		   odm_get_rf_reg(dm, RF_PATH_B, RF_0x0, 0xfffff));
	odm_set_bb_reg(dm, 0x1884, BIT(20), path);
	RF_DBG(dm, DBG_RF_LCK, "[RF][spur]spur compensation end!!!!!!!\n");
}

#if 0
void halrf_polling_check(void *dm_void, u32 add, u32 bmask, u32 data)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 c = 0;

	c = 0;
	while (c < 100000) {
		c++;
		if (odm_get_bb_reg(dm, add, bmask) == data)
			break;
	}
}
#endif

void _phy_x2_calibrate_8733b(struct dm_struct *dm)
{
	u32 rf_18;

	RF_DBG(dm, DBG_RF_LCK, "[X2K]X2K start!!!!!!!\n");
	rf_18 = odm_get_rf_reg(dm, RF_PATH_A, 0x18, RFREG_MASK);
	/*X2K*/
	//Path A
	odm_set_rf_reg(dm, RF_PATH_A, 0x18, RFREG_MASK, 0x10508);
	ODM_delay_ms(1);
	odm_set_rf_reg(dm, RF_PATH_A, 0xed, RFREG_MASK, 0x00004);
	odm_set_rf_reg(dm, RF_PATH_A, 0x33, RFREG_MASK, 0x00000);
	odm_set_rf_reg(dm, RF_PATH_A, 0x3f, RFREG_MASK, 0x10808);
	odm_set_rf_reg(dm, RF_PATH_A, 0xed, RFREG_MASK, 0x00000);

	odm_set_rf_reg(dm, RF_PATH_A, 0xc8, RFREG_MASK, 0x01000);
	odm_set_rf_reg(dm, RF_PATH_A, 0xc8, RFREG_MASK, 0x01004);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb9, RFREG_MASK, 0x00000);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb9, RFREG_MASK, 0x80000);
	odm_set_rf_reg(dm, RF_PATH_A, 0xc8, RFREG_MASK, 0x00000);

	odm_set_rf_reg(dm, RF_PATH_A, 0xb9, RFREG_MASK, 0x00000);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb9, RFREG_MASK, 0x80000);

	ODM_delay_ms(1);
	odm_set_rf_reg(dm, RF_PATH_A, 0x18, RFREG_MASK, rf_18);
	//Path B
	// SYN is in the path A
	RF_DBG(dm, DBG_RF_LCK, "[X2K]X2K end!!!!!!!\n");
}

void phy_x2_check_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 X2K_BUSY;

	/*X2K*/
	_phy_x2_calibrate_8733b(dm);

	RF_DBG(dm, DBG_RF_LCK, "[X2K]X2K check start!!!!!!!\n");
	//Path A
	ODM_delay_ms(1);
	X2K_BUSY = (u8)odm_get_rf_reg(dm, RF_PATH_A, 0xb8, BIT(15));
	RF_DBG(dm, DBG_RF_LCK, "[X2K]X2K_BUSY = %d\n", X2K_BUSY);
	if (X2K_BUSY == 1) {
		odm_set_rf_reg(dm, RF_PATH_A, 0xb8, BIT(14), 0x1);
		odm_set_rf_reg(dm, RF_PATH_A, 0xb8, BIT(15), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, 0xb8, BIT(18), 0x0);
		ODM_delay_ms(1);
	}
	// SYN is in the path A
	RF_DBG(dm, DBG_RF_LCK, "[X2K]X2K check end!!!!!!!\n");

}

/*LCK VERSION:0x1*/
void phy_lc_calibrate_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
#if 0
	_phy_aac_calibrate_8733b(dm);
	_phy_rt_calibrate_8733b(dm);
#endif
}

void configure_txpower_track_8733b(struct txpwrtrack_cfg *config)
{
	config->swing_table_size_cck = TXSCALE_TABLE_SIZE;
	config->swing_table_size_ofdm = TXSCALE_TABLE_SIZE;
	config->threshold_iqk = IQK_THRESHOLD;
	config->threshold_dpk = DPK_THRESHOLD;
	config->average_thermal_num = AVG_THERMAL_NUM_8733B;
	config->rf_path_count = MAX_PATH_NUM_8733B;
	config->thermal_reg_addr = RF_T_METER_8733B;

	config->odm_tx_pwr_track_set_pwr = odm_tx_pwr_track_set_pwr8733b;
	config->do_iqk = do_iqk_8733b;
	config->phy_lc_calibrate = halrf_lck_trigger;
	//config->do_tssi_dck = halrf_tssi_dck;
	config->get_delta_swing_table = get_delta_swing_table_8733b;
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
void phy_set_rf_path_switch_8733b(struct dm_struct *dm, boolean is_main)
#else
void phy_set_rf_path_switch_8733b(void *adapter, boolean is_main)
#endif
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(((PADAPTER)adapter));
	struct dm_struct *dm = &hal_data->DM_OutSrc;
#endif
#endif
	if (is_main) {
		/*SW ctrl GNT_BT=0, SW ctrl GNT_WL=1*/
		odm_set_bb_reg(dm, 0x73, 0xF0, 0x9);
	} else {
		/*SW ctrl GNT_BT=1, SW ctrl GNT_WL=0*/
		odm_set_bb_reg(dm, 0x73, 0xF0, 0x7);
	}
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
boolean _phy_query_rf_path_switch_8733b(struct dm_struct *dm)
#else
boolean _phy_query_rf_path_switch_8733b(void *adapter)
#endif
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(((PADAPTER)adapter));
	struct dm_struct *dm = &hal_data->DM_OutSrc;
#endif
#endif
	if (odm_get_bb_reg(dm, R_0x1884, BIT(20)) == 0x0)
		return true;	/*S0*/
	else
		return false;
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
boolean phy_query_rf_path_switch_8733b(struct dm_struct *dm)
#else
boolean phy_query_rf_path_switch_8733b(void *adapter)
#endif
{
#if DISABLE_BB_RF
	return true;
#endif
#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
	return _phy_query_rf_path_switch_8733b(dm);
#else
	return _phy_query_rf_path_switch_8733b(adapter);
#endif
}

void halrf_rfk_handshake_8733b(void *dm_void, boolean is_before_k)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	void *padapter = dm->adapter;

	u8 u1b_tmp, h2c_parameter;
	u16 count;
	u8 path, rfk_type = rf->rfk_type;

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
	path = phy_query_rf_path_switch_8733b(dm);
#else
	path = phy_query_rf_path_switch_8733b(padapter);
#endif

	if (is_before_k) {
		RF_DBG(dm, DBG_RF_RFK,
		       "[RFK] WiFi / BT RFK handshake start!!\n");
		RF_DBG(dm, DBG_RF_RFK,
		       "[RFK] WiFi rfk_type = 0x%x\n", rfk_type);

		if (!rf->is_bt_iqk_timeout) {
			count = 0;
			u1b_tmp = (u8)odm_get_mac_reg(dm, 0xac, BIT(6) | BIT(5));
			while (u1b_tmp != 0 && count < 30000) {
				ODM_delay_us(20);
				u1b_tmp = (u8)odm_get_mac_reg(dm, 0xac, BIT(6) | BIT(5));
				count++;
			}

			if (count >= 30000) {
				RF_DBG(dm, DBG_RF_RFK,
				       "[RFK] Wait BT IQK finish timeout!!\n");
				rf->is_bt_iqk_timeout = true;
			}
		}

		/* Send RFK start H2C cmd*/
		h2c_parameter = 1;
		odm_fill_h2c_cmd(dm, ODM_H2C_WIFI_CALIBRATION, 1, &h2c_parameter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE) && (CONFIG_BTCOEX_SUPPORT_BTC_CMN == 1)
		hal_btcoex_WLRFKNotify(padapter, path, rfk_type, 0);
#endif
		ODM_delay_us(20);
	} else {
		/* Send RFK finish H2C cmd*/
		h2c_parameter = 0;
		odm_fill_h2c_cmd(dm, ODM_H2C_WIFI_CALIBRATION, 1, &h2c_parameter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE) && (CONFIG_BTCOEX_SUPPORT_BTC_CMN == 1)
		hal_btcoex_WLRFKNotify(padapter, path, rfk_type, 1);
#endif
		ODM_delay_us(20);

		RF_DBG(dm, DBG_RF_RFK,
		       "[RFK] WiFi / BT RFK handshake finish!!\n");
	}
}

void btc_set_gnt_wl_bt_8733b(void *dm_void, boolean is_before_k)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;

	if (is_before_k) {
		rf->gnt_control = odm_get_mac_reg(dm, R_0x70, MASKDWORD);

		/*force GNT control to WL*/
		/*0x73[2] = 0x1, 0x73[7:4] = 0x1001*/
		odm_set_mac_reg(dm, R_0x70, BIT(26), 0x1);
		/*force GNT_WL=1, GNT_BT=0*/
		odm_set_mac_reg(dm, R_0x70, 0xf0000000, 0x9);

	} else {
		odm_set_mac_reg(dm, R_0x70, MASKDWORD, rf->gnt_control);
	}
}

void halrf_rx_port_ctl_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (dm->mp_mode) {
		/*Fixed for mp RX test*/
		/*0x614[20:16]=>0x3//only turn on rx port 0/1*/
		if (*dm->mp_mode)
			odm_write_1byte(dm, 0x616, 0x3);
		else
			odm_write_1byte(dm, 0x616, 0x1f);
	}
}

void halrf_rfk_power_save_8733b(void *dm_void, boolean is_power_save)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (is_power_save) /*default*/
		odm_set_bb_reg(dm, 0x1c64, BIT(30), 0x1);
	else /*RFK disable pwrsave*/
		odm_set_bb_reg(dm, 0x1c64, BIT(30), 0x0);
}

#endif /*(RTL8733B_SUPPORT == 0)*/
