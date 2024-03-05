#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "system_common.h"
#elif defined(ENV_EMU)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif
#endif  // ENV_CVITEST

#include <linux/cvi_comm_vo.h>
#include <linux/vo_disp.h>

#include "vpss_debug.h"
#include "vpss_common.h"
#include "scaler.h"
#include "scaler_reg.h"
#include "reg.h"
#include "cmdq.h"
#include "dsi_phy.h"
#include "reg_sc_core.h"
#include "reg_sc_top.h"
#include "reg_disp.h"
#include "reg_img_in.h"
#include "reg_sc_odma.h"
#include "reg_vgop.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
static struct sclr_top_cfg g_top_cfg;
static struct sclr_oenc_cfg g_oenc_cfg;
static struct sclr_core_cfg g_sc_cfg[SCL_MAX_INST];
static struct sclr_gop_cfg g_gop_cfg[SCL_MAX_INST][SCL_MAX_GOP_INST];
static struct sclr_img_cfg g_img_cfg[2];
static struct sclr_cir_cfg g_cir_cfg[SCL_MAX_INST];
static struct sclr_border_cfg g_bd_cfg[SCL_MAX_INST];
static struct sclr_odma_cfg g_odma_cfg[SCL_MAX_INST];
static struct sclr_disp_cfg g_disp_cfg;
static struct sclr_disp_timing disp_timing;
struct sclr_top_sb_cfg g_top_sb_cfg;
static uintptr_t reg_base;
static spinlock_t disp_mask_spinlock;
static DEFINE_RAW_SPINLOCK(__sc_top_lock);
/****************************************************************************
 * Initial info
 ****************************************************************************/
#define DEFINE_CSC_COEF0(a, b, c) \
		.coef[0][0] = a, .coef[0][1] = b, .coef[0][2] = c,
#define DEFINE_CSC_COEF1(a, b, c) \
		.coef[1][0] = a, .coef[1][1] = b, .coef[1][2] = c,
#define DEFINE_CSC_COEF2(a, b, c) \
		.coef[2][0] = a, .coef[2][1] = b, .coef[2][2] = c,
static struct sclr_csc_matrix csc_mtrx[SCL_CSC_MAX] = {
	// none
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		0)
		DEFINE_CSC_COEF1(0,		BIT(10),	0)
		DEFINE_CSC_COEF2(0,		0,		BIT(10))
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// yuv2rgb
	// 601 Limited
	//  R = Y + 1.402* Pr                           //
	//  G = Y - 0.344 * Pb  - 0.792* Pr             //
	//  B = Y + 1.772 * Pb                          //
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1436)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 352,	BIT(13) | 731)
		DEFINE_CSC_COEF2(BIT(10),	1815,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 601 Full
	//  R = 1.164 *(Y - 16) + 1.596 *(Cr - 128)                     //
	//  G = 1.164 *(Y - 16) - 0.392 *(Cb - 128) - 0.812 *(Cr - 128) //
	//  B = 1.164 *(Y - 16) + 2.016 *(Cb - 128)                     //
	{
		DEFINE_CSC_COEF0(1192,	0,		1634)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 401,	BIT(13) | 833)
		DEFINE_CSC_COEF2(1192,	2065,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Limited
	// R = Y + 1.540(Cr – 128)
	// G = Y - 0.183(Cb – 128) – 0.459(Cr – 128)
	// B = Y + 1.816(Cb – 128)
	{
		DEFINE_CSC_COEF0(BIT(10),	0,		1577)
		DEFINE_CSC_COEF1(BIT(10),	BIT(13) | 187,	BIT(13) | 470)
		DEFINE_CSC_COEF2(BIT(10),	1860,		0)
		.sub[0] = 0,   .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// 709 Full
	//  R = 1.164 *(Y - 16) + 1.792 *(Cr - 128)                     //
	//  G = 1.164 *(Y - 16) - 0.213 *(Cb - 128) - 0.534 *(Cr - 128) //
	//  B = 1.164 *(Y - 16) + 2.114 *(Cb - 128)                     //
	{
		DEFINE_CSC_COEF0(1192,	0,		1836)
		DEFINE_CSC_COEF1(1192,	BIT(13) | 218,	BIT(13) | 547)
		DEFINE_CSC_COEF2(1192,	2166,		0)
		.sub[0] = 16,  .sub[1] = 128, .sub[2] = 128,
		.add[0] = 0,   .add[1] = 0,   .add[2] = 0
	},
	// rgb2yuv
	// 601 Limited
	//  Y = 0.299 * R + 0.587 * G + 0.114 * B       //
	// Pb =-0.169 * R - 0.331 * G + 0.500 * B       //
	// Pr = 0.500 * R - 0.419 * G - 0.081 * B       //
	{
		DEFINE_CSC_COEF0(306,		601,		117)
		DEFINE_CSC_COEF1(BIT(13)|173,	BIT(13)|339,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|429,	BIT(13)|83)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 601 Full
	//  Y = 16  + 0.257 * R + 0.504 * g + 0.098 * b //
	// Cb = 128 - 0.148 * R - 0.291 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.368 * g - 0.071 * b //
	{
		DEFINE_CSC_COEF0(263,		516,		100)
		DEFINE_CSC_COEF1(BIT(13)|152,	BIT(13)|298,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|377,	BIT(13)|73)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
	// 709 Limited
	//   Y =       0.2126   0.7152   0.0722
	//  Cb = 128 - 0.1146  -0.3854   0.5000
	//  Cr = 128 + 0.5000  -0.4542  -0.0468
	{
		DEFINE_CSC_COEF0(218,		732,		74)
		DEFINE_CSC_COEF1(BIT(13)|117,	BIT(13)|395,	512)
		DEFINE_CSC_COEF2(512,		BIT(13)|465,	BIT(13)|48)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 0,   .add[1] = 128, .add[2] = 128
	},
	// 709 Full
	//  Y = 16  + 0.183 * R + 0.614 * g + 0.062 * b //
	// Cb = 128 - 0.101 * R - 0.339 * g + 0.439 * b //
	// Cr = 128 + 0.439 * R - 0.399 * g - 0.040 * b //
	{
		DEFINE_CSC_COEF0(187,		629,		63)
		DEFINE_CSC_COEF1(BIT(13)|103,	BIT(13)|347,	450)
		DEFINE_CSC_COEF2(450,		BIT(13)|408,	BIT(13)|41)
		.sub[0] = 0,   .sub[1] = 0,   .sub[2] = 0,
		.add[0] = 16,  .add[1] = 128, .add[2] = 128
	},
};

static int scl_coef[128][4] = {
	{-4,    1024,   4,  0},
	{-8,    1024,   8,  0},
	{-12,   1024,   12, 0},
	{-16,   1024,   16, 0},
	{-19,   1023,   20, 0},
	{-23,   1023,   25, -1},
	{-26,   1022,   29, -1},
	{-30,   1022,   33, -1},
	{-34,   1022,   37, -1},
	{-37,   1021,   42, -2},
	{-40,   1020,   46, -2},
	{-44,   1020,   50, -2},
	{-47,   1019,   55, -3},
	{-50,   1018,   59, -3},
	{-53,   1017,   63, -3},
	{-56,   1016,   68, -4},
	{-59,   1015,   72, -4},
	{-62,   1014,   77, -5},
	{-65,   1013,   81, -5},
	{-68,   1012,   86, -6},
	{-71,   1011,   90, -6},
	{-74,   1010,   95, -7},
	{-76,   1008,   100,    -8},
	{-79,   1007,   104,    -8},
	{-81,   1005,   109,    -9},
	{-84,   1004,   113,    -9},
	{-86,   1002,   118,    -10},
	{-89,   1001,   123,    -11},
	{-91,   999,    128,    -12},
	{-94,   998,    132,    -12},
	{-96,   996,    137,    -13},
	{-98,   994,    142,    -14},
	{-100,  992,    147,    -15},
	{-102,  990,    152,    -16},
	{-104,  988,    157,    -17},
	{-106,  986,    161,    -17},
	{-108,  984,    166,    -18},
	{-110,  982,    171,    -19},
	{-112,  980,    176,    -20},
	{-114,  978,    181,    -21},
	{-116,  976,    186,    -22},
	{-117,  973,    191,    -23},
	{-119,  971,    196,    -24},
	{-121,  969,    201,    -25},
	{-122,  966,    206,    -26},
	{-124,  964,    211,    -27},
	{-125,  961,    216,    -28},
	{-127,  959,    221,    -29},
	{-128,  956,    226,    -30},
	{-130,  954,    231,    -31},
	{-131,  951,    237,    -33},
	{-132,  948,    242,    -34},
	{-133,  945,    247,    -35},
	{-134,  942,    252,    -36},
	{-136,  940,    257,    -37},
	{-137,  937,    262,    -38},
	{-138,  934,    267,    -39},
	{-139,  931,    273,    -41},
	{-140,  928,    278,    -42},
	{-141,  925,    283,    -43},
	{-142,  922,    288,    -44},
	{-142,  918,    294,    -46},
	{-143,  915,    299,    -47},
	{-144,  912,    304,    -48},
	{-145,  909,    309,    -49},
	{-145,  905,    315,    -51},
	{-146,  902,    320,    -52},
	{-147,  899,    325,    -53},
	{-147,  895,    330,    -54},
	{-148,  892,    336,    -56},
	{-148,  888,    341,    -57},
	{-149,  885,    346,    -58},
	{-149,  881,    352,    -60},
	{-150,  878,    357,    -61},
	{-150,  874,    362,    -62},
	{-150,  870,    367,    -63},
	{-151,  867,    373,    -65},
	{-151,  863,    378,    -66},
	{-151,  859,    383,    -67},
	{-151,  855,    389,    -69},
	{-151,  851,    394,    -70},
	{-152,  848,    399,    -71},
	{-152,  844,    405,    -73},
	{-152,  840,    410,    -74},
	{-152,  836,    415,    -75},
	{-152,  832,    421,    -77},
	{-152,  828,    426,    -78},
	{-152,  824,    431,    -79},
	{-151,  819,    437,    -81},
	{-151,  815,    442,    -82},
	{-151,  811,    447,    -83},
	{-151,  807,    453,    -85},
	{-151,  803,    458,    -86},
	{-151,  799,    463,    -87},
	{-150,  794,    469,    -89},
	{-150,  790,    474,    -90},
	{-150,  786,    479,    -91},
	{-149,  781,    485,    -93},
	{-149,  777,    490,    -94},
	{-149,  773,    495,    -95},
	{-148,  768,    501,    -97},
	{-148,  764,    506,    -98},
	{-147,  759,    511,    -99},
	{-147,  755,    516,    -100},
	{-146,  750,    522,    -102},
	{-146,  746,    527,    -103},
	{-145,  741,    532,    -104},
	{-144,  736,    537,    -105},
	{-144,  732,    543,    -107},
	{-143,  727,    548,    -108},
	{-142,  722,    553,    -109},
	{-142,  718,    558,    -110},
	{-141,  713,    563,    -111},
	{-140,  708,    569,    -113},
	{-140,  704,    574,    -114},
	{-139,  699,    579,    -115},
	{-138,  694,    584,    -116},
	{-137,  689,    589,    -117},
	{-136,  684,    594,    -118},
	{-135,  679,    600,    -120},
	{-135,  675,    605,    -121},
	{-134,  670,    610,    -122},
	{-133,  665,    615,    -123},
	{-132,  660,    620,    -124},
	{-131,  655,    625,    -125},
	{-130,  650,    630,    -126},
	{-129,  645,    635,    -127},
	{-128,  640,    640,    -128},
};

static int scl_coef_z2[128][4] = {
	{-520, 2047, -504,    1},
	{-528, 2047, -496,    1},
	{-536, 2047, -488,    1},
	{-544, 2047, -480,    1},
	{-550, 2047, -472,   -1},
	{-558, 2047, -461,   -4},
	{-564, 2047, -453,   -6},
	{-572, 2047, -445,   -6},
	{-580, 2047, -437,   -6},
	{-587, 2047, -427,   -9},
	{-593, 2047, -419,  -11},
	{-601, 2047, -411,  -11},
	{-607, 2047, -401,  -15},
	{-614, 2047, -392,  -17},
	{-620, 2047, -384,  -19},
	{-626, 2047, -374,  -23},
	{-633, 2047, -366,  -24},
	{-639, 2047, -355,  -29},
	{-646, 2047, -347,  -30},
	{-652, 2047, -336,  -35},
	{-659, 2047, -328,  -36},
	{-665, 2047, -317,  -41},
	{-670, 2047, -306,  -47},
	{-677, 2047, -298,  -48},
	{-681, 2047, -287,  -55},
	{-688, 2047, -279,  -56},
	{-693, 2047, -268,  -62},
	{-700, 2047, -257,  -66},
	{-705, 2047, -246,  -72},
	{-711, 2047, -237,  -75},
	{-716, 2047, -226,  -81},
	{-722, 2047, -214,  -87},
	{-727, 2047, -203,  -93},
	{-732, 2047, -192,  -99},
	{-737, 2047, -180, -106},
	{-742, 2047, -171, -110},
	{-747, 2047, -159, -117},
	{-753, 2047, -148, -122},
	{-758, 2047, -136, -129},
	{-763, 2047, -124, -136},
	{-769, 2047, -112, -142},
	{-773, 2047, -100, -150},
	{-778, 2046,  -88, -156},
	{-782, 2044,  -75, -163},
	{-784, 2037,  -63, -166},
	{-788, 2035,  -50, -173},
	{-789, 2028,  -37, -178},
	{-794, 2025,  -25, -182},
	{-795, 2019,  -12, -188},
	{-800, 2016,    1, -193},
	{-802, 2010,   16, -200},
	{-803, 2003,   29, -205},
	{-804, 1997,   42, -211},
	{-806, 1990,   56, -216},
	{-810, 1987,   68, -221},
	{-812, 1981,   81, -226},
	{-813, 1974,   95, -232},
	{-815, 1968,  111, -240},
	{-817, 1961,  124, -244},
	{-818, 1955,  137, -250},
	{-819, 1948,  151, -256},
	{-818, 1938,  167, -263},
	{-820, 1931,  181, -268},
	{-821, 1924,  194, -273},
	{-822, 1918,  208, -280},
	{-821, 1908,  224, -287},
	{-823, 1901,  238, -292},
	{-824, 1894,  252, -298},
	{-822, 1883,  266, -303},
	{-824, 1877,  282, -311},
	{-822, 1866,  296, -316},
	{-824, 1859,  310, -321},
	{-822, 1849,  327, -330},
	{-824, 1842,  341, -335},
	{-822, 1832,  355, -341},
	{-820, 1821,  369, -346},
	{-822, 1814,  386, -354},
	{-820, 1804,  400, -360},
	{-819, 1793,  414, -364},
	{-817, 1782,  431, -372},
	{-816, 1772,  446, -378},
	{-817, 1765,  460, -384},
	{-816, 1754,  477, -391},
	{-814, 1743,  491, -396},
	{-812, 1733,  506, -403},
	{-811, 1722,  523, -410},
	{-809, 1711,  537, -415},
	{-807, 1700,  552, -421},
	{-802, 1686,  570, -430},
	{-801, 1675,  584, -434},
	{-799, 1664,  598, -439},
	{-797, 1654,  616, -449},
	{-796, 1643,  630, -453},
	{-794, 1632,  645, -459},
	{-789, 1617,  663, -467},
	{-787, 1606,  677, -472},
	{-785, 1595,  692, -478},
	{-781, 1581,  710, -486},
	{-779, 1570,  724, -491},
	{-777, 1559,  739, -497},
	{-772, 1544,  757, -505},
	{-771, 1533,  772, -510},
	{-765, 1518,  786, -515},
	{-764, 1507,  801, -520},
	{-759, 1493,  819, -529},
	{-757, 1482,  834, -535},
	{-752, 1467,  849, -540},
	{-747, 1452,  863, -544},
	{-745, 1441,  882, -554},
	{-740, 1426,  896, -558},
	{-735, 1411,  911, -563},
	{-733, 1400,  926, -569},
	{-728, 1385,  941, -574},
	{-723, 1371,  959, -583},
	{-721, 1360,  974, -589},
	{-716, 1345,  989, -594},
	{-711, 1330, 1003, -598},
	{-706, 1315, 1018, -603},
	{-701, 1300, 1033, -608},
	{-696, 1286, 1051, -617},
	{-694, 1274, 1066, -622},
	{-689, 1259, 1081, -627},
	{-684, 1245, 1096, -633},
	{-679, 1230, 1111, -638},
	{-674, 1215, 1126, -643},
	{-669, 1200, 1141, -648},
	{-663, 1185, 1155, -653},
	{-658, 1170, 1170, -658}
};

static int scl_coef_z3[128][4] = {
	{-262, 1538, -250,   -2},
	{-269, 1539, -244,   -2},
	{-275, 1541, -239,   -3},
	{-281, 1542, -233,   -4},
	{-286, 1541, -227,   -4},
	{-292, 1543, -219,   -8},
	{-296, 1542, -213,   -9},
	{-303, 1544, -207,  -10},
	{-309, 1545, -201,  -11},
	{-314, 1545, -194,  -13},
	{-318, 1544, -187,  -15},
	{-325, 1546, -182,  -15},
	{-329, 1545, -174,  -18},
	{-334, 1545, -167,  -20},
	{-338, 1544, -161,  -21},
	{-343, 1544, -153,  -24},
	{-348, 1543, -147,  -24},
	{-352, 1542, -139,  -27},
	{-357, 1542, -133,  -28},
	{-362, 1541, -125,  -30},
	{-366, 1541, -118,  -33},
	{-371, 1540, -110,  -35},
	{-374, 1538, -102,  -38},
	{-378, 1537,  -96,  -39},
	{-381, 1534,  -87,  -42},
	{-386, 1534,  -81,  -43},
	{-389, 1531,  -73,  -45},
	{-393, 1530,  -65,  -48},
	{-396, 1528,  -56,  -52},
	{-401, 1527,  -50,  -52},
	{-404, 1524,  -41,  -55},
	{-407, 1522,  -33,  -58},
	{-409, 1519,  -24,  -62},
	{-412, 1516,  -16,  -64},
	{-415, 1513,   -7,  -67},
	{-418, 1511,   -1,  -68},
	{-421, 1508,    8,  -71},
	{-424, 1505,   16,  -73},
	{-427, 1502,   25,  -76},
	{-430, 1500,   33,  -79},
	{-433, 1497,   42,  -82},
	{-434, 1492,   51,  -85},
	{-436, 1489,   59,  -88},
	{-439, 1486,   68,  -91},
	{-440, 1482,   77,  -95},
	{-443, 1479,   85,  -97},
	{-444, 1474,   94, -100},
	{-447, 1471,  103, -103},
	{-448, 1466,  112, -106},
	{-451, 1463,  120, -108},
	{-452, 1458,  131, -113},
	{-453, 1453,  140, -116},
	{-454, 1448,  149, -119},
	{-455, 1443,  158, -122},
	{-458, 1441,  167, -126},
	{-459, 1436,  176, -129},
	{-460, 1431,  185, -132},
	{-462, 1426,  196, -136},
	{-463, 1421,  205, -139},
	{-464, 1416,  214, -142},
	{-465, 1411,  223, -145},
	{-464, 1403,  234, -149},
	{-465, 1398,  243, -152},
	{-466, 1393,  252, -155},
	{-467, 1388,  261, -158},
	{-466, 1381,  272, -163},
	{-467, 1376,  281, -166},
	{-468, 1371,  290, -169},
	{-467, 1364,  299, -172},
	{-468, 1359,  310, -177},
	{-467, 1351,  320, -180},
	{-468, 1346,  329, -183},
	{-468, 1339,  340, -187},
	{-469, 1334,  349, -190},
	{-468, 1327,  359, -194},
	{-467, 1319,  368, -196},
	{-468, 1314,  379, -201},
	{-467, 1307,  388, -204},
	{-466, 1300,  398, -208},
	{-465, 1292,  409, -212},
	{-464, 1285,  418, -215},
	{-465, 1280,  428, -219},
	{-465, 1273,  439, -223},
	{-464, 1265,  448, -225},
	{-463, 1258,  458, -229},
	{-462, 1251,  469, -234},
	{-461, 1243,  478, -236},
	{-460, 1236,  488, -240},
	{-457, 1226,  499, -244},
	{-456, 1219,  509, -248},
	{-455, 1212,  518, -251},
	{-454, 1204,  530, -256},
	{-453, 1197,  539, -259},
	{-453, 1190,  548, -261},
	{-450, 1180,  560, -266},
	{-449, 1173,  569, -269},
	{-448, 1165,  579, -272},
	{-445, 1156,  590, -277},
	{-444, 1148,  600, -280},
	{-443, 1141,  609, -283},
	{-440, 1131,  621, -288},
	{-439, 1124,  630, -291},
	{-436, 1114,  640, -294},
	{-436, 1107,  649, -296},
	{-433, 1097,  661, -301},
	{-432, 1090,  671, -305},
	{-429, 1080,  680, -307},
	{-426, 1071,  690, -311},
	{-425, 1063,  701, -315},
	{-422, 1054,  711, -319},
	{-419, 1044,  720, -321},
	{-418, 1037,  730, -325},
	{-415, 1027,  739, -327},
	{-412, 1018,  751, -333},
	{-412, 1010,  761, -335},
	{-409, 1001,  770, -338},
	{-406,  991,  780, -341},
	{-403,  981,  789, -343},
	{-400,  972,  799, -347},
	{-397,  962,  811, -352},
	{-396,  955,  820, -355},
	{-393,  945,  830, -358},
	{-390,  935,  839, -360},
	{-387,  926,  849, -364},
	{-384,  916,  859, -367},
	{-381,  907,  868, -370},
	{-378,  897,  878, -373},
	{-375,  887,  887, -375}
};

int scl_coef_lp[128][4] = {
	{226,  455,  229,  114},
	{224,  454,  230,  116},
	{223,  454,  231,  116},
	{221,  454,  232,  117},
	{220,  453,  233,  118},
	{219,  453,  234,  118},
	{218,  452,  236,  118},
	{216,  452,  237,  119},
	{214,  451,  238,  121},
	{213,  451,  239,  121},
	{212,  450,  240,  122},
	{211,  450,  241,  122},
	{209,  449,  242,  124},
	{208,  449,  243,  124},
	{207,  448,  244,  125},
	{206,  448,  246,  124},
	{205,  447,  247,  125},
	{203,  447,  248,  126},
	{202,  446,  249,  127},
	{201,  446,  250,  127},
	{200,  445,  251,  128},
	{199,  445,  252,  128},
	{198,  444,  254,  128},
	{197,  444,  255,  128},
	{196,  443,  256,  129},
	{194,  443,  257,  130},
	{194,  442,  258,  130},
	{192,  442,  259,  131},
	{191,  441,  260,  132},
	{190,  441,  261,  132},
	{189,  440,  262,  133},
	{189,  439,  264,  132},
	{188,  439,  265,  132},
	{187,  438,  266,  133},
	{186,  438,  267,  133},
	{185,  437,  268,  134},
	{184,  436,  269,  135},
	{183,  436,  270,  135},
	{182,  435,  271,  136},
	{181,  434,  272,  137},
	{181,  434,  274,  135},
	{180,  433,  275,  136},
	{179,  433,  276,  136},
	{178,  432,  277,  137},
	{178,  431,  278,  137},
	{177,  431,  279,  137},
	{176,  430,  280,  138},
	{175,  429,  281,  139},
	{175,  429,  282,  138},
	{174,  428,  283,  139},
	{173,  427,  285,  139},
	{173,  427,  286,  138},
	{172,  426,  287,  139},
	{171,  425,  288,  140},
	{171,  425,  289,  139},
	{170,  424,  290,  140},
	{169,  423,  291,  141},
	{169,  422,  292,  141},
	{168,  422,  293,  141},
	{168,  421,  294,  141},
	{167,  420,  295,  142},
	{167,  420,  297,  140},
	{166,  419,  298,  141},
	{166,  418,  299,  141},
	{165,  417,  300,  142},
	{165,  417,  301,  141},
	{164,  416,  302,  142},
	{163,  415,  303,  143},
	{163,  414,  304,  143},
	{163,  414,  305,  142},
	{162,  413,  306,  143},
	{162,  412,  307,  143},
	{161,  411,  308,  144},
	{161,  411,  309,  143},
	{160,  410,  310,  144},
	{160,  409,  311,  144},
	{160,  408,  313,  143},
	{159,  407,  314,  144},
	{159,  407,  315,  143},
	{159,  406,  316,  143},
	{158,  405,  317,  144},
	{158,  404,  318,  144},
	{157,  404,  319,  144},
	{157,  403,  320,  144},
	{157,  402,  321,  144},
	{156,  401,  322,  145},
	{156,  400,  323,  145},
	{156,  399,  324,  145},
	{156,  398,  325,  145},
	{155,  398,  326,  145},
	{155,  397,  327,  145},
	{155,  396,  328,  145},
	{154,  395,  329,  146},
	{154,  394,  330,  146},
	{154,  393,  331,  146},
	{154,  393,  332,  145},
	{153,  392,  333,  146},
	{153,  391,  334,  146},
	{153,  390,  335,  146},
	{152,  389,  336,  147},
	{152,  388,  337,  147},
	{152,  388,  338,  146},
	{152,  387,  339,  146},
	{152,  386,  340,  146},
	{152,  385,  341,  146},
	{151,  384,  342,  147},
	{151,  383,  343,  147},
	{151,  382,  344,  147},
	{151,  381,  345,  147},
	{151,  380,  346,  147},
	{151,  379,  347,  147},
	{150,  379,  348,  147},
	{150,  378,  349,  147},
	{150,  377,  350,  147},
	{150,  376,  351,  147},
	{150,  375,  352,  147},
	{150,  374,  353,  147},
	{149,  373,  354,  148},
	{149,  372,  355,  148},
	{149,  371,  356,  148},
	{149,  370,  357,  148},
	{149,  369,  358,  148},
	{149,  369,  359,  147},
	{149,  368,  360,  147},
	{149,  367,  361,  147},
	{148,  366,  362,  148},
	{148,  365,  363,  148},
	{148,  364,  364,  148}
};

/****************************************************************************
 * Interfaces
 ****************************************************************************/
void sclr_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}
EXPORT_SYMBOL_GPL(sclr_set_base_addr);

/**
 * sclr_top_set_cfg - set scl-top's configurations.
 *
 * @param cfg: scl-top's settings.
 */
void sclr_top_set_cfg(struct sclr_top_cfg *cfg)
{
	unsigned long flags;
	union sclr_top_cfg_01 cfg_01;

	raw_spin_lock_irqsave(&__sc_top_lock, flags);
	_reg_write(reg_base + REG_SCL_TOP_CFG0,
		   (cfg->force_clk_enable << 31) |
		   (cfg->ip_trig_src << 3));

	cfg_01.raw = 0;
	cfg_01.b.sc_d_enable = cfg->sclr_enable[0];
	cfg_01.b.sc_v1_enable = cfg->sclr_enable[1];
	cfg_01.b.sc_v2_enable = cfg->sclr_enable[2];
	cfg_01.b.sc_v3_enable = cfg->sclr_enable[3];
	cfg_01.b.disp_enable = cfg->disp_enable;
	cfg_01.b.disp_from_sc = cfg->disp_from_sc;
	cfg_01.b.sc_d_src = cfg->sclr_d_src;
	cfg_01.b.sc_debug_en = 1;
	cfg_01.b.qos_en = 0xff;
	_reg_write(reg_base + REG_SCL_TOP_CFG1, cfg_01.raw);
	raw_spin_unlock_irqrestore(&__sc_top_lock, flags);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "cfg_01=0x%x\n", cfg_01.raw);

	sclr_img_set_trig(SCL_IMG_D, cfg->img_in_d_trig_src);
	sclr_img_set_trig(SCL_IMG_V, cfg->img_in_v_trig_src);

	sclr_top_reg_done();
	sclr_top_reg_force_up();
}
EXPORT_SYMBOL_GPL(sclr_top_set_cfg);

/**
 * sclr_rt_set_cfg - configure sc's channels real-time.
 *
 * @param cfg: sc's channels real-time settings.
 */
void sclr_rt_set_cfg(union sclr_rt_cfg cfg)
{
	_reg_write(reg_base + REG_SCL_TOP_AXI, cfg.raw);
}

union sclr_rt_cfg sclr_rt_get_cfg(void)
{
	union sclr_rt_cfg cfg;

	cfg.raw = _reg_read(reg_base + REG_SCL_TOP_AXI);
	return cfg;
}

/**
 * sclr_top_get_cfg - get scl_top's cfg
 *
 * @return: scl_top's cfg
 */
struct sclr_top_cfg *sclr_top_get_cfg(void)
{
	return &g_top_cfg;
}
EXPORT_SYMBOL_GPL(sclr_top_get_cfg);

/**
 * sclr_top_reg_done - to mark all sc-reg valid for update.
 *
 */
void sclr_top_reg_done(void)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_CFG0, BIT(0), BIT(0));
}

/**
 * sclr_top_reg_force_up - trigger reg update by sw.
 *
 */
void sclr_top_reg_force_up(void)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_SHD, 0x00ff, 0xff);
}

u8 sclr_top_pg_late_get_bus(void)
{
	return (_reg_read(reg_base + REG_SCL_TOP_PG) >> 8) & 0xff;
}

void sclr_top_pg_late_clr(void)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_PG, 0x0f0000, 0x80000);
}

void sclr_top_set_shd_reg(u32 val)
{
	_reg_write(reg_base + REG_SCL_TOP_SHD, val);
}

u32 sclr_top_get_shd_reg(void)
{
	return _reg_read(reg_base + REG_SCL_TOP_SHD);
}

/**
 * sclr_top_bld_set_cfg - configure blending
 *   if y-only enable, y-only data should be placed in img-d. uv data will pick from img-v.
 * @param cfg: blending's settings
 */
void sclr_top_bld_set_cfg(struct sclr_bld_cfg *cfg)
{
	u16 w = cfg->width;

	_reg_write(reg_base + REG_SCL_TOP_BLD_CTRL, cfg->ctrl.raw);
	_reg_write(reg_base + REG_SCL_TOP_BLD_SIZE, ((w - 1) << 16) | (0x10000 / w));
}

void sclr_top_get_sb_default(struct sclr_top_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
}

void sclr_top_set_sb(struct sclr_top_sb_cfg *cfg)
{
	u32 val = ((cfg->sb_wr_ctrl0 << SC_TOP_REG_SB_WR_CTRL0_OFFSET) & SC_TOP_REG_SB_WR_CTRL0_MASK) |
		  ((cfg->sb_wr_ctrl1 << SC_TOP_REG_SB_WR_CTRL1_OFFSET) & SC_TOP_REG_SB_WR_CTRL1_MASK) |
		  ((cfg->sb_rd_ctrl << SC_TOP_REG_SB_RD_CTRL_OFFSET) & SC_TOP_REG_SB_RD_CTRL_MASK);

	_reg_write(reg_base + REG_SC_TOP_SB_CBAR, val);
}

void sclr_top_set_vo_type_sel(enum sclr_vo_sel vo_sel)
{
	union  sclr_sc_top_vo_mux_sel vo_mux;

	vo_mux.raw = _reg_read(reg_base + REG_SCL_TOP_VO_MUX);
	vo_mux.b.vo_sel_type = vo_sel;

	_reg_write(reg_base + REG_SCL_TOP_VO_MUX, vo_mux.raw);
}

void sclr_top_set_vo_data_mux(u8 vodata_selID, u8 value)
{
	u32 vo_mux_addr[10] = {REG_SCL_TOP_VO_MUX0, REG_SCL_TOP_VO_MUX1, REG_SCL_TOP_VO_MUX2,
							REG_SCL_TOP_VO_MUX3, REG_SCL_TOP_VO_MUX4, REG_SCL_TOP_VO_MUX5,
							REG_SCL_TOP_VO_MUX6};

	union  sclr_sc_top_vo_mux vo_mux;
	u8 vodata_muxidx = (vodata_selID + 2) / 4;
	u8 vodata_selidx = (vodata_selID + 2) % 4;

	vo_mux.raw = _reg_read(reg_base + vo_mux_addr[vodata_muxidx]);

	switch (vodata_selidx) {
	case 0:
		vo_mux.b.vod_sel0 = value;
		break;
	case 1:
		vo_mux.b.vod_sel1 = value;
		break;
	case 2:
		vo_mux.b.vod_sel2 = value;
		break;
	case 3:
		vo_mux.b.vod_sel3 = value;
		break;
	}
	_reg_write(reg_base + vo_mux_addr[vodata_muxidx], vo_mux.raw);

}

