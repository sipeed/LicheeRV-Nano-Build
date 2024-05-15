#ifndef _MIPI_TX_PARAM_ST_7701_DXQ5D0019_V0_H_
#define _MIPI_TX_PARAM_ST_7701_DXQ5D0019_V0_H_

#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

/*
#define ST7701_DXQ5D0019_V0_VACT	854
#define ST7701_DXQ5D0019_V0_VSA		2
#define ST7701_DXQ5D0019_V0_VBP		20
#define ST7701_DXQ5D0019_V0_VFP		20

#define ST7701_DXQ5D0019_V0_HACT	480
#define ST7701_DXQ5D0019_V0_HSA		10
#define ST7701_DXQ5D0019_V0_HBP		40
#define ST7701_DXQ5D0019_V0_HFP		40
*/

#define ST7701_DXQ5D0019_V0_VACT	854
#define ST7701_DXQ5D0019_V0_VSA		10
#define ST7701_DXQ5D0019_V0_VBP		20
#define ST7701_DXQ5D0019_V0_VFP		3

#define ST7701_DXQ5D0019_V0_HACT	480
#define ST7701_DXQ5D0019_V0_HSA		48
#define ST7701_DXQ5D0019_V0_HBP		72
#define ST7701_DXQ5D0019_V0_HFP		24

#define DXQ_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_st7701_480x854dxq_V0 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_DXQ5D0019_V0_HSA,
		.vid_hbp_pixels = ST7701_DXQ5D0019_V0_HBP,
		.vid_hfp_pixels = ST7701_DXQ5D0019_V0_HFP,
		.vid_hline_pixels = ST7701_DXQ5D0019_V0_HACT,
		.vid_vsa_lines = ST7701_DXQ5D0019_V0_VSA,
		.vid_vbp_lines = ST7701_DXQ5D0019_V0_VBP,
		.vid_vfp_lines = ST7701_DXQ5D0019_V0_VFP,
		.vid_active_lines = ST7701_DXQ5D0019_V0_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = DXQ_PIXEL_CLK(ST7701_DXQ5D0019_V0),
};

