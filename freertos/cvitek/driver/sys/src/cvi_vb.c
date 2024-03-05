//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <linux/queue.h>
//#include <pthread.h>

#include "xil_types.h"
#include "linux/types.h"
#include "linux/queue.h"
#include "FreeRTOS_POSIX.h"
#include <FreeRTOS_POSIX/errno.h>
#include "FreeRTOS_POSIX/pthread.h"
#include "malloc.h"

#include <stdatomic.h>
//#include <inttypes.h>

#include "devmem.h"
#include "cvi_base.h"
#include "cvi_vb.h"
#include "cvi_sys.h"
#include "cvi_buffer.h"
#include "hashmap.h"
#include "cvi_rpc.h"

#define CHECK_VB_HANDLE_NULL(x)							\
	do {									\
		if ((x) == NULL) {						\
			CVI_TRACE_VB(CVI_DBG_ERR, " NULL VB HANDLE\n");		\
			return CVI_ERR_VB_NULL_PTR;				\
		}								\
	} while (0)
#define CHECK_VB_HANDLE_VALID(x)						\
	do {									\
		if ((x)->magic != CVI_VB_MAGIC) {				\
			CVI_TRACE_VB(CVI_DBG_ERR, " invalid VB Handle\n");	\
			return CVI_ERR_VB_INVALID;				\
		}								\
	} while (0)

