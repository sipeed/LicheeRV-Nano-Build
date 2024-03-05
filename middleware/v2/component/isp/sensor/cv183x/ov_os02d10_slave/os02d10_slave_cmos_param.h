#ifndef __OS02D10_SLAVE_CMOS_PARAM_H_
#define __OS02D10_SLAVE_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "os02d10_slave_cmos_ex.h"

static const OS02D10_SLAVE_MODE_S g_astOs02d10_Slave_mode[OS02D10_SLAVE_MODE_NUM] = {
	[OS02D10_SLAVE_MODE_1920X1080P30] = {
		.name = "1920x1080p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1928,
				.u32Height = 1088,
			},
			.stWndRect = {
				.s32X = 4,
				.s32Y = 4,
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stMaxSize = {
				.u32Width = 1928,
				.u32Height = 1088,
			},
		},
		.f32MaxFps = 37.45,
		.f32MinFps = 8,
		.u32HtsDef = 1004,
		.u32VtsDef = 1201,
		.stExp[0] = {
			.u16Min = 5,
			.u16Max = 1201 - 10,
			.u16Def = 500,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u32Min = 1408,
			.u32Max = 15872,
			.u32Def = 1408,
			.u32Step = 1,
		},
		.stDgain[0] = {
			.u32Min = 1024,
			.u32Max = 32768,
			.u32Def = 1024,
			.u32Step = 1,
		},
	},
};

static ISP_CMOS_BLACK_LEVEL_S g_stIspBlcCalibratio10Bit = {
	.bUpdate = CVI_TRUE,
	.blcAttr = {
		.Enable = 1,
		.enOpType = OP_TYPE_AUTO,
		.stManual = {252, 252, 252, 252, 1091, 1091, 1091, 1091},
		.stAuto = {
			{252, 252, 252, 252, 252, 252, 252, 252},
			{252, 252, 252, 252, 252, 252, 252, 252},
			{252, 252, 252, 252, 252, 252, 252, 252},
			{252, 252, 252, 252, 252, 252, 252, 252},
			{1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091,
				/*8*/1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091},
			{1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091,
				/*8*/1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091},
			{1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091,
				/*8*/1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091},
			{1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091,
				/*8*/1091, 1091, 1091, 1091, 1091, 1091, 1091, 1091},
		},
	},
};

struct combo_dev_attr_s os02d10_slave_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_200M,
	.mipi_attr = {
		.raw_data_type = RAW_DATA_10BIT,
		.lane_id = {4, 3, 2, -1, -1},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
		.dphy = {
			.enable = 1,
			.hs_settle = 8, //0x001000FF --> 0x000800FF
		},
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_24M,
	},
	.devno = 1,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __OS02D10_SLAVE_CMOS_PARAM_H_ */
