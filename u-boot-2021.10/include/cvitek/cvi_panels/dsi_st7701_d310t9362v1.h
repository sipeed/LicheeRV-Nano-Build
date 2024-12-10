/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _MIPI_TX_PARAM_ST7701_D310T9362V1_H_
#define _MIPI_TX_PARAM_ST7701_D310T9362V1_H_

// st7701_d310t9362v1
// ST7701_D310T9362V1

#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

#define PANEL_NAME "ST7701_D310T9362V1"

#define ST7701_D310T9362V1_VACT		800
#define ST7701_D310T9362V1_VSA		10
#define ST7701_D310T9362V1_VBP		28
#define ST7701_D310T9362V1_VFP		8

#define ST7701_D310T9362V1_HACT		480
#define ST7701_D310T9362V1_HSA		10
#define ST7701_D310T9362V1_HBP		43
#define ST7701_D310T9362V1_HFP		48

#define ST7701_D310T9362V1_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

const struct combo_dev_cfg_s dev_cfg_st7701_d310t9362v1_480x800 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_D310T9362V1_HSA,
		.vid_hbp_pixels = ST7701_D310T9362V1_HBP,
		.vid_hfp_pixels = ST7701_D310T9362V1_HFP,
		.vid_hline_pixels = ST7701_D310T9362V1_HACT,
		.vid_vsa_lines = ST7701_D310T9362V1_VSA,
		.vid_vbp_lines = ST7701_D310T9362V1_VBP,
		.vid_vfp_lines = ST7701_D310T9362V1_VFP,
		.vid_active_lines = ST7701_D310T9362V1_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = ST7701_D310T9362V1_PIXEL_CLK(ST7701_D310T9362V1),
};

