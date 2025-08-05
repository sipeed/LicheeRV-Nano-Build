#ifndef __JPU_JPU_MM_H__
#define __JPU_JPU_MM_H__

//#define CVI_DBG_MSG_ENABLE

#define CVI_MASK_INFO 0x1
#define CVI_MASK_FLOW 0x2
#define CVI_MASK_MEM 0x10
#define CVI_MASK_TRACE 0x20

static int jpu_klevel = 0x3;

#ifdef CVI_DBG_MSG_ENABLE
#define CVI_DBG_INFO(msg, ...)                                                 \
	do {                                                                   \
		if (jpu_klevel & CVI_MASK_INFO)                                \
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
	} while (0)
#define CVI_DBG_FLOW(msg, ...)                                                 \
	do {                                                                   \
		if (jpu_klevel & CVI_MASK_FLOW)                                \
			pr_info("[FLOW] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
	} while (0)
#define CVI_DBG_MEM(msg, ...)                                                  \
	do {                                                                   \
		if (jpu_klevel & CVI_MASK_MEM)                                 \
			pr_info("[MEM] %s = %d, " msg, __func__, __LINE__,     \
				##__VA_ARGS__);                                \
	} while (0)
#define CVI_DBG_TRACE(msg, ...)                                                \
	do {                                                                   \
		if (jpu_klevel & CVI_MASK_TRACE)                               \
			pr_info("[TRACE] %s = %d, " msg, __func__, __LINE__,   \
				##__VA_ARGS__);                                \
	} while (0)
#else
#define CVI_DBG_INFO(msg, ...)                                                 \
	do {                                                                   \
		if (jpu_klevel & CVI_MASK_INFO)                                \
			pr_info("[INFO] %s = %d, " msg, __func__, __LINE__,    \
				##__VA_ARGS__);                                \
	} while (0)
#define CVI_DBG_FLOW(msg, ...)
#define CVI_DBG_MEM(msg, ...)
#define CVI_DBG_TRACE(msg, ...)
#endif

typedef unsigned long long jmem_key_t;

#define JMEM_PAGE_SIZE (16 * 1024)
#define MAKE_KEY(_a, _b) (((jmem_key_t)_a) << 32 | _b)
#define KEY_TO_VALUE(_key) (_key >> 32)

typedef struct page_struct {
	int pageno;
	unsigned long addr;
	int used;
	int alloc_pages;
	int first_pageno;
} page_t;

typedef struct avl_node_struct {
	jmem_key_t key;
	int height;
	page_t *page;
	struct avl_node_struct *left;
	struct avl_node_struct *right;
} avl_node_t;

typedef struct _jpu_mm_struct {
	avl_node_t *free_tree;
	avl_node_t *alloc_tree;
	page_t *page_list;
	int num_pages;
	unsigned long base_addr;
	unsigned long mem_size;
	int free_page_count;
	int alloc_page_count;
} jpu_mm_t;

#define VMEM_P_ALLOC(_x) vmalloc(_x)
#define VMEM_P_FREE(_x) vfree(_x)

#define VMEM_ASSERT(_exp)                                                      \
	do {                                                                   \
		if (!(_exp))                                                   \
			pr_info("VMEM_ASSERT at %s:%d\n", __FILE__, __LINE__); \
	} while (0)
#define VMEM_HEIGHT(_tree) (_tree == NULL ? -1 : _tree->height)

#define MAX(_a, _b) (_a >= _b ? _a : _b)
#if 0
typedef enum { LEFT, RIGHT } rotation_dir_t;

typedef struct avl_node_data_struct {
	int key;
	page_t *page;
} avl_node_data_t;

static avl_node_t *make_avl_node(jmem_key_t key, page_t *page)
{
	avl_node_t *node = (avl_node_t *)VMEM_P_ALLOC(sizeof(avl_node_t));

	node->key = key;
	node->page = page;
	node->height = 0;
	node->left = NULL;
	node->right = NULL;

	return node;
}

static int get_balance_factor(avl_node_t *tree)
{
	int factor = 0;

	if (tree) {
		factor = VMEM_HEIGHT(tree->right) - VMEM_HEIGHT(tree->left);
	}

	return factor;
}

/*
 * Left Rotation
 *
 *      A                      B
 *       \                    / \
 *        B         =>       A   C
 *       /  \                 \
 *      D    C                 D
 *
 */
static avl_node_t *rotation_left(avl_node_t *tree)
{
	avl_node_t *rchild;
	avl_node_t *lchild;

	if (tree == NULL)
		return NULL;

	rchild = tree->right;
	if (rchild == NULL) {
		return tree;
	}

	lchild = rchild->left;
	rchild->left = tree;
	tree->right = lchild;

	tree->height =
		MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) + 1;
	rchild->height =
		MAX(VMEM_HEIGHT(rchild->left), VMEM_HEIGHT(rchild->right)) + 1;

	return rchild;
}

/*
 * Reft Rotation
 *
 *         A                  B
 *       \                  /  \
 *      B         =>       D    A
 *    /  \                     /
 *   D    C                   C
 *
 */
static avl_node_t *rotation_right(avl_node_t *tree)
{
	avl_node_t *rchild;
	avl_node_t *lchild;

	if (tree == NULL)
		return NULL;

	lchild = tree->left;
	if (lchild == NULL)
		return NULL;

	rchild = lchild->right;
	lchild->right = tree;
	tree->left = rchild;

	tree->height =
		MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) + 1;
	lchild->height =
		MAX(VMEM_HEIGHT(lchild->left), VMEM_HEIGHT(lchild->right)) + 1;

	return lchild;
}

static avl_node_t *do_balance(avl_node_t *tree)
{
	int bfactor = 0, child_bfactor; /* balancing factor */

	bfactor = get_balance_factor(tree);

	if (bfactor >= 2) {
		child_bfactor = get_balance_factor(tree->right);
		if (child_bfactor == 1 || child_bfactor == 0) {
			tree = rotation_left(tree);
		} else if (child_bfactor == -1) {
			tree->right = rotation_right(tree->right);
			tree = rotation_left(tree);
		} else {
			pr_info("invalid balancing factor: %d\n",
				child_bfactor);
			VMEM_ASSERT(0);
			return NULL;
		}
	} else if (bfactor <= -2) {
		child_bfactor = get_balance_factor(tree->left);
		if (child_bfactor == -1 || child_bfactor == 0) {
			tree = rotation_right(tree);
		} else if (child_bfactor == 1) {
			tree->left = rotation_left(tree->left);
			tree = rotation_right(tree);
		} else {
			pr_info("invalid balancing factor: %d\n",
				child_bfactor);
			VMEM_ASSERT(0);
			return NULL;
		}
	}

	return tree;
}
static avl_node_t *unlink_end_node(avl_node_t *tree, int dir,
				   avl_node_t **found_node)
{
	avl_node_t *node;
	*found_node = NULL;

	if (tree == NULL)
		return NULL;

	if (dir == LEFT) {
		if (tree->left == NULL) {
			*found_node = tree;
			return NULL;
		}
	} else {
		if (tree->right == NULL) {
			*found_node = tree;
			return NULL;
		}
	}

	if (dir == LEFT) {
		node = tree->left;
		tree->left = unlink_end_node(tree->left, LEFT, found_node);
		if (tree->left == NULL) {
			tree->left = (*found_node)->right;
			(*found_node)->left = NULL;
			(*found_node)->right = NULL;
		}
	} else {
		node = tree->right;
		tree->right = unlink_end_node(tree->right, RIGHT, found_node);
		if (tree->right == NULL) {
			tree->right = (*found_node)->left;
			(*found_node)->left = NULL;
			(*found_node)->right = NULL;
		}
	}

	tree->height =
		MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) + 1;

	return do_balance(tree);
}

static avl_node_t *avltree_insert(avl_node_t *tree, jmem_key_t key,
				  page_t *page)
{
	if (tree == NULL) {
		tree = make_avl_node(key, page);
	} else {
		if (key >= tree->key) {
			tree->right = avltree_insert(tree->right, key, page);
		} else {
			tree->left = avltree_insert(tree->left, key, page);
		}
	}

	tree = do_balance(tree);

	tree->height =
		MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) + 1;

	return tree;
}

