#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/cvi_comm_sys.h>
#include "sys_ioctl.h"

#define SYS_S_CTRL_VALUE(_fd, _cfg, _ioctl)\
	do {\
		struct sys_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = _cfg;\
		if (ioctl(_fd, SYS_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "SYS_IOC_S_CTRL - %s NG\n", __func__);\
			return -1;\
		} \
		return 0;\
	} while (0)

#define SYS_S_CTRL_VALUE64(_fd, _cfg, _ioctl)\
	do {\
		struct sys_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value64 = _cfg;\
		if (ioctl(_fd, SYS_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "SYS_IOC_S_CTRL - %s NG\n", __func__);\
			perror("SYS_IOC_S_CTRL\n");\
			return -1;\
		} \
		return 0;\
	} while (0)

#define SYS_S_CTRL_PTR(_fd, _cfg, _ioctl)\
	do {\
		struct sys_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		if (ioctl(_fd, SYS_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "SYS_IOC_S_CTRL - %s NG\n", __func__);\
			perror("SYS_IOC_S_CTRL\n");\
			return -1;\
		} \
		return 0;\
	} while (0)

#define SYS_G_CTRL_VALUE(_fd, _out, _ioctl)\
	do {\
		struct sys_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = 0;\
		if (ioctl(_fd, SYS_IOC_G_CTRL, &ec1) < 0) {\
			fprintf(stderr, "SYS_IOC_G_CTRL - %s NG\n", __func__);\
			perror("SYS_IOC_G_CTRL\n");\
			return -1;\
		} \
		*_out = ec1.value;\
		return 0;\
	} while (0)

#define SYS_G_CTRL_PTR(_fd, _cfg, _ioctl)\
	do {\
		struct sys_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		if (ioctl(_fd, SYS_IOC_G_CTRL, &ec1) < 0) {\
			fprintf(stderr, "SYS_IOC_G_CTRL - %s NG\n", __func__);\
			perror("SYS_IOC_G_CTRL\n");\
			return -1;\
		} \
		return 0;\
	} while (0)

int sys_set_vivpssmode(int fd, const VI_VPSS_MODE_S *cfg)
{
	SYS_S_CTRL_PTR(fd, cfg, SYS_IOCTL_SET_VIVPSSMODE);
}

int sys_set_vpssmode(int fd, VPSS_MODE_E val)
{
	SYS_S_CTRL_VALUE(fd, val, SYS_IOCTL_SET_VPSSMODE);
}

int sys_set_vpssmodeex(int fd, const VPSS_MODE_S *cfg)
{
	SYS_S_CTRL_PTR(fd, cfg, SYS_IOCTL_SET_VPSSMODE_EX);
}

int sys_set_sys_init(int fd)
{
	SYS_S_CTRL_VALUE(fd, 0, SYS_IOCTL_SET_SYS_INIT);
}

int sys_get_vivpssmode(int fd, const VI_VPSS_MODE_S *cfg)
{
	SYS_G_CTRL_PTR(fd, cfg, SYS_IOCTL_GET_VIVPSSMODE);
}

int sys_get_vpssmode(int fd, VPSS_MODE_E *val)
{
	SYS_G_CTRL_VALUE(fd, val, SYS_IOCTL_GET_VPSSMODE);
}

int sys_get_vpssmodeex(int fd, const VPSS_MODE_S *cfg)
{
	SYS_G_CTRL_PTR(fd, cfg, SYS_IOCTL_GET_VPSSMODE_EX);
}

int sys_get_sys_init(int fd, CVI_U32 *val)
{
	SYS_G_CTRL_VALUE(fd, val, SYS_IOCTL_GET_SYS_INIT);
}
