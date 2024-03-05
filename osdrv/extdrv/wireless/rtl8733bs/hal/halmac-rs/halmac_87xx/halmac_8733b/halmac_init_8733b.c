/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
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

#include "halmac_init_8733b.h"
#include "halmac_8733b_cfg.h"
#if HALMAC_PCIE_SUPPORT
#include "halmac_pcie_8733b.h"
#endif
#if HALMAC_SDIO_SUPPORT
#include "halmac_sdio_8733b.h"
#include "../halmac_sdio_87xx.h"
#endif
#if HALMAC_USB_SUPPORT
#include "halmac_usb_8733b.h"
#endif
#include "halmac_gpio_8733b.h"
#include "halmac_common_8733b.h"
#include "halmac_cfg_wmac_8733b.h"
#include "../halmac_common_87xx.h"
#include "../halmac_init_87xx.h"
#include "../halmac_cfg_wmac_87xx.h"

#if HALMAC_8733B_SUPPORT

#define SYS_FUNC_EN		0xD8

#define RSVD_PG_DRV_NUM			16
#ifdef CONFIG_FW_OFFLOAD_PARAM_INIT  //whether config 24 pages dynamically  jerry_zhou
#define RSVD_PG_H2C_EXTRAINFO_NUM	24
#else
#define RSVD_PG_H2C_EXTRAINFO_NUM	0
#endif
#define RSVD_PG_H2C_STATICINFO_NUM	8
#define RSVD_PG_H2CQ_NUM		0
#define RSVD_PG_CPU_INSTRUCTION_NUM	0
#define RSVD_PG_FW_TXBUF_NUM		0
#define RSVD_PG_CSIBUF_NUM		4
#define RSVD_PG_DLLB_NUM		(TX_FIFO_SIZE_8733B / 3 >> \
					TX_PAGE_SIZE_SHIFT_87XX)

#define MAC_TRX_ENABLE	(BIT_HCI_TXDMA_EN | BIT_HCI_RXDMA_EN | BIT_TXDMA_EN | \
			BIT_RXDMA_EN | BIT_PROTOCOL_EN | BIT_SCHEDULE_EN | \
			BIT_MACTXEN | BIT_MACRXEN)

#define WLAN_TXQ_RPT_EN		0x1F

#define BLK_DESC_NUM	0x3
#define RX_DLK_TIME	0x14

#define WLAN_SLOT_TIME		0x09
#define WLAN_PIFS_TIME		0x1C
#define WLAN_SIFS_CCK_CONT_TX	0x0A
#define WLAN_SIFS_OFDM_CONT_TX	0x0E
#define WLAN_SIFS_CCK_TRX	0x0A
#define WLAN_SIFS_OFDM_TRX	0x0E
#define WLAN_NAV_MAX		0xC8
#define WLAN_RDG_NAV		0x05
#define WLAN_TXOP_NAV		0x1B
#define WLAN_CCK_RX_TSF		0x30
#define WLAN_OFDM_RX_TSF	0x30
#define WLAN_TBTT_PROHIBIT	0x04 /* unit : 32us */
#define WLAN_TBTT_HOLD_TIME	0x064 /* unit : 32us */
#define WLAN_DRV_EARLY_INT	0x04
#define WLAN_BCN_CTRL_CLT0	0x10
#define WLAN_BCN_DMA_TIME	0x02
#define WLAN_BCN_MAX_ERR	0xFF
#define WLAN_SIFS_CCK_DUR_TUNE	0x0A
#define WLAN_SIFS_OFDM_DUR_TUNE	0x10
#define WLAN_SIFS_CCK_CTX	0x0A
#define WLAN_SIFS_CCK_IRX	0x0A
#define WLAN_SIFS_OFDM_CTX	0x0E
#define WLAN_SIFS_OFDM_IRX	0x0E
#define WLAN_EIFS_DUR_TUNE	0x40
#define WLAN_EDCA_VO_PARAM	0x002FA226
#define WLAN_EDCA_VI_PARAM	0x005EA328
#define WLAN_EDCA_BE_PARAM	0x005EA42B
#define WLAN_EDCA_BK_PARAM	0x0000A44F

#define WLAN_RX_FILTER0		0xFFFFFFFF
#define WLAN_RX_FILTER2		0xFFFF
#define WLAN_RCR_CFG		0xE410220E
#define WLAN_RXPKT_MAX_SZ	12288
#define WLAN_RXPKT_MAX_SZ_512	(WLAN_RXPKT_MAX_SZ >> 9)

#define WLAN_AMPDU_MAX_TIME		0x70
#define WLAN_RTS_LEN_TH			0xFF
#define WLAN_RTS_TX_TIME_TH		0x08
#define WLAN_MAX_AGG_PKT_LIMIT		0x3F
#define WLAN_RTS_MAX_AGG_PKT_LIMIT	0x20
//#define WLAN_PRE_TXCNT_TIME_TH		0x1E4
#define WLAN_PRE_TXCNT_TIME_TH		0x3A       //mac clk 20M£¬config around 3us
#define WALN_FAST_EDCA_VO_TH		0x06
#define WLAN_FAST_EDCA_VI_TH		0x06
#define WLAN_FAST_EDCA_BE_TH		0x06
#define WLAN_FAST_EDCA_BK_TH		0x06
#define WLAN_BAR_RETRY_LIMIT		0x01
#define WLAN_BAR_ACK_TYPE		0x05
#define WLAN_RA_TRY_RATE_AGG_LIMIT	0x08
#define WLAN_RESP_TXRATE		0x84
#define WLAN_ACK_TO			0x21
#define WLAN_ACK_TO_CCK			0x6A
#define WLAN_DATA_RATE_FB_CNT_1_4	0x01000000
#define WLAN_DATA_RATE_FB_CNT_5_8	0x08070504
#define WLAN_RTS_RATE_FB_CNT_5_8	0x08070504
#define WLAN_DATA_RATE_FB_RATE0		0xFE01F010
#define WLAN_DATA_RATE_FB_RATE0_H	0x40000000
#define WLAN_RTS_RATE_FB_RATE1		0x003FF010
#define WLAN_RTS_RATE_FB_RATE1_H	0x40000000
#define WLAN_RTS_RATE_FB_RATE4		0x0600F010
#define WLAN_RTS_RATE_FB_RATE4_H	0x400003E0
#define WLAN_RTS_RATE_FB_RATE5		0x0600F015
#define WLAN_RTS_RATE_FB_RATE5_H	0x000000E0

#define WLAN_TX_FUNC_CFG1		0x30
#define WLAN_TX_FUNC_CFG2		0x30
#define WLAN_MAC_OPT_NORM_FUNC1		0x98
#define WLAN_MAC_OPT_LB_FUNC1		0x80
#define WLAN_MAC_OPT_FUNC2		0xB1810041

#define WLAN_SIFS_CFG	(WLAN_SIFS_CCK_CONT_TX | \
			(WLAN_SIFS_OFDM_CONT_TX << BIT_SHIFT_SIFS_OFDM_CTX) | \
			(WLAN_SIFS_CCK_TRX << BIT_SHIFT_SIFS_CCK_TRX) | \
			(WLAN_SIFS_OFDM_TRX << BIT_SHIFT_SIFS_OFDM_TRX))

#define WLAN_SIFS_DUR_TUNE	(WLAN_SIFS_CCK_DUR_TUNE | \
				(WLAN_SIFS_OFDM_DUR_TUNE << 8))

#define WLAN_TBTT_TIME	(WLAN_TBTT_PROHIBIT |\
			(WLAN_TBTT_HOLD_TIME << BIT_SHIFT_TBTT_HOLD_TIME_AP))

#define WLAN_NAV_CFG		(WLAN_RDG_NAV | (WLAN_TXOP_NAV << 16))
#define WLAN_RX_TSF_CFG		(WLAN_CCK_RX_TSF | (WLAN_OFDM_RX_TSF) << 8)

#if HALMAC_PLATFORM_WINDOWS
/*SDIO RQPN Mapping for Windows, extra queue is not implemented in Driver code*/
static struct halmac_rqpn HALMAC_RQPN_SDIO_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};
#else
/*SDIO RQPN Mapping*/
static struct halmac_rqpn HALMAC_RQPN_SDIO_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};
#endif

/*PCIE RQPN Mapping*/
static struct halmac_rqpn HALMAC_RQPN_PCIE_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};

/*USB 2 Bulkout RQPN Mapping*/
static struct halmac_rqpn HALMAC_RQPN_2BULKOUT_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};

/*USB 3 Bulkout RQPN Mapping*/
static struct halmac_rqpn HALMAC_RQPN_3BULKOUT_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};

/*USB 4 Bulkout RQPN Mapping*/
static struct halmac_rqpn HALMAC_RQPN_4BULKOUT_8733B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM,
	 HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK,
	 HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ,
	 HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};

#if HALMAC_PLATFORM_WINDOWS
/*SDIO Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_SDIO_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 4, 4, 4, 0, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 8, 0, 1},
};
#else
/*SDIO Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_SDIO_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 12, 2, 2, 0, 0},
	{HALMAC_TRX_MODE_TRXSHARE, 4, 4, 4, 4, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 8, 8, 1},
};
#endif

/*PCIE Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_PCIE_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 8, 8, 1},
};

/*USB 2 Bulkout Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_2BULKOUT_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 8, 8, 0, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 8, 8, 0, 0, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 0, 0, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 0, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 0, 0, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 0, 0, 1},
};

/*USB 3 Bulkout Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_3BULKOUT_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 8, 0, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 8, 0, 1},
};

/*USB 4 Bulkout Page Number*/
static struct halmac_pg_num HALMAC_PG_NUM_4BULKOUT_8733B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_WMM, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_P2P, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 8, 8, 8, 8, 1},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 8, 8, 8, 8, 1},
};

static enum halmac_ret_status
txdma_queue_mapping_8733b(struct halmac_adapter *adapter,
			  enum halmac_trx_mode mode);

static enum halmac_ret_status
priority_queue_cfg_8733b(struct halmac_adapter *adapter,
			 enum halmac_trx_mode mode);

static enum halmac_ret_status
set_trx_fifo_info_8733b(struct halmac_adapter *adapter,
			enum halmac_trx_mode mode);

static void
init_txq_ctrl_8733b(struct halmac_adapter *adapter);

static void
init_sifs_ctrl_8733b(struct halmac_adapter *adapter);

static void
init_rate_fallback_ctrl_8733b(struct halmac_adapter *adapter);

u8 CRC5_USB(unsigned char *in,unsigned long byte_num);

enum halmac_ret_status
mount_api_8733b(struct halmac_adapter *adapter)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	adapter->chip_id = HALMAC_CHIP_ID_8733B;
	adapter->hw_cfg_info.efuse_size = EFUSE_SIZE_8733B;
	adapter->hw_cfg_info.eeprom_size = EEPROM_SIZE_8733B;
	adapter->hw_cfg_info.bt_efuse_size = BT_EFUSE_SIZE_8733B;
	adapter->hw_cfg_info.prtct_efuse_size = PRTCT_EFUSE_SIZE_8733B;
	adapter->hw_cfg_info.cam_entry_num = SEC_CAM_NUM_8733B;
	adapter->hw_cfg_info.tx_fifo_size = TX_FIFO_SIZE_8733B;
	adapter->hw_cfg_info.rx_fifo_size = RX_FIFO_SIZE_8733B;
	adapter->hw_cfg_info.ac_oqt_size = OQT_ENTRY_AC_8733B;
	adapter->hw_cfg_info.non_ac_oqt_size = OQT_ENTRY_NOAC_8733B;
	adapter->hw_cfg_info.usb_txagg_num = BLK_DESC_NUM;
	adapter->txff_alloc.rsvd_drv_pg_num = RSVD_PG_DRV_NUM;

	api->halmac_init_trx_cfg = init_trx_cfg_8733b;
	api->halmac_init_system_cfg = init_system_cfg_8733b;
	api->halmac_init_protocol_cfg = init_protocol_cfg_8733b;
	api->halmac_init_h2c = init_h2c_8733b;
	api->halmac_pinmux_get_func = pinmux_get_func_8733b;
	api->halmac_pinmux_set_func = pinmux_set_func_8733b;
	api->halmac_pinmux_free_func = pinmux_free_func_8733b;
	api->halmac_get_hw_value = get_hw_value_8733b;
	api->halmac_set_hw_value = set_hw_value_8733b;
	api->halmac_cfg_drv_info = cfg_drv_info_8733b;
	api->halmac_fill_txdesc_checksum = fill_txdesc_check_sum_8733b;
	api->halmac_init_low_pwr = init_low_pwr_8733b;
	api->halmac_pre_init_system_cfg = pre_init_system_cfg_8733b;

	api->halmac_init_wmac_cfg = init_wmac_cfg_8733b;
	api->halmac_init_edca_cfg = init_edca_cfg_8733b;

	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
#if HALMAC_SDIO_SUPPORT
		adapter->sdio_hw_info.tx_addr_format = HALMAC_SDIO_AGG_MODE;
		api->halmac_init_interface_cfg = init_sdio_cfg_8733b;
		api->halmac_init_sdio_cfg = init_sdio_cfg_8733b;
		api->halmac_mac_power_switch = mac_pwr_switch_sdio_8733b;
		api->halmac_phy_cfg = phy_cfg_sdio_8733b;
		api->halmac_pcie_switch = pcie_switch_sdio_8733b;
		api->halmac_interface_integration_tuning = intf_tun_sdio_8733b;
		api->halmac_tx_allowed_sdio = tx_allowed_sdio_8733b;
		api->halmac_get_sdio_tx_addr = get_sdio_tx_addr_8733b;
		api->halmac_reg_read_8 = reg_r8_sdio_8733b;
		api->halmac_reg_write_8 = reg_w8_sdio_8733b;
		api->halmac_reg_read_16 = reg_r16_sdio_8733b;
		api->halmac_reg_write_16 = reg_w16_sdio_8733b;
		api->halmac_reg_read_32 = reg_r32_sdio_8733b;
		api->halmac_reg_write_32 = reg_w32_sdio_8733b;

		adapter->sdio_fs.macid_map_size = MACID_MAX_8733B << 1;
		if (!adapter->sdio_fs.macid_map) {
			adapter->sdio_fs.macid_map =
			(u8 *)PLTFM_MALLOC(adapter->sdio_fs.macid_map_size);
			if (!adapter->sdio_fs.macid_map)
				PLTFM_MSG_ERR("[ERR]mac id map malloc!!\n");
		}
