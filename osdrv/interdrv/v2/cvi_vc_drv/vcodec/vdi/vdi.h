//------------------------------------------------------------------------------
// File: vdi.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VDI_H_
#define _VDI_H_

#include <linux/version.h>
#include <asm/cacheflush.h>
#include "sys.h"

#include "mm.h"
#include "vpuconfig.h"
#include "vputypes.h"
#include <linux/types.h>
#ifdef CVI_H26X_USE_ION_MEM
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"
#endif
/************************************************************************/
/* COMMON REGISTERS                                                     */
/************************************************************************/
enum {
	CHIP_ID_1822 = 0x1822,
	CHIP_ID_1835 = 0x1835,
	CHIP_ID_1821A = 0x1821A,
};

#define VPU_PRODUCT_NAME_REGISTER 0x1040
#define VPU_PRODUCT_CODE_REGISTER 0x1044

//#define SUPPORT_MULTI_CORE_IN_ONE_DRIVER
#define MAX_VPU_CORE_NUM MAX_NUM_VPU_CORE
#ifdef SUPPORT_SRC_BUF_CONTROL
#define MAX_VPU_BUFFER_POOL 2000
#else
#define MAX_VPU_BUFFER_POOL (64 * MAX_NUM_INSTANCE + 12 * 3)
	//+12*3 => mvCol + YOfsTable + COfsTable
#endif

#define VpuWriteReg(CORE, ADDR, DATA)                                          \
	vdi_write_register(CORE, ADDR, DATA) // system register write
#define VpuReadReg(CORE, ADDR)                                                 \
	vdi_read_register(CORE, ADDR) // system register read
#define VpuWriteMem(CORE, ADDR, DATA, LEN, ENDIAN)                             \
	vdi_write_memory(CORE, ADDR, DATA, LEN, ENDIAN) // system memory write
#define VpuReadMem(CORE, ADDR, DATA, LEN, ENDIAN)                              \
	vdi_read_memory(CORE, ADDR, DATA, LEN, ENDIAN) // system memory read

#define VDI_POWER_ON_DOING_JOB(CORE_IDX, RET, JOB)                             \
	do {                                                                   \
		vdi_set_clock_gate(CORE_IDX, CLK_ENABLE);                      \
		RET = JOB;                                                     \
		vdi_set_clock_gate(CORE_IDX, CLK_DISABLE);                     \
	} while (0)

#define VDI_POWER_ON_DOING_JOB_START(CORE_IDX, RET, JOB)                       \
	do {                                                                   \
		vdi_set_clock_gate(CORE_IDX, CLK_ENABLE);                      \
		RET = JOB;                                                     \
	} while (0)

#define VDI_POWER_ON_DOING_JOB_CONTINUE(CORE_IDX, RET, JOB)                    \
	{                                                                   \
		RET = JOB;                                                     \
	}

#define VDI_POWER_ON_DOING_JOB_FINISH(CORE_IDX, RET, JOB)                      \
	do {                                                                   \
		RET = JOB;                                                     \
		vdi_set_clock_gate(CORE_IDX, CLK_DISABLE);                     \
	} while (0)

typedef struct vpu_buffer_t {
	__u32 size;
	PhysicalAddress phys_addr;
	__u64 base;
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} vpu_buffer_t;

struct clk_ctrl_info {
	int core_idx;
	int enable;
};

typedef enum {
	VDI_LITTLE_ENDIAN = 0, /* 64bit LE */
	VDI_BIG_ENDIAN, /* 64bit BE */
	VDI_32BIT_LITTLE_ENDIAN,
	VDI_32BIT_BIG_ENDIAN,
	/* WAVE PRODUCTS */
	VDI_128BIT_LITTLE_ENDIAN = 16,
	VDI_128BIT_LE_BYTE_SWAP,
	VDI_128BIT_LE_WORD_SWAP,
	VDI_128BIT_LE_WORD_BYTE_SWAP,
	VDI_128BIT_LE_DWORD_SWAP,
	VDI_128BIT_LE_DWORD_BYTE_SWAP,
	VDI_128BIT_LE_DWORD_WORD_SWAP,
	VDI_128BIT_LE_DWORD_WORD_BYTE_SWAP,
	VDI_128BIT_BE_DWORD_WORD_BYTE_SWAP,
	VDI_128BIT_BE_DWORD_WORD_SWAP,
	VDI_128BIT_BE_DWORD_BYTE_SWAP,
	VDI_128BIT_BE_DWORD_SWAP,
	VDI_128BIT_BE_WORD_BYTE_SWAP,
	VDI_128BIT_BE_WORD_SWAP,
	VDI_128BIT_BE_BYTE_SWAP,
	VDI_128BIT_BIG_ENDIAN = 31,
	VDI_ENDIAN_MAX
} EndianMode;

