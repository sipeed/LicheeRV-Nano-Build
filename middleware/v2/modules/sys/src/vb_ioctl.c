#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "vb_ioctl.h"

#define VB_CTRL_PTR(_fd, _cfg, _ioctl)\
	do {\
		struct vb_ext_control ec1;\
		int ret;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		ret = ioctl(_fd, IOCTL_VB_CMD, &ec1);\
		if (ret < 0)\
			fprintf(stderr, "IOCTL_VB_CMD - %s NG\n", __func__);\
		return ret;\
	} while (0)

#define VB_CTRL_S_VALUE(_fd, _cfg, _ioctl)\
	do {\
		struct vb_ext_control ec1;\
		int ret;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = _cfg;\
		ret = ioctl(_fd, IOCTL_VB_CMD, &ec1);\
		if (ret < 0)\
			fprintf(stderr, "IOCTL_VB_CMD - %s NG\n", __func__);\
		return ret;\
	} while (0)

#define VB_CTRL_S_VALUE64(_fd, _cfg, _ioctl)\
	do {\
		struct vb_ext_control ec1;\
		int ret;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value64 = _cfg;\
		ret = ioctl(_fd, IOCTL_VB_CMD, &ec1);\
		if (ret < 0)\
			fprintf(stderr, "IOCTL_VB_CMD - %s NG\n", __func__);\
		return ret;\
	} while (0)

#define VB_CTRL_G_VALUE(_fd, _out, _ioctl)\
	do {\
		struct vb_ext_control ec1;\
		int ret;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = 0;\
		ret = ioctl(_fd, IOCTL_VB_CMD, &ec1);\
		if (ret < 0)\
			fprintf(stderr, "IOCTL_VB_CMD - %s NG\n", __func__);\
		*_out = ec1.value;\
		return ret;\
	} while (0)

int vb_ioctl_set_config(int fd, struct cvi_vb_cfg *cfg)
{
	VB_CTRL_PTR(fd, cfg, VB_IOCTL_SET_CONFIG);
}

int vb_ioctl_get_config(int fd, struct cvi_vb_cfg *cfg)
{
	VB_CTRL_PTR(fd, cfg, VB_IOCTL_GET_CONFIG);
}

int vb_ioctl_init(int fd)
{
	VB_CTRL_PTR(fd, NULL, VB_IOCTL_INIT);
}

int vb_ioctl_exit(int fd)
{
	VB_CTRL_PTR(fd, NULL, VB_IOCTL_EXIT);
}

int vb_ioctl_create_pool(int fd, struct cvi_vb_pool_cfg *cfg)
{
	VB_CTRL_PTR(fd, cfg, VB_IOCTL_CREATE_POOL);
}

int vb_ioctl_create_ex_pool(int fd, struct cvi_vb_pool_ex_cfg *cfg)
{
	VB_CTRL_PTR(fd, cfg, VB_IOCTL_CREATE_EX_POOL);
}

int vb_ioctl_destroy_pool(int fd, VB_POOL poolId)
{
	VB_CTRL_S_VALUE(fd, poolId, VB_IOCTL_DESTROY_POOL);
}

int vb_ioctl_phys_to_handle(int fd, struct cvi_vb_blk_info *blk_info)
{
	VB_CTRL_PTR(fd, blk_info, VB_IOCTL_PHYS_TO_HANDLE);
}

int vb_ioctl_get_blk_info(int fd, struct cvi_vb_blk_info *blk_info)
{
	VB_CTRL_PTR(fd, blk_info, VB_IOCTL_GET_BLK_INFO);
}

int vb_ioctl_get_pool_cfg(int fd, struct cvi_vb_pool_cfg *pool_cfg)
{
	VB_CTRL_PTR(fd, pool_cfg, VB_IOCTL_GET_POOL_CFG);
}

int vb_ioctl_get_block(int fd, struct cvi_vb_blk_cfg *blk_cfg)
{
	VB_CTRL_PTR(fd, blk_cfg, VB_IOCTL_GET_BLOCK);
}

int vb_ioctl_release_block(int fd, VB_BLK blk)
{
	VB_CTRL_S_VALUE64(fd, blk, VB_IOCTL_RELEASE_BLOCK);
}

int vb_ioctl_get_pool_max_cnt(int fd, CVI_U32 *vb_max_pools)
{
	VB_CTRL_G_VALUE(fd, vb_max_pools, VB_IOCTL_GET_POOL_MAX_CNT);
}

int vb_ioctl_print_pool(int fd, VB_POOL poolId)
{
	VB_CTRL_S_VALUE(fd, poolId, VB_IOCTL_PRINT_POOL);
}

int vb_ioctl_unit_test(int fd, CVI_U32 op)
{
	VB_CTRL_S_VALUE(fd, op, VB_IOCTL_UNIT_TEST);
}
