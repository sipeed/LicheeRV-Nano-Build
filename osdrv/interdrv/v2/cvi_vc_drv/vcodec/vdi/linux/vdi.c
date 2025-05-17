//------------------------------------------------------------------------------
// File: vdi.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)
#ifdef __arm__
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define MMAP mmap64
#else
#define MMAP mmap
#endif

#include <linux/time.h>
#include <linux/dma-buf.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <vc_ctx.h>
#include "../vdi.h"
#include "../vdi_osal.h"
#include "vpuapifunc.h"
#include "coda9/coda9_regdefine.h"
#include "driver/vcodec.h"
#include "wave/common/common_regdefine.h"
#include "wave/wave4/wave4_regdefine.h"

#define VCODEC_DEVICE_NAME "/dev/vcodec"
typedef struct mutex MUTEX_HANDLE;
#define MUTEX_INIT(P_MUTEX_HANDLE, ATTR) mutex_init(P_MUTEX_HANDLE)
#define MUTEX_DESTROY(P_MUTEX_HANDLE) mutex_destroy(P_MUTEX_HANDLE)
#define MUTEX_LOCK(P_MUTEX_HANDLE) mutex_lock_interruptible(P_MUTEX_HANDLE)
#define MUTEX_UNLOCK(P_MUTEX_HANDLE) mutex_unlock(P_MUTEX_HANDLE)

#ifndef CVI_H26X_USE_ION_FW_BUFFER
extern int vpu_get_common_memory(vpudrv_buffer_t *vdb);
extern int vpu_release_common_memory(vpudrv_buffer_t *p_vdb);
#endif
extern int vpu_wait_interrupt(vpudrv_intr_info_t *p_intr_info);
extern int vpu_set_clock_gate_ext(struct clk_ctrl_info *p_info);
extern int vpu_get_instance_pool(vpudrv_buffer_t *p_vdb);
extern int vpu_open_instance(vpudrv_inst_info_t *p_inst_info);
extern int vpu_close_instance(vpudrv_inst_info_t *p_inst_info);
extern int vpu_reset(void);
extern int vpu_get_register_info(vpudrv_buffer_t *p_vdb_register);
extern int vpu_get_chip_version(unsigned int *p_chip_version);
extern int vpu_get_chip_cabability(unsigned int *p_chip_capability);
extern int vpu_get_clock_frequency(unsigned long *p_clk_rate);
extern int vpu_get_single_core_config(int *pSingleCoreConfig);
extern int vpu_op_write(vpu_bit_firmware_info_t *p_bit_firmware_info,
			size_t len);

#define SUPPORT_INTERRUPT
#define VPU_BIT_REG_SIZE (0x4000 * MAX_NUM_VPU_CORE)
#define VDI_SRAM_BASE_ADDR 0x00000000
/* if we can know the sram address in SOC directly for vdi
layer. it is possible to set in vdi layer without
allocation from driver */

#define VDI_SRAM_SIZE_CODA9_1821A 0xE400
#define VDI_SRAM_SIZE_CODA9_1822 0xEC00
#define VDI_SRAM_SIZE_CODA9_1835                                               \
	0x1D000 // FHD MAX size, 0x17D00  4K MAX size 0x34600

#define VDI_SRAM_SIZE_WAVE420L_1821A 0x17A00
#define VDI_SRAM_SIZE_WAVE420L_1822 0x19400
#define VDI_SRAM_SIZE_WAVE420L_1835 0x1D000

#define VDI_SYSTEM_ENDIAN VDI_LITTLE_ENDIAN
#define VDI_128BIT_BUS_SYSTEM_ENDIAN VDI_128BIT_LITTLE_ENDIAN
#define VDI_NUM_LOCK_HANDLES 4

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define VPU_CORE_BASE_OFFSET 0x0
#endif

#define DEBUG_MEMCPY 0

typedef struct vpudrv_buffer_pool_t {
	vpudrv_buffer_t vdb;
	int inuse;
} vpudrv_buffer_pool_t;

typedef struct {
	int chip_version;
	int chip_capability;
	unsigned long core_idx;
	unsigned int product_code;
	int vpu_fd;
	vpu_instance_pool_t *pvip;
	int task_num;
	int clock_state;
	vpudrv_buffer_t vdb_register;
	vpudrv_buffer_t ctrl_register;
	vpudrv_buffer_t addr_remap_register;
	vpu_buffer_t vpu_common_memory;
	vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
	int vpu_buffer_pool_count;

	struct mutex *vpu_mutex;
	struct mutex *core_mutex;
	struct mutex *vpu_disp_mutex;
	vpudrv_buffer_t vdb_bitstream;
	int SingleCore;

	BOOL singleEsBuf;
	unsigned int single_es_buf_size;

} vdi_info_t;

typedef struct {
	unsigned long tv_sec;
	unsigned long tv_usec;
} vdi_timeval_t;

static void cviGetSysTime(vdi_timeval_t *pstTimeval)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts = {0};
#else
	struct timeval tv = {0};
#endif

	if (!pstTimeval)
		return;

	memset(pstTimeval, 0, sizeof(vdi_timeval_t));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_real_ts64(&ts);
	pstTimeval->tv_sec = ts.tv_sec;
	pstTimeval->tv_usec = ts.tv_nsec/1000;
#else
		do_gettimeofday(&tv);
		pstTimeval->tv_sec = tv.tv_sec;
		pstTimeval->tv_usec = tv.tv_usec;
#endif
}

struct mutex vcodecLock[MAX_NUM_VPU_CORE];
vdi_info_t *s_vdi_info[MAX_NUM_VPU_CORE] = { 0 };

static int cviVdiCfgReg(vdi_info_t *vdi, int core_idx);
static int allocate_common_memory(unsigned long core_idx);
static int free_common_memory(unsigned long core_idx);
static vdi_info_t *vdi_get_vdi_info(unsigned long core_idx);
static int swap_endian(unsigned long core_idx, unsigned char *data, int len,
		       int endian);
#ifdef ARCH_CV182X
static BOOL vdi_is_share_single_es_buf(unsigned long core_idx,
				       unsigned int single_es_buf_size,
				       vdi_info_t **ppVdiSwitch);
#endif

void cvi_vdi_init(void)
{
	unsigned long core_idx;

	for (core_idx = 0; core_idx < MAX_NUM_VPU_CORE; core_idx++) {
		MUTEX_INIT(&vcodecLock[core_idx], NULL);
	}
}

static int vdi_get_size_common(unsigned long core_idx, int SingleCore)
{
	if (core_idx >= MAX_NUM_VPU_CORE) {
		CVI_VC_ERR("core_idx (%lu) >= MAX_NUM_VPU_CORE (%d)\n",
			   core_idx, MAX_NUM_VPU_CORE);
		return 0;
	}

	if (SingleCore == 1) {
		if (core_idx == 0)
			return SIZE_COMMON_265;
		else
			return SIZE_COMMON_264;
	} else {
		return SIZE_COMMON * MAX_NUM_VPU_CORE;
	}
}

int vdi_get_single_core(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	return vdi->SingleCore;
}

void vdi_change_task_count(unsigned long core_idx, int add)
{
	int locked = 0;
	vdi_info_t *vdi;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return;
	}

	vdi = s_vdi_info[core_idx];

	if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00) {
		vdi->task_num += add;
		CVI_VC_LOCK("vdi->task_num %d\n", vdi->task_num);
	}

	if (locked == 1)
		vdi_unlock(core_idx);
}

int vdi_get_clk_rate(unsigned long core_idx)
{
	vdi_info_t *vdi;
	unsigned long clk_rate = 0;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (vpu_get_clock_frequency(&clk_rate) < 0) {
		CVI_VC_ERR("[VDI] fail to VDI_IOCTL_GET_CLOCK_FREQUENCY\n");
		return -1;
	}

	CVI_VC_INFO("mw : clk_rate %lu\n", clk_rate);

	return clk_rate;
}

static int vdi_get_product_version(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (vpu_get_chip_version(&vdi->chip_version) < 0) {
		CVI_VC_ERR("[VDI] fail to VDI_IOCTL_GET_CHIP_VERSION\n");
		return -1;
	}

	if (vpu_get_chip_cabability(&vdi->chip_capability) < 0) {
		CVI_VC_ERR(
		     "[VDI] fail to VDI_IOCTL_GET_CHIP_CAP\n");
		return -1;
	}

	CVI_VC_INFO("chip_version = 0x%X, chip_capability = 0x%X\n",
			vdi->chip_version, vdi->chip_capability);

	return 0;
}

int vdi_init(unsigned long core_idx, BOOL bCountTaskNum)
{
	vdi_info_t *vdi;
	int i, ret = 0;

	if (core_idx >= MAX_NUM_VPU_CORE)
		return 0;

	if (s_vdi_info[core_idx] == NULL) {
		s_vdi_info[core_idx] = osal_malloc(sizeof(vdi_info_t));
	}
	vdi = s_vdi_info[core_idx];

	if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00) {
		CVI_VC_INFO("Already call vdi_init, vdi->task_num %d\n",
			    vdi->task_num);
		if (bCountTaskNum)
			vdi_change_task_count(core_idx, 1);
		return 0;
	}

// if this API supports VPU parallel
// processing using multi VPU. the driver
// should be made to open multiple times.
	vdi->vpu_fd = 'v' + core_idx; // TODO: refine this!

	CVI_VC_TRACE("core_idx = %ld, vpu_fd = %d\n", core_idx, vdi->vpu_fd);
	if (vdi_get_product_version(core_idx)) {
		CVI_VC_INFO(
		     "[VDI] fail to vdi_get_product_version\n");
		goto ERR_VDI_INIT;
	}

	ret = cviVdiCfgReg(vdi, core_idx);
	if (ret < 0) {
		CVI_VC_TRACE("\n");
		goto ERR_VDI_INIT;
	}

	memset(&vdi->vpu_buffer_pool, 0x00,
	       sizeof(vpudrv_buffer_pool_t) * MAX_VPU_BUFFER_POOL);

	if (!vdi_get_instance_pool(core_idx)) {
		CVI_VC_INFO(
			"[VDI] fail to create shared info for saving context\n");
		goto ERR_VDI_INIT;
	}

	if (vdi->pvip->instance_pool_inited == FALSE) {
		CodecInst *pCodecInst;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
		/* If a process or a thread is terminated abnormally,
		 * pthread_mutexattr_setrobust_np(attr, PTHREAD_MUTEX_ROBUST_NP)
		 * makes next onwer call pthread_mutex_lock() without deadlock.
		 */
		pthread_mutexattr_setrobust_np(&mutexattr,
					       PTHREAD_MUTEX_ROBUST_NP);
#endif
		MUTEX_INIT((MUTEX_HANDLE *)vdi->vpu_mutex, &mutexattr);
		MUTEX_INIT((MUTEX_HANDLE *)vdi->core_mutex, &mutexattr);
		MUTEX_INIT((MUTEX_HANDLE *)vdi->vpu_disp_mutex, &mutexattr);

		for (i = 0; i < MAX_NUM_INSTANCE; i++) {
			pCodecInst = (CodecInst *)vdi->pvip->codecInstPool[i];
			pCodecInst->instIndex = i;
			pCodecInst->inUse = 0;

			CVI_VC_INFO(
				"%d, pCodecInst->instIndex %d, pCodecInst->inUse %d\n",
				i, pCodecInst->instIndex, pCodecInst->inUse);
		}

		vdi->pvip->instance_pool_inited = TRUE;
	}

	if (vdi_lock(core_idx) < 0) {
		CVI_VC_ERR("[VDI] fail to handle lock function\n");
		goto ERR_VDI_INIT;
	}

	vdi_set_clock_gate(core_idx, CLK_ENABLE);

	vdi->product_code =
		vdi_read_register(core_idx, VPU_PRODUCT_CODE_REGISTER);

	CVI_VC_TRACE("core_idx = %ld, product_code = 0x%X\n", core_idx,
		     vdi->product_code);

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		if (vdi_read_register(core_idx, W4_VCPU_CUR_PC) == 0) {
			// if BIT processor is not running.
			for (i = 0; i < 64; i++)
				vdi_write_register(core_idx, (i * 4) + 0x100,
						   0x0);
		}
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		// CODA9XX
		if (vdi_read_register(core_idx, BIT_CUR_PC) == 0) // if BIT
		// processor
		// is not
		// running.
		{
			for (i = 0; i < 64; i++)
				vdi_write_register(core_idx, (i * 4) + 0x100,
						   0x0);
		}
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		vdi_set_clock_gate(core_idx, CLK_DISABLE);
		goto ERR_VDI_INIT;
	}

	vdi_set_clock_gate(core_idx, CLK_DISABLE);

	if (vpu_get_single_core_config(&vdi->SingleCore) < 0) {
		CVI_VC_ERR(
			"[VDI] fail to get single core setting from driver\n");
		goto ERR_VDI_INIT;
	}

#ifdef CVI_H26X_USE_ION_FW_BUFFER
	// TODO: remove singleCore config
	vdi->SingleCore = 1;
#endif

	if (allocate_common_memory(core_idx) < 0) {
		CVI_VC_ERR("[VDI] fail to get common buffer from driver\n");
		goto ERR_VDI_INIT;
	}

	vdi->core_idx = core_idx;

	vdi_unlock(core_idx);
	if (bCountTaskNum)
		vdi_change_task_count(core_idx, 1);

	CVI_VC_INFO("[VDI] success to init driver clk gate %d\n",
		    vdi->clock_state);
	return 0;

ERR_VDI_INIT:
	vdi_unlock(core_idx);
	vdi_release(core_idx);
	return -1;
}

static int cviVdiCfgReg(vdi_info_t *vdi, int core_idx)
{
	CVI_VC_TRACE("vdi %p\n", vdi);

	if (vdi->vdb_register.virt_addr) {
		// already got vdb_register
		return 0;
	}

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
	vdi->vdb_register.size = core_idx;
	if (vpu_get_register_info(&vdi->vdb_register) < 0) {
		CVI_VC_ERR("[VDI] fail to get host interface register\n");
		return -1;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	vdi->vdb_register.virt_addr =
		ioremap(vdi->vdb_register.phys_addr, vdi->vdb_register.size);
#else
	vdi->vdb_register.virt_addr = ioremap_nocache(
		vdi->vdb_register.phys_addr, vdi->vdb_register.size);
#endif
#else
	vdi->vdb_register.size = VPU_BIT_REG_SIZE;
	vdi->vdb_register.virt_addr =
		MMAP(NULL, vdi->vdb_register.size, PROT_READ | PROT_WRITE,
		     MAP_SHARED, vdi->vpu_fd, 0);
#endif

	CVI_VC_INFO(
		"mmap vdi->vdb_register.virt_addr %p, vdi->vdb_register.size %d, vdi->vdb_register.phys_addr 0x%llx\n",
		vdi->vdb_register.virt_addr, vdi->vdb_register.size,
		vdi->vdb_register.phys_addr);

	if ((void *)vdi->vdb_register.virt_addr == NULL) {
		CVI_VC_ERR("[VDI] fail to map %s registers\n",
			   VCODEC_DEVICE_NAME);
		return -1;
	}

	CVI_VC_INFO(
		"[VDI] map vdb_register core_idx=%d, virtaddr=%p, size=%d\n",
		core_idx, vdi->vdb_register.virt_addr, vdi->vdb_register.size);

	return 0;
}

int vdi_set_bit_firmware_to_pm(unsigned long core_idx,
			       const unsigned short *code)
{
	int i;
	vpu_bit_firmware_info_t *p_bit_firmware_info =
		vzalloc(sizeof(vpu_bit_firmware_info_t));
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return 0;
	}

	p_bit_firmware_info->size = sizeof(vpu_bit_firmware_info_t);
	p_bit_firmware_info->core_idx = core_idx;
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	p_bit_firmware_info->reg_base_offset =
		(core_idx * VPU_CORE_BASE_OFFSET);
#else
	p_bit_firmware_info->reg_base_offset = 0;
#endif
	for (i = 0; i < 512; i++)
		p_bit_firmware_info->bit_code[i] = code[i];

	if (vpu_op_write(p_bit_firmware_info, p_bit_firmware_info->size) < 0) {
		CVI_VC_ERR("[VDI] fail to vdi_set_bit_firmware core=%d\n",
			   p_bit_firmware_info->core_idx);
		return -1;
	}
	vfree(p_bit_firmware_info);

	return 0;
}

#if 0
int vdi_get_vpu_fd(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	return vdi->vpu_fd;
}
#endif

int vdi_release(unsigned long core_idx)
{
	int i;
	vpudrv_buffer_t vdb;
	vdi_info_t *vdi;
	int locked = 0;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_WARN("vdi_get_vdi_info\n");
		return 0;
	}

	if (vdi->task_num > 1) {
		// means that the opened instance remains
		vdi_change_task_count(core_idx, -1);

		CVI_VC_INFO("task_num = %d\n", vdi->task_num);

		return 0;
	}

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return 0;
	}

	CVI_VC_INFO("task_num = 0, try to release\n");

	if (vdi->vdb_register.virt_addr) {
		iounmap((void *)vdi->vdb_register.virt_addr);
	}

	osal_memset(&vdi->vdb_register, 0x00, sizeof(vpudrv_buffer_t));
	vdb.size = 0;
	// get common memory information to free virtual address
	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_common_memory.phys_addr >=
			    vdi->vpu_buffer_pool[i].vdb.phys_addr &&
		    vdi->vpu_common_memory.phys_addr <
			    (vdi->vpu_buffer_pool[i].vdb.phys_addr +
			     vdi->vpu_buffer_pool[i].vdb.size)) {
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			vdb = vdi->vpu_buffer_pool[i].vdb;
#ifndef CVI_H26X_USE_ION_FW_BUFFER
			// if use ion to allocate common buffer, it would be unmap() in ion_free
			iounmap((void *)vdb.virt_addr);
#endif
			break;
		}
	}

	if (i >= MAX_VPU_BUFFER_POOL) {
		CVI_VC_ERR("i >= MAX_VPU_BUFFER_POOL\n");
	}

	CVI_VC_INFO("physaddr=0x%llx, virtaddr=%p, size=0x%x, index=%d\n",
		    vdb.phys_addr, vdb.virt_addr, vdb.size, i);

	if (locked)
		vdi_unlock(core_idx);

	vdi_change_task_count(core_idx, -1);

	if (vdi->SingleCore == 1) {
		if (free_common_memory(core_idx) < 0) {
			CVI_VC_ERR(
				"[VDI] fail to free common buffer from driver\n");
		}
	}

	if (vdi->vpu_fd != -1 && vdi->vpu_fd != 0x00) {
		vdi_set_clock_gate(core_idx, CLK_DISABLE);
		vdi->vpu_fd = -1;
	}
	MUTEX_DESTROY(&vcodecLock[core_idx]);

#ifdef ARCH_CV182X
	do {
		vdi_info_t *vdiSwitch = NULL;
		BOOL bIsUseShareSingleBuf = FALSE;

		bIsUseShareSingleBuf = vdi_is_share_single_es_buf(
			core_idx, vdi->single_es_buf_size, &vdiSwitch);

		for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
			if (vdi->vpu_buffer_pool[i].inuse &&
				vdi->vpu_buffer_pool[i].vdb.size > 0) {
				if (bIsUseShareSingleBuf &&
					vdiSwitch->vdb_bitstream.virt_addr ==
						vdi->vpu_buffer_pool[i].vdb.virt_addr) {
					continue;
				}
			}
		}
	} while (0);
#else // ARCH_CV182X
	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse &&
			vdi->vpu_buffer_pool[i].vdb.size > 0) {
		}
	}
#endif // ARCH_CV182X

	memset(vdi, 0x00, sizeof(vdi_info_t));

	if (s_vdi_info[core_idx]) {
		osal_free(s_vdi_info[core_idx]);
	}
	s_vdi_info[core_idx] = NULL;

	return 0;
}

int vdi_get_common_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memcpy(vb, &vdi->vpu_common_memory, sizeof(vpu_buffer_t));

	return 0;
}

int allocate_common_memory(unsigned long core_idx)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	int i;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vdi_get_size_common(core_idx, vdi->SingleCore);

	if (vdb.size == 0) {
		CVI_VC_ERR("[VDI] fail to vdi_get_size_common %d\n", vdb.size);
		return -1;
	}

	CVI_VC_MEM("common mem vdb.sizesize=0x%x\n", vdb.size);

#ifdef CVI_H26X_USE_ION_FW_BUFFER

	if (vdi->vpu_common_memory.base == 0) {
		vpu_buffer_t vb = { 0x0 };
		char ionName[MAX_VPU_ION_BUFFER_NAME];

		vb.size = vdb.size;

		if (core_idx == 0) {
			sprintf(ionName, "VCODEC_H265_FW_Buffer");
		} else {
			sprintf(ionName, "VCODEC_H264_FW_Buffer");
		}

		VDI_ALLOCATE_MEMORY(core_idx, &vb, 0, ionName);

		vdb.phys_addr = vb.phys_addr;
		vdb.base = vb.base;
		vdb.virt_addr = vb.virt_addr;
	} else {
		vdb.phys_addr = vdi->vpu_common_memory.phys_addr;
		vdb.base = vdi->vpu_common_memory.base;
		vdb.virt_addr = vdi->vpu_common_memory.virt_addr;
	}
#else
	if (vpu_get_common_memory(&vdb) < 0) {
		CVI_VC_ERR("[VDI] fail to allocate_common_memory size=%d\n",
			   vdb.size);
		return -1;
	}

	vdb.virt_addr = ioremap_nocache(vdb.phys_addr, vdb.size);
#endif

	CVI_VC_TRACE(
		"mmap vdb.virt_addr %p, vdb.size %d, vdb.phys_addr 0x%llx\n",
		vdb.virt_addr, vdb.size, vdb.phys_addr);

	CVI_VC_MEM(
		"[VDI] allocate_common_memory, physaddr=0x%llx, virtaddr=%p\n",
		vdb.phys_addr, vdb.virt_addr);

	// convert os driver buffer type to vpu buffer type
#if 1 //def SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	if (vdi->SingleCore == 1) {
		vdi->pvip->vpu_common_buffer.size =
			vdi_get_size_common(core_idx, vdi->SingleCore);
		vdi->pvip->vpu_common_buffer.phys_addr = (vdb.phys_addr);
		vdi->pvip->vpu_common_buffer.base = (vdb.base);
		vdi->pvip->vpu_common_buffer.virt_addr = vdb.virt_addr;
	} else {
		vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
		vdi->pvip->vpu_common_buffer.phys_addr =
			(vdb.phys_addr + (core_idx * SIZE_COMMON));
		vdi->pvip->vpu_common_buffer.base =
			(vdb.base + (core_idx * SIZE_COMMON));
		vdi->pvip->vpu_common_buffer.virt_addr =
			(vdb.virt_addr + (core_idx * SIZE_COMMON));
	}
#else
	vdi->pvip->vpu_common_buffer.size = SIZE_COMMON;
	vdi->pvip->vpu_common_buffer.phys_addr = (vdb.phys_addr);
	vdi->pvip->vpu_common_buffer.base = (vdb.base);
	vdi->pvip->vpu_common_buffer.virt_addr = vdb.virt_addr;
#endif

	osal_memcpy(&vdi->vpu_common_memory, &vdi->pvip->vpu_common_buffer,
		    sizeof(vpudrv_buffer_t));

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 0) {
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool_count++;
			vdi->vpu_buffer_pool[i].inuse = 1;
			break;
		}
	}

	CVI_VC_TRACE("physaddr=0x%llx, size=0x%x, virtaddr=%p\n",
		     vdi->vpu_common_memory.phys_addr,
		     (int)vdi->vpu_common_memory.size,
		     vdi->vpu_common_memory.virt_addr);

	return 0;
}

int free_common_memory(unsigned long core_idx)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	vpudrv_buffer_t *p_vdb;
	int i;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memcpy(&vdb, &vdi->pvip->vpu_common_buffer,
		    sizeof(vpudrv_buffer_t));

	if (vdb.size == 0) {
		CVI_VC_ERR("[VDI] fail to get common memory size %x\n",
			   vdb.size);
		return -1;
	}

	CVI_VC_MEM("release common mem physaddr=0x%llx size=0x%x\n",
		   vdb.phys_addr, vdb.size);

#ifdef CVI_H26X_USE_ION_FW_BUFFER
	do {
		vpu_buffer_t vb;
		osal_memcpy(&vb, &vdi->pvip->vpu_common_buffer, sizeof(vpu_buffer_t));
		VDI_FREE_MEMORY(core_idx, &vb);
	} while (0);
#else

	if (vpu_release_common_memory(&vdb) < 0) {
		CVI_VC_ERR("[VDI] fail to vpu_release_common_memory size=%d\n",
			   vdb.size);
		return -1;
	}
	iounmap((void *)vdb.virt_addr);

#endif

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vdb.phys_addr) {
			p_vdb = &vdi->vpu_buffer_pool[i].vdb;
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			break;
		}
	}

	osal_memset(p_vdb, 0x0, sizeof(vpudrv_buffer_t));
	osal_memset(&vdi->pvip->vpu_common_buffer, 0x0, sizeof(vpu_buffer_t));
	osal_memset(&vdi->vpu_common_memory, 0x0, sizeof(vpu_buffer_t));

	return 0;
}

vpu_instance_pool_t *vdi_get_instance_pool(unsigned long core_idx)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	int i = 0;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return NULL;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	if (sizeof(CodecInst) > MAX_INST_HANDLE_SIZE) {
		CVI_VC_ERR("CodecInst = %d, MAX_INST_HANDLE_SIZE = %d\n",
			   (int)sizeof(CodecInst), MAX_INST_HANDLE_SIZE);
	}

	if (!vdi->pvip) {
		vdb.size = sizeof(vpu_instance_pool_t) +
			   sizeof(MUTEX_HANDLE) * VDI_NUM_LOCK_HANDLES;
#if 1 //def SUPPORT_MULTI_CORE_IN_ONE_DRIVER
		vdb.size *= MAX_NUM_VPU_CORE;
#endif
		if (vpu_get_instance_pool(&vdb) < 0) {
			CVI_VC_ERR(
				"[VDI] fail to allocate get instance pool physical space=%d\n",
				(int)vdb.size);
			return NULL;
		}

#ifdef USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY
		vdb.virt_addr =
			(__u8 *)(uintptr_t)(vdb.phys_addr); // instance pool was allocated by vmalloc
#else
		vdb.virt_addr = MMAP(NULL, vdb.size, PROT_READ | PROT_WRITE,
				     MAP_SHARED, vdi->vpu_fd, vdb.phys_addr);
#endif

		CVI_VC_MEM("sizeof(vpu_instance_pool_t) = %zd\n",
			   sizeof(vpu_instance_pool_t));
		CVI_VC_MEM(
			"vmalloc, sizeof(MUTEX_HANDLE) * VDI_NUM_LOCK_HANDLES = %zd\n",
			sizeof(MUTEX_HANDLE) * VDI_NUM_LOCK_HANDLES);
		CVI_VC_MEM(
			"mmap vdb.virt_addr %p, vdb.size %d, vdb.phys_addr 0x%llx, lr %p\n",
			vdb.virt_addr, vdb.size, vdb.phys_addr,
			__builtin_return_address(0));

		if ((void *)vdb.virt_addr == NULL) {
			CVI_VC_ERR(
				"[VDI] fail to map instance pool phyaddr=0x%llx, size = %d\n",
				vdb.phys_addr, (int)vdb.size);
			return NULL;
		}

#if 1 //def SUPPORT_MULTI_CORE_IN_ONE_DRIVER
		vdi->pvip = (vpu_instance_pool_t
				     *)(vdb.virt_addr +
					(core_idx *
					 (sizeof(vpu_instance_pool_t) +
					  sizeof(MUTEX_HANDLE) *
						  VDI_NUM_LOCK_HANDLES)));
#else
		vdi->pvip = (vpu_instance_pool_t *)(vdb.virt_addr);
#endif

		vdi->vpu_mutex = (MUTEX_HANDLE *)((unsigned long)vdi->pvip +
						  sizeof(vpu_instance_pool_t));
		// change the
		// pointer of vpu_mutex to at end pointer of vpu_instance_pool_t
		// to assign at allocated position.
		vdi->vpu_disp_mutex =
			(MUTEX_HANDLE *)((unsigned long)vdi->pvip +
					 sizeof(vpu_instance_pool_t) +
					 sizeof(MUTEX_HANDLE));
		vdi->core_mutex = (MUTEX_HANDLE *)((unsigned long)vdi->pvip +
						   sizeof(vpu_instance_pool_t) +
						   sizeof(MUTEX_HANDLE) * 2);

		CVI_VC_INFO(
			"[VDI] instance pool physaddr=0x%llx, virtaddr=%p, base=0x%llx, size=%d\n",
			vdb.phys_addr, vdb.virt_addr, vdb.base, vdb.size);
		for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
			if (vdi->vpu_buffer_pool[i].inuse == 0) {
				vdi->vpu_buffer_pool[i].vdb = vdb;
				vdi->vpu_buffer_pool_count++;
				vdi->vpu_buffer_pool[i].inuse = 1;
				break;
			}
		}
	}

	return (vpu_instance_pool_t *)vdi->pvip;
}

int vdi_open_instance(unsigned long core_idx, unsigned long inst_idx)
{
	vdi_info_t *vdi;
	vpudrv_inst_info_t inst_info;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	inst_info.core_idx = core_idx;
	inst_info.inst_idx = inst_idx;
	if (vpu_open_instance(&inst_info) < 0) {
		CVI_VC_ERR(
			"[VDI] fail to deliver open instance num inst_idx=%d\n",
			(int)inst_idx);
		return -1;
	}

	vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

	CVI_VC_TRACE("vdi->pvip->vpu_instance_num %d\n",
		     vdi->pvip->vpu_instance_num);

	return 0;
}

int vdi_close_instance(unsigned long core_idx, unsigned long inst_idx)
{
	vdi_info_t *vdi;
	vpudrv_inst_info_t inst_info;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	inst_info.core_idx = core_idx;
	inst_info.inst_idx = inst_idx;
	if (vpu_close_instance(&inst_info) < 0) {
		CVI_VC_ERR(
			"[VDI] fail to deliver open instance num inst_idx=%d\n",
			(int)inst_idx);
		return -1;
	}

	vdi->pvip->vpu_instance_num = inst_info.inst_open_count;

	CVI_VC_INFO(
		"inst_idx %ld, after close vdi->pvip->vpu_instance_num %d\n",
		inst_idx, vdi->pvip->vpu_instance_num);

	return 0;
}

int vdi_get_instance_num(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	return vdi->pvip->vpu_instance_num;
}

#ifdef REDUNDENT_CODE
int vdi_hw_reset(unsigned long core_idx) // DEVICE_ADDR_SW_RESET
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	return vpu_reset();
}
#endif

/*
	pidt : the pid for current thread
	mutex_owner_id : the owner id of the mutex

	if two id are equal, means that current thread is holding the mutex
	if not, means, the mutex was not locked, or other thread is holding the mutex

	return :
		0 : caller do nothing
		1 : caller need to call vdi_lock()
*/
int vdi_check_lock_by_me(unsigned long core_idx)
{
	vdi_info_t *vdi;
	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (vdi_lock_check(core_idx) == 0)
		return 0;

	// this thread not holding the mutex or no one is holding the mutex
	// go to locked the mutex
	vdi_lock(core_idx);

	return 1;
}

int vdi_lock(unsigned long core_idx)
{
	vdi_info_t *vdi;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
	int ret = 0;
#else
	const int MUTEX_TIMEOUT = 0x7fffffff;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
	ret = MUTEX_LOCK((MUTEX_HANDLE *)vdi->vpu_mutex);

	// might see we already hold the mutex, which means that we lock it more than one time
	if (ret)
		CVI_VC_ERR("Mutex lock failed %d\n", ret);
#else
	if (MUTEX_LOCK((MUTEX_HANDLE *)vdi->vpu_mutex) != 0) {
		CVI_VC_ERR("failed to pthread_mutex_lock\n");
		return -1;
	}
#endif

	return 0;
}

/*
	return value :
		-1 : Means that this thread didn't locked the mutex
		 0 : Means that this thread had locked the mutex
*/
int vdi_lock_check(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (mutex_is_locked(vdi->vpu_mutex)) {
		return 0;
	} else {
		return -1;
	}
	return 0;
}

void vdi_unlock(unsigned long core_idx)
{
	vdi_info_t *vdi;
	int ret = 0;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	MUTEX_UNLOCK((MUTEX_HANDLE *)vdi->vpu_mutex);

	if (ret)
		CVI_VC_ERR("unlock failed %d\n", ret);

	CVI_VC_LOCK("vpu_mutex, unlock\n");
}

int vdi_vcodec_trylock(unsigned long core_idx)
{
#ifdef ARCH_CV183X
#else
	core_idx = 0;
#endif
	if (mutex_trylock(&vcodecLock[core_idx]) == 1) {
		// mutex_trylock returns 1 if the mutex has been acquired successfully,
		// and 0 on contention.
		return 0;
	} else {
		return -EAGAIN;
	}
}

int vdi_vcodec_timelock(unsigned long core_idx, int timeout_ms)
{
	int ret;

#ifdef ARCH_CV183X
#else
	core_idx = 0;
#endif
//1ms = 1000000ns
	do {
		int wait_cnt_ms = 1;

		while (mutex_is_locked(&vcodecLock[core_idx])) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(usecs_to_jiffies(1000));
			if (wait_cnt_ms >= timeout_ms) {
				break;
			}
			wait_cnt_ms++;
		}
	} while (0);

	ret = mutex_trylock(&vcodecLock[core_idx]);
	// mutex_trylock returns 1 if the mutex has been acquired successfully,
	// and 0 on contention.
	if (ret == 1) {
		ret = 0;
	} else {
		ret = ETIMEDOUT;
	}

	if (ret != 0) {
		if (ret == ETIMEDOUT)
			CVI_VC_LOCK("lock timeout %d time[%d]\n", ret,
				    timeout_ms);
		else
			CVI_VC_ERR("lock error %d\n", ret);

		return ret;
		//timelock failure , return -1
	} else {
		//timelock success
		return 0;
	}
}

/* vdi_vcodec_lock: lock for all threads in one process,
 *                  which data structure is saved in user space.
 */
void vdi_vcodec_lock(unsigned long core_idx)
{
#ifdef ARCH_CV183X
#else
	core_idx = 0;
#endif
	MUTEX_LOCK(&vcodecLock[core_idx]);
}

void vdi_vcodec_unlock(unsigned long core_idx)
{
#ifdef ARCH_CV183X
#else
	core_idx = 0;
#endif
	MUTEX_UNLOCK(&vcodecLock[core_idx]);
}

static vdi_info_t *vdi_get_vdi_info(unsigned long core_idx)
{
	vdi_info_t *vdi;

	if (core_idx >= MAX_NUM_VPU_CORE)
		return NULL;

	vdi = s_vdi_info[core_idx];

	if (!vdi || vdi->vpu_fd == -1 || vdi->vpu_fd == 0x00) {
		CVI_VC_WARN("lock failed, vdi %p, ret %p\n", vdi,
			    __builtin_return_address(0));
		if (vdi)
			CVI_VC_WARN("core_idx %lu with vpu_fd %d\n", core_idx,
				    vdi->vpu_fd);
		return NULL;
	}

	return vdi;
}

int vdi_disp_lock(unsigned long core_idx)
{
	vdi_info_t *vdi;
#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
#else
	const int MUTEX_TIMEOUT = 5000; // ms
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

#if defined(ANDROID) || !defined(PTHREAD_MUTEX_ROBUST_NP)
	MUTEX_LOCK((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
#else
	if (MUTEX_LOCK((MUTEX_HANDLE *)vdi->vpu_disp_mutex) != 0) {
		CVI_VC_ERR("failed to pthread_mutex_lock\n");
		return -1;
	}

#endif /* ANDROID */

	return 0;
}

void vdi_disp_unlock(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	MUTEX_UNLOCK((MUTEX_HANDLE *)vdi->vpu_disp_mutex);
}

void vdi_write_register(unsigned long core_idx, unsigned long addr,
			unsigned int data)
{
	vdi_info_t *vdi;
	unsigned long *reg_addr;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	if (vdi->clock_state == 0) {
		vdi_set_clock_gate(core_idx, CLK_ENABLE);
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	reg_addr =
		(unsigned long *)(addr +
				  (unsigned long)vdi->vdb_register.virt_addr +
				  (core_idx * VPU_CORE_BASE_OFFSET));
#else
	reg_addr =
		(unsigned long *)(addr +
				  (unsigned long)vdi->vdb_register.virt_addr);
#endif

	CVI_VC_REG("write, %p = 0x%X\n", reg_addr, data);

	writel(data, reg_addr);
}

unsigned int vdi_read_register(unsigned long core_idx, unsigned long addr)
{
	vdi_info_t *vdi;
	unsigned long *reg_addr;
	unsigned int value;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return (unsigned int)-1;
	}

	if (vdi->clock_state == 0) {
		vdi_set_clock_gate(core_idx, CLK_ENABLE);
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	reg_addr =
		(unsigned long *)(addr +
				  (unsigned long)vdi->vdb_register.virt_addr +
				  (core_idx * VPU_CORE_BASE_OFFSET));
#else
	reg_addr =
		(unsigned long *)(addr +
				  (unsigned long)vdi->vdb_register.virt_addr);
#endif

	value = readl(reg_addr);

	CVI_VC_REG("read, %p, 0x%X\n", reg_addr, value);

	return value;
}

#define FIO_TIMEOUT 100

unsigned int vdi_fio_read_register(unsigned long core_idx, unsigned long addr)
{
	unsigned int ctrl;
	unsigned int count = 0;
	unsigned int data = 0xffffffff;

	ctrl = (addr & 0xffff);
	ctrl |= (0 << 16); /* read operation */
	vdi_write_register(core_idx, W4_VPU_FIO_CTRL_ADDR, ctrl);
	count = FIO_TIMEOUT;
	while (count--) {
		ctrl = vdi_read_register(core_idx, W4_VPU_FIO_CTRL_ADDR);
		if (ctrl & 0x80000000) {
			data = vdi_read_register(core_idx, W4_VPU_FIO_DATA);
			break;
		}
	}

	return data;
}

void vdi_fio_write_register(unsigned long core_idx, unsigned long addr,
			    unsigned int data)
{
	unsigned int ctrl;

	vdi_write_register(core_idx, W4_VPU_FIO_DATA, data);
	ctrl = (addr & 0xffff);
	ctrl |= (1 << 16); /* write operation */
	vdi_write_register(core_idx, W4_VPU_FIO_CTRL_ADDR, ctrl);
}

#define VCORE_DBG_ADDR(__vCoreIdx) (0x8000 + (0x1000 * __vCoreIdx) + 0x300)
#define VCORE_DBG_DATA(__vCoreIdx) (0x8000 + (0x1000 * __vCoreIdx) + 0x304)
#define VCORE_DBG_READY(__vCoreIdx) (0x8000 + (0x1000 * __vCoreIdx) + 0x308)

#ifdef REDUNDENT_CODE
static void UNREFERENCED_FUNCTION write_vce_register(unsigned int core_idx,
						     unsigned int vce_core_idx,
						     unsigned int vce_addr,
						     unsigned int udata)
{
	int vcpu_reg_addr;
	unsigned int i = 0;

	vdi_fio_write_register(core_idx, VCORE_DBG_READY(vce_core_idx), 0);

	vcpu_reg_addr = vce_addr >> 2;

	vdi_fio_write_register(core_idx, VCORE_DBG_DATA(vce_core_idx), udata);
	vdi_fio_write_register(core_idx, VCORE_DBG_ADDR(vce_core_idx),
			       (vcpu_reg_addr)&0x00007FFF);

	for (i = 0; i < 16; i++) {
		unsigned int vcpu_reg_val =
			vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx));
		if ((vcpu_reg_val >> 31) & 0x1)
			CVI_VC_ERR("failed to write VCE register: 0x%04x\n",
				   vce_addr);
		else
			break;
	}
}
#endif

#ifdef ENABLE_CNM_DEBUG_MSG
static unsigned int read_vce_register(unsigned int core_idx,
				      unsigned int vce_core_idx,
				      unsigned int vce_addr)
{
	int vcpu_reg_addr;
	int udata;
	int vce_core_base = 0x8000 + 0x1000 * vce_core_idx;

	vdi_fio_write_register(core_idx, VCORE_DBG_READY(vce_core_idx), 0);

	vcpu_reg_addr = vce_addr >> 2;

	vdi_fio_write_register(core_idx, VCORE_DBG_ADDR(vce_core_idx),
			       vcpu_reg_addr + vce_core_base);

	while (TRUE) {
		if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) ==
		    1) {
			udata = vdi_fio_read_register(
				0, VCORE_DBG_DATA(vce_core_idx));
			break;
		}
	}

	return udata;
}
#endif

int vdi_clear_memory(unsigned long core_idx, PhysicalAddress addr, int len)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	unsigned long offset;

	int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 1) {
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr &&
			    addr < (vdb.phys_addr + vdb.size))
				break;
		}
	}

	if (!vdb.size) {
		CVI_VC_ERR("address 0x%08x is not mapped address!!!\n",
			   (int)addr);
		return -1;
	}

	offset = addr - (unsigned long)vdb.phys_addr;
	osal_memset((void *)((unsigned long)vdb.virt_addr + offset), 0x00, len);

	return len;
}

#ifdef REDUNDENT_CODE
void vdi_set_sdram(unsigned long core_idx, unsigned long addr, int len,
		   int endian)
{
	vdi_info_t *vdi;
	unsigned char *buf;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	buf = (unsigned char *)osal_malloc(len);
	osal_memset(buf, 0x00, len);
	vdi_write_memory(core_idx, addr, buf, len, endian);
	osal_free(buf);
}
#endif

int vdi_write_memory(unsigned long core_idx, PhysicalAddress addr,
		     unsigned char *data, int len, int endian)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	PhysicalAddress offset;
	int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 1) {
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr &&
			    addr < (vdb.phys_addr + vdb.size)) {
				break;
			}
		}
	}

	if (!vdb.size) {
		CVI_VC_ERR("address 0x%08x is not mapped address!!!\n",
			   (int)addr);
		return -1;
	}

	offset = addr - vdb.phys_addr;
	swap_endian(core_idx, data, len, endian);
	osal_memcpy((void *)((__u8 *)vdb.virt_addr + offset), data, len);

	return len;
}

int vdi_read_memory(unsigned long core_idx, PhysicalAddress addr,
		    unsigned char *data, int len, int endian)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	PhysicalAddress offset;
	int i;
#if DEBUG_MEMCPY
	unsigned char *buf;
#endif
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 1) {
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr &&
			    addr < (vdb.phys_addr + vdb.size))
				break;
		}
	}

	if (!vdb.size) {
		return -1;
	}

	CVI_VC_TRACE(
		"i = %d, addr = 0x%llX, phys_addr = 0x%llX, virt_addr = %p, data = 0x%p\n",
		i, addr, vdb.phys_addr, (void *)vdb.virt_addr, data);

	offset = addr - (unsigned long)vdb.phys_addr;

#if DEBUG_MEMCPY
	__u8 *srcAddr;

	srcAddr = ((__u8 *)vdb.virt_addr + offset);
	CVI_VC_TRACE("srcAddr = %p, len = 0x%X\n", srcAddr, len);

	buf = (Uint8 *)osal_malloc(0x5000);
	if (buf == NULL) {
		CVI_VC_ERR("osal_malloc\n");
		return FALSE;
	}
	CVI_VC_TRACE("buf = 0x%lX, srcAddr = 0x%lX, len = 0x%X\n", buf, srcAddr,
		     len & (~0x7));
	osal_memcpy(buf, (const void *)srcAddr, len & (~0x7));
	CVI_VC_TRACE("\n");
	swap_endian(core_idx, buf, len, endian);
#else
	osal_memcpy(data, (const void *)((__u8 *)vdb.virt_addr + offset), len);
	swap_endian(core_idx, data, len, endian);
#endif

	CVI_VC_TRACE("\n");

	return len;
}

void *vdi_get_vir_addr(unsigned long core_idx, PhysicalAddress addr)
{
	vdi_info_t *vdi;
	vpudrv_buffer_t vdb;
	PhysicalAddress offset;
	__u8 *srcAddr;
	int i;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return NULL;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 1) {
			vdb = vdi->vpu_buffer_pool[i].vdb;
			if (addr >= vdb.phys_addr &&
			    addr < (vdb.phys_addr + vdb.size))
				break;
		}
	}

	if (!vdb.size) {
		CVI_VC_ERR("size = %d\n", vdb.size);
		return NULL;
	}

	CVI_VC_TRACE(
		"i = %d, addr = 0x%llX, phys_addr = 0x%llX, virt_addr = %p\n",
		i, addr, vdb.phys_addr, (void *)vdb.virt_addr);

	offset = addr - (unsigned long)vdb.phys_addr;

	srcAddr = ((__u8 *)vdb.virt_addr + offset);
	CVI_VC_TRACE("srcAddr = %p\n", srcAddr);

	return srcAddr;
}

#ifdef ARCH_CV182X
static BOOL vdi_is_share_single_es_buf(unsigned long core_idx,
				       unsigned int single_es_buf_size,
				       vdi_info_t **ppVdiSwitch)
{
	unsigned long coreIdxSwitch;
	vdi_info_t *vdiSwitch;
	BOOL bSharingSingleEsBuf = FALSE;

	coreIdxSwitch = (core_idx + 1) % MAX_NUM_VPU_CORE;
	vdiSwitch = vdi_get_vdi_info(coreIdxSwitch);

	// other codec type also enable singleEsBuf flag,
	// as well as with the same es buffer size
	if (vdiSwitch != NULL && vdiSwitch->singleEsBuf &&
	    vdiSwitch->single_es_buf_size == single_es_buf_size) {
		bSharingSingleEsBuf = TRUE;
		*ppVdiSwitch = vdiSwitch;
	}
	CVI_VC_TRACE("bSharingSingleEsBuf %d, vdiSwitch %p\n",
		     bSharingSingleEsBuf, vdiSwitch);
	return bSharingSingleEsBuf;
}

static void vdi_share_single_es_buf(unsigned long core_idx, vdi_info_t *vdi,
				    BOOL *pbReUsedEsBuf)
{
	vdi_info_t *vdiSwitch = NULL;

	if (vdi_is_share_single_es_buf(core_idx, vdi->single_es_buf_size,
				       &vdiSwitch)) {
		int i;

		memcpy(&vdi->vdb_bitstream, &vdiSwitch->vdb_bitstream,
		       sizeof(vpudrv_buffer_t));

		for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
			if (vdi->vpu_buffer_pool[i].inuse == 0) {
				vdi->vpu_buffer_pool_count++;
				vdi->vpu_buffer_pool[i].inuse = 1;
				memcpy(&vdi->vpu_buffer_pool[i].vdb,
				       &vdi->vdb_bitstream,
				       sizeof(vpudrv_buffer_t));
				break;
			}
		}

		*pbReUsedEsBuf = TRUE;
	}
	CVI_VC_INFO("bReUsedEsBuf %d\n", *pbReUsedEsBuf);
}
#endif

int vdi_allocate_single_es_buf_memory(unsigned long core_idx,
				      vpu_buffer_t *vb) // saw not lock
{
	vdi_info_t *vdi;
#if defined(CVI_H26X_USE_ION_MEM)
#if defined(BITSTREAM_ION_CACHED_MEM)
	int bBsStreamCached = 1;
#else
	int bBsStreamCached = 0;
#endif
#endif
#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif
	BOOL bReUsedEsBuf = FALSE;
	char ionName[MAX_VPU_ION_BUFFER_NAME];

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

#ifdef ARCH_CV182X
	vdi_share_single_es_buf(core_idx, vdi, &bReUsedEsBuf);
#endif

	if (vdi->task_num > 1 || bReUsedEsBuf) {
		vb->phys_addr = vdi->vdb_bitstream.phys_addr;
		vb->base = vdi->vdb_bitstream.base;
		vb->virt_addr = vdi->vdb_bitstream.virt_addr;
	} else {
		strncpy(ionName, "singleEsBuf", MAX_VPU_ION_BUFFER_NAME);
		if (VDI_ALLOCATE_MEMORY(core_idx, vb, bBsStreamCached,
					ionName) < 0) {
			CVI_VC_ERR(
				"[VDI] fail to vdi_allocate_single_es_buf_memory size=%d\n",
				vb->size);
			return -1;
		}
		vdi->vdb_bitstream.size = vb->size;
		vdi->vdb_bitstream.phys_addr = vb->phys_addr;
		vdi->vdb_bitstream.base = vb->base;
		vdi->vdb_bitstream.virt_addr = vb->virt_addr;
	}

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p~%p, size=0x%x\n",
		   vb->phys_addr, (void *)vb->virt_addr,
		   (void *)(vb->virt_addr + vb->size), vb->size);

	return 0;
}

void vdi_free_single_es_buf_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	if (vdi->task_num == 1) {
#ifdef ARCH_CV182X
		vdi_info_t *vdiSwitch = NULL;

		if (vdi_is_share_single_es_buf(core_idx,
					       vdi->single_es_buf_size,
					       &vdiSwitch) == FALSE)
#endif
		{
			CVI_VC_MEM(
				"relese physaddr=%llx, virtaddr=%p~%p, size=0x%x\n",
				vb->phys_addr, (void *)vb->virt_addr,
				(void *)(vb->virt_addr + vb->size), vb->size);
			VDI_FREE_MEMORY(core_idx, vb);
		}
		osal_memset(&vdi->vdb_bitstream, 0x00, sizeof(vpudrv_buffer_t));
	}
}

int vdi_attach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t vdb;
	int locked = 0;
	int ret = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return -1;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		ret = -1;
		goto ATTACH_DMA_ERROR;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vb->size;
	vdb.phys_addr = vb->phys_addr;
	vdb.base = vb->base;

	vdb.virt_addr = vb->virt_addr;

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool[i].inuse = 1;
			break;
		}
		if (vdi->vpu_buffer_pool[i].inuse == 0) {
			vdi->vpu_buffer_pool[i].vdb = vdb;
			vdi->vpu_buffer_pool_count++;
			vdi->vpu_buffer_pool[i].inuse = 1;
			break;
		}
	}

	CVI_VC_MEM("physaddr=0x%llx, virtaddr=%p, size=0x%x, index=%d\n",
		   vb->phys_addr, vb->virt_addr, vb->size, i);

ATTACH_DMA_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);

	return ret;
}

int vdi_dettach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	int locked = 0;
	int ret = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return -1;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		ret = -1;
		goto DETCH_DMA_ERROR;
	}

	if (!vb || vb->size == 0) {
		CVI_VC_ERR("null vb or zero vb size\n");
		ret = -1;
		goto DETCH_DMA_ERROR;
	}

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			break;
		}
	}

