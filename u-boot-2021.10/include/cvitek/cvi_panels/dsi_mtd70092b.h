#ifndef _MIPI_TX_PARAM_MTD700920B_H_
#define _MIPI_TX_PARAM_MTD700920B_H_
// ZCT2133V1
#ifndef __UBOOT__
#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>
#else
#include <cvi_mipi.h>
#endif

#define MTD700920B_VACT	1280
#define MTD700920B_VSA	10
#define MTD700920B_VBP	34
#define MTD700920B_VFP	3

#define MTD700920B_HACT	800
#define MTD700920B_HSA	80
#define MTD700920B_HBP	136
#define MTD700920B_HFP	56

#define ZCT_PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_mtd700920b_800x1280 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_CLK, MIPI_TX_LANE_1, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = MTD700920B_HSA,
		.vid_hbp_pixels = MTD700920B_HBP,
		.vid_hfp_pixels = MTD700920B_HFP,
		.vid_hline_pixels = MTD700920B_HACT,
		.vid_vsa_lines = MTD700920B_VSA,
		.vid_vbp_lines = MTD700920B_VBP,
		.vid_vfp_lines = MTD700920B_VFP,
		.vid_active_lines = MTD700920B_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = ZCT_PIXEL_CLK(MTD700920B),
};

const struct hs_settle_s hs_timing_cfg_mtd700920b_800x1280 = { .prepare = 6, .zero = 32, .trail = 1 };

#ifndef CVI_U8
#define CVI_U8 unsigned char
#endif

static CVI_U8 data_MTD700920B_0[] = { 0xE1,0x93 };
static CVI_U8 data_MTD700920B_1[] = { 0xE2,0x65 };
static CVI_U8 data_MTD700920B_2[] = { 0xE3,0xF8 };
static CVI_U8 data_MTD700920B_3[] = { 0x80,0x01 };

static CVI_U8 data_MTD700920B_4[] = { 0x70,0x10 };
static CVI_U8 data_MTD700920B_5[] = { 0x71,0x13 };
static CVI_U8 data_MTD700920B_6[] = { 0x72,0x06 };
static CVI_U8 data_MTD700920B_7[] = { 0x75,0x03 };

static CVI_U8 data_MTD700920B_8[] = { 0xE0,0x01 };

static CVI_U8 data_MTD700920B_9[] = { 0x00,0x00 };
static CVI_U8 data_MTD700920B_10[] = { 0x01,0x39 };//74
static CVI_U8 data_MTD700920B_11[] = { 0x03,0x00 };
static CVI_U8 data_MTD700920B_12[] = { 0x04,0x7A };

static CVI_U8 data_MTD700920B_13[] = { 0x17,0x00 };
static CVI_U8 data_MTD700920B_14[] = { 0x18,0xEF };
static CVI_U8 data_MTD700920B_15[] = { 0x19,0x01 };
static CVI_U8 data_MTD700920B_16[] = { 0x1A,0x00 };
static CVI_U8 data_MTD700920B_17[] = { 0x1B,0xEF };
static CVI_U8 data_MTD700920B_18[] = { 0x1C,0x01 };

static CVI_U8 data_MTD700920B_19[] = { 0x1F,0x6B };
static CVI_U8 data_MTD700920B_20[] = { 0x20,0x24 };
static CVI_U8 data_MTD700920B_21[] = { 0x21,0x24 };
static CVI_U8 data_MTD700920B_22[] = { 0x22,0x4E };
static CVI_U8 data_MTD700920B_23[] = { 0x24,0xFE };

static CVI_U8 data_MTD700920B_24[] = { 0x37,0x09 };

static CVI_U8 data_MTD700920B_25[] = { 0x38,0x04 };
static CVI_U8 data_MTD700920B_26[] = { 0x39,0x08 };
static CVI_U8 data_MTD700920B_27[] = { 0x3A,0x12 };
static CVI_U8 data_MTD700920B_28[] = { 0x3C,0x78 };
static CVI_U8 data_MTD700920B_29[] = { 0x3D,0xFF };
static CVI_U8 data_MTD700920B_30[] = { 0x3E,0xFF };
static CVI_U8 data_MTD700920B_31[] = { 0x3F,0x7F };


