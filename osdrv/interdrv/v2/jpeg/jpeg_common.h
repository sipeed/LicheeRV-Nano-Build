/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: jpeg_common.h
 * Description: jpeg chip common interface definition
 */

#ifndef __JPEG_COMMON_H__
#define __JPEG_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/of.h>
#include "cvi_jpeg.h"

extern const struct of_device_id cvi_jpu_match_table[];

void jpu_clk_disable(struct cvi_jpu_device *jdev);
void jpu_clk_enable(struct cvi_jpu_device *jdev);
void jpu_clk_get(struct cvi_jpu_device *jdev);
void jpu_clk_put(struct cvi_jpu_device *jdev);
void cv1835_config_pll(struct cvi_jpu_device *jdev);

#ifdef __cplusplus
}
#endif

#endif
