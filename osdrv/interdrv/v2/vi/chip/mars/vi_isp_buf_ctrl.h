#ifndef __VI_ISP_BUF_CTRL_H__
#define __VI_ISP_BUF_CTRL_H__

#ifdef __cplusplus
	extern "C" {
#endif

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
#include <uapi/linux/sched/types.h>
#include <linux/vi_tun_cfg.h>
#include <linux/vi_isp.h>
#include <linux/vi_uapi.h>
#include <vi_common.h>
#include <vip/vi_drv.h>
#include <vi_defines.h>

struct isp_buffer {
	enum cvi_isp_raw  raw_num;
	enum cvi_isp_chn_num chn_num;
	uint64_t          addr;
	struct vi_rect    crop_le;
	struct vi_rect    crop_se;
	struct isp_grid_s_info rgbmap_i;
	struct isp_grid_s_info lmap_i;
	struct list_head  list;
	uint32_t          byr_size;
	uint32_t          frm_num;
	uint32_t          ir_idx;
	uint32_t          is_yuv_frm : 1;
};

struct isp_queue {
	struct list_head rdy_queue;
	uint32_t num_rdy;
	enum cvi_isp_raw raw_num;
};

extern spinlock_t buf_lock;

struct isp_buffer *isp_next_buf(struct isp_queue *q);
void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b);
struct isp_buffer *isp_buf_remove(struct isp_queue *q);

#ifdef __cplusplus
}
#endif

#endif //__VI_ISP_BUF_CTRL_H__

