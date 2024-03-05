#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

/****************************************************************************
 * FBC_CONFIG
 ****************************************************************************/
#if defined( __SOC_PHOBOS__)
#define TNR_Y_W		5
#define TNR_C_W		6
#else
#define TNR_Y_W		10
#define TNR_C_W		11
#endif
#define TNR_Y_R		9
#define TNR_C_R		10

#define DEFAULT_K	2
#define CPLX_SHIFT	3
#define PEN_POS_SHIFT	4
#define TARGET_CR	43//, 55, 68, 80, 93, 100

struct vi_fbc_cfg fbc_cfg = {
	.cu_size	= 8,
	.target_cr	= TARGET_CR,
	.is_lossless	= 0,
	.y_bs_size	= 0,
	.c_bs_size	= 0,
	.y_buf_size	= 0,
	.c_buf_size	= 0,
};

/***************************************************************************
 * CA global setting
 ***************************************************************************/
u8 ca_y_lut[] = {
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};

u8 cp_y_lut[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

u8 cp_u_lut[] = {
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
};

u8 cp_v_lut[] = {
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
};

/*******************************************************************************
 *	YUV IPs config
 ******************************************************************************/

/**
 * ispblk_yuvdither_config - setup yuv dither.
 *
 * @param ctx: global settings
 * @param sel: y(0)/uv(1)
 * @param en: dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
int ispblk_yuvdither_config(struct isp_ctx *ctx, uint8_t sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en)
{
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUVDITHER];

	if (sel == 0) {
		union REG_ISP_YUV_DITHER_Y_DITHER reg;

		reg.raw = 0;
		reg.bits.Y_DITHER_ENABLE = en;
		reg.bits.Y_DITHER_MOD_ENABLE = mod_en;
		reg.bits.Y_DITHER_HISTIDX_ENABLE = histidx_en;
		reg.bits.Y_DITHER_FMNUM_ENABLE = fmnum_en;
		reg.bits.Y_DITHER_SHDW_SEL = 1;
		reg.bits.Y_DITHER_WIDTHM1 = ctx->img_width - 1;
		reg.bits.Y_DITHER_HEIGHTM1 = ctx->img_height - 1;

		ISP_WR_REG(dither, REG_ISP_YUV_DITHER_T, Y_DITHER, reg.raw);
	} else if (sel == 1) {
		union REG_ISP_YUV_DITHER_UV_DITHER reg;

		reg.raw = 0;
		reg.bits.UV_DITHER_ENABLE = en;
		reg.bits.UV_DITHER_MOD_ENABLE = mod_en;
		reg.bits.UV_DITHER_HISTIDX_ENABLE = histidx_en;
		reg.bits.UV_DITHER_FMNUM_ENABLE = fmnum_en;
		reg.bits.UV_DITHER_WIDTHM1 = (ctx->img_width >> 1) - 1;
		reg.bits.UV_DITHER_HEIGHTM1 = (ctx->img_height >> 1) - 1;

		ISP_WR_REG(dither, REG_ISP_YUV_DITHER_T, UV_DITHER, reg.raw);
	}

	return 0;
}

int ispblk_pre_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t pre_ee = ctx->phys_regs[ISP_BLK_ID_PRE_EE];
	union REG_ISP_EE_00  reg_0;

	reg_0.raw = ISP_RD_REG(pre_ee, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = en;
	ISP_WR_REG(pre_ee, REG_ISP_EE_T, REG_00, reg_0.raw);

	return 0;
}

void ispblk_tnr_config(struct isp_ctx *ctx, bool en, u8 test_case)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	union REG_ISP_444_422_8 tnr_8;
	union REG_ISP_444_422_13 tnr_13;
	union REG_ISP_444_422_14 tnr_14;
	union REG_ISP_444_422_15 tnr_15;

	if (en) {
		tnr_13.raw = 0;
		tnr_13.bits.REG_3DNR_Y_LUT_IN_0 = 0;
		tnr_13.bits.REG_3DNR_Y_LUT_IN_1 = 255;
		tnr_13.bits.REG_3DNR_Y_LUT_IN_2 = 255;
		tnr_13.bits.REG_3DNR_Y_LUT_IN_3 = 255;
		ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_13, tnr_13.raw);

		tnr_14.raw = 0;
		tnr_14.bits.REG_3DNR_Y_LUT_OUT_0 = 0;
		tnr_14.bits.REG_3DNR_Y_LUT_OUT_1 = 255;
		tnr_14.bits.REG_3DNR_Y_LUT_OUT_2 = 255;
		tnr_14.bits.REG_3DNR_Y_LUT_OUT_3 = 255;
		ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_14, tnr_14.raw);

		tnr_15.raw = 0;
		tnr_15.bits.REG_3DNR_Y_LUT_SLOPE_0 = 16;
		tnr_15.bits.REG_3DNR_Y_LUT_SLOPE_1 = 16;
		ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_15, tnr_15.raw);

		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_16, REG_3DNR_Y_LUT_SLOPE_2, 16);

		if (test_case == 1) {
			//select not pixel mode
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_16, MOTION_SEL, 0);
			//motion map output
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_8, TDNR_DEBUG_SEL, 1);
		}
	}

	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, REG_422_444, ctx->is_yuv_sensor);

	tnr_8.raw = ISP_RD_REG(tnr, REG_ISP_444_422_T, REG_8);
	tnr_8.bits.FORCE_DMA_DISABLE = (en) ? 0 : 0x3f;
	ISP_WR_REG(tnr, REG_ISP_444_422_T, REG_8, tnr_8.raw);

	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, TDNR_ENABLE, en);
	ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE, false);
}

void ispblk_fbc_clear_fbcd_ring_base(struct isp_ctx *ctx, u8 raw_num)
{
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];

	ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, UP_RING_BASE, ((1 << TNR_Y_R) | (1 << TNR_C_R)));
}

void ispblk_fbc_chg_to_sw_mode(struct isp_ctx *ctx, u8 raw_num)
{
	uintptr_t y_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL41];
	uintptr_t uv_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL42];
	union REG_ISP_DMA_CTL_SYS_CONTROL sys_ctrl;

	// Y/UV rdma config seglen to HW mode, other SW mode
	sys_ctrl.raw = ISP_RD_REG(y_rdma, REG_ISP_DMA_CTL_T, SYS_CONTROL);
	sys_ctrl.bits.BASE_SEL		= 0x1;
	sys_ctrl.bits.STRIDE_SEL	= true;
	sys_ctrl.bits.SEGLEN_SEL	= false;
	sys_ctrl.bits.SEGNUM_SEL	= true;
	ISP_WR_REG(y_rdma, REG_ISP_DMA_CTL_T, SYS_CONTROL, sys_ctrl.raw);

	sys_ctrl.raw = ISP_RD_REG(uv_rdma, REG_ISP_DMA_CTL_T, SYS_CONTROL);
	sys_ctrl.bits.BASE_SEL		= 0x1;
	sys_ctrl.bits.STRIDE_SEL	= true;
	sys_ctrl.bits.SEGLEN_SEL	= false;
	sys_ctrl.bits.SEGNUM_SEL	= true;
	ISP_WR_REG(uv_rdma, REG_ISP_DMA_CTL_T, SYS_CONTROL, sys_ctrl.raw);
}

void vi_fbc_calculate_size(struct isp_ctx *ctx, u8 raw_num)
{
	u32 img_w = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_h = ctx->isp_pipe_cfg[raw_num].crop.h;

	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);

	u32 y_bs_size = VI_ALIGN(((total_line_bit_budget * (img_h - 1) + total_first_line_bit_budget) / 512) * 64);
	u32 uv_bs_size = VI_ALIGN((((total_line_bit_budget * (img_h / 2) - 1)
				+ total_first_line_bit_budget) / 512) * 64);
	u32 y_buf_size = VI_256_ALIGN(y_bs_size);
	u32 uv_buf_size = VI_256_ALIGN(uv_bs_size);

	fbc_cfg.y_bs_size = y_bs_size;
	fbc_cfg.c_bs_size = uv_bs_size;
	fbc_cfg.y_buf_size = y_buf_size;
	fbc_cfg.c_buf_size = uv_buf_size;
}

void ispblk_fbc_ring_buf_config(struct isp_ctx *ctx, u8 en)
{
	//WDMA_CORE, RDMA_CORE ring buffer ctrl
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];
	union REG_WDMA_CORE_DISABLE_SEGLEN	disable_seglen;
	union REG_WDMA_CORE_RING_BUFFER_EN	ring_buf_en;

	vi_fbc_calculate_size(ctx, ISP_PRERAW_A);

	if (en) {
		disable_seglen.raw = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, DISABLE_SEGLEN);
		disable_seglen.bits.SEGLEN_DISABLE |= ((1 << TNR_Y_W) | (1 << TNR_C_W));
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, DISABLE_SEGLEN, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_EN);
		ring_buf_en.bits.RING_ENABLE |= ((1 << TNR_Y_W) | (1 << TNR_C_W));
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_EN, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN);
		ring_buf_en.bits.RING_ENABLE |= ((1 << TNR_Y_R) | (1 << TNR_C_R));
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN, ring_buf_en.raw);

		//WDMA ctrl cfg
#if defined( __SOC_PHOBOS__)
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_SIZE5, fbc_cfg.y_buf_size);
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_SIZE6, fbc_cfg.c_buf_size);
#else
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_SIZE10, fbc_cfg.y_buf_size);
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_SIZE11, fbc_cfg.c_buf_size);
#endif
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, UP_RING_BASE, ((1 << TNR_Y_W) | (1 << TNR_C_W)));

		//RDMA ctrl cfg
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE9, fbc_cfg.y_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE10, fbc_cfg.c_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, UP_RING_BASE, ((1 << TNR_Y_R) | (1 << TNR_C_R)));
	} else {

		disable_seglen.raw = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, DISABLE_SEGLEN);
		disable_seglen.bits.SEGLEN_DISABLE &= ~((1 << TNR_Y_W) | (1 << TNR_C_W));
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, DISABLE_SEGLEN, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_EN);
		ring_buf_en.bits.RING_ENABLE &= ~((1 << TNR_Y_W) | (1 << TNR_C_W));
		ISP_WR_REG(wdma_com_1, REG_WDMA_CORE_T, RING_BUFFER_EN, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN);
		ring_buf_en.bits.RING_ENABLE &= ~((1 << TNR_Y_R) | (1 << TNR_C_R));
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN, ring_buf_en.raw);
	}
}

void ispblk_fbcd_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union REG_FBCD_24	d_reg_24;
	union REG_FBCD_28	d_reg_28;
	union REG_FBCE_10	reg_10;
	union REG_FBCE_20	reg_20;

	if (en) {
		reg_10.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_10);
		reg_20.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_20);

		d_reg_24.raw = ISP_RD_REG(fbcd, REG_FBCD_T, REG_24);
		d_reg_24.bits.Y_LOSSLESS		= reg_10.bits.Y_LOSSLESS;
		d_reg_24.bits.Y_BASE_QDPCM_Q		= reg_10.bits.Y_BASE_QDPCM_Q;
		d_reg_24.bits.Y_BASE_PCM_BD_MINUS2	= reg_10.bits.Y_BASE_PCM_BD_MINUS2;
		d_reg_24.bits.Y_DEFAULT_GR_K		= DEFAULT_K;
		ISP_WR_REG(fbcd, REG_FBCD_T, REG_24, d_reg_24.raw);

		d_reg_28.raw = ISP_RD_REG(fbcd, REG_FBCD_T, REG_28);
		d_reg_28.bits.C_LOSSLESS		= reg_20.bits.C_LOSSLESS;
		d_reg_28.bits.C_BASE_QDPCM_Q		= reg_20.bits.C_BASE_QDPCM_Q;
		d_reg_28.bits.C_BASE_PCM_BD_MINUS2	= reg_20.bits.C_BASE_PCM_BD_MINUS2;
		d_reg_28.bits.C_DEFAULT_GR_K		= DEFAULT_K;
		ISP_WR_REG(fbcd, REG_FBCD_T, REG_28, d_reg_28.raw);
	}

	ISP_WR_BITS(fbcd, REG_FBCD_T, REG_00, FBCD_EN, en);
}

void ispblk_fbce_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union REG_FBCE_10	reg_10;
	union REG_FBCE_14	reg_14;
	union REG_FBCE_20	reg_20;
	union REG_FBCE_24	reg_24;
	union REG_FBCE_2C	reg_2C;

	u32 img_w = ctx->isp_pipe_cfg[ISP_PRERAW_A].crop.w;

	u32 cu_md_bit = fbc_cfg.is_lossless ? 1 : 3;
	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);
	u32 cu_target_bit = total_line_bit_budget / line_cu_num;
	u32 base_dpcm_q = ((cu_target_bit <= 27) ? 1 : 0);
	u32 base_pcm_bd = (cu_target_bit - cu_md_bit) / fbc_cfg.cu_size;
	u32 min_cu_bit = fbc_cfg.is_lossless ? max_cu_bit : (base_pcm_bd * fbc_cfg.cu_size + cu_md_bit);

	if (base_pcm_bd < 2)
		base_pcm_bd = 2;
	else if (base_pcm_bd > 8)
		base_pcm_bd = 8;

	if (en) {
		reg_14.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_14);
		reg_14.bits.Y_TOTAL_LINE_BIT_BUDGET	= total_line_bit_budget;
		reg_14.bits.Y_MAX_CU_BIT		= max_cu_bit;
		reg_14.bits.Y_MIN_CU_BIT		= min_cu_bit;
		ISP_WR_REG(fbce, REG_FBCE_T, REG_14, reg_14.raw);

		reg_10.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_10);
		reg_10.bits.Y_BASE_QDPCM_Q		= base_dpcm_q;
		reg_10.bits.Y_BASE_PCM_BD_MINUS2	= base_pcm_bd - 2;
		reg_10.bits.Y_CPLX_SHIFT		= CPLX_SHIFT;
		reg_10.bits.Y_PEN_POS_SHIFT		= PEN_POS_SHIFT;
		reg_10.bits.Y_DEFAULT_GR_K		= DEFAULT_K;
		reg_10.bits.Y_LOSSLESS			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, REG_FBCE_T, REG_10, reg_10.raw);

		reg_2C.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_2C);
		reg_2C.bits.Y_TOTAL_FIRST_LINE_BIT_BUDGET = total_first_line_bit_budget;

		reg_24.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_24);
		reg_24.bits.C_TOTAL_LINE_BIT_BUDGET	= total_line_bit_budget;
		reg_24.bits.C_MAX_CU_BIT		= max_cu_bit;
		reg_24.bits.C_MIN_CU_BIT		= min_cu_bit;
		ISP_WR_REG(fbce, REG_FBCE_T, REG_24, reg_24.raw);

		reg_20.raw = ISP_RD_REG(fbce, REG_FBCE_T, REG_20);
		reg_20.bits.C_BASE_QDPCM_Q		= 0;
		reg_20.bits.C_BASE_PCM_BD_MINUS2	= base_pcm_bd - 2;
		reg_20.bits.C_CPLX_SHIFT		= CPLX_SHIFT;
		reg_20.bits.C_PEN_POS_SHIFT		= PEN_POS_SHIFT;
		reg_20.bits.C_DEFAULT_GR_K		= DEFAULT_K;
		reg_20.bits.C_LOSSLESS			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, REG_FBCE_T, REG_20, reg_20.raw);

		reg_2C.bits.C_TOTAL_FIRST_LINE_BIT_BUDGET = total_first_line_bit_budget;
		ISP_WR_REG(fbce, REG_FBCE_T, REG_2C, reg_2C.raw);
	}

	ISP_WR_BITS(fbce, REG_FBCE_T, REG_00, FBCE_EN, en);
}

void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, uint8_t str_mode, uint8_t test_case)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	union REG_ISP_CNR_ENABLE reg_00;
	union REG_ISP_CNR_STRENGTH_MODE reg_01;
	union REG_ISP_CNR_PURPLE_TH reg_02;
	union REG_ISP_CNR_EDGE_SCALE reg_03;
	union REG_ISP_CNR_EDGE_RATIO_SPEED reg_04;

	// test_case = 0, for cnr_all_off and cnr_all_on, use default lut
	//   if cnr_all_off, en = 0, pfc_en = 0, str_mode = 255
	//   if cnr_all_on, en = 1, pfc_en = 1, str_mode = 255
	if (test_case == 0) {
		reg_00.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_ENABLE);
		reg_00.bits.CNR_ENABLE = en;
		reg_00.bits.PFC_ENABLE = pfc_en;
		reg_00.bits.CNR_DIFF_SHIFT_VAL = 255;
		reg_00.bits.CNR_RATIO = 0;
		reg_00.bits.CNR_OUT_SEL = 0;
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_ENABLE, reg_00.raw);

		reg_01.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_STRENGTH_MODE);
		reg_01.bits.CNR_STRENGTH_MODE = str_mode;
		reg_01.bits.CNR_FLAG_NEIGHBOR_MAX_WEIGHT = 1;
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_STRENGTH_MODE, reg_01.raw);

		reg_02.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_PURPLE_TH);
		reg_02.bits.CNR_PURPLE_TH = 85;
		reg_02.bits.CNR_CORRECT_STRENGTH = 96;
		reg_02.bits.CNR_DIFF_GAIN = 4;
		reg_02.bits.CNR_MOTION_ENABLE = 0;
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_PURPLE_TH, reg_02.raw);

		reg_03.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE);
		reg_03.bits.CNR_EDGE_SCALE = 12;
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE, reg_03.raw);

		reg_04.raw = ISP_RD_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_RATIO_SPEED);
		reg_04.bits.CNR_CB_STR = 8;
		reg_04.bits.CNR_CR_STR = 8;
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_RATIO_SPEED, reg_04.raw);
	} else if (test_case == 1) { // for cnr_set_lut, other registers with default values
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_0, 0x1E1E1E1E);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_4, 0x1E1E1E1E);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_8, 0x1E1E1E1E);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_MOTION_LUT_12, 0x1E1E1E1E);

		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_0, 0xFFFFFFFF);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_4, 0xFFFFFFFF);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_8, 0xFFFFFFFF);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_CORING_MOTION_LUT_12, 0xFFFFFFFF);

		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE_LUT_0, 0x20202020);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE_LUT_4, 0x20202020);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE_LUT_8, 0x20202020);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, CNR_EDGE_SCALE_LUT_12, 0x20202020);

		ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_00, 0x10101010);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_04, 0x10101010);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_08, 0x10101010);
		ISP_WR_REG(cnr, REG_ISP_CNR_T, WEIGHT_LUT_INTER_CNR_12, 0x10101010);
	}
}


void ispblk_ynr_config(struct isp_ctx *ctx, enum ISP_YNR_OUT out_sel, uint8_t ns_gain)
{
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];

	// depth =64
	uint8_t weight_lut[] = {
		   31,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	};

	uint16_t i = 0;

	ISP_WO_BITS(ynr, REG_ISP_YNR_T, INDEX_CLR, YNR_INDEX_CLR, 1);

	for (i = 0; i < ARRAY_SIZE(weight_lut); ++i) {
		   ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_LUT,
					  weight_lut[i]);
	}

	// ns0 luma th
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_00, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_01, 16);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_02, 32);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_03, 64);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_04, 128);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_LUMA_TH_05, 255);

	// ns0 slope
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE_00, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE_01, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE_02, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE_03, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_SLOPE_04, 0);

	// ns0 offset
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_00, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_01, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_02, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_03, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_04, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS0_OFFSET_05, 255);

	// ns1 luma th
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_00, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_01, 16);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_02, 32);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_03, 64);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_04, 128);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_LUMA_TH_05, 255);

	// ns1 slope
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE_00, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE_01, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE_02, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE_03, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_SLOPE_04, 0);

	// ns1 offset
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_00, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_01, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_02, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_03, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_04, 255);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS1_OFFSET_05, 255);

	// motion lut
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_00, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_01, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_02, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_03, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_04, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_05, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_06, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_07, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_08, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_09, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_10, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_11, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_12, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_13, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_14, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, MOTION_LUT_15, 0);

	ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, ISP_YNR_OUT_BYPASS);

	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_0, 6);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_1, 1);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_INTRA_2, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_1, 51);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_NORM_2, 85);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, ALPHA_GAIN, 256);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, VAR_TH, 64);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_SM, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_V, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_H, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_D45, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, WEIGHT_D135, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NEIGHBOR_MAX, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_K_SMOOTH, 128);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, RES_K_TEXTURE, 128);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, FILTER_MODE_EN, 0);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, FILTER_MODE_ALPHA, 128);

	ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, out_sel);
	ISP_WR_REG(ynr, REG_ISP_YNR_T, NS_GAIN, ns_gain);
}

int ispblk_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE];
	union REG_ISP_EE_00  reg_0;

	reg_0.raw = ISP_RD_REG(ee, REG_ISP_EE_T, REG_00);
	reg_0.bits.EE_ENABLE = en;
	ISP_WR_REG(ee, REG_ISP_EE_T, REG_00, reg_0.raw);

	return 0;
}

void ispblk_dci_config(struct isp_ctx *ctx, bool en, uint8_t sel, uint16_t *lut, uint8_t test_case)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	union REG_ISP_DCI_GAMMA_PROG_CTRL dci_gamma_ctrl;
	union REG_ISP_DCI_GAMMA_PROG_DATA dci_gamma_data;
	u16 i = 0;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_HIST_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_MAP_ENABLE, test_case);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_PER1SAMPLE_ENABLE, en);
	ISP_WR_REG(dci, REG_ISP_DCI_T, DCI_DEMO_MODE, test_case);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, en);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL);
	dci_gamma_ctrl.bits.GAMMA_WSEL = sel;
	dci_gamma_ctrl.bits.GAMMA_PROG_EN = 1;
	dci_gamma_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		dci_gamma_data.bits.GAMMA_DATA_E = lut[i];
		dci_gamma_data.bits.GAMMA_DATA_O = lut[i + 1];
		dci_gamma_data.bits.GAMMA_W = 1;
		ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_DATA, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_RSEL, sel);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}

void ispblk_ldci_config(struct isp_ctx *ctx, bool en, uint8_t test_case)
{
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union REG_ISP_LDCI_BLK_SIZE_X          blk_size_x;
	union REG_ISP_LDCI_BLK_SIZE_X1         blk_size_x1;
	union REG_ISP_LDCI_SUBBLK_SIZE_X       subblk_size_x;
	union REG_ISP_LDCI_SUBBLK_SIZE_X1      subblk_size_x1;
	union REG_ISP_LDCI_INTERP_NORM_LR      interp_norm_lr;
	union REG_ISP_LDCI_INTERP_NORM_LR1     interp_norm_lr1;
	union REG_ISP_LDCI_SUB_INTERP_NORM_LR  sub_interp_norm_lr;
	union REG_ISP_LDCI_SUB_INTERP_NORM_LR1 sub_interp_norm_lr1;
	union REG_ISP_LDCI_MEAN_NORM_X         mean_norm_x;
	union REG_ISP_LDCI_VAR_NORM_Y          var_norm_y;

	uint16_t BlockSizeX, BlockSizeY, SubBlockSizeX, SubBlockSizeY;
	uint16_t BlockSizeX1, BlockSizeY1, SubBlockSizeX1, SubBlockSizeY1;
	uint16_t line_mean_num, line_var_num;
	uint16_t dW, dH;

	uint16_t width = ctx->img_width;
	uint16_t height = ctx->img_height;

	if ((width % 16 == 0) && (height % 12 == 0))
		ISP_WR_BITS(ldci, REG_ISP_LDCI_T, LDCI_ENABLE, LDCI_IMAGE_SIZE_DIV_BY_16X12, 1);
	else
		ISP_WR_BITS(ldci, REG_ISP_LDCI_T, LDCI_ENABLE, LDCI_IMAGE_SIZE_DIV_BY_16X12, 0);

	BlockSizeX = (width % 16 == 0) ? (width / 16) : (width / 16) + 1; // Width of one block
	BlockSizeY = (height % 12 == 0) ? (height / 12) : (height / 12) + 1; // Height of one block
	SubBlockSizeX = (BlockSizeX >> 1);
	SubBlockSizeY = (BlockSizeY >> 1);
	line_mean_num = (BlockSizeY / 2) + (BlockSizeY % 2);
	line_var_num  = (BlockSizeY / 2);

	if (width % 16 == 0) {
		BlockSizeX1 = BlockSizeX;
		SubBlockSizeX1 = width - BlockSizeX * (16 - 1) - SubBlockSizeX;
	} else {
		dW = BlockSizeX * 16 - width;
		BlockSizeX1 = 2 * BlockSizeX - SubBlockSizeX - dW;
		SubBlockSizeX1 = 0;
	}

	if (height % 12 == 0) {
		BlockSizeY1 = BlockSizeY;
		SubBlockSizeY1 = height - BlockSizeY * (12 - 1) - SubBlockSizeY;
	} else {
		dH = BlockSizeY * 12 - height;
		BlockSizeY1 = 2 * BlockSizeY - SubBlockSizeY - dH;
		SubBlockSizeY1 = 0;
	}

	blk_size_x.raw = 0;
	blk_size_x.bits.LDCI_BLK_SIZE_X = BlockSizeX;
	blk_size_x.bits.LDCI_BLK_SIZE_Y = BlockSizeY;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_BLK_SIZE_X, blk_size_x.raw);

	BlockSizeX1 = BlockSizeX;
	BlockSizeY1 = BlockSizeY;

	blk_size_x1.raw = 0;
	blk_size_x1.bits.LDCI_BLK_SIZE_X1 = BlockSizeX1;
	blk_size_x1.bits.LDCI_BLK_SIZE_Y1 = BlockSizeY1;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_BLK_SIZE_X1, blk_size_x1.raw);

	subblk_size_x.raw = 0;
	subblk_size_x.bits.LDCI_SUBBLK_SIZE_X = SubBlockSizeX;
	subblk_size_x.bits.LDCI_SUBBLK_SIZE_Y = SubBlockSizeY;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_SUBBLK_SIZE_X, subblk_size_x.raw);

	SubBlockSizeX1 = SubBlockSizeX;
	SubBlockSizeY1 = SubBlockSizeY;

	subblk_size_x1.raw = 0;
	subblk_size_x1.bits.LDCI_SUBBLK_SIZE_X1 = SubBlockSizeX1;
	subblk_size_x1.bits.LDCI_SUBBLK_SIZE_Y1 = SubBlockSizeY1;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_SUBBLK_SIZE_X1, subblk_size_x1.raw);

	interp_norm_lr.raw = 0;
	interp_norm_lr.bits.LDCI_INTERP_NORM_LR = (BlockSizeX == 0) ? 0 : (1 << 16) / BlockSizeX;
	interp_norm_lr.bits.LDCI_INTERP_NORM_UD = (BlockSizeY == 0) ? 0 : (1 << 16) / BlockSizeY;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_INTERP_NORM_LR, interp_norm_lr.raw);

	interp_norm_lr1.raw = 0;
	interp_norm_lr1.bits.LDCI_INTERP_NORM_LR1 = (BlockSizeX1 == 0) ? 0 : (1 << 16) / BlockSizeX1;
	interp_norm_lr1.bits.LDCI_INTERP_NORM_UD1 = (BlockSizeY1 == 0) ? 0 : (1 << 16) / BlockSizeY1;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_INTERP_NORM_LR1, interp_norm_lr1.raw);

	sub_interp_norm_lr.raw = 0;
	sub_interp_norm_lr.bits.LDCI_SUB_INTERP_NORM_LR = (SubBlockSizeX == 0) ? 0 : (1 << 16) / SubBlockSizeX;
	sub_interp_norm_lr.bits.LDCI_SUB_INTERP_NORM_UD = (SubBlockSizeY == 0) ? 0 : (1 << 16) / SubBlockSizeY;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_SUB_INTERP_NORM_LR, sub_interp_norm_lr.raw);

	sub_interp_norm_lr1.raw = 0;
	sub_interp_norm_lr1.bits.LDCI_SUB_INTERP_NORM_LR1 = (SubBlockSizeX1 == 0) ? 0 : (1 << 16) / SubBlockSizeX1;
	sub_interp_norm_lr1.bits.LDCI_SUB_INTERP_NORM_UD1 = (SubBlockSizeY1 == 0) ? 0 : (1 << 16) / SubBlockSizeY1;
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_SUB_INTERP_NORM_LR1, sub_interp_norm_lr1.raw);

	mean_norm_x.raw = 0;
	mean_norm_x.bits.LDCI_MEAN_NORM_X = (1 << 14) / MAX(BlockSizeX, 1);
	mean_norm_x.bits.LDCI_MEAN_NORM_Y = (1 << 13) / MAX(line_mean_num, 1);
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_MEAN_NORM_X, mean_norm_x.raw);

	var_norm_y.raw = 0;
	var_norm_y.bits.LDCI_VAR_NORM_Y = (1 << 13) / MAX(line_var_num, 1);
	ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_VAR_NORM_Y, var_norm_y.raw);

	if (test_case == 1) {
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_00, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_02, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_04, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_06, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_08, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_10, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_12, (1023 << 16) | (1023));
		ISP_WR_REG(ldci, REG_ISP_LDCI_T, LDCI_TONE_CURVE_LUT_P_14, (1023 << 16) | (1023));
	}

	ISP_WR_BITS(ldci, REG_ISP_LDCI_T, LDCI_ENABLE, LDCI_ENABLE, en);
	ISP_WR_BITS(ldci, REG_ISP_LDCI_T, DMI_ENABLE, DMI_ENABLE, en ? 3 : 0);
}


void ispblk_ca_config(struct isp_ctx *ctx, bool en, uint8_t mode)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	u16 i = 0;
	union REG_CA_04 wdata;

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_ENABLE, en);
	// 0 CA mode, 1 CP mode
	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_MODE, mode);

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_ISO_RATIO, 64);

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_MEM_SW_MODE, 1);

	if (mode == 0) {
		for (i = 0; i < sizeof(ca_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ca_y_lut[i];
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_CA_T, REG_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < sizeof(cp_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.CACP_MEM_D = ((cp_v_lut[i]) | (cp_u_lut[i] << 8) | (cp_y_lut[i] << 16));
			wdata.bits.CACP_MEM_W = 1;
			ISP_WR_REG(cacp, REG_CA_T, REG_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_MEM_SW_MODE, 0);
}

void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_00, CA_LITE_ENABLE, en);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_04, CA_LITE_LUT_IN_0, 0x0);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_04, CA_LITE_LUT_IN_1, 0x80);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_08, CA_LITE_LUT_IN_2, 0x100);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_08, CA_LITE_LUT_IN_3, 0x100);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_0C, CA_LITE_LUT_IN_4, 0x100);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_0C, CA_LITE_LUT_IN_5, 0x100);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_10, CA_LITE_LUT_OUT_0, 0x100);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_10, CA_LITE_LUT_OUT_1, 0x80);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_14, CA_LITE_LUT_OUT_2, 0x40);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_14, CA_LITE_LUT_OUT_3, 0x40);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_18, CA_LITE_LUT_OUT_4, 0x40);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_18, CA_LITE_LUT_OUT_5, 0x40);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_1C, CA_LITE_LUT_SLP_0, 0x0);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_1C, CA_LITE_LUT_SLP_1, 0x0);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_20, CA_LITE_LUT_SLP_2, 0x0);
	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_20, CA_LITE_LUT_SLP_3, 0x0);

	ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_24, CA_LITE_LUT_SLP_4, 0x0);
}

void ispblk_ycur_config(struct isp_ctx *ctx, bool en, uint8_t sel, uint16_t *data)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uint16_t i;
	union REG_ISP_YCURV_YCUR_PROG_DATA reg_data;

	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 1);

	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_WSEL, sel);
	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_ST_ADDR, YCUR_ST_ADDR, 0);
	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_ST_ADDR, YCUR_ST_W, 1);
	ISP_WR_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_MAX, data[64]);
	for (i = 0; i < 64; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.YCUR_DATA_E = data[i];
		reg_data.bits.YCUR_DATA_O = data[i + 1];
		reg_data.bits.YCUR_W = 1;
		ISP_WR_REG(ycur, REG_ISP_YCURV_T, YCUR_PROG_DATA, reg_data.raw);
	}

	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_RSEL, sel);
	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_PROG_EN, 0);
}

void ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, uint8_t sel)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];

	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_CTRL, YCUR_ENABLE, enable);
	ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_PROG_CTRL, YCUR_RSEL, sel);
}

#ifdef PORTING_TEST
void ispblk_dci_restore_default_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	union REG_ISP_DCI_GAMMA_PROG_CTRL dci_gamma_ctrl;
	union REG_ISP_DCI_GAMMA_PROG_DATA dci_gamma_data;
	u16 i = 0;

	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_HIST_ENABLE, en);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_MAP_ENABLE, 0);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_MAP_ENABLE, DCI_PER1SAMPLE_ENABLE, en);
	ISP_WR_REG(dci, REG_ISP_DCI_T, DCI_DEMO_MODE, 0);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, DMI_ENABLE, DMI_ENABLE, en);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL);
	dci_gamma_ctrl.bits.GAMMA_WSEL = 0;
	dci_gamma_ctrl.bits.GAMMA_PROG_EN = 0;
	dci_gamma_ctrl.bits.GAMMA_PROG_1TO3_EN = 1;
	ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		ISP_WR_REG(dci, REG_ISP_DCI_T, GAMMA_PROG_DATA, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_RSEL, 1);
	ISP_WR_BITS(dci, REG_ISP_DCI_T, GAMMA_PROG_CTRL, GAMMA_PROG_EN, 0);
}
#endif