#define CHECK_VB_POOL_VALID(x)							\
	do {									\
		if ((x) == VB_STATIC_POOLID)					\
			break;							\
		if ((x) >= (VB_MAX_POOLS)) {					\
			CVI_TRACE_VB(CVI_DBG_ERR, " invalid VB Pool(%d)\n", x);	\
			return CVI_ERR_VB_ILLEGAL_PARAM;			\
		}								\
		if (!isPoolInited(x)) {						\
			CVI_TRACE_VB(CVI_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", x); \
			return CVI_ERR_VB_ILLEGAL_PARAM;			\
		}								\
	} while (0)

static VB_CONFIG_S stVbConfig;
static VB_POOL_S *commPool;
static void *shared_mem;
static pthread_mutex_t commPool_lock[VB_MAX_POOLS];
static Hashmap *vbHashmap;
pthread_mutex_t reqQ_lock;
STAILQ_HEAD(vb_req_q, vb_req) reqQ;
static atomic_bool vb_inited = ATOMIC_VAR_INIT(false);

static int _vb_hash_key(void *key)
{
	return (((uintptr_t)key) >> 10);
}

static bool _vb_hash_equals(void *keyA, void *keyB)
{
	return (keyA == keyB);
}

static bool _hash_print_cb(void *key, void *value, void *context)
{
	VB_S *p = value;
	VB_POOL *poolId = context;
	CVI_CHAR str[64];

	UNUSED(key);
	if (poolId && (*poolId != p->vb_pool))
		return true;

	sprintf(str, "Pool[%d] vb paddr(%#"PRIx64") usr_cnt(%d) /", p->vb_pool, p->phy_addr, p->usr_cnt);
	//poshiun
	//for (CVI_U32 i = 0; i < CVI_ID_BUTT; ++i) {
	//	if (*p->mod_ids & BIT(i)) {
	//	//	strncat(str, CVI_SYS_GetModName(i), sizeof(str));
	//		strcat(str, "/");
	//	}
	//}
	CVI_TRACE_VB(CVI_DBG_ERR, "%s\n", str);
	return true;
}

static VB_BLK _VB_GetBlock_Static(CVI_U32 u32BlkSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	VB_S *p = NULL;
	CVI_U64 *pModIds = NULL;
	CVI_U64 phy_addr = 0;

	//allocate with ion
	ret = CVI_SYS_IonAlloc_Cached(&phy_addr, NULL, u32BlkSize);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ion alloc failed.\n");
		return CVI_ERR_VB_NOMEM;
	}

	//create VB then filling it
	p = malloc(sizeof(*p));
	pModIds = calloc(1, sizeof(*pModIds));

	p->phy_addr = phy_addr;
	p->vir_addr = 0;
	p->vb_pool = VB_STATIC_POOLID;
	p->usr_cnt = ATOMIC_VAR_INIT(0);
	p->magic = CVI_VB_MAGIC;
	p->mod_ids = pModIds;
	p->external = CVI_FALSE;
	hashmapPut(vbHashmap, (void *)(uintptr_t)p->phy_addr, p);
	return (VB_BLK)p;
}

static inline CVI_BOOL isPoolInited(VB_POOL Pool)
{
	return (commPool[Pool].memBase == 0) ? CVI_FALSE : CVI_TRUE;
}

/* _VB_GetBlock: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param u32BlkSize: the size of vb_blk to acquire.
 * @param modId: the Id of mod which acquire this blk
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
static VB_BLK _VB_GetBlock(VB_POOL_S *pool, CVI_U32 u32BlkSize, MOD_ID_E modId)
{
	VB_S *p;

	if (u32BlkSize > pool->config.u32BlkSize) {
		CVI_TRACE_VB(CVI_DBG_ERR, "PoolID(%#x) blksize(%d) > pool's(%d).\n"
			, pool->poolID, u32BlkSize, pool->config.u32BlkSize);
		return VB_INVALID_HANDLE;
	}

	pthread_mutex_lock(&commPool_lock[pool->poolID]);
	if (FIFO_EMPTY(&pool->freeList)) {
		CVI_TRACE_VB(CVI_DBG_ERR, "VB_POOL owner(%#x) poolID(%#x) pool is empty.\n",
			    pool->ownerID, pool->poolID);
		pthread_mutex_unlock(&commPool_lock[pool->poolID]);
		hashmapForEach(vbHashmap, _hash_print_cb, &pool->poolID);
		return VB_INVALID_HANDLE;
	}

	FIFO_POP(&pool->freeList, &p);
	pool->u32FreeBlkCnt--;
	pool->u32MinFreeBlkCnt =
		(pool->u32FreeBlkCnt < pool->u32MinFreeBlkCnt) ? pool->u32FreeBlkCnt : pool->u32MinFreeBlkCnt;
	pthread_mutex_unlock(&commPool_lock[pool->poolID]);
	p->usr_cnt = ATOMIC_VAR_INIT(1);
	// poshiun
	// *p->mod_ids = BIT(modId);
	CVI_TRACE_VB(CVI_DBG_DEBUG, "Mod(%s) phy-addr(%#"PRIx64").\n", CVI_SYS_GetModName(modId), p->phy_addr);
	return (VB_BLK)p;
}

static CVI_S32 _VB_MMAP(void)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (shared_mem != NULL) {
		CVI_TRACE_VB(CVI_DBG_INFO, "already done mmap\n");
		return CVI_SUCCESS;
	}

	shared_mem = base_get_shm();
	if (shared_mem == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "base_get_shm failed!\n");
		return CVI_ERR_VB_NOMEM;
	}

	commPool = (VB_POOL_S *)(shared_mem + BASE_VB_COMM_POOL_OFFSET);
	memset(commPool, 0, VB_COMM_POOL_RSV_SIZE);
	memset(commPool_lock, 0, sizeof(commPool_lock));

	for (CVI_U32 i = 0; i < stVbConfig.u32MaxPoolCnt; ++i) {
		CVI_U32 poolSize = stVbConfig.astCommPool[i].u32BlkSize * stVbConfig.astCommPool[i].u32BlkCnt;
		VB_S *p;
		CVI_BOOL isCache = (stVbConfig.astCommPool[i].enRemapMode == VB_REMAP_MODE_CACHED);
		pthread_mutexattr_t ma;

		pthread_mutexattr_init(&ma);
		pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
		pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

		if (isCache)
			ret = CVI_SYS_IonAlloc_Cached(&commPool[i].memBase, NULL, poolSize);
		else
			ret = CVI_SYS_IonAlloc(&commPool[i].memBase, NULL, poolSize);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_VB(CVI_DBG_ERR, "ion alloc failed.\n");
			return CVI_ERR_VB_NOMEM;
		}

		pthread_mutex_init(&commPool_lock[i], &ma);
		pthread_mutex_lock(&commPool_lock[i]);
		commPool[i].poolID = i;
		commPool[i].ownerID = POOL_OWNER_COMMON;
		commPool[i].vmemBase = 0;
		commPool[i].config = stVbConfig.astCommPool[i];
		commPool[i].bIsCommPool = CVI_TRUE;
		commPool[i].u32FreeBlkCnt = stVbConfig.astCommPool[i].u32BlkCnt;
		commPool[i].u32MinFreeBlkCnt = commPool[i].u32FreeBlkCnt;
		if (strlen(stVbConfig.astCommPool[i].acName) != 0)
			strncpy(commPool[i].acPoolName, stVbConfig.astCommPool[i].acName,
				sizeof(commPool[i].acPoolName));
		else
			strncpy(commPool[i].acPoolName, "vbpool", sizeof(commPool[i].acPoolName));
		commPool[i].acPoolName[MAX_VB_POOL_NAME_LEN - 1] = '\0';

		FIFO_INIT(&commPool[i].freeList, commPool[i].config.u32BlkCnt);
		CVI_U32 u32Offset = BASE_VB_BLK_MOD_ID_OFFSET + (VB_BLK_MOD_ID_RSV_SIZE * i);

		for (CVI_U32 j = 0; j < commPool[i].config.u32BlkCnt; ++j) {
			CVI_U64 *pModIds = (CVI_U64 *)(shared_mem + u32Offset);
			*pModIds = 0;

			p = malloc(sizeof(*p));
			p->phy_addr = commPool[i].memBase + (j * commPool[i].config.u32BlkSize);
			p->vir_addr = 0;
			p->vb_pool = i;
			p->usr_cnt = ATOMIC_VAR_INIT(0);
			p->magic = CVI_VB_MAGIC;
			p->mod_ids = pModIds;
			p->external = CVI_FALSE;
			FIFO_PUSH(&commPool[i].freeList, p);
			hashmapPut(vbHashmap, (void *)(uintptr_t)p->phy_addr, p);
			u32Offset += sizeof(*pModIds);
		}
		pthread_mutex_unlock(&commPool_lock[i]);
	}
	return CVI_SUCCESS;
}

static CVI_S32 _VB_UNMAP(void)
{
	VB_S *vb;

	if (shared_mem == NULL) {
		CVI_TRACE_VB(CVI_DBG_INFO, "No need to unmap\n");
		return CVI_SUCCESS;
	}

	for (CVI_U32 i = 0; i < VB_MAX_POOLS; ++i) {
		if (!isPoolInited(i))
			continue;
		CVI_TRACE_VB(CVI_DBG_INFO, "common pool[%d]: capacity(%d) size(%d).\n"
			    , i, FIFO_CAPACITY(&commPool[i].freeList), FIFO_SIZE(&commPool[i].freeList));
		while (!FIFO_EMPTY(&commPool[i].freeList)) {
			FIFO_POP(&commPool[i].freeList, &vb);
			free(vb);
		}
		FIFO_EXIT(&commPool[i].freeList);
		CVI_SYS_IonFree(commPool[i].memBase, NULL);
	}

	base_release_shm();
	shared_mem = NULL;
	commPool = NULL;
	memset(&commPool_lock, 0, sizeof(commPool_lock));

	return CVI_SUCCESS;
}

/* CVI_VB_AcquireBlock: to register a callback to acquire vb_blk at CVI_VB_ReleaseBlock
 *                      in case of CVI_VB_GetBlock failure.
 *
 * @param fp: callback to acquire blk for module.
 * @param chn: info of the module which needs this helper.
 */
CVI_VOID CVI_VB_AcquireBlock(vb_acquire_fp fp, MMF_CHN_S chn)
{
	struct vb_req *req = malloc(sizeof(*req));

	req->fp = fp;
	req->chn = chn;

	pthread_mutex_lock(&reqQ_lock);
	STAILQ_INSERT_TAIL(&reqQ, req, stailq);
	pthread_mutex_unlock(&reqQ_lock);
}

CVI_VOID CVI_VB_PrintPool(VB_POOL Pool)
{
	hashmapForEach(vbHashmap, _hash_print_cb, &Pool);
}

VB_BLK CVI_VB_GetBlockwithID(VB_POOL Pool, CVI_U32 u32BlkSize, MOD_ID_E modId)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_vb_getblockwithid(Pool, u32BlkSize, modId);
	}
#endif
	// common pool
	if (Pool == VB_INVALID_POOLID) {
		for (int i = 0; i < VB_MAX_COMM_POOLS; ++i) {
			if (!isPoolInited(i))
				continue;
			if (commPool[i].ownerID != POOL_OWNER_COMMON)
				continue;
			if (u32BlkSize > commPool[i].config.u32BlkSize)
				continue;
			if (commPool[i].u32FreeBlkCnt == 0)
				continue;
			if ((Pool == VB_INVALID_POOLID)
				|| (commPool[Pool].config.u32BlkSize > commPool[i].config.u32BlkSize))
				Pool = i;
		}

		if (Pool == VB_INVALID_POOLID) {
			CVI_TRACE_VB(CVI_DBG_ERR, "No valid pool for size(%d).\n", u32BlkSize);
			return VB_INVALID_HANDLE;
		}
		if (!isPoolInited(Pool)) {
			CVI_TRACE_VB(CVI_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", Pool);
			return VB_INVALID_HANDLE;
		}
	} else if (Pool == VB_STATIC_POOLID) {
		return _VB_GetBlock_Static(u32BlkSize);		//need not mapping pool, allocate vb block directly
	} else if (Pool >= VB_MAX_POOLS) {
		CVI_TRACE_VB(CVI_DBG_ERR, " invalid VB Pool(%d)\n", Pool);
		return VB_INVALID_HANDLE;
	} else {
		if (!isPoolInited(Pool)) {
			CVI_TRACE_VB(CVI_DBG_ERR, "VB_POOL(%d) isn't init yet.\n", Pool);
			return VB_INVALID_HANDLE;
		}

		if (u32BlkSize > commPool[Pool].config.u32BlkSize) {
			CVI_TRACE_VB(CVI_DBG_ERR, "required size(%d) > pool(%d)'s blk-size(%d).\n", Pool, u32BlkSize,
				     commPool[Pool].config.u32BlkSize);
			return VB_INVALID_HANDLE;
		}
	}

	return _VB_GetBlock(&commPool[Pool], u32BlkSize, modId);
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
/* CVI_VB_GetBlock: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk. if VB_INVALID_POOLID, go through common-pool to search.
 * @param u32BlkSize: the size of vb_blk to acquire.
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
VB_BLK CVI_VB_GetBlock(VB_POOL Pool, CVI_U32 u32BlkSize)
{
	return CVI_VB_GetBlockwithID(Pool, u32BlkSize, CVI_ID_USER);
}

/* CVI_VB_GetBlock: acquice a vb_blk with specific size from pool.
 *
 * @param pool: the pool to acquice blk.
 * @param u32BlkSize: the size of vb_blk to acquire.
 * @return: the vb_blk if available. otherwise, VB_INVALID_HANDLE.
 */
CVI_S32 CVI_VB_ReleaseBlock(VB_BLK Block)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_vb_releaseblock(Block);
	}
#endif
	VB_S *vb = (VB_S *)Block;
	VB_POOL_S *pool;
	int old_cnt;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);
	CHECK_VB_POOL_VALID(vb->vb_pool);

	old_cnt = atomic_fetch_sub(&vb->usr_cnt, 1);
	if (old_cnt <= 1) {
		CVI_TRACE_VB(CVI_DBG_DEBUG, "%p phy-addr(%#"PRIx64") release.\n",
						__builtin_return_address(0), vb->phy_addr);

		if (vb->external) {
			CVI_TRACE_VB(CVI_DBG_DEBUG, "external buffer phy-addr(%#"PRIx64") release.\n", vb->phy_addr);
			free(vb->mod_ids);
			free(vb);
			return CVI_SUCCESS;
		}

		//free VB_STATIC_POOLID
		if (vb->vb_pool == VB_STATIC_POOLID) {
			CVI_S32 ret = CVI_SUCCESS;

			ret = CVI_SYS_IonFree(vb->phy_addr, NULL);
			hashmapRemove(vbHashmap, (void *)(uintptr_t)vb->phy_addr);
			free(vb->mod_ids);
			free(vb);
			return ret;
		}

		if (old_cnt == 0) {
			int i = 0;
			VB_S *vb_tmp;

			CVI_TRACE_VB(CVI_DBG_WARN, "vb usr_cnt is zero.\n");
			pool = &commPool[vb->vb_pool];
			pthread_mutex_lock(&commPool_lock[pool->poolID]);
			FIFO_FOREACH(vb_tmp, &pool->freeList, i) {
				if (vb_tmp->phy_addr == vb->phy_addr) {
					pthread_mutex_unlock(&commPool_lock[pool->poolID]);
					return CVI_SUCCESS;
				}
			}
			pthread_mutex_unlock(&commPool_lock[pool->poolID]);
		}

		memset(&vb->buf, 0, sizeof(vb->buf));
		vb->usr_cnt = ATOMIC_VAR_INIT(0);
		*vb->mod_ids = 0;
		pool = &commPool[vb->vb_pool];
		pthread_mutex_lock(&commPool_lock[pool->poolID]);
		FIFO_PUSH(&pool->freeList, vb);
		++pool->u32FreeBlkCnt;
		pthread_mutex_unlock(&commPool_lock[pool->poolID]);

		pthread_mutex_lock(&reqQ_lock);
		if (!STAILQ_EMPTY(&reqQ)) {
			struct vb_req *req;

			req = STAILQ_FIRST(&reqQ);
			CVI_TRACE_VB(CVI_DBG_ERR, "Try acquire vb for %s\n", CVI_SYS_GetModName(req->chn.enModId));
			if (req->fp(req->chn) == CVI_SUCCESS) {
				STAILQ_REMOVE_HEAD(&reqQ, stailq);
				free(req);
			}
		}
		pthread_mutex_unlock(&reqQ_lock);
	}

	return CVI_SUCCESS;
}