void sclr_disp_bt_en(u8 vo_intf)
{
	switch (vo_intf) {
	case SCLR_VO_INTF_BT656:
		_reg_write(reg_base + REG_SCL_TOP_BT_CFG, 0x9);
		break;
	case SCLR_VO_INTF_BT1120:
		_reg_write(reg_base + REG_SCL_TOP_BT_CFG, 0xF);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL_GPL(sclr_disp_bt_en);

/**
 * sclr_top_vo_mux_sel - remap vo mux
 * @param vo_mux_sel: origin vo mux
 * @param vo_mux: mapped vo mux
 */
void sclr_top_vo_mux_sel(int vo_sel, int vo_mux)
{
	u32 value = 0;
	u32 reg_addr = REG_SCL_TOP_VO_MUX0 + (vo_sel / 4) * 4;
	u32 offset = (vo_sel % 4) * 8;

	if (vo_sel == 0) {
		_reg_write_mask(reg_base + REG_SCL_TOP_VO_MUX7, BIT(20), BIT(20));
	} else if (vo_sel == 1) {
		_reg_write_mask(reg_base + REG_SCL_TOP_VO_MUX7, BIT(21), BIT(21));
	}

	value = _reg_read(reg_base + reg_addr);
	value |= (vo_mux << offset);
	_reg_write(reg_base + reg_addr, value);
}
EXPORT_SYMBOL_GPL(sclr_top_vo_mux_sel);

void sclr_disp_set_mcu_en(u8 mode)
{
	_reg_write(reg_base + REG_SCL_DISP_MCU_HW_AUTO, mode?0x9:0x19);
}
EXPORT_SYMBOL_GPL(sclr_disp_set_mcu_en);

/**
 * sclr_init - update sclr's scaling coef
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param coef: coef type.
 * @param param: only useful for SCL_COEF_USER to customized coef.
 */
void sclr_update_coef(u8 inst, enum sclr_scaling_coef coef, void *param)
{
	u8 i = 0;

	if (g_sc_cfg[inst].coef == coef)
		return;

	if (coef == SCL_COEF_BICUBIC) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (scl_coef[i][1] << 16) |
				   (scl_coef[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (scl_coef[i][3] << 16) |
				   (scl_coef[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_NEAREST) {
		int nearest_coef[4] = {0, 1024, 0, 0};

		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (nearest_coef[1] << 16) |
				   (nearest_coef[0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (nearest_coef[3] << 16) |
				   (nearest_coef[2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_BILINEAR) {
		int bilinear_coef[4] = {0, 1024, 0, 0};

		for (i = 0; i < 128; ++i) {
			bilinear_coef[1] -= 4;
			bilinear_coef[2] += 4;

			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (bilinear_coef[1] << 16) |
				   (bilinear_coef[0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (bilinear_coef[3] << 16) |
				   (bilinear_coef[2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_Z2) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (scl_coef_z2[i][1] << 16) |
				   (scl_coef_z2[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (scl_coef_z2[i][3] << 16) |
				   (scl_coef_z2[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_Z3) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (scl_coef_z3[i][1] << 16) |
				   (scl_coef_z3[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (scl_coef_z3[i][3] << 16) |
				   (scl_coef_z3[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_DOWNSCALE_SMOOTH) {
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (scl_coef_lp[i][1] << 16) |
				   (scl_coef_lp[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (scl_coef_lp[i][3] << 16) |
				   (scl_coef_lp[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	} else if (coef == SCL_COEF_USER) {
		int **coef = param;

		if (!param)
			return;
		for (i = 0; i < 128; ++i) {
			_reg_write(reg_base + REG_SCL_COEF1(inst),
				   (coef[i][1] << 16) |
				   (coef[i][0] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF2(inst),
				   (coef[i][3] << 16) |
				   (coef[i][2] & 0x0fff));
			_reg_write(reg_base + REG_SCL_COEF0(inst), (0x5 << 8) | i);
		}
	}
	g_sc_cfg[inst].coef = coef;
}

/**
 * sclr_init - setup sclr's default scaling coef, bicubic
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 */
void sclr_init(u8 inst)
{
	if (inst >= SCL_MAX_INST) {
		pr_err("[cvi-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_cont("for the requirement(%d)\n", inst);
		return;
	}

	sclr_update_coef(inst, SCL_COEF_BICUBIC, NULL);
}

/**
 * sclr_set_cfg - set scl's configurations.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param sc_bypass: if bypass scaling-engine.
 * @param gop_bypass: if bypass gop-engine.
 * @param cir_bypass: if bypass circle-engine.
 * @param odma_bypass: if bypass odma-engine.
 */
void sclr_set_cfg(u8 inst, bool sc_bypass, bool gop_bypass,
		  bool cir_bypass, bool odma_bypass)
{
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) sc_bypass=%d, gop_bypass=%d, cir_bypass=%d, odma_bypass=%d\n",
			inst, sc_bypass, gop_bypass, cir_bypass, odma_bypass);

	g_sc_cfg[inst].sc_bypass = sc_bypass;
	g_sc_cfg[inst].gop_bypass = gop_bypass;
	g_sc_cfg[inst].cir_bypass = cir_bypass;
	g_sc_cfg[inst].odma_bypass = odma_bypass;

	_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x800000f7, g_sc_cfg[inst].raw);
}

/**
 * sclr_get_cfg - get scl_core's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_core's cfg
 */
struct sclr_core_cfg *sclr_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_sc_cfg[inst];
}

/**
 * sclr_reg_shadow_sel - control the read reg-bank.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param read_shadow: true(shadow); false(working)
 */
void sclr_reg_shadow_sel(u8 inst, bool read_shadow)
{
	// bit[1]: 0: ip domain, 1: apb domain
	_reg_write(reg_base + REG_SCL_SHD(inst), (read_shadow ? 0x0 : 0x2));
}

void sclr_sc_reset(u8 inst)
{
	_reg_write(reg_base + REG_SCL_SHD(inst), 1);
}

/**
 * sclr_reg_force_up - trigger reg update by sw.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 */
void sclr_reg_force_up(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_SHD, BIT(3+inst), BIT(3+inst));
}

struct sclr_status sclr_get_status(u8 inst)
{
	u32 tmp = _reg_read(reg_base + REG_SCL_STATUS(inst));

	return *(struct sclr_status *)&tmp;
}

/**
 * sclr_set_input_size - update scl's input-size.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param src_rect: size of input-src
 * @param update: update parameter or not
 */
void sclr_set_input_size(u8 inst, struct sclr_size src_rect, bool update)
{
	_reg_write(reg_base + REG_SCL_SRC_SIZE(inst),
		   ((src_rect.h - 1) << 12) | (src_rect.w - 1));

	if (update)
		g_sc_cfg[inst].sc.src = src_rect;
}

/**
 * sclr_set_crop - update scl's crop-size/position.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param crop_rect: to specify pos/size of crop
 * @param update: update crop parameter or not
 */
void sclr_set_crop(u8 inst, struct sclr_rect crop_rect, bool update)
{
	_reg_write(reg_base + REG_SCL_CROP_OFFSET(inst),
		   (crop_rect.y << 12) | crop_rect.x);
	_reg_write(reg_base + REG_SCL_CROP_SIZE(inst),
		   ((crop_rect.h - 1) << 16) | (crop_rect.w - 1));

	if (update)
		g_sc_cfg[inst].sc.crop = crop_rect;
}

/**
 * sclr_set_output_size - update scl's output-size.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param src_rect: size of output-src
 */
void sclr_set_output_size(u8 inst, struct sclr_size size)
{
	_reg_write(reg_base + REG_SCL_OUT_SIZE(inst),
		   ((size.h - 1) << 16) | (size.w - 1));

	g_sc_cfg[inst].sc.dst = size;
}

/**
 * sclr_set_scale_mode - control sclr's scaling coef.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param mir_enable: mirror enable or not
 * @param cb_enable: cb mode or not. cb only work if scaling smaller than 1/4.
 *                   In cb mode, use it's own coeff rather than ones update by sclr_update_coef().
 * @param update: update parameter or not
 */
void sclr_set_scale_mode(u8 inst, bool mir_enable, bool cb_enable, bool update)
{
	u32 tmp = 0;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) mir_enable=%d, cb_enable=%d\n",
			inst, mir_enable, cb_enable);

	if (update) {
		g_sc_cfg[inst].sc.mir_enable = mir_enable;
		g_sc_cfg[inst].sc.cb_enable = cb_enable;
	}

	// mir/cb isn't suggested to enable together
	if (g_sc_cfg[inst].sc.tile_enable) {
		tmp = mir_enable ? 0x03 : 0x02;
	} else {
		tmp = cb_enable ? 0x13 : 0x03;
	}

	// enable cb_mode of scaling, since it only works on fac > 4
	_reg_write(reg_base + REG_SCL_SC_CFG(inst), tmp);
}

void sclr_set_scale_mir(u8 inst, bool enable)
{
	u32 tmp = 0;

	g_sc_cfg[inst].sc.mir_enable = enable;
	tmp = (enable) ? 0x03 : 0x02;
	// enable cb_mode of scaling, since it only works on fac > 4
	_reg_write(reg_base + REG_SCL_SC_CFG(inst), 0x10 | tmp);
}

/**
 * sclr_set_scale_phase - Used in tile mode to decide the phase of first data in scaling.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param h_ph: initial horizontal phase
 * @param v_ph: initial vertical phase
 */
void sclr_set_scale_phase(u8 inst, u16 h_ph, u16 v_ph)
{
	_reg_write(reg_base + REG_SCL_SC_INI_PH(inst), (v_ph << 16) | h_ph);
}

/**
 * sclr_check_factor_over4 - check if scaling down over 4 times.
 *
 * @param inst: (0~2)
 */
bool sclr_check_factor_over4(u8 inst)
{
	u32 tmp = 0;
	bool isFacOver4 = false;

	tmp = (g_sc_cfg[inst].sc.crop.w - 1) / (g_sc_cfg[inst].sc.dst.w - 1);
	isFacOver4 |= (tmp >= 4);

	tmp = (g_sc_cfg[inst].sc.crop.h - 1) / (g_sc_cfg[inst].sc.dst.h - 1);
	isFacOver4 |= (tmp >= 4);

	return isFacOver4;
}

void sclr_get_2tap_scale(struct sclr_scale_2tap_cfg *cfg)
{
	u32 src_wd, src_ht, dst_wd, dst_ht, scale_x, scale_y;
	bool fast_area_x = 0, fast_area_y = 0;
	u32 h_nor = 0, v_nor = 0, h_ph = 0, v_ph = 0;
	bool area_fast = false;

	if (!cfg)
		return;
	if (!cfg->src_wd || !cfg->src_ht || !cfg->dst_wd || !cfg->dst_ht)
		return;

	src_wd = cfg->src_wd;
	src_ht = cfg->src_ht;
	dst_wd = cfg->dst_wd;
	dst_ht = cfg->dst_ht;

	// Scale up: bilinear mode
	// Scale down: area mode
	if (src_wd >= dst_wd && src_ht >= dst_ht) {
		fast_area_x = (src_wd % dst_wd) ? false : true;
		fast_area_y = (src_ht % dst_ht) ? false : true;
		area_fast = (fast_area_x && fast_area_y) ? true : false;
	}

	// scale_x = round((src_wd * 2^13)/dst_wd),0) -> 5.13
	scale_x = (src_wd * 8192) / dst_wd;

	// scale_y = round((src_ht * 2^13)/dst_ht),0) -> 5.13
	scale_y = (src_ht * 8192) / dst_ht;

	if (!area_fast) {
		h_nor =  (65536 * 8192) / scale_x;
		v_nor = (65536 * 8192) / scale_y;
	} else {
		// h_nor: don't care
		h_nor =  (65536 * 8192) / scale_x;
		v_nor = 65536 / ((scale_x >> 13) * (scale_y >> 13));
	}

	// Phase is used when scale up and reg_resize_blnr_mode = 0
	// Note: scale down with nonzero phase caused scaler blocked
	if (src_wd < dst_wd)
		h_ph = scale_x / 2;

	if (src_ht < dst_ht)
		v_ph = scale_y / 2;

	cfg->area_fast = area_fast;
	cfg->scale_x = scale_x;
	cfg->scale_y = scale_y;
	cfg->h_nor = h_nor;
	cfg->v_nor = v_nor;
	cfg->h_ph = h_ph;
	cfg->v_ph = v_ph;
}

void sclr_set_scale_2tap(u8 inst)
{
	u32 tmp;
	bool isFacOver4 = false;
	struct sclr_scale_2tap_cfg cfg;
	u32 blnr_mode = 0; // 0: 0 : neg-phase, sync with IVE C-code
	u32 dbg_sel = 1;

	if (g_sc_cfg[inst].sc.src.w == 0 || g_sc_cfg[inst].sc.src.h == 0)
		return;
	if (g_sc_cfg[inst].sc.crop.w == 0 || g_sc_cfg[inst].sc.crop.h == 0)
		return;
	if (g_sc_cfg[inst].sc.dst.w == 0 || g_sc_cfg[inst].sc.dst.h == 0)
		return;

	cfg.src_wd = g_sc_cfg[inst].sc.crop.w;
	cfg.src_ht = g_sc_cfg[inst].sc.crop.h;
	cfg.dst_wd = g_sc_cfg[inst].sc.dst.w;
	cfg.dst_ht = g_sc_cfg[inst].sc.dst.h;
	sclr_get_2tap_scale(&cfg);

	// scale_x, 5.13
	tmp = (cfg.scale_x << SC_CORE_REG_H_SC_FAC_OFFSET) & SC_CORE_REG_H_SC_FAC_MASK;
	_reg_write(reg_base + REG_SCL_SC_H_CFG(inst), tmp);

	// scale_y, 5.13
	tmp = (cfg.scale_y << SC_CORE_REG_V_SC_FAC_OFFSET) & SC_CORE_REG_V_SC_FAC_MASK;
	_reg_write(reg_base + REG_SCL_SC_V_CFG(inst), tmp);

	sclr_set_scale_mode(inst, g_sc_cfg[inst].sc.mir_enable, isFacOver4, true);

	tmp = (blnr_mode << SC_CORE_REG_RESIZE_BLNR_MODE_OFFSET) & SC_CORE_REG_RESIZE_BLNR_MODE_MASK;
	tmp |= (cfg.area_fast << SC_CORE_REG_RESIZE_AREA_FAST_OFFSET) & SC_CORE_REG_RESIZE_AREA_FAST_MASK;
	tmp |= (dbg_sel << SC_CORE_REG_RESIZE_DBG_SEL_OFFSET) & SC_CORE_REG_RESIZE_DBG_SEL_MASK;
	_reg_write(reg_base + REG_SCL_2TAP_CFG(inst), tmp);

	tmp = ((cfg.h_nor << SC_CORE_REG_RESIZE_H_NOR_OFFSET) & SC_CORE_REG_RESIZE_H_NOR_MASK);
	tmp |= ((cfg.v_nor << SC_CORE_REG_RESIZE_V_NOR_OFFSET) & SC_CORE_REG_RESIZE_V_NOR_MASK);
	_reg_write(reg_base + REG_SCL_2TAP_NOR(inst), tmp);

	sclr_set_scale_phase(inst, cfg.h_ph, cfg.v_ph);
}

/**
 * sclr_set_scale - update scl's scaling settings by current crop/dst values.
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 */
void sclr_set_scale(u8 inst)
{
	u32 tmp = 0;
	bool isFacOver4 = false;
	bool use_2tap = (inst != 1) ? true : false;	// Only sc_v1 is 4 tap

	if (g_sc_cfg[inst].sc.src.w == 0 || g_sc_cfg[inst].sc.src.h == 0)
		return;
	if (g_sc_cfg[inst].sc.crop.w == 0 || g_sc_cfg[inst].sc.crop.h == 0)
		return;
	if (g_sc_cfg[inst].sc.dst.w == 0 || g_sc_cfg[inst].sc.dst.h == 0)
		return;

	if (use_2tap) {
		sclr_set_scale_2tap(inst);
		return;
	}

	tmp = (((g_sc_cfg[inst].sc.crop.w - 1) << 13)
		+ (g_sc_cfg[inst].sc.dst.w >> 1))
	      / MAX((g_sc_cfg[inst].sc.dst.w - 1), 1);
	_reg_write_mask(reg_base + REG_SCL_SC_H_CFG(inst), 0x03ffff00,
			tmp << 8);

	tmp = (((g_sc_cfg[inst].sc.crop.h - 1) << 13)
		+ (g_sc_cfg[inst].sc.dst.h >> 1))
	      / MAX((g_sc_cfg[inst].sc.dst.h - 1), 1);
	_reg_write_mask(reg_base + REG_SCL_SC_V_CFG(inst), 0x03ffff00,
			tmp << 8);

	isFacOver4 = sclr_check_factor_over4(inst);
	sclr_set_scale_mode(inst, g_sc_cfg[inst].sc.mir_enable, isFacOver4, true);

	sclr_set_scale_phase(inst, 0, 0);
}

void sclr_read_2tap_nor(u8 inst, u16 *resize_hnor, u16 *resize_vnor)
{
	u32 val = _reg_read(reg_base + REG_SCL_2TAP_NOR(inst));

	*resize_hnor = (val & SC_CORE_REG_RESIZE_H_NOR_MASK) >> SC_CORE_REG_RESIZE_H_NOR_OFFSET;
	*resize_vnor = (val & SC_CORE_REG_RESIZE_V_NOR_MASK) >> SC_CORE_REG_RESIZE_V_NOR_OFFSET;
}

union sclr_intr sclr_get_intr_mask(void)
{
	union sclr_intr intr_mask;

	intr_mask.raw = _reg_read(reg_base + REG_SCL_TOP_INTR_MASK);
	return intr_mask;
}
EXPORT_SYMBOL_GPL(sclr_get_intr_mask);

/**
 * sclr_set_intr_mask - sclr's interrupt mask. Only enable ones will be
 *			integrated to vip_subsys.  check 'union sclr_intr'
 *			for each bit mask.
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_set_intr_mask(union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_MASK, intr_mask.raw);
}
EXPORT_SYMBOL_GPL(sclr_set_intr_mask);

/**
 * sclr_intr_ctrl - sclr's interrupt on(1)/off(0)
 *                  check 'union sclr_intr' for each bit mask
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_intr_ctrl(union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_ENABLE, intr_mask.raw);
}

/**
 * sclr_intr_clr - clear sclr's interrupt
 *                 check 'union sclr_intr' for each bit mask
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void sclr_intr_clr(union sclr_intr intr_mask)
{
	_reg_write(reg_base + REG_SCL_TOP_INTR_STATUS, intr_mask.raw);
}

/**
 * sclr_intr_status - sclr's interrupt status
 *                    check 'union sclr_intr' for each bit mask
 *
 * @return: The interrupt's status
 */
union sclr_intr sclr_intr_status(void)
{
	union sclr_intr status;

	status.raw = (_reg_read(reg_base + REG_SCL_TOP_INTR_STATUS) & 0xffffffff);
	return status;
}
EXPORT_SYMBOL_GPL(sclr_intr_status);

/****************************************************************************
 * SCALER IMG
 ****************************************************************************/
/**
 * sclr_img_set_cfg - set img's configurations.
 *
 * @param inst: (0~1), the instance of img-in which want to be configured.
 * @param cfg: img's settings.
 */
void sclr_img_set_cfg(u8 inst, struct sclr_img_cfg *cfg)
{
	if (cfg->fmt == SCL_FMT_YUV420 || cfg->fmt == SCL_FMT_NV12 || cfg->fmt == SCL_FMT_NV21) {
		u8 fifo_rd_th_y, fifo_pr_th_y, fifo_rd_th_c, fifo_pr_th_c, pre_fetch_raw;

		pre_fetch_raw = 2 * (cfg->mem.width + 1) / 16 - 1;
		fifo_rd_th_y = MIN(pre_fetch_raw, 32);
		fifo_pr_th_y = fifo_rd_th_y / 2;
		fifo_rd_th_c = fifo_rd_th_y;
		fifo_pr_th_c = fifo_pr_th_y;
		_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(inst), 0x000000ff, fifo_rd_th_y);
		_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(inst), 0x0000ff00, fifo_pr_th_y << 8);
		_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(inst), 0x00ff0000, fifo_rd_th_c << 16);
		_reg_write_mask(reg_base + REG_SCL_IMG_FIFO_THR(inst), 0xff000000, fifo_pr_th_c << 24);
	}

	_reg_write(reg_base + REG_SCL_IMG_CFG(inst),
		   (cfg->force_clk << 31) |
		   (cfg->csc_en << 12) |
		   (cfg->burst << 8) |
		   (cfg->fmt << 4) |
		   cfg->src);

	sclr_img_set_mem(inst, &cfg->mem, true);
	sclr_img_set_trig(inst, cfg->trig_src);

	//uartlog("DBG src(%d) inst(%d)\n", cfg->src, inst);
	if (cfg->csc == SCL_CSC_NONE) {
		sclr_img_csc_en(inst, false);
	} else {
		sclr_img_csc_en(inst, true);
		sclr_img_set_csc(inst, &csc_mtrx[cfg->csc]);
	}
	g_img_cfg[inst] = *cfg;
}

/**
 * sclr_img_set_trig - set img's src of job_start.
 *
 * @param inst: (0~1), the instance of img-in which want to be configured.
 * @param trig_src: img's src of job_start.
 */
void sclr_img_set_trig(u8 inst, enum sclr_img_trig_src trig_src)
{
	u32 mask = BIT(12) | BIT(8);
	u32 val = 0;
	unsigned long flags;

	raw_spin_lock_irqsave(&__sc_top_lock, flags);
	switch (trig_src) {
	default:
	case SCL_IMG_TRIG_SRC_SW:
		break;
	case SCL_IMG_TRIG_SRC_DISP:
		val = BIT(8);
		break;
	case SCL_IMG_TRIG_SRC_ISP:
		val = BIT(12);
		break;
	}

	if (inst == SCL_IMG_V) {
		mask <<= 1;
		val <<= 1;
		g_top_cfg.img_in_v_trig_src = trig_src;
	} else {
		g_top_cfg.img_in_d_trig_src = trig_src;
	}
	//uartlog("mask(%#x) val(%#x)\n", mask, val);
	_reg_write_mask(reg_base + REG_SCL_TOP_IMG_CTRL, mask, val);
	raw_spin_unlock_irqrestore(&__sc_top_lock, flags);
}

/**
 * sclr_img_get_cfg - get scl_img's cfg
 *
 * @param inst: 0~1
 * @return: scl_img's cfg
 */
struct sclr_img_cfg *sclr_img_get_cfg(u8 inst)
{
	if (inst >= SCL_IMG_MAX)
		return NULL;
	return &g_img_cfg[inst];
}

/**
 * sclr_img_reg_shadow_sel - control scl_img's the read reg-bank.
 *
 * @param read_shadow: true(shadow); false(working)
 */
void sclr_img_reg_shadow_sel(u8 inst, bool read_shadow)
{
	_reg_write(reg_base + REG_SCL_IMG_SHD(inst), (read_shadow ? 0x0 : 0x4));
}

/**
 * sclr_img_reg_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param mask: true(mask); false(unmask)
 * @return: mask status before this modification.
 */
bool sclr_img_reg_shadow_mask(u8 inst, bool mask)
{
	bool is_masked = (_reg_read(reg_base + REG_SCL_IMG_SHD(inst)) & BIT(1));

	if (is_masked != mask)
		_reg_write_mask(reg_base + REG_SCL_IMG_SHD(inst), BIT(1),
				(mask ? 0x0 : BIT(1)));

	return is_masked;
}

/**
 * sclr_img_reg_force_up - trigger reg update by sw.
 *
 * @param inst: 0~1
 */
void sclr_img_reg_force_up(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_SHD, BIT(1+inst), BIT(1+inst));
}

void sclr_img_reset(u8 inst)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_DBG(inst), 0x00040000, BIT(18));
}


void sclr_img_start(u8 inst)
{
	// toggle reg_ip_clr_w1t to avoid img hang when switching from online to offline
	sclr_img_reset(inst);

	inst = (~inst) & 0x01;
	_reg_write_mask(reg_base + REG_SCL_TOP_IMG_CTRL, 0x00000003, BIT(inst));
}

/**
 * sclr_img_set_fmt - set scl_img's input data format
 *
 * @param inst: 0~1
 * @param fmt: scl_img's input format
 */
void sclr_img_set_fmt(u8 inst, enum sclr_format fmt)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CFG(inst), 0x000000f0, fmt << 4);

	g_img_cfg[inst].fmt = fmt;
}

/**
 * sclr_img_set_mem - setup img's mem settings. Only work if img from mem.
 *
 * @param mem: mem settings for img
 * @param update: update parameter or not
 */
void sclr_img_set_mem(u8 inst, struct sclr_mem *mem, bool update)
{
	_reg_write(reg_base + REG_SCL_IMG_OFFSET(inst),
		   (mem->start_y << 16) | mem->start_x);
	_reg_write(reg_base + REG_SCL_IMG_SIZE(inst),
		   ((mem->height - 1) << 16) | (mem->width - 1));
	_reg_write(reg_base + REG_SCL_IMG_PITCH_Y(inst), mem->pitch_y);
	_reg_write(reg_base + REG_SCL_IMG_PITCH_C(inst), mem->pitch_c);

	sclr_img_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	if (update)
		g_img_cfg[inst].mem = *mem;
}

/**
 * sclr_img_set_addr - setup img's mem address. Only work if img from mem.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void sclr_img_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
{
	_reg_write(reg_base + REG_SCL_IMG_ADDR0_L(inst), addr0);
	_reg_write(reg_base + REG_SCL_IMG_ADDR0_H(inst), addr0 >> 32);
	_reg_write(reg_base + REG_SCL_IMG_ADDR1_L(inst), addr1);
	_reg_write(reg_base + REG_SCL_IMG_ADDR1_H(inst), addr1 >> 32);
	_reg_write(reg_base + REG_SCL_IMG_ADDR2_L(inst), addr2);
	_reg_write(reg_base + REG_SCL_IMG_ADDR2_H(inst), addr2 >> 32);

	g_img_cfg[inst].mem.addr0 = addr0;
	g_img_cfg[inst].mem.addr1 = addr1;
	g_img_cfg[inst].mem.addr2 = addr2;
}

void sclr_img_csc_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CFG(inst), BIT(12),
			enable ? BIT(12) : 0);
}

/**
 * sclr_img_set_csc - configure scl-img's input CSC's coefficient/offset
 *
 * @param inst: (0~1)
 * @param cfg: The settings for CSC
 */
void sclr_img_set_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF0(inst),
		   (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF1(inst),
		   cfg->coef[0][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF2(inst),
		   (cfg->coef[1][1] << 16) | cfg->coef[1][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF3(inst), cfg->coef[1][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF4(inst),
		   (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_COEF5(inst), cfg->coef[2][2]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_SUB(inst),
		   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) | cfg->sub[0]);
	_reg_write(reg_base + REG_SCL_IMG_CSC_ADD(inst),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) | cfg->add[0]);
}

union sclr_img_dbg_status sclr_img_get_dbg_status(u8 inst, bool clr)
{
	union sclr_img_dbg_status status;

	status.raw = _reg_read(reg_base + REG_SCL_IMG_DBG(inst));

	if (clr) {
		status.b.err_fwr_clr = 1;
		status.b.err_erd_clr = 1;
		status.b.ip_clr = 1;
		status.b.ip_int_clr = 1;
		_reg_write(reg_base + REG_SCL_IMG_DBG(inst), status.raw);
	}

	return status;
}

void sclr_img_checksum_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_IMG_CHECKSUM0(inst), BIT(31),
	                enable ? BIT(31) : 0);
}

void sclr_img_get_checksum_status(u8 inst, struct sclr_img_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(reg_base + REG_SCL_IMG_CHECKSUM0(inst));
	status->axi_read_data = _reg_read(reg_base + REG_SCL_IMG_CHECKSUM1(inst));

}

int sclr_img_validate_sb_cfg(struct sclr_img_in_sb_cfg *cfg)
{
	/* 0 : disable, 1 : free-run mode, 2 : frame base mode */
	if (cfg->sb_mode > 2)
		return -1;

	/* 0 : 64 line, 1 : 128 line, 2 : 192 line, 3 : 256 line */
	if (cfg->sb_size > 3)
		return -1;

	return 0;
}

void sclr_img_get_sb_default(struct sclr_img_in_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	//cfg->mode = 0;	// disable
	//cfg->size = 0;	// 64 line
	cfg->sb_nb = 3;
	cfg->sb_sw_rptr = 0;
	//cfg->set_str = 0;
	//cfg->sw_clr = 0;
}

void sclr_img_set_sb(u8 inst, struct sclr_img_in_sb_cfg *cfg)
{
	u32 val;

	if (inst >= 2)
		return;

	if (sclr_img_validate_sb_cfg(cfg))
		return;

	val = ((cfg->sb_mode << IMG_IN_REG_SB_MODE_OFFSET) & IMG_IN_REG_SB_MODE_MASK) |
	      ((cfg->sb_size << IMG_IN_REG_SB_SIZE_OFFSET) & IMG_IN_REG_SB_SIZE_MASK) |
	      ((cfg->sb_nb << IMG_IN_REG_SB_NB_OFFSET) & IMG_IN_REG_SB_NB_MASK) |
	      ((cfg->sb_sw_rptr << IMG_IN_REG_SB_SW_RPTR_OFFSET) & IMG_IN_REG_SB_SW_RPTR_MASK) |
	      ((cfg->sb_set_str << IMG_IN_REG_SB_SET_STR_OFFSET) & IMG_IN_REG_SB_SET_STR_MASK) |
	      ((cfg->sb_sw_clr << IMG_IN_REG_SB_SW_CLR_OFFSET) & IMG_IN_REG_SB_SW_CLR_MASK);
	_reg_write(reg_base + REG_SCL_IMG_SB_REG_CTRL(inst), val);
}

void sclr_img_set_dwa_to_sclr_sb(u8 inst, u8 sb_mode, u8 sb_size, u8 sb_nb)
{
	struct sclr_img_in_sb_cfg img_sb_cfg;

	// img_in
	sclr_img_get_sb_default(&img_sb_cfg);
	img_sb_cfg.sb_mode = sb_mode;
	img_sb_cfg.sb_size = sb_size;
	img_sb_cfg.sb_nb = sb_nb;
	sclr_img_set_sb(inst, &img_sb_cfg);

	// top
	g_top_sb_cfg.sb_rd_ctrl = (~inst) & 0x01; // 0: img_in_d, 1: img_in_v
	sclr_top_set_sb(&g_top_sb_cfg);
}

/****************************************************************************
 * SCALER CIRCLE
 ****************************************************************************/
/**
 * sclr_cir_set_cfg - configure scl-circle
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: cir's settings.
 */
void sclr_cir_set_cfg(u8 inst, struct sclr_cir_cfg *cfg)
{
	if (cfg->mode == SCL_CIR_DISABLE) {
		_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x20, 0x20);
	} else {
		_reg_write(reg_base + REG_SCL_CIR_CFG(inst),
			   (cfg->line_width << 8) | cfg->mode);
		_reg_write(reg_base + REG_SCL_CIR_CENTER_X(inst),
			   cfg->center.x);
		_reg_write(reg_base + REG_SCL_CIR_CENTER_Y(inst),
			   cfg->center.y);
		_reg_write(reg_base + REG_SCL_CIR_RADIUS(inst), cfg->radius);
		_reg_write(reg_base + REG_SCL_CIR_SIZE(inst),
			   ((cfg->rect.h - 1) << 16) | (cfg->rect.w - 1));
		_reg_write(reg_base + REG_SCL_CIR_OFFSET(inst),
			   (cfg->rect.y << 16) | cfg->rect.x);
		_reg_write(reg_base + REG_SCL_CIR_COLOR(inst),
			   (cfg->color_b << 16) | (cfg->color_g << 8) |
			   cfg->color_r);

		_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x20, 0x00);
	}

	g_cir_cfg[inst] = *cfg;
}

/****************************************************************************
 * SCALER BORDER
 ****************************************************************************/
/**
 * sclr_border_set_cfg - configure scaler's border
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: new border config
 */
void sclr_border_set_cfg(u8 inst, struct sclr_border_cfg *cfg)
{
	if (cfg->cfg.b.enable) {
		if ((g_sc_cfg[inst].sc.dst.w + cfg->start.x) >
		    g_odma_cfg[inst].mem.width) {
			pr_err("[cvi-vip][sc] %s: sc_width(%d) + offset(%d)", __func__,
			       g_sc_cfg[inst].sc.dst.w, cfg->start.x);
			pr_cont(" > odma_width(%d)\n", g_odma_cfg[inst].mem.width);
			cfg->start.x = g_odma_cfg[inst].mem.width -
				       g_sc_cfg[inst].sc.dst.w;
		}

		if ((g_sc_cfg[inst].sc.dst.h + cfg->start.y) >
		    g_odma_cfg[inst].mem.height) {
			pr_err("[cvi-vip][sc] %s: sc_height(%d) + offset(%d)", __func__,
			       g_sc_cfg[inst].sc.dst.h, cfg->start.y);
			pr_cont(" > odma_height(%d)\n", g_odma_cfg[inst].mem.height);
			cfg->start.y = g_odma_cfg[inst].mem.height -
				       g_sc_cfg[inst].sc.dst.h;
		}
	}

	_reg_write(reg_base + REG_SCL_BORDER_CFG(inst), cfg->cfg.raw);
	_reg_write(reg_base + REG_SCL_BORDER_OFFSET(inst),
		   (cfg->start.y << 16) | cfg->start.x);

	g_bd_cfg[inst] = *cfg;
}

/**
 * sclr_border_get_cfg - get scl_border's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_border's cfg
 */
struct sclr_border_cfg *sclr_border_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_bd_cfg[inst];
}

/****************************************************************************
 * SCALER ODMA
 ****************************************************************************/
/**
 * sclr_odma_set_cfg - configure scaler's odma
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param burst: dma's burst length
 * @param fmt: dma's format
 */
void sclr_odma_set_cfg(u8 inst, struct sclr_odma_cfg *cfg)
{
	_reg_write(reg_base + REG_SCL_ODMA_CFG(inst),
		   (cfg->flip << 16) |  (cfg->fmt << 8) | cfg->burst);

	sclr_odma_set_mem(inst, &cfg->mem);

	g_odma_cfg[inst] = *cfg;
}

/**
 * sclr_odma_get_cfg - get scl_odma's cfg
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @return: scl_odma's cfg
 */
struct sclr_odma_cfg *sclr_odma_get_cfg(u8 inst)
{
	if (inst >= SCL_MAX_INST)
		return NULL;
	return &g_odma_cfg[inst];
}

/**
 * sclr_odma_set_fmt - set scl_odma's output data format
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param fmt: scl_odma's output format
 */
void sclr_odma_set_fmt(u8 inst, enum sclr_format fmt)
{
	u32 tmp = fmt << 8;

	if (fmt == SCL_FMT_BF16)
		tmp |= BIT(23);
	_reg_write_mask(reg_base + REG_SCL_ODMA_CFG(inst), 0x0080ff00, tmp);

	g_odma_cfg[inst].fmt = fmt;
}

/**
 * sclr_odma_set_mem - setup odma's mem settings.
 *
 * @param mem: mem settings for odma
 */
void sclr_odma_set_mem(u8 inst, struct sclr_mem *mem)
{
	_reg_write(reg_base + REG_SCL_ODMA_OFFSET_X(inst), mem->start_x);
	_reg_write(reg_base + REG_SCL_ODMA_OFFSET_Y(inst), mem->start_y);
	_reg_write(reg_base + REG_SCL_ODMA_WIDTH(inst), mem->width - 1);
	_reg_write(reg_base + REG_SCL_ODMA_HEIGHT(inst), mem->height - 1);
	_reg_write(reg_base + REG_SCL_ODMA_PITCH_Y(inst), mem->pitch_y);
	_reg_write(reg_base + REG_SCL_ODMA_PITCH_C(inst), mem->pitch_c);

	sclr_odma_set_addr(inst, mem->addr0, mem->addr1, mem->addr2);

	g_odma_cfg[inst].mem = *mem;
}

/**
 * sclr_odma_set_addr - setup odma's mem address.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void sclr_odma_set_addr(u8 inst, u64 addr0, u64 addr1, u64 addr2)
{
	_reg_write(reg_base + REG_SCL_ODMA_ADDR0_L(inst), addr0);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR0_H(inst), addr0 >> 32);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR1_L(inst), addr1);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR1_H(inst), addr1 >> 32);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR2_L(inst), addr2);
	_reg_write(reg_base + REG_SCL_ODMA_ADDR2_H(inst), addr2 >> 32);

	g_odma_cfg[inst].mem.addr0 = addr0;
	g_odma_cfg[inst].mem.addr1 = addr1;
	g_odma_cfg[inst].mem.addr2 = addr2;
}

union sclr_odma_dbg_status sclr_odma_get_dbg_status(u8 inst)
{
	union sclr_odma_dbg_status status;

	status.raw = _reg_read(reg_base + REG_SCL_ODMA_DBG(inst));

	return status;
}

void sclr_odma_get_sb_default(struct sclr_odma_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->sb_nb = 3;
	cfg->sb_full_nb = 2;
}

void sclr_odma_set_sb(u8 inst, struct sclr_odma_sb_cfg *cfg)
{
	u32 val = 0;

	val |= (cfg->sb_mode << SC_ODMA_REG_SB_MODE_OFFSET) & SC_ODMA_REG_SB_MODE_MASK;
	val |= (cfg->sb_size << SC_ODMA_REG_SB_SIZE_OFFSET) & SC_ODMA_REG_SB_SIZE_MASK;
	val |= (cfg->sb_nb << SC_ODMA_REG_SB_NB_OFFSET) & SC_ODMA_REG_SB_NB_MASK;
	val |= (cfg->sb_full_nb << SC_ODMA_REG_SB_FULL_NB_OFFSET) & SC_ODMA_REG_SB_FULL_NB_MASK;
	val |= (cfg->sb_sw_wptr << SC_ODMA_REG_SB_SW_WPTR_OFFSET) & SC_ODMA_REG_SB_SW_WPTR_MASK;
	val |= (cfg->sb_set_str << SC_ODMA_REG_SB_SET_STR_OFFSET) & SC_ODMA_REG_SB_SET_STR_MASK;
	val |= (cfg->sb_sw_clr << SC_ODMA_REG_SB_SW_CLR_OFFSET) & SC_ODMA_REG_SB_SW_CLR_MASK;

	_reg_write(reg_base + REG_SCL_ODMA_SB_CTRL(inst), val);
}

void sclr_odma_clear_sb(u8 inst)
{
	u32 val = 0;

	val |= (1 << SC_ODMA_REG_SB_SET_STR_OFFSET) & SC_ODMA_REG_SB_SET_STR_MASK;
	val |= (1 << SC_ODMA_REG_SB_SW_CLR_OFFSET) & SC_ODMA_REG_SB_SW_CLR_MASK;

	_reg_write(reg_base + REG_SCL_ODMA_SB_CTRL(inst), val);
}

/**
 * sclr_set_out_mode - Control sclr output's mode. CSC/Quantization/HSV
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param mode: csc/quant/hsv/none
 */
void sclr_set_out_mode(u8 inst, enum sclr_out_mode mode)
{
	u32 tmp = 0;

	switch (mode) {
	case SCL_OUT_CSC:
		tmp = BIT(0);
		break;
	case SCL_OUT_QUANT_BF16:
		tmp |= BIT(23);
		tmp |= BIT(1) | BIT(0);
		break;
	case SCL_OUT_QUANT:
		tmp |= BIT(1) | BIT(0);
		break;
	case SCL_OUT_HSV:
		tmp = BIT(4);
		break;
	case SCL_OUT_DISABLE:
		break;
	}

	_reg_write_mask(reg_base + REG_SCL_CSC_EN(inst), 0x01000013, tmp);

	g_odma_cfg[inst].csc_cfg.mode = mode;
}

/**
 * sclr_set_csc_ctrl - configure scl's output CSC's, including hsv/quant/coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings for CSC
 */
void sclr_set_csc_ctrl(u8 inst, struct sclr_csc_cfg *cfg)
{
	u32 tmp = 0;

	sclr_set_out_mode(inst, cfg->mode);

	if (cfg->mode == SCL_OUT_HSV) {
		if (cfg->hsv_round == SCL_HSV_ROUNDING_TOWARD_ZERO)
			tmp |= BIT(5);
		if (!cfg->work_on_border)
			tmp |= BIT(9);

		sclr_set_csc(inst, &csc_mtrx[SCL_CSC_NONE]);
	} else if (cfg->mode == SCL_OUT_QUANT) {
		tmp |= (cfg->quant_round << 2);
		if (!cfg->work_on_border)
			tmp |= BIT(8);
		if (cfg->quant_gain_mode == SCL_QUANT_GAIN_MODE_2)
			tmp |= BIT(11);

		sclr_set_quant(inst, &cfg->quant_form);
	} else if (cfg->mode == SCL_OUT_QUANT_BF16) {
		if (cfg->quant_border_type == SCL_QUANT_BORDER_TYPE_127)
			tmp |= BIT(10);
		if (!cfg->work_on_border)
			tmp |= BIT(8);
		if (cfg->quant_gain_mode == SCL_QUANT_GAIN_MODE_2)
			tmp |= BIT(11);

		sclr_set_quant(inst, &cfg->quant_form);
	} else {
		if (!cfg->work_on_border)
			tmp |= BIT(8);
		sclr_set_csc(inst, &csc_mtrx[cfg->csc_type]);
	}
	_reg_write_mask(reg_base + REG_SCL_CSC_EN(inst), 0x00000fec, tmp);

	g_odma_cfg[inst].csc_cfg = *cfg;
}

struct sclr_csc_cfg *sclr_get_csc_ctrl(u8 inst)
{
	if (inst < SCL_MAX_INST)
		return &g_odma_cfg[inst].csc_cfg;

	return NULL;
}

/**
 * sclr_set_csc - configure scl's output CSC's coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings for CSC
 */
void sclr_set_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	_reg_write(reg_base + REG_SCL_CSC_COEF0(inst),
		   (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF1(inst),
		   (cfg->coef[1][0] << 16) | cfg->coef[0][2]);
	_reg_write(reg_base + REG_SCL_CSC_COEF2(inst),
		   (cfg->coef[1][2] << 16) | cfg->coef[1][1]);
	_reg_write(reg_base + REG_SCL_CSC_COEF3(inst),
		   (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF4(inst), cfg->coef[2][2]);

	_reg_write(reg_base + REG_SCL_CSC_OFFSET(inst),
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) | cfg->add[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC0(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_FRAC1(inst), 0);
}



/**
 * sclr_set_quant - set quantization by csc module
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings of quantization
 */
void sclr_set_quant(u8 inst, struct sclr_quant_formula *cfg)
{
	_reg_write(reg_base + REG_SCL_CSC_COEF0(inst), cfg->sc_frac[0]);
	_reg_write(reg_base + REG_SCL_CSC_COEF1(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_COEF2(inst), cfg->sc_frac[1]);
	_reg_write(reg_base + REG_SCL_CSC_COEF3(inst), 0);
	_reg_write(reg_base + REG_SCL_CSC_COEF4(inst), cfg->sc_frac[2]);

	_reg_write(reg_base + REG_SCL_CSC_OFFSET(inst), (cfg->sub[2] << 16) | (cfg->sub[1] << 8) | cfg->sub[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC0(inst), (cfg->sub_frac[1] << 16) | cfg->sub_frac[0]);
	_reg_write(reg_base + REG_SCL_CSC_FRAC1(inst), cfg->sub_frac[2]);
}

/**
 * sclr_get_csc - get current CSC's coefficient/offset
 *
 * @param inst: (0~3), the instance of scaler which want to be configured.
 * @param cfg: The settings of CSC
 */
void sclr_get_csc(u8 inst, struct sclr_csc_matrix *cfg)
{
	u32 tmp;

	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF0(inst));
	cfg->coef[0][0] = tmp;
	cfg->coef[0][1] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF1(inst));
	cfg->coef[0][2] = tmp;
	cfg->coef[1][0] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF2(inst));
	cfg->coef[1][1] = tmp;
	cfg->coef[1][2] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF3(inst));
	cfg->coef[2][0] = tmp;
	cfg->coef[2][1] = tmp >> 16;
	tmp = _reg_read(reg_base + REG_SCL_CSC_COEF4(inst));
	cfg->coef[2][2] = tmp;

	tmp = _reg_read(reg_base + REG_SCL_CSC_OFFSET(inst));
	cfg->add[0] = tmp;
	cfg->add[1] = tmp >> 8;
	cfg->add[2] = tmp >> 16;
	memset(cfg->sub, 0, sizeof(cfg->sub));
}

void sclr_core_checksum_en(u8 inst, bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_CHECKSUM0(inst), BIT(31),
	                enable ? BIT(31) : 0);
}

void sclr_core_get_checksum_status(u8 inst, struct sclr_core_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(reg_base + REG_SCL_CHECKSUM0(inst));
	status->axi_read_gop0_data = _reg_read(reg_base + REG_SCL_CHECKSUM1(inst));
	status->axi_read_gop1_data = _reg_read(reg_base + REG_SCL_CHECKSUM2(inst));
	status->axi_write_data = _reg_read(reg_base + REG_SCL_CHECKSUM3(inst));
}

/****************************************************************************
 * SCALER Compression(OSD Encoder)
 ****************************************************************************/
/**
 * sclr_oenc_set_cfg - set compression configurations.
 *
 * @param oenc_cfg: compression's settings.
 */
void sclr_oenc_set_cfg(struct sclr_oenc_cfg *oenc_cfg)
{
	//Compression Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	static u8 reg_map_fmt[SCL_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa};

	//reset
	_reg_write_mask(reg_base + REG_SCL_TOP_OENC_RST, BIT(0), 1);
	_reg_write_mask(reg_base + REG_SCL_TOP_OENC_RST, BIT(0), 0);

	oenc_cfg->cfg.b.fmt = reg_map_fmt[oenc_cfg->fmt];
	oenc_cfg->cfg.b.intr_en = 1;
	_reg_write(reg_base + REG_SCL_TOP_OENC_CFG,oenc_cfg->cfg.raw);
	_reg_write(reg_base + REG_SCL_TOP_OENC_RANGE,
	           ((oenc_cfg->src_picture_size.h - 1) << 16) | (oenc_cfg->src_picture_size.w - 1));
	_reg_write(reg_base + REG_SCL_TOP_OENC_PITCH, oenc_cfg->src_pitch);
	_reg_write(reg_base + REG_SCL_TOP_OENC_SRC_ADDR, oenc_cfg->src_adr);
	_reg_write(reg_base + REG_SCL_TOP_OENC_BSO_ADDR, oenc_cfg->bso_adr);

	if (oenc_cfg->cfg.b.wprot_en) {
		_reg_write(reg_base + REG_SCL_TOP_OENC_WPROT_LADDR, oenc_cfg->wprot_laddr);
		_reg_write(reg_base + REG_SCL_TOP_OENC_WPROT_UADDR, oenc_cfg->wprot_uaddr);
	}

	if (oenc_cfg->cfg.b.limit_bsz_en)
		_reg_write(reg_base + REG_SCL_TOP_OENC_LIMIT_BSZ, oenc_cfg->limit_bsz);

	//sclr_oenc_trig
	_reg_write_mask(reg_base + REG_SCL_TOP_OENC_INT_GO, BIT(0), 1);
}

/**
 * sclr_oenc_get_cfg - set compression configurations.
 *
 * @return oenc_cfg: compression's settings.
 *
 */
struct sclr_oenc_cfg *sclr_oenc_get_cfg(void)
{
	struct sclr_oenc_int oenc_trig;

	oenc_trig.go_intr.raw = _reg_read(reg_base + REG_SCL_TOP_OENC_INT_GO);

	if (oenc_trig.go_intr.b.done)
		pr_debug("[cvi-vip][sc] SCLR OSD Compression done!!\n");

	pr_debug("[cvi-vip][sc] SCLR OSD Compression INTR vector:");
	if (oenc_trig.go_intr.b.intr_vec & BIT(0))
		pr_debug("Successful!!\n");
	if (oenc_trig.go_intr.b.intr_vec & BIT(2))
		pr_debug("Fail, Bitstream size great than limiter!!\n");
	if (oenc_trig.go_intr.b.intr_vec & BIT(3))
		pr_debug("Fail, Watch Dog time-out!!\n");
	if (oenc_trig.go_intr.b.intr_vec & BIT(4))
		pr_debug("Fail, Out of Dram Write protection region!!\n");

	g_oenc_cfg.cfg.raw = _reg_read(reg_base + REG_SCL_TOP_OENC_CFG);
	g_oenc_cfg.bso_adr = _reg_read(reg_base + REG_SCL_TOP_OENC_BSO_ADDR);
	g_oenc_cfg.bso_sz  = _reg_read(reg_base + REG_SCL_TOP_OENC_BSO_SZ) + 1; //count from 0
	g_oenc_cfg.bso_mem_size.w = ALIGN(g_oenc_cfg.bso_sz, 16) & 0x3fff;
	g_oenc_cfg.bso_mem_size.h = ALIGN(g_oenc_cfg.bso_sz, 16) >> 14;

	return &g_oenc_cfg;
}


/****************************************************************************
 * SCALER DISP SHADOW REGISTER - USE in DISPLAY and GOP
 ****************************************************************************/
/**
 * sclr_disp_reg_shadow_sel - control the read reg-bank.
 *
 * @param read_shadow: true(shadow); false(working)
 */
void sclr_disp_reg_shadow_sel(bool read_shadow)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, BIT(18),
			(read_shadow ? 0x0 : BIT(18)));
}
EXPORT_SYMBOL_GPL(sclr_disp_reg_shadow_sel);

/**
 * sclr_disp_reg_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param mask: true(mask); false(unmask)
 * @return: mask status before modification.
 */
bool sclr_disp_reg_shadow_mask(bool mask)
{
	bool is_masked = (_reg_read(reg_base + REG_SCL_DISP_CFG) & BIT(17));

	if (is_masked != mask)
		_reg_write_mask(reg_base + REG_SCL_DISP_CFG, BIT(17),
				(mask ? BIT(17) : 0));

	return is_masked;
}

/**
 * sclr_disp_reg_set_shadow_mask - reg won't be update by sw/hw until unmask.
 *
 * @param shadow_mask: true(mask); false(unmask)
 */
void sclr_disp_reg_set_shadow_mask(bool shadow_mask)
{
	if (shadow_mask)
		spin_lock(&disp_mask_spinlock);

	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, BIT(17),
			(shadow_mask ? BIT(17) : 0));

	if (!shadow_mask)
		spin_unlock(&disp_mask_spinlock);
}

/****************************************************************************
 * SCALER GOP
 ****************************************************************************/
/**
 * sclr_gop_set_cfg - configure gop
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: gop's settings
 * @param update: update parameter or not
 */
void sclr_gop_set_cfg(u8 inst, u8 layer, struct sclr_gop_cfg *cfg, bool update)
{
	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), cfg->gop_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP0_FONTCOLOR(inst),
				(cfg->font_fg_color << 16) | cfg->font_bg_color);
			if (cfg->gop_ctrl.b.colorkey_en)
				_reg_write(reg_base + REG_SCL_GOP0_COLORKEY(inst),
					cfg->colorkey);
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_CTRL(inst), cfg->fb_ctrl.raw);

			// ECO item for threshold invert
			_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x10, cfg->fb_ctrl.b.lo_thr_inv << 4);
			// set odec cfg
			_reg_write(reg_base + REG_SCL_GOP0_DEC_CTRL(inst), cfg->odec_cfg.odec_ctrl.raw);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), cfg->gop_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP1_FONTCOLOR(inst),
				(cfg->font_fg_color << 16) | cfg->font_bg_color);
			if (cfg->gop_ctrl.b.colorkey_en)
				_reg_write(reg_base + REG_SCL_GOP1_COLORKEY(inst), cfg->colorkey);
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_CTRL(inst), cfg->fb_ctrl.raw);

			// ECO item for threshold invert
			_reg_write_mask(reg_base + REG_SCL_CFG(inst), 0x10, cfg->fb_ctrl.b.lo_thr_inv << 4);
			// set odec cfg
			_reg_write(reg_base + REG_SCL_GOP1_DEC_CTRL(inst), cfg->odec_cfg.odec_ctrl.raw);
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return;
		}

		if (update)
			g_gop_cfg[inst][layer] = *cfg;
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, cfg->gop_ctrl.raw);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTCOLOR,
			(cfg->font_fg_color << 16) | cfg->font_bg_color);
		if (cfg->gop_ctrl.b.colorkey_en)
			_reg_write(reg_base + REG_SCL_DISP_GOP_COLORKEY,cfg->colorkey);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_CTRL, cfg->fb_ctrl.raw);

		// ECO item for threshold invert
		_reg_write_mask(reg_base + 0x90f8, 0x1, cfg->fb_ctrl.b.lo_thr_inv);
		// set odec cfg
		_reg_write(reg_base + REG_SCL_DISP_GOP_DEC_CTRL, cfg->odec_cfg.odec_ctrl.raw);

		sclr_disp_reg_set_shadow_mask(false);

		if (update)
			g_disp_cfg.gop_cfg = *cfg;
	}
}
EXPORT_SYMBOL_GPL(sclr_gop_set_cfg);

/**
 * sclr_gop_get_cfg - get gop's configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 */
struct sclr_gop_cfg *sclr_gop_get_cfg(u8 inst, u8 layer)
{
	if (inst < SCL_MAX_INST && layer < SCL_MAX_GOP_INST)
		return &g_gop_cfg[inst][layer];
	else if (inst == SCL_GOP_DISP)
		return &g_disp_cfg.gop_cfg;

	return NULL;
}
EXPORT_SYMBOL_GPL(sclr_gop_get_cfg);

/**
 * sclr_gop_setup 256LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 256LUT-table for index 0 ~ length.
 *		There should be smaller than 256 instances.
 * @param data: values of 256LUT-table. There should be 256 instances.
 */
int sclr_gop_setup_256LUT(u8 inst, u8 layer, u16 length, u16 *data)
{
	u16 i = 0;
	struct sclr_gop_cfg gop_cfg;

#if 0
	void *vip_clk_reg = ioremap(0x3002008, 4);
	u32 vip_pll = ioread32(vip_clk_reg);

	iounmap(vip_clk_reg);
	pr_debug("hw vip clk:%#x\n", vip_pll);
#endif

	pr_debug("[cvi-vip][sc] %s:  inst(%d) layer(%d) length(%d)\n", __func__, inst, layer, length);
	gop_cfg = *sclr_gop_get_cfg(inst, layer);
	if (layer == 0) {
		pr_debug("before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			_reg_read(reg_base + REG_SCL_GOP0_CFG(inst)),
			_reg_read(reg_base + REG_SCL_GOP0_FMT(inst, layer)));
	} else if (layer == 1) {
		pr_debug("before update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
			_reg_read(reg_base + REG_SCL_GOP1_CFG(inst)),
			_reg_read(reg_base + REG_SCL_GOP1_FMT(inst, layer)));
	}

	if (length > 256) {
		pr_err("LUT length(%d) error, should less or equal to 256!\n", length);
		return -1;
	}

	if (inst < SCL_MAX_INST) {
		pr_debug("[cvi-vip][sc] update 256LUT in gop%d of sc_%d. Length is %d.\n", layer, inst, length);
		if (layer == 0) {
			//Disable OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), 0x0);
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP0_256LUT0(inst),
				           (i << 16) | *(data + i));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), BIT(16));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), ~BIT(16));
				pr_debug("write LUT index:%d value:%#x\n", i, *(data + i));
			}
#if 0 /* do not read when in normal operation */
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP0_256LUT0(inst), (i << 16));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), BIT(17));
				_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), ~BIT(17));
				pr_debug("read LUT index:%d value:%#x\n",
					i, _reg_read(reg_base + REG_SCL_GOP0_256LUT1(inst)) & 0xFFFF);
			}
#endif
			//Enable original OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), gop_cfg.gop_ctrl.raw);
			pr_debug("After update LUT, gop_cfg ctrl:%#x  fmt:%#x\n",
				_reg_read(reg_base + REG_SCL_GOP0_CFG(inst)),
				_reg_read(reg_base + REG_SCL_GOP0_FMT(inst, layer)));
		} else if (layer == 1) {
			//Disable OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), 0x0);
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP1_256LUT0(inst),
				           (i << 16) | *(data + i));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), BIT(16));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), ~BIT(16));
				pr_debug("write LUT index:%d value:%#x\n", i, *(data + i));
			}
#if 0 /* do not read when in normal operation */
			for (i = 0; i < length; ++i) {
				_reg_write(reg_base + REG_SCL_GOP1_256LUT0(inst), (i << 16));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), BIT(17));
				_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), ~BIT(17));
				pr_debug("read LUT index:%d value:%#x\n",
					i, _reg_read(reg_base + REG_SCL_GOP1_256LUT1(inst)) & 0xFFFF);
			}
#endif
			//Enable original OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), gop_cfg.gop_ctrl.raw);
			pr_debug("After update LUT, gop_cfg ctrl:%#x fmt:%#x\n",
				_reg_read(reg_base + REG_SCL_GOP1_CFG(inst)),
				_reg_read(reg_base + REG_SCL_GOP1_FMT(inst, layer)));
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return -1;
		}
	} else {
		sclr_disp_reg_set_shadow_mask(true);
		//Disable OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, 0x0);

		pr_debug("[cvi-vip][sc] update 256LUT in gop1 of display. layer(%d), sc(%d). Length is %d.\n",
			layer, inst, length);
		for (i = 0; i < length; ++i) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0,
			           (i << 16) | *(data + i));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(16));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(16));
			pr_debug("write LUT index:%d value:%#x\n", i, *(data + i));
		}
		for (i = 0; i < length; ++i) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0, (i << 16));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(17));
			_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(17));
			pr_debug("read LUT index:%d value:%#x\n",
				i, _reg_read(reg_base + REG_SCL_DISP_GOP_256LUT1) & 0xFFFF);
		}

		//Enable original OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, gop_cfg.gop_ctrl.raw);
		pr_debug("After update LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_DISP_GOP_CFG));
		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_gop_setup_256LUT);

/**
 * sclr_gop_update_256LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 256LUT-table. There should be 256 instances.
 * @param data: value of 256LUT-table.
 */
int sclr_gop_update_256LUT(u8 inst, u8 layer, u16 index, u16 data)
{
	struct sclr_gop_cfg gop_cfg = *sclr_gop_get_cfg(inst, layer);

	if (layer == 0)
		pr_debug("before update LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_GOP0_CFG(inst)));
	else if (layer == 1)
		pr_debug("before update LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_GOP1_CFG(inst)));

	if (index > 256)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_debug("[cvi-vip][sc] update 256LUT in gop%d of sc_%d. Index is %d.\n", layer, inst, index);
		if (layer == 0) {
			//Disable OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), 0x0);
			_reg_write(reg_base + REG_SCL_GOP0_256LUT0(inst),
			           (index << 16) | data);
			_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), BIT(16));
			_reg_write(reg_base + REG_SCL_GOP0_256LUT1(inst), ~BIT(16));
			//Enable original OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP0_CFG(inst), gop_cfg.gop_ctrl.raw);
			pr_debug("After upadte LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_GOP0_CFG(inst)));
		} else if (layer == 1) {
			//Disable OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), 0x0);
			_reg_write(reg_base + REG_SCL_GOP1_256LUT0(inst),
			           (index << 16) | data);
			_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), BIT(16));
			_reg_write(reg_base + REG_SCL_GOP1_256LUT1(inst), ~BIT(16));
			//Enable original OW enable in gop ctrl register
			_reg_write(reg_base + REG_SCL_GOP1_CFG(inst), gop_cfg.gop_ctrl.raw);
			pr_debug("After upadte LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_GOP0_CFG(inst)));
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return -1;
		}
	} else {
		sclr_disp_reg_set_shadow_mask(true);
		//Disable OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, 0x0);

		pr_debug("[cvi-vip][sc] update 256LUT in gop1 of display. layer(%d), sc(%d), Index is %d.\n", layer, inst, index);
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT0,
		           (index << 16) | data);
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, BIT(16));
		_reg_write(reg_base + REG_SCL_DISP_GOP_256LUT1, ~BIT(16));

		//Enable original OW enable in gop ctrl register
		_reg_write(reg_base + REG_SCL_DISP_GOP_CFG, gop_cfg.gop_ctrl.raw);
		pr_debug("After upadte LUT, gop_cfg ctrl:%#x\n", _reg_read(reg_base + REG_SCL_DISP_GOP_CFG));
		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_gop_update_256LUT);

/**
 * sclr_gop_setup 16LUT - setup gop's Look-up table
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param length: update 16LUT-table for index 0 ~ length.
 *		There should be smaller than 16 instances.
 * @param data: values of 16LUT-table. There should be 16 instances.
 */
int sclr_gop_setup_16LUT(u8 inst, u8 layer, u8 length, u16 *data)
{
	u16 i = 0;
	if (length > 16)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_debug("[cvi-vip][sc] update 16LUT in gop%d of sc_%d. Length is %d.\n", layer, inst, length);
		if (layer == 0) {
			for (i = 0; i < length; i += 2 ) {
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, i),
						   ((*(data + i + 1) << 16) | (*(data + i))));
			}
		} else if (layer == 1) {
			for (i = 0; i <= length; i += 2) {
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, i),
				           ((*(data + i + 1) << 16) | (*(data + i))));
			}
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return -1;
		}
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		pr_debug("[cvi-vip][sc] update 16LUT in gop1 of display. Length is %d.\n", length);
		for (i = 0; i <= length; i += 2) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(i),
			           ((*(data + i + 1) << 16) | (*(data + i))));
		}

		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_gop_setup_16LUT);

/**
 * sclr_gop_update_16LUT - update gop's Look-up table by index.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param index: start address of 16LUT-table. There should be 16 instances.
 * @param data: value of 16LUT-table.
 */
int sclr_gop_update_16LUT(u8 inst, u8 layer, u8 index, u16 data)
{
	u16 tmp;
	if (index > 16)
		return -1;

	if (inst < SCL_MAX_INST) {
		pr_debug("[cvi-vip][sc] update 16LUT in gop%d of sc_%d. Index is %d.\n", layer, inst, index);
		if (layer == 0) {
			if (index % 2 == 0) {
				tmp = _reg_read(reg_base + REG_SCL_GOP0_16LUT(inst, index + 1));
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, index),
				           ((tmp << 16) | data));
			} else {
				tmp = _reg_read(reg_base + REG_SCL_GOP0_16LUT(inst, index - 1));
				_reg_write(reg_base + REG_SCL_GOP0_16LUT(inst, index - 1),
				           ((data << 16) | tmp));
			}
		} else if (layer == 1) {
			if (index % 2 == 0) {
				tmp = _reg_read(reg_base + REG_SCL_GOP1_16LUT(inst, index + 1));
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, index),
				           ((tmp << 16) | data));
			} else {
				tmp = _reg_read(reg_base + REG_SCL_GOP1_16LUT(inst, index - 1));
				_reg_write(reg_base + REG_SCL_GOP1_16LUT(inst, index - 1),
				           ((data << 16) | tmp));
			}
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return -1;
		}
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		pr_debug("[cvi-vip][sc] update 16LUT in gop1 of display. Index is %d.\n", index);
		if (index % 2 == 0) {
			tmp = _reg_read(reg_base + REG_SCL_DISP_GOP_16LUT(index + 1));
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(index),
			           ((tmp << 16) | data));
		} else {
			tmp = _reg_read(reg_base + REG_SCL_DISP_GOP_16LUT(index - 1));
			_reg_write(reg_base + REG_SCL_DISP_GOP_16LUT(index - 1),
			           ((data << 16) | tmp));
		}

		sclr_disp_reg_set_shadow_mask(false);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_gop_update_16LUT);

/**
 * sclr_gop_ow_set_cfg - set gop's osd-window configurations.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param cfg: ow's settings.
 * @param update: update parameter or not
 */
void sclr_gop_ow_set_cfg(u8 inst, u8 layer, u8 ow_inst, struct sclr_gop_ow_cfg *ow_cfg, bool update)
{
	//OW Format
	//4'b0000: ARGB8888
	//4'b0100: ARGB4444
	//4'b0101: ARGB1555
	//4'b1000: 256LUT-ARGB4444
	//4'b1010: 16-LUT-ARGB4444
	//4'b1100: Font-base"
	static const u8 reg_map_fmt[SCL_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xa, 0xc};

	if (ow_inst >= SCL_MAX_GOP_OW_INST)
		return;

	pr_debug("[cvi-vip][sc] %s: inst:%d layer:%d ow_inst:%d ow_cfg->fmt:%d\n",
		__func__, inst, layer, ow_inst, ow_cfg->fmt);
	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_FMT(inst, ow_inst),
			           reg_map_fmt[ow_cfg->fmt]);
			_reg_write(reg_base + REG_SCL_GOP0_H_RANGE(inst, ow_inst),
			           (ow_cfg->end.x << 16) | ow_cfg->start.x);
			_reg_write(reg_base + REG_SCL_GOP0_V_RANGE(inst, ow_inst),
			           (ow_cfg->end.y << 16) | ow_cfg->start.y);
			_reg_write(reg_base + REG_SCL_GOP0_ADDR_L(inst, ow_inst),
			           ow_cfg->addr);
			_reg_write(reg_base + REG_SCL_GOP0_ADDR_H(inst, ow_inst),
			           ow_cfg->addr >> 32);
			_reg_write(reg_base + REG_SCL_GOP0_CROP_PITCH(inst, ow_inst),
			           (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
			_reg_write(reg_base + REG_SCL_GOP0_SIZE(inst, ow_inst),
			           (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_FMT(inst, ow_inst),
			           reg_map_fmt[ow_cfg->fmt]);
			_reg_write(reg_base + REG_SCL_GOP1_H_RANGE(inst, ow_inst),
			           (ow_cfg->end.x << 16) | ow_cfg->start.x);
			_reg_write(reg_base + REG_SCL_GOP1_V_RANGE(inst, ow_inst),
			           (ow_cfg->end.y << 16) | ow_cfg->start.y);
			_reg_write(reg_base + REG_SCL_GOP1_ADDR_L(inst, ow_inst),
			           ow_cfg->addr);
			_reg_write(reg_base + REG_SCL_GOP1_ADDR_H(inst, ow_inst),
			           ow_cfg->addr >> 32);
			_reg_write(reg_base + REG_SCL_GOP1_CROP_PITCH(inst, ow_inst),
			           (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
			_reg_write(reg_base + REG_SCL_GOP1_SIZE(inst, ow_inst),
			           (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return;
		}

		if (update)
			g_gop_cfg[inst][layer].ow_cfg[ow_inst] = *ow_cfg;
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_FMT(ow_inst),
		           reg_map_fmt[ow_cfg->fmt]);
		_reg_write(reg_base + REG_SCL_DISP_GOP_H_RANGE(ow_inst),
		           (ow_cfg->end.x << 16) | ow_cfg->start.x);
		_reg_write(reg_base + REG_SCL_DISP_GOP_V_RANGE(ow_inst),
		           (ow_cfg->end.y << 16) | ow_cfg->start.y);
		_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_L(ow_inst),
		           ow_cfg->addr);
		_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_H(ow_inst),
		           ow_cfg->addr >> 32);
		_reg_write(reg_base + REG_SCL_DISP_GOP_CROP_PITCH(ow_inst),
		           (ow_cfg->crop_pixels << 16) | ow_cfg->pitch);
		_reg_write(reg_base + REG_SCL_DISP_GOP_SIZE(ow_inst),
		           (ow_cfg->mem_size.h << 16) | ow_cfg->mem_size.w);

		sclr_disp_reg_set_shadow_mask(false);

		if (update)
			g_disp_cfg.gop_cfg.ow_cfg[ow_inst] = *ow_cfg;
	}
}
EXPORT_SYMBOL_GPL(sclr_gop_ow_set_cfg);

/**
 * sclr_gop_ow_get_addr - get gop's osd-window DRAM addr.
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param ow_inst: (0~7), the instance of ow which want to be configured.
 * @param addr: ow's DRAM address.
 */
void sclr_gop_ow_get_addr(u8 inst, u8 layer, u8 ow_inst, u64 *addr)
{
	if (ow_inst >= SCL_MAX_GOP_OW_INST)
		return;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			*addr = _reg_read(reg_base + REG_SCL_GOP0_ADDR_L(inst, ow_inst)) |
				((u64)_reg_read(reg_base + REG_SCL_GOP0_ADDR_H(inst, ow_inst)) << 32);
		} else if (layer == 1) {
			*addr = _reg_read(reg_base + REG_SCL_GOP1_ADDR_L(inst, ow_inst)) |
				((u64)_reg_read(reg_base + REG_SCL_GOP1_ADDR_H(inst, ow_inst)) << 32);
		} else {
			pr_err("[%s]Invalid layer(%d), sc gop only has 2 layer(0 & 1)", __func__, layer);
			return;
		}
	} else {
		*addr = _reg_read(reg_base + REG_SCL_DISP_GOP_ADDR_L(ow_inst)) |
			((u64)_reg_read(reg_base + REG_SCL_DISP_GOP_ADDR_H(ow_inst)) << 32);
	}
}
EXPORT_SYMBOL_GPL(sclr_gop_ow_get_addr);

/**
 * sclr_gop_fb_set_cfg - setup fontbox
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @param cfg: fontbox configuration
 */
void sclr_gop_fb_set_cfg(u8 inst, u8 layer, u8 fb_inst, struct sclr_gop_fb_cfg *fb_cfg)
{
	if (fb_inst >= SCL_MAX_GOP_FB_INST)
		return;

	if (fb_cfg->fb_ctrl.b.sample_rate > 3)
		fb_cfg->fb_ctrl.b.sample_rate = 3;
	if (fb_cfg->fb_ctrl.b.pix_thr > 31)
		fb_cfg->fb_ctrl.b.pix_thr = 31;

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_CFG(inst, fb_inst), fb_cfg->fb_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP0_FONTBOX_INIT(inst, fb_inst), fb_cfg->init_st);
		} else if (layer == 1) {
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_CFG(inst, fb_inst), fb_cfg->fb_ctrl.raw);
			_reg_write(reg_base + REG_SCL_GOP1_FONTBOX_INIT(inst, fb_inst), fb_cfg->init_st);
		} else
			pr_err("[cvi-vip][sc] %s: only 0 or 1 layer, no such layer(%d). ", __func__, layer);
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_CFG(fb_inst), fb_cfg->fb_ctrl.raw);
		_reg_write(reg_base + REG_SCL_DISP_GOP_FONTBOX_INIT(fb_inst), fb_cfg->init_st);

		sclr_disp_reg_set_shadow_mask(false);
	}
}

/**
 * sclr_gop_fb_get_record - get fontbox's record
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param fb_inst: (0~1), the instance of ow which want to be configured.
 * @return: fontbox's record
 */
u32 sclr_gop_fb_get_record(u8 inst, u8 layer, u8 fb_inst)
{
	if (inst < SCL_MAX_INST) {
		if (layer == 0)
			return _reg_read(reg_base + REG_SCL_GOP0_FONTBOX_REC(inst, fb_inst));
		else if (layer == 1)
			return _reg_read(reg_base + REG_SCL_GOP1_FONTBOX_REC(inst, fb_inst));
		else {
			pr_err("[cvi-vip][sc] %s: only 0 or 1 layer, no such layer(%d). ", __func__, layer);
			return -1;
		}
	} else
		return _reg_read(reg_base + REG_SCL_DISP_GOP_FONTBOX_REC(fb_inst));
}

/**
 * sclr_gop_odec_set_cfg_from_oenc - setup odec
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param layer: (0~1) 0 is layer 0(gop0). 1 is layer 1(gop1).
 * @param cfg: odec_cfg configuration
 */
void sclr_gop_odec_set_cfg_from_oenc(u8 inst, u8 layer, struct sclr_gop_odec_cfg *odec_cfg)
{
	struct sclr_oenc_cfg *oenc_cfg = sclr_oenc_get_cfg();

	if (inst < SCL_MAX_INST) {
		if (layer == 0) {
			if (oenc_cfg->bso_sz != 0) {
				_reg_write(reg_base + REG_SCL_GOP0_DEC_CTRL(inst), odec_cfg->odec_ctrl.raw);
				_reg_write(reg_base + REG_SCL_GOP0_ADDR_L(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           oenc_cfg->bso_adr);
				_reg_write(reg_base + REG_SCL_GOP0_ADDR_H(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           oenc_cfg->bso_adr >> 32);
				_reg_write(reg_base + REG_SCL_GOP0_SIZE(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           (oenc_cfg->bso_mem_size.h << 16) | oenc_cfg->bso_mem_size.w);
			}
		} else if (layer == 1) {
			if (oenc_cfg->bso_sz != 0) {
				_reg_write(reg_base + REG_SCL_GOP1_DEC_CTRL(inst), odec_cfg->odec_ctrl.raw);
				_reg_write(reg_base + REG_SCL_GOP1_ADDR_L(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           oenc_cfg->bso_adr);
				_reg_write(reg_base + REG_SCL_GOP1_ADDR_H(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           oenc_cfg->bso_adr >> 32);
				_reg_write(reg_base + REG_SCL_GOP1_SIZE(inst, odec_cfg->odec_ctrl.b.odec_attached_idx),
				           (oenc_cfg->bso_mem_size.h << 16) | oenc_cfg->bso_mem_size.w);
			}
		} else {
			pr_err("[cvi-vip][sc] %s: only 0 or 1 layer, no such layer(%d). ", __func__, layer);
		}
	} else {
		if (oenc_cfg->bso_sz != 0) {
			_reg_write(reg_base + REG_SCL_DISP_GOP_DEC_CTRL, odec_cfg->odec_ctrl.raw);
			_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_L(odec_cfg->odec_ctrl.b.odec_attached_idx),
			           oenc_cfg->bso_adr);
			_reg_write(reg_base + REG_SCL_DISP_GOP_ADDR_H(odec_cfg->odec_ctrl.b.odec_attached_idx),
			           oenc_cfg->bso_adr >> 32);
			_reg_write(reg_base + REG_SCL_DISP_GOP_SIZE(odec_cfg->odec_ctrl.b.odec_attached_idx),
			           (oenc_cfg->bso_mem_size.h << 16) | oenc_cfg->bso_mem_size.w);
		}
	}
}

/**
 * sclr_cover_set_cfg - setup cover
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param cover_cfg: cover_cfg configuration
 */
void sclr_cover_set_cfg(u8 inst, u8 cover_w_inst, struct sclr_cover_cfg *cover_cfg)
{
	if (inst < SCL_MAX_INST) {
		_reg_write(reg_base + REG_SCL_COVER_CFG(inst, cover_w_inst), cover_cfg->start.raw);
		_reg_write(reg_base + REG_SCL_COVER_SIZE(inst, cover_w_inst),
		           (cover_cfg->img_size.h << 16) | cover_cfg->img_size.w);
		_reg_write(reg_base + REG_SCL_COVER_COLOR(inst, cover_w_inst), cover_cfg->color.raw);
	} else {
		_reg_write(reg_base + REG_SCL_DISP_COVER_CFG(cover_w_inst), cover_cfg->start.raw);
		_reg_write(reg_base + REG_SCL_DISP_COVER_SIZE(cover_w_inst),
		           (cover_cfg->img_size.h << 16) | cover_cfg->img_size.w);
		_reg_write(reg_base + REG_SCL_DISP_COVER_COLOR(cover_w_inst), cover_cfg->color.raw);
	}
}
EXPORT_SYMBOL_GPL(sclr_cover_set_cfg);

/****************************************************************************
 * SCALER PRICAY MASK
 ****************************************************************************/
/**
 * sclr_pri_set_cfg - configure privacy mask
 *
 * @param inst: (0~3), the instance of gop which want to be configured.
 *		0~3 is on scl, 4 is on disp.
 * @param cfg: privacy mask's settings
 */
void sclr_pri_set_cfg(u8 inst, struct sclr_privacy_cfg *cfg)
{
	u16 grid_line_per_frame, grid_num_per_line;
	u16 w, h;
	u16 pitch;

	if (inst < SCL_MAX_INST) {
		_reg_write(reg_base + REG_SCL_PRI_CFG(inst), cfg->cfg.raw);

		if (cfg->cfg.b.fit_picture) {
			w = g_sc_cfg[inst].sc.dst.w;
			h = g_sc_cfg[inst].sc.dst.h;
		} else {
			_reg_write(reg_base + REG_SCL_PRI_START(inst), (cfg->start.y << 16) | cfg->start.x);
			_reg_write(reg_base + REG_SCL_PRI_END(inst), (cfg->end.y << 16) | cfg->end.x);
			w = cfg->end.x - cfg->start.x + 1;
			h = cfg->end.y - cfg->start.y + 1;
		}

		_reg_write(reg_base + REG_SCL_PRI_ALPHA(inst),
			   (cfg->map_cfg.alpha_factor << 8) | cfg->map_cfg.no_mask_idx);
		_reg_write(reg_base + REG_SCL_PRI_MAP_ADDR_L(inst), cfg->map_cfg.base);
		//_reg_write(reg_base + REG_SCL_PRI_MAP_ADDR_H(inst), cfg->map_cfg.base >> 32);

		if (cfg->cfg.b.mode == 0) {
			u8 grid_shift = cfg->cfg.b.grid_size ? 4 : 3;

			grid_num_per_line = UPPER(w, grid_shift);
			grid_line_per_frame = UPPER(h, grid_shift);
			_reg_write(reg_base + REG_SCL_PRI_GRID_CFG(inst),
				   (grid_num_per_line << 16) | (grid_line_per_frame - 1));
			pitch = ALIGN(grid_num_per_line, 16);
		} else {
			_reg_write(reg_base + REG_SCL_PRI_GRID_CFG(inst),
				   (w << 16) | (h - 1));
			pitch = ALIGN(w, 16);
		}

		_reg_write(reg_base + REG_SCL_PRI_MAP_AXI_CFG(inst), (cfg->map_cfg.axi_burst << 16) | pitch);
	} else {
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write(reg_base + REG_SCL_TOP_PRI_CFG, cfg->cfg.raw);

		if (cfg->cfg.b.fit_picture) {
			w = g_img_cfg[SCL_IMG_V].mem.width;
			h = g_img_cfg[SCL_IMG_V].mem.height;
		} else {
			_reg_write(reg_base + REG_SCL_TOP_PRI_START, (cfg->start.y << 16) | cfg->start.x);
			_reg_write(reg_base + REG_SCL_TOP_PRI_END, (cfg->end.y << 16) | cfg->end.x);
			w = cfg->end.x - cfg->start.x + 1;
			h = cfg->end.y - cfg->start.y + 1;
		}
		_reg_write(reg_base + REG_SCL_TOP_PRI_SRC_SIZE,
			   ((g_img_cfg[SCL_IMG_V].mem.height - 1) << 16) | (g_img_cfg[SCL_IMG_V].mem.width - 1));

		_reg_write(reg_base + REG_SCL_TOP_PRI_ALPHA,
			   (cfg->map_cfg.alpha_factor << 8) | cfg->map_cfg.no_mask_idx);
		_reg_write(reg_base + REG_SCL_TOP_PRI_MAP_ADDR_L, cfg->map_cfg.base);

		if (cfg->cfg.b.mode == 0) {
			u8 grid_shift = cfg->cfg.b.grid_size ? 4 : 3;

			grid_num_per_line = UPPER(w, grid_shift);
			grid_line_per_frame = UPPER(h, grid_shift);
			_reg_write(reg_base + REG_SCL_TOP_PRI_GRID_CFG,
				   (grid_num_per_line << 16) | (grid_line_per_frame - 1));
			pitch = ALIGN(grid_num_per_line, 16);
		} else {
			_reg_write(reg_base + REG_SCL_PRI_GRID_CFG(inst),
				   (w << 16) | (h - 1));
			pitch = ALIGN(w, 16);
		}

		_reg_write(reg_base + REG_SCL_TOP_PRI_MAP_AXI_CFG, (cfg->map_cfg.axi_burst << 16) | pitch);

		sclr_disp_reg_set_shadow_mask(false);
	}
}

/****************************************************************************
 * SCALER DISP
 ****************************************************************************/
/**
 * sclr_disp_reg_force_up - trigger reg update by sw.
 *
 */
void sclr_disp_reg_force_up(void)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, BIT(16), BIT(16));
}
EXPORT_SYMBOL_GPL(sclr_disp_reg_force_up);

/**
 * sclr_disp_tgen - enable timing-generator on disp.
 *
 * @param enable: AKA.
 * @return: tgen's enable status before change.
 */
bool sclr_disp_tgen_enable(bool enable)
{
	bool is_enable = (_reg_read(reg_base + REG_SCL_DISP_CFG) & 0x80);

	if (is_enable != enable) {
		_reg_write_mask(reg_base + REG_SCL_DISP_CFG, 0x0080,
				enable ? 0x80 : 0x00);
		g_disp_cfg.tgen_en = enable;
	}

	return is_enable;
}
EXPORT_SYMBOL_GPL(sclr_disp_tgen_enable);

/**
 * sclr_disp_tgen - check whether disp timing-generator enable.
 *
 * @return: tgen's enable status.
 */
bool sclr_disp_check_tgen_enable(void)
{
	bool is_enable = (_reg_read(reg_base + REG_SCL_DISP_CFG) & 0x80);

	return is_enable;
}
EXPORT_SYMBOL_GPL(sclr_disp_check_tgen_enable);

/**
 * sclr_disp_i80 - check whether mcu interface enable.
 *
 * @return: i80's enable status.
 */
bool sclr_disp_check_i80_enable(void)
{
	bool is_enable = (_reg_read(reg_base + REG_SCL_DISP_MCU_IF_CTRL) & 0x01);

	return is_enable;
}
EXPORT_SYMBOL_GPL(sclr_disp_check_i80_enable);

/**
 * sclr_disp_set_cfg - set disp's configurations.
 *
 * @param cfg: disp's settings.
 */
void sclr_disp_set_cfg(struct sclr_disp_cfg *cfg)
{
	u32 tmp = 0;

	tmp |= cfg->disp_from_sc;
	tmp |= (cfg->fmt << 12);
	if (cfg->sync_ext)
		tmp |= BIT(4);
	if (cfg->tgen_en)
		tmp |= BIT(7);

	if (!cfg->disp_from_sc) {
		sclr_disp_set_mem(&cfg->mem);
		sclr_disp_reg_set_shadow_mask(true);

		_reg_write_mask(reg_base + REG_SCL_DISP_PITCH_Y, 0xf0000000,
				cfg->burst << 28);

		sclr_disp_set_in_csc(cfg->in_csc);
	} else {
		sclr_disp_reg_set_shadow_mask(true);
		// csc only needed if disp from dram
		sclr_set_out_mode(0, SCL_OUT_DISABLE);
	}

	sclr_disp_set_out_csc(cfg->out_csc);

	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, 0x0000f09f, tmp);
	_reg_write_mask(reg_base + REG_SCL_DISP_CACHE, BIT(0), cfg->cache_mode);

	switch (cfg->out_bit) {
	case 6:
		tmp = 3 << 16;
		break;
	case 8:
		tmp = 2 << 16;
		break;
	default:
		tmp = 0;
		break;
	}
	tmp |= cfg->drop_mode << 18;
	_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR4, 0x000f0000, tmp);

	sclr_disp_reg_set_shadow_mask(false);
	g_disp_cfg = *cfg;
}
EXPORT_SYMBOL_GPL(sclr_disp_set_cfg);

/**
 * sclr_disp_get_cfg - get scl_disp's cfg
 *
 * @return: scl_disp's cfg
 */
struct sclr_disp_cfg *sclr_disp_get_cfg(void)
{
	return &g_disp_cfg;
}
EXPORT_SYMBOL_GPL(sclr_disp_get_cfg);

/**
 * sclr_disp_cfg_setup_from_reg - get settings from register.
 *
 */
void sclr_disp_cfg_setup_from_reg(void)
{
	u32 tmp = 0;

	tmp = _reg_read(reg_base + REG_SCL_DISP_CFG);
	g_disp_cfg.disp_from_sc = tmp & BIT(0);
	g_disp_cfg.sync_ext = tmp & BIT(4);
	g_disp_cfg.tgen_en = tmp & BIT(7);
	g_disp_cfg.fmt = (tmp >> 12) & 0xf;

	tmp = _reg_read(reg_base + REG_SCL_DISP_CACHE);
	g_disp_cfg.cache_mode = tmp & BIT(0);

	tmp = _reg_read(reg_base + REG_SCL_DISP_PAT_COLOR4);
	g_disp_cfg.out_bit = (tmp >> 16) & 0x3;
	g_disp_cfg.drop_mode = (tmp >> 18) & 0x3;

	tmp = _reg_read(reg_base + REG_SCL_DISP_PITCH_Y);
	g_disp_cfg.burst = (tmp >> 28) & 0xf;
}
EXPORT_SYMBOL_GPL(sclr_disp_cfg_setup_from_reg);

/**
 * sclr_disp_set_timing - modify disp's timing-generator.
 *
 * @param timing: new timing of disp.
 */
