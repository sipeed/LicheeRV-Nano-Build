#ifndef _MIPI_TX_PARAM_ST_7701_RFA6400EAWHMNN_H_
#define _MIPI_TX_PARAM_ST_7701_RFA6400EAWHMNN_H_

#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

#define ST7701_RFA6400EAWHMNN_VACT	480
#define ST7701_RFA6400EAWHMNN_VSA	10
#define ST7701_RFA6400EAWHMNN_VBP	20
#define ST7701_RFA6400EAWHMNN_VFP	20

#define ST7701_RFA6400EAWHMNN_HACT	480
#define ST7701_RFA6400EAWHMNN_HSA	32
#define ST7701_RFA6400EAWHMNN_HBP	80
#define ST7701_RFA6400EAWHMNN_HFP	48

#define RFA6400_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_st7701_rfa6400eawhmnn_480x480 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_RFA6400EAWHMNN_HSA,
		.vid_hbp_pixels = ST7701_RFA6400EAWHMNN_HBP,
		.vid_hfp_pixels = ST7701_RFA6400EAWHMNN_HFP,
		.vid_hline_pixels = ST7701_RFA6400EAWHMNN_HACT,
		.vid_vsa_lines = ST7701_RFA6400EAWHMNN_VSA,
		.vid_vbp_lines = ST7701_RFA6400EAWHMNN_VBP,
		.vid_vfp_lines = ST7701_RFA6400EAWHMNN_VFP,
		.vid_active_lines = ST7701_RFA6400EAWHMNN_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = RFA6400_PIXEL_CLK(ST7701_RFA6400EAWHMNN),
};

const struct hs_settle_s hs_timing_cfg_st7701_rfa6400eawhmnn_480x480 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_st7701_rfa6400eawhmnn_0[] = { 0x11 };
static CVI_U8 data_st7701_rfa6400eawhmnn_1[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x10 };
static CVI_U8 data_st7701_rfa6400eawhmnn_2[] = { 0xc0, 0x3b, 0x00 };
static CVI_U8 data_st7701_rfa6400eawhmnn_3[] = { 0xc1, 0x0d, 0x02 };
static CVI_U8 data_st7701_rfa6400eawhmnn_4[] = { 0xc2, 0x21, 0x08 };
static CVI_U8 data_st7701_rfa6400eawhmnn_5[] = { 0xcc, 0x10 };
static CVI_U8 data_st7701_rfa6400eawhmnn_6[] = {
	0xb0, 0x00, 0x05, 0x0f, 0x0d, 0x13, 0x07, 0x01, 0x08, 0x09,
	0x1e, 0x05, 0x12, 0x10, 0xa7, 0x2f, 0x18
};
static CVI_U8 data_st7701_rfa6400eawhmnn_7[] = {
	0xb1, 0x00, 0x0f, 0x17, 0x0c, 0x0d, 0x05, 0x01, 0x08, 0x08,
	0x1e, 0x05, 0x13, 0x11, 0xa7, 0x2f, 0x18
};
static CVI_U8 data_st7701_rfa6400eawhmnn_8[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x11 };
static CVI_U8 data_st7701_rfa6400eawhmnn_9[] = { 0xb0, 0x4d };
static CVI_U8 data_st7701_rfa6400eawhmnn_10[] = { 0xb1, 0x4f };
static CVI_U8 data_st7701_rfa6400eawhmnn_11[] = { 0xb2, 0x07 };
static CVI_U8 data_st7701_rfa6400eawhmnn_12[] = { 0xb3, 0x80 };
static CVI_U8 data_st7701_rfa6400eawhmnn_13[] = { 0xb5, 0x47 };
static CVI_U8 data_st7701_rfa6400eawhmnn_14[] = { 0xb7, 0x85 };
static CVI_U8 data_st7701_rfa6400eawhmnn_15[] = { 0xb8, 0x21 };
static CVI_U8 data_st7701_rfa6400eawhmnn_16[] = { 0xb9, 0x10 };
static CVI_U8 data_st7701_rfa6400eawhmnn_17[] = { 0xc1, 0x78 };
static CVI_U8 data_st7701_rfa6400eawhmnn_18[] = { 0xc2, 0x78 };
static CVI_U8 data_st7701_rfa6400eawhmnn_19[] = { 0xd0, 0x88 };
static CVI_U8 data_st7701_rfa6400eawhmnn_20[] = { 0xe0, 0x00, 0x00, 0x02 };
static CVI_U8 data_st7701_rfa6400eawhmnn_21[] = {
	0xe1, 0x08, 0x00, 0x0a, 0x00, 0x07, 0x00, 0x09, 0x00, 0x00,
	0x33, 0x33
};
static CVI_U8 data_st7701_rfa6400eawhmnn_22[] = {
	0xe2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};
