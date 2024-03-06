#ifndef MODULES_VPU_INCLUDE_RGN_IOCTL_H_
#define MODULES_VPU_INCLUDE_RGN_IOCTL_H_

#include <linux/rgn_uapi.h>
#include <linux/cvi_comm_region.h>

int rgn_create(int fd, int Handle, const RGN_ATTR_S *pstRegion);
int rgn_destroy(int fd, int Handle);
int rgn_get_attr(int fd, int Handle, RGN_ATTR_S *pstRegion);
int rgn_set_attr(int fd, int Handle, const RGN_ATTR_S *pstRegion);
int rgn_set_bit_map(int fd, int Handle, const BITMAP_S *pstBitmap);
int rgn_attach_to_chn(int fd, int Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
int rgn_detach_from_chn(int fd, int Handle, const MMF_CHN_S *pstChn);
int rgn_set_display_attr(int fd, int Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
int rgn_get_display_attr(int fd, int Handle, const MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr);
int rgn_get_canvas_info(int fd, int Handle, RGN_CANVAS_INFO_S *pstCanvasInfo);
int rgn_update_canvas(int fd, int Handle);
int rgn_invert_color(int fd, int Handle, MMF_CHN_S *pstChn, void *pu32Color);
int rgn_set_chn_palette(int fd, int Handle, const MMF_CHN_S *pstChn, RGN_PALETTE_S *pstPalette);

#endif // MODULES_VPU_INCLUDE_RGN_IOCTL_H_
