/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: vcodec_common.c
 * Description:
 */

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/version.h>
#include "cvi_vcodec.h"
#include "vcodec_common.h"

#define DRAM_32_35_REG_BASE_ADDR 0x3000064

int vcodec_mask = 0x1;

module_param(vcodec_mask, int, 0644);

void vpu_clk_get(struct cvi_vpu_device *vdev)
{
	struct device *dev = vdev->dev;

	vdev->clk_axi_video_codec = devm_clk_get(dev, "clk_axi_video_codec");
	if (IS_ERR(vdev->clk_axi_video_codec)) {
		dev_err(dev, "failed to get clk_axi_video_codec\n");
	}

	vdev->clk_h264c = devm_clk_get(dev, "clk_h264c");
	if (IS_ERR(vdev->clk_h264c)) {
		dev_err(dev, "failed to get clk_h264c\n");
	}

	vdev->clk_apb_h264c = devm_clk_get(dev, "clk_apb_h264c");
	if (IS_ERR(vdev->clk_apb_h264c)) {
		dev_err(dev, "failed to get clk_apb_h264c\n");
	}

	vdev->clk_h265c = devm_clk_get(dev, "clk_h265c");
	if (IS_ERR(vdev->clk_h265c)) {
		dev_err(dev, "failed to get clk_h265c\n");
	}

	vdev->clk_apb_h265c = devm_clk_get(dev, "clk_apb_h265c");
	if (IS_ERR(vdev->clk_apb_h265c)) {
		dev_err(dev, "failed to get clk_apb_h265c\n");
	}

	vdev->clk_vc_src0 = devm_clk_get(dev, "clk_vc_src0");
	if (IS_ERR(vdev->clk_vc_src0)) {
		dev_err(dev, "failed to get clk_vc_src0\n");
	}

	vdev->clk_vc_src1 = devm_clk_get(dev, "clk_vc_src1");
	if (IS_ERR(vdev->clk_vc_src1)) {
		dev_err(dev, "failed to get clk_vc_src1\n");
	}

	vdev->clk_vc_src2 = devm_clk_get(dev, "clk_vc_src2");
	if (IS_ERR(vdev->clk_vc_src2)) {
		dev_err(dev, "failed to get clk_vc_src2\n");
	}

	vdev->clk_cfg_reg_vc = devm_clk_get(dev, "clk_cfg_reg_vc");
	if (IS_ERR(vdev->clk_cfg_reg_vc)) {
		dev_err(dev, "failed to get clk_cfg_reg_vc\n");
	}
}

void vpu_clk_put(struct cvi_vpu_device *vdev)
{
	struct device *dev = vdev->dev;

	devm_clk_put(dev, vdev->clk_axi_video_codec);
	devm_clk_put(dev, vdev->clk_h264c);
	devm_clk_put(dev, vdev->clk_apb_h264c);
	devm_clk_put(dev, vdev->clk_h265c);
	devm_clk_put(dev, vdev->clk_apb_h265c);
	devm_clk_put(dev, vdev->clk_vc_src0);
	devm_clk_put(dev, vdev->clk_vc_src1);
	devm_clk_put(dev, vdev->clk_vc_src2);
	devm_clk_put(dev, vdev->clk_cfg_reg_vc);
}

void vpu_clk_enable(struct cvi_vpu_device *vdev, int mask)
{
	WARN_ON(!vdev->clk_h264c);
	WARN_ON(!vdev->clk_h265c);

	VCODEC_DBG_TRACE("mask = 0x%X\n", mask);

	if (mask & BIT(H264_CORE_IDX) && (__clk_is_enabled(vdev->clk_h264c) == false)) {
		clk_prepare_enable(vdev->clk_h264c);
		clk_prepare_enable(vdev->clk_apb_h264c);
	}

	if (mask & BIT(H265_CORE_IDX) && (__clk_is_enabled(vdev->clk_h265c) == false)) {
		clk_prepare_enable(vdev->clk_h265c);
		clk_prepare_enable(vdev->clk_apb_h265c);
	}

	clk_prepare_enable(vdev->clk_vc_src0);
	clk_prepare_enable(vdev->clk_vc_src1);
	clk_prepare_enable(vdev->clk_vc_src2);
	clk_prepare_enable(vdev->clk_cfg_reg_vc);
}

void vpu_clk_disable(struct cvi_vpu_device *vdev, int mask)
{
	if (vcodec_mask & VCODEC_MASK_DISABLE_CLK_GATING)
		return;

	WARN_ON(!vdev->clk_h264c);
	WARN_ON(!vdev->clk_h265c);

	if (mask & BIT(H264_CORE_IDX) && __clk_is_enabled(vdev->clk_h264c)) {
		clk_disable_unprepare(vdev->clk_h264c);
		clk_disable_unprepare(vdev->clk_apb_h264c);
	}

	if (mask & BIT(H265_CORE_IDX) && __clk_is_enabled(vdev->clk_h265c)) {
		clk_disable_unprepare(vdev->clk_h265c);
		clk_disable_unprepare(vdev->clk_apb_h265c);
	}

	clk_disable_unprepare(vdev->clk_vc_src0);
	clk_disable_unprepare(vdev->clk_vc_src1);
	clk_disable_unprepare(vdev->clk_vc_src2);
	clk_disable_unprepare(vdev->clk_cfg_reg_vc);
}

unsigned long vpu_clk_get_rate(struct cvi_vpu_device *vdev)
{
	unsigned long clk_rate = 0;

	if (vdev->clk_vc_src0)
		clk_rate = clk_get_rate(vdev->clk_vc_src0);

	pr_debug("clk_rate %lu\n", clk_rate);

	return clk_rate;
}

void cviConfigDDR(struct cvi_vpu_device *vdev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	unsigned int *dramRemap =
		(unsigned int *)ioremap(DRAM_32_35_REG_BASE_ADDR, 0x10);
#else
	unsigned int *dramRemap =
		(unsigned int *)ioremap_nocache(DRAM_32_35_REG_BASE_ADDR, 0x10);
#endif
	unsigned int val;

	val = *dramRemap;

	val |= (1 << 24);
	*dramRemap = val;

	iounmap((void *)dramRemap);
}
