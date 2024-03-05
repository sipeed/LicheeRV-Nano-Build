#ifndef __VPSS_CB_H__
#define __VPSS_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <base_ctx.h>
#include <dwa_cb.h>

struct ol_tile {
	u16 start;
	u16 end;
};

struct sc_cfg_cb {
	struct _ol_tile_cfg {
		struct ol_tile l_in;
		struct ol_tile l_out;
		struct ol_tile r_in;
		struct ol_tile r_out;
	} ol_tile_cfg;

	u8  snr_num;
	u8  bypass_num;
	u8  is_tile;
	u8  is_left_tile;
	struct mlv_i_s m_lv_i;
};

struct sc_err_handle_cb {
	u8  snr_num;
};

struct vpss_grp_sbm_cfg {
	u16 grp;
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u32 ion_size;
	u64 ion_paddr;
};

struct vpss_vc_sbm_flow_cfg {
	u16 vpss_grp;
	u8 vpss_chn;
	u8 ready;
};

struct vpss_grp_mlv_info {
	u16 vpss_grp;
	struct mlv_i_s m_lv_i;
};

enum VPSS_CB_CMD {
	VPSS_CB_VI_ONLINE_TRIGGER,
	VPSS_CB_QBUF_TRIGGER,
	VPSS_CB_SET_VIVPSSMODE,
	VPSS_CB_SET_VPSSMODE,
	VPSS_CB_SET_VPSSMODE_EX,
	VPSS_CB_GET_RGN_HDLS,
	VPSS_CB_SET_RGN_HDLS,
	VPSS_CB_SET_RGN_CFG,
	VPSS_CB_SET_RGNEX_CFG,
	VPSS_CB_SET_COVEREX_CFG,
	VPSS_CB_SET_MOSAIC_CFG,
	VPSS_CB_SET_RGN_LUT_CFG,
	VPSS_CB_GET_RGN_OW_ADDR,
	VPSS_CB_GET_CHN_SIZE,
	VPSS_CB_ONLINE_ERR_HANDLE,
	VPSS_CB_SET_DWA_2_VPSS_SBM,
	VPSS_CB_VC_SET_SBM_FLOW,
	VPSS_CB_GET_MLV_INFO,
	VPSS_CB_GDC_OP_DONE = DWA_CB_GDC_OP_DONE,
	VPSS_CB_OVERFLOW_CHECK,
	VPSS_CB_VO_STATUS_SET,
	VPSS_CB_SET_FB_ON_VPSS,
	VPSS_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __VPSS_CB_H__ */
