#ifndef __VO_H__
#define __VO_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <linux/sys.h>
#include <linux/vo_uapi.h>
#include <linux/cvi_vo_ctx.h>
#include "scaler.h"
#include <vb.h>
#include <vo_sdk_layer.h>

#define VO_DV_BT_BLANKING_WIDTH(bt) \
	(bt->hfrontporch + bt->hsync + bt->hbackporch)
#define VO_DV_BT_FRAME_WIDTH(bt) \
	(bt->width + VO_DV_BT_BLANKING_WIDTH(bt))
#define VO_DV_BT_BLANKING_HEIGHT(bt) \
	(bt->vfrontporch + bt->vsync + bt->vbackporch + \
	 bt->il_vfrontporch + bt->il_vsync + bt->il_vbackporch)
#define VO_DV_BT_FRAME_HEIGHT(bt) \
	(bt->height + VO_DV_BT_BLANKING_HEIGHT(bt))

enum i80_op_type {
	I80_OP_GO = 0,
	I80_OP_TIMER,
	I80_OP_DONE,
	I80_OP_MAX,
};

enum i80_ctrl_type {
	I80_CTRL_CMD = 0,
	I80_CTRL_DATA,
	I80_CTRL_EOL = I80_CTRL_DATA,
	I80_CTRL_EOF,
	I80_CTRL_END = I80_CTRL_EOF,
	I80_CTRL_MAX
};

struct cvi_disp_buffer {
	struct vo_buffer buf;
	struct list_head       list;
	__u32			sequence;
};

struct vo_disp_pattern {
	enum sclr_disp_pat_type type;
	enum sclr_disp_pat_color color;
	u16 rgb[3];
};

struct vo_b {
	__u32			chnId;
	__u32			sequence;
	//struct timeval		timestamp;
	__u32			reserved;
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_H__ */
