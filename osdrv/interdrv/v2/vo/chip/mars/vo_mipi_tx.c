#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>

#include "linux/vo_mipi_tx.h"
#include "proc/vo_mipi_tx_proc.h"
#include <vip_common.h>
#include "scaler.h"
#include "dsi_phy.h"


//#include "pinctrl-cv1822.h"

static DEFINE_MUTEX(reboot_lock);
struct cvi_vip_mipi_tx_dev *reboot_info;
int dump_reg = 1;

/*
 * macro definition
 */
#define MIPI_TX_DEV_NAME "mipi-tx"
#define MIPI_TX_PROC_NAME "vo_mipi_tx"

#define CVI_VIP_MIPI_TX_INFO(fmt, arg...)								\
		pr_debug("%d:%s(): " fmt, __LINE__, __func__, ## arg)

#define CVI_VIP_MIPI_TX_ERR(fmt, arg...)								\
		pr_err("%d:%s(): " fmt, __LINE__, __func__, ## arg)

struct cvi_vip_mipi_tx_dev {
	struct device *dev;
	struct miscdevice miscdev;
	struct combo_dev_cfg_s dev_cfg;
	struct clk *clk_disp, *clk_dsi;
	int reset_pin, reset_pin_active;
	int pwm_pin, pwm_pin_active;
	int power_ct_pin, power_ct_pin_active;
	int irq_vbat;
	void *vbat_addr[2];
	u8 lane_num;
	u8 bits;
} mipi_tx_dev_ctx;

static const char *const CLK_DISP_NAME = "clk_disp";
static const char *const CLK_DSI_NAME = "clk_dsi";

static int gpio[6] = {-1, -1, -1, -1, -1, -1};
static int n_gpio_para;
/* gpio: for mipi_tx gpio definitions
 * param0: reset pin
 * param1: reset pin active
 * param2: power ctrl pin
 * param3: power ctrl pin active
 * param4: pwm pin
 * param5: pwm pin active
 */
module_param_array(gpio, int, &n_gpio_para, 0664);

static int smooth;

static int debug = 1;

/* debug: debug
 * bit[0]: dcs cmd mode. 0(hw)/1(sw)
 */
module_param(debug, int, 0644);

/*
 * global variables definition
 */

/*
 * function definition
 */
static struct cvi_vip_mipi_tx_dev *file_mipi_tx_dev(struct file *file)
{
	return container_of(file->private_data, struct cvi_vip_mipi_tx_dev, miscdev);
}

static int mipi_tx_check_comb_dev_cfg(struct combo_dev_cfg_s *dev_cfg)
{
	if (dev_cfg->output_mode != OUTPUT_MODE_DSI_VIDEO) {
		CVI_VIP_MIPI_TX_ERR("output_mode(%d) not supported!\n", dev_cfg->output_mode);
		return -EINVAL;
	}

	if (dev_cfg->video_mode != BURST_MODE) {
		CVI_VIP_MIPI_TX_ERR("video_mode(%d) not supported!\n", dev_cfg->video_mode);
		return -EINVAL;
	}

	return 0;
}

static void _fill_disp_timing(struct sclr_disp_timing *timing, struct sync_info_s *sync_info)
{
	timing->vtotal = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines
			+ sync_info->vid_active_lines + sync_info->vid_vfp_lines - 1;
	timing->htotal = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels
			+ sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels - 1;
	timing->vsync_start = 1;
	timing->vsync_end = timing->vsync_start + sync_info->vid_vsa_lines - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + sync_info->vid_vsa_lines + sync_info->vid_vbp_lines;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + sync_info->vid_active_lines - 1;
	timing->hsync_start = 1;
	timing->hsync_end = timing->hsync_start + sync_info->vid_hsa_pixels - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + sync_info->vid_hline_pixels - 1;
	timing->vsync_pol = sync_info->vid_vsa_pos_polarity;
	timing->hsync_pol = sync_info->vid_hsa_pos_polarity;
}

static void _get_sync_info(struct sclr_disp_timing timing, struct sync_info_s *sync_info)
{
	sync_info->vid_hsa_pixels = timing.hsync_end - timing.hsync_start + 1;
	sync_info->vid_hbp_pixels = timing.hfde_start - timing.hsync_start - sync_info->vid_hsa_pixels;
	sync_info->vid_hline_pixels = timing.hfde_end - timing.hfde_start + 1;
	sync_info->vid_hfp_pixels = timing.htotal - sync_info->vid_hsa_pixels
				    - sync_info->vid_hbp_pixels - sync_info->vid_hline_pixels + 1;
	sync_info->vid_vsa_lines = timing.vsync_end - timing.vsync_start + 1;
	sync_info->vid_vbp_lines = timing.vfde_start - timing.vsync_start - sync_info->vid_vsa_lines;
	sync_info->vid_active_lines = timing.vfde_end - timing.vfde_start + 1;
	sync_info->vid_vfp_lines = timing.vtotal - sync_info->vid_vsa_lines
				   - sync_info->vid_vbp_lines - sync_info->vid_active_lines + 1;
	sync_info->vid_vsa_pos_polarity = timing.vsync_pol;
	sync_info->vid_hsa_pos_polarity = timing.hsync_pol;
}

static int mipi_tx_set_combo_dev_cfg(struct cvi_vip_mipi_tx_dev *tdev, struct combo_dev_cfg_s *dev_cfg)
{
	int ret, i;
	bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};

	u8 lane_num = 0;
	struct sclr_disp_timing timing;
	enum sclr_dsi_fmt dsi_fmt;
	u8 bits;
	bool preamble_on = false;

	ret = mipi_tx_check_comb_dev_cfg(dev_cfg);
	if (ret < 0) {
		CVI_VIP_MIPI_TX_ERR("mipi_tx check combo_dev config failed!\n");
		return -EINVAL;
	}

	dphy_dsi_disable_lanes();
	memcpy(&mipi_tx_dev_ctx.dev_cfg, dev_cfg, sizeof(mipi_tx_dev_ctx.dev_cfg));
	for (i = 0; i < LANE_MAX_NUM; i++) {
		if ((dev_cfg->lane_id[i] < 0) || (dev_cfg->lane_id[i] >= MIPI_TX_LANE_MAX)) {
			dphy_dsi_set_lane(i, DSI_LANE_MAX, false, true);
			continue;
		}
		dphy_dsi_set_lane(i, (enum lane_id)dev_cfg->lane_id[i], dev_cfg->lane_pn_swap[i], true);
		if (dev_cfg->lane_id[i] != MIPI_TX_LANE_CLK) {
			++lane_num;
			data_en[dev_cfg->lane_id[i] - 1] = true;
		}
	}
	if (lane_num == 0) {
		CVI_VIP_MIPI_TX_ERR("no active mipi-dsi lane\n");
		return -EINVAL;
	}

	_fill_disp_timing(&timing, &dev_cfg->sync_info);
	switch (dev_cfg->output_format) {
	case OUT_FORMAT_RGB_16_BIT:
		bits = 16;
		dsi_fmt = SCLR_DSI_FMT_RGB565;
	break;

	case OUT_FORMAT_RGB_18_BIT:
		bits = 18;
		dsi_fmt = SCLR_DSI_FMT_RGB666;
	break;

	case OUT_FORMAT_RGB_24_BIT:
		bits = 24;
		dsi_fmt = SCLR_DSI_FMT_RGB888;
	break;

	case OUT_FORMAT_RGB_30_BIT:
		bits = 30;
		dsi_fmt = SCLR_DSI_FMT_RGB101010;
	break;

	default:
	return -EINVAL;
	}

	preamble_on = (dev_cfg->pixel_clk * bits / lane_num) > 1500000;
	mipi_tx_dev_ctx.lane_num = lane_num;
	mipi_tx_dev_ctx.bits = bits;
	dphy_dsi_lane_en(true, data_en, preamble_on);
	dphy_dsi_set_pll(dev_cfg->pixel_clk, lane_num, bits);
	sclr_disp_set_intf(SCLR_VO_INTF_MIPI);
	sclr_dsi_config(lane_num, dsi_fmt, dev_cfg->sync_info.vid_hline_pixels);
	sclr_disp_set_timing(&timing);
	sclr_disp_tgen_enable(true);

	// resx operate after LP-11.
	if (gpio_is_valid(tdev->reset_pin)) {
		gpio_set_value(tdev->reset_pin, !tdev->reset_pin_active);
		usleep_range(5 * 1000, 10 * 1000);
		gpio_set_value(tdev->reset_pin, tdev->reset_pin_active);
		usleep_range(5 * 1000, 10 * 1000);
		gpio_set_value(tdev->reset_pin, !tdev->reset_pin_active);
		msleep(100);
	}

	pr_debug("lane_num(%d) preamble_on(%d) dsi_fmt(%d) bits(%d)\n", lane_num, preamble_on, dsi_fmt, bits);

	return ret;
}

int mipi_tx_get_combo_dev_cfg(struct combo_dev_cfg_s *dev_cfg)
{
	int ret = 0;
	char data_lan_num = 0;
	struct sclr_disp_timing timing;

	if (dev_cfg == NULL) {
		CVI_VIP_MIPI_TX_ERR("ptr dev_cfg is NULL!\n");
		return -EINVAL;
	}

	data_lan_num = dphy_dsi_get_lane((enum lane_id *)dev_cfg->lane_id);
	dev_cfg->video_mode = BURST_MODE;
	dev_cfg->output_mode = OUTPUT_MODE_DSI_VIDEO;
	dev_cfg->output_format = OUT_FORMAT_RGB_24_BIT;

	sclr_disp_get_hw_timing(&timing);
	_get_sync_info(timing, &dev_cfg->sync_info);
	dphy_dsi_get_pixclk(&dev_cfg->pixel_clk, data_lan_num, 24);

	return ret;
}

static int mipi_tx_set_cmd(struct cmd_info_s *cmd_info)
{
	int i;
	char str[160];

	if (cmd_info->cmd_size > CMD_MAX_NUM) {
		CVI_VIP_MIPI_TX_ERR("cmd_size(%d) can't exceed %d!\n", cmd_info->cmd_size, CMD_MAX_NUM);
		return -EINVAL;
	} else if ((cmd_info->cmd_size != 0) && (cmd_info->cmd == NULL)) {
		CVI_VIP_MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", cmd_info->cmd_size);
		return -EINVAL;
	}

	snprintf(str, 160, "%s: ", __func__);
	for (i = 0; i < cmd_info->cmd_size && i < 16; ++i)
		snprintf(str + strlen(str), 160 - strlen(str), "%#x ", cmd_info->cmd[i]);
	pr_debug("%s\n", str);

	return sclr_dsi_dcs_write_buffer(cmd_info->data_type, cmd_info->cmd, cmd_info->cmd_size, debug & 0x01);
}

static int mipi_tx_get_cmd(struct get_cmd_info_s *get_cmd_info)
{
	int ret = 0;

	if (get_cmd_info->get_data_size > RX_MAX_NUM) {
		CVI_VIP_MIPI_TX_ERR("get_data_size(%d) can't exceed %d!\n", get_cmd_info->get_data_size, RX_MAX_NUM);
		return -EINVAL;
	} else if ((get_cmd_info->get_data_size != 0) && (get_cmd_info->get_data == NULL)) {
		CVI_VIP_MIPI_TX_ERR("cmd is NULL, but cmd_size(%d) isn't zero!\n", get_cmd_info->get_data_size);
		return -EINVAL;
	}
	//soc may miss BTA signal send by panel, so increase clk before read, and set to normal after
	dphy_dsi_set_pll(mipi_tx_dev_ctx.dev_cfg.pixel_clk * 2, mipi_tx_dev_ctx.lane_num, mipi_tx_dev_ctx.bits);
	ret = sclr_dsi_dcs_read_buffer(get_cmd_info->data_type, get_cmd_info->data_param
		, get_cmd_info->get_data, get_cmd_info->get_data_size);
	dphy_dsi_set_pll(mipi_tx_dev_ctx.dev_cfg.pixel_clk, mipi_tx_dev_ctx.lane_num, mipi_tx_dev_ctx.bits);

	return ret;
}

static void mipi_tx_enable(void)
{
	sclr_dsi_set_mode(SCLR_DSI_MODE_HS);
	// if tgen disabled, colorbar cannot be showed after sample_dsi
}

static void mipi_tx_disable(void)
{
	int ret = 0;
	int count = 0;

	sclr_dsi_set_mode(SCLR_DSI_MODE_IDLE);
	do {
		usleep_range(1000, 2000);
		ret = sclr_dsi_chk_mode_done(SCLR_DSI_MODE_IDLE);
	} while ((ret != 0) && (count++ < 20));
}

static long mipi_tx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct cvi_vip_mipi_tx_dev *tdev = file_mipi_tx_dev(file);
	int rc = 0;

	switch (cmd) {
	case CVI_VIP_MIPI_TX_SET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&stcombo_dev_cfg, (void *)arg, sizeof(stcombo_dev_cfg))) {
			CVI_VIP_MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = mipi_tx_set_combo_dev_cfg(tdev, &stcombo_dev_cfg);
		if (rc < 0)
			CVI_VIP_MIPI_TX_ERR("mipi_tx set combo_dev config failed!\n");
		else
			tdev->dev_cfg = stcombo_dev_cfg;
	}
	break;

	case CVI_VIP_MIPI_TX_GET_DEV_CFG: {
		struct combo_dev_cfg_s stcombo_dev_cfg;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}

		memset(&stcombo_dev_cfg, 0, sizeof(stcombo_dev_cfg));
		rc = mipi_tx_get_combo_dev_cfg(&stcombo_dev_cfg);
		if (rc < 0)
			CVI_VIP_MIPI_TX_ERR("mipi_tx get combo_dev config failed!\n");

		if (copy_to_user((void *)arg, &stcombo_dev_cfg, sizeof(stcombo_dev_cfg))) {
			CVI_VIP_MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
			break;
		}
	}
	break;

	case CVI_VIP_MIPI_TX_SET_CMD: {
		struct cmd_info_s cmd_info;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&cmd_info, (void *)arg, sizeof(cmd_info))) {
			CVI_VIP_MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		if (cmd_info.cmd_size == 0) {
			CVI_VIP_MIPI_TX_ERR("cmd_size zero.\n");
			rc = -EINVAL;
			break;
		}

		// if cmd is NULL, use cmd_size as cmd.
		if (cmd_info.cmd == NULL) {
			cmd_info.cmd = kmalloc(2, GFP_KERNEL);
			if (cmd_info.cmd == NULL) {
				CVI_VIP_MIPI_TX_ERR("kmalloc failed.\n");
				rc = -ENOMEM;
				break;
			}
			cmd_info.cmd[0] = cmd_info.cmd_size & 0xff;
			cmd_info.cmd[1] = (cmd_info.cmd_size >> 8) & 0xff;
			cmd_info.cmd_size = (cmd_info.data_type == 0x05) ? 1 : 2;
		} else {
			unsigned char *tmp_cmd = kmalloc(cmd_info.cmd_size, GFP_KERNEL);

			if (tmp_cmd == NULL) {
				CVI_VIP_MIPI_TX_ERR("kmalloc failed.\n");
				rc = -ENOMEM;
				break;
			}

			if (copy_from_user(tmp_cmd, (void __user *)cmd_info.cmd, cmd_info.cmd_size)) {
				CVI_VIP_MIPI_TX_ERR("cmd copy_from_user failed.\n");
				kfree(cmd_info.cmd);
				rc = -ENOMEM;
				break;
			}
			cmd_info.cmd = tmp_cmd;
		}

		rc = mipi_tx_set_cmd(&cmd_info);
		if (rc < 0)
			CVI_VIP_MIPI_TX_ERR("mipi_tx set cmd failed!\n");
		kfree(cmd_info.cmd);
	}
	break;

	case CVI_VIP_MIPI_TX_GET_CMD: {
		struct get_cmd_info_s get_cmd_info, get_cmd_info_tmp;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&get_cmd_info, (void *)arg, sizeof(get_cmd_info))) {
			CVI_VIP_MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		memcpy(&get_cmd_info_tmp, &get_cmd_info, sizeof(get_cmd_info));
		// get_cmd_info.get_data is ptr from user space
		// it cannot be used directly in driver in riscv
		get_cmd_info.get_data = kmalloc(get_cmd_info.get_data_size, GFP_KERNEL);
		if (get_cmd_info.get_data == NULL) {
			CVI_VIP_MIPI_TX_ERR("kmalloc failed.\n");
			rc = -ENOMEM;
			break;
		}

		rc = mipi_tx_get_cmd(&get_cmd_info);
		if (rc < 0) {
			kfree(get_cmd_info.get_data);
			CVI_VIP_MIPI_TX_ERR("mipi_tx get cmd failed!\n");
			break;
		}

		if (copy_to_user((void *)get_cmd_info_tmp.get_data, get_cmd_info.get_data,
			get_cmd_info.get_data_size)) {
			CVI_VIP_MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
		}

		kfree(get_cmd_info.get_data);
	}
	break;

	case CVI_VIP_MIPI_TX_ENABLE:
		mipi_tx_enable();
	break;

	case CVI_VIP_MIPI_TX_DISABLE:
		mipi_tx_disable();
	break;

	case CVI_VIP_MIPI_TX_SET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		if (copy_from_user(&settle_cfg, (void *)arg, sizeof(settle_cfg))) {
			CVI_VIP_MIPI_TX_ERR("copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}
		CVI_VIP_MIPI_TX_INFO("Set hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		dphy_set_hs_settle(settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
	}
	break;

	case CVI_VIP_MIPI_TX_GET_HS_SETTLE: {
		struct hs_settle_s settle_cfg;

		if (arg == 0) {
			CVI_VIP_MIPI_TX_ERR("NULL pointer.\n");
			rc = -EINVAL;
			break;
		}
		dphy_get_hs_settle(&settle_cfg.prepare, &settle_cfg.zero, &settle_cfg.trail);
		CVI_VIP_MIPI_TX_INFO("Get hs settle: prepare(%d) zero(%d) trail(%d)\n",
				     settle_cfg.prepare, settle_cfg.zero, settle_cfg.trail);
		if (copy_to_user((void *)arg, &settle_cfg, sizeof(settle_cfg))) {
			CVI_VIP_MIPI_TX_ERR("copy_to_user failed.\n");
			rc = -ENOMEM;
			break;
		}
	}
	break;

	default: {
		CVI_VIP_MIPI_TX_ERR("invalid mipi_tx ioctl cmd\n");
		rc = -EINVAL;
	}
	break;
	}

	return rc;
}

