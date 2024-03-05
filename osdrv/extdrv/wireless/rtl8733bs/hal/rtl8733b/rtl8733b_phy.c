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
#define _RTL8733B_PHY_C_

#include <hal_data.h>		/* HAL_DATA_TYPE */
#include "../hal_halmac.h"	/* rtw_halmac_phy_power_switch() */
#include "rtl8733b.h"


/*
 * Description:
 *	Initialize Register definition offset for Radio Path A/B/C/D
 *	The initialization value is constant and it should never be changes
 */
static void bb_rf_register_definition(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);


	/* RF Interface Sowrtware Control */
	hal->PHYRegDef[RF_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW;
	hal->PHYRegDef[RF_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW;

	/* RF Interface Output (and Enable) */
	hal->PHYRegDef[RF_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE;
	hal->PHYRegDef[RF_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE;

	/* RF Interface (Output and) Enable */
	hal->PHYRegDef[RF_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE;
	hal->PHYRegDef[RF_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE;

	hal->PHYRegDef[RF_PATH_A].rf3wireOffset = rA_LSSIWrite_Jaguar;
	hal->PHYRegDef[RF_PATH_B].rf3wireOffset = rB_LSSIWrite_Jaguar;

	hal->PHYRegDef[RF_PATH_A].rfHSSIPara2 = rHSSIRead_Jaguar;
	hal->PHYRegDef[RF_PATH_B].rfHSSIPara2 = rHSSIRead_Jaguar;

	/* Tranceiver Readback LSSI/HSPI mode */
	hal->PHYRegDef[RF_PATH_A].rfLSSIReadBack = rA_SIRead_Jaguar;
	hal->PHYRegDef[RF_PATH_B].rfLSSIReadBack = rB_SIRead_Jaguar;
	hal->PHYRegDef[RF_PATH_A].rfLSSIReadBackPi = rA_PIRead_Jaguar;
	hal->PHYRegDef[RF_PATH_B].rfLSSIReadBackPi = rB_PIRead_Jaguar;
}

/*
 * Description:
 *	Initialize MAC registers
 *
 * Return:
 *	_TRUE	Success
 *	_FALSE	Fail
 */
u8 rtl8733b_phy_init_mac_register(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	u8 ret = _TRUE;
	int res;
	enum hal_status status;


	hal = GET_HAL_DATA(adapter);

	ret = _FALSE;
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	res = phy_ConfigMACWithParaFile(adapter, PHY_FILE_MAC_REG);
	if (_SUCCESS == res)
		ret = _TRUE;
#endif /* CONFIG_LOAD_PHY_PARA_FROM_FILE */
	if (_FALSE == ret) {
		status = odm_config_mac_with_header_file(&hal->odmpriv);
		if (HAL_STATUS_SUCCESS == status)
			ret = _TRUE;
	}
	if (_FALSE == ret)
		RTW_INFO("%s: Write MAC Reg Fail!!", __FUNCTION__);

	return ret;
}

static u8 _init_bb_reg(PADAPTER Adapter)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(Adapter);
	u8 ret = _TRUE;
	int res;
	enum hal_status status;

	/*
	 * 1. Read PHY_REG.TXT BB INIT!!
	 */
	ret = _FALSE;
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	res = phy_ConfigBBWithParaFile(Adapter, PHY_FILE_PHY_REG, CONFIG_BB_PHY_REG);
	if (_SUCCESS == res)
		ret = _TRUE;
#endif
	if (_FALSE == ret) {
		status = odm_config_bb_with_header_file(&hal->odmpriv, CONFIG_BB_PHY_REG);
		if (HAL_STATUS_SUCCESS == status)
			ret = _TRUE;
	}
	if (_FALSE == ret) {
		RTW_INFO("%s: Write BB Reg Fail!!", __FUNCTION__);
		goto exit;
	}

	/*
	 * 2. Read BB AGC table Initialization
	 */
	ret = _FALSE;
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	res = phy_ConfigBBWithParaFile(Adapter, PHY_FILE_AGC_TAB, CONFIG_BB_AGC_TAB);
	if (_SUCCESS == res)
		ret = _TRUE;
#endif
	if (_FALSE == ret) {
		status = odm_config_bb_with_header_file(&hal->odmpriv, CONFIG_BB_AGC_TAB);
		if (HAL_STATUS_SUCCESS == status)
			ret = _TRUE;
	}
	if (_FALSE == ret) {
		RTW_INFO("%s: Write AGC Table Fail!\n", __FUNCTION__);
		goto exit;
	}

exit:
	return ret;
}

static u8 init_bb_reg(PADAPTER adapter)
{
	u8 ret = _TRUE;
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);


	/*
	 * Config BB and AGC
	 */
	ret = _init_bb_reg(adapter);

	if (rtw_phydm_set_crystal_cap(adapter, hal->crystal_cap) == _FALSE) {
		RTW_ERR("Init crystal_cap failed\n");
		rtw_warn_on(1);
		ret = _FALSE;
	}

	return ret;
}

static u8 _init_rf_reg(PADAPTER adapter)
{
	u8 path;
	enum rf_path phydm_path;
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(adapter);
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	u8 *regfile;
#endif
	enum hal_status status;
	int res;
	u8 ret = _TRUE;


	/*
	 * Initialize IQK
	 */

	status = halrf_config_rfk_with_header_file(&hal->odmpriv, CONFIG_BB_RF_CAL_INIT);
	if (HAL_STATUS_SUCCESS == status)
		ret = _TRUE;

	if (_FALSE == ret) {
		RTW_INFO("%s: Init IQK Fail!\n", __FUNCTION__);
		goto exit;
	}

	/*
	 * Initialize RF
	 */
	for (path = 0; path < hal_spec->rf_reg_path_num; path++) {
		/* Initialize RF from configuration file */
		switch (path) {
		case 0:
			phydm_path = RF_PATH_A;
			#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
			regfile = PHY_FILE_RADIO_A;
			#endif
			break;

		case 1:
			phydm_path = RF_PATH_B;
			#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
			regfile = PHY_FILE_RADIO_B;
			#endif
			break;

		default:
			RTW_INFO("%s: [WARN] Unknown path=%d, skip!\n", __FUNCTION__, path);
			continue;
		}

		ret = _FALSE;
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
		res = PHY_ConfigRFWithParaFile(adapter, regfile, phydm_path);
		if (_SUCCESS == res)
			ret = _TRUE;
#endif
		if (_FALSE == ret) {
			status = odm_config_rf_with_header_file(&hal->odmpriv, CONFIG_RF_RADIO, phydm_path);
			if (HAL_STATUS_SUCCESS != status)
				goto exit;
			ret = _TRUE;
		}
	}

	/*
	 * Configuration of Tx Power Tracking
	 */
	ret = _FALSE;
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
	res = PHY_ConfigRFWithTxPwrTrackParaFile(adapter, PHY_FILE_TXPWR_TRACK);
	if (_SUCCESS == res)
		ret = _TRUE;
#endif
	if (_FALSE == ret) {
		status = odm_config_rf_with_tx_pwr_track_header_file(&hal->odmpriv);
		if (HAL_STATUS_SUCCESS != status) {
			RTW_INFO("%s: Write PwrTrack Table Fail!\n", __FUNCTION__);
			goto exit;
		}
		ret = _TRUE;
	}

exit:
	return ret;
}

static u8 init_rf_reg(PADAPTER adapter)
{
	u8 ret = _TRUE;


	ret = _init_rf_reg(adapter);

	return ret;
}

/*
 * Description:
 *	Initialize PHY(BB/RF) related functions
 *
 * Return:
 *	_TRUE	Success
 *	_FALSE	Fail
 */
u8 rtl8733b_phy_init(PADAPTER adapter)
{
	struct dvobj_priv *d;
	struct dm_struct *phydm;
	int err;
	u8 ok = _TRUE;
	BOOLEAN ret;


	d = adapter_to_dvobj(adapter);
	phydm = adapter_to_phydm(adapter);

	bb_rf_register_definition(adapter);

	err = rtw_halmac_phy_power_switch(d, _TRUE);
	if (err)
		return _FALSE;

	ret = config_phydm_parameter_init_8733b(phydm, ODM_PRE_SETTING);
	if (FALSE == ret)
		return _FALSE;

	ok = init_bb_reg(adapter);
	if (_FALSE == ok)
		return _FALSE;
	ok = init_rf_reg(adapter);
	if (_FALSE == ok)
		return _FALSE;
	ret = config_phydm_parameter_init_8733b(phydm, ODM_POST_SETTING);
	if (FALSE == ret)
		return _FALSE;

	return _TRUE;
}

#ifdef CONFIG_SUPPORT_HW_WPS_PBC
static void dm_CheckPbcGPIO(PADAPTER adapter)
{
	u8 tmp1byte;
	u8 bPbcPressed = _FALSE;

	if (!adapter->registrypriv.hw_wps_pbc)
		return;

#ifdef CONFIG_USB_HCI
	tmp1byte = rtw_read8(adapter, GPIO_IO_SEL);
	tmp1byte |= (HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(adapter, GPIO_IO_SEL, tmp1byte); /* enable GPIO[2] as output mode */

	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(adapter, GPIO_IN, tmp1byte); /* reset the floating voltage level */

	tmp1byte = rtw_read8(adapter, GPIO_IO_SEL);
	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(adapter, GPIO_IO_SEL, tmp1byte); /* enable GPIO[2] as input mode */

	tmp1byte = rtw_read8(adapter, GPIO_IN);
	if (tmp1byte == 0xff)
		return;

	if (tmp1byte & HAL_8192C_HW_GPIO_WPS_BIT)
		bPbcPressed = _TRUE;
#else
	tmp1byte = rtw_read8(adapter, GPIO_IN);

	if ((tmp1byte == 0xff) || adapter->init_adpt_in_progress)
		return;

	if ((tmp1byte & HAL_8192C_HW_GPIO_WPS_BIT) == 0)
		bPbcPressed = _TRUE;
#endif

	if (_TRUE == bPbcPressed) {
		/*
		 * Here we only set bPbcPressed to true
		 * After trigger PBC, the variable will be set to false
		 */
		RTW_INFO("CheckPbcGPIO - PBC is pressed\n");
		rtw_request_wps_pbc_event(adapter);
	}
}
#endif /* CONFIG_SUPPORT_HW_WPS_PBC */

/*
 * ============================================================
 * functions
 * ============================================================
 */
static void init_phydm_cominfo(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	struct dm_struct *p_dm_odm;
	u32 support_ability = 0;

	hal = GET_HAL_DATA(adapter);
	p_dm_odm = &hal->odmpriv;

	Init_ODM_ComInfo(adapter);

	odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, hal->PackageType);
	odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_IC_TYPE, ODM_RTL8733B);

	RTW_INFO("%s: Fv=%d Cv=%d\n", __FUNCTION__, hal->version_id.VendorType, hal->version_id.CUTVersion);
	odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_FAB_VER, hal->version_id.VendorType);
	odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_CUT_VER, hal->version_id.CUTVersion);

	odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_TSSI_ENABLE
		, hal->txpwr_pg_mode == TXPWR_PG_WITH_TSSI_OFFSET ? _TRUE : _FALSE);
}

