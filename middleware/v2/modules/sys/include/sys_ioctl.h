#ifndef _MODULES_SYS_INCLUDE_SYS_IOCTL_H_
#define _MODULES_SYS_INCLUDE_SYS_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/sys_uapi.h>

int sys_set_vivpssmode(int fd, const VI_VPSS_MODE_S *cfg);
int sys_set_vpssmode(int fd, VPSS_MODE_E val);
int sys_set_vpssmodeex(int fd, const VPSS_MODE_S *cfg);
int sys_set_sys_init(int fd);
int sys_get_vivpssmode(int fd, const VI_VPSS_MODE_S *cfg);
int sys_get_vpssmode(int fd, VPSS_MODE_E *val);
int sys_get_vpssmodeex(int fd, const VPSS_MODE_S *cfg);
int sys_get_sys_init(int fd, CVI_U32 *val);

#ifdef __cplusplus
}
#endif

#endif // _MODULES_SYS_INCLUDE_SYS_IOCTL_H_