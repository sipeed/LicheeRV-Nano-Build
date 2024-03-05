#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "sys.h"
#include "vb.h"
#include "queue.h"
#include "vb_test.h"

uint32_t vb_max_pools = 512;
module_param(vb_max_pools, uint, 0644);
uint32_t vb_pool_max_blk = 64;
module_param(vb_pool_max_blk, uint, 0644);

static struct cvi_vb_cfg stVbConfig;
static struct vb_pool *vbPool;
static atomic_t ref_count = ATOMIC_INIT(0);
static struct mutex getVB_lock;
static struct mutex poolLock;

static DEFINE_MUTEX(g_lock);

DEFINE_HASHTABLE(vb_hash, 6);

#define CHECK_VB_HANDLE_NULL(x)							\
	do {									\
		if ((x) == NULL) {						\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " NULL VB HANDLE\n");		\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_HANDLE_VALID(x)						\
	do {									\
		if ((x)->magic != CVI_VB_MAGIC) {				\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Handle\n");	\
			return -EINVAL;				\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_WEAK(x)							\
	do {									\
		if ((x) == VB_STATIC_POOLID)					\
			break;							\
		if ((x) == VB_EXTERNAL_POOLID)					\
			break;							\
		if ((x) >= (vb_max_pools)) {					\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return -EINVAL;			\
		}								\
		if (!isPoolInited(x)) {						\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", x); \
			return -EINVAL;			\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID_STRONG(x)							\
	do {									\
		if ((x) >= (vb_max_pools)) {					\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return -EINVAL; 		\
		}								\
		if (!isPoolInited(x)) { 					\
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", x); \
			return -EINVAL; 		\
		}								\
	} while (0)

static inline bool isPoolInited(VB_POOL poolId)
{
	return (vbPool[poolId].memBase == 0) ? CVI_FALSE : CVI_TRUE;
}

static bool _vb_hash_find(uint64_t u64PhyAddr, struct vb_s **vb, bool del)
{
	bool is_found = false;
	struct vb_s *obj;

	hash_for_each_possible(vb_hash, obj, node, u64PhyAddr) {
		if (obj->phy_addr == u64PhyAddr) {
			if (del)
				hash_del(&obj->node);
			is_found = true;
			break;
		}
	}

	if (is_found)
		*vb = obj;
	return is_found;
}

static bool _is_vb_all_released(void)
{
	uint32_t i;

	for (i = 0; i < vb_max_pools; ++i) {
		if (isPoolInited(i)) {
			if (FIFO_CAPACITY(&vbPool[i].freeList) != FIFO_SIZE(&vbPool[i].freeList)) {
				CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "pool(%d) blk has not been all released yet\n", i);
				return false;
			}
		}
	}
	return true;
}

int32_t vb_print_pool(VB_POOL poolId)
{
	struct vb_s *vb;
	int bkt, i;
	struct hlist_node *tmp;
	char str[64];

	CHECK_VB_POOL_VALID_STRONG(poolId);

	mutex_lock(&vbPool[poolId].lock);
	hash_for_each_safe(vb_hash, bkt, tmp, vb, node) {
		if (vb->vb_pool == poolId) {
			sprintf(str, "Pool[%d] vb paddr(%#llx) usr_cnt(%d) /",
				vb->vb_pool, vb->phy_addr, vb->usr_cnt.counter);

			for (i = 0; i < CVI_ID_BUTT; ++i) {
				if (vb->mod_ids & BIT(i)) {
					strncat(str, sys_get_modname(i), sizeof(str));
					strcat(str, "/");
				}
			}
			CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "%s\n", str);
		}
	}
	mutex_unlock(&vbPool[poolId].lock);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_print_pool);

static int32_t _vb_set_config(struct cvi_vb_cfg *vb_cfg)
{
	int i;

	if (vb_cfg->comm_pool_cnt > VB_COMM_POOL_MAX_CNT
		|| vb_cfg->comm_pool_cnt == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Invalid comm_pool_cnt(%d)\n", vb_cfg->comm_pool_cnt);
		return -EINVAL;
	}

	for (i = 0; i < vb_cfg->comm_pool_cnt; ++i) {
		if (vb_cfg->comm_pool[i].blk_size == 0
			|| vb_cfg->comm_pool[i].blk_cnt == 0
			|| vb_cfg->comm_pool[i].blk_cnt > vb_pool_max_blk) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Invalid pool cfg, pool(%d), blk_size(%d), blk_cnt(%d)\n",
				i, vb_cfg->comm_pool[i].blk_size,
				vb_cfg->comm_pool[i].blk_cnt);
			return -EINVAL;
		}
	}
	stVbConfig = *vb_cfg;

	return 0;
}

