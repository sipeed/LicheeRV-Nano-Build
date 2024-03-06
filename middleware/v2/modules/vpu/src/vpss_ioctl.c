#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

#include "vpss_ioctl.h"

CVI_S32 vpss_send_frame(CVI_S32 fd, struct vpss_snd_frm_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SEND_FRAME, cfg);
}

CVI_S32 vpss_send_chn_frame(CVI_S32 fd, struct vpss_chn_frm_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SEND_CHN_FRAME, cfg);
}

CVI_S32 vpss_release_chn_frame(CVI_S32 fd, const struct vpss_chn_frm_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_RELEASE_CHN_FRAME, cfg);
}

CVI_S32 vpss_create_grp(CVI_S32 fd, struct vpss_crt_grp_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_CREATE_GROUP, cfg);
}

CVI_S32 vpss_destroy_grp(CVI_S32 fd, VPSS_GRP VpssGrp)
{
	return ioctl(fd, CVI_VPSS_DESTROY_GROUP, &VpssGrp);
}

CVI_S32 vpss_get_available_grp(CVI_S32 fd, VPSS_GRP *pVpssGrp)
{
	return ioctl(fd, CVI_VPSS_GET_AVAIL_GROUP, pVpssGrp);
}

CVI_S32 vpss_start_grp(CVI_S32 fd, struct vpss_str_grp_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_START_GROUP, cfg);
}

CVI_S32 vpss_stop_grp(CVI_S32 fd, VPSS_GRP VpssGrp)
{
	return ioctl(fd, CVI_VPSS_STOP_GROUP, &VpssGrp);
}

CVI_S32 vpss_reset_grp(CVI_S32 fd, VPSS_GRP VpssGrp)
{
	return ioctl(fd, CVI_VPSS_RESET_GROUP, &VpssGrp);
}

CVI_S32 vpss_set_grp_attr(CVI_S32 fd, const struct vpss_grp_attr *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_GRP_ATTR, cfg);
}

CVI_S32 vpss_get_grp_attr(CVI_S32 fd, struct vpss_grp_attr *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_GRP_ATTR, cfg);
}

CVI_S32 vpss_set_grp_crop(CVI_S32 fd, const struct vpss_grp_crop_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_GRP_CROP, cfg);
}

CVI_S32 vpss_get_grp_crop(CVI_S32 fd, struct vpss_grp_crop_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_GRP_CROP, cfg);
}

CVI_VOID _vpss_pack_fixed_point_norm(VPSS_NORMALIZE_S *pstNormalize)
{
	CVI_U16 sc_frac[3];
	CVI_U8 sub[3];
	CVI_U16 sub_frac[3];
	CVI_DOUBLE sub_int;
	VPSS_NORMALIZE_S stNormalize;
	struct vpss_int_normalize *int_norm;

	if (!pstNormalize->bEnable)
		return;

	memcpy(&stNormalize, pstNormalize, sizeof(stNormalize));

	for (CVI_U8 i = 0; i < 3; ++i) {
		sc_frac[i] = pstNormalize->factor[i] * 8192;
		sub_frac[i]
			= modf(pstNormalize->mean[i], &sub_int) * 1024;
		sub[i] = sub_int;
	}

	int_norm = (struct vpss_int_normalize *)pstNormalize;
	int_norm->enable = 1;
	int_norm->rounding = pstNormalize->rounding;

	for (CVI_U8 i = 0; i < 3; ++i) {
		int_norm->sc_frac[i] = sc_frac[i];
		int_norm->sub[i] = sub[i];
		int_norm->sub_frac[i] = sub_frac[i];
	}
}

CVI_S32 vpss_set_chn_attr(CVI_S32 fd, struct vpss_chn_attr *attr)
{
	if (sizeof(struct vpss_int_normalize) > sizeof(VPSS_NORMALIZE_S)) {
		fprintf(stderr, "Kernel cannnot use float\n");
		return CVI_FAILURE;
	}

	_vpss_pack_fixed_point_norm(&attr->stChnAttr.stNormalize);

	return ioctl(fd, CVI_VPSS_SET_CHN_ATTR, attr);
}