static CVI_U8 data_MTD700920B_32[] = { 0x40,0x06 };
static CVI_U8 data_MTD700920B_33[] = { 0x41,0xA0 };
static CVI_U8 data_MTD700920B_34[] = { 0x43,0x14 };
static CVI_U8 data_MTD700920B_35[] = { 0x44,0x11 };
static CVI_U8 data_MTD700920B_36[] = { 0x45,0x24 };

static CVI_U8 data_MTD700920B_37[] = { 0x55,0x02 };  //02 for FP7721
static CVI_U8 data_MTD700920B_38[] = { 0x56,0x01 };
static CVI_U8 data_MTD700920B_39[] = { 0x57,0x65 };   //65 for noise
static CVI_U8 data_MTD700920B_40[] = { 0x58,0x0A };
static CVI_U8 data_MTD700920B_41[] = { 0x59,0x0A };
static CVI_U8 data_MTD700920B_42[] = { 0x5A,0x29 };
static CVI_U8 data_MTD700920B_43[] = { 0x5B,0x10 };


static CVI_U8 data_MTD700920B_44[] = { 0x5D,0x70 };
static CVI_U8 data_MTD700920B_45[] = { 0x5E,0x57 };
static CVI_U8 data_MTD700920B_46[] = { 0x5F,0x47 };
static CVI_U8 data_MTD700920B_47[] = { 0x60,0x3A };
static CVI_U8 data_MTD700920B_48[] = { 0x61,0x35 };
static CVI_U8 data_MTD700920B_49[] = { 0x62,0x25 };
static CVI_U8 data_MTD700920B_50[] = { 0x63,0x29 };
static CVI_U8 data_MTD700920B_51[] = { 0x64,0x12 };
static CVI_U8 data_MTD700920B_52[] = { 0x65,0x29 };
static CVI_U8 data_MTD700920B_53[] = { 0x66,0x26 };
static CVI_U8 data_MTD700920B_54[] = { 0x67,0x23 };
static CVI_U8 data_MTD700920B_55[] = { 0x68,0x3F };
static CVI_U8 data_MTD700920B_56[] = { 0x69,0x2B };
static CVI_U8 data_MTD700920B_57[] = { 0x6A,0x32 };
static CVI_U8 data_MTD700920B_58[] = { 0x6B,0x24 };
static CVI_U8 data_MTD700920B_59[] = { 0x6C,0x22 };
static CVI_U8 data_MTD700920B_60[] = { 0x6D,0x16 };
static CVI_U8 data_MTD700920B_61[] = { 0x6E,0x09 };
static CVI_U8 data_MTD700920B_62[] = { 0x6F,0x02 };
static CVI_U8 data_MTD700920B_63[] = { 0x70,0x70 };
static CVI_U8 data_MTD700920B_64[] = { 0x71,0x57 };
static CVI_U8 data_MTD700920B_65[] = { 0x72,0x47 };
static CVI_U8 data_MTD700920B_66[] = { 0x73,0x3A };
static CVI_U8 data_MTD700920B_67[] = { 0x74,0x35 };
static CVI_U8 data_MTD700920B_68[] = { 0x75,0x25 };
static CVI_U8 data_MTD700920B_69[] = { 0x76,0x29 };
static CVI_U8 data_MTD700920B_70[] = { 0x77,0x12 };
static CVI_U8 data_MTD700920B_71[] = { 0x78,0x29 };
static CVI_U8 data_MTD700920B_72[] = { 0x79,0x26 };
static CVI_U8 data_MTD700920B_73[] = { 0x7A,0x23 };
static CVI_U8 data_MTD700920B_74[] = { 0x7B,0x3F };
static CVI_U8 data_MTD700920B_75[] = { 0x7C,0x2B };
static CVI_U8 data_MTD700920B_76[] = { 0x7D,0x32 };
static CVI_U8 data_MTD700920B_77[] = { 0x7E,0x24 };
static CVI_U8 data_MTD700920B_78[] = { 0x7F,0x22 };
static CVI_U8 data_MTD700920B_79[] = { 0x80,0x16 };
static CVI_U8 data_MTD700920B_80[] = { 0x81,0x09 };
static CVI_U8 data_MTD700920B_81[] = { 0x82,0x02 };