VB_BLK CVI_VB_PhysAddr2Handle(CVI_U64 u64PhyAddr)
{
	void *vb;

	vb = hashmapGet(vbHashmap, (void *)(uintptr_t)u64PhyAddr);
	return (vb == NULL) ? VB_INVALID_HANDLE : (VB_BLK)vb;
}

CVI_U64 CVI_VB_Handle2PhysAddr(VB_BLK Block)
{
	VB_S *vb = (VB_S *)Block;

	if ((vb == NULL) || (vb->magic != CVI_VB_MAGIC))
		return 0;
	return vb->phy_addr;
}

VB_POOL CVI_VB_Handle2PoolId(VB_BLK Block)
{
	VB_S *vb = (VB_S *)Block;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	return vb->vb_pool;
}

CVI_S32 CVI_VB_InquireUserCnt(VB_BLK Block, CVI_U32 *pCnt)
{
	VB_S *vb = (VB_S *)Block;

	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	*pCnt = vb->usr_cnt;
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_Init(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	bool expect = false;
#ifdef RPC_MULTI_PROCESS
	rpc_server_init();
#endif
	// Only init once until exit.
	if (!atomic_compare_exchange_strong(&vb_inited, &expect, true))
		return CVI_SUCCESS;

	vbHashmap = hashmapCreate(20, _vb_hash_key, _vb_hash_equals);

	ret = _VB_MMAP();
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "_VB_MMAP failed.\n");
		hashmapFree(vbHashmap);
		vbHashmap = NULL;
		return ret;
	}

	STAILQ_INIT(&reqQ);
	pthread_mutex_init(&reqQ_lock, NULL);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_Exit(void)
{
	bool expect = true;

	// Only exit once.
	if (!atomic_compare_exchange_strong(&vb_inited, &expect, false))
		return CVI_SUCCESS;

	_VB_UNMAP();

	if (vbHashmap) {
		hashmapFree(vbHashmap);
		vbHashmap = NULL;
	}

#ifdef RPC_MULTI_PROCESS
	rpc_server_deinit();
#endif
	return CVI_SUCCESS;
}

