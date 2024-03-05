#ifndef __VO_INTERFACES_H__
#define __VO_INTERFACES_H__

#include "scaler.h"
#ifdef __cplusplus
	extern "C" {
#endif

#include <base_cb.h>

static const char *const clk_disp_name = "clk_disp";
static const char *const clk_bt_name = "clk_bt";
static const char *const clk_dsi_name = "clk_dsi";

/*******************************************************
 *  File operations for core
 ******************************************************/
long vo_ioctl(struct file *filp, u_int cmd, u_long arg);
int vo_open(struct inode *inode, struct file *filp);
int vo_release(struct inode *inode, struct file *filp);
int vo_mmap(struct file *filp, struct vm_area_struct *vm);
unsigned int vo_poll(struct file *filp, struct poll_table_struct *wait);

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vo_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg);
int _vo_call_cb(u32 m_id, u32 cmd_id, void *data);
void vo_irq_handler(struct cvi_vo_dev *vdev, union sclr_intr intr_status);
int vo_create_instance(struct platform_device *pdev);
int vo_destroy_instance(struct platform_device *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __VO_INTERFACES_H__ */