static CVI_U8 data_MTD700920B_82[] = { 0xE0,0x02 };

static CVI_U8 data_MTD700920B_83[] = { 0x00,0x1E };
static CVI_U8 data_MTD700920B_84[] = { 0x01,0x1F };
static CVI_U8 data_MTD700920B_85[] = { 0x02,0x57 };
static CVI_U8 data_MTD700920B_86[] = { 0x03,0x58 };
static CVI_U8 data_MTD700920B_87[] = { 0x04,0x48 };
static CVI_U8 data_MTD700920B_88[] = { 0x05,0x4A };
static CVI_U8 data_MTD700920B_89[] = { 0x06,0x44 };
static CVI_U8 data_MTD700920B_90[] = { 0x07,0x46 };
static CVI_U8 data_MTD700920B_91[] = { 0x08,0x40 };
static CVI_U8 data_MTD700920B_92[] = { 0x09,0x1F };
static CVI_U8 data_MTD700920B_93[] = { 0x0A,0x1F };
static CVI_U8 data_MTD700920B_94[] = { 0x0B,0x1F };
static CVI_U8 data_MTD700920B_95[] = { 0x0C,0x1F };
static CVI_U8 data_MTD700920B_96[] = { 0x0D,0x1F };
static CVI_U8 data_MTD700920B_97[] = { 0x0E,0x1F };
static CVI_U8 data_MTD700920B_98[] = { 0x0F,0x42 };
static CVI_U8 data_MTD700920B_99[] = { 0x10,0x1F };
static CVI_U8 data_MTD700920B_100[] = { 0x11,0x1F };
static CVI_U8 data_MTD700920B_101[] = { 0x12,0x1F };
static CVI_U8 data_MTD700920B_102[] = { 0x13,0x1F };
static CVI_U8 data_MTD700920B_103[] = { 0x14,0x1F };
static CVI_U8 data_MTD700920B_104[] = { 0x15,0x1F };

static CVI_U8 data_MTD700920B_105[] = { 0x16,0x1E };
static CVI_U8 data_MTD700920B_106[] = { 0x17,0x1F };
static CVI_U8 data_MTD700920B_107[] = { 0x18,0x57 };
static CVI_U8 data_MTD700920B_108[] = { 0x19,0x58 };
static CVI_U8 data_MTD700920B_109[] = { 0x1A,0x49 };
static CVI_U8 data_MTD700920B_110[] = { 0x1B,0x4B };
static CVI_U8 data_MTD700920B_111[] = { 0x1C,0x45 };
static CVI_U8 data_MTD700920B_112[] = { 0x1D,0x47 };
static CVI_U8 data_MTD700920B_113[] = { 0x1E,0x41 };
static CVI_U8 data_MTD700920B_114[] = { 0x55,0x55 };
static CVI_U8 data_MTD700920B_115[] = { 0x20,0x1F };
static CVI_U8 data_MTD700920B_116[] = { 0x21,0x55 };
static CVI_U8 data_MTD700920B_117[] = { 0x22,0x55 };
static CVI_U8 data_MTD700920B_118[] = { 0x23,0x55 };
static CVI_U8 data_MTD700920B_119[] = { 0x24,0x55 };
static CVI_U8 data_MTD700920B_120[] = { 0x25,0x43 };
static CVI_U8 data_MTD700920B_121[] = { 0x26,0x55 };
static CVI_U8 data_MTD700920B_122[] = { 0x27,0x55 };
static CVI_U8 data_MTD700920B_123[] = { 0x28,0x55 };
static CVI_U8 data_MTD700920B_124[] = { 0x29,0x55 };
static CVI_U8 data_MTD700920B_125[] = { 0x2A,0x55 };
static CVI_U8 data_MTD700920B_126[] = { 0x2B,0x55 };

