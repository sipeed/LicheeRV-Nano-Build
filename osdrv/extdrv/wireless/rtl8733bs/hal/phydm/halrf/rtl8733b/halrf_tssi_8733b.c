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

void _backup_bb_registers_8733b(void *dm_void,
				u32 *reg,
				u32 *reg_backup,
				u32 reg_num)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_get_bb_reg(dm, reg[i], MASKDWORD);

		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);*/
	}
}

void _reload_bb_registers_8733b(void *dm_void,
				u32 *reg,
				u32 *reg_backup,
				u32 reg_num)

{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		odm_set_bb_reg(dm, reg[i], MASKDWORD, reg_backup[i]);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Reload BB 0x%x = 0x%x\n",
			reg[i], reg_backup[i]);*/
	}
}

#if 1
u8 _halrf_driver_rate_to_tssi_rate_8733b(void *dm_void, u8 rate)
{
	u8 tssi_rate = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == ODM_MGN_1M)
		tssi_rate = 0;
	else if (rate == ODM_MGN_2M)
		tssi_rate = 1;
	else if (rate == ODM_MGN_5_5M)
		tssi_rate = 2;
	else if (rate == ODM_MGN_11M)
		tssi_rate = 3;
	else if (rate == ODM_MGN_6M)
		tssi_rate = 4;
	else if (rate == ODM_MGN_9M)
		tssi_rate = 5;
	else if (rate == ODM_MGN_12M)
		tssi_rate = 6;
	else if (rate == ODM_MGN_18M)
		tssi_rate = 7;
	else if (rate == ODM_MGN_24M)
		tssi_rate = 8;
	else if (rate == ODM_MGN_36M)
		tssi_rate = 9;
	else if (rate == ODM_MGN_48M)
		tssi_rate = 10;
	else if (rate == ODM_MGN_54M)
		tssi_rate = 11;
	else if (rate >= ODM_MGN_MCS0 && rate <= ODM_MGN_MCS7)
		tssi_rate = rate - ODM_MGN_MCS0 + 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF]======>%s not exit tx rate\n", __func__);

	return tssi_rate;
}

u8 _halrf_tssi_rate_to_driver_rate_8733b(void *dm_void, u8 rate)
{
	u8 driver_rate = 0;
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == 0)
		driver_rate = ODM_MGN_1M;
	else if (rate == 1)
		driver_rate = ODM_MGN_2M;
	else if (rate == 2)
		driver_rate = ODM_MGN_5_5M;
	else if (rate == 3)
		driver_rate = ODM_MGN_11M;
	else if (rate == 4)
		driver_rate = ODM_MGN_6M;
	else if (rate == 5)
		driver_rate = ODM_MGN_9M;
	else if (rate == 6)
		driver_rate = ODM_MGN_12M;
	else if (rate == 7)
		driver_rate = ODM_MGN_18M;
	else if (rate == 8)
		driver_rate = ODM_MGN_24M;
	else if (rate == 9)
		driver_rate = ODM_MGN_36M;
	else if (rate == 10)
		driver_rate = ODM_MGN_48M;
	else if (rate == 11)
		driver_rate = ODM_MGN_54M;
	else if (rate >= 12 && rate <= 19)  //83
		driver_rate = rate + ODM_MGN_MCS0 - 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF]======>%s not exit tx rate\n", __func__);
	return driver_rate;
}
#endif
u32 _halrf_get_efuse_tssi_offset_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;
	u32 offset = 0;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 2)
		offset_index = 6;
	else if (channel >= 3 && channel <= 5)
		offset_index = 7;
	else if (channel >= 6 && channel <= 8)
		offset_index = 8;
	else if (channel >= 9 && channel <= 11)
		offset_index = 9;
	else if (channel >= 12 && channel <= 14)
		offset_index = 10;
	else if (channel >= 36 && channel <= 40)
		offset_index = 11;
	else if (channel >= 42 && channel <= 48)
		offset_index = 12;
	else if (channel >= 50 && channel <= 58)
		offset_index = 13;
	else if (channel >= 60 && channel <= 64)
		offset_index = 14;
	else if (channel >= 100 && channel <= 104)
		offset_index = 15;
	else if (channel >= 106 && channel <= 112)
		offset_index = 16;
	else if (channel >= 114 && channel <= 120)
		offset_index = 17;
	else if (channel >= 122 && channel <= 128)
		offset_index = 18;
	else if (channel >= 130 && channel <= 136)
		offset_index = 19;
	else if (channel >= 138 && channel <= 144)
		offset_index = 20;
	else if (channel >= 149 && channel <= 153)
		offset_index = 21;
	else if (channel >= 155 && channel <= 161)
		offset_index = 22;
	else if (channel >= 163 && channel <= 169)
		offset_index = 23;
	else if (channel >= 171 && channel <= 177)
		offset_index = 24;

	offset = (u32)tssi->tssi_efuse[path][offset_index];

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF]=====>%s channel=%d offset_index(Chn Group)=%d offset=%d\n",
	       __func__, channel, offset_index, offset);

	return offset;
}

void halrf_tssi_get_efuse_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);

	u8 pg_tssi = 0xff, i, j, k;
	u32 pg_tssi_tmp = 0xff, pg_tmp = 0x0, thermal_tmp;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI]===>%s\n", __func__);

	/*path s0*/
	j = 0;
	for (i = 0x10; i <= 0x1a; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		j++;
	}

	for (i = 0x22; i <= 0x2f; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		j++;
	}

	k = 0;
	for (i = 0; i < 25; i++) {
		if (tssi->tssi_efuse[0][i] == -1)
			k++;
	}
	if (k == 25) {
		for(i = 0; i < 25; i++)
			tssi->tssi_efuse[0][i] = 0;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI][s0] 8733B efuse tsside no PG\n");
	}

	/*path s1*/
	j = 0;
	for (i = 0x3a; i <= 0x44; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[1][j] = (s8)pg_tssi_tmp;
		/*
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF]tssi->tssi_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_efuse[1][j]);
		*/
		j++;
	}
	k = 0;
	for (i = 0; i < 11; i++) {
		if (tssi->tssi_efuse[1][i] == -1)
			k++;
	}
	if (k == 11) {
		for(i = 0; i < 11; i++)
			tssi->tssi_efuse[1][i] = 0;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI][s1] 8733B efuse tsside no PG\n");
	}

	/*path s0*/
	odm_efuse_logical_map_read(dm, 1, 0xba, &thermal_tmp);
	cali_info->xtal_offset = 0;
	if ((thermal_tmp & 0xff) == 0xff)
		tssi->thermal[RF_PATH_A] = 0x20;
	else
		tssi->thermal[RF_PATH_A] = (u8)thermal_tmp;
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		   "[RF][Xtal] TSSI thermal_tmp = 0x%x\n", tssi->thermal[0]);

	/*power tracking type*/
	odm_efuse_logical_map_read(dm, 1, 0xc8, &pg_tmp);
	if (((pg_tmp >> 4) & 0xf) == 0xf)
		rf->power_track_type = 0x0;
	else
		rf->power_track_type = (u8)((pg_tmp >> 4) & 0xf);

}

u32 halrf_get_online_tssi_de_8733b(void *dm_void, u8 path, s32 pout)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _ADAPTER *adapter = dm->adapter;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 i, tx_rate = 0xff;
	u8 channel = *dm->channel, bandwidth = *dm->band_width;
	s32 de = 0, offset = 0, db_temp;
	u32 offset_index = 0;
	u32 tssi_de = 0;
	u32 tssi_offset;
	u8 idxbyrate[20];
	u32 idxoffset;
	s8 power_trim_de;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);

	tx_rate = phydm_get_tx_rate(dm);
	db_temp = (s32)phydm_get_tx_power_mdbm(dm, RF_PATH_A, tx_rate, bandwidth, channel);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]tx_rate = 0x%x,db_temp = 0x%x\n", tx_rate, db_temp);

	de = ((pout - db_temp * 10) * 8) / 1000;

	power_trim_de = phydm_get_tssi_trim_de(dm, path);

	de = de - power_trim_de;

#if 1
	if (path == RF_PATH_A)
		tssi_offset = odm_get_bb_reg(dm, R_0x4334, 0x0FF00000);
	else
		tssi_offset = odm_get_bb_reg(dm, R_0x4344, 0x0FF00000);

	if (tssi_offset & BIT(7))
		tssi_offset = tssi_offset | 0xffffff00;

	de = de + tssi_offset;
#endif
	if (de & BIT(7))
		tssi_de = (u32)(de | 0xffffff00);
	else
		tssi_de = (u32)de;

	tssi->tssi_de = tssi_de & 0xff;
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tssi->tssi_de = 0x%x\n", tssi->tssi_de);

	return tssi_de;
}

void halrf_tssi_set_efuse_de_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 i,diff = 2;
	s8 tssi_offest_de;
	u32 offset_index = 0;
	u8 channel = *dm->channel;
	s8 tmp;
	s8 efuse = 0xff;

	if (dm->rf_calibrate_info.txpowertrack_control == 4) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "==>%s txpowertrack_control=%d return!!!\n", __func__,
		       dm->rf_calibrate_info.txpowertrack_control);

		if (path == RF_PATH_A) {
			tmp = phydm_get_tssi_trim_de(dm, RF_PATH_A);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp = 0x%x\n", tmp);

			if (tmp > 127)
				tmp = 127;
			else if (tmp < -128)
				tmp = -128;

			tmp = tmp & 0xff;
			odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);
			odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, tmp);
			odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, tmp);
			odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, tmp);
			odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, tmp);
			odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, tmp);
		} else {
			tmp = phydm_get_tssi_trim_de(dm, RF_PATH_B);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp = 0x%x\n", tmp);

			if (tmp > 127)
				tmp = 127;
			else if (tmp < -128)
				tmp = -128;

			tmp = tmp & 0xff;
			odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, tmp);
			odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, tmp);
			odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, tmp);
			odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, tmp);
			odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, tmp);
			odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, tmp);
			odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, tmp);
		}
		return;
	}

		if (channel >= 1 && channel <= 2)
			offset_index = 0;
		else if (channel >= 3 && channel <= 5)
			offset_index = 1;
		else if (channel >= 6 && channel <= 8)
			offset_index = 2;
		else if (channel >= 9 && channel <= 11)
			offset_index = 3;
		else if (channel >= 12 && channel <= 13)
			offset_index = 4;
		else if (channel == 14)
			offset_index = 5;

		efuse = tssi->tssi_efuse[path][offset_index];
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]efuse = 0x%x\n", efuse);

		tmp = efuse + phydm_get_tssi_trim_de(dm, path) ;

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;
		
		tmp = tmp & 0xff;
		//tssi->tssi_de = tmp & 0xff;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp CCK = 0x%x\n", tmp);
		if (path == RF_PATH_A)
			odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, tmp);	//CCK
		else
			odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, tmp);  //CCK

		efuse = (s8)_halrf_get_efuse_tssi_offset_8733b(dm, path);

		tmp = efuse + phydm_get_tssi_trim_de(dm, path) ;

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;

		tmp = tmp & 0xff;
		//tssi->tssi_de = tmp & 0xff;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]tmp OFDM = 0x%x\n", tmp);
		if (path == RF_PATH_A) {
			odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, tmp);	/*HT40*/
			odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, tmp - diff);	/*OFDM*/
			odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, tmp - diff);	/*HT20*/
			odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, tmp);	/*RF40M OFDM 6M*/
			odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, tmp);	/*RF40M OFDM 6M*/
		} else {
			odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, tmp);	/*HT40*/
			odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, tmp);	/*HT40*/
			odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, tmp - diff);	/*OFDM*/
			odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, tmp);	/*RF40M OFDM 6M*/
			odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, tmp);	/*RF40M OFDM 6M*/
			odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, tmp - diff);	/*HT20*/
		}
}