void rtl8733b_phy_init_dm_priv(PADAPTER adapter)
{
	struct dm_struct *podmpriv = adapter_to_phydm(adapter);

	init_phydm_cominfo(adapter);
	odm_init_all_timers(podmpriv);
}

void rtl8733b_phy_deinit_dm_priv(PADAPTER adapter)
{
	struct dm_struct *podmpriv = adapter_to_phydm(adapter);

	odm_cancel_all_timers(podmpriv);
}

void rtl8733b_phy_init_haldm(PADAPTER adapter)
{
	rtw_phydm_init(adapter);
}

static void check_rxfifo_full(PADAPTER adapter)
{
	struct dvobj_priv *psdpriv = adapter->dvobj;
	struct debug_priv *pdbgpriv = &psdpriv->drv_dbg;
	struct registry_priv *regsty = &adapter->registrypriv;
	u8 val8 = 0;

	if (regsty->check_hw_status == 1) {
		/* switch counter to RX fifo */
		val8 = rtw_read8(adapter, REG_RXERR_RPT_8733B + 3);
		rtw_write8(adapter, REG_RXERR_RPT_8733B + 3, (val8 | 0xa0));

		pdbgpriv->dbg_rx_fifo_last_overflow = pdbgpriv->dbg_rx_fifo_curr_overflow;
		pdbgpriv->dbg_rx_fifo_curr_overflow = rtw_read16(adapter, REG_RXERR_RPT_8733B);
		pdbgpriv->dbg_rx_fifo_diff_overflow =
			pdbgpriv->dbg_rx_fifo_curr_overflow - pdbgpriv->dbg_rx_fifo_last_overflow;
	}
}

