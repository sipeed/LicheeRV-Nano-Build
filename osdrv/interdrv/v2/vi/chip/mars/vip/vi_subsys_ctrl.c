#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_VIRT_MAX], g_h_bit[ISP_PRERAW_VIRT_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_VIRT_MAX];

/*******************************************************************************
 *	Subsys config
 ******************************************************************************/
void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe;
	uint32_t width = ctx->isp_pipe_cfg[raw_num].crop.w;
	uint32_t height = ctx->isp_pipe_cfg[raw_num].crop.h;
	union REG_PRE_RAW_FE_PRE_RAW_CTRL raw_ctrl;
	union REG_PRE_RAW_FE_PRE_RAW_FRAME_SIZE  frm_size;
	union REG_PRE_RAW_FE_LE_RGBMAP_GRID_NUMBER  rgbmap_le;
	union REG_PRE_RAW_FE_SE_RGBMAP_GRID_NUMBER  rgbmap_se;
	enum cvi_isp_raw raw;

	raw = find_hw_raw_num(raw_num);

	if (raw == ISP_PRERAW_A) {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	} else if (raw == ISP_PRERAW_B) {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	} else {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];
	}

	frm_size.raw = rgbmap_le.raw = rgbmap_se.raw = 0;

	raw_ctrl.raw = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL);
	raw_ctrl.bits.BAYER_TYPE_LE = ctx->rgb_color_mode[raw_num];
	raw_ctrl.bits.BAYER_TYPE_SE = ctx->rgb_color_mode[raw_num];
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL, raw_ctrl.raw);

	frm_size.bits.FRAME_WIDTHM1 = width - 1;
	frm_size.bits.FRAME_HEIGHTM1 = height - 1;
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_SIZE, frm_size.raw);

	rgbmap_le.bits.LE_RGBMP_H_GRID_SIZE = g_w_bit[raw_num];
	rgbmap_le.bits.LE_RGBMP_V_GRID_SIZE = g_h_bit[raw_num];
	rgbmap_se.bits.SE_RGBMP_H_GRID_SIZE = g_w_bit[raw_num];
	rgbmap_se.bits.SE_RGBMP_V_GRID_SIZE = g_h_bit[raw_num];
#if 0 //only grid size need to program in lmap/rgbmap hw mode
	w_grid_num = UPPER(width, g_w_bit[raw_num]) - 1;
	h_grid_num = UPPER(height, g_h_bit[raw_num]) - 1;

	rgbmap_le.bits.LE_RGBMP_H_GRID_NUMM1 = w_grid_num;
	rgbmap_le.bits.LE_RGBMP_V_GRID_NUMM1 = h_grid_num;
	rgbmap_se.bits.SE_RGBMP_H_GRID_NUMM1 = w_grid_num;
	rgbmap_se.bits.SE_RGBMP_V_GRID_NUMM1 = h_grid_num;
#endif
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, rgbmap_le.raw);
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, rgbmap_se.raw);
}

void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	union REG_PRE_RAW_VI_SEL_1 vi_sel_1;

	vi_sel_1.raw = 0;
	vi_sel_1.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	vi_sel_1.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_1, vi_sel_1.raw);

	if (_is_be_post_online(ctx) && ctx->is_dpcm_on) { // dram->be
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 7);
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 8191);
	} else {
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 0);
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 0);
	}
}

void ispblk_pre_wdma_ctrl_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t pre_wdma = ctx->phys_regs[ISP_BLK_ID_PRE_WDMA];
	union REG_PRE_WDMA_CTRL wdma_ctrl;

	wdma_ctrl.raw = ISP_RD_REG(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL);
	wdma_ctrl.bits.WDMI_EN_LE = ctx->is_offline_postraw;
	wdma_ctrl.bits.WDMI_EN_SE = (ctx->is_offline_postraw && ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_REG(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL, wdma_ctrl.raw);

	// NOTE: for be->dram, 'PRE_RAW_BE_RDMI_DPCM' naming is misleading
	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { // be->dram
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_MODE, 7);
			// 1 if dpcm_mode 7; 0 if dpcm_mode 5
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL, DMA_WR_MSB, 1);
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_MODE, 0);
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_XSTR, 0);
		}
	}
}