#endif
	} else if (adapter->intf == HALMAC_INTERFACE_USB) {
#if HALMAC_USB_SUPPORT
		api->halmac_mac_power_switch = mac_pwr_switch_usb_8733b;
		api->halmac_phy_cfg = phy_cfg_usb_8733b;
		api->halmac_pcie_switch = pcie_switch_usb_8733b;
		api->halmac_interface_integration_tuning = intf_tun_usb_8733b;
#endif
	} else if (adapter->intf == HALMAC_INTERFACE_PCIE) {
#if HALMAC_PCIE_SUPPORT
		api->halmac_mac_power_switch = mac_pwr_switch_pcie_8733b;
		api->halmac_phy_cfg = phy_cfg_pcie_8733b;
		api->halmac_pcie_switch = pcie_switch_8733b;
		api->halmac_interface_integration_tuning = intf_tun_pcie_8733b;
		api->halmac_cfgspc_set_pcie = cfgspc_set_pcie_8733b;
#endif
	} else {
		PLTFM_MSG_ERR("[ERR]Undefined IC\n");
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * init_trx_cfg_8733b() - config trx dma register
 * @adapter : the adapter of halmac
 * @mode : trx mode selection
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_trx_cfg_8733b(struct halmac_adapter *adapter, enum halmac_trx_mode mode)
{
	u8 value8;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	u8 en_fwff;
	u16 value16;

	adapter->trx_mode = mode;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	status = txdma_queue_mapping_8733b(adapter, mode);
	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]queue mapping\n");
		return status;
	}

	/*
	en_fwff = HALMAC_REG_R8(REG_WMAC_FWPKT_CR) & BIT_FWEN;
	if (en_fwff) {
		HALMAC_REG_W8_CLR(REG_WMAC_FWPKT_CR, BIT_FWEN);
		if (fwff_is_empty_87xx(adapter) != HALMAC_RET_SUCCESS)
			PLTFM_MSG_ERR("[ERR]fwff is not empty\n");
	}
	*/
	value8 = 0;
	HALMAC_REG_W8(REG_CR, value8);

	//value16 = HALMAC_REG_R16(REG_FWFF_PKT_INFO);
	//HALMAC_REG_W16(REG_FWFF_CTRL, value16);

	value8 = MAC_TRX_ENABLE;
	HALMAC_REG_W8(REG_CR, value8);

	/*
	if (en_fwff)
		HALMAC_REG_W8_SET(REG_WMAC_FWPKT_CR, BIT_FWEN);
	HALMAC_REG_W32(REG_H2CQ_CSR, BIT(31));
	*/

	status = priority_queue_cfg_8733b(adapter, mode);
	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]priority queue cfg\n");
		return status;
	}

	/*  yx_qi
	status = init_h2c_8733b(adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]init h2cq!\n");
		return status;
	}*/

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
txdma_queue_mapping_8733b(struct halmac_adapter *adapter,
			  enum halmac_trx_mode mode)
{
	u16 value16;
	struct halmac_rqpn *cur_rqpn_sel = NULL;
	enum halmac_ret_status status;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
		cur_rqpn_sel = HALMAC_RQPN_SDIO_8733B;
	} else if (adapter->intf == HALMAC_INTERFACE_PCIE) {
		cur_rqpn_sel = HALMAC_RQPN_PCIE_8733B;
	} else if (adapter->intf == HALMAC_INTERFACE_USB) {
		if (adapter->bulkout_num == 2) {
			cur_rqpn_sel = HALMAC_RQPN_2BULKOUT_8733B;
		} else if (adapter->bulkout_num == 3) {
			cur_rqpn_sel = HALMAC_RQPN_3BULKOUT_8733B;
		} else if (adapter->bulkout_num == 4) {
			cur_rqpn_sel = HALMAC_RQPN_4BULKOUT_8733B;
		} else {
			PLTFM_MSG_ERR("[ERR]interface not support\n");
			return HALMAC_RET_NOT_SUPPORT;
		}
	} else {
		return HALMAC_RET_NOT_SUPPORT;
	}

	status = rqpn_parser_87xx(adapter, mode, cur_rqpn_sel);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	value16 = 0;
	value16 |= BIT_TXDMA_HIQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_HI]);
	value16 |= BIT_TXDMA_MGQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_MG]);
	value16 |= BIT_TXDMA_BKQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_BK]);
	value16 |= BIT_TXDMA_BEQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_BE]);
	value16 |= BIT_TXDMA_VIQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_VI]);
	value16 |= BIT_TXDMA_VOQ_MAP(adapter->pq_map[HALMAC_PQ_MAP_VO]);
	HALMAC_REG_W16(REG_TXDMA_PQ_MAP, value16);

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
priority_queue_cfg_8733b(struct halmac_adapter *adapter,
			 enum halmac_trx_mode mode)
{
	u8 transfer_mode = 0;
	u8 value8;
	u32 cnt;
	struct halmac_txff_allocation *txff_info = &adapter->txff_alloc;
	enum halmac_ret_status status;
	struct halmac_pg_num *cur_pg_num = NULL;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	status = set_trx_fifo_info_8733b(adapter, mode);
	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]set trx fifo!!\n");
		return status;
	}

	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
		cur_pg_num = HALMAC_PG_NUM_SDIO_8733B;
	} else if (adapter->intf == HALMAC_INTERFACE_PCIE) {
		cur_pg_num = HALMAC_PG_NUM_PCIE_8733B;
	} else if (adapter->intf == HALMAC_INTERFACE_USB) {
		if (adapter->bulkout_num == 2) {
			cur_pg_num = HALMAC_PG_NUM_2BULKOUT_8733B;
		} else if (adapter->bulkout_num == 3) {
			cur_pg_num = HALMAC_PG_NUM_3BULKOUT_8733B;
		} else if (adapter->bulkout_num == 4) {
			cur_pg_num = HALMAC_PG_NUM_4BULKOUT_8733B;
		} else {
			PLTFM_MSG_ERR("[ERR]interface not support\n");
			return HALMAC_RET_NOT_SUPPORT;
		}
	} else {
		return HALMAC_RET_NOT_SUPPORT;
	}

	status = pg_num_parser_87xx(adapter, mode, cur_pg_num);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	PLTFM_MSG_TRACE("[TRACE]Set FIFO page\n");

	//HALMAC_REG_W16(REG_FIFOPAGE_INFO_1, txff_info->high_queue_pg_num);
	//HALMAC_REG_W16(REG_FIFOPAGE_INFO_2, txff_info->low_queue_pg_num);
	//HALMAC_REG_W16(REG_FIFOPAGE_INFO_3, txff_info->normal_queue_pg_num);
	//HALMAC_REG_W16(REG_FIFOPAGE_INFO_4, txff_info->extra_queue_pg_num);
	//HALMAC_REG_W16(REG_FIFOPAGE_INFO_5, txff_info->pub_queue_pg_num);
	//HALMAC_REG_W32_SET(REG_RQPN_CTRL_2, BIT(31));
	HALMAC_REG_W32(REG_RQPN_CTRL_HLPQ, ((txff_info->pub_queue_pg_num)<<16) + ((txff_info->low_queue_pg_num)<<8) + txff_info->high_queue_pg_num );
	HALMAC_REG_W32(REG_RQPN_NPQ, ((txff_info->extra_queue_pg_num)<<16) + txff_info->normal_queue_pg_num );
	HALMAC_REG_W8(REG_RQPN_CTRL_HLPQ+3, 0x80);

	

	adapter->sdio_fs.hiq_pg_num = txff_info->high_queue_pg_num;
	adapter->sdio_fs.miq_pg_num = txff_info->normal_queue_pg_num;
	adapter->sdio_fs.lowq_pg_num = txff_info->low_queue_pg_num;
	adapter->sdio_fs.pubq_pg_num = txff_info->pub_queue_pg_num;
	adapter->sdio_fs.exq_pg_num = txff_info->extra_queue_pg_num;

	//HALMAC_REG_W16(REG_FIFOPAGE_CTRL_2, txff_info->rsvd_boundary);
	//HALMAC_REG_W16(REG_WMAC_CSIDMA_CFG, txff_info->rsvd_csibuf_addr);
	//HALMAC_REG_W8_SET(REG_FWHW_TXQ_CTRL + 2, BIT(4));
	HALMAC_REG_W8(REG_DWBCN0_CTRL+1, (u8)(txff_info->rsvd_boundary));

	/*20170411 Soar*/
	/* SDIO sometimes use two CMD52 to do HALMAC_REG_W16 */
	/* and may cause a mismatch between HW status and Reg value. */
	/* A patch is to write high byte first, suggested by Argis */
	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
		value8 = (u8)(txff_info->rsvd_boundary >> 8 & 0xFF);
		HALMAC_REG_W8(REG_BCNQ_BDNY + 1, value8);
		value8 = (u8)(txff_info->rsvd_boundary & 0xFF);
		HALMAC_REG_W8(REG_BCNQ_BDNY, value8);
	} else {
		//HALMAC_REG_W16(REG_BCNQ_BDNY_V1, txff_info->rsvd_boundary);
		HALMAC_REG_W8(REG_BCNQ_BDNY, (u8)txff_info->rsvd_boundary);
	}

	//HALMAC_REG_W16(REG_FIFOPAGE_CTRL_2 + 2, txff_info->rsvd_boundary);

	/*20170411 Soar*/
	/* SDIO sometimes use two CMD52 to do HALMAC_REG_W16 */
	/* and may cause a mismatch between HW status and Reg value. */
	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
		//value8 = (u8)(txff_info->rsvd_boundary >> 8 & 0xFF);
		//HALMAC_REG_W8(REG_BCNQ_BDNY + 1, value8);
		value8 = (u8)(txff_info->rsvd_boundary & 0xFF);
		HALMAC_REG_W8(REG_BCNQ2_BDNY, value8);
	} else {
		//HALMAC_REG_W16(REG_BCNQ1_BDNY_V1, txff_info->rsvd_boundary);
		HALMAC_REG_W8(REG_BCNQ2_BDNY, (u8)(txff_info->rsvd_boundary));
	}

	//HALMAC_REG_W32(REG_RXFF_BNDY, adapter->hw_cfg_info.rx_fifo_size -	 C2H_PKT_BUF_87XX - 1);
	HALMAC_REG_W16(0x116,(u16)(adapter->hw_cfg_info.rx_fifo_size - C2H_PKT_BUF_87XX - 1));

	if (adapter->intf == HALMAC_INTERFACE_USB) {
		//value8 = HALMAC_REG_R8(REG_AUTO_LLT_V1);
		//value8 &= ~(BIT_MASK_BLK_DESC_NUM << BIT_SHIFT_BLK_DESC_NUM);
		//value8 |= (BLK_DESC_NUM << BIT_SHIFT_BLK_DESC_NUM);
		//HALMAC_REG_W8(REG_AUTO_LLT_V1, value8);

		//HALMAC_REG_W8(REG_AUTO_LLT_V1 + 3, BLK_DESC_NUM);
		value8 = HALMAC_REG_R8(REG_DWBCN0_CTRL);
		value8 &= 0x0f;
		value8 |= BLK_DESC_NUM<<4;
		HALMAC_REG_W8(REG_DWBCN0_CTRL, value8);
		HALMAC_REG_W8_SET(REG_TXDMA_OFFSET_CHK + 1, BIT(1));
	}
	HALMAC_REG_W32_SET(REG_AUTO_LLT, BIT(16));
	cnt = 1000;
	while (HALMAC_REG_R32(REG_AUTO_LLT) & BIT(16)) {
		cnt--;
		if (cnt == 0)
			return HALMAC_RET_INIT_LLT_FAIL;
	}

	if (mode == HALMAC_TRX_MODE_DELAY_LOOPBACK) {
		transfer_mode = HALMAC_TRNSFER_LOOPBACK_DELAY;
		HALMAC_REG_W8(REG_WMAC_LBK_BUF_HD,
			       (u8)(adapter->txff_alloc.rsvd_boundary&0xff));   //REG_WMAC_LBK_BUF_HD_V1, yx_qi
	} else if (mode == HALMAC_TRX_MODE_LOOPBACK) {
		transfer_mode = HALMAC_TRNSFER_LOOPBACK_DIRECT;
	} else {
		transfer_mode = HALMAC_TRNSFER_NORMAL;
	}

	adapter->hw_cfg_info.trx_mode = transfer_mode;
	HALMAC_REG_W8(REG_CR + 3, (u8)transfer_mode);


	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
