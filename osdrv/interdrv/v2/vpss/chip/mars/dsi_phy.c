#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#elif defined(ENV_EMU)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/log2.h>
#endif  // ENV_CVITEST

#include "vpss_common.h"
#include "scaler_reg.h"
#include "reg.h"
#include "dsi_phy.h"
#include "reg_vip_sys.h"
#include <vip_common.h>

enum BT_MODE {
	BT_MODE_656 = 0,
	BT_MODE_1120,
	BT_MODE_601,
	BT_MODE_MAX,
};

enum BT_CLK_MODE {
	BT_CLK_MODE_27M = 0,
	BT_CLK_MODE_36M,
	BT_CLK_MODE_37P125M,
	BT_CLK_MODE_72M,
	BT_CLK_MODE_74P25M,
	BT_CLK_MODE_148P5M,
};

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static uintptr_t reg_base;
static u8 data_0_lane;
static bool data_0_pn_swap;

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void dphy_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}
EXPORT_SYMBOL_GPL(dphy_set_base_addr);

/**
 * dphy_dsi_lane_en - set dsi-lanes enable control.
 *                    setup before dphy_dsi_init().
 *
 * @param clk_en: clk lane enable
 * @param data_en: data lane[0-3] enable
 * @param preamble_en: preeamble enable
 */
void dphy_dsi_lane_en(bool clk_en, bool *data_en, bool preamble_en)
{
	u8 val = 0, i = 0;

	val |= clk_en;
	for (i = 0; i < 4; ++i)
		val |= (data_en[i] << (i + 1));
	if (preamble_en)
		val |= 0x20;
	_reg_write_mask(reg_base + REG_DSI_PHY_EN, 0x3f, val);
}
EXPORT_SYMBOL_GPL(dphy_dsi_lane_en);

/**
 * dphy_get_dsi_lane_status - get dsi-lanes status.
 *
 * @param data_en: to store data status of lane[0-3] and clk lane
 */
void dphy_get_dsi_lane_status(bool *data_en)
{
	u32 val = 0, i = 0;

	val = _reg_read(reg_base + REG_DSI_PHY_EN);
	for (i = 0; i < DSI_LANE_MAX; ++i) {
		if (val & (0x1 << i))
			data_en[i] = true;
		else
			data_en[i] = false;
	}
}
EXPORT_SYMBOL_GPL(dphy_get_dsi_lane_status);

int dphy_dsi_disable_lanes(void)
{
	_reg_write_mask(reg_base + REG_DSI_PHY_EN, 0x3f, 0);
	_reg_write_mask(reg_base + REG_DSI_PHY_LANE_SEL, 0xfffff, 0);
	return 0;
}
EXPORT_SYMBOL_GPL(dphy_dsi_disable_lanes);

/**
 * dphy_dsi_set_lane - dsi-lanes control.
 *                     setup before dphy_dsi_lane_en().
 *
 * @param lane_num: lane[0-4].
 * @param lane: the role of this lane.
 * @param pn_swap: if this lane positive/negative swap.
 * @param clk_phase_shift: if this clk lane phase shift 90 degree.
 * @return: 0 for success.
 */