VB_POOL CVI_VB_CreatePool(VB_POOL_CONFIG_S *pstVbPoolCfg)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_vb_createpool(pstVbPoolCfg);
	}
#endif
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 PoolId = VB_INVALID_POOLID;
	CVI_U32 i;

	if ((pstVbPoolCfg->u32BlkSize == 0) ||
			(pstVbPoolCfg->u32BlkCnt == 0) || (pstVbPoolCfg->u32BlkCnt > VB_POOL_MAX_BLK)) {
		CVI_TRACE_VB(CVI_DBG_ERR,
				"BlkSize or BlkCnt is zero, or BlkCnt over max blk_cnt(%d)\n", VB_POOL_MAX_BLK);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}
	if (!atomic_load(&vb_inited)) {
		CVI_TRACE_VB(CVI_DBG_ERR, "VB_POOL isn't init yet.\n");
		return VB_INVALID_POOLID;
	}

	for (i = 0; i < VB_MAX_POOLS; ++i) {
		if (!isPoolInited(i))
			break;
	}
	if (i >= VB_MAX_POOLS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "VB Pool full\n");
		return VB_INVALID_POOLID;
	}

	PoolId = i;
	CVI_U32 poolSize = pstVbPoolCfg->u32BlkSize * pstVbPoolCfg->u32BlkCnt;
	VB_S *p;
	CVI_BOOL isCache = (pstVbPoolCfg->enRemapMode == VB_REMAP_MODE_CACHED);
	pthread_mutexattr_t ma;

	pthread_mutexattr_init(&ma);
	pthread_mutexattr_setpshared(&ma, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);

	if (isCache)
		ret = CVI_SYS_IonAlloc_Cached(&commPool[i].memBase, NULL, poolSize);
	else
		ret = CVI_SYS_IonAlloc(&commPool[i].memBase, NULL, poolSize);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "ion alloc failed.\n");
		return VB_INVALID_POOLID;
	}

	pthread_mutex_init(&commPool_lock[i], &ma);
	pthread_mutex_lock(&commPool_lock[i]);
	commPool[i].poolID = i;
	commPool[i].ownerID = POOL_OWNER_PRIVATE;
	commPool[i].vmemBase = 0;
	commPool[i].config = *pstVbPoolCfg;
	commPool[i].bIsCommPool = CVI_FALSE;
	commPool[i].u32FreeBlkCnt = pstVbPoolCfg->u32BlkCnt;
	commPool[i].u32MinFreeBlkCnt = commPool[i].u32FreeBlkCnt;
	if (strlen(pstVbPoolCfg->acName) != 0)
		strncpy(commPool[i].acPoolName, pstVbPoolCfg->acName, sizeof(commPool[i].acPoolName));
	else
		strncpy(commPool[i].acPoolName, "vbpool", sizeof(commPool[i].acPoolName));
	commPool[i].acPoolName[MAX_VB_POOL_NAME_LEN - 1] = '\0';

	FIFO_INIT(&commPool[i].freeList, commPool[i].config.u32BlkCnt);
	CVI_U32 u32Offset = BASE_VB_BLK_MOD_ID_OFFSET + (VB_BLK_MOD_ID_RSV_SIZE * i);

	for (CVI_U32 j = 0; j < commPool[i].config.u32BlkCnt; ++j) {
		CVI_U64 *pModIds = (CVI_U64 *)(shared_mem + u32Offset);
		*pModIds = 0;

		p = malloc(sizeof(*p));
		p->phy_addr = commPool[i].memBase + (j * commPool[i].config.u32BlkSize);
		p->vir_addr = 0;
		p->vb_pool = i;
		p->usr_cnt = ATOMIC_VAR_INIT(0);
		p->magic = CVI_VB_MAGIC;
		p->mod_ids = pModIds;
		p->external = CVI_FALSE;
		FIFO_PUSH(&commPool[i].freeList, p);
		hashmapPut(vbHashmap, (void *)(uintptr_t)p->phy_addr, p);
		u32Offset += sizeof(*pModIds);
	}
	pthread_mutex_unlock(&commPool_lock[i]);

	return PoolId;
}

