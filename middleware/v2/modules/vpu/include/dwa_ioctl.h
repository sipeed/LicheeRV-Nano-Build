#ifndef MODULES_VPU_INCLUDE_DWA_IOCTL_H_
#define MODULES_VPU_INCLUDE_DWA_IOCTL_H_

#include <sys/ioctl.h>

#include <linux/dwa_uapi.h>

/* Configured from user  */
CVI_S32 gdc_begin_job(CVI_S32 fd, struct gdc_handle_data *cfg);
CVI_S32 gdc_end_job(CVI_S32 fd, struct gdc_handle_data *cfg);
CVI_S32 gdc_cancel_job(CVI_S32 fd, struct gdc_handle_data *cfg);
CVI_S32 gdc_add_rotation_task(CVI_S32 fd, struct gdc_task_attr *attr);
CVI_S32 gdc_add_ldc_task(CVI_S32 fd, struct gdc_task_attr *attr);

/* INTERNAL */
CVI_S32 gdc_set_chn_buf_wrap(CVI_S32 fd, const struct dwa_buf_wrap_cfg *cfg);
CVI_S32 gdc_get_chn_buf_wrap(CVI_S32 fd, struct dwa_buf_wrap_cfg *cfg);

#endif /* MODULES_VPU_INCLUDE_DWA_IOCTL_H_ */
