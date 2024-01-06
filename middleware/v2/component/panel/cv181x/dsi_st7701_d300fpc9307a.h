#ifndef _MIPI_TX_PARAM_ST_7701_D300FPC9307A_H_
#define _MIPI_TX_PARAM_ST_7701_D300FPC9307A_H_

#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>

#define ST7701_D300FPC9307A_VACT	854
#define ST7701_D300FPC9307A_VSA		10
#define ST7701_D300FPC9307A_VBP		42
#define ST7701_D300FPC9307A_VFP		4

#define ST7701_D300FPC9307A_HACT	480
#define ST7701_D300FPC9307A_HSA		2
#define ST7701_D300FPC9307A_HBP		43
#define ST7701_D300FPC9307A_HFP		8

#define PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_st7701_480x854 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_D300FPC9307A_HSA,
		.vid_hbp_pixels = ST7701_D300FPC9307A_HBP,
		.vid_hfp_pixels = ST7701_D300FPC9307A_HFP,
		.vid_hline_pixels = ST7701_D300FPC9307A_HACT,
		.vid_vsa_lines = ST7701_D300FPC9307A_VSA,
		.vid_vbp_lines = ST7701_D300FPC9307A_VBP,
		.vid_vfp_lines = ST7701_D300FPC9307A_VFP,
		.vid_active_lines = ST7701_D300FPC9307A_VACT,
		.vid_vsa_pos_polarity = false,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = PIXEL_CLK(ST7701_D300FPC9307A),
};

const struct hs_settle_s hs_timing_cfg_st7701_480x854 = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_st7701_d300fpc9307a_0[] = {0x11, 0x00 }; // turn off sleep mode
        //{REGFLAG_DELAY, 60, {} }, 
static CVI_U8 data_st7701_d300fpc9307a_1[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x13 };
static CVI_U8 data_st7701_d300fpc9307a_2[] = {0xef, 0x08 };
static CVI_U8 data_st7701_d300fpc9307a_3[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x10 };
static CVI_U8 data_st7701_d300fpc9307a_4[] = {0xc0, 0xe9, 0x03 }; // display line setting, set to 854
static CVI_U8 data_st7701_d300fpc9307a_5[] = {0xc1, 0x10, 0x0c }; // vbp
static CVI_U8 data_st7701_d300fpc9307a_6[] = {0xc2, 0x20, 0x0a }; // 
static CVI_U8 data_st7701_d300fpc9307a_7[] = {0xcc, 0x10 };
static CVI_U8 data_st7701_d300fpc9307a_8[] = {0xb0, 0x00, 0x23, 0x2a, 0x0a, 0x0e, 0x03, 0x12, 0x06,
						0x06, 0x2a, 0x00, 0x10, 0x0f, 0x2d, 0x34, 0x1f };
static CVI_U8 data_st7701_d300fpc9307a_9[] = {0xb1, 0x00, 0x24, 0x2b, 0x0f, 0x12, 0x07, 0x15, 0x0a,
						0x0a, 0x2b, 0x08, 0x13, 0x10, 0x2d, 0x33, 0x1f };
static CVI_U8 data_st7701_d300fpc9307a_10[] = {0xff, 0x77, 0x01, 0x00, 0x00, 0x11 };
static CVI_U8 data_st7701_d300fpc9307a_11[] = {0xb0, 0x4d };
static CVI_U8 data_st7701_d300fpc9307a_12[] = {0xb1, 0x48 };
static CVI_U8 data_st7701_d300fpc9307a_13[] = {0xb2, 0x84 };
static CVI_U8 data_st7701_d300fpc9307a_14[] = {0xb3, 0x80 };
static CVI_U8 data_st7701_d300fpc9307a_15[] = {0xb5, 0x45 };
static CVI_U8 data_st7701_d300fpc9307a_16[] = {0xb7, 0x85 };
static CVI_U8 data_st7701_d300fpc9307a_17[] = {0xb8, 0x33 };
static CVI_U8 data_st7701_d300fpc9307a_18[] = {0xc1, 0x78 };
static CVI_U8 data_st7701_d300fpc9307a_19[] = {0xc2, 0x78 };
//{REGFLAG_DELAY, 50, {} },
static CVI_U8 data_st7701_d300fpc9307a_20[] = {0xd0, 0x88 };
static CVI_U8 data_st7701_d300fpc9307a_21[] = {0xe0, 0x00, 0x00, 0x02 };
static CVI_U8 data_st7701_d300fpc9307a_22[] = {0xe1, 0x06, 0xa0, 0x08, 0xa0, 0x05, 0xa0, 0x07, 0xa0, 0x00, 0x44, 0x44 };
static CVI_U8 data_st7701_d300fpc9307a_23[] = {0xe2, 0x30, 0x30, 0x44, 0x44, 0x6e, 0xa0, 0x00, 0x00, 0x6e, 0xa0, 0x00, 0x00 };
static CVI_U8 data_st7701_d300fpc9307a_24[] = {0xe3, 0x00, 0x00, 0x33, 0x33 };
static CVI_U8 data_st7701_d300fpc9307a_25[] = {0xe4, 0x44, 0x44 };
static CVI_U8 data_st7701_d300fpc9307a_26[] = {0xe5, 0x0D, 0x69, 0x0a, 0xa0, 0x0f, 0x6b, 0x0a, 0xa0, 0x09,
						0x65, 0x0a, 0xa0, 0x0b, 0x67, 0x0a, 0xa0 };
