#ifndef _MIPI_TX_PARAM_ST_7701_DXQ5D0019B480854_H_
#define _MIPI_TX_PARAM_ST_7701_DXQ5D0019B480854_H_

#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>


/*
#define ST7701_DXQ5D0019B480854_VACT	854
#define ST7701_DXQ5D0019B480854_VSA		2
#define ST7701_DXQ5D0019B480854_VBP		20
#define ST7701_DXQ5D0019B480854_VFP		20

#define ST7701_DXQ5D0019B480854_HACT	480
#define ST7701_DXQ5D0019B480854_HSA		10
#define ST7701_DXQ5D0019B480854_HBP		40
#define ST7701_DXQ5D0019B480854_HFP		40
*/

#define ST7701_DXQ5D0019B480854_VACT	854
#define ST7701_DXQ5D0019B480854_VSA		10
#define ST7701_DXQ5D0019B480854_VBP		20
#define ST7701_DXQ5D0019B480854_VFP		3

#define ST7701_DXQ5D0019B480854_HACT	480
#define ST7701_DXQ5D0019B480854_HSA		48
#define ST7701_DXQ5D0019B480854_HBP		72
#define ST7701_DXQ5D0019B480854_HFP		24

#define DXQ_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_st7701_480x854dxq = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = ST7701_DXQ5D0019B480854_HSA,
		.vid_hbp_pixels = ST7701_DXQ5D0019B480854_HBP,
		.vid_hfp_pixels = ST7701_DXQ5D0019B480854_HFP,
		.vid_hline_pixels = ST7701_DXQ5D0019B480854_HACT,
		.vid_vsa_lines = ST7701_DXQ5D0019B480854_VSA,
		.vid_vbp_lines = ST7701_DXQ5D0019B480854_VBP,
		.vid_vfp_lines = ST7701_DXQ5D0019B480854_VFP,
		.vid_active_lines = ST7701_DXQ5D0019B480854_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = DXQ_PIXEL_CLK(ST7701_DXQ5D0019B480854),
};