static CVI_U8 data_MTD700920B_127[] = { 0x2C,0x55 };
static CVI_U8 data_MTD700920B_128[] = { 0x2D,0x1E };
static CVI_U8 data_MTD700920B_129[] = { 0x2E,0x17 };
static CVI_U8 data_MTD700920B_130[] = { 0x2F,0x18 };
static CVI_U8 data_MTD700920B_131[] = { 0x30,0x07 };
static CVI_U8 data_MTD700920B_132[] = { 0x31,0x05 };
static CVI_U8 data_MTD700920B_133[] = { 0x32,0x0B };
static CVI_U8 data_MTD700920B_134[] = { 0x33,0x09 };
static CVI_U8 data_MTD700920B_135[] = { 0x34,0x03 };
static CVI_U8 data_MTD700920B_136[] = { 0x35,0x55 };
static CVI_U8 data_MTD700920B_137[] = { 0x36,0x55 };
static CVI_U8 data_MTD700920B_138[] = { 0x37,0x55 };
static CVI_U8 data_MTD700920B_139[] = { 0x38,0x55 };
static CVI_U8 data_MTD700920B_140[] = { 0x39,0x55 };
static CVI_U8 data_MTD700920B_141[] = { 0x3A,0x55 };
static CVI_U8 data_MTD700920B_142[] = { 0x3B,0x01 };
static CVI_U8 data_MTD700920B_143[] = { 0x3C,0x55 };
static CVI_U8 data_MTD700920B_144[] = { 0x3D,0x55 };
static CVI_U8 data_MTD700920B_145[] = { 0x3E,0x55 };
static CVI_U8 data_MTD700920B_146[] = { 0x3F,0x55 };
static CVI_U8 data_MTD700920B_147[] = { 0x40,0x1F };
static CVI_U8 data_MTD700920B_148[] = { 0x41,0x55 };

static CVI_U8 data_MTD700920B_149[] = { 0x42,0x55 };
static CVI_U8 data_MTD700920B_150[] = { 0x43,0x1E };
static CVI_U8 data_MTD700920B_151[] = { 0x44,0x17 };
static CVI_U8 data_MTD700920B_152[] = { 0x45,0x18 };
static CVI_U8 data_MTD700920B_153[] = { 0x46,0x06 };
static CVI_U8 data_MTD700920B_154[] = { 0x47,0x04 };
static CVI_U8 data_MTD700920B_155[] = { 0x48,0x0A };
static CVI_U8 data_MTD700920B_156[] = { 0x49,0x08 };
static CVI_U8 data_MTD700920B_157[] = { 0x4A,0x02 };
static CVI_U8 data_MTD700920B_158[] = { 0x4B,0x55 };
static CVI_U8 data_MTD700920B_159[] = { 0x4C,0x55 };
static CVI_U8 data_MTD700920B_160[] = { 0x4D,0x55 };
static CVI_U8 data_MTD700920B_161[] = { 0x4E,0x55 };
static CVI_U8 data_MTD700920B_162[] = { 0x4F,0x55 };
static CVI_U8 data_MTD700920B_163[] = { 0x50,0x1F };
static CVI_U8 data_MTD700920B_164[] = { 0x51,0x00 };
static CVI_U8 data_MTD700920B_165[] = { 0x52,0x55 };
static CVI_U8 data_MTD700920B_166[] = { 0x53,0x55 };
static CVI_U8 data_MTD700920B_167[] = { 0x54,0x55 };
static CVI_U8 data_MTD700920B_168[] = { 0x55,0x55 };
static CVI_U8 data_MTD700920B_169[] = { 0x56,0x55 };
static CVI_U8 data_MTD700920B_170[] = { 0x57,0x55 };

