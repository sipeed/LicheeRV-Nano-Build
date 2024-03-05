// $Module: ive_update_bg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Tue, 07 Dec 2021 11:01:06 AM $
//

#ifndef __REG_IVE_UPDATE_BG_STRUCT_H__
#define __REG_IVE_UPDATE_BG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_1_C;
typedef union {
	struct {
		/*set 1 as gradfg active to enable rdma  ;*/
		uint32_t reg_enable:1;
		/*set 1: clk force enable  0: autocg gating ;*/
		uint32_t reg_ck_en:1;
		/*set 1: bypass fg / bg model0 /bg model1;*/
		uint32_t reg_updatebg_byp_model:1;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_H04_C;
typedef union {
	struct {
		/*reg shdw sel;*/
		uint32_t reg_shdw_sel:1;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_2_C;
typedef union {
	struct {
		/*dmy;*/
		uint32_t reg_ctrl_dmy1:32;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_3_C;
typedef union {
	struct {
		/*cuttent frame number;*/
		uint32_t reg_u32CurFrmNum:32;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl0_C;
typedef union {
	struct {
		/*previouse (bg update) frame number;*/
		uint32_t reg_u32PreChkTime:32;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl1_C;
typedef union {
	struct {
		/*time interval of Bg Update (unit: frame) [0,2000];*/
		uint32_t reg_u32FrmChkPeriod:12;
		uint32_t rsv_12_15:4;
		/*the shortest time of Bg Update [20,6000];*/
		uint32_t reg_u32InitMinTime:13;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl2_C;
typedef union {
	struct {
		/*the shortest time of static bg ：[u32InitMinTime, 6000];*/
		uint32_t reg_u32StyBgMinBlendTime:16;
		/*the longest time of static bg ：[u32StyBgMinBlendTime, 40000];*/
		uint32_t reg_u32StyBgMaxBlendTime:16;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl3_C;
typedef union {
	struct {
		/*the shortest time of dynamic bg  [0,6000];*/
		uint32_t reg_u32DynBgMinBlendTime:13;
		uint32_t rsv_13_15:3;
		/*the longest time of Bg detect [20,6000];*/
		uint32_t reg_u32StaticDetMinTime:13;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl4_C;
typedef union {
	struct {
		/*the longest time of Fg is Fading [1,255];*/
		uint32_t reg_u16FgMaxFadeTime:8;
		/*the longest time of Bg is Fading [1,255];*/
		uint32_t reg_u16BgMaxFadeTime:8;
		/*static bg access rate [10,100];*/
		uint32_t reg_u8StyBgAccTimeRateThr:7;
		/*change bg access rate [10,100];*/
		uint32_t reg_u8ChgBgAccTimeRateThr:7;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl5_C;
typedef union {
	struct {
		/*dynamic bg access rate [0,50];*/
		uint32_t reg_u8DynBgAccTimeThr:6;
		uint32_t rsv_6_7:2;
		/*bg intial status rate [90,100];*/
		uint32_t reg_u8BgEffStaRateThr:7;
		uint32_t rsv_15_15:1;
		/*dynamic bg depth [0,3];*/
		uint32_t reg_u8DynBgDepth:2;
		uint32_t rsv_18_23:6;
		/*speed up bg learning [0,1];*/
		uint32_t reg_u8AcceBgLearn:1;
		/*1: detect change region ;*/
		uint32_t reg_u8DetChgRegion:1;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl6_C;
typedef union {
	struct {
		/*bg number;*/
		uint32_t reg_stat_pixnum:32;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl7_C;
typedef union {
	struct {
		/*sum of bg img;*/
		uint32_t reg_stat_sumlum:32;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_ctrl8_C;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop start x ;*/
		uint32_t reg_crop_start_x:16;
		/*crop to wdma for tile mode usage : crop end x ;*/
		uint32_t reg_crop_end_x:16;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_crop_s_C;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop start y ;*/
		uint32_t reg_crop_start_y:16;
		/*crop to wdma for tile mode usage : crop end y ;*/
		uint32_t reg_crop_end_y:16;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_crop_e_C;
typedef union {
	struct {
		/*crop to wdma for tile mode usage : crop enable;*/
		uint32_t reg_crop_enable:1;
	};
	uint32_t val;
} IVE_UPDATE_BG_REG_crop_ctl_C;
typedef struct {
	IVE_UPDATE_BG_REG_1_C REG_1;
	IVE_UPDATE_BG_REG_H04_C REG_H04;
	IVE_UPDATE_BG_REG_2_C REG_2;
	IVE_UPDATE_BG_REG_3_C REG_3;
	IVE_UPDATE_BG_REG_ctrl0_C REG_ctrl0;
	IVE_UPDATE_BG_REG_ctrl1_C REG_ctrl1;
	IVE_UPDATE_BG_REG_ctrl2_C REG_ctrl2;
	IVE_UPDATE_BG_REG_ctrl3_C REG_ctrl3;
	IVE_UPDATE_BG_REG_ctrl4_C REG_ctrl4;
	IVE_UPDATE_BG_REG_ctrl5_C REG_ctrl5;
	IVE_UPDATE_BG_REG_ctrl6_C REG_ctrl6;
	IVE_UPDATE_BG_REG_ctrl7_C REG_ctrl7;
	IVE_UPDATE_BG_REG_ctrl8_C REG_ctrl8;
	IVE_UPDATE_BG_REG_crop_s_C REG_crop_s;
	IVE_UPDATE_BG_REG_crop_e_C REG_crop_e;
	IVE_UPDATE_BG_REG_crop_ctl_C REG_crop_ctl;
} IVE_UPDATE_BG_C;

#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_UPDATE_BG_C \
{\
	.REG_1.reg_softrst = 0x0,\
	.REG_H04.reg_enable = 0x0,\
	.REG_H04.reg_ck_en = 0x1,\
	.REG_H04.reg_updatebg_byp_model = 0x0,\
	.REG_2.reg_shdw_sel = 0x1,\
	.REG_3.reg_ctrl_dmy1 = 0x0,\
	.REG_ctrl0.reg_u32CurFrmNum = 0x0,\
	.REG_ctrl1.reg_u32PreChkTime = 0x0,\
	.REG_ctrl2.reg_u32FrmChkPeriod = 0x32,\
	.REG_ctrl2.reg_u32InitMinTime = 0x64,\
	.REG_ctrl3.reg_u32StyBgMinBlendTime = 0xC8,\
	.REG_ctrl3.reg_u32StyBgMaxBlendTime = 0xC8,\
	.REG_ctrl4.reg_u32DynBgMinBlendTime = 0x0,\
	.REG_ctrl4.reg_u32StaticDetMinTime = 0x50,\
	.REG_ctrl5.reg_u16FgMaxFadeTime = 0xF,\
	.REG_ctrl5.reg_u16BgMaxFadeTime = 0x3C,\
	.REG_ctrl5.reg_u8StyBgAccTimeRateThr = 0x50,\
	.REG_ctrl5.reg_u8ChgBgAccTimeRateThr = 0x3C,\
	.REG_ctrl6.reg_u8DynBgAccTimeThr = 0x0,\
	.REG_ctrl6.reg_u8BgEffStaRateThr = 0x5A,\
	.REG_ctrl6.reg_u8DynBgDepth = 0x3,\
	.REG_ctrl6.reg_u8AcceBgLearn = 0x0,\
	.REG_ctrl6.reg_u8DetChgRegion = 0x0,\
	.REG_ctrl7.reg_stat_pixnum = 0x0,\
	.REG_ctrl8.reg_stat_sumlum = 0x0,\
	.REG_crop_s.reg_crop_start_x = 0x0,\
	.REG_crop_s.reg_crop_end_x = 0x0,\
	.REG_crop_e.reg_crop_start_y = 0x0,\
	.REG_crop_e.reg_crop_end_y = 0x0,\
	.REG_crop_ctl.reg_crop_enable = 0x0,\
}
//#define DEFINE_IVE_UPDATE_BG_C(X) IVE_UPDATE_BG_C X = _DEFINE_IVE_UPDATE_BG_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_UPDATE_BG_STRUCT_H__
