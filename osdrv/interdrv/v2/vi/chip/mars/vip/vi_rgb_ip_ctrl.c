#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

#if defined( __SOC_PHOBOS__)
#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x100
#define LTM_GLOBAL_LUT_SIZE      0x100
#else
#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x200
#define LTM_GLOBAL_LUT_SIZE      0x300
#endif

/*******************************************************************************
 *	RGB IPs config
 ******************************************************************************/

void ispblk_ccm_config(struct isp_ctx *ctx, enum ISP_BLK_ID_T blk_id, bool en, struct isp_ccm_cfg *cfg)
{
	uintptr_t ccm = ctx->phys_regs[blk_id];

	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_SHDW_SEL, 1);
	ISP_WR_BITS(ccm, REG_ISP_CCM_T, CCM_CTRL, CCM_ENABLE, en);

	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, REG_ISP_CCM_T, CCM_22, cfg->coef[2][2]);
}

void ispblk_fusion_hdr_cfg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];

	if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(fusion, REG_FUSION_T, FS_CTRL_0, FS_ENABLE, false);
		ISP_WR_BITS(fusion, REG_FUSION_T, FS_SE_GAIN, FS_OUT_SEL, ISP_FS_OUT_LONG);
	} else
		ISP_WR_BITS(fusion, REG_FUSION_T, FS_CTRL_0, FS_ENABLE, true);
}

void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, bool mc_enable, enum ISP_FS_OUT out_sel)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];
	union REG_FUSION_FS_CTRL_0 reg_ctrl;
	union REG_FUSION_FS_SE_GAIN reg_se_gain;

	reg_ctrl.raw = ISP_RD_REG(fusion, REG_FUSION_T, FS_CTRL_0);
	reg_ctrl.bits.FS_ENABLE = enable;
	reg_ctrl.bits.FS_MC_ENABLE = mc_enable;
	reg_ctrl.bits.FS_S_MAX = 65535;
	reg_ctrl.bits.SE_IN_SEL = 0;
	ISP_WR_REG(fusion, REG_FUSION_T, FS_CTRL_0, reg_ctrl.raw);

	reg_se_gain.raw = ISP_RD_REG(fusion, REG_FUSION_T, FS_SE_GAIN);
	reg_se_gain.bits.FS_LS_GAIN = 64;
	reg_se_gain.bits.FS_OUT_SEL = out_sel;
	ISP_WR_REG(fusion, REG_FUSION_T, FS_SE_GAIN, reg_se_gain.raw);

	ISP_WR_BITS(fusion, REG_FUSION_T, FS_LUMA_THD, FS_LUMA_THD_L, 2048);
	ISP_WR_BITS(fusion, REG_FUSION_T, FS_LUMA_THD, FS_LUMA_THD_H, 2048);
	ISP_WR_BITS(fusion, REG_FUSION_T, FS_WGT, FS_WGT_MAX, 128);
	ISP_WR_BITS(fusion, REG_FUSION_T, FS_WGT, FS_WGT_MIN, 128);
	ISP_WR_REG(fusion, REG_FUSION_T, FS_WGT_SLOPE, 0);
}

void ispblk_hist_v_config(struct isp_ctx *ctx, bool en, uint8_t test_case)
{
	uintptr_t hist_v = ctx->phys_regs[ISP_BLK_ID_HIST_V];

	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, en);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, en);

	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETX, HIST_EDGE_V_OFFSETX, 0);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETY, HIST_EDGE_V_OFFSETY, 0);

	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 1);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, DMI_ENABLE, FORCE_DMA_DISABLE, 0);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, BYPASS, FORCE_CLK_ENABLE, 1);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, BYPASS, BYPASS, 0);
	ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, SW_CTL, TILE_NM, 0);

	// when test_case == 0
	//   en == 1, case_luma
	//   en == 0, case_disable
	if (test_case == 1) {
		/* case_0 and case_all_ff */
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, 1);
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, 0);
	} else if (test_case == 2) {
		/* case_offx64_offy32 */
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_ENABLE, 1);
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, IP_CONFIG, HIST_EDGE_V_LUMA_MODE, 0);
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETX, HIST_EDGE_V_OFFSETX, 64);
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, HIST_EDGE_V_OFFSETY, HIST_EDGE_V_OFFSETY, 32);
	}
}

