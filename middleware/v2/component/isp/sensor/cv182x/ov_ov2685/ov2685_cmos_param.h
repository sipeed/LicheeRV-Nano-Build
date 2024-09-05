#ifndef __OV2685_CMOS_PARAM_H_
#define __OV2685_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef ARCH_CV182X
#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#else
#include <linux/cif_uapi.h>
#include <linux/vi_snsr.h>
#include <linux/cvi_type.h>
#endif
#include "cvi_sns_ctrl.h"
#include "ov2685_cmos_ex.h"

static const OV2685_MODE_S g_astOv2685_mode[OV2685_MODE_NUM] = {
	[OV2685_MODE_1600X1200P30] = {
		.name = "1600X1200P30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1600,
				.u32Height = 1200,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1600,
				.u32Height = 1200,
			},
			.stMaxSize = {
				.u32Width = 1600,
				.u32Height = 1200,
			},
		},
		.f32MaxFps = 30,
		.f32MinFps = 2.75, /* 1500 * 30 / 16383  */
		.u32HtsDef = 1500,
		.u32VtsDef = 1294,
		.stExp[0] = {
			.u16Min = 4,
			.u16Max = 1294 - 4,
			.u16Def = 400,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u32Min = 1024,
			.u32Max = 63448,
			.u32Def = 1024,
			.u32Step = 1,
		},
		.stDgain[0] = {
			.u32Min = 1024,
			.u32Max = 1024,
			.u32Def = 1024,
			.u32Step = 1,
		},
	},
};

static ISP_CMOS_BLACK_LEVEL_S g_stIspBlcCalibratio = {
	.bUpdate = CVI_TRUE,
	.blcAttr = {
		.Enable = 1,
		.enOpType = OP_TYPE_AUTO,
		.stManual = {64, 64, 64, 64, 0, 0, 0, 0
#ifdef ARCH_CV182X
			, 1039, 1039, 1039, 1039
#endif
		},
		.stAuto = {
			{64, 64, 64, 64, 64, 64, 64, 64, /*8*/64, 64, 64, 64, 64, 64, 64, 64},
			{64, 64, 64, 64, 64, 64, 64, 64, /*8*/64, 64, 64, 64, 64, 64, 64, 64},
			{64, 64, 64, 64, 64, 64, 64, 64, /*8*/64, 64, 64, 64, 64, 64, 64, 64},
			{64, 64, 64, 64, 64, 64, 64, 64, /*8*/64, 64, 64, 64, 64, 64, 64, 64},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#ifdef ARCH_CV182X
			{1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
				/*8*/1039, 1039, 1039, 1039, 1204, 1039, 1039, 1039},
			{1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
				/*8*/1039, 1039, 1039, 1039, 1204, 1039, 1039, 1039},
			{1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
				/*8*/1039, 1039, 1039, 1039, 1204, 1039, 1039, 1039},
			{1039, 1039, 1039, 1039, 1039, 1039, 1039, 1039,
				/*8*/1039, 1039, 1039, 1039, 1204, 1039, 1039, 1039},
#endif
		},
	},
};

struct combo_dev_attr_s ov2685_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_600M,
	.mipi_attr = {
		.raw_data_type = RAW_DATA_10BIT,
		.lane_id = {4, 3, 2, -1, -1},
		.pn_swap = {1, 1, 1, 0, 0},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
		.dphy = {
			.enable = 1,
			.hs_settle = 8,
		},
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_24M,
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __OV2685_CMOS_PARAM_H_ */

