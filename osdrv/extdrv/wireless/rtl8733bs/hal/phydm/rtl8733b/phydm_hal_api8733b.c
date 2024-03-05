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
 ******************************************************************* **********/
#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8733B_SUPPORT)
#if (PHYDM_FW_API_ENABLE_8733B)
/* ======================================================================== */
/* These following functions can be used for PHY DM only*/
#ifdef CONFIG_TXAGC_DEBUG_8733B
__odm_func__
boolean phydm_set_pw_by_rate_8733b(struct dm_struct *dm, s8 *pw_idx,
				   u8 rate_idx)
{
	u32 pw_all = 0;
	u8 j = 0;

	if (rate_idx % 4 != 0) {
		pr_debug("[Warning] %s\n", __func__);
		return false;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "pow = {%d, %d, %d, %d}\n",
		  *pw_idx, *(pw_idx - 1), *(pw_idx - 2), *(pw_idx - 3));

	/* @According the rate to write in the ofdm or the cck */
	/* @driver need to construct a 4-byte power index */
	odm_set_bb_reg(dm, 0x3a00 + rate_idx, MASKDWORD, pw_all);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate_idx=0x%x (REG0x%x) = 0x%x\n",
		  rate_idx, 0x3a00 + rate_idx, pw_all);

	for (j = 0; j < 4; j++)
		config_phydm_read_txagc_diff_8733b(dm, rate_idx + j);

	return true;
}

__odm_func__
void phydm_txagc_tab_buff_init_8733b(struct dm_struct *dm)
{
	u8 i;

	for (i = 0; i < NUM_RATE_N_1SS; i++) {
		dm->txagc_buff[RF_PATH_A][i] = i >> 2;
		dm->txagc_buff[RF_PATH_B][i] = i >> 2;
	}
}

__odm_func__
void phydm_txagc_tab_buff_show_8733b(struct dm_struct *dm)
{
	u8 i;

	pr_debug("path A\n");
	for (i = 0; i < NUM_RATE_N_1SS; i++)
		pr_debug("[A][rate:%d] = %d\n", i,
			 dm->txagc_buff[RF_PATH_A][i]);
		pr_debug("path B\n");
	for (i = 0; i < NUM_RATE_N_1SS; i++)
		pr_debug("[B][rate:%d] = %d\n", i,
			 dm->txagc_buff[RF_PATH_B][i]);
}
#endif
#if 0
__odm_func__
void phydm_init_hw_info_by_rfe_type_8733b(struct dm_struct *dm)
{
	dm->is_init_hw_info_by_rfe = false;

	if (dm->rfe_type == 0) {
		panic_printk("[97F] RFE type 0 PHY paratemters: DEFAULT\n");
		odm_cmn_info_init(dm, ODM_CMNINFO_BOARD_TYPE,
				  ODM_BOARD_DEFAULT);
		odm_set_bb_reg(dm, R_0x4c, BIT(24), 0x0); /*GPIO setting*/
		odm_set_bb_reg(dm, R_0x64, 0x30000000, 0x0); /*GPIO setting*/
	} else if (dm->rfe_type == 1 || dm->rfe_type == 3 ||
		   dm->rfe_type == 4 || dm->rfe_type == 5) {
		panic_printk("[97F] RFE type 1,3,4 PHY paratemters: internal with TRSW\n");
		odm_cmn_info_init(dm, ODM_CMNINFO_BOARD_TYPE,
				  ODM_BOARD_EXT_TRSW);
		odm_cmn_info_init(dm, ODM_CMNINFO_EXT_TRSW, 1);
		/*select Pin usecase ID: E9*/
		odm_set_bb_reg(dm, R_0x40, 0xf000000, 0x5);
		odm_set_bb_reg(dm, R_0x4c, BIT(23), 0x0);
		odm_set_bb_reg(dm, R_0x4c, BIT(24), 0x1);
		odm_set_bb_reg(dm, R_0x4c, BIT(25), 0x0);
		odm_set_bb_reg(dm, R_0x4c, BIT(26), 0x0);
		odm_set_bb_reg(dm, R_0x4c, 0x7800000, 0x2);
		odm_set_bb_reg(dm, R_0x64, 0x30000000, 0x3);
		odm_cmn_info_init(dm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(dm, ODM_CMNINFO_EXT_PA, false);
	}

	dm->is_init_hw_info_by_rfe = true;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: RFE type (%d), Board type (0x%x), Package type (%d)\n",
		  __func__, dm->rfe_type, dm->board_type, dm->package_type);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: 5G ePA (%d), 5G eLNA (%d), 2G ePA (%d), 2G eLNA (%d)\n",
		  __func__, dm->ext_pa_5g, dm->ext_lna_5g, dm->ext_pa,
		  dm->ext_lna);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "%s: 5G PA type (%d), 5G LNA type (%d), 2G PA type (%d), 2G LNA type (%d)\n",
		  __func__, dm->type_apa, dm->type_alna, dm->type_gpa,
		  dm->type_glna);
}
#endif

__odm_func__
void phydm_sdm_reset_8733b(struct dm_struct *dm)
{
	/*reset HSSI*/
	/*phydm_rstb_3wire_8733b(dm, false);*/
	/*write RF-0x18*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xbc, BIT(19), 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xbc, BIT(19), 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xbc, BIT(19), 0x0);
	/*reset HSSI*/
	/*phydm_rstb_3wire_8733b(dm, true);*/
}

__odm_func__
void phydm_bb_reset_8733b(struct dm_struct *dm)
{
	if (*dm->mp_mode)
		return;

	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 0);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
}

__odm_func__
boolean phydm_chk_pkg_set_valid_8733b(struct dm_struct *dm,
				      u8 ver_bb, u8 ver_rf)
{
	boolean valid = true;

	if (ver_bb >= 3 && ver_rf >= 3)
		valid = true;
	else if (ver_bb < 3 && ver_rf < 3)
		valid = true;
	else
		valid = false;

	if (!valid) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0);
		pr_debug("[Warning][%s] Pkg_ver{bb, rf}={%d, %d} disable all BB block\n",
			 __func__, ver_bb, ver_rf);
	}

	return valid;
}

__odm_func__
void phydm_igi_toggle_8733b(struct dm_struct *dm)
{
	u32 igi = 0x20;

	/* @Do not use PHYDM API to read/write because FW can not access */
	igi = odm_get_bb_reg(dm, R_0x1d70, 0x7f);
	if ( (igi - 2)>0 )
		odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi - 2);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi);
}

__odm_func__
u32 phydm_check_bit_mask_8733b(u32 bit_mask, u32 data_original, u32 data)
{
	u8 bit_shift = 0;

	if (bit_mask != 0xfffff) {
		for (bit_shift = 0; bit_shift <= 19; bit_shift++) {
			if ((bit_mask >> bit_shift) & 0x1)
				break;
		}
		return (data_original & (~bit_mask)) | (data << bit_shift);
	}

	return data;
}

