#ifndef __VI_DEFINES_H__
#define __VI_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/cvi_defines.h>
#include <linux/vi_tun_cfg.h>
#include <linux/vi_isp.h>
#include <vip/vi_drv.h>

enum E_VI_TH {
	E_VI_TH_PRERAW,
	E_VI_TH_ERR_HANDLER,
	E_VI_TH_EVENT_HANDLER,
	E_VI_TH_VBLANK_HANDLER,
	E_VI_TH_MAX
};

struct vi_thread_attr {
	char th_name[32];
	struct task_struct *w_thread;
	atomic_t           thread_exit;
	wait_queue_head_t  wq;
	u32                flag;
	int (*th_handler)(void *arg);
};

/**
 * struct cvi_vi - VI IP abstraction
 */
struct cvi_vi_dev {
	struct device			*dev;
	struct class			*vi_class;
	struct cdev			cdev;
	dev_t				cdev_id;
	void __iomem			*reg_base;
	int				irq_num;
	struct clk			*clk_sys[4];
	struct clk			*clk_isp[4];
	struct clk			*clk_mac[3];
	void				*shared_mem;
	struct isp_ctx			ctx;
	struct cvi_isp_mbus_framefmt	usr_fmt;
	struct cvi_isp_rect		usr_crop;
	struct list_head		rdy_queue[ISP_PRERAW_VIRT_MAX];
	spinlock_t			rdy_lock;
	u8				num_rdy[ISP_PRERAW_VIRT_MAX];
	u8				chn_id;
	u64				usr_pic_phy_addr[3];
	unsigned long			usr_pic_delay;
	enum cvi_isp_source		isp_source;
	struct cvi_isp_snr_info		snr_info[ISP_PRERAW_VIRT_MAX];
	atomic_t			isp_raw_dump_en[ISP_PRERAW_VIRT_MAX];
	atomic_t			isp_smooth_raw_dump_en[ISP_PRERAW_VIRT_MAX];
	u32				isp_int_flag[ISP_PRERAW_VIRT_MAX];
	wait_queue_head_t		isp_int_wait_q[ISP_PRERAW_VIRT_MAX];
	wait_queue_head_t		isp_dq_wait_q;
	wait_queue_head_t		isp_event_wait_q;
	wait_queue_head_t		isp_dbg_wait_q;
	atomic_t			isp_dbg_flag;
	atomic_t			isp_err_handle_flag;
	enum cvi_isp_raw		offline_raw_num;
	struct tasklet_struct		job_work;
	struct list_head		qbuf_list[ISP_FE_CHN_MAX];
	spinlock_t			qbuf_lock;
	u8				qbuf_num[ISP_FE_CHN_MAX];
	u32				pre_fe_sof_cnt[ISP_PRERAW_VIRT_MAX][ISP_FE_CHN_MAX];
	u32				pre_fe_frm_num[ISP_PRERAW_VIRT_MAX][ISP_FE_CHN_MAX];
	u32				pre_be_frm_num[ISP_PRERAW_VIRT_MAX][ISP_BE_CHN_MAX];
	u32				pre_fe_trig_cnt[ISP_PRERAW_VIRT_MAX][ISP_FE_CHN_MAX];
	bool				is_yuv_trigger;
	bool				isp_triggered[ISP_PRERAW_VIRT_MAX][ISP_FE_CHN_MAX];
	bool				preraw_first_frm[ISP_PRERAW_VIRT_MAX];
	u32				isp_err_times[ISP_PRERAW_VIRT_MAX];
	u32				postraw_frame_number[ISP_PRERAW_VIRT_MAX];
	u32				drop_frame_number[ISP_PRERAW_VIRT_MAX];
	u32				dump_frame_number[ISP_PRERAW_VIRT_MAX];
	u8				postraw_proc_num;
	atomic_t			pre_fe_state[ISP_PRERAW_VIRT_MAX][ISP_FE_CHN_MAX];
	atomic_t			pre_be_state[ISP_BE_CHN_MAX];
	atomic_t			postraw_state;
	atomic_t			isp_streamoff;
	atomic_t			isp_streamon;
	atomic_t			ol_sc_frm_done;
	struct vi_thread_attr		vi_th[E_VI_TH_MAX];
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_DEFINES_H__ */
