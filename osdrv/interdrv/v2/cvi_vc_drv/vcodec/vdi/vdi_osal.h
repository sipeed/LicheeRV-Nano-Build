//------------------------------------------------------------------------------
// File: log.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VDI_OSAL_H_
#define _VDI_OSAL_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include "sys.h"
#include "vputypes.h"

#ifndef G_TEST
#define CVI_VC_MSG_ENABLE
#endif

#define USE_SWAP_264_FW

#define CVI_MASK_ERR 0x1
#define CVI_MASK_WARN 0x2
#define CVI_MASK_INFO 0x4
#define CVI_MASK_FLOW 0x8
#define CVI_MASK_DBG 0x10
#define CVI_MASK_INTR 0x20
#define CVI_MASK_MCU 0x40
#define CVI_MASK_MEM 0x80
#define CVI_MASK_BS 0x100
#define CVI_MASK_SRC 0x200
#define CVI_MASK_IF 0x400
#define CVI_MASK_LOCK 0x800
#define CVI_MASK_PERF 0x1000
#define CVI_MASK_CFG 0x2000
#define CVI_MASK_RC 0x4000
#define CVI_MASK_TRACE 0x8000
#define CVI_MASK_DISP 0x10000
#define CVI_MASK_MOTMAP 0x20000
#define CVI_MASK_UBR 0x40000
#define CVI_MASK_RQ 0x80000
#define CVI_MASK_CVRC 0x100000
#define CVI_MASK_AR 0x200000
#define CVI_MASK_REG 0x80000000

#define CVI_MASK_CURR ((0x0) | CVI_MASK_ERR)

#ifndef UNUSED
#define UNUSED(X) ((X) = (X))
#endif

