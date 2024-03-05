// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: fast_image.c
 * Description:
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"
#include "sys.h"
#include "rtos_cmdqu.h"
#include "fast_image.h"
#include "dump_uart.h"
#include "jenc.h"

//#define pr_debug printk

struct transfer_config_t *transfer_config;
static unsigned int mcu_transfer_config_offset;
struct dump_uart_s *dump_uart_va;
struct _JPEG_PIC *dump_jpg_va;
struct trace_snapshot_t *dump_snapshot_va;

struct cvi_fast_image_device {
	struct device *dev;
	struct miscdevice miscdev;
};

struct mcu_shm_t {
	u64 size;
	u64 phys_addr;
	union{
	u64 virt_base; /* kernel logical address in use kernel */
	u64 addr;
	};
} mcu_shm_t;

static struct mcu_shm_t s_mcu_shm = { 0 };
static struct mcu_shm_t isp_ion = { 0 };
static struct mcu_shm_t img_ion = { 0 };
static struct mcu_shm_t enc_ion = { 0 };

static int cvi_allocate_mcu_shm(struct platform_device *pdev);
static void cvi_fast_image_ion_alloc(void);
//static void __iomem *top_base;
static int vcodec_h264_irq;
static int vcodec_h265_irq;
static int jpu_irq;
static int isp_irq;

static int cvi_fast_image_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cvi_fast_image_release(struct inode *inode, struct file *file)
{
	return 0;
}

int cvi_stop_fast_image(void)
{
	// send command to rtos to stop irq
	cmdqu_t cmdq;
	int ret;

	cmdq.ip_id = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_STOP_ISR;
	cmdq.param_ptr = 0;
	cmdq.resv.mstime = 100;
	// wait done
	ret = rtos_cmdqu_send_wait(&cmdq, SYS_CMD_INFO_STOP_ISR_DONE);
	if (ret)
		pr_err("SYS_CMD_INFO_STOP_ISR wait fail\n");
	pr_debug("%s enable_irq vcode h264 %d\n", __func__, vcodec_h264_irq);
	enable_irq(vcodec_h264_irq);
	pr_debug("%s enable_irq vcode h265 %d\n", __func__, vcodec_h265_irq);
	enable_irq(vcodec_h265_irq);
	pr_debug("%s enable_irq jpu %d\n", __func__, jpu_irq);
	enable_irq(jpu_irq);
	pr_debug("%s enable_irq isp %d\n", __func__, isp_irq);
	enable_irq(isp_irq);
	pr_debug("%s done\n", __func__);
	return 0;
}
EXPORT_SYMBOL_GPL(cvi_stop_fast_image);

phys_addr_t cvi_fast_image_isp_buf_pa(void)
{
	return isp_ion.phys_addr;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_isp_buf_pa);

phys_addr_t cvi_fast_image_isp_buf_va(void)
{
	return isp_ion.virt_base;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_isp_buf_va);

size_t cvi_fast_image_isp_size(void)
{
	return isp_ion.size;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_isp_size);

phys_addr_t cvi_fast_image_encode_img_pa(void)
{
	return img_ion.phys_addr;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_img_pa);

phys_addr_t cvi_fast_image_encode_img_va(void)
{
	return img_ion.virt_base;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_img_va);

size_t cvi_fast_image_encode_img_size(void)
{
	return img_ion.size;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_img_size);

phys_addr_t cvi_fast_image_encode_buf_pa(void)
{
	return enc_ion.phys_addr;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_buf_pa);

phys_addr_t cvi_fast_image_encode_buf_va(void)
{
	return enc_ion.virt_base;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_buf_va);

size_t cvi_fast_image_encode_buf_size(void)
{
	return enc_ion.size;
}
EXPORT_SYMBOL_GPL(cvi_fast_image_encode_buf_size);

void cvi_fast_image_enc_ion_free(void)
{
	if (enc_ion.phys_addr > 0) {
		pr_debug("enc_ion.phys_addr = %llx\n", enc_ion.phys_addr);
		sys_ion_free(enc_ion.phys_addr);
		enc_ion.phys_addr = 0;
		enc_ion.virt_base = 0;
		enc_ion.size = 0;
	}
}
EXPORT_SYMBOL_GPL(cvi_fast_image_enc_ion_free);

