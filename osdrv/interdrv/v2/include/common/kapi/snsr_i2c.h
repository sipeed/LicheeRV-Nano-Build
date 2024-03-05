#ifndef _CVI_SNSR_I2C_H_
#define _CVI_SNSR_I2C_H_

#include <linux/debugfs.h>
#include "linux/vi_snsr.h"
#include <linux/i2c.h>

#define I2C_MAX_NUM		5
#define I2C_MAX_MSG_NUM		32
#define I2C_BUF_SIZE		(I2C_MAX_MSG_NUM << 2)

int vip_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb);

struct cvi_i2c_ctx {
	struct i2c_client	*client;
	struct i2c_msg		msg[I2C_MAX_MSG_NUM];
	uint8_t			*buf;
	uint32_t		msg_idx;
	uint16_t		addr_bytes;
	uint16_t		data_bytes;
};

struct cvi_i2c_dev {
	spinlock_t		lock;
	struct mutex		mutex;
	struct cvi_i2c_ctx	ctx[I2C_MAX_NUM];
	struct dentry		*dbg_root;
};

#define CVI_SNS_I2C_IOC_MAGIC	'i'
#define CVI_SNS_I2C_WRITE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 2, \
					struct isp_i2c_data)
#define CVI_SNS_I2C_BURST_QUEUE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 3, \
					struct isp_i2c_data)
#define CVI_SNS_I2C_BURST_FIRE	_IOWR(CVI_SNS_I2C_IOC_MAGIC, 4, \
					unsigned int)
#endif
