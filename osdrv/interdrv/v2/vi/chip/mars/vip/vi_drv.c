#include <vip/vi_drv.h>
#include <vip_common.h>

#define LUMA_MAP_W_BIT	4
#define LUMA_MAP_H_BIT	4
#define MANR_W_BIT	3
#define MANR_H_BIT	3

#define RGBMAP_MAX_BIT		3
#define SLICE_MAX_GRID_SIZE	5

/****************************************************************************
 * Global parameters
 ****************************************************************************/
uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
uint8_t g_rgbmap_chg_pre[ISP_PRERAW_VIRT_MAX][2];

/****************************************************************************
 * LMAP_CONFIG
 ****************************************************************************/
struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

/****************************************************************************
 * FBC_CONFIG
 ****************************************************************************/
extern struct vi_fbc_cfg fbc_cfg;

/****************************************************************************
 * SLICE_BUFFER_CONFIG
 ****************************************************************************/
enum W_RING_BUF_ID {
#if defined( __SOC_PHOBOS__)
	RGBMAP_LE = 2,
	BE_WDMA_LE = 3,
#else
	RGBMAP_LE = 4,
	BE_WDMA_LE = 6,
#endif
	RGBMAP_SE = 5,
	BE_WDMA_SE = 7,
	W_RING_BUF_ID_MAX,
};

enum R_RING_BUF_ID {
	RAW_RDMA_LE = 1,
	RAW_RDMA_SE = 2,
	MANR_PREV_LE = 4,
	MANR_PREV_SE = 5,
	MANR_CUR_LE = 6,
	MANR_CUR_SE = 7,
	R_RING_BUF_ID_MAX,
};

struct slice_buf_s slc_b_cfg = {
	.line_delay		= 512,
	.buffer			= 16,
	.main_max_grid_size	= 32,
	.sub_max_grid_size	= 8,
	.min_r_thshd		= 1,
};

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_be_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && ctx->is_offline_postraw) //fe->be->dram->post
		return 1;
	return 0;
}

u32 _is_be_post_online(struct isp_ctx *ctx)
{
	if (ctx->is_offline_be && !ctx->is_offline_postraw) //fe->dram->be->post
		return 1;
	return 0;
}

u32 _is_all_online(struct isp_ctx *ctx)
{
	if (!ctx->is_offline_be && !ctx->is_offline_postraw)
		return 1;
	return 0;
}

u32 _is_post_sclr_online(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
		return 1;
	return 0;
}

/****************************************************************************
 *  Interfaces
 ****************************************************************************/
void vi_set_base_addr(void *base)
{
	uintptr_t *addr = isp_get_phys_reg_bases();
	int i = 0;

	for (i = 0; i < ISP_BLK_ID_MAX; ++i) {
		addr[i] += (uintptr_t)base;
	}
}

uintptr_t *isp_get_phys_reg_bases(void)
{
	static uintptr_t m_isp_phys_base_list[ISP_BLK_ID_MAX] = {
		[ISP_BLK_ID_PRE_RAW_FE0]	= (ISP_BLK_BA_PRE_RAW_FE0),
		[ISP_BLK_ID_CSIBDG0]		= (ISP_BLK_BA_CSIBDG0),
		[ISP_BLK_ID_DMA_CTL6]		= (ISP_BLK_BA_DMA_CTL6),
		[ISP_BLK_ID_DMA_CTL7]		= (ISP_BLK_BA_DMA_CTL7),
		[ISP_BLK_ID_DMA_CTL8]		= (ISP_BLK_BA_DMA_CTL8),
		[ISP_BLK_ID_DMA_CTL9]		= (ISP_BLK_BA_DMA_CTL9),
		[ISP_BLK_ID_BLC0]		= (ISP_BLK_BA_BLC0),
		[ISP_BLK_ID_BLC1]		= (ISP_BLK_BA_BLC1),
		[ISP_BLK_ID_RGBMAP0]		= (ISP_BLK_BA_RGBMAP0),
		[ISP_BLK_ID_WBG2]		= (ISP_BLK_BA_WBG2),
		[ISP_BLK_ID_DMA_CTL10]		= (ISP_BLK_BA_DMA_CTL10),
		[ISP_BLK_ID_RGBMAP1]		= (ISP_BLK_BA_RGBMAP1),
		[ISP_BLK_ID_WBG3]		= (ISP_BLK_BA_WBG3),
		[ISP_BLK_ID_DMA_CTL11]		= (ISP_BLK_BA_DMA_CTL11),
		[ISP_BLK_ID_PRE_RAW_FE1]	= (ISP_BLK_BA_PRE_RAW_FE1),
		[ISP_BLK_ID_CSIBDG1]		= (ISP_BLK_BA_CSIBDG1),
		[ISP_BLK_ID_DMA_CTL12]		= (ISP_BLK_BA_DMA_CTL12),
		[ISP_BLK_ID_DMA_CTL13]		= (ISP_BLK_BA_DMA_CTL13),
		[ISP_BLK_ID_DMA_CTL14]		= (ISP_BLK_BA_DMA_CTL14),
		[ISP_BLK_ID_DMA_CTL15]		= (ISP_BLK_BA_DMA_CTL15),
		[ISP_BLK_ID_BLC2]		= (ISP_BLK_BA_BLC2),
		[ISP_BLK_ID_BLC3]		= (ISP_BLK_BA_BLC3),
		[ISP_BLK_ID_RGBMAP2]		= (ISP_BLK_BA_RGBMAP2),
		[ISP_BLK_ID_WBG4]		= (ISP_BLK_BA_WBG4),
		[ISP_BLK_ID_DMA_CTL16]		= (ISP_BLK_BA_DMA_CTL16),
		[ISP_BLK_ID_RGBMAP3]		= (ISP_BLK_BA_RGBMAP3),
		[ISP_BLK_ID_WBG5]		= (ISP_BLK_BA_WBG5),
		[ISP_BLK_ID_DMA_CTL17]		= (ISP_BLK_BA_DMA_CTL17),
		[ISP_BLK_ID_PRE_RAW_FE2]	= (ISP_BLK_BA_PRE_RAW_FE2),
		[ISP_BLK_ID_CSIBDG2]		= (ISP_BLK_BA_CSIBDG2),
		[ISP_BLK_ID_DMA_CTL18]		= (ISP_BLK_BA_DMA_CTL18),
		[ISP_BLK_ID_DMA_CTL19]		= (ISP_BLK_BA_DMA_CTL19),
		[ISP_BLK_ID_BLC4]		= (ISP_BLK_BA_BLC4),
		[ISP_BLK_ID_RGBMAP4]		= (ISP_BLK_BA_RGBMAP4),
		[ISP_BLK_ID_WBG6]		= (ISP_BLK_BA_WBG6),
		[ISP_BLK_ID_DMA_CTL20]		= (ISP_BLK_BA_DMA_CTL20),
		[ISP_BLK_ID_PRE_RAW_BE] 	= (ISP_BLK_BA_PRE_RAW_BE),
		[ISP_BLK_ID_CROP0]		= (ISP_BLK_BA_CROP0),
		[ISP_BLK_ID_CROP1]		= (ISP_BLK_BA_CROP1),
		[ISP_BLK_ID_BLC5]		= (ISP_BLK_BA_BLC5),
		[ISP_BLK_ID_BLC6]		= (ISP_BLK_BA_BLC6),
		[ISP_BLK_ID_AF] 		= (ISP_BLK_BA_AF),
		[ISP_BLK_ID_DMA_CTL21]		= (ISP_BLK_BA_DMA_CTL21),
		[ISP_BLK_ID_DPC0]		= (ISP_BLK_BA_DPC0),
		[ISP_BLK_ID_DPC1]		= (ISP_BLK_BA_DPC1),
		[ISP_BLK_ID_DMA_CTL22]		= (ISP_BLK_BA_DMA_CTL22),
		[ISP_BLK_ID_DMA_CTL23]		= (ISP_BLK_BA_DMA_CTL23),
		[ISP_BLK_ID_PRE_WDMA]		= (ISP_BLK_BA_PRE_WDMA),
		[ISP_BLK_ID_PCHK0]		= (ISP_BLK_BA_PCHK0),
		[ISP_BLK_ID_PCHK1]		= (ISP_BLK_BA_PCHK1),
		[ISP_BLK_ID_RAWTOP] 		= (ISP_BLK_BA_RAWTOP),
		[ISP_BLK_ID_CFA]		= (ISP_BLK_BA_CFA),
		[ISP_BLK_ID_LSC]		= (ISP_BLK_BA_LSC),
		[ISP_BLK_ID_DMA_CTL24]		= (ISP_BLK_BA_DMA_CTL24),
		[ISP_BLK_ID_GMS]		= (ISP_BLK_BA_GMS),
		[ISP_BLK_ID_DMA_CTL25]		= (ISP_BLK_BA_DMA_CTL25),
		[ISP_BLK_ID_AEHIST0]		= (ISP_BLK_BA_AEHIST0),
		[ISP_BLK_ID_DMA_CTL26]		= (ISP_BLK_BA_DMA_CTL26),
		[ISP_BLK_ID_AEHIST1]		= (ISP_BLK_BA_AEHIST1),
		[ISP_BLK_ID_DMA_CTL27]		= (ISP_BLK_BA_DMA_CTL27),
		[ISP_BLK_ID_DMA_CTL28]		= (ISP_BLK_BA_DMA_CTL28),
		[ISP_BLK_ID_DMA_CTL29]		= (ISP_BLK_BA_DMA_CTL29),
		[ISP_BLK_ID_RAW_RDMA]		= (ISP_BLK_BA_RAW_RDMA),
		[ISP_BLK_ID_BNR]		= (ISP_BLK_BA_BNR),
		[ISP_BLK_ID_CROP2]		= (ISP_BLK_BA_CROP2),
		[ISP_BLK_ID_CROP3]		= (ISP_BLK_BA_CROP3),
		[ISP_BLK_ID_LMAP0]		= (ISP_BLK_BA_LMAP0),
		[ISP_BLK_ID_DMA_CTL30]		= (ISP_BLK_BA_DMA_CTL30),
		[ISP_BLK_ID_LMAP1]		= (ISP_BLK_BA_LMAP1),
		[ISP_BLK_ID_DMA_CTL31]		= (ISP_BLK_BA_DMA_CTL31),
		[ISP_BLK_ID_WBG0]		= (ISP_BLK_BA_WBG0),
		[ISP_BLK_ID_WBG1]		= (ISP_BLK_BA_WBG1),
		[ISP_BLK_ID_PCHK2]		= (ISP_BLK_BA_PCHK2),
		[ISP_BLK_ID_PCHK3]		= (ISP_BLK_BA_PCHK3),
		[ISP_BLK_ID_LCAC]		= (ISP_BLK_BA_LCAC),
		[ISP_BLK_ID_RGBCAC] 		= (ISP_BLK_BA_RGBCAC),
		[ISP_BLK_ID_RGBTOP] 		= (ISP_BLK_BA_RGBTOP),
		[ISP_BLK_ID_CCM0]		= (ISP_BLK_BA_CCM0),
		[ISP_BLK_ID_CCM1]		= (ISP_BLK_BA_CCM1),
		[ISP_BLK_ID_RGBGAMMA]		= (ISP_BLK_BA_RGBGAMMA),
		[ISP_BLK_ID_YGAMMA] 		= (ISP_BLK_BA_YGAMMA),
		[ISP_BLK_ID_MMAP]		= (ISP_BLK_BA_MMAP),
		[ISP_BLK_ID_DMA_CTL32]		= (ISP_BLK_BA_DMA_CTL32),
		[ISP_BLK_ID_DMA_CTL33]		= (ISP_BLK_BA_DMA_CTL33),
		[ISP_BLK_ID_DMA_CTL34]		= (ISP_BLK_BA_DMA_CTL34),
		[ISP_BLK_ID_DMA_CTL35]		= (ISP_BLK_BA_DMA_CTL35),
		[ISP_BLK_ID_DMA_CTL36]		= (ISP_BLK_BA_DMA_CTL36),
		[ISP_BLK_ID_DMA_CTL37]		= (ISP_BLK_BA_DMA_CTL37),
		[ISP_BLK_ID_CLUT]		= (ISP_BLK_BA_CLUT),
		[ISP_BLK_ID_DHZ]		= (ISP_BLK_BA_DHZ),
		[ISP_BLK_ID_CSC]		= (ISP_BLK_BA_CSC),
		[ISP_BLK_ID_RGBDITHER]		= (ISP_BLK_BA_RGBDITHER),
		[ISP_BLK_ID_PCHK4]		= (ISP_BLK_BA_PCHK4),
		[ISP_BLK_ID_PCHK5]		= (ISP_BLK_BA_PCHK5),
		[ISP_BLK_ID_HIST_V] 		= (ISP_BLK_BA_HIST_V),
		[ISP_BLK_ID_DMA_CTL38]		= (ISP_BLK_BA_DMA_CTL38),
		[ISP_BLK_ID_HDRFUSION]		= (ISP_BLK_BA_HDRFUSION),
		[ISP_BLK_ID_HDRLTM] 		= (ISP_BLK_BA_HDRLTM),
		[ISP_BLK_ID_DMA_CTL39]		= (ISP_BLK_BA_DMA_CTL39),
		[ISP_BLK_ID_DMA_CTL40]		= (ISP_BLK_BA_DMA_CTL40),
		[ISP_BLK_ID_YUVTOP] 		= (ISP_BLK_BA_YUVTOP),
		[ISP_BLK_ID_TNR]		= (ISP_BLK_BA_TNR),
		[ISP_BLK_ID_DMA_CTL41]		= (ISP_BLK_BA_DMA_CTL41),
		[ISP_BLK_ID_DMA_CTL42]		= (ISP_BLK_BA_DMA_CTL42),
		[ISP_BLK_ID_FBCE]		= (ISP_BLK_BA_FBCE),
		[ISP_BLK_ID_DMA_CTL43]		= (ISP_BLK_BA_DMA_CTL43),
		[ISP_BLK_ID_DMA_CTL44]		= (ISP_BLK_BA_DMA_CTL44),
		[ISP_BLK_ID_FBCD]		= (ISP_BLK_BA_FBCD),
		[ISP_BLK_ID_YUVDITHER]		= (ISP_BLK_BA_YUVDITHER),
		[ISP_BLK_ID_CA] 		= (ISP_BLK_BA_CA),
		[ISP_BLK_ID_CA_LITE]		= (ISP_BLK_BA_CA_LITE),
		[ISP_BLK_ID_YNR]		= (ISP_BLK_BA_YNR),
		[ISP_BLK_ID_CNR]		= (ISP_BLK_BA_CNR),
		[ISP_BLK_ID_EE] 		= (ISP_BLK_BA_EE),
		[ISP_BLK_ID_YCURVE] 		= (ISP_BLK_BA_YCURVE),
		[ISP_BLK_ID_DCI]		= (ISP_BLK_BA_DCI),
		[ISP_BLK_ID_DMA_CTL45]		= (ISP_BLK_BA_DMA_CTL45),
		[ISP_BLK_ID_DCI_GAMMA]		= (ISP_BLK_BA_DCI_GAMMA),
		[ISP_BLK_ID_CROP4]		= (ISP_BLK_BA_CROP4),
		[ISP_BLK_ID_DMA_CTL46]		= (ISP_BLK_BA_DMA_CTL46),
		[ISP_BLK_ID_CROP5]		= (ISP_BLK_BA_CROP5),
		[ISP_BLK_ID_DMA_CTL47]		= (ISP_BLK_BA_DMA_CTL47),
		[ISP_BLK_ID_LDCI]		= (ISP_BLK_BA_LDCI),
		[ISP_BLK_ID_DMA_CTL48]		= (ISP_BLK_BA_DMA_CTL48),
		[ISP_BLK_ID_DMA_CTL49]		= (ISP_BLK_BA_DMA_CTL49),
		[ISP_BLK_ID_PRE_EE] 		= (ISP_BLK_BA_PRE_EE),
		[ISP_BLK_ID_PCHK6]		= (ISP_BLK_BA_PCHK6),
		[ISP_BLK_ID_PCHK7]		= (ISP_BLK_BA_PCHK7),
		[ISP_BLK_ID_ISPTOP] 		= (ISP_BLK_BA_ISPTOP),
		[ISP_BLK_ID_WDMA_CORE0] 	= (ISP_BLK_BA_WDMA_CORE0),
		[ISP_BLK_ID_RDMA_CORE]		= (ISP_BLK_BA_RDMA_CORE),
		[ISP_BLK_ID_CSIBDG_LITE]	= (ISP_BLK_BA_CSIBDG_LITE),
		[ISP_BLK_ID_DMA_CTL0]		= (ISP_BLK_BA_DMA_CTL0),
		[ISP_BLK_ID_DMA_CTL1]		= (ISP_BLK_BA_DMA_CTL1),
		[ISP_BLK_ID_DMA_CTL2]		= (ISP_BLK_BA_DMA_CTL2),
		[ISP_BLK_ID_DMA_CTL3]		= (ISP_BLK_BA_DMA_CTL3),
		[ISP_BLK_ID_WDMA_CORE1] 	= (ISP_BLK_BA_WDMA_CORE1),
		[ISP_BLK_ID_PRE_RAW_VI_SEL] 	= (ISP_BLK_BA_PRE_RAW_VI_SEL),
		[ISP_BLK_ID_DMA_CTL4]		= (ISP_BLK_BA_DMA_CTL4),
		[ISP_BLK_ID_DMA_CTL5]		= (ISP_BLK_BA_DMA_CTL5),
		[ISP_BLK_ID_CMDQ]		= (ISP_BLK_BA_CMDQ),
	};
	return m_isp_phys_base_list;
}