void cvi_fast_image_img_ion_free(void)
{
	if (img_ion.phys_addr > 0) {
		pr_debug("img_ion.phys_addr = %llx\n", img_ion.phys_addr);
		sys_ion_free(img_ion.phys_addr);
		img_ion.phys_addr = 0;
		img_ion.virt_base = 0;
		img_ion.size = 0;
	}
}
EXPORT_SYMBOL_GPL(cvi_fast_image_img_ion_free);

void cvi_fast_image_isp_ion_free(void)
{
	if (isp_ion.phys_addr > 0) {
		pr_debug("isp_ion.phys_addr = %llx\n", isp_ion.phys_addr);
		sys_ion_free(isp_ion.phys_addr);
		isp_ion.phys_addr = 0;
		isp_ion.virt_base = 0;
		isp_ion.size = 0;
	}
}
EXPORT_SYMBOL_GPL(cvi_fast_image_isp_ion_free);


struct _JPEG_PIC *cvi_fast_image_dump_jpg(int idx)
{
	struct _JPEG_PIC *dump_jpg_va;
	cmdqu_t cmdq;
	int ret;

	cmdq.ip_id = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_DUMP_JPG;
	cmdq.param_ptr = idx;
	cmdq.resv.mstime = 1000;
	ret = rtos_cmdqu_send_wait(&cmdq, SYS_CMD_INFO_DUMP_JPG);
	if (ret) {
		pr_err("SYS_CMD_INFO_DUMP_JPG wait fail\n");
		return 0;
	}
	pr_debug("dump_jpg phys_addr = 0x%llx, virt_base = 0x%llx, size = 0x%llx\n",
		(u64)s_mcu_shm.phys_addr, (__u64)s_mcu_shm.virt_base, s_mcu_shm.size);

	// dump jpg va = rtos va + dump_ offset
#ifdef __LP64__
	dump_jpg_va = (struct _JPEG_PIC *) ((u64)s_mcu_shm.virt_base + cmdq.param_ptr - (u64) s_mcu_shm.phys_addr);
#else
	dump_jpg_va = (struct _JPEG_PIC *) le32_to_cpu(((u64)s_mcu_shm.virt_base +
		cmdq.param_ptr - (u64) s_mcu_shm.phys_addr));
#endif
	pr_debug("dump_jpg cmdq.param_ptr = %x\n", cmdq.param_ptr);
	pr_debug("dump_jpg s_mcu_shm.phys_addr = %llx\n", s_mcu_shm.phys_addr);
	pr_debug("dump_jpg_va = %lx\n", (unsigned long)dump_jpg_va);
	pr_debug("dump_jpg [%d] width=%d height=%d addr=%x size=%d\n",
		idx, dump_jpg_va->width, dump_jpg_va->height, dump_jpg_va->addr, dump_jpg_va->size);
	return dump_jpg_va;
}

struct dump_uart_s *cvi_fast_image_dump_msg(void)
{
	struct dump_uart_s *dump_uart_va;
	cmdqu_t cmdq;
	int ret;

	cmdq.ip_id = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_DUMP_MSG;
	cmdq.param_ptr = 0;
	cmdq.resv.mstime = 1000;
	ret = rtos_cmdqu_send_wait(&cmdq, SYS_CMD_INFO_DUMP_MSG);
	if (ret) {
		pr_err("SYS_CMD_INFO_DUMP_MSG wait fail\n");
		return 0;
	}
	pr_debug("dump_msg phys_addr = 0x%llx, virt_base = 0x%llx, size = 0x%llx\n",
		(u64)s_mcu_shm.phys_addr, (__u64)s_mcu_shm.virt_base, s_mcu_shm.size);

	// dump uart va = rtos va + dump_uart offset
#ifdef __LP64__
	dump_uart_va = (struct dump_uart_s *) ((u64)s_mcu_shm.virt_base + cmdq.param_ptr - (u64) s_mcu_shm.phys_addr);
#else
	dump_uart_va = (struct dump_uart_s *) le32_to_cpu((u64)s_mcu_shm.virt_base +
		cmdq.param_ptr - (u64) s_mcu_shm.phys_addr);
#endif
	pr_debug("dump_msg cmdq.param_ptr = %x\n", cmdq.param_ptr);
	pr_debug("dump_msg s_mcu_shm.phys_addr = %llx\n", s_mcu_shm.phys_addr);
	pr_debug("dump_uart_va = %lx\n", (unsigned long)dump_uart_va);
	return dump_uart_va;
}

