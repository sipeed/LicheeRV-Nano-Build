
#ifndef _JDI_HPI_H_
#define _JDI_HPI_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"
#include "sys.h"
#include "../jpuapi/jpuconfig.h"
#include "../jpuapi/regdefine.h"
#include "mm.h"

#define CVI_DBG_MSG_ENABLE

#define CVI_MASK_ERR 0x1
#define CVI_MASK_WARN 0x2
#define CVI_MASK_INFO 0x4
#define CVI_MASK_FLOW 0x8
#define CVI_MASK_DBG 0x10
#define CVI_MASK_IF 0x20
#define CVI_MASK_LOCK 0x40
#define CVI_MASK_RC 0x80
#define CVI_MASK_CVRC 0x100
#define CVI_MASK_FLOAT 0x200
#define CVI_MASK_MEM 0x400
#define CVI_MASK_PERF 0x1000
#define CVI_MASK_TRACE 0x10000
#define CVI_MASK_CURR (0x3)

#ifdef CVI_DBG_MSG_ENABLE
extern uint jpeg_mask;

#define CVI_PRNT(msg, ...) pr_info(msg, ##__VA_ARGS__)
#define CVI_JPG_DBG_ERR(msg, ...)                                              \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_ERR)                                  \
			pr_err("[ERR] %s = %d, " msg, __func__, __LINE__,      \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_WARN(msg, ...)                                             \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_WARN)                                 \
			pr_warn("[WARN] %s = %d, " msg, __func__, __LINE__,     \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_INFO(msg, ...)                                             \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_INFO)                                 \
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,     \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_FLOW(msg, ...)                                             \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_FLOW)                                 \
			pr_info("[FLOW] %s = %d, " msg, __func__, __LINE__,     \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_DBG(msg, ...)                                             \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_DBG)                                 \
			pr_info("[DBG] %s = %d, " msg, __func__, __LINE__,     \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_IF(msg, ...)                                               \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_IF)                                   \
			pr_info("[IF] %s = %d, " msg, __func__, __LINE__,       \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_LOCK(msg, ...)                                             \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_LOCK)                                 \
			pr_info("[LOCK] %s = %d, " msg, __func__, __LINE__,     \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_RC(msg, ...)                                               \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_RC)                                   \
			pr_info("[RC] %s = %d, " msg, __func__, __LINE__,       \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_CVRC(msg, ...)                                               \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_CVRC)                                   \
			pr_info("[CVRC] %s = %d, " msg, __func__, __LINE__,       \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_FLOAT(msg, ...)
#define CVI_JPG_DBG_MEM(msg, ...)                                              \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_MEM)                                  \
			pr_info("[MEM] %s = %d, " msg, __func__, __LINE__,      \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_PERF(msg, ...)                                            \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_PERF)                                \
			pr_info("[PERF] %s = %d, " msg, __func__, __LINE__,    \
			       ##__VA_ARGS__);                                 \
	} while (0)
#define CVI_JPG_DBG_TRACE(msg, ...)                                            \
	do {                                                                   \
		if (jpeg_mask & CVI_MASK_TRACE)                                \
			pr_info("[TRACE] %s = %d, " msg, __func__, __LINE__,    \
			       ##__VA_ARGS__);                                 \
	} while (0)
#else
#define CVI_JPG_DBG_ERR(msg, ...)
#define CVI_JPG_DBG_WARN(msg, ...)
#define CVI_JPG_DBG_INFO(msg, ...)
#define CVI_JPG_DBG_FLOW(msg, ...)
#define CVI_JPG_DBG_DBG(msg, ...)
#define CVI_JPG_DBG_IF(msg, ...)
#define CVI_JPG_DBG_LOCK(msg, ...)
#define CVI_JPG_DBG_RC(msg, ...)
#define CVI_JPG_DBG_CVRC(msg, ...)
#define CVI_JPG_DBG_FLOAT(msg, ...)
#define CVI_JPG_DBG_MEM(msg, ...)
#define CVI_JPG_DBG_PERF(msg, ...)
#define CVI_JPG_DBG_TRACE(msg, ...)
#endif

#define MAX_BS_SIZE (16384 * 1024)
#define MAX_JPU_BUFFER_POOL 32

#ifdef CVI_JPG_USE_ION_MEM
#define MAX_JPU_ION_BUFFER_NAME (32)
#define JDI_ALLOCATE_MEMORY(VB_STREAM, IS_JPE, IS_CACHED)                      \
	jdi_allocate_ion_memory((VB_STREAM), (IS_JPE), (IS_CACHED))
#define JDI_FREE_MEMORY(VB_STREAM) jdi_free_ion_memory((VB_STREAM))
#else
#define JDI_ALLOCATE_MEMORY(VB_STREAM, IS_JPE, IS_CACHED)                      \
	jdi_allocate_dma_memory((VB_STREAM), (IS_JPE))
#define JDI_FREE_MEMORY(VB_STREAM) jdi_free_dma_memory((VB_STREAM))
#endif

#define JpuWriteReg(ADDR, DATA)                                                \
	jdi_write_register(ADDR, DATA) // system register write
#define JpuReadReg(ADDR) jdi_read_register(ADDR) // system register write
#define JpuWriteMem(ADDR, DATA, LEN, ENDIAN)                                   \
	jdi_write_memory(ADDR, DATA, LEN, ENDIAN) // system memory write
#define JpuReadMem(ADDR, DATA, LEN, ENDIAN)                                    \
	jdi_read_memory(ADDR, DATA, LEN, ENDIAN) // system memory write

typedef struct jpu_buffer_t {
	__u32 size;
	__u64 phys_addr;
	__u64 base;
	__u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
} jpu_buffer_t;

typedef struct jpu_instance_pool_t {
	unsigned char jpgInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
#if defined(LIBCVIJPULITE)
	long jpu_mutex;
#else
	struct mutex jpu_mutex;
#endif
	int jpu_instance_num;
	int instance_pool_inited;
	void *pendingInst;
#ifdef REDUNDENT_CODE
	jpeg_mm_t vmem;
#endif
} jpu_instance_pool_t;

#ifdef SUPPORT_128BIT_BUS

typedef enum {
	JDI_128BIT_LITTLE_64BIT_LITTLE_ENDIAN =
		((0 << 2) + (0 << 1) + (0 << 0)), //  128 bit little, 64 bit
	//  little
	JDI_128BIT_BIG_64BIT_LITTLE_ENDIAN =
		((1 << 2) + (0 << 1) + (0 << 0)), //  128 bit big , 64 bit
	//  little
	JDI_128BIT_LITTLE_64BIT_BIG_ENDIAN =
		((0 << 2) + (0 << 1) + (1 << 0)), //  128 bit little, 64 bit big
	JDI_128BIT_BIG_64BIT_BIG_ENDIAN =
		((1 << 2) + (0 << 1) + (1 << 0)), //  128 bit big, 64 bit big
	JDI_128BIT_LITTLE_32BIT_LITTLE_ENDIAN =
		((0 << 2) + (1 << 1) + (0 << 0)), //  128 bit little, 32 bit
	//  little
	JDI_128BIT_BIG_32BIT_LITTLE_ENDIAN =
		((1 << 2) + (1 << 1) + (0 << 0)), //  128 bit big , 32 bit
	//  little
	JDI_128BIT_LITTLE_32BIT_BIG_ENDIAN =
		((0 << 2) + (1 << 1) + (1 << 0)), //  128 bit little, 32 bit big
	JDI_128BIT_BIG_32BIT_BIG_ENDIAN =
		((1 << 2) + (1 << 1) + (1 << 0)), //  128 bit big, 32 bit big
} EndianMode;
#define JDI_LITTLE_ENDIAN JDI_128BIT_LITTLE_64BIT_LITTLE_ENDIAN
#define JDI_BIG_ENDIAN JDI_128BIT_BIG_64BIT_BIG_ENDIAN
#define JDI_128BIT_ENDIAN_MASK (1 << 2)
#define JDI_64BIT_ENDIAN_MASK (1 << 1)
#define JDI_ENDIAN_MASK (1 << 0)

#define JDI_32BIT_LITTLE_ENDIAN JDI_128BIT_LITTLE_32BIT_LITTLE_ENDIAN
#define JDI_32BIT_BIG_ENDIAN JDI_128BIT_LITTLE_32BIT_BIG_ENDIAN

#else

typedef enum {
	JDI_LITTLE_ENDIAN = 0,
	JDI_BIG_ENDIAN,
	JDI_32BIT_LITTLE_ENDIAN,
	JDI_32BIT_BIG_ENDIAN,
} EndianMode;
#endif

typedef enum { JDI_LOG_CMD_PICRUN = 0, JDI_LOG_CMD_MAX } jdi_log_cmd;

#if defined(__cplusplus)
extern "C" {
#endif
int jdi_probe(void);
int jdi_init(void);
int jdi_release(void); // this function may be called only at system off.
int jdi_get_task_num(void);
int jdi_use_single_es_buffer(void);
int jdi_set_enc_task(int bSingleEsBuf, int *singleEsBufSize);
int jdi_get_enc_task_num(void);
int jdi_delete_enc_task(void);
jpu_instance_pool_t *jdi_get_instance_pool(void);
int jdi_get_allocated_memory(jpu_buffer_t *vb, int is_jpe);
#ifndef CVI_JPG_USE_ION_MEM
int jdi_allocate_dma_memory(jpu_buffer_t *vb, int is_jpe);
void jdi_free_dma_memory(jpu_buffer_t *vb);
#else
int jdi_allocate_ion_memory(jpu_buffer_t *vb, int is_jpe, int is_cached);
int jdi_free_ion_memory(jpu_buffer_t *vb);
int jdi_invalidate_ion_cache(uint64_t u64PhyAddr, void *pVirAddr,
			     uint32_t u32Len);
int jdi_flush_ion_cache(uint64_t u64PhyAddr, void *pVirAddr, uint32_t u32Len);
#endif

int jdi_wait_interrupt(int timeout);
int jdi_hw_reset(void);

int jdi_set_clock_gate(int enable);
int jdi_get_clock_gate(void);

int jdi_open_instance(unsigned long instIdx);
int jdi_close_instance(unsigned long instIdx);
int jdi_get_instance_num(void);

void jdi_write_register(unsigned int addr, unsigned int data);
unsigned int jdi_read_register(unsigned int addr);

int jdi_write_memory(unsigned long addr, unsigned char *data, int len,
		     int endian);
int jdi_read_memory(unsigned long addr, unsigned char *data, int len,
		    int endian);

#if defined(LIBCVIJPULITE)
int jdi_lock(int sleep_us);
#else
int jdi_lock(void);
#endif
void jdi_unlock(void);

void jdi_log(int cmd, int step);

#ifdef JPU_FPGA_PLATFORM
#define HPI_SET_TIMING_MAX 1000
int jdi_set_timing_opt(void);
int jdi_set_clock_freg(int Device, int OutFreqMHz, int InFreqMHz);
#endif

PhysicalAddress jdi_get_memory_addr_high(PhysicalAddress addr);

#if defined(__cplusplus)
}
#endif

#endif //#ifndef _JDI_HPI_H_
