#ifndef _CVI_VIP_GDC_PROC_H_
#define _CVI_VIP_GDC_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>

#include <linux/cvi_base_ctx.h>

int gdc_proc_init(void *shm);
int gdc_proc_remove(void);

#endif // _CVI_VIP_GDC_PROC_H_
