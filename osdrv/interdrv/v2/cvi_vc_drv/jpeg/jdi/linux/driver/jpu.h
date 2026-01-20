

#ifndef __JPU_DRV_H__
#define __JPU_DRV_H__

#include <linux/fs.h>
#include <linux/types.h>

#define USE_VMALLOC_FOR_INSTANCE_POOL_MEMORY

#define JDI_IOCTL_MAGIC 'J'

#define JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY _IO(JDI_IOCTL_MAGIC, 0)
#define JDI_IOCTL_FREE_PHYSICAL_MEMORY _IO(JDI_IOCTL_MAGIC, 1)
#define JDI_IOCTL_WAIT_INTERRUPT _IO(JDI_IOCTL_MAGIC, 2)
#define JDI_IOCTL_SET_CLOCK_GATE _IO(JDI_IOCTL_MAGIC, 3)
#define JDI_IOCTL_RESET _IO(JDI_IOCTL_MAGIC, 4)
#define JDI_IOCTL_GET_INSTANCE_POOL _IO(JDI_IOCTL_MAGIC, 5)
#define JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO _IO(JDI_IOCTL_MAGIC, 6)
#define JDI_IOCTL_OPEN_INSTANCE _IO(JDI_IOCTL_MAGIC, 7)
#define JDI_IOCTL_CLOSE_INSTANCE _IO(JDI_IOCTL_MAGIC, 8)
#define JDI_IOCTL_GET_INSTANCE_NUM _IO(JDI_IOCTL_MAGIC, 9)
#define JDI_IOCTL_GET_REGISTER_INFO _IO(JDI_IOCTL_MAGIC, 10)
#define JDI_IOCTL_GET_CONTROL_REG _IO(JDI_IOCTL_MAGIC, 11)
#define JDI_IOCTL_SET_JENC_PROC_INFO _IO(JDI_IOCTL_MAGIC, 12)
#define JDI_IOCTL_SET_JDEC_PROC_INFO _IO(JDI_IOCTL_MAGIC, 13)
#define JDI_IOCTL_GET_JENC_DEBUG_CONFIG _IO(JDI_IOCTL_MAGIC, 14)
#define JDI_IOCTL_GET_JDEC_DEBUG_CONFIG _IO(JDI_IOCTL_MAGIC, 15)

typedef struct jpudrv_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base; /* kernel logical address in use kernel */
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} jpudrv_buffer_t;

#endif
