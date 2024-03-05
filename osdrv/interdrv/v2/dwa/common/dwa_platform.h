#ifndef _DWA_PLATFORM_H_
#define _DWA_PLATFORM_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>

struct cvi_dwa_vdev {
	struct miscdevice miscdev;

	struct list_head ctx_list;
	u32 ctx_num;

	u32 vid_caps;
	struct mutex mutex;
	struct completion sem;
	struct completion sem_sbm;
	struct clk *clk_sys[5];
	struct clk *clk;
	void *shared_mem;

	unsigned int irq_num_dwa;

	struct task_struct *thread;
	struct kthread_worker worker;
	struct kthread_work work;
	struct list_head event_list;
	spinlock_t lock;
	struct list_head jobq;
	wait_queue_head_t cond_queue;
	bool job_done;
};

void dwa_irq_handler(u8 intr_status, struct cvi_dwa_vdev *wdev);

#endif /* _DWA_PLATFORM_H_ */