void rtl8733b_phy_haldm_watchdog(PADAPTER adapter)
{
	BOOLEAN bFwCurrentInPSMode = _FALSE;
	u8 bFwPSAwake = _TRUE;
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);
	u8 in_lps = _FALSE;
	PADAPTER current_lps_iface = NULL, iface = NULL;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	u8 i = 0;

	if (!rtw_is_hw_init_completed(adapter))
		goto skip_dm;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = adapter_to_pwrctl(adapter)->bFwCurrentInPSMode;
	rtw_hal_get_hwreg(adapter, HW_VAR_FWLPS_RF_ON, &bFwPSAwake);
#endif /* CONFIG_LPS */

#ifdef CONFIG_P2P_PS
	/*
	 * Fw is under p2p powersaving mode, driver should stop dynamic mechanism.
	 */
	if (adapter->wdinfo.p2p_ps_mode)
		bFwPSAwake = _FALSE;
#endif /* CONFIG_P2P_PS */

	if ((rtw_is_hw_init_completed(adapter))
	    && ((!bFwCurrentInPSMode) && bFwPSAwake)) {

		/* check rx fifo */
		check_rxfifo_full(adapter);
	}

#ifdef CONFIG_LPS
	if (pwrpriv->bLeisurePs && bFwCurrentInPSMode && pwrpriv->pwr_mode != PS_MODE_ACTIVE) {
		in_lps = _TRUE;

		for (i = 0; i < dvobj->iface_nums; i++) {
			iface = dvobj->padapters[i];
			if (pwrpriv->current_lps_hw_port_id == rtw_hal_get_port(iface)) {
				current_lps_iface = iface;
				rtw_lps_rfon_ctrl(current_lps_iface, rf_on);
				break;
			}
		}

		if (!current_lps_iface) {
			RTW_WARN("Can't find a adapter with LPS to enable RFON function !\n");
			goto skip_dm;
		}
	}