static void _vb_cleanup(void)
{
	struct vb_s *vb;
	int bkt, i;
	struct hlist_node *tmp;
	struct vb_pool *pstPool;
	struct vb_req *req, *req_tmp;


	// free vb pool
	for (i = 0; i < vb_max_pools; ++i) {
		if (isPoolInited(i)) {
			pstPool = &vbPool[i];
			mutex_lock(&pstPool->lock);
			FIFO_EXIT(&pstPool->freeList);
			sys_ion_free(pstPool->memBase);
			mutex_unlock(&pstPool->lock);
			mutex_destroy(&pstPool->lock);
			// free reqQ
			mutex_lock(&pstPool->reqQ_lock);
			if (!STAILQ_EMPTY(&pstPool->reqQ)) {
				STAILQ_FOREACH_SAFE(req, &pstPool->reqQ, stailq, req_tmp) {
					STAILQ_REMOVE(&pstPool->reqQ, req, vb_req, stailq);
					kfree(req);
				}
			}
			mutex_unlock(&pstPool->reqQ_lock);
			mutex_destroy(&pstPool->reqQ_lock);
		}
	}
	vfree(vbPool);
	vbPool = NULL;

	// free vb blk
	hash_for_each_safe(vb_hash, bkt, tmp, vb, node) {
		if (vb->vb_pool == VB_STATIC_POOLID)
			sys_ion_free(vb->phy_addr);
		hash_del(&vb->node);
		vfree(vb);
	}

	mutex_destroy(&getVB_lock);
	mutex_destroy(&poolLock);
}

static int32_t _vb_create_pool(struct cvi_vb_pool_cfg *config, bool isComm)
{
	uint32_t poolSize;
	struct vb_s *p;
	bool isCache;
	char ion_name[10];
	int32_t ret, i;
	VB_POOL poolId = config->pool_id;
	void *ion_v = NULL;

	poolSize = config->blk_size * config->blk_cnt;
	isCache = (config->remap_mode == VB_REMAP_MODE_CACHED);

	snprintf(ion_name, 10, "VbPool%d", poolId);
	ret = sys_ion_alloc(&vbPool[poolId].memBase, &ion_v, (uint8_t *)ion_name, poolSize, isCache);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "sys_ion_alloc fail! ret(%d)\n", ret);
		return ret;
	}
	config->mem_base = vbPool[poolId].memBase;

	STAILQ_INIT(&vbPool[poolId].reqQ);
	mutex_init(&vbPool[poolId].reqQ_lock);
	mutex_init(&vbPool[poolId].lock);
	mutex_lock(&vbPool[poolId].lock);
	vbPool[poolId].poolID = poolId;
	vbPool[poolId].ownerID = (isComm) ? POOL_OWNER_COMMON : POOL_OWNER_PRIVATE;
	vbPool[poolId].vmemBase = ion_v;
	vbPool[poolId].blk_cnt = config->blk_cnt;
	vbPool[poolId].blk_size = config->blk_size;
	vbPool[poolId].remap_mode = config->remap_mode;
	vbPool[poolId].bIsCommPool = isComm;
	vbPool[poolId].bIsExternal = CVI_FALSE;
	vbPool[poolId].u32FreeBlkCnt = config->blk_cnt;
	vbPool[poolId].u32MinFreeBlkCnt = vbPool[poolId].u32FreeBlkCnt;
	if (strlen(config->pool_name) != 0)
		strncpy(vbPool[poolId].acPoolName, config->pool_name,
			sizeof(vbPool[poolId].acPoolName));
	else
		strncpy(vbPool[poolId].acPoolName, "vbpool", sizeof(vbPool[poolId].acPoolName));
	vbPool[poolId].acPoolName[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&vbPool[poolId].freeList, vbPool[poolId].blk_cnt);
	for (i = 0; i < vbPool[poolId].blk_cnt; ++i) {
		p = vzalloc(sizeof(*p));
		p->phy_addr = vbPool[poolId].memBase + (i * vbPool[poolId].blk_size);
		p->vir_addr = vbPool[poolId].vmemBase + (p->phy_addr - vbPool[poolId].memBase);
		p->vb_pool = poolId;
		atomic_set(&p->usr_cnt, 0);
		p->magic = CVI_VB_MAGIC;
		p->mod_ids = 0;
		p->external = false;
		FIFO_PUSH(&vbPool[poolId].freeList, p);
		hash_add(vb_hash, &p->node, p->phy_addr);
	}
	mutex_unlock(&vbPool[poolId].lock);

	return 0;
}

