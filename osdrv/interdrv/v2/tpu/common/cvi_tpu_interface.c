/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_tpu_interface.c
 * Description: tpu kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/uaccess.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <asm/cacheflush.h>
#include <linux/of.h>
#include <linux/version.h>
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/signal.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/dma-map-ops.h>
#endif
#include "cvi_tpu_interface.h"
#include "cvi_tpu_ioctl.h"
#include "tpu_platform.h"

#include "ion.h"

#define CVI_TPU_CDEV_NAME "cvi-tpu"
#define CVI_TPU_CLASS_NAME "cvi-tpu"

enum tpu_submit_path {
	TPU_PATH_DESNORMAL = 0,
	TPU_PATH_DESSEC = 1,
	TPU_PATH_PIOTDMA = 2,
	TPU_PATH_MAX = 3,
};

struct cvi_list_node {
	struct device *dev;
	struct list_head list;
	pid_t pid;
	uint32_t seq_no;
	uint32_t pio_seq_no;
	int dmabuf_fd;
	enum dma_data_direction dma_dir;
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *dma_attach;
	struct sg_table *dma_sgt;
	void *dmabuf_vaddr;
	uint64_t dmabuf_paddr;
	struct tpu_tee_submit_info tee_info;
	enum tpu_submit_path tpu_path;
	int ret;
	struct tpu_tdma_pio_info pio_info;
};

struct tpu_profiling_info {
	uint32_t enable_usage_profiling;
	uint32_t run_current_us;
	uint64_t run_sum_us;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 timer_start;
#else
	struct timespec timer_start;
#endif
	uint32_t timer_current_us;
	uint32_t usage;
};

struct tpu_suspend_info {
	spinlock_t spin_lock;
	uint8_t running_cnt;
};

struct class *npu_class;
static dev_t npu_cdev_id;
struct proc_dir_entry *tpu_proc_dir;
static struct tpu_suspend_info tpu_suspend;

#define STORE_NPU_USAGE 1
#define STORE_INTERVAL 2
#define STORE_ENABLE 3

#define TASK_LIST_MAX 100
#define DONE_LIST_MAX 1000

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
typedef struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
} _timer;
#else
typedef struct timer_list _timer;
#endif //(KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)

static struct tpu_profiling_info info = {0};
static _timer profile_timer;
#define PROFILING_INTERVAL_MS 1000

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);

	lt->function(lt->data);
}
#endif //(KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static uint32_t get_duration_us(const struct timespec64 *start, const struct timespec64 *end)
#else
static uint32_t get_duration_us(const struct timespec *start, const struct timespec *end)
#endif
{
	return (uint32_t)((end->tv_sec - start->tv_sec)*1000000 + (end->tv_nsec - start->tv_nsec)/1000);
}

#define SUAGE_LOOP 2
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
static void tpu_profile_timer_handler(struct timer_list *t)
#else
static void tpu_profile_timer_handler(unsigned long data)
#endif
{
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 timer_end;
#else
	struct timespec timer_end;
#endif
	static uint64_t last_sum_us, dividend;

	//get time interval
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	ktime_get_real_ts64(&timer_end);
#else
	getnstimeofday(&timer_end);
#endif
	info.timer_current_us = get_duration_us(&info.timer_start, &timer_end);

	dividend = (info.run_sum_us - last_sum_us) * 100;
	last_sum_us = info.run_sum_us;
	do_div(dividend, info.timer_current_us);
	info.usage = (uint32_t)dividend;
	if (info.enable_usage_profiling == SUAGE_LOOP) {
		pr_err("timer_dur=%d, sum_us=%lld, usage=%d%%\n", info.timer_current_us, last_sum_us, info.usage);
	}

	//reset next timer
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	mod_timer(&profile_timer.t, jiffies + msecs_to_jiffies(PROFILING_INTERVAL_MS));
#else
	mod_timer(&profile_timer, jiffies + msecs_to_jiffies(PROFILING_INTERVAL_MS));
#endif
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	ktime_get_real_ts64(&info.timer_start);
#else
	getnstimeofday(&info.timer_start);
#endif
}

static void tpu_profile_timer_remove(void)
{
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	if (timer_pending(&profile_timer.t)) {
		del_timer_sync(&profile_timer.t);
		timer_setup(&profile_timer.t, legacy_timer_emu_func, 0);
#else
	if (timer_pending(&profile_timer)) {
		del_timer_sync(&profile_timer);
		init_timer(&profile_timer);
#endif
	}
}

static int __init tpu_profile_timer_init(void)
{
	tpu_profile_timer_remove();
	profile_timer.data = 0;
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	profile_timer.t.function = tpu_profile_timer_handler;
	profile_timer.t.expires = jiffies + msecs_to_jiffies(PROFILING_INTERVAL_MS);
	add_timer(&profile_timer.t);
#else
	profile_timer.function = tpu_profile_timer_handler;
	profile_timer.expires = jiffies + msecs_to_jiffies(PROFILING_INTERVAL_MS);
	add_timer(&profile_timer);
#endif
	return 0;
}

static int tpu_proc_show(struct seq_file *m, void *v)
{
	if (info.enable_usage_profiling) {
		seq_puts(m, "profiling is running\n");
		seq_printf(m, "interval=%dms usage=%d%%\n", PROFILING_INTERVAL_MS, info.usage);
	} else {
		seq_puts(m, "profiling is disabled\n");
	}
	return 0;
}

static ssize_t tpu_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	uint32_t input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("input parameter incorrect,please input again\n exp:echo 1 > /proc/tpu/usage_profiling\n");
		return count;
	}

	//reset related info
	tpu_profile_timer_remove();
	memset(&info, 0, sizeof(struct tpu_profiling_info));

	info.enable_usage_profiling = input_param;
	if (info.enable_usage_profiling) {
		tpu_profile_timer_init();
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&info.timer_start);
#else
		getnstimeofday(&info.timer_start);
#endif
		pr_err("tpu usage profiline is started\n");
	} else {
		pr_err("tpu usage profiling is ended\n");
	}

	return count;
}

