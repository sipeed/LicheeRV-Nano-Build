#ifndef _CVI_VIP_VPSS_PROC_H_
#define _CVI_VIP_VPSS_PROC_H_

#include <linux/seq_file.h>
#include <generated/compile.h>

#include <linux/cvi_vpss_ctx.h>
#include <linux/cvi_base_ctx.h>

#include "vpss_core.h"

int vpss_proc_init(struct cvi_vip_dev *dev);
int vpss_proc_remove(struct cvi_vip_dev *dev);

#endif // _CVI_VIP_VPSS_PROC_H_