void sclr_disp_set_timing(struct sclr_disp_timing *timing)
{
	u32 tmp = 0;
	bool is_enable = sclr_disp_tgen_enable(false);

	if (timing->vsync_pol)
		tmp |= 0x20;
	if (timing->hsync_pol)
		tmp |= 0x40;

	_reg_write_mask(reg_base + REG_SCL_DISP_CFG, 0x0060, tmp);
	_reg_write(reg_base + REG_SCL_DISP_TOTAL,
		   (timing->htotal << 16) | timing->vtotal);
	_reg_write(reg_base + REG_SCL_DISP_VSYNC,
		   (timing->vsync_end << 16) | timing->vsync_start);
	_reg_write(reg_base + REG_SCL_DISP_VFDE,
		   (timing->vfde_end << 16) | timing->vfde_start);
	_reg_write(reg_base + REG_SCL_DISP_VMDE,
		   (timing->vmde_end << 16) | timing->vmde_start);
	_reg_write(reg_base + REG_SCL_DISP_HSYNC,
		   (timing->hsync_end << 16) | timing->hsync_start);
	_reg_write(reg_base + REG_SCL_DISP_HFDE,
		   (timing->hfde_end << 16) | timing->hfde_start);
	_reg_write(reg_base + REG_SCL_DISP_HMDE,
		   (timing->hmde_end << 16) | timing->hmde_start);

	if (is_enable)
		sclr_disp_tgen_enable(true);
	disp_timing = *timing;
}
EXPORT_SYMBOL_GPL(sclr_disp_set_timing);

struct sclr_disp_timing *sclr_disp_get_timing(void)
{
	return &disp_timing;
}
EXPORT_SYMBOL_GPL(sclr_disp_get_timing);

void sclr_disp_get_hw_timing(struct sclr_disp_timing *timing)
{
	u32 tmp = 0;

	if (!timing)
		return;

	tmp = _reg_read(reg_base + REG_SCL_DISP_TOTAL);
	timing->htotal = (tmp >> 16) & 0xffff;
	timing->vtotal = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VSYNC);
	timing->vsync_end = (tmp >> 16) & 0xffff;
	timing->vsync_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VFDE);
	timing->vfde_end = (tmp >> 16) & 0xffff;
	timing->vfde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_VMDE);
	timing->vmde_end = (tmp >> 16) & 0xffff;
	timing->vmde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HSYNC);
	timing->hsync_end = (tmp >> 16) & 0xffff;
	timing->hsync_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HFDE);
	timing->hfde_end = (tmp >> 16) & 0xffff;
	timing->hfde_start = tmp & 0xffff;
	tmp = _reg_read(reg_base + REG_SCL_DISP_HMDE);
	timing->hmde_end = (tmp >> 16) & 0xffff;
	timing->hmde_start = tmp & 0xffff;
}
EXPORT_SYMBOL_GPL(sclr_disp_get_hw_timing);

/**
 * sclr_disp_set_rect - setup rect(me) of disp
 *
 * @param rect: the pos/size of me, which should fit with disp's input.
 */
int sclr_disp_set_rect(struct sclr_rect rect)
{
	if ((rect.y > disp_timing.vfde_end) ||
	    (rect.x > disp_timing.hfde_end) ||
	    ((disp_timing.vfde_start + rect.y + rect.h - 1) >
	      disp_timing.vfde_end) ||
	    ((disp_timing.hfde_start + rect.x + rect.w - 1) >
	      disp_timing.hfde_end)) {
		pr_err("[cvi-vip][sc] %s: me's pos(%d, %d) size(%d, %d) ",
		       __func__, rect.x, rect.y, rect.w, rect.h);
		pr_cont(" out of range(%d, %d).\n",
			disp_timing.hfde_end, disp_timing.vfde_end);
		return -EINVAL;
	}

	disp_timing.vmde_start = rect.y + disp_timing.vfde_start;
	disp_timing.hmde_start = rect.x + disp_timing.hfde_start;
	disp_timing.vmde_end = disp_timing.vmde_start + rect.h - 1;
	disp_timing.hmde_end = disp_timing.hmde_start + rect.w - 1;

	sclr_disp_reg_set_shadow_mask(true);

	_reg_write(reg_base + REG_SCL_DISP_HMDE,
		   (disp_timing.hmde_end << 16) | disp_timing.hmde_start);
	_reg_write(reg_base + REG_SCL_DISP_VMDE,
		   (disp_timing.vmde_end << 16) | disp_timing.vmde_start);

	sclr_disp_reg_set_shadow_mask(false);
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_disp_set_rect);

/**
 * sclr_disp_set_mem - setup disp's mem settings. Only work if disp from mem.
 *
 * @param mem: mem settings for disp
 */
void sclr_disp_set_mem(struct sclr_mem *mem)
{
	sclr_disp_reg_set_shadow_mask(true);

	_reg_write(reg_base + REG_SCL_DISP_OFFSET,
		   (mem->start_y << 16) | mem->start_x);
	_reg_write(reg_base + REG_SCL_DISP_SIZE,
		   ((mem->height - 1) << 16) | (mem->width - 1));
	_reg_write_mask(reg_base + REG_SCL_DISP_PITCH_Y, 0x00ffffff,
			mem->pitch_y);
	_reg_write(reg_base + REG_SCL_DISP_PITCH_C, mem->pitch_c);

	sclr_disp_reg_set_shadow_mask(false);

	sclr_disp_set_addr(mem->addr0, mem->addr1, mem->addr2);

	g_disp_cfg.mem = *mem;
}
EXPORT_SYMBOL_GPL(sclr_disp_set_mem);

/**
 * sclr_disp_set_addr - setup disp's mem address. Only work if disp from mem.
 *
 * @param addr0: address of planar0
 * @param addr1: address of planar1
 * @param addr2: address of planar2
 */
void sclr_disp_set_addr(u64 addr0, u64 addr1, u64 addr2)
{
	sclr_disp_reg_set_shadow_mask(true);

	_reg_write(reg_base + REG_SCL_DISP_ADDR0_L, addr0);
	_reg_write(reg_base + REG_SCL_DISP_ADDR0_H, addr0 >> 32);
	_reg_write(reg_base + REG_SCL_DISP_ADDR1_L, addr1);
	_reg_write(reg_base + REG_SCL_DISP_ADDR1_H, addr1 >> 32);
	_reg_write(reg_base + REG_SCL_DISP_ADDR2_L, addr2);
	_reg_write(reg_base + REG_SCL_DISP_ADDR2_H, addr2 >> 32);

	sclr_disp_reg_set_shadow_mask(false);

	g_disp_cfg.mem.addr0 = addr0;
	g_disp_cfg.mem.addr1 = addr1;
	g_disp_cfg.mem.addr2 = addr2;
}

/**
 * sclr_disp_set_csc - configure disp's input CSC's coefficient/offset
 *
 * @param cfg: The settings for CSC
 */
void sclr_disp_set_csc(struct sclr_csc_matrix *cfg)
{
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC0, BIT(31) |
		   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC1,
		   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC2,
		   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC3,
		   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC4, (cfg->coef[2][2]));
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC_SUB,
		   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
		   cfg->sub[0]);
	_reg_write(reg_base + REG_SCL_DISP_IN_CSC_ADD,
		   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
		   cfg->add[0]);
}
EXPORT_SYMBOL_GPL(sclr_disp_set_csc);

/**
 * sclr_disp_set_in_csc - setup disp's csc on input. Only work if disp from mem.
 *
 * @param csc: csc settings
 */
void sclr_disp_set_in_csc(enum sclr_csc csc)
{
	if (csc == SCL_CSC_NONE) {
		_reg_write(reg_base + REG_SCL_DISP_IN_CSC0, 0);
	} else if (csc < SCL_CSC_MAX) {
		sclr_disp_set_csc(&csc_mtrx[csc]);
	}

	g_disp_cfg.in_csc = csc;
}

/**
 * sclr_disp_set_out_csc - setup disp's csc on output.
 *
 * @param csc: csc settings
 */
void sclr_disp_set_out_csc(enum sclr_csc csc)
{
	if (csc == SCL_CSC_NONE) {
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC0, 0);
	} else if (csc < SCL_CSC_MAX) {
		struct sclr_csc_matrix *cfg = &csc_mtrx[csc];

		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC0, BIT(31) |
			   (cfg->coef[0][1] << 16) | (cfg->coef[0][0]));
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC1,
			   (cfg->coef[1][0] << 16) | (cfg->coef[0][2]));
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC2,
			   (cfg->coef[1][2] << 16) | (cfg->coef[1][1]));
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC3,
			   (cfg->coef[2][1] << 16) | (cfg->coef[2][0]));
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC4, (cfg->coef[2][2]));
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC_SUB,
			   (cfg->sub[2] << 16) | (cfg->sub[1] << 8) |
			   cfg->sub[0]);
		_reg_write(reg_base + REG_SCL_DISP_OUT_CSC_ADD,
			   (cfg->add[2] << 16) | (cfg->add[1] << 8) |
			   cfg->add[0]);
	}

	g_disp_cfg.out_csc = csc;
}
EXPORT_SYMBOL_GPL(sclr_disp_set_out_csc);

/**
 * sclr_disp_set_pattern - setup disp's pattern generator.
 *
 * @param type: type of pattern
 * @param color: color of pattern. Only for Gradient/FULL type.
 */
void sclr_disp_set_pattern(enum sclr_disp_pat_type type,
			   enum sclr_disp_pat_color color, const u16 *rgb)
{
	switch (type) {
	case SCL_PAT_TYPE_OFF:
		_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0x16, 0);
		break;

	case SCL_PAT_TYPE_SNOW:
		_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0x16, 0x10);
		break;

	case SCL_PAT_TYPE_AUTO:
		_reg_write(reg_base + REG_SCL_DISP_PAT_COLOR0, 0x03ff03ff);
		_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR1, 0x000003ff, 0x3ff);
		_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0xff0016,
				0x780006);
		break;

	case SCL_PAT_TYPE_V_GRAD:
	case SCL_PAT_TYPE_H_GRAD:
	case SCL_PAT_TYPE_FULL: {
		if (color == SCL_PAT_COLOR_USR) {
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR1, 0x03ff0000, rgb[0] << 16);
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR2, 0x03ff03ff, rgb[2] << 16 | rgb[1]);
			_reg_write(reg_base + REG_SCL_DISP_PAT_COLOR3, rgb[1] << 16 | rgb[0]);
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR4, 0x000003ff, rgb[2]);
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0x1f000016,
					(type << 27) | (SCL_PAT_COLOR_WHITE << 24) | 0x0002);
		} else {
			_reg_write(reg_base + REG_SCL_DISP_PAT_COLOR0, 0x03ff03ff);
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR1, 0x000003ff, 0x3ff);
			_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0x1f000016,
					(type << 27) | (color << 24) | 0x0002);
		}
		if (color == SCL_PAT_COLOR_BAR) {
			_reg_write_mask(reg_base + REG_SCL_DISP_CFG, 0x80, 0x80);
		}
		break;
	}
	default:
		pr_err("%s - unacceptiable pattern-type(%d)\n", __func__, type);
		break;
	}
}
EXPORT_SYMBOL_GPL(sclr_disp_set_pattern);

/**
 * sclr_disp_set_frame_bgcolro - setup disp frame(area outside mde)'s
 *				 background color.
 *
 * @param r: 10bit red value
 * @param g: 10bit green value
 * @param b: 10bit blue value
 */
void sclr_disp_set_frame_bgcolor(u16 r, u16 g, u16 b)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR1, 0x0fff0000,
			r << 16);
	_reg_write(reg_base + REG_SCL_DISP_PAT_COLOR2, b << 16 | g);
}
EXPORT_SYMBOL_GPL(sclr_disp_set_frame_bgcolor);

/**
 * sclr_disp_set_window_bgcolro - setup disp window's background color.
 *
 * @param r: 10bit red value
 * @param g: 10bit green value
 * @param b: 10bit blue value
 */
void sclr_disp_set_window_bgcolor(u16 r, u16 g, u16 b)
{
	_reg_write(reg_base + REG_SCL_DISP_PAT_COLOR3, g << 16 | r);
	_reg_write_mask(reg_base + REG_SCL_DISP_PAT_COLOR4, 0x0fff, b);
}
EXPORT_SYMBOL_GPL(sclr_disp_set_window_bgcolor);

/**
 * sclr_disp_enable_window_bgcolor - Use window bg-color to hide everything
 *				     including test-pattern.
 *
 * @param enable: enable window bgcolor or not.
 */
void sclr_disp_enable_window_bgcolor(bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_PAT_CFG, 0x20, enable ? 0x20 : 0);
}
EXPORT_SYMBOL_GPL(sclr_disp_enable_window_bgcolor);

union sclr_disp_dbg_status sclr_disp_get_dbg_status(bool clr)
{
	union sclr_disp_dbg_status status;

	status.raw = _reg_read(reg_base + REG_SCL_DISP_DBG);

	if (clr) {
		status.b.err_fwr_clr = 1;
		status.b.err_erd_clr = 1;
		status.b.bw_fail_clr = 1;
		status.b.osd_bw_fail_clr = 1;
		_reg_write(reg_base + REG_SCL_DISP_DBG, status.raw);
	}

	return status;
}
EXPORT_SYMBOL_GPL(sclr_disp_get_dbg_status);

void sclr_disp_gamma_ctrl(bool enable, bool pre_osd)
{
	u32 value = 0;

	if (enable)
		value |= 0x04;
	if (pre_osd)
		value |= 0x08;
	_reg_write_mask(reg_base + REG_SCL_DISP_GAMMA_CTRL, 0x0C, value);
}
EXPORT_SYMBOL_GPL(sclr_disp_gamma_ctrl);

void sclr_disp_gamma_lut_update(const u8 *b, const u8 *g, const u8 *r)
{
	u8 i;
	u32 value;

	_reg_write_mask(reg_base + REG_SCL_DISP_GAMMA_CTRL, 0x03, 0x03);

	for (i = 0; i < SCL_DISP_GAMMA_NODE; ++i) {
		value = *(b + i) | (*(g + i) << 8) | (*(r + i) << 16)
			| (i << 24) | 0x80000000;
		_reg_write(reg_base + REG_SCL_DISP_GAMMA_WR_LUT, value);
	}

	_reg_write_mask(reg_base + REG_SCL_DISP_GAMMA_CTRL, 0x03, 0x00);
}
EXPORT_SYMBOL_GPL(sclr_disp_gamma_lut_update);

void sclr_disp_gamma_lut_read(struct sclr_disp_gamma_attr *gamma_attr)
{
	u8 i;
	u32 value;

	value = _reg_read(reg_base + REG_SCL_DISP_GAMMA_CTRL);
	gamma_attr->enable = value & 0x04;
	gamma_attr->pre_osd = value & 0x08;
	_reg_write_mask(reg_base + REG_SCL_DISP_GAMMA_CTRL, 0x03, 0x01);

	for (i = 0; i < 65; ++i) {
		value = (i << 24) | 0x80000000;
		_reg_write(reg_base + REG_SCL_DISP_GAMMA_WR_LUT, value);
		gamma_attr->table[i] = _reg_read(reg_base + REG_SCL_DISP_GAMMA_RD_LUT);
	}

	_reg_write_mask(reg_base + REG_SCL_DISP_GAMMA_CTRL, 0x03, 0x00);
}
EXPORT_SYMBOL_GPL(sclr_disp_gamma_lut_read);

void sclr_lvdstx_set(union sclr_lvdstx cfg)
{
	_reg_write(reg_base + REG_SCL_TOP_LVDSTX, cfg.raw);
}
EXPORT_SYMBOL_GPL(sclr_lvdstx_set);

void sclr_lvdstx_get(union sclr_lvdstx *cfg)
{
	cfg->raw = _reg_read(reg_base + REG_SCL_TOP_LVDSTX);
}

void sclr_bt_set(union sclr_bt_enc enc, union sclr_bt_sync_code sync)
{
	_reg_write(reg_base + REG_SCL_TOP_BT_ENC, enc.raw);
	_reg_write(reg_base + REG_SCL_TOP_BT_SYNC_CODE, sync.raw);
}
EXPORT_SYMBOL_GPL(sclr_bt_set);

void sclr_bt_get(union sclr_bt_enc *enc, union sclr_bt_sync_code *sync)
{
	enc->raw = _reg_read(reg_base + REG_SCL_TOP_BT_ENC);
	sync->raw = _reg_read(reg_base + REG_SCL_TOP_BT_SYNC_CODE);
}

void sclr_disp_mux_sel(enum sclr_vo_sel sel)
{
	_reg_write_mask(reg_base + REG_SCL_TOP_VO_MUX, 0xF, sel);
}
EXPORT_SYMBOL_GPL(sclr_disp_mux_sel);

enum sclr_vo_sel sclr_disp_mux_get(void)
{
	return _reg_read(reg_base + REG_SCL_TOP_VO_MUX) & 0xF;
}
EXPORT_SYMBOL_GPL(sclr_disp_mux_get);

void sclr_disp_set_intf(enum sclr_vo_intf intf)
{
	void *pll_reg = ioremap(0x03002840, 4);
	u32 pll = ioread32(pll_reg) & 0xf5;
	bool data_en[5] = {true, true, true, true};

	if (intf == SCLR_VO_INTF_DISABLE)
		iowrite32(pll, pll_reg);
	else
		iowrite32(pll | 0x0a, pll_reg);
	iounmap(pll_reg);

	dphy_init(intf);

	if (intf == SCLR_VO_INTF_DISABLE) {
		sclr_disp_mux_sel(SCLR_VO_SEL_DISABLE);
	} else if ((intf == SCLR_VO_INTF_BT601) || (intf == SCLR_VO_INTF_BT656) || (intf == SCLR_VO_INTF_BT1120)) {
		if (intf == SCLR_VO_INTF_BT601)
			sclr_disp_mux_sel(SCLR_VO_SEL_BT601);
		else if (intf == SCLR_VO_INTF_BT656)
			sclr_disp_mux_sel(SCLR_VO_SEL_BT656);
		else if (intf == SCLR_VO_INTF_BT1120)
			sclr_disp_mux_sel(SCLR_VO_SEL_BT1120);
		dphy_dsi_lane_en(true, data_en, false);
	} else if (intf == SCLR_VO_INTF_I80) {
		sclr_disp_mux_sel(SCLR_VO_SEL_I80);
		dphy_dsi_lane_en(true, data_en, false);
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_IF_CTRL, BIT(0), 1);
	} else if (intf == SCLR_VO_INTF_HW_MCU) {
		dphy_dsi_lane_en(true, data_en, false);
		_reg_write(reg_base + REG_SCL_DISP_MCU_IF_CTRL, 0x4);
	} else if (intf == SCLR_VO_INTF_SW) {
		sclr_disp_mux_sel(SCLR_VO_SEL_SW);
	} else if (intf == SCLR_VO_INTF_MIPI) {
		sclr_disp_mux_sel(SCLR_VO_SEL_DISABLE);
	} else if (intf == SCLR_VO_INTF_LVDS) {
		sclr_disp_mux_sel(SCLR_VO_SEL_DISABLE);
	}
}
EXPORT_SYMBOL_GPL(sclr_disp_set_intf);

/**
 * sclr_disp_timing_setup_from_reg - get settings from register.
 *
 */
void sclr_disp_timing_setup_from_reg(void)
{
	u32 tmp = 0;

	tmp = _reg_read(reg_base + REG_SCL_DISP_CFG);
	disp_timing.vsync_pol = (tmp & 0x20);
	disp_timing.hsync_pol = (tmp & 0x40);

	tmp = _reg_read(reg_base + REG_SCL_DISP_TOTAL);
	disp_timing.vtotal = tmp & 0xffff;
	disp_timing.htotal = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_VSYNC);
	disp_timing.vsync_start = tmp & 0xffff;
	disp_timing.vsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_VFDE);
	disp_timing.vfde_start = tmp & 0xffff;
	disp_timing.vfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_VMDE);
	disp_timing.vmde_start = tmp & 0xffff;
	disp_timing.vmde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_HSYNC);
	disp_timing.hsync_start = tmp & 0xffff;
	disp_timing.hsync_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_HFDE);
	disp_timing.hfde_start = tmp & 0xffff;
	disp_timing.hfde_end = (tmp >> 16) & 0xffff;

	tmp = _reg_read(reg_base + REG_SCL_DISP_HMDE);
	disp_timing.hmde_start = tmp & 0xffff;
	disp_timing.hmde_end = (tmp >> 16) & 0xffff;
}

void sclr_disp_checksum_en(bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_CHECKSUM0, BIT(31),
	                enable ? BIT(31) : 0);
}

void sclr_disp_get_checksum_status(struct sclr_disp_checksum_status *status)
{
	status->checksum_base.raw = _reg_read(reg_base + REG_SCL_DISP_CHECKSUM0);
	status->axi_read_from_dram = _reg_read(reg_base + REG_SCL_DISP_CHECKSUM1);
	status->axi_read_from_gop = _reg_read(reg_base + REG_SCL_DISP_CHECKSUM2);

}

/**
 * sclr_dsi_get_mode - get current dsi mode
 *
 * @return: current dsi mode
 */
enum sclr_dsi_mode sclr_dsi_get_mode(void)
{
	return (_reg_read(reg_base + REG_SCL_DSI_MAC_EN) & 0x0f);
}

/**
 * sclr_dsi_clr_mode - let dsi back to idle mode
 *
 */
void sclr_dsi_clr_mode(void)
{
	u32 mode = _reg_read(reg_base + REG_SCL_DSI_MAC_EN);

	pr_debug("%s: mac_en reg(%#x)\n", __func__, mode);
	if (mode != SCLR_DSI_MODE_IDLE)
		_reg_write(reg_base + REG_SCL_DSI_MAC_EN, mode);
}

/**
 * sclr_dsi_set_mode - set dsi mode
 *
 * @param mode: new dsi mode except for idle
 * @return: 0 if success
 */
int sclr_dsi_set_mode(enum sclr_dsi_mode mode)
{
	if (mode >= SCLR_DSI_MODE_MAX)
		return -1;

	if (mode == SCLR_DSI_MODE_IDLE) {
		sclr_dsi_clr_mode();
		return 0;
	}

	if (_reg_read(reg_base + REG_SCL_DSI_MAC_EN))
		return -1;

	_reg_write(reg_base + REG_SCL_DSI_MAC_EN, mode);
	return 0;
}
EXPORT_SYMBOL_GPL(sclr_dsi_set_mode);

/**
 * sclr_dsi_chk_mode_done - check if dsi's work done.
 *
 * @param mode: the mode to check.
 * @return: 0 if success
 */
int sclr_dsi_chk_mode_done(enum sclr_dsi_mode mode)
{
	u32 val = 0;

	if ((mode == SCLR_DSI_MODE_ESC) || (mode == SCLR_DSI_MODE_SPKT)) {
		val = _reg_read(reg_base + REG_SCL_DSI_MAC_EN) & 0xf0;
		return (val ^ (mode << 4)) ? -1 : 0;
	}

	if ((mode == SCLR_DSI_MODE_IDLE) || (mode == SCLR_DSI_MODE_HS)) {
		val = _reg_read(reg_base + REG_SCL_DSI_MAC_EN) & 0x0f;
		return (val == (mode)) ? 0 : -1;
	}

	return -1;
}
EXPORT_SYMBOL_GPL(sclr_dsi_chk_mode_done);

int _dsi_chk_and_clean_mode(enum sclr_dsi_mode mode)
{
	int i, ret;

	for (i = 0; i < 5; ++i) {
		udelay(20);
		ret = sclr_dsi_chk_mode_done(mode);
		if (ret == 0) {
			sclr_dsi_clr_mode();
			break;
		}
	}
	return ret;
}

#define POLY 0x8408
/*
 *                                      16   12   5
 * this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 * This works out to be 0x1021, but the way the algorithm works
 * lets us use 0x8408 (the reverse of the bit pattern).  The high
 * bit is always assumed to be set, thus we only use 16 bits to
 * represent the 17 bit value.
 */
static u16 crc16(unsigned char *data_p, unsigned short length)
{
	u8 i, data;
	u16 crc = 0xffff;

	if (length == 0)
		return (~crc);

	do {
		for (i = 0, data = 0xff & *data_p++; i < 8; i++, data >>= 1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	return crc;
}

static unsigned char ecc(unsigned char *data)
{
	char D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12;
	char D13, D14, D15, D16, D17, D18, D19, D20, D21, D22, D23;
	char P0, P1, P2, P3, P4, P5, P6, P7;

	D0  = data[0] & 0x01;
	D1  = (data[0] >> 1) & 0x01;
	D2  = (data[0] >> 2) & 0x01;
	D3  = (data[0] >> 3) & 0x01;
	D4  = (data[0] >> 4) & 0x01;
	D5  = (data[0] >> 5) & 0x01;
	D6  = (data[0] >> 6) & 0x01;
	D7  = (data[0] >> 7) & 0x01;

	D8  = data[1] & 0x01;
	D9  = (data[1] >> 1) & 0x01;
	D10 = (data[1] >> 2) & 0x01;
	D11 = (data[1] >> 3) & 0x01;
	D12 = (data[1] >> 4) & 0x01;
	D13 = (data[1] >> 5) & 0x01;
	D14 = (data[1] >> 6) & 0x01;
	D15 = (data[1] >> 7) & 0x01;

	D16 = data[2] & 0x01;
	D17 = (data[2] >> 1) & 0x01;
	D18 = (data[2] >> 2) & 0x01;
	D19 = (data[2] >> 3) & 0x01;
	D20 = (data[2] >> 4) & 0x01;
	D21 = (data[2] >> 5) & 0x01;
	D22 = (data[2] >> 6) & 0x01;
	D23 = (data[2] >> 7) & 0x01;

	P7 = 0;
	P6 = 0;
	P5 = (D10^D11^D12^D13^D14^D15^D16^D17^D18^D19^D21^D22^D23) & 0x01;
	P4 = (D4^D5^D6^D7^D8^D9^D16^D17^D18^D19^D20^D22^D23) & 0x01;
	P3 = (D1^D2^D3^D7^D8^D9^D13^D14^D15^D19^D20^D21^D23) & 0x01;
	P2 = (D0^D2^D3^D5^D6^D9^D11^D12^D15^D18^D20^D21^D22) & 0x01;
	P1 = (D0^D1^D3^D4^D6^D8^D10^D12^D14^D17^D20^D21^D22^D23) & 0x01;
	P0 = (D0^D1^D2^D4^D5^D7^D10^D11^D13^D16^D20^D21^D22^D23) & 0x01;

	return (P7 << 7) | (P6 << 6) | (P5 << 5) | (P4 << 4) |
		(P3 << 3) | (P2 << 2) | (P1 << 1) | P0;
}

/**
 * sclr_dsi_long_packet_raw - send dsi long packet by escapet-lpdt.
 *
 * @param data: long packet data including header and crc.
 * @param count: number of long packet data.
 * @return: 0 if success
 */
int sclr_dsi_long_packet_raw(const u8 *data, u8 count)
{
	uintptr_t addr = reg_base + REG_SCL_DSI_ESC_TX0;
	u32 val = 0;
	u8 i = 0, packet_count, data_offset = 0;
	int ret;
	char str[128];

	pr_debug("%s; count(%d)\n", __func__, count);
	while (count != 0) {
		if (count <= SCL_MAX_DSI_LP) {
			packet_count = count;
		} else if (count == SCL_MAX_DSI_LP + 1) {
			// [HW WorkAround] LPDT over can't take just one byte
			packet_count = SCL_MAX_DSI_LP - 1;
		} else {
			packet_count = SCL_MAX_DSI_LP;
		}
		count -= packet_count;
		val = 0x01 | ((packet_count - 1) << 8) | (count ? 0 : 0x10000);
		_reg_write(reg_base + REG_SCL_DSI_ESC, val);
		pr_debug("%s: esc reg(%#x)\n", __func__, val);

		snprintf(str, 128, "%s: packet_count(%d) data(", __func__, packet_count);
		for (i = 0; i < packet_count; i += 4) {
			if (packet_count - i < 4) {
				val = 0;
				memcpy(&val, &data[data_offset], packet_count - i);
				data_offset += packet_count - i;
				_reg_write(addr + i, val);
				snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
				break;
			}
			memcpy(&val, &data[data_offset], 4);
			data_offset += 4;
			_reg_write(addr + i, val);
			snprintf(str + strlen(str), 128 - strlen(str), "%#x ", val);
		}
		pr_debug("%s)\n", str);

		sclr_dsi_set_mode(SCLR_DSI_MODE_ESC);
		ret = _dsi_chk_and_clean_mode(SCLR_DSI_MODE_ESC);
		if (ret != 0) {
			pr_err("%s: packet_count(%d) data0(%#x)\n", __func__, packet_count, data[0]);
			break;
		}
	}
	return ret;
}

/**
 * sclr_dsi_long_packet - send dsi long packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: long packet data
 * @param count: number of long packet data, 100 at most.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int sclr_dsi_long_packet(u8 di, const u8 *data, u8 count, bool sw_mode)
{
	u8 packet[128] = {di, count & 0xff, count >> 8, 0};
	u16 crc;

	if (count > 128 - 6) {
		pr_err("%s: count(%d) invalid\n", __func__, count);
		return -1;
	}

	packet[3] = ecc(packet);
	memcpy(&packet[4], data, count);
	count += 4;
	crc = crc16(packet, count);
	packet[count++] = crc & 0xff;
	packet[count++] = crc >> 8;

	if (!sw_mode)
		return sclr_dsi_long_packet_raw(packet, count);

	dpyh_mipi_tx_manual_packet(packet, count);
	return 0;
}

/**
 * sclr_dsi_short_packet - send dsi short packet by escapet-lpdt.
 *   *NOTE*: ecc is hw-generated.
 *
 * @param di: data ID
 * @param data: short packet data
 * @param count: number of short packet data, 1 or 2.
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: 0 if success
 */
int sclr_dsi_short_packet(u8 di, const u8 *data, u8 count, bool sw_mode)
{
	u32 val = 0;

	if ((count > SCL_MAX_DSI_SP) || (count == 0))
		return -1;

	val = di;
	if (count == 2) {
		//val = 0x15;
		val |= (data[0] << 8) | (data[1] << 16);
	} else {
		//val = 0x05;
		val |= data[0] << 8;
	}

	if (!sw_mode) {
		_reg_write_mask(reg_base + REG_SCL_DSI_HS_0, 0x00ffffff, val);
		sclr_dsi_set_mode(SCLR_DSI_MODE_SPKT);
		return _dsi_chk_and_clean_mode(SCLR_DSI_MODE_SPKT);
	}

	val |= (ecc((u8 *)&val) << 24);
	dpyh_mipi_tx_manual_packet((u8 *)&val, 4);
	return 0;
}

/**
 * sclr_dsi_dcs_write_buffer - send dsi packet by escapet-lpdt.
 *
 * @param di: data ID
 * @param data: packet data
 * @param count: number of packet data
 * @param sw_mode: use sw-overwrite to create dcs cmd
 * @return: Zero on success or a negative error code on failure.
 */
int sclr_dsi_dcs_write_buffer(u8 di, const void *data, size_t len, bool sw_mode)
{
	if (len == 0) {
		pr_err("[cvi_mipi_tx] %s: 0 param unacceptable.\n", __func__);
		return -1;
	}

	if ((di == 0x06) || (di == 0x05) || (di == 0x04) || (di == 0x03)) {
		if (len != 1) {
			pr_err("[cvi_mipi_tx] %s: cmd(0x%02x) should has 1 param.\n", __func__, di);
			return -1;
		}
		return sclr_dsi_short_packet(di, data, len, sw_mode);
	}
	if ((di == 0x15) || (di == 0x37) || (di == 0x13) || (di == 0x14) || (di == 0x23)) {
		if (len != 2) {
			pr_err("[cvi_mipi_tx] %s: cmd(0x%02x) should has 2 param.\n", __func__, di);
			return -1;
		}
		return sclr_dsi_short_packet(di, data, len, sw_mode);
	}
	if ((di == 0x29) || (di == 0x39))
		return sclr_dsi_long_packet(di, data, len, sw_mode);

	return sclr_dsi_long_packet(di, data, len, sw_mode);
}
EXPORT_SYMBOL_GPL(sclr_dsi_dcs_write_buffer);

#define ACK_WR       0x02
#define GEN_READ_LP  0x1A
#define GEN_READ_SP1 0x11
#define GEN_READ_SP2 0x12
#define DCS_READ_LP  0x1C
#define DCS_READ_SP1 0x21
#define DCS_READ_SP2 0x22

int sclr_dsi_dcs_read_buffer(u8 di, const u16 data_param, u8 *data, size_t len)
{
	int ret = 0;
	u32 rx_data;
	int i = 0;

	if (len > 4)
		len = 4;

	if (sclr_dsi_get_mode() == SCLR_DSI_MODE_HS) {
		pr_err("[cvi_mipi_tx] %s: not work in HS.\n", __func__);
		return -1;
	}

	// [2:0] reg_esc_mode
	// [7:4] reg_esc_trig
	// [11:8] reg_tx_bc
	// [15:12] reg_bta_rx_bc
	// [16:16] reg_tx_bc_over: TX LPDT transfer over,0: Extend to next trigger,1: Transfer over in this trigger
	// only set necessery bits
	_reg_write_mask(reg_base + REG_SCL_DSI_ESC, 0x07, 0x04);

	// send read cmd
	sclr_dsi_short_packet(di, (u8 *)&data_param, 2, true);

	// goto BTA
	sclr_dsi_set_mode(SCLR_DSI_MODE_ESC);
	if (_dsi_chk_and_clean_mode(SCLR_DSI_MODE_ESC) != 0) {
		pr_err("[cvi_mipi_tx] %s: BTA error.\n", __func__);
		return ret;
	}

	// check result
	rx_data = _reg_read(reg_base + REG_SCL_DSI_ESC_RX0);
	switch (rx_data & 0xff) {
	case GEN_READ_SP1:
	case DCS_READ_SP1:
		data[0] = (rx_data >> 8) & 0xff;
		break;
	case GEN_READ_SP2:
	case DCS_READ_SP2:
		data[0] = (rx_data >> 8) & 0xff;
		data[1] = (rx_data >> 16) & 0xff;
		break;
	case GEN_READ_LP:
	case DCS_READ_LP:
		rx_data = _reg_read(reg_base + REG_SCL_DSI_ESC_RX1);
		for (i = 0; i < len; ++i)
			data[i] = (rx_data >> (i * 8)) & 0xff;
		break;
	case ACK_WR:
		pr_err("[cvi_mipi_tx] %s: dcs read, ack with error(%#x %#x).\n"
			, __func__, (rx_data >> 8) & 0xff, (rx_data >> 16) & 0xff);
		ret = -1;
		break;
	default:
		pr_err("[cvi_mipi_tx] %s: unknown DT, %#x.", __func__, rx_data);
		ret = -1;
		break;
	}

	//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "%s: %#x %#x\n", __func__, rx_data0, rx_data1);
	return ret;
}
EXPORT_SYMBOL_GPL(sclr_dsi_dcs_read_buffer);

int sclr_dsi_config(u8 lane_num, enum sclr_dsi_fmt fmt, u16 width)
{
	u32 val = 0;
	u8 bit_depth[] = {24, 18, 16, 30};

	if ((lane_num != 1) && (lane_num != 2) && (lane_num != 4))
		return -EINVAL;
	if (fmt > SCLR_DSI_FMT_MAX)
		return -EINVAL;

	lane_num >>= 1;
	val = (fmt << 30) | (lane_num << 24);
	_reg_write_mask(reg_base + REG_SCL_DSI_HS_0, 0xc3000000, val);
	val = (width / 10) << 16 | UPPER(width * bit_depth[fmt], 3);
	_reg_write(reg_base + REG_SCL_DSI_HS_1, val);

	return 0;
}
EXPORT_SYMBOL_GPL(sclr_dsi_config);

#ifdef  __SOC_MARS__
void i80_set_cmd0(u32 cmd)
{
	_reg_write(reg_base + REG_SCL_DISP_MCU_HW_CMD0, cmd);
}

void i80_set_cmd1(u32 cmd)
{
	cmd = (_reg_read(reg_base + REG_SCL_DISP_MCU_HW_CMD0) | cmd << 16);
	_reg_write(reg_base + REG_SCL_DISP_MCU_HW_CMD0, cmd);
}

void i80_set_cmd2(u32 cmd)
{
	_reg_write(reg_base + REG_SCL_DISP_MCU_HW_CMD1, cmd);
}

void i80_set_cmd3(u32 cmd)
{
	cmd = (_reg_read(reg_base + REG_SCL_DISP_MCU_HW_CMD1) | cmd << 16);
	_reg_write(reg_base + REG_SCL_DISP_MCU_HW_CMD1, cmd);
}

void i80_set_cmd_cnt(u32 cmdcnt)
{
	if (cmdcnt == 1) {
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(4), 0);//sw_tx_num=0
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(5), 0);
	}
	if (cmdcnt == 2) {
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(4), BIT(4));
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(5), 0);//sw_tx_num=1
	}
	if (cmdcnt == 3) {
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(4), 0);//sw_tx_num=2
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(5), BIT(5));
	}
	if (cmdcnt == 4) {
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(4), BIT(4));//sw_tx_num=3
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(5), BIT(5));
	}
}

void i80_trig(void)
{
	int cnt = 0;
	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(0), 0);//rising edge to trig
	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(0), 1);//rising edge to trig

	do {
		udelay(1);
		if (_reg_read(reg_base + REG_SCL_DISP_MCU_HW_CMD) & BIT(3)) {
			break;
		}
	} while (++cnt < 10);

	if (cnt == 10)
		pr_err("[I80] %s: hw  mcu cmd not ready.\n", __func__);

	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_HW_CMD, BIT(3), BIT(3));//rising edge to trig
}

int hw_mcu_cmd_send(void *cmds, int size)
{
	struct cvi_i80_instr *instr = (struct cvi_i80_instr *)cmds;
	int i = 0;
	unsigned int sw_cmd0, sw_cmd1, sw_cmd2, sw_cmd3;
	int cmd_cnt = 0;

	for (i = 0; i < size; i = i + 4) {
		cmd_cnt = 0;
		if (i < size) {
			cmd_cnt++;
			sw_cmd0 = (instr[i].data_type << 8) | instr[i].data;
			i80_set_cmd0(sw_cmd0);
		}
		if ((i + 1) < size) {
			cmd_cnt++;
			sw_cmd1 = (instr[i+1].data_type << 8) | instr[i+1].data;
			i80_set_cmd1(sw_cmd1);
		}
		if ((i + 2) < size) {
			cmd_cnt++;
			sw_cmd2 = (instr[i+2].data_type << 8) | instr[i+2].data;
			i80_set_cmd2(sw_cmd2);
		}
		if ((i + 3) < size) {
			cmd_cnt++;
			sw_cmd3 = (instr[i+3].data_type << 8) | instr[i+3].data;
			i80_set_cmd3(sw_cmd3);
		}
		i80_set_cmd_cnt(cmd_cnt);
		i80_trig();
	}
	return 0;
}
EXPORT_SYMBOL_GPL(hw_mcu_cmd_send);
#endif

void sclr_i80_sw_mode(bool enable)
{
	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_IF_CTRL, BIT(11) | BIT(1), enable ? 0x802 : 0x000);

	if (enable) {
		sclr_disp_tgen_enable(true);
		mdelay(40);
		sclr_disp_tgen_enable(false);
	}
}
EXPORT_SYMBOL_GPL(sclr_i80_sw_mode);

void sclr_i80_packet(u32 cmd)
{
	u8 cnt = 0;

	_reg_write(reg_base + REG_SCL_DISP_MCU_SW_CTRL, cmd);
	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_SW_CTRL, BIT(31), BIT(31));

	do {
		udelay(1);
		if (_reg_read(reg_base + REG_SCL_DISP_MCU_SW_CTRL) & BIT(24))
			break;
	} while (++cnt < 10);

	if (cnt == 10)
		pr_err("[cvi_vip] %s: cmd(%#x) not ready.\n", __func__, cmd);
}
EXPORT_SYMBOL_GPL(sclr_i80_packet);

bool sclr_i80_chk_idle(void)
{
#define I80_BUSY_WAITING_TIMES 50
#define I80_CLEAR_TRAIL 3
	u8 cnt = 0, i = 0;

	for (i = 0; i < I80_CLEAR_TRAIL; ++i) {
		do {
			if (_reg_read(reg_base + REG_SCL_DISP_MCU_STATUS) == 0x08)
				return true;
			usleep_range(1 * 1000, 3 * 1000);
		} while (++cnt < I80_BUSY_WAITING_TIMES);

		pr_err("%s: waiting idle failed. sw try to clear it.\n", __func__);
		_reg_write_mask(reg_base + REG_SCL_DISP_MCU_IF_CTRL, BIT(10), BIT(10));
	}

	pr_err("%s: failed.\n", __func__);
	return false;
}

void sclr_i80_run(void)
{
	if (!sclr_i80_chk_idle())
		return; //do nothing if i80 status not idle

	_reg_write_mask(reg_base + REG_SCL_DISP_MCU_IF_CTRL, BIT(11), BIT(11));

	sclr_i80_chk_idle(); //wait i80 idle
}
EXPORT_SYMBOL_GPL(sclr_i80_run);

/****************************************************************************
 * SCALER CTRL
 ****************************************************************************/
/**
 * sclr_ctrl_init - setup all sc instances.
 *
 */
void sclr_ctrl_init(bool is_resume)
{
	union sclr_intr intr_mask;
	union sclr_rt_cfg rt_cfg;
	bool disp_from_sc = false;
	unsigned int i = 0;
	u8 layer;

	memset(&intr_mask, 0, sizeof(intr_mask));

	if (!is_resume) {
		// init variables
		memset(&g_top_cfg, 0, sizeof(g_top_cfg));
		memset(&g_sc_cfg, 0, sizeof(g_sc_cfg));
		memset(&g_bd_cfg, 0, sizeof(g_bd_cfg));
		memset(&g_cir_cfg, 0, sizeof(g_cir_cfg));
		memset(&g_img_cfg, 0, sizeof(g_img_cfg));
		memset(&g_odma_cfg, 0, sizeof(g_odma_cfg));
		memset(&g_disp_cfg, 0, sizeof(g_disp_cfg));
		memset(&disp_timing, 0, sizeof(disp_timing));
		memset(&g_gop_cfg, 0, sizeof(g_gop_cfg));
		memset(&g_top_sb_cfg, 0, sizeof(g_top_sb_cfg));

		// init disp mask up lock
		spin_lock_init(&disp_mask_spinlock);

		for (i = 0; i < SCL_MAX_INST; ++i) {
			g_sc_cfg[i].force_clk = true;
			g_sc_cfg[i].coef = SCL_COEF_MAX;
			g_sc_cfg[i].sc.mir_enable = true;
			g_odma_cfg[i].flip = SCL_FLIP_NO;
			g_odma_cfg[i].burst = false;
			layer = 0;
			g_gop_cfg[i][layer].gop_ctrl.b.burst = SCL_DEFAULT_BURST;
			layer = 1;
			g_gop_cfg[i][layer].gop_ctrl.b.burst = SCL_DEFAULT_BURST;
		}

		g_img_cfg[SCL_IMG_D].burst = SCL_DEFAULT_BURST;
		g_img_cfg[SCL_IMG_D].src = SCL_INPUT_MEM;
		g_img_cfg[SCL_IMG_D].force_clk = true;
		g_img_cfg[SCL_IMG_V].burst = SCL_DEFAULT_BURST;
		g_img_cfg[SCL_IMG_V].src = SCL_INPUT_MEM;
		g_img_cfg[SCL_IMG_V].force_clk = true;
		g_top_cfg.ip_trig_src = true;	// no more use
		g_top_cfg.force_clk_enable = true;
		g_top_cfg.sclr_enable[0] = false;
		g_top_cfg.sclr_enable[1] = false;
		g_top_cfg.sclr_enable[2] = false;
		g_top_cfg.sclr_enable[3] = false;
		g_top_cfg.disp_enable = false;
		g_top_cfg.disp_from_sc = disp_from_sc;
		g_top_cfg.img_in_d_trig_src = SCL_IMG_TRIG_SRC_SW;
		g_top_cfg.img_in_v_trig_src = SCL_IMG_TRIG_SRC_SW;
		g_disp_cfg.disp_from_sc = disp_from_sc;
		g_disp_cfg.cache_mode = true;
		g_disp_cfg.sync_ext = false;
		g_disp_cfg.tgen_en = false;
		g_disp_cfg.fmt = SCL_FMT_RGB_PLANAR;
		g_disp_cfg.in_csc = SCL_CSC_NONE;
		g_disp_cfg.out_csc = SCL_CSC_NONE;
		g_disp_cfg.out_bit = 8;
		g_disp_cfg.drop_mode = SCL_DISP_DROP_MODE_DITHER;
		g_disp_cfg.mem.width = 80;
		g_disp_cfg.mem.height = 80;
		//burst length = (burst+1)*16 bytes
		//display burst length keep 128 bytes
		g_disp_cfg.burst = SCL_DEFAULT_BURST;
		//display osd burst length set to 256 bytes for short latency
		g_disp_cfg.gop_cfg.gop_ctrl.b.burst = SCL_DEFAULT_BURST;
	} else {
		for (i = 0; i < SCL_MAX_INST; ++i)
			g_sc_cfg[i].coef = SCL_COEF_MAX;
	}

	// get current hw-timings
	sclr_disp_timing_setup_from_reg();

	// init hw
	sclr_top_set_cfg(&g_top_cfg);

	rt_cfg.b.sc_d_rt = 0;
	rt_cfg.b.sc_v_rt = 0;
	rt_cfg.b.sc_rot_rt = 0;
	rt_cfg.b.img_d_rt = 0;
	rt_cfg.b.img_v_rt = 0;
	rt_cfg.b.img_d_sel = 1;
	sclr_rt_set_cfg(rt_cfg);

	intr_mask.b.img_in_d_frame_end = true;
	intr_mask.b.img_in_v_frame_end = true;
	intr_mask.b.scl0_frame_end = true;
	intr_mask.b.scl1_frame_end = true;
	intr_mask.b.scl2_frame_end = true;
	intr_mask.b.scl3_frame_end = true;
	intr_mask.b.prog_too_late = true;
	intr_mask.b.cmdq = true;
	intr_mask.b.disp_frame_end = true;
	sclr_set_intr_mask(intr_mask);

#if 0
	sclr_img_reg_shadow_sel(0, false);
	sclr_img_reg_shadow_sel(1, false);
	sclr_disp_reg_shadow_sel(false);
	for (i = 0; i < SCL_MAX_INST; ++i) {
		sclr_reg_shadow_sel(i, false);
		sclr_init(i);
		sclr_set_cfg(i, false, false, true, false);
		sclr_reg_force_up(i);
	}

	sclr_disp_tgen_enable(false);
	sclr_disp_set_cfg(&g_disp_cfg);
#endif
	// pull high threshold of urgent to avoid garbage
	_reg_write(reg_base + REG_SCL_DISP_FIFO_THR, 0x10201020);

	// reg_shrd_sel(bit[9]): 1: raw register
	_reg_write(reg_base + REG_SCL_TOP_SHD, 1<<9);

	sclr_top_reg_done();
	sclr_top_reg_force_up();
	sclr_top_pg_late_clr();
}

/**
 * sclr_set_opencv_scale - setup scaling param to align with opencv bilinear.
 *
 * @param inst: (0~3), the instance of sc
 */
void sclr_set_opencv_scale(u8 inst)
{
	u32 h_fac, v_fac;
	u32 h_pos, v_pos;
	struct sclr_rect crop_rect;
	bool use_2tap = (inst != 1) ? true : false;	// Only sc_v1 is 4 tap

	sclr_set_scale_mode(inst, true, false, false);

	if (!use_2tap) {
		h_fac = ((g_sc_cfg[inst].sc.crop.w) << 13)
			/ (g_sc_cfg[inst].sc.dst.w);
		_reg_write_mask(reg_base + REG_SCL_SC_H_CFG(inst), 0x03ffff00, h_fac << 8);
		v_fac = ((g_sc_cfg[inst].sc.crop.h) << 13)
			/ (g_sc_cfg[inst].sc.dst.h);
		_reg_write_mask(reg_base + REG_SCL_SC_V_CFG(inst), 0x03ffff00, v_fac << 8);

		h_pos = (h_fac > (1 << 13)) ? ((h_fac - (1 << 13)) >> 1) : 0;
		v_pos = (v_fac > (1 << 13)) ? ((v_fac - (1 << 13)) >> 1) : 0;
		crop_rect.x = g_sc_cfg[inst].sc.crop.x + (h_pos >> 13);
		crop_rect.y = g_sc_cfg[inst].sc.crop.y + (v_pos >> 13);
		crop_rect.w = g_sc_cfg[inst].sc.crop.w - (h_pos >> 13);
		crop_rect.h = g_sc_cfg[inst].sc.crop.h - (v_pos >> 13);

		sclr_set_crop(inst, crop_rect, false);
		sclr_set_scale_phase(inst, h_pos & 0x1fff, v_pos & 0x1fff);

		pr_debug("h_fac(%#x) v_fac(%#x)\n", h_fac, v_fac);
		pr_debug("h_pos(%#x) v_pos(%#x)\n", h_pos, v_pos);
	} else {
		sclr_set_crop(inst, g_sc_cfg[inst].sc.crop, false);
		sclr_set_scale_2tap(inst);
	}
}

/**
 * sclr_ctrl_set_scale - setup scaling
 *
 * @param inst: (0~3), the instance of sc
 * @param cfg: scaling settings, include in/crop/out size.
 */
void sclr_ctrl_set_scale(u8 inst, struct sclr_scale_cfg *cfg)
{
	if (inst >= SCL_MAX_INST) {
		pr_err("[cvi-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_cont("for the requirement(%d)\n", inst);
		return;
	}
	if (memcmp(cfg, &g_sc_cfg[inst].sc, sizeof(*cfg)) == 0)
		return;

	sclr_set_input_size(inst, cfg->src, true);

	// if crop invalid, use src-size
	if (cfg->crop.w + cfg->crop.x > cfg->src.w) {
		cfg->crop.x = 0;
		cfg->crop.w = cfg->src.w;
	}
	if (cfg->crop.h + cfg->crop.y > cfg->src.h) {
		cfg->crop.y = 0;
		cfg->crop.h = cfg->src.h;
	}
	sclr_set_crop(inst, cfg->crop, true);
	sclr_set_output_size(inst, cfg->dst);

	g_sc_cfg[inst].sc.mir_enable = cfg->mir_enable;
	g_sc_cfg[inst].sc.tile_enable = cfg->tile_enable;
	g_sc_cfg[inst].sc.tile = cfg->tile;
	sclr_set_scale(inst);
	//if (cfg->tile_enable)
	//	sclr_tile_cal_size(inst);
}

/**
 * sclr_ctrl_set_input - setup input
 *
 * @param inst: (0~1), the instance of img_in
 * @param input: input mux
 * @param fmt: color format
 * @param csc: csc which used to convert from yuv to rgb.
 * @return: 0 if success
 */
int sclr_ctrl_set_input(enum sclr_img_in inst, enum sclr_input input,
			enum sclr_format fmt, enum sclr_csc csc, bool isp_online)
{
	int ret = 0;

	if (inst >= SCL_IMG_MAX)
		return -EINVAL;

	g_img_cfg[inst].src = input;
	g_img_cfg[inst].fmt = fmt;
	if (input == SCL_INPUT_ISP)
		g_img_cfg[inst].trig_src = SCL_IMG_TRIG_SRC_SW;
	else {
		g_img_cfg[inst].trig_src =
		    (inst == SCL_IMG_D && g_top_cfg.disp_from_sc)
		    ? SCL_IMG_TRIG_SRC_DISP : SCL_IMG_TRIG_SRC_SW;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d) -> sclr_img_set_cfg, input=%d, fmt=%d, csc=%d, isp_online=%d\n",
			inst, input, fmt, csc, isp_online);
	sclr_img_set_cfg(inst, &g_img_cfg[inst]);

	g_img_cfg[inst].csc = csc;
	if (csc == SCL_CSC_NONE) {
		sclr_img_csc_en(inst, false);
	} else {
		sclr_img_csc_en(inst, true);
		sclr_img_set_csc(inst, &csc_mtrx[csc]);
	}

	return ret;
}

/**
 * sclr_ctrl_set_output - setup output of sc
 *
 * @param inst: (0~3), the instance of sc
 * @param cfg: csc config, including hvs/quant settings if any
 * @param fmt: color format
 * @return: 0 if success
 */
int sclr_ctrl_set_output(u8 inst, struct sclr_csc_cfg *cfg,
			 enum sclr_format fmt)
{
	if (inst >= SCL_MAX_INST)
		return -EINVAL;

	if (cfg->mode == SCL_OUT_CSC) {
		// Use yuv for csc
		// sc's data is always rgb
		if (!IS_YUV_FMT(fmt) ||
		    ((cfg->csc_type < SCL_CSC_601_LIMIT_RGB2YUV) &&
		     (cfg->csc_type != SCL_CSC_NONE)))
			return -EINVAL;
	} else {
		// Use rgb/y-only for quant/hsv
		if (IS_YUV_FMT(fmt) && (fmt != SCL_FMT_Y_ONLY))
			return -EINVAL;
	}

	sclr_set_csc_ctrl(inst, cfg);
	sclr_odma_set_fmt(inst, fmt);

	return 0;
}

/**
 * sclr_ctrl_set_disp_src - setup input-src of disp.
 *
 * @param disp_from_sc: true(from sc_0); false(from mem)
 * @return: 0 if success
 */
int sclr_ctrl_set_disp_src(bool disp_from_sc)
{
	g_top_cfg.disp_from_sc = disp_from_sc;
	g_disp_cfg.disp_from_sc = disp_from_sc;
	if (disp_from_sc)
		g_top_cfg.img_in_d_trig_src = SCL_IMG_TRIG_SRC_DISP;

	sclr_top_set_cfg(&g_top_cfg);
	sclr_disp_set_cfg(&g_disp_cfg);
	sclr_set_cfg(0, g_sc_cfg[0].sc_bypass, g_sc_cfg[0].gop_bypass,
		     g_sc_cfg[0].cir_bypass, disp_from_sc);

	return 0;
}
EXPORT_SYMBOL_GPL(sclr_ctrl_set_disp_src);


/**
 * vip_axi_realtime_fab_priority - change vip_subsys m1/2/3/4 port priority.
 */
void vip_axi_realtime_fab_priority(void)
{
	void __iomem *vip_base;

	// axi_realtime_fab priority
	vip_base = ioremap(0x0A0C8070, 4);
	iowrite32(0x1032AA0F, vip_base);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "vip_subsys patch applied.\n");

	iounmap(vip_base);
}
EXPORT_SYMBOL_GPL(vip_axi_realtime_fab_priority);

/**
 * _ddr_ctrl_patch - update ddr controller for OSD soft timeout 250ns.
 */
void _ddr_ctrl_patch(bool enable)
{
	void __iomem *ddr_base;

	// _ddr_ctrl_patch(enable); in _fb_enable functions
	ddr_base = ioremap(0x08004544, 8);

	if (ddr_base) {
		if (enable) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "ddr patch applied.\n");
			iowrite32(0x01110e00, ddr_base);
			iowrite32(0x0, ddr_base + 0x04);
		} else {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "ddr patch exit.\n");
			iowrite32(0x07, ddr_base);
			iowrite32(0x6a, ddr_base + 0x04);
		}
	}

	iounmap(ddr_base);
}
EXPORT_SYMBOL_GPL(_ddr_ctrl_patch);

/*
 * @inst: 0: sc_d, 1: sc_v1, 2: sc_v2, 3: sc_v3
 * @sb_mode: 0: disable, 1: free-run mode, 2: frame-base mode
 * @sb_size: slice buffer line number, 0: 64 line, 1: 128 line
 * @sb_nb: slice buffer depth
 * @sb_wr_ctrl_idx: 0: sb_wr_ctrl0, 1: sb_wr_ctrl1
 */
void sclr_set_sclr_to_vc_sb(u8 inst, u8 sb_mode, u8 sb_size, u8 sb_nb, u8 sb_wr_ctrl_idx)
{
	struct sclr_odma_sb_cfg odma_sb_cfg;

	if (inst >= SCL_MAX_INST || sb_mode >= 3 ||
	    (sb_mode && (sb_size >= 2 || sb_wr_ctrl_idx >= 2))) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "sc(%d), sb_mode(%d), sb_wr_ctrl_idx(%d) invalid\n",
				inst, sb_mode, sb_wr_ctrl_idx);
		return;
	}
	sclr_odma_get_sb_default(&odma_sb_cfg);

	if (sb_mode == 0) {
		odma_sb_cfg.sb_mode = 0;
		sclr_odma_set_sb(inst, &odma_sb_cfg);
		return;
	}

	// top
	if (!sb_wr_ctrl_idx)
		g_top_sb_cfg.sb_wr_ctrl0 = inst;
	else
		g_top_sb_cfg.sb_wr_ctrl1 = inst;

	sclr_top_set_sb(&g_top_sb_cfg);

	// odma
	odma_sb_cfg.sb_mode = sb_mode;
	odma_sb_cfg.sb_size = sb_size;
	odma_sb_cfg.sb_nb = sb_nb;
	odma_sb_cfg.sb_full_nb = sb_nb;
	sclr_odma_set_sb(inst, &odma_sb_cfg);
}


struct sclr_csc_matrix *sclr_get_csc_mtrx(enum sclr_csc csc)
{
	if (csc < SCL_CSC_MAX)
		return &csc_mtrx[csc];

	return &csc_mtrx[SCL_CSC_601_LIMIT_RGB2YUV];
}

#if defined(CONFIG_RGN_EX)
/**
 * sclr_cmdq_intr_clr - clear sclr's cmdq interrupt
 *                 bit0: cmdQ intr, bit1: cmdQ end, bit2: cmdQ wait
 *
 * @param intr_mask: the mask of the interrupt to clear.
 */
void sclr_cmdq_intr_clr(u8 intr_mask)
{
	cmdQ_intr_clr(reg_base + REG_SCL_CMDQ_BASE, intr_mask);
}

/**
 * sclr_cmdq_intr_status - sclr's cmdq interrupt status
 *                    bit0: cmdQ intr, bit1: cmdQ end, bit2: cmdQ wait
 *
 * @return: The interrupt's status
 */
u8 sclr_cmdq_intr_status(void)
{
	u8 status;

	status = cmdQ_intr_status(reg_base + REG_SCL_CMDQ_BASE);
	return status;
}

static void _map_ctrl_to_img(struct sclr_ctrl_cfg *cfg,
			     struct sclr_img_cfg *img_cfg)
{
	img_cfg->burst = g_img_cfg[cfg->img_inst].burst;
	img_cfg->fmt = cfg->src_fmt;
	img_cfg->csc = cfg->src_csc;
	img_cfg->src = cfg->input;
	img_cfg->mem.addr0 = cfg->src_addr0;
	img_cfg->mem.addr1 = cfg->src_addr1;
	img_cfg->mem.addr2 = cfg->src_addr2;
	img_cfg->mem.start_x = 0;
	img_cfg->mem.start_y = 0;
	img_cfg->mem.width = cfg->src.w;
	img_cfg->mem.height = cfg->src.h;
	img_cfg->mem.pitch_y = IS_PACKED_FMT(cfg->src_fmt)
				? VIP_ALIGN(3 * cfg->src.w)
				: VIP_ALIGN(cfg->src.w);
	if ((cfg->src_fmt == SCL_FMT_YUV420) ||
		(cfg->src_fmt == SCL_FMT_YUV422))
		img_cfg->mem.pitch_c = VIP_ALIGN(cfg->src.w >> 1);
	else
		img_cfg->mem.pitch_c = VIP_ALIGN(cfg->src.w);

	if (cfg->src_csc == SCL_CSC_NONE)
		img_cfg->csc_en = false;
	else
		img_cfg->csc_en = true;
}

static void _map_ctrl_to_sc(struct sclr_ctrl_cfg *cfg,
			    struct sclr_core_cfg *sc_cfg, u8 inst)
{
	sc_cfg->sc_bypass = false;
	sc_cfg->gop_bypass = false;
	sc_cfg->cir_bypass = true;
	sc_cfg->odma_bypass = false;
	sc_cfg->sc.src = cfg->src;
	sc_cfg->sc.crop = cfg->src_crop;
	sc_cfg->sc.dst = cfg->dst[inst].window;
}

static void _map_ctrl_to_odma(struct sclr_ctrl_cfg *cfg,
			      struct sclr_odma_cfg *odma_cfg, u8 inst)
{
	odma_cfg->flip = SCL_FLIP_NO;
	odma_cfg->burst = g_odma_cfg[inst].burst;
	odma_cfg->fmt = cfg->dst[inst].fmt;
	odma_cfg->csc_cfg.csc_type = cfg->dst[inst].csc;
	odma_cfg->mem.addr0 = cfg->dst[inst].addr0;
	odma_cfg->mem.addr1 = cfg->dst[inst].addr1;
	odma_cfg->mem.addr2 = cfg->dst[inst].addr2;
	odma_cfg->mem.start_x = cfg->dst[inst].offset.x;
	odma_cfg->mem.start_y = cfg->dst[inst].offset.y;
	odma_cfg->mem.width = cfg->dst[inst].window.w;
	odma_cfg->mem.height = cfg->dst[inst].window.h;
	odma_cfg->mem.pitch_y = IS_PACKED_FMT(cfg->dst[inst].fmt)
				? VIP_ALIGN(3 * cfg->dst[inst].frame.w)
				: VIP_ALIGN(cfg->dst[inst].frame.w);
	if ((cfg->dst[inst].fmt == SCL_FMT_YUV420) ||
		(cfg->dst[inst].fmt == SCL_FMT_YUV422))
		odma_cfg->mem.pitch_c = VIP_ALIGN(cfg->dst[inst].frame.w >> 1);
	else
		odma_cfg->mem.pitch_c = VIP_ALIGN(cfg->dst[inst].frame.w);
}

int _map_img_to_cmdq(struct sclr_ctrl_cfg *ctrl_cfg, union cmdq_set *cmd_start,
		     u16 cmd_idx)
{
	struct sclr_img_cfg img_cfg;
	struct sclr_csc_matrix *mtrx;
	u8 img_inst = ctrl_cfg->img_inst;

	if (img_inst >= SCL_IMG_MAX)
		return cmd_idx;

	_map_ctrl_to_img(ctrl_cfg, &img_cfg);
	mtrx = &csc_mtrx[img_cfg.csc];

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CFG(img_inst),
			 (img_cfg.csc_en << 12) | (img_cfg.burst << 8) |
			 (img_cfg.fmt << 4) | img_cfg.src);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_OFFSET(img_inst),
			 (img_cfg.mem.start_y << 16) | img_cfg.mem.start_x);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_SIZE(img_inst),
			 ((img_cfg.mem.height - 1) << 16) |
			 (img_cfg.mem.width - 1));
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_PITCH_Y(img_inst),
			 img_cfg.mem.pitch_y);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_PITCH_C(img_inst),
			 img_cfg.mem.pitch_c);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR0_L(img_inst),
			 img_cfg.mem.addr0);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR0_H(img_inst),
			 img_cfg.mem.addr0 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR1_L(img_inst),
			 img_cfg.mem.addr1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR1_H(img_inst),
			 img_cfg.mem.addr1 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR2_L(img_inst),
			 img_cfg.mem.addr2);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR2_H(img_inst),
			 img_cfg.mem.addr2 >> 32);

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF0(img_inst),
			 (mtrx->coef[0][1] << 16) | mtrx->coef[0][0]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF1(img_inst),
			 mtrx->coef[0][2]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF2(img_inst),
			 (mtrx->coef[1][1] << 16) | mtrx->coef[1][0]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF3(img_inst),
			 mtrx->coef[1][2]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF4(img_inst),
			 (mtrx->coef[2][1] << 16) | mtrx->coef[2][0]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF5(img_inst),
			 mtrx->coef[2][2]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_SUB(img_inst),
			 (mtrx->sub[2] << 16) | (mtrx->sub[1] << 8)
			  | mtrx->sub[0]);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_ADD(img_inst),
			 (mtrx->add[2] << 16) | (mtrx->add[1] << 8)
			  | mtrx->add[0]);

	return cmd_idx;
}

int _map_sc_to_cmdq(struct sclr_ctrl_cfg *ctrl_cfg, u8 j,
		    union cmdq_set *cmd_start, u16 cmd_idx)
{
	u32 tmp = 0;
	u8 inst = ctrl_cfg->dst[j].inst;
	struct sclr_core_cfg sc_cfg;
	struct sclr_odma_cfg odma_cfg;

	if (inst >= SCL_MAX_INST)
		return cmd_idx;

	_map_ctrl_to_sc(ctrl_cfg, &sc_cfg, j);
	_map_ctrl_to_odma(ctrl_cfg, &odma_cfg, j);

	// sc core
	tmp = 0;
	if (sc_cfg.sc_bypass)
		tmp |= 0x02;
	if (sc_cfg.gop_bypass)
		tmp |= 0x04;
	if (sc_cfg.cir_bypass)
		tmp |= 0x20;
	if (sc_cfg.odma_bypass)
		tmp |= 0x40;

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CFG(inst), tmp);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SRC_SIZE(inst),
			 ((sc_cfg.sc.src.h - 1) << 12) | (sc_cfg.sc.src.w - 1));
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CROP_OFFSET(inst),
			 (sc_cfg.sc.crop.y << 12) | sc_cfg.sc.crop.x);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CROP_SIZE(inst),
			 ((sc_cfg.sc.crop.h - 1) << 16)
			  | (sc_cfg.sc.crop.w - 1));
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_OUT_SIZE(inst),
			 ((sc_cfg.sc.dst.h - 1) << 16) | (sc_cfg.sc.dst.w - 1));
	tmp = ((sc_cfg.sc.crop.w - 1) << 13) / (sc_cfg.sc.dst.w - 1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SC_H_CFG(inst),
			 tmp << 8);
	tmp = ((sc_cfg.sc.crop.h - 1) << 13) / (sc_cfg.sc.dst.h - 1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SC_V_CFG(inst),
			 tmp << 8);

	// sc odma
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_CFG(inst),
			 (odma_cfg.flip << 16) |  (odma_cfg.fmt << 8) |
			 odma_cfg.burst);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_OFFSET_X(inst),
			 odma_cfg.mem.start_x);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_OFFSET_Y(inst),
			 odma_cfg.mem.start_y);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_WIDTH(inst),
			 odma_cfg.mem.width - 1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_HEIGHT(inst),
			 odma_cfg.mem.height - 1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_PITCH_Y(inst),
			 odma_cfg.mem.pitch_y);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_PITCH_C(inst),
			 odma_cfg.mem.pitch_c);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR0_L(inst),
			 odma_cfg.mem.addr0);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR0_H(inst),
			 odma_cfg.mem.addr0 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR1_L(inst),
			 odma_cfg.mem.addr1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR1_H(inst),
			 odma_cfg.mem.addr1 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR2_L(inst),
			 odma_cfg.mem.addr2);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR2_H(inst),
			 odma_cfg.mem.addr2 >> 32);

	// sc csc
	if (odma_cfg.csc_cfg.csc_type == SCL_CSC_NONE) {
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_EN(inst), 0);
	} else {
		struct sclr_csc_matrix *cfg = &csc_mtrx[odma_cfg.csc_cfg.csc_type];

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_EN(inst), 1);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF0(inst),
				 (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF1(inst),
				 (cfg->coef[1][0] << 16) | cfg->coef[0][2]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF2(inst),
				 (cfg->coef[1][2] << 16) | cfg->coef[1][1]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF3(inst),
				 (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF4(inst), cfg->coef[2][2]);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_OFFSET(inst),
				 (cfg->add[2] << 16) | (cfg->add[1] << 8) |
				 cfg->add[0]);
	}

	return cmd_idx;
}

/**
 * sclr_engine_cmdq - trigger sc  by cmdq
 *
 * @param cfgs: settings for cmdq
 * @param cnt: the number of settings
 * @param cmdq_addr: memory-address to put cmdq
 */
void sclr_engine_cmdq(struct sclr_ctrl_cfg *cfgs, u8 cnt, void *cmdq_addr,
		uintptr_t phyaddr)
{
	u8 i = 0, j = 0;
	u16 cmd_idx = 0;
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_addr;
	u32 tmp = 0, inst_enable = 0;
	u32 flag_num = 1;   // 0: sc_str_flag, sc_stp_flag

	_reg_write(reg_base + REG_SCL_TOP_CMDQ_START, 0x18000);
	_reg_write(reg_base + REG_SCL_TOP_CMDQ_STOP, 0x18000);

	for (i = 0; i < cnt; ++i) {
		struct sclr_ctrl_cfg *cfg = (cfgs + i);
		u8 img_inst = cfg->img_inst;

		cmd_idx = _map_img_to_cmdq(cfg, cmd_start, cmd_idx);

		for (j = 0; j < SCL_MAX_INST; ++j)
			cmd_idx = _map_sc_to_cmdq(cfg, j, cmd_start, cmd_idx);

		img_inst = (~img_inst) & 0x01;
		// modify & clear sc-cmdq's stop flag
		tmp = 0x10000 | BIT(img_inst+1);
		inst_enable = 0;
		for (j = 0; j < SCL_MAX_INST; ++j) {
			u8 inst = cfg->dst[j].inst;

			if (inst < SCL_MAX_INST) {
				tmp |= BIT(inst + 3);
				inst_enable |= BIT(inst);
			}
		}
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_CMDQ_STOP,
				 tmp);
		tmp = _reg_read(reg_base + REG_SCL_TOP_CFG1);
		tmp &= ~0x1f;
		tmp |= inst_enable;
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_CFG1,
				 tmp);

		// start sclr
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_IMG_CTRL,
				 BIT(img_inst));

		// wait end-inter(1)
		cmdQ_set_wait(&cmd_start[cmd_idx++], false, flag_num, 0);
	}
	// clear intr
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, REG_SCL_TOP_INTR_STATUS,
			 0x1000ffff);

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;

	cmdQ_intr_ctrl(reg_base + REG_SCL_CMDQ_BASE, 0x02);

	cmdQ_engine(reg_base + REG_SCL_CMDQ_BASE, phyaddr,
		    REG_SCL_CMDQ_TOP_BASE >> 22, true, false, cmd_idx);
}

int _map_rgnex_img_to_cmdq(struct sclr_rgnex_cfg *rgnex_cfg, union cmdq_set *cmd_start,
		     u16 cmd_idx)
{
	struct sclr_img_cfg img_cfg;
	struct sclr_csc_matrix *mtrx;
	u8 img_inst = SCL_IMG_D;

	img_cfg.burst = g_img_cfg[img_inst].burst;
	img_cfg.fmt = rgnex_cfg->fmt;
	img_cfg.csc = rgnex_cfg->src_csc;
	if (rgnex_cfg->src_csc == SCL_CSC_NONE)
		img_cfg.csc_en = false;
	else
		img_cfg.csc_en = true;
	img_cfg.src = SCL_INPUT_MEM;
	img_cfg.mem.addr0 = rgnex_cfg->addr0;
	img_cfg.mem.addr1 = rgnex_cfg->addr1;
	img_cfg.mem.addr2 = rgnex_cfg->addr2;
	img_cfg.mem.pitch_y = rgnex_cfg->bytesperline[0];
	img_cfg.mem.pitch_c = rgnex_cfg->bytesperline[1];
	mtrx = &csc_mtrx[img_cfg.csc];

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CFG(img_inst),
			 (img_cfg.csc_en << 12) | (img_cfg.burst << 8) |
			 (img_cfg.fmt << 4) | img_cfg.src);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_PITCH_Y(img_inst),
			 img_cfg.mem.pitch_y);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_PITCH_C(img_inst),
			 img_cfg.mem.pitch_c);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR0_L(img_inst),
			 img_cfg.mem.addr0);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR0_H(img_inst),
			 img_cfg.mem.addr0 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR1_L(img_inst),
			 img_cfg.mem.addr1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR1_H(img_inst),
			 img_cfg.mem.addr1 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR2_L(img_inst),
			 img_cfg.mem.addr2);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_ADDR2_H(img_inst),
			 img_cfg.mem.addr2 >> 32);

	if (img_cfg.csc != SCL_CSC_NONE) {
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF0(img_inst),
			 (mtrx->coef[0][1] << 16) | mtrx->coef[0][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF1(img_inst),
				 mtrx->coef[0][2]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF2(img_inst),
				 (mtrx->coef[1][1] << 16) | mtrx->coef[1][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF3(img_inst),
				 mtrx->coef[1][2]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF4(img_inst),
				 (mtrx->coef[2][1] << 16) | mtrx->coef[2][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_COEF5(img_inst),
				 mtrx->coef[2][2]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_SUB(img_inst),
				 (mtrx->sub[2] << 16) | (mtrx->sub[1] << 8)
				  | mtrx->sub[0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_CSC_ADD(img_inst),
				 (mtrx->add[2] << 16) | (mtrx->add[1] << 8)
				  | mtrx->add[0]);
	}

	return cmd_idx;
}

int _map_rgnex_sc_to_cmdq(struct sclr_rgnex_cfg *rgnex_cfg, u8 j,
		    union cmdq_set *cmd_start, u16 cmd_idx)
{
	u32 tmp = 0;
	u8 inst = 0;
	struct sclr_core_cfg sc_cfg;
	struct sclr_odma_cfg odma_cfg;
	struct sclr_border_cfg border_cfg;

	sc_cfg.sc_bypass = false;
	sc_cfg.gop_bypass = false;
	sc_cfg.cir_bypass = true;
	sc_cfg.odma_bypass = false;

	odma_cfg.flip = SCL_FLIP_NO;
	odma_cfg.burst = g_odma_cfg[inst].burst;
	odma_cfg.fmt = rgnex_cfg->fmt;
	odma_cfg.csc_cfg.csc_type = rgnex_cfg->dst_csc;
	odma_cfg.mem.addr0 = rgnex_cfg->addr0;
	odma_cfg.mem.addr1 = rgnex_cfg->addr1;
	odma_cfg.mem.addr2 = rgnex_cfg->addr2;
	odma_cfg.mem.pitch_y = rgnex_cfg->bytesperline[0];
	odma_cfg.mem.pitch_c = rgnex_cfg->bytesperline[1];

	border_cfg = g_bd_cfg[inst];
	border_cfg.cfg.b.enable = 0;

	// sc core
	tmp = 0;
	if (sc_cfg.sc_bypass)
		tmp |= 0x02;
	if (sc_cfg.gop_bypass)
		tmp |= 0x04;
	if (sc_cfg.cir_bypass)
		tmp |= 0x20;
	if (sc_cfg.odma_bypass)
		tmp |= 0x40;

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CFG(inst), tmp);

	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SC_INI_PH(inst), 0);

	// sc odma
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_CFG(inst),
			 (odma_cfg.flip << 16) |  (odma_cfg.fmt << 8) |
			 odma_cfg.burst);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_PITCH_Y(inst),
			 odma_cfg.mem.pitch_y);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_PITCH_C(inst),
			 odma_cfg.mem.pitch_c);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR0_L(inst),
			 odma_cfg.mem.addr0);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR0_H(inst),
			 odma_cfg.mem.addr0 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR1_L(inst),
			 odma_cfg.mem.addr1);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR1_H(inst),
			 odma_cfg.mem.addr1 >> 32);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR2_L(inst),
			 odma_cfg.mem.addr2);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_ADDR2_H(inst),
			 odma_cfg.mem.addr2 >> 32);

	// sc csc
	if (odma_cfg.csc_cfg.csc_type == SCL_CSC_NONE) {
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_EN(inst), 0);
	} else {
		struct sclr_csc_matrix *cfg = &csc_mtrx[odma_cfg.csc_cfg.csc_type];

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_EN(inst), 1);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF0(inst),
				 (cfg->coef[0][1] << 16) | cfg->coef[0][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF1(inst),
				 (cfg->coef[1][0] << 16) | cfg->coef[0][2]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF2(inst),
				 (cfg->coef[1][2] << 16) | cfg->coef[1][1]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF3(inst),
				 (cfg->coef[2][1] << 16) | cfg->coef[2][0]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_COEF4(inst),
				 cfg->coef[2][2]);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_OFFSET(inst),
				 (cfg->add[2] << 16) | (cfg->add[1] << 8) |
				 cfg->add[0]);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_FRAC0(inst), 0);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CSC_FRAC1(inst), 0);
	}

	// sc border
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_BORDER_CFG(inst), border_cfg.cfg.raw);
	return cmd_idx;
}

/**
 * sclr_engine_cmdq_rgnex - trigger sc by cmdq for rgn_ex task
 *
 * @param cfgs: settings for cmdq
 * @param cnt: the number of settings
 * @param cmdq_addr: memory-address to put cmdq
 */
void sclr_engine_cmdq_rgnex(struct sclr_rgnex_cfg *cfgs, u8 cnt, uintptr_t cmdq_addr)
{
	u8 i = 0;
	u8 layer = 0;
	u16 cmd_idx = 0;
	union cmdq_set *cmd_start = (union cmdq_set *)cmdq_addr;
	u32 tmp = 0;
	u8 cmdq_intr_status;
	struct sclr_rgnex_cfg *cfg = cfgs;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg ow_cfg;
	static const u8 reg_map_fmt[SCL_GOP_FMT_MAX] = {0, 0x4, 0x5, 0x8, 0xc};

	memset((void *)cmdq_addr, 0, 0x2000);

	_reg_write(reg_base + REG_SCL_TOP_CMDQ_START, 0x18000);
	_reg_write(reg_base + REG_SCL_TOP_CMDQ_STOP, 0x18000);

	// set common part for each rgn_ex
	cmd_idx = _map_rgnex_img_to_cmdq(cfg, cmd_start, cmd_idx);
	cmd_idx = _map_rgnex_sc_to_cmdq(cfg, 0, cmd_start, cmd_idx);

	for (i = 0; i < cnt; ++i) {
		struct sclr_rgnex_cfg *cfg = (cfgs + i);
		u8 img_inst = SCL_IMG_D;
		u8 inst = 0;
		u8 ow_inst = 0;

		// set different part for each rgn_ex
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_OFFSET(img_inst),
			 (cfg->rgnex_rect.y << 16) | cfg->rgnex_rect.x);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_IMG_SIZE(img_inst),
			 ((cfg->rgnex_rect.h - 1) << 16) |
			 (cfg->rgnex_rect.w - 1));

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SRC_SIZE(inst),
			 ((cfg->rgnex_rect.h - 1) << 12) | (cfg->rgnex_rect.w - 1));
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CROP_OFFSET(inst), 0);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_CROP_SIZE(inst),
			 ((cfg->rgnex_rect.h - 1) << 16) | (cfg->rgnex_rect.w - 1));
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_OUT_SIZE(inst),
			 ((cfg->rgnex_rect.h - 1) << 16) | (cfg->rgnex_rect.w - 1));

		tmp = (((cfg->rgnex_rect.w - 1) << 13) + (cfg->rgnex_rect.w >> 1))
			/ MAX((cfg->rgnex_rect.w - 1), 1);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SC_H_CFG(inst), tmp << 8);

		tmp = (((cfg->rgnex_rect.h - 1) << 13) + (cfg->rgnex_rect.h >> 1))
			/ MAX((cfg->rgnex_rect.h - 1), 1);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_SC_V_CFG(inst), tmp << 8);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_OFFSET_X(inst),
			 cfg->rgnex_rect.x);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_OFFSET_Y(inst),
			 cfg->rgnex_rect.y);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_WIDTH(inst),
			 cfg->rgnex_rect.w - 1);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_ODMA_HEIGHT(inst),
			 cfg->rgnex_rect.h - 1);

		// sc gop 0
		// TODO: sc gop 1
		gop_cfg = cfg->gop_cfg;
		ow_cfg = gop_cfg.ow_cfg[ow_inst];
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_FMT(inst, ow_inst),
				 reg_map_fmt[ow_cfg.fmt]);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_H_RANGE(inst, ow_inst),
				 (ow_cfg.end.x << 16) | ow_cfg.start.x);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_V_RANGE(inst, ow_inst),
				 (ow_cfg.end.y << 16) | ow_cfg.start.y);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_ADDR_L(inst, ow_inst),
				 ow_cfg.addr);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_ADDR_H(inst, ow_inst),
				 ow_cfg.addr >> 32);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_CROP_PITCH(inst, ow_inst),
				 ow_cfg.pitch);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_SIZE(inst, ow_inst),
				 (ow_cfg.mem_size.h << 16) | ow_cfg.mem_size.w);

		gop_cfg.gop_ctrl.b.burst = g_gop_cfg[inst][layer].gop_ctrl.b.burst;
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_CFG(inst),
				 gop_cfg.gop_ctrl.raw & 0xffff);
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_FONTCOLOR(inst),
				 (gop_cfg.font_fg_color << 16) | gop_cfg.font_bg_color);
		if (gop_cfg.gop_ctrl.b.colorkey_en)
			cmdQ_set_package(&cmd_start[cmd_idx++].reg,
					 REG_SCL_CMDQ_TOP_BASE + REG_SCL_GOP0_COLORKEY(inst),
					 gop_cfg.colorkey);

		img_inst = (~img_inst) & 0x01;
		// modify & clear sc-cmdq's stop flag
		tmp = 0x10000 | BIT(img_inst+1);
		if (inst < SCL_MAX_INST)
			tmp |= BIT(inst + 3);

		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_CMDQ_STOP, tmp);

		// start sclr
		cmdQ_set_package(&cmd_start[cmd_idx++].reg,
				 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_IMG_CTRL,
				 BIT(img_inst));

		// wait end-inter(1)
		cmdQ_set_wait(&cmd_start[cmd_idx++], false, 1, 0);
	}
	// clear intr
	cmdQ_set_package(&cmd_start[cmd_idx++].reg,
			 REG_SCL_CMDQ_TOP_BASE + REG_SCL_TOP_INTR_STATUS,
			 0x1000884C);

	cmdq_intr_status = sclr_cmdq_intr_status();
	if (cmdq_intr_status)
		sclr_cmdq_intr_clr(cmdq_intr_status);

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device((phys_addr_t)cmdq_addr, 0x2000, DMA_TO_DEVICE);
#else
	__dma_map_area((void *)cmdq_addr, 0x2000, DMA_TO_DEVICE);
#endif

	cmdQ_intr_ctrl(reg_base + REG_SCL_CMDQ_BASE, 0x02);
	cmdQ_engine(reg_base + REG_SCL_CMDQ_BASE, (uintptr_t)virt_to_phys((void *)cmdq_addr),
		    REG_SCL_CMDQ_TOP_BASE >> 22, true, false, cmd_idx);
}
#endif


#if defined(CONFIG_TILE_MODE)
/**
 * sclr_tile_cal_size - calculate parameters for left/right tile
 *
 * @param inst: (0~3), the instance of sc
 */
u8 sclr_tile_cal_size(u8 inst, bool is_online_from_isp, struct sc_cfg_cb *post_para)
{
#ifndef TILE_ON_IMG
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u32 out_l_width = (out_size.w >> 1) & ~0x01; // make sure op on even pixels.
	u32 h_sc_fac = (((crop_size.w - 1) << 13) + (out_size.w >> 1))
		       / (out_size.w - 1);
	u32 L_last_phase, R_first_phase;
	u16 L_last_pixel, R_first_pixel;
	u8 mode = SCL_TILE_BOTH;

	L_last_phase = (out_l_width - 1) * h_sc_fac;
	L_last_pixel = (L_last_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.src_l_width = L_last_pixel + 2
				+ ((L_last_phase & 0x1fff) ? 1 : 0);
	cfg->tile.out_l_width = out_l_width;

	// right tile no mirror
	R_first_phase = L_last_phase + h_sc_fac;
	R_first_pixel = (R_first_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
	cfg->tile.r_ini_phase = R_first_phase & 0x1fff;
	cfg->tile.src_r_offset = R_first_pixel - 1;
	cfg->tile.src_r_width = crop_size.w - cfg->tile.src_r_offset;
#else
	u16 src_l_last_pixel_max = (is_online_from_isp)
				 ? post_para->ol_tile_cfg.l_out.end
				 : (g_sc_cfg[inst].sc.src.w >> 1) - 1;
	struct sclr_scale_cfg *cfg = &g_sc_cfg[inst].sc;
	struct sclr_size crop_size = { .w = cfg->crop.w, .h = cfg->crop.h };
	struct sclr_size out_size = cfg->dst;
	u32 out_l_width = 0;// = (out_size.w >> 1) & ~0x01; // make sure op on even pixels.
	u32 h_sc_fac = (((crop_size.w - 1) << 13) + (out_size.w >> 1))
		       / (out_size.w - 1);
	u32 L_last_phase = 0, R_first_phase = 0;
	u16 L_last_pixel = 0, R_first_pixel = 0;
	u8 mode = SCL_TILE_BOTH;

	pr_debug("%s: on sc(%d)\n", __func__, inst);
	pr_debug("width: src(%d), crop(%d), dst(%d)\n", g_sc_cfg[inst].sc.src.w, crop_size.w, out_size.w);

	if (cfg->crop.x > src_l_last_pixel_max) {
		// do nothing on left tile if crop out-of-range.
		cfg->tile.src_l_width = 0;
		cfg->tile.out_l_width = 0;
		cfg->tile.r_ini_phase = 0;
		cfg->tile.src_r_offset = 0;
		cfg->tile.src_r_width = crop_size.w;
		mode = SCL_TILE_RIGHT;
	} else {
		// find available out_l_width
		if (cfg->crop.x + cfg->crop.w - 1 <= src_l_last_pixel_max) {
			out_l_width = out_size.w; // make sure op on even pixels.
			L_last_phase = (out_l_width - 1) * h_sc_fac;
			L_last_pixel = (L_last_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.src_l_width = L_last_pixel + 2
						+ ((L_last_phase & 0x1fff) ? 1 : 0);
			cfg->tile.out_l_width = out_l_width;

			// do nothing on right tile if crop out-of-range.
			cfg->tile.r_ini_phase = 0;
			cfg->tile.src_r_offset = 0;
			cfg->tile.src_r_width = 0;
			mode = SCL_TILE_LEFT;
		} else {
			out_l_width = (((src_l_last_pixel_max - cfg->crop.x - ((cfg->mir_enable) ? 0 : 1)) << 13) /
				       h_sc_fac + 1) & ~0x01;
			L_last_phase = (out_l_width - 1) * h_sc_fac;
			L_last_pixel = (L_last_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.src_l_width = L_last_pixel + 2
						+ ((L_last_phase & 0x1fff) ? 1 : 0);
			cfg->tile.out_l_width = out_l_width;

			// right tile no mirror
			R_first_phase = L_last_phase + h_sc_fac;
			R_first_pixel = (R_first_phase >> 13) + ((cfg->mir_enable) ? 0 : 1);
			cfg->tile.r_ini_phase = R_first_phase & 0x1fff;
			cfg->tile.src_r_offset = R_first_pixel - 1;
			cfg->tile.src_r_width = crop_size.w - cfg->tile.src_r_offset;
			mode = SCL_TILE_BOTH;
		}
	}
#endif

	cfg->tile.src = crop_size;
	cfg->tile.out = out_size;
	cfg->tile.border_enable = g_bd_cfg[inst].cfg.b.enable;
	if (g_bd_cfg[inst].cfg.b.enable) {
		// if border, then only on left tile enabled to fill bgcolor.
		cfg->tile.dma_l_x = g_bd_cfg[inst].start.x & ~0x01;
		cfg->tile.dma_l_y = g_bd_cfg[inst].start.y;
		cfg->tile.dma_r_x = cfg->tile.dma_l_x + out_l_width;
		cfg->tile.dma_r_y = g_bd_cfg[inst].start.y;
		cfg->tile.dma_l_width = g_odma_cfg[inst].mem.width;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->tile.dma_r_x
				= g_odma_cfg[inst].frame_size.w - out_size.w - (g_bd_cfg[inst].start.x & ~0x01);
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP))
			cfg->tile.dma_r_y = g_odma_cfg[inst].frame_size.h - out_size.h - g_bd_cfg[inst].start.y;
	} else {
		cfg->tile.dma_l_x = g_odma_cfg[inst].mem.start_x;
		cfg->tile.dma_l_y = g_odma_cfg[inst].mem.start_y;
		cfg->tile.dma_r_x = g_odma_cfg[inst].mem.start_x + out_l_width;
		cfg->tile.dma_r_y = g_odma_cfg[inst].mem.start_y;
		cfg->tile.dma_l_width = out_l_width;
		if ((g_odma_cfg[inst].flip == SCL_FLIP_HFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->tile.dma_l_x = g_odma_cfg[inst].frame_size.w - out_l_width - g_odma_cfg[inst].mem.start_x;
			cfg->tile.dma_r_x = g_odma_cfg[inst].frame_size.w - out_size.w - g_odma_cfg[inst].mem.start_x;
		}
		if ((g_odma_cfg[inst].flip == SCL_FLIP_VFLIP) || (g_odma_cfg[inst].flip == SCL_FLIP_HVFLIP)) {
			cfg->tile.dma_l_y = g_odma_cfg[inst].frame_size.h - out_size.h - g_odma_cfg[inst].mem.start_y;
			cfg->tile.dma_r_y = cfg->tile.dma_l_y;
		}
	}

	pr_debug("h_sc_fac:%d\n", h_sc_fac);
	pr_debug("L last phase(%d) last pixel(%d)\n", L_last_phase, L_last_pixel);
	pr_debug("R first phase(%d) first pixel(%d)\n", R_first_phase, R_first_pixel);
	pr_debug("tile cfg: Left src width(%d) out(%d)\n", cfg->tile.src_l_width, cfg->tile.out_l_width);
	pr_debug("tile cfg: Right src offset(%d) width(%d) phase(%d)\n"
		, cfg->tile.src_r_offset, cfg->tile.src_r_width, cfg->tile.r_ini_phase);
	pr_debug("tile cfg: odma left x(%d) y(%d) width(%d)\n"
		, cfg->tile.dma_l_x, cfg->tile.dma_l_y, cfg->tile.dma_l_width);
	pr_debug("tile cfg: odma right x(%d) y(%d)\n"
		, cfg->tile.dma_r_x, cfg->tile.dma_r_y);
	return mode;
}

/**
 * _gop_tile_shift - update tile for gop
 *
 * @param gop_ow_cfg: gop windows cfg which will be referenced and updated
 * @param left_width: width of gop on left tile
 * @param is_right_tile: true for right tile; false for left tile
 */
static void _gop_tile_shift(struct sclr_gop_ow_cfg *gop_ow_cfg, u16 left_width, u16 total_width, bool is_right_tile)
{
	s16 byte_offset;
	s8 pixel_offset = 0;
	u8 bpp = (gop_ow_cfg->fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		 (gop_ow_cfg->fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;

	// workaround for dma-address 32 alignment
	byte_offset = (bpp * (left_width - gop_ow_cfg->start.x)) & (GOP_ALIGNMENT - 1);
	if (byte_offset) {
		if (byte_offset <= (GOP_ALIGNMENT - byte_offset)) {
			pixel_offset = (byte_offset / bpp);
			// use another side if out-of-window
			if ((gop_ow_cfg->end.x + pixel_offset) >= total_width)
				pixel_offset = -(GOP_ALIGNMENT - byte_offset) / bpp;
		} else {
			pixel_offset = -(GOP_ALIGNMENT - byte_offset) / bpp;
			// use another side if out-of-window
			if ((gop_ow_cfg->start.x + pixel_offset) < 0)
				pixel_offset = (byte_offset / bpp);
		}

		gop_ow_cfg->start.x += pixel_offset;
		gop_ow_cfg->end.x += pixel_offset;
		pr_debug("gop tile wa: byte_offset(%d) pixoffset(%d)\n", byte_offset, pixel_offset);
	}

	if (is_right_tile) {
		gop_ow_cfg->addr += bpp * (left_width - gop_ow_cfg->start.x);
		gop_ow_cfg->start.x = 0;
		gop_ow_cfg->end.x -= left_width;
		gop_ow_cfg->img_size.w = gop_ow_cfg->end.x;
		gop_ow_cfg->mem_size.w = ALIGN(gop_ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
	} else {
		gop_ow_cfg->img_size.w = left_width - gop_ow_cfg->start.x;
		gop_ow_cfg->end.x = left_width;
		gop_ow_cfg->mem_size.w = ALIGN(gop_ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
	}
	pr_debug("gop_tile(%s): addr(%#llx), startx(%d), endx(%d) img_width(%d) mem_width(%d)\n",
		is_right_tile ? "right" : "left",
		gop_ow_cfg->addr, gop_ow_cfg->start.x, gop_ow_cfg->end.x,
		gop_ow_cfg->img_size.w, gop_ow_cfg->mem_size.w);
}

/**
 * sclr_left_tile - update parameters for left tile
 *
 * @param inst: (0~3), the instance of sc
 * @param src_l_w: width of left tile from img
 * @return: true if success; false if no need or something wrong
 */
bool sclr_left_tile(u8 inst, u16 src_l_w)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_top_cfg *top_cfg;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	int i = 0;
	u8 layer;

	if (inst >= SCL_MAX_INST) {
		pr_err("[cvi-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_cont("for the requirement(%d)\n", inst);
		return false;
	}

	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);
	top_cfg = sclr_top_get_cfg();

	sc->tile_enable = true;

	// skip if crop in the right tile.
	if (sc->tile.src_l_width == 0) {
		top_cfg->sclr_enable[inst] = false;
		sclr_top_set_cfg(top_cfg);
		return false;
	}
	top_cfg->sclr_enable[inst] = true;
	sclr_top_set_cfg(top_cfg);

	src.w = src_l_w;
	src.h = sc->src.h;
	crop.x = sc->crop.x;
	crop.y = sc->crop.y;
	crop.w = sc->tile.src_l_width;
	crop.h = sc->tile.src.h;
	dst.w = sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_input_size(inst, src, false);
	sclr_set_crop(inst, crop, false);
	sclr_set_output_size(inst, dst);
	sclr_set_scale_mode(inst, sc->mir_enable, false, false);
	sclr_set_scale_phase(inst, 0, 0);

	if (sc->tile.border_enable) {
		odma_cfg->mem.start_x = 0;
		odma_cfg->mem.start_y = 0;
		odma_cfg->mem.width = sc->tile.dma_l_width;
		sclr_odma_set_mem(inst, &odma_cfg->mem);

		g_bd_cfg[inst].cfg.b.enable = true;
		g_bd_cfg[inst].start.x = sc->tile.dma_l_x;
		g_bd_cfg[inst].start.y = sc->tile.dma_l_y;
		sclr_border_set_cfg(inst, &g_bd_cfg[inst]);
	} else {
		odma_cfg->mem.start_x = sc->tile.dma_l_x;
		odma_cfg->mem.start_y = sc->tile.dma_l_y;
		odma_cfg->mem.width = sc->tile.dma_l_width;
		sclr_odma_set_mem(inst, &odma_cfg->mem);
	}
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	//layer 0 cfg by gop0_cfg
	layer = 0;
	gop_cfg = *sclr_gop_get_cfg(inst, layer);
	for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i)
		if (gop_cfg.gop_ctrl.raw & BIT(i)) {
			gop_ow_cfg = gop_cfg.ow_cfg[i];
			pr_debug("gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
				i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

			// gop-window on right tile: pass
			if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
				gop_cfg.gop_ctrl.raw &= ~BIT(i);
				continue;
			}
			// gop-window on left tile: ok to go
			if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
				sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
				continue;
			}

			// gop-window on both tile: workaround
			_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, false);
			sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
		}
	pr_debug("gop_cfg of layer %d:%#x\n", layer, gop_cfg.gop_ctrl.raw);
	sclr_gop_set_cfg(inst, layer, &gop_cfg, false);

	//layer 1 cfg by gop1_cfg
	layer = 1;
	gop_cfg = *sclr_gop_get_cfg(inst, layer);
	for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i)
		if (gop_cfg.gop_ctrl.raw & BIT(i)) {
			gop_ow_cfg = gop_cfg.ow_cfg[i];
			pr_debug("gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
			         i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

			// gop-window on right tile: pass
			if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
				gop_cfg.gop_ctrl.raw &= ~BIT(i);
				continue;
			}
			// gop-window on left tile: ok to go
			if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
				sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
				continue;
			}

			// gop-window on both tile: workaround
			_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, false);
			sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
		}
	pr_debug("gop_cfg of layer %d:%#x\n", layer, gop_cfg.gop_ctrl.raw);
	sclr_gop_set_cfg(inst, layer, &gop_cfg, false);

	sclr_reg_force_up(inst);
	return true;
}

/**
 * sclr_right_tile - update parameters for right tile
 *
 * @param inst: (0~3), the instance of sc
 * @param src_offset: offset of the right tile relative to original image
 * @return: true if success; false if no need or something wrong
 */
bool sclr_right_tile(u8 inst, u16 src_offset)
{
	struct sclr_scale_cfg *sc;
	struct sclr_odma_cfg *odma_cfg;
	struct sclr_rect crop;
	struct sclr_size src, dst;
	struct sclr_top_cfg *top_cfg;
	struct sclr_gop_cfg gop_cfg;
	struct sclr_gop_ow_cfg gop_ow_cfg;
	int i = 0;
	u8 layer;

	if (inst >= SCL_MAX_INST) {
		pr_err("[cvi-vip][sc] %s: no enough sclr-instance ", __func__);
		pr_cont("for the requirement(%d)\n", inst);
		return false;
	}
	sc = &(sclr_get_cfg(inst)->sc);
	odma_cfg = sclr_odma_get_cfg(inst);
	top_cfg = sclr_top_get_cfg();

	// skip if crop in the right tile.
	if (sc->tile.src_r_width == 0) {
		top_cfg->sclr_enable[inst] = false;
		sclr_top_set_cfg(top_cfg);
		sc->tile_enable = false;
		return false;
	}
	top_cfg->sclr_enable[inst] = true;
	sclr_top_set_cfg(top_cfg);

	src.w = sc->src.w - src_offset;
	src.h = sc->src.h;
	crop.x = sc->crop.x + sc->tile.src_r_offset - src_offset;
	crop.y = sc->crop.y;
	crop.w = sc->tile.src_r_width;
	crop.h = sc->tile.src.h;
	dst.w = sc->tile.out.w - sc->tile.out_l_width;
	dst.h = sc->tile.out.h;

	sclr_set_input_size(inst, src, false);
	sclr_set_crop(inst, crop, false);
	sclr_set_output_size(inst, dst);
	sclr_set_scale_mode(inst, false, false, false);
	sclr_set_scale_phase(inst, sc->tile.r_ini_phase, 0);

	odma_cfg->mem.start_x = sc->tile.dma_r_x;
	odma_cfg->mem.start_y = sc->tile.dma_r_y;
	odma_cfg->mem.width = dst.w;
	odma_cfg->mem.height = dst.h;
	sclr_odma_set_mem(inst, &odma_cfg->mem);

	// right tile don't do border.
	g_bd_cfg[inst].cfg.b.enable = false;
	sclr_border_set_cfg(inst, &g_bd_cfg[inst]);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d input size: w=%d h=%d\n", inst, src.w, src.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d crop size: x=%d y=%d w=%d h=%d\n", inst, crop.x, crop.y, crop.w, crop.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d output size: w=%d h=%d\n", inst, dst.w, dst.h);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc%d odma mem: x=%d y=%d w=%d h=%d pitch_y=%d pitch_c=%d\n", inst,
		odma_cfg->mem.start_x, odma_cfg->mem.start_y, odma_cfg->mem.width,
		odma_cfg->mem.height, odma_cfg->mem.pitch_y, odma_cfg->mem.pitch_c);

	//check gop0 - layer 0 of gop
	layer = 0;
	gop_cfg = *sclr_gop_get_cfg(inst, layer);
	for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i)
		if (gop_cfg.gop_ctrl.raw & BIT(i)) {
			gop_ow_cfg = gop_cfg.ow_cfg[i];
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
			        i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

			// gop-window on left tile: pass
			if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
				gop_cfg.gop_ctrl.raw &= ~BIT(i);
				continue;
			}
			// gop-window on right tile: ok to go
			if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
				gop_ow_cfg.start.x -= sc->tile.out_l_width;
				gop_ow_cfg.end.x -= sc->tile.out_l_width;
				sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
				continue;
			}

			// gop-window on both tile: workaround
			_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, true);
			sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
		}
	pr_debug("gop_cfg of layer %d:%#x\n", layer, gop_cfg.gop_ctrl.raw);
	sclr_gop_set_cfg(inst, layer, &gop_cfg, false);

	//check gop1 - layer 1 of gop
	layer = 1;
	gop_cfg = *sclr_gop_get_cfg(inst, layer);
	for (i = 0; i < SCL_MAX_GOP_OW_INST; ++i)
		if (gop_cfg.gop_ctrl.raw & BIT(i)) {
			gop_ow_cfg = gop_cfg.ow_cfg[i];
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "gop(%d) startx(%d) endx(%d), tile_l_width(%d)\n",
				i, gop_ow_cfg.start.x, gop_ow_cfg.end.x, sc->tile.out_l_width);

			// gop-window on left tile: pass
			if (gop_ow_cfg.end.x <= sc->tile.out_l_width) {
				gop_cfg.gop_ctrl.raw &= ~BIT(i);
				continue;
			}
			// gop-window on right tile: ok to go
			if (gop_ow_cfg.start.x >= sc->tile.out_l_width) {
				gop_ow_cfg.start.x -= sc->tile.out_l_width;
				gop_ow_cfg.end.x -= sc->tile.out_l_width;
				sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
				continue;
			}

			// gop-window on both tile: workaround
			_gop_tile_shift(&gop_ow_cfg, sc->tile.out_l_width, sc->tile.out.w, true);
			sclr_gop_ow_set_cfg(inst, layer, i, &gop_ow_cfg, false);
		}
	pr_debug("gop_cfg of layer %d:%#x\n", layer, gop_cfg.gop_ctrl.raw);
	sclr_gop_set_cfg(inst, layer, &gop_cfg, false);
	sclr_reg_force_up(inst);

	sc->tile_enable = false;
	return true;
}
#endif


#if defined(CONFIG_REG_DUMP)
void sclr_dump_top_register(void)
{
	u32 val, rdy;
	uintptr_t top_base = reg_base + REG_SCL_TOP_BASE;
	//void *addr = (void *)(PAGE_MASK & top_base);
	//struct vm_struct *vm;

	//vm = find_vm_area(addr);

	//if (vm)
	//	CVI_TRACE_VPSS(CVI_DBG_INFO, "top base address: va(0x%08x), pa(0x%08llx)\n",
	//		top_base, (unsigned long long)vm->phys_addr);
	//else
	//	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "top base address\n");

	//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "top base address: va(0x%08x)\n", top_base);

	val = _reg_read(top_base + SC_TOP_REG_00);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_00=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    ip_trig_src=%d, force_clk_enable=%d\n",
		(val & SC_TOP_REG_IP_TRIG_SRC_MASK) >> SC_TOP_REG_IP_TRIG_SRC_OFFSET,
		(val & SC_TOP_REG_FORCE_CLK_ENABLE_MASK) >> SC_TOP_REG_FORCE_CLK_ENABLE_OFFSET);

	val = _reg_read(top_base + SC_TOP_REG_01);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_01=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_d_en=%d, sc_v1_en=%d, sc_v2_en=%d, sc_v3_en=%d\n",
		(val & SC_TOP_REG_SC_D_EN_MASK) >> SC_TOP_REG_SC_D_EN_OFFSET,
		(val & SC_TOP_REG_SC_V1_EN_MASK) >> SC_TOP_REG_SC_V1_EN_OFFSET,
		(val & SC_TOP_REG_SC_V2_EN_MASK) >> SC_TOP_REG_SC_V2_EN_OFFSET,
		(val & SC_TOP_REG_SC_V3_EN_MASK) >> SC_TOP_REG_SC_V3_EN_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    disp_en=%d, disp_src_sel=%d, sc_d_src_sel=%d\n",
		(val & SC_TOP_REG_DISP_EN_MASK) >> SC_TOP_REG_DISP_EN_OFFSET,
		(val & SC_TOP_REG_DISP_SRC_SEL_MASK) >> SC_TOP_REG_DISP_SRC_SEL_OFFSET,
		(val & SC_TOP_REG_SC_D_SRC_SEL_MASK) >> SC_TOP_REG_SC_D_SRC_SEL_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_rot_sw_rst=%d, sc_debug_en=%d, qos_en=%d\n",
		(val & SC_TOP_REG_SC_ROT_SW_RST_MASK) >> SC_TOP_REG_SC_ROT_SW_RST_OFFSET,
		(val & SC_TOP_REG_SC_DEBUG_EN_MASK) >> SC_TOP_REG_SC_DEBUG_EN_OFFSET,
		(val & SC_TOP_REG_QOS_EN_MASK) >> SC_TOP_REG_QOS_EN_OFFSET);

	val = _reg_read(top_base + SC_TOP_REG_02);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_02=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    img_d_sel=%d\n",
		(val & SC_TOP_REG_IMG_D_SEL_MASK) >> SC_TOP_REG_IMG_D_SEL_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_03=0x%08x\n", _reg_read(top_base + SC_TOP_REG_03));

	val = _reg_read(top_base + SC_TOP_REG_04);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_04=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sw_force_up_b0=%d, sw_force_up_b1=%d, sw_force_up_b2=%d\n",
		(val & SC_TOP_REG_SW_FORCE_UP_B0_MASK) >> SC_TOP_REG_SW_FORCE_UP_B0_OFFSET,
		(val & SC_TOP_REG_SW_FORCE_UP_B1_MASK) >> SC_TOP_REG_SW_FORCE_UP_B1_OFFSET,
		(val & SC_TOP_REG_SW_FORCE_UP_B2_MASK) >> SC_TOP_REG_SW_FORCE_UP_B2_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sw_force_up_b3=%d, sw_force_up_b4=%d, sw_force_up_b5=%d\n",
		(val & SC_TOP_REG_SW_FORCE_UP_B3_MASK) >> SC_TOP_REG_SW_FORCE_UP_B3_OFFSET,
		(val & SC_TOP_REG_SW_FORCE_UP_B4_MASK) >> SC_TOP_REG_SW_FORCE_UP_B4_OFFSET,
		(val & SC_TOP_REG_SW_FORCE_UP_B5_MASK) >> SC_TOP_REG_SW_FORCE_UP_B5_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sw_force_up_b6=%d, sw_force_up_b7=%d, sw_mask_up=%d, shrd_sel=%d\n",
		(val & SC_TOP_REG_SW_FORCE_UP_B6_MASK) >> SC_TOP_REG_SW_FORCE_UP_B6_OFFSET,
		(val & SC_TOP_REG_SW_FORCE_UP_B7_MASK) >> SC_TOP_REG_SW_FORCE_UP_B7_OFFSET,
		(val & SC_TOP_REG_SW_MASK_UP_MASK) >> SC_TOP_REG_SW_MASK_UP_OFFSET,
		(val & SC_TOP_REG_SHRD_SEL_MASK) >> SC_TOP_REG_SHRD_SEL_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_05=0x%08x\n", _reg_read(top_base + SC_TOP_REG_05));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_06=0x%08x\n", _reg_read(top_base + SC_TOP_REG_06));

	val = _reg_read(top_base + SC_TOP_REG_SB_CBAR);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_SB_CBAR=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sb_wr_ctrl0=%d, sb_wr_ctrl1=%d, sb_rd_ctrl=%d\n",
		(val & SC_TOP_REG_SB_WR_CTRL0_MASK) >> SC_TOP_REG_SB_WR_CTRL0_OFFSET,
		(val & SC_TOP_REG_SB_WR_CTRL1_MASK) >> SC_TOP_REG_SB_WR_CTRL1_OFFSET,
		(val & SC_TOP_REG_SB_RD_CTRL_MASK) >> SC_TOP_REG_SB_RD_CTRL_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_08=0x%08x\n", _reg_read(top_base + SC_TOP_REG_08));

	val = _reg_read(top_base + SC_TOP_REG_0C);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_0C=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    int_mask=0x%08x\n", val);

	val = _reg_read(top_base + SC_TOP_REG_0D);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_0D=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    int_status=0x%08x\n", val);

	val = _reg_read(top_base + SC_TOP_REG_0E);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_0E=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    int_enable=0x%08x\n", val);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_0F=0x%08x\n", _reg_read(top_base + SC_TOP_REG_0F));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_10=0x%08x\n", _reg_read(top_base + SC_TOP_REG_10));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_11=0x%08x\n", _reg_read(top_base + SC_TOP_REG_11));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_12=0x%08x\n", _reg_read(top_base + SC_TOP_REG_12));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_13=0x%08x\n", _reg_read(top_base + SC_TOP_REG_13));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_LVDSTX=0x%08x\n", _reg_read(top_base + SC_TOP_REG_LVDSTX));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_BT_ENC_0=0x%08x\n", _reg_read(top_base + SC_TOP_REG_BT_ENC_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_RGG_BT_SYNC_CODE=0x%08x\n",
			_reg_read(top_base + SC_TOP_RGG_BT_SYNC_CODE));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_BK_BLK_DATA=0x%08x\n", _reg_read(top_base + SC_TOP_REG_BK_BLK_DATA));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_SC_074=0x%08x\n", _reg_read(top_base + SC_TOP_REG_SC_074));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_SC_078=0x%08x\n", _reg_read(top_base + SC_TOP_REG_SC_078));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_EXT=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_EXT));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_0=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_1=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_2=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_3=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_4=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_5=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_6=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_6));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_7=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_7));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_8=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_8));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_9=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_9));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_VO_MUX_A=0x%08x\n", _reg_read(top_base + SC_TOP_REG_VO_MUX_A));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_RMX_CTRL=0x%08x\n", _reg_read(top_base + SC_TOP_REG_RMX_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_FAB_LP=0x%08x\n", _reg_read(top_base + SC_TOP_REG_FAB_LP));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_CLK_EN=0x%08x\n", _reg_read(top_base + SC_TOP_REG_CLK_EN));

	val = _reg_read(top_base + SC_TOP_REG_DBG_VALID);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_DBG_VALID=0x%08x\n", val);
	rdy = _reg_read(top_base + SC_TOP_REG_DBG_READY);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_DBG_READY=0x%08x\n", rdy);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "               valid     ready\n");
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "isp2ip_y in       %d        %d\n",
		val & 1, rdy & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "isp2ip_u in       %d        %d\n",
		(val >> 1) & 1, (rdy >> 1) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "isp2ip_v in       %d        %d\n",
		(val >> 2) & 1, (rdy >> 2) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img_d out         %d        %d\n",
		(val >> 3) & 1, (rdy >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img_v out         %d        %d\n",
		(val >> 4) & 1, (rdy >> 4) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bld_sa(img_v)     %d        %d\n",
		(val >> 5) & 1, (rdy >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bld_sb(img_d)     %d        %d\n",
		(val >> 6) & 1, (rdy >> 6) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bld_m             %d        %d\n",
		(val >> 7) & 1, (rdy >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "pri_sp            %d        %d\n",
		(val >> 8) & 1, (rdy >> 8) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "pri_m             %d        %d\n",
		(val >> 9) & 1, (rdy >> 9) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_d              %d        %d\n",
		(val >> 10) & 1, (rdy >> 10) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_v1             %d        %d\n",
		(val >> 11) & 1, (rdy >> 11) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_v2             %d        %d\n",
		(val >> 12) & 1, (rdy >> 12) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_v3             %d        %d\n",
		(val >> 13) & 1, (rdy >> 13) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_d_out          %d        %d\n",
		(val >> 14) & 1, (rdy >> 14) & 1);

	val = _reg_read(top_base + SC_TOP_REG_BLD_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_BLD_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    bld_en=%d, bld_fix_alpha=%d, bld_blend_y_only=%d\n",
		(val & SC_TOP_REG_BLD_EN_MASK) >> SC_TOP_REG_BLD_EN_OFFSET,
		(val & SC_TOP_REG_BLD_FIX_ALPHA_MASK) >> SC_TOP_REG_BLD_FIX_ALPHA_OFFSET,
		(val & SC_TOP_REG_BLD_BLEND_Y_ONLY_MASK) >> SC_TOP_REG_BLD_BLEND_Y_ONLY_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    bld_y2r_enable=%d, bld_alpha_factor=%d\n",
		(val & SC_TOP_REG_BLD_Y2R_ENABLE_MASK) >> SC_TOP_REG_BLD_Y2R_ENABLE_OFFSET,
		(val & SC_TOP_REG_BLD_ALPHA_FACTOR_MASK) >> SC_TOP_REG_BLD_ALPHA_FACTOR_OFFSET);

	val = _reg_read(top_base + SC_TOP_REG_BLD_SIZE);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_REG_BLD_SIZE=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    bld_alpha_stp=%d, bld_wd=%d\n",
		(val & SC_TOP_REG_BLD_ALPHA_STP_MASK) >> SC_TOP_REG_BLD_ALPHA_STP_OFFSET,
		(val & SC_TOP_REG_BLD_WD_MASK) >> SC_TOP_REG_BLD_WD_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_0=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_1=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_2=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_3=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_4=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_5=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_6=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_6));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_TOP_PRI_M_REG_8=0x%08x\n", _reg_read(top_base + SC_TOP_PRI_M_REG_8));
}

void sclr_show_top_status(void)
{
	u32 val, rdy;
	uintptr_t top_base = reg_base + REG_SCL_TOP_BASE;

	val = _reg_read(top_base + SC_TOP_REG_01);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP_REG_01=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_d_en=%d, sc_v1_en=%d, sc_v2_en=%d, sc_v3_en=%d\n",
		(val & SC_TOP_REG_SC_D_EN_MASK) >> SC_TOP_REG_SC_D_EN_OFFSET,
		(val & SC_TOP_REG_SC_V1_EN_MASK) >> SC_TOP_REG_SC_V1_EN_OFFSET,
		(val & SC_TOP_REG_SC_V2_EN_MASK) >> SC_TOP_REG_SC_V2_EN_OFFSET,
		(val & SC_TOP_REG_SC_V3_EN_MASK) >> SC_TOP_REG_SC_V3_EN_OFFSET);

	val = _reg_read(top_base + SC_TOP_REG_SB_CBAR);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP_REG_SB_CBAR=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sb_wr_ctrl0=%d, sb_wr_ctrl1=%d, sb_rd_ctrl=%d\n",
		(val & SC_TOP_REG_SB_WR_CTRL0_MASK) >> SC_TOP_REG_SB_WR_CTRL0_OFFSET,
		(val & SC_TOP_REG_SB_WR_CTRL1_MASK) >> SC_TOP_REG_SB_WR_CTRL1_OFFSET,
		(val & SC_TOP_REG_SB_RD_CTRL_MASK) >> SC_TOP_REG_SB_RD_CTRL_OFFSET);

	val = _reg_read(top_base + SC_TOP_REG_0D);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP_REG_0D=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    int_status=0x%08x\n", val);

	val = _reg_read(top_base + SC_TOP_REG_DBG_VALID);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP_REG_DBG_VALID=0x%08x\n", val);
	rdy = _reg_read(top_base + SC_TOP_REG_DBG_READY);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP_REG_DBG_READY=0x%08x\n", rdy);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_TOP valid=0x%08x, ready=0x%08x\n", val, rdy);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "               valid     ready\n");
	CVI_TRACE_VPSS(CVI_DBG_INFO, "isp2ip_y in       %d        %d\n",
		val & 1, rdy & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "isp2ip_u in       %d        %d\n",
		(val >> 1) & 1, (rdy >> 1) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "isp2ip_v in       %d        %d\n",
		(val >> 2) & 1, (rdy >> 2) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "img_d out         %d        %d\n",
		(val >> 3) & 1, (rdy >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "img_v out         %d        %d\n",
		(val >> 4) & 1, (rdy >> 4) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "bld_sa(img_v)     %d        %d\n",
		(val >> 5) & 1, (rdy >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "bld_sb(img_d)     %d        %d\n",
		(val >> 6) & 1, (rdy >> 6) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "bld_m             %d        %d\n",
		(val >> 7) & 1, (rdy >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "pri_sp            %d        %d\n",
		(val >> 8) & 1, (rdy >> 8) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "pri_m             %d        %d\n",
		(val >> 9) & 1, (rdy >> 9) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc_d              %d        %d\n",
		(val >> 10) & 1, (rdy >> 10) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc_v1             %d        %d\n",
		(val >> 11) & 1, (rdy >> 11) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc_v2             %d        %d\n",
		(val >> 12) & 1, (rdy >> 12) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc_v3             %d        %d\n",
		(val >> 13) & 1, (rdy >> 13) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc_d_out          %d        %d\n",
		(val >> 14) & 1, (rdy >> 14) & 1);
}

void sclr_dump_img_in_register(int img_inst)
{
	uintptr_t img_base = reg_base + REG_SCL_IMG_BASE(img_inst);
	u32 val;

	if (img_inst >= 2)
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d) out of range\n", img_inst);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "img(%d)\n", img_inst);

	val = _reg_read(img_base + IMG_IN_REG_00);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_00=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_sel=%d, fmt_sel=%d, burst_len=%d, img_csc_en=%d\n",
		(val & IMG_IN_REG_SRC_SEL_MASK) >> IMG_IN_REG_SRC_SEL_OFFSET,
		(val & IMG_IN_REG_FMT_SEL_MASK) >> IMG_IN_REG_FMT_SEL_OFFSET,
		(val & IMG_IN_REG_BURST_LN_MASK) >> IMG_IN_REG_BURST_LN_OFFSET,
		(val & IMG_IN_REG_IMG_CSC_EN_MASK) >> IMG_IN_REG_IMG_CSC_EN_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    auto_csc_en=%d, 64b_align=%d, force_clk_enable=%d\n",
		(val & IMG_IN_REG_AUTO_CSC_EN_MASK) >> IMG_IN_REG_AUTO_CSC_EN_OFFSET,
		(val & IMG_IN_REG_64B_ALIGN_MASK) >> IMG_IN_REG_64B_ALIGN_OFFSET,
		(val & IMG_IN_REG_FORCE_CLK_ENABLE_MASK) >> IMG_IN_REG_FORCE_CLK_ENABLE_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_01);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_01=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_x_str=%d, src_y_str=%d\n",
		(val & IMG_IN_REG_SRC_X_STR_MASK) >> IMG_IN_REG_SRC_X_STR_OFFSET,
		(val & IMG_IN_REG_SRC_Y_STR_MASK) >> IMG_IN_REG_SRC_Y_STR_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_02);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_02=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_wd=%d, src_ht=%d\n",
		(val & IMG_IN_REG_SRC_WD_MASK) >> IMG_IN_REG_SRC_WD_OFFSET,
		(val & IMG_IN_REG_SRC_HT_MASK) >> IMG_IN_REG_SRC_HT_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_03);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_03=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_y_pitch=%d\n",
		(val & IMG_IN_REG_SRC_Y_PITCH_MASK) >> IMG_IN_REG_SRC_Y_PITCH_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_04);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_04=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_c_pitch=%d\n",
		(val & IMG_IN_REG_SRC_C_PITCH_MASK) >> IMG_IN_REG_SRC_C_PITCH_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_05);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_05=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sw_force_up=%d, sw_mask_up=%d, shrd_sel=%d\n",
		(val & IMG_IN_REG_SW_FORCE_UP_MASK) >> IMG_IN_REG_SW_FORCE_UP_OFFSET,
		(val & IMG_IN_REG_SW_MASK_UP_MASK) >> IMG_IN_REG_SW_MASK_UP_OFFSET,
		(val & IMG_IN_REG_SHRD_SEL_MASK) >> IMG_IN_REG_SHRD_SEL_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_06=0x%08x\n", _reg_read(img_base + IMG_IN_REG_06));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_07=0x%08x\n", _reg_read(img_base + IMG_IN_REG_07));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_08=0x%08x\n", _reg_read(img_base + IMG_IN_REG_08));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_Y_BASE_0=0x%08x\n", _reg_read(img_base + IMG_IN_REG_Y_BASE_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_Y_BASE_1=0x%08x\n", _reg_read(img_base + IMG_IN_REG_Y_BASE_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_U_BASE_0=0x%08x\n", _reg_read(img_base + IMG_IN_REG_U_BASE_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_U_BASE_1=0x%08x\n", _reg_read(img_base + IMG_IN_REG_U_BASE_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_V_BASE_0=0x%08x\n", _reg_read(img_base + IMG_IN_REG_V_BASE_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_V_BASE_1=0x%08x\n", _reg_read(img_base + IMG_IN_REG_V_BASE_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_040=0x%08x\n", _reg_read(img_base + IMG_IN_REG_040));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_044=0x%08x\n", _reg_read(img_base + IMG_IN_REG_044));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_048=0x%08x\n", _reg_read(img_base + IMG_IN_REG_048));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_04C=0x%08x\n", _reg_read(img_base + IMG_IN_REG_04C));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_050=0x%08x\n", _reg_read(img_base + IMG_IN_REG_050));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_054=0x%08x\n", _reg_read(img_base + IMG_IN_REG_054));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_058=0x%08x\n", _reg_read(img_base + IMG_IN_REG_058));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_05C=0x%08x\n", _reg_read(img_base + IMG_IN_REG_05C));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_060=0x%08x\n", _reg_read(img_base + IMG_IN_REG_060));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_064=0x%08x\n", _reg_read(img_base + IMG_IN_REG_064));

	val = _reg_read(img_base + IMG_IN_REG_068);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_068=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_err_fwr_y=%d, reg_err_fwr_u=%d, reg_err_fwr_v=%d\n",
		(val & IMG_IN_REG_ERR_FWR_Y_MASK) >> IMG_IN_REG_ERR_FWR_Y_OFFSET,
		(val & IMG_IN_REG_ERR_FWR_U_MASK) >> IMG_IN_REG_ERR_FWR_U_OFFSET,
		(val & IMG_IN_REG_ERR_FWR_V_MASK) >> IMG_IN_REG_ERR_FWR_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_err_erd_y=%d, reg_err_erd_u=%d, reg_err_erd_v=%d\n",
		(val & IMG_IN_REG_ERR_ERD_Y_MASK) >> IMG_IN_REG_ERR_ERD_Y_OFFSET,
		(val & IMG_IN_REG_ERR_ERD_U_MASK) >> IMG_IN_REG_ERR_ERD_U_OFFSET,
		(val & IMG_IN_REG_ERR_ERD_V_MASK) >> IMG_IN_REG_ERR_ERD_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_lb_full_y=%d, reg_lb_full_u=%d, reg_lb_full_v=%d\n",
		(val & IMG_IN_REG_LB_FULL_Y_MASK) >> IMG_IN_REG_LB_FULL_Y_OFFSET,
		(val & IMG_IN_REG_LB_FULL_U_MASK) >> IMG_IN_REG_LB_FULL_U_OFFSET,
		(val & IMG_IN_REG_LB_FULL_V_MASK) >> IMG_IN_REG_LB_FULL_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_lb_empty_y=%d, reg_lb_empty_u=%d, reg_lb_empty_v=%d\n",
		(val & IMG_IN_REG_LB_EMPTY_Y_MASK) >> IMG_IN_REG_LB_EMPTY_Y_OFFSET,
		(val & IMG_IN_REG_LB_EMPTY_U_MASK) >> IMG_IN_REG_LB_EMPTY_U_OFFSET,
		(val & IMG_IN_REG_LB_EMPTY_V_MASK) >> IMG_IN_REG_LB_EMPTY_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_ip_idle=%d, reg_ip_int=%d\n",
		(val & IMG_IN_REG_IP_IDLE_MASK) >> IMG_IN_REG_IP_IDLE_OFFSET,
		(val & IMG_IN_REG_IP_INT_MASK) >> IMG_IN_REG_IP_INT_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_AXI_ST);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_AXI_ST=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_axi_idle=%d, reg_axi_status=%d\n",
		(val & IMG_IN_REG_AXI_IDLE_MASK) >> IMG_IN_REG_AXI_IDLE_OFFSET,
		(val & IMG_IN_REG_AXI_STATUS_MASK) >> IMG_IN_REG_AXI_STATUS_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_BW_LIMIT=0x%08x\n", _reg_read(img_base + IMG_IN_REG_BW_LIMIT));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_CATCH=0x%08x\n", _reg_read(img_base + IMG_IN_REG_CATCH));

	val = _reg_read(img_base + IMG_IN_REG_CHK_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_REG_CHK_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "  reg_chksum_dat_out=0x%x, reg_checksum_en=%d\n",
		(val & IMG_IN_REG_CHKSUM_DAT_OUT_MASK) >> IMG_IN_REG_CHKSUM_DAT_OUT_OFFSET,
		(val & IMG_IN_REG_CHECKSUM_EN_MASK) >> IMG_IN_REG_CHECKSUM_EN_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_CHKSUM_AXI_RD=0x%08x\n", _reg_read(img_base + IMG_IN_CHKSUM_AXI_RD));

	val = _reg_read(img_base + IMG_IN_SB_REG_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_SB_REG_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sb_mode=%d, sb_size=%d, sb_nb=%d, sb_sw_rptr=%d\n",
		(val & IMG_IN_REG_SB_MODE_MASK) >> IMG_IN_REG_SB_MODE_OFFSET,
		(val & IMG_IN_REG_SB_SIZE_MASK) >> IMG_IN_REG_SB_SIZE_OFFSET,
		(val & IMG_IN_REG_SB_NB_MASK) >> IMG_IN_REG_SB_NB_OFFSET,
		(val & IMG_IN_REG_SB_SW_RPTR_MASK) >> IMG_IN_REG_SB_SW_RPTR_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_SB_REG_C_STAT=0x%08x\n", _reg_read(img_base + IMG_IN_SB_REG_C_STAT));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "IMG_IN_SB_REG_Y_STAT=0x%08x\n", _reg_read(img_base + IMG_IN_SB_REG_Y_STAT));
}

void sclr_show_img_in_status(int img_inst)
{
	uintptr_t img_base = reg_base + REG_SCL_IMG_BASE(img_inst);
	u32 val;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "img(%d)\n", img_inst);

	val = _reg_read(img_base + IMG_IN_REG_068);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "IMG_IN_REG_068=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_err_fwr_y=%d, reg_err_fwr_u=%d, reg_err_fwr_v=%d\n",
		(val & IMG_IN_REG_ERR_FWR_Y_MASK) >> IMG_IN_REG_ERR_FWR_Y_OFFSET,
		(val & IMG_IN_REG_ERR_FWR_U_MASK) >> IMG_IN_REG_ERR_FWR_U_OFFSET,
		(val & IMG_IN_REG_ERR_FWR_V_MASK) >> IMG_IN_REG_ERR_FWR_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_err_erd_y=%d, reg_err_erd_u=%d, reg_err_erd_v=%d\n",
		(val & IMG_IN_REG_ERR_ERD_Y_MASK) >> IMG_IN_REG_ERR_ERD_Y_OFFSET,
		(val & IMG_IN_REG_ERR_ERD_U_MASK) >> IMG_IN_REG_ERR_ERD_U_OFFSET,
		(val & IMG_IN_REG_ERR_ERD_V_MASK) >> IMG_IN_REG_ERR_ERD_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_lb_full_y=%d, reg_lb_full_u=%d, reg_lb_full_v=%d\n",
		(val & IMG_IN_REG_LB_FULL_Y_MASK) >> IMG_IN_REG_LB_FULL_Y_OFFSET,
		(val & IMG_IN_REG_LB_FULL_U_MASK) >> IMG_IN_REG_LB_FULL_U_OFFSET,
		(val & IMG_IN_REG_LB_FULL_V_MASK) >> IMG_IN_REG_LB_FULL_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_lb_empty_y=%d, reg_lb_empty_u=%d, reg_lb_empty_v=%d\n",
		(val & IMG_IN_REG_LB_EMPTY_Y_MASK) >> IMG_IN_REG_LB_EMPTY_Y_OFFSET,
		(val & IMG_IN_REG_LB_EMPTY_U_MASK) >> IMG_IN_REG_LB_EMPTY_U_OFFSET,
		(val & IMG_IN_REG_LB_EMPTY_V_MASK) >> IMG_IN_REG_LB_EMPTY_V_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_ip_idle=%d, reg_ip_int=%d\n",
		(val & IMG_IN_REG_IP_IDLE_MASK) >> IMG_IN_REG_IP_IDLE_OFFSET,
		(val & IMG_IN_REG_IP_INT_MASK) >> IMG_IN_REG_IP_INT_OFFSET);

	val = _reg_read(img_base + IMG_IN_REG_AXI_ST);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "IMG_IN_REG_AXI_ST=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "  reg_axi_idle=%d, reg_axi_status=%d\n",
		(val & IMG_IN_REG_AXI_IDLE_MASK) >> IMG_IN_REG_AXI_IDLE_OFFSET,
		(val & IMG_IN_REG_AXI_STATUS_MASK) >> IMG_IN_REG_AXI_STATUS_OFFSET);
}

void sclr_dump_core_register(int inst)
{
	u32 val;
	uintptr_t core_base = reg_base + REG_SCL_CORE_BASE(inst);

	if (inst >= SCL_MAX_INST)
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) core\n", inst);

	val = _reg_read(core_base + SC_CORE_SC_WRAP_REG_0);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_WRAP_REG_0=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_core_bypass_en=%d, vgop_bypass_en=%d, dbg_en=%d\n",
		(val & SC_CORE_REG_SC_CORE_BYPASS_EN_MASK) >> SC_CORE_REG_SC_CORE_BYPASS_EN_OFFSET,
		(val & SC_CORE_REG_VGOP_BYPASS_EN_MASK) >> SC_CORE_REG_VGOP_BYPASS_EN_OFFSET,
		(val & SC_CORE_REG_DBG_EN_MASK) >> SC_CORE_REG_DBG_EN_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    cir_bypass_en=%d, dma_bypass_en=%d, force_clk_enable=%d\n",
		(val & SC_CORE_REG_CIR_BYPASS_EN_MASK) >> SC_CORE_REG_CIR_BYPASS_EN_OFFSET,
		(val & SC_CORE_REG_DMA_BYPASS_EN_MASK) >> SC_CORE_REG_DMA_BYPASS_EN_OFFSET,
		(val & SC_CORE_REG_FORCE_CLK_ENABLE_MASK) >> SC_CORE_REG_FORCE_CLK_ENABLE_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_WRAP_REG_1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_WRAP_REG_1=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    shdw_read_sel=%d\n",
		(val & SC_CORE_REG_SHDW_READ_SEL_MASK) >> SC_CORE_REG_SHDW_READ_SEL_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_WRAP_REG_2);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_WRAP_REG_2=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG,
		"    crop_idle=%d, hscale_idle=%d, vscale_idle=%d, vgop_idle=%d, sc_wdma_idle=%d\n",
		(val & SC_CORE_REG_CROP_IDLE_MASK) >> SC_CORE_REG_CROP_IDLE_OFFSET,
		(val & SC_CORE_REG_HSCALE_IDLE_MASK) >> SC_CORE_REG_HSCALE_IDLE_OFFSET,
		(val & SC_CORE_REG_VSCALE_IDLE_MASK) >> SC_CORE_REG_VSCALE_IDLE_OFFSET,
		(val & SC_CORE_REG_VGOP_IDLE_MASK) >> SC_CORE_REG_VGOP_IDLE_OFFSET,
		(val & SC_CORE_REG_SC_WDMA_IDLE_MASK) >> SC_CORE_REG_SC_WDMA_IDLE_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CROP_REG_0);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CROP_REG_0=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    src_wd=%d, src_ht=%d\n",
		(val & SC_CORE_REG_SRC_WD_MASK) >> SC_CORE_REG_SRC_WD_OFFSET,
		(val & SC_CORE_REG_SRC_HT_MASK) >> SC_CORE_REG_SRC_HT_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CROP_REG_1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CROP_REG_1=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    crop_x_str=%d, crop_y_str=%d\n",
		(val & SC_CORE_REG_CROP_X_STR_MASK) >> SC_CORE_REG_CROP_X_STR_OFFSET,
		(val & SC_CORE_REG_CROP_Y_STR_MASK) >> SC_CORE_REG_CROP_Y_STR_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CROP_REG_2);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CROP_REG_2=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    crop_wd=%d, crop_ht=%d\n",
		(val & SC_CORE_REG_CROP_WD_MASK) >> SC_CORE_REG_CROP_WD_OFFSET,
		(val & SC_CORE_REG_CROP_HT_MASK) >> SC_CORE_REG_CROP_HT_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CROP_REG_3);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CROP_REG_3=0x%08x\n", val);

	val = (val & SC_CORE_REG_CROP_DEBUG_MASK) >> SC_CORE_REG_CROP_DEBUG_OFFSET;
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    crop_debug=0x%x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    ip2s_req=%d              s2ip_rdy=%d\n",
		(val >> 0) & 1, (val >> 16) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    d2sc_aff_req=%d          sc_aff2d_rdy=%d\n",
		(val >> 1) & 1, (val >> 17) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    crop2s_req=%d            s2crop_rdy=%d\n",
		(val >> 2) & 1, (val >> 18) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc2s_req=%d              s2sc_rdy=%d\n",
		(val >> 3) & 1, (val >> 19) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    cir2s_req=%d             s2cir_rdy=%d\n",
		(val >> 4) & 1, (val >> 20) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    pri_sp_tready_o=%d       pri_sp_tvalid_i=%d\n",
		(val >> 5) & 1, (val >> 21) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    border2s_req=%d          s2border_rdy=%d\n",
		(val >> 6) & 1, (val >> 22) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    vgop2front_req=%d        front2vgop_rdy=%d\n",
		(val >> 7) & 1, (val >> 23) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    vgop_b2front_req=%d      front2vgop_b_rdy=%d\n",
		(val >> 8) & 1, (val >> 24) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    s_tready_sl_csc_in=%d    s_tvalid_sl_csc_in=%d\n",
		(val >> 9) & 1, (val >> 25) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    csc2s_req=%d             s2csc_rdy=%d\n",
		(val >> 10) & 1, (val >> 26) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    hsv2s_req=%d             s2hsv_rdy=%d\n",
		(val >> 11) & 1, (val >> 27) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    dma2s_req=%d             s2dma_rdy=%d\n",
		(val >> 12) & 1, (val >> 28) & 1);

	val = _reg_read(core_base + SC_CORE_SC_CORE_CFG);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_CFG=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    tap number=%d, Version=%d, line buffer width=%d\n",
		(val & 0xf0000000) >> 28,
		(val & 0x0f000000) >> 24,
		(val & 0xffff));

	val = _reg_read(core_base + SC_CORE_SC_2TAP_CFG);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_2TAP_CFG=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    resize_blnr_mode=%d, resize_area_fast=%d, resize_dbg_sel=%d\n",
		(val & SC_CORE_REG_RESIZE_BLNR_MODE_MASK) >> SC_CORE_REG_RESIZE_BLNR_MODE_OFFSET,
		(val & SC_CORE_REG_RESIZE_AREA_FAST_MASK) >> SC_CORE_REG_RESIZE_AREA_FAST_OFFSET,
		(val & SC_CORE_REG_RESIZE_DBG_SEL_MASK) >> SC_CORE_REG_RESIZE_DBG_SEL_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_2TAP_NOR);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_2TAP_NOR=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    resize_h_nor=0x%04x, resize_v_nor=0x%04x\n",
		(val & SC_CORE_REG_RESIZE_H_NOR_MASK) >> SC_CORE_REG_RESIZE_H_NOR_OFFSET,
		(val & SC_CORE_REG_RESIZE_V_NOR_MASK) >> SC_CORE_REG_RESIZE_V_NOR_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_2_TAP_DBG=0x%08x\n", _reg_read(core_base + SC_CORE_SC_2_TAP_DBG));

	val = _reg_read(core_base + SC_CORE_BORDER_0);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BORDER_0=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    bd_r=%d, bd_g=%d, bd_b=%d, border_en=%d\n",
		(val & SC_CORE_REG_BD_R_MASK) >> SC_CORE_REG_BD_R_OFFSET,
		(val & SC_CORE_REG_BD_G_MASK) >> SC_CORE_REG_BD_G_OFFSET,
		(val & SC_CORE_REG_BD_B_MASK) >> SC_CORE_REG_BD_B_OFFSET,
		(val & SC_CORE_REG_BORDER_EN_MASK) >> SC_CORE_REG_BORDER_EN_OFFSET);

	val = _reg_read(core_base + SC_CORE_BORDER_1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BORDER_1=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    border_x_str=%d, border_y_str=%d\n",
		(val & SC_CORE_REG_BORDER_X_STR_MASK) >> SC_CORE_REG_BORDER_X_STR_OFFSET,
		(val & SC_CORE_REG_BORDER_Y_STR_MASK) >> SC_CORE_REG_BORDER_Y_STR_OFFSET);

	val = _reg_read(core_base + SC_CORE_REG_CHK_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_REG_CHK_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_chksum_dat_in=0x%x, reg_chksum_dat_out=0x%x, reg_checksum_en=%d\n",
		(val & SC_CORE_REG_CHKSUM_DAT_IN_MASK) >> SC_CORE_REG_CHKSUM_DAT_IN_OFFSET,
		(val & SC_CORE_REG_CHKSUM_DAT_OUT_MASK) >> SC_CORE_REG_CHKSUM_DAT_OUT_OFFSET,
		(val & SC_CORE_REG_CHECKSUM_EN_MASK) >> SC_CORE_REG_CHECKSUM_EN_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_CHK_AXI_VGOP_0=0x%08x\n", _reg_read(core_base + SC_CORE_CHK_AXI_VGOP_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_CHK_AXI_VGOP_1=0x%08x\n", _reg_read(core_base + SC_CORE_CHK_AXI_VGOP_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_CHK_AXI_WDATA=0x%08x\n", _reg_read(core_base + SC_CORE_CHK_AXI_WDATA));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_0=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_1=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_2=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_3=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_4=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_5=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_6=0x%08x\n", _reg_read(core_base + SC_CORE_SC_CORE_REG_6));

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_7);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_7=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_h_str_mir=%d, sc_v_str_mir=%d, sc_cb_mode_en=%d\n",
		(val & SC_CORE_REG_SC_H_STR_MIR_MASK) >> SC_CORE_REG_SC_H_STR_MIR_OFFSET,
		(val & SC_CORE_REG_SC_V_STR_MIR_MASK) >> SC_CORE_REG_SC_V_STR_MIR_OFFSET,
		(val & SC_CORE_REG_SC_CB_MODE_EN_MASK) >> SC_CORE_REG_SC_CB_MODE_EN_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_8);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_8=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    h_sc_fac=0x%x\n",
		(val & SC_CORE_REG_H_SC_FAC_MASK) >> SC_CORE_REG_H_SC_FAC_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_9);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_9=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    v_sc_fac=0x%x\n",
		(val & SC_CORE_REG_V_SC_FAC_MASK) >> SC_CORE_REG_V_SC_FAC_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_10);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_10=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_core_wd=%d, sc_core_ht=%d\n",
		(val & SC_CORE_REG_SC_CORE_WD_MASK) >> SC_CORE_REG_SC_CORE_WD_OFFSET,
		(val & SC_CORE_REG_SC_CORE_HT_MASK) >> SC_CORE_REG_SC_CORE_HT_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_11);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_11=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    vscale_pix_fifo_empty=%d, vscale_pix_fifo_full=%d\n",
			val & 1, (val >> 1) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    hscale_pix_fifo_empty=%d, hscale_pix_fifo_full=%d\n",
			(val >> 2) & 1, (val >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    v_ahead=%d, vscale_en=%d\n",
			(val >> 4) & 1, (val >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    hscale_en=%d, sc_async_fifo_empty=%d\n",
			(val >> 6) & 1, (val >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_async_fifo_full=%d, reg_v_lb_err=%d\n",
			(val >> 8) & 1, (val >> 9) & 1);

	val =  _reg_read(core_base + SC_CORE_SC_CORE_REG_12);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_CORE_REG_12=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    h_ini_ph=0x%04x, v_ini_ph=0x%04x\n",
		(val & SC_CORE_REG_H_INI_PH_MASK) >> SC_CORE_REG_H_INI_PH_OFFSET,
		(val & SC_CORE_REG_V_INI_PH_MASK) >> SC_CORE_REG_V_INI_PH_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_0=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_1=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_2=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_3=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_4=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_5=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_BLT_CIR_REG_6=0x%08x\n", _reg_read(core_base + SC_CORE_BLT_CIR_REG_6));

	val = _reg_read(core_base + SC_CORE_PRI_M_REG_0);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_0=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    pri_m_en=%d, pri_m_pixel_md=%d, pri_force_alpha=%d\n",
		(val & SC_CORE_REG_PRI_M_EN_MASK) >> SC_CORE_REG_PRI_M_EN_OFFSET,
		(val & SC_CORE_REG_PRI_M_PIXEL_MD_MASK) >> SC_CORE_REG_PRI_M_PIXEL_MD_OFFSET,
		(val & SC_CORE_REG_PRI_FORCE_ALPHA_MASK) >> SC_CORE_REG_PRI_FORCE_ALPHA_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    pri_mask_rgb332=%d, pri_blend_y_only=%d, pri_y2r_enable=%d\n",
		(val & SC_CORE_REG_PRI_MASK_RGB332_MASK) >> SC_CORE_REG_PRI_MASK_RGB332_OFFSET,
		(val & SC_CORE_REG_PRI_BLEND_Y_ONLY_MASK) >> SC_CORE_REG_PRI_BLEND_Y_ONLY_OFFSET,
		(val & SC_CORE_REG_PRI_Y2R_ENABLE_MASK) >> SC_CORE_REG_PRI_Y2R_ENABLE_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, " pri_grid_size=%d, pri_m_fit_p=%d\n",
		(val & SC_CORE_REG_PRI_GRID_SIZE_MASK) >> SC_CORE_REG_PRI_GRID_SIZE_OFFSET,
		(val & SC_CORE_REG_PRI_M_FIT_P_MASK) >> SC_CORE_REG_PRI_M_FIT_P_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_1=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_2=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_3=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_4=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_5=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_6=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_6));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_PRI_M_REG_7=0x%08x\n", _reg_read(core_base + SC_CORE_PRI_M_REG_7));
}

void sclr_show_core_status(int inst)
{
	u32 val;
	uintptr_t core_base = reg_base + REG_SCL_CORE_BASE(inst);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc(%d) core\n", inst);

	val = _reg_read(core_base + SC_CORE_SC_WRAP_REG_2);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_CORE_SC_WRAP_REG_2=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO,
		"    crop_idle=%d, hscale_idle=%d, vscale_idle=%d, vgop_idle=%d, sc_wdma_idle=%d\n",
		(val & SC_CORE_REG_CROP_IDLE_MASK) >> SC_CORE_REG_CROP_IDLE_OFFSET,
		(val & SC_CORE_REG_HSCALE_IDLE_MASK) >> SC_CORE_REG_HSCALE_IDLE_OFFSET,
		(val & SC_CORE_REG_VSCALE_IDLE_MASK) >> SC_CORE_REG_VSCALE_IDLE_OFFSET,
		(val & SC_CORE_REG_VGOP_IDLE_MASK) >> SC_CORE_REG_VGOP_IDLE_OFFSET,
		(val & SC_CORE_REG_SC_WDMA_IDLE_MASK) >> SC_CORE_REG_SC_WDMA_IDLE_OFFSET);

	val = _reg_read(core_base + SC_CORE_SC_CROP_REG_3);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_CORE_SC_CROP_REG_3=0x%08x\n", val);

	val = (val & SC_CORE_REG_CROP_DEBUG_MASK) >> SC_CORE_REG_CROP_DEBUG_OFFSET;
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    crop_debug=0x%x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    ip2s_req=%d              s2ip_rdy=%d\n",
		(val >> 0) & 1, (val >> 16) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    d2sc_aff_req=%d          sc_aff2d_rdy=%d\n",
		(val >> 1) & 1, (val >> 17) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    crop2s_req=%d            s2crop_rdy=%d\n",
		(val >> 2) & 1, (val >> 18) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc2s_req=%d              s2sc_rdy=%d\n",
		(val >> 3) & 1, (val >> 19) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    cir2s_req=%d             s2cir_rdy=%d\n",
		(val >> 4) & 1, (val >> 20) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    pri_sp_tready_o=%d       pri_sp_tvalid_i=%d\n",
		(val >> 5) & 1, (val >> 21) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    border2s_req=%d          s2border_rdy=%d\n",
		(val >> 6) & 1, (val >> 22) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    vgop2front_req=%d        front2vgop_rdy=%d\n",
		(val >> 7) & 1, (val >> 23) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    vgop_b2front_req=%d      front2vgop_b_rdy=%d\n",
		(val >> 8) & 1, (val >> 24) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    s_tready_sl_csc_in=%d    s_tvalid_sl_csc_in=%d\n",
		(val >> 9) & 1, (val >> 25) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    csc2s_req=%d             s2csc_rdy=%d\n",
		(val >> 10) & 1, (val >> 26) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    hsv2s_req=%d             s2hsv_rdy=%d\n",
		(val >> 11) & 1, (val >> 27) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    dma2s_req=%d             s2dma_rdy=%d\n",
		(val >> 12) & 1, (val >> 28) & 1);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_CORE_SC_2_TAP_DBG=0x%08x\n", _reg_read(core_base + SC_CORE_SC_2_TAP_DBG));

	val = _reg_read(core_base + SC_CORE_SC_CORE_REG_11);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_CORE_SC_CORE_REG_11=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    vscale_pix_fifo_empty=%d, vscale_pix_fifo_full=%d\n",
			val & 1, (val >> 1) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    hscale_pix_fifo_empty=%d, hscale_pix_fifo_full=%d\n",
			(val >> 2) & 1, (val >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    v_ahead=%d, vscale_en=%d\n",
			(val >> 4) & 1, (val >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    hscale_en=%d, sc_async_fifo_empty=%d\n",
			(val >> 6) & 1, (val >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_async_fifo_full=%d, reg_v_lb_err=%d\n",
			(val >> 8) & 1, (val >> 9) & 1);
}

void sclr_dump_odma_register(int inst)
{
	u32 val;
	uintptr_t odma_base = reg_base + REG_SCL_ODMA_BASE(inst);

	if (inst >= SCL_MAX_INST)
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) odma\n", inst);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_00);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_00=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    blen=%d, fmt_sel=%d, hflip=%d, vflip=%d\n",
		(val & SC_ODMA_REG_DMA_BLEN_MASK) >> SC_ODMA_REG_DMA_BLEN_OFFSET,
		(val & SC_ODMA_REG_FMT_SEL_MASK) >> SC_ODMA_REG_FMT_SEL_OFFSET,
		(val & SC_ODMA_REG_SC_ODMA_HFLIP_MASK) >> SC_ODMA_REG_SC_ODMA_HFLIP_OFFSET,
		(val & SC_ODMA_REG_SC_ODMA_VFLIP_MASK) >> SC_ODMA_REG_SC_ODMA_VFLIP_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    422_avg=%d, 420_avg=%d, c_round_mode=%d, bf16_en=%d\n",
		(val & SC_ODMA_REG_SC_422_AVG_MASK) >> SC_ODMA_REG_SC_422_AVG_OFFSET,
		(val & SC_ODMA_REG_SC_420_AVG_MASK) >> SC_ODMA_REG_SC_420_AVG_OFFSET,
		(val & SC_ODMA_REG_C_ROUND_MODE_MASK) >> SC_ODMA_REG_C_ROUND_MODE_OFFSET,
		(val & SC_ODMA_REG_BF16_EN_MASK) >> SC_ODMA_REG_BF16_EN_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_01);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_01=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_y_base_low_part=0x%08x\n", val);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_02=0x%08x\n", _reg_read(odma_base + SC_ODMA_ODMA_REG_02));

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_03);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_03=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_u_base_low_part=0x%08x\n", val);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_04=0x%08x\n", _reg_read(odma_base + SC_ODMA_ODMA_REG_04));

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_05);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_05=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_v_base_low_part=0x%08x\n", val);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_06=0x%08x\n", _reg_read(odma_base + SC_ODMA_ODMA_REG_06));

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_07);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_07=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_y_pitch=%d\n",
		(val & SC_ODMA_REG_DMA_Y_PITCH_MASK) >> SC_ODMA_REG_DMA_Y_PITCH_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_08);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_08=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_c_pitch=%d\n",
		(val & SC_ODMA_REG_DMA_Y_PITCH_MASK) >> SC_ODMA_REG_DMA_Y_PITCH_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_09);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_09=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_x_str=%d\n",
		(val & SC_ODMA_REG_DMA_X_STR_MASK) >> SC_ODMA_REG_DMA_X_STR_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_10);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_10=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_y_str=%d\n",
		(val & SC_ODMA_REG_DMA_Y_STR_MASK) >> SC_ODMA_REG_DMA_Y_STR_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_11);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_11=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_wd=%d\n",
		(val & SC_ODMA_REG_DMA_WD_MASK) >> SC_ODMA_REG_DMA_WD_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_12);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_12=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_ht=%d\n",
		(val & SC_ODMA_REG_DMA_HT_MASK) >> SC_ODMA_REG_DMA_HT_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_13);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_13=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_debug=0x%x\n",
		(val & SC_ODMA_REG_DMA_DEBUG_MASK) >> SC_ODMA_REG_DMA_DEBUG_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_axi_cmd_cs [0]=%d, [1]=%d, [2]=%d, [3]=%d\n",
		val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_v_buf_empty=%d, sc_odma_v_buf_full=%d\n",
		(val >> 4) & 1, (val >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_u_buf_empty=%d, sc_odma_u_buf_full=%d\n",
		(val >> 6) & 1, (val >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_y_buf_empty=%d, sc_odma_y_buf_full=%d\n",
		(val >> 8) & 1, (val >> 9) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_axi_v_active=%d, sc_odma_axi_u_active=%d\n",
		(val >> 10) & 1, (val >> 11) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_odma_axi_y_active=%d, sc_odma_axi_active=%d\n",
		(val >> 12) & 1, (val >> 13) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_v_sb_empty=%d, reg_v_sb_full=%d\n",
		(val >> 14) & 1, (val >> 15) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_u_sb_empty=%d, reg_u_sb_full=%d\n",
		(val >> 16) & 1, (val >> 17) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_y_sb_empty=%d, reg_y_sb_full=%d\n",
		(val >> 18) & 1, (val >> 19) & 1);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_sb_full=%d\n",
		(val >> 20) & 1);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_14);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_14=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_int_line_target=%d, reg_dma_int_line_target_sel=%d\n",
		(val & SC_ODMA_REG_DMA_INT_LINE_TARGET_MASK) >> SC_ODMA_REG_DMA_INT_LINE_TARGET_OFFSET,
		(val & SC_ODMA_REG_DMA_INT_LINE_TARGET_SEL_MASK) >> SC_ODMA_REG_DMA_INT_LINE_TARGET_SEL_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_15);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_15=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_int_cycle_line_target=%d, reg_dma_int_cycle_line_target_sel=%d\n",
		(val & SC_ODMA_REG_DMA_INT_CYCLE_LINE_TARGET_MASK) >> SC_ODMA_REG_DMA_INT_CYCLE_LINE_TARGET_OFFSET,
		(val & SC_ODMA_REG_DMA_INT_CYCLE_LINE_TARGET_SEL_MASK) >>
		SC_ODMA_REG_DMA_INT_CYCLE_LINE_TARGET_SEL_OFFSET);

	_reg_write(odma_base + SC_ODMA_ODMA_REG_16, 0x1);
	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_16);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_ODMA_REG_16=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_dma_latch_line_cnt=%d, reg_dma_latched_line_cnt=%d\n",
		(val & SC_ODMA_REG_DMA_LATCH_LINE_CNT_MASK) >> SC_ODMA_REG_DMA_LATCH_LINE_CNT_OFFSET,
		(val & SC_ODMA_REG_DMA_LATCHED_LINE_CNT_MASK) >> SC_ODMA_REG_DMA_LATCHED_LINE_CNT_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SB_REG_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sb_mode=%d, sb_size=%d, sb_nb=%d, sb_full_nb=%d, sb_sw_wptr=%d\n",
		(val & SC_ODMA_REG_SB_MODE_MASK) >> SC_ODMA_REG_SB_MODE_OFFSET,
		(val & SC_ODMA_REG_SB_SIZE_MASK) >> SC_ODMA_REG_SB_SIZE_OFFSET,
		(val & SC_ODMA_REG_SB_NB_MASK) >> SC_ODMA_REG_SB_NB_OFFSET,
		(val & SC_ODMA_REG_SB_FULL_NB_MASK) >> SC_ODMA_REG_SB_FULL_NB_OFFSET,
		(val & SC_ODMA_REG_SB_SW_WPTR_MASK) >> SC_ODMA_REG_SB_SW_WPTR_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_C_STAT);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SB_REG_C_STAT=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    u_sb_wptr_ro=%d, u_sb_full=%d, u_sb_empty=%d, u_sb_dptr_ro=%d\n",
		(val & SC_ODMA_REG_U_SB_WPTR_RO_MASK) >> SC_ODMA_REG_U_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_U_SB_FULL_MASK) >> SC_ODMA_REG_U_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_U_SB_EMPTY_MASK) >> SC_ODMA_REG_U_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_U_SB_DPTR_RO_MASK) >> SC_ODMA_REG_U_SB_DPTR_RO_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    v_sb_wptr_ro=%d, v_sb_full=%d, v_sb_empty=%d, v_sb_dptr_ro=%d\n",
		(val & SC_ODMA_REG_V_SB_WPTR_RO_MASK) >> SC_ODMA_REG_V_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_V_SB_FULL_MASK) >> SC_ODMA_REG_V_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_V_SB_EMPTY_MASK) >> SC_ODMA_REG_V_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_V_SB_DPTR_RO_MASK) >> SC_ODMA_REG_V_SB_DPTR_RO_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_Y_STAT);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SB_REG_Y_STAT=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    y_sb_wptr_ro=%d, y_sb_full=%d, y_sb_empty=%d, y_sb_dptr_ro=%d, sb_full=%d\n",
		(val & SC_ODMA_REG_Y_SB_WPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_Y_SB_FULL_MASK) >> SC_ODMA_REG_Y_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_Y_SB_EMPTY_MASK) >> SC_ODMA_REG_Y_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_Y_SB_DPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_DPTR_RO_OFFSET,
		(val & SC_ODMA_REG_SB_FULL_MASK) >> SC_ODMA_REG_SB_FULL_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SC_CSC_REG_00);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_00=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    csc_en=%d, csc_q_mode=%d, csc_q_drop=%d, hsv_en=%d, hsv_floor_en=%d\n",
		(val & SC_ODMA_REG_SC_CSC_EN_MASK) >> SC_ODMA_REG_SC_CSC_EN_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_Q_MODE_MASK) >> SC_ODMA_REG_SC_CSC_Q_MODE_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_Q_DROP_MASK) >> SC_ODMA_REG_SC_CSC_Q_DROP_OFFSET,
		(val & SC_ODMA_REG_SC_HSV_EN_MASK) >> SC_ODMA_REG_SC_HSV_EN_OFFSET,
		(val & SC_ODMA_REG_SC_HSV_FLOOR_EN_MASK) >> SC_ODMA_REG_SC_HSV_FLOOR_EN_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    csc_bd_dis=%d, hsv_bd_dis=%d, bf16_bd_type=%d, csc_q_gain_mode=%d\n",
		(val & SC_ODMA_REG_SC_CSC_BD_DIS_MASK) >> SC_ODMA_REG_SC_CSC_BD_DIS_OFFSET,
		(val & SC_ODMA_REG_SC_HSV_BD_DIS_MASK) >> SC_ODMA_REG_SC_HSV_BD_DIS_OFFSET,
		(val & SC_ODMA_REG_BF16_BD_TYPE_MASK) >> SC_ODMA_REG_BF16_BD_TYPE_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_Q_GAIN_MODE_MASK) >> SC_ODMA_REG_SC_CSC_Q_GAIN_MODE_OFFSET);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_01=0x%08x\n", _reg_read(odma_base + SC_ODMA_SC_CSC_REG_01));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_02=0x%08x\n", _reg_read(odma_base + SC_ODMA_SC_CSC_REG_02));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_03=0x%08x\n", _reg_read(odma_base + SC_ODMA_SC_CSC_REG_03));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_04=0x%08x\n", _reg_read(odma_base + SC_ODMA_SC_CSC_REG_04));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_05=0x%08x\n", _reg_read(odma_base + SC_ODMA_SC_CSC_REG_05));

	val = _reg_read(odma_base + SC_ODMA_SC_CSC_REG_06);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_06=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_csc_r2y_add_0=0x%x, sc_csc_r2y_add_1=0x%x, sc_csc_r2y_add_2=0x%x\n",
		(val & SC_ODMA_REG_SC_CSC_R2Y_ADD_0_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_ADD_0_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_R2Y_ADD_1_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_ADD_1_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_R2Y_ADD_2_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_ADD_2_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SC_CSC_REG_07);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_07=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_csc_r2y_frac_0=0x%x, sc_csc_r2y_frac_1=0x%x\n",
		(val & SC_ODMA_REG_SC_CSC_R2Y_FRAC_0_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_FRAC_0_OFFSET,
		(val & SC_ODMA_REG_SC_CSC_R2Y_FRAC_1_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_FRAC_1_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SC_CSC_REG_08);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "SC_ODMA_SC_CSC_REG_08=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    sc_csc_r2y_frac_2=0x%x\n",
		(val & SC_ODMA_REG_SC_CSC_R2Y_FRAC_2_MASK) >> SC_ODMA_REG_SC_CSC_R2Y_FRAC_2_OFFSET);
}

