#include <vo_core.h>
#include <base_cb.h>

#include "scaler.h"

#define CVI_VO_IRQ_NAME            "sc"
#define CVI_VO_DEV_NAME            "cvi-vo"
#define CVI_VO_CLASS_NAME          "cvi-vo"

static long vo_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	long ret = 0;

	ret = vo_ioctl(filp, cmd, arg);
	return ret;
}

static int vo_core_open(struct inode *inode, struct file *filp)
{
	return vo_open(inode, filp);
}

static int vo_core_release(struct inode *inode, struct file *filp)
{
	return vo_release(inode, filp);
}

static int vo_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return vo_mmap(filp, vm);
}

static unsigned int vo_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return vo_poll(filp, wait);
}

const struct file_operations vo_fops = {
	.owner = THIS_MODULE,
	.open = vo_core_open,
	.unlocked_ioctl = vo_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vo_core_ioctl,
#endif
	.release = vo_core_release,
	.mmap = vo_core_mmap,
	.poll = vo_core_poll,
};

int vo_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	return vo_cb(dev, caller, cmd, arg);
}

static int vo_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VO);
}

static int vo_core_register_cb(struct cvi_vo_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VO;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vo_core_cb;

	return base_reg_module_cb(&reg_cb);
}

#if 0
static irqreturn_t vo_core_isr(int irq, void *priv)
{
	return IRQ_HANDLED;
}
#endif
static int vo_core_register_cdev(struct cvi_vo_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vo_class = class_create(THIS_MODULE, CVI_VO_CLASS_NAME);
	if (IS_ERR(dev->vo_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vo_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, CVI_VO_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "allocate chrdev failed\n");
		return err;
	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &vo_fops);

	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "add chrdev failed\n");
		return err;
	}

	dev_t = device_create(dev->vo_class, dev->dev, dev->cdev_id, NULL, "%s", CVI_VO_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		return err;
	}

	return err;
}

static int vo_core_clk_init(struct platform_device *pdev)
{
	struct cvi_vo_dev *dev;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vo drvdata\n");
		return -EINVAL;
	}
	dev->clk_disp = devm_clk_get(&pdev->dev, clk_disp_name);
	if (IS_ERR(dev->clk_disp)) {
		pr_err("Cannot get clk for disp\n");
		dev->clk_disp = NULL;
	}
	dev->clk_bt = devm_clk_get(&pdev->dev, clk_bt_name);
	if (IS_ERR(dev->clk_bt)) {
		pr_err("Cannot get clk for bt\n");
		dev->clk_bt = NULL;
	}
	dev->clk_dsi = devm_clk_get(&pdev->dev, clk_dsi_name);
	if (IS_ERR(dev->clk_dsi)) {
		pr_err("Cannot get clk for dsi\n");
		dev->clk_dsi = NULL;
	}
	return 0;
}

#if 0//callback by vpss
static irqreturn_t vo_core_isr(int irq, void *priv)
{
	struct cvi_vo_dev *vdev = priv;

	vo_irq_handler(vdev);

	return IRQ_HANDLED;
}
#endif
static int vo_core_probe(struct platform_device *pdev)
{
		struct cvi_vo_dev *dev;
		//struct resource *res; for set reg_bas
		int ret = 0;
		bool status = true;
		//int i = 0; for set reg_base

		dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
		if (!dev)
			return -ENOMEM;

		dev->dev = &pdev->dev;
		dev_set_drvdata(&pdev->dev, dev);
#if 0
		for (i = 0; i < 3; ++i) {
			/* IP register base address */
			res = platform_get_resource(pdev, IORESOURCE_MEM, i);
			dev->reg_base[i] = devm_ioremap_resource(&pdev->dev, res);
			vo_pr(VO_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
					res->start, res->end, dev->reg_base[i]);

			if (IS_ERR(dev->reg_base)) {
				ret = PTR_ERR(dev->reg_base);
				return ret;
			}
		}
#endif
#if 0 // get sc interrupt by vpss cb
		/* Interrupt */
		dev->irq_num = platform_get_irq_byname(pdev, CVI_VO_IRQ_NAME);
		if (dev->irq_num < 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n", CVI_VO_IRQ_NAME);
			return -ENODEV;
		}
		vo_pr(VO_INFO, "irq(%d) for %s get from platform driver.\n",
				dev->irq_num, CVI_VO_IRQ_NAME);
#endif

		ret = vo_core_clk_init(pdev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to init clk, err %d\n", ret);
			goto err_clk_init;
		}

		ret = vo_core_register_cdev(dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to register dev, err %d\n", ret);
			goto err_dev_register;
		}

		ret = vo_create_instance(pdev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
			goto err_create_instance;
		}
#if 0//implement by vpss  isr , need to provide callback to vpss
		ret = devm_request_irq(&pdev->dev, dev->irq_num, vo_core_isr, 0,
					pdev->name, dev);
		if (ret) {
			dev_err(&pdev->dev, "Failed to request irq_num(%d) ret(%d)\n",
					dev->irq_num, ret);
			ret = -EINVAL;
			goto err_req_irq;
		}
#endif

	ret = vo_core_register_cb(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register vi cb, err %d\n", ret);
		goto err_create_instance;
	}

	_vo_call_cb(E_MODULE_VPSS, VPSS_CB_VO_STATUS_SET, &status);

err_create_instance:

err_dev_register:

err_clk_init:

	return ret;
}

static int vo_core_remove(struct platform_device *pdev)
{
	int ret = 0;
	bool status = false;

	struct cvi_vo_dev *dev = dev_get_drvdata(&pdev->dev);

	_vo_call_cb(E_MODULE_VPSS, VPSS_CB_VO_STATUS_SET, &status);

	ret = vo_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vo_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vi cb, err %d\n", ret);
	}

	device_destroy(dev->vo_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vo_class);

	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	vo_pr(VO_INFO, "%s -\n", __func__);

	return ret;
}


static const struct of_device_id vo_core_match[] = {
	{
		.compatible = "cvitek,vo",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, vo_core_match);

static struct platform_driver vo_core_driver = {
	.probe = vo_core_probe,
	.remove = vo_core_remove,
	.driver = {
		.name = CVI_VO_DEV_NAME,
		.of_match_table = vo_core_match,
	},
};

module_platform_driver(vo_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video input driver");
MODULE_LICENSE("GPL");
