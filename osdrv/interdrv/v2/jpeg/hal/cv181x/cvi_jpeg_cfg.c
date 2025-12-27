/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cv182x_jpeg_cfg.c
 * Description: jpeg cv182x implementation
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/of.h>
#include "cvi_jpeg.h"
#include "jpeg_common.h"

static void jpu_cfg_clk_enable(struct cvi_jpu_device *jdev);
static void jpu_cfg_clk_disable(struct cvi_jpu_device *jdev);
// static void jpu_fpga_init(struct cvi_jpu_device *jdev);

const struct jpu_ops asic_jpu_ops = {
	.clk_get = jpu_clk_get,
	.clk_put = jpu_clk_put,
	.clk_enable = jpu_cfg_clk_enable,
	.clk_disable = jpu_cfg_clk_disable,
	.config_pll = NULL,
};
#if 0

const struct jpu_ops fpga_jpu_ops = {
	.clk_get = NULL,
	.clk_put = NULL,
	.clk_enable = jpu_fpga_init,
	.clk_disable = NULL,
	.config_pll = NULL,
};

static const struct jpu_pltfm_data fpga_jpu_pdata = {
	.ops = &fpga_jpu_ops,
	.quirks = JPU_QUIRK_SUPPORT_FPGA,
	.version = 0x1821A,
};

static const struct jpu_pltfm_data pxp_jpu_pdata = {
	.ops = NULL,
	.quirks = 0,
	.version = 0x1821A,
};
#endif
static const struct jpu_pltfm_data asic_jpu_pdata = {
	.ops = &asic_jpu_ops,
	.quirks = JPU_QUIRK_SUPPORT_CLOCK_CONTROL,
	.version = 0x1821A,
};

const struct of_device_id cvi_jpu_match_table[] = {
#if 0
	{ .compatible = "cvitek,fpga-jpeg", .data = &fpga_jpu_pdata},  // for fpga
	{ .compatible = "cvitek,pxp-jpeg",  .data = &pxp_jpu_pdata},   // for pxp
#endif
	{ .compatible = "cvitek,asic-jpeg", .data = &asic_jpu_pdata},  // for asic
	{},
};

MODULE_DEVICE_TABLE(of, cvi_jpu_match_table);

static void jpu_cfg_clk_enable(struct cvi_jpu_device *jdev)
{
	WARN_ON(!jdev->clk_jpeg);
	WARN_ON(!jdev->clk_apb_jpeg);
	WARN_ON(!jdev->clk_vc_src0);
	WARN_ON(!jdev->clk_cfg_reg_vc);
	WARN_ON(!jdev->clk_axi_video_codec);

	clk_prepare_enable(jdev->clk_jpeg);
	clk_prepare_enable(jdev->clk_apb_jpeg);

	clk_prepare_enable(jdev->clk_vc_src0);
	clk_prepare_enable(jdev->clk_cfg_reg_vc);
	clk_prepare_enable(jdev->clk_axi_video_codec);
}

static void jpu_cfg_clk_disable(struct cvi_jpu_device *jdev)
{
	if (jpu_mask & JPU_MASK_DISABLE_CLK_GATING)
		return;
#if 0
	clk_disable_unprepare(jdev->clk_jpeg);
	clk_disable_unprepare(jdev->clk_apb_jpeg);

	clk_disable_unprepare(jdev->clk_vc_src0);
	clk_disable_unprepare(jdev->clk_cfg_reg_vc);
	clk_disable_unprepare(jdev->clk_axi_video_codec);
#endif
}
#if 0
static void jpu_fpga_init(struct cvi_jpu_device *jdev)
{
#if 0
	jpudrv_buffer_t *pReg = &jdev->jpu_control_register;

	writel(0xFF, pReg->virt_addr + 0x30); // reg_dis_fab_lp_opt, bus low power control off
#endif
}
#endif