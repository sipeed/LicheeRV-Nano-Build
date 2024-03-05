/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vcodec.c
 * Description:
 */

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/streamline_annotate.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif

#include "vpuconfig.h"
#include "cvi_vcodec.h"
#include "vcodec_common.h"

/* definitions to be changed as customer  configuration */
/* if you want to have clock gating scheme frame by frame */
//#define VPU_SUPPORT_CLOCK_CONTROL

/* if the driver want to use interrupt service from kernel ISR */
//#define VPU_SUPPORT_ISR

/* if the platform driver knows the name of this driver */
/* VPU_PLATFORM_DEVICE_NAME */
//#define VPU_SUPPORT_PLATFORM_DRIVER_REGISTER

/* if this driver knows the dedicated video memory address */
#define VPU_SUPPORT_RESERVED_VIDEO_MEMORY

/* global device context to avoid kernal config mismatch in filp->private_data */
#define VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT

#define VPU_PLATFORM_DEVICE_NAME	"vcodec"
#define VPU_CLK_NAME			"vcodec"
#define VPU_CLASS_NAME			"vcodec"
#define VPU_DEV_NAME			"vcodec"

/* if the platform driver knows this driver */
/* the definition of VPU_REG_BASE_ADDR and VPU_REG_SIZE are not meaningful */

#define VPU_REG_BASE_ADDR 0xb020000
#define VPU_REG_SIZE (0x4000 * MAX_NUM_VPU_CORE)

#define VPU_IRQ_NUM (77 + 32)

/* this definition is only for chipsnmedia FPGA board env */
/* so for SOC env of customers can be ignored */

#ifndef VM_RESERVED /*for kernel up to 3.7.0 version*/
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

/* To track the allocated memory buffer */
typedef struct vpudrv_buffer_pool_t {
	struct list_head list;
	struct vpudrv_buffer_t vb;
	struct file *filp;
} vpudrv_buffer_pool_t;

/* To track the instance index and buffer in instance pool */
typedef struct vpudrv_instanace_list_t {
	struct list_head list;
	unsigned long inst_idx;
	unsigned long core_idx;
	struct file *filp;
} vpudrv_instanace_list_t;

typedef struct vpudrv_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
} vpudrv_instance_pool_t;

struct clk_ctrl_info {
	int core_idx;
	int enable;
};

#ifndef CVI_H26X_USE_ION_FW_BUFFER
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
#define VPU_INIT_VIDEO_MEMORY_SIZE_IN_BYTE (62 * 1024 * 1024)
#define VPU_DRAM_PHYSICAL_BASE 0x86C00000
#include "vmm.h"
static video_mm_t s_vmem;
static vpudrv_buffer_t s_video_memory = { 0 };
#endif /*VPU_SUPPORT_RESERVED_VIDEO_MEMORY*/
#endif /*CVI_H26X_USE_ION_FW_BUFFER*/

#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
// global variable to avoid kernal config mismatch in filp->private_data
static void *pCviVpuDevice;
#endif

static int vpu_hw_reset(void);

static int s_vpu_open_ref_count;

static int coreIdxMapping[VENC_MAX_CHN_NUM] = {-1};
static int chnIdxMapping[MAX_NUM_VPU_CORE] = {-1};
wait_queue_head_t tWaitQueue[VENC_MAX_CHN_NUM];
EXPORT_SYMBOL(tWaitQueue);

static spinlock_t s_vpu_lock = __SPIN_LOCK_UNLOCKED(s_vpu_lock);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
static DECLARE_MUTEX(s_vpu_sem);
#else
static DEFINE_SEMAPHORE(s_vpu_sem);
#endif
static struct list_head s_vbp_head = LIST_HEAD_INIT(s_vbp_head);
static struct list_head s_inst_list_head = LIST_HEAD_INIT(s_inst_list_head);

struct cvi_vcodec_device vcodec_dev;

/* implement to power management functions */
#define BIT_BASE 0x0000
#define BIT_CODE_RUN (BIT_BASE + 0x000)
#define BIT_CODE_DOWN (BIT_BASE + 0x004)
#define BIT_INT_CLEAR (BIT_BASE + 0x00C)
#define BIT_INT_STS (BIT_BASE + 0x010)
#define BIT_CODE_RESET (BIT_BASE + 0x014)
#define BIT_INT_REASON (BIT_BASE + 0x174)
#define BIT_BUSY_FLAG (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND (BIT_BASE + 0x164)
#define BIT_RUN_INDEX (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD (BIT_BASE + 0x16C)

/* WAVE4 registers */
#define W4_REG_BASE 0x0000
#define W4_VPU_BUSY_STATUS (W4_REG_BASE + 0x0070)
#define W4_VPU_INT_REASON_CLEAR (W4_REG_BASE + 0x0034)
#define W4_VPU_VINT_CLEAR (W4_REG_BASE + 0x003C)
#define W4_VPU_VPU_INT_STS (W4_REG_BASE + 0x0044)
#define W4_VPU_INT_REASON (W4_REG_BASE + 0x004c)

#define W4_RET_SUCCESS (W4_REG_BASE + 0x0110)
#define W4_RET_FAIL_REASON (W4_REG_BASE + 0x0114)

/* WAVE4 INIT, WAKEUP */
#define W4_PO_CONF (W4_REG_BASE + 0x0000)
#define W4_VCPU_CUR_PC (W4_REG_BASE + 0x0004)

#define W4_VPU_VINT_ENABLE (W4_REG_BASE + 0x0048)

#define W4_VPU_RESET_REQ (W4_REG_BASE + 0x0050)
#define W4_VPU_RESET_STATUS (W4_REG_BASE + 0x0054)

#define W4_VPU_REMAP_CTRL (W4_REG_BASE + 0x0060)
#define W4_VPU_REMAP_VADDR (W4_REG_BASE + 0x0064)
#define W4_VPU_REMAP_PADDR (W4_REG_BASE + 0x0068)
#define W4_VPU_REMAP_CORE_START (W4_REG_BASE + 0x006C)
#define W4_VPU_BUSY_STATUS (W4_REG_BASE + 0x0070)

#define W4_REMAP_CODE_INDEX 0
enum {
	W4_INT_INIT_VPU = 0,
	W4_INT_DEC_PIC_HDR = 1,
	W4_INT_SET_PARAM = 1,
	W4_INT_ENC_INIT_SEQ = 1,
	W4_INT_FINI_SEQ = 2,
	W4_INT_DEC_PIC = 3,
	W4_INT_ENC_PIC = 3,
	W4_INT_SET_FRAMEBUF = 4,
	W4_INT_FLUSH_DEC = 5,
	W4_INT_ENC_SLICE_INT = 7,
	W4_INT_GET_FW_VERSION = 8,
	W4_INT_QUERY_DEC = 9,
	W4_INT_SLEEP_VPU = 10,
	W4_INT_WAKEUP_VPU = 11,
	W4_INT_CHANGE_INT = 12,
	W4_INT_CREATE_INSTANCE = 14,
	W4_INT_BSBUF_EMPTY = 15, /*!<< Bitstream buffer empty[dec]/full[enc] */
};

enum {
	W5_INT_INIT_VPU = 0,
	W5_INT_WAKEUP_VPU = 1,
	W5_INT_SLEEP_VPU = 2,
	W5_INT_CREATE_INSTANCE = 3,
	W5_INT_FLUSH_INSTANCE = 4,
	W5_INT_DESTROY_INSTANCE = 5,
	W5_INT_INIT_SEQ = 6,
	W5_INT_SET_FRAMEBUF = 7,
	W5_INT_DEC_PIC = 8,
	W5_INT_ENC_PIC = 8,
	W5_INT_ENC_SET_PARAM = 9,
	W5_INT_DEC_QUERY = 14,
	W5_INT_BSBUF_EMPTY = 15,
};

#define W4_HW_OPTION (W4_REG_BASE + 0x0124)
#define W4_CODE_SIZE (W4_REG_BASE + 0x011C)
/* Note: W4_INIT_CODE_BASE_ADDR should be aligned to 4KB */
#define W4_ADDR_CODE_BASE (W4_REG_BASE + 0x0118)
#define W4_CODE_PARAM (W4_REG_BASE + 0x0120)
#define W4_INIT_VPU_TIME_OUT_CNT (W4_REG_BASE + 0x0134)

/************************************************************************/
/* DECODER - DEC_PIC_HDR/DEC_PIC                                        */
/************************************************************************/
#define W4_BS_PARAM (W4_REG_BASE + 0x0128)
#define W4_BS_RD_PTR (W4_REG_BASE + 0x0130)
#define W4_BS_WR_PTR (W4_REG_BASE + 0x0134)

/* WAVE5 registers */
#define W5_ADDR_CODE_BASE (W4_REG_BASE + 0x0110)
#define W5_CODE_SIZE (W4_REG_BASE + 0x0114)
#define W5_CODE_PARAM (W4_REG_BASE + 0x0128)
#define W5_INIT_VPU_TIME_OUT_CNT (W4_REG_BASE + 0x0130)

#define W5_HW_OPTION (W4_REG_BASE + 0x012C)

#define W5_RET_SUCCESS (W4_REG_BASE + 0x0108)

/* WAVE4 Wave4BitIssueCommand */
#define W4_CORE_INDEX (W4_REG_BASE + 0x0104)
#define W4_INST_INDEX (W4_REG_BASE + 0x0108)
#define W4_COMMAND (W4_REG_BASE + 0x0100)
#define W4_VPU_HOST_INT_REQ (W4_REG_BASE + 0x0038)

/* Product register */
#define VPU_PRODUCT_CODE_REGISTER (BIT_BASE + 0x1044)

/* Clock enable/disable */
#define VCODEC_CLK_ENABLE 1
#define VCODEC_CLK_DISABLE 0

#ifdef CONFIG_PM
static u32 s_vpu_reg_store[MAX_NUM_VPU_CORE][64];
#endif

#define ReadVpuRegister(addr)	\
	(readl(pvctx->s_vpu_register.virt_addr + \
			pvctx->s_bit_firmware_info.reg_base_offset + \
			(addr)))
#define WriteVpuRegister(addr, val)	\
	(writel((val), pvctx->s_vpu_register.virt_addr + \
			pvctx->s_bit_firmware_info.reg_base_offset + \
			(addr)))
#define WriteVpu(addr, val)	\
	(writel((val), (addr)))

#define ReadSbmRegister(addr)	\
		(readl(vcodec_dev.sbm_register.virt_addr + \
				(addr)))
#define WritSbmRegister(addr, val)	\
			(writel((val), vcodec_dev.sbm_register.virt_addr + \
					(addr)))


static int bSingleCore;
module_param(bSingleCore, int, 0644);

static int cviIoctlGetInstPool(u_long arg);
static int cviGetRegResource(struct cvi_vpu_device *vdev, struct platform_device *pdev);
static int cvi_vcodec_register_cdev(struct cvi_vpu_device *vdev);
static int cviCfgIrq(struct platform_device *pdev);
static void cviFreeIrq(void);
static void cviReleaseRegResource(struct cvi_vpu_device *vdev);
static void cviUnmapReg(vpudrv_buffer_t *pReg);
#ifndef CVI_H26X_USE_ION_FW_BUFFER
static int cvi_vcodec_allocate_memory(struct platform_device *pdev);
#endif

static void set_clock_enable(struct cvi_vpu_device *vdev, int enable, int mask)
{
	if (vdev->pdata->quirks & (VCODEC_QUIRK_SUPPORT_CLOCK_CONTROL | VCDOEC_QUIRK_SUPPORT_FPGA)) {
		if (enable) {
			if (vdev->pdata->ops->clk_enable)
				vdev->pdata->ops->clk_enable(vdev, mask);
		} else {
			if (vdev->pdata->ops->clk_disable)
				vdev->pdata->ops->clk_disable(vdev, mask);
		}
	}
}

#ifndef CVI_H26X_USE_ION_FW_BUFFER
static int vpu_alloc_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return -1;

	VCODEC_DBG_TRACE("size = 0x%X\n", vb->size);
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	vb->phys_addr = (unsigned long)vmem_alloc(&s_vmem, vb->size, 0);
	if ((unsigned long)vb->phys_addr == (unsigned long)-1) {
		VCODEC_DBG_ERR("Physical memory allocation error size=%d\n",
		       vb->size);
		return -1;
	}

	vb->base = (unsigned long)(s_video_memory.base +
				   (vb->phys_addr - s_video_memory.phys_addr));
#else
	vb->base = (unsigned long)dma_alloc_coherent(
		NULL, PAGE_ALIGN(vb->size), (dma_addr_t *)(&vb->phys_addr),
		GFP_DMA | GFP_KERNEL);
	if ((void *)(vb->base) == NULL) {
		VCODEC_DBG_ERR("Physical memory allocation error size=%d\n",
		       vb->size);
		return -1;
	}
#endif
	return 0;
}

static void vpu_free_dma_buffer(vpudrv_buffer_t *vb)
{
	if (!vb)
		return;

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (vb->base)
		vmem_free(&s_vmem, vb->phys_addr, 0);
#else
	if (vb->base)
		dma_free_coherent(0, PAGE_ALIGN(vb->size), (void *)vb->base,
				  vb->phys_addr);
#endif
}
#endif

#ifndef CVI_H26X_USE_ION_FW_BUFFER
static int vpu_free_buffers(struct file *filp)
{
	vpudrv_buffer_pool_t *pool, *n;
	vpudrv_buffer_t vb;

	list_for_each_entry_safe(pool, n, &s_vbp_head, list) {
		if (pool->filp == filp) {
			vb = pool->vb;
			if (vb.base) {
				vpu_free_dma_buffer(&vb);
				list_del(&pool->list);
				kfree(pool);
			}
		}
	}

	return 0;
}
#endif

static irqreturn_t vpu_irq_handler(int irq, void *dev_id)
{
	vpu_drv_context_t *dev = &vcodec_dev.s_vpu_drv_context;
	struct cvi_vcodec_context *pvctx = (struct cvi_vcodec_context *) dev_id;
	int coreIdx = -1;
	// (INT_BIT_PIC_RUN | INT_BIT_BIT_BUF_FULL)
	unsigned long bsMask = (1 << W4_INT_ENC_PIC) | (1 << W4_INT_BSBUF_EMPTY);

	/* this can be removed. it also work in VPU_WaitInterrupt of API function */
	int product_code;

	VCODEC_DBG_INTR("[+]%s\n", __func__);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
#else
	disable_irq_nosync(pvctx->s_vcodec_irq);
#endif

	if (pvctx->s_bit_firmware_info.size ==
			0) {
		/* it means that we didn't get an information		  */
		/* the current core from API layer. No core activated.*/
		VCODEC_DBG_ERR("s_bit_firmware_info.size is zero\n");
		return IRQ_HANDLED;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_ts64(&pvctx->irq_timestamp);
#else
	ktime_get_ts(&pvctx->irq_timestamp);
#endif

	product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

	VCODEC_DBG_TRACE("product_code = 0x%X\n", product_code);

	if (PRODUCT_CODE_W_SERIES(product_code)) {
		if (ReadVpuRegister(W4_VPU_VPU_INT_STS)) {
			pvctx->interrupt_reason =
				ReadVpuRegister(W4_VPU_INT_REASON);
			WriteVpuRegister(W4_VPU_INT_REASON_CLEAR,
					pvctx->interrupt_reason);
			WriteVpuRegister(W4_VPU_VINT_CLEAR, 0x1);
		}
		coreIdx = 0;
	} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
		if (ReadVpuRegister(BIT_INT_STS)) {
			pvctx->interrupt_reason =
				ReadVpuRegister(BIT_INT_REASON);
			WriteVpuRegister(BIT_INT_CLEAR, 0x1);
		}
		coreIdx = 1;
	} else {
		VCODEC_DBG_ERR("Unknown product id : %08x\n",
				product_code);
		return IRQ_HANDLED;
	}

	VCODEC_DBG_INTR("product: 0x%08x intr_reason: 0x%08lx\n",
			product_code, pvctx->interrupt_reason);
	VCODEC_DBG_INTR("intr_reason: 0x%lX\n",
			pvctx->interrupt_reason);

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO,
				POLL_IN); /* notify the interrupt to user space */

	pvctx->s_interrupt_flag = 1;

	if ((pvctx->interrupt_reason & bsMask)) {
		if (chnIdxMapping[coreIdx] != -1) {
			//wake_up(&tWaitQueue[chnIdxMapping[coreIdx]]);
		}
	}
	wake_up(&pvctx->s_interrupt_wait_q);
	VCODEC_DBG_INTR("[-]%s\n", __func__);

	return IRQ_HANDLED;
}

void cvi_VENC_SBM_IrqEnable(void)
{
	unsigned int reg;

	// sbm interrupt enable (1: enable, 0: disable/clear)
	reg = ReadSbmRegister(0x08);
	// [13]: push0
	reg |= (0x1 << 13);
	WritSbmRegister(0x08, reg);
	VCODEC_DBG_TRACE("%s %d reg_08:0x%x\n", __func__, __LINE__, reg);
}
EXPORT_SYMBOL(cvi_VENC_SBM_IrqEnable);

void cvi_VENC_SBM_IrqDisable(void)
{
	unsigned int reg;

	// printk("%s\n", __FUNCTION__);
	// sbm interrupt enable (1: enable, 0: disable/clear)
	reg = ReadSbmRegister(0x08);
	// [13]: push0
	reg &= ~(0x1 << 13);
	WritSbmRegister(0x08, reg);
	VCODEC_DBG_TRACE("%s %d reg_08:0x%x\n", __func__, __LINE__, reg);
}
EXPORT_SYMBOL(cvi_VENC_SBM_IrqDisable);

static irqreturn_t sbm_irq_handler(int irq, void *dev_id)
{
	struct cvi_vcodec_context *pvctx = NULL;

	pvctx = &vcodec_dev.vcodec_ctx[0];

	cvi_VENC_SBM_IrqDisable();

	pvctx->s_sbm_interrupt_flag = 1;
	wake_up(&pvctx->s_sbm_interrupt_wait_q);

	return IRQ_HANDLED;
}

void wake_sbm_waitinng(void)
{
	struct cvi_vcodec_context *pvctx = NULL;

	pvctx = &vcodec_dev.vcodec_ctx[0];

	pvctx->s_sbm_interrupt_flag = 1;
	wake_up(&pvctx->s_sbm_interrupt_wait_q);
}
EXPORT_SYMBOL(wake_sbm_waitinng);

int sbm_wait_interrupt(int timeout)
{
	int ret = 0;
	struct cvi_vcodec_context *pvctx = NULL;

	pvctx = &vcodec_dev.vcodec_ctx[0];

	ret = wait_event_timeout(
		pvctx->s_sbm_interrupt_wait_q, pvctx->s_sbm_interrupt_flag != 0, msecs_to_jiffies(timeout));
	if (!ret) {
		ret = -1;
		return ret;
	}

	pvctx->s_sbm_interrupt_flag = 0;

	return ret;
}
EXPORT_SYMBOL(sbm_wait_interrupt);

// VDI_IOCTL_WAIT_INTERRUPT
int vpu_wait_interrupt(vpudrv_intr_info_t *p_intr_info)
{
	int ret = 0;
	vpudrv_intr_info_t info;
	struct cvi_vcodec_context *pvctx = NULL;

	memcpy(&info, p_intr_info, sizeof(vpudrv_intr_info_t));

	if (ret != 0 || info.coreIdx < 0 || info.coreIdx >= MAX_NUM_VPU_CORE)
		return -EFAULT;

	pvctx = &vcodec_dev.vcodec_ctx[info.coreIdx];

	VCODEC_DBG_TRACE("coreIdx = %d, s_interrupt_flag = 0x%X\n",
			info.coreIdx, pvctx->s_interrupt_flag);

	ret = wait_event_timeout(
		pvctx->s_interrupt_wait_q, pvctx->s_interrupt_flag != 0,
		msecs_to_jiffies(info.timeout));
	if (!ret) {
		ret = -ETIME;
		return ret;
	}

	ANNOTATE_CHANNEL_COLOR(1, ANNOTATE_GREEN, "vcodec end");
	if (signal_pending(current)) {
		ret = -ERESTARTSYS;
		return ret;
	}

	VCODEC_DBG_INTR("s_interrupt_flag(%d), reason(0x%08lx)\n",
		pvctx->s_interrupt_flag, pvctx->interrupt_reason);

	info.intr_reason = pvctx->interrupt_reason;
	info.intr_tv_sec = pvctx->irq_timestamp.tv_sec;
	info.intr_tv_nsec = pvctx->irq_timestamp.tv_nsec;
	pvctx->s_interrupt_flag = 0;
	pvctx->interrupt_reason = 0;

	memcpy(p_intr_info, &info, sizeof(vpudrv_intr_info_t));
	ret = 0;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
#else
	enable_irq(pvctx->s_vcodec_irq);
#endif

	ANNOTATE_CHANNEL_END(1);
	ANNOTATE_NAME_CHANNEL(1, 1, "vcodec end");

	return ret;
}
EXPORT_SYMBOL(vpu_wait_interrupt);

// VDI_IOCTL_SET_CLOCK_GATE_EXT
int vpu_set_clock_gate_ext(struct clk_ctrl_info *p_info)
{
	int ret = 0;
	struct clk_ctrl_info info;
#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
#else
	struct cvi_vpu_device *vdev = filp->private_data;
#endif

	memcpy(&info, p_info, sizeof(struct clk_ctrl_info));
	if (ret != 0 || info.core_idx < 0 || info.core_idx >= MAX_NUM_VPU_CORE)
		return -EFAULT;

	VCODEC_DBG_INFO("vdev %p\n", vdev);
	VCODEC_DBG_INFO("vdev->pdata->quirks %d\n", vdev->pdata->quirks);

	set_clock_enable(vdev, info.enable, 1 << info.core_idx);
	return ret;
}
EXPORT_SYMBOL(vpu_set_clock_gate_ext);

// VDI_IOCTL_GET_INSTANCE_POOL
int vpu_get_instance_pool(vpudrv_buffer_t *p_vdb)
{
	int ret = 0;

	ret = cviIoctlGetInstPool((u_long) p_vdb);

	return ret;
}
EXPORT_SYMBOL(vpu_get_instance_pool);

#ifndef CVI_H26X_USE_ION_FW_BUFFER
// VDI_IOCTL_GET_COMMON_MEMORY
int vpu_get_common_memory(vpudrv_buffer_t *p_vdb)
{
	int ret = 0;

	if (vcodec_dev.s_common_memory.base != 0) {
		memcpy(p_vdb, &vcodec_dev.s_common_memory, sizeof(vpudrv_buffer_t));
	} else {
		memcpy(&vcodec_dev.s_common_memory, p_vdb, sizeof(vpudrv_buffer_t));

		if (vpu_alloc_dma_buffer(&vcodec_dev.s_common_memory) != -1) {
			memcpy(p_vdb, &vcodec_dev.s_common_memory, sizeof(vpudrv_buffer_t));
		}
	}
	return ret;
}
EXPORT_SYMBOL(vpu_get_common_memory);
#endif

// VDI_IOCTL_OPEN_INSTANCE
int vpu_open_instance(vpudrv_inst_info_t *p_inst_info)
{
	int ret = 0;
	vpudrv_inst_info_t inst_info;
	vpudrv_instanace_list_t *vil, *n;

	vil = kzalloc(sizeof(*vil), GFP_KERNEL);
	if (!vil)
		return -ENOMEM;

	memcpy(&inst_info, p_inst_info, sizeof(vpudrv_inst_info_t));

	vil->inst_idx = inst_info.inst_idx;
	vil->core_idx = inst_info.core_idx;
	//vil->filp = filp;

	spin_lock(&s_vpu_lock);
	list_add(&vil->list, &s_inst_list_head);

	inst_info.inst_open_count =
		0; /* counting the current open instance number */
	list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
		if (vil->core_idx == inst_info.core_idx)
			inst_info.inst_open_count++;
	}
	spin_unlock(&s_vpu_lock);

	s_vpu_open_ref_count++; /* flag just for that vpu is in opened or closed */

	memcpy(p_inst_info, &inst_info, sizeof(vpudrv_inst_info_t));

	VCODEC_DBG_TRACE("core_idx=%d, inst_idx=%d, ref cnt=%d, inst cnt=%d\n",
		(int)inst_info.core_idx, (int)inst_info.inst_idx,
		s_vpu_open_ref_count, inst_info.inst_open_count);
	return ret;
}
EXPORT_SYMBOL(vpu_open_instance);

// VDI_IOCTL_CLOSE_INSTANCE
int vpu_close_instance(vpudrv_inst_info_t *p_inst_info)
{
	int ret = 0;
	vpudrv_inst_info_t inst_info;
	vpudrv_instanace_list_t *vil, *n;

	memcpy(&inst_info, p_inst_info, sizeof(vpudrv_inst_info_t));

	spin_lock(&s_vpu_lock);
	list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
		if (vil->inst_idx == inst_info.inst_idx &&
		    vil->core_idx == inst_info.core_idx) {
			list_del(&vil->list);
			kfree(vil);
			break;
		}
	}

	inst_info.inst_open_count =
		0; /* counting the current open instance number */
	list_for_each_entry_safe(vil, n, &s_inst_list_head, list) {
		if (vil->core_idx == inst_info.core_idx)
			inst_info.inst_open_count++;
	}
	spin_unlock(&s_vpu_lock);

	s_vpu_open_ref_count--; /* flag just for that vpu is in opened or closed */

	memcpy(p_inst_info, &inst_info, sizeof(vpudrv_inst_info_t));

	VCODEC_DBG_TRACE("core_idx=%d, inst_idx=%d, ref cnt=%d, inst cnt=%d\n",
		(int)inst_info.core_idx, (int)inst_info.inst_idx,
		s_vpu_open_ref_count, inst_info.inst_open_count);

	return ret;
}
EXPORT_SYMBOL(vpu_close_instance);

// VDI_IOCTL_RESET
int vpu_reset(void)
{
	int ret = 0;

	vpu_hw_reset();

	return ret;
}
EXPORT_SYMBOL(vpu_reset);

// VDI_IOCTL_GET_REGISTER_INFO
int vpu_get_register_info(vpudrv_buffer_t *p_vdb_register)
{
	int ret = 0;
	vpudrv_buffer_t info;
	int core_idx;
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx = NULL;

	memcpy(&info, p_vdb_register, sizeof(vpudrv_buffer_t));

	core_idx = info.size;
	pvctx = &vcodec_dev.vcodec_ctx[core_idx];
	vdev->pvctx = pvctx;

	VCODEC_DBG_TRACE("core_idx = %d\n", core_idx);

	memcpy(p_vdb_register, &pvctx->s_vpu_register, sizeof(vpudrv_buffer_t));

	VCODEC_DBG_TRACE("[-]pa=0x%llx, va=0x%lx, size=%d\n",
		pvctx->s_vpu_register.phys_addr,
		(unsigned long int)pvctx->s_vpu_register.virt_addr,
		pvctx->s_vpu_register.size);

	return ret;
}
EXPORT_SYMBOL(vpu_get_register_info);

// VDI_IOCTL_GET_CHIP_VERSION
int vpu_get_chip_version(unsigned int *p_chip_version)
{
	int ret = 0;
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = filp->private_data;
	#endif

	memcpy(p_chip_version, &vdev->pdata->version, sizeof(unsigned int));

	VCODEC_DBG_TRACE("VDI_IOCTL_GET_CHIP_VERSION chip_ver=0x%x\n", vdev->pdata->version);
	return ret;
}
EXPORT_SYMBOL(vpu_get_chip_version);

// VDI_IOCTL_GET_CHIP_CAP
int vpu_get_chip_cabability(unsigned int *p_chip_capability)
{
	int ret = 0;
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = filp->private_data;
	#endif

	memcpy(p_chip_capability, &vdev->pdata->quirks, sizeof(unsigned int));

	VCODEC_DBG_TRACE("VDI_IOCTL_GET_CHIP_CAP chip_cap=0x%x\n", vdev->pdata->quirks);
	return ret;
}
EXPORT_SYMBOL(vpu_get_chip_cabability);

// VDI_IOCTL_GET_CLOCK_FREQUENCY
int vpu_get_clock_frequency(unsigned long *p_clk_rate)
{
	int ret = 0;
	unsigned long clk_rate = 0;
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = filp->private_data;
	#endif

	if (vdev->pdata->ops->clk_get_rate)
		clk_rate = vdev->pdata->ops->clk_get_rate(vdev);

	VCODEC_DBG_TRACE("VDI_IOCTL_GET_CLOCK_FREQENCY %lu\n", clk_rate);

	memcpy(p_clk_rate, &clk_rate, sizeof(unsigned long));

	return ret;
}
EXPORT_SYMBOL(vpu_get_clock_frequency);

#ifndef CVI_H26X_USE_ION_FW_BUFFER
// VDI_IOCTL_RELEASE_COMMON_MEMORY
int vpu_release_common_memory(vpudrv_buffer_t *p_vdb)
{
	int ret = 0;
	vpudrv_buffer_t vb;

	VCODEC_DBG_INFO("[+]VDI_IOCTL_RELEASE_COMMON_MEMORY\n");

	ret = down_interruptible(&s_vpu_sem);
	if (ret == 0) {
		ret = copy_from_user(&vb, (vpudrv_buffer_t *)p_vdb,
					 sizeof(vpudrv_buffer_t));
		if (ret) {
			up(&s_vpu_sem);
			return -EACCES;
		}

		if (vcodec_dev.s_common_memory.base == vb.base) {
			vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
			vcodec_dev.s_common_memory.base = 0;
		} else {
			VCODEC_DBG_ERR("common memory addr mismatch, driver: 0x%llx user: 0x%llx\n",
				vcodec_dev.s_common_memory.base, vb.base);
			ret = -EFAULT;
		}
		up(&s_vpu_sem);
	}
	VCODEC_DBG_INFO("[-]VDI_IOCTL_RELEASE_COMMON_MEMORY\n");

	return ret;
}
EXPORT_SYMBOL(vpu_release_common_memory);
#endif

// VDI_IOCTL_GET_SINGLE_CORE_CONFIG
int vpu_get_single_core_config(int *pSingleCoreConfig)
{
	int ret = 0;

	VCODEC_DBG_INFO("[+]VDI_IOCTL_GET_SINGLE_CORE_CONFIG\n");

	memcpy(pSingleCoreConfig, &bSingleCore, sizeof(int));

	VCODEC_DBG_INFO("[-]VDI_IOCTL_GET_SINGLE_CORE_CONFIG\n");

	return ret;
}
EXPORT_SYMBOL(vpu_get_single_core_config);

