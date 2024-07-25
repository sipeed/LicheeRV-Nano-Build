/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_jpeg.c
 * Description: jpeg system interface
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/reset.h>
#include <linux/of_reserved_mem.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "cvi_jpeg.h"
#include "jpeg_common.h"

#define VERSION "CVITEK"
//#define ENABLE_DEBUG_MSG
#ifdef ENABLE_DEBUG_MSG
#define DPRINTK(args...) pr_info(args)
#else
#define DPRINTK(args...)
#endif

#define JPU_PLATFORM_DEVICE_NAME	"jpeg"
#define JPU_CLK_NAME			"jpeg_clk"
#define JPU_CLASS_NAME			"jpu"
#define JPU_DEV_NAME			"jpu"

#define JPU_REG_SIZE 0x300
#define JPU_CONTROL_REG_ADDR 0x50008008
#define JPU_CONTROL_REG_SIZE 0x4

#ifndef VM_RESERVED // for kernel up to 3.7.0 version
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define MJPEG_PIC_STATUS_REG 0x4
#define MJPEG_INTR_MASK_REG  0x0C0

static DEFINE_SEMAPHORE(s_jpu_sem);

int jpu_mask = JPU_MASK_ERR;
module_param(jpu_mask, int, 0644);

static int jpu_chn_idx = -1;
extern wait_queue_head_t tWaitQueue[];

#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
// global variable to avoid kernal config mismatch in filp->private_data
static void *pCviJpuDevice;
#endif

static int cvi_jpu_register_cdev(struct cvi_jpu_device *jdev);
static void cvi_jpu_unregister_cdev(struct platform_device *pdev);
#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
static int jpu_allocate_memory(struct cvi_jpu_device *jdev, struct platform_device *pdev);
#endif

static void set_clock_enable(struct cvi_jpu_device *jdev, int enable)
{
	if (jdev->pdata->quirks & (JPU_QUIRK_SUPPORT_CLOCK_CONTROL | JPU_QUIRK_SUPPORT_FPGA)) {
		if (enable) {
			if (jdev->pdata->ops->clk_enable)
				jdev->pdata->ops->clk_enable(jdev);
		} else {
			if (jdev->pdata->ops->clk_disable)
			jdev->pdata->ops->clk_disable(jdev);
		}
	}
}

irqreturn_t jpu_irq_handler(int irq, void *data)
{
	struct cvi_jpu_device *jdev = data;
	jpudrv_buffer_t *pReg = &jdev->jpu_register;

	if (jpu_chn_idx == -1)
		return IRQ_HANDLED;

	writel(0xFF, pReg->virt_addr + MJPEG_INTR_MASK_REG);//disable_irq_nosync(jdev->jpu_irq);

	if (jdev->async_queue)
		kill_fasync(&jdev->async_queue, SIGIO,
			    POLL_IN); // notify the interrupt to userspace

	jdev->interrupt_flag = 1;
	wake_up(&tWaitQueue[jpu_chn_idx]);
	wake_up(&jdev->interrupt_wait_q);

	return IRQ_HANDLED;
}

// JDI_IOCTL_WAIT_INTERRUPT
int jpu_wait_interrupt(int timeout)
{
	u32 u32TimeOut;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	u32TimeOut = (u32)timeout;
	if (!wait_event_timeout(
			jdev->interrupt_wait_q, jdev->interrupt_flag != 0,
			msecs_to_jiffies(u32TimeOut))) {
		return -ETIME;
	}

	if (signal_pending(current)) {
		return -ERESTARTSYS;
	}

	jdev->interrupt_flag = 0;

	return 0;
}
EXPORT_SYMBOL(jpu_wait_interrupt);

// JDI_IOCTL_SET_CLOCK_GATE
int jpu_set_clock_gate(int *pEnable)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	u32 clkgate;

	clkgate = *(u32 *)pEnable;

	set_clock_enable(jdev, clkgate);
	return 0;
}
EXPORT_SYMBOL(jpu_set_clock_gate);

