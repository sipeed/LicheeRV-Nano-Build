#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "dwa_ioctl.h"

CVI_S32 gdc_begin_job(CVI_S32 fd, struct gdc_handle_data *cfg)
{
	return ioctl(fd, CVI_DWA_BEGIN_JOB, cfg);
}

CVI_S32 gdc_end_job(CVI_S32 fd, struct gdc_handle_data *cfg)
{
	return ioctl(fd, CVI_DWA_END_JOB, cfg);
}

CVI_S32 gdc_cancel_job(CVI_S32 fd, struct gdc_handle_data *cfg)
{
	return ioctl(fd, CVI_DWA_CANCEL_JOB, cfg);
}

CVI_S32 gdc_add_rotation_task(CVI_S32 fd, struct gdc_task_attr *attr)
{
	return ioctl(fd, CVI_DWA_ADD_ROT_TASK, attr);
}

CVI_S32 gdc_add_ldc_task(CVI_S32 fd, struct gdc_task_attr *attr)
{
	return ioctl(fd, CVI_DWA_ADD_LDC_TASK, attr);
}

CVI_S32 gdc_set_chn_buf_wrap(CVI_S32 fd, const struct dwa_buf_wrap_cfg *cfg)
{
	return ioctl(fd, CVI_DWA_SET_BUF_WRAP, cfg);
}

CVI_S32 gdc_get_chn_buf_wrap(CVI_S32 fd, struct dwa_buf_wrap_cfg *cfg)
{
	return ioctl(fd, CVI_DWA_GET_BUF_WRAP, cfg);
}
