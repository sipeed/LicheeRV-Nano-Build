#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>

#include <linux/cvi_comm_video.h>
#include <linux/dwa_uapi.h>

#include <base_cb.h>
#include <dwa_cb.h>
#include <vip_common.h>

#include "dwa_debug.h"
#include "dwa_platform.h"
#include "cvi_vip_dwa.h"
#include "cvi_vip_gdc_proc.h"
#include "gdc.h"
#include "ldc.h"
#include "ldc_test.h"
#include "mesh.h"

#define DWA_CLASS_NAME "cvi-dwa"
#define DWA_DEV_NAME "cvi-dwa"

#define GDC_SHARE_MEM_SIZE (0x8000)

u32 dwa_log_lv = CVI_DBG_DEBUG/*CVI_DBG_INFO*/;

static const char *const CLK_DWA_NAME = "clk_dwa";

module_param(dwa_log_lv, int, 0644);

static int dwa_open(struct inode *inode, struct file *filp)
{
	struct cvi_dwa_vdev *wdev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);

	if (!wdev) {
		pr_err("cannot find dwa private data\n");
		return -ENODEV;
	}

	return 0;
}

static int dwa_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct cvi_dwa_vdev *wdev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = wdev->shared_mem;

	if ((vm_size + offset) > GDC_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE,
				    vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("gdc proc mmap vir(%p) phys(%#llx)\n", pos,
			 (unsigned long long)virt_to_phys((void *)pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long dwa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_dwa_vdev *wdev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, drv_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(cmd);
	out_size = _IOC_SIZE(cmd);
	in_size = out_size;
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size)) {
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
#endif
		CVI_TRACE_DWA(CVI_DBG_ERR, "access_ok failed\n");
	}

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case CVI_DWA_BEGIN_JOB:
	{
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_BEGIN_JOB\n");
		ret = gdc_begin_job(wdev, data);
		break;
	}

	case CVI_DWA_END_JOB:
	{
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_END_JOB, handle=0x%llx\n",
			      (unsigned long long)data->handle);
		ret = gdc_end_job(wdev, data->handle);
		break;
	}

	case CVI_DWA_ADD_ROT_TASK:
	{
		struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_ROT_TASK, handle=0x%llx\n",
			      (unsigned long long)attr->handle);
		ret = gdc_add_rotation_task(wdev, attr);
		break;
	}

	case CVI_DWA_ADD_LDC_TASK:
	{
		struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_LDC_TASK, handle=0x%llx\n",
			      (unsigned long long)attr->handle);
		ret = gdc_add_ldc_task(wdev, attr);
		break;
	}

	case CVI_DWA_SET_BUF_WRAP:
	{
		struct dwa_buf_wrap_cfg *cfg = (struct dwa_buf_wrap_cfg *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVI_DWA_SET_BUF_WRAP, handle=0x%llx\n",
			      (unsigned long long)cfg->handle);

		ret = gdc_set_buf_wrap(wdev, cfg);
		break;
	}

	case CVI_DWA_GET_BUF_WRAP:
	{
		struct dwa_buf_wrap_cfg *cfg = (struct dwa_buf_wrap_cfg *)kdata;

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVI_DWA_GET_BUF_WRAP, handle=0x%llx\n",
			      (unsigned long long)cfg->handle);

		ret = gdc_get_buf_wrap(wdev, cfg);
		break;
	}

	default:
		ret = -ENOTTY;
		goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}

static int dwa_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations dwa_fops = {
	.owner = THIS_MODULE,
	.open = dwa_open,
	.release = dwa_release,
	.mmap = dwa_mmap,
	.unlocked_ioctl = dwa_ioctl,
};