static CVI_U8 data_MTD700920B_171[] = { 0x58,0x40 };
static CVI_U8 data_MTD700920B_172[] = { 0x59,0x00 };
static CVI_U8 data_MTD700920B_173[] = { 0x5A,0x00 };
static CVI_U8 data_MTD700920B_174[] = { 0x5B,0x30 };
static CVI_U8 data_MTD700920B_175[] = { 0x5C,0x07 };
static CVI_U8 data_MTD700920B_176[] = { 0x5D,0x30 };
static CVI_U8 data_MTD700920B_177[] = { 0x5E,0x01 };
static CVI_U8 data_MTD700920B_178[] = { 0x5F,0x02 };
static CVI_U8 data_MTD700920B_179[] = { 0x60,0x30 };
static CVI_U8 data_MTD700920B_180[] = { 0x61,0x03 };
static CVI_U8 data_MTD700920B_181[] = { 0x62,0x04 };
static CVI_U8 data_MTD700920B_182[] = { 0x63,0x6A };
static CVI_U8 data_MTD700920B_183[] = { 0x64,0x6A };
static CVI_U8 data_MTD700920B_184[] = { 0x65,0x35 };
static CVI_U8 data_MTD700920B_185[] = { 0x66,0x0D };
static CVI_U8 data_MTD700920B_186[] = { 0x67,0x73 };
static CVI_U8 data_MTD700920B_187[] = { 0x68,0x0B };
static CVI_U8 data_MTD700920B_188[] = { 0x69,0x6A };
static CVI_U8 data_MTD700920B_189[] = { 0x6A,0x6A };
static CVI_U8 data_MTD700920B_190[] = { 0x6B,0x08 };
static CVI_U8 data_MTD700920B_191[] = { 0x6C,0x00 };
static CVI_U8 data_MTD700920B_192[] = { 0x6D,0x04 };
static CVI_U8 data_MTD700920B_193[] = { 0x6E,0x00 };
static CVI_U8 data_MTD700920B_194[] = { 0x6F,0x88 };
static CVI_U8 data_MTD700920B_195[] = { 0x70,0x00 };
static CVI_U8 data_MTD700920B_196[] = { 0x71,0x00 };
static CVI_U8 data_MTD700920B_197[] = { 0x72,0x06 };
static CVI_U8 data_MTD700920B_198[] = { 0x73,0x7B };
static CVI_U8 data_MTD700920B_199[] = { 0x74,0x00 };
static CVI_U8 data_MTD700920B_200[] = { 0x75,0xBC };
static CVI_U8 data_MTD700920B_201[] = { 0x76,0x00 };
static CVI_U8 data_MTD700920B_202[] = { 0x77,0x0D };
static CVI_U8 data_MTD700920B_203[] = { 0x78,0x1B };
static CVI_U8 data_MTD700920B_204[] = { 0x79,0x00 };
static CVI_U8 data_MTD700920B_205[] = { 0x7A,0x00 };
static CVI_U8 data_MTD700920B_206[] = { 0x7B,0x00 };
static CVI_U8 data_MTD700920B_207[] = { 0x7C,0x00 };
static CVI_U8 data_MTD700920B_208[] = { 0x7D,0x03 };
static CVI_U8 data_MTD700920B_209[] = { 0x7E,0x7B };

static CVI_U8 data_MTD700920B_210[] = { 0xE0,0x04 };
static CVI_U8 data_MTD700920B_211[] = { 0x02,0x23 };
static CVI_U8 data_MTD700920B_212[] = { 0x09,0x10 };
static CVI_U8 data_MTD700920B_213[] = { 0x0E,0x38 };
static CVI_U8 data_MTD700920B_214[] = { 0x36,0x49 };
static CVI_U8 data_MTD700920B_215[] = { 0x2B,0x08 };
static CVI_U8 data_MTD700920B_216[] = { 0x2E,0x03 };

static CVI_U8 data_MTD700920B_217[] = { 0xE0,0x00 };

static CVI_U8 data_MTD700920B_218[] = { 0x80,0x01 };		// 2lan

static CVI_U8 data_MTD700920B_219[] = { 0xE6,0x02 };
static CVI_U8 data_MTD700920B_220[] = { 0xE7,0x06 };


static CVI_U8 data_MTD700920B_221[] = { 0xE0,0x00 };
static CVI_U8 data_MTD700920B_222[] = { 0xE0,0x00 };


