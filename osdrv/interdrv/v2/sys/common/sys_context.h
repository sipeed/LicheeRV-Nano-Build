#ifndef __SYS_CONTEXT_H__
#define __SYS_CONTEXT_H__

#include <linux/types.h>
#include <linux/cvi_comm_sys.h>
#include "base.h"

struct sys_info {
	char version[VERSION_NAME_MAXLEN];
	uint32_t chip_id;
};

struct sys_mode_cfg {
	VI_VPSS_MODE_S vivpss_mode;
	VPSS_MODE_S vpss_mode;
};

struct sys_ctx_info {
	struct sys_info sys_info;
	struct sys_mode_cfg mode_cfg;
	atomic_t sys_inited;
};

struct mem_mapping {
	uint64_t phy_addr;
	int32_t dmabuf_fd;
	void *vir_addr;
	void *dmabuf;
	pid_t fd_pid;
	void *ionbuf;
};

int32_t sys_ctx_init(void);
struct sys_ctx_info *sys_get_ctx(void);
int32_t sys_ctx_mem_put(struct mem_mapping *mem_config);
int32_t sys_ctx_mem_get(struct mem_mapping *mem_config);
int32_t sys_ctx_mem_dump(void);

uint32_t sys_ctx_get_chipid(void);
uint8_t *sys_ctx_get_version(void);
void *sys_ctx_get_sysinfo(void);

VPSS_MODE_E sys_ctx_get_vpssmode(void);

void sys_ctx_release_bind(void);
int32_t sys_ctx_bind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn);
int32_t sys_ctx_unbind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn);

int32_t sys_ctx_get_bindbysrc(MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest);
int32_t sys_ctx_get_bindbydst(MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn);



#endif  /* __SYS_CONTEXT_H__ */

