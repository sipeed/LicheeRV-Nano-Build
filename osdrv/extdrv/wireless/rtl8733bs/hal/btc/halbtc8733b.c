/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
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

#include "mp_precomp.h"

#if (BT_SUPPORT == 1 && COEX_SUPPORT == 1)

static u8 *trace_buf = &gl_btc_trace_buf[0];

/* rssi express in percentage % (dbm = % - 100)  */
static const u8 wl_rssi_step_8733b[] = {60, 50, 44, 30};
static const u8 bt_rssi_step_8733b[] = {8, 15, 20, 25};

/* Shared-Antenna Coex Table */
static const struct btc_coex_table_para table_sant_8733b[] = {
				{0xffffffff, 0xffffffff}, /*case-0*/
				{0x55555555, 0x55555555},
				{0x66555555, 0x66555555},
				{0xaaaaaaaa, 0xaaaaaaaa},
				{0x5a5a5a5a, 0x5a5a5a5a},
				{0xfafafafa, 0xfafafafa}, /*case-5*/
				{0x6a5a5555, 0xaaaaaaaa},
				{0x6a5a56aa, 0x6a5a56aa},
				{0x6a5a5a5a, 0x6a5a5a5a},
				{0x66555555, 0x5a5a5a5a},
				{0x66555555, 0x6a5a5a5a}, /*case-10*/
				{0x66555555, 0x6a5a5aaa},
				{0x66555555, 0x5a5a5aaa},
				{0x66555555, 0x6aaa5aaa},
				{0x66555555, 0xaaaa5aaa},
				{0x66555555, 0xaaaaaaaa}, /*case-15*/
				{0xffff55ff, 0xfafafafa},
				{0xffff55ff, 0x6afa5afa},
				{0xaaffffaa, 0xfafafafa},
				{0xaa5555aa, 0x5a5a5a5a},
				{0xaa5555aa, 0x6a5a5a5a}, /*case-20*/
				{0xaa5555aa, 0xaaaaaaaa},
				{0xffffffff, 0x5a5a5a5a},
				{0xffffffff, 0x5a5a5a5a},
				{0xffffffff, 0x55555555},
				{0xffffffff, 0x5a5a5aaa}, /*case-25*/
				{0x55555555, 0x5a5a5a5a},
				{0x55555555, 0xaaaaaaaa},
				{0x55555555, 0x6a5a6a5a},
				{0x66556655, 0x66556655},
				{0x66556aaa, 0x6a5a6aaa}, /*case-30*/
				{0xffffffff, 0x5aaa5aaa},
				{0x56555555, 0x5a5a5aaa},
				{0xdaffdaff, 0xdaffdaff},
				{0x6a555a5a, 0x5a5a5a5a},
				{0xe5555555, 0xe5555555}, /*case-35*/
				{0xea5a5a5a, 0xea5a5a5a} };

/* Non-Shared-Antenna Coex Table */
static const struct btc_coex_table_para table_nsant_8733b[] = {
				{0xffffffff, 0xffffffff}, /*case-100*/
				{0x55555555, 0x55555555},
				{0x66555555, 0x66555555},
				{0xaaaaaaaa, 0xaaaaaaaa},
				{0x5a5a5a5a, 0x5a5a5a5a},
				{0xfafafafa, 0xfafafafa}, /*case-105*/
				{0x5afa5afa, 0x5afa5afa},
				{0x55555555, 0xfafafafa},
				{0x66555555, 0xfafafafa},
				{0x66555555, 0x5a5a5a5a},
				{0x66555555, 0x6a5a5a5a}, /*case-110*/
				{0x66555555, 0xaaaaaaaa},
				{0xffff55ff, 0xfafafafa},
				{0xffff55ff, 0x5afa5afa},
				{0xffff55ff, 0xaaaaaaaa},
				{0xffff55ff, 0xffff55ff}, /*case-115*/
				{0xaaffffaa, 0x5afa5afa},
				{0xaaffffaa, 0xaaaaaaaa},
				{0xffffffff, 0xfafafafa},
				{0xffffffff, 0x5afa5afa},
				{0xffffffff, 0xaaaaaaaa},/*case-120*/
				{0x55ff55ff, 0x5afa5afa},
				{0x55ff55ff, 0xaaaaaaaa},
				{0x55ff55ff, 0x55ff55ff},
				{0x6a555a5a, 0xfafafafa} };

/* Shared-Antenna TDMA*/
static const struct btc_tdma_para tdma_sant_8733b[] = {
				{ {0x00, 0x00, 0x00, 0x00, 0x00} }, /*case-0*/
				{ {0x61, 0x45, 0x03, 0x11, 0x11} }, /*case-1*/
				{ {0x61, 0x3a, 0x03, 0x11, 0x11} },
				{ {0x61, 0x30, 0x03, 0x11, 0x11} },
				{ {0x61, 0x20, 0x03, 0x11, 0x11} },
				{ {0x61, 0x10, 0x03, 0x11, 0x11} }, /*case-5*/
				{ {0x61, 0x45, 0x03, 0x11, 0x10} },
				{ {0x61, 0x3a, 0x03, 0x11, 0x10} },
				{ {0x61, 0x30, 0x03, 0x11, 0x10} },
				{ {0x61, 0x20, 0x03, 0x11, 0x10} },
				{ {0x61, 0x10, 0x03, 0x11, 0x10} }, /*case-10*/
				{ {0x61, 0x08, 0x03, 0x11, 0x14} },
				{ {0x61, 0x08, 0x03, 0x10, 0x14} },
				{ {0x51, 0x08, 0x03, 0x10, 0x54} },
				{ {0x51, 0x08, 0x03, 0x10, 0x55} },
				{ {0x51, 0x08, 0x07, 0x10, 0x54} }, /*case-15*/
				{ {0x51, 0x45, 0x03, 0x10, 0x50} },
				{ {0x51, 0x3a, 0x03, 0x10, 0x50} },
				{ {0x51, 0x30, 0x03, 0x10, 0x50} },
				{ {0x51, 0x20, 0x03, 0x10, 0x50} },
				{ {0x51, 0x10, 0x03, 0x10, 0x50} }, /*case-20*/
				{ {0x51, 0x4a, 0x03, 0x10, 0x50} },
				{ {0x51, 0x0c, 0x03, 0x10, 0x54} },
				{ {0x55, 0x08, 0x03, 0x10, 0x54} },
				{ {0x65, 0x10, 0x03, 0x11, 0x10} },
				{ {0x51, 0x10, 0x03, 0x10, 0x51} }, /*case-25*/
				{ {0x51, 0x08, 0x03, 0x10, 0x50} },
				{ {0x61, 0x08, 0x03, 0x11, 0x11} } };


/* Non-Shared-Antenna TDMA*/
static const struct btc_tdma_para tdma_nsant_8733b[] = {
				{ {0x00, 0x00, 0x00, 0x00, 0x00} }, /*case-100*/
				{ {0x61, 0x45, 0x03, 0x11, 0x11} }, /*case-101*/
				{ {0x61, 0x3a, 0x03, 0x11, 0x11} },
				{ {0x61, 0x30, 0x03, 0x11, 0x11} },
				{ {0x61, 0x20, 0x03, 0x11, 0x11} },
				{ {0x61, 0x10, 0x03, 0x11, 0x11} }, /*case-105*/
				{ {0x61, 0x45, 0x03, 0x11, 0x10} },
				{ {0x61, 0x3a, 0x03, 0x11, 0x10} },
				{ {0x61, 0x30, 0x03, 0x11, 0x10} },
				{ {0x61, 0x20, 0x03, 0x11, 0x10} },
				{ {0x61, 0x10, 0x03, 0x11, 0x10} }, /*case-110*/
				{ {0x61, 0x08, 0x03, 0x11, 0x14} },
				{ {0x61, 0x08, 0x03, 0x10, 0x14} },
				{ {0x51, 0x08, 0x03, 0x10, 0x54} },
				{ {0x51, 0x08, 0x03, 0x10, 0x55} },
				{ {0x51, 0x08, 0x07, 0x10, 0x54} }, /*case-115*/
				{ {0x51, 0x45, 0x03, 0x10, 0x50} },
				{ {0x51, 0x3a, 0x03, 0x10, 0x50} },
				{ {0x51, 0x30, 0x03, 0x10, 0x50} },
				{ {0x51, 0x20, 0x03, 0x10, 0x50} },
				{ {0x51, 0x10, 0x03, 0x10, 0x50} }, /*case-120*/
				{ {0x51, 0x08, 0x03, 0x10, 0x50} },
				{ {0x61, 0x30, 0x03, 0x10, 0x11} },
				{ {0x61, 0x08, 0x03, 0x10, 0x11} },
				{ {0x61, 0x08, 0x07, 0x10, 0x14} },
				{ {0x61, 0x08, 0x03, 0x10, 0x10} }, /*case-125*/
				{ {0x61, 0x08, 0x03, 0x11, 0x15} } };