static CVI_U8 data_st7701_d300fpc9307a_27[] = {0xe6, 0x00, 0x00, 0x33, 0x33 };
static CVI_U8 data_st7701_d300fpc9307a_28[] = {0xe7, 0x44, 0x44 };
static CVI_U8 data_st7701_d300fpc9307a_29[] = {0xe8, 0x0C, 0x68, 0x0a, 0xa0, 0x0e, 0x6a, 0x0a, 0xa0, 0x08, 0x64,
						0x0a, 0xa0, 0x0a, 0x66, 0x0a, 0xa0 };
static CVI_U8 data_st7701_d300fpc9307a_30[] = {0xe9, 0x36, 0x00 };
static CVI_U8 data_st7701_d300fpc9307a_31[] = {0xeb, 0x00, 0x01, 0xe4, 0xe4, 0x44, 0x88, 0x40 };
//{0xec, 0x3c, 0x01 },
static CVI_U8 data_st7701_d300fpc9307a_32[] = {0xed, 0xff, 0x45, 0x67, 0xfa, 0x01, 0x2b, 0xcf, 0xff, 0xff, 0xfc, 0xb2,
						0x10, 0xaf, 0x76, 0x54, 0xff };
static CVI_U8 data_st7701_d300fpc9307a_33[] = {0xef, 0x10, 0x0d, 0x04, 0x08, 0x3f, 0x1f };
static CVI_U8 data_st7701_d300fpc9307a_34[] = {0x11 };
//{0xff, 0x77, 0x01, 0x00, 0x00, 0x00 },
//{REGFLAG_DELAY, 50, {} },
static CVI_U8 data_st7701_d300fpc9307a_35[] = {0x3a, 0x55 };
static CVI_U8 data_st7701_d300fpc9307a_36[] = {0x29, 0x00 };
//{REGFLAG_END_OF_TABLE, 0x00, {} }


// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1 0x05
#define TYPE2 0x15
#define TYPE3 0x29
const struct dsc_instr dsi_init_cmds_st7701_480x854[] = {
	{.delay = 60, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_0 },
	{.delay = 0, .data_type = TYPE3, .size = 6, .data = data_st7701_d300fpc9307a_1 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_2 },
	{.delay = 0, .data_type = TYPE3, .size = 6, .data = data_st7701_d300fpc9307a_3 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_4 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_5 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_6 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_7 },
	{.delay = 0, .data_type = TYPE3, .size = 17, .data = data_st7701_d300fpc9307a_8 },
	{.delay = 0, .data_type = TYPE3, .size = 17, .data = data_st7701_d300fpc9307a_9 },
	{.delay = 0, .data_type = TYPE3, .size = 6, .data = data_st7701_d300fpc9307a_10 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_11 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_12 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_13 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_14 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_15 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_16 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_17 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_18 },
	{.delay = 50, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_19 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_20 },
	{.delay = 0, .data_type = TYPE3, .size = 4, .data = data_st7701_d300fpc9307a_21 },
	{.delay = 0, .data_type = TYPE3, .size = 12, .data = data_st7701_d300fpc9307a_22 },
	{.delay = 0, .data_type = TYPE3, .size = 13, .data = data_st7701_d300fpc9307a_23 },
	{.delay = 0, .data_type = TYPE3, .size = 5, .data = data_st7701_d300fpc9307a_24 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_25 },
	{.delay = 0, .data_type = TYPE3, .size = 17, .data = data_st7701_d300fpc9307a_26 },
	{.delay = 0, .data_type = TYPE3, .size = 5, .data = data_st7701_d300fpc9307a_27 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_28 },
	{.delay = 0, .data_type = TYPE3, .size = 17, .data = data_st7701_d300fpc9307a_29 },
	{.delay = 0, .data_type = TYPE3, .size = 3, .data = data_st7701_d300fpc9307a_30 },
	{.delay = 0, .data_type = TYPE3, .size = 8, .data = data_st7701_d300fpc9307a_31 },
	{.delay = 0, .data_type = TYPE3, .size = 17, .data = data_st7701_d300fpc9307a_32 },
	{.delay = 50, .data_type = TYPE3, .size = 7, .data = data_st7701_d300fpc9307a_33 },
	{.delay = 0, .data_type = TYPE1, .size = 1, .data = data_st7701_d300fpc9307a_34 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_35 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_st7701_d300fpc9307a_36 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif
