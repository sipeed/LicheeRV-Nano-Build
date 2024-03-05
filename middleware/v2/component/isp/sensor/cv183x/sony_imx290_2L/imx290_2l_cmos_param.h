#ifndef __IMX290_CMOS_PARAM_H_
#define __IMX290_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#include "cvi_sns_ctrl.h"
#include "imx290_2l_cmos_ex.h"

static const IMX290_2L_MODE_S g_astImx290_2l_mode[IMX290_2L_MODE_NUM] = {
	[IMX290_2L_MODE_720P30] = {
		.name = "720p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
			.stWndRect = {
				.s32X = 12,
				.s32Y = 11,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
		},
		.f32MaxFps = 30,
		.f32MinFps = 0.09, /* 750 * 30 / 0x3FFFF */
		.u32HtsDef = 0x19C8,
		.u32VtsDef = 750,
		.stExp[0] = {
			.u16Min = 1,
			.u16Max = 748,
			.u16Def = 353,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u16Min = 1024,
			.u16Max = 62416,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[0] = {
			.u16Min = 1024,
			.u16Max = 38485,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.u16RHS1 = 9,
		.u16BRL = 735,
		.u16OpbSize = 4,
		.u16MarginVtop = 11,
		.u16MarginVbot = 5,
	},
	[IMX290_2L_MODE_720P15_WDR] = {
		.name = "720p15wdr",
		/* sef */
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
			.stWndRect = {
				.s32X = 12,
				.s32Y = 11,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
		},
		/* sef */
		.astImg[1] = {
			.stSnsSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
			.stWndRect = {
				.s32X = 12,
				.s32Y = 11,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1308,
				.u32Height = 736,
			},
		},
		.f32MaxFps = 15,
		.f32MinFps = 0.05, /* 750 * 15 / 0x3FFFF */
		.u32HtsDef = 0x19C8,
		.u32VtsDef = 750,
		.stExp[0] = {
			.u16Min = 1,
			.u16Max = 6,
			.u16Def = 6,
			.u16Step = 1,
		},
		.stExp[1] = {
			.u16Min = 1,
			.u16Max = 1488,
			.u16Def = 828,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u16Min = 1024,
			.u16Max = 62416,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stAgain[1] = {
			.u16Min = 1024,
			.u16Max = 62416,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[0] = {
			.u16Min = 1024,
			.u16Max = 38485,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[1] = {
			.u16Min = 1024,
			.u16Max = 38485,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.u16RHS1 = 9,
		.u16BRL = 735,
		.u16OpbSize = 4,
		.u16MarginVtop = 11,
		.u16MarginVbot = 5,
	},
};

struct combo_dev_attr_s imx290_2l_rx_attr = {
	.input_mode = INPUT_MODE_SUBLVDS,
	.lvds_attr = {
		.wdr_mode = CVI_WDR_MODE_DOL_2F,
		.sync_mode = LVDS_SYNC_MODE_SAV,
		.raw_data_type = RAW_DATA_10BIT,
		.data_endian = LVDS_ENDIAN_BIG,
		.sync_code_endian = LVDS_ENDIAN_BIG,
		.lane_id = {0, 1, 2, -1, -1},
		.sync_code = {
			{
				{0x004, 0x1D4, 0x404, 0x5D4},
				{0x008, 0x1D8, 0x408, 0x5D8},
				{0x00C, 0x1DC, 0x40C, 0x5DC},
			},
		},
		.vsync_type = {
			.sync_type = LVDS_VSYNC_NORMAL,
		},
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX290_CMOS_PARAM_H_ */
