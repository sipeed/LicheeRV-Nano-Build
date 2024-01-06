#ifndef _MIPI_TX_PARAM_ST_7701_HD228001C31_H_
#define _MIPI_TX_PARAM_ST_7701_HD228001C31_H_

#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>

// vendor
#define ST7701_HD228001C31_VACT	552
#define ST7701_HD228001C31_VSA		2
#define ST7701_HD228001C31_VBP		20
#define ST7701_HD228001C31_VFP		20

#define ST7701_HD228001C31_HACT		368
#define ST7701_HD228001C31_HSA		8
#define ST7701_HD228001C31_HBP		160
#define ST7701_HD228001C31_HFP		160


// calc
/*
#define ST7701_HD228001C31_VACT	552
#define ST7701_HD228001C31_VSA	10
#define ST7701_HD228001C31_VBP	6
#define ST7701_HD228001C31_VFP	3

#define ST7701_HD228001C31_HACT	368
#define ST7701_HD228001C31_HSA	32
#define ST7701_HD228001C31_HBP	80
#define ST7701_HD228001C31_HFP	48
*/

#define HD22_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_st7701_368x552 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_HD228001C31_HSA,
		.vid_hbp_pixels = ST7701_HD228001C31_HBP,
		.vid_hfp_pixels = ST7701_HD228001C31_HFP,
		.vid_hline_pixels = ST7701_HD228001C31_HACT,
		.vid_vsa_lines = ST7701_HD228001C31_VSA,
		.vid_vbp_lines = ST7701_HD228001C31_VBP,
		.vid_vfp_lines = ST7701_HD228001C31_VFP,
		.vid_active_lines = ST7701_HD228001C31_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = HD22_PIXEL_CLK(ST7701_HD228001C31),
};

const struct hs_settle_s hs_timing_cfg_st7701_368x552 = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_st7701_hd228001c31_0[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x13 };
static CVI_U8 data_st7701_hd228001c31_1[] = { 0xef, 0x08 };
static CVI_U8 data_st7701_hd228001c31_2[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x10 };
static CVI_U8 data_st7701_hd228001c31_3[] = { 0xc0, 0x44, 0x00 };
static CVI_U8 data_st7701_hd228001c31_4[] = { 0xc1, 0x0b, 0x02 };
static CVI_U8 data_st7701_hd228001c31_5[] = { 0xc2, 0x07, 0x1f };
static CVI_U8 data_st7701_hd228001c31_6[] = { 0xcc, 0x10 };

