//--=========================================================================--
//  This file is linux device driver for VPU.
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2015  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#ifndef __VPU_DRV_H__
#define __VPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>

#define USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY

#define VDI_IOCTL_MAGIC 'V'
#define VDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY _IO(VDI_IOCTL_MAGIC, 0)
#define VDI_IOCTL_FREE_PHYSICALMEMORY _IO(VDI_IOCTL_MAGIC, 1)
#define VDI_IOCTL_WAIT_INTERRUPT _IO(VDI_IOCTL_MAGIC, 2)
#define VDI_IOCTL_SET_CLOCK_GATE _IO(VDI_IOCTL_MAGIC, 3)
#define VDI_IOCTL_RESET _IO(VDI_IOCTL_MAGIC, 4)
#define VDI_IOCTL_GET_INSTANCE_POOL _IO(VDI_IOCTL_MAGIC, 5)
#define VDI_IOCTL_GET_COMMON_MEMORY _IO(VDI_IOCTL_MAGIC, 6)
#define VDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO _IO(VDI_IOCTL_MAGIC, 8)
#define VDI_IOCTL_OPEN_INSTANCE _IO(VDI_IOCTL_MAGIC, 9)
#define VDI_IOCTL_CLOSE_INSTANCE _IO(VDI_IOCTL_MAGIC, 10)
#define VDI_IOCTL_GET_INSTANCE_NUM _IO(VDI_IOCTL_MAGIC, 11)
#define VDI_IOCTL_GET_REGISTER_INFO _IO(VDI_IOCTL_MAGIC, 12)
#define VDI_IOCTL_SET_CLOCK_GATE_EXT _IO(VDI_IOCTL_MAGIC, 13)
#define VDI_IOCTL_GET_CHIP_VERSION _IO(VDI_IOCTL_MAGIC, 14)
#define VDI_IOCTL_GET_CLOCK_FREQUENCY _IO(VDI_IOCTL_MAGIC, 15)
#define VDI_IOCTL_RELEASE_COMMON_MEMORY _IO(VDI_IOCTL_MAGIC, 16)
#define VDI_IOCTL_GET_SINGLE_CORE_CONFIG _IO(VDI_IOCTL_MAGIC, 17)

typedef struct vpudrv_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base; /* kernel logical address in use kernel */
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} vpudrv_buffer_t;

typedef struct vpu_bit_firmware_info_t {
	__u32 size; /* size of this structure*/
	__u32 core_idx;
	__u64 reg_base_offset;
	__u16 bit_code[512];
} vpu_bit_firmware_info_t;

typedef struct vpudrv_inst_info_t {
	unsigned int core_idx;
	unsigned int inst_idx;
	int inst_open_count; /* for output only*/
} vpudrv_inst_info_t;

typedef struct vpudrv_intr_info_t {
	unsigned int timeout;
	int intr_reason;
	int coreIdx;
	__u64 intr_tv_sec;
	__u64 intr_tv_nsec;
} vpudrv_intr_info_t;

#define TEST_FPGA

#endif
