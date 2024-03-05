#ifndef _HALMAC_RX_DESC_CHIP_H_
#define _HALMAC_RX_DESC_CHIP_H_
#if (HALMAC_8814A_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8814A(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8814A(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8814A(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8814A(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8814A(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8814A(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8814A(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8814A(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8814A(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8814A(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8814A(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8814A(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8814A(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8814A(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8814A(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8814A(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8814A(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8814A(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8814A(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8814A(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8814A(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8814A(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8814A(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8814A(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8814A(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8814A(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8814A(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8814A(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8814A(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_C2H_8814A(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8814A(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8814A(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8814A(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8814A(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8814A(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8814A(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8814A(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8814A(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8814A(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8814A(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8814A(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8814A(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8814A(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8814A(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8814A(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8814A(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8814A(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8814A(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8814A(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8814A(rxdesc)    GET_RX_DESC_PATTERN_IDX(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8814A(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8822B_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8822B(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8822B(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8822B(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8822B(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8822B(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8822B(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8822B(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8822B(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8822B(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8822B(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8822B(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8822B(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8822B(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8822B(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8822B(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8822B(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8822B(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8822B(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8822B(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8822B(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8822B(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8822B(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8822B(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8822B(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8822B(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8822B(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8822B(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8822B(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8822B(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8822B(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8822B(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8822B(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8822B(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8822B(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8822B(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8822B(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8822B(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8822B(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8822B(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8822B(rxdesc)    GET_RX_DESC_RXPAYLOAD_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8822B(rxdesc)    GET_RX_DESC_RXPAYLOAD_ID(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8822B(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8822B(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8822B(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8822B(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8822B(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8822B(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8822B(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8822B(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8822B(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8822B(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8822B(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8822B(rxdesc)    GET_RX_DESC_PATTERN_IDX(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8822B(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8197F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8197F(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8197F(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8197F(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8197F(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8197F(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8197F(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8197F(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8197F(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8197F(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8197F(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8197F(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8197F(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8197F(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8197F(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8197F(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8197F(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8197F(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8197F(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8197F(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8197F(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8197F(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8197F(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8197F(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8197F(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8197F(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8197F(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8197F(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8197F(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8197F(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_C2H_8197F(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8197F(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8197F(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8197F(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8197F(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8197F(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8197F(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8197F(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8197F(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8197F(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8197F(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8197F(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8197F(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8197F(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8197F(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8197F(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8197F(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8197F(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8197F(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8197F(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_FC_POWER_8197F(rxdesc)    GET_RX_DESC_FC_POWER(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8197F(rxdesc)    GET_RX_DESC_PATTERN_IDX_V1(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8197F(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8821C_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8821C(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8821C(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8821C(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8821C(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8821C(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8821C(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8821C(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8821C(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8821C(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8821C(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8821C(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8821C(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8821C(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8821C(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8821C(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8821C(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8821C(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8821C(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8821C(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8821C(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8821C(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8821C(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8821C(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8821C(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8821C(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8821C(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8821C(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8821C(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8821C(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8821C(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8821C(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8821C(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8821C(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8821C(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8821C(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8821C(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8821C(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8821C(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8821C(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8821C(rxdesc)    GET_RX_DESC_RXPAYLOAD_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8821C(rxdesc)    GET_RX_DESC_RXPAYLOAD_ID(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8821C(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8821C(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8821C(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8821C(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8821C(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8821C(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8821C(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8821C(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8821C(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8821C(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8821C(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8821C(rxdesc)    GET_RX_DESC_PATTERN_IDX(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8821C(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8198F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8198F(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8198F(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8198F(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8198F(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8198F(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8198F(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8198F(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8198F(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8198F(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8198F(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8198F(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8198F(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8198F(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8198F(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8198F(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8198F(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8198F(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8198F(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8198F(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8198F(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8198F(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8198F(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8198F(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8198F(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8198F(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8198F(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8198F(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8198F(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8198F(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8198F(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8198F(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8198F(rxdesc)    GET_RX_DESC_HWRSVD_V1(rxdesc)
#define GET_RX_DESC_RXMAGPKT_8198F(rxdesc)    GET_RX_DESC_RXMAGPKT(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8198F(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8198F(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8198F(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8198F(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8198F(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8198F(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8198F(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8198F(rxdesc)    GET_RX_DESC_RXPAYLOAD_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8198F(rxdesc)    GET_RX_DESC_RXPAYLOAD_ID(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8198F(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8198F(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8198F(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8198F(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8198F(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8198F(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_A1_8198F(rxdesc)    GET_RX_DESC_A1_FIT_A1(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8198F(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8198F(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8198F(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8198F(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_FC_POWER_8198F(rxdesc)    GET_RX_DESC_FC_POWER(rxdesc)
#define GET_RX_DESC_TXRPTMID_CTL_MASK_8198F(rxdesc)    GET_RX_DESC_TXRPTMID_CTL_MASK(rxdesc)
#define GET_RX_DESC_SWPS_RPT_8198F(rxdesc)    GET_RX_DESC_SWPS_RPT(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8198F(rxdesc)    GET_RX_DESC_PATTERN_IDX_V1(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8198F(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8822C_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8822C(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8822C(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8822C(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8822C(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8822C(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8822C(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8822C(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8822C(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8822C(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8822C(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8822C(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8822C(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8822C(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8822C(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8822C(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8822C(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8822C(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8822C(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8822C(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8822C(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8822C(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8822C(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8822C(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8822C(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8822C(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8822C(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8822C(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8822C(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8822C(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8822C(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8822C(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8822C(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8822C(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_STATISTICS_8822C(rxdesc)    GET_RX_DESC_RX_STATISTICS(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8822C(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8822C(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8822C(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8822C(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8822C(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8822C(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8822C(rxdesc)    GET_RX_DESC_RXPAYLOAD_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8822C(rxdesc)    GET_RX_DESC_RXPAYLOAD_ID(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8822C(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8822C(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8822C(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8822C(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8822C(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8822C(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8822C(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8822C(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8822C(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8822C(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8822C(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8822C(rxdesc)    GET_RX_DESC_PATTERN_IDX(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8822C(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8197G_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8197G(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8197G(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8197G(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8197G(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8197G(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8197G(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8197G(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8197G(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8197G(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8197G(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8197G(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8197G(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8197G(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8197G(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8197G(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8197G(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8197G(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8197G(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8197G(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8197G(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8197G(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8197G(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8197G(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8197G(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8197G(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8197G(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8197G(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8197G(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8197G(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8197G(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8197G(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8197G(rxdesc)    GET_RX_DESC_HWRSVD_V1(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8197G(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_STATISTICS_8197G(rxdesc)    GET_RX_DESC_RX_STATISTICS(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8197G(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8197G(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8197G(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_DMA_AGG_NUM_8197G(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8197G(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8197G(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8197G(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8197G(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8197G(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_A1_8197G(rxdesc)    GET_RX_DESC_A1_FIT_A1(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8197G(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8197G(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8197G(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8197G(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_FC_POWER_8197G(rxdesc)    GET_RX_DESC_FC_POWER(rxdesc)
#define GET_RX_DESC_TXRPTMID_CTL_MASK_8197G(rxdesc)    GET_RX_DESC_TXRPTMID_CTL_MASK(rxdesc)
#define GET_RX_DESC_SWPS_RPT_8197G(rxdesc)    GET_RX_DESC_SWPS_RPT(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8197G(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8812F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8812F(rxdesc)    GET_RX_DESC_EOR(rxdesc)
#define GET_RX_DESC_PHYPKTIDC_8812F(rxdesc)    GET_RX_DESC_PHYPKTIDC(rxdesc)
#define GET_RX_DESC_SWDEC_8812F(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8812F(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8812F(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8812F(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8812F(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8812F(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8812F(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8812F(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8812F(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8812F(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8812F(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8812F(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8812F(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8812F(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8812F(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8812F(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8812F(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8812F(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8812F(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8812F(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8812F(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8812F(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8812F(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8812F(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8812F(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_MACID_8812F(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8812F(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8812F(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8812F(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_HWRSVD_8812F(rxdesc)    GET_RX_DESC_HWRSVD(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8812F(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_RX_STATISTICS_8812F(rxdesc)    GET_RX_DESC_RX_STATISTICS(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8812F(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8812F(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8812F(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8812F(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8812F(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8812F(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8812F(rxdesc)    GET_RX_DESC_RXPAYLOAD_MATCH(rxdesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8812F(rxdesc)    GET_RX_DESC_RXPAYLOAD_ID(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8812F(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8812F(rxdesc)    GET_RX_DESC_BSSID_FIT_1_0(rxdesc)
#define GET_RX_DESC_EOSP_8812F(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8812F(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8812F(rxdesc)    GET_RX_DESC_BSSID_FIT_4_2(rxdesc)
#define GET_RX_DESC_RX_RATE_8812F(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8812F(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8812F(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8812F(rxdesc)    GET_RX_DESC_RX_PRE_NDP_VLD(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8812F(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8812F(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_PATTERN_IDX_8812F(rxdesc)    GET_RX_DESC_PATTERN_IDX(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8812F(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif

#if (HALMAC_8852A_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_BAND_SEL_8852A(rxdesc)    GET_RX_DESC_BAND_SEL(rxdesc)
#define GET_RX_DESC_PKT TYPE_8852A(rxdesc)    GET_RX_DESC_PKT TYPE(rxdesc)
#define GET_RX_DESC_AMPDU_8852A(rxdesc)    GET_RX_DESC_AMPDU(rxdesc)
#define GET_RX_DESC_AMPDU_8852A(rxdesc)    GET_RX_DESC_AMPDU(rxdesc)
#define GET_RX_DESC_AMPDU_END_PKT_8852A(rxdesc)    GET_RX_DESC_AMPDU_END_PKT(rxdesc)
#define GET_RX_DESC_CRC32_8852A(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8852A(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8852A(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8852A(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TYPE_8852A(rxdesc)    GET_RX_DESC_TYPE(rxdesc)
#define GET_RX_DESC_MF_8852A(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8852A(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8852A(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_A1_MATCH_8852A(rxdesc)    GET_RX_DESC_A1_MATCH(rxdesc)
#define GET_RX_DESC_A1_MATCH_8852A(rxdesc)    GET_RX_DESC_A1_MATCH(rxdesc)
#define GET_RX_DESC_TCP_CHKSUM_VLD_8852A(rxdesc)    GET_RX_DESC_TCP_CHKSUM_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8852A(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8852A(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_TCP_CHKSUM_ERR_8852A(rxdesc)    GET_RX_DESC_TCP_CHKSUM_ERR(rxdesc)
#define GET_RX_DESC_PHY_PKT_IDC_8852A(rxdesc)    GET_RX_DESC_PHY_PKT_IDC(rxdesc)
#define GET_RX_DESC_FW_FIFO_FULL_8852A(rxdesc)    GET_RX_DESC_FW_FIFO_FULL(rxdesc)
#define GET_RX_DESC_RXCMD_IDC_8852A(rxdesc)    GET_RX_DESC_RXCMD_IDC(rxdesc)
#define GET_RX_DESC_AMSDU_8852A(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8852A(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8852A(rxdesc)    GET_RX_DESC_TID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_PPDU_CNT_8852A(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_BSSID_FIT_8852A(rxdesc)    GET_RX_DESC_BSSID_FIT(rxdesc)
#define GET_RX_DESC_ADDRESS_CAM_VLD_8852A(rxdesc)    GET_RX_DESC_ADDRESS_CAM_VLD(rxdesc)
#define GET_RX_DESC_ADDRESS_CAM_8852A(rxdesc)    GET_RX_DESC_ADDRESS_CAM(rxdesc)
#define GET_RX_DESC_SW_DEC_8852A(rxdesc)    GET_RX_DESC_SW_DEC(rxdesc)
#define GET_RX_DESC_SEC CAM INDEX_8852A(rxdesc)    GET_RX_DESC_SEC CAM INDEX(rxdesc)
#define GET_RX_DESC_SEQ_8852A(rxdesc)    GET_RX_DESC_SEQ(rxdesc)
#define GET_RX_DESC_MAC_ID_8852A(rxdesc)    GET_RX_DESC_MAC_ID(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_RX_RATE_8852A(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)
#define GET_RX_DESC_FREERUN_CNT_8852A(rxdesc)    GET_RX_DESC_FREERUN_CNT(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_RKT TYPE_8852A(rxdesc)    GET_RX_DESC_RKT TYPE(rxdesc)

