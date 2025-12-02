#ifndef _AICWF_MANAGER_H_
#define _AICWF_MANAGER_H_

#include "lmac_types.h"
#include "rwnx_defs.h"

#define DRV_WLAN_MANAGER_VER       "v1.0"


/* TBD */
#define CONFIG_IFACE_NUMBER        1

#define NL_AIC_PROTOCOL_FRT_DRV    28
#define NL_AIC_PROTOCOL_SEC_DRV    29
#define NL_AIC_PROTOCOL            NL_AIC_PROTOCOL_SEC_DRV

#define NL_AIC_MANAGER_PID        5185

#define NL_MAX_MSG_SIZE            768

/* Netlink Message Type List */
#define AIC_NL_DAEMON_ON_TYPE            1
#define AIC_NL_DAEMON_OFF_TYPE           2
#define AIC_NL_DAEMON_ALIVE_TYPE         3
#define AIC_NL_DEL_STA_TYPE              4
#define AIC_NL_NEW_STA_TYPE              5
#define AIC_NL_INTF_RPT_TYPE             6
#define AIC_NL_STA_RPT_TYPE              7
#define AIC_NL_FRAME_RPT_TYPE            8
#define AIC_NL_TIME_TICK_TYPE            9
#define AIC_NL_PRIV_INFO_CMD_TYPE        10
#ifdef CONFIG_BAND_STEERING
#define AIC_NL_B_STEER_CMD_TYPE          11
#define AIC_NL_B_STEER_BLOCK_ADD_TYPE    12
#define AIC_NL_B_STEER_BLOCK_DEL_TYPE    13
#define AIC_NL_B_STEER_ROAM_TYPE         14
#endif

#define AIC_NL_GENERAL_CMD_TYPE          100
#define AIC_NL_CUSTOMER_TYPE             101
#define AIC_NL_CONFIG_UPDATE_TYPE        102

/* Element ID */
#define AIC_ELM_INTF_ID            1
#define AIC_ELM_INTF_INFO_ID       2
#define AIC_ELM_FRAME_INFO_ID      3
#define AIC_ELM_STA_INFO_ID        4
#define AIC_ELM_ROAM_INFO_ID       5
#define AIC_ELM_BUFFER_ID          6
#define AIC_ELM_STA_INFO_EXT_ID    7

#define LOW_RSSI_IGNORE              (-85)

struct b_nl_message {
	u32_l type;
	u32_l len;
	u8_l  content[NL_MAX_MSG_SIZE];
};

struct b_elm_header {
	u8_l id;
	u8_l len;
};

struct b_elm_intf {
	u8_l  mac[6];
	u8_l  root;
	u8_l  band;
	u8_l  ssid;
	s8_l  name[16];
};

struct b_elm_intf_info {
	u16_l ch;
	u8_l  ch_clm;
	u8_l  ch_noise;
	u32_l tx_tp;
	u32_l rx_tp;
	u32_l assoc_sta_num;
	/* self neighbor report info */
	u32_l bss_info;
	u8_l  reg_class;
	u8_l  phy_type;
};

struct b_elm_frame_info {
	u16_l frame_type;
	u8_l  sa[6];
	s8_l  rssi;
};

struct b_elm_sta_info {
	u8_l  mac[6];
	s8_l rssi;
	u32_l link_time;
	u32_l tx_tp; /* kbits */
	u32_l rx_tp; /* kbits */
};

struct b_elm_roam_info {
	u8_l  sta_mac[6]; /* station mac */
	u8_l  bss_mac[6]; /* target bss mac */
	u16_l bss_ch; /* target bss channel */
	u8_l  method; /* 0: 11V, 1: Deauth */
};

struct b_elm_buffer {
	u8_l buf[255];
};

struct b_elm_sta_info_ext {
	u8_l mac[6];
	u8_l supported_band; /* bit0:2g bit1:5g */
	u8_l empty[119]; /* for future use */
};


/* Element Size List */
#define ELM_HEADER_LEN            (sizeof(struct b_elm_header))
#define ELM_INTF_LEN              (sizeof(struct b_elm_intf))
#define ELM_INTF_INFO_LEN         (sizeof(struct b_elm_intf_info))
#define ELM_FRAME_INFO_LEN        (sizeof(struct b_elm_frame_info))
#define ELM_STA_INFO_LEN          (sizeof(struct b_elm_sta_info))
#define ELM_ROAM_INFO_LEN         (sizeof(struct b_elm_roam_info))
#define ELM_BUFFER_LEN            (sizeof(struct b_elm_buffer))
#define ELM_STA_INFO_EXT_LEN      (sizeof(struct b_elm_sta_info_ext))



void aicwf_nl_send_del_sta_msg(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_nl_send_new_sta_msg(struct rwnx_vif *rwnx_vif, u8_l *mac);
void aicwf_nl_send_intf_rpt_msg(struct rwnx_vif *rwnx_vif);
void aicwf_nl_send_sta_rpt_msg(struct rwnx_vif *rwnx_vif, struct rwnx_sta *sta);
void aicwf_nl_send_frame_rpt_msg(struct rwnx_vif *rwnx_vif, u16_l frame_type, u8_l *sa, s8_l rssi);
void aicwf_nl_send_time_tick_msg(struct rwnx_vif *rwnx_vif);
void aicwf_nl_hook(struct rwnx_vif *rwnx_vif, u8_l band, u8_l iface_id);
void aicwf_nl_hook_deinit(u8_l band, u8_l ssid);
void aicwf_nl_init(void);
void aicwf_nl_deinit(void);
void aicwf_wlan_manager_recv_msg(struct b_nl_message *msg);
void aicwf_nl_recv_msg(struct sk_buff *skb);
void aicwf_nl_send_msg(void *msg, u32_l msg_len);


#endif

