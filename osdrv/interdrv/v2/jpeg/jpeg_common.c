/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: jpeg_common.c
 * Description: jpeg chip common interface
 */

#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/version.h>
#include "cvi_jpeg.h"
#include "jpeg_common.h"

void jpu_clk_get(struct cvi_jpu_device *jdev)
{
	struct device *dev = jdev->dev;

	jdev->clk_axi_video_codec = devm_clk_get(dev, "clk_axi_video_codec");
	if (IS_ERR(jdev->clk_axi_video_codec)) {
		dev_err(dev, "failed to get clk_axi_video_codec\n");
	}
	jdev->clk_jpeg = devm_clk_get(dev, "clk_jpeg");
	if (IS_ERR(jdev->clk_jpeg)) {
		dev_err(dev, "failed to get clk_jpeg\n");
	}
	jdev->clk_apb_jpeg = devm_clk_get(dev, "clk_apb_jpeg");
	if (IS_ERR(jdev->clk_apb_jpeg)) {
		dev_err(dev, "failed to get clk_apb_jpeg\n");
	}
	jdev->clk_vc_src0 = devm_clk_get(dev, "clk_vc_src0");
	if (IS_ERR(jdev->clk_vc_src0)) {
		dev_err(dev, "failed to get clk_vc_src0\n");
	}
	jdev->clk_vc_src1 = devm_clk_get(dev, "clk_vc_src1");
	if (IS_ERR(jdev->clk_vc_src1)) {
		dev_err(dev, "failed to get clk_vc_src1\n");
	}
	jdev->clk_vc_src2 = devm_clk_get(dev, "clk_vc_src2");
	if (IS_ERR(jdev->clk_vc_src2)) {
		dev_err(dev, "failed to get clk_vc_src2\n");
	}
	jdev->clk_cfg_reg_vc = devm_clk_get(dev, "clk_cfg_reg_vc");
	if (IS_ERR(jdev->clk_cfg_reg_vc)) {
		dev_err(dev, "failed to get clk_cfg_reg_vc\n");
	}
}

void jpu_clk_put(struct cvi_jpu_device *jdev)
{
	struct device *dev = jdev->dev;

	devm_clk_put(dev, jdev->clk_jpeg);
}
#if 0
void jpu_clk_enable(struct cvi_jpu_device *jdev)
{
	JPU_DBG_CLK("enable\n");

	clk_prepare_enable(jdev->clk_jpeg);

	clk_prepare_enable(jdev->clk_apb_jpeg);

	clk_prepare_enable(jdev->clk_vc_src0);
	clk_prepare_enable(jdev->clk_vc_src1);
	clk_prepare_enable(jdev->clk_vc_src2);

	clk_prepare_enable(jdev->clk_cfg_reg_vc);
}

void jpu_clk_disable(struct cvi_jpu_device *jdev)
{
	if (jpu_mask & JPU_MASK_DISABLE_CLK_GATING)
		return;

	clk_disable_unprepare(jdev->clk_jpeg);

	clk_disable_unprepare(jdev->clk_apb_jpeg);

	clk_disable_unprepare(jdev->clk_vc_src0);
	clk_disable_unprepare(jdev->clk_vc_src1);
	clk_disable_unprepare(jdev->clk_vc_src2);

	clk_disable_unprepare(jdev->clk_cfg_reg_vc);
}

void cv1835_config_pll(struct cvi_jpu_device *jdev)
{
	unsigned int val;
	void __iomem *base_030001;
	void __iomem *base_030028;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	base_030001 = ioremap(0x03000100, 0x100);
	base_030028 = ioremap(0x03002800, 0x100);
#else
	base_030001 = ioremap_nocache(0x03000100, 0x100);
	base_030028 = ioremap_nocache(0x03002800, 0x100);
#endif

	// 0x03002840   = 0x3E   //enable synthesizer clock enable
	writel(0x3E, base_030028 + 0x40);

	// 0x03002854   = 385505882   //set apll synthesizer  104.4480001 M
	writel(385505882, base_030028 + 0x54);

	//0x03002850 ^= 0x00000001 // bit 0 toggle
	val = readl(base_030028 + 0x50);
	writel((val ^ 0x1), base_030028 + 0x50);

	// 0x0300280C   = 0x01108201 // set apll *8/2
	writel(0x01108201, base_030028 + 0x0C);

	// 0x03002800  &= ~0x00000010 //clear apll pd  , apll = 417.792
	val = readl(base_030028 + 0x00);
	writel((val &= (~0x10)), base_030028 + 0x00);

	// 0x03002884   = 421827145   //set cam1 synthesizer  95.45454549 M
	writel(421827145, base_030028 + 0x84);

	// 0x03002880 ^= 0x00000001 // bit 0 toggle
	val = readl(base_030028 + 0x80);
	writel((val ^ 0x1), base_030028 + 0x80);

	// 0x03002818   = 0x00168000 // set cam1 *11/1
	writel(0x00168000, base_030028 + 0x18);

	// 0x03002800  &= ~0x00010000 //clear cam1pll pd  , cam1pll = 1050
	val = readl(base_030028 + 0x00);
	writel((val &= (~0x00010000)), base_030028 + 0x00);

	// #=========measure apll
	// 0x03000140  = 0x00000000
	// 0x03000140  = 0x00000049
	// Read 0x03000140 >>8 =  ~ 2085

	// #=========measure cam1pll
	// 0x03000140  = 0x00000000
	// 0x03000140  = 0x00000079
	// Read 0x03000140 >>8 = ~ 5250
	iounmap(base_030001);
	iounmap(base_030028);
}
#endif