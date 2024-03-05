#ifndef _HALMAC_TX_DESC_AP_H_
#define _HALMAC_TX_DESC_AP_H_
#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

/*TXDESC_WORD0*/

#define SET_TX_DESC_DISQSELSEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 31)
#define SET_TX_DESC_DISQSELSEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 31)
#define GET_TX_DESC_DISQSELSEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 31)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_RDG_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 31)
#define SET_TX_DESC_RDG_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 31)
#define GET_TX_DESC_RDG_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 31)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_GF(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 30)
#define SET_TX_DESC_GF_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 30)
#define GET_TX_DESC_GF(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 30)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_BCNPKT_TSF_CTRL_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 30)
#define SET_TX_DESC_BCNPKT_TSF_CTRL_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 30)
#define GET_TX_DESC_BCNPKT_TSF_CTRL_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 30)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_NO_ACM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 29)
#define SET_TX_DESC_NO_ACM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 29)
#define GET_TX_DESC_NO_ACM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 29)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_AGG_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 29)
#define SET_TX_DESC_AGG_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 29)
#define GET_TX_DESC_AGG_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 29)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_BCNPKT_TSF_CTRL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 28)
#define SET_TX_DESC_BCNPKT_TSF_CTRL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 28)
#define GET_TX_DESC_BCNPKT_TSF_CTRL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 28)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_AMSDU_PAD_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 27)
#define SET_TX_DESC_AMSDU_PAD_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 27)
#define GET_TX_DESC_AMSDU_PAD_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 27)
#define SET_TX_DESC_LS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 26)
#define SET_TX_DESC_LS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 26)
#define GET_TX_DESC_LS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 26)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_HTC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 25)
#define SET_TX_DESC_HTC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 25)
#define GET_TX_DESC_HTC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 25)
#define SET_TX_DESC_BMC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 24)
#define SET_TX_DESC_BMC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1, 24)
#define GET_TX_DESC_BMC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_PKT_OFFSET_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1f, 24)
#define SET_TX_DESC_PKT_OFFSET_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0x1f, 24)
#define GET_TX_DESC_PKT_OFFSET_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0x1f, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8852A_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_OFFSET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0xff, 16)
#define SET_TX_DESC_OFFSET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0xff, 16)
#define GET_TX_DESC_OFFSET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0xff, 16)
#define SET_TX_DESC_TXPKTSIZE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0xffff, 0)
#define SET_TX_DESC_TXPKTSIZE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword0, value, 0xffff, 0)
#define GET_TX_DESC_TXPKTSIZE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword0, 0xffff, 0)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)


/*WORD1*/

#define SET_TX_DESC_HW_AES_IV_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define SET_TX_DESC_HW_AES_IV_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define GET_TX_DESC_HW_AES_IV_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 31)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_USERATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define SET_TX_DESC_USERATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define GET_TX_DESC_USERATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 31)

#endif

#if (HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_HW_AESIV(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define SET_TX_DESC_HW_AESIV_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 31)
#define GET_TX_DESC_HW_AESIV(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 31)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_FTM_EN_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 30)
#define SET_TX_DESC_FTM_EN_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 30)
#define GET_TX_DESC_FTM_EN_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 30)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_AMSDU(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 30)
#define SET_TX_DESC_AMSDU_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 30)
#define GET_TX_DESC_AMSDU(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 30)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_MOREDATA(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 29)
#define SET_TX_DESC_MOREDATA_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 29)
#define GET_TX_DESC_MOREDATA(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 29)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_EN_HWSEQ_MODE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 28)
#define SET_TX_DESC_EN_HWSEQ_MODE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 28)
#define GET_TX_DESC_EN_HWSEQ_MODE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x3, 28)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_PKT_OFFSET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 24)
#define SET_TX_DESC_PKT_OFFSET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 24)
#define GET_TX_DESC_PKT_OFFSET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1f, 24)
#define SET_TX_DESC_SEC_TYPE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 22)
#define SET_TX_DESC_SEC_TYPE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 22)
#define GET_TX_DESC_SEC_TYPE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x3, 22)
#define SET_TX_DESC_EN_DESC_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 21)
#define SET_TX_DESC_EN_DESC_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 21)
#define GET_TX_DESC_EN_DESC_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 21)
#define SET_TX_DESC_RATE_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 16)
#define SET_TX_DESC_RATE_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 16)
#define GET_TX_DESC_RATE_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1f, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_SW_SEQ_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0xfff, 16)
#define SET_TX_DESC_SW_SEQ_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0xfff, 16)
#define GET_TX_DESC_SW_SEQ_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0xfff, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_PIFS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 15)
#define SET_TX_DESC_PIFS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 15)
#define GET_TX_DESC_PIFS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 15)
#define SET_TX_DESC_LSIG_TXOP_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 14)
#define SET_TX_DESC_LSIG_TXOP_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 14)
#define GET_TX_DESC_LSIG_TXOP_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 14)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DROP_ID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 14)
#define SET_TX_DESC_DROP_ID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x3, 14)
#define GET_TX_DESC_DROP_ID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x3, 14)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RD_NAV_EXT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 13)
#define SET_TX_DESC_RD_NAV_EXT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 13)
#define GET_TX_DESC_RD_NAV_EXT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 13)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MOREDATA_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 13)
#define SET_TX_DESC_MOREDATA_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 13)
#define GET_TX_DESC_MOREDATA_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 13)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_QSEL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 8)
#define SET_TX_DESC_QSEL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 8)
#define GET_TX_DESC_QSEL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1f, 8)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_QSEL(5B)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 8)
#define SET_TX_DESC_QSEL(5B)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1f, 8)
#define GET_TX_DESC_QSEL(5B)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1f, 8)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SPECIAL_CW(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 7)
#define SET_TX_DESC_SPECIAL_CW_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 7)
#define GET_TX_DESC_SPECIAL_CW(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 7)

#endif

#if (HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_BIP_KEYID_SEL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 7)
#define SET_TX_DESC_BIP_KEYID_SEL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x1, 7)
#define GET_TX_DESC_BIP_KEYID_SEL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x1, 7)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_MACID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x7f, 0)
#define SET_TX_DESC_MACID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x7f, 0)
#define GET_TX_DESC_MACID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x7f, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MACID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x7f, 0)
#define SET_TX_DESC_MACID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword1, value, 0x7f, 0)
#define GET_TX_DESC_MACID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword1, 0x7f, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)


/*TXDESC_WORD2*/

#define SET_TX_DESC_HW_AES_IV(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 31)
#define SET_TX_DESC_HW_AES_IV_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 31)
#define GET_TX_DESC_HW_AES_IV(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 31)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_CHK_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 31)
#define SET_TX_DESC_CHK_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 31)
#define GET_TX_DESC_CHK_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 31)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_FTM_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 30)
#define SET_TX_DESC_FTM_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 30)
#define GET_TX_DESC_FTM_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 30)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DISQSELSEQ_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 30)
#define SET_TX_DESC_DISQSELSEQ_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 30)
#define GET_TX_DESC_DISQSELSEQ_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 30)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANTCEL_D_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 28)
#define SET_TX_DESC_ANTCEL_D_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 28)
#define GET_TX_DESC_ANTCEL_D_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0xf, 28)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_SND_PKT_SEL_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3, 28)
#define SET_TX_DESC_SND_PKT_SEL_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3, 28)
#define GET_TX_DESC_SND_PKT_SEL_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x3, 28)
#define SET_TX_DESC_DMA_PRI(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 27)
#define SET_TX_DESC_DMA_PRI_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 27)
#define GET_TX_DESC_DMA_PRI(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 27)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_G_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3f, 24)
#define SET_TX_DESC_G_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3f, 24)
#define GET_TX_DESC_G_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x3f, 24)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANTSEL_C_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 24)
#define SET_TX_DESC_ANTSEL_C_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 24)
#define GET_TX_DESC_ANTSEL_C_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0xf, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MAX_AMSDU_MODE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x7, 24)
#define SET_TX_DESC_MAX_AMSDU_MODE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x7, 24)
#define GET_TX_DESC_MAX_AMSDU_MODE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x7, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_BT_NULL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 23)
#define SET_TX_DESC_BT_NULL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 23)
#define GET_TX_DESC_BT_NULL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 23)
#define SET_TX_DESC_AMPDU_DENSITY(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x7, 20)
#define SET_TX_DESC_AMPDU_DENSITY_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x7, 20)
#define GET_TX_DESC_AMPDU_DENSITY(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x7, 20)
#define SET_TX_DESC_SPE_RPT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 19)
#define SET_TX_DESC_SPE_RPT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 19)
#define GET_TX_DESC_SPE_RPT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 19)
#define SET_TX_DESC_RAW(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 18)
#define SET_TX_DESC_RAW_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 18)
#define GET_TX_DESC_RAW(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 18)
#define SET_TX_DESC_MOREFRAG(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 17)
#define SET_TX_DESC_MOREFRAG_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 17)
#define GET_TX_DESC_MOREFRAG(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 17)
#define SET_TX_DESC_BK(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 16)
#define SET_TX_DESC_BK_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 16)
#define GET_TX_DESC_BK(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 16)
#define SET_TX_DESC_NULL_1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 15)
#define SET_TX_DESC_NULL_1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 15)
#define GET_TX_DESC_NULL_1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 15)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_RTY_LMT_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 15)
#define SET_TX_DESC_RTY_LMT_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 15)
#define GET_TX_DESC_RTY_LMT_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 15)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_NULL_0(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 14)
#define SET_TX_DESC_NULL_0_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 14)
#define GET_TX_DESC_NULL_0(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 14)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_HW_RTS_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 14)
#define SET_TX_DESC_HW_RTS_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 14)
#define GET_TX_DESC_HW_RTS_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 14)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RDG_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 13)
#define SET_TX_DESC_RDG_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 13)
#define GET_TX_DESC_RDG_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 13)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_RTSEN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 13)
#define SET_TX_DESC_RTSEN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 13)
#define GET_TX_DESC_RTSEN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 13)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_AGG_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 12)
#define SET_TX_DESC_AGG_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 12)
#define GET_TX_DESC_AGG_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 12)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_CTS2SELF_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 12)
#define SET_TX_DESC_CTS2SELF_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 12)
#define GET_TX_DESC_CTS2SELF_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 12)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_CCA_RTS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3, 10)
#define SET_TX_DESC_CCA_RTS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x3, 10)
#define GET_TX_DESC_CCA_RTS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x3, 10)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TRI_FRAME(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 9)
#define SET_TX_DESC_TRI_FRAME_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1, 9)
#define GET_TX_DESC_TRI_FRAME(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1, 9)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TAILPAGE_H(4B)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 8)
#define SET_TX_DESC_TAILPAGE_H(4B)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xf, 8)
#define GET_TX_DESC_TAILPAGE_H(4B)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0xf, 8)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_P_AID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1ff, 0)
#define SET_TX_DESC_P_AID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0x1ff, 0)
#define GET_TX_DESC_P_AID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0x1ff, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TAILPAGE_L(8B)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xff, 0)
#define SET_TX_DESC_TAILPAGE_L(8B)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword2, value, 0xff, 0)
#define GET_TX_DESC_TAILPAGE_L(8B)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword2, 0xff, 0)

/*WORD3*/