void isp_debug_dump(struct isp_ctx *ctx)
{
}

void isp_intr_status(
	struct isp_ctx *ctx,
	union REG_ISP_TOP_INT_EVENT0 *s0,
	union REG_ISP_TOP_INT_EVENT1 *s1,
	union REG_ISP_TOP_INT_EVENT2 *s2)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	s0->raw = ISP_RD_REG(isp_top, REG_ISP_TOP_T, INT_EVENT0);
	//clear isp top event0 status
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT0, s0->raw);

	s1->raw = ISP_RD_REG(isp_top, REG_ISP_TOP_T, INT_EVENT1);
	//clear isp top event1 status
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT1, s1->raw);

	s2->raw = ISP_RD_REG(isp_top, REG_ISP_TOP_T, INT_EVENT2);
	//clear isp top event2 status
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT2, s2->raw);
}

void isp_csi_intr_status(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_0 *s0,
	union REG_ISP_CSI_BDG_INTERRUPT_STATUS_1 *s1)
{
	uintptr_t csi_bdg;

	if (raw_num == ISP_PRERAW_A) {
		csi_bdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0];
	} else if (raw_num == ISP_PRERAW_B) {
		csi_bdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG1];
	} else {
		csi_bdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG2];
	}

	s0->raw = ISP_RD_REG(csi_bdg, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_0);
	//clear status
	ISP_WR_REG(csi_bdg, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_0, s0->raw);

	s1->raw = ISP_RD_REG(csi_bdg, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_1);
	//clear status
	ISP_WR_REG(csi_bdg, REG_ISP_CSI_BDG_T, INTERRUPT_STATUS_1, s1->raw);
}

void isp_init(struct isp_ctx *ctx)
{
	u8 i = 0;

	for (i = 0; i < ISP_PRERAW_VIRT_MAX; i++) {
		g_w_bit[i] = MANR_W_BIT;
		g_h_bit[i] = MANR_H_BIT;
		g_rgbmap_chg_pre[i][0] = false;
		g_rgbmap_chg_pre[i][1] = false;

		g_lmp_cfg[i].pre_chg[0] = false;
		g_lmp_cfg[i].pre_chg[1] = false;
		g_lmp_cfg[i].pre_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].pre_h_bit = LUMA_MAP_H_BIT;

		g_lmp_cfg[i].post_w_bit = LUMA_MAP_W_BIT;
		g_lmp_cfg[i].post_h_bit = LUMA_MAP_H_BIT;
	}
}

