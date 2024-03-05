#ifndef __N5_CMOS_PARAM_H_
#define __N5_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "n5_cmos_ex.h"

static const N5_MODE_S g_astN5_mode[N5_MODE_NUM] = {
	[N5_MODE_1080P_25P] = {
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
};

struct combo_dev_attr_s n5_rx_attr = {
	.input_mode = INPUT_MODE_BT656_9B,
	.mac_clk = RX_MAC_CLK_400M,
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_27M,
	},
	.devno = 0,
};

struct combo_dev_attr_s n5_rx1_attr = {
	.input_mode = INPUT_MODE_BT656_9B,
	.mac_clk = RX_MAC_CLK_400M,
	.mclk = {
		.cam = 1,
		.freq = CAMPLL_FREQ_27M,
	},
	.devno = 1,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __N5_CMOS_PARAM_H_ */
