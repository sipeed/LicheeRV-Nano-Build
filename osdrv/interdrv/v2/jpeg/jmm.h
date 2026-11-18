/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: jmm.h
 * Description: jpeg memory management definition
 */

#ifndef __JMM_H__
#define __JMM_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int jpu_mask;

#define JPU_MASK_ERR 0x1
#define JPU_MASK_WARN 0x2
#define JPU_MASK_INFO 0x4
#define JPU_MASK_FLOW 0x8
#define JPU_MASK_DBG 0x10
#define JPU_MASK_MEM 0x80
#define JPU_MASK_CLK 0x100
#define JPU_MASK_TRACE 0x1000
#define JPU_MASK_DISABLE_CLK_GATING 0x2000

#define JPU_DBG_MSG_ENABLE
#ifdef JPU_DBG_MSG_ENABLE
#define JPU_DBG_ERR(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_ERR)							\
			pr_info("[ERR] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_WARN(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_WARN)							\
			pr_info("[WARN] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_INFO(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_INFO)							\
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_FLOW(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_FLOW)							\
			pr_info("[FLOW] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_DBG(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_DBG)							\
			pr_info("[DBG] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_MEM(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_MEM)							\
			pr_info("[MEM] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_CLK(msg, ...)									\
	do { \
		if (jpu_mask & JPU_MASK_CLK) \
			pr_info("[CLK] %s = %d, " msg, __func__, __LINE__, \
				##__VA_ARGS__); \
	} while (0)
#define JPU_DBG_TRACE(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_TRACE)							\
			pr_info("[TRACE] %s = %d, " msg, __func__, __LINE__,\
				##__VA_ARGS__);									\
	} while (0)
#else
#define JPU_DBG_ERR(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_ERR)							\
			pr_info("[ERR] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_WARN(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_WARN)							\
			pr_info("[WARN] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_INFO(msg, ...)									\
	do {														\
		if (jpu_mask & JPU_MASK_INFO)							\
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,	\
				##__VA_ARGS__);									\
	} while (0)
#define JPU_DBG_FLOW(msg, ...)
#define JPU_DBG_DBG(msg, ...)
#define JPU_DBG_MEM(msg, ...)
#define JPU_DBG_TRACE(msg, ...)
#endif

#define JMEM_PAGE_SIZE (16 * 1024)
#define MAKE_KEY(_a, _b) (((unsigned long long)_a) << 32 | _b)
#define KEY_TO_VALUE(_key) (_key >> 32)

enum rotation_dir_t { LEFT, RIGHT };

struct page_struct {
	int pageno;
	unsigned long addr;
	int used;
	int alloc_pages;
	int first_pageno;
};

struct avl_node_struct {
	unsigned long long key;
	int height;
	struct page_struct *page;
	struct avl_node_struct *left;
	struct avl_node_struct *right;
};

struct jpu_mm_struct {
	struct avl_node_struct *free_tree;
	struct avl_node_struct *alloc_tree;
	struct page_struct *page_list;
	int num_pages;
	unsigned long base_addr;
	unsigned long mem_size;
	int free_page_count;
	int alloc_page_count;
};

#define VMEM_P_ALLOC(_x) vmalloc(_x)
#define VMEM_P_FREE(_x) vfree(_x)
#define VMEM_ASSERT(_exp)											\
	do {															\
		if (!(_exp))												\
			JPU_DBG_MEM("VMEM_ASSERT at %s:%d\n", __FILE__, __LINE__);	\
	} while (0)
#define VMEM_HEIGHT(_tree) (_tree == NULL ? -1 : _tree->height)
#define MAX(_a, _b) (_a >= _b ? _a : _b)

struct avl_node_data_struct {
	int key;
	struct page_struct *page;
};

int jmem_init(struct jpu_mm_struct *mm, unsigned long addr, unsigned long size);
int jmem_exit(struct jpu_mm_struct *mm);
unsigned long jmem_alloc(struct jpu_mm_struct *mm, int size, unsigned long pid);
int jmem_free(struct jpu_mm_struct *mm, unsigned long ptr, unsigned long pid);

#ifdef __cplusplus
}
#endif

#endif /* __JMM_H__ */