void ispblk_ltm_d_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	union REG_LTM_H34 reg_34;
	union REG_LTM_H3C reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, REG_LTM_T, REG_H44, data[LTM_DARK_TONE_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H34);
	reg_34.bits.LUT_DBG_RSEL = sel;
	reg_34.bits.LUT_PROG_EN_DARK = 1;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H3C);
	reg_3c.bits.LUT_WSEL = sel;
	reg_3c.bits.LUT_WSTADDR = 0;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H3C, reg_3c.raw);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR, 0);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);

	for (i = 0; i < LTM_DARK_TONE_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, REG_LTM_T, REG_H38, val);
		ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);
	}

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H34, LUT_PROG_EN_DARK, 0);
}

void ispblk_ltm_b_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	union REG_LTM_H34 reg_34;
	union REG_LTM_H3C reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, REG_LTM_T, REG_H40, data[LTM_BRIGHT_TONE_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H34);
	reg_34.bits.LUT_DBG_RSEL = sel;
	reg_34.bits.LUT_PROG_EN_BRIGHT = 1;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H3C);
	reg_3c.bits.LUT_WSEL = sel;
	reg_3c.bits.LUT_WSTADDR = 0;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H3C, reg_3c.raw);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR, 0);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);

	for (i = 0; i < LTM_BRIGHT_TONE_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, REG_LTM_T, REG_H38, val);
		ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);
	}

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H34, LUT_PROG_EN_BRIGHT, 0);
}

void ispblk_ltm_g_lut(struct isp_ctx *ctx, uint8_t sel, uint16_t *data)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	union REG_LTM_H34 reg_34;
	union REG_LTM_H3C reg_3c;
	u16 i = 0;
	u32 val = 0;

	ISP_WR_REG(ltm, REG_LTM_T, REG_H48, data[LTM_GLOBAL_LUT_SIZE]);

	reg_34.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H34);
	reg_34.bits.LUT_DBG_RSEL = sel;
	reg_34.bits.LUT_PROG_EN_GLOBAL = 1;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H34, reg_34.raw);

	reg_3c.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H3C);
	reg_3c.bits.LUT_WSEL = sel;
	reg_3c.bits.LUT_WSTADDR = 0;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H3C, reg_3c.raw);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR, 0);
	ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WSTADDR_TRIG_1T, 1);

	for (i = 0; i < LTM_GLOBAL_LUT_SIZE; i += 2) {
		val = (data[i] | (data[i + 1] << 16));
		ISP_WR_REG(ltm, REG_LTM_T, REG_H38, val);
		ISP_WR_BITS(ltm, REG_LTM_T, REG_H3C, LUT_WDATA_TRIG_1T, 1);
	}

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H34, LUT_PROG_EN_GLOBAL, 0);
}

void ispblk_ltm_config(struct isp_ctx *ctx, u8 ltm_en, u8 dehn_en, u8 behn_en, u8 ee_en)
{
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
	union REG_LTM_H00 reg_00;
	union REG_LTM_H8C reg_8c;

	reg_00.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H00);
	reg_00.bits.LTM_ENABLE          = ltm_en;
	reg_00.bits.LTM_DARK_ENH_ENABLE = dehn_en;
	reg_00.bits.LTM_BRIT_ENH_ENABLE = behn_en;
	reg_00.bits.FORCE_DMA_DISABLE   = ((!dehn_en) | (!behn_en << 1));
	ISP_WR_REG(ltm, REG_LTM_T, REG_H00, reg_00.raw);

	ISP_WR_BITS(ltm, REG_LTM_T, REG_H64, LTM_EE_ENABLE, ee_en);

	reg_8c.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H8C);
	reg_8c.bits.LMAP_W_BIT = g_lmp_cfg[ISP_PRERAW_A].post_w_bit;
	reg_8c.bits.LMAP_H_BIT = g_lmp_cfg[ISP_PRERAW_A].post_h_bit;
	ISP_WR_REG(ltm, REG_LTM_T, REG_H8C, reg_8c.raw);
}