__odm_func__
u32 config_phydm_read_rf_reg_8733b(struct dm_struct *dm, enum rf_path path,
				      u32 reg_addr, u32 bit_mask)
{
	u32 readback_value = 0, direct_addr = 0;
	u32 offset_read_rf[2] = {0x3c00,0x4c00};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Error handling.*/
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n", path);
		return INVALID_RF_DATA;
	}

	/* @Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_read_rf[path] + (reg_addr << 2);

	/* @RF register only has 20bits */
	bit_mask &= RFREG_MASK;

	/* @Read RF register directly */
	readback_value = odm_get_bb_reg(dm, direct_addr, bit_mask);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "RF-%d 0x%x = 0x%x, bit mask = 0x%x\n", path, reg_addr,
		  readback_value, bit_mask);
	return readback_value;
}

__odm_func__
boolean
config_phydm_direct_write_rf_reg_8733b(struct dm_struct *dm,
					  enum rf_path path, u32 reg_addr,
					  u32 bit_mask, u32 data)
{
	u32 direct_addr = 0;
	u32 offset_write_rf[2] = {0x3c00,0x4c00};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_write_rf[path] + (reg_addr << 2);

	/* @RF register only has 20bits */
	bit_mask &= RFREG_MASK;

	/* @write RF register directly*/
	odm_set_bb_reg(dm, direct_addr, bit_mask, data);

	ODM_delay_us(1);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "RF-%d 0x%x = 0x%x , bit mask = 0x%x\n",
		  path, reg_addr, data, bit_mask);

	return true;
}

__odm_func__
boolean
config_phydm_write_rf_reg_8733b(struct dm_struct *dm, enum rf_path path,
				   u32 reg_addr, u32 bit_mask, u32 data)
{
	boolean result = false;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Error handling.*/
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid path=%d\n", path);
		return false;
	}
		result = config_phydm_direct_write_rf_reg_8733b(dm, path,reg_addr,
								   bit_mask,data);
		return true;
}

__odm_func__
boolean
phydm_write_txagc_1byte_8733b(struct dm_struct *dm, u32 pw_idx, u8 hw_rate)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8733B)
	u32 offset_txagc = 0x3a00;
	u8 rate_idx = (hw_rate & 0xfc);
	u8 rate_offset = (hw_rate & 0x3);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);
	/* @For debug command only!!!! */

	/* @Error handling */
	if (hw_rate > ODM_RATEMCS7) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return false;
	}
	switch (rate_offset) {
	case 0x0:
		odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKBYTE0,
			       pw_idx);
		break;
	case 0x1:
		odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKBYTE1,
			       pw_idx);
		break;
	case 0x2:
		odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKBYTE2,
			       pw_idx);
		break;
	case 0x3:
		odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKBYTE3,
			       pw_idx);
		break;
	default:
		break;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate_idx 0x%x (0x%x) = 0x%x\n",
		  hw_rate, (offset_txagc + hw_rate), pw_idx);
	return true;
#else
	return false;
#endif
}

__odm_func__
boolean
config_phydm_write_txagc_ref_8733b(struct dm_struct *dm, u8 power_index,
				      enum rf_path path,
				      enum PDM_RATE_TYPE rate_type)
{
	/*2-antenna power reference */
	u32 txagc_ref = R_0x4308;

	
	struct _hal_rf_ *rf = &dm->rf_table;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;



	if (*dm->mp_mode == 1) { //mp mode define
		if (cali_info->txpowertrack_control >= 3) //tssi on/cal
			return false;
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7)
			return false;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	/* @Error handling */
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n",
			  path);
		return false;
	}

	/* @According the rate to write in the ofdm or the cck */
	/* @CCK reference setting Bit=22:16 and Bit=6:0 */
	if (rate_type == PDM_CCK) {
		if (path == RF_PATH_A)
			odm_set_bb_reg(dm, txagc_ref, 0x0000007f, power_index);
		else
			odm_set_bb_reg(dm, txagc_ref, 0x007f0000, power_index);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "path-%d rate type %d (0x%x) = 0x%x\n",
			  path, rate_type, txagc_ref, power_index);

	/* @OFDM reference setting Bit=30:24 or Bit=14:8 */
	} else {
		if (path == RF_PATH_A)
			odm_set_bb_reg(dm, txagc_ref, 0x00007f00, power_index);
		else
			odm_set_bb_reg(dm, txagc_ref, 0x7f000000, power_index);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "path-%d rate type %d (0x%x) = 0x%x\n",
			  path, rate_type, txagc_ref, power_index);
	}

	return true;
}

__odm_func__
boolean
config_phydm_write_txagc_diff_8733b(struct dm_struct *dm, s8 power_index1,
				       s8 power_index2, s8 power_index3,
				       s8 power_index4, u8 hw_rate)
{
	u32 offset_txagc = 0x3a00;
	u8 rate_idx = hw_rate & 0xfc; /* @Extract the 0xfc */
	u8 power_idx1 = 0;
	u8 power_idx2 = 0;
	u8 power_idx3 = 0;
	u8 power_idx4 = 0;
	u32 pw_all = 0;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;



	if (*dm->mp_mode == 1) { //mp mode define
		if (cali_info->txpowertrack_control >= 3) //tssi on/cal
			return false;
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7)
			return false;
	}
	power_idx1 = power_index1 & 0x7f;
	power_idx2 = power_index2 & 0x7f;
	power_idx3 = power_index3 & 0x7f;
	power_idx4 = power_index4 & 0x7f;
	pw_all = (power_idx4 << 24) | (power_idx3 << 16) |
		(power_idx2 << 8) | power_idx1;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	/* @Error handling */
	if (hw_rate > ODM_RATEMCS7) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return false;
	}
	/* @According the rate to write in the ofdm or the cck */
	/* @driver need to construct a 4-byte power index */
	odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKDWORD, pw_all);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate index 0x%x (0x%x) = 0x%x\n",
		  hw_rate, (offset_txagc + hw_rate), pw_all);
	return true;
}

#if 1 /*Will remove when FW fill TXAGC funciton well verified*/
__odm_func__
void config_phydm_set_txagc_to_hw_8733b(struct dm_struct *dm)
{
#if (defined(CONFIG_RUN_IN_DRV))
	s8 diff_tab[2][NUM_RATE_N_1SS]; /*power diff table of 2 paths*/
	s8 diff_tab_min[NUM_RATE_N_1SS];
	u8 ref_pow_cck[2] = {dm->txagc_buff[RF_PATH_A][ODM_RATE11M],
			     dm->txagc_buff[RF_PATH_B][ODM_RATE11M]};
	u8 ref_pow_ofdm[2] = {dm->txagc_buff[RF_PATH_A][ODM_RATEMCS7],
			      dm->txagc_buff[RF_PATH_B][ODM_RATEMCS7]};
	u8 ref_pow_tmp = 0;
	enum rf_path path = 0;
	u8 i, j = 0;
		struct _hal_rf_ *rf = &dm->rf_table;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;



	if (*dm->mp_mode == 1) { //mp mode define
		if (cali_info->txpowertrack_control >= 3) //tssi on/cal
			return;
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7)
			return;
	}

	/* === [Reference base] =============================================*/