void ispblk_preraw_be_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	union REG_PRE_RAW_BE_TOP_CTRL top_ctrl;
	union REG_PRE_RAW_BE_IMG_SIZE_LE img_size;

	top_ctrl.raw = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL);
	top_ctrl.bits.BAYER_TYPE_LE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.BAYER_TYPE_SE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL, top_ctrl.raw);

	img_size.raw = 0;
	img_size.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	img_size.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, IMG_SIZE_LE, img_size.raw);

	ISP_WO_BITS(preraw_be, REG_PRE_RAW_BE_T, UP_PQ_EN, UP_PQ_EN, 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, DEBUG_ENABLE, DEBUG_EN, 1);
}

void ispblk_raw_rdma_ctrl_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t raw_rdma = ctx->phys_regs[ISP_BLK_ID_RAW_RDMA];
	union REG_RAW_RDMA_CTRL_CONFIG ctrl_config;
	union REG_RAW_RDMA_CTRL_RDMA_SIZE rdma_size;

	ctrl_config.raw = 0;
	ctrl_config.bits.LE_RDMA_EN = ctx->is_offline_postraw;
	ctrl_config.bits.SE_RDMA_EN = ctx->is_offline_postraw && ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	ISP_WR_REG(raw_rdma, REG_RAW_RDMA_CTRL_T, CONFIG, ctrl_config.raw);

	rdma_size.raw = 0;
	rdma_size.bits.RDMI_WIDTHM1 = ctx->img_width - 1;
	rdma_size.bits.RDMI_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(raw_rdma, REG_RAW_RDMA_CTRL_T, RDMA_SIZE, rdma_size.raw);

	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on &&
		    !ctx->isp_pipe_cfg[raw_num].is_yuv_bypass_path) { //dram->post
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 7);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	}
}

void ispblk_rawtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	enum ISP_BAYER_TYPE bayer_id = ctx->rgb_color_mode[raw_num];
	union REG_RAW_TOP_RAW_2 raw_2;
	union REG_RAW_TOP_RDMI_ENABLE rdmi_enable;
	union REG_RAW_TOP_LE_LMAP_GRID_NUMBER   le_lmap_size;
	union REG_RAW_TOP_SE_LMAP_GRID_NUMBER   se_lmap_size;
#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
	union REG_RAW_TOP_PATGEN1 patgen1;
#endif

	raw_2.raw = 0;
	raw_2.bits.IMG_WIDTHM1 = ctx->img_width - 1;
	raw_2.bits.IMG_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, RAW_2, raw_2.raw);

	ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, BAYER_TYPE_TOPLEFT, bayer_id);

	rdmi_enable.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE);
	rdmi_enable.bits.CH_NUM = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
	if (!(ctx->isp_pipe_cfg[raw_num].is_hdr_on)
	    && (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
		//In order for linearMode use guideWeight
		rdmi_enable.bits.CH_NUM		= 1;
		patgen1.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, PATGEN1);
		patgen1.bits.PG_ENABLE		= 1;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, PATGEN1, patgen1.raw);
	} else {
		rdmi_enable.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		patgen1.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, PATGEN1);
		patgen1.bits.PG_ENABLE		= 0;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, PATGEN1, patgen1.raw);
	}
#endif
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, rdmi_enable.raw);

	if (ctx->is_yuv_sensor) {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);
	} else {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}
#if 0
	if (_is_fe_be_online(ctx)) { // fe->be->dram->post single sensor frame_base/slice buffer
		union REG_RAW_TOP_RDMA_SIZE rdma_size;

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 1);
		rdma_size.raw = 0;
		rdma_size.bits.RDMI_WIDTHM1 = ctx->img_width - 1;
		rdma_size.bits.RDMI_HEIGHTM1 = ctx->img_height - 1;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, RDMA_SIZE, rdma_size.raw);
	} else { //fe->dram->be->post (2/3 sensors) or fe->be->post (onthefly)
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 0);
	}
#endif
	if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { //dram->post
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	}

	le_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER);
	le_lmap_size.bits.LE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
	le_lmap_size.bits.LE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER, le_lmap_size.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		se_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER);
		se_lmap_size.bits.SE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
		se_lmap_size.bits.SE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER, se_lmap_size.raw);
	}
}