static CVI_U8 data_st7701_hd228001c31_7[] = {
	0xb0, 0x0F,0x1E,0x25,0x0D,0x11,0x06,0x12,0x08,0x08,0x2A,0x05,0x12,0x10,0x2B,0x32,0x1F
};
static CVI_U8 data_st7701_hd228001c31_8[] = {
	0xb1, 0x0F,0x1E,0x25,0x0D,0x11,0x05,0x12,0x08,0x08,0x2B,0x05,0x12,0x10,0x2B,0x32,0x1F
};
static CVI_U8 data_st7701_hd228001c31_9[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x11 };
static CVI_U8 data_st7701_hd228001c31_10[] = { 0xb0, 0x35 };
static CVI_U8 data_st7701_hd228001c31_11[] = { 0xb1, 0x45 };
static CVI_U8 data_st7701_hd228001c31_12[] = { 0xb2, 0x87 };
static CVI_U8 data_st7701_hd228001c31_13[] = { 0xb3, 0x80 };
static CVI_U8 data_st7701_hd228001c31_14[] = { 0xb5, 0x49 };
static CVI_U8 data_st7701_hd228001c31_15[] = { 0xb7, 0x85 };
static CVI_U8 data_st7701_hd228001c31_16[] = { 0xb8, 0x11 };
//static CVI_U8 data_st7701_hd228001c31_17[] = { 0xb9, 0x10, 0x1f };
//static CVI_U8 data_st7701_hd228001c31_18[] = { 0xbb, 0x03 };
static CVI_U8 data_st7701_hd228001c31_19[] = { 0xc0, 0x07 };
static CVI_U8 data_st7701_hd228001c31_20[] = { 0xc1, 0x78 };
static CVI_U8 data_st7701_hd228001c31_21[] = { 0xc2, 0x78 };
static CVI_U8 data_st7701_hd228001c31_22[] = { 0xc8, 0xbe };
static CVI_U8 data_st7701_hd228001c31_23[] = { 0xd0, 0x88 };
static CVI_U8 data_st7701_hd228001c31_24[] = {
	0xe0, 0x00, 0x00, 0x02 
};
static CVI_U8 data_st7701_hd228001c31_25[] = {
	0xe1, 0x03,0x30,0x07,0x30,0x02,0x30,0x06,0x30,0x00,0x44,0x44
};
static CVI_U8 data_st7701_hd228001c31_26[] = {
	0xe2, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static CVI_U8 data_st7701_hd228001c31_27[] = { 0xe3, 0x00, 0x00, 0x22, 0x00 };
static CVI_U8 data_st7701_hd228001c31_28[] = { 0xe4, 0x22, 0x00 };
static CVI_U8 data_st7701_hd228001c31_29[] = {
	0xe5, 0x0A,0x34,0x30,0xE0,0x08,0x32,0x30,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static CVI_U8 data_st7701_hd228001c31_30[] = { 0xe6, 0x00, 0x00, 0x22, 0x00 };
static CVI_U8 data_st7701_hd228001c31_31[] = { 0xe7, 0x22, 0x00 };
static CVI_U8 data_st7701_hd228001c31_32[] = {
	0xe8, 0x09,0x33,0x30,0xE0,0x07,0x31,0x30,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static CVI_U8 data_st7701_hd228001c31_33[] = {
	0xeb, 0x00,0x01,0x10,0x10,0x11,0x00,0x00
};
static CVI_U8 data_st7701_hd228001c31_34[] = {
	0xed, 0xFF,0xFF,0xF0,0x45,0xBA,0x2F,0xFF,0xFF,0xFF,0xFF,0xF2,0xAB,0x54,0x0F,0xFF,0xFF
};
static CVI_U8 data_st7701_hd228001c31_35[] = {
	0xef, 0x08,0x08,0x08,0x45,0x3F,0x54
};
static CVI_U8 data_st7701_hd228001c31_36[] = { 0xff, 0x77, 0x01, 0x00, 0x00, 0x13 };
static CVI_U8 data_st7701_hd228001c31_37[] = { 0xe8, 0x00, 0x0e};
static CVI_U8 data_st7701_hd228001c31_38[] = { 0x11, 0x00};
static CVI_U8 data_st7701_hd228001c31_39[] = { 0xE0, 0x00, 0x0C };
static CVI_U8 data_st7701_hd228001c31_40[] = { 0xE0, 0x00, 0x00 };
static CVI_U8 data_st7701_hd228001c31_41[] = { 0xFF, 0x77,0x01,0x00,0x00,0x00};
static CVI_U8 data_st7701_hd228001c31_42[] = { 0x29, 0x00};


//Delay(500);
//bist模式
//彩条
/*
static CVI_U8 data_st7701_hd228001c31_43[] = {0xFF,0x77,0x01,0x00,0x00,0x12};
static CVI_U8 data_st7701_hd228001c31_44[] = {0xd1,0x81,0x10,0x03,0x03,0x08,0x01,0xA0,0x01,0xe0,0xB0,0x01,0xe0,0x03,0x20};
// static CVI_U8 data_st7701_hd228001c31_45[] = {0xd2,0x00};///彩条 grey
static CVI_U8 data_st7701_hd228001c31_45[] = {0xd2,0x02};///彩条 red
static CVI_U8 data_st7701_hd228001c31_46[] = {0xFF,0x77,0x01,0x00,0x00,0x10};
static CVI_U8 data_st7701_hd228001c31_47[] = {0xC2,0x31,0x08};
*/


// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29
const struct dsc_instr dsi_init_cmds_st7701_368x552[] = {
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_hd228001c31_0 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_1 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_hd228001c31_2 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_3 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_4 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_5 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_6 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_hd228001c31_7 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_hd228001c31_8 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_hd228001c31_9 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_10 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_11 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_12 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_13 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_14 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_15 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_16 },
	//{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_17 },
	//{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_18 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_19 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_20 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_21 },
	{.delay = 100, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_22 },
	{.delay = 100, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_23 },
	{.delay = 100, .data_type = 0x39, .size = 4, .data = data_st7701_hd228001c31_24 },
	{.delay = 0, .data_type = 0x39, .size = 12, .data = data_st7701_hd228001c31_25 },
	{.delay = 0, .data_type = 0x39, .size = 12, .data = data_st7701_hd228001c31_26 },
	{.delay = 0, .data_type = 0x39, .size = 5, .data = data_st7701_hd228001c31_27 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_28 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_hd228001c31_29 },
	{.delay = 0, .data_type = 0x39, .size = 5, .data = data_st7701_hd228001c31_30 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_31 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_hd228001c31_32 },
	{.delay = 0, .data_type = 0x39, .size = 8, .data = data_st7701_hd228001c31_33 },
	{.delay = 0, .data_type = 0x39, .size = 17, .data = data_st7701_hd228001c31_34 },
	{.delay = 0, .data_type = 0x39, .size = 7, .data = data_st7701_hd228001c31_35 },
	{.delay = 0, .data_type = 0x39, .size = 6, .data = data_st7701_hd228001c31_36 },
	{.delay = 254, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_37 },
	{.delay = 254, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_38 },
	{.delay = 254, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_39 },
	{.delay = 0, .data_type = 0x39, .size = 3, .data = data_st7701_hd228001c31_40 },
	{.delay = 254, .data_type = 0x39, .size = 6, .data = data_st7701_hd228001c31_41 },
	{.delay = 254, .data_type = 0x15, .size = 2, .data = data_st7701_hd228001c31_42 },
	/* bist mode vvv */
/*
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_hd228001c31_43 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 15, .data = data_st7701_hd228001c31_44 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_hd228001c31_45 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_hd228001c31_46 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_hd228001c31_47 },
*/
/* bist mode ^^^ */
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif // _MIPI_TX_PARAM_ST_7701_HD22801C31_H_