/* wl_tx_dec_power, bt_tx_dec_power, wl_rx_gain, bt_rx_lna_constrain */
static const struct btc_rf_para rf_para_tx_8733b[] = {
				{0, 0, FALSE, 7},  /* for normal */
				{0, 16, FALSE, 7}, /* for WL-CPT */
				{0, 8, TRUE, 7},  /* 2 for RCU SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},  /* 6 for RCU OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, 4},
				{0, 8, TRUE, 7},   /* 10 for A2DP SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},   /* 14 for A2DP OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, 4},
				{0, 8, TRUE, 7},  /* 18 for A2DP+RCU SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},  /* 22 for A2DP+RCU OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, 4} };

static const struct btc_rf_para rf_para_rx_8733b[] = {
				{0, 0, FALSE, 7},  /* for normal */
				{0, 16, FALSE, 7}, /* for WL-CPT */
				{0, 8, TRUE, 7},  /* 2 for RCU SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},  /* 6 for RCU OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, 4},
				{0, 8, TRUE, 7},   /* 10 for A2DP SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},   /* 14 for A2DP OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, },
				{0, 8, TRUE, 7},  /* 18 for A2DP+RCU SDR*/
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{0, 8, TRUE, 7},
				{16, 4, TRUE, 4},  /* 22 for A2DP+RCU OFC*/
				{15, 5, TRUE, 4},
				{7, 8, TRUE, 4},
				{6, 10, TRUE, 4} };

const struct btc_5g_afh_map afh_5g_8733b[] = { {0, 0, 0} };

const struct btc_chip_para btc_chip_para_8733b = {
	"8733b",				/*.chip_name */
	20211210,				/*.para_ver_date */
	0x06,					/*.para_ver */
	0x04,					/* bt_desired_ver */
	0x1000c,				/* wl_desired_ver */
	TRUE,					/* scbd_support */
	0xac,					/* scbd_reg*/
	BTC_SCBD_32_BIT,			/* scbd_bit_num */
	TRUE,					/* mailbox_support*/
	FALSE,					/* lte_indirect_access */
	TRUE,					/* new_scbd10_def */
	BTC_INDIRECT_1700,			/* indirect_type */
	BTC_PSTDMA_FORCE_LPSOFF,		/* pstdma_type */
	BTC_BTRSSI_DBM,				/* bt_rssi_type */
	15,					/*.ant_isolation */
	2,					/*.rssi_tolerance */
	2,					/* rx_path_num */
	ARRAY_SIZE(wl_rssi_step_8733b),		/*.wl_rssi_step_num */
	wl_rssi_step_8733b,			/*.wl_rssi_step */
	ARRAY_SIZE(bt_rssi_step_8733b),		/*.bt_rssi_step_num */
	bt_rssi_step_8733b,			/*.bt_rssi_step */
	ARRAY_SIZE(table_sant_8733b),		/*.table_sant_num */
	table_sant_8733b,			/*.table_sant = */
	ARRAY_SIZE(table_nsant_8733b),		/*.table_nsant_num */
	table_nsant_8733b,			/*.table_nsant = */
	ARRAY_SIZE(tdma_sant_8733b),		/*.tdma_sant_num */
	tdma_sant_8733b,			/*.tdma_sant = */
	ARRAY_SIZE(tdma_nsant_8733b),		/*.tdma_nsant_num */
	tdma_nsant_8733b,			/*.tdma_nsant */
	ARRAY_SIZE(rf_para_tx_8733b),		/* wl_rf_para_tx_num */
	rf_para_tx_8733b,		        /* wl_rf_para_tx */
	rf_para_rx_8733b,		        /* wl_rf_para_rx */
	0x24,					/*.bt_afh_span_bw20 */
	0x36,					/*.bt_afh_span_bw40 */
	ARRAY_SIZE(afh_5g_8733b),		/*.afh_5g_num */
	afh_5g_8733b,				/*.afh_5g */
	halbtc8733b_chip_setup			/* chip_setup function */
};