void sclr_show_odma_status(u8 inst)
{
	u32 val, latched_line_cnt = 0;
	uintptr_t odma_base = reg_base + REG_SCL_ODMA_BASE(inst);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "sc(%d) odma\n", inst);

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_13);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_ODMA_ODMA_REG_13=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    reg_dma_debug=0x%x\n",
		(val & SC_ODMA_REG_DMA_DEBUG_MASK) >> SC_ODMA_REG_DMA_DEBUG_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_axi_cmd_cs [0]=%d, [1]=%d, [2]=%d, [3]=%d\n",
		val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_v_buf_empty=%d, sc_odma_v_buf_full=%d\n",
		(val >> 4) & 1, (val >> 5) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_u_buf_empty=%d, sc_odma_u_buf_full=%d\n",
		(val >> 6) & 1, (val >> 7) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_y_buf_empty=%d, sc_odma_y_buf_full=%d\n",
		(val >> 8) & 1, (val >> 9) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_axi_v_active=%d, sc_odma_axi_u_active=%d\n",
		(val >> 10) & 1, (val >> 11) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sc_odma_axi_y_active=%d, sc_odma_axi_active=%d\n",
		(val >> 12) & 1, (val >> 13) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    reg_v_sb_empty=%d, reg_v_sb_full=%d\n",
		(val >> 14) & 1, (val >> 15) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    reg_u_sb_empty=%d, reg_u_sb_full=%d\n",
		(val >> 16) & 1, (val >> 17) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    reg_y_sb_empty=%d, reg_y_sb_full=%d\n",
		(val >> 18) & 1, (val >> 19) & 1);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    reg_sb_full=%d\n",
		(val >> 20) & 1);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_CTRL);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_ODMA_SB_REG_CTRL=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    sb_mode=%d, sb_size=%d, sb_nb=%d, sb_full_nb=%d, sb_sw_wptr=%d\n",
		(val & SC_ODMA_REG_SB_MODE_MASK) >> SC_ODMA_REG_SB_MODE_OFFSET,
		(val & SC_ODMA_REG_SB_SIZE_MASK) >> SC_ODMA_REG_SB_SIZE_OFFSET,
		(val & SC_ODMA_REG_SB_NB_MASK) >> SC_ODMA_REG_SB_NB_OFFSET,
		(val & SC_ODMA_REG_SB_FULL_NB_MASK) >> SC_ODMA_REG_SB_FULL_NB_OFFSET,
		(val & SC_ODMA_REG_SB_SW_WPTR_MASK) >> SC_ODMA_REG_SB_SW_WPTR_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_C_STAT);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_ODMA_SB_REG_C_STAT=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    u_sb_wptr_ro=%d, u_sb_full=%d, u_sb_empty=%d, u_sb_dptr_ro=%d\n",
		(val & SC_ODMA_REG_U_SB_WPTR_RO_MASK) >> SC_ODMA_REG_U_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_U_SB_FULL_MASK) >> SC_ODMA_REG_U_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_U_SB_EMPTY_MASK) >> SC_ODMA_REG_U_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_U_SB_DPTR_RO_MASK) >> SC_ODMA_REG_U_SB_DPTR_RO_OFFSET);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    v_sb_wptr_ro=%d, v_sb_full=%d, v_sb_empty=%d, v_sb_dptr_ro=%d\n",
		(val & SC_ODMA_REG_V_SB_WPTR_RO_MASK) >> SC_ODMA_REG_V_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_V_SB_FULL_MASK) >> SC_ODMA_REG_V_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_V_SB_EMPTY_MASK) >> SC_ODMA_REG_V_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_V_SB_DPTR_RO_MASK) >> SC_ODMA_REG_V_SB_DPTR_RO_OFFSET);

	val = _reg_read(odma_base + SC_ODMA_SB_REG_Y_STAT);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_ODMA_SB_REG_Y_STAT=0x%08x\n", val);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "    y_sb_wptr_ro=%d, y_sb_full=%d, y_sb_empty=%d, y_sb_dptr_ro=%d, sb_full=%d\n",
		(val & SC_ODMA_REG_Y_SB_WPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_WPTR_RO_OFFSET,
		(val & SC_ODMA_REG_Y_SB_FULL_MASK) >> SC_ODMA_REG_Y_SB_FULL_OFFSET,
		(val & SC_ODMA_REG_Y_SB_EMPTY_MASK) >> SC_ODMA_REG_Y_SB_EMPTY_OFFSET,
		(val & SC_ODMA_REG_Y_SB_DPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_DPTR_RO_OFFSET,
		(val & SC_ODMA_REG_SB_FULL_MASK) >> SC_ODMA_REG_SB_FULL_OFFSET);

	_reg_write(odma_base + SC_ODMA_ODMA_REG_16, 0x1);
	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_16);
	latched_line_cnt = val >> 8;
	CVI_TRACE_VPSS(CVI_DBG_INFO, "SC_ODMA_REG_16=0x%08x, latched_line_cnt=%d\n", val, latched_line_cnt);
}

void sclr_dump_disp_register(void)
{
	uintptr_t disp_base = reg_base + REG_SCL_DISP_BASE;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "disp\n");
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_00=0x%08x\n", _reg_read(disp_base + DISP_REG_00));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_01=0x%08x\n", _reg_read(disp_base + DISP_REG_01));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_02=0x%08x\n", _reg_read(disp_base + DISP_REG_02));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_03=0x%08x\n", _reg_read(disp_base + DISP_REG_03));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_04=0x%08x\n", _reg_read(disp_base + DISP_REG_04));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_05=0x%08x\n", _reg_read(disp_base + DISP_REG_05));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_06=0x%08x\n", _reg_read(disp_base + DISP_REG_06));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_07=0x%08x\n", _reg_read(disp_base + DISP_REG_07));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_08=0x%08x\n", _reg_read(disp_base + DISP_REG_08));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_12=0x%08x\n", _reg_read(disp_base + DISP_REG_12));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_13=0x%08x\n", _reg_read(disp_base + DISP_REG_13));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_14=0x%08x\n", _reg_read(disp_base + DISP_REG_14));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_15=0x%08x\n", _reg_read(disp_base + DISP_REG_15));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_16=0x%08x\n", _reg_read(disp_base + DISP_REG_16));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_17=0x%08x\n", _reg_read(disp_base + DISP_REG_17));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_18=0x%08x\n", _reg_read(disp_base + DISP_REG_18));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_19=0x%08x\n", _reg_read(disp_base + DISP_REG_19));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_20=0x%08x\n", _reg_read(disp_base + DISP_REG_20));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_21=0x%08x\n", _reg_read(disp_base + DISP_REG_21));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_22=0x%08x\n", _reg_read(disp_base + DISP_REG_22));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_23=0x%08x\n", _reg_read(disp_base + DISP_REG_23));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_24=0x%08x\n", _reg_read(disp_base + DISP_REG_24));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_25=0x%08x\n", _reg_read(disp_base + DISP_REG_25));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_26=0x%08x\n", _reg_read(disp_base + DISP_REG_26));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_27=0x%08x\n", _reg_read(disp_base + DISP_REG_27));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_28=0x%08x\n", _reg_read(disp_base + DISP_REG_28));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_29=0x%08x\n", _reg_read(disp_base + DISP_REG_29));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_30=0x%08x\n", _reg_read(disp_base + DISP_REG_30));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_31=0x%08x\n", _reg_read(disp_base + DISP_REG_31));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_32=0x%08x\n", _reg_read(disp_base + DISP_REG_32));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_33=0x%08x\n", _reg_read(disp_base + DISP_REG_33));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_34=0x%08x\n", _reg_read(disp_base + DISP_REG_34));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_35=0x%08x\n", _reg_read(disp_base + DISP_REG_35));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_36=0x%08x\n", _reg_read(disp_base + DISP_REG_36));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_37=0x%08x\n", _reg_read(disp_base + DISP_REG_37));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_38=0x%08x\n", _reg_read(disp_base + DISP_REG_38));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_39=0x%08x\n", _reg_read(disp_base + DISP_REG_39));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_40=0x%08x\n", _reg_read(disp_base + DISP_REG_40));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_41=0x%08x\n", _reg_read(disp_base + DISP_REG_41));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_42=0x%08x\n", _reg_read(disp_base + DISP_REG_42));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_43=0x%08x\n", _reg_read(disp_base + DISP_REG_43));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_AXI_ST=0x%08x\n", _reg_read(disp_base + DISP_REG_AXI_ST));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_CATCH=0x%08x\n", _reg_read(disp_base + DISP_REG_CATCH));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_CHK_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_CHK_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_CHK_RD_OSD=0x%08x\n", _reg_read(disp_base + DISP_CHK_RD_OSD));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_CHK_RD_IMG=0x%08x\n", _reg_read(disp_base + DISP_CHK_RD_IMG));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_3E=0x%08x\n", _reg_read(disp_base + DISP_REG_3E));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_3F=0x%08x\n", _reg_read(disp_base + DISP_REG_3F));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_IMG_BWL=0x%08x\n", _reg_read(disp_base + DISP_REG_IMG_BWL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_GAMMA_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_GAMMA_WR_LUT=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_WR_LUT));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_GAMMA_RD_LUT=0x%08x\n", _reg_read(disp_base + DISP_REG_GAMMA_RD_LUT));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_MCU_IF_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_IF_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_MCU_SW_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_SW_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_MCU_STATUS=0x%08x\n", _reg_read(disp_base + DISP_REG_MCU_STATUS));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_AUTO=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_AUTO));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_0=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_0));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_1=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_1));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_2=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_2));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_3=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_3));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_4=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_4));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_5=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_5));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_6=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_6));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_CMD_7=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_CMD_7));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_HW_MCU_OV=0x%08x\n", _reg_read(disp_base + DISP_REG_HW_MCU_OV));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_SRGB_CTRL=0x%08x\n", _reg_read(disp_base + DISP_REG_SRGB_CTRL));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_SIZE=0x%08x\n",
			_reg_read(disp_base + DISP_REG_TGEN_LITE_SIZE));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_VS=0x%08x\n", _reg_read(disp_base + DISP_REG_TGEN_LITE_VS));
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "DISP_REG_TGEN_LITE_HS=0x%08x\n", _reg_read(disp_base + DISP_REG_TGEN_LITE_HS));
}

void sclr_dump_gop_register(int inst)
{
	u32 val, i, j;
	uintptr_t gop_base = reg_base + REG_SCL_GOP0_BASE(inst);

	if (inst >= SCL_MAX_INST)
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	for (i = 0; i < SCL_MAX_GOP_INST; i++) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) gop%d\n", inst, i);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d enable =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_80);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_80=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow0_en=%d, reg_ow1_en=%d, reg_ow2_en=%d, reg_ow3_en=%d\n",
			       (val & VGOP_REG_OW0_EN_MASK) >> VGOP_REG_OW0_EN_OFFSET,
			       (val & VGOP_REG_OW1_EN_MASK) >> VGOP_REG_OW1_EN_OFFSET,
			       (val & VGOP_REG_OW2_EN_MASK) >> VGOP_REG_OW2_EN_OFFSET,
			       (val & VGOP_REG_OW3_EN_MASK) >> VGOP_REG_OW3_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow4_en=%d, reg_ow5_en=%d, reg_ow6_en=%d, reg_ow7_en=%d\n",
			       (val & VGOP_REG_OW0_EN_MASK) >> VGOP_REG_OW4_EN_OFFSET,
			       (val & VGOP_REG_OW1_EN_MASK) >> VGOP_REG_OW5_EN_OFFSET,
			       (val & VGOP_REG_OW2_EN_MASK) >> VGOP_REG_OW6_EN_OFFSET,
			       (val & VGOP_REG_OW3_EN_MASK) >> VGOP_REG_OW3_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_vgop_hscal=%d, reg_vgop_vscal=%d, reg_clr_key_en=%d\n",
			       (val & VGOP_REG_VGOP_HSCAL_MASK) >> VGOP_REG_VGOP_HSCAL_OFFSET,
			       (val & VGOP_REG_VGOP_VSCAL_MASK) >> VGOP_REG_VGOP_VSCAL_OFFSET,
			       (val & VGOP_REG_CLR_KEY_EN_MASK) >> VGOP_REG_CLR_KEY_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_vgop_arlen=%d, reg_vgop_sw_rst=%d\n",
			       (val & VGOP_REG_VGOP_ARLEN_MASK) >> VGOP_REG_VGOP_ARLEN_OFFSET,
			       (val & VGOP_REG_VGOP_SW_RST_MASK) >> VGOP_REG_VGOP_SW_RST_OFFSET);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d OW =========================\n", i);
		for (j = 0; j < SCL_MAX_GOP_OW_INST; j++) {
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_0 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_0(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_format=0x%02x\n", j,
				       (val & VGOP_REG_OW0_FORMAT_MASK) >> VGOP_REG_OW0_FORMAT_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_1 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_1(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_h_start=%d, reg_ow%d_h_end=%d\n",
				       j, (val & VGOP_REG_OW0_H_START_MASK) >> VGOP_REG_OW0_H_START_OFFSET,
				       j, (val & VGOP_REG_OW0_H_END_MASK) >> VGOP_REG_OW0_H_END_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_2 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_2(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_v_start=%d, reg_ow%d_v_end=%d\n",
				       j, (val & VGOP_REG_OW0_V_START_MASK) >> VGOP_REG_OW0_V_START_OFFSET,
				       j, (val & VGOP_REG_OW0_V_END_MASK) >> VGOP_REG_OW0_V_END_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_3 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_3(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_dram_str_adr_l = 0x%08x\n", j,
				       (val & VGOP_REG_OW0_DRAM_STR_ADR_L_MASK) >> VGOP_REG_OW0_DRAM_STR_ADR_L_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_4 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_4(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_dram_str_adr_h = 0x%08x\n", j,
				       (val & VGOP_REG_OW0_DRAM_STR_ADR_H_MASK) >> VGOP_REG_OW0_DRAM_STR_ADR_H_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_5 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_5(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_dram_strip = %d, reg_ow%d_crop_pixels = %d\n",
				       j, (val & VGOP_REG_OW0_DRAM_STRIP_MASK) >> (VGOP_REG_OW0_DRAM_STRIP_OFFSET - 4),
				       j, (val & VGOP_REG_OW0_CROP_PIXELS_MASK) >> VGOP_REG_OW0_CROP_PIXELS_OFFSET);
			val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_6 + j * 0x20);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_6(ow%d)=0x%08x\n", j, val);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow%d_dram_hsize = %d, reg_ow%d_dram_vsize = %d\n",
				       j, (val & VGOP_REG_OW0_DRAM_HSIZE_MASK) >> (VGOP_REG_OW0_DRAM_HSIZE_OFFSET - 4),
				       j, (val & VGOP_REG_OW0_DRAM_VSIZE_MASK) >> VGOP_REG_OW0_DRAM_VSIZE_OFFSET);
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d Ctrl =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_83);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_83=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_clr_key=0x%08x\n",
			       (val & VGOP_REG_CLR_KEY_MASK) >> VGOP_REG_CLR_KEY_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_84);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_84=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_const_argb0=0x%04x, reg_const_argb1=0x%04x\n",
			       (val & VGOP_REG_CONST_ARGB0_MASK) >> VGOP_REG_CONST_ARGB0_OFFSET,
			       (val & VGOP_REG_CONST_ARGB1_MASK) >> VGOP_REG_CONST_ARGB1_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_85);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_85=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_vgop_debug=0x%08x\n",
			       (val & VGOP_REG_VGOP_DEBUG_MASK) >> VGOP_REG_VGOP_DEBUG_OFFSET);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d FB =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_86);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_86=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb_clr_hi_thr=%d, reg_fb_clr_lo_thr=%d\n",
			       (val & VGOP_REG_FB_CLR_HI_THR_MASK) >> VGOP_REG_FB_CLR_HI_THR_OFFSET,
			       (val & VGOP_REG_FB_CLR_LO_THR_MASK) >> VGOP_REG_FB_CLR_LO_THR_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb_init=%d, reg_fb_font_is_dark=%d\n",
			       (val & VGOP_REG_FB_INIT_MASK) >> VGOP_REG_FB_INIT_OFFSET,
			       (val & VGOP_REG_FB_FONT_IS_DARK_MASK) >> VGOP_REG_FB_FONT_IS_DARK_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb_diff_fnum=%d\n",
			       (val & VGOP_REG_FB_DIFF_FNUM_MASK) >> VGOP_REG_FB_DIFF_FNUM_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_87);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_87=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb0_width=%d, reg_fb0_pix_thr=%d\n",
			       (val & VGOP_REG_FB0_WIDTH_MASK) >> VGOP_REG_FB0_WIDTH_OFFSET,
			       (val & VGOP_REG_FB0_PIX_THR_MASK) >> VGOP_REG_FB0_PIX_THR_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb0_sample_rate=%d, reg_fb0_num=%d\n",
			       (val & VGOP_REG_FB0_SAMPLE_RATE_MASK) >> VGOP_REG_FB0_SAMPLE_RATE_OFFSET,
			       (val & VGOP_REG_FB0_NUM_MASK) >> VGOP_REG_FB0_NUM_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb0_attached_idx=%d, reg_fb0_en=%d\n",
			       (val & VGOP_REG_FB0_ATTACHED_IDX_MASK) >> VGOP_REG_FB0_ATTACHED_IDX_OFFSET,
			       (val & VGOP_REG_FB0_EN_MASK) >> VGOP_REG_FB0_EN_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_88);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_88=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb0_init_st=0x%08x\n",
			       (val & VGOP_REG_FB0_INIT_ST_MASK) >> VGOP_REG_FB0_INIT_ST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_89);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_89=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb0_record=0x%08x\n",
			       (val & VGOP_REG_FB0_RECORD_MASK) >> VGOP_REG_FB0_RECORD_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_90);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_90=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb1_width=%d, reg_fb1_pix_thr=%d\n",
			       (val & VGOP_REG_FB1_WIDTH_MASK) >> VGOP_REG_FB1_WIDTH_OFFSET,
			       (val & VGOP_REG_FB1_PIX_THR_MASK) >> VGOP_REG_FB1_PIX_THR_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb1_sample_rate=%d, reg_fb1_num=%d\n",
			       (val & VGOP_REG_FB1_SAMPLE_RATE_MASK) >> VGOP_REG_FB1_SAMPLE_RATE_OFFSET,
			       (val & VGOP_REG_FB1_NUM_MASK) >> VGOP_REG_FB1_NUM_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb1_attached_idx=%d, reg_fb1_en=%d\n",
			       (val & VGOP_REG_FB1_ATTACHED_IDX_MASK) >> VGOP_REG_FB1_ATTACHED_IDX_OFFSET,
			       (val & VGOP_REG_FB1_EN_MASK) >> VGOP_REG_FB1_EN_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_91);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_91=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb1_init_st=0x%08x\n",
			       (val & VGOP_REG_FB1_INIT_ST_MASK) >> VGOP_REG_FB1_INIT_ST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_92);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_92=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_fb1_record=0x%08x\n",
			       (val & VGOP_REG_FB1_RECORD_MASK) >> VGOP_REG_FB1_RECORD_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_BW_LIMIT);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_BW_LIMIT=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_bwl_win=%d, reg_bwl_vld=%d, reg_bwl_en=%d\n",
			       (val & VGOP_REG_BWL_WIN_MASK) >> VGOP_REG_BWL_WIN_OFFSET,
			       (val & VGOP_REG_BWL_VLD_MASK) >> VGOP_REG_BWL_VLD_OFFSET,
			       (val & VGOP_REG_BWL_EN_MASK) >> VGOP_REG_BWL_EN_OFFSET);

		//ODEC
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d ODEC =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_DEC_00);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_DEC_00=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG,
			       "    reg_odec_en=%d, reg_odec_int_en=%d, reg_odec_int_clr=%d, reg_odec_wdt_en=%d\n",
			       (val & VGOP_REG_ODEC_EN_MASK) >> VGOP_REG_ODEC_EN_OFFSET,
			       (val & VGOP_REG_ODEC_INT_EN_MASK) >> VGOP_REG_ODEC_INT_EN_OFFSET,
			       (val & VGOP_REG_ODEC_INT_CLR_MASK) >> VGOP_REG_ODEC_INT_CLR_OFFSET,
			       (val & VGOP_REG_ODEC_WDT_EN_MASK) >> VGOP_REG_ODEC_WDT_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG,
			       "    reg_odec_dbg_ridx=0x%04x, reg_odec_done=%d, reg_odec_attached_idx=%d\n",
			       (val & VGOP_REG_ODEC_DBG_RIDX_MASK) >> VGOP_REG_ODEC_DBG_RIDX_OFFSET,
			       (val & VGOP_REG_ODEC_DONE_MASK) >> VGOP_REG_ODEC_DONE_OFFSET,
			       (val & VGOP_REG_ODEC_ATTACHED_IDX_MASK) >> VGOP_REG_ODEC_ATTACHED_IDX_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_odec_wdt_fdiv_bit=%d, reg_odec_int_vec=0x%08x\n",
			       (val & VGOP_REG_ODEC_INT_VEC_MASK) >> VGOP_REG_ODEC_INT_VEC_OFFSET,
			       (val & VGOP_REG_ODEC_WDT_FDIV_BIT_MASK) >> VGOP_REG_ODEC_WDT_FDIV_BIT_OFFSET);

		//ODEC_DBG
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_DEC_01);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_DEC_01=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_odec_dbg_rdata=0x%08x\n",
			       (val & VGOP_REG_ODEC_DBG_RDATA_MASK) >> VGOP_REG_ODEC_DBG_RDATA_OFFSET);

		//LUT-4
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "========================= GOP%d LUT16 =========================\n", i);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_0);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_0=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt0=0x%04x, reg_lut16_cnt1=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT0_MASK) >> VGOP_REG_LUT16_CNT0_OFFSET,
			       (val & VGOP_REG_LUT16_CNT1_MASK) >> VGOP_REG_LUT16_CNT1_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_1);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_1=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt2=0x%04x, reg_lut16_cnt3=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT2_MASK) >> VGOP_REG_LUT16_CNT2_OFFSET,
			       (val & VGOP_REG_LUT16_CNT3_MASK) >> VGOP_REG_LUT16_CNT3_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_2);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_2=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt4=0x%04x, reg_lut16_cnt5=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT4_MASK) >> VGOP_REG_LUT16_CNT4_OFFSET,
			       (val & VGOP_REG_LUT16_CNT5_MASK) >> VGOP_REG_LUT16_CNT5_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_3);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_3=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt6=0x%04x, reg_lut16_cnt7=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT6_MASK) >> VGOP_REG_LUT16_CNT6_OFFSET,
			       (val & VGOP_REG_LUT16_CNT7_MASK) >> VGOP_REG_LUT16_CNT7_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_4);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_4=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt8=0x%04x, reg_lut16_cnt9=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT8_MASK) >> VGOP_REG_LUT16_CNT8_OFFSET,
			       (val & VGOP_REG_LUT16_CNT9_MASK) >> VGOP_REG_LUT16_CNT9_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_5);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_5=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt10=0x%04x, reg_lut16_cnt11=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT10_MASK) >> VGOP_REG_LUT16_CNT10_OFFSET,
			       (val & VGOP_REG_LUT16_CNT11_MASK) >> VGOP_REG_LUT16_CNT11_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_6);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_6=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt12=0x%04x, reg_lut16_cnt13=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT12_MASK) >> VGOP_REG_LUT16_CNT12_OFFSET,
			       (val & VGOP_REG_LUT16_CNT13_MASK) >> VGOP_REG_LUT16_CNT13_OFFSET);
		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_LUT16_7);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_LUT16_7=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_lut16_cnt14=0x%04x, reg_lut16_cnt15=0x%04x\n",
			       (val & VGOP_REG_LUT16_CNT14_MASK) >> VGOP_REG_LUT16_CNT14_OFFSET,
			       (val & VGOP_REG_LUT16_CNT15_MASK) >> VGOP_REG_LUT16_CNT15_OFFSET);
	}
}

void sclr_show_gop_status(int inst)
{
	u32 val, i;
	uintptr_t gop_base = reg_base + REG_SCL_GOP0_BASE(inst);

	if (inst >= SCL_MAX_INST)
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) out of range\n", inst);

	for (i = 0; i < SCL_MAX_GOP_INST; i++) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc(%d) gop%d\n", inst, i);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_REG_80);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_REG_80=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow0_en=%d, reg_ow1_en=%d, reg_ow2_en=%d, reg_ow3_en=%d\n",
			       (val & VGOP_REG_OW0_EN_MASK) >> VGOP_REG_OW0_EN_OFFSET,
			       (val & VGOP_REG_OW1_EN_MASK) >> VGOP_REG_OW1_EN_OFFSET,
			       (val & VGOP_REG_OW2_EN_MASK) >> VGOP_REG_OW2_EN_OFFSET,
			       (val & VGOP_REG_OW3_EN_MASK) >> VGOP_REG_OW3_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_ow4_en=%d, reg_ow5_en=%d, reg_ow6_en=%d, reg_ow7_en=%d\n",
			       (val & VGOP_REG_OW0_EN_MASK) >> VGOP_REG_OW4_EN_OFFSET,
			       (val & VGOP_REG_OW1_EN_MASK) >> VGOP_REG_OW5_EN_OFFSET,
			       (val & VGOP_REG_OW2_EN_MASK) >> VGOP_REG_OW6_EN_OFFSET,
			       (val & VGOP_REG_OW3_EN_MASK) >> VGOP_REG_OW3_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_vgop_hscal=%d, reg_vgop_vscal=%d, reg_clr_key_en=%d\n",
			       (val & VGOP_REG_VGOP_HSCAL_MASK) >> VGOP_REG_VGOP_HSCAL_OFFSET,
			       (val & VGOP_REG_VGOP_VSCAL_MASK) >> VGOP_REG_VGOP_VSCAL_OFFSET,
			       (val & VGOP_REG_CLR_KEY_EN_MASK) >> VGOP_REG_CLR_KEY_EN_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_vgop_arlen=%d, reg_vgop_sw_rst=%d\n",
			       (val & VGOP_REG_VGOP_ARLEN_MASK) >> VGOP_REG_VGOP_ARLEN_OFFSET,
			       (val & VGOP_REG_VGOP_SW_RST_MASK) >> VGOP_REG_VGOP_SW_RST_OFFSET);

		val = _reg_read(gop_base + i * 0x200 + VGOP_VGOP_DEC_00);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "VGOP_VGOP_DEC_00=0x%08x\n", val);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG,
			       "    reg_odec_en=%d, reg_odec_int_en=%d, reg_odec_int_clr=%d, reg_odec_wdt_en=%d\n",
			       (val & VGOP_REG_OW0_EN_MASK) >> VGOP_REG_ODEC_EN_OFFSET,
			       (val & VGOP_REG_ODEC_INT_EN_MASK) >> VGOP_REG_ODEC_INT_EN_OFFSET,
			       (val & VGOP_REG_ODEC_INT_CLR_MASK) >> VGOP_REG_ODEC_INT_CLR_OFFSET,
			       (val & VGOP_REG_ODEC_WDT_EN_MASK) >> VGOP_REG_OW2_EN_MASK);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG,
			       "    reg_odec_dbg_ridx=0x%04x, reg_odec_done=%d, reg_odec_attached_idx=%d\n",
			       (val & VGOP_REG_ODEC_DBG_RIDX_MASK) >> VGOP_REG_ODEC_DBG_RIDX_OFFSET,
			       (val & VGOP_REG_ODEC_DONE_MASK) >> VGOP_REG_ODEC_DONE_OFFSET,
			       (val & VGOP_REG_ODEC_ATTACHED_IDX_MASK) >> VGOP_REG_ODEC_ATTACHED_IDX_OFFSET);
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "    reg_odec_wdt_fdiv_bit=%d, reg_odec_int_vec=0x%08x\n",
			       (val & VGOP_REG_ODEC_INT_VEC_MASK) >> VGOP_REG_ODEC_INT_VEC_OFFSET,
			       (val & VGOP_REG_ODEC_WDT_FDIV_BIT_MASK) >> VGOP_REG_ODEC_WDT_FDIV_BIT_OFFSET);
	}
}

void sclr_check_register(void)
{
	u32 val;
	uintptr_t top_base = reg_base + REG_SCL_TOP_BASE;
	u8 sc_d_en, sc_v1_en, sc_v2_en, sc_v3_en, sc_d_src_sel;

	val = _reg_read(top_base + SC_TOP_REG_01);
	sc_d_en = (val & SC_TOP_REG_SC_D_EN_MASK) >> SC_TOP_REG_SC_D_EN_OFFSET;
	sc_v1_en = (val & SC_TOP_REG_SC_V1_EN_MASK) >> SC_TOP_REG_SC_V1_EN_OFFSET;
	sc_v2_en = (val & SC_TOP_REG_SC_V2_EN_MASK) >> SC_TOP_REG_SC_V2_EN_OFFSET;
	sc_v3_en = (val & SC_TOP_REG_SC_V3_EN_MASK) >> SC_TOP_REG_SC_V3_EN_OFFSET;
	sc_d_src_sel = (val & SC_TOP_REG_SC_D_SRC_SEL_MASK) >> SC_TOP_REG_SC_D_SRC_SEL_OFFSET;

	sclr_show_top_status();

	// img_in_d
	if (sc_d_en && !sc_d_src_sel)
		sclr_show_img_in_status(1);

	// img_in_v
	if ((sc_d_en && sc_d_src_sel) || (sc_v1_en || sc_v2_en || sc_v3_en))
		sclr_show_img_in_status(0);

	if (sc_d_en) {
		sclr_show_core_status(0);
		sclr_show_odma_status(0);
		sclr_show_gop_status(0);
	}

	if (sc_v1_en) {
		sclr_show_core_status(1);
		sclr_show_odma_status(1);
		sclr_show_gop_status(1);
	}

	if (sc_v2_en) {
		sclr_show_core_status(2);
		sclr_show_odma_status(2);
		sclr_show_gop_status(2);
	}

	if (sc_v3_en) {
		sclr_show_core_status(3);
		sclr_show_odma_status(3);
		sclr_show_gop_status(3);
	}
}
#else
void sclr_check_register(void)
{
}
#endif

void sclr_check_overflow_reg(struct cvi_vpss_info *vpss_info)
{
	uintptr_t top_base = reg_base + REG_SCL_TOP_BASE;
	uintptr_t odma_base = 0;
	u32 val, rdy;
	u8 sc_en[SCL_MAX_INST] = {0}, i;

	val = _reg_read(top_base + SC_TOP_REG_01);
	sc_en[0] = (val & SC_TOP_REG_SC_D_EN_MASK) >> SC_TOP_REG_SC_D_EN_OFFSET;
	sc_en[1] = (val & SC_TOP_REG_SC_V1_EN_MASK) >> SC_TOP_REG_SC_V1_EN_OFFSET;
	sc_en[2] = (val & SC_TOP_REG_SC_V2_EN_MASK) >> SC_TOP_REG_SC_V2_EN_OFFSET;
#ifdef  __SOC_MARS__
	sc_en[3] = (val & SC_TOP_REG_SC_V3_EN_MASK) >> SC_TOP_REG_SC_V3_EN_OFFSET;
#endif

	val = _reg_read(top_base + SC_TOP_REG_DBG_VALID);
	rdy = _reg_read(top_base + SC_TOP_REG_DBG_READY);
	vpss_info->sc_top.isp2ip_y_in[0] = (val >> 0) & 1;
	vpss_info->sc_top.isp2ip_y_in[1] = (rdy >> 0) & 1;
	vpss_info->sc_top.isp2ip_u_in[0] = (val >> 1) & 1;
	vpss_info->sc_top.isp2ip_u_in[1] = (rdy >> 1) & 1;
	vpss_info->sc_top.isp2ip_v_in[0] = (val >> 2) & 1;
	vpss_info->sc_top.isp2ip_v_in[1] = (rdy >> 2) & 1;
	vpss_info->sc_top.img_d_out[0]	 = (val >> 3) & 1;
	vpss_info->sc_top.img_d_out[1]	 = (rdy >> 3) & 1;
	vpss_info->sc_top.img_v_out[0]	 = (val >> 4) & 1;
	vpss_info->sc_top.img_v_out[1]	 = (rdy >> 4) & 1;
	vpss_info->sc_top.bld_sa[0]	 = (val >> 5) & 1;
	vpss_info->sc_top.bld_sa[1]	 = (rdy >> 5) & 1;
	vpss_info->sc_top.bld_sb[0]	 = (val >> 6) & 1;
	vpss_info->sc_top.bld_sb[1]	 = (rdy >> 6) & 1;
	vpss_info->sc_top.bld_m[0]	 = (val >> 7) & 1;
	vpss_info->sc_top.bld_m[1]	 = (rdy >> 7) & 1;
	vpss_info->sc_top.pri_sp[0]	 = (val >> 8) & 1;
	vpss_info->sc_top.pri_sp[1]	 = (rdy >> 8) & 1;
	vpss_info->sc_top.pri_m[0]	 = (val >> 9) & 1;
	vpss_info->sc_top.pri_m[1]	 = (rdy >> 9) & 1;
	vpss_info->sc_top.sc_d[0]	 = (val >> 10) & 1;
	vpss_info->sc_top.sc_d[1]	 = (rdy >> 10) & 1;
	vpss_info->sc_top.sc_v1[0]	 = (val >> 11) & 1;
	vpss_info->sc_top.sc_v1[1]	 = (rdy >> 11) & 1;
	vpss_info->sc_top.sc_v2[0]	 = (val >> 12) & 1;
	vpss_info->sc_top.sc_v2[1]	 = (rdy >> 12) & 1;
	vpss_info->sc_top.sc_v3[0]	 = (val >> 13) & 1;
	vpss_info->sc_top.sc_v3[1]	 = (rdy >> 13) & 1;
	vpss_info->sc_top.sc_d_out[0]	 = (val >> 14) & 1;
	vpss_info->sc_top.sc_d_out[1]	 = (rdy >> 14) & 1;

	for (i = 0; i < SCL_MAX_INST; i++) {
		if (sc_en[i]) {
			odma_base = reg_base + REG_SCL_ODMA_BASE(i);
			val = _reg_read(odma_base + SC_ODMA_SB_REG_CTRL);

			if ((val & SC_ODMA_REG_SB_MODE_MASK) >> SC_ODMA_REG_SB_MODE_OFFSET)
				break;
		}
	}

	if (i == SCL_MAX_INST)
		return;

	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_13);
	vpss_info->odma.sc_odma_axi_cmd_cs[0] = val & 1;
	vpss_info->odma.sc_odma_axi_cmd_cs[1] = (val >> 1) & 1;
	vpss_info->odma.sc_odma_axi_cmd_cs[2] = (val >> 2) & 1;
	vpss_info->odma.sc_odma_axi_cmd_cs[3] = (val >> 3) & 1;
	vpss_info->odma.sc_odma_v_buf_empty = (val >> 4) & 1;
	vpss_info->odma.sc_odma_v_buf_full = (val >> 5) & 1;
	vpss_info->odma.sc_odma_u_buf_empty = (val >> 6) & 1;
	vpss_info->odma.sc_odma_u_buf_full = (val >> 7) & 1;
	vpss_info->odma.sc_odma_y_buf_empty = (val >> 8) & 1;
	vpss_info->odma.sc_odma_y_buf_full = (val >> 9) & 1;
	vpss_info->odma.sc_odma_axi_v_active = (val >> 10) & 1;
	vpss_info->odma.sc_odma_axi_u_active = (val >> 11) & 1;
	vpss_info->odma.sc_odma_axi_y_active = (val >> 12) & 1;
	vpss_info->odma.sc_odma_axi_active = (val >> 13) & 1;
	vpss_info->odma.reg_v_sb_empty = (val >> 14) & 1;
	vpss_info->odma.reg_v_sb_full = (val >> 15) & 1;
	vpss_info->odma.reg_u_sb_empty = (val >> 16) & 1;
	vpss_info->odma.reg_u_sb_full = (val >> 17) & 1;
	vpss_info->odma.reg_y_sb_empty = (val >> 18) & 1;
	vpss_info->odma.reg_y_sb_full = (val >> 19) & 1;
	vpss_info->odma.reg_sb_full = (val >> 20) & 1;

	val = _reg_read(odma_base + SC_ODMA_SB_REG_CTRL);
	vpss_info->sb_ctrl.sb_mode = (val & SC_ODMA_REG_SB_MODE_MASK) >> SC_ODMA_REG_SB_MODE_OFFSET;
	vpss_info->sb_ctrl.sb_size = (val & SC_ODMA_REG_SB_SIZE_MASK) >> SC_ODMA_REG_SB_SIZE_OFFSET;
	vpss_info->sb_ctrl.sb_nb = (val & SC_ODMA_REG_SB_NB_MASK) >> SC_ODMA_REG_SB_NB_OFFSET;
	vpss_info->sb_ctrl.sb_full_nb = (val & SC_ODMA_REG_SB_FULL_NB_MASK) >> SC_ODMA_REG_SB_FULL_NB_OFFSET;
	vpss_info->sb_ctrl.sb_sw_wptr = (val & SC_ODMA_REG_SB_SW_WPTR_MASK) >> SC_ODMA_REG_SB_SW_WPTR_OFFSET;

	val = _reg_read(odma_base + SC_ODMA_SB_REG_C_STAT);
	vpss_info->sb_stat.u_sb_wptr_ro = (val & SC_ODMA_REG_U_SB_WPTR_RO_MASK) >> SC_ODMA_REG_U_SB_WPTR_RO_OFFSET;
	vpss_info->sb_stat.u_sb_full = (val & SC_ODMA_REG_U_SB_FULL_MASK) >> SC_ODMA_REG_U_SB_FULL_OFFSET;
	vpss_info->sb_stat.u_sb_empty = (val & SC_ODMA_REG_U_SB_EMPTY_MASK) >> SC_ODMA_REG_U_SB_EMPTY_OFFSET;
	vpss_info->sb_stat.u_sb_dptr_ro = (val & SC_ODMA_REG_U_SB_DPTR_RO_MASK) >> SC_ODMA_REG_U_SB_DPTR_RO_OFFSET;
	vpss_info->sb_stat.v_sb_wptr_ro = (val & SC_ODMA_REG_V_SB_WPTR_RO_MASK) >> SC_ODMA_REG_V_SB_WPTR_RO_OFFSET;
	vpss_info->sb_stat.v_sb_full = (val & SC_ODMA_REG_V_SB_FULL_MASK) >> SC_ODMA_REG_V_SB_FULL_OFFSET;
	vpss_info->sb_stat.v_sb_empty = (val & SC_ODMA_REG_V_SB_EMPTY_MASK) >> SC_ODMA_REG_V_SB_EMPTY_OFFSET;
	vpss_info->sb_stat.v_sb_dptr_ro = (val & SC_ODMA_REG_V_SB_DPTR_RO_MASK) >> SC_ODMA_REG_V_SB_DPTR_RO_OFFSET;

	val = _reg_read(odma_base + SC_ODMA_SB_REG_Y_STAT);
	vpss_info->sb_stat.y_sb_wptr_ro = (val & SC_ODMA_REG_Y_SB_WPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_WPTR_RO_OFFSET;
	vpss_info->sb_stat.y_sb_full = (val & SC_ODMA_REG_Y_SB_FULL_MASK) >> SC_ODMA_REG_Y_SB_FULL_OFFSET;
	vpss_info->sb_stat.y_sb_empty = (val & SC_ODMA_REG_Y_SB_EMPTY_MASK) >> SC_ODMA_REG_Y_SB_EMPTY_OFFSET;
	vpss_info->sb_stat.y_sb_dptr_ro = (val & SC_ODMA_REG_Y_SB_DPTR_RO_MASK) >> SC_ODMA_REG_Y_SB_DPTR_RO_OFFSET;
	vpss_info->sb_stat.sb_full = (val & SC_ODMA_REG_SB_FULL_MASK) >> SC_ODMA_REG_SB_FULL_OFFSET;

	_reg_write(odma_base + SC_ODMA_ODMA_REG_16, 0x1);
	val = _reg_read(odma_base + SC_ODMA_ODMA_REG_16);
	vpss_info->latched_line_cnt = val >> 8;
	vpss_info->sc = i;
	vpss_info->enable = true;
}
