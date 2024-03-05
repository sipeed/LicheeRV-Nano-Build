#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

/*******************************************************************************
 *	RAW IPs config
 ******************************************************************************/
static void _bnr_init(struct isp_ctx *ctx)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	uint8_t intensity_sel[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t weight_lut[256] = {
		31, 16, 8,  4,	2,  1,	1,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
		0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,	0,  0,
	};
	uint16_t i = 0;

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, INDEX_CLR, BNR_INDEX_CLR, 1);
	for (i = 0; i < ARRAY_SIZE(intensity_sel); ++i)
		ISP_WR_REG(bnr, REG_ISP_BNR_T, INTENSITY_SEL, intensity_sel[i]);
	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i)
		ISP_WR_REG(bnr, REG_ISP_BNR_T, WEIGHT_LUT, weight_lut[i]);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, SHADOW_RD_SEL,
		    SHADOW_RD_SEL, 1);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL,
		    BNR_OUT_SEL, ISP_BNR_OUT_BYPASS);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, STRENGTH_MODE,
		    BNR_STRENGTH_MODE, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_0,
		    BNR_WEIGHT_INTRA_0, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_1,
		    BNR_WEIGHT_INTRA_1, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_INTRA_2,
		    BNR_WEIGHT_INTRA_2, 6);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_NORM_1,
		    BNR_WEIGHT_NORM_1, 7);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_NORM_2,
		    BNR_WEIGHT_NORM_2, 5);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NEIGHBOR_MAX,
		    BNR_FLAG_NEIGHBOR_MAX, 1);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, RES_K_SMOOTH,
		    BNR_RES_RATIO_K_SMOOTH, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, RES_K_TEXTURE,
		    BNR_RES_RATIO_K_TEXTURE, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, VAR_TH, BNR_VAR_TH, 128);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_SM, BNR_WEIGHT_SMOOTH, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_V, BNR_WEIGHT_V, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_H, BNR_WEIGHT_H, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_D45, BNR_WEIGHT_D45, 0);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, WEIGHT_D135, BNR_WEIGHT_D135, 0);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_B,
		    BNR_NS_SLOPE_B, 135);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_GB,
		    BNR_NS_SLOPE_GB, 106);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_GR,
		    BNR_NS_SLOPE_GR, 106);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_SLOPE_R,
		    BNR_NS_SLOPE_R, 127);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_B,
		    BNR_NS_LOW_OFFSET_B, 177);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_GB,
		    BNR_NS_LOW_OFFSET_GB, 169);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_GR,
		    BNR_NS_LOW_OFFSET_GR, 169);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET0_R,
		    BNR_NS_LOW_OFFSET_R, 182);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_B,
		    BNR_NS_HIGH_OFFSET_B, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_GB,
		    BNR_NS_HIGH_OFFSET_GB, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_GR,
		    BNR_NS_HIGH_OFFSET_GR, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_OFFSET1_R,
		    BNR_NS_HIGH_OFFSET_R, 1023);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_B,
		    BNR_NS_LUMA_TH_B, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_GB,
		    BNR_NS_LUMA_TH_GB, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_GR,
		    BNR_NS_LUMA_TH_GR, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_LUMA_TH_R,
		    BNR_NS_LUMA_TH_R, 160);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_GAIN, BNR_NS_GAIN, 0);
}

void ispblk_bnr_config(struct isp_ctx *ctx, enum ISP_BNR_OUT out_sel, bool lsc_en, uint8_t ns_gain, uint8_t str)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];

	// Init once tuning
	_bnr_init(ctx);

	ISP_WO_BITS(bnr, REG_ISP_BNR_T, OUT_SEL, BNR_OUT_SEL, out_sel);

	//ISP_WO_BITS(bnr, REG_ISP_BNR_T, HSIZE, BNR_HSIZE, ctx->img_width);
	//ISP_WO_BITS(bnr, REG_ISP_BNR_T, VSIZE, BNR_VSIZE, ctx->img_height);
	//ISP_WO_BITS(bnr, REG_ISP_BNR_T, NS_GAIN, BNR_NS_GAIN, ns_gain);
	ISP_WO_BITS(bnr, REG_ISP_BNR_T, STRENGTH_MODE, BNR_STRENGTH_MODE, str);
}