void halbtc8733b_cfg_init(struct btc_coexist *btc)
{
	u32 scbd_32;

	/* enable TBTT nterrupt */
	btc->btc_write_1byte_bitmask(btc, 0x550, 0x8, 0x1);

	/* BT report packet sample rate	 */
	/* 0x790[5:0]=0x5 */
	btc->btc_write_1byte(btc, 0x790, 0x5);

	/* Enable BT counter statistics */
	btc->btc_write_1byte(btc, 0x778, 0x1);

	/* Enable PTA (3-wire function form BT side) */
	btc->btc_write_1byte_bitmask(btc, 0x40, 0x20, 0x1);
	btc->btc_write_1byte_bitmask(btc, 0x41, 0x02, 0x1);

	/* Enable PTA (tx/rx signal form WiFi side) */
	btc->btc_write_1byte_bitmask(btc, 0x4c6, BIT(4), 0x1);
	btc->btc_write_1byte_bitmask(btc, 0x4c6, BIT(5), 0x0);
	/*GNT_BT=1 while select both */
	btc->btc_write_1byte_bitmask(btc, 0x763, BIT(4), 0x1);

#if 1
	/* Standby mode setting = RX mode setting*/
	//btc->btc_set_rf_reg(btc, BTC_RF_A, 0xef, 0x80000, 0x1);
	//btc->btc_set_rf_reg(btc, BTC_RF_A, 0x33, 0xf, 0x1);
	//btc->btc_set_rf_reg(btc, BTC_RF_A, 0x3f, 0xfffff, 0x320a3);
	//btc->btc_set_rf_reg(btc, BTC_RF_A, 0xef, 0x80000, 0x0);
	btc->btc_set_rf_reg(btc, BTC_RF_B, 0xef, 0x80000, 0x1);
	btc->btc_set_rf_reg(btc, BTC_RF_B, 0x33, 0xf, 0x1);
	btc->btc_set_rf_reg(btc, BTC_RF_B, 0x3f, 0xfffff, 0x341a3);
	btc->btc_set_rf_reg(btc, BTC_RF_B, 0xef, 0x80000, 0x0);
#endif
	if (btc->board_info.btdm_ant_num == 2) {
		/* COEX-464: To avoid wifi reduce power when GNT_BT = 1 */
		btc->btc_write_1byte_bitmask(btc, 0x43a9, 0xf0, 0x0);
		btc->btc_write_1byte_bitmask(btc, 0x4304, BIT(7), 0x0);
	}

	/*For wifi/BT co-Rx*/
	btc->btc_write_scbd_32bit(btc, BIT(13), TRUE);

	/*Check if BT off, wifi write scbd[15] = 1 & reg setting; if BT on, no action*/
	if (!(btc->btc_read_scbd_32bit(btc, &scbd_32) & BIT(15))) {
		btc->btc_write_scbd_32bit(btc, BIT(15), TRUE);
		btc->btc_write_1byte_bitmask(btc, 0x1064, 0xC0, 0x2);
		btc->btc_write_1byte_bitmask(btc, 0x1065, BIT(0), 0x0);
		btc->btc_write_1byte_bitmask(btc, 0x1066, 0xE0, 0x1);
	}
}

void halbtc8733b_cfg_ant_switch(struct btc_coexist *btc)
{}

void halbtc8733b_cfg_gnt_fix(struct btc_coexist *btc)
{
}