static int _register_dev(struct cvi_dwa_vdev *wdev)
{
	int rc;

	wdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	wdev->miscdev.name = DWA_DEV_NAME;
	wdev->miscdev.fops = &dwa_fops;

	rc = misc_register(&wdev->miscdev);
	if (rc) {
		pr_err("cvi_dwa: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
int dwa_create_instance(struct platform_device *pdev)
{
	int i, rc = 0;
	struct cvi_dwa_vdev *wdev;
	static const char * const clk_sys_name[] = {"clk_sys_0", "clk_sys_1", "clk_sys_2", "clk_sys_3", "clk_sys_4"};

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("gdc cannot get drv data for dwa\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&wdev->ctx_list);
	wdev->ctx_num = 0;
	mutex_init(&wdev->mutex);

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		wdev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(wdev->clk_sys[i])) {
			pr_err("gdc cannot get clk for clk_sys_%d\n", i);
			wdev->clk_sys[i] = NULL;
		}
	}

	wdev->clk = devm_clk_get(&pdev->dev, CLK_DWA_NAME);
	if (IS_ERR(wdev->clk)) {
		pr_err("gdc cannot get clk for dwa\n");
		wdev->clk = NULL;
	}

	// clk_ldc_src_sel default 1(clk_src_vip_sys_2), 600 MHz
	// set 0(clk_src_vip_sys_4), 400MHz for ND
	vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL1, BIT(20), 0);

	//wdev->align = LDC_ADDR_ALIGN;

	_register_dev(wdev);

	wdev->shared_mem = kzalloc(GDC_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!wdev->shared_mem) {
		pr_err("gdc shared mem alloc fail\n");
		return -ENOMEM;
	}

	if (gdc_proc_init(wdev->shared_mem) < 0)
		pr_err("gdc proc init failed\n");

	if (cvi_gdc_init(wdev)) {
		pr_err("gdc init fail\n");
		goto err_work_init;
	}

	return 0;

err_work_init:
	return rc;
}

int dwa_destroy_instance(struct platform_device *pdev)
{
	struct cvi_dwa_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("invalid data\n");
		return -EINVAL;
	}

	misc_deregister(&wdev->miscdev);

	gdc_proc_remove();
	kfree(wdev->shared_mem);

	return 0;
}

void dwa_irq_handler(u8 intr_status, struct cvi_dwa_vdev *wdev)
{
	struct cvi_dwa_job *job = NULL;
	struct gdc_task *tsk;

	if (!list_empty(&wdev->jobq))
		job = list_entry(wdev->jobq.next, struct cvi_dwa_job, node);

	if (job) {
		if (!list_empty(&job->task_list)) {
			tsk = list_entry(job->task_list.next, struct gdc_task,
					 node);
			tsk->state = GDC_TASK_STATE_DONE;
		} else
			CVI_TRACE_DWA(CVI_DBG_DEBUG, "%s: error, empty tsk\n", __func__);

		gdc_proc_record_hw_end(job);

		complete(&wdev->sem);
	} else
		CVI_TRACE_DWA(CVI_DBG_DEBUG, "%s: error, empty job\n", __func__);
}

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0;
	int irq_num;
	static const char *const irq_name = "dwa";
	struct resource *res = NULL;
	void *reg_base;
	struct cvi_dwa_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata\n");
		return -EINVAL;
	}

	/* [scaler, dwa, vip, isp, dphy] */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		dev_err(&pdev->dev, "invalid resource\n");
		return -EINVAL;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	reg_base = devm_ioremap(&pdev->dev, res->start,
					res->end - res->start);
#else
	reg_base = devm_ioremap_nocache(&pdev->dev, res->start,
					res->end - res->start);
#endif
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "  (%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
	       1, res->start, res->end, reg_base);

	ldc_set_base_addr(reg_base);

	/* Interrupt */
	irq_num = platform_get_irq_byname(pdev, irq_name);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", irq_name);
		return -ENODEV;
	}
	CVI_TRACE_DWA(CVI_DBG_DEBUG, "irq(%d) for %s get from platform driver.\n", irq_num,
		irq_name);

	wdev->irq_num_dwa = irq_num;

	return rc;
}

static MOD_ID_E convert_mod_id(enum ENUM_MODULES_ID cbModId)
{
	if (cbModId == E_MODULE_VI)
		return CVI_ID_VI;
	else if (cbModId == E_MODULE_VO)
		return CVI_ID_VO;
	else if (cbModId == E_MODULE_VPSS)
		return CVI_ID_VPSS;

	return CVI_ID_BUTT;
}

