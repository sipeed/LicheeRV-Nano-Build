#ifndef _CVI_VIP_RGN_PROC_H_
#define _CVI_VIP_RGN_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>

#include <linux/cvi_rgn_ctx.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_region.h>
#include <linux/cvi_comm_vo.h>

#define GDC_PROC_JOB_INFO_NUM      (500)
#define RGN_PROC_INFO_OFFSET       (sizeof(struct cvi_vpss_proc_ctx) * VPSS_MAX_GRP_NUM)

int rgn_proc_init(void);
int rgn_proc_remove(void);

#endif // _CVI_VIP_RGN_PROC_H_