static avl_node_t *do_unlink(avl_node_t *tree)
{
	avl_node_t *node;
	avl_node_t *end_node;

	node = unlink_end_node(tree->right, LEFT, &end_node);
	if (node) {
		tree->right = node;
	} else {
		node = unlink_end_node(tree->left, RIGHT, &end_node);
		if (node)
			tree->left = node;
	}

	if (node == NULL) {
		node = tree->right ? tree->right : tree->left;
		end_node = node;
	}

	if (end_node) {
		end_node->left =
			(tree->left != end_node) ? tree->left : end_node->left;
		end_node->right = (tree->right != end_node) ? tree->right :
								    end_node->right;
		end_node->height = MAX(VMEM_HEIGHT(end_node->left),
				       VMEM_HEIGHT(end_node->right)) +
				   1;
	}

	tree = end_node;

	return tree;
}

static avl_node_t *avltree_remove(avl_node_t *tree, avl_node_t **found_node,
				  jmem_key_t key)
{
	*found_node = NULL;
	if (tree == NULL) {
		pr_info("failed to find key %lu\n", (unsigned long)key);
		return NULL;
	}

	if (key == tree->key) {
		*found_node = tree;
		tree = do_unlink(tree);
	} else if (key > tree->key) {
		tree->right = avltree_remove(tree->right, found_node, key);
	} else {
		tree->left = avltree_remove(tree->left, found_node, key);
	}

	if (tree)
		tree->height =
			MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) +
			1;

	tree = do_balance(tree);

	return tree;
}