static int32_t _vb_create_ex_pool(struct cvi_vb_pool_ex_cfg *config)
{
	struct vb_s *p;
	int32_t i;
	VB_POOL poolId = config->pool_id;

	STAILQ_INIT(&vbPool[poolId].reqQ);
	mutex_init(&vbPool[poolId].reqQ_lock);
	mutex_init(&vbPool[poolId].lock);
	mutex_lock(&vbPool[poolId].lock);
	vbPool[poolId].poolID = poolId;
	vbPool[poolId].ownerID = POOL_OWNER_PRIVATE;
	vbPool[poolId].memBase = config->au64PhyAddr[0][0];
	vbPool[poolId].vmemBase = (void *)config->au64PhyAddr[0][0];
	vbPool[poolId].blk_cnt = config->blk_cnt;
	vbPool[poolId].blk_size = 0xffffffff;
	vbPool[poolId].remap_mode = 0;
	vbPool[poolId].bIsCommPool = CVI_FALSE;
	vbPool[poolId].bIsExternal = CVI_TRUE;
	vbPool[poolId].u32FreeBlkCnt = config->blk_cnt;
	vbPool[poolId].u32MinFreeBlkCnt = vbPool[poolId].u32FreeBlkCnt;
	strncpy(vbPool[poolId].acPoolName, "vbpoolex", sizeof(vbPool[poolId].acPoolName));
	vbPool[poolId].acPoolName[VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&vbPool[poolId].freeList, vbPool[poolId].blk_cnt);
	for (i = 0; i < vbPool[poolId].blk_cnt; ++i) {
		p = vzalloc(sizeof(*p));
		p->phy_addr = config->au64PhyAddr[i][0];
		p->vir_addr = (void *)config->au64PhyAddr[i][0];
		p->vb_pool = poolId;
		atomic_set(&p->usr_cnt, 0);
		p->magic = CVI_VB_MAGIC;
		p->mod_ids = 0;
		p->external = true;
		p->buf.phy_addr[0] = config->au64PhyAddr[i][0];
		p->buf.phy_addr[1] = config->au64PhyAddr[i][1];
		p->buf.phy_addr[2] = config->au64PhyAddr[i][2];
		FIFO_PUSH(&vbPool[poolId].freeList, p);
		hash_add(vb_hash, &p->node, p->phy_addr);
	}
	mutex_unlock(&vbPool[poolId].lock);

	return 0;
}

static void _vb_destroy_pool(VB_POOL poolId)
{
	struct vb_pool *pstPool = &vbPool[poolId];
	struct vb_s *vb, *tmp_vb;
	struct vb_req *req, *req_tmp;

	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb destroy pool, pool[%d]: capacity(%d) size(%d).\n"
		, poolId, FIFO_CAPACITY(&pstPool->freeList), FIFO_SIZE(&pstPool->freeList));
	if (FIFO_CAPACITY(&pstPool->freeList) != FIFO_SIZE(&pstPool->freeList)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "pool(%d) blk should be all released before destroy pool\n", poolId);
		return;
	}

	mutex_lock(&pstPool->lock);
	while (!FIFO_EMPTY(&pstPool->freeList)) {
		FIFO_POP(&pstPool->freeList, &vb);
		_vb_hash_find(vb->phy_addr, &tmp_vb, true);
		vfree(vb);
	}
	FIFO_EXIT(&pstPool->freeList);
	if (!pstPool->bIsExternal)
		sys_ion_free(pstPool->memBase);
	mutex_unlock(&pstPool->lock);
	mutex_destroy(&pstPool->lock);

	// free reqQ
	mutex_lock(&pstPool->reqQ_lock);
	if (!STAILQ_EMPTY(&pstPool->reqQ)) {
		STAILQ_FOREACH_SAFE(req, &pstPool->reqQ, stailq, req_tmp) {
			STAILQ_REMOVE(&pstPool->reqQ, req, vb_req, stailq);
			kfree(req);
		}
	}
	mutex_unlock(&pstPool->reqQ_lock);
	mutex_destroy(&pstPool->reqQ_lock);

	memset(&vbPool[poolId], 0, sizeof(vbPool[poolId]));
}

static int32_t _vb_init(void)
{
	uint32_t i;
	int32_t ret;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		vbPool = vzalloc(sizeof(struct vb_pool) * vb_max_pools);
		if (!vbPool) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vbPool kzalloc fail!\n");
			mutex_unlock(&g_lock);
			return -ENOMEM;
		}

		for (i = 0; i < stVbConfig.comm_pool_cnt; ++i) {
			stVbConfig.comm_pool[i].pool_id = i;
			ret = _vb_create_pool(&stVbConfig.comm_pool[i], true);
			if (ret) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
				goto VB_INIT_FAIL;
			}
		}

		mutex_init(&getVB_lock);
		mutex_init(&poolLock);
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, " -\n");
	}
	atomic_add(1, &ref_count);
	mutex_unlock(&g_lock);
	return 0;

