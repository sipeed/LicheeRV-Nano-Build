#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>

#include "hashmap.h"
#include "devmem.h"
#include "cvi_base.h"
#include "cvi_vb.h"
#include "cvi_sys.h"
#include "vb_ioctl.h"
#include "linux/vb_uapi.h"

#ifndef UNUSED
#define UNUSED(x) ((x) = (x))
#endif

typedef struct _pool {
	CVI_U64 memBase;
	void *vmemBase;
	CVI_U32 u32BlkSize;
	CVI_U32 u32BlkCnt;
	VB_REMAP_MODE_E enRemapMode;
} VB_POOL_S;

static Hashmap *vbMmapHash;
pthread_mutex_t hash_lock;

static atomic_bool vb_inited = ATOMIC_VAR_INIT(false);


static int _vb_hash_key(void *key)
{
	return (((uintptr_t)key) >> 10);
}

static bool _vb_hash_equals(void *keyA, void *keyB)
{
	return (keyA == keyB);
}

VB_BLK CVI_VB_GetBlockwithID(VB_POOL Pool, CVI_U32 u32BlkSize, MOD_ID_E modId)
{
	CVI_TRACE_VB(CVI_DBG_ERR, "Pls use CVI_VB_GetBlock\n");
	UNUSED(Pool);
	UNUSED(u32BlkSize);
	UNUSED(modId);
	return 0;
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
	CVI_S32 s32Ret, fd;
	struct cvi_vb_blk_cfg cfg;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return VB_INVALID_HANDLE;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.pool_id = Pool;
	cfg.blk_size = u32BlkSize;
	s32Ret = vb_ioctl_get_block(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_block fail, ret(%d)\n", s32Ret);
		return VB_INVALID_HANDLE;
	}
	return (VB_BLK)cfg.blk;
}

/* CVI_VB_ReleaseBlock: release a vb_blk.
 *
 * @param Block: the vb_blk going to be released.
 * @return: CVI_SUCCESS if success; others if fail.
 */
CVI_S32 CVI_VB_ReleaseBlock(VB_BLK Block)
{
	CVI_S32 s32Ret, fd;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	s32Ret = vb_ioctl_release_block(fd, Block);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_release_block fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

VB_BLK CVI_VB_PhysAddr2Handle(CVI_U64 u64PhyAddr)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_blk_info blk_info;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return VB_INVALID_HANDLE;
	}

	memset(&blk_info, 0, sizeof(blk_info));
	blk_info.phy_addr = u64PhyAddr;
	s32Ret = vb_ioctl_phys_to_handle(fd, &blk_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_phys_to_handle fail, ret(%d)\n", s32Ret);
		return VB_INVALID_HANDLE;
	}
	return (VB_BLK)blk_info.blk;
}

CVI_U64 CVI_VB_Handle2PhysAddr(VB_BLK Block)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_blk_info blk_info;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return 0;
	}

	memset(&blk_info, 0, sizeof(blk_info));
	blk_info.blk = Block;
	s32Ret = vb_ioctl_get_blk_info(fd, &blk_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_blk_info fail, ret(%d)\n", s32Ret);
		return 0;
	}
	return (CVI_U64)blk_info.phy_addr;
}

VB_POOL CVI_VB_Handle2PoolId(VB_BLK Block)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_blk_info blk_info;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return VB_INVALID_POOLID;
	}

	memset(&blk_info, 0, sizeof(blk_info));
	blk_info.blk = Block;
	s32Ret = vb_ioctl_get_blk_info(fd, &blk_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_blk_info fail, ret(%d)\n", s32Ret);
		return VB_INVALID_POOLID;
	}
	return (VB_POOL)blk_info.pool_id;
}

CVI_S32 CVI_VB_InquireUserCnt(VB_BLK Block, CVI_U32 *pCnt)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_blk_info blk_info;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, pCnt);
	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	memset(&blk_info, 0, sizeof(blk_info));
	blk_info.blk = Block;
	s32Ret = vb_ioctl_get_blk_info(fd, &blk_info);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_blk_info fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}
	*pCnt = blk_info.usr_cnt;
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_Init(void)
{
	CVI_S32 s32Ret, fd;
	bool expect = false;

	// Only init once until exit.
	if (!atomic_compare_exchange_strong(&vb_inited, &expect, true))
		return CVI_SUCCESS;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	s32Ret = vb_ioctl_init(fd);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_init fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_Exit(void)
{
	CVI_S32 s32Ret, fd;
	bool expect = true;

	// Only exit once.
	if (!atomic_compare_exchange_strong(&vb_inited, &expect, false))
		return CVI_SUCCESS;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	s32Ret = vb_ioctl_exit(fd);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_exit fail, ret(%d)\n", s32Ret);
		base_dev_close();
		return CVI_FAILURE;
	}

	base_dev_close();
	return CVI_SUCCESS;
}