set_trx_fifo_info_8733b(struct halmac_adapter *adapter,
			enum halmac_trx_mode mode)
{
	u16 cur_pg_addr;
	u32 txff_size = TX_FIFO_SIZE_8733B;
	u32 rxff_size = RX_FIFO_SIZE_8733B;
	struct halmac_txff_allocation *info = &adapter->txff_alloc;

	if (info->rx_fifo_exp_mode == HALMAC_RX_FIFO_EXPANDING_MODE_1_BLOCK) {
		txff_size = TX_FIFO_SIZE_RX_EXPAND_1BLK_8733B;
		rxff_size = RX_FIFO_SIZE_RX_EXPAND_1BLK_8733B;
	} else if (info->rx_fifo_exp_mode ==
		   HALMAC_RX_FIFO_EXPANDING_MODE_2_BLOCK) {
		txff_size = TX_FIFO_SIZE_RX_EXPAND_2BLK_8733B;
		rxff_size = RX_FIFO_SIZE_RX_EXPAND_2BLK_8733B;
	} else if (info->rx_fifo_exp_mode ==
		   HALMAC_RX_FIFO_EXPANDING_MODE_3_BLOCK) {
		txff_size = TX_FIFO_SIZE_RX_EXPAND_3BLK_8733B;
		rxff_size = RX_FIFO_SIZE_RX_EXPAND_3BLK_8733B;
	} else if (info->rx_fifo_exp_mode ==
		   HALMAC_RX_FIFO_EXPANDING_MODE_4_BLOCK) {
		txff_size = TX_FIFO_SIZE_RX_EXPAND_4BLK_8733B;
		rxff_size = RX_FIFO_SIZE_RX_EXPAND_4BLK_8733B;
	}

	if (info->la_mode != HALMAC_LA_MODE_DISABLE) {
		txff_size = TX_FIFO_SIZE_LA_8733B;
		rxff_size = RX_FIFO_SIZE_8733B;
	}

	adapter->hw_cfg_info.tx_fifo_size = txff_size;
	adapter->hw_cfg_info.rx_fifo_size = rxff_size;
	info->tx_fifo_pg_num = (u16)(txff_size >> TX_PAGE_SIZE_SHIFT_87XX);

	info->rsvd_pg_num = info->rsvd_drv_pg_num +
					RSVD_PG_H2C_EXTRAINFO_NUM +
					RSVD_PG_H2C_STATICINFO_NUM +
					RSVD_PG_H2CQ_NUM +
					RSVD_PG_CPU_INSTRUCTION_NUM +
					RSVD_PG_FW_TXBUF_NUM +
					RSVD_PG_CSIBUF_NUM;

	if (mode == HALMAC_TRX_MODE_DELAY_LOOPBACK)
		info->rsvd_pg_num += RSVD_PG_DLLB_NUM;

	if (info->rsvd_pg_num > info->tx_fifo_pg_num)
		return HALMAC_RET_CFG_TXFIFO_PAGE_FAIL;

	info->acq_pg_num = info->tx_fifo_pg_num - info->rsvd_pg_num;
	info->rsvd_boundary = info->tx_fifo_pg_num - info->rsvd_pg_num;