#ifdef CONFIG_COMPAT
static long mipi_tx_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int mipi_tx_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int mipi_tx_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations mipi_tx_fops = {
	.owner = THIS_MODULE,
	.open = mipi_tx_open,
	.release = mipi_tx_release,
	.unlocked_ioctl = mipi_tx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mipi_tx_compat_ioctl,
#endif
};

static int _device_register(struct cvi_vip_mipi_tx_dev *tdev)
{
	int rc;

	tdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	tdev->miscdev.name = MIPI_TX_DEV_NAME;
	tdev->miscdev.fops = &mipi_tx_fops;

	rc = misc_register(&tdev->miscdev);
	if (rc) {
		dev_err(tdev->dev, "mipi_tx: failed to register misc device.\n");
		return rc;
	}

	return rc;
}

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_mipi_tx_dev *tdev;
	enum of_gpio_flags flags;

	tdev = dev_get_drvdata(&pdev->dev);
	if (!tdev) {
		dev_err(&pdev->dev, "Can not get cvi_mipi_tx drvdata");
		return -ENODEV;
	}

	tdev->irq_vbat = platform_get_irq(pdev, 0);

	// clk
	tdev->clk_disp = devm_clk_get(&pdev->dev, CLK_DISP_NAME);
	if (IS_ERR(tdev->clk_disp)) {
		pr_err("Cannot get clk for clk_disp\n");
		tdev->clk_disp = NULL;
	}
	if (tdev->clk_disp)
		clk_prepare_enable(tdev->clk_disp);
	tdev->clk_dsi = devm_clk_get(&pdev->dev, CLK_DSI_NAME);
	if (IS_ERR(tdev->clk_dsi)) {
		pr_err("Cannot get clk for clk_dsi\n");
		tdev->clk_dsi = NULL;
	}
	if (tdev->clk_dsi)
		clk_prepare_enable(tdev->clk_dsi);

	if (n_gpio_para) {
		tdev->reset_pin = gpio[0];
		tdev->reset_pin_active = gpio[1];
		if (n_gpio_para > 2) {
			tdev->power_ct_pin = gpio[2];
			tdev->power_ct_pin_active = gpio[3];
		} else {
			tdev->power_ct_pin = -EINVAL;
			tdev->power_ct_pin_active = -EINVAL;
		}
		if (n_gpio_para > 4) {
			tdev->pwm_pin = gpio[4];
			tdev->pwm_pin_active = gpio[5];
		} else {
			tdev->pwm_pin = -EINVAL;
			tdev->pwm_pin_active = -EINVAL;
		}
	} else {
		// reset pin
		tdev->reset_pin = of_get_named_gpio_flags(pdev->dev.of_node,
					 "reset-gpio", 0, &flags);
		tdev->reset_pin_active = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		// pwm pin
		tdev->pwm_pin = of_get_named_gpio_flags(pdev->dev.of_node,
					 "pwm-gpio", 0, &flags);
		tdev->pwm_pin_active = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		// power ctrl pin
		tdev->power_ct_pin = of_get_named_gpio_flags(pdev->dev.of_node,
					 "power-ct-gpio", 0, &flags);
		tdev->power_ct_pin_active = (flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	}

	smooth = sclr_disp_check_tgen_enable();
	if (!smooth) {
		if (gpio_is_valid(tdev->reset_pin)) {
			flags = GPIOF_DIR_OUT | (tdev->reset_pin_active ? GPIOF_INIT_HIGH : GPIOF_INIT_LOW);
			rc = devm_gpio_request_one(&pdev->dev, tdev->reset_pin, flags, "cvi_panel_reset");
			if (rc) {
				tdev->reset_pin = -EINVAL;
				dev_err(&pdev->dev, "request reset-gpio fail!\n");
			}
		} else {
			tdev->reset_pin = -EINVAL;
			dev_err(&pdev->dev, "get reset-gpio fail!\n");
		}

		if (gpio_is_valid(tdev->pwm_pin)) {
			flags = GPIOF_DIR_OUT | (tdev->pwm_pin_active ? GPIOF_INIT_HIGH : GPIOF_INIT_LOW);
			rc = devm_gpio_request_one(&pdev->dev, tdev->pwm_pin, flags, "cvi_pwm");
			if (rc) {
				tdev->pwm_pin = -EINVAL;
				dev_err(&pdev->dev, "request&set pwm-gpio fail!\n");
			}
		}

		if (gpio_is_valid(tdev->power_ct_pin)) {
			flags = GPIOF_DIR_OUT | (tdev->power_ct_pin_active ? GPIOF_INIT_HIGH : GPIOF_INIT_LOW);
			rc = devm_gpio_request_one(&pdev->dev, tdev->power_ct_pin, flags, "cvi_power_ct");
			if (rc) {
				tdev->power_ct_pin = -EINVAL;
				dev_err(&pdev->dev, "request&set power-ct-gpio fail!\n");
			}
		}
	}

	dev_info(&pdev->dev, "vbat irq(%d)\n", tdev->irq_vbat);
	dev_info(&pdev->dev, "reset gpio pin(%d) active(%d)\n", tdev->reset_pin, tdev->reset_pin_active);
	dev_info(&pdev->dev, "power ctrl gpio pin(%d) active(%d)\n", tdev->power_ct_pin, tdev->power_ct_pin_active);
	dev_info(&pdev->dev, "pwm gpio pin(%d) active(%d)\n", tdev->pwm_pin, tdev->pwm_pin_active);

	return rc;
}

