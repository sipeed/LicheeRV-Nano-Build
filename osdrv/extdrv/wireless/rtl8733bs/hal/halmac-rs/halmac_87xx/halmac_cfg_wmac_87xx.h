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

#ifndef _HALMAC_CFG_WMAC_87XX_H_
#define _HALMAC_CFG_WMAC_87XX_H_

#include "../halmac_api.h"

#if HALMAC_87XX_SUPPORT

enum halmac_ret_status
cfg_mac_addr_87xx(struct halmac_adapter *adapter, u8 port,
		  union halmac_wlan_addr *addr);

enum halmac_ret_status
cfg_bssid_87xx(struct halmac_adapter *adapter, u8 port,
	       union halmac_wlan_addr *addr);

enum halmac_ret_status
cfg_transmitter_addr_87xx(struct halmac_adapter *adapter, u8 port,
			  union halmac_wlan_addr *addr);

enum halmac_ret_status
cfg_net_type_87xx(struct halmac_adapter *adapter, u8 port,
		  enum halmac_network_type_select net_type);

enum halmac_ret_status
cfg_tsf_rst_87xx(struct halmac_adapter *adapter, u8 port);

enum halmac_ret_status
cfg_bcn_space_87xx(struct halmac_adapter *adapter, u8 port, u32 bcn_space);

enum halmac_ret_status
rw_bcn_ctrl_87xx(struct halmac_adapter *adapter, u8 port, u8 write_en,
		 struct halmac_bcn_ctrl *ctrl);

enum halmac_ret_status
cfg_multicast_addr_87xx(struct halmac_adapter *adapter,
			union halmac_wlan_addr *addr);

enum halmac_ret_status
cfg_operation_mode_87xx(struct halmac_adapter *adapter,
			enum halmac_wireless_mode mode);

enum halmac_ret_status
cfg_ch_bw_87xx(struct halmac_adapter *adapter, u8 ch,
	       enum halmac_pri_ch_idx idx, enum halmac_bw bw);

enum halmac_ret_status
cfg_ch_87xx(struct halmac_adapter *adapter, u8 ch);

enum halmac_ret_status
cfg_pri_ch_idx_87xx(struct halmac_adapter *adapter, enum halmac_pri_ch_idx idx);

enum halmac_ret_status
cfg_bw_87xx(struct halmac_adapter *adapter, enum halmac_bw bw);

void
cfg_txfifo_lt_87xx(struct halmac_adapter *adapter,
		   struct halmac_txfifo_lifetime_cfg *cfg);

enum halmac_ret_status
enable_bb_rf_87xx(struct halmac_adapter *adapter, u8 enable);

enum halmac_ret_status
cfg_la_mode_87xx(struct halmac_adapter *adapter, enum halmac_la_mode mode);

enum halmac_ret_status
cfg_rxfifo_expand_mode_87xx(struct halmac_adapter *adapter,
			    enum halmac_rx_fifo_expanding_mode mode);

enum halmac_ret_status
config_security_87xx(struct halmac_adapter *adapter,
		     struct halmac_security_setting *setting);

u8
get_used_cam_entry_num_87xx(struct halmac_adapter *adapter,
			    enum hal_security_type sec_type);

enum halmac_ret_status
write_cam_87xx(struct halmac_adapter *adapter, u32 idx,
	       struct halmac_cam_entry_info *info);

enum halmac_ret_status
read_cam_entry_87xx(struct halmac_adapter *adapter, u32 idx,
		    struct halmac_cam_entry_format *content);

enum halmac_ret_status
clear_cam_entry_87xx(struct halmac_adapter *adapter, u32 idx);

void
rx_shift_87xx(struct halmac_adapter *adapter, u8 enable);

enum halmac_ret_status
cfg_edca_para_87xx(struct halmac_adapter *adapter, enum halmac_acq_id acq_id,
		   struct halmac_edca_para *param);

void
rx_clk_gate_87xx(struct halmac_adapter *adapter, u8 enable);

enum halmac_ret_status
rx_cut_amsdu_cfg_87xx(struct halmac_adapter *adapter,
		      struct halmac_cut_amsdu_cfg *cfg);

enum halmac_ret_status
fast_edca_cfg_87xx(struct halmac_adapter *adapter,
		   struct halmac_fast_edca_cfg *cfg);

enum halmac_ret_status
get_mac_addr_87xx(struct halmac_adapter *adapter, u8 port,
		  union halmac_wlan_addr *addr);

void
rts_full_bw_87xx(struct halmac_adapter *adapter, u8 enable);

void
cfg_mac_clk_87xx(struct halmac_adapter *adapter);

#endif/* HALMAC_87XX_SUPPORT */

#endif/* _HALMAC_CFG_WMAC_87XX_H_ */