VB_INIT_FAIL:
	for (i = 0; i < stVbConfig.comm_pool_cnt; ++i) {
		if (isPoolInited(i))
			_vb_destroy_pool(i);
	}

	if (vbPool)
		vfree(vbPool);
	vbPool = NULL;
	mutex_unlock(&g_lock);
	return ret;
}

static int32_t _vb_exit(void)
{
	int i;

	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb has already exited\n");
		mutex_unlock(&g_lock);
		return 0;
	}
	if (atomic_sub_return(1, &ref_count) > 0) {
		mutex_unlock(&g_lock);
		return 0;
	}

	if (!_is_vb_all_released()) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb has not been all released\n");
		for (i = 0; i < vb_max_pools; ++i) {
			if (isPoolInited(i))
				vb_print_pool(i);
		}
	}
	_vb_cleanup();
	mutex_unlock(&g_lock);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, " -\n");
	return 0;
}

VB_POOL find_vb_pool(uint32_t u32BlkSize)
{
	VB_POOL Pool = VB_INVALID_POOLID;
	int i;

	for (i = 0; i < VB_COMM_POOL_MAX_CNT; ++i) {
		if (!isPoolInited(i))
			continue;
		if (vbPool[i].ownerID != POOL_OWNER_COMMON)
			continue;
		if (u32BlkSize > vbPool[i].blk_size)
			continue;
		if ((Pool == VB_INVALID_POOLID)
			|| (vbPool[Pool].blk_size > vbPool[i].blk_size))
			Pool = i;
	}
	return Pool;
}
EXPORT_SYMBOL_GPL(find_vb_pool);

static VB_BLK _vb_get_block_static(uint32_t u32BlkSize)
{
	int32_t ret = CVI_SUCCESS;
	uint64_t phy_addr = 0;
	void *ion_v = NULL;

	//allocate with ion
	ret = sys_ion_alloc(&phy_addr, &ion_v, "static_pool", u32BlkSize, true);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "sys_ion_alloc fail! ret(%d)\n", ret);
		return VB_INVALID_HANDLE;
	}

	return vb_create_block(phy_addr, ion_v, VB_STATIC_POOLID, false);
}

/* _vb_get_block: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param u32BlkSize: the size of vb_blk to acquire.
 * @param modId: the Id of mod which acquire this blk
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
static VB_BLK _vb_get_block(struct vb_pool *pool, CVI_U32 u32BlkSize, MOD_ID_E modId)
{
	struct vb_s *p;

	if (u32BlkSize > pool->blk_size) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "PoolID(%#x) blksize(%d) > pool's(%d).\n"
			, pool->poolID, u32BlkSize, pool->blk_size);
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&pool->lock);
	if (FIFO_EMPTY(&pool->freeList)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "VB_POOL owner(%#x) poolID(%#x) pool is empty.\n",
			pool->ownerID, pool->poolID);
		mutex_unlock(&pool->lock);
		vb_print_pool(pool->poolID);
		return VB_INVALID_HANDLE;
	}

	FIFO_POP(&pool->freeList, &p);
	pool->u32FreeBlkCnt--;
	pool->u32MinFreeBlkCnt =
		(pool->u32FreeBlkCnt < pool->u32MinFreeBlkCnt) ? pool->u32FreeBlkCnt : pool->u32MinFreeBlkCnt;
	atomic_set(&p->usr_cnt, 1);
	p->mod_ids = BIT(modId);
	mutex_unlock(&pool->lock);
	CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "Mod(%s) phy-addr(%#llx).\n", sys_get_modname(modId), p->phy_addr);
	return (VB_BLK)p;
}

static int32_t _vb_get_blk_info(struct cvi_vb_blk_info *blk_info)
{
	VB_BLK blk = (VB_BLK)blk_info->blk;
	struct vb_s *vb;

	vb = (struct vb_s *)blk;
	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	blk_info->phy_addr = vb->phy_addr;
	blk_info->pool_id = vb->vb_pool;
	blk_info->usr_cnt = vb->usr_cnt.counter;
	return 0;
}

static int32_t _vb_get_pool_cfg(struct cvi_vb_pool_cfg *pool_cfg)
{
	VB_POOL poolId = pool_cfg->pool_id;

	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb module hasn't inited yet.\n");
		return VB_INVALID_POOLID;
	}
	CHECK_VB_POOL_VALID_STRONG(poolId);

	pool_cfg->blk_cnt = vbPool[poolId].blk_cnt;
	pool_cfg->blk_size = vbPool[poolId].blk_size;
	pool_cfg->remap_mode = vbPool[poolId].remap_mode;
	pool_cfg->mem_base = vbPool[poolId].memBase;

	return 0;
}

/**************************************************************************
 *   global APIs.
 **************************************************************************/