#endif

#ifdef CONFIG_BEAMFORMING
#ifdef RTW_BEAMFORMING_VERSION_2
	if (check_fwstate(&adapter->mlmepriv, WIFI_STATION_STATE) &&
			check_fwstate(&adapter->mlmepriv, WIFI_ASOC_STATE))
		rtw_hal_beamforming_config_csirate(adapter);
#endif
#endif

#ifdef CONFIG_DISABLE_ODM
	goto skip_dm;
#endif

	rtw_phydm_watchdog(adapter, in_lps);

skip_dm:

#ifdef CONFIG_LPS
	if (current_lps_iface)
		rtw_lps_rfon_ctrl(current_lps_iface, rf_off);
#endif
	/*
	 * Check GPIO to determine current RF on/off and Pbc status.
	 * Check Hardware Radio ON/OFF or not
	 */
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
	dm_CheckPbcGPIO(adapter);
#else /* !CONFIG_SUPPORT_HW_WPS_PBC */
	return;
#endif /* !CONFIG_SUPPORT_HW_WPS_PBC */
}

static u32 phy_calculatebitshift(u32 mask)
{
	u32 i;


	for (i = 0; i <= 31; i++)
		if (mask & BIT(i))
			break;

	return i;
}

u32 rtl8733b_read_bb_reg(PADAPTER adapter, u32 addr, u32 mask)
{
	u32 val = 0, val_org, shift;


#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	val_org = rtw_read32(adapter, addr);
	shift = phy_calculatebitshift(mask);
	val = (val_org & mask) >> shift;

	return val;
}

void rtl8733b_write_bb_reg(PADAPTER adapter, u32 addr, u32 mask, u32 val)
{
	u32 val_org, shift;


#if (DISABLE_BB_RF == 1)
	return;
#endif

	if (mask != 0xFFFFFFFF) {
		/* not "double word" write */
		val_org = rtw_read32(adapter, addr);
		shift = phy_calculatebitshift(mask);
		val = ((val_org & (~mask)) | ((val << shift) & mask));
	}

	rtw_write32(adapter, addr, val);
}

u32 rtl8733b_read_rf_reg(PADAPTER adapter, enum rf_path path, u32 addr, u32 mask)
{
	struct dm_struct *phydm = adapter_to_phydm(adapter);
	u32 val = 0;

	val = config_phydm_read_rf_reg_8733b(phydm, path, addr, mask);
	if (!config_phydm_read_rf_check_8733b(val))
		RTW_INFO(FUNC_ADPT_FMT ": read RF reg path=%d addr=0x%x mask=0x%x FAIL!\n",
			 FUNC_ADPT_ARG(adapter), path, addr, mask);

	return val;
}

void rtl8733b_write_rf_reg(PADAPTER adapter, enum rf_path path, u32 addr, u32 mask, u32 val)
{
	struct dm_struct *phydm = adapter_to_phydm(adapter);
	u8 ret;

	ret = config_phydm_write_rf_reg_8733b(phydm, path, addr, mask, val);
	if (_FALSE == ret)
		RTW_INFO(FUNC_ADPT_FMT ": write RF reg path=%d addr=0x%x mask=0x%x val=0x%x FAIL!\n",
			 FUNC_ADPT_ARG(adapter), path, addr, mask, val);
}

static void set_tx_power_level_by_path(PADAPTER adapter, u8 channel, u8 path)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	u8 under_survey_ch = phy_check_under_survey_ch(adapter);
	u8 under_24g = (hal->current_band_type == BAND_ON_2_4G);

	if (under_24g)
		phy_set_tx_power_index_by_rate_section(adapter, path, channel, CCK);

	phy_set_tx_power_index_by_rate_section(adapter, path, channel, OFDM);

	if (!under_survey_ch) {
		phy_set_tx_power_index_by_rate_section(adapter, path, channel, HT_MCS0_MCS7);
	}
}

void rtl8733b_set_tx_power_level(PADAPTER adapter, u8 channel)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	struct dm_struct *phydm = adapter_to_phydm(adapter);
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(adapter);
	u8 path;

	for (path = RF_PATH_A; path < hal_spec->rf_reg_path_num; ++path) {
#ifdef CONFIG_TXPWR_PG_WITH_TSSI_OFFSET
		if (hal_data->txpwr_pg_mode == TXPWR_PG_WITH_TSSI_OFFSET) {
			_halrf_tssi_set_powerlevel_8733b(phydm, 0, path);
		} else 
#endif
		{
			/*
			* can't bypass unused path
			* because phydm need all path values to calculate min diff
			*/
			set_tx_power_level_by_path(adapter, channel, path);
		}
	}
}

void rtl8733b_set_txpwr_done(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	struct dm_struct *phydm = adapter_to_phydm(adapter);

	if (hal_data->txpwr_pg_mode == TXPWR_PG_WITH_PWR_IDX
#ifdef CONFIG_MP_INCLUDED
	|| rtw_mp_mode_check(adapter) == _TRUE
#endif
	)
		config_phydm_set_txagc_to_hw_8733b(phydm);
}

/*
 * Parameters:
 *	padatper
 *	powerindex	power index for rate
 *	rfpath		Antenna(RF) path, type "enum rf_path"
 *	rate		data rate, type "enum MGN_RATE"
 */
void rtl8733b_set_tx_power_index(PADAPTER adapter, u32 powerindex, enum rf_path rfpath, u8 rate)
{
	HAL_DATA_TYPE *hal = GET_HAL_DATA(adapter);
	struct dm_struct *phydm = adapter_to_phydm(adapter);
	u8 shift = 0;
	boolean write_ret;

	if (!IS_1T_RATE(rate) && !IS_2T_RATE(rate)) {
		RTW_ERR(FUNC_ADPT_FMT" invalid rate(%s)\n", FUNC_ADPT_ARG(adapter), MGN_RATE_STR(rate));
		rtw_warn_on(1);
		goto exit;
	}

	rate = MRateToHwRate(rate);

	/*
	* For 8733B, phydm api use 4 bytes txagc value
	* driver must combine every four 1 byte to one 4 byte and send to phydm api
	*/
	shift = rate % 4;
	hal->txagc_set_buf |= ((powerindex & 0xff) << (shift * 8));

	if (shift != 3)
		goto exit;

	rate = rate & 0xFC;
	write_ret = config_phydm_write_txagc_8733b(phydm, hal->txagc_set_buf, rfpath, rate);

	if (write_ret == true && !DBG_TX_POWER_IDX)
		goto clear_buf;

	RTW_INFO(FUNC_ADPT_FMT" (index:0x%08x, %c, rate:%s(0x%02x), disable api:%d) %s\n"
		, FUNC_ADPT_ARG(adapter), hal->txagc_set_buf, rf_path_char(rfpath)
		, HDATA_RATE(rate), rate, phydm->is_disable_phy_api
		, write_ret == true ? "OK" : "FAIL");

	rtw_warn_on(write_ret != true);

clear_buf:
	hal->txagc_set_buf = 0;

exit:
	return;
}

/*
 * Description:
 *	Check need to switch band or not
 * Parameters:
 *	channelToSW	channel wiii be switch to
 * Return:
 *	_TRUE		need to switch band
 *	_FALSE		not need to switch band
 */
static u8 need_switch_band(PADAPTER adapter, u8 channelToSW)
{
	u8 u1tmp = 0;
	u8 ret_value = _TRUE;
	u8 Band = BAND_ON_5G, BandToSW = BAND_ON_5G;
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);

	Band = hal->current_band_type;

	/* Use current swich channel to judge Band Type and switch Band if need */
	if (channelToSW > 14)
		BandToSW = BAND_ON_5G;
	else
		BandToSW = BAND_ON_2_4G;

	if (BandToSW != Band) {
		/* record current band type for other hal use */
		hal->current_band_type = (BAND_TYPE)BandToSW;
		ret_value = _TRUE;
	} else
		ret_value = _FALSE;

	return ret_value;
}