const struct hs_settle_s hs_timing_cfg_st7701_480x854dxq_V0 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_ST7701_DXQ5D0019_V0_0[] = {0x11, 0x00 }; // turn off sleep mode
//{REGFLAG_DELAY, 60, {} },
static CVI_U8 data_ST7701_DXQ5D0019_V0_1[] = {0xFF,0x77,0x01,0x00,0x00,0x10};
static CVI_U8 data_ST7701_DXQ5D0019_V0_2[] = {0xC0,0xE9,0x03};
static CVI_U8 data_ST7701_DXQ5D0019_V0_3[] = {0xC1,0x0A,0x02};
static CVI_U8 data_ST7701_DXQ5D0019_V0_4[] = {0xC2,0x37,0x08};
static CVI_U8 data_ST7701_DXQ5D0019_V0_5[] = {0xCC,0x30};
static CVI_U8 data_ST7701_DXQ5D0019_V0_6[] = {0xB0,0x00,0x12,0x1A,0x0D,0x11,0x07,0x0C,0x0A,0x09,0x26,0x05,0x11,0x10,0x2B,0x33,0x1B};
static CVI_U8 data_ST7701_DXQ5D0019_V0_7[] = {0xB1,0x00,0x12,0x1A,0x0D,0x11,0x06,0x0B,0x07,0x08,0x26,0x03,0x11,0x0F,0x2B,0x33,0x1B};
//------------------------------------End Gamma etting------------------------------------------//
//-------------------------------End Display Control setting------------------------------------//
//-------------------------------------Bank0 Setting End-----------------------------------------//
//---------------------------------------Bank1 setting----------------------------------------------//
//---------------------------- Power Control Registers Initial ---------------------------------//
static CVI_U8 data_ST7701_DXQ5D0019_V0_8[] = {0xFF,0x77,0x01,0x00,0x00,0x11};
static CVI_U8 data_ST7701_DXQ5D0019_V0_9[] = {0xB0,0x4D};
//---------------------------------------Vcom setting----------------------------------------------//
static CVI_U8 data_ST7701_DXQ5D0019_V0_10[] = {0xB1,0x2E};
//-----------------------------------End Vcom Setting---------------------------------------------//
static CVI_U8 data_ST7701_DXQ5D0019_V0_11[] = {0xB2,0x07};
static CVI_U8 data_ST7701_DXQ5D0019_V0_12[] = {0xB3,0x80};
static CVI_U8 data_ST7701_DXQ5D0019_V0_13[] = {0xB5,0x47};
static CVI_U8 data_ST7701_DXQ5D0019_V0_14[] = {0xB7,0x85};
static CVI_U8 data_ST7701_DXQ5D0019_V0_15[] = {0xB8,0x21};
static CVI_U8 data_ST7701_DXQ5D0019_V0_16[] = {0xB9,0x10};
static CVI_U8 data_ST7701_DXQ5D0019_V0_17[] = {0xC1,0x78};
static CVI_U8 data_ST7701_DXQ5D0019_V0_18[] = {0xC2,0x78};
static CVI_U8 data_ST7701_DXQ5D0019_V0_19[] = {0xD0,0x88};
//-----------------------------End Power Control Registers Initial ----------------------------//
//Delayms (100);
//-------------------------------------GIP Setting-------------------------------------------------//
static CVI_U8 data_ST7701_DXQ5D0019_V0_20[] = {0xE0,0x00,0x00,0x02};
static CVI_U8 data_ST7701_DXQ5D0019_V0_21[] = {0xE1,0x04,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x20,0x20};
static CVI_U8 data_ST7701_DXQ5D0019_V0_22[] = {0xE2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_23[] = {0xE3,0x00,0x00,0x33,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_24[] = {0xE4,0x22,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_25[] = {0xE5,0x04,0x5C,0xA0,0xA0,0x06,0x5C,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_26[] = {0xE6,0x00,0x00,0x33,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_27[] = {0xE7,0x22,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_28[] = {0xE8,0x05,0x5C,0xA0,0xA0,0x07,0x5C,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_29[] = {0xEB,0x02,0x00,0x40,0x40,0x00,0x00,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_30[] = {0xEC,0x00,0x00};
static CVI_U8 data_ST7701_DXQ5D0019_V0_31[] = {0xED,0xFA,0x45,0x0B,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xB0,0x54,0xAF};
//-----------------------------------------End GIP Setting-----------------------------------------//
//--------------------------- Power Control Registers Initial End------------------------------//
//-------------------------------------Bank1 Setting------------------------------------------------//
//-----------------------------------//
//	Example 1: Read - Direct Procedure
//-----------------------------------//
static CVI_U8 data_ST7701_DXQ5D0019_V0_32[] = {0xFF,0x77,0x01,0x00,0x00,0x11};
//DCS_Long_Read_NP(0xA1,3,BUFFER+0);
//DCS_Long_Read_NP(0xDA,1,BUFFER+0);
//DCS_Long_Read_NP(0xDB,1,BUFFER+1);
//DCS_Long_Read_NP(0xDC,1,BUFFER+2);

//DCS_Short_Write_1P(0x00);
static CVI_U8 data_ST7701_DXQ5D0019_V0_33[] = {0x00};
//Delay(500);
//bist模式
//彩条
//static CVI_U8 data_ST7701_DXQ5D0019_V0_34[] = {0xFF,0x77,0x01,0x00,0x00,0x12};
//static CVI_U8 data_ST7701_DXQ5D0019_V0_35[] = {0xd1,0x81,0x10,0x03,0x03,0x08,0x01,0xA0,0x01,0xe0,0xB0,0x01,0xe0,0x03,0x20};
//static CVI_U8 data_ST7701_DXQ5D0019_V0_36[] = {0xd2,0x08};///彩条
//static CVI_U8 data_ST7701_DXQ5D0019_V0_37[] = {0xFF,0x77,0x01,0x00,0x00,0x10};
//static CVI_U8 data_ST7701_DXQ5D0019_V0_38[] = {0xC2,0x31,0x08};
//DCS_Short_Write_1P(0x29,0x00);
static CVI_U8 data_ST7701_DXQ5D0019_V0_39[] = {0x29,0x00};
//Delay(100);
static CVI_U8 data_ST7701_DXQ5D0019_V0_40[] = {0x35,0x00};

// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29
const struct dsc_instr dsi_init_cmds_st7701_480x854dxq_V0[] = {
	{.delay = 60, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_0 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_ST7701_DXQ5D0019_V0_1 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_2 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_3 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_4 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_5 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_ST7701_DXQ5D0019_V0_6 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_ST7701_DXQ5D0019_V0_7 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_ST7701_DXQ5D0019_V0_8 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_9 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_10 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_11 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_12 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_13 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_14 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_15 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_16 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_17 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_18 },
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_19 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 4, .data = data_ST7701_DXQ5D0019_V0_20 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 12, .data = data_ST7701_DXQ5D0019_V0_21 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 14, .data = data_ST7701_DXQ5D0019_V0_22 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_ST7701_DXQ5D0019_V0_23 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_24 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_ST7701_DXQ5D0019_V0_25 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_ST7701_DXQ5D0019_V0_26 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_27 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_ST7701_DXQ5D0019_V0_28 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 8, .data = data_ST7701_DXQ5D0019_V0_29 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_30 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_ST7701_DXQ5D0019_V0_31 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_ST7701_DXQ5D0019_V0_32 },
	{.delay = 255, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_ST7701_DXQ5D0019_V0_33 },
/* bist mode vvv */
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_ST7701_DXQ5D0019_V0_34 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 15, .data = data_ST7701_DXQ5D0019_V0_35 },
	//{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_36 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_ST7701_DXQ5D0019_V0_37 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_38 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_ST7701_DXQ5D0019_V0_38 },
/* bist mode ^^^ */
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_39 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_ST7701_DXQ5D0019_V0_40 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif
