/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_ive_interface.h
 * Description: ive driver interface header file
 */

#ifndef __CVI_IVE_INTERFACE_H__
#define __CVI_IVE_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/version.h>

#ifdef DEBUG
#define CVI_DBG_INFO(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define CVI_DBG_INFO(fmt, ...)
#endif

struct cvi_ive_device {
	struct device *dev;
	struct cdev cdev;
	void __iomem *ive_base; /* NULL if not initialized. */
	int ive_irq; /* alarm and periodic irq */
	struct proc_dir_entry *proc_dir;
	struct clk *clk;
	spinlock_t close_lock;
	struct completion frame_done;
	struct completion op_done;
	int cur_optype;
	int tile_num;
	int total_tile;
	int use_count;
	uintptr_t *private_data;
};

struct ive_profiling_info {
	char op_name[16];
	int tile_num;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 time_ioctl_start;
	struct timespec64 time_ioctl_end;
	struct timespec64 time_vld_start;
	struct timespec64 time_vld_end;
#else
	struct timespec time_ioctl_start;
	struct timespec time_ioctl_end;
	struct timespec time_vld_start;
	struct timespec time_vld_end;
#endif
	uint32_t time_ioctl_diff_us;
	uint32_t time_vld_diff_us[6];
	uint32_t time_tile_diff_us;
};

void start_vld_time(int optype);
void stop_vld_time(int optype, int tile_num);
#endif /* __CVI_IVE_INTERFACE_H__ */
