#ifndef __COMMON_VIP_H__
#define __COMMON_VIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "reg_vip_sys.h"

#define GOP_ALIGNMENT 0x10

#define VIP_NORM_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_NORM_DIV_##CLK_NAME##_MASK
#define VIP_NORM_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_NORM_DIV_##CLK_NAME##_OFFSET
#define VIP_NORM_CLK_RATIO_CONFIG(CLK_NAME, RATIO) \
		vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_##CLK_NAME, \
			VIP_NORM_CLK_RATIO_MASK(CLK_NAME), \
			RATIO << VIP_NORM_CLK_RATIO_OFFSET(CLK_NAME))

#define VIP_UPDATE_CLK_RATIO_MASK(CLK_NAME) VIP_SYS_REG_UPDATE_##CLK_NAME##_MASK
#define VIP_UPDATE_CLK_RATIO_OFFSET(CLK_NAME) VIP_SYS_REG_UPDATE_##CLK_NAME##_OFFSET
#define VIP_UPDATE_CLK_RATIO(CLK_NAME) \
	vip_sys_reg_write_mask(VIP_SYS_REG_UPDATE_##CLK_NAME, \
		VIP_UPDATE_CLK_RATIO_MASK(CLK_NAME), \
		1 << VIP_UPDATE_CLK_RATIO_OFFSET(CLK_NAME))

union vip_sys_reset {
	struct {
		u32 resv_b0 : 1;
		u32 isp_top : 1;
		u32 img_d : 1;
		u32 img_v : 1;
		u32 sc_top : 1;
		u32 sc_d : 1;
		u32 sc_v1 : 1;
		u32 sc_v2 : 1;
		u32 sc_v3 : 1;
		u32 disp : 1;
		u32 bt : 1;
		u32 dsi_mac : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 ldc : 1;
		u32 clk_div : 1;
		u32 csi_mac2 : 1;
		u32 isp_top_apb : 1;
		u32 sc_top_apb : 1;
		u32 ldc_top_apb : 1;
		u32 dsi_mac_apb : 1;
		u32 csi_mac0_apb : 1;
		u32 csi_mac1_apb : 1;
		u32 dsi_phy_apb : 1;
		u32 csi_phy0_apb : 1;
		u32 rsv_b25 : 1;
		u32 dsi_phy : 1;
		u32 csi_phy0 : 1;
		u32 rsv_b28 : 1;
		u32 csi_be : 1;
		u32 csi_mac2_apb : 1;
	} b;
	u32 raw;
};

union vip_sys_isp_clk {
	struct {
		u32 rsv : 18;
		u32 clk_isp_top_en : 1;
		u32 clk_axi_isp_en : 1;
		u32 clk_csi_mac0_en : 1;
		u32 clk_csi_mac1_en : 1;
	} b;
	u32 raw;
};

union vip_sys_clk_ctrl0 {
	struct {
		u32 bt_src_sel : 2;
		u32 lvds0_src_sel : 1;
		u32 lvds1_src_sel : 1;
		u32 disp_sel_bt_div1 : 1;
		u32 dsi_mac_sel_div : 2;
		u32 dsi_mac_src_sel : 1;
		u32 csi0_rx_src_sel : 1;
		u32 csi1_rx_src_sel : 1;
		u32 vi_clk_src_sel : 1;
		u32 vi1_clk_src_sel : 1;
	} b;
	u32 raw;
};

union vip_sys_clk {
	struct {
		u32 sc_top : 1;
		u32 isp_top : 1;
		u32 ldc_top : 1;
		u32 ive_top : 1;
		u32 vip_sys : 1;
		u32 csi_phy : 1;
		u32 dsi_phy : 1;
		u32 rev_b7 : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
		u32 rsv2 : 2;
		u32 sc_x2p_busy_en : 1;
		u32 isp_x2p_busy_en : 1;
		u32 ldc_x2p_busy_en : 1;
		u32 ive_x2p_busy_en : 1;
		u32 auto_sc_top : 1;
		u32 auto_isp_top : 1;
		u32 auto_ldc : 1;
		u32 rev_b19 : 1;
		u32 auto_vip_sys : 1;
		u32 auto_csi_phy : 1;
		u32 auto_dsi_phy : 1;
		u32 rev_b23 : 1;
		u32 auto_csi_mac0 : 1;
		u32 auto_csi_mac1 : 1;
		u32 csi_mac2 : 1;
		u32 auto_csi_mac2 : 1;
	} b;
	u32 raw;
};

union vip_sys_intr {
	struct {
		u32 sc : 1;
		u32 rsv1 : 15;
		u32 isp : 1;
		u32 rsv2 : 7;
		u32 dwa : 1;
		u32 rsv3 : 3;
		u32 rot : 1;
		u32 csi_mac0 : 1;
		u32 csi_mac1 : 1;
	} b;
	u32 raw;
};

enum vip_sys_axi_bus {
	VIP_SYS_AXI_BUS_SC_TOP = 0,
	VIP_SYS_AXI_BUS_ISP_RAW,
	VIP_SYS_AXI_BUS_ISP_YUV,
	VIP_SYS_AXI_BUS_MAX,
};

/********************************************************************
 *   APIs to replace bmtest's standard APIs
 ********************************************************************/
void vip_set_base_addr(void *base);
union vip_sys_clk vip_get_clk_lp(void);
void vip_set_clk_lp(union vip_sys_clk clk);
union vip_sys_reset vip_get_reset(void);
void vip_set_reset(union vip_sys_reset reset);
void vip_toggle_reset(union vip_sys_reset mask);
union vip_sys_intr vip_get_intr_status(void);
unsigned int vip_sys_reg_read(uintptr_t addr);
void vip_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data);
void vip_sys_set_offline(enum vip_sys_axi_bus bus, bool offline);
int vip_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb);
int vip_sys_cmm_cb_i2c(unsigned int cmd, void *arg);
int vip_sys_cmm_cb_ssp(unsigned int cmd, void *arg);

#ifdef __cplusplus
}
#endif

#endif //__COMMON_VIP_H__