static CVI_U8 data_st7701_rfa6400eawhmnn_23[] = { 0xe3, 0x00, 0x00, 0x33, 0x33 };
static CVI_U8 data_st7701_rfa6400eawhmnn_24[] = { 0xe4, 0x44, 0x44 };
static CVI_U8 data_st7701_rfa6400eawhmnn_25[] = {
	0xe5, 0x0e, 0x2d, 0xa0, 0xa0, 0x10, 0x2d, 0xa0, 0xa0, 0x0a,
	0x2d, 0xa0, 0xa0, 0x0c, 0x2d, 0xa0, 0xa0
};
static CVI_U8 data_st7701_rfa6400eawhmnn_26[] = { 0xe6, 0x00, 0x00, 0x33, 0x33 };
static CVI_U8 data_st7701_rfa6400eawhmnn_27[] = { 0xe7, 0x44, 0x44 };
static CVI_U8 data_st7701_rfa6400eawhmnn_28[] = {
	0xe8, 0x0d, 0x2d, 0xa0, 0xa0, 0x0f, 0x2d, 0xa0, 0xa0, 0x09,
	0x2d, 0xa0, 0xa0, 0x0b, 0x2d, 0xa0, 0xa0
};
static CVI_U8 data_st7701_rfa6400eawhmnn_29[] = { 0xeb, 0x02, 0x01, 0xe4, 0xe4, 0x44, 0x00, 0x40 };
static CVI_U8 data_st7701_rfa6400eawhmnn_30[] = { 0xec, 0x02, 0x01 };
static CVI_U8 data_st7701_rfa6400eawhmnn_31[] = {
	0xed, 0xab, 0x89, 0x76, 0x54, 0x01, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x10, 0x45, 0x67, 0x98, 0xba
};
static CVI_U8 data_st7701_rfa6400eawhmnn_32[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x00 };
static CVI_U8 data_st7701_rfa6400eawhmnn_33[] = { 0x11 };
static CVI_U8 data_st7701_rfa6400eawhmnn_34[] = { 0x36, 0x00 };
static CVI_U8 data_st7701_rfa6400eawhmnn_35[] = { 0x29 };

const struct dsc_instr dsi_init_cmds_st7701_rfa6400eawhmnn_480x480[] = {
	{.delay = 120, .data_type = 0x05, .size = 1, .data = data_st7701_rfa6400eawhmnn_0 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_rfa6400eawhmnn_1 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_2 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_3 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_4 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_5 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_rfa6400eawhmnn_6 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_rfa6400eawhmnn_7 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_rfa6400eawhmnn_8 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_9 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_10 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_11 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_12 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_13 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_14 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_15 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_16 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_17 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_18 },
	{.delay = 100, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_19 },
	{.delay = 0, .data_type = 0x39, .size = 4, .data = data_st7701_rfa6400eawhmnn_20 },
	{.delay = 0, .data_type = 0x39, .size = 12, .data = data_st7701_rfa6400eawhmnn_21 },
	{.delay = 0, .data_type = 0x39, .size = 14, .data = data_st7701_rfa6400eawhmnn_22 },
	{.delay = 0, .data_type = 0x39, .size = 5, .data = data_st7701_rfa6400eawhmnn_23 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_24 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_rfa6400eawhmnn_25 },
	{.delay = 0, .data_type = 0x39, .size = 5, .data = data_st7701_rfa6400eawhmnn_26 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_27 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_rfa6400eawhmnn_28 },
	{.delay = 0, .data_type = 0x39, .size = 8, .data = data_st7701_rfa6400eawhmnn_29 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_rfa6400eawhmnn_30 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_rfa6400eawhmnn_31 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_rfa6400eawhmnn_32 },
	{.delay = 0, .data_type = 0x05, .size = 1, .data = data_st7701_rfa6400eawhmnn_33 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_rfa6400eawhmnn_34 },
	{.delay = 0, .data_type = 0x05, .size = 1, .data = data_st7701_rfa6400eawhmnn_35 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif
