// $Module: ive_match_bg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Tue, 07 Dec 2021 11:00:50 AM $
//

#ifndef __REG_IVE_MATCH_BG_STRUCT_H__
#define __REG_IVE_MATCH_BG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Matchbg enable
		0: disable
		1: enable;*/
		uint32_t reg_matchbg_en:1;
		/*Matchbg bgmodel bypass
		0: normal mode
		1: Read bgmodel from DMA and bypass to Update bg;*/
		uint32_t reg_matchbg_bypass_model:1;
		uint32_t rsv_2_3:2;
		/*Matchbg software reset
		0: normal
		1: reset;*/
		uint32_t reg_matchbg_softrst:1;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_00_C;
typedef union {
	struct {
		/*Current frame timestamp, in frame units;*/
		uint32_t reg_matchbg_curfrmnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_04_C;
typedef union {
	struct {
		/*Previous frame timestamp, in frame units;*/
		uint32_t reg_matchbg_prefrmnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_08_C;
typedef union {
	struct {
		/*Potential background replacement time threshold (range: 2 to 100; default: 20);*/
		uint32_t reg_matchbg_timethr:7;
		uint32_t rsv_7_7:1;
		/*Correlation coefficients between differential threshold and gray value (range: 0 to 5; default: 0);*/
		uint32_t reg_matchbg_diffthrcrlcoef:3;
		uint32_t rsv_11_11:1;
		/*Maximum of background differential threshold (range: 3 to 15; default: 6);*/
		uint32_t reg_matchbg_diffmaxthr:4;
		/*Minimum of background differential threshold (range: 3 to 15; default: 4);*/
		uint32_t reg_matchbg_diffminthr:4;
		/*Dynamic Background differential threshold increment (range: 0 to 6; default: 0);*/
		uint32_t reg_matchbg_diffthrinc:3;
		uint32_t rsv_23_23:1;
		/*Quick background learning rate (range: 0 to 4; default: 2);*/
		uint32_t reg_matchbg_fastlearnrate:3;
		uint32_t rsv_27_27:1;
		/*Whether to detect change region (range: 0(no), 1(yes); default: 0);*/
		uint32_t reg_matchbg_detchgregion:1;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_0C_C;
typedef union {
	struct {
		/*Pixel numbers of fg;*/
		uint32_t reg_matchbg_stat_pixnum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_10_C;
typedef union {
	struct {
		/*Summary of all input pixel luminance;*/
		uint32_t reg_matchbg_stat_sumlum:32;
	};
	uint32_t val;
} IVE_MATCH_BG_REG_14_C;
typedef struct {
	IVE_MATCH_BG_REG_00_C REG_00;
	IVE_MATCH_BG_REG_04_C REG_04;
	IVE_MATCH_BG_REG_08_C REG_08;
	IVE_MATCH_BG_REG_0C_C REG_0C;
	IVE_MATCH_BG_REG_10_C REG_10;
	IVE_MATCH_BG_REG_14_C REG_14;
} IVE_MATCH_BG_C;

#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_MATCH_BG_C \
{\
	.REG_00.reg_matchbg_en = 0x0,\
	.REG_00.reg_matchbg_bypass_model = 0x0,\
	.REG_00.reg_matchbg_softrst = 0x0,\
	.REG_04.reg_matchbg_curfrmnum = 0x0,\
	.REG_08.reg_matchbg_prefrmnum = 0x0,\
	.REG_0C.reg_matchbg_timethr = 0x14,\
	.REG_0C.reg_matchbg_diffthrcrlcoef = 0x0,\
	.REG_0C.reg_matchbg_diffmaxthr = 0x6,\
	.REG_0C.reg_matchbg_diffminthr = 0x4,\
	.REG_0C.reg_matchbg_diffthrinc = 0x0,\
	.REG_0C.reg_matchbg_fastlearnrate = 0x2,\
	.REG_0C.reg_matchbg_detchgregion = 0x0,\
}
//#define DEFINE_IVE_MATCH_BG_C(X) IVE_MATCH_BG_C X = _DEFINE_IVE_MATCH_BG_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_MATCH_BG_STRUCT_H__