u32 halrf_tssi_set_de_8733b(void *dm_void, u32 tssi_de)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	s8 de = 0;
	s32 offset = 0, db_temp;
	u8 i, rate, channel = *dm->channel, bandwidth = *dm->band_width;
	u8 idxbyrate[20];
	u32 tssi_dbm;
	u32 reg0x3axx, idxoffset;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);

	i = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	de = (s8)(tssi_de & 0xff);
	de += phydm_get_tssi_trim_de(dm, i);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]de= 0x%x\n", de);
	if (i == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, de);
		odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, de);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4334= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4334, MASKDWORD));
	} else {
		odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, de);
		odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, de);
		odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, de);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4344= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4344, MASKDWORD));
	}

	offset = (s32)((de + 0x80) & 0xff);
	tssi_dbm = (offset * 100 + 5) / 8;
	return tssi_dbm;
}

void halrf_tssi_set_de_for_tx_verify_8733b(void *dm_void, u32 tssi_de, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	s8 de = 0;
	s32 offset = 0, db_temp;
	u8 i, rate, channel = *dm->channel, bandwidth = *dm->band_width;
	u8 idxbyrate[20];
	u32 tssi_dbm;
	u32 reg0x3axx, idxoffset;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	i = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));
	de = (s8)(tssi_de & 0xff);
	de += phydm_get_tssi_trim_de(dm, path);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]de= 0x%x\n", de);
	if (i == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x4334, 0x0FF00000, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x43b0, 0xFF000000, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x0000FF00, de);
		odm_set_bb_reg(dm, R_0x43b0, 0x00FF0000, de);
		odm_set_bb_reg(dm, R_0x433c, 0x0FF00000, de);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4334= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4334, MASKDWORD));
	} else {
		odm_set_bb_reg(dm, R_0x4344, 0x0FF00000, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x0000FF00, de);
		odm_set_bb_reg(dm, R_0x43b4, 0x00FF0000, de);
		odm_set_bb_reg(dm, R_0x43b4, 0xFF000000, de);
		odm_set_bb_reg(dm, R_0x43b8, 0x000000FF, de);
		odm_set_bb_reg(dm, R_0x434c, 0x0FF00000, de);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4344= 0x%x\n",
			odm_get_bb_reg(dm, R_0x4344, MASKDWORD));
	}
}

void _halrf_tssi_anapar_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32  reg_rf18;
	u8  band;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);
	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);
	band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF]==>tssi in band %s\n", (band == 0 ? "2G" : "5G"));
	/*00_set_tssi_sys_2G/5G.txt*/
	odm_set_bb_reg(dm, R_0x1860, BIT(30), 0x0);//anapar non dbg mode
	if (band == 0) {/*ANAPAR*/
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705f0041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	} else {//5G,only s0
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0048);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705f0041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	}  //0xFFA1005E
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffb5005e); //AD/DA fifo rst
	odm_set_bb_reg(dm, R_0x1d40, BIT(3), 0x0);
	odm_set_bb_reg(dm, R_0x1e1c, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x1e1c, BIT(26), 0x1);
	odm_set_bb_reg(dm, R_0x1ca4, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x1e1c, 0x0000F000, 0x8);//0xc ADC 160M,0x8 ADC 10M,
}

void _halrf_tssi_rf_setting_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, BIT(8), 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x55, BIT(7), 0x1); //Enable RF power tracking at RFC
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x7f, BIT(8), 0x1);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x55, BIT(7), 0x1); //Enable RF power tracking at RFC

}

void halrf_tssi_dck_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	u8 channel = *dm->channel;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s channel=%d ======\n",
	       __func__, channel);
	//05_set_tssi_dck.txt  auto DCK
	odm_set_bb_reg(dm, R_0x4328, BIT(24), 0x1);
	odm_set_bb_reg(dm, R_0x4328, BIT(25), 0x1);
	odm_set_bb_reg(dm, R_0x4328, BIT(29) | BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0x4328, BIT(30), 0x1);
	odm_set_bb_reg(dm, R_0x432c, 0x000000FF, 0x51);/*0x51*/
	//odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x000003fe);
	odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x00000002);
	odm_set_bb_reg(dm, R_0x4378, MASKDWORD, 0x000003fd);
	odm_set_bb_reg(dm, R_0x436c, MASKDWORD, 0x00000000);
}
void _halrf_tssi_set_powerlevel_8733b(void *dm_void, s16 power_offset,u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	s32 db_temp;
	u32 idxoffset;
	s32 de = 0, offset = 0;
	u8 i, rate, channel = *dm->channel, bandwidth = *dm->band_width;
	u8 idxbyrate[20];
	u16 reg0x3axx;
	s8 offset_mcs7,offset_cck11m,offset_6m;
	s8 diff1 = 0xe4,diff2 = 0x18;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] ======>%s\n", __func__);

	for (i = 0; i < 20; i++) {  //ODM_MGN_MCS7 = 0x87,tssi_rate = 19
		rate = _halrf_tssi_rate_to_driver_rate_8733b(dm, i);
		db_temp = (s32)phydm_get_tx_power_mdbm(dm, path, rate, bandwidth, channel);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]db_temp = 0x%x\n", db_temp);*/
		offset = (db_temp - 1600 ) * 4 / 100;
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]offset = 0x%x\n", offset);*/
		offset = offset + (power_offset / 25);
		if (offset > 127)
			offset = 127;
		else if (offset < -128)
			offset = -128;
		if (offset & BIT(8))
			idxbyrate[i] = (offset & 0xff) | BIT(7);
		else
			idxbyrate[i] = offset & 0xff;
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI]idxbyrate[%d] = 0x%x\n", i, offset);*/
	}
	offset_mcs7 = idxbyrate[19];
	offset_cck11m = idxbyrate[0];
	offset_6m = idxbyrate[4];
	if(!(power_offset == 0)){
		if ((offset_mcs7 < diff1) || (offset_cck11m > diff2) || (offset_6m > diff2)){
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] Out of range\n");
			return;
		}
	}
	for (i = 0; i < 20; i = i + 4) {
		reg0x3axx = (u16)(0x3a00 + i);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] before reg0x%x = 0x%x\n",
			reg0x3axx, odm_get_bb_reg(dm, reg0x3axx, MASKDWORD));*/
		idxoffset = (idxbyrate[i] & 0xff) |
			(idxbyrate[i + 1] & 0xff) << 8 |
			(idxbyrate[i + 2] & 0xff) << 16 |
			(idxbyrate[i + 3] & 0xff) << 24;
		odm_set_bb_reg(dm, reg0x3axx, MASKDWORD, idxoffset);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] after reg0x%x = 0x%x\n",
			reg0x3axx, odm_get_bb_reg(dm, reg0x3axx, MASKDWORD));*/

	}
}