VB_POOL CVI_VB_CreatePool(VB_POOL_CONFIG_S *pstVbPoolCfg)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_pool_cfg cfg;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbPoolCfg);
	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.blk_size = pstVbPoolCfg->u32BlkSize;
	cfg.blk_cnt = pstVbPoolCfg->u32BlkCnt;
	cfg.remap_mode = pstVbPoolCfg->enRemapMode;
	strncpy(cfg.pool_name, pstVbPoolCfg->acName, VB_POOL_NAME_LEN - 1);

	s32Ret = vb_ioctl_create_pool(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_create_pool fail, ret(%d)\n", s32Ret);
		return VB_INVALID_POOLID;
	}

	return (VB_POOL)cfg.pool_id;
}

VB_POOL CVI_VB_CreateExPool(VB_POOL_CONFIG_EX_S *pstVbPoolExCfg)
{
	CVI_S32 s32Ret, fd, i;
	struct cvi_vb_pool_ex_cfg cfg;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbPoolExCfg);
	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.blk_cnt = pstVbPoolExCfg->u32BlkCnt;
	for (i = 0; i < VB_POOL_MAX_BLK; i++) {
		cfg.au64PhyAddr[i][0] = pstVbPoolExCfg->astUserBlk[i].au64PhyAddr[0];
		cfg.au64PhyAddr[i][1] = pstVbPoolExCfg->astUserBlk[i].au64PhyAddr[1];
		cfg.au64PhyAddr[i][2] = pstVbPoolExCfg->astUserBlk[i].au64PhyAddr[2];
	}

	s32Ret = vb_ioctl_create_ex_pool(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_create_ex_pool fail, ret(%d)\n", s32Ret);
		return VB_INVALID_POOLID;
	}

	return (VB_POOL)cfg.pool_id;
}

CVI_S32 CVI_VB_DestroyPool(VB_POOL Pool)
{
	CVI_S32 s32Ret, fd;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	s32Ret = vb_ioctl_destroy_pool(fd, Pool);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_destroy_pool fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}
	if (vbMmapHash && hashmapGet(vbMmapHash, (void *)(uintptr_t)Pool))
		CVI_VB_MunmapPool(Pool);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_SetConfig(const VB_CONFIG_S *pstVbConfig)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_cfg cfg;
	CVI_U32 i;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbConfig);
	if (pstVbConfig->u32MaxPoolCnt > VB_COMM_POOL_MAX_CNT
		|| pstVbConfig->u32MaxPoolCnt == 0) {
		CVI_TRACE_VB(CVI_DBG_ERR, "Invalid vb u32MaxPoolCnt(%d)\n",
			pstVbConfig->u32MaxPoolCnt);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.comm_pool_cnt = pstVbConfig->u32MaxPoolCnt;
	for (i = 0; i < cfg.comm_pool_cnt; ++i) {
		cfg.comm_pool[i].blk_size = pstVbConfig->astCommPool[i].u32BlkSize;
		cfg.comm_pool[i].blk_cnt = pstVbConfig->astCommPool[i].u32BlkCnt;
		cfg.comm_pool[i].remap_mode = pstVbConfig->astCommPool[i].enRemapMode;
		strncpy(cfg.comm_pool[i].pool_name,
			pstVbConfig->astCommPool[i].acName, VB_POOL_NAME_LEN - 1);
	}
	s32Ret = vb_ioctl_set_config(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_set_config fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_GetConfig(VB_CONFIG_S *pstVbConfig)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_cfg cfg;
	CVI_U32 i;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, pstVbConfig);
	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	s32Ret = vb_ioctl_get_config(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_config fail, ret(%d)\n", s32Ret);
		return CVI_FAILURE;
	}

	memset(pstVbConfig, 0, sizeof(*pstVbConfig));
	pstVbConfig->u32MaxPoolCnt = cfg.comm_pool_cnt;
	for (i = 0; i < cfg.comm_pool_cnt; ++i) {
		pstVbConfig->astCommPool[i].u32BlkSize = cfg.comm_pool[i].blk_size;
		pstVbConfig->astCommPool[i].u32BlkCnt = cfg.comm_pool[i].blk_cnt;
		pstVbConfig->astCommPool[i].enRemapMode = cfg.comm_pool[i].remap_mode;
		strncpy(pstVbConfig->astCommPool[i].acName, cfg.comm_pool[i].pool_name,
			MAX_VB_POOL_NAME_LEN - 1);
	}
	return CVI_SUCCESS;
}

/* CVI_VB_MmapPool - mmap the whole pool to get virtual-address
 *
 * @param Pool: pool id
 * @return CVI_SUCCESS if success; others if fail
 */
