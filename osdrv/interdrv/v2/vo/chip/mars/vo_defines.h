#ifndef __VO_DEFINES_H__
#define __VO_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/vo_disp.h>
#include <linux/vo_uapi.h>

enum E_VO_TH {
	E_VO_TH_DISP,
	E_VO_TH_DISP_ISR_TEST,
	E_VO_TH_MAX
};

struct vo_thread_attr {
	char th_name[32];
	struct task_struct *w_thread;
	atomic_t           thread_exit;
	wait_queue_head_t  wq;
	u32                flag;
	int (*th_handler)(void *arg);
};

struct cvi_vo_dev {
	// private data
	struct device			*dev;
	struct class			*vo_class;
	struct cdev			cdev;
	dev_t				cdev_id;
	void __iomem			*reg_base[4];
	struct clk			*clk_disp;
	struct clk			*clk_bt;
	struct clk			*clk_dsi;
	struct clk			*clk_sc_top;
	int				irq_num;

	struct vo_dv_timings		dv_timings;
	struct vo_rect			sink_rect;
	struct vo_rect			compose_out;
	struct vo_rect			crop_rect;
	enum cvi_disp_intf		disp_interface;
	bool				bgcolor_enable;
	void				*shared_mem;
	u8				align;
	bool				disp_online;
	u32				vid_caps;
	u32				frame_number;
	u8				num_rdy;
	spinlock_t			rdy_lock;
	struct list_head		rdy_queue;
	u32				seq_count;
	u8				chn_id;
	u32				bytesperline[3];
	u32				sizeimage[3];
	u32				 colorspace;
	spinlock_t			qbuf_lock;
	struct list_head		qbuf_list[1];
	u8				qbuf_num[1];
	atomic_t			disp_streamon;
	struct vo_thread_attr		vo_th[E_VO_TH_MAX];
	u8				numOfPlanes;
};


#ifdef __cplusplus
}
#endif

#endif /* __VO_DEFINES_H__ */
