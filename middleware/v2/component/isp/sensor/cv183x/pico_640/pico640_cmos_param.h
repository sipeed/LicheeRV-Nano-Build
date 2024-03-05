#ifndef __PICO640_CMOS_PARAM_H_
#define __PICO640_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "pico640_cmos_ex.h"

static const PICO640_MODE_S g_astPICO640_mode[PICO640_MODE_NUM] = {
	[PICO640_MODE_480P30] = {
		.name = "480p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 632,
				.u32Height = 479,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 632,
				.u32Height = 479,
			},
			.stMaxSize = {
				.u32Width = 632,
				.u32Height = 479,
			},
		},
	},
};

struct combo_dev_attr_s pico640_rx_attr = {
	.input_mode = INPUT_MODE_CUSTOM_0,
	.mac_clk = RX_MAC_CLK_200M,
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_NONE,
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __PICO640_CMOS_PARAM_H_ */