#define SET_TX_DESC_BAR_RC(HW)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 30)
#define SET_TX_DESC_BAR_RC(HW)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 30)
#define GET_TX_DESC_BAR_RC(HW)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x3, 30)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_AMPDU_MAX_TIME(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xff, 24)
#define SET_TX_DESC_AMPDU_MAX_TIME_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xff, 24)
#define GET_TX_DESC_AMPDU_MAX_TIME(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0xff, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_RTS_RC(HW)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3f, 24)
#define SET_TX_DESC_RTS_RC(HW)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3f, 24)
#define GET_TX_DESC_RTS_RC(HW)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x3f, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_NDPA(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 22)
#define SET_TX_DESC_NDPA_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 22)
#define GET_TX_DESC_NDPA(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x3, 22)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MU_RC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 20)
#define SET_TX_DESC_MU_RC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 20)
#define GET_TX_DESC_MU_RC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0xf, 20)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_MAX_AGG_NUM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1f, 17)
#define SET_TX_DESC_MAX_AGG_NUM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1f, 17)
#define GET_TX_DESC_MAX_AGG_NUM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1f, 17)
#define SET_TX_DESC_USE_MAX_TIME_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 16)
#define SET_TX_DESC_USE_MAX_TIME_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 16)
#define GET_TX_DESC_USE_MAX_TIME_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MU_2ND_RC (HW)(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 16)
#define SET_TX_DESC_MU_2ND_RC (HW)_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 16)
#define GET_TX_DESC_MU_2ND_RC (HW)(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0xf, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_NAVUSEHDR(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 15)
#define SET_TX_DESC_NAVUSEHDR_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 15)
#define GET_TX_DESC_NAVUSEHDR(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 15)
#define SET_TX_DESC_CHK_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 14)
#define SET_TX_DESC_CHK_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 14)
#define GET_TX_DESC_CHK_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 14)
#define SET_TX_DESC_HW_RTS_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 13)
#define SET_TX_DESC_HW_RTS_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 13)
#define GET_TX_DESC_HW_RTS_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 13)
#define SET_TX_DESC_RTSEN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 12)
#define SET_TX_DESC_RTSEN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 12)
#define GET_TX_DESC_RTSEN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 12)
#define SET_TX_DESC_CTS2SELF(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 11)
#define SET_TX_DESC_CTS2SELF_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 11)
#define GET_TX_DESC_CTS2SELF(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 11)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_CHANNEL_DMA_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 11)
#define SET_TX_DESC_CHANNEL_DMA_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 11)
#define GET_TX_DESC_CHANNEL_DMA_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0xf, 11)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_DISDATAFB(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 10)
#define SET_TX_DESC_DISDATAFB_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 10)
#define GET_TX_DESC_DISDATAFB(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 10)
#define SET_TX_DESC_DISRTSFB(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 9)
#define SET_TX_DESC_DISRTSFB_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 9)
#define GET_TX_DESC_DISRTSFB(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 9)
#define SET_TX_DESC_USE_RATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 8)
#define SET_TX_DESC_USE_RATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 8)
#define GET_TX_DESC_USE_RATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 8)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MBSSID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 7)
#define SET_TX_DESC_MBSSID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0xf, 7)
#define GET_TX_DESC_MBSSID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0xf, 7)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_HW_SSN_SEL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 6)
#define SET_TX_DESC_HW_SSN_SEL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3, 6)
#define GET_TX_DESC_HW_SSN_SEL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x3, 6)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_BK_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 6)
#define SET_TX_DESC_BK_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1, 6)
#define GET_TX_DESC_BK_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1, 6)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_WHEADER_LEN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1f, 0)
#define SET_TX_DESC_WHEADER_LEN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x1f, 0)
#define GET_TX_DESC_WHEADER_LEN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x1f, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DATA_RC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3f, 0)
#define SET_TX_DESC_DATA_RC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword3, value, 0x3f, 0)
#define GET_TX_DESC_DATA_RC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword3, 0x3f, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)


/*TXDESC_WORD4*/

#define SET_TX_DESC_PCTS_MASK_IDX(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x3, 30)
#define SET_TX_DESC_PCTS_MASK_IDX_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x3, 30)
#define GET_TX_DESC_PCTS_MASK_IDX(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x3, 30)
#define SET_TX_DESC_PCTS_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 29)
#define SET_TX_DESC_PCTS_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 29)
#define GET_TX_DESC_PCTS_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1, 29)
#define SET_TX_DESC_RTSRATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 24)
#define SET_TX_DESC_RTSRATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 24)
#define GET_TX_DESC_RTSRATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1f, 24)
#define SET_TX_DESC_RTS_DATA_RTY_LMT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x3f, 18)
#define SET_TX_DESC_RTS_DATA_RTY_LMT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x3f, 18)
#define GET_TX_DESC_RTS_DATA_RTY_LMT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x3f, 18)
#define SET_TX_DESC_RTY_LMT_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 17)
#define SET_TX_DESC_RTY_LMT_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 17)
#define GET_TX_DESC_RTY_LMT_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1, 17)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TXDESC_CHECKSUM_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xffff, 16)
#define SET_TX_DESC_TXDESC_CHECKSUM_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xffff, 16)
#define GET_TX_DESC_TXDESC_CHECKSUM_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0xffff, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RTS_RTY_LOWEST_RATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xf, 13)
#define SET_TX_DESC_RTS_RTY_LOWEST_RATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xf, 13)
#define GET_TX_DESC_RTS_RTY_LOWEST_RATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0xf, 13)
#define SET_TX_DESC_DATA_RTY_LOWEST_RATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 8)
#define SET_TX_DESC_DATA_RTY_LOWEST_RATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 8)
#define GET_TX_DESC_DATA_RTY_LOWEST_RATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1f, 8)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DMA_TXAGG_NUM_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xff, 8)
#define SET_TX_DESC_DMA_TXAGG_NUM_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0xff, 8)
#define GET_TX_DESC_DMA_TXAGG_NUM_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0xff, 8)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TRY_RATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 7)
#define SET_TX_DESC_TRY_RATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 7)
#define GET_TX_DESC_TRY_RATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1, 7)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_POLLUTED_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 7)
#define SET_TX_DESC_POLLUTED_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 7)
#define GET_TX_DESC_POLLUTED_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1, 7)
#define SET_TX_DESC_TXD_INFO(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 6)
#define SET_TX_DESC_TXD_INFO_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1, 6)
#define GET_TX_DESC_TXD_INFO(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1, 6)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_DATARATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x7f, 0)
#define SET_TX_DESC_DATARATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x7f, 0)
#define GET_TX_DESC_DATARATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x7f, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_WHEADER_LEN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 0)
#define SET_TX_DESC_WHEADER_LEN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword4, value, 0x1f, 0)
#define GET_TX_DESC_WHEADER_LEN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword4, 0x1f, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)


/*TXDESC_WORD5*/

#define SET_TX_DESC_POLLUTED(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 31)
#define SET_TX_DESC_POLLUTED_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 31)
#define GET_TX_DESC_POLLUTED(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 31)

#endif

#if (HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_ANTSEL_EN_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 30)
#define SET_TX_DESC_ANTSEL_EN_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 30)
#define GET_TX_DESC_ANTSEL_EN_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 30)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TXPWR_OFSET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 28)
#define SET_TX_DESC_TXPWR_OFSET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 28)
#define GET_TX_DESC_TXPWR_OFSET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x7, 28)

#endif

#if (HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_TXPWR_OFSET_TYPE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 28)
#define SET_TX_DESC_TXPWR_OFSET_TYPE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 28)
#define GET_TX_DESC_TXPWR_OFSET_TYPE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 28)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TX_ANT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 24)
#define SET_TX_DESC_TX_ANT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 24)
#define GET_TX_DESC_TX_ANT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0xf, 24)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_DROP_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 24)
#define SET_TX_DESC_DROP_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 24)
#define GET_TX_DESC_DROP_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_PORT_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 21)
#define SET_TX_DESC_PORT_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 21)
#define GET_TX_DESC_PORT_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x7, 21)