/*RXDESC_WORD5*/


#endif

#if (HALMAC_8733B_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_SENT_RXCMD_8733B(rxdesc)    GET_RX_DESC_SENT_RXCMD(rxdesc)
#define GET_RX_DESC_FW_STS_PKT_8733B(rxdesc)    GET_RX_DESC_FW_STS_PKT(rxdesc)
#define GET_RX_DESC_SWDEC_8733B(rxdesc)    GET_RX_DESC_SWDEC(rxdesc)
#define GET_RX_DESC_PHYST_8733B(rxdesc)    GET_RX_DESC_PHYST(rxdesc)
#define GET_RX_DESC_SHIFT_8733B(rxdesc)    GET_RX_DESC_SHIFT(rxdesc)
#define GET_RX_DESC_QOS_8733B(rxdesc)    GET_RX_DESC_QOS(rxdesc)
#define GET_RX_DESC_SECURITY_8733B(rxdesc)    GET_RX_DESC_SECURITY(rxdesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8733B(rxdesc)    GET_RX_DESC_DRV_INFO_SIZE(rxdesc)
#define GET_RX_DESC_ICV_ERR_8733B(rxdesc)    GET_RX_DESC_ICV_ERR(rxdesc)
#define GET_RX_DESC_CRC32_8733B(rxdesc)    GET_RX_DESC_CRC32(rxdesc)
#define GET_RX_DESC_PKT_LEN_8733B(rxdesc)    GET_RX_DESC_PKT_LEN(rxdesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8733B(rxdesc)    GET_RX_DESC_BC(rxdesc)
#define GET_RX_DESC_MC_8733B(rxdesc)    GET_RX_DESC_MC(rxdesc)
#define GET_RX_DESC_TY_PE_8733B(rxdesc)    GET_RX_DESC_TY_PE(rxdesc)
#define GET_RX_DESC_MF_8733B(rxdesc)    GET_RX_DESC_MF(rxdesc)
#define GET_RX_DESC_MD_8733B(rxdesc)    GET_RX_DESC_MD(rxdesc)
#define GET_RX_DESC_PWR_8733B(rxdesc)    GET_RX_DESC_PWR(rxdesc)
#define GET_RX_DESC_PAM_8733B(rxdesc)    GET_RX_DESC_PAM(rxdesc)
#define GET_RX_DESC_CHK_VLD_8733B(rxdesc)    GET_RX_DESC_CHK_VLD(rxdesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8733B(rxdesc)    GET_RX_DESC_RX_IS_TCP_UDP(rxdesc)
#define GET_RX_DESC_RX_IPV_8733B(rxdesc)    GET_RX_DESC_RX_IPV(rxdesc)
#define GET_RX_DESC_CHKERR_8733B(rxdesc)    GET_RX_DESC_CHKERR(rxdesc)
#define GET_RX_DESC_PAGGR_8733B(rxdesc)    GET_RX_DESC_PAGGR(rxdesc)
#define GET_RX_DESC_RXID_MATCH_8733B(rxdesc)    GET_RX_DESC_RXID_MATCH(rxdesc)
#define GET_RX_DESC_AMSDU_8733B(rxdesc)    GET_RX_DESC_AMSDU(rxdesc)
#define GET_RX_DESC_MACID_VLD_8733B(rxdesc)    GET_RX_DESC_MACID_VLD(rxdesc)
#define GET_RX_DESC_TID_8733B(rxdesc)    GET_RX_DESC_TID(rxdesc)
#define GET_RX_DESC_EXT_SECTYPE_8733B(rxdesc)    GET_RX_DESC_EXT_SECTYPE(rxdesc)
#define GET_RX_DESC_MACID_8733B(rxdesc)    GET_RX_DESC_MACID(rxdesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8733B(rxdesc)    GET_RX_DESC_FCS_OK(rxdesc)
#define GET_RX_DESC_PPDU_CNT_8733B(rxdesc)    GET_RX_DESC_PPDU_CNT(rxdesc)
#define GET_RX_DESC_C2H_8733B(rxdesc)    GET_RX_DESC_C2H(rxdesc)
#define GET_RX_DESC_RXMAGPKT_8733B(rxdesc)    GET_RX_DESC_RXMAGPKT(rxdesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8733B(rxdesc)    GET_RX_DESC_WLANHD_IV_LEN(rxdesc)
#define GET_RX_DESC_IFS_CNT0_PHY_8733B(rxdesc)    GET_RX_DESC_IFS_CNT0_PHY(rxdesc)
#define GET_RX_DESC_RX_IS_QOS_8733B(rxdesc)    GET_RX_DESC_RX_IS_QOS(rxdesc)
#define GET_RX_DESC_FRAG_8733B(rxdesc)    GET_RX_DESC_FRAG(rxdesc)
#define GET_RX_DESC_SEQ_8733B(rxdesc)    GET_RX_DESC_SEQ(rxdesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8733B(rxdesc)    GET_RX_DESC_MAGIC_WAKE(rxdesc)
#define GET_RX_DESC_UNICAST_WAKE_8733B(rxdesc)    GET_RX_DESC_UNICAST_WAKE(rxdesc)
#define GET_RX_DESC_PATTERN_MATCH_8733B(rxdesc)    GET_RX_DESC_PATTERN_MATCH(rxdesc)
#define GET_RX_DESC_DMA_AGG_NUM_8733B(rxdesc)    GET_RX_DESC_DMA_AGG_NUM(rxdesc)
#define GET_RX_DESC_BSSID_FIT_L_8733B(rxdesc)    GET_RX_DESC_BSSID_FIT_L(rxdesc)
#define GET_RX_DESC_EOSP_8733B(rxdesc)    GET_RX_DESC_EOSP(rxdesc)
#define GET_RX_DESC_HTC_8733B(rxdesc)    GET_RX_DESC_HTC(rxdesc)
#define GET_RX_DESC_BSSID_FIT_H_8733B(rxdesc)    GET_RX_DESC_BSSID_FIT_H(rxdesc)
#define GET_RX_DESC_RX_RATE_8733B(rxdesc)    GET_RX_DESC_RX_RATE(rxdesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8733B(rxdesc)    GET_RX_DESC_A1_FIT(rxdesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8733B(rxdesc)    GET_RX_DESC_MACID_RPT_BUFF(rxdesc)
#define GET_RX_DESC_RX_PRE_NDP_OK_8733B(rxdesc)    GET_RX_DESC_RX_PRE_NDP_OK(rxdesc)
#define GET_RX_DESC_RX_SCRAMBLER_8733B(rxdesc)    GET_RX_DESC_RX_SCRAMBLER(rxdesc)
#define GET_RX_DESC_RX_EOF_8733B(rxdesc)    GET_RX_DESC_RX_EOF(rxdesc)
#define GET_RX_DESC_FC_POWER_8733B(rxdesc)    GET_RX_DESC_FC_POWER(rxdesc)
#define GET_RX_DESC_TXRPT_MID_CTL_MASK_8733B(rxdesc)    GET_RX_DESC_TXRPT_MID_CTL_MASK(rxdesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8733B(rxdesc)    GET_RX_DESC_TSFL(rxdesc)

#endif


#endif

