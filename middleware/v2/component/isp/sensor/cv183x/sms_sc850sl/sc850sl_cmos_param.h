#ifndef __SC850SL_CMOS_PARAM_H_
#define __SC850SL_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "sc850sl_cmos_ex.h"

static const SC850SL_MODE_S g_astSC850SL_mode[SC850SL_MODE_NUM] = {
	[SC850SL_MODE_2160P30] = {
		.name = "2160p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stMaxSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
		},
		.f32MaxFps = 30,
		.f32MinFps = 2.06, /* 2250 * 30 / 0x7FFF*/
		.u32HtsDef = 53333,
		.u32VtsDef = 2250,
		.stExp[0] = {
			.u16Min = 1,
			.u16Max = 2246,
			.u16Def = 400,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u32Min = 1024,
			.u32Max = 50799,
			.u32Def = 1024,
			.u32Step = 1,
		},
		.stDgain[0] = {
			.u32Min = 1024,
			.u32Max = 8192,
			.u32Def = 1024,
			.u32Step = 1,
		},
	},
	[SC850SL_MODE_2160P30_WDR] = {
		.name = "2160p30wdr",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stMaxSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
		},
		.astImg[1] = {
			.stSnsSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 3840,
				.u32Height = 2160,
			},
			.stMaxSize = {
				.u32Width = 3840,
				.u32Height = 2160,
			},
		},
		.f32MaxFps = 30,
		.f32MinFps = 4.11, /* 4500 * 30 / 0x7FFF*/
		.u32HtsDef = 2160,
		.u32VtsDef = 4500,//0x1194
		.u16SexpMaxReg = 0x10a,
		.stAgain[0] = {
			.u32Min = 1024,
			.u32Max = 50799,
			.u32Def = 1024,
			.u32Step = 1,
		},
		.stDgain[0] = {
			.u32Min = 1024,
			.u32Max = 8192,
			.u32Def = 1024,
			.u32Step = 1,
		},
		.stAgain[1] = {
			.u32Min = 1024,
			.u32Max = 50799,
			.u32Def = 1024,
			.u32Step = 1,
		},
		.stDgain[1] = {
			.u32Min = 1024,
			.u32Max = 8192,
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
		.stManual = {273, 273, 273, 273, 1097, 1097, 1097, 1097},
		.stAuto = {
			{273, 273, 273, 273, 273, 273, 273, 273, /*8*/273, 273, 273, 273, 273, 273, 273, 273},
			{273, 273, 273, 273, 273, 273, 273, 273, /*8*/273, 273, 273, 273, 273, 273, 273, 273},
			{273, 273, 273, 273, 273, 273, 273, 273, /*8*/273, 273, 273, 273, 273, 273, 273, 273},
			{273, 273, 273, 273, 273, 273, 273, 273, /*8*/273, 273, 273, 273, 273, 273, 273, 273},
			{1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097,
				/*8*/1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097},
			{1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097,
				/*8*/1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097},
			{1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097,
				/*8*/1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097},
			{1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097,
				/*8*/1097, 1097, 1097, 1097, 1097, 1097, 1097, 1097},

		},
	},
};

struct combo_dev_attr_s sc850sl_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_400M,
	.mipi_attr = {
		.raw_data_type = RAW_DATA_12BIT,
		.lane_id = {0, 2, 3, 1, 4},
		.wdr_mode = CVI_MIPI_WDR_MODE_VC,
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_27M,
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC850SL_CMOS_PARAM_H_ */