static void _power_off(struct cvi_vip_mipi_tx_dev *tdev)
{
	u8 cmd = 0x28;

	// send mipi display off
	sclr_dsi_dcs_write_buffer(0x05, &cmd, 1, debug & 0x01);

	if (gpio_is_valid(tdev->reset_pin)) {
		gpio_set_value(tdev->reset_pin, tdev->reset_pin_active);
		usleep_range(1000, 2000);
	}
}

static int _reboot_notify(struct notifier_block *nb,
			       unsigned long code, void *unused)
{
	if (code != SYS_RESTART)
		return NOTIFY_DONE;

	mutex_lock(&reboot_lock);

	if (!reboot_info)
		goto out;

	dev_info(reboot_info->dev, "%s\n", __func__);
	_power_off(reboot_info);

out:
	mutex_unlock(&reboot_lock);

	return NOTIFY_DONE;
}

static struct notifier_block _reboot_notifier = {
	.notifier_call = _reboot_notify,
};

static irqreturn_t vbat_isr(int irq, void *_dev)
{
	struct cvi_vip_mipi_tx_dev *tdev = _dev;

	// clr interrupt
	iowrite32(0x01, tdev->vbat_addr[0]);
	// disable since just one shot job
	iowrite32(0x00, tdev->vbat_addr[1]);

	_power_off(tdev);

	return IRQ_HANDLED;
}

