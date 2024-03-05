/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_regcpu.h
 * Description: tpu dma_buf header structure
 */

#ifndef __CVI_REG_CPU_H__
#define __CVI_REG_CPU_H__

#define CPU_ENGINE_DESCRIPTOR_NUM 56

// CPU_OP_SYNC structure
struct cvi_cpu_sync_desc_t {
	u32 op_type; // CPU_CMD_ACCPI0
	u32 num_bd; // CPU_CMD_ACCPI1
	u32 num_gdma; // CPU_CMD_ACCPI2
	u32 offset_bd; // CPU_CMD_ACCPI3
	u32 offset_gdma; // CPU_CMD_ACCPI4
	u32 reserved[2]; // CPU_CMD_ACCPI5-CPU_CMD_ACCPI6
	char str[(CPU_ENGINE_DESCRIPTOR_NUM - 7) * sizeof(u32)];
};

#endif