void isp_streaming(struct isp_ctx *ctx, uint32_t on, enum cvi_isp_raw raw_num)
{
	uintptr_t csibdg;
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SW_CTRL_0 sw_ctrl_0;
	union REG_ISP_TOP_SW_CTRL_1 sw_ctrl_1;
	union REG_ISP_CSI_BDG_TOP_CTRL csibdg_topctl;

	sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

	if (raw_num == ISP_PRERAW_A)
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0];
	else if (raw_num == ISP_PRERAW_B)
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG1];
	else
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG2];

	if (on) {
		if (raw_num == ISP_PRERAW_A) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->is_synthetic_hdr_on) {
				sw_ctrl_0.bits.SHAW_UP_FE0	= 3;
				sw_ctrl_1.bits.PQ_UP_FE0	= 3;
			} else {
				sw_ctrl_0.bits.SHAW_UP_FE0	= 1;
				sw_ctrl_1.bits.PQ_UP_FE0	= 1;
			}
		} else if (raw_num == ISP_PRERAW_B) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->is_synthetic_hdr_on) {
				sw_ctrl_0.bits.SHAW_UP_FE1	= 3;
				sw_ctrl_1.bits.PQ_UP_FE1	= 3;
			} else {
				sw_ctrl_0.bits.SHAW_UP_FE1	= 1;
				sw_ctrl_1.bits.PQ_UP_FE1	= 1;
			}
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && !ctx->is_synthetic_hdr_on) {
				sw_ctrl_0.bits.SHAW_UP_FE2	= 3;
				sw_ctrl_1.bits.PQ_UP_FE2	= 3;
			} else {
				sw_ctrl_0.bits.SHAW_UP_FE2	= 1;
				sw_ctrl_1.bits.PQ_UP_FE2	= 1;
			}
		}

		sw_ctrl_0.bits.SHAW_UP_BE	= 1;
		sw_ctrl_1.bits.PQ_UP_BE 	= 1;
		sw_ctrl_0.bits.SHAW_UP_RAW	= 1;
		sw_ctrl_1.bits.PQ_UP_RAW	= 1;
		sw_ctrl_0.bits.SHAW_UP_POST	= 1;
		sw_ctrl_1.bits.PQ_UP_POST	= 1;

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_0, sw_ctrl_0.raw);

		csibdg_topctl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			csibdg_topctl.bits.CSI_UP_REG = 0;
			csibdg_topctl.bits.CSI_ENABLE = 0;
			csibdg_topctl.bits.TGEN_ENABLE = 0;
		} else {
			csibdg_topctl.bits.CSI_UP_REG = 1;
			csibdg_topctl.bits.CSI_ENABLE = 1;

			if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
				csibdg_topctl.bits.TGEN_ENABLE = 1;
			else
				csibdg_topctl.bits.TGEN_ENABLE = 0;
		}

		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, csibdg_topctl.raw);
	} else {
		csibdg_topctl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
		csibdg_topctl.bits.CSI_ENABLE = 0;

		if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
			csibdg_topctl.bits.TGEN_ENABLE = 0;
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, csibdg_topctl.raw);
	}
}

void isp_pre_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t preraw0;
	u8 trig_chn_num = chn_num;
	enum cvi_isp_raw raw = raw_num;

	raw = find_hw_raw_num(raw_num);

	if (raw == ISP_PRERAW_A)
		preraw0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	else if (raw == ISP_PRERAW_B)
		preraw0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	else
		preraw0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];

	if (ctx->isp_pipe_cfg[raw_num].is_offline_preraw) { //dram->be
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union REG_ISP_TOP_SW_CTRL_0 sw_ctrl_0;
		union REG_ISP_TOP_SW_CTRL_1 sw_ctrl_1;

		sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

		if (raw == ISP_PRERAW_A) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.TRIG_STR_BE	= 0x3;
				sw_ctrl_1.bits.PQ_UP_BE		= 0x3;
				sw_ctrl_0.bits.SHAW_UP_BE	= 0x3;
				sw_ctrl_0.bits.TRIG_STR_RAW	= 0x1;
				sw_ctrl_0.bits.SHAW_UP_RAW	= 0x1;
				sw_ctrl_1.bits.PQ_UP_RAW	= 0x1;
				sw_ctrl_0.bits.TRIG_STR_POST	= 0x1;
				sw_ctrl_0.bits.SHAW_UP_POST	= 0x1;
				sw_ctrl_1.bits.PQ_UP_POST	= 0x1;
			} else {
				sw_ctrl_0.bits.TRIG_STR_BE	= 0x1;
				sw_ctrl_1.bits.PQ_UP_BE		= 0x1;
				sw_ctrl_0.bits.SHAW_UP_BE	= 0x1;
				sw_ctrl_0.bits.TRIG_STR_RAW	= 0x1;
				sw_ctrl_0.bits.SHAW_UP_RAW	= 0x1;
				sw_ctrl_1.bits.PQ_UP_RAW	= 0x1;
				sw_ctrl_0.bits.TRIG_STR_POST	= 0x1;
				sw_ctrl_0.bits.SHAW_UP_POST	= 0x1;
				sw_ctrl_1.bits.PQ_UP_POST	= 0x1;
			}
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_1, sw_ctrl_1.raw);
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_0, sw_ctrl_0.raw);

			vi_pr(VI_DBG, "Raw replay trigger fe_%d\n", raw_num);
		}
	} else { // patgen or sensor->fe
		vi_pr(VI_INFO, "trigger fe_%d chn_num_%d frame_vld\n", raw, chn_num);

		// In synthetic HDR mode, always trigger chn 0.
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on && ctx->is_synthetic_hdr_on)
			trig_chn_num = ISP_FE_CH0;

		switch (trig_chn_num) {
		case 0:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH0, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH0, 1);
			break;
		case 1:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH1, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH1, 1);
			break;
		case 2:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH2, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH2, 1);
			break;
		case 3:
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_PQ_VLD_CH3, 1);
			ISP_WR_BITS(preraw0, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH3, 1);
			break;
		default:
			break;
		}
	}
}

void isp_post_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SW_CTRL_0 sw_ctrl_0;
	union REG_ISP_TOP_SW_CTRL_1 sw_ctrl_1;

	sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		vi_pr(VI_DBG, "dram->post trig raw_num(%d), is_slice_buf_on(%d)\n",
				raw_num, ctx->is_slice_buf_on);

		sw_ctrl_0.bits.SHAW_UP_RAW	= 1;
		sw_ctrl_0.bits.TRIG_STR_RAW	= 1;
		sw_ctrl_1.bits.PQ_UP_RAW	= 1;
		sw_ctrl_0.bits.SHAW_UP_POST	= 1;
		sw_ctrl_0.bits.TRIG_STR_POST	= 1;
		sw_ctrl_1.bits.PQ_UP_POST	= 1;

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_0, sw_ctrl_0.raw);
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		vi_pr(VI_DBG, "dram->be post trig raw_num(%d), is_hdr_on(%d)\n",
				raw_num, ctx->isp_pipe_cfg[raw_num].is_hdr_on);

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.SHAW_UP_BE	= 3;
				sw_ctrl_0.bits.TRIG_STR_BE	= 3;
				sw_ctrl_1.bits.PQ_UP_BE		= 3;
				sw_ctrl_0.bits.SHAW_UP_RAW	= 1;
				sw_ctrl_0.bits.TRIG_STR_RAW	= 1;
				sw_ctrl_1.bits.PQ_UP_RAW	= 1;
				sw_ctrl_0.bits.SHAW_UP_POST	= 1;
				sw_ctrl_0.bits.TRIG_STR_POST	= 1;
				sw_ctrl_1.bits.PQ_UP_POST	= 1;
			} else {
				sw_ctrl_0.bits.SHAW_UP_BE	= 1;
				sw_ctrl_0.bits.TRIG_STR_BE	= 1;
				sw_ctrl_1.bits.PQ_UP_BE		= 1;
				sw_ctrl_0.bits.SHAW_UP_RAW	= 1;
				sw_ctrl_0.bits.TRIG_STR_RAW	= 1;
				sw_ctrl_1.bits.PQ_UP_RAW	= 1;
				sw_ctrl_0.bits.SHAW_UP_POST	= 1;
				sw_ctrl_0.bits.TRIG_STR_POST	= 1;
				sw_ctrl_1.bits.PQ_UP_POST	= 1;
			}
		} else {
			sw_ctrl_0.bits.SHAW_UP_RAW	= 1;
			sw_ctrl_0.bits.TRIG_STR_RAW	= 1;
			sw_ctrl_1.bits.PQ_UP_RAW	= 1;
			sw_ctrl_0.bits.SHAW_UP_POST	= 1;
			sw_ctrl_0.bits.TRIG_STR_POST	= 1;
			sw_ctrl_1.bits.PQ_UP_POST	= 1;
		}

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_CTRL_0, sw_ctrl_0.raw);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //slice buffer path
		vi_pr(VI_DBG, "dram->post trig raw_num(%d), is_slice_buf_on(%d)\n",
				raw_num, ctx->is_slice_buf_on);

		isp_slice_buf_trig(ctx, ISP_PRERAW_A);
	}
}

/*********************************************************************************
 *	Common IPs for subsys
 ********************************************************************************/
struct isp_grid_s_info ispblk_lmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	struct isp_grid_s_info dummy = {0};
	return dummy;
}

uint64_t ispblk_dma_getaddr(struct isp_ctx *ctx, uint32_t dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint64_t addr_h = ISP_RD_BITS(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, BASEH);

	return ((uint64_t)ISP_RD_REG(dmab, REG_ISP_DMA_CTL_T, BASE_ADDR) | (addr_h << 32));
}

