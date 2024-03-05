// $Module: sc_odma $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Sun, 26 Sep 2021 04:20:45 PM $
//

#ifndef __REG_SC_ODMA_STRUCT_H__
#define __REG_SC_ODMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_blen:1;
		uint32_t rsv_1_7:7;
		/*4'h0 : YUV420 planar
		4'h1 : YUV422 planar
		4'h2 : RGB888 planar
		4'h3 : RGB packed {R,G,B}
		4'h4 : RGB packed {B,G,R}
		4'h5 : Y only
		4'h6 : BF16 (only support in sc_odma)
		4'h7 : reserved
		4'h8 : NV21 {V,U}
		4'h9 : NV12 {U,V}
		4'ha : YUV422-SP1{V,U}
		4'hb : YUV422-SP2{U,V}
		4'hc : YUV2-1 {U,Y,V,Y}
		4'hd : YUV2-2 {V,Y,U,Y}
		4'he : YUV2-3 {Y,U,Y,V}
		4'hf  : YUV2-4 {Y,V,Y,U};*/
		uint32_t reg_fmt_sel:4;
		uint32_t rsv_12_15:4;
		/*;*/
		uint32_t reg_sc_odma_hflip:1;
		/*;*/
		uint32_t reg_sc_odma_vflip:1;
		uint32_t rsv_18_19:2;
		/*;*/
		uint32_t reg_sc_422_avg:1;
		/*;*/
		uint32_t reg_sc_420_avg:1;
		/*0 : UV run away from zero
		1 : UV run to zero;*/
		uint32_t reg_c_round_mode:1;
		/*csc q mode outout BF16 enable
		when this bit set to 1
		1. reg_fmt_sel must set to 0x6
		2. reg_sc_csc_q_mode must set to 1

		and notice aht sc_hsv function would be auto disable ;*/
		uint32_t reg_bf16_en:1;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_00_C;