int vpu_op_write(vpu_bit_firmware_info_t *p_bit_firmware_info, size_t len)
{
	VCODEC_DBG_TRACE("vpu_write len=%d\n", (int)len);

	if (!p_bit_firmware_info) {
		VCODEC_DBG_ERR("vpu_write buf = NULL error\n");
		return -EFAULT;
	}

	if (len == sizeof(vpu_bit_firmware_info_t)) {
		vpu_bit_firmware_info_t *bit_firmware_info;
		struct cvi_vcodec_context *pvctx;

		bit_firmware_info =
			vmalloc(sizeof(vpu_bit_firmware_info_t));
		if (!bit_firmware_info) {
			VCODEC_DBG_ERR("bit_firmware_info allocation error\n");
			return -EFAULT;
		}

		memcpy(bit_firmware_info, p_bit_firmware_info, len);

		VCODEC_DBG_INFO("bit_firmware_info->size=%d %d\n",
				bit_firmware_info->size,
				(int)sizeof(vpu_bit_firmware_info_t));

		if (bit_firmware_info->size ==
		    sizeof(vpu_bit_firmware_info_t)) {
			VCODEC_DBG_INFO("bit_firmware_info size\n");

			if (bit_firmware_info->core_idx > MAX_NUM_VPU_CORE) {
				VCODEC_DBG_ERR("core_idx[%d] > MAX_NUM_VPU_CORE[%d]\n",
				       bit_firmware_info->core_idx,
				       MAX_NUM_VPU_CORE);
				return -ENODEV;
			}

			pvctx = &vcodec_dev.vcodec_ctx[bit_firmware_info->core_idx];
			memcpy((void *)&pvctx->s_bit_firmware_info,
			       bit_firmware_info,
			       sizeof(vpu_bit_firmware_info_t));
			vfree(bit_firmware_info);

			return len;
		}

		vfree(bit_firmware_info);
	}

	return -1;
}
EXPORT_SYMBOL(vpu_op_write);

void vpu_set_channel_core_mapping(int chnIdx, int coreIdx)
{
	coreIdxMapping[chnIdx] = coreIdx;
	if (coreIdx >= 0) {
		chnIdxMapping[coreIdx] = chnIdx;
	} else {
		if (chnIdxMapping[0] == chnIdx)
			chnIdxMapping[0] = -1;
		else if (chnIdxMapping[1] == chnIdx)
			chnIdxMapping[1] = -1;
	}
}
EXPORT_SYMBOL(vpu_set_channel_core_mapping);

unsigned long vpu_get_interrupt_reason(int chnIdx)
{
	struct cvi_vcodec_context *pvctx;

	if (coreIdxMapping[chnIdx] == -1)
		return 0;

	pvctx = &vcodec_dev.vcodec_ctx[coreIdxMapping[chnIdx]];

	return pvctx->interrupt_reason;
}
EXPORT_SYMBOL(vpu_get_interrupt_reason);

static int cviIoctlGetInstPool(u_long arg)
{
	int ret = 0;

	ret = down_interruptible(&s_vpu_sem);
	if (ret == 0) {
		if (vcodec_dev.s_instance_pool.base != 0) {
			memcpy((void *)arg, &vcodec_dev.s_instance_pool, sizeof(vpudrv_buffer_t));
		} else {
			memcpy(&vcodec_dev.s_instance_pool, (vpudrv_buffer_t *)arg, sizeof(vpudrv_buffer_t));
			if (ret == 0) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
				vcodec_dev.s_instance_pool.size = PAGE_ALIGN(
						vcodec_dev.s_instance_pool.size);
				vcodec_dev.s_instance_pool.base =
					(unsigned long)vmalloc(
							vcodec_dev.s_instance_pool.size);
				vcodec_dev.s_instance_pool.phys_addr =
					vcodec_dev.s_instance_pool.base;

				if (vcodec_dev.s_instance_pool.base != 0)
#else
				if (vpu_alloc_dma_buffer(
							&vcodec_dev.s_instance_pool) != -1)
#endif
				{
					memset((void *)(uintptr_t)vcodec_dev.s_instance_pool.base,
							0x0,
							vcodec_dev.s_instance_pool.size);
					memcpy((void *)arg, &vcodec_dev.s_instance_pool, sizeof(vpudrv_buffer_t));
					if (ret == 0) {
						/* success to get memory for instance pool */
						up(&s_vpu_sem);
						return ret;
					}
				}
			}
			ret = -EFAULT;
		}

		up(&s_vpu_sem);
	}

	return ret;
}

#ifndef USE_KERNEL_MODE
static ssize_t vpu_write(struct file *filp, const char __user *buf, size_t len,
			 loff_t *ppos)
{
	VCODEC_DBG_TRACE("vpu_write len=%d\n", (int)len);

	if (!buf) {
		VCODEC_DBG_ERR("vpu_write buf = NULL error\n");
		return -EFAULT;
	}

	if (len == sizeof(vpu_bit_firmware_info_t)) {
		vpu_bit_firmware_info_t *bit_firmware_info;
		struct cvi_vcodec_context *pvctx;

		bit_firmware_info =
			kmalloc(sizeof(vpu_bit_firmware_info_t), GFP_KERNEL);
		if (!bit_firmware_info) {
			VCODEC_DBG_ERR("bit_firmware_info allocation error\n");
			return -EFAULT;
		}

		if (copy_from_user(bit_firmware_info, buf, len)) {
			VCODEC_DBG_ERR("copy_from_user error\n");
			return -EFAULT;
		}

		VCODEC_DBG_INFO("bit_firmware_info->size=%d %d\n",
				bit_firmware_info->size,
				(int)sizeof(vpu_bit_firmware_info_t));

		if (bit_firmware_info->size ==
		    sizeof(vpu_bit_firmware_info_t)) {
			VCODEC_DBG_INFO("bit_firmware_info size\n");

			if (bit_firmware_info->core_idx > MAX_NUM_VPU_CORE) {
				VCODEC_DBG_ERR("core_idx[%d] > MAX_NUM_VPU_CORE[%d]\n",
				       bit_firmware_info->core_idx,
				       MAX_NUM_VPU_CORE);
				return -ENODEV;
			}

			pvctx = &vcodec_dev.vcodec_ctx[bit_firmware_info->core_idx];
			memcpy((void *)&pvctx->s_bit_firmware_info,
			       bit_firmware_info,
			       sizeof(vpu_bit_firmware_info_t));
			kfree(bit_firmware_info);

			return len;
		}

		kfree(bit_firmware_info);
	}

	return -1;
}

static int vpu_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	ret = down_interruptible(&s_vpu_sem);
	if (ret == 0) {
		/* found and free the not handled buffer by user applications */
		#ifndef CVI_H26X_USE_ION_FW_BUFFER
		vpu_free_buffers(filp);
		#endif

		/* found and free the not closed instance by user applications */
		vpu_free_instances(filp);
		vcodec_dev.s_vpu_drv_context.open_count--;
		if (vcodec_dev.s_vpu_drv_context.open_count == 0) {
			if (vcodec_dev.s_instance_pool.base) {
				VCODEC_DBG_INFO("free instance pool\n");
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
				vfree((const void *)vcodec_dev.s_instance_pool.base);
#else
				vpu_free_dma_buffer(&vcodec_dev.s_instance_pool);
#endif
				vcodec_dev.s_instance_pool.base = 0;
			}

			#ifndef CVI_H26X_USE_ION_FW_BUFFER
			if (vcodec_dev.s_common_memory.base) {
				VCODEC_DBG_INFO("free common memory\n");
				vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
				vcodec_dev.s_common_memory.base = 0;
			}
			#endif
		}
	}
	up(&s_vpu_sem);

	return 0;
}