	cur_pg_addr = info->tx_fifo_pg_num;
	cur_pg_addr -= RSVD_PG_CSIBUF_NUM;
	info->rsvd_csibuf_addr = cur_pg_addr;
	cur_pg_addr -= RSVD_PG_FW_TXBUF_NUM;
	info->rsvd_fw_txbuf_addr = cur_pg_addr;
	cur_pg_addr -= RSVD_PG_CPU_INSTRUCTION_NUM;
	info->rsvd_cpu_instr_addr = cur_pg_addr;
	cur_pg_addr -= RSVD_PG_H2CQ_NUM;
	info->rsvd_h2cq_addr = cur_pg_addr;
	cur_pg_addr -= RSVD_PG_H2C_STATICINFO_NUM;
	info->rsvd_h2c_sta_info_addr = cur_pg_addr;
	cur_pg_addr -= RSVD_PG_H2C_EXTRAINFO_NUM;
	info->rsvd_h2c_info_addr = cur_pg_addr;
	cur_pg_addr -= info->rsvd_drv_pg_num;
	info->rsvd_drv_addr = cur_pg_addr;

	if (mode == HALMAC_TRX_MODE_DELAY_LOOPBACK)
		info->rsvd_drv_addr -= RSVD_PG_DLLB_NUM;

	if (info->rsvd_boundary != info->rsvd_drv_addr)
		return HALMAC_RET_CFG_TXFIFO_PAGE_FAIL;

	return HALMAC_RET_SUCCESS;
}

/**
 * init_system_cfg_8733b() -  init system config
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_system_cfg_8733b(struct halmac_adapter *adapter)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	u8 value8;
	u32 tmp = 0;
	u32 value32;
	enum halmac_ret_status status;
	u8 hwval;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (adapter->intf == HALMAC_INTERFACE_PCIE) {
		hwval = 1;
		status = set_hw_value_8733b(adapter, HALMAC_HW_PCIE_REF_AUTOK,
					    &hwval);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}

	value32 = HALMAC_REG_R32(REG_EXT_SYS_FUNC_EN);
	value32 |= (BIT_PLFM_FUNC_EN | BIT_DDMA_FUNC_EN);
	HALMAC_REG_W32(REG_EXT_SYS_FUNC_EN, value32);

	value8 = HALMAC_REG_R8(REG_SYS_FUNC_EN + 1) | SYS_FUNC_EN;
	HALMAC_REG_W8(REG_SYS_FUNC_EN + 1, value8);

	/*PHY_REQ_DELAY reg 0x1100[27:24] = 0x0C*/
	value8 = (HALMAC_REG_R8(REG_CR_EXT + 3) & 0xF0) | 0x0C;
	HALMAC_REG_W8(REG_CR_EXT + 3, value8);

	/*disable boot-from-flash for driver's DL FW*/
	/*
	tmp = HALMAC_REG_R32(REG_MCUFW_CTRL);
	if (tmp & BIT_BOOT_FSPI_EN) {
		HALMAC_REG_W32(REG_MCUFW_CTRL, tmp & (~BIT_BOOT_FSPI_EN));
		value32 = HALMAC_REG_R32(REG_GPIO_MUXCFG) & (~BIT_FSPI_EN);
		HALMAC_REG_W32(REG_GPIO_MUXCFG, value32);
	}
	*/
	
	/*
	if (adapter->chip_ver == HALMAC_CHIP_VER_B_CUT) {
		value8 = HALMAC_REG_R8(REG_ANAPAR_MAC_0);
		value8 = value8 & ~(BITS_LDO_VSEL);
		HALMAC_REG_W8(REG_ANAPAR_MAC_0, value8);
	}*/

	//jerry_zhou
	HALMAC_REG_W8(REG_SYS_SDIO_CTRL+3,0x4);//wifi bt PTA

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * init_protocol_cfg_8733b() - config protocol register
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_protocol_cfg_8733b(struct halmac_adapter *adapter)
{
	u32 max_agg_num;
	u32 max_rts_agg_num;
	u32 value32;
	u16 pre_txcnt;
	u8 value8;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	init_txq_ctrl_8733b(adapter);
	init_sifs_ctrl_8733b(adapter);
	init_rate_fallback_ctrl_8733b(adapter);

	HALMAC_REG_W8(REG_AMPDU_MAX_TIME, WLAN_AMPDU_MAX_TIME);
	HALMAC_REG_W8_SET(REG_TX_HANG_CTRL, BIT_R_EOF_EN);

	pre_txcnt = WLAN_PRE_TXCNT_TIME_TH | BIT_PRETX_AGGR_EN;
	HALMAC_REG_W8(REG_PRECNT_CTRL, (u16)(pre_txcnt & 0xFF));
	HALMAC_REG_W8(REG_PRECNT_CTRL + 1, (u16)(pre_txcnt >> 8));

	max_agg_num = WLAN_MAX_AGG_PKT_LIMIT;
	max_rts_agg_num = WLAN_RTS_MAX_AGG_PKT_LIMIT;
	value32 = WLAN_RTS_LEN_TH | (WLAN_RTS_TX_TIME_TH << 8) |
				(max_agg_num << 16) | (max_rts_agg_num << 24);
	HALMAC_REG_W32(REG_PROT_MODE_CTRL, value32);

	HALMAC_REG_W16(REG_BAR_MODE_CTRL + 2,
		       WLAN_BAR_RETRY_LIMIT | WLAN_RA_TRY_RATE_AGG_LIMIT << 8);

	HALMAC_REG_W8(REG_FAST_EDCA_VOVI_SETTING, WALN_FAST_EDCA_VO_TH);
	HALMAC_REG_W8(REG_FAST_EDCA_VOVI_SETTING + 2, WLAN_FAST_EDCA_VI_TH);
	HALMAC_REG_W8(REG_FAST_EDCA_BEBK_SETTING, WLAN_FAST_EDCA_BE_TH);
	HALMAC_REG_W8(REG_FAST_EDCA_BEBK_SETTING + 2, WLAN_FAST_EDCA_BK_TH);

	/*close A/B/C/D-cut BA parser*/
	HALMAC_REG_W8_CLR(REG_LIFETIME_EN, BIT(5));

	/*Bypass TXBF error protection due to sounding failure*/
	value32 = HALMAC_REG_R32(REG_BF0_TIME_SETTING) & (~BIT_EN_BF0_UPDATE);//bit29
	HALMAC_REG_W32(REG_BF0_TIME_SETTING, value32 | BIT_EN_BF0_TIMER);
	value32 = HALMAC_REG_R32(REG_BF1_TIME_SETTING) & (~BIT_EN_BF1_UPDATE);//bit29
	HALMAC_REG_W32(REG_BF1_TIME_SETTING, value32 | BIT_EN_BF1_TIMER);
	value32 = HALMAC_REG_R32(REG_BF_TIMEOUT_EN) & (~BIT_BF0_TIMEOUT_EN) &
		 (~BIT_BF1_TIMEOUT_EN);
	HALMAC_REG_W32(REG_BF_TIMEOUT_EN, value32);

	/*Fix incorrect HW default value of RSC*/
	value32 = BIT_CLEAR_RRSR_RSC(HALMAC_REG_R32(REG_RRSR)); 
	HALMAC_REG_W32(REG_RRSR, value32);

	value8 = HALMAC_REG_R8(REG_INIRTS_RATE_SEL);
	HALMAC_REG_W8(REG_INIRTS_RATE_SEL, value8 | BIT(5));

	//jerry_zhou
	value8=HALMAC_REG_R8(REG_CCK_CHECK);
	value8=value8 & (~(BIT(3)))& (~(BIT(6)));
	HALMAC_REG_W8(REG_CCK_CHECK,value8);//for driver not update beacon every TBTT

	value8=HALMAC_REG_R8(REG_FWHW_TXQ_CTRL+2);
	value8=value8| BIT(2);
	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL+2,value8);
	
	//for HW update SSN,if tx desc EN_HWSEQ=1,seq update by hw,else,seq update by sw
	//beacon update seq by hw only according 0x423[6]=1
	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL+3,0xFF);

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * init_h2c_8733b() - config h2c packet buffer
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_h2c_8733b(struct halmac_adapter *adapter)
{
	u8 value8;
	u32 value32;
	u32 h2cq_addr;
	u32 h2cq_size;
	struct halmac_txff_allocation *txff_info = &adapter->txff_alloc;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	h2cq_addr = txff_info->rsvd_h2cq_addr << TX_PAGE_SIZE_SHIFT_87XX;
	h2cq_size = RSVD_PG_H2CQ_NUM << TX_PAGE_SIZE_SHIFT_87XX;

	value32 = HALMAC_REG_R32(REG_H2C_HEAD);
	value32 = (value32 & 0xFFFC0000) | h2cq_addr;
	HALMAC_REG_W32(REG_H2C_HEAD, value32);

	value32 = HALMAC_REG_R32(REG_H2C_READ_ADDR);
	value32 = (value32 & 0xFFFC0000) | h2cq_addr;
	HALMAC_REG_W32(REG_H2C_READ_ADDR, value32);

	value32 = HALMAC_REG_R32(REG_H2C_TAIL);
	value32 = (value32 & 0xFFFC0000);
	value32 |= (h2cq_addr + h2cq_size);
	HALMAC_REG_W32(REG_H2C_TAIL, value32);

	value8 = HALMAC_REG_R8(REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFC) | 0x01);
	HALMAC_REG_W8(REG_H2C_INFO, value8);

	value8 = HALMAC_REG_R8(REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFB) | 0x04);
	HALMAC_REG_W8(REG_H2C_INFO, value8);

	value8 = HALMAC_REG_R8(REG_TXDMA_OFFSET_CHK + 1);
	value8 = (u8)((value8 & 0x7f) | 0x80);
	HALMAC_REG_W8(REG_TXDMA_OFFSET_CHK + 1, value8);

	adapter->h2c_info.buf_size = h2cq_size;
	get_h2c_buf_free_space_87xx(adapter);

	if (adapter->h2c_info.buf_size != adapter->h2c_info.buf_fs) {
		PLTFM_MSG_ERR("[ERR]get h2c free space error!\n");
		return HALMAC_RET_GET_H2C_SPACE_ERR;
	}

	PLTFM_MSG_TRACE("[TRACE]h2c fs : %d\n", adapter->h2c_info.buf_fs);

	return HALMAC_RET_SUCCESS;
}

/**
 * init_edca_cfg_8733b() - init EDCA config
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_edca_cfg_8733b(struct halmac_adapter *adapter)
{
	u8 value8;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	HALMAC_REG_W32(REG_EDCA_VO_PARAM, WLAN_EDCA_VO_PARAM);
	HALMAC_REG_W32(REG_EDCA_VI_PARAM, WLAN_EDCA_VI_PARAM);
	HALMAC_REG_W32(REG_EDCA_BE_PARAM, WLAN_EDCA_BE_PARAM);
	HALMAC_REG_W32(REG_EDCA_BK_PARAM, WLAN_EDCA_BK_PARAM);

	HALMAC_REG_W8(REG_PIFS, WLAN_PIFS_TIME);

	HALMAC_REG_W8_CLR(REG_TX_PTCL_CTRL + 1, BIT(4));

	value8 = HALMAC_REG_R8(REG_RD_CTRL + 1);
	value8 = (value8 | BIT(0) | BIT(1) | BIT(2));
	HALMAC_REG_W8(REG_RD_CTRL + 1, value8);

	cfg_mac_clk_87xx(adapter);

	value8 = HALMAC_REG_R8(REG_MISC_CTRL);
	value8 = (value8 | BIT(3) | BIT(1) | BIT(0));
	HALMAC_REG_W8(REG_MISC_CTRL, value8);

	/* Init SYNC_CLI_SEL : reg 0x5B4[6:4] = 0 */
	//HALMAC_REG_W8_CLR(REG_TIMER0_SRC_SEL, BIT(4) | BIT(5) | BIT(6));   //yx_qi

	/* Clear TX pause */
	HALMAC_REG_W16(REG_TXPAUSE, 0x0000);

	HALMAC_REG_W8(REG_SLOT, WLAN_SLOT_TIME);

	HALMAC_REG_W32(REG_RD_NAV_NXT, WLAN_NAV_CFG);
	HALMAC_REG_W16(REG_RXTSF_OFFSET_CCK, WLAN_RX_TSF_CFG);

	/* Set beacon cotnrol - enable TSF and other related functions */
	HALMAC_REG_W8(REG_BCN_CTRL, (u8)(HALMAC_REG_R8(REG_BCN_CTRL) |
					  BIT_EN_BCN_FUNCTION));

	/* Set send beacon related registers */
	HALMAC_REG_W32(REG_TBTT_PROHIBIT, WLAN_TBTT_TIME);
	HALMAC_REG_W8(REG_DRVERLYINT, WLAN_DRV_EARLY_INT);
	HALMAC_REG_W8(REG_BCN_CTRL_CLINT0, WLAN_BCN_CTRL_CLT0);
	HALMAC_REG_W8(REG_BCNDMATIM, WLAN_BCN_DMA_TIME);
	HALMAC_REG_W8(REG_BCN_MAX_ERR, WLAN_BCN_MAX_ERR);

	/* MU primary packet fail, BAR packet will not issue */
	//HALMAC_REG_W8_SET(REG_BAR_TX_CTRL, BIT(0));   //yx_qi

	HALMAC_REG_W8_SET(REG_DIS_TXREQ_CLR, BIT(7));   //yx_qi, disable BT CCA temporaryly

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

static void
init_txq_ctrl_8733b(struct halmac_adapter *adapter)
{
	u8 value8;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	value8 = HALMAC_REG_R8(REG_FWHW_TXQ_CTRL);
	value8 |= (BIT(7) & ~BIT(1) & ~BIT(2));
	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL, value8);

	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL + 1, WLAN_TXQ_RPT_EN);
}

static void
init_sifs_ctrl_8733b(struct halmac_adapter *adapter)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	HALMAC_REG_W16(REG_SPEC_SIFS, WLAN_SIFS_DUR_TUNE);
	HALMAC_REG_W32(REG_SIFS, WLAN_SIFS_CFG);
	HALMAC_REG_W16(REG_RESP_SIFS_CCK,
		       WLAN_SIFS_CCK_CTX | WLAN_SIFS_CCK_IRX << 8);
	HALMAC_REG_W16(REG_RESP_SIFS_OFDM,
		       WLAN_SIFS_OFDM_CTX | WLAN_SIFS_OFDM_IRX << 8);
}