#endif

#if (HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_MULTI_PORT_ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 21)
#define SET_TX_DESC_MULTI_PORT_ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 21)
#define GET_TX_DESC_MULTI_PORT_ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x7, 21)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MULTI-PORT ID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 20)
#define SET_TX_DESC_MULTI-PORT ID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 20)
#define GET_TX_DESC_MULTI-PORT ID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x7, 20)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_MULTIPLE_PORT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 18)
#define SET_TX_DESC_MULTIPLE_PORT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x7, 18)
#define GET_TX_DESC_MULTIPLE_PORT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x7, 18)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SIGNALING_TAPKT_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 17)
#define SET_TX_DESC_SIGNALING_TAPKT_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 17)
#define GET_TX_DESC_SIGNALING_TAPKT_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 17)

#endif

#if (HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_SIGNALING_TAPKT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 17)
#define SET_TX_DESC_SIGNALING_TAPKT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 17)
#define GET_TX_DESC_SIGNALING_TAPKT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 17)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RTS_SC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 13)
#define SET_TX_DESC_RTS_SC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 13)
#define GET_TX_DESC_RTS_SC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0xf, 13)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_SIGNALING_TA_PKT_SC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 13)
#define SET_TX_DESC_SIGNALING_TA_PKT_SC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 13)
#define GET_TX_DESC_SIGNALING_TA_PKT_SC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0xf, 13)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RTS_SHORT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 12)
#define SET_TX_DESC_RTS_SHORT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 12)
#define GET_TX_DESC_RTS_SHORT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 12)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_VCS_STBC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 10)
#define SET_TX_DESC_VCS_STBC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 10)
#define GET_TX_DESC_VCS_STBC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 10)

#endif

#if (HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_RTS_STBC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 10)
#define SET_TX_DESC_RTS_STBC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 10)
#define GET_TX_DESC_RTS_STBC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 10)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_DATA_STBC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 8)
#define SET_TX_DESC_DATA_STBC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 8)
#define GET_TX_DESC_DATA_STBC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 8)
#define SET_TX_DESC_DATA_LDPC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 7)
#define SET_TX_DESC_DATA_LDPC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 7)
#define GET_TX_DESC_DATA_LDPC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 7)
#define SET_TX_DESC_DATA_BW(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 5)
#define SET_TX_DESC_DATA_BW_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x3, 5)
#define GET_TX_DESC_DATA_BW(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x3, 5)
#define SET_TX_DESC_DATA_SHORT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 4)
#define SET_TX_DESC_DATA_SHORT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0x1, 4)
#define GET_TX_DESC_DATA_SHORT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0x1, 4)
#define SET_TX_DESC_DATA_SC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 0)
#define SET_TX_DESC_DATA_SC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xf, 0)
#define GET_TX_DESC_DATA_SC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0xf, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_PCIE_SEQ_NUM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xffff, 0)
#define SET_TX_DESC_PCIE_SEQ_NUM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword5, value, 0xffff, 0)
#define GET_TX_DESC_PCIE_SEQ_NUM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword5, 0xffff, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)


/*TXDESC_WORD6*/

#define SET_TX_DESC_ANTSEL_D(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 30)
#define SET_TX_DESC_ANTSEL_D_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 30)
#define GET_TX_DESC_ANTSEL_D(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 30)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANT_MAPD_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 30)
#define SET_TX_DESC_ANT_MAPD_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 30)
#define GET_TX_DESC_ANT_MAPD_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 30)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANT_MAPD(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 28)
#define SET_TX_DESC_ANT_MAPD_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 28)
#define GET_TX_DESC_ANT_MAPD(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 28)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANT_MAPC_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 28)
#define SET_TX_DESC_ANT_MAPC_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 28)
#define GET_TX_DESC_ANT_MAPC_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 28)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANT_MAPC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 26)
#define SET_TX_DESC_ANT_MAPC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 26)
#define GET_TX_DESC_ANT_MAPC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 26)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANT_MAPB_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 26)
#define SET_TX_DESC_ANT_MAPB_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 26)
#define GET_TX_DESC_ANT_MAPB_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 26)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TRY_RATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 26)
#define SET_TX_DESC_TRY_RATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 26)
#define GET_TX_DESC_TRY_RATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x1, 26)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANT_MAPB(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define SET_TX_DESC_ANT_MAPB_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define GET_TX_DESC_ANT_MAPB(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 24)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANT_MAPA_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define SET_TX_DESC_ANT_MAPA_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define GET_TX_DESC_ANT_MAPA_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DATA_BW_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define SET_TX_DESC_DATA_BW_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 24)
#define GET_TX_DESC_DATA_BW_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 24)
#define SET_TX_DESC_DATA_SHORT_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 23)
#define SET_TX_DESC_DATA_SHORT_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 23)
#define GET_TX_DESC_DATA_SHORT_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x1, 23)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANT_MAPA(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 22)
#define SET_TX_DESC_ANT_MAPA_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 22)
#define GET_TX_DESC_ANT_MAPA(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 22)
#define SET_TX_DESC_ANTSEL_C(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 20)
#define SET_TX_DESC_ANTSEL_C_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 20)
#define GET_TX_DESC_ANTSEL_C(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 20)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANTSEL_B_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 20)
#define SET_TX_DESC_ANTSEL_B_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 20)
#define GET_TX_DESC_ANTSEL_B_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0xf, 20)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANTSEL_B(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 18)
#define SET_TX_DESC_ANTSEL_B_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 18)
#define GET_TX_DESC_ANTSEL_B(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 18)
#define SET_TX_DESC_ANTSEL_A(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 16)
#define SET_TX_DESC_ANTSEL_A_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x3, 16)
#define GET_TX_DESC_ANTSEL_A(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x3, 16)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_ANTSEL_A_V1(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 16)
#define SET_TX_DESC_ANTSEL_A_V1_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 16)
#define GET_TX_DESC_ANTSEL_A_V1(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0xf, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DATARATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x7f, 16)
#define SET_TX_DESC_DATARATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x7f, 16)
#define GET_TX_DESC_DATARATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x7f, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_MBSSID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 12)
#define SET_TX_DESC_MBSSID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xf, 12)
#define GET_TX_DESC_MBSSID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0xf, 12)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TXBF_PATH_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 11)
#define SET_TX_DESC_TXBF_PATH_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x1, 11)
#define GET_TX_DESC_TXBF_PATH_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x1, 11)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_SW_DEFINE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xfff, 0)
#define SET_TX_DESC_SW_DEFINE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xfff, 0)
#define GET_TX_DESC_SW_DEFINE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0xfff, 0)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SWPS_SEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xfff, 0)
#define SET_TX_DESC_SWPS_SEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0xfff, 0)
#define GET_TX_DESC_SWPS_SEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0xfff, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_GROUP_BIT IE_OFFSET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x7ff, 0)
#define SET_TX_DESC_GROUP_BIT IE_OFFSET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword6, value, 0x7ff, 0)
#define GET_TX_DESC_GROUP_BIT IE_OFFSET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword6, 0x7ff, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)