static int vpu_fasync(int fd, struct file *filp, int mode)
{
	struct vpu_drv_context_t *dev =
		(struct vpu_drv_context_t *)&vcodec_dev.s_vpu_drv_context;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int vpu_map_to_register(struct file *filp, struct vm_area_struct *vm)
{
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx =
		(struct cvi_vcodec_context *) vdev->pvctx;

	unsigned long pfn;

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = pvctx->s_vpu_register.phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int vpu_map_to_physical_memory(struct file *flip,
				      struct vm_area_struct *vm)
{
	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
}

static int vpu_map_to_instance_pool_memory(struct file *filp,
					   struct vm_area_struct *vm)
{
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	int ret;
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)vcodec_dev.s_instance_pool.base;
	unsigned long pfn;

	vm->vm_flags |= VM_RESERVED;

	/* loop over all pages, map it page individually */
	while (length > 0) {
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		ret = remap_pfn_range(vm, start, pfn, PAGE_SIZE, PAGE_SHARED);
		if (ret < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}

	return 0;
#else
	vm->vm_flags |= VM_RESERVED;
	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ?
		       -EAGAIN :
		       0;
#endif
}

/*!
 * @brief memory map interface for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_mmap(struct file *filp, struct vm_area_struct *vm)
{
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)pCviVpuDevice;
	#else
	struct cvi_vpu_device *vdev = (struct cvi_vpu_device *)filp->private_data;
	#endif
	struct cvi_vcodec_context *pvctx =
		(struct cvi_vcodec_context *) vdev->pvctx;

	if (vm->vm_pgoff == 0)
		return vpu_map_to_instance_pool_memory(filp, vm);

	if (vm->vm_pgoff == (pvctx->s_vpu_register.phys_addr >> PAGE_SHIFT))
		return vpu_map_to_register(filp, vm);

	return vpu_map_to_physical_memory(filp, vm);
#else
	if (vm->vm_pgoff) {
		if (vm->vm_pgoff == (vcodec_dev.s_instance_pool.phys_addr >> PAGE_SHIFT))
			return vpu_map_to_instance_pool_memory(filp, vm);

		return vpu_map_to_physical_memory(filp, vm);
	} else {
		return vpu_map_to_register(filp, vm);
	}
#endif
}

const struct file_operations vpu_fops = {
	.owner = THIS_MODULE,
	.open = vpu_open,
	.write = vpu_write,
	/*.ioctl = vpu_ioctl, // for kernel 2.6.9 of C&M*/
	.unlocked_ioctl = vpu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vpu_ioctl,
#endif
	.release = vpu_release,
	.fasync = vpu_fasync,
	.mmap = vpu_mmap,
};
#endif

static int vpu_probe(struct platform_device *pdev)
{
	int err = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cvi_vpu_device *vdev;

	vdev = devm_kzalloc(&pdev->dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev)
		return -ENOMEM;

	memset(vdev, 0, sizeof(*vdev));
	#ifdef VPU_SUPPORT_GLOBAL_DEVICE_CONTEXT
	pCviVpuDevice = vdev;
	#endif

	vdev->dev = dev;

	match = of_match_device(cvi_vpu_match_table, &pdev->dev);
	if (!match)
		return -EINVAL;

	vdev->pdata = match->data;

	VCODEC_DBG_INFO("pdata version 0x%x quirks 0x%x\n", vdev->pdata->version, vdev->pdata->quirks);

	err = cviGetRegResource(vdev, pdev);
	if (err) {
		VCODEC_DBG_ERR("cviGetRegResource\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_REMAP_DDR)
		cviConfigDDR(vdev);

	err = cvi_vcodec_register_cdev(vdev);
	if (err < 0) {
		VCODEC_DBG_ERR("cvi_vcodec_register_cdev\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vdev->pdata->ops && vdev->pdata->ops->clk_get)
		vdev->pdata->ops->clk_get(vdev);

	err = cviCfgIrq(pdev);
	if (err) {
		VCODEC_DBG_ERR("cviCfgIrq\n");
		goto ERROR_PROBE_DEVICE;
	}

#ifndef CVI_H26X_USE_ION_FW_BUFFER
#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (cvi_vcodec_allocate_memory(pdev) < 0) {
		VCODEC_DBG_ERR("fail to remap\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (!s_video_memory.base) {
		VCODEC_DBG_ERR("fail to remap\n");
		goto ERROR_PROBE_DEVICE;
	}

	if (vmem_init(&s_vmem, s_video_memory.phys_addr, s_video_memory.size) <
	    0) {
		VCODEC_DBG_ERR(":  fail to init vmem system\n");
		goto ERROR_PROBE_DEVICE;
	}
	VCODEC_DBG_INFO("success to probe, pa = 0x%llx, base = 0x%llx\n",
		s_video_memory.phys_addr, s_video_memory.base);
#else
	VCODEC_DBG_INFO("success to probe\n");
#endif
#endif

	platform_set_drvdata(pdev, vdev);

	return 0;

ERROR_PROBE_DEVICE:

	if (vdev->s_vpu_major)
		unregister_chrdev_region(vdev->cdev_id, 1);

	cviReleaseRegResource(vdev);

	platform_set_drvdata(pdev, &vcodec_dev);

	return err;
}

static int cviGetRegResource(struct cvi_vpu_device *vdev, struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	struct resource *res = NULL;
	int idx;

	vpudrv_buffer_t *pReg;

	if (!pdev) {
		VCODEC_DBG_ERR("pdev = NULL\n");
		return -1;
	}

	for (idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
		pvctx = &vcodec_dev.vcodec_ctx[idx];

		res = platform_get_resource(pdev, IORESOURCE_MEM, idx);
		if (res) {
			pReg = &pvctx->s_vpu_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("idx = %d, reg base, pa = 0x%llX, va = 0x%p\n",
				     idx, pReg->phys_addr, pReg->virt_addr);
		} else
			return -ENXIO;
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_CTRL_REG) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (res) {
			pReg = &vcodec_dev.ctrl_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc ctrl register, reg base, pa = 0x%llX, va = %p, size = 0x%X\n",
				     pReg->phys_addr, pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_ADDR_REMAP) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_addr_remap");
		VCODEC_DBG_INFO("platform_get_resource_byname vc_addr_remap, res = %p\n", res);
		if (res) {
			pReg = &vcodec_dev.remap_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc_addr_remap reg base, pa = 0x%llX, va = %p, size = 0x%X\n",
					 pReg->phys_addr, pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_SBM) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_sbm");
		VCODEC_DBG_INFO("platform_get_resource_byname vc_sbm, res = %p\n", res);
		if (res) {
			pReg = &vcodec_dev.sbm_register;
			pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			pReg->virt_addr = (__u8 *)ioremap(
					res->start, res->end - res->start);
#else
			pReg->virt_addr = (__u8 *)ioremap_nocache(
					res->start, res->end - res->start);
#endif
			pReg->size = res->end - res->start;
			VCODEC_DBG_INFO("vc_sbm reg base, pa = 0x%llX, va = %p, size = 0x%X\n",
					 pReg->phys_addr, pReg->virt_addr, pReg->size);
		} else {
			return -ENXIO;
		}
	}

	return 0;
}

static int cvi_vcodec_register_cdev(struct cvi_vpu_device *vdev)
{
	int err = 0;

	vdev->vpu_class = class_create(THIS_MODULE, VPU_CLASS_NAME);
	if (IS_ERR(vdev->vpu_class)) {
		VCODEC_DBG_ERR("create class failed\n");
		return PTR_ERR(vdev->vpu_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&vdev->cdev_id, 0, 1, VPU_DEV_NAME)) < 0) {
		err = -EBUSY;
		VCODEC_DBG_ERR("could not allocate major number\n");
		return err;
	}
	vdev->s_vpu_major = MAJOR(vdev->cdev_id);
	VCODEC_DBG_INFO("SUCCESS alloc_chrdev_region major %d\n", vdev->s_vpu_major);

	return err;
}

static int cviCfgIrq(struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	static const char * const irq_name[] = {"h265", "h264", "sbm"};
	int core, err;

	if (!pdev) {
		VCODEC_DBG_ERR("pdev = NULL\n");
		return -1;
	}

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];

		pvctx->s_vcodec_irq = platform_get_irq_byname(pdev, irq_name[core]);

		if (pvctx->s_vcodec_irq < 0) {
			VCODEC_DBG_ERR("No IRQ resource for %s\n", irq_name[core]);
			return -ENODEV;
		}

		VCODEC_DBG_INFO("core = %d, s_vcodec_irq = %d\n",
				core, pvctx->s_vcodec_irq);

		err = request_irq(pvctx->s_vcodec_irq, vpu_irq_handler, 0, irq_name[core],
				(void *)pvctx);
		if (err) {
			VCODEC_DBG_ERR("fail to register interrupt handler\n");
			return -1;
		}
	}

	pr_info("[INFO] Register SBM IRQ ###################################");

	pvctx->s_sbm_irq = platform_get_irq_byname(pdev, irq_name[2]);
	if (pvctx->s_sbm_irq < 0) {
		VCODEC_DBG_ERR("No IRQ resource for %s\n", irq_name[2]);
		return -ENODEV;
	}

	pr_info("[INFO] pvctx->s_sbm_irq = %d", pvctx->s_sbm_irq);

	err = request_irq(pvctx->s_sbm_irq, sbm_irq_handler, 0, irq_name[2],
			(void *)&vcodec_dev);
	if (err) {
		VCODEC_DBG_ERR("fail to register interrupt handler\n");
		return -1;
	}

	return 0;
}

static void cviFreeIrq(void)
{
	struct cvi_vcodec_context *pvctx;
	int core = 0;

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];

		VCODEC_DBG_INFO("core = %d, s_vcodec_irq = %d\n", core, pvctx->s_vcodec_irq);
		free_irq(pvctx->s_vcodec_irq, (void *)pvctx);
	}
}