DETCH_DMA_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);

	return ret;
}

#ifdef CVI_H26X_USE_ION_MEM

int vdi_allocate_ion_memory(unsigned long core_idx, vpu_buffer_t *vb,
			    int is_cached, const char *str)
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t vdb;
	int locked = 0;
	int ret = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("[VDI] lock failed\n");
		return -1;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("[VDI] vdi_get_vdi_info\n");
		ret = -1;
		goto ALLOCATE_ION_ERROR;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vb->size;

	if (sys_ion_alloc_nofd((uint64_t *)&vb->phys_addr, (void **)&vb->virt_addr,
			  (uint8_t *)str, vb->size, is_cached) != 0) {
		CVI_VC_ERR("[VDI] fail to allocate ion memory. size=%d\n",
			   vb->size);
		ret = -1;
		goto ALLOCATE_ION_ERROR;
	}
	vdb.phys_addr = vb->phys_addr;
	vdb.virt_addr = vb->virt_addr;

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 0) {
			vdi->vpu_buffer_pool_count++;
			vdi->vpu_buffer_pool[i].inuse = 1;
			memcpy(&vdi->vpu_buffer_pool[i].vdb, &vdb,
			       sizeof(vpudrv_buffer_t));
			break;
		}
	}

	if (i >= MAX_VPU_BUFFER_POOL) {
		CVI_VC_ERR("[VDI] vpu_buffer_pool out of limitation!\n");
		ret = -1;
		goto ALLOCATE_ION_ERROR;
	}

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p~%p, size=0x%x, cached=%d\n",
		   vb->phys_addr, (void *)vb->virt_addr,
		   (void *)(vb->virt_addr + vb->size), vb->size, is_cached);

ALLOCATE_ION_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);

	return ret;
}

void vdi_free_ion_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t *p_vdb;
	int locked = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		goto FREE_ION_ERROR;
	}

	if (!vb || vb->size == 0)
		goto FREE_ION_ERROR;

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
			p_vdb = &vdi->vpu_buffer_pool[i].vdb;
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			break;
		}
	}

	if (i >= MAX_VPU_BUFFER_POOL) {
		CVI_VC_ERR("[VDI] vpu_buffer_pool out of limitation!\n");
		goto FREE_ION_ERROR;
	}

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p~%p, size=0x%x\n",
		   p_vdb->phys_addr, p_vdb->virt_addr,
		   (void *)(p_vdb->virt_addr + p_vdb->size), p_vdb->size);

	if (!p_vdb->size) {
		CVI_VC_ERR("[VDI] invalid buffer to free address = %p\n",
			   p_vdb->virt_addr);
		goto FREE_ION_ERROR;
	}

	if (sys_ion_free_nofd((uint64_t)vb->phys_addr) != 0) {
		CVI_VC_ERR("[VDI] fail to free ion phys_addr = 0x%llx\n",
			   vb->phys_addr);
		goto FREE_ION_ERROR;
	}

	osal_memset(p_vdb, 0x0, sizeof(vpudrv_buffer_t));
	osal_memset(vb, 0, sizeof(vpu_buffer_t));

FREE_ION_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);
}

int vdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
			     uint32_t u32Len)
{
	return sys_cache_invalidate(u64PhyAddr, phys_to_virt(u64PhyAddr), u32Len);
}

int vdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len)
{
	return sys_cache_flush(u64PhyAddr, phys_to_virt(u64PhyAddr), u32Len);
}

#else //CVI_H26X_USE_ION_MEM

int vdi_allocate_dma_memory(unsigned long core_idx,
			    vpu_buffer_t *vb) // saw not lock
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t vdb;
	int locked = 0;
	int ret = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return -1;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		ret = -1;
		goto ALLOCATE_DMA_ERROR;
	}

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	vdb.size = vb->size;

	if (ioctl(vdi->vpu_fd, VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0) {
		CVI_VC_ERR("[VDI] fail to vdi_allocate_dma_memory size=%d\n",
			   vb->size);
		ret = -1;
		goto ALLOCATE_DMA_ERROR;
	}

	vb->phys_addr = vdb.phys_addr;
	vb->base = vdb.base;

	// map to virtual address
	vdb.virt_addr = MMAP(NULL, vdb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			     vdi->vpu_fd, vdb.phys_addr);
	if ((void *)vdb.virt_addr == MAP_FAILED) {
		CVI_VC_ERR("[VDI]MAP_FAILED %llx size %x\n", vdb.phys_addr,
			   vdb.size);
		memset(vb, 0x00, sizeof(vpu_buffer_t));
		ret = -1;
		goto ALLOCATE_DMA_ERROR;
	}
	vb->virt_addr = vdb.virt_addr;

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 0) {
			memcpy(&vdi->vpu_buffer_pool[i].vdb, &vdb,
			       sizeof(vpudrv_buffer_t));
			vdi->vpu_buffer_pool[i].inuse = 1;
			vdi->vpu_buffer_pool_count++;
			break;
		}
	}

	assert(i < MAX_VPU_BUFFER_POOL);

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p~%p, size=0x%x\n",
		   vb->phys_addr, (void *)vb->virt_addr,
		   (void *)(vb->virt_addr + vb->size), vb->size);

ALLOCATE_DMA_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);

	return ret;
}

void vdi_free_dma_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi;
	int i;
	vpudrv_buffer_t *p_vdb;
	int locked = 0;

	locked = vdi_check_lock_by_me(core_idx);
	if (locked < 0) {
		CVI_VC_ERR("lock failed\n");
		return;
	}

#ifdef SUPPORT_MULTI_CORE_IN_ONE_DRIVER
	core_idx = 0;
#endif

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		goto FREE_DMA_ERROR;
	}

	if (!vb || vb->size == 0) {
		CVI_VC_ERR("null vb or zero vb size\n");
		goto FREE_DMA_ERROR;
	}

	for (i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].vdb.phys_addr == vb->phys_addr) {
			p_vdb = &vdi->vpu_buffer_pool[i].vdb;
			vdi->vpu_buffer_pool[i].inuse = 0;
			vdi->vpu_buffer_pool_count--;
			break;
		}
	}

	assert(i < MAX_VPU_BUFFER_POOL);

	CVI_VC_MEM("physaddr=%llx, virtaddr=%p~%p, size=0x%x\n",
		   p_vdb->phys_addr, (void *)p_vdb->virt_addr,
		   (void *)(p_vdb->virt_addr + p_vdb->size), p_vdb->size);

	if (!p_vdb->size) {
		CVI_VC_ERR("[VDI] invalid buffer to free address = %p\n",
			   p_vdb->virt_addr);
		goto FREE_DMA_ERROR;
	}

	ioctl(vdi->vpu_fd, VDI_IOCTL_FREE_PHYSICALMEMORY, p_vdb);

	if (munmap((void *)p_vdb->virt_addr, p_vdb->size) != 0) {
		CVI_VC_ERR("[VDI] fail to munmap virtial address = %p\n",
			   p_vdb->virt_addr);
	}
	osal_memset(p_vdb, 0x0, sizeof(vpudrv_buffer_t));
	osal_memset(vb, 0x0, sizeof(vpu_buffer_t));

FREE_DMA_ERROR:
	if (locked == 1)
		vdi_unlock(core_idx);
}

#endif

static unsigned int _get_sram_size(vdi_info_t *vdi)
{
	switch (vdi->chip_version) {
	case CHIP_ID_1821A:
		switch (vdi->product_code) {
		case CODA980_CODE:
			return VDI_SRAM_SIZE_CODA9_1821A;
		case WAVE420L_CODE:
			return VDI_SRAM_SIZE_WAVE420L_1821A;
		}
		break;

	case CHIP_ID_1822:
		switch (vdi->product_code) {
		case CODA980_CODE:
			return VDI_SRAM_SIZE_CODA9_1822;
		case WAVE420L_CODE:
			return VDI_SRAM_SIZE_WAVE420L_1822;
		}
		break;

	case CHIP_ID_1835:
		switch (vdi->product_code) {
		case CODA980_CODE:
			return VDI_SRAM_SIZE_CODA9_1835;
		case WAVE420L_CODE:
			return VDI_SRAM_SIZE_WAVE420L_1835;
		}
		break;
	}

	CVI_VC_ERR("chip_version = 0x%x, product_code = 0x%x\n",
		   vdi->chip_version, vdi->product_code);
	return 0;
}

int vdi_get_sram_memory(unsigned long core_idx, vpu_buffer_t *vb)
{
	vdi_info_t *vdi = NULL;
	vpudrv_buffer_t vdb;
	unsigned int sram_size = 0;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (!vb)
		return -1;

	osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));

	sram_size = _get_sram_size(vdi);
	CVI_VC_TRACE("sram_size = 0x%X\n", sram_size);

	if (sram_size > 0) // if we can know the sram address directly in vdi
	// layer, we use it first for sdram address
	{
		vb->phys_addr = VDI_SRAM_BASE_ADDR;
		// base addr to
		// VDI_SRAM_BASE_ADDR.
		vb->size = sram_size;

		return 0;
	}

	return 0;
}

int vdi_set_clock_gate(unsigned long core_idx, int enable)
{
	vdi_info_t *vdi = NULL;
	int ret = 0;
	struct clk_ctrl_info info;

	vdi = vdi_get_vdi_info(core_idx);

	if (!vdi) {
		CVI_VC_WARN("vdi_get_vdi_info\n");
		return -1;
	}

	if (vdi->product_code == WAVE510_CODE ||
	    vdi->product_code == WAVE512_CODE ||
	    vdi->product_code == WAVE515_CODE) {
		CVI_VC_ERR("product code error\n");
		return 0;
	}

	// if clk is already on, return
	if (enable && vdi_get_clock_gate(core_idx) == 1)
		return 1;

	// if clk is already off, return
	if (!enable && vdi_get_clock_gate(core_idx) == 0)
		return 1;

	vdi->clock_state = enable;

	info.core_idx = core_idx;
	info.enable = enable;

	ret = vpu_set_clock_gate_ext((void *)&info);
	return ret;
}

int vdi_get_clock_gate(unsigned long core_idx)
{
	vdi_info_t *vdi;
	int ret;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	ret = vdi->clock_state;
	return ret;
}

int vdi_wait_bus_busy(unsigned long core_idx, int timeout,
		      unsigned int gdi_busy_flag)
{
	Int64 elapse, cur;
	vdi_timeval_t tv;
	vdi_info_t *vdi;

	vdi = s_vdi_info[core_idx];

	cviGetSysTime(&tv);
	elapse = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	while (1) {
		if (vdi->product_code == WAVE420L_CODE) {
			if (vdi_fio_read_register(core_idx, gdi_busy_flag) ==
			    0) {
				break;
			}
		} else if (vdi->product_code == WAVE520_CODE) {
			if (vdi_fio_read_register(core_idx, gdi_busy_flag) ==
			    0x3f)
				break;
		} else if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
			if (vdi_fio_read_register(core_idx, gdi_busy_flag) ==
			    0x738)
				break;
		} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
			if (vdi_read_register(core_idx, gdi_busy_flag) == 0x77)
				break;
		} else {
			CVI_VC_ERR("Unknown product id : %08x\n",
				   vdi->product_code);
			return -1;
		}

		if (timeout > 0) {
			cviGetSysTime(&tv);
			cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			if ((cur - elapse) > timeout) {
				CVI_VC_ERR(
					"[VDI] vdi_wait_bus_busy timeout, PC=0x%x\n",
					vdi_read_register(core_idx, 0x018));
				return -1;
			}
		}
	}
	return 0;
}

int vdi_wait_vpu_busy(unsigned long core_idx, int timeout,
		      unsigned int addr_bit_busy_flag)
{
	Int64 elapse, cur;
	vdi_timeval_t tv;
	Uint32 pc;
	Uint32 code, normalReg = TRUE;

	cviGetSysTime(&tv);
	elapse = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	code = vdi_read_register(core_idx, VPU_PRODUCT_CODE_REGISTER);
	/* read product code */
	if (PRODUCT_CODE_W_SERIES(code)) {
		pc = W4_VCPU_CUR_PC;
		if (addr_bit_busy_flag & 0x8000)
			normalReg = FALSE;
	} else if (PRODUCT_CODE_NOT_W_SERIES(code)) {
		pc = BIT_CUR_PC;
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", code);
		return -1;
	}

	while (1) {
		if (normalReg == TRUE) {
			if (vdi_read_register(core_idx, addr_bit_busy_flag) ==
			    0)
				break;
		} else {
			if (vdi_fio_read_register(core_idx,
						  addr_bit_busy_flag) == 0)
				break;
		}

		if (timeout > 0) {
			cviGetSysTime(&tv);
			cur = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			if ((cur - elapse) > timeout) {
				Uint32 index;
				for (index = 0; index < 50; index++) {
					CVI_VC_ERR("[VDI] timeout, PC=0x%x\n",
						   vdi_read_register(core_idx,
								     pc));
				}
				return -1;
			}
		}
	}
	return 0;
}

int vdi_wait_interrupt(unsigned long coreIdx, int timeout,
		       uint64_t *pu64TimeStamp)
{
	int intr_reason = 0;
	int ret;
	vdi_info_t *vdi;
	vpudrv_intr_info_t intr_info;

	vdi = vdi_get_vdi_info(coreIdx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

#ifdef SUPPORT_INTERRUPT
	intr_info.timeout = timeout;
	intr_info.intr_reason = 0;
	intr_info.coreIdx = coreIdx;
	ret = vpu_wait_interrupt((void *)&intr_info);
	if (ret != 0)
		return -1;
	intr_reason = intr_info.intr_reason;
	if (pu64TimeStamp) {
		*pu64TimeStamp = ((long int)intr_info.intr_tv_sec * 1000000) +
				 ((long int)intr_info.intr_tv_nsec / 1000);
	}
#else
	vdi_timeval_t tv;
	Uint32 intrStatusReg;
	Uint32 pc;
	Int32 startTime, endTime, elaspedTime;

	UNREFERENCED_PARAMETER(intr_info);

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		pc = W4_VCPU_CUR_PC;
		intrStatusReg = W4_VPU_VPU_INT_STS;
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		pc = BIT_CUR_PC;
		intrStatusReg = BIT_INT_STS;
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return -1;
	}

	cviGetSysTime(&tv);
	startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	while (TRUE) {
		if (vdi_read_register(coreIdx, intrStatusReg)) {
			intr_reason =
				vdi_read_register(coreIdx, addr_bit_int_reason);
			if (intr_reason)
				break;
		}

		cviGetSysTime(&tv);
		endTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		if (timeout > 0 && (endTime - startTime) >= timeout) {
			return -1;
		}
	}
#endif

	return intr_reason;
}

#ifdef DBG_CNM
static int read_pinfo_buffer(int core_idx, int addr)
{
	int ack;
	int rdata;
#define VDI_LOG_GDI_PINFO_ADDR (0x1068)
#define VDI_LOG_GDI_PINFO_REQ (0x1060)
#define VDI_LOG_GDI_PINFO_ACK (0x1064)
#define VDI_LOG_GDI_PINFO_DATA (0x106c)
	//------------------------------------------
	// read pinfo - indirect read
	// 1. set read addr     (GDI_PINFO_ADDR)
	// 2. send req          (GDI_PINFO_REQ)
	// 3. wait until ack==1 (GDI_PINFO_ACK)
	// 4. read data         (GDI_PINFO_DATA)
	//------------------------------------------
	vdi_write_register(core_idx, VDI_LOG_GDI_PINFO_ADDR, addr);
	vdi_write_register(core_idx, VDI_LOG_GDI_PINFO_REQ, 1);

	ack = 0;
	while (ack == 0) {
		ack = vdi_read_register(core_idx, VDI_LOG_GDI_PINFO_ACK);
	}

	rdata = vdi_read_register(core_idx, VDI_LOG_GDI_PINFO_DATA);

	// printf("[READ PINFO] ADDR[%x], DATA[%x]", addr, rdata);
	return rdata;
}

enum { VDI_PRODUCT_ID_980, VDI_PRODUCT_ID_960 };

static void printf_gdi_info(int core_idx, int num, int reset)
{
	int i = 0;
	int bus_info_addr = 0;
	int tmp = 0;
	int val = 0;
	int productId = 0;

	val = vdi_read_register(core_idx, VPU_PRODUCT_CODE_REGISTER);
	if ((val & 0xff00) == 0x3200)
		val = 0x3200;

	if (PRODUCT_CODE_W_SERIES(val)) {
		return;
	} else if (PRODUCT_CODE_NOT_W_SERIES(val)) {
		if (val == CODA960_CODE || val == BODA950_CODE)
			productId = VDI_PRODUCT_ID_960;
		else if (val == CODA980_CODE || val == WAVE320_CODE)
			productId = VDI_PRODUCT_ID_980;
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", val);
		return;
	}

	if (productId == VDI_PRODUCT_ID_980)
		CVI_VC_INFO("\n**GDI information for GDI_20\n");
	else
		CVI_VC_INFO("\n**GDI information for GDI_10\n");

	for (i = 0; i < num; i++) {
#define VDI_LOG_GDI_INFO_CONTROL 0x1400
		if (productId == VDI_PRODUCT_ID_980)
			bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i * (0x20);
		else
			bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i * 0x14;
		if (reset) {
			vdi_write_register(core_idx, bus_info_addr, 0x00);
			bus_info_addr += 4;
			vdi_write_register(core_idx, bus_info_addr, 0x00);
			bus_info_addr += 4;
			vdi_write_register(core_idx, bus_info_addr, 0x00);
			bus_info_addr += 4;
			vdi_write_register(core_idx, bus_info_addr, 0x00);
			bus_info_addr += 4;
			vdi_write_register(core_idx, bus_info_addr, 0x00);

			if (productId == VDI_PRODUCT_ID_980) {
				bus_info_addr += 4;
				vdi_write_register(core_idx, bus_info_addr,
						   0x00);

				bus_info_addr += 4;
				vdi_write_register(core_idx, bus_info_addr,
						   0x00);

				bus_info_addr += 4;
				vdi_write_register(core_idx, bus_info_addr,
						   0x00);
			}
		} else {
			CVI_VC_INFO("index = %02d", i);

			tmp = read_pinfo_buffer(core_idx,
						bus_info_addr); // TiledEn<<20
			// ,GdiFormat<<17,IntlvCbCr,<<16
			// GdiYuvBufStride
			CVI_VC_INFO(" control = 0x%08x", tmp);

			bus_info_addr += 4;
			tmp = read_pinfo_buffer(core_idx, bus_info_addr);
			CVI_VC_INFO(" pic_size = 0x%08x", tmp);

			bus_info_addr += 4;
			tmp = read_pinfo_buffer(core_idx, bus_info_addr);
			CVI_VC_INFO(" y-top = 0x%08x", tmp);

			bus_info_addr += 4;
			tmp = read_pinfo_buffer(core_idx, bus_info_addr);
			CVI_VC_INFO(" cb-top = 0x%08x", tmp);

			bus_info_addr += 4;
			tmp = read_pinfo_buffer(core_idx, bus_info_addr);
			CVI_VC_INFO(" cr-top = 0x%08x", tmp);
			if (productId == VDI_PRODUCT_ID_980) {
				bus_info_addr += 4;
				tmp = read_pinfo_buffer(core_idx,
							bus_info_addr);
				CVI_VC_INFO(" y-bot = 0x%08x", tmp);

				bus_info_addr += 4;
				tmp = read_pinfo_buffer(core_idx,
							bus_info_addr);
				CVI_VC_INFO(" cb-bot = 0x%08x", tmp);

				bus_info_addr += 4;
				tmp = read_pinfo_buffer(core_idx,
							bus_info_addr);
				CVI_VC_INFO(" cr-bot = 0x%08x", tmp);
			}
			CVI_VC_INFO("\n");
		}
	}
}
#endif