CVI_S32 vpss_get_chn_attr(CVI_S32 fd, struct vpss_chn_attr *attr)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_ATTR, attr);
}

CVI_S32 vpss_show_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SHOW_CHN, cfg);
}

CVI_S32 vpss_hide_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_HIDE_CHN, cfg);
}

CVI_S32 vpss_enable_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_ENABLE_CHN, cfg);
}

CVI_S32 vpss_disable_chn(CVI_S32 fd, struct vpss_en_chn_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_DISABLE_CHN, cfg);
}

CVI_S32 vpss_get_chn_frame(CVI_S32 fd, struct vpss_chn_frm_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_FRAME, cfg);
}

CVI_S32 vpss_set_chn_wrap(CVI_S32 fd, const struct vpss_chn_wrap_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_BUF_WRAP, cfg);
}

CVI_S32 vpss_get_chn_wrap(CVI_S32 fd, struct vpss_chn_wrap_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_BUF_WRAP, cfg);
}

CVI_S32 vpss_set_chn_rotation(CVI_S32 fd, const struct vpss_chn_rot_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_ROTATION, cfg);
}

CVI_S32 vpss_get_chn_rotation(CVI_S32 fd, struct vpss_chn_rot_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_ROTATION, cfg);
}

CVI_S32 vpss_set_chn_align(CVI_S32 fd, const struct vpss_chn_align_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_ALIGN, cfg);
}

CVI_S32 vpss_get_chn_align(CVI_S32 fd, struct vpss_chn_align_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_ALIGN, cfg);
}

CVI_S32 vpss_set_chn_ldc(CVI_S32 fd, const struct vpss_chn_ldc_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_LDC, cfg);
}

CVI_S32 vpss_get_chn_ldc(CVI_S32 fd, struct vpss_chn_ldc_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_LDC, cfg);
}

CVI_S32 vpss_set_chn_crop(CVI_S32 fd, const struct vpss_chn_crop_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_CROP, cfg);
}

CVI_S32 vpss_get_chn_crop(CVI_S32 fd, struct vpss_chn_crop_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_CROP, cfg);
}

CVI_S32 vpss_set_coef_level(CVI_S32 fd, const struct vpss_chn_coef_level_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_SCALE_COEFF_LEVEL, cfg);
}

CVI_S32 vpss_get_coef_level(CVI_S32 fd, struct vpss_chn_coef_level_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_SCALE_COEFF_LEVEL, cfg);
}

CVI_S32 vpss_set_chn_yratio(CVI_S32 fd, const struct vpss_chn_yratio_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_CHN_YRATIO, cfg);
}

CVI_S32 vpss_get_chn_yratio(CVI_S32 fd, struct vpss_chn_yratio_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_CHN_YRATIO, cfg);
}

CVI_S32 vpss_set_grp_csc(CVI_S32 fd, const struct vpss_grp_csc_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_SET_GRP_CSC_CFG, cfg);
}

CVI_S32 vpss_attach_vbpool(CVI_S32 fd, const struct vpss_vb_pool_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_ATTACH_VB_POOL, cfg);
}

CVI_S32 vpss_detach_vbpool(CVI_S32 fd, const struct vpss_vb_pool_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_DETACH_VB_POOL, cfg);
}

CVI_S32 vpss_trigger_snap_frame(CVI_S32 fd, struct vpss_snap_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_TRIGGER_SNAP_FRAME, cfg);
}

CVI_S32 vpss_get_proc_amp_ctrl(CVI_S32 fd, struct vpss_proc_amp_ctrl_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_AMP_CTRL, cfg);
}

CVI_S32 vpss_get_proc_amp(CVI_S32 fd, struct vpss_proc_amp_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_AMP_CFG, cfg);
}

CVI_S32 vpss_get_all_proc_amp(CVI_S32 fd, struct vpss_all_proc_amp_cfg *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_ALL_AMP, cfg);
}

CVI_S32 vpss_get_binscene(CVI_S32 fd, struct vpss_scene *cfg)
{
	return ioctl(fd, CVI_VPSS_GET_SCENE, cfg);
}