// JDI_IOCTL_GET_INSTANCE_POOL
int jpu_get_instance_pool(jpudrv_buffer_t *p_jdb)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	down(&s_jpu_sem);

	if (jdev->jpu_instance_pool.base == 0) {

		memcpy(&jdev->jpu_instance_pool, (struct jpudrv_buffer_t *)p_jdb,
				     sizeof(struct jpudrv_buffer_t));

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		jdev->jpu_instance_pool.size =
			PAGE_ALIGN(jdev->jpu_instance_pool.size);
		jdev->jpu_instance_pool.phys_addr =
			jdev->jpu_instance_pool.base =
				(unsigned long)vmalloc(
					jdev->jpu_instance_pool.size);
		if (jdev->jpu_instance_pool.base == 0) {
			up(&s_jpu_sem);
			return -EFAULT;
		}
		memset((void *)(uintptr_t)jdev->jpu_instance_pool.base, 0x0,
		       jdev->jpu_instance_pool.size);
#else
		if (jpu_alloc_dma_buffer(jdev, &jdev->jpu_instance_pool) == -1) {
			up(&s_jpu_sem);
			return -EFAULT;
		}
		memset_io((void *)jdev->jpu_instance_pool.base, 0x0,
			  jdev->jpu_instance_pool.size);
#endif
		JPU_DBG_INFO("jdev->jpu_instance_pool base: 0x%llx, size: %d\n",
			jdev->jpu_instance_pool.base,
			jdev->jpu_instance_pool.size);
	}

	memcpy(p_jdb, &jdev->jpu_instance_pool, sizeof(struct jpudrv_buffer_t));

	up(&s_jpu_sem);

	return 0;
}
EXPORT_SYMBOL(jpu_get_instance_pool);

// JDI_IOCTL_OPEN_INSTANCE
int jpu_open_instance(unsigned long *pInstIdx)
{
	u32 inst_idx;
	struct jpudrv_instanace_list_t *jil;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	unsigned long flags = 0;

	jil = kzalloc(sizeof(*jil), GFP_KERNEL);
	if (!jil)
		return -ENOMEM;

	inst_idx = *(u32 *)pInstIdx;

	jil->inst_idx = inst_idx;
	//jil->filp = filp;
	spin_lock_irqsave(&jdev->jpu_lock, flags);
	list_add(&jil->list, &jdev->s_jpu_inst_list_head);
	jdev->open_instance_count++;
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	return 0;
}
EXPORT_SYMBOL(jpu_open_instance);

// JDI_IOCTL_CLOSE_INSTANCE
int jpu_close_instance(unsigned long *pInstIdx)
{
	u32 inst_idx;
	struct jpudrv_instanace_list_t *jil, *n;
	bool find_jil = false;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif
	unsigned long flags = 0;

	inst_idx = *(u32 *)pInstIdx;

	spin_lock_irqsave(&jdev->jpu_lock, flags);
	list_for_each_entry_safe(jil, n, &jdev->s_jpu_inst_list_head, list) {
		if (jil->inst_idx == inst_idx) {
			jdev->open_instance_count--;
			list_del(&jil->list);
			find_jil = true;
			break;
		}
	}
	spin_unlock_irqrestore(&jdev->jpu_lock, flags);

	if (find_jil) {
		kfree(jil);
		JPU_DBG_INFO("IOCTL_CLOSE_INSTANCE, inst_idx=%d, open_count=%d\n",
			(int)inst_idx, jdev->open_instance_count);
	}

	return 0;
}
EXPORT_SYMBOL(jpu_close_instance);

// JDI_IOCTL_GET_INSTANCE_NUM
int jpu_get_instance_num(int *pInstNum)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	down(&s_jpu_sem);

	*pInstNum = jdev->open_instance_count;

	JPU_DBG_INFO("IOCTL_GET_INSTANCE_NUM open_count=%d\n",
		jdev->open_instance_count);

	up(&s_jpu_sem);
	return 0;
}
EXPORT_SYMBOL(jpu_get_instance_num);

