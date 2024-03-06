#include <stdbool.h>
#include <stdint.h>
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

#include <linux/cvi_comm_video.h>
#include "rgn_ioctl.h"

#include <linux/rgn_uapi.h>

static inline CVI_S32 S_CTRL_PTR(int _fd, void *_cfg, int _ioctl, CVI_U32 _handle)
{
	struct rgn_ext_control ec1;

	memset(&ec1, 0, sizeof(ec1));
	ec1.id = _ioctl;
	ec1.handle = _handle;
	ec1.ptr1 = (void *) _cfg;

	if (ioctl(_fd, RGN_IOC_S_CTRL, &ec1) < 0) {
		fprintf(stderr, "RGN_IOC_S_CTRL - %s NG, %s\n", __func__, strerror(errno));
		return -1;
	}
	return 0;
}

static inline CVI_S32 SDK_CTRL_GET_CFG(int _fd, void *_cfg1, void *_cfg2, int _ioctl, CVI_U32 _handle)
{
	struct rgn_ext_control ec1;

	memset(&ec1, 0, sizeof(ec1));
	ec1.id = RGN_IOCTL_SDK_CTRL;
	ec1.sdk_id = _ioctl;
	ec1.handle = _handle;
	ec1.ptr1 = _cfg1;
	ec1.ptr2 = _cfg2;

	if (ioctl(_fd, RGN_IOC_G_CTRL, &ec1) < 0) {
		fprintf(stderr, "RGN_SDK_IOC_G_CTRL(%d-%d) - %s NG, %s\n",
			ec1.id, ec1.sdk_id, __func__, strerror(errno));
		return -1;
	}
	return 0;
}

static inline CVI_S32 SDK_CTRL_SET_CFG(int _fd, void *_cfg1, void *_cfg2, int _ioctl, CVI_U32 _handle)
{
	struct rgn_ext_control ec1;

	memset(&ec1, 0, sizeof(ec1));
	ec1.id = RGN_IOCTL_SDK_CTRL;
	ec1.sdk_id = _ioctl;
	ec1.handle = _handle;
	ec1.ptr1 = _cfg1;
	ec1.ptr2 = _cfg2;

	if (ioctl(_fd, RGN_IOC_S_CTRL, &ec1) < 0) {
		fprintf(stderr, "RGN_SDK_IOC_S_CTRL(%d-%d) - %s NG, %s\n",
			ec1.id, ec1.sdk_id, __func__, strerror(errno));
		return -1;
	}
	return 0;
}

int rgn_create(int fd, int Handle, const RGN_ATTR_S *pstRegion)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstRegion, NULL, RGN_SDK_CREATE, Handle);
}

int rgn_destroy(int fd, int Handle)
{
	return SDK_CTRL_SET_CFG(fd, NULL, NULL, RGN_SDK_DESTORY, Handle);
}

int rgn_get_attr(int fd, int Handle, RGN_ATTR_S *pstRegion)
{
	return SDK_CTRL_GET_CFG(fd, (void *)pstRegion, NULL, RGN_SDK_GET_ATTR, Handle);
}

int rgn_set_attr(int fd, int Handle, const RGN_ATTR_S *pstRegion)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstRegion, NULL, RGN_SDK_SET_ATTR, Handle);
}

int rgn_set_bit_map(int fd, int Handle, const BITMAP_S *pstBitmap)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstBitmap, NULL, RGN_SDK_SET_BIT_MAP, Handle);
}

int rgn_attach_to_chn(int fd, int Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstChn, (void *)pstChnAttr, RGN_SDK_ATTACH_TO_CHN, Handle);
}

int rgn_detach_from_chn(int fd, int Handle, const MMF_CHN_S *pstChn)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstChn, NULL, RGN_SDK_DETACH_FROM_CHN, Handle);
}

int rgn_set_display_attr(int fd, int Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstChn, (void *)pstChnAttr, RGN_SDK_SET_DISPLAY_ATTR, Handle);
}

int rgn_get_display_attr(int fd, int Handle, const MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr)
{
	return SDK_CTRL_GET_CFG(fd, (void *)pstChn, pstChnAttr, RGN_SDK_GET_DISPLAY_ATTR, Handle);
}

int rgn_get_canvas_info(int fd, int Handle, RGN_CANVAS_INFO_S *pstCanvasInfo)
{
	return SDK_CTRL_GET_CFG(fd, (void *)pstCanvasInfo, NULL, RGN_SDK_GET_CANVAS_INFO, Handle);
}

int rgn_update_canvas(int fd, int Handle)
{
	return SDK_CTRL_SET_CFG(fd, NULL, NULL, RGN_SDK_UPDATE_CANVAS, Handle);
}

int rgn_invert_color(int fd, int Handle, MMF_CHN_S *pstChn, void *pu32Color)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstChn, pu32Color, RGN_SDK_INVERT_COLOR, Handle);
}

int rgn_set_chn_palette(int fd, int Handle, const MMF_CHN_S *pstChn, RGN_PALETTE_S *pstPalette)
{
	return SDK_CTRL_SET_CFG(fd, (void *)pstChn, (void *)pstPalette, RGN_SDK_SET_CHN_PALETTE, Handle);
}
