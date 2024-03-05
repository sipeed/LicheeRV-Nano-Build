#ifndef _CVI_VIP_DWA_H_
#define _CVI_VIP_DWA_H_

#include <linux/cvi_vip_dwa.h>
#include <linux/irqreturn.h>

#define VIP_MAX_PLANES 3

struct cvi_dwa_data {
	__u32 bytesperline[VIP_MAX_PLANES];
	__u32 sizeimage[VIP_MAX_PLANES];
	__u16 w;
	__u16 h;
};

struct cvi_dwa_job {
	enum cvi_dwa_op op;
	struct cvi_dwa_data cap_data, out_data;

	__u64 mesh_id_addr;
	__u32 bgcolor;

	struct list_head node;
	struct list_head task_list;
	bool sync_io;
	MOD_ID_E enModId;

	u64 hw_start_time;
};

struct cvi_dwa_ctx {
	struct list_head list;
	struct cvi_dwa_vdev *wdev;
	const struct cvi_vip_fmt *fmt;
	u32 colorspace;
	struct cvi_dwa_data cap_data, out_data;

	enum cvi_dwa_op op;
	u64 mesh_id_addr;
	u32 bgcolor;
};

irqreturn_t dwa_isr(int irq, void *_dev);

#endif