int ispblk_dma_buf_get_size(struct isp_ctx *ctx, int dmaid, u8 raw_num)
{
	uint32_t len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL6: //fe0_csibdg_le
	case ISP_BLK_ID_DMA_CTL7: //fe0_csibdg_se
	case ISP_BLK_ID_DMA_CTL8: //fe1_csibdg_le
	case ISP_BLK_ID_DMA_CTL9: //fe1_csibdg_se
	case ISP_BLK_ID_DMA_CTL12: //fe1_csibdg_le
	case ISP_BLK_ID_DMA_CTL13: //fe1_csibdg_se
	case ISP_BLK_ID_DMA_CTL18: //fe2_csibdg_le
	case ISP_BLK_ID_DMA_CTL19: //fe2_csibdg_se
	{
		/* csibdg */
		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);

		break;
	}
	case ISP_BLK_ID_DMA_CTL10: //fe0_rgbmap_le
	case ISP_BLK_ID_DMA_CTL11: //fe0_rgbmap_se
	case ISP_BLK_ID_DMA_CTL16: //fe1_rgbmap_le
	case ISP_BLK_ID_DMA_CTL17: //fe1_rgbmap_se
	case ISP_BLK_ID_DMA_CTL20: //fe2_rgbmap_le
	{ //rgbmap max size
		if (ctx->is_rgbmap_sbm_on) {
			if (dmaid == ISP_BLK_ID_DMA_CTL10)
				len = slc_b_cfg.sub_path.le_buf_size;
			else if (dmaid == ISP_BLK_ID_DMA_CTL11)
				len = slc_b_cfg.sub_path.se_buf_size;
			num = 1;
		} else {
			uint8_t grid_size = RGBMAP_MAX_BIT;

			len = (((UPPER(ctx->isp_pipe_cfg[raw_num].crop.w, grid_size)) * 6 + 15) >> 4) << 4;
			num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, grid_size);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL22: //pre_be_wdma_le
	case ISP_BLK_ID_DMA_CTL23: //pre_be_wdma_se
	{
		if (ctx->is_slice_buf_on) {
			if (dmaid == ISP_BLK_ID_DMA_CTL22)
				len = slc_b_cfg.main_path.le_buf_size;
			else
				len = slc_b_cfg.main_path.se_buf_size;
			num = 1;
		} else {
			u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;
			u32 w = 0;

			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
				w = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_width;
				num = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_height;
				len = 3 * UPPER(w, 1);
			} else {
				w = ctx->img_width;
				num = ctx->img_height;
				len = 3 * UPPER(w, dpcm_on);
			}
		}
		break;
	}
	case ISP_BLK_ID_DMA_CTL21:
	{
		/* af */
		uint16_t block_num_x = 17, block_num_y = 15;

		len = (block_num_x * block_num_y) << 5;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL24: //lsc
	{
		/* lsc rdma */
		// fixed value for 37x37

		num = 0xde;
		len = 0x40;

		break;
	}
	case ISP_BLK_ID_DMA_CTL25:
	{
		/* gms */
		u32 sec_size = 1023;

		len = (((sec_size + 1) >> 1) << 5) * 3;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL26: //rawtop_aehist_le
	case ISP_BLK_ID_DMA_CTL27: //rawtop_aehist_se
	{
		int ae_dma_counts, hist_dma_counts, faceae_dma_counts;

		ae_dma_counts		= 0x21C0;
		hist_dma_counts		= 0x2000;
		faceae_dma_counts	= 0x80;

		len = ae_dma_counts + hist_dma_counts + faceae_dma_counts;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL30: //rawtop_lmap_le
	case ISP_BLK_ID_DMA_CTL31: //rawtop_lmap_se
	{ //lmap max size
		len = (((UPPER(ctx->isp_pipe_cfg[raw_num].crop.w, 3))  + 15) >> 4) << 4;
		num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, 3);

		break;
	}
	case ISP_BLK_ID_DMA_CTL36: //manr_iir_r
	case ISP_BLK_ID_DMA_CTL37: //manr_iir_w
	{
		/* manr rdma */
		uint8_t grid_size = RGBMAP_MAX_BIT;

		len = (((UPPER(ctx->isp_pipe_cfg[raw_num].crop.w, grid_size) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, grid_size);

		break;
	}
	case ISP_BLK_ID_DMA_CTL38: //hist_v
	{
		len = 2048;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL42:
	case ISP_BLK_ID_DMA_CTL44:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.c_buf_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[raw_num].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL41:
	case ISP_BLK_ID_DMA_CTL43:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.y_buf_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[raw_num].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[raw_num].crop.h;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL45:
	{
		// dci
		len = 0x200;
		num = 0x1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL48: //ldci_iir_w
	case ISP_BLK_ID_DMA_CTL49: //ldci_iir_r
	{
		len = 0x300;
		num = 0x1;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_INFO, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

void ispblk_dma_setaddr(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, BASE_ADDR, (buf_addr & 0xFFFFFFFF));
	ISP_WR_BITS(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, BASEH, ((buf_addr >> 32) & 0xFFFFFFFF));
}

void ispblk_dma_set_sw_mode(struct isp_ctx *ctx, uint32_t dmaid, bool is_sw_mode)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union REG_ISP_DMA_CTL_SYS_CONTROL sys_ctrl;

#if defined( __SOC_PHOBOS__)
	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL5://be_se_rdma_ctl
	case ISP_BLK_ID_DMA_CTL11://fe0 rgbmap SE
	case ISP_BLK_ID_DMA_CTL16://fe1 rgbmap LE
	case ISP_BLK_ID_DMA_CTL17://fe1 rgbmap SE
	case ISP_BLK_ID_DMA_CTL20://fe2 rgbmap LE
	case ISP_BLK_ID_DMA_CTL23://be_se_wdma_ctl
	case ISP_BLK_ID_DMA_CTL31://lmap SE
	case ISP_BLK_ID_DMA_CTL8://fe0 csi2/fe1 ch0
	case ISP_BLK_ID_DMA_CTL9://fe0 csi3/fe1 ch1
	case ISP_BLK_ID_DMA_CTL27://aehist1
	case ISP_BLK_ID_DMA_CTL29://raw crop SE
	case ISP_BLK_ID_DMA_CTL33://MANR_P_SE
	case ISP_BLK_ID_DMA_CTL35://MANR_C_SE
		return;
	default:
		break;
	}
#endif

	//SW mode: config by SW
	sys_ctrl.raw = ISP_RD_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL);
	sys_ctrl.bits.BASE_SEL		= 0x1;
	sys_ctrl.bits.STRIDE_SEL	= is_sw_mode;
	sys_ctrl.bits.SEGLEN_SEL	= is_sw_mode;
	sys_ctrl.bits.SEGNUM_SEL	= is_sw_mode;
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, sys_ctrl.raw);
}

void ispblk_rgbmap_dma_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, int dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 grid_size = (1 << g_w_bit[raw_num]);
	u32 w = ctx->isp_pipe_cfg[raw_num].crop.w, stride = 0, seglen = 0;
	u32 num = UPPER(ctx->isp_pipe_cfg[raw_num].crop.h, g_w_bit[raw_num]);

	stride = ((((w + grid_size - 1) / grid_size) * 6 + 15) / 16) * 16;
	seglen = ((w + grid_size - 1) / grid_size) * 6;

	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_STRIDE, stride);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGLEN, seglen);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGNUM, num);

	vi_pr(VI_DBG, "len=%d stride=%d num=%d\n", seglen, stride, num);
}

void ispblk_mmap_dma_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, int dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 grid_size = (1 << g_w_bit[raw_num]);
	u32 w = ctx->isp_pipe_cfg[raw_num].crop.w, stride = 0;

	stride = ((((w + grid_size - 1) / grid_size) * 6 + 15) / 16) * 16;

	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_STRIDE, stride);
}

int ispblk_dma_config(struct isp_ctx *ctx, int dmaid, enum cvi_isp_raw raw_num, uint64_t buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint32_t w = 0, len = 0, stride = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL4: //pre_be_rdma_le
	case ISP_BLK_ID_DMA_CTL5: //pre_be_rdma_se
	case ISP_BLK_ID_DMA_CTL22: //pre_be_wdma_le
	case ISP_BLK_ID_DMA_CTL23: //pre_be_wdma_se
	{
		// preraw be read/write dma
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
			w = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_width;
			num = ctx->isp_pipe_cfg[ISP_PRERAW_A].csibdg_height;
			len = 3 * UPPER(w, 1);
		} else {
			w = ctx->img_width;
			num = ctx->img_height;
			len = 3 * UPPER(w, dpcm_on);
		}
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL6: //fe0_csibdg_le
	case ISP_BLK_ID_DMA_CTL7: //fe0_csibdg_se
	case ISP_BLK_ID_DMA_CTL8: //fe1_csibdg_le
	case ISP_BLK_ID_DMA_CTL9: //fe1_csibdg_se
	case ISP_BLK_ID_DMA_CTL18: //fe2_csibdg_le
	case ISP_BLK_ID_DMA_CTL19: //fe2_csibdg_se
		/* csibdg */
		w = ctx->isp_pipe_cfg[raw_num].crop.w;
		num = ctx->isp_pipe_cfg[raw_num].crop.h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);
		stride = len;

		break;
	case ISP_BLK_ID_DMA_CTL21:
	{
		/* af */
		uint16_t block_num_x = 17, block_num_y = 15;

		num = 1;
		len = (block_num_x * block_num_y) << 5;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL24: //lsc
	{
		/* lsc rdma */
		// fixed value for 37x37

		num = 0xde;
		len = 0x40;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL25:
	{
		/* gms */
		uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_GMS];

		u32 x_sec_size = ISP_RD_REG(sts, REG_ISP_GMS_T, GMS_X_SIZEM1);
		u32 y_sec_size = ISP_RD_REG(sts, REG_ISP_GMS_T, GMS_Y_SIZEM1);
		u32 sec_size = (x_sec_size >= y_sec_size) ? x_sec_size : y_sec_size;

		num = 1;
		len = (((sec_size + 1) >> 1) << 5) * 3;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL26: //rawtop_aehist_le
	case ISP_BLK_ID_DMA_CTL27: //rawtop_aehist_se
	{
		int ae_dma_counts, hist_dma_counts, faceae_dma_counts;

		ae_dma_counts		= 0x21C0;
		hist_dma_counts		= 0x2000;
		faceae_dma_counts	= 0x80;

		num = 1;
		len = ae_dma_counts + hist_dma_counts + faceae_dma_counts;
		stride = len;

		break;
	}

	case ISP_BLK_ID_DMA_CTL28: //rawtop_rdma_le
	case ISP_BLK_ID_DMA_CTL29: //rawtop_rdma_se
	{
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		w = ctx->img_width;
		num = ctx->img_height;
		len = 3 * UPPER(w, dpcm_on);
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL32: //manr_prv_le
	case ISP_BLK_ID_DMA_CTL33: //manr_prv_se
	case ISP_BLK_ID_DMA_CTL34: //manr_cur_le
	case ISP_BLK_ID_DMA_CTL35: //manr_cur_se
	{
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MMAP];

		u16 w_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT);
		u16 h_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT);

		len = ((UPPER(ctx->img_width, w_bit) * 48 + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);

		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL36: //manr_iir_r
	case ISP_BLK_ID_DMA_CTL37: //manr_iir_w
	{
		/* manr rdma */
		uintptr_t blk = ctx->phys_regs[ISP_BLK_ID_MMAP];

		u16 w_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_W_BIT);
		u16 h_bit = ISP_RD_BITS(blk, REG_ISP_MMAP_T, REG_60, RGBMAP_H_BIT);

		len = (((UPPER(ctx->img_width, w_bit) << 4) + 127) >> 7) << 4;
		num = UPPER(ctx->img_height, h_bit);
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL42:
	case ISP_BLK_ID_DMA_CTL44:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.c_bs_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[raw_num].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[raw_num].crop.h >> 1;
		}
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL41:
	case ISP_BLK_ID_DMA_CTL43:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, raw_num);
			len = fbc_cfg.y_bs_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[raw_num].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[raw_num].crop.h;
		}
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL45:
	{
		// dci
		len = 0x200;
		num = 0x1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL46:
		//yuvtop y out
		len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.h : ctx->img_height;
		stride = len;

		break;
	case ISP_BLK_ID_DMA_CTL47:
		//yuvtop uv out
		len = (ctx->isp_pipe_cfg[raw_num].postout_crop.w) ?
			ctx->isp_pipe_cfg[raw_num].postout_crop.w : ctx->img_width;
		num = (ctx->isp_pipe_cfg[raw_num].postout_crop.h) ?
			(ctx->isp_pipe_cfg[raw_num].postout_crop.h >> 1) : (ctx->img_height >> 1);
		stride = len;

		break;
	case ISP_BLK_ID_DMA_CTL48: //ldci_iir_w
	case ISP_BLK_ID_DMA_CTL49: //ldci_iir_r
	{
		len = 0x300;
		num = 0x1;
		stride = len;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);
	stride = VI_ALIGN(stride);

	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGLEN, len);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_STRIDE, stride);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGNUM, num);

	vi_pr(VI_DBG, "dmaid=%d, len=%d, stride=%d, num=%d\n", dmaid, len, stride, num);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	return len * num;
}

