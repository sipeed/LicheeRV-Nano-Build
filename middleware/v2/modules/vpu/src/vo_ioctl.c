#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/cvi_common.h>
#include <linux/cvi_comm_vo.h>
#include <linux/vo_uapi.h>
#include "vo_ioctl.h"

#define VO_S_CTRL_VALUE(_fd, _cfg, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = _cfg;\
		if (ioctl(_fd, VO_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_S_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

#define VO_S_CTRL_VALUE64(_fd, _cfg, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value64 = _cfg;\
		if (ioctl(_fd, VO_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_S_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

#define VO_SDK_CTRL_PTR(_fd, _cfg, _ioctl, _sdk_id)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.sdk_id = _sdk_id;\
		ec1.ptr = (void *)_cfg;\
		if (ioctl(_fd, VO_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_S_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

#define VO_S_CTRL_PTR(_fd, _cfg, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		if (ioctl(_fd, VO_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_S_CTRL - %s NG,%s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

#define VO_G_CTRL_VALUE(_fd, _out, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.value = 0;\
		if (ioctl(_fd, VO_IOC_G_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_G_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		*_out = ec1.value;\
		return 0;\
	} while (0)

#define VO_G_CTRL_PTR(_fd, _cfg, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		if (ioctl(_fd, VO_IOC_G_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_G_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

#define VO_S_CTRL_ARR(_fd, _cfg, _size, _ioctl)\
	do {\
		struct vo_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _ioctl;\
		ec1.ptr = (void *)_cfg;\
		ec1.size = _size;\
		if (ioctl(_fd, VO_IOC_S_CTRL, &ec1) < 0) {\
			fprintf(stderr, "VO_IOC_G_CTRL - %s NG, %s\n", __func__, strerror(errno));\
			return -1;\
		} \
		return 0;\
	} while (0)

int vo_set_pattern(int fd, enum cvi_vip_pattern pattern)
{
	VO_S_CTRL_VALUE(fd, pattern, VO_IOCTL_PATTERN);
}

int vo_set_mode(int fd, int mode)
{
	VO_S_CTRL_VALUE(fd, mode, VO_IOCTL_ONLINE);
}

int vo_set_frame_bgcolor(int fd, void *rgb)
{
	VO_S_CTRL_PTR(fd, rgb, VO_IOCTL_FRAME_BGCOLOR);
}

int vo_set_window_bgcolor(int fd, void *rgb)
{
	VO_S_CTRL_PTR(fd, rgb, VO_IOCTL_WINDOW_BGCOLOR);
}

int vo_set_intf(int fd, struct cvi_disp_intf_cfg *cfg)
{
	VO_S_CTRL_PTR(fd, cfg, VO_IOCTL_INTF);
}

int vo_enable_window_bgcolor(int fd, int enable)
{
	VO_S_CTRL_VALUE(fd, enable, VO_IOCTL_ENABLE_WIN_BGCOLOR);
}

int vo_set_align(int fd, int align)
{
	VO_S_CTRL_VALUE(fd, align, VO_IOCTL_SET_ALIGN);
}

int vo_set_rgn(int fd, struct cvi_rgn_cfg *cfg)
{
	VO_S_CTRL_PTR(fd, cfg, VO_IOCTL_SET_RGN);
}

int vo_set_csc(int fd, struct cvi_csc_cfg *cfg)
{
	VO_S_CTRL_PTR(fd, cfg, VO_IOCTL_SET_CUSTOM_CSC);
}

int vo_set_clk(int fd, CVI_U32 clk_freq)
{
	VO_S_CTRL_VALUE(fd, clk_freq, VO_IOCTL_SET_CLK);
}

int vo_set_i80_sw_mode(int fd, CVI_U32 enable)
{
	VO_S_CTRL_VALUE(fd, enable, VO_IOCTL_I80_SW_MODE);
}

int vo_send_i80_cmd(int fd, CVI_U32 cmd)
{
	VO_S_CTRL_VALUE(fd, cmd, VO_IOCTL_I80_CMD);
}

int vo_get_videolayer_size(int fd, SIZE_S *vsize)
{
	VO_S_CTRL_PTR(fd, vsize, VO_IOCTL_GET_VLAYER_SIZE);
}

int vo_get_intf_type(int fd, CVI_S32 *vo_sel)
{
	VO_G_CTRL_PTR(fd, vo_sel, VO_IOCTL_GET_INTF_TYPE);
}

int vo_get_panel_status(int fd, CVI_U32 *is_init)
{
	VO_G_CTRL_PTR(fd, is_init, VO_IOCTL_GET_PANEL_STATUS);
}

int vo_set_gamma_ctrl(int fd, VO_GAMMA_INFO_S *gamma_attr)
{
	VO_S_CTRL_PTR(fd, gamma_attr, VO_IOCTL_GAMMA_LUT_UPDATE);
}

int vo_get_gamma_ctrl(int fd, VO_GAMMA_INFO_S *gamma_attr)
{
	VO_G_CTRL_PTR(fd, gamma_attr, VO_IOCTL_GAMMA_LUT_READ);
}

int vo_set_tgt_compose(int fd, struct vo_rect *area)
{
	VO_S_CTRL_PTR(fd, area, VO_IOCTL_SEL_TGT_COMPOSE);
}

int vo_set_tgt_crop(int fd, struct vo_rect *area)
{
	VO_S_CTRL_PTR(fd, area, VO_IOCTL_SEL_TGT_CROP);
}

int vo_set_dv_timings(int fd, struct vo_dv_timings *timings)
{
	VO_S_CTRL_PTR(fd, timings, VO_IOCTL_SET_DV_TIMINGS);
}

int vo_get_dv_timings(int fd, struct vo_dv_timings *timings)
{
	VO_G_CTRL_PTR(fd, timings, VO_IOCTL_GET_DV_TIMINGS);
}

int vo_set_stop_streaming(int fd)
{
	VO_S_CTRL_VALUE(fd, 1, VO_IOCTL_STOP_STREAMING);
}

int vo_set_start_streaming(int fd)
{
	VO_S_CTRL_VALUE(fd, 1, VO_IOCTL_START_STREAMING);
}

int vo_enq_buf(int fd, struct vo_buffer *buf)
{
	VO_S_CTRL_PTR(fd, buf, VO_IOCTL_ENQ_BUF);
}

//vo sdk API list
int vo_sdk_clearchnbuf(int fd, struct vo_clear_chn_buf_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_CLEAR_CHNBUF);
}

int vo_sdk_send_frame(int fd, struct vo_snd_frm_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SEND_FRAME);
}

int vo_sdk_get_panelstatue(int fd, struct vo_panel_status_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_PANELSTATUE);

}

int vo_sdk_get_pubattr(int fd, struct vo_pub_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_PUBATTR);
}

int vo_sdk_set_pubattr(int fd, struct vo_pub_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SET_PUBATTR);
}

int vo_sdk_suspend(int fd)
{
	VO_SDK_CTRL_PTR(fd, NULL, VO_IOCTL_SDK_CTRL, VO_SDK_SUSPEND);
}

int vo_sdk_resume(int fd)
{
	VO_SDK_CTRL_PTR(fd, NULL, VO_IOCTL_SDK_CTRL, VO_SDK_RESUME);
}

int vo_sdk_get_displaybuflen(int fd, struct vo_display_buflen_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_DISPLAYBUFLEN);
}

int vo_sdk_set_displaybuflen(int fd, struct vo_display_buflen_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SET_DISPLAYBUFLEN);
}