int32_t vb_get_pool_info(struct vb_pool **pool_info)
{
	CHECK_VB_HANDLE_NULL(pool_info);
	CHECK_VB_HANDLE_NULL(vbPool);

	*pool_info = vbPool;
	return 0;
}

void vb_cleanup(void)
{
	mutex_lock(&g_lock);
	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb has already exited\n");
		mutex_unlock(&g_lock);
		return;
	}
	_vb_cleanup();
	atomic_set(&ref_count, 0);
	mutex_unlock(&g_lock);
	CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "done\n");
}

int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig)
{
	if (!pstVbConfig) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb_get_config NULL ptr!\n");
		return -EINVAL;
	}

	*pstVbConfig = stVbConfig;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_get_config);

int32_t vb_create_pool(struct cvi_vb_pool_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_size == 0) || (config->blk_cnt == 0)
		|| (config->blk_cnt > vb_pool_max_blk)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Invalid pool cfg, blk_size(%d), blk_cnt(%d)\n",
				config->blk_size, config->blk_cnt);
		return -EINVAL;
	}

	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb module hasn't inited yet.\n");
		return -EPERM;
	}

	mutex_lock(&poolLock);
	for (i = 0; i < vb_max_pools; ++i) {
		if (!isPoolInited(i))
			break;
	}
	if (i >= vb_max_pools) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		mutex_unlock(&poolLock);
		return -ENOMEM;
	}

	config->pool_id = i;
	ret = _vb_create_pool(config, false);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "_vb_create_pool fail, ret(%d)\n", ret);
		mutex_unlock(&poolLock);
		return ret;
	}
	mutex_unlock(&poolLock);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_create_pool);

int32_t vb_create_ex_pool(struct cvi_vb_pool_ex_cfg *config)
{
	uint32_t i;
	int32_t ret;

	config->pool_id = VB_INVALID_POOLID;
	if ((config->blk_cnt == 0) || (config->blk_cnt > vb_pool_max_blk)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Invalid pool cfg, blk_cnt(%d)\n", config->blk_cnt);
		return -EINVAL;
	}

	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb module hasn't inited yet.\n");
		return -EPERM;
	}

	mutex_lock(&poolLock);
	for (i = 0; i < vb_max_pools; ++i) {
		if (!isPoolInited(i))
			break;
	}
	if (i >= vb_max_pools) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Exceed vb_max_pools cnt: %d\n", vb_max_pools);
		mutex_unlock(&poolLock);
		return -ENOMEM;
	}

	config->pool_id = i;
	ret = _vb_create_ex_pool(config);
	if (ret) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "_vb_create_ex_pool fail, ret(%d)\n", ret);
		mutex_unlock(&poolLock);
		return ret;
	}
	mutex_unlock(&poolLock);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_create_ex_pool);

int32_t vb_destroy_pool(VB_POOL poolId)
{
	CHECK_VB_POOL_VALID_STRONG(poolId);

	_vb_destroy_pool(poolId);
	return 0;
}
EXPORT_SYMBOL_GPL(vb_destroy_pool);

/* vb_create_block: create a vb blk per phy-addr given.
 *
 * @param phyAddr: phy-address of the buffer for this new vb.
 * @param virAddr: virtual-address of the buffer for this new vb.
 * @param VbPool: the pool of the vb belonging.
 * @param isExternal: if the buffer is not allocated by mmf
 */
VB_BLK vb_create_block(uint64_t phyAddr, void *virAddr, VB_POOL VbPool, bool isExternal)
{
	struct vb_s *p = NULL;

	p = vmalloc(sizeof(*p));
	if (!p) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vmalloc failed.\n");
		return VB_INVALID_HANDLE;
	}

	p->phy_addr = phyAddr;
	p->vir_addr = virAddr;
	p->vb_pool = VbPool;
	atomic_set(&p->usr_cnt, 1);
	p->magic = CVI_VB_MAGIC;
	p->mod_ids = 0;
	p->external = isExternal;
	hash_add(vb_hash, &p->node, p->phy_addr);
	return (VB_BLK)p;
}
EXPORT_SYMBOL_GPL(vb_create_block);

VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize, MOD_ID_E modId)
{
	VB_BLK blk = VB_INVALID_HANDLE;

	if (atomic_read(&ref_count) == 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb module hasn't inited yet.\n");
		return VB_INVALID_POOLID;
	}

	mutex_lock(&getVB_lock);
	// common pool
	if (poolId == VB_INVALID_POOLID) {
		poolId = find_vb_pool(u32BlkSize);
		if (poolId == VB_INVALID_POOLID) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "No valid pool for size(%d).\n", u32BlkSize);
			goto get_vb_done;
		}
	} else if (poolId == VB_STATIC_POOLID) {
		blk = _vb_get_block_static(u32BlkSize);		//need not mapping pool, allocate vb block directly
		goto get_vb_done;
	} else if (poolId >= vb_max_pools) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Pool(%d)\n", poolId);
		goto get_vb_done;
	} else {
		if (!isPoolInited(poolId)) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", poolId);
			goto get_vb_done;
		}

		if (u32BlkSize > vbPool[poolId].blk_size) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "required size(%d) > pool(%d)'s blk-size(%d).\n",
				u32BlkSize,
				poolId,
				     vbPool[poolId].blk_size);
			goto get_vb_done;
		}
	}
	blk = _vb_get_block(&vbPool[poolId], u32BlkSize, modId);

get_vb_done:
	mutex_unlock(&getVB_lock);
	return blk;
}
EXPORT_SYMBOL_GPL(vb_get_block_with_id);

int32_t vb_release_block(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	struct vb_s *vb_tmp;
	struct vb_pool *pool;
	int cnt;
	int32_t result;
	bool bReq = false;
	struct vb_req *req, *tmp;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);
	CHECK_VB_POOL_VALID_WEAK(vb->vb_pool);

	cnt = atomic_sub_return(1, &vb->usr_cnt);
	if (cnt <= 0) {
		CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "%p phy-addr(%#llx) release.\n",
			__builtin_return_address(0), vb->phy_addr);

		if ((vb->vb_pool == VB_EXTERNAL_POOLID) && vb->external) {
			CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "external buffer phy-addr(%#llx) release.\n", vb->phy_addr);
			_vb_hash_find(vb->phy_addr, &vb_tmp, true);
			vfree(vb);
			return 0;
		}

		//free VB_STATIC_POOLID
		if (vb->vb_pool == VB_STATIC_POOLID) {
			int32_t ret = 0;

			ret = sys_ion_free(vb->phy_addr);
			_vb_hash_find(vb->phy_addr, &vb_tmp, true);
			vfree(vb);
			return ret;
		}

		if (cnt < 0) {
			int i = 0;

			CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "vb usr_cnt is zero.\n");
			pool = &vbPool[vb->vb_pool];
			mutex_lock(&pool->lock);
			FIFO_FOREACH(vb_tmp, &pool->freeList, i) {
				if (vb_tmp->phy_addr == vb->phy_addr) {
					mutex_unlock(&pool->lock);
					atomic_set(&vb->usr_cnt, 0);
					return 0;
				}
			}
			mutex_unlock(&pool->lock);
		}

		pool = &vbPool[vb->vb_pool];
		mutex_lock(&pool->lock);
		if (vb->external) {
			CVI_U64 addr[3] = {0};
			memcpy(addr, vb->buf.phy_addr, sizeof(addr));
			memset(&vb->buf, 0, sizeof(vb->buf));
			memcpy(vb->buf.phy_addr, addr, sizeof(addr));
		} else {
			memset(&vb->buf, 0, sizeof(vb->buf));
		}
		atomic_set(&vb->usr_cnt, 0);
		vb->mod_ids = 0;
		FIFO_PUSH(&pool->freeList, vb);
		++pool->u32FreeBlkCnt;
		mutex_unlock(&pool->lock);

		mutex_lock(&pool->reqQ_lock);
		if (!STAILQ_EMPTY(&pool->reqQ)) {
			STAILQ_FOREACH_SAFE(req, &pool->reqQ, stailq, tmp) {
				if (req->VbPool != pool->poolID)
					continue;
				CVI_TRACE_BASE(CVI_BASE_DBG_INFO, "pool(%d) vb(%#llx) release, Try acquire vb for %s\n",
					pool->poolID,
					vb->phy_addr,
					sys_get_modname(req->chn.enModId));
				STAILQ_REMOVE(&pool->reqQ, req, vb_req, stailq);
				bReq = true;
				break;
			}
		}
		mutex_unlock(&pool->reqQ_lock);
		if (bReq) {
			result = req->fp(req->chn);
			if (result) { // req->fp return fail
				mutex_lock(&pool->reqQ_lock);
				STAILQ_INSERT_TAIL(&pool->reqQ, req, stailq);
				mutex_unlock(&pool->reqQ_lock);
			} else
				kfree(req);
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vb_release_block);

VB_BLK vb_physAddr2Handle(uint64_t u64PhyAddr)
{
	struct vb_s *vb = NULL;

	if (!_vb_hash_find(u64PhyAddr, &vb, false)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_DEBUG, "Cannot find vb corresponding to phyAddr:%#llx\n", u64PhyAddr);
		return VB_INVALID_HANDLE;
	} else
		return (VB_BLK)vb;
}
EXPORT_SYMBOL_GPL(vb_physAddr2Handle);