/*TXDESC_WORD7*/

#define SET_TX_DESC_DMA_TXAGG_NUM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xff, 24)
#define SET_TX_DESC_DMA_TXAGG_NUM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xff, 24)
#define GET_TX_DESC_DMA_TXAGG_NUM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xff, 24)
#define SET_TX_DESC_FINAL_DATA_RATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xff, 24)
#define SET_TX_DESC_FINAL_DATA_RATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xff, 24)
#define GET_TX_DESC_FINAL_DATA_RATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xff, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_NTX_MAP(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xf, 20)
#define SET_TX_DESC_NTX_MAP_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xf, 20)
#define GET_TX_DESC_NTX_MAP(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xf, 20)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_ANTSEL_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 19)
#define SET_TX_DESC_ANTSEL_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 19)
#define GET_TX_DESC_ANTSEL_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 19)

#endif

#if (HALMAC_8198F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_MBSSID_EX(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x7, 16)
#define SET_TX_DESC_MBSSID_EX_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x7, 16)
#define GET_TX_DESC_MBSSID_EX(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x7, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_OFFLOAD_SIZE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 16)
#define SET_TX_DESC_OFFLOAD_SIZE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 16)
#define GET_TX_DESC_OFFLOAD_SIZE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xffff, 16)
#define SET_TX_DESC_NAVUSEHDR_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 15)
#define SET_TX_DESC_NAVUSEHDR_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 15)
#define GET_TX_DESC_NAVUSEHDR_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 15)
#define SET_TX_DESC_BMC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 14)
#define SET_TX_DESC_BMC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 14)
#define GET_TX_DESC_BMC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 14)
#define SET_TX_DESC_RTS_DATA_RTY_LMT_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x3f, 8)
#define SET_TX_DESC_RTS_DATA_RTY_LMT_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x3f, 8)
#define GET_TX_DESC_RTS_DATA_RTY_LMT_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x3f, 8)
#define SET_TX_DESC_HW_AES_IV_V3(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 7)
#define SET_TX_DESC_HW_AES_IV_V3_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 7)
#define GET_TX_DESC_HW_AES_IV_V3(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 7)
#define SET_TX_DESC_BT_NULL_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 3)
#define SET_TX_DESC_BT_NULL_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 3)
#define GET_TX_DESC_BT_NULL_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 3)
#define SET_TX_DESC_EN_DESC_ID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 2)
#define SET_TX_DESC_EN_DESC_ID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x1, 2)
#define GET_TX_DESC_EN_DESC_ID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x1, 2)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_TX_BUFF_SIZE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define SET_TX_DESC_TX_BUFF_SIZE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define GET_TX_DESC_TX_BUFF_SIZE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xffff, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TXDESC_CHECKSUM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define SET_TX_DESC_TXDESC_CHECKSUM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define GET_TX_DESC_TXDESC_CHECKSUM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xffff, 0)
#define SET_TX_DESC_TIMESTAMP(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define SET_TX_DESC_TIMESTAMP_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0xffff, 0)
#define GET_TX_DESC_TIMESTAMP(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0xffff, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_SECTYPE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x3, 0)
#define SET_TX_DESC_SECTYPE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword7, value, 0x3, 0)
#define GET_TX_DESC_SECTYPE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword7, 0x3, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)


/*TXDESC_WORD8*/

#define SET_TX_DESC_TXWIFI_CP(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 31)
#define SET_TX_DESC_TXWIFI_CP_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 31)
#define GET_TX_DESC_TXWIFI_CP(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 31)
#define SET_TX_DESC_MAC_CP(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 30)
#define SET_TX_DESC_MAC_CP_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 30)
#define GET_TX_DESC_MAC_CP(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 30)
#define SET_TX_DESC_STW_PKTRE_DIS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 29)
#define SET_TX_DESC_STW_PKTRE_DIS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 29)
#define GET_TX_DESC_STW_PKTRE_DIS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 29)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MULTIPLE_PORT_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x7, 29)
#define SET_TX_DESC_MULTIPLE_PORT_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x7, 29)
#define GET_TX_DESC_MULTIPLE_PORT_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x7, 29)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_STW_RB_DIS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 28)
#define SET_TX_DESC_STW_RB_DIS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 28)
#define GET_TX_DESC_STW_RB_DIS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 28)
#define SET_TX_DESC_STW_RATE_DIS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 27)
#define SET_TX_DESC_STW_RATE_DIS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 27)
#define GET_TX_DESC_STW_RATE_DIS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 27)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_NULL_1_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 27)
#define SET_TX_DESC_NULL_1_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 27)
#define GET_TX_DESC_NULL_1_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 27)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_STW_ANT_DIS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 26)
#define SET_TX_DESC_STW_ANT_DIS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 26)
#define GET_TX_DESC_STW_ANT_DIS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 26)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_NULL_0_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 26)
#define SET_TX_DESC_NULL_0_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 26)
#define GET_TX_DESC_NULL_0_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 26)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_STW_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 25)
#define SET_TX_DESC_STW_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 25)
#define GET_TX_DESC_STW_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 25)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_TRI_FRAME_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 25)
#define SET_TX_DESC_TRI_FRAME_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 25)
#define GET_TX_DESC_TRI_FRAME_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 25)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SMH_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 24)
#define SET_TX_DESC_SMH_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 24)
#define GET_TX_DESC_SMH_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_TAILPAGE_L(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 24)
#define SET_TX_DESC_TAILPAGE_L_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 24)
#define GET_TX_DESC_TAILPAGE_L(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0xff, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_SPE_RPT_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 24)
#define SET_TX_DESC_SPE_RPT_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 24)
#define GET_TX_DESC_SPE_RPT_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 24)
#define SET_TX_DESC_FTM_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 23)
#define SET_TX_DESC_FTM_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 23)
#define GET_TX_DESC_FTM_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 23)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SDIO_DMASEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 16)
#define SET_TX_DESC_SDIO_DMASEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 16)
#define GET_TX_DESC_SDIO_DMASEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0xff, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_NEXTHEADPAGE_L(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 16)
#define SET_TX_DESC_NEXTHEADPAGE_L_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0xff, 16)
#define GET_TX_DESC_NEXTHEADPAGE_L(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0xff, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_MU_DATARATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x7f, 16)
#define SET_TX_DESC_MU_DATARATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x7f, 16)
#define GET_TX_DESC_MU_DATARATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x7f, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_EN_HWSEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 15)
#define SET_TX_DESC_EN_HWSEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 15)
#define GET_TX_DESC_EN_HWSEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 15)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT)