struct trace_snapshot_t *cvi_fast_image_dump_snapshot(void)
{
	struct trace_snapshot_t *dump_snapshot_va;
	cmdqu_t cmdq;
	int ret;

	cmdq.ip_id = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_TRACE_SNAPSHOT_STOP;
	cmdq.param_ptr = 0;
	cmdq.resv.mstime = 1000;
	ret = rtos_cmdqu_send_wait(&cmdq, SYS_CMD_INFO_TRACE_SNAPSHOT_STOP);
	if (ret) {
		pr_err("SYS_CMD_INFO_TRACE_SNAPSHOT_STOP wait fail\n");
		return 0;
	}
	pr_debug("dump_snapshot phys_addr = 0x%llx, virt_base = 0x%llx, size = 0x%llx\n",
		(u64)s_mcu_shm.phys_addr, (__u64)s_mcu_shm.virt_base, s_mcu_shm.size);

	// dump uart va = rtos va + dump_uart offset
#ifdef __LP64__
	dump_snapshot_va = (struct trace_snapshot_t *) ((u64)s_mcu_shm.virt_base
						+ cmdq.param_ptr - (u64) s_mcu_shm.phys_addr);
#else
	dump_snapshot_va = (struct trace_snapshot_t *) le32_to_cpu((u64)s_mcu_shm.virt_base
						+ cmdq.param_ptr - (u64) s_mcu_shm.phys_addr);
#endif
	pr_debug("dump_snapshot cmdq.param_ptr = %x\n", cmdq.param_ptr);
	pr_debug("dump_snapshot s_mcu_shm.phys_addr = %llx\n", s_mcu_shm.phys_addr);
	pr_debug("dump_snapshot_va = %lx\n", (unsigned long)dump_snapshot_va);
	return dump_snapshot_va;
}