u32 halrf_tssi_set_powerbyrate_pout_8733b(void *dm_void, s16 power_offset, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 mask;
        s32 pout;
	u16 rate_reg;
	u8 tx_rate = phydm_get_tx_rate(dm);
	s8 rateidx_offset = 0, value0 = 0;
	u8 value;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);

	rateidx_offset = (s8)(power_offset / 25);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			   "[RF][TSSI] tx_rate = %d\n", tx_rate);

	switch (tx_rate) {
		case MGN_1M: 
			rate_reg = 0x3A00;
			mask = MASKBYTE0;
			break;
		case MGN_2M:
			rate_reg = 0x3A00;
			mask = MASKBYTE1;
			break;
		case MGN_5_5M:
			rate_reg = 0x3A00;
			mask = MASKBYTE2;
			break;
		case MGN_11M:
			rate_reg = 0x3A00;
			mask = MASKBYTE3;
			break;
		case MGN_6M:
			rate_reg = 0x3A04;
			mask = MASKBYTE0;
			break;
		case MGN_9M:
			rate_reg = 0x3A04;
			mask = MASKBYTE1;
			break;
		case MGN_12M:
			rate_reg = 0x3A04;
			mask = MASKBYTE2;
			break;
		case MGN_18M:
			rate_reg = 0x3A04;
			mask = MASKBYTE3;
			break;
		case MGN_24M:
			rate_reg = 0x3A08;
			mask = MASKBYTE0;
			break;
		case MGN_36M:
			rate_reg = 0x3A08;
			mask = MASKBYTE1;
			break;
		case MGN_48M:
			rate_reg = 0x3A08;
			mask = MASKBYTE2;
			break;
		case MGN_54M:
			rate_reg = 0x3A08;
			mask = MASKBYTE3;
			break;
		case MGN_MCS0:
			rate_reg = 0x3A0c;
			mask = MASKBYTE0;
			break;
		case MGN_MCS1:
			rate_reg = 0x3A0c;
			mask = MASKBYTE1;
			break;
		case MGN_MCS2:
			rate_reg = 0x3A0c;
			mask = MASKBYTE2;
			break;
		case MGN_MCS3:
			rate_reg = 0x3A0c;
			mask = MASKBYTE3;
			break;
		case MGN_MCS4:
			rate_reg = 0x3A10;
			mask = MASKBYTE0;
			break;
		case MGN_MCS5:
			rate_reg = 0x3A10;
			mask = MASKBYTE1;
			break;
		case MGN_MCS6:
			rate_reg = 0x3A10;
			mask = MASKBYTE2;
			break;
		case MGN_MCS7:
			rate_reg = 0x3A10;
			mask = MASKBYTE3;
			break;
		default:
			rate_reg = 0x3A10;
			mask = MASKBYTE3;
			break;
		   	}

	value0 = (s8)(odm_get_bb_reg(dm, rate_reg, mask) & 0xff) + rateidx_offset;
	value = (u8)(value0 & 0xFF);
	odm_set_bb_reg(dm, rate_reg, mask, value);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] 0x%x [%x]= 0x%x \n", rate_reg, mask, value);
	pout = (s32)value0 * 100 / 4 +1600;
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] Pout = %d (/100)\n", pout);
	return pout;
}

void _halrf_tssi_set_txpwr_bb_com_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);

	//01_set_txpwr_bb_com.txt, txagc_by_rate
#if 1

	if (cali_info->txpowertrack_control == 2) {
		odm_set_bb_reg(dm, R_0x3a00, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x3a04, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x3a08, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x3a0c, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x3a10, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x3a14, MASKDWORD, 0x00000000);
	} else {
		_halrf_tssi_set_powerlevel_8733b(dm, 0, path);
	}

