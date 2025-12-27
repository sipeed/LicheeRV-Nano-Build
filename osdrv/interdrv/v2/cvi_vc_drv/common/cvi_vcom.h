#ifndef _CVI_VCOM_H_
#define _CVI_VCOM_H_

#include <linux/printk.h>

#define CVI_DBG_MSG_ENABLE

#define CVI_VCOM_MASK_ERR 0x1
#define CVI_VCOM_MASK_WARN 0x2
#define CVI_VCOM_MASK_INFO 0x4
#define CVI_VCOM_MASK_FLOW 0x8
#define CVI_VCOM_MASK_DBG 0x10
#define CVI_VCOM_MASK_IF 0x20
#define CVI_VCOM_MASK_LOCK 0x40
#define CVI_VCOM_MASK_RC 0x80
#define CVI_VCOM_MASK_CVRC 0x100
#define CVI_VCOM_MASK_FLOAT 0x200
#define CVI_VCOM_MASK_MEM 0x400
#define CVI_VCOM_MASK_TRACE 0x1000
#define CVI_VCOM_MASK_CURR (0x3)

#define PRINTF pr_info

#ifdef CVI_DBG_MSG_ENABLE
extern int vcom_mask;
#define CVI_VCOM_PRNT(msg, ...)	\
			PRINTF(msg, ##__VA_ARGS__)

#define CVI_VCOM_ERR(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_ERR)							\
			PRINTF("[ERR] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_WARN(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_WARN)							\
			PRINTF("[WARN] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_INFO(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_INFO)							\
			PRINTF("[INFO] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_FLOW(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_FLOW)							\
			PRINTF("[FLOW] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_DBG(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_DBG)							\
			PRINTF("[DBG] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_MEM(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_MEM)							\
			PRINTF("[MEM] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_IF(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_IF)							\
			PRINTF("[IF] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_LOCK(msg, ...)									\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_LOCK)							\
			PRINTF("[LOCK] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_RC(msg, ...)										\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_RC)							\
			PRINTF("[RC] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_CVRC(msg, ...)								\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_CVRC)							\
			PRINTF("[CVRC] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#define CVI_VCOM_FLOAT(msg, ...)
#define CVI_VCOM_TRACE(msg, ...)								\
	do {	\
		if (vcom_mask & CVI_VCOM_MASK_TRACE)							\
			PRINTF("[TRACE] %s = %d, " msg, __func__, __LINE__,	\
			       ##__VA_ARGS__);								\
	} while (0)
#else
#define CVI_VCOM_ERR(msg, ...)
#define CVI_VCOM_WARN(msg, ...)
#define CVI_VCOM_INFO(msg, ...)
#define CVI_VCOM_FLOW(msg, ...)
#define CVI_VCOM_DBG(msg, ...)
#define CVI_VCOM_MEM(msg, ...)
#define CVI_VCOM_IF(msg, ...)
#define CVI_VCOM_LOCK(msg, ...)
#define CVI_VCOM_RC(msg, ...)
#define CVI_VCOM_CVRC(msg, ...)
#define CVI_VCOM_FLOAT(msg, ...)
#define CVI_VCOM_TRACE(msg, ...)
#endif

#endif //#ifndef _CVI_VCOM_H_