#define VDI_128BIT_ENDIAN_MASK 0xf

typedef struct vpu_pending_intr_t {
	int instance_id[COMMAND_QUEUE_DEPTH];
	int int_reason[COMMAND_QUEUE_DEPTH];
	int order_num[COMMAND_QUEUE_DEPTH];
	int in_use[COMMAND_QUEUE_DEPTH];
	int num_pending_intr;
	int count;
} vpu_pending_intr_t;

typedef enum {
	VDI_LINEAR_FRAME_MAP = 0,
	VDI_TILED_FRAME_V_MAP = 1,
	VDI_TILED_FRAME_H_MAP = 2,
	VDI_TILED_FIELD_V_MAP = 3,
	VDI_TILED_MIXED_V_MAP = 4,
	VDI_TILED_FRAME_MB_RASTER_MAP = 5,
	VDI_TILED_FIELD_MB_RASTER_MAP = 6,
	VDI_TILED_FRAME_NO_BANK_MAP = 7,
	VDI_TILED_FIELD_NO_BANK_MAP = 8,
	VDI_LINEAR_FIELD_MAP = 9,
	VDI_TILED_MAP_TYPE_MAX
} vdi_gdi_tiled_map;

enum { CLK_DISABLE = 0, CLK_ENABLE = 1 };

typedef struct vpu_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE]
				   [MAX_INST_HANDLE_SIZE]; // Since VDI don't
		// know the size of
		// CodecInst
		// structure, VDI
		// should have the
		// enough space not
		// to overflow.
	vpu_buffer_t vpu_common_buffer;
	int vpu_instance_num;
	int instance_pool_inited;
	void *pendingInst;
	int pendingInstIdxPlus1;
#ifdef REDUNDENT_CODE
	video_mm_t vmem;
#endif
	vpu_pending_intr_t pending_intr_list;
} vpu_instance_pool_t;

typedef struct _cviCapability_ {
	Uint32 addrRemapEn;
	Uint32 sliceBufferModeEn;
} cviCapability;

#define MAX_VPU_ION_BUFFER_NAME (32)

#ifdef CVI_H26X_USE_ION_MEM
#define VDI_ALLOCATE_MEMORY(CORE_IDX, VPU_BUFFER, IS_CACHED, STR_NAME)         \
	vdi_allocate_ion_memory((CORE_IDX), (VPU_BUFFER), (IS_CACHED),         \
				(STR_NAME))
#define VDI_FREE_MEMORY(CORE_IDX, VB_STREAM)                                   \
	vdi_free_ion_memory((CORE_IDX), (VB_STREAM))
#else
#define VDI_ALLOCATE_MEMORY(CORE_IDX, VPU_BUFFER, IS_CACHED, STR_NAME)         \
	vdi_allocate_dma_memory((CORE_IDX), (VPU_BUFFER))
#define VDI_FREE_MEMORY(CORE_IDX, VB_STREAM)                                   \
	vdi_free_dma_memory((CORE_IDX), (VB_STREAM))
#endif

#if defined(__cplusplus)
extern "C" {
#endif
void cvi_vdi_init(void);

int vdi_init(unsigned long core_idx, BOOL bCountTaskNum);
int vdi_get_single_core(unsigned long core_idx);
int vdi_release(unsigned long core_idx); // this function may be called only at
	// system off.

vpu_instance_pool_t *vdi_get_instance_pool(unsigned long core_idx);
int vdi_allocate_common_memory(unsigned long core_idx, Uint32 size);
int vdi_get_common_memory(unsigned long core_idx, vpu_buffer_t *vb);
int vdi_allocate_single_es_buf_memory(unsigned long core_idx, vpu_buffer_t *vb);
void vdi_free_single_es_buf_memory(unsigned long core_idx, vpu_buffer_t *vb);
int vdi_attach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);
int vdi_get_sram_memory(unsigned long core_idx, vpu_buffer_t *vb);
int vdi_dettach_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);

#ifdef CVI_H26X_USE_ION_MEM
int vdi_allocate_ion_memory(unsigned long core_idx, vpu_buffer_t *vb,
			    int is_cached, const char *str);
void vdi_free_ion_memory(unsigned long core_idx, vpu_buffer_t *vb);
int vdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
			     uint32_t u32Len);
int vdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len);
#else
int vdi_allocate_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);
void vdi_free_dma_memory(unsigned long core_idx, vpu_buffer_t *vb);
#endif

int vdi_wait_interrupt(unsigned long core_idx, int timeout,
		       uint64_t *pu64TimeStamp);
int vdi_wait_vpu_busy(unsigned long core_idx, int timeout,
		      unsigned int addr_bit_busy_flag);
int vdi_wait_bus_busy(unsigned long core_idx, int timeout,
		      unsigned int gdi_busy_flag);
#ifdef REDUNDENT_CODE
int vdi_hw_reset(unsigned long core_idx);
#endif

int vdi_set_clock_gate(unsigned long core_idx, int enable);
int vdi_get_clock_gate(unsigned long core_idx);
int vdi_get_clk_rate(unsigned long core_idx);

/**
 * @brief       make clock stable before changing clock frequency
 * @detail      Before inoking vdi_set_clock_freg() caller MUST invoke
 * vdi_ready_change_clock() function. after changing clock frequency caller also
 * invoke vdi_done_change_clock() function.
 * @return  0   failure
 *          1   success
 */
int vdi_ready_change_clock(unsigned long core_idx);
int vdi_set_change_clock(unsigned long core_idx, unsigned long clock_mask);
int vdi_done_change_clock(unsigned long core_idx);

int vdi_get_instance_num(unsigned long core_idx);

void vdi_write_register(unsigned long core_idx, unsigned long addr,
			unsigned int data);
unsigned int vdi_read_register(unsigned long core_idx, unsigned long addr);
void vdi_fio_write_register(unsigned long core_idx, unsigned long addr,
			    unsigned int data);
unsigned int vdi_fio_read_register(unsigned long core_idx, unsigned long addr);
int vdi_clear_memory(unsigned long core_idx, PhysicalAddress addr, int len);
int vdi_write_memory(unsigned long core_idx, PhysicalAddress addr,
		     unsigned char *data, int len, int endian);
int vdi_read_memory(unsigned long core_idx, PhysicalAddress addr,
		    unsigned char *data, int len, int endian);
void *vdi_get_vir_addr(unsigned long core_idx, PhysicalAddress addr);

int vdi_check_lock_by_me(unsigned long core_idx);

int vdi_lock(unsigned long core_idx);

int vdi_lock_check(unsigned long core_idx);
void vdi_unlock(unsigned long core_idx);
void vdi_vcodec_lock(unsigned long core_idx);
int vdi_vcodec_timelock(unsigned long core_idx, int timeout_ms);
int vdi_vcodec_trylock(unsigned long core_idx);
void vdi_vcodec_unlock(unsigned long core_idx);
int vdi_disp_lock(unsigned long core_idx);
void vdi_disp_unlock(unsigned long core_idx);
#ifdef REDUNDENT_CODE
void vdi_set_sdram(unsigned long core_idx, unsigned long addr, int len,
		   int endian);
#endif
#ifdef ENABLE_CNM_DEBUG_MSG
void vdi_log(unsigned long core_idx, int cmd, int step);
#endif
int vdi_open_instance(unsigned long core_idx, unsigned long inst_idx);
int vdi_close_instance(unsigned long core_idx, unsigned long inst_idx);
int vdi_set_bit_firmware_to_pm(unsigned long core_idx,
			       const unsigned short *code);
int vdi_get_vpu_fd(unsigned long core_idx);
#ifdef REDUNDENT_CODE
int vdi_get_system_endian(unsigned long core_idx);
#endif
int vdi_convert_endian(unsigned long core_idx, unsigned int endian);
#ifdef ENABLE_CNM_DEBUG_MSG
void vdi_print_vpu_status(unsigned long coreIdx);
#endif
PhysicalAddress vdi_remap_memory_address(unsigned long coreIdx,
					 PhysicalAddress address);

int vdi_set_single_es_buf(unsigned long core_idx, BOOL use_single_es_buf,
			  unsigned int *single_es_buf_size);
BOOL vdi_get_is_single_es_buf(unsigned long core_idx);
int vdi_get_chip_version(void);
int vdi_get_chip_capability(unsigned long core_idx, cviCapability *pCap);
int vdi_get_task_num(unsigned long core_idx);

#ifdef CLI_DEBUG_SUPPORT
int vdi_show_vdi_info(unsigned long core_idx);
#endif

#if defined(__cplusplus)
}
#endif

#endif //#ifndef _VDI_H_
