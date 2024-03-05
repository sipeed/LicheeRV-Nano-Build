/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: tpu_platform.h
 * Description: hw driver header file
 */

#ifndef __TPU_PLATFORM_H__
#define __TPU_PLATFORM_H__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include "cvi_tpu_interface.h"

struct dma_hdr_t {
	uint16_t dmabuf_magic_m;
	uint16_t dmabuf_magic_s;
	uint32_t dmabuf_size;
	uint32_t cpu_desc_count;
	uint32_t bd_desc_count; //16bytes
	uint32_t tdma_desc_count;
	uint32_t tpu_clk_rate;
	uint32_t pmubuf_size;
	uint32_t pmubuf_offset; //32bytes
	uint32_t arraybase_0_L;
	uint32_t arraybase_0_H;
	uint32_t arraybase_1_L;
	uint32_t arraybase_1_H; //48bytes
	uint32_t arraybase_2_L;
	uint32_t arraybase_2_H;
	uint32_t arraybase_3_L;
	uint32_t arraybase_3_H; //64bytes

	uint32_t arraybase_4_L;
	uint32_t arraybase_4_H;
	uint32_t arraybase_5_L;
	uint32_t arraybase_5_H;
	uint32_t arraybase_6_L;
	uint32_t arraybase_6_H;
	uint32_t arraybase_7_L;
	uint32_t arraybase_7_H;
	uint32_t reserve[8];   //128bytes, 128bytes align
};
struct TPU_PLATFORM_CFG {
	uint8_t *iomem_tdmaBase;
	uint8_t *iomem_tiuBase;
	uint32_t pmubuf_size;
	uint64_t pmubuf_addr_p;
};

struct tpu_tee_load_info {
	//ree
	uint64_t cmdbuf_addr_ree;
	uint32_t cmdbuf_len_ree;
	uint64_t weight_addr_ree;
	uint32_t weight_len_ree;
	uint64_t neuron_addr_ree;

	//tee
	uint64_t dmabuf_addr_tee;
};

struct tpu_tee_submit_info {
  //tee
	uint64_t dmabuf_paddr;
	uint64_t gaddr_base2;
	uint64_t gaddr_base3;
	uint64_t gaddr_base4;
	uint64_t gaddr_base5;
	uint64_t gaddr_base6;
	uint64_t gaddr_base7;
};

enum TPU_SEC_SMCCALL {
	TPU_SEC_SMC_LOADCMD = 0x1001,
	TPU_SEC_SMC_RUN,
	TPU_SEC_SMC_WAIT,
};


int platform_loadcmdbuf_tee(struct cvi_tpu_device *ndev, struct tpu_tee_load_info *p_info);
int platform_run_dmabuf_tee(struct cvi_tpu_device *ndev, struct tpu_tee_submit_info *p_info);
int platform_unload_tee(struct cvi_tpu_device *ndev, uint64_t paddr, uint64_t size);

irqreturn_t platform_tdma_irq(struct cvi_tpu_device *ndev);
int platform_run_dmabuf(struct cvi_tpu_device *ndev, void *dmabuf_v, uint64_t dmabuf_p);

int platform_tpu_suspend(struct cvi_tpu_device *ndev);
int platform_tpu_resume(struct cvi_tpu_device *ndev);
int platform_tpu_open(struct cvi_tpu_device *ndev);
int platform_tpu_reset(struct cvi_tpu_device *ndev);
int platform_tpu_init(struct cvi_tpu_device *ndev);
void platform_tpu_deinit(struct cvi_tpu_device *ndev);
void platform_tpu_spll_divide(struct cvi_tpu_device *ndev, u32 div);
int platform_tpu_probe_setting(void);
int platform_run_pio(struct cvi_tpu_device *ndev, struct tpu_tdma_pio_info *info);

#define RAW_READ32(addr) readl(addr)
#define RAW_WRITE32(addr, value) writel(value, addr)


#endif