void jpu_avltree_free(avl_node_t *tree)
{
	if (tree == NULL)
		return;
	if (tree->left == NULL && tree->right == NULL) {
		VMEM_P_FREE(tree);
		return;
	}

	jpu_avltree_free(tree->left);
	tree->left = NULL;
	jpu_avltree_free(tree->right);
	tree->right = NULL;
	VMEM_P_FREE(tree);
}

static avl_node_t *remove_approx_value(avl_node_t *tree, avl_node_t **found,
				       jmem_key_t key)
{
	*found = NULL;
	if (tree == NULL) {
		return NULL;
	}

	if (key == tree->key) {
		*found = tree;
		tree = do_unlink(tree);
	} else if (key > tree->key) {
		tree->right = remove_approx_value(tree->right, found, key);
	} else {
		tree->left = remove_approx_value(tree->left, found, key);
		if (*found == NULL) {
			*found = tree;
			tree = do_unlink(tree);
		}
	}
	if (tree)
		tree->height =
			MAX(VMEM_HEIGHT(tree->left), VMEM_HEIGHT(tree->right)) +
			1;
	tree = do_balance(tree);

	return tree;
}

static void set_blocks_free(jpu_mm_t *mm, int pageno, int npages)
{
	int last_pageno = pageno + npages - 1;
	int i;
	page_t *page;
	page_t *last_page;

	VMEM_ASSERT(npages);

	if (last_pageno >= mm->num_pages) {
		pr_info("set_blocks_free: invalid last page number: %d\n",
			last_pageno);
		VMEM_ASSERT(0);
		return;
	}

	for (i = pageno; i <= last_pageno; i++) {
		mm->page_list[i].used = 0;
		mm->page_list[i].alloc_pages = 0;
		mm->page_list[i].first_pageno = -1;
	}

	page = &mm->page_list[pageno];
	page->alloc_pages = npages;
	last_page = &mm->page_list[last_pageno];
	last_page->first_pageno = pageno;

	mm->free_tree =
		avltree_insert(mm->free_tree, MAKE_KEY(npages, pageno), page);
}

static void set_blocks_alloc(jpu_mm_t *mm, int pageno, int npages)
{
	int last_pageno = pageno + npages - 1;
	int i;
	page_t *page;
	page_t *last_page;

	if (last_pageno >= mm->num_pages) {
		pr_info("set_blocks_free: invalid last page number: %d\n",
			last_pageno);
		VMEM_ASSERT(0);
		return;
	}

	for (i = pageno; i <= last_pageno; i++) {
		mm->page_list[i].used = 1;
		mm->page_list[i].alloc_pages = 0;
		mm->page_list[i].first_pageno = -1;
	}

	page = &mm->page_list[pageno];
	page->alloc_pages = npages;

	last_page = &mm->page_list[last_pageno];
	last_page->first_pageno = pageno;

	mm->alloc_tree =
		avltree_insert(mm->alloc_tree, MAKE_KEY(page->addr, 0), page);
}

int jmem_init(jpu_mm_t *mm, unsigned long addr, unsigned long size)
{
	int i;

	mm->base_addr = (addr + (JMEM_PAGE_SIZE - 1)) & ~(JMEM_PAGE_SIZE - 1);
	mm->mem_size = size & ~JMEM_PAGE_SIZE;
	mm->num_pages = mm->mem_size / JMEM_PAGE_SIZE;
	mm->free_tree = NULL;
	mm->alloc_tree = NULL;
	mm->free_page_count = mm->num_pages;
	mm->alloc_page_count = 0;
	mm->page_list = (page_t *)VMEM_P_ALLOC(mm->num_pages * sizeof(page_t));
	if (mm->page_list == NULL) {
		pr_err("%s:%d failed to vmalloc(%d)\n", __func__, __LINE__,
		       (int)(mm->num_pages * sizeof(page_t)));
		return -1;
	}

	CVI_DBG_MEM("mem_size = 0x%x, num_pages = %d\n", mm->mem_size,
		    mm->num_pages);

	for (i = 0; i < mm->num_pages; i++) {
		mm->page_list[i].pageno = i;
		mm->page_list[i].addr = mm->base_addr + i * JMEM_PAGE_SIZE;
		mm->page_list[i].alloc_pages = 0;
		mm->page_list[i].used = 0;
		mm->page_list[i].first_pageno = -1;
	}

	set_blocks_free(mm, 0, mm->num_pages);

	return 0;
}