int dphy_dsi_set_lane(u8 lane_num, enum lane_id lane, bool pn_swap, bool clk_phase_shift)
{
	if ((lane_num > 4) || (lane > DSI_LANE_MAX))
		return -1;

	_reg_write_mask(reg_base + REG_DSI_PHY_LANE_SEL, 0x7 << (4 * lane_num), lane << (4 * lane_num));
	_reg_write_mask(reg_base + REG_DSI_PHY_LANE_PN_SWAP, BIT(lane_num), pn_swap << lane_num);

	if (lane == DSI_LANE_CLK)
		_reg_write_mask(reg_base + REG_DSI_PHY_LANE_SEL, 0x1f << 24,
				clk_phase_shift ? ((1 << 24) << lane_num) : 0);
	if (lane == DSI_LANE_0) {
		data_0_lane = lane_num;
		data_0_pn_swap = pn_swap;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dphy_dsi_set_lane);

/**
 * dphy_dsi_get_lane - get dsi-lanes lane num.
 *
 * @param lane_num: to store data0 lane num, -1 if disable.
 * @param lane_swap: to store data0 lane swap.
 * @return: 0 for success, -1 for failure.
 */
static int dphy_dsi_get_data0_lane(u8 *lane_num, bool *lane_swap)
{
	u32 val = 0, i = 0;

	val = _reg_read(reg_base + REG_DSI_PHY_LANE_SEL);

	for (i = 0; i < 5; ++i) {
		if ((val & 0x7) == DSI_LANE_0) {
			*lane_num = i;
			break;
		}
		val >>= 4;
	}

	if (i != 5) {
		val = _reg_read(reg_base + REG_DSI_PHY_LANE_PN_SWAP);
		*lane_swap = (val >> i) & 0x01;
		return 0;
	} else
		return -1;

}

/**
 * dphy_dsi_get_lane - get dsi-lanes lane num.
 *
 * @param lane_num: to store lane num, -1 if disable.
 * @return: num of data lane.
 */
int dphy_dsi_get_lane(enum lane_id *lane_num)
{
	bool data_en[DSI_LANE_MAX] = {false, false, false, false, false};
	u32 val = 0, i = 0, j = 0;

	dphy_get_dsi_lane_status(data_en);

	val = _reg_read(reg_base + REG_DSI_PHY_LANE_SEL);

	for (i = 0, j = 0; i < DSI_LANE_MAX; ++i) {
		if (data_en[i]) {
			lane_num[i] = (val >> i * 4) & 0x07;
			++j;
		} else {
			lane_num[i] = -1;
		}
	}
	return --j;
}
EXPORT_SYMBOL_GPL(dphy_dsi_get_lane);

/**
 * dphy_dsi_init - dphy init.
 *                 Invoked after dphy_dsi_set_lane() and dphy_dsi_lane_en().
 *
 */
void dphy_init(enum sclr_vo_intf intf)
{
	int lptrx = 0, lptx_rx = 0, hstx = 0, i;

	for (i = 0; i < DSI_LANE_MAX; ++i) {
		if (((_reg_read(reg_base + REG_DSI_PHY_LANE_SEL) >> i * 4) & 0x0F) > 4) {
			lptrx |= ((1 << i) | (1 << (8 + i)));
			lptx_rx  |= ((1 << i) | (1 << (16 + i)));
			hstx  |= ((1 << i) | (1 << (8 + i)) | (1 << (16 + i)) | (1 << (24 + i)));
		}
	}

	_reg_write(reg_base + REG_DSI_PHY_PD, (intf == SCLR_VO_INTF_MIPI || intf == SCLR_VO_INTF_LVDS)
		   ? lptrx : 0x1f1f);
	_reg_write(reg_base + REG_DSI_PHY_LPTX_OV, lptx_rx);
	_reg_write(reg_base + REG_DSI_PHY_PD_EN_TX, lptrx);
	_reg_write(reg_base + REG_DSI_PHY_PD_TXDRV, hstx);
	_reg_write(reg_base + REG_DSI_PHY_GPO, lptrx);
	_reg_write(reg_base + REG_DSI_PHY_GPI, lptrx);
	_reg_write(reg_base + REG_DSI_PHY_ESC_INIT, 0x100);
	_reg_write(reg_base + REG_DSI_PHY_ESC_WAKE, 0x100);

	if ((intf == SCLR_VO_INTF_BT656) || (intf == SCLR_VO_INTF_BT1120) || (intf == SCLR_VO_INTF_I80))
		_reg_write(reg_base + REG_DSI_PHY_EXT_GPIO, 0x000fffff);
	else
		_reg_write(reg_base + REG_DSI_PHY_EXT_GPIO, 0x0);

	_reg_write(reg_base + REG_DSI_PHY_LVDS_EN, (intf == SCLR_VO_INTF_LVDS));
	// if lvds: 1. en txbias. 2. en sublvds. 3. set vsel to maximum
	_reg_write_mask(reg_base + REG_DSI_PHY_EN_TXBIAS_OP, 0x10000,
				(intf == SCLR_VO_INTF_LVDS) ? 0x10000 : 0);
	_reg_write_mask(reg_base + REG_DSI_PHY_EN_SUBLVDS, 0x1F000000,
				(intf == SCLR_VO_INTF_LVDS) ? 0x1F000000 : 0);
	_reg_write_mask(reg_base + REG_DSI_PHY_VSEL, 0xFFFFF,
				(intf == SCLR_VO_INTF_LVDS) ? 0xFFFFF : 0);
}

void _cal_pll_reg(u32 clkkHz, u32 VCORx10000, u32 *reg_txpll, u32 *reg_set, u32 factor)
{
	u8 gain = 1 << ilog2(MAX(1, 25000000UL / VCORx10000));
	u32 VCOCx1000 = VCORx10000 * gain / 10;
	u8 reg_disp_div_sel = VCOCx1000 / clkkHz;
	u8 dig_dig = ilog2(gain);
	u8 reg_divout_sel = MIN(3, dig_dig);
	u8 reg_div_sel = dig_dig - reg_divout_sel;
	u32 loop_gainx1000 = VCOCx1000 / 133;
	bool bt_div = reg_disp_div_sel > 0x7f;
	u32 loop_c = 8 * ((loop_gainx1000 / 8) / 1000);
	u8 div_loop = loop_c > 32 ? 3 : loop_c / 8;
	u8 loop_gain1 = div_loop * 8;
	u64 modifies = (u64)(factor * loop_gain1) << 26;

	do_div(modifies, VCOCx1000);
	*reg_set = (u32)modifies;

	if (bt_div) {
		vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL0, 0x10, 0);
		reg_disp_div_sel >>= 1;
	} else
		vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL0, 0x10, 0x10);

	_reg_write_mask(reg_base + REG_DSI_PHY_TXPLL, 0x300000, div_loop << 20);

	*reg_txpll = (reg_div_sel << 10) | (reg_divout_sel << 8) | reg_disp_div_sel;

	pr_debug("clkkHz(%d) VCORx10000(%d) gain(%d)\n", clkkHz, VCORx10000, gain);
	pr_debug("VCOCx1000(%d) dig_dig(%d) loop_gain(%d)\n", VCOCx1000, dig_dig, loop_gainx1000);
	pr_debug("loop_c(%d) div_loop(%d) loop_gain1(%d)\n", loop_c, div_loop, loop_gain1);
	pr_debug("regs: disp_div_sel(%d), divout_sel(%d), div_sel(%d), set(%#x)\n",
			reg_disp_div_sel, reg_divout_sel, reg_div_sel, *reg_set);
	pr_debug("vip_sy : bt_div(%d)\n", bt_div);
}

