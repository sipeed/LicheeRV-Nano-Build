// $Module: img_in $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:07:20 PM $
//

#ifndef __REG_IMG_IN_STRUCT_H__
#define __REG_IMG_IN_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*source select
		0 : ISP
		1 : dewap/map_convert
		other : DRAM;*/
		uint32_t reg_src_sel:2;
		uint32_t rsv_2_3:2;
		/*source format
		4'h0 : YUV420 planar
		4'h1 : YUV422 planar
		4'h2 : RGB888 planar
		4'h3 : RGB packed {R,G,B}
		4'h4 : RGB packed {B,G,R}
		4'h5 : Y only
		4'h6 : reserved
		4'h7 : reserved
		4'h8 : NV12
		4'h9 : NV21
		4'ha : YUV422-SP1
		4'hb : YUV422-SP2
		4'hc : YUV2-1 {U,Y,V,Y}
		4'hd : YUV2-2 {V,Y,U,Y}
		4'he : YUV2-3 {Y,U,Y,V}
		4'hf  : YUV2-4 {Y,V,Y,U};*/
		uint32_t reg_fmt_sel:4;
		/*burst length;*/
		uint32_t reg_burst_ln:4;
		/*sw csc force enable;*/
		uint32_t reg_img_csc_en:1;
		/*auto csc enable when input format is set to yuv;*/
		uint32_t reg_auto_csc_en:1;
		uint32_t rsv_14_15:2;
		/*read address 64-byte align enable, only when burst lengh is set to 3,7,11,15;*/
		uint32_t reg_64b_align:1;
		uint32_t rsv_17_30:14;
		/*force clock enable
		0 : enable dynamic clock gating
		1 : disable dynamic clock gating;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} IMG_IN_REG_00_C;
typedef union {
	struct {
		/*crop x start, pixel base;*/
		uint32_t reg_src_x_str:16;
		/*crop y start, line base;*/
		uint32_t reg_src_y_str:16;
	};
	uint32_t val;
} IMG_IN_REG_01_C;
typedef union {
	struct {
		/*crop image width, count from 0;*/
		uint32_t reg_src_wd:16;
		/*crop image height, count from 0;*/
		uint32_t reg_src_ht:16;
	};
	uint32_t val;
} IMG_IN_REG_02_C;
typedef union {
	struct {
		/*source y/r/pkt pitch, must be 32byte alignment;*/
		uint32_t reg_src_y_pitch:24;
	};
	uint32_t val;
} IMG_IN_REG_03_C;
typedef union {
	struct {
		/*source cb/cr/r/b pitch, must be 32byte alignment;*/
		uint32_t reg_src_c_pitch:24;
	};
	uint32_t val;
} IMG_IN_REG_04_C;
typedef union {
	struct {
		/*trig to force register up;*/
		uint32_t reg_sw_force_up:1;
		/*register update sw_mask
		0 : not mask
		1 : mask ;*/
		uint32_t reg_sw_mask_up:1;
		/*shadow register read control;*/
		uint32_t reg_shrd_sel:1;
	};
	uint32_t val;
} IMG_IN_REG_05_C;
typedef union {
	struct {
		/*dummy read register;*/
		uint32_t reg_dummy_ro:32;
	};
	uint32_t val;
} IMG_IN_REG_06_C;
typedef union {
	struct {
		/*dummy register 0;*/
		uint32_t reg_dummy_0:32;
	};
	uint32_t val;
} IMG_IN_REG_07_C;
typedef union {
	struct {
		/*dummy register 1;*/
		uint32_t reg_dummy_1:32;
	};
	uint32_t val;
} IMG_IN_REG_08_C;
typedef union {
	struct {
		/*source y/r base address, must be 16 byte align
		packet, y only mode , also use this address;*/
		uint32_t reg_src_y_base_b0:32;
	};
	uint32_t val;
} IMG_IN_REG_Y_BASE_0_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_y_base_b1:8;
	};
	uint32_t val;
} IMG_IN_REG_Y_BASE_1_C;
typedef union {
	struct {
		/*source u/g base address, must be 16 byte align;*/
		uint32_t reg_src_u_base_b0:32;
	};
	uint32_t val;
} IMG_IN_REG_U_BASE_0_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_u_base_b1:8;
	};
	uint32_t val;
} IMG_IN_REG_U_BASE_1_C;
typedef union {
	struct {
		/*source v/b base address, must be 16 byte align;*/
		uint32_t reg_src_v_base_b0:32;
	};
	uint32_t val;
} IMG_IN_REG_V_BASE_0_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_src_v_base_b1:8;
	};
	uint32_t val;
} IMG_IN_REG_V_BASE_1_C;
typedef union {
	struct {
		/*yuv2rgb c00;*/
		uint32_t reg_c00:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c01;*/
		uint32_t reg_c01:14;
	};
	uint32_t val;
} IMG_IN_REG_040_C;
typedef union {
	struct {
		/*yuv2rgb c02;*/
		uint32_t reg_c02:14;
	};
	uint32_t val;
} IMG_IN_REG_044_C;
typedef union {
	struct {
		/*yuv2rgb c10;*/
		uint32_t reg_c10:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c11;*/
		uint32_t reg_c11:14;
	};
	uint32_t val;
} IMG_IN_REG_048_C;
typedef union {
	struct {
		/*yuv2rgb c12;*/
		uint32_t reg_c12:14;
	};
	uint32_t val;
} IMG_IN_REG_04C_C;
typedef union {
	struct {
		/*yuv2rgb c20;*/
		uint32_t reg_c20:14;
		uint32_t rsv_14_15:2;
		/*yuv2rgb c21;*/
		uint32_t reg_c21:14;
	};
	uint32_t val;
} IMG_IN_REG_050_C;
typedef union {
	struct {
		/*yuv2rgb c22;*/
		uint32_t reg_c22:14;
	};
	uint32_t val;
} IMG_IN_REG_054_C;
typedef union {
	struct {
		/*yuv2rgb sub0;*/
		uint32_t reg_sub_0:8;
		/*yuv2rgb sub1;*/
		uint32_t reg_sub_1:8;
		/*yuv2rgb sub2;*/
		uint32_t reg_sub_2:8;
	};
	uint32_t val;
} IMG_IN_REG_058_C;
typedef union {
	struct {
		/*yuv2rgb add0;*/
		uint32_t reg_add_0:8;
		/*yuv2rgb add1;*/
		uint32_t reg_add_1:8;
		/*yuv2rgb add2;*/
		uint32_t reg_add_2:8;
	};
	uint32_t val;
} IMG_IN_REG_05C_C;
typedef union {
	struct {
		/*y buffer read threshold;*/
		uint32_t reg_fifo_rd_th_y:8;
		/*y buffer high priority thresh;*/
		uint32_t reg_fifo_pr_th_y:8;
		/*c buffer read threshold;*/
		uint32_t reg_fifo_rd_th_c:8;
		/*c buffer high priority thresh;*/
		uint32_t reg_fifo_pr_th_c:8;
	};
	uint32_t val;
} IMG_IN_REG_060_C;
typedef union {
	struct {
		/*outstanding max number;*/
		uint32_t reg_os_max:4;
	};
	uint32_t val;
} IMG_IN_REG_064_C;
typedef union {
	struct {
		/*debug information;*/
		uint32_t reg_err_fwr_y:1;
		/*debug information;*/
		uint32_t reg_err_fwr_u:1;
		/*debug information;*/
		uint32_t reg_err_fwr_v:1;
		/*debug information;*/
		uint32_t reg_clr_fwr_w1t:1;
		/*debug information;*/
		uint32_t reg_err_erd_y:1;
		/*debug information;*/
		uint32_t reg_err_erd_u:1;
		/*debug information;*/
		uint32_t reg_err_erd_v:1;
		/*debug information;*/
		uint32_t reg_clr_erd_w1t:1;
		/*debug information;*/
		uint32_t reg_lb_full_y:1;
		/*debug information;*/
		uint32_t reg_lb_full_u:1;
		/*debug information;*/
		uint32_t reg_lb_full_v:1;
		uint32_t rsv_11_11:1;
		/*debug information;*/
		uint32_t reg_lb_empty_y:1;
		/*debug information;*/
		uint32_t reg_lb_empty_u:1;
		/*debug information;*/
		uint32_t reg_lb_empty_v:1;
		uint32_t rsv_15_15:1;
		/*IP idle index;*/
		uint32_t reg_ip_idle:1;
		/*IP interupt status;*/
		uint32_t reg_ip_int:1;
		/*sw reset IP;*/
		uint32_t reg_ip_clr_w1t:1;
		/*write 1 to clear interrupt;*/
		uint32_t reg_clr_int_w1t:1;
	};
	uint32_t val;
} IMG_IN_REG_068_C;
typedef union {
	struct {
		/*image_in IP axi interface idle index;*/
		uint32_t reg_axi_idle:1;
		uint32_t rsv_1_7:7;
		/*image_in IP axi interface status;*/
		uint32_t reg_axi_status:8;
	};
	uint32_t val;
} IMG_IN_REG_AXI_ST_C;
typedef union {
	struct {
		/*B.W. limit window period, unit : clk_axi cycle;*/
		uint32_t reg_bwl_win:10;
		uint32_t rsv_10_15:6;
		/*B.W. limit valid number in reg_bwl_win, unit : Byte;*/
		uint32_t reg_bwl_vld:10;
		uint32_t rsv_26_30:5;
		/*B.W. limit enable;*/
		uint32_t reg_bwl_en:1;
	};
	uint32_t val;
} IMG_IN_REG_BW_LIMIT_C;
typedef union {
	struct {
		/*catch mode for B.W. issue;*/
		uint32_t reg_catch_mode:1;
		/*enable dma_urgent signal;*/
		uint32_t reg_dma_urgent_en:1;
		/*qos criteria selection
		0 : data in buffer + outstanding
		1 : data in buffer only;*/
		uint32_t reg_qos_sel_rr:1;
		uint32_t rsv_3_3:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_y:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_u:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_act_v:1;
		uint32_t rsv_7_7:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_y:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_u:1;
		/*use clr_erd_1t to clear this flag;*/
		uint32_t reg_catch_fail_v:1;
	};
	uint32_t val;
} IMG_IN_REG_CATCH_C;
typedef union {
	struct {
		/*img_in data out check sum;*/
		uint32_t reg_chksum_dat_out:8;
		uint32_t rsv_8_30:23;
		/*checksum function enable;*/
		uint32_t reg_checksum_en:1;
	};
	uint32_t val;
} IMG_IN_REG_CHK_CTRL_C;
typedef union {
	struct {
		/*img_in axi read in data checksum;*/
		uint32_t reg_chksum_axi_rd:32;
	};
	uint32_t val;
} IMG_IN_CHKSUM_AXI_RD_C;
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
		1 : 128 line
		2 : 192 line
		3 : 256 line ;*/
		uint32_t reg_sb_size:2;
		uint32_t rsv_4_7:4;
		/*slice buffer depth;*/
		uint32_t reg_sb_nb:4;
		uint32_t rsv_12_23:12;
		/*sw sb fifo depth for frame strt;*/
		uint32_t reg_sb_sw_rptr:4;
		uint32_t rsv_28_29:2;
		/*set 1 to over-write frame start fb fifo read index;*/
		uint32_t reg_sb_set_str:1;
		/*set 1 to clear slice buffer control;*/
		uint32_t reg_sb_sw_clr:1;
	};
	uint32_t val;
} IMG_IN_SB_REG_CTRL_C;
typedef union {
	struct {
		/*U channel SB read pointer;*/
		uint32_t reg_u_sb_rptr_ro:4;
		uint32_t rsv_4_5:2;
		/*U channel SB FIFO full;*/
		uint32_t reg_u_sb_full:1;
		/*U channel SB FIFO empty;*/
		uint32_t reg_u_sb_empty:1;
		/*U channel SB FIFO data pointer;*/
		uint32_t reg_u_sb_dptr_ro:5;
		uint32_t rsv_13_15:3;
		/*V channel SB read pointer;*/
		uint32_t reg_v_sb_rptr_ro:4;
		uint32_t rsv_20_21:2;
		/*V channel SB FIFO full;*/
		uint32_t reg_v_sb_full:1;
		/*V channel SB FIFO empty;*/
		uint32_t reg_v_sb_empty:1;
		/*V channel SB FIFO data pointer;*/
		uint32_t reg_v_sb_dptr_ro:5;
	};
	uint32_t val;
} IMG_IN_SB_REG_C_STAT_C;
typedef union {
	struct {
		/*Y channel SB read pointer;*/
		uint32_t reg_y_sb_rptr_ro:4;
		uint32_t rsv_4_4:1;
		/*Y channel SB FIFO full;*/
		uint32_t reg_y_sb_full:1;
		/*Y channel SB FIFO empty;*/
		uint32_t reg_y_sb_empty:1;
		uint32_t rsv_7_7:1;
		/*Y channel SB FIFO data pointer;*/
		uint32_t reg_y_sb_dptr_ro:5;
		uint32_t rsv_13_14:2;
		/*SB mode FIFO empty;*/
		uint32_t reg_sb_empty:1;
	};
	uint32_t val;
} IMG_IN_SB_REG_Y_STAT_C;
typedef struct {
	IMG_IN_REG_00_C REG_00;
	IMG_IN_REG_01_C REG_01;
	IMG_IN_REG_02_C REG_02;
	IMG_IN_REG_03_C REG_03;
	IMG_IN_REG_04_C REG_04;
	IMG_IN_REG_05_C REG_05;
	IMG_IN_REG_06_C REG_06;
	IMG_IN_REG_07_C REG_07;
	IMG_IN_REG_08_C REG_08;
	IMG_IN_REG_Y_BASE_0_C REG_Y_BASE_0;
	IMG_IN_REG_Y_BASE_1_C REG_Y_BASE_1;
	IMG_IN_REG_U_BASE_0_C REG_U_BASE_0;
	IMG_IN_REG_U_BASE_1_C REG_U_BASE_1;
	IMG_IN_REG_V_BASE_0_C REG_V_BASE_0;
	IMG_IN_REG_V_BASE_1_C REG_V_BASE_1;
	uint32_t _REG_040_0; // 0x3C
	IMG_IN_REG_040_C REG_040;
	IMG_IN_REG_044_C REG_044;
	IMG_IN_REG_048_C REG_048;
	IMG_IN_REG_04C_C REG_04C;
	IMG_IN_REG_050_C REG_050;
	IMG_IN_REG_054_C REG_054;
	IMG_IN_REG_058_C REG_058;
	IMG_IN_REG_05C_C REG_05C;
	IMG_IN_REG_060_C REG_060;
	IMG_IN_REG_064_C REG_064;
	IMG_IN_REG_068_C REG_068;
	uint32_t _REG_AXI_ST_0; // 0x6C
	IMG_IN_REG_AXI_ST_C REG_AXI_ST;
	IMG_IN_REG_BW_LIMIT_C REG_BW_LIMIT;
	uint32_t _REG_CATCH_0; // 0x78
	uint32_t _REG_CATCH_1; // 0x7C
	IMG_IN_REG_CATCH_C REG_CATCH;
	IMG_IN_REG_CHK_CTRL_C REG_CHK_CTRL;
	IMG_IN_CHKSUM_AXI_RD_C CHKSUM_AXI_RD;
	uint32_t _SB_REG_CTRL_0; // 0x8C
	IMG_IN_SB_REG_CTRL_C SB_REG_CTRL;
	IMG_IN_SB_REG_C_STAT_C SB_REG_C_STAT;
	IMG_IN_SB_REG_Y_STAT_C SB_REG_Y_STAT;
} IMG_IN_C;