int vo_sdk_set_videolayerattr(int fd, struct vo_video_layer_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SET_VIDEOLAYERATTR);
}

int vo_sdk_get_videolayerattr(int fd, struct vo_video_layer_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_VIDEOLAYERATTR);
}

int vo_sdk_enable_videolayer(int fd, struct vo_video_layer_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_ENABLE_VIDEOLAYER);
}

int vo_sdk_disable_videolayer(int fd, struct vo_video_layer_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_DISABLE_VIDEOLAYER);
}

int vo_sdk_set_chnattr(int fd, struct vo_chn_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SET_CHNATTR);
}

int vo_sdk_get_chnattr(int fd, struct vo_chn_attr_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_CHNATTR);
}

int vo_sdk_set_chnrotation(int fd, struct vo_chn_rotation_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_SET_CHNROTATION);
}

int vo_sdk_get_chnrotation(int fd, struct vo_chn_rotation_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_GET_CHNROTATION);
}

int vo_sdk_enable_chn(int fd, struct vo_chn_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_ENABLE_CHN);
}

int vo_sdk_disable_chn(int fd, struct vo_chn_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_DISABLE_CHN);
}

int vo_sdk_enable(int fd, struct vo_dev_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_ENABLE);
}

int vo_sdk_disable(int fd, struct vo_dev_cfg *cfg)
{
	VO_SDK_CTRL_PTR(fd, cfg, VO_IOCTL_SDK_CTRL, VO_SDK_DISABLE);
}