static void cviReleaseRegResource(struct cvi_vpu_device *vdev)
{
	int idx;
	vpudrv_buffer_t *pReg;

	for (idx = 0; idx < MAX_NUM_VPU_CORE; idx++) {
		pReg = &vcodec_dev.vcodec_ctx[idx].s_vpu_register;
		cviUnmapReg(pReg);
	}

	if (vdev->pdata->quirks & VCODEC_QUIRK_SUPPORT_VC_CTRL_REG) {
		pReg = &vcodec_dev.ctrl_register;
		cviUnmapReg(pReg);
	}
}

static void cviUnmapReg(vpudrv_buffer_t *pReg)
{
	if (pReg->virt_addr) {
		iounmap((void *)pReg->virt_addr);
		pReg->virt_addr = NULL;
	}
}

#ifndef CVI_H26X_USE_ION_FW_BUFFER
static int cvi_vcodec_allocate_memory(struct platform_device *pdev)
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
			VCODEC_DBG_ERR(": cannot acquire memory-region\n");
			return -1;
		}
	} else {
		VCODEC_DBG_ERR(": cannot find the node, memory-region\n");
		return -1;
	}

	VCODEC_DBG_INFO("pool name = %s, size = 0x%llx, base = 0x%llx\n",
	       prmem->name, prmem->size, prmem->base);

	s_video_memory.phys_addr = (unsigned long)prmem->base;
	s_video_memory.size = (unsigned int)prmem->size;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	s_video_memory.base = (unsigned long)devm_ioremap(
		&pdev->dev, s_video_memory.phys_addr, s_video_memory.size);
#else
	s_video_memory.base = (unsigned long)devm_ioremap_nocache(
		&pdev->dev, s_video_memory.phys_addr, s_video_memory.size);
#endif

	if (bSingleCore && s_video_memory.size >= (MAX_NUM_VPU_CORE * SIZE_COMMON)) {
		VCODEC_DBG_WARN("using singleCore but with 2 core reserved mem!\n");
	}
	if (!bSingleCore && s_video_memory.size < (MAX_NUM_VPU_CORE * SIZE_COMMON)) {
		VCODEC_DBG_ERR("not enough reserved memory for VPU\n");
		return -1;
	}

	if (!s_video_memory.base) {
		VCODEC_DBG_ERR("ioremap fail!\n");
		VCODEC_DBG_ERR("s_video_memory.base = 0x%llx\n", s_video_memory.base);
		return -1;
	}

	VCODEC_DBG_INFO("pa = 0x%llx, base = 0x%llx, size = 0x%x\n",
		s_video_memory.phys_addr, s_video_memory.base,
		s_video_memory.size);
	VCODEC_DBG_INFO("success to probe vcodec\n");

	return 0;
}
#endif

static int vpu_remove(struct platform_device *pdev)
{
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	if (vcodec_dev.s_instance_pool.base) {
#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		vfree((const void *)(uintptr_t)vcodec_dev.s_instance_pool.base);
#else
		vpu_free_dma_buffer(&vcodec_dev.s_instance_pool);
#endif
		vcodec_dev.s_instance_pool.base = (uintptr_t)0;
	}

#ifndef CVI_H26X_USE_ION_FW_BUFFER
	if (vcodec_dev.s_common_memory.base) {
		vpu_free_dma_buffer(&vcodec_dev.s_common_memory);
		vcodec_dev.s_common_memory.base = 0;
	}

#ifdef VPU_SUPPORT_RESERVED_VIDEO_MEMORY
	if (s_video_memory.base) {
		s_video_memory.base = 0;
		vmem_exit(&s_vmem);
	}
#endif
#endif

	if (vdev->s_vpu_major > 0) {
		VCODEC_DBG_INFO("vdev %p vdev->cdev %p\n", vdev, &vdev->cdev);
		device_destroy(vdev->vpu_class, vdev->cdev_id);
		class_destroy(vdev->vpu_class);
		cdev_del(&vdev->cdev);
		unregister_chrdev_region(vdev->cdev_id, 1);
		vdev->s_vpu_major = 0;
	}

	cviFreeIrq();

	cviReleaseRegResource(vdev);

	return 0;
}

#ifdef CONFIG_PM
#define W4_MAX_CODE_BUF_SIZE (512 * 1024)
#define W4_CMD_INIT_VPU (0x0001)
#define W4_CMD_SLEEP_VPU (0x0400)
#define W4_CMD_WAKEUP_VPU (0x0800)
#define W5_CMD_SLEEP_VPU (0x0004)
#define W5_CMD_WAKEUP_VPU (0x0002)

static void Wave4BitIssueCommand(int core, u32 cmd)
{
	struct cvi_vcodec_context *pvctx = &vcodec_dev.vcodec_ctx[core];

	WriteVpuRegister(W4_VPU_BUSY_STATUS, 1);
	WriteVpuRegister(W4_CORE_INDEX, 0);
	/*	coreIdx = ReadVpuRegister(W4_VPU_BUSY_STATUS);*/
	/*	coreIdx = 0;*/
	/*	WriteVpuRegister(W4_INST_INDEX,  (instanceIndex&0xffff)|(codecMode<<16));*/
	WriteVpuRegister(W4_COMMAND, cmd);
	WriteVpuRegister(W4_VPU_HOST_INT_REQ, 1);
}

static int vpu_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct cvi_vcodec_context *pvctx;
	int i;
	int core;
	unsigned long timeout = jiffies + HZ; /* vpu wait timeout to 1sec */
	int product_code;
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	if (s_vpu_open_ref_count > 0) {
		for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
			pvctx = &vcodec_dev.vcodec_ctx[core];
			if (pvctx->s_bit_firmware_info.size == 0)
				continue;
			product_code =
				ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);

			if (PRODUCT_CODE_W_SERIES(product_code)) {
				unsigned long cmd_reg = W4_CMD_SLEEP_VPU;
				unsigned long suc_reg = W4_RET_SUCCESS;

				while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
					if (time_after(jiffies, timeout)) {
						VCODEC_DBG_ERR("SLEEP_VPU BUSY timeout");
						goto DONE_SUSPEND;
					}
				}

				if (product_code == WAVE512_CODE ||
				    product_code == WAVE520_CODE) {
					cmd_reg = W5_CMD_SLEEP_VPU;
					suc_reg = W5_RET_SUCCESS;
				}
				Wave4BitIssueCommand(core, cmd_reg);

				while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
					if (time_after(jiffies, timeout)) {
						VCODEC_DBG_ERR("SLEEP_VPU BUSY timeout");
						goto DONE_SUSPEND;
					}
				}
				if (ReadVpuRegister(suc_reg) == 0) {
					VCODEC_DBG_ERR("SLEEP_VPU failed [0x%x]",
						ReadVpuRegister(
							W4_RET_FAIL_REASON));
					goto DONE_SUSPEND;
				}
			} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
				while (ReadVpuRegister(BIT_BUSY_FLAG)) {
					if (time_after(jiffies, timeout))
						goto DONE_SUSPEND;
				}

				for (i = 0; i < 64; i++)
					s_vpu_reg_store[core][i] =
						ReadVpuRegister(
							BIT_BASE +
							(0x100 + (i * 4)));
			} else {
				VCODEC_DBG_ERR("Unknown product id : %08x\n",
					product_code);
				goto DONE_SUSPEND;
			}
		}
	}

	set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return 0;