void ispblk_ygamma_config(struct isp_ctx *ctx, bool en,
				uint8_t sel, uint16_t *data, uint8_t inv, uint8_t test_case)
{
	uintptr_t ygamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];
	int16_t i;
	bool lut_check_pass;
	union REG_YGAMMA_GAMMA_PROG_DATA reg_data;
	union REG_YGAMMA_GAMMA_PROG_CTRL prog_ctrl;
	union REG_YGAMMA_GAMMA_MEM_SW_RADDR sw_raddr;
	union REG_YGAMMA_GAMMA_MEM_SW_RDATA sw_rdata;

	prog_ctrl.raw = ISP_RD_REG(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL);
	prog_ctrl.bits.GAMMA_WSEL    = sel;
	prog_ctrl.bits.GAMMA_PROG_EN = 1;
	//prog_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
	ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);

	if (inv) {
		for (i = 255; i >= 0; i -= 2) {
			reg_data.raw = 0;
			reg_data.bits.GAMMA_DATA_E = data[i];
			reg_data.bits.GAMMA_DATA_O = data[i + 1];
			ISP_WR_REG(ygamma, REG_YGAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
			ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_W, 1);
		}

		// set max to 0
		ISP_WR_REG(ygamma, REG_YGAMMA_T, GAMMA_PROG_MAX, 0);
	} else {
		for (i = 0; i < 256; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.GAMMA_DATA_E = data[i];
			reg_data.bits.GAMMA_DATA_O = data[i + 1];
			ISP_WR_REG(ygamma, REG_YGAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
			ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_W, 1);
		}
	}

	ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, sel);
	ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);

	// sw read mem0/mem1, check value
	if (test_case == 1) {
		lut_check_pass = true;
		ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 1);
		sw_raddr.raw = ISP_RD_REG(ygamma, REG_YGAMMA_T, GAMMA_SW_RADDR);
		for (i = 0; i < 256; i++) {
			sw_raddr.bits.GAMMA_SW_R_MEM_SEL = sel;
			sw_raddr.bits.GAMMA_SW_RADDR = i;
			ISP_WR_REG(ygamma, REG_YGAMMA_T, GAMMA_SW_RADDR, sw_raddr.raw);
			ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_SW_RDATA, GAMMA_SW_R, 1);

			sw_rdata.raw = ISP_RD_REG(ygamma, REG_YGAMMA_T, GAMMA_SW_RDATA);
			if ((sw_rdata.raw & 0xFFFF) != data[i]) {
				lut_check_pass = false;
				vi_pr(VI_DBG, "Ygamma LUT Check failed, lut[%d] = %d, should be %d\n",
					i, (sw_rdata.raw & 0xFFFF), data[i]);
				break;
			}
		}
		ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);

		if (lut_check_pass)
			vi_pr(VI_WARN, "Ygamma LUT Check passed\n");
	}
}

void ispblk_ygamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t ygamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];

	ISP_WR_BITS(ygamma, REG_YGAMMA_T, GAMMA_CTRL, YGAMMA_ENABLE, enable);
}

void ispblk_gamma_config(struct isp_ctx *ctx, bool en, uint8_t sel, uint16_t *data, uint8_t inv)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;
	union REG_ISP_GAMMA_PROG_DATA reg_data;
	union REG_ISP_GAMMA_PROG_CTRL prog_ctrl;

	prog_ctrl.raw = ISP_RD_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL);
	prog_ctrl.bits.GAMMA_WSEL    = sel;
	prog_ctrl.bits.GAMMA_PROG_EN = 1;
	prog_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, prog_ctrl.raw);

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_ADDR, 0);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_ST_ADDR, GAMMA_ST_W, 1);

	if (inv) {
		for (i = 255; i >= 0; i -= 2) {
			reg_data.raw = 0;
			reg_data.bits.GAMMA_DATA_E = data[i];
			reg_data.bits.GAMMA_DATA_O = data[i + 1];
			reg_data.bits.GAMMA_W = 1;
			ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
		}

		// set max to 0
		ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_MAX, 0);
	} else {
		for (i = 0; i < 256; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.GAMMA_DATA_E = data[i];
			reg_data.bits.GAMMA_DATA_O = data[i + 1];
			reg_data.bits.GAMMA_W = 1;
			ISP_WR_REG(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_DATA, reg_data.raw);
		}
	}

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_RSEL, sel);
	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];

	ISP_WR_BITS(gamma, REG_ISP_GAMMA_T, GAMMA_CTRL, GAMMA_ENABLE, enable);
}

void ispblk_dhz_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DHZ];

	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_BYPASS, DEHAZE_ENABLE, en);
	ISP_WR_BITS(dhz, REG_ISP_DEHAZE_T, DHZ_BYPASS, DEHAZE_SKIN_LUT_ENABLE, 1);
}

