// $Module: ive_ccl $
// $RegisterBank Version: V 1.0.00 $
// $Author: andy.tsao $
// $Date: Tue, 07 Dec 2021 11:00:20 AM $
//

#ifndef __REG_IVE_CCL_STRUCT_H__
#define __REG_IVE_CCL_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*0: ccl 4c ; 1: ccl 8c;*/
		uint32_t reg_ccl_mode:1;
		uint32_t rsv_1_3:3;
		/*[0:1];*/
		uint32_t reg_ccl_shdw_sel:1;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_00_C;
typedef union {
	struct {
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_thr:16;
		/*unsigned 16b; min area = 1;*/
		uint32_t reg_ccl_area_step:16;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_01_C;
typedef union {
	struct {
		/*0: no force; 1:force clk;*/
		uint32_t reg_force_clk_enable:1;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_02_C;
typedef union {
	struct {
		/*total region number,[0:254];*/
		uint32_t reg_ccl_region_num:8;
		/*8h0: label successful; 8hff: label failed ;*/
		uint32_t reg_ccl_label_status:8;
		/*current region area threshold;*/
		uint32_t reg_ccl_cur_area_thr:16;
	};
	uint32_t val;
} IVE_CCL_REG_CCL_03_C;
typedef struct {
	IVE_CCL_REG_CCL_00_C REG_CCL_00;
	IVE_CCL_REG_CCL_01_C REG_CCL_01;
	IVE_CCL_REG_CCL_02_C REG_CCL_02;
	IVE_CCL_REG_CCL_03_C REG_CCL_03;
} IVE_CCL_C;
#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_CCL_C \
	{\
		.REG_CCL_00.reg_ccl_mode = 0x0,\
		.REG_CCL_00.reg_ccl_shdw_sel = 0x1,\
		.REG_CCL_01.reg_ccl_area_thr = 0x1,\
		.REG_CCL_01.reg_ccl_area_step = 0x1,\
		.REG_CCL_02.reg_force_clk_enable = 0x0,\
		.REG_CCL_03.reg_ccl_region_num = 0x0,\
		.REG_CCL_03.reg_ccl_label_status = 0x0,\
		.REG_CCL_03.reg_ccl_cur_area_thr = 0x0,\
	}
//#define DEFINE_IVE_CCL_C(X) IVE_CCL_C X = _DEFINE_IVE_CCL_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_CCL_STRUCT_H__