void ispblk_cfa_config(struct isp_ctx *ctx)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];
	union REG_ISP_CFA_00 reg_0;

	reg_0.raw = ISP_RD_REG(cfa, REG_ISP_CFA_T, REG_00);
	reg_0.bits.CFA_SHDW_SEL = 1;
	reg_0.bits.CFA_ENABLE	= 1;
	//reg_0.bits.CFA_FCR_ENABLE = 1;
	//reg_0.bits.CFA_MOIRE_ENABLE = 1;
	ISP_WR_REG(cfa, REG_ISP_CFA_T, REG_00, reg_0.raw);
}

#define F_D (15)
void ispblk_lsc_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSC];

	int width = ctx->img_width;
	int height = ctx->img_height;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int InnerBlkY = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << F_D)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << F_D)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;

	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = InnerBlkX + 2;
	int OuterBlkY = InnerBlkY + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << F_D) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << F_D) - image_h_in_mesh_unit) / 2;

	union REG_ISP_LSC_INTERPOLATION inter_p;

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_BLD, LSC_BLDRATIO_ENABLE, en);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_BLD, LSC_BLDRATIO, 0x100);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_DMI_HEIGHTM1, 0xdd);

	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_XSTEP, reg_lsc_xstep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_YSTEP, reg_lsc_ystep);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_IMGY0, reg_lsc_imgy0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITX0, reg_lsc_imgx0);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_INITY0, reg_lsc_imgy0);

	//Tuning
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_STRENGTH, 0xfff);
	ISP_WR_REG(lsc, REG_ISP_LSC_T, LSC_GAIN_BASE, 0x0);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_3P9_0_4P8_1, 0);
	//ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_RENORMALIZE_ENABLE, 1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_GAIN_BICUBIC_0_BILINEAR_1, 1);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_BOUNDARY_INTERPOLATION_MODE, 1);

	inter_p.raw = ISP_RD_REG(lsc, REG_ISP_LSC_T, INTERPOLATION);
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_LF_RANGE = 0x3;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_UP_RANGE = 0x4;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_RT_RANGE = 0x1f;
	inter_p.bits.LSC_BOUNDARY_INTERPOLATION_DN_RANGE = 0x1c;
	ISP_WR_REG(lsc, REG_ISP_LSC_T, INTERPOLATION, inter_p.raw);

	ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, en);
	ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_ENABLE, en);
}

void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum cvi_isp_raw raw_num)
{
#if 0
	uintptr_t sts = ctx->phys_regs[blk_id];

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_GRACE_RESET, 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, AE_HIST_GRACE_RESET, 0);
#endif
}

void ispblk_aehist_config(struct isp_ctx *ctx, int blk_id, bool enable)
{
	uintptr_t sts = ctx->phys_regs[blk_id];
	uint8_t num_x = 34, num_y = 30;
	uint8_t sub_window_w = 0, sub_window_h = 0;
	union REG_ISP_AE_HIST_STS_AE0_HIST_ENABLE ae_enable;

#if defined( __SOC_PHOBOS__)
	if (blk_id == ISP_BLK_ID_AEHIST1)
		return;
#endif

	ae_enable.raw = ISP_RD_REG(sts, REG_ISP_AE_HIST_T, STS_AE0_HIST_ENABLE);
	ae_enable.bits.STS_AE0_HIST_ENABLE	= enable;
	ae_enable.bits.AE0_GAIN_ENABLE	= enable;
	ae_enable.bits.HIST0_ENABLE	= enable;
	ae_enable.bits.HIST0_GAIN_ENABLE = enable;
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE0_HIST_ENABLE, ae_enable.raw);
	ISP_WR_BITS(sts, REG_ISP_AE_HIST_T, DMI_ENABLE, DMI_ENABLE, enable);
	if (!enable)
		return;

	sub_window_w = ctx->img_width / num_x;
	sub_window_h = ctx->img_height / num_y;

	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMXM1, num_x - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_NUMYM1, num_y - 1);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_WIDTH, sub_window_w);
	ISP_WR_REG(sts, REG_ISP_AE_HIST_T, STS_AE_HEIGHT, sub_window_h);
}