void halbtc8733b_cfg_gnt_debug(struct btc_coexist *btc)
{
	btc->btc_write_1byte_bitmask(btc, 0x66, BIT(4), 0);
	btc->btc_write_1byte_bitmask(btc, 0x67, BIT(0), 0);
	btc->btc_write_1byte_bitmask(btc, 0x42, BIT(3), 0);
	btc->btc_write_1byte_bitmask(btc, 0x65, BIT(7), 0);

	//btc->btc_write_1byte_bitmask(btc, 0x10de, 0x1f, 0x1b);
	//btc->btc_write_1byte_bitmask(btc, 0x10df, 0x1f, 0x1b);
	//btc->btc_write_scbd(btc, BTC_SCBD_MAILBOX_DBG, TRUE);
}

void halbtc8733b_cfg_rfe_type(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	struct btc_board_info *board_info = &btc->board_info;

	rfe_type->rfe_module_type = board_info->rfe_type;
	rfe_type->ant_switch_polarity = 0;
	rfe_type->ant_switch_exist = FALSE;
	rfe_type->ant_switch_with_bt = FALSE;
	rfe_type->ant_switch_type = BTC_SWITCH_NONE;
	rfe_type->ant_switch_diversity = FALSE;

	rfe_type->band_switch_exist = FALSE;
	rfe_type->band_switch_type = 0;
	rfe_type->band_switch_polarity = 0;

	switch (rfe_type->rfe_module_type){
	case 2:
	case 4:
	case 9:
		rfe_type->wlg_at_btg = TRUE;
		if (coex_sta->kt_ver <= 2) {
			/*Before C cut, BT power cut mode to avoid BT cannot use*/
			btc->btc_set_bt_reg(btc, 0, 0x0e, 0x80c3);
			btc->btc_set_bt_reg(btc, 0, 0x22, 0xFF50);
			btc->btc_set_bt_reg(btc, 0, 0x0e, 0x00c3);
		}
		break;
	case 3:
		rfe_type->wlg_at_btg = FALSE;
		break;
	case 5:
		rfe_type->wlg_at_btg = FALSE;
		break;
	case 6:
		rfe_type->ant_switch_exist = TRUE;
		rfe_type->ant_switch_type = BTC_SWITCH_DPDT;
		break;
	case 7:
		rfe_type->ant_switch_exist = TRUE;
		rfe_type->ant_switch_type = BTC_SWITCH_DPDT;
		break;
	default:
		break;
	}

	coex_sta->rf4ce_en = FALSE;
}

void halbtc8733b_cfg_coexinfo_hw(struct btc_coexist *btc)
{
	u8 *cli_buf = btc->cli_buf, u8tmp[4];
	u16 u16tmp[4];
	u32 u32tmp[4];
	boolean lte_coex_on = FALSE;

	u32tmp[0] = btc->btc_read_4byte(btc, 0x764);
	u32tmp[1] = btc->btc_read_4byte(btc, 0x10dc);
	u8tmp[0] = btc->btc_read_1byte(btc, 0x73);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s",
		   "Path Owner",((u8tmp[0] & BIT(2)) ? "WL" : "BT"));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = RF:%s_BB:%s/ RF:%s_BB:%s/ 0x%x",
		   "GNT_WL_Ctrl/GNT_BT_Ctrl/0x10dc",
		   ((u8tmp[0] & BIT(4)) ? "SW" : "HW"),
		   ((u8tmp[0] & BIT(4)) ? "SW" : "HW"),
		   (((u8tmp[0] & BIT(4)) && (u32tmp[0] & (BIT(11) | BIT(12)))) ? "SW" : "HW"),
		   (((u8tmp[0] & BIT(4)) && (u32tmp[0] & (BIT(9) | BIT(10)))) ? "SW" : "HW"),
		   	u32tmp[1]);
	CL_PRINTF(cli_buf);
#if 0
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d",
		   "GNT_WL/GNT_BT", (int)((u32tmp[1] & BIT(2)) >> 2),
		   (int)((u32tmp[1] & BIT(3)) >> 3));
	CL_PRINTF(cli_buf);
