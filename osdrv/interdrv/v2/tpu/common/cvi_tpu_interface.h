/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_tpu_interface.h
 * Description: tpu driver interface header file
 */

#ifndef __CVI_TPU_INTERFACE_H__
#define __CVI_TPU_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/dma-direction.h>

#include "cvi_regcpu.h"

struct cvi_kernel_work {
	struct task_struct *work_thread;
	wait_queue_head_t task_wait_queue;
	wait_queue_head_t done_wait_queue;
	struct list_head task_list;
	spinlock_t task_list_lock;
	struct list_head done_list;
	spinlock_t done_list_lock;
};

struct cvi_tpu_device {
	struct device *dev;
	struct reset_control *rst_tdma;
	struct reset_control *rst_tpu;
	struct reset_control *rst_tpusys;
	struct clk *clk_tpu_axi;
	struct clk *clk_tpu_fab;
	dev_t cdev_id;
	struct cdev cdev;
	struct completion tdma_done;
	u8 __iomem *tdma_vaddr;
	u8 __iomem *tiu_vaddr;
	int tdma_irq;
	struct mutex dev_lock;
	spinlock_t close_lock;
	int use_count;
	int running_count;
	int suspend_count;
	int resume_count;
	void *private_data;
	struct cvi_kernel_work kernel_work;
};

struct CMD_ID_NODE {
	unsigned int bd_cmd_id;
	unsigned int tdma_cmd_id;
};

struct tpu_tdma_pio_info {
	uint64_t paddr_src;
	uint64_t paddr_dst;
	uint32_t h;
	uint32_t w_bytes;
	uint32_t stride_bytes_src;
	uint32_t stride_bytes_dst;
	uint32_t enable_2d;
	uint32_t leng_bytes;
};

#endif /* __CVI_TPU_INTERFACE_H__ */