void ispblk_rgbtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	union REG_ISP_RGB_TOP_0 reg_0;
	union REG_ISP_RGB_TOP_9 reg_9;

	reg_0.raw = reg_9.raw = 0;

	reg_0.raw = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_0);
	reg_0.bits.RGBTOP_BAYER_TYPE = ctx->rgb_color_mode[raw_num];
	ISP_WR_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_0, reg_0.raw);

	reg_9.bits.RGBTOP_IMGW_M1 = ctx->img_width - 1;
	reg_9.bits.RGBTOP_IMGH_M1 = ctx->img_height - 1;
	ISP_WR_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_9, reg_9.raw);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_TOP_T, DBG_IP_S_VLD, IP_DBG_EN, 1);
}

void ispblk_yuvtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	union REG_YUV_TOP_IMGW_M1 imgw_m1;

	imgw_m1.raw = 0;

	ISP_WO_BITS(yuvtop, REG_YUV_TOP_T, YUV_3, YONLY_EN, ctx->is_yuv_sensor);

	imgw_m1.bits.YUV_TOP_IMGW_M1 = ctx->img_width - 1;
	imgw_m1.bits.YUV_TOP_IMGH_M1 = ctx->img_height - 1;
	ISP_WR_REG(yuvtop, REG_YUV_TOP_T, IMGW_M1, imgw_m1.raw);

	if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_scaler) {
		//bypass_v = 1 -> 422P online to scaler
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_CTRL, BYPASS_V, 1);
	} else {
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_CTRL, BYPASS_V, !ctx->isp_pipe_cfg[raw_num].is_offline_scaler);
	}
}

void ispblk_isptop_config(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uint8_t pre_fe0_trig_by_hw = 0, pre_fe1_trig_by_hw = 0, pre_fe2_trig_by_hw = 0;
	uint8_t pre_be_trig_by_hw = 0;
	uint8_t post_trig_by_hw = 0;

	union REG_ISP_TOP_INT_EVENT0_EN ev0_en;
	union REG_ISP_TOP_INT_EVENT1_EN ev1_en;
	union REG_ISP_TOP_INT_EVENT2_EN ev2_en;
	union REG_ISP_TOP_CTRL_MODE_SEL0 trig_sel0;
	union REG_ISP_TOP_CTRL_MODE_SEL1 trig_sel1;
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;

	ev0_en.raw = ev1_en.raw = ev2_en.raw = 0;
	trig_sel0.raw = trig_sel1.raw = scene_ctrl.raw = 0;

	if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
		pre_fe0_trig_by_hw = 0;
	else {
		if (!ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path) { //RGB sensor
			if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_hdr_on && !ctx->is_synthetic_hdr_on)
				pre_fe0_trig_by_hw = 3;
			else
				pre_fe0_trig_by_hw = 1;
		} else { //YUV sensor
			switch (ctx->total_chn_num) {
			case 1:
				pre_fe0_trig_by_hw = 1;
				break;
			case 2:
				pre_fe0_trig_by_hw = 3;
				break;
			case 3:
				pre_fe0_trig_by_hw = 7;
				break;
			case 4:
				pre_fe0_trig_by_hw = 15;
				break;
			default:
				break;
			}
		}
	}

	if (ctx->is_multi_sensor) {
		if (ctx->isp_pipe_cfg[ISP_PRERAW_B].is_offline_preraw)
			pre_fe1_trig_by_hw = 0;
		else {
			if (!ctx->isp_pipe_cfg[ISP_PRERAW_B].is_yuv_bypass_path) { //RGB sensor
				if (ctx->isp_pipe_cfg[ISP_PRERAW_B].is_hdr_on)
					pre_fe1_trig_by_hw = 3;
				else
					pre_fe1_trig_by_hw = 1;
			} else { //YUV sensor
				switch (ctx->total_chn_num - ctx->rawb_chnstr_num) {
				case 1:
					pre_fe1_trig_by_hw = 1;
					break;
				case 2:
					pre_fe1_trig_by_hw = 3;
					break;
				default:
					break;
				}
			}
		}
		if (ctx->isp_pipe_cfg[ISP_PRERAW_C].is_offline_preraw)
			pre_fe2_trig_by_hw = 0;
		else {
			if (!ctx->isp_pipe_cfg[ISP_PRERAW_C].is_yuv_bypass_path) { //RGB sensor
				if (ctx->isp_pipe_cfg[ISP_PRERAW_C].is_hdr_on)
					pre_fe2_trig_by_hw = 3;
				else
					pre_fe2_trig_by_hw = 1;
			} else { //YUV sensor
				switch (ctx->total_chn_num - ctx->rawb_chnstr_num) {
				case 1:
					pre_fe2_trig_by_hw = 1;
					break;
				case 2:
					pre_fe2_trig_by_hw = 3;
					break;
				default:
					break;
				}
			}
		}
	}

	if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw)
		pre_be_trig_by_hw = 0;
	else if (ctx->is_offline_be)
		pre_be_trig_by_hw = 0;
	else { //be online, on the fly mode or fe_A->be
		if (ctx->isp_pipe_cfg[ISP_PRERAW_A].is_yuv_bypass_path)
			pre_be_trig_by_hw = 0;
		else { //Single RGB sensor
			if (ctx->is_hdr_on)
				pre_be_trig_by_hw = 3;
			else
				pre_be_trig_by_hw = 1;
		}
	}

	// fly mode or single sensor and slice buffer mode on. post trigger by vsync
	if ((ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw == 0) &&
		(_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)))
		post_trig_by_hw = 1;
	else //trigger by SW
		post_trig_by_hw = 0;

	//pre_fe0
	ev0_en.bits.FRAME_DONE_ENABLE_FE0	= 0xF;
	ev2_en.bits.FRAME_START_ENABLE_FE0	= 0xF;
	trig_sel0.bits.TRIG_STR_SEL_FE0		= pre_fe0_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_FE0		= pre_fe0_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_FE0		= pre_fe0_trig_by_hw;

	//pre_fe1
	ev0_en.bits.FRAME_DONE_ENABLE_FE1	= 0x3;
	ev2_en.bits.FRAME_START_ENABLE_FE1	= 0x3;
	trig_sel0.bits.TRIG_STR_SEL_FE1		= pre_fe1_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_FE1		= pre_fe1_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_FE1		= pre_fe1_trig_by_hw;

	//pre_fe2
	ev0_en.bits.FRAME_DONE_ENABLE_FE2	= 0x3;
	ev2_en.bits.FRAME_START_ENABLE_FE2	= 0x3;
	trig_sel0.bits.TRIG_STR_SEL_FE2		= pre_fe2_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_FE2		= pre_fe2_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_FE2		= pre_fe2_trig_by_hw;

	//pre_be
	ev0_en.bits.FRAME_DONE_ENABLE_BE	= 0x3;
	trig_sel0.bits.TRIG_STR_SEL_BE		= pre_be_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_BE		= pre_be_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_BE		= pre_be_trig_by_hw;

	//postraw
	ev0_en.bits.FRAME_DONE_ENABLE_POST	= 0x1;
	ev0_en.bits.SHAW_DONE_ENABLE_POST	= 0x1;
	trig_sel0.bits.TRIG_STR_SEL_RAW		= post_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_RAW		= post_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_RAW		= post_trig_by_hw;
	trig_sel0.bits.TRIG_STR_SEL_POST	= post_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_POST		= post_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_POST		= post_trig_by_hw;

	//err int
	ev2_en.bits.FRAME_ERR_ENABLE	= 1;
	ev2_en.bits.INT_DMA_ERR_ENABLE	= 1;

	//scenario_ctrl mode
	//single sensor
	scene_ctrl.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL);
	scene_ctrl.bits.PRE2BE_L_ENABLE		= !ctx->is_offline_be;
	scene_ctrl.bits.PRE2BE_S_ENABLE		= !ctx->is_offline_be && ctx->is_hdr_on;
	scene_ctrl.bits.PRE2YUV_422_ENABLE	= 0;
	//Multi sensors
	scene_ctrl.bits.BE2RAW_L_ENABLE		= !ctx->is_offline_postraw;
	scene_ctrl.bits.BE2RAW_S_ENABLE		= !ctx->is_offline_postraw && ctx->is_hdr_on;
	// Multi sensors or raw replay
	scene_ctrl.bits.BE_RDMA_L_ENABLE	= ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw;
	scene_ctrl.bits.BE_RDMA_S_ENABLE	= (ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[ISP_PRERAW_A].is_offline_preraw) &&
						  ctx->is_hdr_on;
	// onthefly -> 0 or single sensor
	scene_ctrl.bits.BE_WDMA_L_ENABLE	= ctx->is_offline_postraw;
	scene_ctrl.bits.BE_WDMA_S_ENABLE	= ctx->is_offline_postraw && ctx->is_hdr_on;
	scene_ctrl.bits.BE_SRC_SEL		= 0; //0/1/2 from fe0/fe1/fe2
	scene_ctrl.bits.AF_RAW0YUV1		= 0; //0 from be/1 from post
	scene_ctrl.bits.RGBMP_ONLINE_L_ENABLE	= _is_all_online(ctx); //onthefly -> 1
	//single sensor hdr and slice buffer mode on
	scene_ctrl.bits.RGBMP_ONLINE_S_ENABLE	= 0;
	scene_ctrl.bits.RAW2YUV_422_ENABLE	= 0;
	scene_ctrl.bits.HDR_ENABLE		= ctx->is_hdr_on;