uint64_t vb_handle2PhysAddr(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((vb == NULL) || (vb->magic != CVI_VB_MAGIC))
		return 0;
	return vb->phy_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2PhysAddr);

void *vb_handle2VirtAddr(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((vb == NULL) || (vb->magic != CVI_VB_MAGIC))
		return NULL;
	return vb->vir_addr;
}
EXPORT_SYMBOL_GPL(vb_handle2VirtAddr);

VB_POOL vb_handle2PoolId(VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;

	if ((vb == NULL) || (vb->magic != CVI_VB_MAGIC))
		return VB_INVALID_POOLID;
	return vb->vb_pool;
}
EXPORT_SYMBOL_GPL(vb_handle2PoolId);

int32_t vb_inquireUserCnt(VB_BLK blk, uint32_t *pCnt)
{
	struct vb_s *vb = (struct vb_s *)blk;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	*pCnt = vb->usr_cnt.counter;
	return 0;
}
EXPORT_SYMBOL_GPL(vb_inquireUserCnt);

/* vb_acquire_block: to register a callback to acquire vb_blk at CVI_VB_ReleaseBlock
 *                      in case of CVI_VB_GetBlock failure.
 *
 * @param fp: callback to acquire blk for module.
 * @param chn: info of the module which needs this helper.
 */
void vb_acquire_block(vb_acquire_fp fp, MMF_CHN_S chn,
	uint32_t u32BlkSize, VB_POOL VbPool)
{
	struct vb_req *req = NULL;

	if (VbPool == VB_INVALID_POOLID) {
		VbPool = find_vb_pool(u32BlkSize);
		if (VbPool == VB_INVALID_POOLID) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Not find pool for size(%d).\n", u32BlkSize);
			return;
		}
	}
	if (VbPool >= vb_max_pools) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Pool(%d)\n", VbPool);
		return;
	}
	if (!isPoolInited(VbPool)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", VbPool);
		return;
	}

	req = kmalloc(sizeof(*req), GFP_ATOMIC);
	if (!req) {
		//CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "kmalloc failed.\n");  warning2error fail
		return;
	}

	mutex_lock(&vbPool[VbPool].reqQ_lock);
	req->fp = fp;
	req->chn = chn;
	req->VbPool = VbPool;

	STAILQ_INSERT_TAIL(&vbPool[VbPool].reqQ, req, stailq);
	mutex_unlock(&vbPool[VbPool].reqQ_lock);
}
EXPORT_SYMBOL_GPL(vb_acquire_block);

void vb_cancel_block(MMF_CHN_S chn, uint32_t u32BlkSize, VB_POOL VbPool)
{
	struct vb_req *req, *tmp;

	if (!vbPool)
		return;

	if (VbPool == VB_INVALID_POOLID) {
		VbPool = find_vb_pool(u32BlkSize);
		if (VbPool == VB_INVALID_POOLID) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "Not find pool for size(%d).\n", u32BlkSize);
			return;
		}
	}
	if (VbPool >= vb_max_pools) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, " invalid VB Pool(%d)\n", VbPool);
		return;
	}
	if (!isPoolInited(VbPool)) {
		CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", VbPool);
		return;
	}

	mutex_lock(&vbPool[VbPool].reqQ_lock);
	if (!STAILQ_EMPTY(&vbPool[VbPool].reqQ)) {
		STAILQ_FOREACH_SAFE(req, &vbPool[VbPool].reqQ, stailq, tmp) {
			if (CHN_MATCH(&req->chn, &chn)) {
				STAILQ_REMOVE(&vbPool[VbPool].reqQ, req, vb_req, stailq);
				kfree(req);
			}
		}
	}
	mutex_unlock(&vbPool[VbPool].reqQ_lock);
}
EXPORT_SYMBOL_GPL(vb_cancel_block);