int jmem_exit(jpu_mm_t *mm)
{
	if (mm == NULL) {
		pr_info("vmem_exit: invalid handle\n");
		return -1;
	}

	if (mm->free_tree) {
		jpu_avltree_free(mm->free_tree);
	}
	if (mm->alloc_tree) {
		jpu_avltree_free(mm->alloc_tree);
	}

	if (mm->page_list) {
		VMEM_P_FREE(mm->page_list);
		mm->page_list = NULL;
	}

	mm->base_addr = 0;
	mm->mem_size = 0;
	mm->num_pages = 0;
	mm->page_list = NULL;
	mm->free_tree = NULL;
	mm->alloc_tree = NULL;
	mm->free_page_count = 0;
	mm->alloc_page_count = 0;
	return 0;
}

unsigned long jmem_alloc(jpu_mm_t *mm, int size)
{
	avl_node_t *node;
	page_t *free_page;
	int npages, free_size;
	int alloc_pageno;
	unsigned long ptr;

	if (mm == NULL) {
		pr_info("jmem_alloc: invalid handle\n");
		return -1;
	}

	if (size <= 0)
		return -1;

	npages = (size + JMEM_PAGE_SIZE - 1) / JMEM_PAGE_SIZE;

	mm->free_tree =
		remove_approx_value(mm->free_tree, &node, MAKE_KEY(npages, 0));
	if (node == NULL) {
		return -1;
	}
	free_page = node->page;
	free_size = KEY_TO_VALUE(node->key);

	alloc_pageno = free_page->pageno;
	set_blocks_alloc(mm, alloc_pageno, npages);
	if (npages != free_size) {
		int free_pageno = alloc_pageno + npages;

		set_blocks_free(mm, free_pageno, (free_size - npages));
	}

	VMEM_P_FREE(node);

	ptr = mm->page_list[alloc_pageno].addr;
	mm->alloc_page_count += npages;
	mm->free_page_count -= npages;

	CVI_DBG_MEM("npages = %d, free_page_count = %d\n", npages,
		    mm->free_page_count);

	return ptr;
}

int jmem_free(jpu_mm_t *mm, unsigned long ptr, unsigned long pid)
{
	unsigned long addr;
	avl_node_t *found;
	page_t *page;
	int pageno, prev_free_pageno, next_free_pageno;
	int prev_size, next_size;
	int merge_page_no, merge_page_size, free_page_size;

	if (mm == NULL) {
		pr_info("vmem_free: invalid handle\n");
		return -1;
	}

	addr = ptr;

	mm->alloc_tree =
		avltree_remove(mm->alloc_tree, &found, MAKE_KEY(addr, 0));
	if (found == NULL) {
		pr_info("vmem_free: 0x%lx not found\n", addr);
		VMEM_ASSERT(0);
		return -1;
	}

	/* find previous free block */
	page = found->page;
	pageno = page->pageno;
	free_page_size = page->alloc_pages;
	prev_free_pageno = pageno - 1;
	prev_size = -1;
	if (prev_free_pageno >= 0) {
		if (mm->page_list[prev_free_pageno].used == 0) {
			prev_free_pageno =
				mm->page_list[prev_free_pageno].first_pageno;
			prev_size = mm->page_list[prev_free_pageno].alloc_pages;
		}
	}

	/* find next free block */
	next_free_pageno = pageno + page->alloc_pages;
	next_free_pageno =
		(next_free_pageno == mm->num_pages) ? -1 : next_free_pageno;
	next_size = -1;
	if (next_free_pageno >= 0) {
		if (mm->page_list[next_free_pageno].used == 0) {
			next_size = mm->page_list[next_free_pageno].alloc_pages;
		}
	}
	VMEM_P_FREE(found);

	/* merge */
	merge_page_no = page->pageno;
	merge_page_size = page->alloc_pages;
	if (prev_size >= 0) {
		mm->free_tree =
			avltree_remove(mm->free_tree, &found,
				       MAKE_KEY(prev_size, prev_free_pageno));
		if (found == NULL) {
			VMEM_ASSERT(0);
			return -1;
		}
		merge_page_no = found->page->pageno;
		merge_page_size += found->page->alloc_pages;
		VMEM_P_FREE(found);
	}
	if (next_size >= 0) {
		mm->free_tree =
			avltree_remove(mm->free_tree, &found,
				       MAKE_KEY(next_size, next_free_pageno));
		if (found == NULL) {
			VMEM_ASSERT(0);
			return -1;
		}
		merge_page_size += found->page->alloc_pages;
		VMEM_P_FREE(found);
	}

	page->alloc_pages = 0;
	page->first_pageno = -1;

	set_blocks_free(mm, merge_page_no, merge_page_size);

	mm->alloc_page_count -= free_page_size;
	mm->free_page_count += free_page_size;

	return 0;
}
#endif
#endif /* __JPU_JPU_MM_H__ */