#if (defined( __SOC_MARS__) && !defined(PORTING_TEST))
	if (!(ctx->is_hdr_on)
	    && (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
		//In order for linearMode use guideWeight
		scene_ctrl.bits.HDR_ENABLE	= 1;
	}
#endif
	// to verify IP, turn off HW LUT of rgbgamma, ynr, and cnr.
	scene_ctrl.bits.HW_AUTO_ENABLE		= 0;
	// set the position of the beginning of YUV suggested by HW
	scene_ctrl.bits.DCI_RGB0YUV1		= 0;

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0_EN, ev0_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1_EN, ev1_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2_EN, ev2_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0, trig_sel0.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL1, trig_sel1.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);

	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, DUMMY, DBUS_SEL, 4);
	//ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_1C, 7);
}

#ifdef PORTING_TEST

void ispblk_isptop_fpga_config(struct isp_ctx *ctx, uint16_t test_case)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;

	scene_ctrl.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL);
	// to verify IP, turn off HW LUT of rgbgamma, ynr, and cnr.
	if (test_case == 0) {
		scene_ctrl.bits.HW_AUTO_ENABLE		= 0;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	} else if (test_case == 1) {
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	} else if (test_case == 2) { //auto_iso
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 2;
	} else if (test_case == 0xFF) { //store default config
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	}

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);
}

#endif

void isp_intr_set_mask(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT0_EN, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT1_EN, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT2_EN, 0);
}

void isp_reset(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_CTRL_MODE_SEL0 mode_sel0;

	// disable interrupt
	isp_intr_set_mask(ctx);

	// switch back to hw trig.
	mode_sel0.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0);
	mode_sel0.bits.TRIG_STR_SEL_FE0 = 0xf;
	mode_sel0.bits.TRIG_STR_SEL_FE1 = 0x3;
	mode_sel0.bits.TRIG_STR_SEL_FE2 = 0x3;
	mode_sel0.bits.TRIG_STR_SEL_BE  = 0x3;
	mode_sel0.bits.TRIG_STR_SEL_RAW  = 0x1;
	mode_sel0.bits.TRIG_STR_SEL_POST  = 0x1;
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0, mode_sel0.raw);

	// reset
	//AXI_RST first, then reset other
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x40);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x37F);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x40);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0);

	// clear intr
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1, 0xffffffff);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2, 0xffffffff);
}

/*******************************************************************************
 *	Subsys Debug Infomation
 ******************************************************************************/

struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_fe;
	struct _fe_dbg_i data;

	if (raw_num == ISP_PRERAW_A) {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	} else if (raw_num == ISP_PRERAW_B) {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
	} else {
		preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];
	}

	data.fe_idle_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_DEBUG_STATE);
	data.fe_done_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, FE_IDLE_INFO);

	return data;
}

struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	struct _be_dbg_i data;

	data.be_done_sts = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_INFO);
	data.be_dma_idle_sts = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_DMA_IDLE_INFO);

	return data;
}

struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	struct _post_dbg_i data;

	data.top_sts = ISP_RD_REG(isptopb, REG_ISP_TOP_T, BLK_IDLE);

	return data;
}

struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t rdma_com = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE];
	struct _dma_dbg_i data;

	//RDMA err status
	data.rdma_err_sts = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, NORM_STATUS0); //0x10
	//RDMA status
	data.rdma_idle = ISP_RD_REG(rdma_com, REG_RDMA_CORE_T, NORM_STATUS1); //0x14

	//WDMA_0 err status
	data.wdma_0_err_sts = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, NORM_STATUS0); //0x10
	//WDMA_0 status
	data.wdma_0_idle = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, NORM_STATUS1); //0x14

	//WDMA_1 err status
	data.wdma_1_err_sts = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, NORM_STATUS0); //0x10
	//WDMA_1 status
	data.wdma_1_idle = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, NORM_STATUS1); //0x14

	return data;
}