#endif
	u32tmp[0] = btc->btc_read_4byte(btc, 0x1c38);
	u8tmp[0] = btc->btc_read_1byte(btc, 0x1860);
	u8tmp[1] = btc->btc_read_1byte(btc, 0x4160);
	u8tmp[2] = btc->btc_read_1byte(btc, 0x1c32);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ %d/ %d/ %d",
		   "1860[3]/4160[3]/1c30[22]/1c38[28]",
		   (int)((u8tmp[0] & BIT(3)) >> 3),
		   (int)((u8tmp[1] & BIT(3)) >> 3),
		   (int)((u8tmp[2] & BIT(6)) >> 6),
		   (int)((u32tmp[0] & BIT(28)) >> 28));
	CL_PRINTF(cli_buf);

	u32tmp[0] = btc->btc_read_4byte(btc, 0x430);
	u32tmp[1] = btc->btc_read_4byte(btc, 0x434);
	u16tmp[0] = btc->btc_read_2byte(btc, 0x42a);
	u16tmp[1] = btc->btc_read_1byte(btc, 0x454);
	u8tmp[0] = btc->btc_read_1byte(btc, 0x426);
	u8tmp[1] = btc->btc_read_1byte(btc, 0x45e);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x/ 0x%x/ 0x%x",
		   "430/434/42a/426/45e[3]/454",
		   u32tmp[0], u32tmp[1], u16tmp[0], u8tmp[0],
		   (int)((u8tmp[1] & BIT(3)) >> 3), u16tmp[1]);
	CL_PRINTF(cli_buf);

	u32tmp[0] = btc->btc_read_4byte(btc, 0x4c);
	u8tmp[2] = btc->btc_read_1byte(btc, 0x64);
	u8tmp[0] = btc->btc_read_1byte(btc, 0x4c6);
	u8tmp[1] = btc->btc_read_1byte(btc, 0x40);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x/ 0x%x",
		   "4c[24:23]/64[0]/4c6[4]/40[5]/RF_0x1",
		   (int)(u32tmp[0] & (BIT(24) | BIT(23))) >> 23, u8tmp[2] & 0x1,
		   (int)((u8tmp[0] & BIT(4)) >> 4),
		   (int)((u8tmp[1] & BIT(5)) >> 5),
		   (int)(btc->btc_get_rf_reg(btc, BTC_RF_B, 0x1, 0xfffff)));
	CL_PRINTF(cli_buf);

	u32tmp[0] = btc->btc_read_4byte(btc, 0x550);
	u8tmp[0] = btc->btc_read_1byte(btc, 0x522);
	u8tmp[1] = btc->btc_read_1byte(btc, 0x953);
	u8tmp[2] = btc->btc_read_1byte(btc, 0xc50);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ %s/ 0x%x",
		   "550/522/4-RxAGC/c50", u32tmp[0], u8tmp[0],
		   (u8tmp[1] & 0x2) ? "On" : "Off", u8tmp[2]);
	CL_PRINTF(cli_buf);

	u8tmp[0] = btc->btc_read_1byte(btc, 0x43a9);
	u8tmp[1] = btc->btc_read_1byte(btc, 0x4304);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x",
		   "43a9/4304[7]",
		   u8tmp[0], (int)((u8tmp[1] & BIT(7)) >> 7));
	CL_PRINTF(cli_buf);

	u8tmp[0] = btc->btc_read_1byte(btc, 0xf8e);
	u8tmp[1] = btc->btc_read_1byte(btc, 0xf8f);
	u8tmp[2] = btc->btc_read_1byte(btc, 0xd14);
	u8tmp[3] = btc->btc_read_1byte(btc, 0xd54);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "EVM_A/ EVM_B/ SNR_A/ SNR_B",
		   (u8tmp[0] > 127 ? u8tmp[0] - 256 : u8tmp[0]),
		   (u8tmp[1] > 127 ? u8tmp[1] - 256 : u8tmp[1]),
		   (u8tmp[2] > 127 ? u8tmp[2] - 256 : u8tmp[2]),
		   (u8tmp[3] > 127 ? u8tmp[3] - 256 : u8tmp[3]));
	CL_PRINTF(cli_buf);
}

void halbtc8733b_cfg_wl_tx_power(struct btc_coexist *btc)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	btc->btc_reduce_wl_tx_power(btc, coex_dm->cur_wl_pwr_lvl);
}

