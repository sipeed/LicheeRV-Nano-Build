#ifndef MODULES_VB_IOCTL_H_
#define MODULES_VB_IOCTL_H_

#include <linux/types.h>
#include <linux/vb_uapi.h>
#include <linux/cvi_base.h>
#include "cvi_comm_vb.h"

int vb_ioctl_set_config(int fd, struct cvi_vb_cfg *cfg);
int vb_ioctl_get_config(int fd, struct cvi_vb_cfg *cfg);
int vb_ioctl_init(int fd);
int vb_ioctl_exit(int fd);
int vb_ioctl_create_pool(int fd, struct cvi_vb_pool_cfg *cfg);
int vb_ioctl_create_ex_pool(int fd, struct cvi_vb_pool_ex_cfg *cfg);
int vb_ioctl_destroy_pool(int fd, VB_POOL poolId);
int vb_ioctl_phys_to_handle(int fd, struct cvi_vb_blk_info *blk_info);
int vb_ioctl_get_blk_info(int fd, struct cvi_vb_blk_info *blk_info);
int vb_ioctl_get_pool_cfg(int fd, struct cvi_vb_pool_cfg *pool_cfg);
int vb_ioctl_get_block(int fd, struct cvi_vb_blk_cfg *blk_cfg);
int vb_ioctl_release_block(int fd, VB_BLK blk);
int vb_ioctl_get_pool_max_cnt(int fd, CVI_U32 *vb_max_pools);
int vb_ioctl_print_pool(int fd, VB_POOL poolId);
int vb_ioctl_unit_test(int fd, CVI_U32 op);

#endif // MODULES_VB_IOCTL_H_