#endif
	//02_ini_txpwr_ctrl_bb.txt
	odm_set_bb_reg(dm, R_0x4300, 0x1F, 0x00);
	odm_set_bb_reg(dm, R_0x4300, 0x00FFFF00, 0x00ff);
	odm_set_bb_reg(dm, R_0x4300, 0x07000000, 0x4);
	odm_set_bb_reg(dm, R_0x4300, 0xF0000000, 0x4);
	if ((channel <= 14) && (dm-> rfe_type == 2||
		dm->rfe_type == 4 || dm->rfe_type == 9))
		odm_set_bb_reg(dm, R_0x4304, 0x0000FFFF, 0x8080);
	else
		odm_set_bb_reg(dm, R_0x4304, 0x0000FFFF, 0x0000);
	odm_set_bb_reg(dm, R_0x4304, 0xFFFF0000, 0x0000);
	odm_set_bb_reg(dm, R_0x4308, MASKDWORD, 0x50405040);
	odm_set_bb_reg(dm, R_0x430c, MASKDWORD, 0x3f3f3f3f);
	odm_set_bb_reg(dm, R_0x4310, MASKDWORD, 0x003f3f3f);
	odm_set_bb_reg(dm, R_0x4314, 0x000001FF, 0x000);
	odm_set_bb_reg(dm, R_0x4314, 0x00007000, 0x7);
	odm_set_bb_reg(dm, R_0x4314, 0x00038000, 0x7);
	odm_set_bb_reg(dm, R_0x4314, 0x007C0000, 0x1f);
	odm_set_bb_reg(dm, R_0x4314, 0x0F800000, 0x00);
	odm_set_bb_reg(dm, R_0x4318, 0x0000FFFF, 0x807f);
	odm_set_bb_reg(dm, R_0x4318, 0x7FFF0000, 0x0);
	odm_set_bb_reg(dm, R_0x431c, MASKDWORD, 0x0076280a);
	odm_set_bb_reg(dm, R_0x4320, 0x0000007F, 0x00);
	odm_set_bb_reg(dm, R_0x4320, 0x00000100, 0x1);
	odm_set_bb_reg(dm, R_0x4320, 0x0000FE00, 0x00);
	odm_set_bb_reg(dm, R_0x4320, 0x00FF0000, 0x88);
	odm_set_bb_reg(dm, R_0x4320, 0x0F000000, 0x2);
	odm_set_bb_reg(dm, R_0x4324, MASKDWORD, 0x807f807f);
	odm_set_bb_reg(dm, R_0x4328, 0x00FFFFFF, 0x280200);
	odm_set_bb_reg(dm, R_0x4328, 0x7F000000, 0x43);
	odm_set_bb_reg(dm, R_0x432c, 0x000000FF, 0x50);
	odm_set_bb_reg(dm, R_0x432c, 0x0001FF00, 0x0ff);
	odm_set_bb_reg(dm, R_0x432c, 0x1FF00000, 0x100);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x03FF0000, 0x000);
	/*odm_set_bb_reg(dm, R_0x4334, MASKDWORD, 0x00000000);*/
	odm_set_bb_reg(dm, R_0x4338, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4338, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x433c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4340, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4340, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4348, 0x00000FFF, 0x800);
	/*odm_set_bb_reg(dm, R_0x4348, 0x03FF0000, 0x000);*/
	odm_set_bb_reg(dm, R_0x434c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4350, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4354, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4358, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x435c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4360, 0x00000003, 0x0);
	odm_set_bb_reg(dm, R_0x4360, 0x01FFFFF0, 0x1f1f1f);
	odm_set_bb_reg(dm, R_0x4364, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4368, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x436c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4370, 0x001FFFFF, 0x1f1f1f);
	odm_set_bb_reg(dm, R_0x4374, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4378, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x437c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4380, MASKDWORD, 0x00000002);
	odm_set_bb_reg(dm, R_0x4384, MASKDWORD, 0x100000ff);
	odm_set_bb_reg(dm, R_0x4388, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x438c, 0x00007FFF, 0x4040);
	odm_set_bb_reg(dm, R_0x438c, 0xFFFF0000, 0xA0A0);
	odm_set_bb_reg(dm, R_0x4390, 0x0000FFFF, 0x4040);
	odm_set_bb_reg(dm, R_0x4390, 0xFFFF0000, 0x8080);
	odm_set_bb_reg(dm, R_0x4394, 0x00007FFF, 0x4040);
	odm_set_bb_reg(dm, R_0x4394, 0xFFFF0000, 0xA4A4);
	odm_set_bb_reg(dm, R_0x4398, 0x0000FFFF, 0x8080);
	odm_set_bb_reg(dm, R_0x4398, 0xFFFF0000, 0x8080);
	odm_set_bb_reg(dm, R_0x439c, 0x00000007, 0x1);
	odm_set_bb_reg(dm, R_0x439c, 0x0FFFFFF0, 0x080080);
	odm_set_bb_reg(dm, R_0x439c, 0x30000000, 0x0);
	odm_set_bb_reg(dm, R_0x43a0, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x43a4, 0x00001FFF, 0x0000);
	odm_set_bb_reg(dm, R_0x43a4, BIT(16), 0x1);
	odm_set_bb_reg(dm, R_0x43a8, 0x0000001F, 0x00);
	odm_set_bb_reg(dm, R_0x43a8, 0x00000F00, 0xd);
	if ((channel <= 14)&& (dm->rfe_type == 2
		|| dm->rfe_type == 4 || dm->rfe_type == 9))
		odm_set_bb_reg(dm, R_0x43a8, 0x0000F000, 0xf);
	else
		odm_set_bb_reg(dm, R_0x43a8, 0x0000F000, 0x0);
	odm_set_bb_reg(dm, R_0x43a8, 0x00070000, 0x7);
	odm_set_bb_reg(dm, R_0x43a8, 0x00380000, 0x0);
	odm_set_bb_reg(dm, R_0x43a8, 0x03C00000, 0xd);
	odm_set_bb_reg(dm, R_0x43a8, 0x7C000000, 0x1d);
	odm_set_bb_reg(dm, R_0x43ac, 0x0000FFFF, 0x4040);
	odm_set_bb_reg(dm, R_0x1ca4, BIT(30), 0x1);
	odm_set_bb_reg(dm, R_0x1c84, 0x0000FC00, 0x8);/*ofdm*/
	odm_set_bb_reg(dm, R_0x1c84, 0x000003c0, 0x0);/*cck*/
}

void _halrf_tssi_set_tmeter_tbl_zero_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	//struct _halrf_tssi_info *tssi = &rf->halrf_tssi_info;
	u8 i = 0x0, thermal = 0xff;
	u32 thermal_offset_tmp = 0x0;
	s8 thermal_offset[64] = {0};
	u16 reg0x42xx;
	u8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};
	u8 thermal_up_b[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_b[DELTA_SWINGIDX_SIZE] = {0};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);

	odm_set_bb_reg(dm, R_0x4380, 0x00000007, 0x3);
	odm_set_bb_reg(dm, R_0x4380, 0x000FFFF0, 0x0000);
	odm_set_bb_reg(dm, R_0x4380, 0xFFF00000, 0x000);

	for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
			(thermal_offset[i + 1] & 0xff) << 8 |
			(thermal_offset[i + 2] & 0xff) << 16 |
			(thermal_offset[i + 3] & 0xff) << 24;
		reg0x42xx = (u16)(0x4200 + i);
		odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] reg0x%x = 0x%x\n",
		        reg0x42xx, odm_get_bb_reg(dm, reg0x42xx, MASKDWORD));*/
	}
}