#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("ref_pow_cck={%d, %d}, ref_pow_ofdm={%d, %d}\n",
		 ref_pow_cck[0], ref_pow_cck[1], ref_pow_ofdm[0],
		 ref_pow_ofdm[1]);
#endif
	/*Set OFDM/CCK Ref. power index*/
	config_phydm_write_txagc_ref_8733b(dm, ref_pow_cck[0], RF_PATH_A,
					      PDM_CCK);
	config_phydm_write_txagc_ref_8733b(dm, ref_pow_cck[1], RF_PATH_B,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8733b(dm, ref_pow_ofdm[0], RF_PATH_A,
					      PDM_OFDM);
	config_phydm_write_txagc_ref_8733b(dm, ref_pow_ofdm[1], RF_PATH_B,
					   PDM_OFDM);

	/* === [Power By Rate] ==============================================*/
	odm_move_memory(dm, diff_tab, dm->txagc_buff, NUM_RATE_N_1SS * 2);
#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("1. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("2. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (path = RF_PATH_A; path <= RF_PATH_B; path++) {
		/*CCK*/
		ref_pow_tmp = ref_pow_cck[path];
		for (j = ODM_RATE1M; j <= ODM_RATE11M; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		/*OFDM*/
		ref_pow_tmp = ref_pow_ofdm[path];
		for (j = ODM_RATE6M; j <= ODM_RATEMCS7; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
	}

#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("3. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("4. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (i = ODM_RATE1M; i <= ODM_RATEMCS7; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8733B
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8733b(dm,
							    diff_tab_min[i - 3],
							    diff_tab_min[i - 2],
							    diff_tab_min[i - 1],
							    diff_tab_min[i],
							    i - 3);
		}
	}
#endif
}

__odm_func__
boolean config_phydm_write_txagc_8733b(struct dm_struct *dm, u32 pw_idx,
					  enum rf_path path, u8 hw_rate)
{
#if (defined(CONFIG_RUN_IN_DRV))
	u8 ref_rate = ODM_RATEMCS7;
	u8 fill_valid_cnt = 0;
	u8 i = 0;

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	if (path > RF_PATH_B) {
		pr_debug("[Warning 1] %s\n", __func__);
		return false;
	}

	if (hw_rate > ODM_RATEMCS7) {
		pr_debug("[Warning 2] %s\n", __func__);
		return false;
	}

	if (hw_rate <= ODM_RATEMCS7)
		ref_rate = ODM_RATEMCS7;

	fill_valid_cnt = ref_rate - hw_rate + 1;
	if (fill_valid_cnt > 4)
		fill_valid_cnt = 4;

	for (i = 0; i < fill_valid_cnt; i++) {
		if (hw_rate + i > NUM_RATE_N_1SS) /*Just for protection*/
			break;

		dm->txagc_buff[path][hw_rate + i] = (pw_idx >> (8 * i)) & 0xff;
	}
#endif
	return true;
}
#endif

#if 1 /*API for FW fill txagc*/
__odm_func__
void phydm_set_txagc_by_table_8733b(struct dm_struct *dm,
				       struct txagc_table_8733b *tab)
{
	u8 i = 0;

	/* === [Reference base] =============================================*/
	/*Set OFDM/CCK Ref. power index*/
	config_phydm_write_txagc_ref_8733b(dm, tab->ref_pow_cck[0], RF_PATH_A,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8733b(dm, tab->ref_pow_cck[1], RF_PATH_B,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8733b(dm, tab->ref_pow_ofdm[0], RF_PATH_A,
					   PDM_OFDM);
	config_phydm_write_txagc_ref_8733b(dm, tab->ref_pow_ofdm[1], RF_PATH_B,
					   PDM_OFDM);

	for (i = ODM_RATE1M; i <= ODM_RATEMCS7; i++) {
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8733b(dm,
							       tab->diff_t[i -3],
							       tab->diff_t[i -2],
							       tab->diff_t[i -1],
							       tab->diff_t[i],
							       i - 3);
		}
	}
}

__odm_func__
void phydm_get_txagc_ref_and_diff_8733b(struct dm_struct *dm,
					   u8 txagc_buff[2][NUM_RATE_N_1SS],
					   u16 length,
					   struct txagc_table_8733b *tab)
{
	s8 diff_tab[2][NUM_RATE_N_1SS]; /*power diff table of 2 paths*/
	s8 diff_tab_min[NUM_RATE_N_1SS];
	u8 ref_pow_cck[2];
	u8 ref_pow_ofdm[2];
	u8 ref_pow_tmp = 0;
	enum rf_path path = 0;
	u8 i, j = 0;

	if (length != NUM_RATE_N_1SS) {
		pr_debug("[warning] %s\n", __func__);
		return;
	}

	/* === [Reference base] =============================================*/
#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("ref_pow_cck={%d, %d}, ref_pow_ofdm={%d, %d}\n",
		 ref_pow_cck[0], ref_pow_cck[1], ref_pow_ofdm[0],
		 ref_pow_ofdm[1]);
#endif

	/* === [Power By Rate] ==============================================*/
	odm_move_memory(dm, diff_tab, txagc_buff, NUM_RATE_N_1SS * 2);

	ref_pow_cck[0] = diff_tab[RF_PATH_A][ODM_RATE11M];
	ref_pow_cck[1] = diff_tab[RF_PATH_B][ODM_RATE11M];

	ref_pow_ofdm[0] = diff_tab[RF_PATH_A][ODM_RATEMCS7];
	ref_pow_ofdm[1] = diff_tab[RF_PATH_B][ODM_RATEMCS7];

#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("1. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("2. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (path = RF_PATH_A; path <= RF_PATH_B; path++) {
		/*CCK*/
		ref_pow_tmp = ref_pow_cck[path];
		for (j = ODM_RATE1M; j <= ODM_RATE11M; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		/*OFDM*/
		ref_pow_tmp = ref_pow_ofdm[path];
		for (j = ODM_RATE6M; j <= ODM_RATEMCS7; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
	}

#ifdef CONFIG_TXAGC_DEBUG_8733B
	pr_debug("3. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("4. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEMCS7; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (i = ODM_RATE1M; i <= ODM_RATEMCS7; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8733B
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
	}

	odm_move_memory(dm, tab->ref_pow_cck, ref_pow_cck, 2);
	odm_move_memory(dm, tab->ref_pow_ofdm, ref_pow_ofdm, 2);
	odm_move_memory(dm, tab->diff_t, diff_tab_min, NUM_RATE_N_1SS);
}
#endif

__odm_func__
s8 config_phydm_read_txagc_diff_8733b(struct dm_struct *dm, u8 hw_rate)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8733B)
	s8 read_txagc = 0;
	u32 offset_txagc = R_0x3a00;
	u8 rate_idx = (hw_rate & 0xfc);
	u8 rate_offset = (hw_rate & 0x3);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */

	/* @Error handling */
	if (hw_rate > ODM_RATEMCS7) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return INVALID_TXAGC_DATA;
	}
	switch (rate_offset) {
	case 0x0:
		read_txagc = (s8)odm_get_bb_reg(dm, (offset_txagc + rate_idx),
						MASKBYTE0);
		break;
	case 0x1:
		read_txagc = (s8)odm_get_bb_reg(dm, (offset_txagc + rate_idx),
						MASKBYTE1);
		break;
	case 0x2:
		read_txagc = (s8)odm_get_bb_reg(dm, (offset_txagc + rate_idx),
						MASKBYTE2);
		break;
	case 0x3:
		read_txagc = (s8)odm_get_bb_reg(dm, (offset_txagc + rate_idx),
						MASKBYTE3);
		break;
	default:
		break;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate index 0x%x = 0x%x\n", hw_rate,
		  read_txagc);
	return read_txagc;
#else
	return 0;
#endif
}

__odm_func__
u8 config_phydm_read_txagc_8733b(struct dm_struct *dm, enum rf_path path,
				    u8 hw_rate, enum PDM_RATE_TYPE rate_type)
{
	s8 read_back_data = 0;
	u8 ref_data = 0;
	u8 result_data = 0;
	/* @2-antenna power reference */
	u32 r_txagc = R_0x4308;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */

	/* @Error handling */
	if (path > RF_PATH_B || hw_rate > ODM_RATEMCS7) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n", path);
		return INVALID_TXAGC_DATA;
	}
	
	read_back_data = (s8)config_phydm_read_txagc_diff_8733b(dm, hw_rate);

	/* @Read power reference value report */
	if (rate_type == PDM_CCK) {
		/* @Bit=22:16 or Bit=6:0 */
		if (path == RF_PATH_A)
			ref_data = (u8)odm_get_bb_reg(dm, r_txagc, 0x0000007f);
		else
			ref_data = (u8)odm_get_bb_reg(dm, r_txagc, 0x007F0000);
	} else if (rate_type == PDM_OFDM) {
		/* @Bit=30:24 or Bit=14:8 */
		if (path == RF_PATH_A)
			ref_data = (u8)odm_get_bb_reg(dm, r_txagc, 0x00007f00);
		else
			ref_data = (u8)odm_get_bb_reg(dm, r_txagc, 0x7F000000);
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "diff=%d ref=%d\n", read_back_data,
		  ref_data);

	if (read_back_data + ref_data < 0)
		result_data = 0;
	else
		result_data = read_back_data + ref_data;
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "path-%d rate index 0x%x = 0x%x\n",
		  path, hw_rate, result_data);
	return result_data;
}

__odm_func__
boolean
config_phydm_trx_mode_8733b(struct dm_struct *dm, enum bb_path tx_path_en,
			       enum bb_path rx_path,
			       enum bb_path tx_path_sel_1ss)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		pr_debug("[%s] Disable PHY API\n", __func__);
		return true;
	}

	if ((tx_path_en & ~BB_PATH_AB) || (rx_path & ~BB_PATH_AB)) {
		pr_debug("[Warning][%s] T/RX:0x%x/0x%x\n", __func__, tx_path_en,
			 rx_path);
		return false;
	}

	if (tx_path_en != rx_path) {
		pr_debug("[%s] T/RX are not the same!\n", __func__);
		return false;
	}

	/* @[mode table] RF mode of path-A and path-B =======================*/
	/*
	 * @Cannot shutdown path-A, beacause synthesizer will be shutdown
	 * @when path-A is in shut down mode
	 */
	/* @3-wire setting */
	/* @0: shutdown, 1: standby, 2: TX, 3: RX */
	/* @RF mode setting*/
	odm_set_bb_reg(dm, R_0x1800, MASK20BITS, 0x33311);

	/* Switch WL_B_SEL_BTG_opt to use sw control s0/s1 */
	if (odm_get_bb_reg(dm, R_0x1884, BIT(21)))
		odm_set_bb_reg(dm, R_0x1884, BIT(21), 0x0);

	if (tx_path_en == BB_PATH_A && rx_path == BB_PATH_A)
		odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x0);
	else
		odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x1);

	odm_set_bb_reg(dm, R_0x1800, MASK20BITS, 0x33312);

	tx_path_sel_1ss = tx_path_en;
	dm->tx_ant_status = tx_path_en;
	dm->tx_1ss_status = tx_path_sel_1ss;

	/*
	 * @Toggle igi to let RF enter RX mode,
	 * @because BB doesn't send 3-wire command
	 * @when RX path is enable
	 */

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "T/RX_en:0x%x/0x%x, tx_1ss:%x\n",
		  tx_path_en, rx_path, tx_path_sel_1ss);

	phydm_bb_reset_8733b(dm);
	phydm_igi_toggle_8733b(dm);

	return true;
}

void phydm_dis_cck_trx_8733b(struct dm_struct *dm, u8 set_type)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "CCK TRX Setting\n");

	if (set_type == PHYDM_SET) {
		/* @ Enable CCK CCA*/
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x0);
		/* Enable BB CCK Tx */
		odm_set_bb_reg(dm, R_0x2a00, BIT(1), 0x0);
	} else if (set_type == PHYDM_REVERT) {
		/* @ Disable CCK CCA*/
		odm_set_bb_reg(dm, R_0x2a24, BIT(13), 0x1);
		/* Disable BB CCK Tx */
		odm_set_bb_reg(dm, R_0x2a00, BIT(1), 0x1);
	}
	phydm_bb_reset_8733b(dm);
}
__odm_func__
void
phydm_cck_agc_tab_sel_8733b(struct dm_struct *dm, u8 table)
{
	odm_set_bb_reg(dm, R_0x18ac, 0xf000, table);
}
__odm_func__
void
phydm_ofdm_agc_tab_sel_8733b(struct dm_struct *dm, u8 table)
{
	struct phydm_dig_struct *dig_tab = &dm->dm_dig_table;
	odm_set_bb_reg(dm, R_0x18ac, 0x1f0, table);
	dig_tab->agc_table_idx = table;
	/*AGC lower bound, need to be updated with AGC table*/
	//odm_set_bb_reg(dm, R_0x828, 0xf8, lower_bound);
}

