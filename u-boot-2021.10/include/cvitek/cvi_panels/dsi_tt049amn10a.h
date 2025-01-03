/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _MIPI_TX_PARAM_TT049_1920x1080_H_
#define _MIPI_TX_PARAM_TT049_1920x1080_H_

// TT049_1920x1080
// TT049_1920x1080

#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

//#define PANEL_NAME "TT049-1920x1080"

#define TT049_1920x1080_VACT		1080
#define TT049_1920x1080_VSA		2
#define TT049_1920x1080_VBP		14
#define TT049_1920x1080_VFP		16

#define TT049_1920x1080_HACT		1920
#define TT049_1920x1080_HSA		4
#define TT049_1920x1080_HBP		60
#define TT049_1920x1080_HFP		88

#define TT049_1920x1080_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

const struct combo_dev_cfg_s dev_cfg_tt049amn10a_1920x1080 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = TT049_1920x1080_HSA,
		.vid_hbp_pixels = TT049_1920x1080_HBP,
		.vid_hfp_pixels = TT049_1920x1080_HFP,
		.vid_hline_pixels = TT049_1920x1080_HACT,
		.vid_vsa_lines = TT049_1920x1080_VSA,
		.vid_vbp_lines = TT049_1920x1080_VBP,
		.vid_vfp_lines = TT049_1920x1080_VFP,
		.vid_active_lines = TT049_1920x1080_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = TT049_1920x1080_PIXEL_CLK(TT049_1920x1080),
};

const struct hs_settle_s hs_timing_cfg_tt049amn10a_1920x1080 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_TT049_1920x1080_1[]  = {0x03, 0x00}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_2[]  = {0x53, 0x2D}; // type 0x39, len 2

static CVI_U8 data_TT049_1920x1080_3[]  = {0x51, 0xFF, 0x01}; // type 0x39, len 3
static CVI_U8 data_TT049_1920x1080_4[]  = {0x90, 0x01, 0xE0, 0xE0, 0x0E, 0x00, 0x31};
static CVI_U8 data_TT049_1920x1080_5[]  = {0x81, 0x03, 0x04, 0x00, 0x10, 0x00, 0x10, 0x00};
static CVI_U8 data_TT049_1920x1080_6[]  = {0x82, 0x03, 0x04, 0x00, 0x10, 0x00, 0x10, 0x00};
static CVI_U8 data_TT049_1920x1080_7[]  = {0x35, 0x00}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_8[]  = {0x26, 0x20}; // type 0x39, len 2

static CVI_U8 data_TT049_1920x1080_9[]  = {0xF0, 0xAA, 0x13}; // type 0x39, len 3
static CVI_U8 data_TT049_1920x1080_10[] = {0xD2, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3C, 0x38, 0x34, 0x30, 0x28, 0x20, 0x18, 0x10, 0x0C, 0x08, 0x04, 0x00, 0x00}; // type 0x39, len 19
static CVI_U8 data_TT049_1920x1080_11[] = {0xD3, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3C, 0x38, 0x34, 0x30, 0x28, 0x20, 0x18, 0x10, 0x0C, 0x08, 0x04, 0x00, 0x00}; // type 0x39, len 19

static CVI_U8 data_TT049_1920x1080_12[] = {0xF0, 0xAA, 0x12}; // type 0x39, len 3
static CVI_U8 data_TT049_1920x1080_13[] = {0x65, 0x02}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_14[] = {0xD2, 0x05}; // type 0x39, len 2

static CVI_U8 data_TT049_1920x1080_15[] = {0xFF, 0x5A, 0x81}; // type 0x39, len 3
static CVI_U8 data_TT049_1920x1080_16[] = {0x65, 0x2F}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_17[] = {0xF2, 0x01}; // type 0x39, len 2

static CVI_U8 data_TT049_1920x1080_18[] = {0xFF, 0x5A, 0x81}; // type 0x39, len 3
static CVI_U8 data_TT049_1920x1080_19[] = {0x65, 0x16}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_20[] = {0xF9, 0x00, 0x52, 0x59, 0x60, 0x67, 0x6B, 0x70, 0x75, 0x79, 0x7E, 0x83, 0x87, 0x8C, 0x91, 0x95, 0x9A, 0x9F, 0xA6}; // type 0x39, len 19
static CVI_U8 data_TT049_1920x1080_21[] = {0x65, 0x05}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_22[] = {0xF2, 0x23}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_23[] = {0x65, 0x0A}; // type 0x39, len 2
static CVI_U8 data_TT049_1920x1080_24[] = {0xF2, 0x00}; // type 0x39, len 2

static CVI_U8 data_TT049_1920x1080_25[] = {0x11}; // type 0x05
// delayms 200
static CVI_U8 data_TT049_1920x1080_26[] = {0x29}; // type 0x05

// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29

const struct dsc_instr dsi_init_cmds_tt049amn10a_1920x1080[] = {
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 1, .data = data_TT049_1920x1080_1 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 1, .data = data_TT049_1920x1080_2 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_TT049_1920x1080_3 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_4 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_5 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_TT049_1920x1080_6 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_TT049_1920x1080_7 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_TT049_1920x1080_8 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_TT049_1920x1080_9 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_10 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_11 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_TT049_1920x1080_12 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_TT049_1920x1080_13 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_TT049_1920x1080_14 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_15 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_16 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_17 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_18 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_19 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_20 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_21 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_22 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_23 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 2, .data = data_TT049_1920x1080_24 },
	{.delay = 0, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 4, .data = data_TT049_1920x1080_25 },
	{.delay = 200, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 12, .data = data_TT049_1920x1080_26 },
};

#else
#error "_MIPI_TX_PARAM_TT049_1920x1080_H_ multi-delcaration!!"
#endif // _MIPI_TX_PARAM_TT049_1920x1080_H_