#define SET_TX_DESC_EN_HWEXSEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 14)
#define SET_TX_DESC_EN_HWEXSEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1, 14)
#define GET_TX_DESC_EN_HWEXSEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1, 14)

#endif

#if (HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_EN_HWSEQ_MODE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 14)
#define SET_TX_DESC_EN_HWSEQ_MODE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 14)
#define GET_TX_DESC_EN_HWSEQ_MODE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3, 14)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_CCA_RTS_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 14)
#define SET_TX_DESC_CCA_RTS_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 14)
#define GET_TX_DESC_CCA_RTS_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3, 14)
#define SET_TX_DESC_NDPA_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 12)
#define SET_TX_DESC_NDPA_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 12)
#define GET_TX_DESC_NDPA_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3, 12)
#define SET_TX_DESC_TXPWR_OFSET_TYPE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 9)
#define SET_TX_DESC_TXPWR_OFSET_TYPE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 9)
#define GET_TX_DESC_TXPWR_OFSET_TYPE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3, 9)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_DATA_RC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3f, 8)
#define SET_TX_DESC_DATA_RC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3f, 8)
#define GET_TX_DESC_DATA_RC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3f, 8)
#define SET_TX_DESC_BAR_RTY_TH(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 6)
#define SET_TX_DESC_BAR_RTY_TH_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3, 6)
#define GET_TX_DESC_BAR_RTY_TH(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3, 6)
#define SET_TX_DESC_RTS_RC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3f, 0)
#define SET_TX_DESC_RTS_RC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x3f, 0)
#define GET_TX_DESC_RTS_RC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x3f, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_P_AID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1ff, 0)
#define SET_TX_DESC_P_AID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword8, value, 0x1ff, 0)
#define GET_TX_DESC_P_AID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword8, 0x1ff, 0)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)


/*TXDESC_WORD9*/

#define SET_TX_DESC_TAILPAGE_H(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 28)
#define SET_TX_DESC_TAILPAGE_H_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 28)
#define GET_TX_DESC_TAILPAGE_H(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xf, 28)
#define SET_TX_DESC_NEXTHEADPAGE_H(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 24)
#define SET_TX_DESC_NEXTHEADPAGE_H_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 24)
#define GET_TX_DESC_NEXTHEADPAGE_H(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xf, 24)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_SW_DEFINE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xfff, 16)
#define SET_TX_DESC_SW_DEFINE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xfff, 16)
#define GET_TX_DESC_SW_DEFINE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xfff, 16)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_SW_SEQ(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xfff, 12)
#define SET_TX_DESC_SW_SEQ_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xfff, 12)
#define GET_TX_DESC_SW_SEQ(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xfff, 12)
#define SET_TX_DESC_TXBF_PATH(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1, 11)
#define SET_TX_DESC_TXBF_PATH_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1, 11)
#define GET_TX_DESC_TXBF_PATH(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0x1, 11)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_CTRL_CNT_VALID(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1, 9)
#define SET_TX_DESC_CTRL_CNT_VALID_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1, 9)
#define GET_TX_DESC_CTRL_CNT_VALID(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0x1, 9)
#define SET_TX_DESC_CTRL_CNT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 5)
#define SET_TX_DESC_CTRL_CNT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xf, 5)
#define GET_TX_DESC_CTRL_CNT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xf, 5)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8198F_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT || HALMAC_8733B_SUPPORT)

#define SET_TX_DESC_PADDING_LEN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x7ff, 0)
#define SET_TX_DESC_PADDING_LEN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x7ff, 0)
#define GET_TX_DESC_PADDING_LEN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0x7ff, 0)
#define SET_TX_DESC_GROUP_BIT_IE_OFFSET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xff, 0)
#define SET_TX_DESC_GROUP_BIT_IE_OFFSET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0xff, 0)
#define GET_TX_DESC_GROUP_BIT_IE_OFFSET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0xff, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_DATA_RTY_LOWEST_RATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1f, 0)
#define SET_TX_DESC_DATA_RTY_LOWEST_RATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword9, value, 0x1f, 0)
#define GET_TX_DESC_DATA_RTY_LOWEST_RATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword9, 0x1f, 0)

#endif

#if (HALMAC_8812F_SUPPORT)


/*WORD10*/

#define SET_TX_DESC_HT_DATA_SND(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 31)
#define SET_TX_DESC_HT_DATA_SND_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 31)
#define GET_TX_DESC_HT_DATA_SND(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x1, 31)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_PATH_MAPA(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 30)
#define SET_TX_DESC_PATH_MAPA_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 30)
#define GET_TX_DESC_PATH_MAPA(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3, 30)
#define SET_TX_DESC_PATH_MAPB(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 28)
#define SET_TX_DESC_PATH_MAPB_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 28)
#define GET_TX_DESC_PATH_MAPB(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3, 28)
#define SET_TX_DESC_PATH_MAPC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 26)
#define SET_TX_DESC_PATH_MAPC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 26)
#define GET_TX_DESC_PATH_MAPC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3, 26)
#define SET_TX_DESC_PATH_MAPD(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 24)
#define SET_TX_DESC_PATH_MAPD_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 24)
#define GET_TX_DESC_PATH_MAPD(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3, 24)
#define SET_TX_DESC_ANTSEL _ A(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 20)
#define SET_TX_DESC_ANTSEL _ A_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 20)
#define GET_TX_DESC_ANTSEL _ A(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 20)
#define SET_TX_DESC_ANTSEL _ B(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 17)
#define SET_TX_DESC_ANTSEL _ B_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 17)
#define GET_TX_DESC_ANTSEL _ B(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 17)

#endif

#if (HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SHCUT_CAM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3f, 16)
#define SET_TX_DESC_SHCUT_CAM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3f, 16)
#define GET_TX_DESC_SHCUT_CAM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3f, 16)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_ANTSEL _ C(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 13)
#define SET_TX_DESC_ANTSEL _ C_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 13)
#define GET_TX_DESC_ANTSEL _ C(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 13)
#define SET_TX_DESC_ANTSEL _ D(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 10)
#define SET_TX_DESC_ANTSEL _ D_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 10)
#define GET_TX_DESC_ANTSEL _ D(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 10)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_MU_DATARATE(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xff, 8)
#define SET_TX_DESC_MU_DATARATE_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xff, 8)
#define GET_TX_DESC_MU_DATARATE(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xff, 8)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_NTX_PATH_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 6)
#define SET_TX_DESC_NTX_PATH_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 6)
#define GET_TX_DESC_NTX_PATH_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 6)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_MU_RC(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 4)
#define SET_TX_DESC_MU_RC_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0xf, 4)
#define GET_TX_DESC_MU_RC(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0xf, 4)

#endif

#if (HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_NDPA_RATE_SEL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 3)
#define SET_TX_DESC_NDPA_RATE_SEL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 3)
#define GET_TX_DESC_NDPA_RATE_SEL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x1, 3)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_ANTLSEL_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 3)
#define SET_TX_DESC_ANTLSEL_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 3)
#define GET_TX_DESC_ANTLSEL_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x1, 3)

#endif

#if (HALMAC_8812F_SUPPORT)

#define SET_TX_DESC_HW_NDPA_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 2)
#define SET_TX_DESC_HW_NDPA_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x1, 2)
#define GET_TX_DESC_HW_NDPA_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x1, 2)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8822C_SUPPORT || HALMAC_8812F_SUPPORT || HALMAC_8197G_SUPPORT)

#define SET_TX_DESC_SND_PKT_SEL(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 0)
#define SET_TX_DESC_SND_PKT_SEL_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x3, 0)
#define GET_TX_DESC_SND_PKT_SEL(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x3, 0)

#endif

#if (HALMAC_8852A_SUPPORT)

#define SET_TX_DESC_AMPDU_DENSITY_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x7, 0)
#define SET_TX_DESC_AMPDU_DENSITY_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword10, value, 0x7, 0)
#define GET_TX_DESC_AMPDU_DENSITY_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword10, 0x7, 0)

/*WORD11*/

#define SET_TX_DESC_VCS_STBC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x3, 30)
#define SET_TX_DESC_VCS_STBC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x3, 30)
#define GET_TX_DESC_VCS_STBC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x3, 30)
#define SET_TX_DESC_DATA_STBC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x3, 28)
#define SET_TX_DESC_DATA_STBC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x3, 28)
#define GET_TX_DESC_DATA_STBC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x3, 28)
#define SET_TX_DESC_RTS_RTY_LOWEST_RATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0xf, 24)
#define SET_TX_DESC_RTS_RTY_LOWEST_RATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0xf, 24)
#define GET_TX_DESC_RTS_RTY_LOWEST_RATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0xf, 24)
#define SET_TX_DESC_SIGNALING_TA_PKT_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 23)
#define SET_TX_DESC_SIGNALING_TA_PKT_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 23)
#define GET_TX_DESC_SIGNALING_TA_PKT_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 23)
#define SET_TX_DESC_MHR_CP_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 22)
#define SET_TX_DESC_MHR_CP_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 22)
#define GET_TX_DESC_MHR_CP_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 22)
#define SET_TX_DESC_SMH_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 21)
#define SET_TX_DESC_SMH_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 21)
#define GET_TX_DESC_SMH_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 21)
#define SET_TX_DESC_RTSRATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1f, 16)
#define SET_TX_DESC_RTSRATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1f, 16)
#define GET_TX_DESC_RTSRATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1f, 16)
#define SET_TX_DESC_SMH_CAM_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0xff, 8)
#define SET_TX_DESC_SMH_CAM_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0xff, 8)
#define GET_TX_DESC_SMH_CAM_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0xff, 8)
#define SET_TX_DESC_ARFR_VHT_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 7)
#define SET_TX_DESC_ARFR_VHT_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 7)
#define GET_TX_DESC_ARFR_VHT_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 7)
#define SET_TX_DESC_ARFR_HT_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 6)
#define SET_TX_DESC_ARFR_HT_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 6)
#define GET_TX_DESC_ARFR_HT_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 6)
#define SET_TX_DESC_ARFR_OFDM_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 5)
#define SET_TX_DESC_ARFR_OFDM_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 5)
#define GET_TX_DESC_ARFR_OFDM_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 5)
#define SET_TX_DESC_ARFR_CCK_EN(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 4)
#define SET_TX_DESC_ARFR_CCK_EN_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 4)
#define GET_TX_DESC_ARFR_CCK_EN(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 4)
#define SET_TX_DESC_RTS_SHORT_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 3)
#define SET_TX_DESC_RTS_SHORT_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 3)
#define GET_TX_DESC_RTS_SHORT_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 3)
#define SET_TX_DESC_DISDATAFB_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 2)
#define SET_TX_DESC_DISDATAFB_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 2)
#define GET_TX_DESC_DISDATAFB_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 2)
#define SET_TX_DESC_DISRTSFB_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 1)
#define SET_TX_DESC_DISRTSFB_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 1)
#define GET_TX_DESC_DISRTSFB_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 1)
#define SET_TX_DESC_EXT_EDCA_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 0)
#define SET_TX_DESC_EXT_EDCA_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword11, value, 0x1, 0)
#define GET_TX_DESC_EXT_EDCA_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword11, 0x1, 0)

/*WORD12*/