static long cvi_fast_image_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	phys_addr_t addr;
	long ret = 0;
	size_t size, idx;
	struct dump_uart_s *dump_info;
	char *msg_va;
	cmdqu_t cmdq;

	pr_debug("%s cmd = %d", __func__, cmd);
	switch (cmd) {
	case FAST_IMAGE_SEND_STOP_REC:
		pr_debug("FAST_IMAGE_SEND_STOP_REC\n");
		cvi_stop_fast_image();
		break;
	case FAST_IMAGE_QUERY_ISP_PADDR:
		addr = cvi_fast_image_isp_buf_pa();
		pr_debug("FAST_IMAGE_QUERY_ISP_PADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(phys_addr_t));
		break;
	case FAST_IMAGE_QUERY_ISP_VADDR:
		addr = cvi_fast_image_isp_buf_va();
		pr_debug("FAST_IMAGE_QUERY_ISP_VADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(phys_addr_t));
		break;
	case FAST_IMAGE_QUERY_ISP_SIZE:
		size = cvi_fast_image_isp_size();
		pr_debug("FAST_IMAGE_QUERY_ISP_SIZE %zx\n", size);
		ret = copy_to_user((size_t __user *)arg,
		&size,
		sizeof(size_t));
		break;
	case FAST_IMAGE_QUERY_ISP_CTXT:
		addr = cvi_fast_image_isp_buf_va();
		ret = copy_to_user((size_t __user *)arg,
#ifdef __LP64__
			(void *)addr,
#else
			(void *)le32_to_cpu(addr),
#endif
			0x100);
		break;
	case FAST_IMAGE_QUERY_IMG_PADDR:
		addr = cvi_fast_image_encode_img_pa();
		pr_debug("FAST_IMAGE_QUERY_IMG_PADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(struct cmdqu_t));
		break;
	case FAST_IMAGE_QUERY_IMG_VADDR:
		addr = cvi_fast_image_encode_img_va();
		pr_debug("FAST_IMAGE_QUERY_IMG_VADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(phys_addr_t));
		break;
	case FAST_IMAGE_QUERY_IMG_SIZE:
		size = cvi_fast_image_encode_img_size();
		pr_debug("FAST_IMAGE_QUERY_IMG_SIZE %zx\n", size);
		ret = copy_to_user((size_t __user *)arg,
		&size,
		sizeof(size_t));
		break;
	case FAST_IMAGE_QUERY_IMG_CTXT:
		addr = cvi_fast_image_encode_img_va();
		ret = copy_to_user((size_t __user *)arg,
#ifdef __LP64__
			(void *)addr,
#else
			(void *)le32_to_cpu(addr),
#endif
			0x100);
		break;
	case FAST_IMAGE_QUERY_ENC_PADDR:
		addr = cvi_fast_image_encode_buf_pa();
		pr_debug("FAST_IMAGE_QUERY_ENC_PADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(phys_addr_t));
		break;
	case FAST_IMAGE_QUERY_ENC_VADDR:
		addr = cvi_fast_image_encode_buf_va();
		pr_debug("FAST_IMAGE_QUERY_ENC_VADDR %llx\n", addr);
		ret = copy_to_user((phys_addr_t __user *)arg,
		&addr,
		sizeof(phys_addr_t));
		break;
	case FAST_IMAGE_QUERY_ENC_SIZE:
		size = cvi_fast_image_encode_buf_size();
		pr_debug("FAST_IMAGE_QUERY_ENC_SIZE %zx\n", size);
		ret = copy_to_user((size_t __user *)arg,
		&size,
		sizeof(size_t));
		break;
	case FAST_IMAGE_QUERY_ENC_CTXT:
		addr = cvi_fast_image_encode_buf_va();
		ret = copy_to_user((size_t __user *)arg,
#ifdef __LP64__
			(void *)addr,
#else
			(void *)le32_to_cpu(addr),
#endif
			0x100);
		break;
	case FAST_IMAGE_QUERY_FREE_ISP_ION:
		cvi_fast_image_isp_ion_free();
		break;
	case FAST_IMAGE_QUERY_FREE_IMG_ION:
		cvi_fast_image_img_ion_free();
		break;
	case FAST_IMAGE_QUERY_FREE_ENC_ION:
		cvi_fast_image_enc_ion_free();
		break;
	case FAST_IMAGE_QUERY_DUMP_EN:
		cmdq.ip_id = IP_SYSTEM;
		cmdq.cmd_id = SYS_CMD_INFO_DUMP_EN;
		cmdq.param_ptr = 0;
		rtos_cmdqu_send(&cmdq);
		break;
	case FAST_IMAGE_QUERY_DUMP_DIS:
		cmdq.ip_id = IP_SYSTEM;
		cmdq.cmd_id = SYS_CMD_INFO_DUMP_DIS;
		cmdq.param_ptr = 0;
		rtos_cmdqu_send(&cmdq);
		break;
	case FAST_IMAGE_QUERY_DUMP_JPG_INFO:
		copy_from_user(&idx,
			(size_t __user *)arg,
			sizeof(size_t));
		dump_jpg_va = cvi_fast_image_dump_jpg(idx);
		if (!dump_jpg_va) {
			pr_err("dump_jpg_va = 0\n");
			break;
		}
		size = dump_jpg_va->size;
		pr_debug("FAST_IMAGE_QUERY_DUMP_JPG_INFO size=%zx\n", size);
		ret = copy_to_user((size_t __user *)arg,
			&size,
			sizeof(size_t));
		break;
	case FAST_IMAGE_QUERY_DUMP_JPG:
		//dump_info = kzalloc(dump_jpg_va->size, GFP_KERNEL);
#ifdef __LP64__
		msg_va = (char *)((u64)cvi_fast_image_encode_img_va() +
			dump_jpg_va->addr - (u64) cvi_fast_image_encode_img_pa());
#else
		msg_va = (char *)le32_to_cpu((u64)cvi_fast_image_encode_img_va() +
			dump_jpg_va->addr - (u64) cvi_fast_image_encode_img_pa());
#endif
		pr_debug("FAST_IMAGE_QUERY_DUMP_JPG PA=%p\n", msg_va);
		ret = copy_to_user((size_t __user *)arg,
			msg_va,
			dump_jpg_va->size);
		break;
	case FAST_IMAGE_QUERY_DUMP_MSG_INFO:
		dump_uart_va = cvi_fast_image_dump_msg();
		if (!dump_uart_va) {
			pr_err("dump_uart_va = 0\n");
			break;
		}
		if (!dump_uart_va->dump_uart_enable) {
			pr_err("dump_uart_disable\n");
			break;
		}
		pr_debug("DUMP_MSG dump_uart_va = %lx\n", (unsigned long)dump_uart_va);
		if (dump_uart_va->dump_uart_max_size + sizeof(struct dump_uart_s) < DUMP_PRINT_DEFAULT_SIZE) {
			pr_debug("1dump_uart_va->dump_uart_max_size addr=%lx\n",
				(unsigned long)&dump_uart_va->dump_uart_max_size);
			pr_debug("1dump_info->dump_uart_max_size = %x\n", dump_uart_va->dump_uart_max_size);
			break;
		}
		ret = copy_to_user((size_t __user *)arg,
			dump_uart_va,
			sizeof(struct dump_uart_s));
		break;
	case FAST_IMAGE_QUERY_DUMP_MSG:
		dump_info = kzalloc(dump_uart_va->dump_uart_max_size
					+ sizeof(struct dump_uart_s), GFP_KERNEL);
		dump_info->dump_uart_max_size = dump_uart_va->dump_uart_max_size;
#ifdef __LP64__
		msg_va = (char *)((u64)s_mcu_shm.virt_base + dump_uart_va->dump_uart_ptr - (u64) s_mcu_shm.phys_addr);
#else
		msg_va = (char *)le32_to_cpu((u64)s_mcu_shm.virt_base +
			dump_uart_va->dump_uart_ptr - (u64) s_mcu_shm.phys_addr);
#endif

		pr_debug("dump_info->dump_uart_max_size = %x\n", dump_uart_va->dump_uart_max_size);
		pr_debug("dump_info->dump_uart_all_size = %zx\n",
			dump_uart_va->dump_uart_max_size + sizeof(struct dump_uart_s));
		/*concat the message*/
		if (dump_uart_va->dump_uart_overflow) {
			pr_debug("dump_uart_va->dump_uart_overflow =%x\n", dump_uart_va->dump_uart_overflow);
			pr_debug("dump_info =%lx\n", (unsigned long) dump_info);

			memcpy((char *) dump_info + sizeof(struct dump_uart_s),
				msg_va + dump_uart_va->dump_uart_pos,
				dump_uart_va->dump_uart_max_size - dump_uart_va->dump_uart_pos);
			memcpy((char *) dump_info + sizeof(struct dump_uart_s)
				+ dump_uart_va->dump_uart_max_size - dump_uart_va->dump_uart_pos,
				msg_va, dump_uart_va->dump_uart_pos);
		} else {
			pr_debug("dump_info =%lx\n", (unsigned long) dump_info);
			memcpy((char *) dump_info + sizeof(struct dump_uart_s),
				msg_va, dump_uart_va->dump_uart_pos);
		}
		ret = copy_to_user((size_t __user *)arg,
			dump_info,
			dump_info->dump_uart_max_size + sizeof(struct dump_uart_s));
		kfree(dump_info);
		break;
	case FAST_IMAGE_QUERY_TRACE_SNAPSHOT_START:
		cmdq.ip_id = IP_SYSTEM;
		cmdq.cmd_id = SYS_CMD_INFO_TRACE_SNAPSHOT_START;
		cmdq.param_ptr = 0;
		rtos_cmdqu_send(&cmdq);
		break;
	case FAST_IMAGE_QUERY_TRACE_SNAPSHOT_STOP:
		dump_snapshot_va = cvi_fast_image_dump_snapshot();
		pr_debug("FAST_IMAGE_QUERY_TRACE_SNAPSHOT_STOP VA=%lx\n", (unsigned long)dump_snapshot_va);
		size = dump_snapshot_va->size;
		ret = copy_to_user((size_t __user *)arg,
			&size,
			sizeof(size_t));
		break;
	case FAST_IMAGE_QUERY_TRACE_SNAPSHOT_DUMP:
#ifdef __LP64__
		msg_va = (char *)((u64)s_mcu_shm.virt_base + dump_snapshot_va->ptr - (u64) s_mcu_shm.phys_addr);
#else
		msg_va = (char *)le32_to_cpu((u64)s_mcu_shm.virt_base +
			dump_snapshot_va->ptr - (u64) s_mcu_shm.phys_addr);
#endif
		ret = copy_to_user((size_t __user *)arg,
			msg_va,
			dump_snapshot_va->size);
		break;
	case FAST_IMAGE_QUERY_TRACE_STREAM_START:
		cmdq.ip_id = IP_SYSTEM;
		cmdq.cmd_id = SYS_CMD_INFO_TRACE_STREAM_START;
		cmdq.param_ptr = 0;
		rtos_cmdqu_send(&cmdq);
		break;
	default:
		pr_debug("ioctl default: %d\n", cmd);
		ret = -EFAULT;
		break;
	}
	return ret;
}

static const struct file_operations fast_image_fops = {
	.owner = THIS_MODULE,
	.open = cvi_fast_image_open,
	.release = cvi_fast_image_release,
	.unlocked_ioctl = cvi_fast_image_ioctl
};

static int _register_dev(struct cvi_fast_image_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = FAST_IMAGE_DEV_NAME;
	ndev->miscdev.fops = &fast_image_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_fast_image: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

void cvi_fast_image_callback_handler(unsigned char cmd_id, unsigned int ptr, void *dev_id)
{
	pr_debug("%s cmd_id =%x ptr=%x\n", __func__, cmd_id, ptr);
	mcu_transfer_config_offset = ptr;
}

static int cvi_fast_image_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_fast_image_device *ndev;
	int ret = 0;
	cmdqu_t cmdq;

	pr_debug("%s start\n", __func__);
	pr_debug("name=%s\n", pdev->name);

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->dev = dev;

	ret = _register_dev(ndev);
	if (ret < 0) {
		pr_err("regsiter cvi_fast_image chrdev error\n");
		return ret;
	}
	// TODO: send wait with block mode to freertos
	cmdq.ip_id = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_LINUX_INIT_DONE;
	cmdq.param_ptr = 0;
	cmdq.resv.mstime = 200;
	ret = rtos_cmdqu_send_wait(&cmdq, SYS_CMD_INFO_RTOS_INIT_DONE);
	if (ret)
		pr_err("SYS_CMD_INFO_LINUX_INIT_DONE fail\n");
	mcu_transfer_config_offset = cmdq.param_ptr;
	pr_debug("mcu_transfer_config_offset = %x\n", mcu_transfer_config_offset);
	// get communication pa from rtos
	ret = cvi_allocate_mcu_shm(pdev);
	if (ret == 0)
		cvi_fast_image_ion_alloc();
	platform_set_drvdata(pdev, ndev);
	pr_debug("%s DONE\n", __func__);
	return 0;
}

static int cvi_fast_image_remove(struct platform_device *pdev)
{
	struct cvi_fast_image_device *ndev = platform_get_drvdata(pdev);

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);
	pr_debug("%s DONE\n", __func__);

	return 0;
}

static const struct of_device_id cvi_rtos_image_match[] = {
	{ .compatible = "cvitek,rtos_image" },
	{},
};

static struct platform_driver cvi_fast_image_driver = {
	.probe = cvi_fast_image_probe,
	.remove = cvi_fast_image_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = FAST_IMAGE_DEV_NAME,
		.of_match_table = cvi_rtos_image_match,
	},
};

struct class *pbase_class;
static void cvi_fast_image_exit(void)
{
	pr_debug("%s start\n", __func__);
	cvi_stop_fast_image();
	platform_driver_unregister(&cvi_fast_image_driver);
	class_destroy(pbase_class);
	pr_debug("%s DONE\n", __func__);
}

static const struct of_device_id cvi_vpu_match_table[] = {
	{ .compatible = "cvitek,asic-vcodec"},       // for cv181x asic
	{},
};

static const struct of_device_id cvi_jpu_match_table[] = {
	{ .compatible = "cvitek,asic-jpeg"},       // for cv181x asic
	{},
};

static const struct of_device_id cvi_vip_match_table[] = {
	{ .compatible = "cvitek,vi"},       // for cv181x asic
	{},
};

#define VPU_PLATFORM_DEVICE_NAME    "vcodec"
#define JPU_PLATFORM_DEVICE_NAME    "jpu"
#define VIP_PLATFORM_DEVICE_NAME    "vip"

static int vpu_probe(struct platform_device *pdev);
static int jpu_probe(struct platform_device *pdev);
static int vip_probe(struct platform_device *pdev);

static struct platform_driver vpu_driver = {
	.driver = {
		.name = VPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_vpu_match_table,
	},
	.probe = vpu_probe,
};

static struct platform_driver jpu_driver = {
	.driver = {
		.name = JPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_jpu_match_table,
	},
	.probe = jpu_probe,
};

static struct platform_driver vip_driver = {
	.driver = {
		.name = VIP_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_vip_match_table,
	},
	.probe = vip_probe,
};

static int GetIrq(struct platform_device *pdev, char *irq_name)
{
	int irq;

	if (!pdev) {
		pr_err("%s pdev = NULL\n", __func__);
		return -1;
	}

	irq = platform_get_irq_byname(pdev, irq_name);
	pr_debug("irq[%s] = %d\n", irq_name, irq);
	return irq;
}

static int vpu_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;

	pr_debug("%s\n", __func__);
	match = of_match_device(cvi_vpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	vcodec_h264_irq = GetIrq(pdev, "h264");
	if (vcodec_h264_irq < 0) {
		pr_err("vcodec h264 irq = %d\n", vcodec_h264_irq);
		return -EINVAL;
	}

	irq_set_status_flags(vcodec_h264_irq, IRQ_NOAUTOEN);

	vcodec_h265_irq = GetIrq(pdev, "h265");
	if (vcodec_h265_irq < 0) {
		pr_err("vcodec h265 irq = %d\n", vcodec_h265_irq);
		return -EINVAL;
	}

	irq_set_status_flags(vcodec_h265_irq, IRQ_NOAUTOEN);

	return 0;
}

static int jpu_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;

	pr_debug("%s\n", __func__);
	match = of_match_device(cvi_jpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	jpu_irq = GetIrq(pdev, "jpeg");
	if (jpu_irq < 0) {
		pr_err("jpu irq = %d\n", jpu_irq);
		return -EINVAL;
	}

	irq_set_status_flags(jpu_irq, IRQ_NOAUTOEN);

	return 0;
}

static int vip_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;

	pr_debug("%s\n", __func__);
	match = of_match_device(cvi_vip_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	isp_irq = GetIrq(pdev, "isp");
	if (isp_irq < 0) {
		pr_err("isp irq = %d\n", isp_irq);
		return -EINVAL;
	}

	irq_set_status_flags(isp_irq, IRQ_NOAUTOEN);

	return 0;
}

int cvi_allocate_mcu_shm(struct platform_device *pdev)
{
	struct resource *res = NULL;
	int i;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		pr_err("failed to retrieve rtos region\n");
		return -ENXIO;
	}
	pr_debug("res->start=%pa res->end=%pa\n", &res->start, &res->end);
	s_mcu_shm.phys_addr = res->start;
	s_mcu_shm.size = (res->end - res->start + 1);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#ifdef __LP64__
	s_mcu_shm.virt_base = (u64)devm_ioremap(&pdev->dev, res->start,
							res->end - res->start + 1);
#else
	s_mcu_shm.virt_base = le64_to_cpu((u32)devm_ioremap(&pdev->dev, res->start,
							res->end - res->start + 1));
#endif
#else
#ifdef __LP64__
	s_mcu_shm.virt_base = (u64)devm_ioremap_nocache(&pdev->dev, res->start,
							res->end - res->start + 1);
#else
	s_mcu_shm.virt_base = le64_to_cpu((u32)devm_ioremap_nocache(&pdev->dev, res->start,
							res->end - res->start + 1));
#endif
#endif

	if (!s_mcu_shm.virt_base) {
		pr_err("[FreeRTOS] ioremap fail!\n");
		pr_err("s_mcu_shm.virt_base = 0x%llx\n", s_mcu_shm.virt_base);
		return -1;
	}

	pr_debug("reserved memory phys_addr = 0x%llx, virt_base = 0x%llx, size = 0x%llx\n",
		(u64)s_mcu_shm.phys_addr, (__u64)s_mcu_shm.virt_base, s_mcu_shm.size);
	pr_debug("success to probe mcu shm\n");

	if (mcu_transfer_config_offset == 0) {
		pr_err("communicate with rtos fail\n");
		return -1;
	}
#ifdef __LP64__
	transfer_config = (struct transfer_config_t *)
		 (s_mcu_shm.virt_base + mcu_transfer_config_offset - s_mcu_shm.phys_addr);
#else
	transfer_config = (struct transfer_config_t *)
		 le32_to_cpu(s_mcu_shm.virt_base + mcu_transfer_config_offset - s_mcu_shm.phys_addr);
#endif
	pr_debug("transfer_config addr = %lx\n", (unsigned long)transfer_config);
	if (transfer_config->conf_magic == RTOS_MAGIC_HEADER) {
		unsigned short checksum = 0;
		unsigned char *ptr = (unsigned char *) &transfer_config->conf_magic;

		for (i = 0; i < transfer_config->conf_size; i++, ptr++)
			checksum += *ptr;
		if (checksum != transfer_config->checksum) {
			pr_err("checksum (%d) fail, transfer_config (%d)\n", checksum, transfer_config->checksum);
			return -1;
		}
	} else {
		pr_err("transfer_config->conf_magic fail (%x)\n", transfer_config->conf_magic);
		return -1;
	}
	pr_debug("transfer_config->magic=%x\n", transfer_config->conf_magic);
	pr_debug("transfer_config->conf_size=%x\n", transfer_config->conf_size);
	pr_debug("transfer_config->isp_buffer_addr =%x\n", transfer_config->isp_buffer_addr);
	pr_debug("transfer_config->image_type =%x\n", transfer_config->image_type);

	if (transfer_config->image_type == E_FAST_NONE) {
		return -1;
	}
	return 0;
}

void cvi_fast_image_ion_alloc(void)
{
	int ret;

	/* used to remap ion and encode buffer context */
	if (transfer_config->encode_img_size) {
		img_ion.size = transfer_config->encode_img_size;
		ret = sys_ion_alloc(&img_ion.phys_addr, (void *)&img_ion.virt_base,
							 "fast_image_img", img_ion.size, 0);
		if (ret)
			pr_err("img_ion_fd alloc size(%llx)=%d fail\n", img_ion.size, ret);
		if (img_ion.phys_addr != transfer_config->encode_img_addr) {
			pr_err("img_ionbuf->paddr = %llx is not match transfer_config->encode_img_addr = %x\n",
				img_ion.phys_addr, transfer_config->encode_img_addr);
			cvi_fast_image_img_ion_free();
		} else
			pr_debug("img_ion phys_addr = %llx virt_base = %llx size =%llx\n",
					 img_ion.phys_addr, img_ion.virt_base, img_ion.size);
	}
	if (transfer_config->encode_buf_size) {
		enc_ion.size = transfer_config->encode_buf_size;
		ret = sys_ion_alloc(&enc_ion.phys_addr, (void *)&enc_ion.virt_base,
							 "fast_image_enc", enc_ion.size, 0);
		if (ret)
			pr_err("enc_ion_fd alloc size(%llx)=%d fail\n", enc_ion.size, ret);
		if (enc_ion.phys_addr != transfer_config->encode_buf_addr) {
			pr_err("enc_ionbuf->paddr = %llx is not match transfer_config->encode_buf_addr = %x\n",
				enc_ion.phys_addr, transfer_config->encode_buf_addr);
			cvi_fast_image_enc_ion_free();
		} else
			pr_debug("enc_ion phys_addr = %llx virt_base = %llx size =%llx\n",
					 enc_ion.phys_addr, enc_ion.virt_base, enc_ion.size);
	}

	if (transfer_config->isp_buffer_size) {
		isp_ion.size = transfer_config->isp_buffer_size;
		ret = sys_ion_alloc(&isp_ion.phys_addr, (void *)&isp_ion.virt_base,
							 "fast_image_isp", isp_ion.size, 0);
		if (ret)
			pr_err("isp_ion_fd alloc size(%llx)=%d fail\n", isp_ion.size, ret);
		if (isp_ion.phys_addr != transfer_config->isp_buffer_addr) {
			pr_err("isp_ionbuf->paddr = %llx is not match transfer_config->isp_buffer_addr = %x\n",
				isp_ion.phys_addr, transfer_config->isp_buffer_addr);
			cvi_fast_image_isp_ion_free();
		} else
			pr_debug("isp_ion phys_addr = %llx virt_base = %llx size =%llx\n",
					 isp_ion.phys_addr, isp_ion.virt_base, isp_ion.size);
	}

}

static int cvi_fast_image_init(void)
{
	int rc;

	pr_debug("cvi_fast_image_init");
	pbase_class = class_create(THIS_MODULE, FAST_IMAGE_DEV_NAME);
	if (IS_ERR(pbase_class)) {
		pr_err("create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	platform_driver_register(&cvi_fast_image_driver);

	/* image_type = 0 --> not enable camera sensor */
	if (transfer_config != NULL && transfer_config->image_type) {
		pr_debug("register/unregister vpu/jpu/vip driver\n");
		platform_driver_register(&vpu_driver);
		platform_driver_unregister(&vpu_driver);
		platform_driver_register(&jpu_driver);
		platform_driver_unregister(&jpu_driver);
		platform_driver_register(&vip_driver);
		platform_driver_unregister(&vip_driver);
	}
	pr_debug("%s\n", __func__);

	return 0;
cleanup:
	cvi_fast_image_exit();
	return rc;
}

MODULE_DESCRIPTION("CVI_FAST_IMAGE");
MODULE_LICENSE("GPL");
module_init(cvi_fast_image_init);
module_exit(cvi_fast_image_exit);