int dwa_cmd_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_dwa_vdev *wdev = (struct cvi_dwa_vdev *)dev;
	int rc = -1;

	switch (cmd) {
	case DWA_CB_MESH_GDC_OP:
	{
		struct mesh_gdc_cfg *cfg =
			(struct mesh_gdc_cfg *)arg;
		MOD_ID_E enModId = convert_mod_id(caller);

		if (enModId == CVI_ID_BUTT)
			return CVI_FAILURE;

		rc = mesh_gdc_do_op(wdev, cfg->usage, cfg->pUsageParam,
				    cfg->vb_in, cfg->enPixFormat, cfg->mesh_addr,
				    cfg->sync_io, cfg->pcbParam,
				    cfg->cbParamSize, enModId, cfg->enRotation);
	}
	break;

	case DWA_CB_VPSS_SBM_DONE:
	{
		rc = dwa_vpss_sdm_cb_done(wdev);
		break;
	}

	default:
	break;
	}

	return rc;
}

static int dwa_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_DWA);
}

static int dwa_register_cb(struct cvi_dwa_vdev *wdev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_DWA;
	reg_cb.dev		= (void *)wdev;
	reg_cb.cb		= dwa_cmd_cb;

	return base_reg_module_cb(&reg_cb);
}

static int cvi_dwa_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_dwa_vdev *wdev;

	/* allocate main structure */
	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	/* initialize locks */
	//spin_lock_init(&dev->lock);
	//mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, wdev);

	// get hw-resources
	rc = _init_resources(pdev);
	if (rc)
		goto err_irq;

	// for dwa
	rc = dwa_create_instance(pdev);
	if (rc) {
		pr_err("Failed to create dwa instance\n");
		goto err_dwa_reg;
	}

	ldc_intr_ctrl(0x01);

	if (devm_request_irq(&pdev->dev, wdev->irq_num_dwa, dwa_isr,
			     IRQF_SHARED, "CVI_VIP_DWA", wdev)) {
		dev_err(&pdev->dev, "Unable to request dwa IRQ(%d)\n",
			wdev->irq_num_dwa);
		return -EINVAL;
	}

	/* ldc self test */
	ldc_test_proc_init();

	/* dwa register cb */
	if (dwa_register_cb(wdev)) {
		dev_err(&pdev->dev, "Failed to register dwa cb, err %d\n", rc);
		return -EINVAL;
	}

	CVI_TRACE_DWA(CVI_DBG_INFO, "done with rc(%d).\n", rc);

	return rc;

err_dwa_reg:
err_irq:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * cvi_ldc_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int cvi_dwa_remove(struct platform_device *pdev)
{
	struct cvi_dwa_vdev *wdev;

	/* ldc self test */
	ldc_test_proc_deinit();

	dwa_destroy_instance(pdev);

	/* dwa rm cb */
	if (dwa_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm dwa cb\n");
	}

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata");
		return 0;
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id cvi_dwa_dt_match[] = {
	{ .compatible = "cvitek,dwa" },
	{}
};

#ifdef CONFIG_PM_SLEEP
static int dwa_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int dwa_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	//VIP_CLK_RATIO_CONFIG(LDC, 0x10);

	return 0;
}

static SIMPLE_DEV_PM_OPS(dwa_pm_ops, dwa_suspend, dwa_resume);
#else
static SIMPLE_DEV_PM_OPS(dwa_pm_ops, NULL, NULL);
#endif

static struct platform_driver cvi_dwa_driver = {
	.probe      = cvi_dwa_probe,
	.remove     = cvi_dwa_remove,
	.driver     = {
		.name		= "cvi-dwa",
		.owner		= THIS_MODULE,
		.of_match_table	= cvi_dwa_dt_match,
		.pm		= &dwa_pm_ops,
	},
};

module_platform_driver(cvi_dwa_driver);

MODULE_DESCRIPTION("Cvitek dwa driver");
MODULE_LICENSE("GPL");