static int cvi_mipi_tx_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_mipi_tx_dev *tdev;

	/* allocate main structure */
	tdev = devm_kzalloc(&pdev->dev, sizeof(*tdev), GFP_KERNEL);
	if (!tdev)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, tdev);
	platform_set_drvdata(pdev, tdev);
	tdev->dev = &pdev->dev;

	_init_resources(pdev);

	if (tdev->irq_vbat > 0) {
		tdev->vbat_addr[0] = ioremap(0x03000220, 0x4);
		tdev->vbat_addr[1] = ioremap(0x03005144, 0x4);
		if (devm_request_irq(&pdev->dev, tdev->irq_vbat, vbat_isr, IRQF_SHARED,
			 "CVI_MIPI_TX_VBAT", tdev)) {
			dev_err(&pdev->dev, "Unable to request vbat IRQ(%d)\n", tdev->irq_vbat);
			return -EINVAL;
		}
	}

	rc = mipi_tx_proc_init();
	if (rc) {
		dev_err(&pdev->dev, "mipi_tx proc init fail\n");
	}

	rc = _device_register(tdev);
	if (rc)
		goto err_res;

	mutex_lock(&reboot_lock);
	if (!reboot_info)
		reboot_info = tdev;
	mutex_unlock(&reboot_lock);

	register_reboot_notifier(&_reboot_notifier);

	return rc;