typedef union {
	struct {
		/*output Y (R) base address;*/
		uint32_t reg_dma_y_base_low_part:32;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_01_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_y_base_high_part:8;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_02_C;
typedef union {
	struct {
		/*putput U (G) base address;*/
		uint32_t reg_dma_u_base_low_part:32;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_03_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_u_base_high_part:8;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_04_C;
typedef union {
	struct {
		/*output V (B) base address;*/
		uint32_t reg_dma_v_base_low_part:32;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_05_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_dma_v_base_high_part:8;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_06_C;
typedef union {
	struct {
		/*line pitch for Y/R/Packet data;*/
		uint32_t reg_dma_y_pitch:24;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_07_C;
typedef union {
	struct {
		/*line pitch for C;*/
		uint32_t reg_dma_c_pitch:24;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_08_C;
typedef union {
	struct {
		/*crop write start point ( in pixel base);*/
		uint32_t reg_dma_x_str:12;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_09_C;
typedef union {
	struct {
		/*crop write start line (line base);*/
		uint32_t reg_dma_y_str:12;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_10_C;
typedef union {
	struct {
		/*output width, fill sub 1;*/
		uint32_t reg_dma_wd:12;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_11_C;
typedef union {
	struct {
		/*output height, fill sub 1;*/
		uint32_t reg_dma_ht:12;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_12_C;
typedef union {
	struct {
		/*dma debug information;*/
		uint32_t reg_dma_debug:32;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_13_C;
typedef union {
	struct {
		/*line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_line_target:12;
		uint32_t rsv_12_15:4;
		/*line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_line_target_sel:2;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_14_C;
typedef union {
	struct {
		/*cycle line count hit target in axi_cmd_gen;*/
		uint32_t reg_dma_int_cycle_line_target:11;
		uint32_t rsv_11_15:5;
		/*cycle line count hit target select:
		2'd0: from Y,
		2'd1: from U,
		2'd2: from V;*/
		uint32_t reg_dma_int_cycle_line_target_sel:2;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_15_C;
typedef union {
	struct {
		/*latch odma y line count;*/
		uint32_t reg_dma_latch_line_cnt:1;
		uint32_t rsv_1_7:7;
		/*latched odma y line count;*/
		uint32_t reg_dma_latched_line_cnt:12;
	};
	uint32_t val;
} SC_ODMA_ODMA_REG_16_C;
typedef union {
	struct {
		/*slice buffer mode
		2'h0 : disable
		2'h1 : free-run mode
		2'h2 : frame base mode
		2'h3 : reserved;*/
		uint32_t reg_sb_mode:2;
		/*slice buffer line number, in data Y
		0 : 64 line
		1 : 128 line ;*/
		uint32_t reg_sb_size:1;
		uint32_t rsv_3_7:5;
		/*slice buffer depth;*/
		uint32_t reg_sb_nb:5;
		uint32_t rsv_13_15:3;
		/*slice buffer full point,  reg_sb_full_nb <= reg_sb_nb;*/
		uint32_t reg_sb_full_nb:5;
		uint32_t rsv_21_23:3;
		/*sw sb fifo depth for frame strt;*/
		uint32_t reg_sb_sw_wptr:5;
		uint32_t rsv_29_29:1;
		/*set 1 to over-write frame strat sb fifo write index;*/
		uint32_t reg_sb_set_str:1;
		/*set 1 to clear slice buffer controller;*/
		uint32_t reg_sb_sw_clr:1;
	};
	uint32_t val;
} SC_ODMA_SB_REG_CTRL_C;
typedef union {
	struct {
		/*U channel SB write pointer;*/
		uint32_t reg_u_sb_wptr_ro:5;
		uint32_t rsv_5_5:1;
		/*U channel SB FIFO full;*/
		uint32_t reg_u_sb_full:1;
		/*U channel SB FIFO empty;*/
		uint32_t reg_u_sb_empty:1;
		/*U channel SB FIFO data pointer;*/
		uint32_t reg_u_sb_dptr_ro:6;
		uint32_t rsv_14_15:2;
		/*V channel SB write pointer;*/
		uint32_t reg_v_sb_wptr_ro:5;
		uint32_t rsv_21_21:1;
		/*V channel SB FIFO full;*/
		uint32_t reg_v_sb_full:1;
		/*V channel SB FIFO empty;*/
		uint32_t reg_v_sb_empty:1;
		/*V channel SB FIFO data pointer;*/
		uint32_t reg_v_sb_dptr_ro:6;
	};
	uint32_t val;
} SC_ODMA_SB_REG_C_STAT_C;
typedef union {
	struct {
		/*Y channel SB write pointer;*/
		uint32_t reg_y_sb_wptr_ro:5;
		uint32_t rsv_5_5:1;
		/*Y channel SB FIFO full;*/
		uint32_t reg_y_sb_full:1;
		/*Y channel SB FIFO empty;*/
		uint32_t reg_y_sb_empty:1;
		/*Y channel SB FIFO data pointer;*/
		uint32_t reg_y_sb_dptr_ro:6;
		uint32_t rsv_14_14:1;
		/*SB mode FIFO full;*/
		uint32_t reg_sb_full:1;
	};
	uint32_t val;
} SC_ODMA_SB_REG_Y_STAT_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_sc_csc_en:1;
		/*csc q mode;*/
		uint32_t reg_sc_csc_q_mode:1;
		/*q mode rounding method
		2'b00 : ties to even
		2'b01 : ties away from zero
		2'b1x : truncate;*/
		uint32_t reg_sc_csc_q_drop:2;
		/*RGB to HSV enable ;*/
		uint32_t reg_sc_hsv_en:1;
		/*0 : hsv rounding ties away from zero
		1 : hsv rounding toward - infinity;*/
		uint32_t reg_sc_hsv_floor_en:1;
		uint32_t rsv_6_7:2;
		/*csc disable for border (padding) pixel
		0 : apply csc to padding pixel
		1 : don't apply csc to padding pixel;*/
		uint32_t reg_sc_csc_bd_dis:1;
		/*csc disable for border (padding) pixel
		0 : apply hsv to padding pixel
		1 : don't apply hsv to padding pixel;*/
		uint32_t reg_sc_hsv_bd_dis:1;
		/*border color type under BF16 & keep border color (reg_sc_csc_bd_dis = 1 )
		0 : border color 0 ~ 255
		1 : border color -128 ~ 127;*/
		uint32_t reg_bf16_bd_type:1;
		/*csc q_mode option
		0 : {gain, offset} = {s.0.13, 8.10}
		1 : {gain, offset} = {s.1.12, 9.9  };*/
		uint32_t reg_sc_csc_q_gain_mode:1;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_00_C;
typedef union {
	struct {
		/*csc matrix coef_00 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c00:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_01 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c01:14;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_01_C;
typedef union {
	struct {
		/*csc matrix coef_02 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c02:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_10 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c10:14;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_02_C;
typedef union {
	struct {
		/*csc matrix coef_11 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c11:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_12 for RGB2YUV,s3.10;*/
		uint32_t reg_sc_csc_r2y_c12:14;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_03_C;
typedef union {
	struct {
		/*csc matrix coef_20 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c20:14;
		uint32_t rsv_14_15:2;
		/*csc matrix coef_21 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c21:14;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_04_C;
typedef union {
	struct {
		/*csc matrix coef_22 for RGB2YUV, s3.10;*/
		uint32_t reg_sc_csc_r2y_c22:14;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_05_C;
typedef union {
	struct {
		/*csc add_0 value;*/
		uint32_t reg_sc_csc_r2y_add_0:8;
		/*csc add_1 value;*/
		uint32_t reg_sc_csc_r2y_add_1:8;
		/*csc add_2 value;*/
		uint32_t reg_sc_csc_r2y_add_2:8;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_06_C;
typedef union {
	struct {
		/*csc frac_0 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_0:10;
		uint32_t rsv_10_15:6;
		/*csc frac_1 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_1:10;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_07_C;
typedef union {
	struct {
		/*csc frac_2 value, for Q mode only, 0.10;*/
		uint32_t reg_sc_csc_r2y_frac_2:10;
	};
	uint32_t val;
} SC_ODMA_SC_CSC_REG_08_C;
typedef struct {
	SC_ODMA_ODMA_REG_00_C ODMA_REG_00;
	SC_ODMA_ODMA_REG_01_C ODMA_REG_01;
	SC_ODMA_ODMA_REG_02_C ODMA_REG_02;
	SC_ODMA_ODMA_REG_03_C ODMA_REG_03;
	SC_ODMA_ODMA_REG_04_C ODMA_REG_04;
	SC_ODMA_ODMA_REG_05_C ODMA_REG_05;
	SC_ODMA_ODMA_REG_06_C ODMA_REG_06;
	SC_ODMA_ODMA_REG_07_C ODMA_REG_07;
	SC_ODMA_ODMA_REG_08_C ODMA_REG_08;
	SC_ODMA_ODMA_REG_09_C ODMA_REG_09;
	SC_ODMA_ODMA_REG_10_C ODMA_REG_10;
	SC_ODMA_ODMA_REG_11_C ODMA_REG_11;
	SC_ODMA_ODMA_REG_12_C ODMA_REG_12;
	SC_ODMA_ODMA_REG_13_C ODMA_REG_13;
	SC_ODMA_ODMA_REG_14_C ODMA_REG_14;
	SC_ODMA_ODMA_REG_15_C ODMA_REG_15;
	SC_ODMA_ODMA_REG_16_C ODMA_REG_16;
	uint32_t _SB_REG_CTRL_0; // 0x44
	uint32_t _SB_REG_CTRL_1; // 0x48
	uint32_t _SB_REG_CTRL_2; // 0x4C
	SC_ODMA_SB_REG_CTRL_C SB_REG_CTRL;
	SC_ODMA_SB_REG_C_STAT_C SB_REG_C_STAT;
	SC_ODMA_SB_REG_Y_STAT_C SB_REG_Y_STAT;
	uint32_t _SC_CSC_REG_00_0; // 0x5C
	uint32_t _SC_CSC_REG_00_1; // 0x60
	uint32_t _SC_CSC_REG_00_2; // 0x64
	uint32_t _SC_CSC_REG_00_3; // 0x68
	uint32_t _SC_CSC_REG_00_4; // 0x6C
	uint32_t _SC_CSC_REG_00_5; // 0x70
	uint32_t _SC_CSC_REG_00_6; // 0x74
	uint32_t _SC_CSC_REG_00_7; // 0x78
	uint32_t _SC_CSC_REG_00_8; // 0x7C
	uint32_t _SC_CSC_REG_00_9; // 0x80
	uint32_t _SC_CSC_REG_00_10; // 0x84
	uint32_t _SC_CSC_REG_00_11; // 0x88
	uint32_t _SC_CSC_REG_00_12; // 0x8C
	uint32_t _SC_CSC_REG_00_13; // 0x90
	uint32_t _SC_CSC_REG_00_14; // 0x94
	uint32_t _SC_CSC_REG_00_15; // 0x98
	uint32_t _SC_CSC_REG_00_16; // 0x9C
	uint32_t _SC_CSC_REG_00_17; // 0xA0
	uint32_t _SC_CSC_REG_00_18; // 0xA4
	uint32_t _SC_CSC_REG_00_19; // 0xA8
	uint32_t _SC_CSC_REG_00_20; // 0xAC
	uint32_t _SC_CSC_REG_00_21; // 0xB0
	uint32_t _SC_CSC_REG_00_22; // 0xB4
	uint32_t _SC_CSC_REG_00_23; // 0xB8
	uint32_t _SC_CSC_REG_00_24; // 0xBC
	uint32_t _SC_CSC_REG_00_25; // 0xC0
	uint32_t _SC_CSC_REG_00_26; // 0xC4
	uint32_t _SC_CSC_REG_00_27; // 0xC8
	uint32_t _SC_CSC_REG_00_28; // 0xCC
	uint32_t _SC_CSC_REG_00_29; // 0xD0
	uint32_t _SC_CSC_REG_00_30; // 0xD4
	uint32_t _SC_CSC_REG_00_31; // 0xD8
	uint32_t _SC_CSC_REG_00_32; // 0xDC
	uint32_t _SC_CSC_REG_00_33; // 0xE0
	uint32_t _SC_CSC_REG_00_34; // 0xE4
	uint32_t _SC_CSC_REG_00_35; // 0xE8
	uint32_t _SC_CSC_REG_00_36; // 0xEC
	uint32_t _SC_CSC_REG_00_37; // 0xF0
	uint32_t _SC_CSC_REG_00_38; // 0xF4
	uint32_t _SC_CSC_REG_00_39; // 0xF8
	uint32_t _SC_CSC_REG_00_40; // 0xFC
	SC_ODMA_SC_CSC_REG_00_C SC_CSC_REG_00;
	SC_ODMA_SC_CSC_REG_01_C SC_CSC_REG_01;
	SC_ODMA_SC_CSC_REG_02_C SC_CSC_REG_02;
	SC_ODMA_SC_CSC_REG_03_C SC_CSC_REG_03;
	SC_ODMA_SC_CSC_REG_04_C SC_CSC_REG_04;
	SC_ODMA_SC_CSC_REG_05_C SC_CSC_REG_05;
	SC_ODMA_SC_CSC_REG_06_C SC_CSC_REG_06;
	SC_ODMA_SC_CSC_REG_07_C SC_CSC_REG_07;
	SC_ODMA_SC_CSC_REG_08_C SC_CSC_REG_08;
} SC_ODMA_C;
#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_SC_ODMA_C \
{\
	.ODMA_REG_00.reg_dma_blen = 0x0,\
	.ODMA_REG_00.reg_fmt_sel = 0x0,\
	.ODMA_REG_00.reg_sc_odma_hflip = 0x0,\
	.ODMA_REG_00.reg_sc_odma_vflip = 0x0,\
	.ODMA_REG_00.reg_sc_422_avg = 0x0,\
	.ODMA_REG_00.reg_sc_420_avg = 0x0,\
	.ODMA_REG_00.reg_c_round_mode = 0x0,\
	.ODMA_REG_00.reg_bf16_en = 0x0,\
	.ODMA_REG_01.reg_dma_y_base_low_part = 0x0,\
	.ODMA_REG_02.reg_dma_y_base_high_part = 0x0,\
	.ODMA_REG_03.reg_dma_u_base_low_part = 0x0,\
	.ODMA_REG_04.reg_dma_u_base_high_part = 0x0,\
	.ODMA_REG_05.reg_dma_v_base_low_part = 0x0,\
	.ODMA_REG_06.reg_dma_v_base_high_part = 0x0,\
	.ODMA_REG_07.reg_dma_y_pitch = 0x0,\
	.ODMA_REG_08.reg_dma_c_pitch = 0x0,\
	.ODMA_REG_09.reg_dma_x_str = 0x0,\
	.ODMA_REG_10.reg_dma_y_str = 0x0,\
	.ODMA_REG_11.reg_dma_wd = 0x0,\
	.ODMA_REG_12.reg_dma_ht = 0x0,\
	.ODMA_REG_13.reg_dma_debug = 0x0,\
	.ODMA_REG_14.reg_dma_int_line_target = 0x0,\
	.ODMA_REG_14.reg_dma_int_line_target_sel = 0x0,\
	.ODMA_REG_15.reg_dma_int_cycle_line_target = 0x0,\
	.ODMA_REG_15.reg_dma_int_cycle_line_target_sel = 0x0,\
	.ODMA_REG_16.reg_dma_latch_line_cnt = 0x0,\
	.ODMA_REG_16.reg_dma_latched_line_cnt = 0x0,\
	.SB_REG_CTRL.reg_sb_mode = 0x0,\
	.SB_REG_CTRL.reg_sb_size = 0x0,\
	.SB_REG_CTRL.reg_sb_nb = 0x3,\
	.SB_REG_CTRL.reg_sb_full_nb = 0x2,\
	.SB_REG_CTRL.reg_sb_sw_wptr = 0x0,\
	.SB_REG_CTRL.reg_sb_set_str = 0x0,\
	.SB_REG_CTRL.reg_sb_sw_clr = 0x0,\
	.SC_CSC_REG_00.reg_sc_csc_en = 0x0,\
	.SC_CSC_REG_00.reg_sc_csc_q_mode = 0x0,\
	.SC_CSC_REG_00.reg_sc_csc_q_drop = 0x0,\
	.SC_CSC_REG_00.reg_sc_hsv_en = 0x0,\
	.SC_CSC_REG_00.reg_sc_hsv_floor_en = 0x0,\
	.SC_CSC_REG_00.reg_sc_csc_bd_dis = 0x0,\
	.SC_CSC_REG_00.reg_sc_hsv_bd_dis = 0x0,\
	.SC_CSC_REG_00.reg_bf16_bd_type = 0x0,\
	.SC_CSC_REG_00.reg_sc_csc_q_gain_mode = 0x0,\
	.SC_CSC_REG_01.reg_sc_csc_r2y_c00 = 0x132,\
	.SC_CSC_REG_01.reg_sc_csc_r2y_c01 = 0x259,\
	.SC_CSC_REG_02.reg_sc_csc_r2y_c02 = 0x75,\
	.SC_CSC_REG_02.reg_sc_csc_r2y_c10 = 0x1f53,\
	.SC_CSC_REG_03.reg_sc_csc_r2y_c11 = 0x1ead,\
	.SC_CSC_REG_03.reg_sc_csc_r2y_c12 = 0x200,\
	.SC_CSC_REG_04.reg_sc_csc_r2y_c20 = 0x200,\
	.SC_CSC_REG_04.reg_sc_csc_r2y_c21 = 0x1e53,\
	.SC_CSC_REG_05.reg_sc_csc_r2y_c22 = 0x1fad,\
	.SC_CSC_REG_06.reg_sc_csc_r2y_add_0 = 0x0,\
	.SC_CSC_REG_06.reg_sc_csc_r2y_add_1 = 0x80,\
	.SC_CSC_REG_06.reg_sc_csc_r2y_add_2 = 0x80,\
	.SC_CSC_REG_07.reg_sc_csc_r2y_frac_0 = 0x200,\
	.SC_CSC_REG_07.reg_sc_csc_r2y_frac_1 = 0x200,\
	.SC_CSC_REG_08.reg_sc_csc_r2y_frac_2 = 0x200,\
}
//#define DEFINE_SC_ODMA_C(X) SC_ODMA_C X = _DEFINE_SC_ODMA_C
#endif /* ifdef __cplusplus */
#endif //__REG_SC_ODMA_STRUCT_H__