/**
 * ispblk_rgbdither_config - setup rgb dither.
 *
 * @param ctx: global settings
 * @param en: rgb dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en,
			    bool histidx_en, bool fmnum_en)
{
	uintptr_t rgbdither = ctx->phys_regs[ISP_BLK_ID_RGBDITHER];
	union REG_ISP_RGB_DITHER_RGB_DITHER reg;

	reg.raw = 0;
	reg.bits.RGB_DITHER_ENABLE = en;
	reg.bits.RGB_DITHER_MOD_EN = mod_en;
	reg.bits.RGB_DITHER_HISTIDX_EN = histidx_en;
	reg.bits.RGB_DITHER_FMNUM_EN = fmnum_en;
	reg.bits.RGB_DITHER_SHDW_SEL = 1;
	reg.bits.CROP_WIDTHM1 = ctx->img_width - 1;
	reg.bits.CROP_HEIGHTM1 = ctx->img_height - 1;

	ISP_WR_REG(rgbdither, REG_ISP_RGB_DITHER_T, RGB_DITHER, reg.raw);
}

void ispblk_clut_config(struct isp_ctx *ctx, bool en,
				int16_t *r_lut, int16_t *g_lut, int16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	uint16_t r_idx, g_idx, b_idx;
	union REG_ISP_CLUT_CTRL      ctrl;
	union REG_ISP_CLUT_PROG_ADDR prog_addr;
	union REG_ISP_CLUT_PROG_DATA prog_data;
	u32 idx = 0;

	ctrl.raw = ISP_RD_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL);
	ctrl.bits.PROG_EN = 1;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				prog_addr.raw = 0;
				prog_addr.bits.SRAM_R_IDX = r_idx;
				prog_addr.bits.SRAM_G_IDX = g_idx;
				prog_addr.bits.SRAM_B_IDX = b_idx;
				ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_ADDR, prog_addr.raw);

				prog_data.raw		  = 0;
				prog_data.bits.SRAM_WDATA = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				prog_data.bits.SRAM_WR	  = 1;
				ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_PROG_DATA, prog_data.raw);
			}
		}
	}

	ctrl.bits.CLUT_ENABLE = en;
	ctrl.bits.PROG_EN = 0;
	ISP_WR_REG(clut, REG_ISP_CLUT_T, CLUT_CTRL, ctrl.raw);
}

void ispblk_csc_config(struct isp_ctx *ctx)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];

	ISP_WR_BITS(csc, REG_ISP_CSC_T, REG_0, CSC_ENABLE, 1);
}

static void _manr_init(struct isp_ctx *ctx)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union REG_ISP_MMAP_04 reg_04;
	union REG_ISP_MMAP_08 reg_08;
	union REG_ISP_MMAP_38 reg_38;

	uint16_t data[] = {
		264,  436,  264,   60,	262,  436,  266,   60,	260,  435,  268,   61,	258,  435,  270,   61,
		255,  434,  272,   63,	253,  434,  274,   63,	251,  433,  275,   65,	249,  433,  277,   65,
		246,  432,  279,   67,	244,  432,  281,   67,	242,  431,  283,   68,	240,  431,  285,   68,
		237,  430,  286,   71,	235,  429,  288,   72,	233,  429,  290,   72,	231,  428,  292,   73,
		229,  427,  294,   74,	227,  427,  296,   74,	224,  426,  297,   77,	222,  425,  299,   78,
		220,  424,  301,   79,	218,  424,  303,   79,	216,  423,  305,   80,	214,  422,  306,   82,
		212,  421,  308,   83,	210,  420,  310,   84,	208,  419,  312,   85,	206,  419,  313,   86,
		204,  418,  315,   87,	202,  417,  317,   88,	199,  416,  319,   90,	197,  415,  321,   91,
		195,  414,  322,   93,	194,  413,  324,   93,	192,  412,  326,   94,	190,  411,  328,   95,
		188,  410,  329,   97,	186,  409,  331,   98,	184,  408,  333,   99,	182,  407,  334,  101,
		180,  405,  336,  103,	178,  404,  338,  104,	176,  403,  340,  105,	174,  402,  341,  107,
		172,  401,  343,  108,	171,  400,  345,  108,	169,  398,  346,  111,	167,  397,  348,  112,
		165,  396,  349,  114,	163,  395,  351,  115,	161,  393,  353,  117,	160,  392,  354,  118,
		158,  391,  356,  119,	156,  390,  358,  120,	154,  388,  359,  123,	153,  387,  361,  123,
		151,  386,  362,  125,	149,  384,  364,  127,	148,  383,  365,  128,	146,  381,  367,  130,
		144,  380,  368,  132,	143,  379,  370,  132,	141,  377,  371,  135,	139,  376,  373,  136,
		138,  374,  374,  138,
	};

	uint8_t i = 0;

	reg_04.bits.MMAP_0_LPF_00 = 3;
	reg_04.bits.MMAP_0_LPF_01 = 4;
	reg_04.bits.MMAP_0_LPF_02 = 3;
	reg_04.bits.MMAP_0_LPF_10 = 4;
	reg_04.bits.MMAP_0_LPF_11 = 4;
	reg_04.bits.MMAP_0_LPF_12 = 4;
	reg_04.bits.MMAP_0_LPF_20 = 3;
	reg_04.bits.MMAP_0_LPF_21 = 4;
	reg_04.bits.MMAP_0_LPF_22 = 3;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_04, reg_04.raw);

	reg_08.bits.MMAP_0_MAP_CORING = 0;
	reg_08.bits.MMAP_0_MAP_GAIN   = 8;
	reg_08.bits.MMAP_0_MAP_THD_L  = 64; /* for imx327 tuning */
	reg_08.bits.MMAP_0_MAP_THD_H  = 255;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_08, reg_08.raw);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_0C,
		    MMAP_0_LUMA_ADAPT_LUT_IN_0, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_0C,
		    MMAP_0_LUMA_ADAPT_LUT_IN_1, 600);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_10,
		    MMAP_0_LUMA_ADAPT_LUT_IN_2, 1500);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_10,
		    MMAP_0_LUMA_ADAPT_LUT_IN_3, 2500);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_0, 63);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_1, 48);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_2, 8);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_14,
		    MMAP_0_LUMA_ADAPT_LUT_OUT_3, 2);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_18,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_0, -27);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_18,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_1, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_1C,
		    MMAP_0_LUMA_ADAPT_LUT_SLOPE_2, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_1C, MMAP_0_MAP_DSHIFT_BIT, 5);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_0, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_1, 45);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_2, 90);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_20, MMAP_0_IIR_PRTCT_LUT_IN_3, 255);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_0, 6);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_1, 10);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_2, 9);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_24, MMAP_0_IIR_PRTCT_LUT_OUT_3, 2);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_28,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_0, 12);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_28,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_1, -4);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_2C,
		    MMAP_0_IIR_PRTCT_LUT_SLOPE_2, -4);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_70, MMAP_0_GAIN_RATIO_R, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_70, MMAP_0_GAIN_RATIO_G, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_74, MMAP_0_GAIN_RATIO_B, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_78, MMAP_0_NS_SLOPE_R, 5);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_78, MMAP_0_NS_SLOPE_G, 4);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_7C, MMAP_0_NS_SLOPE_B, 6);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_80, MMAP_0_NS_LUMA_TH0_R, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_80, MMAP_0_NS_LUMA_TH0_G, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_84, MMAP_0_NS_LUMA_TH0_B, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_84, MMAP_0_NS_LOW_OFFSET_R, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_88, MMAP_0_NS_LOW_OFFSET_G, 2);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_88, MMAP_0_NS_LOW_OFFSET_B, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_8C, MMAP_0_NS_HIGH_OFFSET_R, 724);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_8C, MMAP_0_NS_HIGH_OFFSET_G, 724);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_90, MMAP_0_NS_HIGH_OFFSET_B, 724);

	reg_38.bits.MMAP_1_MAP_CORING = 0;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_38, reg_38.raw);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_A0, MMAP_1_GAIN_RATIO_R, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_A0, MMAP_1_GAIN_RATIO_G, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_A4, MMAP_1_GAIN_RATIO_B, 4096);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_A8, MMAP_1_NS_SLOPE_R, 5 * 4);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_A8, MMAP_1_NS_SLOPE_G, 4 * 4);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_AC, MMAP_1_NS_SLOPE_B, 6 * 4);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B0, MMAP_1_NS_LUMA_TH0_R, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B0, MMAP_1_NS_LUMA_TH0_G, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B4, MMAP_1_NS_LUMA_TH0_B, 16);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B4, MMAP_1_NS_LOW_OFFSET_R, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B8, MMAP_1_NS_LOW_OFFSET_G, 2);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_B8, MMAP_1_NS_LOW_OFFSET_B, 0);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_BC, MMAP_1_NS_HIGH_OFFSET_R, 724);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_BC, MMAP_1_NS_HIGH_OFFSET_G, 724);
	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_C0, MMAP_1_NS_HIGH_OFFSET_B, 724);

	for (i = 0; i < ARRAY_SIZE(data) / 4; ++i) {
		uint64_t val = 0;

		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 0);
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, SRAM_WADD, i);

		val = ((uint64_t)data[i * 4] | (uint64_t)data[i * 4 + 1] << 13 |
			(uint64_t)data[i * 4 + 2] << 26 | (uint64_t)data[i * 4 + 3] << 39);
		ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_64, val & 0xffffffff);
		ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_68, val >> 32);

		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 1);
	}

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, SRAM_WEN, 0);
}