const struct hs_settle_s hs_timing_cfg_st7701_d310t9362v1_480x800 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_st7701_d310t9362v1_1[] = {0x01}; // len: 1, delay 120ms
static CVI_U8 data_st7701_d310t9362v1_2[] = {0x11}; // len: 1, delay 120ms
static CVI_U8 data_st7701_d310t9362v1_3[] = {0xFF,0x77,0x01,0x00,0x00,0x11}; // len: 6, 
static CVI_U8 data_st7701_d310t9362v1_4[] = {0xD1,0x11}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_5[] = {0x55,0xb0}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_6[] = {0xFF,0x77,0x01,0x00,0x00,0x10}; // len: 6, 
static CVI_U8 data_st7701_d310t9362v1_7[] = {0xC0,0x63,0x00}; // len: 3, 
static CVI_U8 data_st7701_d310t9362v1_8[] = {0xC1,0x09,0x02}; // len: 3, 
static CVI_U8 data_st7701_d310t9362v1_9[] = {0xC2,0x37,0x08}; // len: 3, 
static CVI_U8 data_st7701_d310t9362v1_10[] = {0xC7,0x04}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_11[] = {0xCC,0x38}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_12[] = {0xB0,0x00,0x11,0x19,0x0C,0x10,0x06,0x07,0x0A,0x09,0x22,0x04,0x10,0x0E,0x28,0x30,0x1C}; // len: 17, 
static CVI_U8 data_st7701_d310t9362v1_13[] = {0xB1,0x00,0x12,0x19,0x0D,0x10,0x04,0x06,0x07,0x08,0x23,0x04,0x12,0x11,0x28,0x30,0x1C}; // len: 17, 
static CVI_U8 data_st7701_d310t9362v1_14[] = {0xFF,0x77,0x01,0x00,0x00,0x11}; // len: 6, 
static CVI_U8 data_st7701_d310t9362v1_15[] = {0xB0,0x4D}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_16[] = {0xB1,0x60}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_17[] = {0xB2,0x07}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_18[] = {0xB3,0x80}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_19[] = {0xB5,0x47}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_20[] = {0xB7,0x8A}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_21[] = {0xB8,0x21}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_22[] = {0xC1,0x78}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_23[] = {0xC2,0x78}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_24[] = {0xD0,0x88}; // len: 2, delay 100ms
static CVI_U8 data_st7701_d310t9362v1_25[] = {0xE0,0x00,0x00,0x02}; // len: 4, 
static CVI_U8 data_st7701_d310t9362v1_26[] = {0xE1,0x01,0xA0,0x03,0xA0,0x02,0xA0,0x04,0xA0,0x00,0x44,0x44}; // len: 12, 
static CVI_U8 data_st7701_d310t9362v1_27[] = {0xE2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // len: 13, 
static CVI_U8 data_st7701_d310t9362v1_28[] = {0xE3,0x00,0x00,0x33,0x33}; // len: 5, 
static CVI_U8 data_st7701_d310t9362v1_29[] = {0xE4,0x44,0x44}; // len: 3, 
static CVI_U8 data_st7701_d310t9362v1_30[] = {0xE5,0x01,0x26,0xA0,0xA0,0x03,0x28,0xA0,0xA0,0x05,0x2A,0xA0,0xA0,0x07,0x2C,0xA0,0xA0}; // len: 17, 
static CVI_U8 data_st7701_d310t9362v1_31[] = {0xE6,0x00,0x00,0x33,0x33}; // len: 5, 
static CVI_U8 data_st7701_d310t9362v1_32[] = {0xE7,0x44,0x44}; // len: 3, 
static CVI_U8 data_st7701_d310t9362v1_33[] = {0xE8,0x02,0x26,0xA0,0xA0,0x04,0x28,0xA0,0xA0,0x06,0x2A,0xA0,0xA0,0x08,0x2C,0xA0,0xA0}; // len: 17, 
static CVI_U8 data_st7701_d310t9362v1_34[] = {0xEB,0x00,0x01,0xE4,0xE4,0x44,0x00,0x40}; // len: 8, 
static CVI_U8 data_st7701_d310t9362v1_35[] = {0xED,0xFF,0xF7,0x65,0x4F,0x0B,0xA1,0xCF,0xFF,0xFF,0xFC,0x1A,0xB0,0xF4,0x56,0x7F,0xFF}; // len: 17, 
static CVI_U8 data_st7701_d310t9362v1_36[] = {0xFF,0x77,0x01,0x00,0x00,0x00}; // len: 6, 
static CVI_U8 data_st7701_d310t9362v1_37[] = {0x36,0x10}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_38[] = {0x3A,0x55}; // len: 2, 
static CVI_U8 data_st7701_d310t9362v1_39[] = {0x29}; // len: 1, 

// LCD_Rotate_180
// static CVI_U8 data_st7701_d310t9362v1_40[] = {0xFF,0x77,0x01,0x00,0x00,0x10}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_41[] = {0xC7,0x00}; // len: 2, 
// static CVI_U8 data_st7701_d310t9362v1_42[] = {0xFF,0x77,0x01,0x00,0x00,0x00}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_43[] = {0x36,0x00}; // len: 2, 
// static CVI_U8 data_st7701_d310t9362v1_44[] = {0xFF,0x77,0x01,0x00,0x00,0x11}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_45[] = {0xB1,0x5B}; // len: 2,

// LCD_Rotate_0
// static CVI_U8 data_st7701_d310t9362v1_46[] = {0xFF,0x77,0x01,0x00,0x00,0x10}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_47[] = {0xC7,0x04}; // len: 2, 
// static CVI_U8 data_st7701_d310t9362v1_48[] = {0xFF,0x77,0x01,0x00,0x00,0x00}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_49[] = {0x36,0x10}; // len: 2, 
// static CVI_U8 data_st7701_d310t9362v1_50[] = {0xFF,0x77,0x01,0x00,0x00,0x11}; // len: 6, 
// static CVI_U8 data_st7701_d310t9362v1_51[] = {0xB1,0x60}; // len: 2, 


// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29

const struct dsc_instr dsi_init_cmds_st7701_d310t9362v1_480x800[] = {
	{.delay = 120, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_st7701_d310t9362v1_1 },
	{.delay = 120, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_st7701_d310t9362v1_2 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_3 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_4 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_5 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_6 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 3, .data = data_st7701_d310t9362v1_7 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 3, .data = data_st7701_d310t9362v1_8 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 3, .data = data_st7701_d310t9362v1_9 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_10 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_11 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 17, .data = data_st7701_d310t9362v1_12 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 17, .data = data_st7701_d310t9362v1_13 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_14 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_15 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_16 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_17 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_18 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_19 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_20 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_21 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_22 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_23 },
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_24 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 4, .data = data_st7701_d310t9362v1_25 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 12, .data = data_st7701_d310t9362v1_26 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 13, .data = data_st7701_d310t9362v1_27 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 5, .data = data_st7701_d310t9362v1_28 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 3, .data = data_st7701_d310t9362v1_29 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 17, .data = data_st7701_d310t9362v1_30 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 5, .data = data_st7701_d310t9362v1_31 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 3, .data = data_st7701_d310t9362v1_32 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 17, .data = data_st7701_d310t9362v1_33 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 8, .data = data_st7701_d310t9362v1_34 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 17, .data = data_st7701_d310t9362v1_35 },
	{.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_36 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_37 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_38 },
	{.delay = 0, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_st7701_d310t9362v1_39 },

	/* LCD_Rotate_180 */
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_40 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_41 },
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_42 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_43 },
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_44 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_45 },

	/* LCD_Rotate_0 */
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_46 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_47 },
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_48 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_49 },
	// {.delay = 0, .data_type = TYPE3_GENERIC_LONG_WRITE, .size = 6, .data = data_st7701_d310t9362v1_50 },
	// {.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_d310t9362v1_51 },
};


#else
#error "_MIPI_TX_PARAM_ST_7701_H_ multi-delcaration!!"
#endif // _MIPI_TX_PARAM_ST7701_D310T9362V1_H_