void ispblk_gms_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

	u8 gap_x = 10, gap_y = 10;
	u16 start_x = 0, start_y = 0;
	// section size must be even, and size % 4 should be 2
	u16 x_section_size = 62, y_section_size = 62;
#if 0
	x_section_size = (x_section_size > ((ctx->img_width - start_x - gap_x * 2) / 3)) ?
				((ctx->img_width - start_x - gap_x * 2) / 3) : x_section_size;
	y_section_size = (y_section_size > ((ctx->img_height - start_y - gap_y * 2) / 3)) ?
				((ctx->img_height - start_y - gap_y * 2) / 3)  : y_section_size;
#endif
	ISP_WR_BITS(sts, REG_ISP_GMS_T, GMS_ENABLE, GMS_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_GMS_T, GMS_ENABLE, OUT_SHIFTBIT, 0);
	ISP_WR_BITS(sts, REG_ISP_GMS_T, DMI_ENABLE, DMI_ENABLE, enable);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_START_X, start_x);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_START_Y, start_y);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_X_SIZEM1, x_section_size - 1);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_Y_SIZEM1, y_section_size - 1);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_X_GAP, gap_x);
	ISP_WR_REG(sts, REG_ISP_GMS_T, GMS_Y_GAP, gap_y);
}

void ispblk_lmap_chg_size(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, const enum cvi_isp_pre_chn_num chn_num)
{
}

void ispblk_lmap_config(struct isp_ctx *ctx, int map_id, bool en)
{
	uintptr_t map = ctx->phys_regs[map_id];
	union REG_ISP_LMAP_LMP_0 reg0;

#if defined( __SOC_PHOBOS__)
	if (map_id == ISP_BLK_ID_LMAP1)
		return;
#endif

	reg0.raw = ISP_RD_REG(map, REG_ISP_LMAP_T, LMP_0);
	reg0.bits.LMAP_ENABLE = en;
	reg0.bits.LMAP_Y_MODE = 0;
	ISP_WR_REG(map, REG_ISP_LMAP_T, LMP_0, reg0.raw);
}

void ispblk_rgbcac_config(struct isp_ctx *ctx, bool en, uint8_t test_case)
{
	uintptr_t rgbcac = ctx->phys_regs[ISP_BLK_ID_RGBCAC];

	ISP_WR_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_CTRL, RGBCAC_ENABLE, en);

	if (test_case == 1) {
		ISP_WR_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_PURPLE_TH, RGBCAC_PURPLE_TH_LE, 0xFF);
		ISP_WR_BITS(rgbcac, REG_ISP_RGBCAC_T, RGBCAC_PURPLE_TH, RGBCAC_CORRECT_STRENGTH_LE, 0xFF);
	}
}

void ispblk_lcac_config(struct isp_ctx *ctx, bool en, uint8_t test_case)
{
	uintptr_t lcac = ctx->phys_regs[ISP_BLK_ID_LCAC];

	ISP_WR_BITS(lcac, REG_ISP_LCAC_T, REG00, LCAC_ENABLE, en);

	if (test_case == 1) {
		union REG_ISP_LCAC_REG08 reg8;
		union REG_ISP_LCAC_REG0C regC;

		reg8.raw = ISP_RD_REG(lcac, REG_ISP_LCAC_T, REG08);
		reg8.bits.LCAC_LTI_STR_R2_LE	= 64;
		reg8.bits.LCAC_LTI_STR_B2_LE	= 64;
		reg8.bits.LCAC_LTI_WGT_R_LE	= 0;
		reg8.bits.LCAC_LTI_WGT_B_LE	= 0;
		ISP_WR_REG(lcac, REG_ISP_LCAC_T, REG08, reg8.raw);

		regC.raw = ISP_RD_REG(lcac, REG_ISP_LCAC_T, REG0C);
		regC.bits.LCAC_LTI_STR_R2_SE	= 64;
		regC.bits.LCAC_LTI_STR_B2_SE	= 64;
		regC.bits.LCAC_LTI_WGT_R_SE	= 0;
		regC.bits.LCAC_LTI_WGT_B_SE	= 0;
		ISP_WR_REG(lcac, REG_ISP_LCAC_T, REG0C, regC.raw);
	}
}