void ispblk_dma_enable(struct isp_ctx *ctx, uint32_t dmaid, uint32_t on, uint8_t dma_disable)
{
	uintptr_t srcb = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL46:
		/* yuvtop y crop4 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP4];
		break;
	case ISP_BLK_ID_DMA_CTL47:
		/* yuvtop uv crop5 */
		srcb = ctx->phys_regs[ISP_BLK_ID_CROP5];
		break;

	default:
		break;
	}

	if (srcb) {
		ISP_WR_BITS(srcb, REG_CROP_T, REG_0, DMA_ENABLE, on);
		ISP_WR_BITS(srcb, REG_CROP_T, DEBUG, FORCE_DMA_DISABLE, dma_disable);
	}
}

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];

	ISP_WR_BITS(cropb, REG_CROP_T, REG_0, CROP_ENABLE, en);
}

int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vi_rect crop)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];
	union REG_CROP_1 reg1;
	union REG_CROP_2 reg2;

	// crop out size
	reg1.bits.CROP_START_Y = crop.y;
	reg1.bits.CROP_END_Y = crop.y + crop.h - 1;
	reg2.bits.CROP_START_X = crop.x;
	reg2.bits.CROP_END_X = crop.x + crop.w - 1;
	ISP_WR_REG(cropb, REG_CROP_T, REG_1, reg1.raw);
	ISP_WR_REG(cropb, REG_CROP_T, REG_2, reg2.raw);
	ISP_WR_BITS(cropb, REG_CROP_T, REG_0, CROP_ENABLE, true);

	return 0;
}

enum cvi_isp_raw find_hw_raw_num(enum cvi_isp_raw raw_num)
{
	return raw_num > ISP_PRERAW_MAX - 1 ? raw_num - ISP_PRERAW_MAX : raw_num;
}

int find_ac_chn_num(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	int chn_num = raw_num;
	enum cvi_isp_raw raw;

	if (raw_num > ISP_PRERAW_MAX - 1) {
		chn_num = 0;
		for (raw = ISP_PRERAW_A; raw < raw_num; raw++) {
			if (!ctx->isp_pipe_enable[raw])
				continue;
			if (!ctx->isp_pipe_cfg[raw].is_yuv_bypass_path) //RGB sensor
				chn_num++;
			else //YUV sensor
				chn_num += ctx->isp_pipe_cfg[raw].muxMode + 1;
		}
	}

	return chn_num;
}

enum cvi_isp_raw find_raw_num_by_chn(struct isp_ctx *ctx, int chn_num)
{
	enum cvi_isp_raw raw_num;
	int total_chn_num = 0;

	for (raw_num = ISP_PRERAW_A; raw_num < ISP_PRERAW_VIRT_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) //RGB sensor
			total_chn_num++;
		else //YUV sensor
			total_chn_num += ctx->isp_pipe_cfg[raw_num].muxMode + 1;
		if (total_chn_num >= chn_num)
			break;
	}

	return raw_num;
}

int ccm_find_hwid(int id)
{
	int ccm_id = -1;

	switch (id) {
	case ISP_CCM_ID_0:
		ccm_id = ISP_BLK_ID_CCM0;
		break;
#if !defined( __SOC_PHOBOS__)
	case ISP_CCM_ID_1:
		ccm_id = ISP_BLK_ID_CCM1;
		break;
#endif
	default:
		break;
	}

	return ccm_id;
}

int blc_find_hwid(int id)
{
	int blc_id = -1;

	switch (id) {
	case ISP_BLC_ID_FE0_LE:
		blc_id = ISP_BLK_ID_BLC0;
		break;
#if !defined( __SOC_PHOBOS__)
	case ISP_BLC_ID_FE0_SE:
		blc_id = ISP_BLK_ID_BLC1;
		break;
	case ISP_BLC_ID_FE1_LE:
		blc_id = ISP_BLK_ID_BLC2;
		break;
	case ISP_BLC_ID_FE1_SE:
		blc_id = ISP_BLK_ID_BLC3;
		break;
	case ISP_BLC_ID_FE2_LE:
		blc_id = ISP_BLK_ID_BLC4;
		break;
#endif
	case ISP_BLC_ID_BE_LE:
		blc_id = ISP_BLK_ID_BLC5;
		break;
#if !defined( __SOC_PHOBOS__)
	case ISP_BLC_ID_BE_SE:
		blc_id = ISP_BLK_ID_BLC6;
		break;
#endif
	default:
		break;
	}

	return blc_id;
}

void ispblk_blc_set_offset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, boffset);
}

void ispblk_blc_set_2ndoffset(struct isp_ctx *ctx, int blc_id,
				uint16_t roffset, uint16_t groffset,
				uint16_t gboffset, uint16_t boffset)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, roffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, groffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, gboffset);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, boffset);
}

void ispblk_blc_set_gain(struct isp_ctx *ctx, int blc_id,
				uint16_t rgain, uint16_t grgain,
				uint16_t gbgain, uint16_t bgain)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;
	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, rgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, grgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, gbgain);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, bgain);
}

void ispblk_blc_enable(struct isp_ctx *ctx, int blc_id, bool en, bool bypass)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, en);
}

int ispblk_blc_config(struct isp_ctx *ctx, uint32_t blc_id, bool en, bool bypass)
{
	int id = blc_find_hwid(blc_id);
	uintptr_t blc;

	if (id < 0)
		return -1;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_0, BLC_BYPASS, bypass);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_2, BLC_ENABLE, en);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_R, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_3, BLC_OFFSET_GR, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_GB, 511);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_4, BLC_OFFSET_B, 511);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_R, 0x40f);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_5, BLC_GAIN_GR, 0x419);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_GB, 0x419);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_6, BLC_GAIN_B, 0x405);

	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_R, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_9, BLC_2NDOFFSET_GR, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_GB, 0);
	ISP_WR_BITS(blc, REG_ISP_BLC_T, BLC_A, BLC_2NDOFFSET_B, 0);

	return 0;
}

int wbg_find_hwid(int id)
{
	int wbg_id = -1;

	switch (id) {
	case ISP_WBG_ID_FE0_RGBMAP_LE:
		wbg_id = ISP_BLK_ID_WBG2;
		break;
#if !defined( __SOC_PHOBOS__)
	case ISP_WBG_ID_FE0_RGBMAP_SE:
		wbg_id = ISP_BLK_ID_WBG3;
		break;
	case ISP_WBG_ID_FE1_RGBMAP_LE:
		wbg_id = ISP_BLK_ID_WBG4;
		break;
	case ISP_WBG_ID_FE1_RGBMAP_SE:
		wbg_id = ISP_BLK_ID_WBG5;
		break;
	case ISP_WBG_ID_FE2_RGBMAP_LE:
		wbg_id = ISP_BLK_ID_WBG6;
		break;
#endif
	case ISP_WBG_ID_RAW_TOP_LE:
		wbg_id = ISP_BLK_ID_WBG0;
		break;
#if !defined( __SOC_PHOBOS__)
	case ISP_WBG_ID_RAW_TOP_SE:
		wbg_id = ISP_BLK_ID_WBG1;
		break;
#endif
	default:
		break;
	}
	return wbg_id;
}

int ispblk_wbg_config(struct isp_ctx *ctx, int wbg_id, uint16_t rgain, uint16_t ggain, uint16_t bgain)
{
	int id = wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_4, WBG_RGAIN, rgain);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_4, WBG_GGAIN, ggain);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_5, WBG_BGAIN, bgain);

	return 0;
}

int ispblk_wbg_enable(struct isp_ctx *ctx, int wbg_id, bool enable, bool bypass)
{
	int id = wbg_find_hwid(wbg_id);
	uintptr_t wbg;

	if (id < 0)
		return -EINVAL;

	wbg = ctx->phys_regs[id];
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_0, WBG_BYPASS, bypass);
	ISP_WR_BITS(wbg, REG_ISP_WBG_T, WBG_2, WBG_ENABLE, enable);

	return 0;
}

/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void isp_first_frm_reset(struct isp_ctx *ctx, uint8_t reset)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	//0: reg_first_frame_reset(in IP)
	//1: reg_first_frame_sw
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, FIRST_FRAME, FIRST_FRAME_TOP, 1);

	//reg_first_frame_sw
	//[0]: LTM
	//[1]: LDCI
	//[2]: TNR
	//[3]: MMAP
	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, FIRST_FRAME, FIRST_FRAME_SW, reset ? 0xF : 0x0);
}

void _ispblk_isptop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;

	scene_ctrl.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL);

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
			scene_ctrl.bits.RAW2YUV_422_ENABLE = 1;
			scene_ctrl.bits.DCI_RGB0YUV1 = 1;
			scene_ctrl.bits.HDR_ENABLE = 0;
		} else { //RGB sensor
			scene_ctrl.bits.RAW2YUV_422_ENABLE = 0;
			scene_ctrl.bits.DCI_RGB0YUV1 = 0;
			scene_ctrl.bits.HDR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		}
	} else if (_is_be_post_online(ctx)) {
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
			scene_ctrl.bits.RAW2YUV_422_ENABLE = 1;
			scene_ctrl.bits.DCI_RGB0YUV1 = 1;
			scene_ctrl.bits.HDR_ENABLE = 0;

			scene_ctrl.bits.BE2RAW_L_ENABLE = 0;
			scene_ctrl.bits.BE2RAW_S_ENABLE = 0;
			scene_ctrl.bits.BE_RDMA_L_ENABLE = 0;
			scene_ctrl.bits.BE_RDMA_S_ENABLE = 0;
		} else { //RGB sensor
			scene_ctrl.bits.RAW2YUV_422_ENABLE = 0;
			scene_ctrl.bits.DCI_RGB0YUV1 = 0;
			scene_ctrl.bits.HDR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;

			scene_ctrl.bits.BE2RAW_L_ENABLE = 1;
			scene_ctrl.bits.BE2RAW_S_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
			scene_ctrl.bits.BE_RDMA_L_ENABLE = 1;
			scene_ctrl.bits.BE_RDMA_S_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		}
	}

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);
}

