// $Module: ive_dma $
// $RegisterBank Version: V 1.0.00 $
// $Author:  $
// $Date: Wed, 03 Nov 2021 05:09:55 PM $
//

#ifndef __REG_IVE_DMA_STRUCT_H__
#define __REG_IVE_DMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*ip enable;*/
		uint32_t reg_ive_dma_enable:1;
		/*reg shdw sel;*/
		uint32_t reg_shdw_sel:1;
		/*soft reset for pipe engine;*/
		uint32_t reg_softrst:1;
		uint32_t rsv_3_3:1;
		/*dma mode control
		0 : direct copy
		1 : interval copy
		2 : set 3bytes
		3 : set 8bytes;*/
		uint32_t reg_ive_dma_mode:2;
		uint32_t rsv_7_6:2;
		uint32_t reg_force_clk_enable:1;
		uint32_t reg_force_rdma_disable:1;
		uint32_t reg_force_wdma_disable:1;
	};
	uint32_t val;
} IVE_DMA_REG_0_C;
typedef union {
	struct {
		/*source stride;*/
		uint32_t reg_ive_dma_src_stride:16;
		/*Destination stride;*/
		uint32_t reg_ive_dma_dst_stride:16;
	};
	uint32_t val;
} IVE_DMA_REG_1_C;
typedef union {
	struct {
		/*source memory address;*/
		uint32_t reg_ive_dma_src_mem_addr:32;
	};
	uint32_t val;
} IVE_DMA_REG_2_C;
typedef union {
	struct {
		/*Destination memory address;*/
		uint32_t reg_ive_dma_dst_mem_addr:32;
	};
	uint32_t val;
} IVE_DMA_REG_3_C;
typedef union {
	struct {
		/*horizotal segment size;*/
		uint32_t reg_ive_dma_horsegsize:8;
		/*element size;*/
		uint32_t reg_ive_dma_elemsize:8;
		/*vertical segment row;*/
		uint32_t reg_ive_dma_versegrow:8;
	};
	uint32_t val;
} IVE_DMA_REG_4_C;
typedef union {
	struct {
		/*U64 value;*/
		uint32_t reg_ive_dma_u64_val[2];
	};
	uint32_t val[2];
} IVE_DMA_REG_5_C;
typedef struct {
	IVE_DMA_REG_0_C REG_0;
	IVE_DMA_REG_1_C REG_1;
	IVE_DMA_REG_2_C REG_2;
	IVE_DMA_REG_3_C REG_3;
	IVE_DMA_REG_4_C REG_4;
	IVE_DMA_REG_5_C REG_5;
} IVE_DMA_C;

#ifdef __cplusplus
#error "removed."
#else /* !ifdef __cplusplus */
#define _DEFINE_IVE_DMA_C \
{\
	.REG_0.reg_ive_dma_enable = 0x0,\
	.REG_0.reg_shdw_sel = 0x1,\
	.REG_0.reg_softrst = 0x0,\
	.REG_0.reg_ive_dma_mode = 0x0,\
	.REG_0.reg_force_clk_enable = 0x1,\
	.REG_0.reg_force_rdma_disable = 0x0,\
	.REG_0.reg_force_wdma_disable = 0x0,\
	.REG_1.reg_ive_dma_src_stride = 0x0,\
	.REG_1.reg_ive_dma_dst_stride = 0x0,\
	.REG_2.reg_ive_dma_src_mem_addr = 0x1,\
	.REG_3.reg_ive_dma_dst_mem_addr = 0x1,\
	.REG_4.reg_ive_dma_horsegsize = 0x0,\
	.REG_4.reg_ive_dma_elemsize = 0x0,\
	.REG_4.reg_ive_dma_versegrow = 0x0,\
	.REG_5.reg_ive_dma_u64_val = {0, 0},\
}
//#define DEFINE_IVE_DMA_C(X) IVE_DMA_C X = _DEFINE_IVE_DMA_C
#endif /* ifdef __cplusplus */
#endif //__REG_IVE_DMA_STRUCT_H__