void ispblk_tnr_post_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	int w = ctx->isp_pipe_cfg[raw_num].crop.w;
	int h = ctx->isp_pipe_cfg[raw_num].crop.h;
	int grid_size = (1 << ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit);

	union REG_ISP_MMAP_60 reg_60;
	union REG_ISP_MMAP_30 reg_30;
	union REG_ISP_MMAP_D0 reg_d0;
	union REG_ISP_MMAP_D4 reg_d4;
	union REG_ISP_MMAP_D8 reg_d8;

	reg_60.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_60);
	reg_60.bits.RGBMAP_W_BIT = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	reg_60.bits.RGBMAP_H_BIT = ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_60, reg_60.raw);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_04, WH_SW_MODE, 1);

	reg_30.raw = 0;
	reg_30.bits.IMG_WIDTHM1_SW	= ((((w + grid_size - 1) / grid_size) * 6 + 47) / 48 * 8 * grid_size - 1);
	reg_30.bits.IMG_HEIGHTM1_SW	= h - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_30, reg_30.raw);

	reg_d0.raw = 0;
	reg_d0.bits.CROP_ENABLE_SCALAR		= 1;
	reg_d0.bits.IMG_WIDTH_CROP_SCALAR	= reg_30.bits.IMG_WIDTHM1_SW;
	reg_d0.bits.IMG_HEIGHT_CROP_SCALAR	= reg_30.bits.IMG_HEIGHTM1_SW;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D0, reg_d0.raw);

	reg_d4.raw = 0;
	reg_d4.bits.CROP_W_STR_SCALAR		= 0;
	reg_d4.bits.CROP_W_END_SCALAR		= w - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D4, reg_d4.raw);

	reg_d8.raw = 0;
	reg_d8.bits.CROP_H_STR_SCALAR		= 0;
	reg_d8.bits.CROP_H_END_SCALAR		= h - 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_D8, reg_d8.raw);

	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL32);
	ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL34);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL33);
		ispblk_mmap_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL35);
	}
}