DONE_SUSPEND:

	set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return -EAGAIN;
}

static int vpu_resume(struct platform_device *pdev)
{
	struct cvi_vcodec_context *pvctx;
	int i;
	int core;
	u32 val;
	unsigned long timeout = jiffies + HZ; /* vpu wait timeout to 1sec */
	int product_code;
	struct cvi_vpu_device *vdev = platform_get_drvdata(pdev);

	unsigned long code_base;
	u32 code_size;
	u32 remap_size;
	int regVal;
	u32 hwOption = 0;

	set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];
		if (pvctx->s_bit_firmware_info.size == 0) {
			continue;
		}

		product_code = ReadVpuRegister(VPU_PRODUCT_CODE_REGISTER);
		if (PRODUCT_CODE_W_SERIES(product_code)) {
			unsigned long addr_code_base_reg = W4_ADDR_CODE_BASE;
			unsigned long code_size_reg = W4_CODE_SIZE;
			unsigned long code_param_reg = W4_CODE_PARAM;
			unsigned long timeout_cnt_reg =
				W4_INIT_VPU_TIME_OUT_CNT;
			unsigned long hw_opt_reg = W4_HW_OPTION;
			unsigned long suc_reg = W4_RET_SUCCESS;

			if (product_code == WAVE512_CODE ||
			    product_code == WAVE520_CODE) {
				addr_code_base_reg = W5_ADDR_CODE_BASE;
				code_size_reg = W5_CODE_SIZE;
				code_param_reg = W5_CODE_PARAM;
				timeout_cnt_reg = W5_INIT_VPU_TIME_OUT_CNT;
				hw_opt_reg = W5_HW_OPTION;
				suc_reg = W5_RET_SUCCESS;
			}

			code_base = vcodec_dev.s_common_memory.phys_addr;
			/* ALIGN TO 4KB */
			code_size = (W4_MAX_CODE_BUF_SIZE & ~0xfff);
			if (code_size < pvctx->s_bit_firmware_info.size * 2) {
				goto DONE_WAKEUP;
			}

			regVal = 0;
			WriteVpuRegister(W4_PO_CONF, regVal);

			/* Reset All blocks */
			regVal = 0x7ffffff;
			WriteVpuRegister(W4_VPU_RESET_REQ,
					 regVal); /*Reset All blocks*/

			/* Waiting reset done */
			while (ReadVpuRegister(W4_VPU_RESET_STATUS)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

			WriteVpuRegister(W4_VPU_RESET_REQ, 0);

			/* remap page size */
			remap_size = (code_size >> 12) & 0x1ff;
			regVal = 0x80000000 | (W4_REMAP_CODE_INDEX << 12) |
				 (0 << 16) | (1 << 11) | remap_size;
			WriteVpuRegister(W4_VPU_REMAP_CTRL, regVal);
			WriteVpuRegister(W4_VPU_REMAP_VADDR,
					 0x00000000); /* DO NOT CHANGE! */
			WriteVpuRegister(W4_VPU_REMAP_PADDR, code_base);
			WriteVpuRegister(addr_code_base_reg, code_base);
			WriteVpuRegister(code_size_reg, code_size);
			WriteVpuRegister(code_param_reg, 0);
			WriteVpuRegister(timeout_cnt_reg, timeout);

			WriteVpuRegister(hw_opt_reg, hwOption);

			/* Interrupt */
			if (product_code == WAVE512_CODE) {
				// decoder
				regVal = (1 << W5_INT_INIT_SEQ);
				regVal |= (1 << W5_INT_DEC_PIC);
				regVal |= (1 << W5_INT_BSBUF_EMPTY);
			} else if (product_code == WAVE520_CODE) {
				regVal = (1 << W5_INT_ENC_SET_PARAM);
				regVal |= (1 << W5_INT_ENC_PIC);
			} else {
				regVal = (1 << W4_INT_DEC_PIC_HDR);
				regVal |= (1 << W4_INT_DEC_PIC);
				regVal |= (1 << W4_INT_QUERY_DEC);
				regVal |= (1 << W4_INT_SLEEP_VPU);
				regVal |= (1 << W4_INT_BSBUF_EMPTY);
			}

			WriteVpuRegister(W4_VPU_VINT_ENABLE, regVal);

			Wave4BitIssueCommand(core, W4_CMD_INIT_VPU);
			WriteVpuRegister(W4_VPU_REMAP_CORE_START, 1);

			while (ReadVpuRegister(W4_VPU_BUSY_STATUS)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

			if (ReadVpuRegister(suc_reg) == 0) {
				VCODEC_DBG_ERR("WAKEUP_VPU failed [0x%x]",
					ReadVpuRegister(W4_RET_FAIL_REASON));
				goto DONE_WAKEUP;
			}
		} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
			WriteVpuRegister(BIT_CODE_RUN, 0);

			/*---- LOAD BOOT CODE*/
			for (i = 0; i < 512; i++) {
				val = pvctx->s_bit_firmware_info.bit_code[i];
				WriteVpuRegister(BIT_CODE_DOWN,
						 ((i << 16) | val));
			}

			for (i = 0; i < 64; i++)
				WriteVpuRegister(BIT_BASE + (0x100 + (i * 4)),
						 s_vpu_reg_store[core][i]);

			WriteVpuRegister(BIT_BUSY_FLAG, 1);
			WriteVpuRegister(BIT_CODE_RESET, 1);
			WriteVpuRegister(BIT_CODE_RUN, 1);

			while (ReadVpuRegister(BIT_BUSY_FLAG)) {
				if (time_after(jiffies, timeout))
					goto DONE_WAKEUP;
			}

		} else {
			VCODEC_DBG_ERR("Unknown product id : %08x\n",
				product_code);
			goto DONE_WAKEUP;
		}
	}

	if (s_vpu_open_ref_count == 0)
		set_clock_enable(vdev, VCODEC_CLK_DISABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

DONE_WAKEUP:

	if (s_vpu_open_ref_count > 0)
		set_clock_enable(vdev, VCODEC_CLK_ENABLE, BIT(H264_CORE_IDX) | BIT(H265_CORE_IDX));

	return 0;
}

#else
#define vpu_suspend NULL
#define vpu_resume NULL
#endif /* !CONFIG_PM */

static struct platform_driver vpu_driver = {
	.driver = {
		.name = VPU_PLATFORM_DEVICE_NAME,
		.of_match_table = cvi_vpu_match_table,
	},
	.probe = vpu_probe,
	.remove = vpu_remove,
	.suspend = vpu_suspend,
	.resume = vpu_resume,
};

static int __init vpu_init(void)
{
	struct cvi_vcodec_context *pvctx;
	int res, core, i;

	for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
		pvctx = &vcodec_dev.vcodec_ctx[core];
		init_waitqueue_head(&pvctx->s_interrupt_wait_q);
		init_waitqueue_head(&pvctx->s_sbm_interrupt_wait_q);
	}

	for (i = 0; i < VENC_MAX_CHN_NUM; i++) {
		init_waitqueue_head(&tWaitQueue[i]);
	}

	vcodec_dev.s_common_memory.base = 0;
	vcodec_dev.s_instance_pool.base = 0;
	res = platform_driver_register(&vpu_driver);

	return res;
}

static void __exit vpu_exit(void)
{
	platform_driver_unregister(&vpu_driver);
}

MODULE_AUTHOR("CVITEKVPU Inc.");
MODULE_DESCRIPTION("CVITEK VPU linux driver");
MODULE_LICENSE("GPL");

module_init(vpu_init);
module_exit(vpu_exit);

int vpu_hw_reset(void)
{
	return 0;
}