const struct hs_settle_s hs_timing_cfg_st7701_480x854dxq = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_st7701_dxq5d0019b480854_0[] = {0x11, 0x00 }; // turn off sleep mode
//{REGFLAG_DELAY, 60, {} },
static CVI_U8 data_st7701_dxq5d0019b480854_1[] = {0xff,0x77,0x01,0x00,0x00,0x10};
static CVI_U8 data_st7701_dxq5d0019b480854_2[] = {0xC0,0xE9,0x03};
static CVI_U8 data_st7701_dxq5d0019b480854_3[] = {0xC1,0x08,0x02};
static CVI_U8 data_st7701_dxq5d0019b480854_4[] = {0xC2,0x31,0x08};
static CVI_U8 data_st7701_dxq5d0019b480854_5[] = {0xCC,0x10};
static CVI_U8 data_st7701_dxq5d0019b480854_6[] = {0xB0,0x00,0x0B,0x10,0x0D,0x11,0x06,0x01,0x08,0x08,0x1D,0x04,0x10,0x10,0x27,0x30,0x19};
static CVI_U8 data_st7701_dxq5d0019b480854_7[] = {0xB1,0x00,0x0B,0x14,0x0C,0x11,0x05,0x03,0x08,0x08,0x20,0x04,0x13,0x10,0x28,0x30,0x19};
//------------------------------------End Gamma etting------------------------------------------//
//-------------------------------End Display Control setting------------------------------------//
//-------------------------------------Bank0 Setting End-----------------------------------------//
//---------------------------------------Bank1 setting----------------------------------------------//
//---------------------------- Power Control Registers Initial ---------------------------------//
static CVI_U8 data_st7701_dxq5d0019b480854_8[] = {0xff,0x77,0x01,0x00,0x00,0x11};
static CVI_U8 data_st7701_dxq5d0019b480854_9[] = {0xB0,0x35};
//---------------------------------------Vcom setting----------------------------------------------//
static CVI_U8 data_st7701_dxq5d0019b480854_10[] = {0xB1,0x38};
//-----------------------------------End Vcom Setting---------------------------------------------//
static CVI_U8 data_st7701_dxq5d0019b480854_11[] = {0xB2,0x02};
static CVI_U8 data_st7701_dxq5d0019b480854_12[] = {0xB3,0x80};
static CVI_U8 data_st7701_dxq5d0019b480854_13[] = {0xB5,0x4E};
static CVI_U8 data_st7701_dxq5d0019b480854_14[] = {0xB7,0x85};
static CVI_U8 data_st7701_dxq5d0019b480854_15[] = {0xB8,0x20};
static CVI_U8 data_st7701_dxq5d0019b480854_16[] = {0xB9,0x10};
static CVI_U8 data_st7701_dxq5d0019b480854_17[] = {0xC1,0x78};
static CVI_U8 data_st7701_dxq5d0019b480854_18[] = {0xC2,0x78};
static CVI_U8 data_st7701_dxq5d0019b480854_19[] = {0xD0,0x88};
//-----------------------------End Power Control Registers Initial ----------------------------//
//Delayms (100);
//-------------------------------------GIP Setting-------------------------------------------------//
static CVI_U8 data_st7701_dxq5d0019b480854_20[] = {0xE0,0x00,0x00,0x02};
static CVI_U8 data_st7701_dxq5d0019b480854_21[] = {0xE1,0x05,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x20,0x20};
static CVI_U8 data_st7701_dxq5d0019b480854_22[] = {0xE2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_23[] = {0xE3,0x00,0x00,0x33,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_24[] = {0xE4,0x22,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_25[] = {0xE5,0x07,0x34,0xA0,0xA0,0x05,0x34,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_26[] = {0xE6,0x00,0x00,0x33,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_27[] = {0xE7,0x22,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_28[] = {0xE8,0x06,0x34,0xA0,0xA0,0x04,0x34,0xA0,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_29[] = {0xEB,0x02,0x00,0x10,0x10,0x00,0x00,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_30[] = {0xEC,0x02,0x00};
static CVI_U8 data_st7701_dxq5d0019b480854_31[] = {0xED,0xAA,0x54,0x0B,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFB,0xB0,0x45,0xAA};
//-----------------------------------------End GIP Setting-----------------------------------------//
//--------------------------- Power Control Registers Initial End------------------------------//
//-------------------------------------Bank1 Setting------------------------------------------------//
//-----------------------------------//
//	Example 1: Read - Direct Procedure
//-----------------------------------//
static CVI_U8 data_st7701_dxq5d0019b480854_32[] = {0xFF,0x77,0x01,0x00,0x00,0x11};
//DCS_Long_Read_NP(0xA1,3,BUFFER+0);
//DCS_Long_Read_NP(0xDA,1,BUFFER+0);
//DCS_Long_Read_NP(0xDB,1,BUFFER+1);
//DCS_Long_Read_NP(0xDC,1,BUFFER+2);

//DCS_Short_Write_1P(0x00);
static CVI_U8 data_st7701_dxq5d0019b480854_33[] = {0x00};
//Delay(500);
//bist模式
//彩条
//static CVI_U8 data_st7701_dxq5d0019b480854_34[] = {0xFF,0x77,0x01,0x00,0x00,0x12};
//static CVI_U8 data_st7701_dxq5d0019b480854_35[] = {0xd1,0x81,0x10,0x03,0x03,0x08,0x01,0xA0,0x01,0xe0,0xB0,0x01,0xe0,0x03,0x20};
//static CVI_U8 data_st7701_dxq5d0019b480854_36[] = {0xd2,0x08};///彩条
//static CVI_U8 data_st7701_dxq5d0019b480854_37[] = {0xFF,0x77,0x01,0x00,0x00,0x10};
//static CVI_U8 data_st7701_dxq5d0019b480854_38[] = {0xC2,0x31,0x08};
//DCS_Short_Write_1P(0x29,0x00);
static CVI_U8 data_st7701_dxq5d0019b480854_39[] = {0x29,0x00};
//Delay(100);
static CVI_U8 data_st7701_dxq5d0019b480854_40[] = {0x35,0x00};

// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1_DCS_SHORT_WRITE 0x05
#define TYPE2_DCS_SHORT_WRITE 0x15
#define TYPE3_DCS_LONG_WRITE 0x39
#define TYPE3_GENERIC_LONG_WRITE 0x29
const struct dsc_instr dsi_init_cmds_st7701_480x854dxq[] = {
	{.delay = 60, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_0 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_dxq5d0019b480854_1 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_2 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_3 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_4 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_5 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_st7701_dxq5d0019b480854_6 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_st7701_dxq5d0019b480854_7 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_dxq5d0019b480854_8 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_9 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_10 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_11 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_12 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_13 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_14 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_15 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_16 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_17 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_18 },
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_19 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 4, .data = data_st7701_dxq5d0019b480854_20 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 12, .data = data_st7701_dxq5d0019b480854_21 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 14, .data = data_st7701_dxq5d0019b480854_22 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_st7701_dxq5d0019b480854_23 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_24 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_st7701_dxq5d0019b480854_25 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 5, .data = data_st7701_dxq5d0019b480854_26 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_27 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_st7701_dxq5d0019b480854_28 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 8, .data = data_st7701_dxq5d0019b480854_29 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_30 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 17, .data = data_st7701_dxq5d0019b480854_31 },
	{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_dxq5d0019b480854_32 },
	{.delay = 255, .data_type = TYPE1_DCS_SHORT_WRITE, .size = 1, .data = data_st7701_dxq5d0019b480854_33 },
/* bist mode vvv */
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_dxq5d0019b480854_34 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 15, .data = data_st7701_dxq5d0019b480854_35 },
	//{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_36 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 6, .data = data_st7701_dxq5d0019b480854_37 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_38 },
	//{.delay = 0, .data_type = TYPE3_DCS_LONG_WRITE, .size = 3, .data = data_st7701_dxq5d0019b480854_38 },
/* bist mode ^^^ */
	{.delay = 100, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_39 },
	{.delay = 0, .data_type = TYPE2_DCS_SHORT_WRITE, .size = 2, .data = data_st7701_dxq5d0019b480854_40 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif
