#ifndef __MCS369Q_CMOS_PARAM_H_
#define __MCS369Q_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "mcs369q_cmos_ex.h"

static const MCS369Q_MODE_S g_astMCS369Q_mode[MCS369Q_MODE_NUM] = {
	[MCS369Q_MODE_1440P30] = {
		.name = "1440p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stMaxSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
		},
	},
};

struct combo_dev_attr_s mcs369q_rx_attr = {
	.input_mode = INPUT_MODE_BT1120,
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


#endif /* __MCS369Q_CMOS_PARAM_H_ */