void _halrf_tssi_set_tmeter_tbl_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel, i, thermal;
	s8 j;
	u16 reg0x42xx;
	u8 rate = phydm_get_tx_rate(dm);
	u32 thermal_offset_tmp = 0, thermal_offset_index = 0x10, thermal_tmp = 0xff, tmp;
	s8 thermal_offset[64] = {0};
	u8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};
	u8 thermal_up_b[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_b[DELTA_SWINGIDX_SIZE] = {0};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[RF][TSSI] ======>%s\n", __func__);

	odm_set_bb_reg(dm, R_0x4380, 0x00000007, 0x3);
        odm_set_bb_reg(dm, R_0x4380, 0x00000FF0, tssi->thermal[RF_PATH_A]);
	odm_set_bb_reg(dm, R_0x4380, 0x000FF000, 0x0000);
	odm_set_bb_reg(dm, R_0x4380, 0xFFF00000, 0x000);

	if (cali_info->txpowertrack_control == 4) {
		for (i = 0; i < 64; i = i + 4) {
			thermal_offset_tmp = (thermal_offset[i] & 0xff) |
				(thermal_offset[i + 1] & 0xff) << 8 |
				(thermal_offset[i + 2] & 0xff) << 16 |
				(thermal_offset[i + 3] & 0xff) << 24;
			reg0x42xx = (u16)(0x4200 + i);
			odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "[RF][TSSI]TSSI_CAL reg0x%x = 0x%x\n",
			       reg0x42xx, odm_get_bb_reg(dm, reg0x42xx, MASKDWORD));
		}
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]TSSI_CAL, return!\n");
		return;
	}

	if (rate == ODM_MGN_1M || rate == ODM_MGN_2M || rate == ODM_MGN_5_5M || rate == ODM_MGN_11M) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2g_cck_a_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2g_cck_a_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2g_cck_b_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2g_cck_b_n, sizeof(thermal_down_b));
	} else if (channel >= 1 && channel <= 14) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2ga_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2ga_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2gb_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2gb_n, sizeof(thermal_down_b));
	} else if (channel >= 36 && channel <= 64) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[0], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[0], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[0], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[0], sizeof(thermal_down_b));
	} else if (channel >= 100 && channel <= 144) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[1], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[1], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[1], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[1], sizeof(thermal_down_b));
	} else if (channel >= 149 && channel <= 177) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[2], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[2], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[2], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[2], sizeof(thermal_down_b));
	}

	i = 0;
	for (j = 0; j < 32; j++) {
		if (i < DELTA_SWINGIDX_SIZE)
			thermal_offset[j] = -1 *thermal_down_a[i++];
		else
			thermal_offset[j] = -1 *thermal_down_a[DELTA_SWINGIDX_SIZE - 1];

	}
	i = 1;
	for (j = 63; j >= 32; j--) {
		if (i < DELTA_SWINGIDX_SIZE)
			thermal_offset[j] = thermal_up_a[i++];
		else
			thermal_offset[j] = thermal_up_a[DELTA_SWINGIDX_SIZE - 1];
	}

	for (i = 0; i < 64; i = i + 4) {
		thermal_offset_tmp = (thermal_offset[i] & 0xff) |
				(thermal_offset[i + 1] & 0xff) << 8 |
				(thermal_offset[i + 2] & 0xff) << 16 |
				(thermal_offset[i + 3] & 0xff) << 24;
		reg0x42xx = (u16)(0x4200 + i);
		odm_set_bb_reg(dm, reg0x42xx, MASKDWORD, thermal_offset_tmp);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] reg 0x%x =0x%x\n",
		       reg0x42xx, thermal_offset_tmp);
	}
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]set_tmeter_tbl over!\n");
}

void _tssi_set_hwtx_8733b(struct dm_struct *dm, u8 path,u8 rate,u8 pkt_cnt, boolean is_tx_start)
{
	struct phydm_pmac_info tx_info;

	if (is_tx_start) {
		tx_info.en_pmac_tx = true;
		tx_info.mode = PKTS_TX;
		tx_info.ndp_sound = false;
		tx_info.bw = CHANNEL_WIDTH_20;
		tx_info.tx_sc = 0x0; /*duplicate*/
		tx_info.m_stbc = 0x0; /*disable*/
		tx_info.tx_rate = rate;
		tx_info.packet_count = pkt_cnt;//0
		tx_info.length = 1000;
		tx_info.packet_period = 5; /*d'500 us*/
		tx_info.packet_length = 0;

		if (tx_info.tx_rate == ODM_RATE11M) {
			tx_info.signal_field = 0x6e; /*rate = 11M*/
			tx_info.service_field_bit2= 0x1;
			tx_info.packet_length = 1000; /*1000 bytes*/
			tx_info.length = 8000; /*d'8000 us=1000 bytes*/
		}

		phydm_reset_bb_hw_cnt(dm);
		phydm_set_pmac_tx(dm, &tx_info, path);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] S%d Start pmac_tx mode.\n", path);*/

	} else {
		tx_info.en_pmac_tx = false;
		phydm_set_pmac_tx(dm, &tx_info, path);
		phydm_set_tmac_tx(dm);
		/*RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] S%d Stop pmac_tx and turn on true mac mode.\n",
			path);*/
	}
}
void _tssi_tx_pause_8733b(struct dm_struct *dm)
{
	u8 reg_rf0_a, reg_rf0_b;
	u16 count = 0;

	odm_write_1byte(dm, R_0x522, 0xff);
	odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2); /*hw tx stop*/

	reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
	reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);

	while (((reg_rf0_a == 2) || (reg_rf0_b == 2)) && count < 2500) {
		reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
		reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);
		ODM_delay_us(2);
		count++;
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] Tx pause!!\n");

}

void _halrf_tssi_set_rf_gap_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;

	odm_set_bb_reg(dm, R_0x4350, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4354, MASKDWORD, 0x00000000);
}

void _halrf_tssi_set_slope_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	odm_set_bb_reg(dm, R_0x4320, BIT(24), 0x1);
	odm_set_bb_reg(dm, R_0x4328, 0x00FFFFFF, 0x280200);//640,512pts
	odm_set_bb_reg(dm, R_0x4320, 0x0000F000, 0x3);
	odm_set_bb_reg(dm, R_0x4330, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4330, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x4338, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4338, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x433c, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4340, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x4340, 0x03FF0000, 0x000);
	odm_set_bb_reg(dm, R_0x4344, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x4348, 0x00000FFF, 0x800);
	odm_set_bb_reg(dm, R_0x434c, MASKDWORD, 0x00000000);
}

void _halrf_tssi_set_slope_cal_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4390, MASKDWORD, 0x80808080);
	odm_set_bb_reg(dm, R_0x4398, MASKDWORD, 0x80808080);
	odm_set_bb_reg(dm, R_0x439c, BIT(0), 0x1);
}

