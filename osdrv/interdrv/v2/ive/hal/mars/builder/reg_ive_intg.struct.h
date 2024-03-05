// $Module: ive_intg $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:10:29 PM $
//

#ifndef __REG_IVE_INTG_STRUCT_H__
#define __REG_IVE_INTG_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*ip enable;*/
		uint32_t reg_ive_intg_enable:1;
		/*reg shdw sel;*/
		uint32_t reg_shdw_sel:1;
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		uint32_t rsv_3_3:1;
		/*tile number, 0 for no tile;*/
		uint32_t reg_ive_intg_tile_nm:4;
		/*output mode control
		0 : sum+sq_sum
		1 : sum
		2 : sq_sum;*/
		uint32_t reg_ive_intg_ctrl:2;
	};
	uint32_t val;
} IVE_INTG_REG_0_C;
typedef union {
	struct {
		/*stride;*/
		uint32_t reg_ive_intg_stride:16;
	};
	uint32_t val;
} IVE_INTG_REG_1_C;
typedef union {
	struct {
		/*memory address;*/
		uint32_t reg_ive_intg_mem_addr:32;
	};
	uint32_t val;
} IVE_INTG_REG_2_C;
typedef struct {
	IVE_INTG_REG_0_C REG_0;
	IVE_INTG_REG_1_C REG_1;
	IVE_INTG_REG_2_C REG_2;
} IVE_INTG_C;
#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_INTG_C \
{\
	.REG_0.reg_ive_intg_enable = 0x0,\
	.REG_0.reg_shdw_sel = 0x1,\
	.REG_0.reg_softrst = 0x0,\
	.REG_0.reg_ive_intg_tile_nm = 0x0,\
	.REG_0.reg_ive_intg_ctrl = 0x0,\
	.REG_1.reg_ive_intg_stride = 0x0,\
	.REG_2.reg_ive_intg_mem_addr = 0x1,\
}
//#define DEFINE_IVE_INTG_C(X) IVE_INTG_C X = _DEFINE_IVE_INTG_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_INTG_STRUCT_H__