// JDI_IOCTL_GET_REGISTER_INFO
int jpu_get_register_info(jpudrv_buffer_t *p_jdb_register)
{
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	JPU_DBG_INFO("[+]JDI_IOCTL_GET_REGISTER_INFO\n");

	memcpy(p_jdb_register, &jdev->jpu_register, sizeof(struct jpudrv_buffer_t));

	JPU_DBG_INFO("[-]GET_REGISTER_INFO, pa = 0x%llx, va = %p, size = %d\n",
		jdev->jpu_register.phys_addr, jdev->jpu_register.virt_addr,
		jdev->jpu_register.size);

	return 0;
}
EXPORT_SYMBOL(jpu_get_register_info);

// JDI_IOCTL_RESET
int jpu_reset(int InstIdx)
{
	return 0;
}
EXPORT_SYMBOL(jpu_reset);

unsigned long jpu_get_interrupt_flag(int chnIdx)
{
	unsigned long interrupt_flag = 0;
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_jpu_device *jdev = (struct cvi_jpu_device *)pCviJpuDevice;
	#else
	struct cvi_jpu_device *jdev = filp->private_data;
	#endif

	interrupt_flag = (jpu_chn_idx == chnIdx) ? jdev->interrupt_flag : 0;

	return interrupt_flag;
}
EXPORT_SYMBOL(jpu_get_interrupt_flag);

void jpu_set_channel_num(int chnIdx)
{
	jpu_chn_idx = chnIdx;
}
EXPORT_SYMBOL(jpu_set_channel_num);

static int jpu_probe(struct platform_device *pdev)
{
	int err = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cvi_jpu_device *jdev;

	struct resource *res = NULL;

	jdev = devm_kzalloc(&pdev->dev, sizeof(*jdev), GFP_KERNEL);
	if (!jdev)
		return -ENOMEM;

	memset(jdev, 0, sizeof(*jdev));
	#ifdef JPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	pCviJpuDevice = jdev;
	#endif

	jdev->dev = dev;

	match = of_match_device(cvi_jpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	jdev->pdata = match->data;

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res) {
		unsigned long size = resource_size(res);

		jdev->jpu_register.phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_register.virt_addr =
			(__u8 *)ioremap(res->start, size);
#else
		jdev->jpu_register.virt_addr =
			(__u8 *)ioremap_nocache(res->start, size);
#endif
		jdev->jpu_register.size = size;
		JPU_DBG_INFO("jpu reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_register.phys_addr, jdev->jpu_register.virt_addr,
			jdev->jpu_register.size);
	} else {
		pr_err("Unable to get base register\n");
		return -EINVAL;
	}

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	// if platform driver is implemented
	if (res) {
		unsigned long size = resource_size(res);

		jdev->jpu_control_register.phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap(res->start, size);
#else
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap_nocache(res->start, size);
#endif
		jdev->jpu_control_register.size = size;
		pr_info("jpu ctrl reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_control_register.phys_addr,
			jdev->jpu_control_register.virt_addr,
			jdev->jpu_control_register.size);
	} else {
		//TODO get res from kernel
		jdev->jpu_control_register.phys_addr = JPU_CONTROL_REG_ADDR;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap(
				jdev->jpu_control_register.phys_addr,
				JPU_CONTROL_REG_SIZE);
#else
		jdev->jpu_control_register.virt_addr =
			(__u8 *)ioremap_nocache(
				jdev->jpu_control_register.phys_addr,
				JPU_CONTROL_REG_SIZE);
#endif
		jdev->jpu_control_register.size = JPU_CONTROL_REG_SIZE;
		JPU_DBG_INFO("jpu ctrl reg pa = 0x%llx, va = %p, size = %u\n",
			jdev->jpu_control_register.phys_addr,
			jdev->jpu_control_register.virt_addr,
			jdev->jpu_control_register.size);
	}

	err = cvi_jpu_register_cdev(jdev);

	if (err < 0) {
		pr_err("jpu_register_cdev\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (jdev->pdata->quirks & JPU_QUIRK_SUPPORT_CLOCK_CONTROL) {
		if (jdev->pdata->ops->clk_get)
			jdev->pdata->ops->clk_get(jdev);  // jpu_clk_get
	}

	if (pdev)
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	// if platform driver is implemented
	if (res) {
		jdev->jpu_irq = res->start;
		JPU_DBG_INFO("jpu irq number irq = %d\n", jdev->jpu_irq);
	} else {
		JPU_DBG_ERR("jpu irq number irq = %d\n", jdev->jpu_irq);
	}
#if 0
	err = request_irq(jdev->jpu_irq, jpu_irq_handler, 0, "JPU_CODEC_IRQ", jdev);
	if (err) {
		pr_err("[JPUDRV] :  fail to register interrupt handler\n");
		goto ERROR_PROBE_DEVICE;
	}
#endif
#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (jpu_allocate_memory(jdev, pdev) < 0) {
		pr_err("[JPUDRV] :  fail to remap jpu memory\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (jmem_init(&jdev->jmem, jdev->video_memory.phys_addr, jdev->video_memory.size) <
	    0) {
		pr_err("[JPUDRV] :  fail to init vmem system\n");
		goto ERROR_PROBE_DEVICE;
	}
#endif

	jdev->jpu_instance_pool.base = 0;

	if (jdev->pdata->quirks & JPU_QUIRK_SUPPORT_SWITCH_TO_PLL && jdev->pdata->ops->config_pll) {
		jdev->pdata->ops->config_pll(jdev); //cv1835_config_pll
	}

	init_waitqueue_head(&jdev->interrupt_wait_q);

	spin_lock_init(&jdev->jpu_lock);

	INIT_LIST_HEAD(&jdev->s_jbp_head);
	INIT_LIST_HEAD(&jdev->s_jpu_inst_list_head);

	platform_set_drvdata(pdev, jdev);

	return 0;

ERROR_PROBE_DEVICE:

	if (jdev->s_jpu_major)
		unregister_chrdev_region(jdev->cdev_id, 1);

	if (jdev->jpu_register.virt_addr) {
		iounmap((void *)jdev->jpu_register.virt_addr);
		jdev->jpu_register.virt_addr = 0x00;
	}

	if (jdev->jpu_control_register.virt_addr) {
		iounmap((void *)jdev->jpu_control_register.virt_addr);
		jdev->jpu_control_register.virt_addr = 0x00;
	}

	return err;
}

static int cvi_jpu_register_cdev(struct cvi_jpu_device *jdev)
{
	int err = 0;

	jdev->jpu_class = class_create(THIS_MODULE, JPU_CLASS_NAME);
	if (IS_ERR(jdev->jpu_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(jdev->jpu_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&jdev->cdev_id, 0, 1, JPU_DEV_NAME)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate major number\n");
		return err;
	}
	jdev->s_jpu_major = MAJOR(jdev->cdev_id);

	return err;
}

#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
static int jpu_allocate_memory(struct cvi_jpu_device *jdev, struct platform_device *pdev)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;

	if (pdev) {
		target =
			of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	}

	if (target) {
		prmem = of_reserved_mem_lookup(target);
		of_node_put(target);

		if (!prmem) {
			pr_err("[JPUDRV]: cannot acquire memory-region\n");
			return -1;
		}
	} else {
		pr_err("[JPUDRV]: cannot find the node, memory-region\n");
		return -1;
	}

	JPU_DBG_INFO("pool name = %s, size = 0x%llx, base = 0x%llx\n",
	       prmem->name, prmem->size, prmem->base);

	jdev->video_memory.phys_addr = (unsigned long)prmem->base;
	jdev->video_memory.size = (unsigned int)prmem->size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	jdev->video_memory.base = (unsigned long)devm_ioremap(
		&pdev->dev, jdev->video_memory.phys_addr, jdev->video_memory.size);
#else
	jdev->video_memory.base = (unsigned long)devm_ioremap_nocache(
		&pdev->dev, jdev->video_memory.phys_addr, jdev->video_memory.size);
#endif

	if (!jdev->video_memory.base) {
		pr_err("[JPUDRV] ioremap fail!\n");
		pr_err("jdev->video_memory.base = 0x%llx\n", jdev->video_memory.base);
		return -1;
	}

	JPU_DBG_INFO("reserved mem pa = 0x%llx, base = 0x%llx, size = 0x%x\n",
		jdev->video_memory.phys_addr, jdev->video_memory.base,
		jdev->video_memory.size);
	JPU_DBG_INFO("success to probe jpu\n");

	return 0;
}
#endif // #ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY

static int jpu_remove(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	pr_info("jpu_remove\n");

	if (jdev->jpu_instance_pool.base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		vfree((const void *)(uintptr_t)jdev->jpu_instance_pool.base);
#else
		jpu_free_dma_buffer(jdev, &jdev->jpu_instance_pool);
#endif
		jdev->jpu_instance_pool.base = 0;
	}
#ifndef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	if (jdev->video_memory.base) {
		jdev->video_memory.base = 0;
		jmem_exit(&jdev->jmem);
	}
#endif

	cvi_jpu_unregister_cdev(pdev);

	if (jdev->jpu_irq)
		free_irq(jdev->jpu_irq, jdev);

	if (jdev->jpu_register.virt_addr) {
		iounmap((void *)jdev->jpu_register.virt_addr);
		jdev->jpu_register.virt_addr = 0x00;
	}

	if (jdev->jpu_control_register.virt_addr) {
		iounmap((void *)jdev->jpu_control_register.virt_addr);
		jdev->jpu_control_register.virt_addr = 0x00;
	}

	return 0;
}