__odm_func__
boolean
config_phydm_switch_band_8733b(struct dm_struct *dm, u8 central_ch)
{
	u32 rf_reg18 = 0;
	u8 i = 0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for dbg\n");
		return true;
	}

	rf_reg18 = config_phydm_read_rf_reg_8733b(dm, RF_PATH_A, 0x18,
						     RFREG_MASK);
	if (rf_reg18 == INVALID_RF_DATA) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid RF_0x18\n");
		return false;
	}
	if (central_ch <= 14) {
		/* @2.4G */

		/* @Enable CCK TRx */
		phydm_dis_cck_trx_8733b(dm, PHYDM_SET);
		
		/* Enable CCK block */
		//odm_set_bb_reg(dm, R_0x1c3c, BIT(1), 0x1);
		
		/* @Disable MAC CCK check */
		odm_set_mac_reg(dm, R_0x454, BIT(7), 0x0);

		/* @CCA Mask, default = 0xf */
		odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0xF);
		/* RF band */
		rf_reg18 = (rf_reg18 & (~(BIT(17) | BIT(16) |
			    BIT(9) | BIT(8) | MASKBYTE0)));
		rf_reg18 = (rf_reg18 | central_ch);

		/* Switch WLG/BTG*/
		if ( dm->rfe_type == 2 || dm->rfe_type == 4 || dm->rfe_type == 9 ) {
			/*RF S1*/
			odm_set_bb_reg(dm, R_0x1884, BIT(21), 0x0);
			odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x1);
		} else {
			/*RF S0*/
			odm_set_bb_reg(dm, R_0x1884, BIT(21), 0x0);
			odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x0);
		}

		/* for BTTX*/
		odm_set_bb_reg(dm, R_0x1968, BIT(0), 0x1);
		odm_set_bb_reg(dm, R_0x1968, BIT(8), 0x1);
		odm_set_bb_reg(dm, R_0x1968, BIT(14), 0x1);
	} else if (central_ch > 35) {
		/* 5G */
		/* @Enable CCK TRx */
		phydm_dis_cck_trx_8733b(dm, PHYDM_REVERT);
		
		/* @Disable CCK block*/
		//odm_set_bb_reg(dm, R_0x1c3c, BIT(1), 0x0);

		/* @Enable MAC CCK check */
		odm_set_mac_reg(dm, R_0x454, BIT(7), 0x1);

		/* @CCA Mask, default = 0xf */
		odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0xF);

		/* RF band and channel */
		rf_reg18 = (rf_reg18 & (~(BIT(17) | BIT(9) | MASKBYTE0)));
		rf_reg18 = (rf_reg18 | BIT(16) | BIT(8) | central_ch);

		/* Switch WLA */
		/*RF S0*/
		odm_set_bb_reg(dm, R_0x1884, BIT(21), 0x0);
		odm_set_bb_reg(dm, R_0x1884, BIT(20), 0x0);
		if ( dm->rfe_type == 2 || dm->rfe_type == 3 ) {
			PHYDM_DBG(dm, ODM_PHY_CONFIG, "rfe_type(: %d) not support 5G\n",
			  	dm->rfe_type);
			return false;
		}

		/* for BTTX*/
		odm_set_bb_reg(dm, R_0x1968, BIT(0), 0x0);
		odm_set_bb_reg(dm, R_0x1968, BIT(8), 0x0);
		odm_set_bb_reg(dm, R_0x1968, BIT(14), 0x0);
	} else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Fail to switch band (ch: %d)\n",
			  central_ch);
		return false;
	}
	if (dm->cut_version < ODM_CUT_D) {
		for (i = 0; i < 20; i++) {
			/*write RF-0x18*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, 0xA0, 0x4, 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x0);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	} else {
		for (i = 0; i < 20; i++) {
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	}
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "Success to switch band (ch: %d)\n",
		  central_ch);

	phydm_bb_reset_8733b(dm);
	return true;
}
__odm_func__
void
phydm_cck_tx_shaping_filter_8733b(struct dm_struct *dm, u8 central_ch)
{
	if (central_ch == 14) {	
		/*TX Shaping Filter C0~1 */
		odm_set_bb_reg(dm, R_0x1a00, MASKL3BYTES, 0x452484);
		/*TX Shaping Filter C2~5 */
		odm_set_bb_reg(dm, R_0x1a04, MASKL3BYTES, 0x0fe3c8);
		odm_set_bb_reg(dm, R_0x1a08, MASKL3BYTES, 0x000000);
		/*TX Shaping Filter C6~7 */
		odm_set_bb_reg(dm, R_0x1a0c, MASKL3BYTES, 0x000000);
		/*TX Shaping Filter C8~9 */
		odm_set_bb_reg(dm, R_0x1a10, MASKL3BYTES, 0x000000);
		/*TX Shaping Filter C10~11 */
		odm_set_bb_reg(dm, R_0x1a14, MASKL3BYTES, 0x000000);
		/*TX Shaping Filter C12~15 */
		odm_set_bb_reg(dm, R_0x1a18, MASKL3BYTES, 0x000000);
		odm_set_bb_reg(dm, R_0x1a1c, MASKL3BYTES, 0x000000);
	} else {
		/*TX Shaping Filter C0~1 */
		odm_set_bb_reg(dm, R_0x1a00, MASKL3BYTES, 0x7847CF);
		/*TX Shaping Filter C2~5 */
		odm_set_bb_reg(dm, R_0x1a04, MASKL3BYTES, 0x57A6B1);
		odm_set_bb_reg(dm, R_0x1a08, MASKDWORD, 0x1F2AF412);
		/*TX Shaping Filter C6~7 */
		odm_set_bb_reg(dm, R_0x1a0c, MASKL3BYTES, 0x09717D);
		/*TX Shaping Filter C8~9 */
		odm_set_bb_reg(dm, R_0x1a10, MASKL3BYTES, 0xFB9003);
		/*TX Shaping Filter C10~11 */
		odm_set_bb_reg(dm, R_0x1a14, MASKL3BYTES, 0xFB1FA5);
		/*TX Shaping Filter C12~15 */
		odm_set_bb_reg(dm, R_0x1a18, MASKL3BYTES, 0xFE2FCA);
		odm_set_bb_reg(dm, R_0x1a1c, MASKL3BYTES, 0xFFCFF3);
	}
}
__odm_func__
void
phydm_sco_trk_fc_setting_8733b(struct dm_struct *dm, u8 central_ch)
{
	u32 reg_2a38[15] = {0x0, 0x1cfea, 0x1d0e1, 0x1d1d7, 0x1d2cd, 0x1d3c3,
			    0x1d4b9, 0x1d5b0, 0x1d6a6, 0x1d79c, 0x1d892,
			    0x1d988, 0x1da7f, 0x1db75, 0x1ddc4};
	u32 reg_2a3c[15] = {0x0, 0x27de3, 0x27f35, 0x28088, 0x281da, 0x2832d,
			    0x2847f, 0x285d2, 0x28724, 0x28877, 0x289c9,
			    0x28b1c, 0x28c6e, 0x28dc1, 0x290ed};
	if (central_ch <= 14) {
		/* 0x2a38[27:8] barker, 0x2a3c[19:0] cck */
		odm_set_bb_reg(dm, R_0x2a38, BIT(27), 0x0); //1:for FGPA
		odm_set_bb_reg(dm, R_0x2a38, 0x07ffff00, reg_2a38[central_ch]);
		odm_set_bb_reg(dm, R_0x2a3c, 0xfffff, reg_2a3c[central_ch]);
	}
	if (central_ch == 13 || central_ch == 14) {
		/* @n:41, s:37 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x969);
	} else if (central_ch == 11 || central_ch == 12) {
		/* @n:42, s:37 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x96a);
	} else if (central_ch >= 1 && central_ch <= 10) {
		/* @n:42, s:38 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x9aa);
	} else if (central_ch >= 36 && central_ch <= 51) {
		/* @n:20, s:18 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x494);
	} else if (central_ch >= 52 && central_ch <= 55) {
		/* @n:19, s:18 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x493);
	} else if ((central_ch >= 56) && (central_ch <= 111)) {
		/* @n:19, s:17 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x453);
	} else if ((central_ch >= 112) && (central_ch <= 119)) {
		/* @n:18, s:17 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x452);
	} else if ((central_ch >= 120) && (central_ch <= 172)) {
		/* @n:18, s:16 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x412);
	} else { /* if ((central_ch >= 173) && (central_ch <= 177)) */
		/* n:17, s:16 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x411);
	}
}

__odm_func__
void
phydm_tx_dfir_setting_8733b(struct dm_struct *dm, u8 central_ch)
{
	/* TX band edge improvement */
	if (central_ch <= 14) {
		if (central_ch == 13)
			odm_set_bb_reg(dm, R_0x808, 0x7f, 0x30);
		else
			odm_set_bb_reg(dm, R_0x808, 0x7f, 0x40);
	} else {
		odm_set_bb_reg(dm, R_0x808, 0x7f, 0x40);
	}
}

__odm_func__
boolean
config_phydm_switch_channel_8733b(struct dm_struct *dm, u8 central_ch)
{
	u32 rf_reg18 = 0;
	u32 rf_reg19 = 0;
	boolean is_2g_ch = true;
	u8 i = 0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API\n");
		return true;
	}

	if ((central_ch > 14 && central_ch < 36) ||
	    (central_ch > 64 && central_ch < 100) ||
	    (central_ch > 144 && central_ch < 149) ||
	    central_ch > 177) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Error CH:%d\n", central_ch);
		return false;
	}

	/* @RF register setting */
	rf_reg18 = config_phydm_read_rf_reg_8733b(dm, RF_PATH_A, RF_0x18,
						     RFREG_MASK);
	rf_reg19 = config_phydm_read_rf_reg_8733b(dm, RF_PATH_A, RF_0x19,
						     RFREG_MASK);
							 
	if (rf_reg18 == INVALID_RF_DATA) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid RF_0x18\n");
		return false;
	}

	if (rf_reg19 == INVALID_RF_DATA) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid RF_0x19\n");
		return false;
	}
	
	is_2g_ch = (central_ch <= 14) ? true : false;

	/* @Switch band and channel */
	if (is_2g_ch) {
		/* @2.4G */

		/* @1. RF band and channel*/
		rf_reg18 = (rf_reg18 & (~(BIT(17) | BIT(16) |
			    BIT(9) | BIT(8) | MASKBYTE0)));
		rf_reg18 |= central_ch;
		
	} else {
		/* 5G */

		/* @1. RF band and channel*/
		rf_reg18 = (rf_reg18 & (~(BIT(17) | BIT(9) | MASKBYTE0)));
		rf_reg18 = (rf_reg18 | BIT(16) | BIT(8) | central_ch);
		rf_reg19 = rf_reg19 &  (~(BIT(19) | BIT(18)));
		/* 5G Sub-Band, 01: 5400<f<=5720, 10: f>5720*/
		if (central_ch > 144)
			rf_reg19 |= BIT(19);
		else if (central_ch > 80)
			rf_reg19 |= BIT(18);
		}
	if (dm->cut_version < ODM_CUT_D) {
		for (i = 0; i < 20; i++) {
			/*write RF-0x18*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, 0xA0, 0x4, 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x0);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	} else {
		for (i = 0; i < 20; i++) {
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	}
	/*write RF-0x19*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x19, RFREG_MASK, rf_reg19);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x19, RFREG_MASK, rf_reg19);
	/* @2. AGC table selection */
	if (central_ch <= 14) {
		/* RX idle agc table 12 */
		odm_set_bb_reg(dm, R_0x1ea8, BIT(7), 0x1);
		/* RX cck agc table 5*/
		phydm_cck_agc_tab_sel_8733b(dm, CCK_BW20_40_8733B);
		if (*dm->band_width == CHANNEL_WIDTH_20) {
		/* ofdm 20M agc table 4 */
			phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_2G_BW20_40_8733B);
		} else {
		/* ofdm 40M agc table 4 */
			phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_2G_BW20_40_8733B);
		}
	} else if (central_ch >= 36 && central_ch <= 64) {
		/* 5G RX idle agc table = low band table = 1 */
		odm_set_bb_reg(dm, R_0x1ea8, BIT(7), 0x0);
		phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_5G_LOW_BAND_8733B);
	} else if ((central_ch >= 100) && (central_ch <= 144)) {
		/* 5G RX idle agc table = low band table = 2 */
		odm_set_bb_reg(dm, R_0x1ea8, BIT(7), 0x0);
		phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_5G_MID_BAND_8733B);
	} else if (central_ch >= 149) {
		/* 5G RX idle agc table = low band table = 3 */
		odm_set_bb_reg(dm, R_0x1ea8, BIT(7), 0x0);
		phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_5G_HIGH_BAND_8733B);
	}

		/* @3. Set central frequency for clock offset tracking */
	phydm_sco_trk_fc_setting_8733b(dm, central_ch);
	/* 4. Other 2.4G tx_shaping_fliter Settings*/
		if (is_2g_ch)
		phydm_cck_tx_shaping_filter_8733b(dm, central_ch);
	/* 5. TX DFIR*/
	phydm_tx_dfir_setting_8733b(dm, central_ch);
	phydm_bb_reset_8733b(dm);
	phydm_igi_toggle_8733b(dm);
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "Success to switch channel : %d\n",central_ch);
	return true;
}


__odm_func__
boolean
config_phydm_switch_bandwidth_8733b(struct dm_struct *dm, u8 pri_ch,
				       enum channel_width bw)
{
	struct phydm_dig_struct *dig_tab = &dm->dm_dig_table;
	u32 rf_reg18 = 0;
	boolean rf_reg_status = true;
	u8 i = 0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug!!\n");
		return true;
	}

	/* @Error handling */
	if (bw >= CHANNEL_WIDTH_MAX || (bw == CHANNEL_WIDTH_20 && pri_ch > 1) ||
	    (bw == CHANNEL_WIDTH_40 && pri_ch > 2)) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw(bw:%d, pri ch:%d)\n", bw, pri_ch);
		return false;
	}
	
		rf_reg18 = config_phydm_read_rf_reg_8733b(dm, RF_PATH_A, RF_0x18,
						     RFREG_MASK);
	if (rf_reg18 != INVALID_RF_DATA)
		rf_reg_status = true;
	else
		rf_reg_status = false;

	rf_reg18 &= ~(BIT(11) | BIT(10));

	/* @Switch bandwidth */
	switch (bw) {
	case CHANNEL_WIDTH_5:
	case CHANNEL_WIDTH_10:
	case CHANNEL_WIDTH_20:
		if (bw == CHANNEL_WIDTH_5) {
			/* @RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x19b);

			/* @small BW:[7:6]=0x1 */
			/* @TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x1);

			/* @DAC clock = 40M clock for BW5 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x1);

			/* @ADC clock = 40M clock for BW5 */
			odm_set_bb_reg(dm, R_0x9f0, 0xf, 0xa);

			/* TX BPSK/QPSK BandEdge configure */
			odm_set_bb_reg(dm, R_0x81c, 0xf, 0x0);
		} else if (bw == CHANNEL_WIDTH_10) {
			/* @RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x19b);

			/* @small BW:[7:6]=0x2 */
			/* @TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x2);

			/* @DAC clock = 80M clock for BW10 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x2);

			/* @ADC clock = 80M clock for BW10 */
			odm_set_bb_reg(dm, R_0x9f0, 0xf, 0xb);

			/* TX BPSK/QPSK BandEdge configure */
			odm_set_bb_reg(dm, R_0x81c, 0xf, 0x0);
		} else if (bw == CHANNEL_WIDTH_20) {
			/* @RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x19b);

			/* @small BW:[7:6]=0x0 */
			/* @TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x0);

			/* @DAC clock = 160M clock for BW20 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x3);

			/* @ADC clock = 160M clock for BW20 */
			odm_set_bb_reg(dm, R_0x9f0, 0xf, 0xc);

			/* TX BPSK/QPSK BandEdge configure */
			odm_set_bb_reg(dm, R_0x81c, 0xf, 0xa);
		}

		/* @TX_RF_BW:[1:0]=0x0, RX_RF_BW:[3:2]=0x0 */
		odm_set_bb_reg(dm, R_0x9b0, 0xf, 0x0);

		/* @RF bandwidth */
		rf_reg18 |= (BIT(11) | BIT(10));

		/* @RF RXBB setting*/

		/* @pilot smoothing on */
		odm_set_bb_reg(dm, R_0xcbc, BIT(21), 0x0);

		if (*dm->band_type == ODM_BAND_2_4G) {
			/* @CCK*/
			phydm_cck_agc_tab_sel_8733b(dm, CCK_BW20_40_8733B);
			/* @OFDM*/
			phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_2G_BW20_40_8733B);
		}

		break;
	case CHANNEL_WIDTH_40:
		#if 0
		/*CCK primary channel */
		if (pri_ch == 1)
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), pri_ch);
		else
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), 0);
		#endif

		/* @TX_RF_BW:[1:0]=0x1, RX_RF_BW:[3:2]=0x1 */
		odm_set_bb_reg(dm, R_0x9b0, 0xf, 0x5);

		/* @small BW */
		odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);

		/* @TX pri ch:[11:8], RX pri ch:[15:12] */
		odm_set_bb_reg(dm, R_0x9b0, 0xff00, (pri_ch | (pri_ch << 4)));

		/* @RF bandwidth */
		rf_reg18 |= BIT(11);

		/* @pilot smoothing off */
		odm_set_bb_reg(dm, R_0xcbc, BIT(21), 0x1);

		if (*dm->band_type == ODM_BAND_2_4G) {
			/*CCK*/
			phydm_cck_agc_tab_sel_8733b(dm, CCK_BW20_40_8733B);
			/* @OFDM*/
			phydm_ofdm_agc_tab_sel_8733b(dm, OFDM_2G_BW20_40_8733B);
		}

		/* TX band edge improvement by Anchi */
		odm_set_bb_reg(dm, R_0x808, 0x000f0000, 0xd);
		/* TX BPSK/QPSK BandEdge configure */
		odm_set_bb_reg(dm, R_0x81c, 0xf, 0xa);

		break;
	default:
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw (bw:%d, pri ch:%d)\n", bw, pri_ch);
	}

	/* @Write RF register */
	#if 0
	/* RF RXBB setting, modify 0x3f for WLANBB-1081*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xee, 0x4, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, 0x1F, 0x12);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x3f, RFREG_MASK, 0x10);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xee, 0x4, 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xee, 0x4, 0x1);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x33, 0x1F, 0x12);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x3f, RFREG_MASK, 0x10);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xee, 0x4, 0x0);
	#endif
	if (dm->cut_version < ODM_CUT_D) {
		for (i = 0; i < 20; i++) {
			/*write RF-0x18*/
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x1);
			odm_set_rf_reg(dm, RF_PATH_A, 0xA0, 0x4, 0x0);
			odm_set_rf_reg(dm, RF_PATH_A, RF_0xdd, 0x10, 0x0);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	} else {
		for (i = 0; i < 20; i++) {
			odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
			odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
			ODM_delay_us(250);
			if(odm_get_rf_reg(dm, RF_PATH_A, 0xc5, 0x8000))
				break;
		}
	}
	if (!rf_reg_status) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw (bw:%d, primary ch:%d), because writing RF register is fail\n",
			  bw, pri_ch);
		return false;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "Success to switch bw (bw:%d, pri ch:%d)\n", bw, pri_ch);

	phydm_bb_reset_8733b(dm);

	phydm_igi_toggle_8733b(dm);
	return true;
}

__odm_func__
boolean
config_phydm_switch_channel_bw_8733b(struct dm_struct *dm, u8 central_ch,
					u8 primary_ch_idx,
					enum channel_width bandwidth)
{
	/* @Switch band */
	if (!config_phydm_switch_band_8733b(dm, central_ch))
		return false;

	/* @Switch channel */
	if (!config_phydm_switch_channel_8733b(dm, central_ch))
		return false;

	/* @Switch bandwidth */
	if (!config_phydm_switch_bandwidth_8733b(dm, primary_ch_idx,bandwidth))
		return false;

	return true;
}

__odm_func__
void phydm_i_only_setting_8733b(struct dm_struct *dm, boolean en_i_only,
				   boolean en_before_cca)
{
	if (en_i_only) { /*@ Set path-a*/
		if (en_before_cca) {
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x833);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x2);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038001);
		} else {
			if (!(*dm->band_width == CHANNEL_WIDTH_40))
				return;

			dm->bp_0x9b0 = odm_get_bb_reg(dm, R_0x9b0, MASKDWORD);
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x888);
			odm_set_bb_reg(dm, R_0x898, BIT(30), 0x1);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x1);
			odm_set_bb_reg(dm, R_0x9b0, MASKDWORD, 0x2200);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70538041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70738041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70838041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70938041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70a38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70b38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70c38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70d38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70e38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70f38041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70f38041);
		}
	} else {
		if (en_before_cca) {
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x333);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x0);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8001);
		} else {
			if (!(*dm->band_width == CHANNEL_WIDTH_40))
				return;

			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x333);
			odm_set_bb_reg(dm, R_0x898, BIT(30), 0x0);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x0);
			odm_set_bb_reg(dm, R_0x9b0, MASKDWORD, dm->bp_0x9b0);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
		}
	}
}