#define SET_TX_DESC_AMPDU_MAX_TIME_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0xff, 24)
#define SET_TX_DESC_AMPDU_MAX_TIME_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0xff, 24)
#define GET_TX_DESC_AMPDU_MAX_TIME_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0xff, 24)
#define SET_TX_DESC_SPECIAL_CW_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 23)
#define SET_TX_DESC_SPECIAL_CW_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 23)
#define GET_TX_DESC_SPECIAL_CW_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 23)
#define SET_TX_DESC_RDG_NAV_EXT(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 22)
#define SET_TX_DESC_RDG_NAV_EXT_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 22)
#define GET_TX_DESC_RDG_NAV_EXT(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 22)
#define SET_TX_DESC_RAW_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 21)
#define SET_TX_DESC_RAW_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 21)
#define GET_TX_DESC_RAW_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 21)
#define SET_TX_DESC_MAX_AGG_NUM_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1f, 16)
#define SET_TX_DESC_MAX_AGG_NUM_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1f, 16)
#define GET_TX_DESC_MAX_AGG_NUM_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1f, 16)
#define SET_TX_DESC_FINAL_DATA_RATE_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0xff, 8)
#define SET_TX_DESC_FINAL_DATA_RATE_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0xff, 8)
#define GET_TX_DESC_FINAL_DATA_RATE_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0xff, 8)
#define SET_TX_DESC_GF_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 7)
#define SET_TX_DESC_GF_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 7)
#define GET_TX_DESC_GF_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 7)
#define SET_TX_DESC_MOREFRAG_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 6)
#define SET_TX_DESC_MOREFRAG_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 6)
#define GET_TX_DESC_MOREFRAG_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 6)
#define SET_TX_DESC_NO_ACM_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 5)
#define SET_TX_DESC_NO_ACM_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 5)
#define GET_TX_DESC_NO_ACM_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 5)
#define SET_TX_DESC_HTC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 4)
#define SET_TX_DESC_HTC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 4)
#define GET_TX_DESC_HTC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 4)
#define SET_TX_DESC_TX_PKT_AFTER_PIFS(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 3)
#define SET_TX_DESC_TX_PKT_AFTER_PIFS_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 3)
#define GET_TX_DESC_TX_PKT_AFTER_PIFS(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 3)
#define SET_TX_DESC_USE_MAX_TIME_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 2)
#define SET_TX_DESC_USE_MAX_TIME_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x1, 2)
#define GET_TX_DESC_USE_MAX_TIME_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x1, 2)
#define SET_TX_DESC_HW_SSN_SEL_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x3, 0)
#define SET_TX_DESC_HW_SSN_SEL_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword12, value, 0x3, 0)
#define GET_TX_DESC_HW_SSN_SEL_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword12, 0x3, 0)

/*WORD13*/

#define SET_TX_DESC_ADDR_CAM(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xff, 24)
#define SET_TX_DESC_ADDR_CAM_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xff, 24)
#define GET_TX_DESC_ADDR_CAM(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0xff, 24)
#define SET_TX_DESC_SND_TARGET(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xff, 16)
#define SET_TX_DESC_SND_TARGET_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xff, 16)
#define GET_TX_DESC_SND_TARGET(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0xff, 16)
#define SET_TX_DESC_DATA_LDPC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x1, 15)
#define SET_TX_DESC_DATA_LDPC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x1, 15)
#define GET_TX_DESC_DATA_LDPC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0x1, 15)
#define SET_TX_DESC_LSIG_TXOP_EN_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x1, 14)
#define SET_TX_DESC_LSIG_TXOP_EN_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x1, 14)
#define GET_TX_DESC_LSIG_TXOP_EN_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0x1, 14)
#define SET_TX_DESC_G_ID_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x3f, 8)
#define SET_TX_DESC_G_ID_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0x3f, 8)
#define GET_TX_DESC_G_ID_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0x3f, 8)
#define SET_TX_DESC_SIGNALING_TA_PKT_SC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xf, 4)
#define SET_TX_DESC_SIGNALING_TA_PKT_SC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xf, 4)
#define GET_TX_DESC_SIGNALING_TA_PKT_SC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0xf, 4)
#define SET_TX_DESC_DATA_SC_V2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xf, 0)
#define SET_TX_DESC_DATA_SC_V2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword13, value, 0xf, 0)
#define GET_TX_DESC_DATA_SC_V2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword13, 0xf, 0)

/*WORD14*/

#define SET_TX_DESC_LEN1_L(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0x7f, 17)
#define SET_TX_DESC_LEN1_L_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0x7f, 17)
#define GET_TX_DESC_LEN1_L(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword14, 0x7f, 17)
#define SET_TX_DESC_LEN0(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0x1fff, 4)
#define SET_TX_DESC_LEN0_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0x1fff, 4)
#define GET_TX_DESC_LEN0(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword14, 0x1fff, 4)
#define SET_TX_DESC_PACKET NUMBER <=15(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0xf, 0)
#define SET_TX_DESC_PACKET NUMBER <=15_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword14, value, 0xf, 0)
#define GET_TX_DESC_PACKET NUMBER <=15(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword14, 0xf, 0)

/*WORD15*/

#define SET_TX_DESC_LEN3(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x1fff, 19)
#define SET_TX_DESC_LEN3_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x1fff, 19)
#define GET_TX_DESC_LEN3(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword15, 0x1fff, 19)
#define SET_TX_DESC_LEN2(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x1fff, 6)
#define SET_TX_DESC_LEN2_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x1fff, 6)
#define GET_TX_DESC_LEN2(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword15, 0x1fff, 6)
#define SET_TX_DESC_LEN1_H(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x3f, 0)
#define SET_TX_DESC_LEN1_H_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword15, value, 0x3f, 0)
#define GET_TX_DESC_LEN1_H(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword15, 0x3f, 0)

/*WORD16*/

#define SET_TX_DESC_AES_IV_L(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword16, value, 0xffffffff, 0)
#define SET_TX_DESC_AES_IV_L_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword16, value, 0xffffffff, 0)
#define GET_TX_DESC_AES_IV_L(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword16, 0xffffffff, 0)

/*WORD17*/

#define SET_TX_DESC_AES_IV_H(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword17, value, 0xffff, 0)
#define SET_TX_DESC_AES_IV_H_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword17, value, 0xffff, 0)
#define GET_TX_DESC_AES_IV_H(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword17, 0xffff, 0)

/*WORD18*/

#define SET_TX_DESC_TCP_IP_CHECKSUM_H(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword18, value, 0xffffffff, 0)
#define SET_TX_DESC_TCP_IP_CHECKSUM_H_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword18, value, 0xffffffff, 0)
#define GET_TX_DESC_TCP_IP_CHECKSUM_H(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword18, 0xffffffff, 0)

/*WORD19*/

#define SET_TX_DESC_TCP_IP_CHECKSUM_L(txdesc, value)    HALMAC_SET_DESC_FIELD_CLR(((struct halmac_tx_desc *)txdesc)->dword19, value, 0xffffffff, 0)
#define SET_TX_DESC_TCP_IP_CHECKSUM_L_NO_CLR(txdesc, value)    HALMAC_SET_DESC_FIELD_NO_CLR(((struct halmac_tx_desc *)txdesc)->dword19, value, 0xffffffff, 0)
#define GET_TX_DESC_TCP_IP_CHECKSUM_L(txdesc)    HALMAC_GET_DESC_FIELD(((struct halmac_tx_desc *)txdesc)->dword19, 0xffffffff, 0)

#endif


#endif