long vb_ctrl(struct vb_ext_control *p)
{
	long ret = 0;
	u32 id = p->id;

	switch (id) {
	case VB_IOCTL_SET_CONFIG: {
		struct cvi_vb_cfg cfg;

		if (atomic_read(&ref_count)) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "vb has already inited, set_config cmd has no effect\n");
			break;
		}

		memset(&cfg, 0, sizeof(struct cvi_vb_cfg));
		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_vb_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_SET_CONFIG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}
		ret = _vb_set_config(&cfg);
		break;
	}

	case VB_IOCTL_GET_CONFIG: {
		if (copy_to_user(p->ptr, &stVbConfig, sizeof(struct cvi_vb_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_CONFIG copy_to_user failed.\n");
			ret = -ENOMEM;
		}
		break;
	}

	case VB_IOCTL_INIT:
		ret = _vb_init();
		break;

	case VB_IOCTL_EXIT:
		ret = _vb_exit();
		break;

	case VB_IOCTL_CREATE_POOL: {
		struct cvi_vb_pool_cfg cfg;

		memset(&cfg, 0, sizeof(struct cvi_vb_pool_cfg));
		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_vb_pool_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_CREATE_POOL copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = vb_create_pool(&cfg);
		if (ret == 0) {
			if (copy_to_user(p->ptr, &cfg, sizeof(struct cvi_vb_pool_cfg))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_CREATE_POOL copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_CREATE_EX_POOL: {
		struct cvi_vb_pool_ex_cfg cfg;

		memset(&cfg, 0, sizeof(struct cvi_vb_pool_ex_cfg));
		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_vb_pool_ex_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_CREATE_EX_POOL copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = vb_create_ex_pool(&cfg);
		if (ret == 0) {
			if (copy_to_user(p->ptr, &cfg, sizeof(struct cvi_vb_pool_ex_cfg))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_CREATE_EX_POOL copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_DESTROY_POOL: {
		VB_POOL pool;

		pool = (VB_POOL)p->value;
		ret = vb_destroy_pool(pool);
		break;
	}

	case VB_IOCTL_GET_BLOCK: {
		struct cvi_vb_blk_cfg cfg;
		VB_BLK block;

		memset(&cfg, 0, sizeof(struct cvi_vb_blk_cfg));
		if (copy_from_user(&cfg, p->ptr, sizeof(struct cvi_vb_blk_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_BLOCK copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_get_block_with_id(cfg.pool_id, cfg.blk_size, CVI_ID_USER);
		if (block == VB_INVALID_HANDLE)
			ret = -ENOMEM;
		else {
			cfg.blk = (uint64_t)block;
			if (copy_to_user(p->ptr, &cfg, sizeof(struct cvi_vb_blk_cfg))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_BLOCK copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_RELEASE_BLOCK: {
		VB_BLK blk = (VB_BLK)p->value64;

		ret = vb_release_block(blk);
		break;
	}

	case VB_IOCTL_PHYS_TO_HANDLE: {
		struct cvi_vb_blk_info blk_info;
		VB_BLK block;

		memset(&blk_info, 0, sizeof(struct cvi_vb_blk_info));
		if (copy_from_user(&blk_info, p->ptr, sizeof(struct cvi_vb_blk_info))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_PHYS_TO_HANDLE copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		block = vb_physAddr2Handle(blk_info.phy_addr);
		if (block == VB_INVALID_HANDLE)
			ret = -EINVAL;
		else {
			blk_info.blk = (uint64_t)block;
			if (copy_to_user(p->ptr, &blk_info, sizeof(struct cvi_vb_blk_info))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_PHYS_TO_HANDLE copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_BLK_INFO: {
		struct cvi_vb_blk_info blk_info;

		memset(&blk_info, 0, sizeof(struct cvi_vb_blk_info));
		if (copy_from_user(&blk_info, p->ptr, sizeof(struct cvi_vb_blk_info))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_BLK_INFO copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_blk_info(&blk_info);
		if (ret == 0) {
			if (copy_to_user(p->ptr, &blk_info, sizeof(struct cvi_vb_blk_info))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_BLK_INFO copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_CFG: {
		struct cvi_vb_pool_cfg pool_cfg;

		memset(&pool_cfg, 0, sizeof(struct cvi_vb_pool_cfg));
		if (copy_from_user(&pool_cfg, p->ptr, sizeof(struct cvi_vb_pool_cfg))) {
			CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_POOL_CFG copy_from_user failed.\n");
			ret = -ENOMEM;
			break;
		}

		ret = _vb_get_pool_cfg(&pool_cfg);
		if (ret == 0) {
			if (copy_to_user(p->ptr, &pool_cfg, sizeof(struct cvi_vb_pool_cfg))) {
				CVI_TRACE_BASE(CVI_BASE_DBG_ERR, "VB_IOCTL_GET_POOL_CFG copy_to_user failed.\n");
				ret = -ENOMEM;
			}
		}
		break;
	}

	case VB_IOCTL_GET_POOL_MAX_CNT: {
		p->value = (int32_t)vb_max_pools;
		break;
	}

	case VB_IOCTL_PRINT_POOL: {
		VB_POOL pool;

		pool = (VB_POOL)p->value;
		ret = vb_print_pool(pool);
		break;
	}

#ifdef DRV_TEST
	case VB_IOCTL_UNIT_TEST: {
		int32_t op = p->value;

		ret = vb_unit_test(op);
		break;
	}
#endif

	default:
	break;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(vb_ctrl);


