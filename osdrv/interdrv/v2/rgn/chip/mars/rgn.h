#ifndef __RGN_H__
#define __RGN_H__

#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_region.h>
#include <linux/rgn_uapi.h>

#include <base_cb.h>
#include <base_ctx.h>
#include <rgn_common.h>
#include <vip/rgn_drv.h>
#include <rgn_defines.h>

static atomic_t	dev_open_cnt = ATOMIC_INIT(0);

u32 rgn_log_lv = RGN_WARN;
module_param(rgn_log_lv, int, 0644);

/*********************************************************************************************/
/* Configured from user, IOCTL */
CVI_S32 rgn_create(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);
CVI_S32 rgn_destory(RGN_HANDLE Handle);
CVI_S32 rgn_get_attr(RGN_HANDLE Handle, RGN_ATTR_S *pstRegion);
CVI_S32 rgn_set_attr(RGN_HANDLE Handle, const RGN_ATTR_S *pstRegion);
CVI_S32 rgn_set_bit_map(RGN_HANDLE Handle, const BITMAP_S *pstBitmap);
CVI_S32 rgn_attach_to_chn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
CVI_S32 rgn_detach_from_chn(RGN_HANDLE Handle, const MMF_CHN_S *pstChn);
CVI_S32 rgn_set_display_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);
CVI_S32 rgn_get_display_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_CHN_ATTR_S *pstChnAttr);
CVI_S32 rgn_get_canvas_info(RGN_HANDLE Handle, RGN_CANVAS_INFO_S *pstCanvasInfo);
CVI_S32 rgn_update_canvas(RGN_HANDLE Handle);
CVI_S32 rgn_invert_color(RGN_HANDLE Handle, MMF_CHN_S *pstChn, CVI_U32 *pu32Color);
CVI_S32 rgn_set_chn_palette(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, RGN_PALETTE_S *pstPalette,
			RGN_RGBQUARD_S *pstInputPixelTable);

/* INTERNAL */
int32_t _rgn_init(void);
int32_t _rgn_exit(void);
CVI_U32 _rgn_proc_get_idx(RGN_HANDLE hHandle);
bool is_rect_overlap(RECT_S *r0, RECT_S *r1);
CVI_BOOL _rgn_check_order(RECT_S *r0, RECT_S *r1);
int _rgn_insert(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl);
int _rgn_ex_insert(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl);
int _rgn_remove(RGN_HANDLE hdls[], CVI_U8 size, RGN_HANDLE hdl);
void _rgn_fill_pattern(void *buf, CVI_U32 len, CVI_U32 color, CVI_U8 bpp);
CVI_S32 _rgn_check_chn_attr(RGN_HANDLE Handle, const MMF_CHN_S *pstChn, const RGN_CHN_ATTR_S *pstChnAttr);

#endif /* __RGN_H__ */