CVI_S32 CVI_VB_DestroyPool(VB_POOL Pool)
{
#ifdef RPC_MULTI_PROCESS
	if (!isMaster()) {
		return rpc_client_vb_destroypool(Pool);
	}
#endif
	CHECK_VB_POOL_VALID(Pool);
	VB_S *vb;

	pthread_mutex_lock(&commPool_lock[Pool]);
	while (!FIFO_EMPTY(&commPool[Pool].freeList)) {
		FIFO_POP(&commPool[Pool].freeList, &vb);
		hashmapRemove(vbHashmap, (void *)(uintptr_t)vb->phy_addr);
		free(vb);
	}
	FIFO_EXIT(&commPool[Pool].freeList);
	pthread_mutex_unlock(&commPool_lock[Pool]);
	pthread_mutex_destroy(&commPool_lock[Pool]);
	memset(&commPool_lock[Pool], 0, sizeof(commPool_lock[Pool]));

	CVI_SYS_IonFree(commPool[Pool].memBase, NULL);
	memset(&commPool[Pool], 0, sizeof(commPool[Pool]));
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_SetConfig(const VB_CONFIG_S *pstVbConfig)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbConfig);
	if (pstVbConfig->u32MaxPoolCnt > VB_MAX_COMM_POOLS) {
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}
	if (pstVbConfig->u32MaxPoolCnt == 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "u32MaxPoolCnt is zero\n");
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}
	for (CVI_U32 i = 0; i < pstVbConfig->u32MaxPoolCnt; ++i)
		if ((pstVbConfig->astCommPool[i].u32BlkSize == 0)
			|| (pstVbConfig->astCommPool[i].u32BlkCnt == 0)
			|| (pstVbConfig->astCommPool[i].u32BlkCnt > VB_POOL_MAX_BLK)) {
			CVI_TRACE_VB(CVI_DBG_ERR,
				"u32BlkSize or u32BlkCnt is zero, or u32BlkCnt over max blk_cnt(%d)\n",
				VB_POOL_MAX_BLK);
			return CVI_ERR_VB_ILLEGAL_PARAM;
		}

	memset(&stVbConfig, 0, sizeof(stVbConfig));
	stVbConfig = *pstVbConfig;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_GetConfig(VB_CONFIG_S *pstVbConfig)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbConfig);
	*pstVbConfig = stVbConfig;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_InitModCommPool(VB_UID_E enVbUid)
{
	CVI_TRACE_VB(CVI_DBG_WARN, "VB_UID(%d) not supported yet.\n", enVbUid);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_ExitModCommPool(VB_UID_E enVbUid)
{
	CVI_TRACE_VB(CVI_DBG_WARN, "VB_UID(%d) not supported yet.\n", enVbUid);
	return CVI_SUCCESS;
}

/* CVI_VB_MmapPool - mmap the whole pool to get virtual-address
 *
 * @param Pool: pool id
 * @return CVI_SUCCESS if success; others if fail
 */
CVI_S32 CVI_VB_MmapPool(VB_POOL Pool)
{
	CHECK_VB_POOL_VALID(Pool);

	VB_POOL_S *pool;
	void *vaddr;
	int i = 0;
	VB_S *vb;

	pool = &commPool[Pool];
	if (pool->vmemBase != 0)
		return CVI_SUCCESS;

	if (pool->config.enRemapMode == VB_REMAP_MODE_CACHED)
		vaddr = CVI_SYS_MmapCache(pool->memBase, pool->config.u32BlkCnt * pool->config.u32BlkSize);
	else
		vaddr = CVI_SYS_Mmap(pool->memBase, pool->config.u32BlkCnt * pool->config.u32BlkSize);
	if (vaddr == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "mmap failed.\n");
		return CVI_ERR_VB_NOMEM;
	}

	pthread_mutex_lock(&commPool_lock[pool->poolID]);
	pool->vmemBase = vaddr;

	FIFO_FOREACH(vb, &(pool->freeList), i) {
		vb->vir_addr = pool->vmemBase + (vb->phy_addr - pool->memBase);
		CVI_TRACE_VB(CVI_DBG_DEBUG, "%d: phy(%#"PRIx64") vir(%p)\n", i, vb->phy_addr, vb->vir_addr);
	}
	pthread_mutex_unlock(&commPool_lock[pool->poolID]);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_MunmapPool(VB_POOL Pool)
{
	CHECK_VB_POOL_VALID(Pool);

	VB_POOL_S *pool;

	pool = &commPool[Pool];
	if (pool->vmemBase == 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "not mmap yet.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	if (CVI_SYS_Munmap(pool->vmemBase, pool->config.u32BlkCnt * pool->config.u32BlkSize) != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "unmap failed.\n");
		return CVI_FAILURE;
	}
	pthread_mutex_lock(&commPool_lock[pool->poolID]);
	pool->vmemBase = 0;
	pthread_mutex_unlock(&commPool_lock[pool->poolID]);
	return CVI_SUCCESS;
}

/* CVI_VB_GetBlockVirAddr - to get virtual-address of the Block
 *
 * @param Pool: pool id
 * @param Block: block id
 * @param ppVirAddr: virtual-address of the Block, cached if pool create with VB_REMAP_MODE_CACHED
 * @return CVI_SUCCESS if success; others if fail
 */
CVI_S32 CVI_VB_GetBlockVirAddr(VB_POOL Pool, VB_BLK Block, void **ppVirAddr)
{
	VB_POOL_S *pool;
	VB_S *vb = (VB_S *)Block;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, ppVirAddr);
	CHECK_VB_POOL_VALID(Pool);
	CHECK_VB_HANDLE_NULL(vb);
	CHECK_VB_HANDLE_VALID(vb);

	pool = &commPool[Pool];
	if (pool->vmemBase == 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "Blk's Pool isn't mmap yet.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	if (vb->vb_pool != Pool) {
		CVI_TRACE_VB(CVI_DBG_ERR, "Blk's Pool(%d) isn't given one(%d).\n", vb->vb_pool, Pool);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}
	*ppVirAddr = vb->vir_addr;
	return CVI_SUCCESS;
}