err_res:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);
	return rc;
}

static int cvi_mipi_tx_remove(struct platform_device *pdev)
{
	struct cvi_vip_mipi_tx_dev *tdev;

	tdev = dev_get_drvdata(&pdev->dev);
	if (!tdev) {
		dev_err(&pdev->dev, "Can not get cvi_mipi_tx drvdata");
		return -ENODEV;
	}

	unregister_reboot_notifier(&_reboot_notifier);
	mutex_lock(&reboot_lock);
	if (reboot_info == tdev)
		reboot_info = NULL;
	mutex_unlock(&reboot_lock);
	_power_off(tdev);

	misc_deregister(&tdev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	if (tdev->clk_dsi && __clk_is_enabled(tdev->clk_dsi))
		clk_disable_unprepare(tdev->clk_dsi);
	if (tdev->clk_disp && __clk_is_enabled(tdev->clk_disp))
		clk_disable_unprepare(tdev->clk_disp);

	mipi_tx_proc_remove();

	return 0;
}

static const struct of_device_id cvi_mipi_tx_dt_match[] = { { .compatible = "cvitek,mipi_tx" }, {} };

static struct platform_driver cvi_mipi_tx_pdrv = {
	.probe = cvi_mipi_tx_probe,
	.remove = cvi_mipi_tx_remove,
	.driver = {
		.name = MIPI_TX_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cvi_mipi_tx_dt_match,
	},
};

static int __init mipi_tx_init(void)
{
	return platform_driver_register(&cvi_mipi_tx_pdrv);
}

static void __exit mipi_tx_exit(void)
{
	platform_driver_unregister(&cvi_mipi_tx_pdrv);
}

MODULE_DESCRIPTION("Cvitek MIPI-TX Driver");
MODULE_AUTHOR("Jammy Huang");
MODULE_LICENSE("GPL");
module_init(mipi_tx_init);
module_exit(mipi_tx_exit);