CVI_S32 CVI_VB_MmapPool(VB_POOL Pool)
{
	CVI_S32 s32Ret, fd;
	struct cvi_vb_pool_cfg cfg;
	VB_POOL_S *pstVbPool = NULL;
	void *vaddr;

	if (!vbMmapHash) {
		vbMmapHash = hashmapCreate(20, _vb_hash_key, _vb_hash_equals);
		pthread_mutex_init(&hash_lock, NULL);
	}

	pthread_mutex_lock(&hash_lock);
	pstVbPool = hashmapGet(vbMmapHash, (void *)(uintptr_t)Pool);
	if (pstVbPool) {
		pthread_mutex_unlock(&hash_lock);
		return CVI_SUCCESS;
	}

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_NOTREADY;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.pool_id = Pool;

	s32Ret = vb_ioctl_get_pool_cfg(fd, &cfg);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_get_pool_cfg fail, ret(%d)\n", s32Ret);
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	pstVbPool = (VB_POOL_S *)malloc(sizeof(*pstVbPool));
	if (pstVbPool == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "malloc failed.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_NOMEM;
	}

	if (cfg.remap_mode == VB_REMAP_MODE_CACHED)
		vaddr = CVI_SYS_MmapCache(cfg.mem_base, cfg.blk_cnt * cfg.blk_size);
	else
		vaddr = CVI_SYS_Mmap(cfg.mem_base, cfg.blk_cnt * cfg.blk_size);
	if (vaddr == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "mmap failed.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_NOMEM;
	}

	pstVbPool->vmemBase = vaddr;
	pstVbPool->memBase = cfg.mem_base;
	pstVbPool->u32BlkSize = cfg.blk_size;
	pstVbPool->u32BlkCnt = cfg.blk_cnt;
	pstVbPool->enRemapMode = cfg.remap_mode;

	hashmapPut(vbMmapHash, (void *)(uintptr_t)Pool, (void *)pstVbPool);
	pthread_mutex_unlock(&hash_lock);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VB_MunmapPool(VB_POOL Pool)
{
	VB_POOL_S *pstVbPool = NULL;

	if (vbMmapHash == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "not mmap yet.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	pthread_mutex_lock(&hash_lock);
	pstVbPool = hashmapGet(vbMmapHash, (void *)(uintptr_t)Pool);
	if (pstVbPool == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "not mmap yet.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_NOTREADY;
	}

	if (CVI_SYS_Munmap(pstVbPool->vmemBase, pstVbPool->u32BlkCnt * pstVbPool->u32BlkSize) != CVI_SUCCESS) {
		CVI_TRACE_VB(CVI_DBG_ERR, "unmap failed.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	hashmapRemove(vbMmapHash, (void *)(uintptr_t)Pool);
	free(pstVbPool);
	pthread_mutex_unlock(&hash_lock);

	if (hashmapSize(vbMmapHash) == 0) {
		hashmapFree(vbMmapHash);
		vbMmapHash = NULL;
		pthread_mutex_destroy(&hash_lock);
	}
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
	VB_POOL poolId;
	CVI_U64 phyAddr;
	VB_POOL_S *pstVbPool = NULL;

	MOD_CHECK_NULL_PTR(CVI_ID_VB, ppVirAddr);

	if (vbMmapHash == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "not mmap yet.\n");
		return CVI_ERR_VB_NOTREADY;
	}

	pthread_mutex_lock(&hash_lock);
	pstVbPool = hashmapGet(vbMmapHash, (void *)(uintptr_t)Pool);
	if (pstVbPool == NULL) {
		CVI_TRACE_VB(CVI_DBG_ERR, "not mmap yet.\n");
		pthread_mutex_unlock(&hash_lock);
		return CVI_ERR_VB_NOTREADY;
	}
	pthread_mutex_unlock(&hash_lock);

	poolId = CVI_VB_Handle2PoolId(Block);
	if (poolId != Pool) {
		CVI_TRACE_VB(CVI_DBG_ERR, "Blk's Pool(%d) isn't given one(%d).\n", poolId, Pool);
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	phyAddr = CVI_VB_Handle2PhysAddr(Block);
	if (!phyAddr) {
		CVI_TRACE_VB(CVI_DBG_ERR, "phyAddr = 0.\n");
		return CVI_ERR_VB_ILLEGAL_PARAM;
	}

	*ppVirAddr = pstVbPool->vmemBase + (phyAddr - pstVbPool->memBase);
	return CVI_SUCCESS;
}

CVI_VOID CVI_VB_PrintPool(VB_POOL Pool)
{
	CVI_S32 s32Ret, fd;

	fd = get_base_fd();
	if (fd == -1) {
		CVI_TRACE_VB(CVI_DBG_ERR, "get_base_fd failed.\n");
		return;
	}
	s32Ret = vb_ioctl_print_pool(fd, Pool);
	if (s32Ret != CVI_SUCCESS)
		CVI_TRACE_VB(CVI_DBG_ERR, "vb_ioctl_print_pool fail, ret(%d)\n", s32Ret);
}