static u8 get_pri_ch_id(PADAPTER adapter)
{
	u8 pri_ch_idx = 0;
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);

	if (hal->current_channel_bw == CHANNEL_WIDTH_80) {
		/* primary channel is at lower subband of 80MHz & 40MHz */
		if ((hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER) && (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER))
			pri_ch_idx = VHT_DATA_SC_20_LOWEST_OF_80MHZ;
		/* primary channel is at lower subband of 80MHz & upper subband of 40MHz */
		else if ((hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER) && (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER))
			pri_ch_idx = VHT_DATA_SC_20_LOWER_OF_80MHZ;
		/* primary channel is at upper subband of 80MHz & lower subband of 40MHz */
		else if ((hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER) && (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER))
			pri_ch_idx = VHT_DATA_SC_20_UPPER_OF_80MHZ;
		/* primary channel is at upper subband of 80MHz & upper subband of 40MHz */
		else if ((hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER) && (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER))
			pri_ch_idx = VHT_DATA_SC_20_UPPERST_OF_80MHZ;
		else {
			if (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER)
				pri_ch_idx = VHT_DATA_SC_40_LOWER_OF_80MHZ;
			else if (hal->nCur80MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
				pri_ch_idx = VHT_DATA_SC_40_UPPER_OF_80MHZ;
			else
				RTW_INFO("SCMapping: DONOT CARE Mode Setting\n");
		}
	} else if (hal->current_channel_bw == CHANNEL_WIDTH_40) {
		/* primary channel is at upper subband of 40MHz */
		if (hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
			pri_ch_idx = VHT_DATA_SC_20_UPPER_OF_80MHZ;
		/* primary channel is at lower subband of 40MHz */
		else if (hal->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER)
			pri_ch_idx = VHT_DATA_SC_20_LOWER_OF_80MHZ;
		else
			RTW_INFO("SCMapping: DONOT CARE Mode Setting\n");
	}

	return  pri_ch_idx;
}

static void mac_switch_bandwidth(PADAPTER adapter, u8 pri_ch_idx)
{
	u8 channel = 0, bw = 0;
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	int err;

	channel = hal->current_channel;
	bw = hal->current_channel_bw;
	err = rtw_halmac_set_bandwidth(adapter_to_dvobj(adapter), channel, pri_ch_idx, bw);
	if (err) {
		RTW_INFO(FUNC_ADPT_FMT ": (channel=%d, pri_ch_idx=%d, bw=%d) fail\n",
			 FUNC_ADPT_ARG(adapter), channel, pri_ch_idx, bw);
	}
}

static void switch_chnl_and_set_bw_by_drv(PADAPTER adapter, u8 switch_band)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	struct dm_struct *p_dm_odm = &hal->odmpriv;
	u8 center_ch = 0, ret = 0;

	/* set channel & Bandwidth register */
	/* 1. set switch band register if need to switch band */
	if (switch_band) {
		/* hal->current_channel is center channel of pmlmeext->cur_channel(primary channel) */
		ret = config_phydm_switch_band_8733b(p_dm_odm, hal->current_channel);

		if (!ret) {
			RTW_INFO("%s: config_phydm_switch_band_8733b fail\n", __FUNCTION__);
			rtw_warn_on(1);
			return;
		}
	}

	/* 2. set channel register */
	if (hal->bSwChnl) {
		ret = config_phydm_switch_channel_8733b(p_dm_odm, hal->current_channel);
		hal->bSwChnl = _FALSE;

		if (!ret) {
			RTW_INFO("%s: config_phydm_switch_channel_8733b fail\n", __FUNCTION__);
			rtw_warn_on(1);
			return;
		}
	}

	/* 3. set Bandwidth register */
	if (hal->bSetChnlBW) {
		/* get primary channel index */
		u8 pri_ch_idx = get_pri_ch_id(adapter);

		/* 3.1 set MAC register */
		mac_switch_bandwidth(adapter, pri_ch_idx);

		/* 3.2 set BB/RF registet */
		ret = config_phydm_switch_bandwidth_8733b(p_dm_odm, pri_ch_idx, hal->current_channel_bw);
		hal->bSetChnlBW = _FALSE;

		if (!ret) {
			RTW_INFO("%s: config_phydm_switch_bandwidth_8733b fail\n", __FUNCTION__);
			rtw_warn_on(1);
			return;
		}
	}
}

#ifdef RTW_CHANNEL_SWITCH_OFFLOAD
static void switch_chnl_and_set_bw_by_fw(PADAPTER adapter, u8 switch_band)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);

	if (switch_band ||hal->bSwChnl || hal->bSetChnlBW) {
		rtw_hal_switch_chnl_and_set_bw_offload(adapter,
			hal->current_channel, get_pri_ch_id(adapter), hal->current_channel_bw);

		hal->bSwChnl = _FALSE;
		hal->bSetChnlBW = _FALSE;
	}
}
#endif

/*
 * Description:
 *	Set channel & bandwidth & offset
 */