static CVI_U8 data_MTD700920B_223[] = { 0x11 };
// Delay(120);
static CVI_U8 data_MTD700920B_224[] = { 0x29 };
// Delay(50);

// static CVI_U8 data_MTD700920B_0[] = { 0xE1, 0x93 };
// static CVI_U8 data_MTD700920B_1[] = { 0xE2, 0x65 };
// static CVI_U8 data_MTD700920B_2[] = { 0xE3, 0xF8 };
// static CVI_U8 data_MTD700920B_3[] = { 0x80, 0x01 };
// static CVI_U8 data_MTD700920B_4[] = { 0x11 };
// static CVI_U8 data_MTD700920B_5[] = { 0x29 };

// len == 1 , type 0x05
// len == 2 , type 0x15 or type 23
// len >= 3 , type 0x29 or type 0x39
#define TYPE1 0x05
#define TYPE2 0x15
#define TYPE3 0x29

const struct dsc_instr dsi_init_cmds_mtd700920b_800x1280[] = {
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_0 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_1 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_2 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_3 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_4 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_5 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_6 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_7 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_8 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_9 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_10 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_11 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_12 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_13 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_14 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_15 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_16 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_17 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_18 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_19 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_20 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_21 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_22 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_23 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_24 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_25 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_26 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_27 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_28 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_29 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_30 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_31 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_32 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_33 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_34 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_35 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_36 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_37 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_38 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_39 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_40 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_41 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_42 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_43 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_44 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_45 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_46 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_47 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_48 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_49 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_50 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_51 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_52 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_53 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_54 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_55 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_56 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_57 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_58 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_59 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_60 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_61 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_62 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_63 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_64 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_65 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_66 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_67 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_68 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_69 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_70 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_71 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_72 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_73 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_74 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_75 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_76 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_77 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_78 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_79 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_80 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_81 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_82 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_83 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_84 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_85 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_86 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_87 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_88 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_89 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_90 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_91 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_92 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_93 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_94 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_95 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_96 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_97 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_98 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_99 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_100 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_101 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_102 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_103 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_104 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_105 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_106 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_107 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_108 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_109 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_110 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_111 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_112 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_113 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_114 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_115 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_116 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_117 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_118 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_119 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_120 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_121 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_122 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_123 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_124 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_125 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_126 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_127 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_128 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_129 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_130 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_131 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_132 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_133 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_134 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_135 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_136 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_137 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_138 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_139 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_140 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_141 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_142 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_143 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_144 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_145 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_146 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_147 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_148 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_149 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_150 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_151 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_152 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_153 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_154 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_155 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_156 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_157 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_158 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_159 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_160 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_161 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_162 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_163 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_164 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_165 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_166 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_167 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_168 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_169 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_170 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_171 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_172 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_173 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_174 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_175 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_176 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_177 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_178 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_179 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_180 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_181 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_182 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_183 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_184 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_185 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_186 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_187 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_188 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_189 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_190 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_191 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_192 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_193 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_194 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_195 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_196 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_197 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_198 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_199 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_200 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_201 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_202 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_203 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_204 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_205 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_206 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_207 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_208 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_209 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_210 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_211 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_212 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_213 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_214 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_215 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_216 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_217 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_218 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_219 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_220 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_221 },
	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_222 },
	{.delay = 0, .data_type = TYPE1, .size = 1, .data = data_MTD700920B_223 },
	{.delay = 0, .data_type = TYPE1, .size = 1, .data = data_MTD700920B_224 },
};

// const struct dsc_instr dsi_init_cmds_MTD700920B_800x1280[] = {
// 	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_0 },
// 	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_1 },
// 	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_2 },
// 	{.delay = 0, .data_type = TYPE2, .size = 2, .data = data_MTD700920B_3 },
// 	{.delay = 0, .data_type = TYPE1, .size = 1, .data = data_MTD700920B_4 },
// 	{.delay = 0, .data_type = TYPE1, .size = 1, .data = data_MTD700920B_5 },
// };

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif // _MIPI_TX_PARAM_MTD700920B_H_