static void
init_rate_fallback_ctrl_8733b(struct halmac_adapter *adapter)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	HALMAC_REG_W32(REG_DARFRC, WLAN_DATA_RATE_FB_CNT_1_4);
	HALMAC_REG_W32(REG_DARFRCH, WLAN_DATA_RATE_FB_CNT_5_8);
	HALMAC_REG_W32(REG_RARFRCH, WLAN_RTS_RATE_FB_CNT_5_8);

	HALMAC_REG_W32(REG_ARFRL0, WLAN_DATA_RATE_FB_RATE0);
	HALMAC_REG_W32(REG_ARFRH0, WLAN_DATA_RATE_FB_RATE0_H);
	HALMAC_REG_W32(REG_ARFR1_V1, WLAN_RTS_RATE_FB_RATE1);
	HALMAC_REG_W32(REG_ARFRH1_V1, WLAN_RTS_RATE_FB_RATE1_H);
	HALMAC_REG_W32(REG_ARFR4, WLAN_RTS_RATE_FB_RATE4);
	HALMAC_REG_W32(REG_ARFRH4_H, WLAN_RTS_RATE_FB_RATE4_H);
	HALMAC_REG_W32(REG_ARFR5, WLAN_RTS_RATE_FB_RATE5);
	HALMAC_REG_W32(REG_ARFRH5, WLAN_RTS_RATE_FB_RATE5_H);
}

/**
 * init_wmac_cfg_8733b() - init wmac config
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_wmac_cfg_8733b(struct halmac_adapter *adapter)
{
	u8 value8;
	u16 value16;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	u8 DA[6]={0x00,0x00,0x00,0x00,0x00,0x02};
	//u8 DA[6]={0x00,0x00,0x00,0x00,0x00,0x03};
	u8 sec_cfg;
	u8 crc5;
	u32 rdata, wdata, offset, start_page;   //crc5_addr,addr,wdata_L,wdata_H,
	u8 i=0,j=0;
	start_page=0;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	HALMAC_REG_W32(REG_MAR, 0xFFFFFFFF);
	HALMAC_REG_W32(REG_MAR + 4, 0xFFFFFFFF);

	HALMAC_REG_W8(REG_BBPSF_CTRL + 2, WLAN_RESP_TXRATE);
	HALMAC_REG_W8(REG_ACKTO, WLAN_ACK_TO);
	HALMAC_REG_W8(REG_ACKTO_CCK, WLAN_ACK_TO_CCK);
	HALMAC_REG_W16(REG_EIFS, WLAN_EIFS_DUR_TUNE);

	HALMAC_REG_W8(REG_NAV_CTRL + 2, WLAN_NAV_MAX);

	value8 = HALMAC_REG_R8(REG_WMAC_TRXPTCL_CTL+2);
	value8 = value8 & ~BIT(4);
	HALMAC_REG_W8(REG_WMAC_TRXPTCL_CTL+2, value8);//0x668[21:20]=2'b10  define RX busy according to PHYRXON

	value16=HALMAC_REG_R16(REG_WMAC_TRXPTCL_CTL_H);
	value16=value16 & 0xFEFC;
	HALMAC_REG_W16(REG_WMAC_TRXPTCL_CTL_H, value16); //0x66C[8]=0  0x66C[1:0]=2'b00  in itself txop can send CTS
	HALMAC_REG_W8(REG_WMAC_TRXPTCL_CTL_H  + 2, WLAN_BAR_ACK_TYPE);

	HALMAC_REG_W32(REG_RXFLTMAP0, WLAN_RX_FILTER0);
	HALMAC_REG_W16(REG_RXFLTMAP2, WLAN_RX_FILTER2);

	HALMAC_REG_W32(REG_RCR, WLAN_RCR_CFG);
	value8 = HALMAC_REG_R8(REG_RXPSF_CTRL + 2);
	value8 = value8 | 0xe;
	HALMAC_REG_W8(REG_RXPSF_CTRL + 2, value8);

	HALMAC_REG_W8(REG_RX_PKT_LIMIT, WLAN_RXPKT_MAX_SZ_512);

	HALMAC_REG_W8(REG_TCR + 2, WLAN_TX_FUNC_CFG2);
	HALMAC_REG_W8(REG_TCR + 1, WLAN_TX_FUNC_CFG1);

	value16 = HALMAC_REG_R16(REG_WMAC_CSIDMA_CFG);
	value16 = (value16 & 0xF000) | 0xFC;
	HALMAC_REG_W16(REG_WMAC_CSIDMA_CFG,value16);//jerry_zhou csi_report put in page 252,0x169C[11:0]=0xFC

	//HALMAC_REG_W16_SET(REG_GENERAL_OPTION, BIT_DUMMY_FCS_READY_MASK_EN);  yx_qi

	//jerry_zhou 20201202
	//for driver handle 802.11w management frame encrypt and decode by SW
	sec_cfg = HALMAC_REG_R8(REG_WSEC_OPTION + 2);
	sec_cfg&= ~(BIT(4) | BIT(6)| BIT(3)|BIT(5));
	HALMAC_REG_W8(REG_WSEC_OPTION + 2, sec_cfg);

	HALMAC_REG_W8_SET(REG_SND_PTCL_CTRL, BIT_R_DISABLE_CHECK_VHTSIGB_CRC);

	HALMAC_REG_W32(REG_WMAC_OPTION_FUNCTION_2, WLAN_MAC_OPT_FUNC2);

	if (adapter->hw_cfg_info.trx_mode == HALMAC_TRNSFER_NORMAL)
		value8 = WLAN_MAC_OPT_NORM_FUNC1;
	else
		value8 = WLAN_MAC_OPT_LB_FUNC1;

	value8=value8 & 0x7F; //MAC don't receive early drop pkt

	HALMAC_REG_W8(REG_WMAC_OPTION_FUNCTION_1, value8);
	//rdata=0xffffffff;
	//HALMAC_REG_W16_CLR(0x7D4,BIT(15));  //yx_qi, disable rx macid search temporaryly,
	/*

	//indirect access,8733B not supported

	rdata=HALMAC_REG_R32(0x6bc);
	wdata=(rdata&0x0000FFFF)|(0x150<<16);
	HALMAC_REG_W32(0x6bc,wdata);
	rdata=HALMAC_REG_R32(0x6bc);

	rdata=HALMAC_REG_R32(0x7d4);
	wdata=(rdata&0xfff7ffff)|BIT(20)|BIT(15)|BIT(16);
	HALMAC_REG_W32(0x7d4,wdata);
	rdata=HALMAC_REG_R32(0x7d4);

	rdata=HALMAC_REG_R32(0x7cc);
	wdata=(rdata&0xfffffffe);
	HALMAC_REG_W32(0x7cc,wdata);
	rdata=HALMAC_REG_R32(0x7cc);
	
	//set ctrl info and CRC5
	rdata=HALMAC_REG_R32(0x104);
	wdata=(rdata&0xff00ffff)|(0x7F<<16);
	HALMAC_REG_W32(0x104,wdata);  //read TXRPT
	rdata=HALMAC_REG_R32(0x104);

	//reset ctrl info
	//for(i=0;i< 4*32;i++)
	for(i=0;i< 5*16;i++)//16 macid,each macid 40Bytes
	{
		HALMAC_REG_W8(0x143,0x0);
		HALMAC_REG_W16(0x140,i);
		HALMAC_REG_W32(0x144,0x0);
		HALMAC_REG_W32(0x148,0x0);
		rdata=HALMAC_REG_R32(0x140);
		wdata=rdata|BIT(20)|(0xFF<24);
		HALMAC_REG_W32(0x140,wdata);
		rdata=HALMAC_REG_R32(0x140);
		rdata=HALMAC_REG_R32(0x144);
		rdata=HALMAC_REG_R32(0x148);
	}
	
	//write MAC address in ctrl info
	HALMAC_REG_W8(0x143,0x0);
	HALMAC_REG_W16(0x140,0x2);
	HALMAC_REG_W32(0x144,0x02);
	HALMAC_REG_W32(0x148,0x0);
	rdata=HALMAC_REG_R32(0x140);
	wdata=rdata|BIT(20)|(0xFF<24);
	HALMAC_REG_W32(0x140,wdata);
	rdata=HALMAC_REG_R32(0x144);
	rdata=HALMAC_REG_R32(0x148);

	//crc5=CRC5_USB(DA,6);
	//HALMAC_REG_W32(REG_SEARCH_MACID_8733B);

	//reset crc5
	for(i=0;i<12;i++)
	{
		HALMAC_REG_W8(0x143,0x0);
		crc5_addr=0x150+i;//i is entry num
		HALMAC_REG_W16(0x140,crc5_addr);
		crc5=0;
		HALMAC_REG_W32(0x144,crc5);
		HALMAC_REG_W32(0x148,0x0);//make high 32bit to 0x148
		rdata=HALMAC_REG_R32(0x140);
		wdata=rdata|BIT(20)|(0xFF<24);
		HALMAC_REG_W32(0x140,wdata);
	}

	crc5=CRC5_USB(DA,6);

	//write CRC5 into txrpt buffer
	HALMAC_REG_W8(0x143,0x0);
	crc5_addr=0x150;//crc5 default address:0x150
	HALMAC_REG_W16(0x140,0x150);
	wdata_H=0x1<<28;
	wdata_L=crc5;//bit60 valid
	HALMAC_REG_W32(0x144,wdata_L);
	HALMAC_REG_W32(0x148,wdata_H);//make high 32bit to 0x148
	rdata=HALMAC_REG_R32(0x140);
	wdata=rdata|BIT(20)|(0xFF<24);
	HALMAC_REG_W32(0x140,wdata);
	*/

/**********************************************************************************************************/
	//MACID search , receive data frame
	//direct access txrpt buffer,write MAC address and crc5 into txrpt buffer
	//jerry_zhou
	//test MACID search
	rdata=HALMAC_REG_R32(REG_SEARCH_MACID);
	wdata=(rdata&0xE000E000)|(0x150<<16);
	HALMAC_REG_W32(REG_SEARCH_MACID,wdata);
	rdata=HALMAC_REG_R32(REG_SEARCH_MACID);
	wdata=rdata|BIT(13);
	HALMAC_REG_W32(REG_SEARCH_MACID,wdata);


	crc5=CRC5_USB(DA,6);
	rdata=HALMAC_REG_R32(REG_PKTBUF_DBG_CTRL);
	rdata=rdata&0xF000;
	offset=0;
	start_page=0x660;
	wdata=rdata|start_page;
	HALMAC_REG_W32(REG_PKTBUF_DBG_CTRL,wdata);
	wdata=0x00000000;
	HALMAC_REG_W32(0x8010,wdata);//txrpt buffer 0x2 position
	wdata=0x00000200;
	//wdata=0x00000300;
	HALMAC_REG_W32(0x8014,wdata);
	//HALMAC_REG_W32(0x8A80,0x0000001a);
	HALMAC_REG_W32(0x8A80,crc5);
	wdata=0x20000000;
	HALMAC_REG_W32(0x8A84,wdata);
	

	
	rdata=HALMAC_REG_R32(REG_WMAC_OPTION_FUNCTION_1);
	//wdata=(rdata&0xfff7ffff)|BIT(20)|BIT(15)|BIT(16);
	wdata=(rdata&0xfff6ffff)|BIT(20)|BIT(15)|BIT(16);
	HALMAC_REG_W32(REG_WMAC_OPTION_FUNCTION_1,wdata);
	rdata=HALMAC_REG_R32(REG_WMAC_OPTION_FUNCTION_1);

	rdata=HALMAC_REG_R32(REG_WMAC_FTM_CTL);
	wdata=(rdata&0xfffffffe);
	HALMAC_REG_W32(REG_WMAC_FTM_CTL,wdata);
	rdata=HALMAC_REG_R32(REG_WMAC_FTM_CTL);
/**********************************************************************************************************/	
    //fw
	/*rdata=HALMAC_REG_R8(0x1008);
	wdata=rdata|BIT(1);
	HALMAC_REG_W8(0x1008,wdata);

	rdata=HALMAC_REG_R8(0x1001);
	wdata=rdata|BIT(5) | BIT(4);
	HALMAC_REG_W8(0x1001,wdata);

	rdata=HALMAC_REG_R8(0x1000);
	wdata=rdata &0x3F;
	HALMAC_REG_W8(0x1000,wdata);
	*/

	//HALMAC_REG_W32(0x200,0x80790808);
	status = api->halmac_init_low_pwr(adapter);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

u8 CRC5_USB(unsigned char *in,unsigned long byte_num)
{
	int a,b;
	unsigned long i,j;
	u8 mask,smask=0x1;
	int CRCMask=0x00000010;
	int POLY=0x05;//CRC5 100101 ==>00101
	int CRC_internal=0x1f;//initial value
	int t;


	for(i=0;i<byte_num;i++)
	{
		mask=smask;
		for(j=0;j<8;j++)
		{
			a=((CRC_internal&CRCMask)!=0);
			b=((in[i]&mask)!=0);
			
			CRC_internal<<=1;
			mask<<=1;

			if(a^b)
			{
				CRC_internal^=POLY;
			}
		}
	}
	//*((unsigned long *)out)=(CRC_internal^0x1f)&0x1f;
	t=(CRC_internal^0x1f)&0x1f;
	return (CRC_internal^0x1f)&0x1f;

}

/**
 * pre_init_system_cfg_8733b() - pre-init system config
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
pre_init_system_cfg_8733b(struct halmac_adapter *adapter)
{
	u32 value32;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	u8 enable_bb;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	HALMAC_REG_W8(REG_REG_ACCESS_CTRL, 0);

	if (adapter->intf == HALMAC_INTERFACE_SDIO) {
#if HALMAC_SDIO_SUPPORT
		if (leave_sdio_suspend_87xx(adapter) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_SDIO_LEAVE_SUSPEND_FAIL;
#endif
	} else if (adapter->intf == HALMAC_INTERFACE_USB) {
#if HALMAC_USB_SUPPORT
		if (HALMAC_REG_R8(REG_SYS_CFG2 + 3) == 0x20)
			HALMAC_REG_W8(0xFE5B, HALMAC_REG_R8(0xFE5B) | BIT(4));
#endif
	} else if (adapter->intf == HALMAC_INTERFACE_PCIE) {
#if HALMAC_PCIE_SUPPORT
		/* For PCIE power on fail issue */
		HALMAC_REG_W8(REG_HCI_OPT_CTRL + 1,
			      HALMAC_REG_R8(REG_HCI_OPT_CTRL + 1) | BIT(0));
#endif
	}

	/* Config PIN Mux */
	value32 = HALMAC_REG_R32(REG_PAD_CTRL1);
	value32 = value32 & (~(BIT(28) | BIT(29)));
	value32 = value32 | BIT(28) | BIT(29);
	HALMAC_REG_W32(REG_PAD_CTRL1, value32);

	value32 = HALMAC_REG_R32(REG_LED_CFG);
	value32 = value32 & (~(BIT(25) | BIT(26)));
	HALMAC_REG_W32(REG_LED_CFG, value32);

	value32 = HALMAC_REG_R32(REG_GPIO_MUXCFG);
	value32 = value32 & (~(BIT(2)));
	value32 = value32 | BIT(2);
	HALMAC_REG_W32(REG_GPIO_MUXCFG, value32);

	enable_bb = 0;
	set_hw_value_87xx(adapter, HALMAC_HW_EN_BB_RF, &enable_bb);

	if (HALMAC_REG_R8(REG_SYS_CFG1 + 2) & BIT(4)) {
		PLTFM_MSG_ERR("[ERR]test mode!!\n");
		return HALMAC_RET_WLAN_MODE_FAIL;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_8733B_SUPPORT */