static int tpu_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, tpu_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops tpu_proc_ops = {
	.proc_open = tpu_proc_open,
	.proc_read = seq_read,
	.proc_write = tpu_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations tpu_proc_ops = {
	.owner = THIS_MODULE,
	.open = tpu_proc_open,
	.read = seq_read,
	.write = tpu_proc_write,
	.release = single_release,
};
#endif

static irqreturn_t cvi_tpu_tdma_irq(int irq, void *data)
{
	struct cvi_tpu_device *ndev = data;
	irqreturn_t ret;

	spin_lock(&ndev->close_lock);

	if (ndev->use_count == 0) {
		spin_unlock(&ndev->close_lock);
		return IRQ_HANDLED;
	}

	ret = platform_tdma_irq(ndev);

	spin_unlock(&ndev->close_lock);

	return ret;
}

static int cvi_tpu_prepare_buffer(struct cvi_list_node *node)
{
	int ret = 0;
	struct ion_buffer *buffer;
	struct file *fp;
	struct dma_hdr_t *header;

	node->dma_buf = dma_buf_get(node->dmabuf_fd);
	pr_debug("dma_buf=0x%llx, dmabuf_fd=%d\n", (uint64_t)node->dma_buf, node->dmabuf_fd);

	if (IS_ERR(node->dma_buf)) {
		pr_debug("get dma failed\n");
		return -EINVAL;
	}

	ret = dma_buf_begin_cpu_access(node->dma_buf, DMA_TO_DEVICE);
	if (ret) {
		dma_buf_put(node->dma_buf);
		return ret;
	}

	buffer = node->dma_buf->priv;
	node->dmabuf_vaddr = buffer->vaddr;
	pr_debug("v=0x%llx\n", (uint64_t)node->dmabuf_vaddr);

	if (IS_ERR(node->dmabuf_vaddr)) {
		pr_err("got v_addr failed\n");
		ret = -EINVAL;
		dma_buf_put(node->dma_buf);
		return ret;
	}

	node->dmabuf_paddr = buffer->paddr;
	fp = (struct file *)(node->dma_buf->file);
	pr_debug("p=0x%llx, ref_count=%lld\n", (uint64_t)node->dmabuf_paddr, file_count(fp));

	// Check parameters
	header = (struct dma_hdr_t *)node->dmabuf_vaddr;
	if ((header->pmubuf_offset & 0xF) && (header->pmubuf_size & 0xF)) {
		pr_err("error: pmubuf_offset=0x%x, pmubuf_size=0x%x\n", header->pmubuf_offset, header->pmubuf_size);
	}

	if (node->dmabuf_paddr & 0xFFF) {
		pr_err("error: dmabuf_paddr=0x%p\n", node->dmabuf_paddr);
	}

	return 0;
}

static void cvi_tpu_cleanup_buffer(struct cvi_list_node *node)
{
	struct file *fp;

	//do nothing while security path
	if (node->tpu_path == TPU_PATH_DESNORMAL) {

		dma_buf_end_cpu_access(node->dma_buf, DMA_TO_DEVICE);
		dma_buf_put(node->dma_buf);
		fp = (struct file *)(node->dma_buf->file);
		pr_debug("ref_count=%lld\n", file_count(fp));

	}
}

static struct cvi_list_node *
get_from_done_list(struct cvi_kernel_work *kernel_work, u32 seq_no, enum tpu_submit_path path)
{
	struct cvi_list_node *node = NULL;
	struct cvi_list_node *pos;

	spin_lock(&kernel_work->done_list_lock);

	if (path == TPU_PATH_PIOTDMA) {
		list_for_each_entry(pos, &kernel_work->done_list, list) {
			if ((pos->pid == current->pid) && (pos->pio_seq_no == seq_no)) {
				node = pos;
				break;
			}
		}
	} else {
		list_for_each_entry(pos, &kernel_work->done_list, list) {
			if ((pos->pid == current->pid) && (pos->seq_no == seq_no)) {
				node = pos;
				break;
			}
		}
	}
	spin_unlock(&kernel_work->done_list_lock);

	return node;
}

static void remove_from_done_list(struct cvi_kernel_work *kernel_work,
				  struct cvi_list_node *node)
{
	spin_lock(&kernel_work->done_list_lock);
	list_del(&node->list);
	spin_unlock(&kernel_work->done_list_lock);
	vfree(node);
}

static int cvi_tpu_submit(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	u32 task_list_count = 0;
	struct cvi_submit_dma_arg run_dmabuf_arg;
	struct cvi_list_node *node;
	struct cvi_list_node *pos;
	struct cvi_kernel_work *kernel_work;

	ret = copy_from_user(&run_dmabuf_arg,
			     (struct cvi_submit_dma_arg __user *)arg,
			     sizeof(struct cvi_submit_dma_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail\n");
		return ret;
	}

	kernel_work = &ndev->kernel_work;
	while (1) {
		spin_lock(&ndev->kernel_work.task_list_lock);
		list_for_each_entry(pos, &kernel_work->task_list, list) {
			task_list_count++;
		}
		spin_unlock(&ndev->kernel_work.task_list_lock);
		if (task_list_count > TASK_LIST_MAX) {
			dev_info(ndev->dev, "too much task in task list\n");
			msleep(20);
		} else {
			break;
		}
	}

	pr_debug("cvi_tpu_submit path()\n");
	node = vmalloc(sizeof(struct cvi_list_node));
	if (!node)
		return -ENOMEM;
	memset(node, 0, sizeof(struct cvi_list_node));

	node->pid = current->pid;
	node->seq_no = run_dmabuf_arg.seq_no;
	node->dmabuf_fd = run_dmabuf_arg.fd;
	node->dev = ndev->dev;
	node->tpu_path = TPU_PATH_DESNORMAL;
	cvi_tpu_prepare_buffer(node);

	spin_lock(&ndev->kernel_work.task_list_lock);
	list_add_tail(&node->list, &ndev->kernel_work.task_list);
	wake_up_interruptible(&ndev->kernel_work.task_wait_queue);
	spin_unlock(&ndev->kernel_work.task_list_lock);

	return 0;
}

static int cvi_tpu_submit_tee(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	u32 task_list_count = 0;
	struct cvi_submit_tee_arg ioctl_arg;
	struct cvi_list_node *node;
	struct cvi_list_node *pos;
	struct cvi_kernel_work *kernel_work;

	ret = copy_from_user(&ioctl_arg,
			     (struct cvi_submit_tee_arg __user *)arg,
			     sizeof(struct cvi_submit_tee_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail\n");
		return ret;
	}

	kernel_work = &ndev->kernel_work;
	while (1) {
		spin_lock(&ndev->kernel_work.task_list_lock);
		list_for_each_entry(pos, &kernel_work->task_list, list) {
			task_list_count++;
		}
		spin_unlock(&ndev->kernel_work.task_list_lock);
		if (task_list_count > TASK_LIST_MAX) {
			dev_info(ndev->dev, "too much task in task list\n");
			msleep(20);
		} else {
			break;
		}
	}

	pr_debug("cvi_tpu_submit_tee path()\n");
	node = vmalloc(sizeof(struct cvi_list_node));
	if (!node)
		return -ENOMEM;
	memset(node, 0, sizeof(struct cvi_list_node));

	node->pid = current->pid;
	node->seq_no = ioctl_arg.seq_no;
	node->dmabuf_vaddr = NULL;
	node->tee_info.dmabuf_paddr = ioctl_arg.dmabuf_tee_addr;
	node->tee_info.gaddr_base2 = ioctl_arg.gaddr_base2;
	node->tee_info.gaddr_base3 = ioctl_arg.gaddr_base3;
	node->tee_info.gaddr_base4 = ioctl_arg.gaddr_base4;
	node->tee_info.gaddr_base5 = ioctl_arg.gaddr_base5;
	node->tee_info.gaddr_base6 = ioctl_arg.gaddr_base6;
	node->tee_info.gaddr_base7 = ioctl_arg.gaddr_base7;
	node->dev = ndev->dev;
	node->tpu_path = TPU_PATH_DESSEC;

	spin_lock(&ndev->kernel_work.task_list_lock);
	list_add_tail(&node->list, &ndev->kernel_work.task_list);
	wake_up_interruptible(&ndev->kernel_work.task_wait_queue);
	spin_unlock(&ndev->kernel_work.task_list_lock);

	return 0;
}

static int cvi_tpu_load_tee(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_load_tee_arg ioctl_arg;
	struct tpu_tee_load_info sec_info;

	mutex_lock(&ndev->dev_lock);

	ret = copy_from_user(&ioctl_arg,
			     (struct cvi_load_tee_arg __user *)arg,
			     sizeof(struct cvi_load_tee_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail, cvi_tpu_loadcmdbuf_sec\n");
		return ret;
	}

	//assign ioctl param
	sec_info.cmdbuf_addr_ree = ioctl_arg.cmdbuf_addr_ree;
	sec_info.cmdbuf_len_ree = ioctl_arg.cmdbuf_len_ree;
	sec_info.weight_addr_ree = ioctl_arg.weight_addr_ree;
	sec_info.weight_len_ree = ioctl_arg.weight_len_ree;
	sec_info.neuron_addr_ree = ioctl_arg.neuron_addr_ree;

	ret = platform_loadcmdbuf_tee(ndev, &sec_info);

	memset(&ioctl_arg, 0, sizeof(ioctl_arg));
	ioctl_arg.dmabuf_addr_tee = sec_info.dmabuf_addr_tee;

	ret = copy_to_user((struct cvi_load_tee_arg __user *)arg,
			     &ioctl_arg,
			     sizeof(struct cvi_load_tee_arg));
	if (ret) {
		dev_err(ndev->dev, "copy to user fail, cvi_tpu_loadcmdbuf_sec\n");
		return ret;
	}

	mutex_unlock(&ndev->dev_lock);
	return ret;
}

static int cvi_tpu_run_dmabuf(struct cvi_tpu_device *ndev, struct cvi_list_node *node)
{
	int ret = 0;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 ts_start, ts_end;
#else
	struct timespec ts_start, ts_end;
#endif

	mutex_lock(&ndev->dev_lock);
	platform_tpu_init(ndev);

	if (info.enable_usage_profiling) {
		//get start time
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&ts_start);
#else
		getnstimeofday(&ts_start);
#endif
	}

	//two different ways of normal and security submit
	switch (node->tpu_path) {
	case TPU_PATH_DESNORMAL:
		ret = platform_run_dmabuf(ndev, node->dmabuf_vaddr, node->dmabuf_paddr);
		break;
	case TPU_PATH_DESSEC:
		ret = platform_run_dmabuf_tee(ndev, &node->tee_info);
		break;
	case TPU_PATH_PIOTDMA:
		ret = platform_run_pio(ndev, &node->pio_info);
		break;
	default:
		break;
	}

	if (info.enable_usage_profiling) {
		//get duration
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&ts_end);
#else
		getnstimeofday(&ts_end);
#endif
		info.run_current_us = get_duration_us(&ts_start, &ts_end);
		info.run_sum_us += info.run_current_us;
		pr_debug("cur=%d, sum=%lld\n", info.run_current_us, info.run_sum_us);
	}

	if (ret == -ETIMEDOUT) {
		platform_tpu_reset(ndev);
	} else {
		platform_tpu_deinit(ndev);
	}
	mutex_unlock(&ndev->dev_lock);

	return ret;
}

static int cvi_tpu_wait_pio(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;
	struct cvi_tdma_wait_arg wait_pio_arg;
	struct cvi_list_node *node;

	ret = copy_from_user(&wait_pio_arg,
			     (struct cvi_tdma_wait_arg __user *)arg,
			     sizeof(struct cvi_tdma_wait_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail\n");
		return ret;
	}

	ret = wait_event_interruptible(
		kernel_work->done_wait_queue,
		get_from_done_list(kernel_work, wait_pio_arg.seq_no, TPU_PATH_PIOTDMA));

	if (ret) {
		dev_dbg(ndev->dev, "wait_event_interruptible() ret=%x\n", ret);
		return -EINTR;
	}

	node = get_from_done_list(kernel_work, wait_pio_arg.seq_no, TPU_PATH_PIOTDMA);
	if (!node) {
		dev_err(ndev->dev, "get node failed\n");
		return -EINVAL;
	}

	wait_pio_arg.ret = node->ret;
	ret = copy_to_user((unsigned long __user *)arg,
			   (const void *)&wait_pio_arg,
			   sizeof(struct cvi_tdma_wait_arg));
	if (ret)
		dev_err(ndev->dev, "copy to user fail\n");

	remove_from_done_list(kernel_work, node);
	return ret;
}

static int cvi_tpu_submit_pio(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_tdma_copy_arg ioctl_arg;
	u32 task_list_count = 0;
	struct cvi_list_node *node;
	struct cvi_list_node *pos;
	struct cvi_kernel_work *kernel_work;

	ret = copy_from_user(&ioctl_arg,
			     (struct cvi_tdma_copy_arg __user *)arg,
			     sizeof(struct cvi_tdma_copy_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail, cvi_tpu_submit_pio\n");
		return ret;
	}

	kernel_work = &ndev->kernel_work;
	while (1) {
		spin_lock(&ndev->kernel_work.task_list_lock);
		list_for_each_entry(pos, &kernel_work->task_list, list) {
			task_list_count++;
		}
		spin_unlock(&ndev->kernel_work.task_list_lock);
		if (task_list_count > TASK_LIST_MAX) {
			dev_info(ndev->dev, "too much task in task list\n");
			msleep(20);
		} else {
			break;
		}
	}

	pr_debug("cvi_tpu_submit_pio path()\n");
	node = vmalloc(sizeof(struct cvi_list_node));
	if (!node)
		return -ENOMEM;
	memset(node, 0, sizeof(struct cvi_list_node));

	node->pid = current->pid;
	node->pio_seq_no = ioctl_arg.seq_no;
	node->tpu_path = TPU_PATH_PIOTDMA;
	if (ioctl_arg.enable_2d) {
		node->pio_info.enable_2d = 1;
		node->pio_info.paddr_src = ioctl_arg.paddr_src;
		node->pio_info.paddr_dst = ioctl_arg.paddr_dst;
		node->pio_info.h = ioctl_arg.h;
		node->pio_info.w_bytes = ioctl_arg.w_bytes;
		node->pio_info.stride_bytes_src = ioctl_arg.stride_bytes_src;
		node->pio_info.stride_bytes_dst = ioctl_arg.stride_bytes_dst;
	} else {
		node->pio_info.paddr_src = ioctl_arg.paddr_src;
		node->pio_info.paddr_dst = ioctl_arg.paddr_dst;
		node->pio_info.leng_bytes = ioctl_arg.leng_bytes;
	}

	spin_lock(&ndev->kernel_work.task_list_lock);
	list_add_tail(&node->list, &ndev->kernel_work.task_list);
	wake_up_interruptible(&ndev->kernel_work.task_wait_queue);
	spin_unlock(&ndev->kernel_work.task_list_lock);
	return 0;
}

static int cvi_tpu_unload_tee(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_unload_tee_arg ioctl_arg;

	mutex_lock(&ndev->dev_lock);

	ret = copy_from_user(&ioctl_arg,
			     (struct cvi_unload_tee_arg __user *)arg,
			     sizeof(struct cvi_unload_tee_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail, cvi_tpu_unload_tee\n");
		return ret;
	}

	ret = platform_unload_tee(ndev, ioctl_arg.addr, ioctl_arg.size);
	mutex_unlock(&ndev->dev_lock);
	return ret;
}

static void cvi_tpu_cleanup_done_list(struct cvi_tpu_device *ndev,
				     struct cvi_kernel_work *kernel_work)
{
	u32 done_list_count = 0;
	struct cvi_list_node *pos, *tmp;
	struct task_struct *task;
	bool alive;

	spin_lock(&kernel_work->done_list_lock);

	list_for_each_entry(pos, &kernel_work->done_list, list) {
		done_list_count++;
	}

	if (done_list_count < DONE_LIST_MAX) {
		spin_unlock(&kernel_work->done_list_lock);
		return;
	}

	dev_info(ndev->dev, "done list too much node, clean up\n");

	list_for_each_entry_safe(pos, tmp, &kernel_work->done_list, list) {
		alive = false;
		for_each_process(task) {
			if (pos->pid == task->pid) {
				alive = true;
				break;
			}
		}
		if (!alive) {
			list_del(&pos->list);
			vfree(pos);
		}
	}

	spin_unlock(&kernel_work->done_list_lock);
}

static void work_thread_run(struct cvi_tpu_device *ndev)
{
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;
	struct cvi_list_node *first_node;
	int ret = 0;

	spin_lock(&kernel_work->task_list_lock);
	first_node = list_first_entry(&kernel_work->task_list,
				      struct cvi_list_node, list);
	list_del(&first_node->list);
	spin_unlock(&kernel_work->task_list_lock);

	//before tpu inference
	tpu_suspend.running_cnt = 1;

	//tpu inference HW running process
	ret = cvi_tpu_run_dmabuf(ndev, first_node);
	first_node->ret = ret;
	cvi_tpu_cleanup_buffer(first_node);

	//after tpu inference
	tpu_suspend.running_cnt = 0;

	spin_lock(&kernel_work->done_list_lock);
	list_add_tail(&first_node->list, &kernel_work->done_list);
	spin_unlock(&kernel_work->done_list_lock);

	wake_up_all(&kernel_work->done_wait_queue);

	cvi_tpu_cleanup_done_list(ndev, kernel_work);
}

static int task_list_empty(struct cvi_kernel_work *kernel_work)
{
	int ret;

	spin_lock(&kernel_work->task_list_lock);
	ret = list_empty(&kernel_work->task_list);
	spin_unlock(&kernel_work->task_list_lock);

	return ret;
}

static void work_thread_exit(struct cvi_tpu_device *ndev)
{
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;
	struct cvi_list_node *pos, *tmp;

	spin_lock(&kernel_work->task_list_lock);
	list_for_each_entry_safe(pos, tmp, &kernel_work->task_list, list) {
		list_del(&pos->list);
		vfree(pos);
	}
	spin_unlock(&kernel_work->task_list_lock);

	spin_lock(&kernel_work->done_list_lock);
	list_for_each_entry_safe(pos, tmp, &kernel_work->done_list, list) {
		list_del(&pos->list);
		vfree(pos);
	}
	spin_unlock(&kernel_work->done_list_lock);
}

static int work_thread_main(void *data)
{
	struct cvi_tpu_device *ndev = (struct cvi_tpu_device *)data;
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;

	dev_dbg(ndev->dev, "enter work thread\n");

	while (!kthread_should_stop()) {
		wait_event_interruptible(kernel_work->task_wait_queue,
					 !task_list_empty(kernel_work) ||
						 kthread_should_stop());

		if (!task_list_empty(kernel_work))
			work_thread_run(ndev);
	}

	work_thread_exit(ndev);

	dev_dbg(ndev->dev, "exit work thread\n");

	return 0;
}

static int work_thread_init(struct cvi_tpu_device *ndev)
{
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;

	init_waitqueue_head(&kernel_work->task_wait_queue);
	init_waitqueue_head(&kernel_work->done_wait_queue);
	INIT_LIST_HEAD(&kernel_work->task_list);
	spin_lock_init(&kernel_work->task_list_lock);
	INIT_LIST_HEAD(&kernel_work->done_list);
	spin_lock_init(&kernel_work->done_list_lock);

	kernel_work->work_thread =
		kthread_run(work_thread_main, ndev, "cvitask_tpu_wor");
	if (IS_ERR(kernel_work->work_thread)) {
		dev_err(ndev->dev, "kthread run fail\n");
		return PTR_ERR(kernel_work->work_thread);
	}

	return 0;
}

static int cvi_tpu_wait_dmabuf(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;
	struct cvi_wait_dma_arg wait_dmabuf_arg;
	struct cvi_list_node *node;

	ret = copy_from_user(&wait_dmabuf_arg,
			     (struct cvi_wait_dma_arg __user *)arg,
			     sizeof(struct cvi_wait_dma_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail\n");
		return ret;
	}

	ret = wait_event_interruptible(
		kernel_work->done_wait_queue,
		get_from_done_list(kernel_work, wait_dmabuf_arg.seq_no, TPU_PATH_DESNORMAL));
	if (ret) {
		dev_dbg(ndev->dev, "wait_event_interruptible() ret=%x\n", ret);
		return -EINTR;
	}

	node = get_from_done_list(kernel_work, wait_dmabuf_arg.seq_no, TPU_PATH_DESNORMAL);
	if (!node) {
		dev_err(ndev->dev, "get node failed\n");
		return -EINVAL;
	}

	wait_dmabuf_arg.ret = node->ret;
	ret = copy_to_user((unsigned long __user *)arg,
			   (const void *)&wait_dmabuf_arg,
			   sizeof(struct cvi_wait_dma_arg));
	if (ret)
		dev_err(ndev->dev, "copy to user fail\n");

	remove_from_done_list(kernel_work, node);

	return ret;
}

static int cvi_tpu_map_dmabuf(struct cvi_tpu_device *ndev, unsigned long arg,
			     enum dma_data_direction direction)
{
	int ret = 0;
	int dmabuf_fd;
	struct dma_buf *dma_buf = NULL;
	struct dma_buf_attachment *attachment = NULL;
	struct sg_table *sgt = NULL;

	ret = get_user(dmabuf_fd, (int __user *)arg);
	if (ret)
		return ret;

	dma_buf = dma_buf_get(dmabuf_fd);
	if (IS_ERR(dma_buf))
		return -EINVAL;

	attachment = dma_buf_attach(dma_buf, ndev->dev);
	if (IS_ERR(attachment)) {
		ret = -EINVAL;
		goto err_attach;
	}

	sgt = dma_buf_map_attachment(attachment, direction);
	if (IS_ERR(sgt)) {
		ret = -EINVAL;
		goto err_map;
	}

	dma_buf_unmap_attachment(attachment, sgt, direction);

err_map:
	dma_buf_detach(dma_buf, attachment);

err_attach:
	dma_buf_put(dma_buf);

	return ret;
}

static int cvi_tpu_cache_flush(struct cvi_tpu_device *ndev, unsigned long arg)
{
	int ret = 0;
	struct cvi_cache_op_arg flush_arg;

	ret = copy_from_user(&flush_arg, (struct cvi_cache_op_arg __user *)arg, sizeof(struct cvi_cache_op_arg));
	if (ret) {
		dev_err(ndev->dev, "copy c user fail\n");
		return ret;
	}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(flush_arg.paddr, flush_arg.size, DMA_TO_DEVICE);
#else
	__dma_map_area(phys_to_virt(flush_arg.paddr), flush_arg.size, DMA_TO_DEVICE);
#endif

	smp_mb();	/*memory barrier*/
	return 0;
}

static int cvi_tpu_cache_invalidate(struct cvi_tpu_device *ndev,
				   unsigned long arg)
{
	int ret = 0;
	struct cvi_cache_op_arg invalidate_arg;

	ret = copy_from_user(&invalidate_arg, (struct cvi_cache_op_arg __user *)arg, sizeof(struct cvi_cache_op_arg));
	if (ret) {
		dev_err(ndev->dev, "copy from user fail\n");
		return ret;
	}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
	arch_sync_dma_for_device(invalidate_arg.paddr, invalidate_arg.size, DMA_FROM_DEVICE);
#else
	__dma_map_area(phys_to_virt(invalidate_arg.paddr), invalidate_arg.size, DMA_FROM_DEVICE);
#endif

	smp_mb();	/*memory barrier*/
	return 0;
}

static long cvi_tpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_tpu_device *ndev = filp->private_data;
	long ret = 0;

	switch (cmd) {
	case CVITPU_SUBMIT_DMABUF:
		ret = cvi_tpu_submit(ndev, arg);
		break;
	case CVITPU_DMABUF_FLUSH_FD:
		ret = cvi_tpu_map_dmabuf(ndev, arg, DMA_TO_DEVICE);
		break;
	case CVITPU_DMABUF_INVLD_FD:
		ret = cvi_tpu_map_dmabuf(ndev, arg, DMA_FROM_DEVICE);
		break;
	case CVITPU_DMABUF_FLUSH:
		ret = cvi_tpu_cache_flush(ndev, arg);
		break;
	case CVITPU_DMABUF_INVLD:
		ret = cvi_tpu_cache_invalidate(ndev, arg);
		break;
	case CVITPU_WAIT_DMABUF:
		ret = cvi_tpu_wait_dmabuf(ndev, arg);
		break;

	case CVITPU_LOAD_TEE:
		ret = cvi_tpu_load_tee(ndev, arg);
		break;
	case CVITPU_SUBMIT_TEE:
		ret = cvi_tpu_submit_tee(ndev, arg);
		break;
	case CVITPU_UNLOAD_TEE:
		ret = cvi_tpu_unload_tee(ndev, arg);
		break;

	case CVITPU_SUBMIT_PIO:
		ret = cvi_tpu_submit_pio(ndev, arg);
		break;
	case CVITPU_WAIT_PIO:
		ret = cvi_tpu_wait_pio(ndev, arg);
		break;

	default:
		return -ENOTTY;
	}

	return ret;
}

static int cvi_tpu_open(struct inode *inode, struct file *filp)
{
	struct cvi_tpu_device *ndev =
		container_of(inode->i_cdev, struct cvi_tpu_device, cdev);
	unsigned long flags = 0;
	int ret;

	spin_lock_irqsave(&ndev->close_lock, flags);

	if (ndev->use_count == 0) {

		ret = platform_tpu_open(ndev);
		if (ret < 0) {
			pr_err("npu open failed\n");
			return ret;
		}
	}
	ndev->use_count++;

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = ndev;

	return 0;
}

static int cvi_tpu_close(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	struct cvi_tpu_device *ndev =
		container_of(inode->i_cdev, struct cvi_tpu_device, cdev);

	spin_lock_irqsave(&ndev->close_lock, flags);

	ndev->use_count--;

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = NULL;

	return 0;
}

static const struct file_operations npu_fops = {
	.owner = THIS_MODULE,
	.open = cvi_tpu_open,
	.release = cvi_tpu_close,
	.unlocked_ioctl = cvi_tpu_ioctl,
	.compat_ioctl = cvi_tpu_ioctl,
};

int cvi_tpu_register_cdev(struct cvi_tpu_device *ndev)
{
	int ret;

	npu_class = class_create(THIS_MODULE, CVI_TPU_CLASS_NAME);
	if (IS_ERR(npu_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(npu_class);
	}

	ret = alloc_chrdev_region(&npu_cdev_id, 0, 1, CVI_TPU_CDEV_NAME);
	if (ret < 0) {
		pr_err("alloc chrdev failed\n");
		return ret;
	}

	cdev_init(&ndev->cdev, &npu_fops);
	ndev->cdev.owner = THIS_MODULE;
	cdev_add(&ndev->cdev, npu_cdev_id, 1);

	device_create(npu_class, ndev->dev, npu_cdev_id, NULL, "%s%d",
		      CVI_TPU_CDEV_NAME, 0);

	return 0;
}

static u64 cvi_npu_dma_mask = DMA_BIT_MASK(40);

static int cvi_tpu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_tpu_device *ndev;
	struct resource *res;
	int ret;

	pr_debug("===cvi_tpu_probe start\n");

	pdev->dev.dma_mask = &cvi_npu_dma_mask;
	pdev->dev.coherent_dma_mask = cvi_npu_dma_mask;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tdma");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve tdma io\n");
		return -ENXIO;
	}
	ndev->tdma_vaddr = devm_ioremap_resource(&pdev->dev, res);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tiu");
	if (res == NULL) {
		dev_err(dev, "failed to retrieve tiu io\n");
		return -ENXIO;
	}
	ndev->tiu_vaddr = devm_ioremap_resource(&pdev->dev, res);

	ndev->tdma_irq = platform_get_irq(pdev, 1);
	if (ndev->tdma_irq < 0) {
		dev_err(dev, "failed to retrieve tdma irq");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, ndev->tdma_irq, cvi_tpu_tdma_irq, 0,
			       "cvi-tpu-tdma", ndev);
	if (ret)
		return -ENXIO;

	//get clock related
	ndev->clk_tpu_axi = devm_clk_get(&pdev->dev, "clk_tpu_axi");
	if (IS_ERR(ndev->clk_tpu_axi)) {
		pr_err("Cannot get clk_tpu_axi\n");
		ndev->clk_tpu_axi = NULL;
	}

	ndev->clk_tpu_fab = devm_clk_get(&pdev->dev, "clk_tpu_fab");
	if (IS_ERR(ndev->clk_tpu_fab)) {
		pr_err("Cannot get clk_tpu_fab\n");
		ndev->clk_tpu_fab = NULL;
	}

	//get reset related
	ndev->rst_tdma = devm_reset_control_get(&pdev->dev, "res_tdma");
	if (IS_ERR(ndev->rst_tdma)) {
		pr_err("Cannot get res_tdma\n");
		ndev->rst_tdma = NULL;
	}
	ndev->rst_tpu = devm_reset_control_get(&pdev->dev, "res_tpu");
	if (IS_ERR(ndev->rst_tpu)) {
		pr_err("Cannot get res_tpu\n");
		ndev->rst_tpu = NULL;
	}
	ndev->rst_tpusys = devm_reset_control_get(&pdev->dev, "res_tpusys");
	if (IS_ERR(ndev->rst_tpusys)) {
		pr_err("Cannot get res_tpusys\n");
		ndev->rst_tpusys = NULL;
	}
	//init tpu suspend info
	spin_lock_init(&tpu_suspend.spin_lock);
	tpu_suspend.running_cnt = 0;

	//probe tpu setting
	platform_tpu_probe_setting();

	init_completion(&ndev->tdma_done);
	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	ret = cvi_tpu_register_cdev(ndev);
	if (ret < 0) {
		pr_err("register chrdev error\n");
		return ret;
	}

	ret = work_thread_init(ndev);
	if (ret < 0) {
		pr_err("work thread init error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);

	//create tpu proc
	tpu_proc_dir = proc_mkdir("tpu", NULL);
	if (proc_create_data("usage_profiling", 0644, tpu_proc_dir, &tpu_proc_ops, ndev) == NULL)
		pr_err("tpu usage_profiling proc creation failed\n");

	pr_debug("===cvi_tpu_probe end\n");
	return 0;
}

static int cvi_tpu_remove(struct platform_device *pdev)
{
	struct cvi_tpu_device *ndev = platform_get_drvdata(pdev);
	struct cvi_kernel_work *kernel_work = &ndev->kernel_work;

	kthread_stop(kernel_work->work_thread);

	//put clock related
	if (ndev->clk_tpu_axi)
		devm_clk_put(&pdev->dev, ndev->clk_tpu_axi);

	if (ndev->clk_tpu_fab)
		devm_clk_put(&pdev->dev, ndev->clk_tpu_fab);

	device_destroy(npu_class, npu_cdev_id);

	cdev_del(&ndev->cdev);
	unregister_chrdev_region(npu_cdev_id, 1);

	class_destroy(npu_class);

	platform_set_drvdata(pdev, NULL);
	pr_debug("===cvi_tpu_remove\n");

	//remove tpu proc
	proc_remove(tpu_proc_dir);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cvi_tpu_suspend(struct device *dev)
{
	if (tpu_suspend.running_cnt) {
		//doing register backup
		struct platform_device *pdev = to_platform_device(dev);
		struct cvi_tpu_device *ndev = platform_get_drvdata(pdev);

		platform_tpu_suspend(ndev);
	}

	return 0;
}

static int cvi_tpu_resume(struct device *dev)
{
	if (tpu_suspend.running_cnt) {
		struct platform_device *pdev = to_platform_device(dev);
		struct cvi_tpu_device *ndev = platform_get_drvdata(pdev);

		platform_tpu_resume(ndev);
	}
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(cvi_tpu_pm_ops, cvi_tpu_suspend, cvi_tpu_resume);

static const struct of_device_id cvi_tpu_match[] = {
	{ .compatible = "cvitek,tpu" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_tpu_match);

static struct platform_driver cvi_tpu_driver = {
	.probe = cvi_tpu_probe,
	.remove = cvi_tpu_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = "cvi-tpu",
			.pm = &cvi_tpu_pm_ops,
			.of_match_table = cvi_tpu_match,
		},
};
module_platform_driver(cvi_tpu_driver);

MODULE_AUTHOR("Wellken Chen<wellken.chen@cvitek.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC TPU driver");
MODULE_LICENSE("GPL");
