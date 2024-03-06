#ifndef MODULES_VPU_INCLUDE_VPSS_IOCTL_H_
#define MODULES_VPU_INCLUDE_VPSS_IOCTL_H_

#include <sys/ioctl.h>

#include <linux/vpss_uapi.h>

/* Configured from user  */
CVI_S32 vpss_create_grp(CVI_S32 fd, struct vpss_crt_grp_cfg *cfg);
CVI_S32 vpss_destroy_grp(CVI_S32 fd, VPSS_GRP VpssGrp);
CVI_S32 vpss_get_available_grp(CVI_S32 fd, VPSS_GRP *pVpssGrp);

CVI_S32 vpss_start_grp(CVI_S32 fd, struct vpss_str_grp_cfg *cfg);
CVI_S32 vpss_stop_grp(CVI_S32 fd, VPSS_GRP VpssGrp);

CVI_S32 vpss_reset_grp(CVI_S32 fd, VPSS_GRP VpssGrp);

CVI_S32 vpss_set_grp_attr(CVI_S32 fd, const struct vpss_grp_attr *cfg);
CVI_S32 vpss_get_grp_attr(CVI_S32 fd, struct vpss_grp_attr *cfg);

CVI_S32 vpss_set_grp_crop(CVI_S32 fd, const struct vpss_grp_crop_cfg *cfg);
CVI_S32 vpss_get_grp_crop(CVI_S32 fd, struct vpss_grp_crop_cfg *cfg);

CVI_S32 vpss_send_frame(CVI_S32 fd, struct vpss_snd_frm_cfg *cfg);
CVI_S32 vpss_send_chn_frame(CVI_S32 fd, struct vpss_chn_frm_cfg *cfg);

CVI_S32 vpss_set_chn_attr(CVI_S32 fd, struct vpss_chn_attr *attr);
CVI_S32 vpss_get_chn_attr(CVI_S32 fd, struct vpss_chn_attr *attr);

CVI_S32 vpss_enable_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg);
CVI_S32 vpss_disable_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg);

CVI_S32 vpss_set_chn_crop(CVI_S32 fd, const struct vpss_chn_crop_cfg *cfg);
CVI_S32 vpss_get_chn_crop(CVI_S32 fd, struct vpss_chn_crop_cfg *cfg);

CVI_S32 vpss_set_chn_rotation(CVI_S32 fd, const struct vpss_chn_rot_cfg *cfg);
CVI_S32 vpss_get_chn_rotation(CVI_S32 fd, struct vpss_chn_rot_cfg *cfg);

CVI_S32 vpss_show_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg);
CVI_S32 vpss_hide_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg);

CVI_S32 vpss_set_chn_ldc(CVI_S32 fd, const struct vpss_chn_ldc_cfg *cfg);
CVI_S32 vpss_get_chn_ldc(CVI_S32 fd, struct vpss_chn_ldc_cfg *cfg);

CVI_S32 vpss_get_chn_frame(CVI_S32 fd, struct vpss_chn_frm_cfg *cfg);

CVI_S32 vpss_release_chn_frame(CVI_S32 fd, const struct vpss_chn_frm_cfg *cfg);

CVI_S32 vpss_attach_vbpool(CVI_S32 fd, const struct vpss_vb_pool_cfg *cfg);
CVI_S32 vpss_detach_vbpool(CVI_S32 fd, const struct vpss_vb_pool_cfg *cfg);

CVI_S32 vpss_set_chn_align(CVI_S32 fd, const struct vpss_chn_align_cfg *cfg);
CVI_S32 vpss_get_chn_align(CVI_S32 fd, struct vpss_chn_align_cfg *cfg);

CVI_S32 vpss_set_chn_yratio(CVI_S32 fd, const struct vpss_chn_yratio_cfg *cfg);
CVI_S32 vpss_get_chn_yratio(CVI_S32 fd, struct vpss_chn_yratio_cfg *cfg);

CVI_S32 vpss_set_coef_level(CVI_S32 fd, const struct vpss_chn_coef_level_cfg *cfg);
CVI_S32 vpss_get_coef_level(CVI_S32 fd, struct vpss_chn_coef_level_cfg *cfg);

CVI_S32 vpss_set_chn_wrap(CVI_S32 fd, const struct vpss_chn_wrap_cfg *cfg);
CVI_S32 vpss_get_chn_wrap(CVI_S32 fd, struct vpss_chn_wrap_cfg *cfg);

CVI_S32 vpss_trigger_snap_frame(CVI_S32 fd, struct vpss_snap_cfg *cfg);

CVI_S32 vpss_get_proc_amp_ctrl(CVI_S32 fd, struct vpss_proc_amp_ctrl_cfg *cfg);
CVI_S32 vpss_get_proc_amp(CVI_S32 fd, struct vpss_proc_amp_cfg *cfg);
CVI_S32 vpss_get_all_proc_amp(CVI_S32 fd, struct vpss_all_proc_amp_cfg *cfg);

/* INTERNAL */
CVI_S32 vpss_set_grp_csc(CVI_S32 fd, const struct vpss_grp_csc_cfg *csc_cfg);
CVI_S32 vpss_get_binscene(CVI_S32 fd, struct vpss_scene *csc_cfg);


#endif /* MODULES_VPU_INCLUDE_VPSS_IOCTL_H_ */