#ifdef ENABLE_CNM_DEBUG_MSG
void vdi_print_vpu_status(unsigned long coreIdx)
{
	unsigned int product_code;

	product_code = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);

	if (PRODUCT_CODE_W_SERIES(product_code)) {
		unsigned long rd, wr;
		unsigned int tq, ip, mc, lf;
		unsigned int avail_cu, avail_tu, avail_tc, avail_lf, avail_ip;
		unsigned int ctu_fsm, nb_fsm, cabac_fsm, cu_info, mvp_fsm,
			tc_busy, lf_fsm, bs_data, bbusy, fv;
		unsigned int reg_val;
		unsigned int index;
		unsigned int vcpu_reg[31] = {
			0,
		};

		pr_err("-------------------------------------------------------------------------------\n");
		pr_err("------                            VCPU STATUS                             -----\n");
		pr_err("-------------------------------------------------------------------------------\n");
		rd = vdi_remap_memory_address(
			coreIdx, VpuReadReg(coreIdx, W4_BS_RD_PTR));
		wr = vdi_remap_memory_address(
			coreIdx, VpuReadReg(coreIdx, W4_BS_WR_PTR));
		pr_err("RD_PTR: 0x%lX WR_PTR: 0x%lX BS_OPT: 0x%08x BS_PARAM: 0x%08x\n",
		       rd, wr, VpuReadReg(coreIdx, W4_BS_OPTION),
		       VpuReadReg(coreIdx, W4_BS_PARAM));

		// --------- VCPU register Dump
		pr_err("[+] VCPU REG Dump\n");
		for (index = 0; index < 25; index++) {
			VpuWriteReg(coreIdx, 0x14, (1 << 9) | (index & 0xff));
			vcpu_reg[index] = VpuReadReg(coreIdx, 0x1c);

			if (index < 16) {
				pr_err("0x%08x\t", vcpu_reg[index]);
				if ((index % 4) == 3)
					pr_err("\n");
			} else {
				switch (index) {
				case 16:
					pr_err("CR0: 0x%08x\t",
					       vcpu_reg[index]);
					break;
				case 17:
					pr_err("CR1: 0x%08x\n",
					       vcpu_reg[index]);
					break;
				case 18:
					pr_err("ML:  0x%08x\t",
					       vcpu_reg[index]);
					break;
				case 19:
					pr_err("MH:  0x%08x\n",
					       vcpu_reg[index]);
					break;
				case 21:
					pr_err("LR:  0x%08x\n",
					       vcpu_reg[index]);
					break;
				case 22:
					pr_err("PC:  0x%08x\n",
					       vcpu_reg[index]);
					break;
				case 23:
					pr_err("SR:  0x%08x\n",
					       vcpu_reg[index]);
					break;
				case 24:
					pr_err("SSP: 0x%08x\n",
					       vcpu_reg[index]);
					break;
				}
			}
		}
		pr_err("[-] VCPU REG Dump\n");
		// --------- BIT register Dump
		pr_err("[+] BPU REG Dump\n");
		pr_err("BITPC = 0x%08x\n",
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x8000 + 0x18)));
		pr_err("BIT START=0x%08x, BIT END=0x%08x\n",
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x8000 + 0x11c)),
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x8000 + 0x120)));
		if (product_code == WAVE410_CODE)
			pr_err("BIT COMMAND 0x%x\n",
			       vdi_fio_read_register(
				       coreIdx,
				       (W4_REG_BASE + 0x8000 + 0x100)));
		if (product_code == WAVE4102_CODE ||
		    product_code == WAVE510_CODE)
			pr_err("BIT COMMAND 0x%x\n",
			       vdi_fio_read_register(
				       coreIdx,
				       (W4_REG_BASE + 0x8000 + 0x1FC)));

		pr_err("CODE_BASE			%x\n",
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x7000 + 0x18)));
		pr_err("VCORE_REINIT_FLAG	%x\n",
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x7000 + 0x0C)));

		// --------- BIT HEVC Status Dump
		ctu_fsm = vdi_fio_read_register(coreIdx,
						(W4_REG_BASE + 0x8000 + 0x48));
		nb_fsm = vdi_fio_read_register(coreIdx,
					       (W4_REG_BASE + 0x8000 + 0x4c));
		cabac_fsm = vdi_fio_read_register(
			coreIdx, (W4_REG_BASE + 0x8000 + 0x50));
		cu_info = vdi_fio_read_register(coreIdx,
						(W4_REG_BASE + 0x8000 + 0x54));
		mvp_fsm = vdi_fio_read_register(coreIdx,
						(W4_REG_BASE + 0x8000 + 0x58));
		tc_busy = vdi_fio_read_register(coreIdx,
						(W4_REG_BASE + 0x8000 + 0x5c));
		lf_fsm = vdi_fio_read_register(coreIdx,
					       (W4_REG_BASE + 0x8000 + 0x60));
		bs_data = vdi_fio_read_register(coreIdx,
						(W4_REG_BASE + 0x8000 + 0x64));
		bbusy = vdi_fio_read_register(coreIdx,
					      (W4_REG_BASE + 0x8000 + 0x68));
		fv = vdi_fio_read_register(coreIdx,
					   (W4_REG_BASE + 0x8000 + 0x6C));

		pr_err("[DEBUG-BPUHEVC] CTU_X: %4d, CTU_Y: %4d\n",
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x8000 + 0x40)),
		       vdi_fio_read_register(coreIdx,
					     (W4_REG_BASE + 0x8000 + 0x44)));
		pr_err("[DEBUG-BPUHEVC] CTU_FSM>   Main: 0x%02x, FIFO: 0x%1x, NB: 0x%02x, DBK: 0x%1x\n",
		       ((ctu_fsm >> 24) & 0xff), ((ctu_fsm >> 16) & 0xff),
		       ((ctu_fsm >> 8) & 0xff), (ctu_fsm & 0xff));
		pr_err("[DEBUG-BPUHEVC] NB_FSM:	0x%02x\n",
		       nb_fsm & 0xff);
		pr_err("[DEBUG-BPUHEVC] CABAC_FSM> SAO: 0x%02x, CU: 0x%02x, PU: 0x%02x, TU: 0x%02x, EOS: 0x%02x\n",
		       ((cabac_fsm >> 25) & 0x3f), ((cabac_fsm >> 19) & 0x3f),
		       ((cabac_fsm >> 13) & 0x3f), ((cabac_fsm >> 6) & 0x7f),
		       (cabac_fsm & 0x3f));
		pr_err("[DEBUG-BPUHEVC] CU_INFO value = 0x%04x\n\t\t(l2cb: 0x%1x, cux: %1d, cuy; %1d, pred: %1d, pcm: %1d, wr_done: %1d, par_done: %1d, nbw_done: %1d, dec_run: %1d)\n",
		       cu_info, ((cu_info >> 16) & 0x3),
		       ((cu_info >> 13) & 0x7), ((cu_info >> 10) & 0x7),
		       ((cu_info >> 9) & 0x3), ((cu_info >> 8) & 0x1),
		       ((cu_info >> 6) & 0x3), ((cu_info >> 4) & 0x3),
		       ((cu_info >> 2) & 0x3), (cu_info & 0x3));
		pr_err("[DEBUG-BPUHEVC] MVP_FSM> 0x%02x\n",
		       mvp_fsm & 0xf);
		pr_err("[DEBUG-BPUHEVC] TC_BUSY> tc_dec_busy: %1d, tc_fifo_busy: 0x%02x\n",
		       ((tc_busy >> 3) & 0x1), (tc_busy & 0x7));
		pr_err("[DEBUG-BPUHEVC] LF_FSM>  SAO: 0x%1x, LF: 0x%1x\n",
		       ((lf_fsm >> 4) & 0xf), (lf_fsm & 0xf));
		pr_err("[DEBUG-BPUHEVC] BS_DATA> ExpEnd=%1d, bs_valid: 0x%03x, bs_data: 0x%03x\n",
		       ((bs_data >> 31) & 0x1), ((bs_data >> 16) & 0xfff),
		       (bs_data & 0xfff));
		pr_err("[DEBUG-BPUHEVC] BUS_BUSY> mib_wreq_done: %1d, mib_busy: %1d, sdma_bus: %1d\n",
		       ((bbusy >> 2) & 0x1), ((bbusy >> 1) & 0x1),
		       (bbusy & 0x1));
		pr_err("[DEBUG-BPUHEVC] FIFO_VALID> cu: %1d, tu: %1d, iptu: %1d, lf: %1d, coff: %1d\n\n",
		       ((fv >> 4) & 0x1), ((fv >> 3) & 0x1), ((fv >> 2) & 0x1),
		       ((fv >> 1) & 0x1), (fv & 0x1));
		pr_err("[-] BPU REG Dump\n");

		// --------- VCE register Dump
		pr_err("[+] VCE REG Dump\n");
		tq = read_vce_register(0, 0, 0xd0);
		ip = read_vce_register(0, 0, 0xd4);
		mc = read_vce_register(0, 0, 0xd8);
		lf = read_vce_register(0, 0, 0xdc);
		avail_cu = (read_vce_register(0, 0, 0x11C) >> 16) -
			   (read_vce_register(0, 0, 0x110) >> 16);
		avail_tu = (read_vce_register(0, 0, 0x11C) & 0xFFFF) -
			   (read_vce_register(0, 0, 0x110) & 0xFFFF);
		avail_tc = (read_vce_register(0, 0, 0x120) >> 16) -
			   (read_vce_register(0, 0, 0x114) >> 16);
		avail_lf = (read_vce_register(0, 0, 0x120) & 0xFFFF) -
			   (read_vce_register(0, 0, 0x114) & 0xFFFF);
		avail_ip = (read_vce_register(0, 0, 0x124) >> 16) -
			   (read_vce_register(0, 0, 0x118) >> 16);
		pr_err("       TQ           IP             MC            LF        GDI_EMPTY         ROOM\n");
		pr_err("------------------------------------------------------------------------------------------------------------\n");
		pr_err(
		       "| %d %04d %04d | %d %04d %04d |  %d %04d %04d | %d %04d %04d | 0x%08x | CU(%d) TU(%d) TC(%d) LF(%d) IP(%d)\n",
		       (tq >> 22) & 0x07, (tq >> 11) & 0x3ff, tq & 0x3ff,
		       (ip >> 22) & 0x07, (ip >> 11) & 0x3ff, ip & 0x3ff,
		       (mc >> 22) & 0x07, (mc >> 11) & 0x3ff, mc & 0x3ff,
		       (lf >> 22) & 0x07, (lf >> 11) & 0x3ff, lf & 0x3ff,
		       vdi_fio_read_register(0, 0x88f4), /* GDI empty */
		       avail_cu, avail_tu, avail_tc, avail_lf, avail_ip);
		/* CU/TU Queue count */
		reg_val = read_vce_register(0, 0, 0x12C);
		pr_err("[DCIDEBUG] QUEUE COUNT: CU(%5d) TU(%5d) ",
		       (reg_val >> 16) & 0xffff, reg_val & 0xffff);
		reg_val = read_vce_register(0, 0, 0x1A0);
		pr_err("TC(%5d) IP(%5d) ", (reg_val >> 16) & 0xffff,
		       reg_val & 0xffff);
		reg_val = read_vce_register(0, 0, 0x1A4);
		pr_err("LF(%5d)\n", (reg_val >> 16) & 0xffff);
		pr_err("VALID SIGNAL : CU0(%d)  CU1(%d)  CU2(%d) TU(%d) TC(%d) IP(%5d) LF(%5d)\nDCI_FALSE_RUN(%d) VCE_RESET(%d) CORE_INIT(%d) SET_RUN_CTU(%d)\n",
		       (reg_val >> 6) & 1, (reg_val >> 5) & 1,
		       (reg_val >> 4) & 1, (reg_val >> 3) & 1,
		       (reg_val >> 2) & 1, (reg_val >> 1) & 1,
		       (reg_val >> 0) & 1, (reg_val >> 10) & 1,
		       (reg_val >> 9) & 1, (reg_val >> 8) & 1,
		       (reg_val >> 7) & 1);

		pr_err(
		       "State TQ: 0x%08x IP: 0x%08x MC: 0x%08x LF: 0x%08x\n",
		       read_vce_register(0, 0, 0xd0),
		       read_vce_register(0, 0, 0xd4),
		       read_vce_register(0, 0, 0xd8),
		       read_vce_register(0, 0, 0xdc));
		pr_err("BWB[1]: RESPONSE_CNT(0x%08x) INFO(0x%08x)\n",
		       read_vce_register(0, 0, 0x194),
		       read_vce_register(0, 0, 0x198));
		pr_err("BWB[2]: RESPONSE_CNT(0x%08x) INFO(0x%08x)\n",
		       read_vce_register(0, 0, 0x194),
		       read_vce_register(0, 0, 0x198));
		pr_err("DCI INFO\n");
		pr_err("READ_CNT_0 : 0x%08x\n",
		       read_vce_register(0, 0, 0x110));
		pr_err("READ_CNT_1 : 0x%08x\n",
		       read_vce_register(0, 0, 0x114));
		pr_err("READ_CNT_2 : 0x%08x\n",
		       read_vce_register(0, 0, 0x118));
		pr_err("WRITE_CNT_0: 0x%08x\n",
		       read_vce_register(0, 0, 0x11c));
		pr_err("WRITE_CNT_1: 0x%08x\n",
		       read_vce_register(0, 0, 0x120));
		pr_err("WRITE_CNT_2: 0x%08x\n",
		       read_vce_register(0, 0, 0x124));
		reg_val = read_vce_register(0, 0, 0x128);
		pr_err("LF_DEBUG_PT: 0x%08x\n", reg_val & 0xffffffff);
		pr_err("cur_main_state %2d, r_lf_pic_deblock_disable %1d, r_lf_pic_sao_disable %1d\n",
		       (reg_val >> 16) & 0x1f, (reg_val >> 15) & 0x1,
		       (reg_val >> 14) & 0x1);
		pr_err("para_load_done %1d, i_rdma_ack_wait %1d, i_sao_intl_col_done %1d,i_sao_outbuf_full %1d\n",
		       (reg_val >> 13) & 0x1, (reg_val >> 12) & 0x1,
		       (reg_val >> 11) & 0x1, (reg_val >> 10) & 0x1);
		pr_err("lf_sub_done %1d, i_wdma_ack_wait %1d, lf_all_sub_done %1d, cur_ycbcr %1d, sub8x8_done %2d\n",
		       (reg_val >> 9) & 0x1, (reg_val >> 8) & 0x1,
		       (reg_val >> 6) & 0x1, (reg_val >> 4) & 0x1,
		       reg_val & 0xf);
		pr_err("[-] VCE REG Dump\n");
		pr_err("[-] VCE REG Dump\n");

		pr_err("-------------------------------------------------------------------------------\n");
	} else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", product_code);
	}
}

void vdi_make_log(unsigned long core_idx, const char *str, int step)
{
	int val;

	val = VpuReadReg(core_idx, W4_INST_INDEX);
	val &= 0xffff;
	if (step == 1)
		CVI_VC_INFO("\n**%s start(%d)\n", str, val);
	else if (step == 2) //
		CVI_VC_INFO("\n**%s timeout(%d)\n", str, val);
	else
		CVI_VC_INFO("\n**%s end(%d)\n", str, val);
}