static void cvi_jpu_unregister_cdev(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	if (jdev->s_jpu_major > 0) {
		device_destroy(jdev->jpu_class, jdev->cdev_id);
		class_destroy(jdev->jpu_class);
		cdev_del(&jdev->cdev);
		unregister_chrdev_region(jdev->cdev_id, 1);
		jdev->s_jpu_major = 0;
	}
}

#ifdef CONFIG_PM
static int jpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	set_clock_enable(jdev, JPEG_CLK_DISABLE);

	return 0;
}

static int jpu_resume(struct platform_device *pdev)
{
	struct cvi_jpu_device *jdev = platform_get_drvdata(pdev);

	set_clock_enable(jdev, JPEG_CLK_ENABLE);

	return 0;
}
#else
#define jpu_suspend NULL
#define jpu_resume NULL
#endif /* !CONFIG_PM */

static struct platform_driver jpu_driver = {
	.driver   = {
		.owner = THIS_MODULE,
		.name = JPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_jpu_match_table,
	},
	.probe    = jpu_probe,
	.remove   = jpu_remove,
#ifdef CONFIG_PM
	.suspend  = jpu_suspend,
	.resume   = jpu_resume,
#endif /* !CONFIG_PM */
};

static int __init jpu_init(void)
{
	int res = 0;

	res = platform_driver_register(&jpu_driver);

	pr_info("end jpu_init result = 0x%x\n", res);
	return res;
}

static void __exit jpu_exit(void)
{
	pr_info("jpu_exit\n");
	platform_driver_unregister(&jpu_driver);
}

MODULE_AUTHOR("Cvitek");
MODULE_DESCRIPTION("JPEG linux driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);

module_init(jpu_init);
module_exit(jpu_exit);
