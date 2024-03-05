#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];

/*******************************************************************************
 *	BE IPs config
 ******************************************************************************/

void ispblk_dpc_config(struct isp_ctx *ctx, enum ISP_RAW_PATH path, bool enable, uint8_t test_case)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
		       ? ctx->phys_regs[ISP_BLK_ID_DPC0]
		       : ctx->phys_regs[ISP_BLK_ID_DPC1];
	union REG_ISP_DPC_2 reg_2;

#if defined( __SOC_PHOBOS__)
	if (path == ISP_RAW_PATH_SE)
		return;
#endif

	reg_2.raw = ISP_RD_REG(dpc, REG_ISP_DPC_T, DPC_2);
	reg_2.bits.DPC_ENABLE = enable;
	reg_2.bits.GE_ENABLE = enable;
	reg_2.bits.DPC_DYNAMICBPC_ENABLE = enable;
	reg_2.bits.DPC_STATICBPC_ENABLE = enable;
	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_2, reg_2.raw);

	if (test_case == 1) { // test static dpc
		reg_2.raw = ISP_RD_REG(dpc, REG_ISP_DPC_T, DPC_2);
		reg_2.bits.DPC_DYNAMICBPC_ENABLE = 0;
		reg_2.bits.DPC_STATICBPC_ENABLE = 1;
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_2, reg_2.raw);
	} else if (test_case == 2) { // test dynamic dpc
		reg_2.raw = ISP_RD_REG(dpc, REG_ISP_DPC_T, DPC_2);
		reg_2.bits.DPC_DYNAMICBPC_ENABLE = 1;
		reg_2.bits.DPC_STATICBPC_ENABLE = 0;
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_2, reg_2.raw);
	}
}

/**
 * ispblk_dpc_set_static - set defect pixels for static dpc.
 *
 * @param ctx: global settings
 * @param offset: mem-offset for 4k tile
 * @param bps: array of defect pixels. [23:12]-row, [11:0]-col.
 * @param count: number of defect pixels.
 */

void ispblk_dpc_set_static(struct isp_ctx *ctx, enum ISP_RAW_PATH path,
			     uint16_t offset, uint32_t *bps, uint16_t count)
{
	uintptr_t dpc = (path == ISP_RAW_PATH_LE)
		       ? ctx->phys_regs[ISP_BLK_ID_DPC0]
		       : ctx->phys_regs[ISP_BLK_ID_DPC1];
	uint16_t i = 0;

#if defined( __SOC_PHOBOS__)
	if (path == ISP_RAW_PATH_SE)
		return;
#endif

	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 1);

	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_ST_ADDR, 0x80000000 | offset);

	for (i = 0; (i < count) && (i < 2048); ++i)
		ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0,
			   0x80000000 | *(bps + i));

	// write 1 fff-fff to end
	ISP_WR_REG(dpc, REG_ISP_DPC_T, DPC_MEM_W0, 0x80ffffff);
	ISP_WR_BITS(dpc, REG_ISP_DPC_T, DPC_17, DPC_MEM_PROG_MODE, 0);
}

void ispblk_af_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	int numx = 17, numy = 15;

	ISP_WR_BITS(sts, REG_ISP_AF_T, KICKOFF, AF_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HORIZON_0_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HORIZON_1_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_VERTICAL_0_ENABLE, enable);
	ISP_WR_BITS(sts, REG_ISP_AF_T, ENABLES, AF_HLC_ENABLE, 1);
	ISP_WR_REG(sts, REG_ISP_AF_T, BYPASS, !enable);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_X, 0x8);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(sts, REG_ISP_AF_T, OFFSET_X, AF_OFFSET_Y, 0x2);
	// block_width >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_WIDTH, (ctx->img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_HEIGHT, (ctx->img_height - 4) / numy);
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_X, 17);
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_NUM_Y, 15);

	ISP_WR_REG(sts, REG_ISP_AF_T, HIGH_Y_THRE, 0x258);

	ISP_WR_REG(sts, REG_ISP_AF_T, IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_HEIGHT, ctx->img_height - 1);
}