void vdi_log(unsigned long core_idx, int cmd, int step)
{
	vdi_info_t *vdi;
	int i;

	// BIT_RUN command
	enum {
		SEQ_INIT = 1,
		SEQ_END = 2,
		PIC_RUN = 3,
		SET_FRAME_BUF = 4,
		ENCODE_HEADER = 5,
		ENC_PARA_SET = 6,
		DEC_PARA_SET = 7,
		DEC_BUF_FLUSH = 8,
		RC_CHANGE_PARAMETER = 9,
		VPU_SLEEP = 10,
		VPU_WAKE = 11,
		ENC_ROI_INIT = 12,
		FIRMWARE_GET = 0xf,
		VPU_RESET = 0x10,
	};

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return;
	}

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		switch (cmd) {
		case INIT_VPU:
			vdi_make_log(core_idx, "INIT_VPU", step);
			break;
		case DEC_PIC_HDR: // SET_PARAM for ENC
			vdi_make_log(core_idx,
				     "SET_PARAM(ENC), DEC_PIC_HDR(DEC)", step);
			break;
		case FINI_SEQ:
			vdi_make_log(core_idx, "FINI_SEQ", step);
			break;
		case DEC_PIC: // ENC_PIC for ENC
			vdi_make_log(core_idx, "DEC_PIC, ENC_PIC", step);
			break;
		case SET_FRAMEBUF:
			vdi_make_log(core_idx, "SET_FRAMEBUF", step);
			break;
		case FLUSH_DECODER:
			vdi_make_log(core_idx, "FLUSH_DECODER", step);
			break;
		case GET_FW_VERSION:
			vdi_make_log(core_idx, "GET_FW_VERSION", step);
			break;
		case QUERY_DECODER:
			vdi_make_log(core_idx, "QUERY_DECODER", step);
			break;
		case SLEEP_VPU:
			vdi_make_log(core_idx, "SLEEP_VPU", step);
			break;
		case CREATE_INSTANCE:
			vdi_make_log(core_idx, "CREATE_INSTANCE", step);
			break;
		case RESET_VPU:
			vdi_make_log(core_idx, "RESET_VPU", step);
			break;
		default:
			vdi_make_log(core_idx, "ANY_CMD", step);
			break;
		}
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		switch (cmd) {
		case SEQ_INIT:
			vdi_make_log(core_idx, "SEQ_INIT", step);
			break;
		case SEQ_END:
			vdi_make_log(core_idx, "SEQ_END", step);
			break;
		case PIC_RUN:
			vdi_make_log(core_idx, "PIC_RUN", step);
			break;
		case SET_FRAME_BUF:
			vdi_make_log(core_idx, "SET_FRAME_BUF", step);
			break;
		case ENCODE_HEADER:
			vdi_make_log(core_idx, "ENCODE_HEADER", step);
			break;
		case RC_CHANGE_PARAMETER:
			vdi_make_log(core_idx, "RC_CHANGE_PARAMETER", step);
			break;
		case DEC_BUF_FLUSH:
			vdi_make_log(core_idx, "DEC_BUF_FLUSH", step);
			break;
		case FIRMWARE_GET:
			vdi_make_log(core_idx, "FIRMWARE_GET", step);
			break;
		case VPU_RESET:
			vdi_make_log(core_idx, "VPU_RESET", step);
			break;
		case ENC_PARA_SET:
			vdi_make_log(core_idx, "ENC_PARA_SET", step);
			break;
		case DEC_PARA_SET:
			vdi_make_log(core_idx, "DEC_PARA_SET", step);
			break;
		default:
			vdi_make_log(core_idx, "ANY_CMD", step);
			break;
		}
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return;
	}

	for (i = 0; i < 0x200; i = i + 16) {
		CVI_VC_INFO("0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
			    vdi_read_register(core_idx, i),
			    vdi_read_register(core_idx, i + 4),
			    vdi_read_register(core_idx, i + 8),
			    vdi_read_register(core_idx, i + 0xc));
	}

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		// WAVE4xx
		if (cmd == INIT_VPU || cmd == VPU_RESET ||
		    cmd == CREATE_INSTANCE) {
			vdi_print_vpu_status(core_idx);
		}
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		// if ((cmd == PIC_RUN && step== 0) || cmd == VPU_RESET)
		if (cmd == VPU_RESET) {
#ifdef DBG_CNM
			printf_gdi_info(core_idx, 32, 0);
#endif

#define VDI_LOG_MBC_BUSY 0x0440
#define VDI_LOG_MC_BASE 0x0C00
#define VDI_LOG_MC_BUSY 0x0C04
#define VDI_LOG_GDI_BUS_STATUS (0x10F4)
#define VDI_LOG_ROT_SRC_IDX (0x400 + 0x10C)
#define VDI_LOG_ROT_DST_IDX (0x400 + 0x110)

			CVI_VC_INFO("MBC_BUSY = %x\n",
				    vdi_read_register(core_idx,
						      VDI_LOG_MBC_BUSY));
			CVI_VC_INFO("MC_BUSY = %x\n",
				    vdi_read_register(core_idx,
						      VDI_LOG_MC_BUSY));
			CVI_VC_INFO(
				"MC_MB_XY_DONE=(y:%d, x:%d)\n",
				(vdi_read_register(core_idx, VDI_LOG_MC_BASE) >>
				 20) & 0x3F,
				(vdi_read_register(core_idx, VDI_LOG_MC_BASE) >>
				 26) & 0x3F);
			CVI_VC_INFO("GDI_BUS_STATUS = %x\n",
				    vdi_read_register(core_idx,
						      VDI_LOG_GDI_BUS_STATUS));

			CVI_VC_INFO("ROT_SRC_IDX = %x\n",
				    vdi_read_register(core_idx,
						      VDI_LOG_ROT_SRC_IDX));
			CVI_VC_INFO("ROT_DST_IDX = %x\n",
				    vdi_read_register(core_idx,
						      VDI_LOG_ROT_DST_IDX));

			CVI_VC_INFO("P_MC_PIC_INDEX_0 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x200));
			CVI_VC_INFO("P_MC_PIC_INDEX_1 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x20c));
			CVI_VC_INFO("P_MC_PIC_INDEX_2 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x218));
			CVI_VC_INFO("P_MC_PIC_INDEX_3 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x230));
			CVI_VC_INFO("P_MC_PIC_INDEX_3 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x23C));
			CVI_VC_INFO("P_MC_PIC_INDEX_4 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x248));
			CVI_VC_INFO("P_MC_PIC_INDEX_5 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x254));
			CVI_VC_INFO("P_MC_PIC_INDEX_6 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x260));
			CVI_VC_INFO("P_MC_PIC_INDEX_7 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x26C));
			CVI_VC_INFO("P_MC_PIC_INDEX_8 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x278));
			CVI_VC_INFO("P_MC_PIC_INDEX_9 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x284));
			CVI_VC_INFO("P_MC_PIC_INDEX_a = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x290));
			CVI_VC_INFO("P_MC_PIC_INDEX_b = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x29C));
			CVI_VC_INFO("P_MC_PIC_INDEX_c = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x2A8));
			CVI_VC_INFO("P_MC_PIC_INDEX_d = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x2B4));

			CVI_VC_INFO("P_MC_PICIDX_0 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x028));
			CVI_VC_INFO("P_MC_PICIDX_1 = %x\n",
				    vdi_read_register(core_idx,
						      MC_BASE + 0x02C));
		}
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return;
	}
}
#endif

static void byte_swap(unsigned char *data, int len)
{
	Uint8 temp;
	Int32 i;

	for (i = 0; i < len; i += 2) {
		temp = data[i];
		data[i] = data[i + 1];
		data[i + 1] = temp;
	}
}

static void word_swap(unsigned char *data, int len)
{
	Uint16 temp;
	Uint16 *ptr = (Uint16 *)data;
	Int32 i, size = len / sizeof(Uint16);

	for (i = 0; i < size; i += 2) {
		temp = ptr[i];
		ptr[i] = ptr[i + 1];
		ptr[i + 1] = temp;
	}
}

static void dword_swap(unsigned char *data, int len)
{
	Uint32 temp;
	Uint32 *ptr = (Uint32 *)data;
	Int32 i, size = len / sizeof(Uint32);

	for (i = 0; i < size; i += 2) {
		temp = ptr[i];
		ptr[i] = ptr[i + 1];
		ptr[i + 1] = temp;
	}
}

static void lword_swap(unsigned char *data, int len)
{
	Uint64 temp;
	Uint64 *ptr = (Uint64 *)data;
	Int32 i, size = len / sizeof(Uint64);

	for (i = 0; i < size; i += 2) {
		temp = ptr[i];
		ptr[i] = ptr[i + 1];
		ptr[i + 1] = temp;
	}
}

#ifdef REDUNDENT_CODE
int vdi_get_system_endian(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		return VDI_128BIT_BUS_SYSTEM_ENDIAN;
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		return VDI_SYSTEM_ENDIAN;
	}

	CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
	return -1;
}
#endif

int vdi_convert_endian(unsigned long core_idx, unsigned int endian)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		switch (endian) {
		case VDI_LITTLE_ENDIAN:
			endian = 0x00;
			break;
		case VDI_BIG_ENDIAN:
			endian = 0x0f;
			break;
		case VDI_32BIT_LITTLE_ENDIAN:
			endian = 0x04;
			break;
		case VDI_32BIT_BIG_ENDIAN:
			endian = 0x03;
			break;
		}
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return -1;
	}

	return (endian & 0x0f);
}

PhysicalAddress vdi_remap_memory_address(unsigned long coreIdx,
					 PhysicalAddress address)
{
	vdi_info_t *vdi;

	vdi = s_vdi_info[coreIdx];

	switch (vdi->chip_version) {
	case CHIP_ID_1835:
		return VPU_TO_64BIT(address);

	case CHIP_ID_1821A:
	case CHIP_ID_1822:
		return VPU_TO_32BIT(address);

	default:
		CVI_VC_ERR("Unsupported CHIP 0x%x\n", vdi->chip_version);
		break;
	}

	return 0;
}

static Uint32 convert_endian_coda9_to_wave4(Uint32 endian)
{
	Uint32 converted_endian = endian;
	switch (endian) {
	case VDI_LITTLE_ENDIAN:
		converted_endian = 0;
		break;
	case VDI_BIG_ENDIAN:
		converted_endian = 7;
		break;
	case VDI_32BIT_LITTLE_ENDIAN:
		converted_endian = 4;
		break;
	case VDI_32BIT_BIG_ENDIAN:
		converted_endian = 3;
		break;
	}
	return converted_endian;
}

int swap_endian(unsigned long core_idx, unsigned char *data, int len,
		int endian)
{
	vdi_info_t *vdi;
	int changes;
	int sys_endian;
	BOOL byteChange, wordChange, dwordChange, lwordChange;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_ERR("vdi_get_vdi_info\n");
		return -1;
	}

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
		sys_endian = VDI_128BIT_BUS_SYSTEM_ENDIAN;
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		sys_endian = VDI_SYSTEM_ENDIAN;
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return -1;
	}

	endian = vdi_convert_endian(core_idx, endian);
	sys_endian = vdi_convert_endian(core_idx, sys_endian);
	if (endian == sys_endian)
		return 0;

	if (PRODUCT_CODE_W_SERIES(vdi->product_code)) {
	} else if (PRODUCT_CODE_NOT_W_SERIES(vdi->product_code)) {
		endian = convert_endian_coda9_to_wave4(endian);
		sys_endian = convert_endian_coda9_to_wave4(sys_endian);
	} else {
		CVI_VC_ERR("Unknown product id : %08x\n", vdi->product_code);
		return -1;
	}

	changes = endian ^ sys_endian;
	byteChange = changes & 0x01;
	wordChange = ((changes & 0x02) == 0x02);
	dwordChange = ((changes & 0x04) == 0x04);
	lwordChange = ((changes & 0x08) == 0x08);

	if (byteChange)
		byte_swap(data, len);
	if (wordChange)
		word_swap(data, len);
	if (dwordChange)
		dword_swap(data, len);
	if (lwordChange)
		lword_swap(data, len);

	return 1;
}

int vdi_set_single_es_buf(unsigned long core_idx, BOOL use_single_es_buf,
			  unsigned int *single_es_buf_size)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_WARN("vdi_get_vdi_info\n");
		return -1;
	}

	if (vdi->task_num <= 0) {
		CVI_VC_ERR("invalid vdi task num\n");
		return -1;
	} else if (vdi->task_num == 1) {
		vdi->singleEsBuf = use_single_es_buf;
		vdi->single_es_buf_size = *single_es_buf_size;
	} else {
		if (vdi->singleEsBuf != use_single_es_buf) {
			CVI_VC_ERR(
				"use_single_es_buf flag must be the same for all tasks\n");
			return -1;
		}
	}

	if (vdi->singleEsBuf) {
		if (vdi->single_es_buf_size != *single_es_buf_size) {
			CVI_VC_WARN("different single es buf size %d ignored\n",
				    *single_es_buf_size);
			CVI_VC_WARN("current single es buf size %d\n",
				    vdi->single_es_buf_size);
			*single_es_buf_size = vdi->single_es_buf_size;
		}
	}

	return 0;
}

BOOL vdi_get_is_single_es_buf(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		CVI_VC_WARN("vdi_get_vdi_info\n");
		return FALSE;
	}

	return vdi->singleEsBuf;
}

int vdi_get_chip_version(void)
{
	vdi_info_t *vdi;
	int i = 0;

	for (i = 0; i < MAX_NUM_VPU_CORE; i++) {
		vdi = vdi_get_vdi_info(i);
		if (vdi) {
			return vdi->chip_version;
		}
	}

	return -1;
}

int vdi_get_chip_capability(unsigned long core_idx, cviCapability *pCap)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		return -1;
	}

	pCap->addrRemapEn = (vdi->chip_capability & VCODEC_QUIRK_SUPPORT_VC_ADDR_REMAP) ? 1 : 0;

	return 0;
}

int vdi_get_task_num(unsigned long core_idx)
{
	vdi_info_t *vdi;

	if (core_idx >= MAX_NUM_VPU_CORE)
		return -1;

	if (s_vdi_info[core_idx] == NULL) {
		return 0;
	}

	vdi = s_vdi_info[core_idx];

	if (!vdi) {
		CVI_VC_WARN("lock failed, vdi %p, ret %p\n", vdi,
			    __builtin_return_address(0));
		return -1;
	}

	return vdi->task_num;
}

#ifdef CLI_DEBUG_SUPPORT
int vdi_show_vdi_info(unsigned long core_idx)
{
	vdi_info_t *vdi;

	vdi = vdi_get_vdi_info(core_idx);
	if (!vdi) {
		tcli_print("vdi_get_vdi_info\n");
		return -1;
	}

	tcli_print("chip_version:0x%x\n", vdi->chip_version);
	tcli_print("core_idx:%d\n", vdi->core_idx);
	tcli_print("product_code:0x%x\n", vdi->product_code);
	tcli_print("vpu_fd:%d\n", vdi->vpu_fd);
	tcli_print("task_num:%d\n", vdi->task_num);
	tcli_print("clock_state:%d\n", vdi->clock_state);
	tcli_print("chip_version:0x%x\n", vdi->chip_version);
	tcli_print("vdb_register_size:%d\n", vdi->vdb_register.size);
	tcli_print("vpu_common_memory_size:%d\n", vdi->vpu_common_memory.size);
	tcli_print("vpu_buffer_pool_count:%d\n", vdi->vpu_buffer_pool_count);
	tcli_print("vdb_bitstream_size:%d\n", vdi->vdb_bitstream.size);
	tcli_print("SingleCore:%d\n", vdi->SingleCore);
	tcli_print("singleEsBuf:%d\n", vdi->singleEsBuf);
	tcli_print("single_es_buf_size:%d\n", vdi->single_es_buf_size);

	for (int i = 0; i < MAX_VPU_BUFFER_POOL; i++) {
		if (vdi->vpu_buffer_pool[i].inuse == 1) {
			vpudrv_buffer_t *pVdb = &vdi->vpu_buffer_pool[i].vdb;
			tcli_print("[%d]vdb_size:%d\n", i, pVdb->size);
		}
	}

	return 0;
}

#endif

#endif //#if defined(linux) || defined(__linux) || defined(ANDROID)