void ispblk_manr_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	uint8_t dma_enable;
	union REG_ISP_MMAP_00 reg_00;
	u8 raw_num = ISP_PRERAW_A;

	if (!en) {
		reg_00.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_00);
		reg_00.bits.MMAP_0_ENABLE = 0;
		reg_00.bits.MMAP_1_ENABLE = 0;
		reg_00.bits.BYPASS = 1;
		ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_00, reg_00.raw);

		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, 0xff);
		return;
	}

	//Init once for tuning
	_manr_init(ctx);

	if (_is_all_online(ctx)) //all online mode
		dma_enable = 0xae;
	else { // fe->be->dram->post or fe->dram->be->post
		dma_enable = (ctx->is_hdr_on) ? 0xa0 : 0x0a;
	}

	reg_00.raw = ISP_RD_REG(manr, REG_ISP_MMAP_T, REG_00);
	reg_00.bits.MMAP_0_ENABLE = 1;
	reg_00.bits.MMAP_1_ENABLE = (ctx->is_hdr_on) ? 1 : 0;
	reg_00.bits.BYPASS = 0;
	reg_00.bits.REG_2_TAP_EN = 1;
	ISP_WR_REG(manr, REG_ISP_MMAP_T, REG_00, reg_00.raw);

	ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = g_w_bit[raw_num];
	ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = g_h_bit[raw_num];

	ispblk_tnr_post_chg(ctx, raw_num);

	ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, dma_enable);
}

void ispblk_mmap_dma_mode(struct isp_ctx *ctx, uint32_t dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union REG_ISP_DMA_CTL_SYS_CONTROL sys_ctrl;

	//1: SW mode: config by SW 0: HW mode: auto config by HW
	sys_ctrl.raw = ISP_RD_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL);
	sys_ctrl.bits.BASE_SEL		= 0x1;
	sys_ctrl.bits.STRIDE_SEL	= 0x1;
	sys_ctrl.bits.SEGLEN_SEL	= 0x0;
	sys_ctrl.bits.SEGNUM_SEL	= 0x0;
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, sys_ctrl.raw);
}
