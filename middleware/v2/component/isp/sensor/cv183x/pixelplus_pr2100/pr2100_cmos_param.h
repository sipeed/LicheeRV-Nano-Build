#ifndef __PR2100_CMOS_PARAM_H_
#define __PR2100_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "pr2100_cmos_ex.h"

static const PR2100_MODE_S g_astPr2100_mode[PR2100_MODE_NUM] = {
	[PR2100_MODE_1080P] = {
		.name = "1080p25",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stMaxSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
		},

	},
	[PR2100_MODE_1080P_2CH] = {
		.name = "1080p25_2ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 3844,
				.u32Height = 1124,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 3844,
				.u32Height = 1124,
			},
			.stMaxSize = {
				.u32Width = 3844,
				.u32Height = 1124,
			},
		},

	},
	[PR2100_MODE_1080P_4CH] = {
		.name = "1080p25_4ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 7688,
				.u32Height = 1124,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 7688,
				.u32Height = 1124,
			},
			.stMaxSize = {
				.u32Width = 7688,
				.u32Height = 1124,
			},
		},
	},
};

struct combo_dev_attr_s pr2100_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_600M,
	.mipi_attr = {
		.raw_data_type = YUV422_8BIT,
		.lane_id = {2, 0, 1, 3, 4},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_NONE,
	},
	.devno = 0,
};

struct combo_dev_attr_s pr2100_rx1_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_600M,
	.mipi_attr = {
		.raw_data_type = YUV422_8BIT,
		.lane_id = {2, 0, 1, 3, 4},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
	},
	.mclk = {
		.cam = 1,
		.freq = CAMPLL_FREQ_NONE,
	},
	.devno = 1,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __PR2100_CMOS_PARAM_H_ */