void dphy_lvds_set_pll(u32 clkkHz, u8 link)
{
	u32 VCORx10000 = clkkHz * 70 / link;
	u32 reg_txpll, reg_set;

	_cal_pll_reg(clkkHz, VCORx10000, &reg_txpll, &reg_set, 900000);

	_reg_write_mask(reg_base + REG_DSI_PHY_TXPLL, 0x7ff, reg_txpll);
	_reg_write(reg_base + REG_DSI_PHY_REG_SET, reg_set);
	// update
	_reg_write_mask(reg_base + REG_DSI_PHY_REG_8C, BIT(0), 0);
	_reg_write_mask(reg_base + REG_DSI_PHY_REG_8C, BIT(0), 1);
}
EXPORT_SYMBOL_GPL(dphy_lvds_set_pll);

void dphy_dsi_get_pixclk(u32 *clkkHz, u8 lane, u8 bits)
{
	u32 VCOCx1000, VCORx10000;
	u32 reg_txpll, reg_set;
	u8 reg_disp_div_sel;
	bool bt_div;
	u32 factor = 900000;
	u8 gain, loop_gain, loop_gain_tmp;
	u64 modifies = 0;

	reg_set = _reg_read(reg_base + REG_DSI_PHY_REG_SET);
	bt_div = !(vip_sys_reg_read(VIP_SYS_VIP_CLK_CTRL0) & 0x10);
	reg_txpll = _reg_read(reg_base + REG_DSI_PHY_TXPLL) & 0x7ff;

	reg_disp_div_sel = reg_txpll & 0x7f;
	reg_disp_div_sel = bt_div ? reg_disp_div_sel << 1 : reg_disp_div_sel;
	gain = reg_disp_div_sel * lane / bits;

	for (loop_gain = 8; loop_gain < 255; loop_gain += 8) {
		modifies = ((u64)factor << 26) * loop_gain;
		do_div(modifies, reg_set);
		VCOCx1000 = (u32)modifies;
		loop_gain_tmp = (((VCOCx1000 / 266000) + 7) >> 3) << 3;
		if (loop_gain_tmp == loop_gain)
			break;
	}

	VCORx10000 = VCOCx1000 * 10 / gain;
	*clkkHz = VCORx10000 * lane / 10 / bits;
}
EXPORT_SYMBOL_GPL(dphy_dsi_get_pixclk);

