#ifndef __VO_IOCTL_H__
#define __VO_IOCTL_H__

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_vo.h>
#include <linux/cvi_vip.h>

#include <linux/vo_uapi.h>
#include <linux/vo_disp.h>

#include "cvi_base.h"

int vo_set_pattern(int fd, enum cvi_vip_pattern pattern);
int vo_set_mode(int fd, int mode);
int vo_set_frame_bgcolor(int fd, void *rgb);
int vo_set_window_bgcolor(int fd, void *rgb);
int vo_set_intf(int fd, struct cvi_disp_intf_cfg *cfg);
int vo_enable_window_bgcolor(int fd, int enable);
int vo_set_align(int fd, int align);
int vo_set_rgn(int fd, struct cvi_rgn_cfg *cfg);
int vo_set_csc(int fd, struct cvi_csc_cfg *cfg);
int vo_set_clk(int fd, CVI_U32 clk_freq);
int vo_set_i80_sw_mode(int fd, CVI_U32 enable);
int vo_send_i80_cmd(int fd, CVI_U32 cmd);
int vo_get_videolayer_size(int fd, SIZE_S *vsize);
int vo_get_panel_status(int fd, CVI_U32 *is_init);
int vo_get_intf_type(int fd, CVI_S32 *vo_sel);
int vo_set_gamma_ctrl(int fd, VO_GAMMA_INFO_S *gamma_attr);
int vo_get_gamma_ctrl(int fd, VO_GAMMA_INFO_S *gamma_attr);
int vo_set_tgt_compose(int fd, struct vo_rect *sel);
int vo_set_tgt_crop(int fd, struct vo_rect *sel);
int vo_set_dv_timings(int fd, struct vo_dv_timings *timings);
int vo_get_dv_timings(int fd, struct vo_dv_timings *timings);
int vo_set_start_streaming(int fd);
int vo_set_stop_streaming(int fd);
int vo_enq_buf(int fd, struct vo_buffer *buf);

//vo sdk layer apis
int vo_sdk_send_frame(int fd, struct vo_snd_frm_cfg *cfg);
int vo_sdk_get_panelstatue(int fd, struct vo_panel_status_cfg *cfg);
int vo_sdk_get_pubattr(int fd, struct vo_pub_attr_cfg *cfg);
int vo_sdk_set_pubattr(int fd, struct vo_pub_attr_cfg *cfg);
int vo_sdk_get_displaybuflen(int fd, struct vo_display_buflen_cfg *cfg);
int vo_sdk_set_displaybuflen(int fd, struct vo_display_buflen_cfg *cfg);
int vo_sdk_set_videolayerattr(int fd, struct vo_video_layer_attr_cfg *cfg);
int vo_sdk_get_videolayerattr(int fd, struct vo_video_layer_attr_cfg *cfg);
int vo_sdk_enable_videolayer(int fd, struct vo_video_layer_cfg *cfg);
int vo_sdk_disable_videolayer(int fd, struct vo_video_layer_cfg *cfg);
int vo_sdk_set_chnattr(int fd, struct vo_chn_attr_cfg *cfg);
int vo_sdk_get_chnattr(int fd, struct vo_chn_attr_cfg *cfg);
int vo_sdk_enable_chn(int fd, struct vo_chn_cfg *cfg);
int vo_sdk_disable_chn(int fd, struct vo_chn_cfg *cfg);
int vo_sdk_enable(int fd, struct vo_dev_cfg *cfg);
int vo_sdk_disable(int fd, struct vo_dev_cfg *cfg);
int vo_sdk_suspend(int fd);
int vo_sdk_resume(int fd);
int vo_sdk_clearchnbuf(int fd, struct vo_clear_chn_buf_cfg *cfg);
int vo_sdk_set_chnrotation(int fd, struct vo_chn_rotation_cfg *cfg);
int vo_sdk_get_chnrotation(int fd, struct vo_chn_rotation_cfg *cfg);

#endif