#ifdef CVI_VC_MSG_ENABLE
extern unsigned int vcodec_mask;
#define CVI_PRNT(msg, ...) pr_info(msg, ##__VA_ARGS__)
#ifndef CVI_FUNC_COND
#define CVI_FUNC_COND(FLAG, FUNC)
#endif
#ifdef LOG_WITH_FUNC_NAME
#define CVI_VC_ERR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_ERR) {                              \
			pr_info("[ERR] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_WARN(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_WARN) {                             \
			pr_warn("[WARN] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_INFO(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_INFO) {                             \
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_FLOW(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_FLOW) {                             \
			pr_info("[FLOW] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_DBG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_DBG) {                              \
			pr_info("[DBG] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_INTR(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_INTR) {                             \
			pr_info("[INTR] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MCU(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MCU) {                              \
			pr_info("[MCU] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MEM(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MEM) {                              \
			pr_info("[MEM] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_BS(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_BS) {                               \
			pr_info("[BS] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_SRC(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_SRC) {                              \
			pr_info("[SRC] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_IF(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_IF) {                               \
			pr_info("[IF] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_LOCK(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_LOCK) {                             \
			pr_info("[LOCK] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_PERF(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_PERF) {                             \
			pr_info("[PERF] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_CFG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_CFG) {                              \
			pr_info("[CFG] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_RC(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_RC) {                               \
			pr_info("[RC] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_TRACE(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_TRACE) {                            \
			pr_info("[TRACE] %s = %d, " msg, __func__, __LINE__,   \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_DISP(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_DISP) {                             \
			pr_info("[DISP] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MOTMAP(msg, ...)                                                \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MOTMAP) {                           \
			pr_info(msg, ##__VA_ARGS__);                           \
		}                                                              \
	} while (0)
#define CVI_VC_UBR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_UBR) {                              \
			pr_info("[UBR] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_RQ(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_RQ) {                               \
			pr_info("[RQ] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_CVRC(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_CVRC) {                               \
			pr_info("[CVRC] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_AR(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_AR) {                               \
			pr_info("[AR] %s = %d, " msg, __func__, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_REG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_REG) {                              \
			pr_info("[REG] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#else
#ifndef VC_DEBUG_BASIC_LEVEL
#define CVI_VC_ERR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_ERR) {                              \
			pr_info("[ERR] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_WARN(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_WARN) {                             \
			pr_warn("[WARN] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_INFO(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_INFO) {                             \
			pr_info("[INFO] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_FLOW(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_FLOW) {                             \
			pr_info("[FLOW] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_DBG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_DBG) {                              \
			pr_info("[DBG] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_INTR(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_INTR) {                             \
			pr_info("[INTR] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MCU(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MCU) {                              \
			pr_info("[MCU] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MEM(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MEM) {                              \
			pr_info("[MEM] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_BS(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_BS) {                               \
			pr_info("[BS] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_SRC(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_SRC) {                              \
			pr_info("[SRC] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_IF(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_IF) {                               \
			pr_info("[IF] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_LOCK(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_LOCK) {                             \
			pr_info("[LOCK] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_PERF(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_PERF) {                             \
			pr_info("[PERF] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_CFG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_CFG) {                              \
			pr_info("[CFG] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_RC(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_RC) {                               \
			pr_info("[RC] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_TRACE(msg, ...)                                                 \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_TRACE) {                            \
			pr_info("[TRACE] %d, " msg, __LINE__,   \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_DISP(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_DISP) {                             \
			pr_info("[DISP] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_MOTMAP(msg, ...)                                                \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_MOTMAP) {                           \
			pr_info(msg, ##__VA_ARGS__);                           \
		}                                                              \
	} while (0)
#define CVI_VC_UBR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_UBR) {                              \
			pr_info("[UBR] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_RQ(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_RQ) {                               \
			pr_info("[RQ] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_CVRC(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_CVRC) {                               \
			pr_info("[CVRC] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_AR(msg, ...)                                                    \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_AR) {                               \
			pr_info("[AR] %d, " msg, __LINE__,      \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_REG(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_REG) {                              \
			pr_info("[REG] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#else
#define CVI_VC_ERR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_ERR) {                              \
			pr_info("[ERR] %d, " msg, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_WARN(msg, ...)                                                  \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_WARN) {                             \
			pr_warn("[WARN] %d, " msg, __LINE__,    \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)
#define CVI_VC_INFO(msg, ...)
#define CVI_VC_FLOW(msg, ...)
#define CVI_VC_DBG(msg, ...)
#define CVI_VC_INTR(msg, ...)
#define CVI_VC_MCU(msg, ...)
#define CVI_VC_MEM(msg, ...)
#define CVI_VC_BS(msg, ...)
#define CVI_VC_SRC(msg, ...)
#define CVI_VC_IF(msg, ...)
#define CVI_VC_LOCK(msg, ...)
#define CVI_VC_PERF(msg, ...)
#define CVI_VC_CFG(msg, ...)
#define CVI_VC_RC(msg, ...)
#define CVI_VC_TRACE(msg, ...)
#define CVI_VC_DISP(msg, ...)
#define CVI_VC_MOTMAP(msg, ...)
#define CVI_VC_UBR(msg, ...)
#define CVI_VC_RQ(msg, ...)
#define CVI_VC_CVRC(msg, ...)
#define CVI_VC_AR(msg, ...)
#define CVI_VC_REG(msg, ...)
#endif
#endif
#else
#define CVI_FUNC_COND(FLAG, FUNC)
#define CVI_VC_ERR(msg, ...)                                                   \
	do {                                                                   \
		if (vcodec_mask & CVI_MASK_ERR) {                              \
			pr_info("[ERR] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)

#define CVI_VC_WARN(msg, ...)
#define CVI_VC_INFO(msg, ...)
#define CVI_VC_FLOW(msg, ...)
#define CVI_VC_DBG(msg, ...)
#define CVI_VC_INTR(msg, ...)
#define CVI_VC_MCU(msg, ...)
#define CVI_VC_MEM(msg, ...)
#define CVI_VC_BS(msg, ...)
#define CVI_VC_SRC(msg, ...)
#define CVI_VC_IF(msg, ...)
#define CVI_VC_LOCK(msg, ...)
#define CVI_VC_PERF(msg, ...)
#define CVI_VC_CFG(msg, ...)
#define CVI_VC_RC(msg, ...)
#define CVI_VC_TRACE(msg, ...)
#define CVI_VC_DISP(msg, ...)
#define CVI_VC_MOTMAP(msg, ...)
#define CVI_VC_UBR(msg, ...)
#define CVI_VC_RQ(msg, ...)
#define CVI_VC_CVRC(msg, ...)
#define CVI_VC_AR(msg, ...)
#define CVI_VC_REG(msg, ...)
#endif

#ifdef PLATFORM_NON_OS
#define DRAM_START (0x10B000000)

#define WAVE420L_CODE_ADDR (0x10B000000)
#define VPU_DRAM_PHYSICAL_BASE (0x113100000) //(0x10B100000 + 128*0x100000)
#define SRC_YUV_BASE (0x11B100000) //(0x10B100000 + 256*0x100000)

#define WAVE420L_CODE_SIZE (0x100000)
#define SRC_YUV_SIZE (0x8000000) // (128*0x100000)
#define VPU_DRAM_SIZE (0x8000000) // (128*0x100000)

#define SUPPORT_INTERRUPT
#define VPU_TO_32BIT(x) ((x)&0xFFFFFFFF)
#define VPU_TO_64BIT(x) ((x) | 0x100000000)
#define ENABLE_RC_LIB 0
#define CFG_MEM 1
#define DIRECT_YUV 1
#define SRC_YUV_CYCLIC 1
#define ES_CYCLIC 1
#define PROFILE_PERFORMANCE 1
#define DUMP_SRC 0
#define FFMPEG_EN 0
#define MAX_TRANSRATE 51000000

#else

#define VPU_TO_32BIT(x) ((x)&0xFFFFFFFF)
#define VPU_TO_64BIT(x) ((x) | 0x100000000)
#define ENABLE_RC_LIB 1
#ifdef G_TEST
#define CFG_MEM 1
#else
#define CFG_MEM 0
#endif
#define DIRECT_YUV 0
#define SRC_YUV_CYCLIC 0
#define ES_CYCLIC 0
#define PROFILE_PERFORMANCE 0
#define DUMP_SRC 0
#define FFMPEG_EN 0
#define MAX_TRANSRATE 51000000

#endif

#if CFG_MEM
typedef struct _DRAM_CFG_ {
	unsigned long pucCodeAddr;
	int iCodeSize;
	unsigned long pucVpuDramAddr;
	int iVpuDramSize;
	unsigned long pucSrcYuvAddr;
	int iSrcYuvSize;
} DRAM_CFG;

extern DRAM_CFG dramCfg;
#endif

#define TO_VIR(addr, vb) ((addr) - ((vb)->phys_addr) + ((vb)->virt_addr))

enum { NONE = 0, INFO, WARN, ERR, TRACE, MAX_LOG_LEVEL };
enum {
	LOG_HAS_DAY_NAME = 1, /**< Include day name [default: no]     */
	LOG_HAS_YEAR = 2, /**< Include year digit [no]		      */
	LOG_HAS_MONTH = 4, /**< Include month [no]		      */
	LOG_HAS_DAY_OF_MON = 8, /**< Include day of month [no]	      */
	LOG_HAS_TIME = 16, /**< Include time [yes]		      */
	LOG_HAS_MICRO_SEC = 32, /**< Include microseconds [yes]       */
	LOG_HAS_FILE = 64, /**< Include sender in the log [yes]	      */
	LOG_HAS_NEWLINE = 128, /**< Terminate each call with newline [yes] */
	LOG_HAS_CR = 256, /**< Include carriage return [no]           */
	LOG_HAS_SPACE = 512, /**< Include two spaces before log [yes] */
	LOG_HAS_COLOR = 1024, /**< Colorize logs [yes on win32]	      */
	LOG_HAS_LEVEL_TEXT =
		2048 /**< Include level text string [no]	      */
};
enum {
	TERM_COLOR_R = 2, /**< Red            */
	TERM_COLOR_G = 4, /**< Green          */
	TERM_COLOR_B = 1, /**< Blue.          */
	TERM_COLOR_BRIGHT = 8 /**< Bright mask.   */
};

#define MAX_PRINT_LENGTH 512

#ifdef ANDROID
#include <utils/Log.h>
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "VPUAPI"
#endif

#define VLOG LogMsg

#ifdef REDUNDENT_CODE
#define LOG_ENABLE_FILE SetLogDecor(GetLogDecor() | LOG_HAS_FILE)
#endif

typedef void *osal_file_t;
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) ||                \
	defined(WIN32) || defined(__MINGW32__)
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#else

#ifndef stdout
#define stdout (void *)1
#endif
#ifndef stderr
#define stderr (void *)1
#endif

#endif

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef ENABLE_LOG
int InitLog(void);
void DeInitLog(void);
#endif

void SetMaxLogLevel(int level);
int GetMaxLogLevel(void);

#ifdef REDUNDENT_CODE
void SetLogColor(int level, int color);
int GetLogColor(int level);

void SetLogDecor(int decor);
int GetLogDecor(void);
#endif

// log print
void LogMsg(int level, const char *format, ...);

// math
int math_div(int number, int denom);
int math_modulo(int number, int denom);

#ifdef REDUNDENT_CODE
// terminal
void osal_init_keyboard(void);
void osal_close_keyboard(void);
#endif

// memory
void *osal_memcpy(void *dst, const void *src, int count);
void *osal_memset(void *dst, int val, int count);
int osal_memcmp(const void *src, const void *dst, int size);
void *osal_malloc(int size);
#ifdef REDUNDENT_CODE
void *osal_realloc(void *ptr, int size);
#endif
void osal_free(void *p);

osal_file_t osal_fopen(const char *osal_file_tname, const char *mode);
size_t osal_fwrite(const void *p, int size, int count, osal_file_t fp);
size_t osal_fread(void *p, int size, int count, osal_file_t fp);
long osal_ftell(osal_file_t fp);
int osal_fseek(osal_file_t fp, long offset, int origin);
int osal_fclose(osal_file_t fp);
#ifdef REDUNDENT_CODE
int osal_fprintf(osal_file_t fp, const char *_Format, ...);
int osal_fscanf(osal_file_t fp, const char *_Format, ...);
int osal_kbhit(void);
int osal_getch(void);
int osal_flush_ch(void);
#endif
int osal_feof(osal_file_t fp);
void *osal_create_mutex(const char *name);
void osal_close_mutex(void *handle);
int osal_mutex_lock(void *handle);
int osal_mutex_unlock(void *handle);
int osal_rand(void);
void *osal_kmalloc(int size);
void osal_kfree(void *p);
void *osal_ion_alloc(int size);
void osal_ion_free(void *p);

#if defined(__cplusplus)
}
#endif

#endif //#ifndef _VDI_OSAL_H_