void _ispblk_be_yuv_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	uintptr_t af = ctx->phys_regs[ISP_BLK_ID_AF];

	if (!_is_be_post_online(ctx))
		return;

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable af dma
		ISP_WR_BITS(af, REG_ISP_AF_T, KICKOFF, AF_ENABLE, 0);
		ISP_WR_BITS(af, REG_ISP_AF_T, DMI_ENABLE, DMI_ENABLE, 0);
		//dpcm off
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 0);
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 0);
	} else { //RGB sensor
		if (ctx->is_dpcm_on) {
			ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 7);
			ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 8191);
		}
	}

	ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_1, FRAME_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_1, FRAME_HEIGHTM1, ctx->img_height - 1);

	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, IMG_SIZE_LE, FRAME_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, IMG_SIZE_LE, FRAME_HEIGHTM1, ctx->img_height - 1);
}

void _ispblk_rawtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t raw_rdma = ctx->phys_regs[ISP_BLK_ID_RAW_RDMA];
	uintptr_t lsc = ctx->phys_regs[ISP_BLK_ID_LSC];
	uintptr_t hist_v = ctx->phys_regs[ISP_BLK_ID_HIST_V];
	uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
	union REG_RAW_TOP_PATGEN1 patgen1;
#endif
	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable lsc
		ISP_WR_BITS(lsc, REG_ISP_LSC_T, LSC_ENABLE, LSC_ENABLE, 0);
		ISP_WR_BITS(lsc, REG_ISP_LSC_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable hist_v dma
		ISP_WR_BITS(hist_v, REG_ISP_HIST_EDGE_V_T, DMI_ENABLE, DMI_ENABLE, 0);

		//Disable ltm dma
		ISP_WR_BITS(ltm, REG_LTM_T, REG_H00, LTM_DARK_ENH_ENABLE, 0);
		ISP_WR_BITS(ltm, REG_LTM_T, REG_H00, LTM_BRIT_ENH_ENABLE, 0);

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, CH_NUM, 0);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);

		if (_is_be_post_online(ctx)) {
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, CONFIG, LE_RDMA_EN, 1);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, CONFIG, SE_RDMA_EN, 0);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, RDMA_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	} else { //RGB sensor
		if (_is_be_post_online(ctx)) //fe->dram->be->post
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 0);
		else if (_is_fe_be_online(ctx)) {//fe->be->dram->post
#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
			patgen1.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, PATGEN1);
			patgen1.bits.PG_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on ? 0 : ctx->is_slice_buf_on;
			ISP_WR_REG(rawtop, REG_RAW_TOP_T, PATGEN1, patgen1.raw);
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, CH_NUM,
				ctx->isp_pipe_cfg[raw_num].is_hdr_on ? 1 : ctx->is_slice_buf_on);
#else
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, CH_NUM, ctx->isp_pipe_cfg[raw_num].is_hdr_on);
#endif
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 1);

			if (ctx->is_dpcm_on) {
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 8191);
			} else {
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
				ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
			}
		}

		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMA_SIZE, RDMI_HEIGHTM1, ctx->img_height - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_WIDTHM1, ctx->img_width - 1);
	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_2, IMG_HEIGHTM1, ctx->img_height - 1);
}

void _ispblk_rgbtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable manr
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE, 0xff);
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_00, BYPASS, 1);
		ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, 0);
	} else { //RGB sensor
		if (ctx->is_3dnr_on) {
			//Enable manr
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_6C, FORCE_DMA_DISABLE,
					(ctx->isp_pipe_cfg[raw_num].is_hdr_on) ? 0xa0 : 0x0a);
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_00, BYPASS, 0);
			ISP_WR_BITS(manr, REG_ISP_MMAP_T, REG_D0, CROP_ENABLE_SCALAR, 1);
		}
	}

	ISP_WR_BITS(rgbtop, REG_ISP_RGB_TOP_T, REG_9, RGBTOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_TOP_T, REG_9, RGBTOP_IMGH_M1, ctx->img_height - 1);
}

void _ispblk_yuvtop_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUVDITHER];
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	uintptr_t pre_ee = ctx->phys_regs[ISP_BLK_ID_PRE_EE];
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_EE];
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //YUV sensor
		//Disable 3DNR dma
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_8, FORCE_DMA_DISABLE, 0x3f);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, REG_422_444, 1);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, TDNR_ENABLE, 0);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, FORCE_MONO_ENABLE, 0);
		//Because the yuv sensor don't pass the 444->422 module
		//Therefore the format output from 3dnr is yvu, need to swap to yuv
		//workaround, hw yuvtop's bug, only support yuyv and yvyu
		//for uyvy and vyuy, should set csi_ctrl_top's csi_format_frc[1] and csi_format_set[raw_16]
		if (ctx->isp_pipe_cfg[raw_num].enDataSeq == VI_DATA_SEQ_UYVY ||
			ctx->isp_pipe_cfg[raw_num].enDataSeq == VI_DATA_SEQ_YUYV)
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, SWAP, 3);
		else
			ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, SWAP, 0);

		//Disable yuv dither
		ISP_WR_BITS(dither, REG_ISP_YUV_DITHER_T, Y_DITHER, Y_DITHER_ENABLE, 0);
		ISP_WR_BITS(dither, REG_ISP_YUV_DITHER_T, UV_DITHER, UV_DITHER_ENABLE, 0);
		//Disable cnr
		ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_ENABLE, CNR_ENABLE, 0);
		ISP_WR_BITS(cnr, REG_ISP_CNR_T, CNR_ENABLE, PFC_ENABLE, 0);
		//Disable ynr
		ISP_WR_REG(ynr, REG_ISP_YNR_T, OUT_SEL, ISP_YNR_OUT_Y_DELAY);
		//Disable pre_ee
		ISP_WR_BITS(pre_ee, REG_ISP_EE_T, REG_00, EE_ENABLE, 0);
		//Disable ee
		ISP_WR_BITS(ee, REG_ISP_EE_T, REG_00, EE_ENABLE, 0);
		//Disable ycurv
		ISP_WR_BITS(ycur, REG_ISP_YCURV_T, YCUR_CTRL, YCUR_ENABLE, 0);
		//Disable dci
		ISP_WR_BITS(dci, REG_ISP_DCI_T, DCI_ENABLE, DCI_ENABLE, 0);
		//Disable ldci
		ISP_WR_BITS(ldci, REG_ISP_LDCI_T, LDCI_ENABLE, LDCI_ENABLE, 0);
		ISP_WR_BITS(ldci, REG_ISP_LDCI_T, DMI_ENABLE, DMI_ENABLE, 0);
		//Disable cacp
		ISP_WR_BITS(cacp, REG_CA_T, REG_00, CACP_ENABLE, 0);
		//Disable ca_lite
		ISP_WR_BITS(ca_lite, REG_CA_LITE_T, REG_00, CA_LITE_ENABLE, 0);

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, true, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, true, false);
		} else {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL46, false, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL47, false, false);
		}
	} else { //RGB sensor
		//Enable 3DNR dma
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_8, FORCE_DMA_DISABLE, 0);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, REG_422_444, 0);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_5, TDNR_ENABLE, 1);
		ISP_WR_BITS(tnr, REG_ISP_444_422_T, REG_4, SWAP, 0);
	}

	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGW_M1, ctx->img_width - 1);
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, IMGW_M1, YUV_TOP_IMGH_M1, ctx->img_height - 1);

	//bypass_v = 1 -> 422P online to scaler
	ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_CTRL, BYPASS_V, !ctx->isp_pipe_cfg[raw_num].is_offline_scaler);
}

void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	_ispblk_isptop_cfg_update(ctx, raw_num);
	// _ispblk_be_yuv_cfg_update(ctx, raw_num);
	_ispblk_rawtop_cfg_update(ctx, raw_num);
	_ispblk_rgbtop_cfg_update(ctx, raw_num);
	_ispblk_yuvtop_cfg_update(ctx, raw_num);
}

void ispblk_post_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	ispblk_raw_rdma_ctrl_config(ctx, raw_num);
	ispblk_rawtop_config(ctx, raw_num);
	ispblk_rgbtop_config(ctx, raw_num);
	ispblk_yuvtop_config(ctx, raw_num);
#if 0
	_ispblk_lsc_cfg_update(ctx, raw_num);
#endif
	//LTM grid_size update
	{
		uintptr_t ltm = ctx->phys_regs[ISP_BLK_ID_HDRLTM];
		union REG_LTM_H8C reg_8c;

		reg_8c.raw = ISP_RD_REG(ltm, REG_LTM_T, REG_H8C);
		reg_8c.bits.LMAP_W_BIT = g_lmp_cfg[raw_num].post_w_bit;
		reg_8c.bits.LMAP_H_BIT = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(ltm, REG_LTM_T, REG_H8C, reg_8c.raw);
	}
}

int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, uint32_t _w, uint32_t _h)
{
	uint32_t len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL6: //fe0_csibdg_le
	case ISP_BLK_ID_DMA_CTL7: //fe0_csibdg_se
	case ISP_BLK_ID_DMA_CTL8: //fe1_csibdg_le
	case ISP_BLK_ID_DMA_CTL9: //fe1_csibdg_se
	case ISP_BLK_ID_DMA_CTL18: //fe2_csibdg_le
	case ISP_BLK_ID_DMA_CTL19: //fe2_csibdg_se
		w = _w;
		num = _h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);

		break;
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_INFO, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

void ispblk_pre_be_cfg_update(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	union REG_PRE_RAW_VI_SEL_1 vi_sel_1;
	union REG_PRE_RAW_BE_TOP_CTRL top_ctrl;
	union REG_PRE_RAW_BE_IMG_SIZE_LE img_size;
	int numx = 17, numy = 15;

	vi_sel_1.raw = 0;
	vi_sel_1.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	vi_sel_1.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_1, vi_sel_1.raw);

	top_ctrl.raw = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL);
	top_ctrl.bits.BAYER_TYPE_LE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.BAYER_TYPE_SE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL, top_ctrl.raw);

	img_size.raw = 0;
	img_size.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	img_size.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, IMG_SIZE_LE, img_size.raw);

	// block_width >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_WIDTH, (ctx->img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, REG_ISP_AF_T, BLOCK_HEIGHT, (ctx->img_height - 4) / numy);

	ISP_WR_REG(sts, REG_ISP_AF_T, IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_WIDTH, ctx->img_width - 1);
	ISP_WR_BITS(sts, REG_ISP_AF_T, MXN_IMAGE_WIDTH_M1, AF_MXN_IMAGE_HEIGHT, ctx->img_height - 1);
}

int isp_frm_err_handler(struct isp_ctx *ctx, const enum cvi_isp_raw err_raw_num, const u8 step)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SW_RST sw_rst;
	union vip_sys_reset vip_rst;

	if (step == 1) {
		uintptr_t fe;

		if (err_raw_num == ISP_PRERAW_A)
			fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		else if (err_raw_num == ISP_PRERAW_B)
			fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
		else
			fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];

		ISP_WR_BITS(fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH0, 0);
		ISP_WR_BITS(fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_VLD, FE_FRAME_VLD_CH1, 0);
	} else if (step == 3) {
		uintptr_t csibdg;
		uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
		uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
		uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];
		u8 count = 10;

		if (err_raw_num == ISP_PRERAW_A) {
			csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0];
		} else if (err_raw_num == ISP_PRERAW_B) {
			csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG1];
		} else {
			csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG2]; // REG_ISP_CSI_BDG_DVP_T
		}

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, ABORT, 1);

		while (--count > 0) {
			if (ISP_RD_BITS(wdma_com_0, REG_WDMA_CORE_T, NORM_STATUS0, ABORT_DONE) &&
				ISP_RD_BITS(wdma_com_1, REG_WDMA_CORE_T, NORM_STATUS0, ABORT_DONE) &&
				ISP_RD_BITS(rdma_com, REG_RDMA_CORE_T, NORM_STATUS0, ABORT_DONE)) {
				vi_pr(VI_INFO, "W/RDMA_CORE abort done, count(%d)\n", count);
				break;
			}
			usleep_range(1, 5);
		}

		if (count == 0) {
			vi_pr(VI_ERR, "WDMA/RDMA_CORE abort fail\n");
			return -1;
		}
	} else if (step == 4) {
		if (ctx->isp_pipe_cfg[err_raw_num].is_422_to_420) {
			uintptr_t csibdg;

			if (err_raw_num == ISP_PRERAW_A) {
				csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0];
			} else if (err_raw_num == ISP_PRERAW_B) {
				csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG1];
			} else {
				csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG2]; // REG_ISP_CSI_BDG_DVP_T
			}

			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, RESET, 1);
		} else {
			sw_rst.raw = 0;
			sw_rst.bits.AXI_RST = 1;
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, sw_rst.raw);
			sw_rst.raw = 0x37f;
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, sw_rst.raw);

			vip_rst.raw = 0;
			vip_rst.b.isp_top = 1;
			vip_set_reset(vip_rst);
		}

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull up\n");
	} else if (step == 5) {
		if (!ctx->isp_pipe_cfg[err_raw_num].is_422_to_420) {
			vip_rst = vip_get_reset();
			vip_rst.b.isp_top = 0;
			vip_set_reset(vip_rst);

			sw_rst.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SW_RST);
			sw_rst.bits.AXI_RST = 0;
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, sw_rst.raw);
			ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0);
		}

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull down\n");
	} else if (step == 6) {
		uintptr_t fe0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		uintptr_t fe1 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
		uintptr_t be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		u8 cnt = 10;

		vi_pr(VI_INFO, "Wait ISP idle\n");

		while (--cnt > 0) {
			if ((ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, FE_IDLE_INFO) == 0x3F) &&
#if !defined( __SOC_PHOBOS__)
				(ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, FE_IDLE_INFO) == 0x3F) &&
#endif
				((ISP_RD_REG(be, REG_PRE_RAW_BE_T, BE_IP_IDLE_INFO) & 0x1F003F) == 0x1F003F) &&
				(ISP_RD_REG(isptopb, REG_ISP_TOP_T, BLK_IDLE)) == 0x3FF) {
				vi_pr(VI_INFO, "FE/BE/ISP idle done, count(%d)\n", cnt);
				break;
			}
			usleep_range(1, 5);
		}

		if (cnt == 0) {
			vi_pr(VI_ERR, "FE0(0x%x)/FE1(0x%x)/BE(0x%x)/ISP(0x%x) not idle.",
				ISP_RD_REG(fe0, REG_PRE_RAW_FE_T, FE_IDLE_INFO),
				ISP_RD_REG(fe1, REG_PRE_RAW_FE_T, FE_IDLE_INFO),
				ISP_RD_REG(be, REG_PRE_RAW_BE_T, BE_IP_IDLE_INFO),
				ISP_RD_REG(isptopb, REG_ISP_TOP_T, BLK_IDLE));
			return -1;
		}
	}

	return 0;
}
/****************************************************************************
 *	YUV Bypass Control Flow Config
 ****************************************************************************/
u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, uint32_t dmaid, uint64_t buf_addr,
					const enum cvi_isp_raw raw_num)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	uint32_t len = 0, stride = 0, num = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL6: //pre_raw_fe0 ch0
	case ISP_BLK_ID_DMA_CTL7: //pre_raw_fe0 ch1
	case ISP_BLK_ID_DMA_CTL8: //pre_raw_fe0 ch2
	case ISP_BLK_ID_DMA_CTL9: //pre_raw_fe0 ch3
	case ISP_BLK_ID_DMA_CTL12: //pre_raw_fe1 ch0
	case ISP_BLK_ID_DMA_CTL13: //pre_raw_fe1 ch1
	case ISP_BLK_ID_DMA_CTL18: //pre_raw_fe2 ch0
	case ISP_BLK_ID_DMA_CTL19: //pre_raw_fe2 ch1
	{
		/* csibdg */
		len = ctx->isp_pipe_cfg[raw_num].csibdg_width * 2;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL28: //raw_top crop_le
	case ISP_BLK_ID_DMA_CTL29: //raw_top crop_se
	{
		/* raw_top */
		len = ctx->isp_pipe_cfg[raw_num].csibdg_width * 2;
		num = ctx->isp_pipe_cfg[raw_num].csibdg_height;
		stride = len;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGLEN, len);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_STRIDE, stride);
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, DMA_SEGNUM, num);

	if (buf_addr)
		ispblk_dma_setaddr(ctx, dmaid, buf_addr);

	return len * num;
}

/****************************************************************************
 *	Slice buffer Control
 ****************************************************************************/
void vi_calculate_slice_buf_setting(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	u32 main_le_num = 0, main_se_num = 0;
	u32 main_le_size = 0, main_se_size = 0;
	u32 sub_le_num = 0, sub_se_num = 0;
	u32 sub_le_size = 0, sub_se_size = 0;
	u16 main_le_w_th = 0, main_le_r_th = 0;
	u16 main_se_w_th = 0, main_se_r_th = 0;
	u16 sub_le_w_th = 0, sub_le_r_th = 0;
	u16 sub_se_w_th = 0, sub_se_r_th = 0;

	u32 line_delay = slc_b_cfg.line_delay, buffer = slc_b_cfg.buffer;
	u32 main_max_grid_size = slc_b_cfg.main_max_grid_size;
	u32 sub_max_grid_size = slc_b_cfg.sub_max_grid_size;
	u32 min_r_th = slc_b_cfg.min_r_thshd;

	u32 w = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 h = ctx->isp_pipe_cfg[raw_num].crop.h;

	// Calculate the ring buffer line number
	main_le_num = line_delay + 2 * main_max_grid_size + buffer;
	main_le_size = VI_256_ALIGN(main_le_num * ((w * 3) / 2));
	sub_le_num  = (h + sub_max_grid_size - 1) / sub_max_grid_size + line_delay / sub_max_grid_size + buffer;
	sub_le_size = VI_256_ALIGN(sub_le_num *
			(((((w + sub_max_grid_size - 1) / sub_max_grid_size) * 48 + 127) >> 7) << 4));
#if 1
	// Calculate the r/w threshold
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		main_le_r_th = 2 * main_max_grid_size;
	else
		main_le_r_th = 2 * main_max_grid_size + buffer;

	main_le_w_th = main_le_num - 1;

	sub_le_r_th = min_r_th;
	sub_le_w_th = (line_delay / sub_max_grid_size) + buffer - 1;
#else //tmp change r_th to 2, wait for brian
	// Calculate the r/w threshold
	main_le_r_th = 2;
	main_le_w_th = main_le_num - 1;

	sub_le_r_th = 2;
	sub_le_w_th = (line_delay / max_grid_size) + buffer - 1;
#endif
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		main_se_num = 2 * main_max_grid_size + buffer;
		main_se_size = VI_256_ALIGN(main_se_num * ((w * 3) / 2));
		sub_se_num  = (h + sub_max_grid_size - 1) / sub_max_grid_size + buffer;
		sub_se_size = VI_256_ALIGN(sub_se_num *
				(((((w + sub_max_grid_size - 1) / sub_max_grid_size) * 48 + 127) >> 7) << 4));
#if 1
		main_se_r_th = 2 * main_max_grid_size;
		main_se_w_th = main_se_num - 1;

		sub_se_r_th = min_r_th;
		sub_se_w_th = buffer - 1;
#else //tmp change r_th to 2, wait for brian
		main_se_r_th = 2;//2 * max_grid_size;
		main_se_w_th = main_se_num - 1;

		sub_se_r_th = 2;//min_r_th;
		sub_se_w_th = buffer - 1;
#endif
	}

	slc_b_cfg.main_path.le_buf_size = main_le_size;
	slc_b_cfg.main_path.le_w_thshd  = main_le_w_th;
	slc_b_cfg.main_path.le_r_thshd  = main_le_r_th;

	slc_b_cfg.sub_path.le_buf_size  = sub_le_size;
	slc_b_cfg.sub_path.le_w_thshd   = sub_le_w_th;
	slc_b_cfg.sub_path.le_r_thshd   = sub_le_r_th;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		slc_b_cfg.main_path.se_buf_size = main_se_size;
		slc_b_cfg.main_path.se_w_thshd  = main_se_w_th;
		slc_b_cfg.main_path.se_r_thshd  = main_se_r_th;

		slc_b_cfg.sub_path.se_buf_size  = sub_se_size;
		slc_b_cfg.sub_path.se_w_thshd   = sub_se_w_th;
		slc_b_cfg.sub_path.se_r_thshd   = sub_se_r_th;
	}
}

void manr_clear_prv_ring_base(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];

	ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, UP_RING_BASE, (1 << MANR_PREV_LE));
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, UP_RING_BASE, (1 << MANR_PREV_SE));
}

void isp_slice_buf_trig(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, RAW_FRAME_VALID, 0x3);
	}
}

void _ispblk_dma_slice_config(struct isp_ctx *ctx, int dmaid, int en)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

#if defined( __SOC_PHOBOS__)
	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL29://raw crop SE
	case ISP_BLK_ID_DMA_CTL23://be_se_wdma_ctl
	case ISP_BLK_ID_DMA_CTL11://fe0 rgbmap SE
	case ISP_BLK_ID_DMA_CTL33://MANR_P_SE
	case ISP_BLK_ID_DMA_CTL35://MANR_C_SE
		return;
	default:
		break;
	}
#endif

	ISP_WR_BITS(dmab, REG_ISP_DMA_CTL_T, DMA_SLICESIZE, SLICE_SIZE, 1);
	ISP_WR_BITS(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, SLICE_ENABLE, en);

	if (dmaid == ISP_BLK_ID_DMA_CTL10 || dmaid == ISP_BLK_ID_DMA_CTL11) { //rgbmap dma
		if (en)
			ISP_WR_BITS(dmab, REG_ISP_DMA_CTL_T, DMA_DUMMY, PERF_PATCH_ENABLE, 0);
		else
			ISP_WR_BITS(dmab, REG_ISP_DMA_CTL_T, DMA_DUMMY, PERF_PATCH_ENABLE, 1);
	}
}

void ispblk_slice_buf_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, u8 en)
{
	// Multi-sensor don't support slice buffer mode.
	if (en && _is_fe_be_online(ctx)) {
		// isptop config
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union REG_ISP_TOP_SCLIE_ENABLE			slice_en;
		union REG_ISP_TOP_W_SLICE_THRESH_MAIN		w_th_main;
		union REG_ISP_TOP_W_SLICE_THRESH_SUB_CURR	w_th_sub_cur;
		union REG_ISP_TOP_W_SLICE_THRESH_SUB_PRV	w_th_sub_prev;
		union REG_ISP_TOP_R_SLICE_THRESH_MAIN		r_th_main;
		union REG_ISP_TOP_R_SLICE_THRESH_SUB_CURR	r_th_sub_cur;
		union REG_ISP_TOP_R_SLICE_THRESH_SUB_PRV	r_th_sub_prev;
		// wdma/rdma core config
		uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
		uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];
		union REG_WDMA_CORE_RING_BUFFER_EN      w_ring_buf_en;
		union REG_RDMA_CORE_RING_BUFFER_EN      r_ring_buf_en;
		bool is_sub_slice_en = ctx->is_3dnr_on && ctx->is_rgbmap_sbm_on;


		slice_en.raw = 0;
		slice_en.bits.SLICE_ENABLE_MAIN_LEXP = en;
		slice_en.bits.SLICE_ENABLE_SUB_LEXP = en && is_sub_slice_en;

		w_th_main.raw = 0;
		w_th_main.bits.W_SLICE_THR_MAIN_LEXP = slc_b_cfg.main_path.le_w_thshd;

		w_th_sub_cur.raw = 0;
		w_th_sub_cur.bits.W_SLICE_THR_SUB_CUR_LEXP = slc_b_cfg.sub_path.le_w_thshd;

		w_th_sub_prev.raw = 0;
		w_th_sub_prev.bits.W_SLICE_THR_SUB_PRV_LEXP = slc_b_cfg.sub_path.le_w_thshd;

		r_th_main.raw = 0;
		r_th_main.bits.R_SLICE_THR_MAIN_LEXP = slc_b_cfg.main_path.le_r_thshd;

		r_th_sub_cur.raw = 0;
		r_th_sub_cur.bits.R_SLICE_THR_SUB_CUR_LEXP = slc_b_cfg.sub_path.le_r_thshd;

		r_th_sub_prev.raw = 0;
		r_th_sub_prev.bits.R_SLICE_THR_SUB_PRV_LEXP = slc_b_cfg.sub_path.le_r_thshd;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			slice_en.bits.SLICE_ENABLE_MAIN_SEXP = en;
			slice_en.bits.SLICE_ENABLE_SUB_SEXP = en && is_sub_slice_en;

			w_th_main.bits.W_SLICE_THR_MAIN_SEXP		= slc_b_cfg.main_path.se_w_thshd;
			w_th_sub_cur.bits.W_SLICE_THR_SUB_CUR_SEXP	= slc_b_cfg.sub_path.se_w_thshd;
			w_th_sub_prev.bits.W_SLICE_THR_SUB_PRV_SEXP	= slc_b_cfg.sub_path.se_w_thshd;
			r_th_main.bits.R_SLICE_THR_MAIN_SEXP		= slc_b_cfg.main_path.se_r_thshd;
			r_th_sub_cur.bits.R_SLICE_THR_SUB_CUR_SEXP	= slc_b_cfg.sub_path.se_r_thshd;
			r_th_sub_prev.bits.R_SLICE_THR_SUB_PRV_SEXP	= slc_b_cfg.sub_path.se_r_thshd;
		}

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCLIE_ENABLE, slice_en.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, W_SLICE_THRESH_MAIN, w_th_main.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, W_SLICE_THRESH_SUB_CURR, w_th_sub_cur.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, W_SLICE_THRESH_SUB_PRV, w_th_sub_prev.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, R_SLICE_THRESH_MAIN, r_th_main.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, R_SLICE_THRESH_SUB_CURR, r_th_sub_cur.raw);
		ISP_WR_REG(isptopb, REG_ISP_TOP_T, R_SLICE_THRESH_SUB_PRV, r_th_sub_prev.raw);

		// wdma/rdma core config
		w_ring_buf_en.raw = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_EN);
		w_ring_buf_en.raw |= (1 << BE_WDMA_LE);
		w_ring_buf_en.raw |= ((is_sub_slice_en) ? (1 << RGBMAP_LE) : 0);

		r_ring_buf_en.raw = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN);
		r_ring_buf_en.raw |= (1 << RAW_RDMA_LE);
		r_ring_buf_en.raw |= ((is_sub_slice_en) ? ((1 << MANR_CUR_LE) | (1 << MANR_PREV_LE)) : 0);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en.raw |= (1 << BE_WDMA_SE);
			w_ring_buf_en.raw |= ((is_sub_slice_en) ? (1 << RGBMAP_SE) : 0);

			r_ring_buf_en.raw |= (1 << RAW_RDMA_SE);
			r_ring_buf_en.raw |= ((is_sub_slice_en) ? ((1 << MANR_CUR_SE) | (1 << MANR_PREV_SE)) : 0);
		}

#if defined( __SOC_PHOBOS__)
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_EN, w_ring_buf_en.raw);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE2, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE3, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN, r_ring_buf_en.raw);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE1, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE4, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE6, slc_b_cfg.sub_path.le_buf_size);
#else
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_EN, w_ring_buf_en.raw);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE4, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE5, slc_b_cfg.sub_path.se_buf_size);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE6, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_SIZE7, slc_b_cfg.main_path.se_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN, r_ring_buf_en.raw);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE1, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE2, slc_b_cfg.main_path.se_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE4, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE5, slc_b_cfg.sub_path.se_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE6, slc_b_cfg.sub_path.le_buf_size);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_SIZE7, slc_b_cfg.sub_path.se_buf_size);
#endif
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL10, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL32, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL34, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL29, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL23, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL11, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL33, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL35, false);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL28, true);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL22, true);
		if (is_sub_slice_en) {
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL10, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL32, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL34, true);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL29, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL23, true);
			if (is_sub_slice_en) {
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL11, true);
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL33, true);
				_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL35, true);
			}
		}
	} else if (!en || _is_be_post_online(ctx)) {
		uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
		uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union REG_WDMA_CORE_RING_BUFFER_EN      w_ring_buf_en;
		union REG_RDMA_CORE_RING_BUFFER_EN      r_ring_buf_en;

		w_ring_buf_en.raw = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_EN);
		w_ring_buf_en.raw &= ~(1 << BE_WDMA_LE);
		w_ring_buf_en.raw &= ~((ctx->is_3dnr_on) ? (1 << RGBMAP_LE) : 0);

		r_ring_buf_en.raw = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN);
		r_ring_buf_en.raw &= ~(1 << RAW_RDMA_LE);
		r_ring_buf_en.raw &= ~((ctx->is_3dnr_on) ? ((1 << MANR_CUR_LE) | (1 << MANR_PREV_LE)) : 0);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en.raw &= ~(1 << BE_WDMA_SE);
			w_ring_buf_en.raw &= ~((ctx->is_3dnr_on) ? (1 << RGBMAP_SE) : 0);

			r_ring_buf_en.raw &= ~(1 << RAW_RDMA_SE);
			r_ring_buf_en.raw &= ~((ctx->is_3dnr_on) ? ((1 << MANR_CUR_SE) | (1 << MANR_PREV_SE)) : 0);
		}

		ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCLIE_ENABLE, 0);
		ISP_WR_REG(wdma_com_0, REG_WDMA_CORE_T, RING_BUFFER_EN, w_ring_buf_en.raw);
		ISP_WR_REG(rdma_com, REG_RDMA_CORE_T, RING_BUFFER_EN, r_ring_buf_en.raw);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL28, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL22, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL10, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL32, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL34, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL29, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL23, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL11, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL33, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL35, false);
	}
}

void isp_runtime_hdr_patgen(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, uint8_t en)
{
	//we should enable fusion in LinearMode in order to use refineWeight on Mars
	uintptr_t isptop = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_HDRFUSION];
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;
	union REG_RAW_TOP_RDMI_ENABLE rdmi_enable;
	union REG_RAW_TOP_PATGEN1 patgen1;
	union REG_FUSION_FS_CTRL_0 fs_ctrl;

	scene_ctrl.raw  = ISP_RD_REG(isptop, REG_ISP_TOP_T, SCENARIOS_CTRL);
	rdmi_enable.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE);
	patgen1.raw     = ISP_RD_REG(rawtop, REG_RAW_TOP_T, PATGEN1);
	if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		scene_ctrl.bits.HDR_ENABLE = en;
		patgen1.bits.PG_ENABLE     = en;
		rdmi_enable.bits.CH_NUM    = en;

		ISP_WR_REG(isptop, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, PATGEN1, patgen1.raw);
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, rdmi_enable.raw);

		if (en) {
			ispblk_fusion_config(ctx, en, en, ISP_FS_OUT_FS);

			fs_ctrl.raw = ISP_RD_REG(ba, REG_FUSION_T, FS_CTRL_0);
			fs_ctrl.bits.SE_IN_SEL			= 1;
			ISP_WR_REG(ba, REG_FUSION_T, FS_CTRL_0, fs_ctrl.raw);
		} else {
			ispblk_fusion_config(ctx, en, en, ISP_FS_OUT_LONG);
		}
	}
}