void _halrf_tssi_set_tssi_track_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x4320, BIT(24), 0x0);
	odm_set_bb_reg(dm, R_0x439c, 0x0FFFFFF0, 0x080080);//0.125db/cw
}

void _halrf_run_tssi_slope_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	//wire r_tssi_en
	odm_set_bb_reg(dm, R_0x4318, BIT(28), 0x0);
	odm_set_bb_reg(dm, R_0x4318, BIT(28), 0x1);
}

void _halrf_rpt_tssi_adc_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32  adc_preamble, dck_auto_avg, dck_auto_max, dck_auto_min;
	//wire  r_tssi_en
	odm_set_bb_reg(dm, R_0x43a8, 0x0000001F, 0x0d);
	adc_preamble = odm_get_bb_reg(dm, R_0x4380, 0x000003FF);
	dck_auto_avg = odm_get_bb_reg(dm, R_0x42b0, 0x000003FF);
	dck_auto_max = odm_get_bb_reg(dm, R_0x42b0, 0x003FF000);
	dck_auto_min = odm_get_bb_reg(dm, R_0x42b4, 0x003FF000);
}

void halrf_enable_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	_halrf_tssi_set_tssi_track_8733b(dm);//08
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x7);
	iqk_info->is_tssi_mode = true;
}

void halrf_disable_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	odm_set_bb_reg(dm, R_0x4318, 0x70000000, 0x0);
	iqk_info->is_tssi_mode = false;
}

void _halrf_tssi_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "[RF][TSSI] ======>%s\n", __func__);
	_halrf_tssi_anapar_8733b(dm, path);//00
	_halrf_tssi_rf_setting_8733b(dm, path);//00 ?==>radioa&b
	_halrf_tssi_set_txpwr_bb_com_8733b(dm, path); //01,02
	_halrf_tssi_set_tmeter_tbl_8733b(dm); //03
	_halrf_tssi_set_rf_gap_8733b(dm); //04
	halrf_tssi_dck_8733b(dm);//05
	_halrf_tssi_set_slope_8733b(dm);//06
	_halrf_tssi_set_slope_cal_8733b(dm);//07
}

void _halrf_set_power_base_8733b(void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	u32 tx_cnt = 0x0, poll_cnt = 0x0;
	u32 bb_reg[5] = {R_0x43b0, R_0x3a10, R_0x43b8, R_0x1884,R_0x1b20};
	u32 bb_reg_backup[5] = {0};
	u8 channel = *dm->channel, bandwidth = *dm->band_width;
	u8  txagc, tssi_offset, txagc_bb, rf_reg18;
	u8 i = 0, backup_num = 5;
	s8 tmp[2] = {0};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);

	_backup_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
	odm_set_bb_reg(dm, R_0x1b20, 0x0F000000, 0x0); // disable DPD
	rf_reg18 = (u8)odm_get_rf_reg(dm, path, RF_0x18, MASKBYTE0);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] R_0x1884 = 0x%x\n",
		odm_get_bb_reg(dm, R_0x1884, MASKDWORD));
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] rf_reg18 = 0x%x,\n", rf_reg18);
	tmp[0] = tssi->tssi_efuse[0][9];  /*S0 efuse_de*/
	tmp[1] = tssi->tssi_efuse[1][9];   /*S1 efuse_de*/
	odm_set_bb_reg(dm, R_0x43b0, MASKBYTE3, tmp[0]);//S0:HT20
	odm_set_bb_reg(dm, R_0x43b8, MASKBYTE0, tmp[1]);//S1:HT20
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] 0x43b0 = 0x%x, 0x43b8 = 0x%x\n",tmp[0], tmp[1]);
	if (dm->cut_version < ODM_CUT_D) {
		for (i = 0; i < 5; i++) {
			/*write RF-0x18*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, MASKBYTE0, 9);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, MASKBYTE0, 9);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, 0xA0, 0x4, 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x0);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	} else {
		/*write RF-0x18*/
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, MASKBYTE0, 9);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, MASKBYTE0, 9);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] rf_0x18 = 0x%x\n",
		odm_get_rf_reg(dm, path, RF_0x18, RFREG_MASK));
	odm_set_bb_reg(dm, R_0x3a10, MASKBYTE3, 0xfc);/*15dBm 0xfc*/
	halrf_enable_tssi_8733b(dm);
	_tssi_set_hwtx_8733b(dm, path,ODM_RATEMCS7,20, true);
	while (1) {
		tx_cnt = odm_get_bb_reg(dm, R_0x2de0, MASKLWORD);
		if (tx_cnt >= 20 || poll_cnt >= 100)
			break;
		ODM_delay_ms(1);
		poll_cnt++;
	}
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] ofdm cnt = %d, poll cnt=%d.\n",tx_cnt, poll_cnt);
	txagc = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x0000001f);
	tssi_offset = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x001f0000);
	txagc_bb = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x0000ff00);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] path = %d, 0x42f0 = 0x%x\n",
		path, odm_get_bb_reg(dm, R_0x42f0, MASKDWORD));
	tssi->txagc_offset_thermaltrack[path] =
		8 + 8 * (txagc + tssi_offset -0x14) + txagc_bb;
	tssi->thermal_cal = (u8)odm_get_rf_reg(dm, 0, R_0x42, 0x7e);
	_tssi_set_hwtx_8733b(dm, path,ODM_RATEMCS7,20, false);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] current_path = %d, txagc_offset[0] = 0x%x\n",
		path, tssi->txagc_offset_thermaltrack[path]);
	if ((!(dm->rfe_type <= 2 ||
		dm->rfe_type == 4 || dm->rfe_type == 9))) {
		/*if not only one path, do another pathK*/
		poll_cnt = 0;
		path = ~path & 0x1;
		odm_set_bb_reg(dm, R_0x1884, BIT(20), path);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "RF][TSSI] Set RF path =S%d\n", path);
		//ODM_delay_ms(1);
		_tssi_set_hwtx_8733b(dm, path,ODM_RATEMCS7,20, true);
		while (1) {
			tx_cnt = odm_get_bb_reg(dm, R_0x2de0, MASKLWORD);
			if (tx_cnt >= 20 || poll_cnt >= 100)
				break;
			ODM_delay_ms(1);
			poll_cnt++;
		}
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] ofdm cnt = %d, poll cnt=%d.\n",
			tx_cnt, poll_cnt);
		txagc = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x0000001f);
		tssi_offset = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x001f0000);
		txagc_bb = (u8)odm_get_bb_reg(dm, R_0x42f0, 0x0000ff00);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] path = %d, 0x42f0 = 0x%x\n",
		path, odm_get_bb_reg(dm, R_0x42f0, MASKDWORD));
		tssi->txagc_offset_thermaltrack[path] =
			8 + 8 * (txagc + tssi_offset -0x14) + txagc_bb;
		_tssi_set_hwtx_8733b(dm, path,ODM_RATEMCS7,20, false);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] another_path = %d, txagc_offset = 0x%x\n",
			path, tssi->txagc_offset_thermaltrack[path]);
	}
	_tssi_tx_pause_8733b(dm);
	halrf_disable_tssi_8733b(dm);
	_reload_bb_registers_8733b(dm, bb_reg, bb_reg_backup, backup_num);
	if (dm->cut_version < ODM_CUT_D) {
		for (i = 0; i < 5; i++) {
			/*write RF-0x18*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, MASKBYTE0, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, MASKBYTE0, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, 0xA0, 0x4, 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x0);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	} else {
		/*write RF-0x18*/
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, MASKBYTE0, rf_reg18);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, MASKBYTE0, rf_reg18);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] thermal = 0x%x\n", tssi->thermal_cal);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] rf_0x18 = 0x%x\n",
		odm_get_rf_reg(dm, 0, RF_0x18, RFREG_MASK));
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] R_0x1884 = 0x%x\n",
		odm_get_bb_reg(dm, R_0x1884, MASKDWORD));
}

