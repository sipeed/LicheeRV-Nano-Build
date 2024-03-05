#ifndef __VCODEC_CB_H__
#define __VCODEC_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <base_ctx.h>

enum VCODEC_CB_CMD {
	VCODEC_CB_SEND_FRM,
	VCODEC_CB_SKIP_FRM,
	VCODEC_CB_SNAP_JPG_FRM,
	VCODEC_CB_SWITCH_CHN,
	VCODEC_CB_OVERFLOW_CHECK,
	VCODEC_CB_MAX
};

struct venc_send_frm_info {
	CVI_S32 vpss_grp;
	CVI_S32 vpss_chn;
	CVI_S32 vpss_chn1;
	struct cvi_buffer stInFrmBuf;
	struct cvi_buffer stInFrmBuf1;
	CVI_BOOL isOnline;
	CVI_U32 sb_nb;
};

struct venc_snap_frm_info {
	CVI_S32 vpss_grp;
	CVI_S32 vpss_chn;
	CVI_U32 skip_frm_cnt;
};

struct venc_switch_chn {
	CVI_S32 vpss_grp;
	CVI_S32 vpss_chn;
};

#ifdef __cplusplus
}
#endif

#endif /* __VCODEC_CB_H__ */

