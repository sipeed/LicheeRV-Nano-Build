// $Module: ive_gmm $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:32 AM $
//

#ifndef __REG_IVE_GMM_STRUCT_H__
#define __REG_IVE_GMM_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*if(frm_cnt> 500) reg_gmm_learn_rate = 131; otherwise, reg_gmm_learn_rate = 65535/(frm_cnt+1);*/
		uint32_t reg_gmm_learn_rate:16;
		/*unsigned 16b, threshold for weighted sum ratio;*/
		uint32_t reg_gmm_bg_ratio:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_0_C;
typedef union {
	struct {
		/*unsigned 8.8f;*/
		uint32_t reg_gmm_var_thr:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_1_C;
typedef union {
	struct {
		/*reg_gmm_noise_var = 225*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_noise_var:32;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_2_C;
typedef union {
	struct {
		/*reg_gmm_max_thr = 2000*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_max_var:32;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_3_C;
typedef union {
	struct {
		/*reg_gmm_min_thr = 200*1024*ch_num(rgb:3,y_only:1);*/
		uint32_t reg_gmm_min_var:32;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_4_C;
typedef union {
	struct {
		/*unsigned 32b;*/
		uint32_t reg_gmm_init_weight:32;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_5_C;
typedef union {
	struct {
		/*0: enable detect_shadow; 1: disable detect shadow;*/
		uint32_t reg_gmm_detect_shadow:8;
		/*unsigned 8b;*/
		uint32_t reg_gmm_shadow_thr:8;
		/*unsigned 8b;*/
		uint32_t reg_gmm_sns_factor:8;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_6_C;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_life_update_factor = 0xffff;*/
		uint32_t reg_gmm2_life_update_factor:16;
		/*;*/
		uint32_t reg_gmm2_var_rate:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_7_C;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_freq_update_factor = (frm_cnt >= 500) ? 0xffa0 : 0xfc00;*/
		uint32_t reg_gmm2_freq_redu_factor:16;
		/*unsigned 9.8f; (16x16)<<7;*/
		uint32_t reg_gmm2_max_var:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_8_C;
typedef union {
	struct {
		/*unsigned 9.8f; (8x8)<<7;*/
		uint32_t reg_gmm2_min_var:16;
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_add_factor:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_9_C;
typedef union {
	struct {
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_init:16;
		/*unsigned 16b;*/
		uint32_t reg_gmm2_freq_thr:16;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_10_C;
typedef union {
	struct {
		/*unsigned 16b;*/
		uint32_t reg_gmm2_life_thr:16;
		/*unsigned 8b;*/
		uint32_t reg_gmm2_sns_factor:8;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_11_C;
typedef union {
	struct {
		/*unsigned 16b; reg_gmm2_factor = (frm_cnt>500) ? 0x0408: (0xffff/frm_cnt);*/
		uint32_t reg_gmm2_factor:16;
		/*1: factor = reg_gmm2_factor[15:8]; 0: factor = reg_gmm2_life_update_factor;*/
		uint32_t reg_gmm2_life_update_factor_mode:1;
		uint32_t rsv_17_19:3;
		/*1: factor = reg_gmm2_factor[7:0]; 0: factor = reg_gmm2_sns_factor;*/
		uint32_t reg_gmm2_sns_factor_mode:1;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_12_C;
typedef union {
	struct {
		/*00: gmm/gmm2 disable; 01: gmm enable/gmm2 disable;
		10: gmm2 enable/gmm disable; 11: gmm enable/gmm2 disable;
		*/
		uint32_t reg_gmm_gmm2_enable:2;
		uint32_t rsv_2_3:2;
		/*0:rgb; 1:yonly;*/
		uint32_t reg_gmm_gmm2_yonly:1;
		uint32_t rsv_5_7:3;
		/*[0:1];*/
		uint32_t reg_gmm_gmm2_shdw_sel:1;
		uint32_t rsv_9_11:3;
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
		uint32_t rsv_13_15:3;
		/*unsigned 3b,
		if(reg_gmm_gmm2_enable == 2'b01) model_num = [3:5];
		if(reg_gmm_gmm2_enable == 2'b10) model_num = [1:5];
		else don't care;*/
		uint32_t reg_gmm_gmm2_model_num:3;
		uint32_t rsv_19_19:1;
		/*prob model sel[0:4], choose one model to prob;*/
		uint32_t reg_prob_model_sel:3;
		uint32_t rsv_23_23:1;
		/*prob model byte sel,[0:15], choose one model byte to prob
		if(byte_sel == i), prob model_byte[i], I = [0:11]
		else if(byte_sel == 12) prob bg data,
		else if(byte_sel== 13) prob fg data,
		else if(byte_sel == 14) prob match info;*/
		uint32_t reg_prob_byte_sel:4;
		/*prob bg data sel;*/
		uint32_t reg_prob_bg_sel:2;
		/*prob enable;*/
		uint32_t reg_prob_en:1;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_13_C;
typedef union {
	struct {
		/*line to prob;*/
		uint32_t reg_prob_line:12;
		/*pix to prob;*/
		uint32_t reg_prob_pix:12;
		/*prob byte data;*/
		uint32_t reg_prob_byte_data:8;
	};
	uint32_t val;
} IVE_GMM_REG_GMM_14_C;
typedef struct {
	IVE_GMM_REG_GMM_0_C REG_GMM_0;
	IVE_GMM_REG_GMM_1_C REG_GMM_1;
	IVE_GMM_REG_GMM_2_C REG_GMM_2;
	IVE_GMM_REG_GMM_3_C REG_GMM_3;
	IVE_GMM_REG_GMM_4_C REG_GMM_4;
	IVE_GMM_REG_GMM_5_C REG_GMM_5;
	IVE_GMM_REG_GMM_6_C REG_GMM_6;
	uint32_t _REG_GMM_7_0; // 0x1C
	IVE_GMM_REG_GMM_7_C REG_GMM_7;
	IVE_GMM_REG_GMM_8_C REG_GMM_8;
	IVE_GMM_REG_GMM_9_C REG_GMM_9;
	IVE_GMM_REG_GMM_10_C REG_GMM_10;
	IVE_GMM_REG_GMM_11_C REG_GMM_11;
	IVE_GMM_REG_GMM_12_C REG_GMM_12;
	IVE_GMM_REG_GMM_13_C REG_GMM_13;
	IVE_GMM_REG_GMM_14_C REG_GMM_14;
} IVE_GMM_C;

#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_GMM_C \
{\
	.REG_GMM_0.reg_gmm_learn_rate = 0xffff,\
	.REG_GMM_0.reg_gmm_bg_ratio = 0xb333,\
	.REG_GMM_1.reg_gmm_var_thr = 0x640,\
	.REG_GMM_2.reg_gmm_noise_var = 0xa8c00,\
	.REG_GMM_3.reg_gmm_max_var = 0x5dc000,\
	.REG_GMM_4.reg_gmm_min_var = 0x96000,\
	.REG_GMM_5.reg_gmm_init_weight = 0xccd,\
	.REG_GMM_6.reg_gmm_detect_shadow = 0x1,\
	.REG_GMM_6.reg_gmm_shadow_thr = 0x0,\
	.REG_GMM_6.reg_gmm_sns_factor = 0x8,\
	.REG_GMM_7.reg_gmm2_life_update_factor = 0xffff,\
	.REG_GMM_7.reg_gmm2_var_rate = 0x1,\
	.REG_GMM_8.reg_gmm2_freq_redu_factor = 0xff00,\
	.REG_GMM_8.reg_gmm2_max_var = 0x8000,\
	.REG_GMM_9.reg_gmm2_min_var = 0x2000,\
	.REG_GMM_9.reg_gmm2_freq_add_factor = 0xef,\
	.REG_GMM_10.reg_gmm2_freq_init = 0x4e20,\
	.REG_GMM_10.reg_gmm2_freq_thr = 0x2ee0,\
	.REG_GMM_11.reg_gmm2_life_thr = 0x1388,\
	.REG_GMM_11.reg_gmm2_sns_factor = 0x8,\
	.REG_GMM_12.reg_gmm2_factor = 0xffff,\
	.REG_GMM_12.reg_gmm2_life_update_factor_mode = 0x1,\
	.REG_GMM_12.reg_gmm2_sns_factor_mode = 0x1,\
	.REG_GMM_13.reg_gmm_gmm2_enable = 0x0,\
	.REG_GMM_13.reg_gmm_gmm2_yonly = 0x1,\
	.REG_GMM_13.reg_gmm_gmm2_shdw_sel = 0x1,\
	.REG_GMM_13.reg_force_clk_enable = 0x0,\
	.REG_GMM_13.reg_gmm_gmm2_model_num = 0x3,\
	.REG_GMM_13.reg_prob_model_sel = 0x0,\
	.REG_GMM_13.reg_prob_byte_sel = 0x0,\
	.REG_GMM_13.reg_prob_bg_sel = 0x0,\
	.REG_GMM_13.reg_prob_en = 0x0,\
	.REG_GMM_14.reg_prob_line = 0x0,\
	.REG_GMM_14.reg_prob_pix = 0x0,\
	.REG_GMM_14.reg_prob_byte_data = 0x0,\
}
//#define DEFINE_IVE_GMM_C(X) IVE_GMM_C X = _DEFINE_IVE_GMM_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_GMM_STRUCT_H__