void halbtc8733b_cfg_wl_rx_gain(struct btc_coexist *btc)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 i;

	/* WL Rx Low gain on  */
	static const u32	wl_rx_gain_on_HT20[] = {0x00000000};
	static const u32	wl_rx_gain_on_HT40[] = {0x00000000};

	/* WL Rx Low gain off  */
	static const u32	wl_rx_gain_off_HT20[] = {0x00000000};
	static const u32	wl_rx_gain_off_HT40[] = {0x00000000};

	u32		*wl_rx_gain_on, *wl_rx_gain_off;

	if (coex_dm->cur_wl_rx_low_gain_en) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Hi-Li Table On!\n");
		BTC_TRACE(trace_buf);
#if 0
		if (link_info_ext->wifi_bw == BTC_WIFI_BW_HT40)
			wl_rx_gain_on = wl_rx_gain_on_HT40;
		else
			wl_rx_gain_on = wl_rx_gain_on_HT20;
		for (i = 0; i < ARRAY_SIZE(wl_rx_gain_on); i++)
			btc->btc_write_4byte(btc, 0x1d90, wl_rx_gain_on[i]);
#endif
		/* set Rx filter corner RCK offset */
		btc->btc_set_rf_reg(btc, BTC_RF_A, 0xde, 0x2, 0x1);
		btc->btc_set_rf_reg(btc, BTC_RF_A, 0x1d, 0x3f, 0x3F);
		btc->btc_set_rf_reg(btc, BTC_RF_B, 0xde, 0x2, 0x1);
		btc->btc_set_rf_reg(btc, BTC_RF_B, 0x1d, 0x3f, 0x3F);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Hi-Li Table Off!\n");
		BTC_TRACE(trace_buf);

#if 0
		if (link_info_ext->wifi_bw == BTC_WIFI_BW_HT40)
			wl_rx_gain_off = wl_rx_gain_off_HT40;
		else
			wl_rx_gain_off = wl_rx_gain_off_HT20;
		for (i = 0; i < ARRAY_SIZE(wl_rx_gain_off); i++)
			btc->btc_write_4byte(btc, 0x1d90, wl_rx_gain_off[i]);
#endif
		/* disable Rx filter corner RCK offset */
		btc->btc_set_rf_reg(btc, BTC_RF_A, 0xde, 0x2, 0x0);
		btc->btc_set_rf_reg(btc, BTC_RF_A, 0x1d, 0x3f, 0x0);
		btc->btc_set_rf_reg(btc, BTC_RF_B, 0xde, 0x2, 0x0);
		btc->btc_set_rf_reg(btc, BTC_RF_B, 0x1d, 0x3f, 0x0);
	}
}

void halbtc8733b_cfg_wlan_act_ips(struct btc_coexist *btc)
{}

void halbtc8733b_cfg_bt_ctrl_act(struct btc_coexist * btc)
{
	/*SW control GNT_BT high*/
	btc->btc_write_1byte_bitmask(btc, 0x765, 0x1e, 0xf);
	btc->btc_write_1byte(btc, 0x73, 0x74);
}

void halbtc8733b_chip_setup(struct btc_coexist *btc, u8 type)
{
	switch (type) {
	case BTC_CSETUP_INIT_HW:
		halbtc8733b_cfg_init(btc);
		break;
	case BTC_CSETUP_ANT_SWITCH:
		halbtc8733b_cfg_ant_switch(btc);
		break;
	case BTC_CSETUP_GNT_FIX:
		halbtc8733b_cfg_gnt_fix(btc);
		break;
	case BTC_CSETUP_GNT_DEBUG:
		halbtc8733b_cfg_gnt_debug(btc);
		break;
	case BTC_CSETUP_RFE_TYPE:
		halbtc8733b_cfg_rfe_type(btc);
		break;
	case BTC_CSETUP_COEXINFO_HW:
		halbtc8733b_cfg_coexinfo_hw(btc);
		break;
	case BTC_CSETUP_WL_TX_POWER:
		halbtc8733b_cfg_wl_tx_power(btc);
		break;
	case BTC_CSETUP_WL_RX_GAIN:
		halbtc8733b_cfg_wl_rx_gain(btc);
		break;
	case BTC_CSETUP_WLAN_ACT_IPS:
		halbtc8733b_cfg_wlan_act_ips(btc);
		break;
	case BTC_CSETUP_BT_CTRL_ACT:
		halbtc8733b_cfg_bt_ctrl_act(btc);
		break;
	}
}
#endif