void dphy_dsi_set_pll(u32 clkkHz, u8 lane, u8 bits)
{
	u32 VCORx10000 = clkkHz * bits * 10 / lane;
	u32 reg_txpll, reg_set;

	_cal_pll_reg(clkkHz, VCORx10000, &reg_txpll, &reg_set, 900000);

	_reg_write_mask(reg_base + REG_DSI_PHY_TXPLL, 0x7ff, reg_txpll);
	_reg_write(reg_base + REG_DSI_PHY_REG_SET, reg_set);

	// update
	_reg_write_mask(reg_base + REG_DSI_PHY_REG_8C, BIT(0), 0);
	_reg_write_mask(reg_base + REG_DSI_PHY_REG_8C, BIT(0), 1);
}
EXPORT_SYMBOL_GPL(dphy_dsi_set_pll);

void vip_sys_clk_setting(u32 value)
{
	vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL0, 0xFFFFFFFF, value);
}
EXPORT_SYMBOL_GPL(vip_sys_clk_setting);

void dphy_dsi_analog_setting(bool is_lvds)
{
	//mercury needs this analog setting while lvds tx mode
	if (is_lvds)
		_reg_write_mask(reg_base + REG_DSI_PHY_REG_74, 0x3ff, 0x2AA);
	else
		_reg_write_mask(reg_base + REG_DSI_PHY_REG_74, 0x3ff, 0x0);
}
EXPORT_SYMBOL_GPL(dphy_dsi_analog_setting);

#define dcs_delay 1

enum LP_DATA {
	LP_DATA_00 = 0x00010001,
	LP_DATA_01 = 0x00010101,
	LP_DATA_10 = 0x01010001,
	LP_DATA_11 = 0x01010101,
	LP_DATA_MAX
};

static inline void _data_0_manual_data(enum LP_DATA data)
{
	dphy_dsi_get_data0_lane(&data_0_lane, &data_0_pn_swap);
	if (data_0_pn_swap) {
		switch (data) {
		case LP_DATA_01:
			_reg_write(reg_base + REG_DSI_PHY_DATA_OV, LP_DATA_10 << data_0_lane);
			break;
		case LP_DATA_10:
			_reg_write(reg_base + REG_DSI_PHY_DATA_OV, LP_DATA_01 << data_0_lane);
			break;
		default:
			_reg_write(reg_base + REG_DSI_PHY_DATA_OV, data << data_0_lane);
			break;
		}
	} else {
		_reg_write(reg_base + REG_DSI_PHY_DATA_OV, data << data_0_lane);
	}
	udelay(dcs_delay);
}

// LP-11, LP-10, LP-00, LP-01, LP-00
static void _esc_entry(void)
{
	_data_0_manual_data(LP_DATA_11);
	_data_0_manual_data(LP_DATA_10);
	_data_0_manual_data(LP_DATA_00);
	_data_0_manual_data(LP_DATA_01);
	_data_0_manual_data(LP_DATA_00);
}

// LP-00, LP-10, LP-11
static void _esc_exit(void)
{
	_data_0_manual_data(LP_DATA_00);
	_data_0_manual_data(LP_DATA_10);
	_data_0_manual_data(LP_DATA_11);
}

static void _esc_data(u8 data)
{
	u8 i = 0;

	for (i = 0; i < 8; ++i) {
		_data_0_manual_data(((data & (1 << i)) ? LP_DATA_10 : LP_DATA_01));
		_data_0_manual_data(LP_DATA_00);
	}
}

void dpyh_mipi_tx_manual_packet(const u8 *data, u8 count)
{
	u8 i = 0;

	_esc_entry();
	_esc_data(0x87); // LPDT
	for (i = 0; i < count; ++i)
		_esc_data(data[i]);
	_esc_exit();
	_reg_write(reg_base + REG_DSI_PHY_DATA_OV, 0x0);
}

void dphy_set_hs_settle(u8 prepare, u8 zero, u8 trail)
{
	_reg_write_mask(reg_base + REG_DSI_PHY_HS_CFG1, 0xffffff00, (trail << 24) | (zero << 16) | (prepare << 8));
}
EXPORT_SYMBOL_GPL(dphy_set_hs_settle);

void dphy_get_hs_settle(u8 *prepare, u8 *zero, u8 *trail)
{
	u32 value = _reg_read(reg_base + REG_DSI_PHY_HS_CFG1);

	if (prepare)
		*prepare = (value >> 8) & 0xff;
	if (zero)
		*zero = (value >> 16) & 0xff;
	if (trail)
		*trail = (value >> 24) & 0xff;
}
EXPORT_SYMBOL_GPL(dphy_get_hs_settle);