#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IMG_IN_C \
	{\
		.REG_00.reg_src_sel = 0x1,\
		.REG_00.reg_fmt_sel = 0x0,\
		.REG_00.reg_burst_ln = 0x3,\
		.REG_00.reg_img_csc_en = 0x0,\
		.REG_00.reg_auto_csc_en = 0x1,\
		.REG_00.reg_64b_align = 0x1,\
		.REG_00.reg_force_clk_enable = 0x0,\
		.REG_01.reg_src_x_str = 0x0,\
		.REG_01.reg_src_y_str = 0x0,\
		.REG_02.reg_src_wd = 0x13f,\
		.REG_02.reg_src_ht = 0xef,\
		.REG_03.reg_src_y_pitch = 0x0,\
		.REG_04.reg_src_c_pitch = 0x0,\
		.REG_05.reg_sw_force_up = 0x0,\
		.REG_05.reg_sw_mask_up = 0x0,\
		.REG_05.reg_shrd_sel = 0x0,\
		.REG_07.reg_dummy_0 = 0x0,\
		.REG_08.reg_dummy_1 = 0xffffffff,\
		.REG_Y_BASE_0.reg_src_y_base_b0 = 0x0,\
		.REG_Y_BASE_1.reg_src_y_base_b1 = 0x0,\
		.REG_U_BASE_0.reg_src_u_base_b0 = 0x0,\
		.REG_U_BASE_1.reg_src_u_base_b1 = 0x0,\
		.REG_V_BASE_0.reg_src_v_base_b0 = 0x0,\
		.REG_V_BASE_1.reg_src_v_base_b1 = 0x0,\
		.REG_040.reg_c00 = 0x400,\
		.REG_040.reg_c01 = 0x0,\
		.REG_044.reg_c02 = 0x59c,\
		.REG_048.reg_c10 = 0x400,\
		.REG_048.reg_c11 = 0x2160,\
		.REG_04C.reg_c12 = 0x22db,\
		.REG_050.reg_c20 = 0x400,\
		.REG_050.reg_c21 = 0x717,\
		.REG_054.reg_c22 = 0x0,\
		.REG_058.reg_sub_0 = 0x0,\
		.REG_058.reg_sub_1 = 0x80,\
		.REG_058.reg_sub_2 = 0x80,\
		.REG_05C.reg_add_0 = 0x0,\
		.REG_05C.reg_add_1 = 0x0,\
		.REG_05C.reg_add_2 = 0x0,\
		.REG_060.reg_fifo_rd_th_y = 0x20,\
		.REG_060.reg_fifo_pr_th_y = 0x10,\
		.REG_060.reg_fifo_rd_th_c = 0x20,\
		.REG_060.reg_fifo_pr_th_c = 0x10,\
		.REG_064.reg_os_max = 0x7,\
		.REG_068.reg_clr_fwr_w1t = 0x0,\
		.REG_068.reg_clr_erd_w1t = 0x0,\
		.REG_068.reg_ip_clr_w1t = 0x0,\
		.REG_068.reg_clr_int_w1t = 0x0,\
		.REG_BW_LIMIT.reg_bwl_win = 0x0,\
		.REG_BW_LIMIT.reg_bwl_vld = 0x0,\
		.REG_BW_LIMIT.reg_bwl_en = 0x0,\
		.REG_CATCH.reg_catch_mode = 0x0,\
		.REG_CATCH.reg_dma_urgent_en = 0x1,\
		.REG_CATCH.reg_qos_sel_rr = 0x1,\
		.REG_CHK_CTRL.reg_checksum_en = 0x0,\
		.SB_REG_CTRL.reg_sb_mode = 0x0,\
		.SB_REG_CTRL.reg_sb_size = 0x0,\
		.SB_REG_CTRL.reg_sb_nb = 0x3,\
		.SB_REG_CTRL.reg_sb_sw_rptr = 0x0,\
		.SB_REG_CTRL.reg_sb_set_str = 0x0,\
		.SB_REG_CTRL.reg_sb_sw_clr = 0x0,\
	}

//#define DEFINE_IMG_IN_C(X) IMG_IN_C X = _DEFINE_IMG_IN_C
#endif /* ifdef __cplusplus */
#endif //__REG_IMG_IN_STRUCT_H__