__odm_func__
void phydm_cck_gi_bound_8733b(struct dm_struct *dm)
{
	struct phydm_physts *physts_table = &dm->dm_physts_table;
	u8 cck_gi_u_bnd = 0;
	u8 cck_gi_l_bnd = 0;

	cck_gi_u_bnd = (u8)odm_get_bb_reg(dm, R_0x1a38, 0x3f80);
	cck_gi_l_bnd = (u8)odm_get_bb_reg(dm, R_0x1a38, 0x1fc000);

	physts_table->cck_gi_u_bnd = cck_gi_u_bnd;
	physts_table->cck_gi_l_bnd = cck_gi_l_bnd;
}

__odm_func__
boolean
config_phydm_parameter_init_8733b(struct dm_struct *dm,
				     enum odm_parameter_init type)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	phydm_cck_gi_bound_8733b(dm);

	/* @Do not use PHYDM API to read/write because FW can not access */
	/* @Turn on 3-wire*/
	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x3);
	odm_set_bb_reg(dm, R_0x180c, BIT(28), 0x1);

	/* Read phystatus for MP tool */
	if (*dm->mp_mode) {
		odm_set_bb_reg(dm, R_0x8c0, 0x3ff0, 0x0);
		odm_set_bb_reg(dm, R_0x8c4, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1c90, BIT(0), 0x1);
	}

	if (type == ODM_PRE_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Pre setting: disable OFDM and CCK block\n");
	} else if (type == ODM_POST_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x3);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Post setting: enable OFDM and CCK block\n");
#if 0			  
#if (PHYDM_FW_API_FUNC_ENABLE_8733B)
	} else if (type == ODM_INIT_FW_SETTING) {
		u8 h2c_content[4] = {0};

		h2c_content[0] = dm->rfe_type;
		h2c_content[1] = dm->rf_type;
		h2c_content[2] = dm->cut_version;
		h2c_content[3] = (dm->tx_ant_status << 4) | dm->rx_ant_status;

		odm_fill_h2c_cmd(dm, PHYDM_H2C_FW_GENERAL_INIT, 4, h2c_content);
#endif
#endif
	} else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Wrong type!!\n");
		return false;
	}

	phydm_bb_reset_8733b(dm);
	#ifdef CONFIG_TXAGC_DEBUG_8733B
	/*phydm_txagc_tab_buff_init_8733b(dm);*/
	#endif

	return true;
}

__odm_func__
boolean
phydm_chk_bb_state_idle_8733b(struct dm_struct *dm)
{
	u32 dbgport = 0;

	/* Do not check GNT_WL for LPS */
	odm_set_bb_reg(dm, R_0x1c3c, 0x00f00000, 0x0);
	dbgport = odm_get_bb_reg(dm, R_0x2db4, MASKDWORD);
	if ((dbgport & 0x1ffeff3f) == 0 &&
	    (dbgport & 0xc0000000) == 0xc0000000)
		return true;
	else
		return false;
}

#if CONFIG_POWERSAVING
__odm_func_aon__
boolean
phydm_8733b_lps(struct dm_struct *dm, boolean enable_lps)
{
	u16 poll_cnt = 0;

	if (enable_lps == _TRUE) {
		/* turn off direct ctrl*/
		config_phydm_write_rf_reg_8733b(dm, RF_PATH_A, RF_0x5, BIT(0), 0x0);
		
		/* backup RF reg0x0 */
		SysMib.Wlan.PS.PSParm.RxGainPathA = (u16)(config_phydm_read_rf_reg_8733b(dm, RF_PATH_A, RF_0x0, RFREG_MASK));

		/* Set RF enter shutdown mode */
		config_phydm_write_rf_reg_8733b(dm, RF_PATH_A, RF_0x0,RFREG_MASK, 0x0);
		
		/* Check BB state is idle, do not check GNT_WL only for LPS */
		while (1) {
			if (phydm_chk_bb_state_idle_8733b(dm))
				break;

			if (poll_cnt > WAIT_TXSM_STABLE_CNT) {
				WriteMACRegDWord(REG_DBG_DW_FW_ERR, ReadMACRegDWord(REG_DBG_DW_FW_ERR) | FES_BBSTATE_IDLE);
			/* SysMib.Wlan.DbgPort.DbgInfoParm.u4ErrFlag[0] |= FES_BBSTATE_IDLE; */
				return _FALSE;
			}

			DelayUS(WAIT_TXSM_STABLE_ONCE_TIME);
			poll_cnt++;
		}

		/*When BB reset = 0, enter shutdown mode*/
		odm_set_bb_reg(dm, R_0x1c64, BIT(3), 0x0);
		/* disable CCK and OFDM module */
		WriteMACRegByte(REG_SYS_FUNC_EN, ReadMACRegByte(REG_SYS_FUNC_EN)
				& ~BIT_FEN_BBRSTB);

		if (poll_cnt < WAIT_TXSM_STABLE_CNT) {
			/* Gated BBclk 0x1c24[0] = 1 */
		odm_set_bb_reg(dm, R_0x1c24, BIT(0), 0x1);
		}

		return _TRUE;
	} else {
		/* release BB clk 0x1c24[0] = 0 */
		odm_set_bb_reg(dm, R_0x1c24, BIT(0), 0x0);

		/* Enable CCK and OFDM module, */
		/* should be a delay large than 200ns before RF access */
		WriteMACRegByte(REG_SYS_FUNC_EN, ReadMACRegByte(REG_SYS_FUNC_EN)
				| BIT_FEN_BBRSTB);
		DelayUS(1);

		/*When BB reset = 0, enter standby mode*/
		odm_set_bb_reg(dm, R_0x1c64, BIT(3), 0x1);
		/* Set RF enter active mode */
		config_phydm_write_rf_reg_8733b(dm, RF_PATH_A,R_0x0, RFREG_MASK,(0x30000 | SysMib.Wlan.PS.PSParm.RxGainPathA));

		/* turn on direct ctrl*/
		config_phydm_write_rf_reg_8733b(dm, RF_PATH_A, RF_0x5, BIT(0), 0x1);

		/*sdm reset for rf shutdown mode spur issue*/
		phydm_sdm_reset_8733b(dm);

		return _TRUE;
	}
}
#endif /* #if CONFIG_POWERSAVING */

/* ======================================================================== */
#endif /* PHYDM_FW_API_ENABLE_8733B */
#endif /* RTL8733B_SUPPORT */
