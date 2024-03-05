#ifndef __VB_H__
#define __VB_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "linux/vb_uapi.h"
#include "linux/cvi_common.h"
#include "linux/cvi_base.h"
#include "base_ctx.h"

/* Generall common pool use this owner id, module common pool use VB_UID as owner id */
#define POOL_OWNER_COMMON -1
/* Private pool use this owner id */
#define POOL_OWNER_PRIVATE -2

#define VB_INVALID_POOLID       (-1U)
#define VB_INVALID_HANDLE       (-1U)
#define VB_STATIC_POOLID        (-2U)
#define VB_EXTERNAL_POOLID      (-3U)

#define CVI_VB_MAGIC 0xbabeface

int32_t vb_get_pool_info(struct vb_pool **pool_info);
void vb_cleanup(void);
int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig);

int32_t vb_create_pool(struct cvi_vb_pool_cfg *config);
int32_t vb_destroy_pool(uint32_t poolId);

VB_BLK vb_physAddr2Handle(uint64_t u64PhyAddr);
uint64_t vb_handle2PhysAddr(VB_BLK blk);
void *vb_handle2VirtAddr(VB_BLK blk);
VB_POOL vb_handle2PoolId(VB_BLK blk);
int32_t vb_inquireUserCnt(VB_BLK blk, uint32_t *pCnt);
int32_t vb_print_pool(VB_POOL poolId);
VB_POOL find_vb_pool(uint32_t u32BlkSize);

VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize, MOD_ID_E modId);
VB_BLK vb_create_block(uint64_t phyAddr, void *virAddr, VB_POOL VbPool, bool isExternal);
int32_t vb_release_block(VB_BLK blk);
void vb_acquire_block(vb_acquire_fp fp, MMF_CHN_S chn,
	uint32_t u32BlkSize, VB_POOL VbPool);
void vb_cancel_block(MMF_CHN_S chn, uint32_t u32BlkSize, VB_POOL VbPool);
long vb_ctrl(struct vb_ext_control *p);
int32_t vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk);
int32_t vb_dqbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK *blk);
int32_t vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, VB_BLK blk);

extern uint32_t vb_max_pools;
extern uint32_t vb_pool_max_blk;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __VB_H__ */