void rtl8733b_switch_chnl_and_set_bw(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	struct dm_struct *p_dm_odm = &hal->odmpriv;
	u8 center_ch = 0, ret = 0, switch_band = _FALSE;

	if (adapter->bNotifyChannelChange) {
		RTW_INFO("[%s] bSwChnl=%d, ch=%d, bSetChnlBW=%d, bw=%d\n",
			 __FUNCTION__,
			 hal->bSwChnl,
			 hal->current_channel,
			 hal->bSetChnlBW,
			 hal->current_channel_bw);
	}

	if (RTW_CANNOT_RUN(adapter)) {
		hal->bSwChnlAndSetBWInProgress = _FALSE;
		return;
	}

	switch_band = need_switch_band(adapter, hal->current_channel);

	/* config channel, bw, offset setting */
#ifdef RTW_CHANNEL_SWITCH_OFFLOAD
	if (hal->ch_switch_offload) {

	#ifdef RTW_REDUCE_SCAN_SWITCH_CH_TIME
		struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
		_adapter *iface;
		struct mlme_ext_priv *mlmeext;
		u8 drv_switch = _TRUE;
		int i;

		for (i = 0; i < dvobj->iface_nums; i++) {
			iface = dvobj->padapters[i];
			mlmeext = &iface->mlmeextpriv;

			/* check scan state */
			if (mlmeext_scan_state(mlmeext) != SCAN_DISABLE
				&& mlmeext_scan_state(mlmeext) != SCAN_COMPLETE
					&& mlmeext_scan_state(mlmeext) != SCAN_BACKING_OP)
				drv_switch = _FALSE;
		}
	#else
		u8 drv_switch = _FALSE;
	#endif

		if (drv_switch == _TRUE)
			switch_chnl_and_set_bw_by_drv(adapter, switch_band);
		else
		switch_chnl_and_set_bw_by_fw(adapter, switch_band);

	} else {
		switch_chnl_and_set_bw_by_drv(adapter, switch_band);
	}
#else
	switch_chnl_and_set_bw_by_drv(adapter, switch_band);
#endif /* RTW_CHANNEL_SWITCH_OFFLOAD */


	/* config coex setting */
	if (switch_band) {
#ifdef CONFIG_BT_COEXIST
		if (hal->EEPROMBluetoothCoexist) {
			struct mlme_ext_priv *mlmeext;

			/* switch band under site survey or not, must notify to BT COEX */
			mlmeext = &adapter->mlmeextpriv;
			if (mlmeext_scan_state(mlmeext) != SCAN_DISABLE)
				rtw_btcoex_switchband_notify(_TRUE, hal->current_band_type);
			else
				rtw_btcoex_switchband_notify(_FALSE, hal->current_band_type);
		} else
			rtw_btcoex_wifionly_switchband_notify(adapter);
#else /* !CONFIG_BT_COEXIST */
		rtw_btcoex_wifionly_switchband_notify(adapter);
#endif /* CONFIG_BT_COEXIST */
	}

#ifdef CONFIG_TXPWR_PG_WITH_PWR_IDX
	/* Suggested by winnita_zhao, TSSI doesn't need to do these */
	if (hal->txpwr_pg_mode != TXPWR_PG_WITH_TSSI_OFFSET) {
		phydm_config_kfree(p_dm_odm, hal->current_channel);
		/* TX Power Setting */
		odm_clear_txpowertracking_state(p_dm_odm);
	}
#endif

	rtw_hal_set_tx_power_level(adapter, hal->current_channel);

	/* IQK */
	if ((hal->bNeedIQK == _TRUE)
	    || (adapter->registrypriv.mp_mode == 1)) {
		/*phy_iq_calibrate_8733b(p_dm_odm, _FALSE);*/
		rtw_phydm_iqk_trigger(adapter);
		hal->bNeedIQK = _FALSE;
	}
}

/*
 * Description:
 *	Store channel setting to hal date
 * Parameters:
 *	bSwitchChannel		swith channel or not
 *	bSetBandWidth		set band or not
 *	ChannelNum		center channel
 *	ChnlWidth		bandwidth
 *	ChnlOffsetOf40MHz	channel offset for 40MHz Bandwidth
 *	ChnlOffsetOf80MHz	channel offset for 80MHz Bandwidth
 *	CenterFrequencyIndex1	center channel index
 */