void halrf_do_tssi_8733b(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u32 bb_reg[10] = {R_0x9f0, R_0x9b4, R_0x1c38, R_0x1860, R_0x1cd0,
			  R_0x824, R_0x2a24, R_0x1d40, R_0x1c20, R_0x1880};
	u32 tx_pause_reg[3] = {R_0x1e70, R_0x522, R_0x4384};
	u32 bb_reg_backup[10] = {0};
	u32 tx_pause_reg_backup[3] = {0};
	u32 backup_num = 10;
	u32 backup_num2 =3;
	u8 i;
	s32 db_temp;
	s8 pwr_threshold = 0xe4;
	s8 offset = 0;
	u8 channel = *dm->channel, bandwidth = *dm->band_width;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI] ======>%s\n", __func__);

	i = (u8)odm_get_bb_reg(dm, 0x1884, BIT(20));

	rf->is_tssi_in_progress = 1;
	_backup_bb_registers_8733b(dm,
		tx_pause_reg, tx_pause_reg_backup, backup_num2);
	halrf_disable_tssi_8733b(dm);
	_halrf_tssi_8733b(dm, i);
	halrf_tssi_set_efuse_de_8733b(dm, RF_PATH_A);
	halrf_tssi_set_efuse_de_8733b(dm, RF_PATH_B);
	db_temp = (s32)phydm_get_tx_power_mdbm(dm,
		i, MGN_MCS7, bandwidth, channel);
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"[RF][TSSI] target_pwr_MCS7 = %d(/100)\n", db_temp);
	offset = (s8)((db_temp - 1600 ) * 4 / 100);
	if (offset > 127)
		offset = 127;
	else if (offset < -128)
		offset = -128;

	if (offset & BIT(8))
		offset = (offset & 0xff) | BIT(7);
	else
		offset = offset & 0xff;

	if (offset < pwr_threshold) {
		rf->rfk_type = RF00_PWR_TRK;
		halrf_rfk_handshake(dm, true);
		btc_set_gnt_wl_bt_8733b(dm, true);
		_halrf_set_power_base_8733b(dm, i);
		btc_set_gnt_wl_bt_8733b(dm, false);
		halrf_rfk_handshake(dm, false);
		_tssi_tx_pause_8733b(dm);
		halrf_disable_tssi_8733b(dm);
		_reload_bb_registers_8733b(dm,
			tx_pause_reg, tx_pause_reg_backup, backup_num2);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] disable tssi: 0x3A10 = 0x%x\n",offset);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[RF][TSSI] Out of tssi cover range\n");
			rf->is_tssi_in_progress = 0;
		return;
	}
	_reload_bb_registers_8733b(dm,
		tx_pause_reg, tx_pause_reg_backup, backup_num2);


	if (*dm->mp_mode == 1) {
		if (cali_info->txpowertrack_control >= 3) { /*TSSI ON/cal*/
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "[RF][TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
			       cali_info->txpowertrack_control);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4308= 0x%x\n",
				odm_get_bb_reg(dm, R_0x4308, MASKDWORD));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4334= 0x%x\n",
				odm_get_bb_reg(dm, R_0x4334, MASKDWORD));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF][TSSI]0x4344= 0x%x\n",
				odm_get_bb_reg(dm, R_0x4344, MASKDWORD));
			halrf_enable_tssi_8733b(dm);
			rf->is_tssi_in_progress = 0;
			return;
		} else {
			halrf_disable_tssi_8733b(dm);
			rf->is_tssi_in_progress = 0;
			return;
		}
	} else {  //efuse 0xC8 define 0x4h-0x7h for power tracking by TSSI
#if 1
		if (!(rf->rf_supportability & HAL_RF_TX_PWR_TRACK)) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"[RF][TSSI] rf_supportability HAL_RF_TX_PWR_TRACK=%d, return!!!\n",
				(rf->rf_supportability & HAL_RF_TX_PWR_TRACK));
			halrf_disable_tssi_8733b(dm);
			rf->is_tssi_in_progress = 0;
			return;
		}

#endif
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7) {

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"[RF][TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
				cali_info->txpowertrack_control);
			halrf_enable_tssi_8733b(dm);
			rf->is_tssi_in_progress = 0;
			return;
		}
	}
	rf->is_tssi_in_progress = 0;
}


#endif
