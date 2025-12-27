/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_jpeg.h
 * Description: jpeg system interface definition
 */

#ifndef __CVI_JPEG_H__
#define __CVI_JPEG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include "jpuconfig.h"
#include "jmm.h"

#define USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY

/* global device context to avoid kernal config mismatch in filp->private_data */
#define JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT

typedef struct jpudrv_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base; /* kernel logical address in use kernel */
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} jpudrv_buffer_t;

/* To track the allocated memory buffer */
struct jpudrv_buffer_pool_t {
	struct list_head list;
	struct jpudrv_buffer_t jb;
	struct file *filp;
};

struct jpudrv_instance_pool_t {
	unsigned char jpgInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
};

/* To track the instance index and buffer in instance pool */
struct jpudrv_instanace_list_t {
	struct list_head list;
	unsigned long inst_idx;
	struct file *filp;
};

#define JPU_QUIRK_SUPPORT_CLOCK_CONTROL		(1<<0)
#define JPU_QUIRK_SUPPORT_SWITCH_TO_PLL		(1<<1)
#define JPU_QUIRK_SUPPORT_FPGA			(1<<2)

struct jpu_pltfm_data {
	const struct jpu_ops *ops;
	unsigned int quirks;
	unsigned int version;
};

struct cvi_jpu_device {
	struct device *dev;

	dev_t cdev_id;
	struct cdev cdev;

	int s_jpu_major;
	struct class *jpu_class;

	int jpu_irq;
	int interrupt_flag;

	// inc when JDI_IOCTL_OPEN_INSTANCE is called, dec when JDI_IOCTL_CLOSE_INSTANCE is called
	int open_instance_count;

	// inc when jpu_open is called, dec when jpu_release is called
	int process_count;

	struct clk *clk_axi_video_codec;
	struct clk *clk_jpeg;
	struct clk *clk_apb_jpeg;
	struct clk *clk_vc_src0;
	struct clk *clk_vc_src1;
	struct clk *clk_vc_src2;
	struct clk *clk_cfg_reg_vc;

	jpudrv_buffer_t jpu_instance_pool;
	struct jpudrv_buffer_t video_memory;
	struct jpu_mm_struct jmem;
	struct jpudrv_buffer_t jpu_register;
	struct jpudrv_buffer_t jpu_control_register;

	wait_queue_head_t interrupt_wait_q;
	spinlock_t jpu_lock;
	struct fasync_struct *async_queue;
	struct list_head s_jbp_head;
	struct list_head s_jpu_inst_list_head;

	const struct jpu_pltfm_data *pdata;
};

struct jpu_ops {
	void	(*clk_get)(struct cvi_jpu_device *jdev);
	void	(*clk_put)(struct cvi_jpu_device *jdev);
	void	(*clk_enable)(struct cvi_jpu_device *jdev);
	void	(*clk_disable)(struct cvi_jpu_device *jdev);
	void	(*config_pll)(struct cvi_jpu_device *jdev);
};

struct jpu_intr_info_t {
	unsigned int timeout;
	int intr_reason;
};

#define JPEG_CLK_ENABLE		1
#define JPEG_CLK_DISABLE	0

extern bool __clk_is_enabled(struct clk *clk);

#ifdef __cplusplus
}
#endif

#endif // __CVI_JPEG_H__