void rtl8733b_handle_sw_chnl_and_set_bw(
	PADAPTER Adapter, u8 bSwitchChannel, u8 bSetBandWidth,
	u8 ChannelNum, enum channel_width ChnlWidth, u8 ChnlOffsetOf40MHz,
	u8 ChnlOffsetOf80MHz, u8 CenterFrequencyIndex1)
{
	PADAPTER pDefAdapter = GetDefaultAdapter(Adapter);
	PHAL_DATA_TYPE hal = GET_HAL_DATA(pDefAdapter);
	u8 tmpChannel = hal->current_channel;
	enum channel_width tmpBW = hal->current_channel_bw;
	u8 tmpnCur40MhzPrimeSC = hal->nCur40MhzPrimeSC;
	u8 tmpnCur80MhzPrimeSC = hal->nCur80MhzPrimeSC;
	u8 tmpCenterFrequencyIndex1 = hal->CurrentCenterFrequencyIndex1;
	struct mlme_ext_priv *pmlmeext = &Adapter->mlmeextpriv;


	/* check swchnl or setbw */
	if (!bSwitchChannel && !bSetBandWidth) {
		RTW_INFO("%s: not switch channel and not set bandwidth\n", __FUNCTION__);
		return;
	}

	/* skip switch channel operation for current channel & ChannelNum(will be switch) are the same */
	if (bSwitchChannel) {
		if (hal->current_channel != ChannelNum) {
			if (HAL_IsLegalChannel(Adapter, ChannelNum))
				hal->bSwChnl = _TRUE;
			else
				return;
		}
	}

	/* check set BandWidth */
	if (bSetBandWidth) {
		/* initial channel bw setting */
		if (hal->bChnlBWInitialized == _FALSE) {
			hal->bChnlBWInitialized = _TRUE;
			hal->bSetChnlBW = _TRUE;
		} else if ((hal->current_channel_bw != ChnlWidth) || /* check whether need set band or not */
			   (hal->nCur40MhzPrimeSC != ChnlOffsetOf40MHz) ||
			   (hal->nCur80MhzPrimeSC != ChnlOffsetOf80MHz) ||
			(hal->CurrentCenterFrequencyIndex1 != CenterFrequencyIndex1))
			hal->bSetChnlBW = _TRUE;
	}

	/* return if not need set bandwidth nor channel after check*/
	if (!hal->bSetChnlBW && !hal->bSwChnl && hal->bNeedIQK != _TRUE)
		return;

	/* set channel number to hal data */
	if (hal->bSwChnl) {
		hal->current_channel = ChannelNum;
		hal->CurrentCenterFrequencyIndex1 = ChannelNum;
	}

	/* set bandwidth info to hal data */
	if (hal->bSetChnlBW) {
		hal->current_channel_bw = ChnlWidth;
		hal->nCur40MhzPrimeSC = ChnlOffsetOf40MHz;
		hal->nCur80MhzPrimeSC = ChnlOffsetOf80MHz;
		hal->CurrentCenterFrequencyIndex1 = CenterFrequencyIndex1;
	}

	/* switch channel & bandwidth */
	if (!RTW_CANNOT_RUN(Adapter))
		rtl8733b_switch_chnl_and_set_bw(Adapter);
	else {
		if (hal->bSwChnl) {
			hal->current_channel = tmpChannel;
			hal->CurrentCenterFrequencyIndex1 = tmpChannel;
		}

		if (hal->bSetChnlBW) {
			hal->current_channel_bw = tmpBW;
			hal->nCur40MhzPrimeSC = tmpnCur40MhzPrimeSC;
			hal->nCur80MhzPrimeSC = tmpnCur80MhzPrimeSC;
			hal->CurrentCenterFrequencyIndex1 = tmpCenterFrequencyIndex1;
		}
	}
}

/*
 * Description:
 *	Change channel, bandwidth & offset
 * Parameters:
 *	center_ch	center channel
 *	bw		bandwidth
 *	offset40	channel offset for 40MHz Bandwidth
 *	offset80	channel offset for 80MHz Bandwidth
 */
void rtl8733b_set_channel_bw(PADAPTER adapter, u8 center_ch, enum channel_width bw, u8 offset40, u8 offset80)
{
	rtl8733b_handle_sw_chnl_and_set_bw(adapter, _TRUE, _TRUE, center_ch, bw, offset40, offset80, center_ch);
}

void rtl8733b_notch_filter_switch(PADAPTER adapter, bool enable)
{
	if (enable)
		RTW_INFO("%s: Enable notch filter\n", __FUNCTION__);
	else
		RTW_INFO("%s: Disable notch filter\n", __FUNCTION__);
}

#ifdef CONFIG_MP_INCLUDED
/*
 * Description:
 *	Config RF path
 *
 * Parameters:
 *	adapter	pointer of struct _ADAPTER
 */
void rtl8733b_mp_config_rfpath(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	PMPT_CONTEXT mpt;
	ANTENNA_PATH anttx, antrx;
	enum bb_path bb_tx, bb_rx;


	hal = GET_HAL_DATA(adapter);
	mpt = &adapter->mppriv.mpt_ctx;
	anttx = hal->antenna_tx_path;
	antrx = hal->AntennaRxPath;
	hal->antenna_test = _TRUE;
	RTW_INFO("+Config RF Path, tx=0x%x rx=0x%x\n", anttx, antrx);

	switch (anttx) {
	case ANTENNA_A:
		mpt->mpt_rf_path = RF_PATH_A;
		bb_tx = BB_PATH_A;
		break;
	case ANTENNA_B:
		mpt->mpt_rf_path = RF_PATH_B;
		bb_tx = BB_PATH_B;
		break;
	case ANTENNA_AB:
	default:
		mpt->mpt_rf_path = RF_PATH_AB;
		bb_tx = BB_PATH_A | BB_PATH_B;
		break;
	}

	switch (antrx) {
	case ANTENNA_A:
		bb_rx = BB_PATH_A;
		break;
	case ANTENNA_B:
		bb_rx = BB_PATH_B;
		break;
	case ANTENNA_AB:
	default:
		bb_rx = BB_PATH_A | BB_PATH_B;
		break;
	}

	phydm_api_trx_mode(GET_PDM_ODM(adapter), bb_tx, bb_rx, bb_tx);

	RTW_INFO("-Config RF Path Finish\n");
}
#endif /* CONFIG_MP_INCLUDED */
