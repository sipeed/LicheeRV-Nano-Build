/*
 * linux/drivers/video/cvifb.c
 *
 * Frame Buffer Device for CVITEK.
 *
 * Copyright (C) 2020 cvitek
 * Copyright (C) 2020 Jammy Huang <jammy.huang@wisecore.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif

#include <linux/cvi_comm_vo.h>

#include <vip_common.h>
#include "scaler.h"
#include "base.h"
#include "vo_cb.h"

#define MAX_PALETTES 16
#define VXRES_SIZE(xres, bpp)                                                 \
	ALIGN((xres), GOP_ALIGNMENT / (bpp/8))
#define FB_LINE_SIZE(vxres, bpp)                                               \
	ALIGN(ALIGN((vxres) * (bpp), 8) / 8, GOP_ALIGNMENT)

static unsigned long def_vxres;
static unsigned long def_vyres;
static char *mode_option;
static bool double_buffer;
static int scale;
static bool fb_on_sc;
static int rdma_window;
static int option = 1; // default enable double buffer
static int opt_bpp = 32;

static const struct fb_fix_screeninfo cvifb_fix = {
	.id =		"cvifb",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_TRUECOLOR,
	.xpanstep =	1,
	.ypanstep =	1,
	.accel =	FB_ACCEL_NONE,
};

/*
 * base: phy-addr of the framebuffer.
 * len: length of the framebuffer in bytes.
 * offset: offset in the framebuffer for OSD start to read.
 * reg_base: phy-addr of the dev registers.
 * mem_base: phy-addr of frame buffer mem
 */
struct cvifb_par {
	u64 reg_base, mem_base;
	u32 reg_len, mem_len, mem_offset;
	int irq_num;
	u32 pseudo_palette[MAX_PALETTES];
	atomic_t ref_count;

	/* cvitek specific registers */
	enum sclr_gop_format fmt;
	u32 colorkey;       // RGB888
	u16 font_fg_color;  // ARGB4444
	u16 font_bg_color;  // ARGB4444
};

static void _fb_enable(bool enable)
{
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	u8 layer = 1;
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP, layer);
#else
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
#endif

#ifdef CONFIG_ARCH_CV182X
	_ddr_ctrl_patch(enable);
#endif

	cfg->gop_ctrl.raw &= ~0xff;
	cfg->gop_ctrl.b.ow0_en = enable;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	sclr_gop_set_cfg(SCL_GOP_DISP, layer, cfg, true);
#else
	sclr_gop_set_cfg(SCL_GOP_DISP, cfg, true);
#endif
}

static void _fb_update_mode(struct fb_info *info)
{
	struct cvifb_par *par = info->par;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	u8 layer = 1;
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP, layer);
#else
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
#endif
	struct sclr_gop_ow_cfg ow_cfg;
	u8 ow_number;

	fb_dbg(info, "%s+\n", __func__);

	cfg->gop_ctrl.b.hscl_en = scale & BIT(0);
	cfg->gop_ctrl.b.vscl_en = ((scale & BIT(1)) != 0);

	ow_cfg.fmt = par->fmt;
	ow_cfg.img_size.w = info->var.xres;
	ow_cfg.img_size.h = info->var.yres;
	ow_cfg.mem_size.w =
		FB_LINE_SIZE(info->var.xres_virtual, info->var.bits_per_pixel);
	ow_cfg.mem_size.h = info->var.yres;
	ow_cfg.pitch = ow_cfg.mem_size.w;
	ow_cfg.start.x = 0;
	ow_cfg.start.y = 0;
	ow_cfg.crop_pixels = 0;
	ow_cfg.end.x = ow_cfg.start.x +
			(info->var.xres << cfg->gop_ctrl.b.hscl_en) - cfg->gop_ctrl.b.hscl_en;
	ow_cfg.end.y = ow_cfg.start.y +
			(info->var.yres << cfg->gop_ctrl.b.vscl_en) - cfg->gop_ctrl.b.vscl_en;
	ow_cfg.addr = par->mem_base + par->mem_offset;

	ow_number = 0;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	sclr_gop_ow_set_cfg(SCL_GOP_DISP, layer, ow_number, &ow_cfg, true);
#else
	sclr_gop_ow_set_cfg(SCL_GOP_DISP, ow_number, &ow_cfg, true);
#endif
}

static void _fb_activate_var(struct fb_info *info)
{
	fb_dbg(info, "%s+\n", __func__);

	_fb_enable(false);
	_fb_update_mode(info);
	if (!fb_on_sc)
		_fb_enable(true);
}

static int cvifb_open(struct fb_info *info, int user)
{
	struct cvifb_par *par = info->par;
	struct base_exe_m_cb exe_cb;

	fb_dbg(info, "%s+\n", __func__);

	fb_on_sc =
		(option & BIT(1)) ? true : (sclr_disp_mux_get() == SCLR_VO_SEL_I80);
	fb_dbg(info, "fb %s blended on sc.\n", fb_on_sc ? "is" : "isn't");
	if (fb_on_sc) {
		exe_cb.callee = E_MODULE_VO;
		exe_cb.caller = E_MODULE_FB;
		exe_cb.cmd_id = VO_CB_SET_FB_ON_VPSS;
		exe_cb.data   = (void *)&fb_on_sc;

		if (base_exe_module_cb(&exe_cb)) {
			fb_dbg(info, "base_exe_module_cb set fb_on_sc failed!.\n");
		}
	}

	if (atomic_add_return(1, &par->ref_count) == 1)
		_fb_activate_var(info);

	return 0;
}

static int cvifb_release(struct fb_info *info, int user)
{
	struct cvifb_par *par = info->par;

	fb_dbg(info, "%s+\n", __func__);

	if (atomic_sub_return(1, &par->ref_count) == 0)
		_fb_enable(false);

	return 0;
}

static int _cvifb_decode_var(const struct fb_var_screeninfo *var,
		struct cvifb_par *par, struct fb_info *info)
{
	/*
	 * Get the video params out of 'var'.
	 * If it's too big, return -EINVAL.
	 */

	u32 xres, right, hslen, left, xtotal;
	u32 yres, lower, vslen, upper, ytotal;
	u32 vxres, xoffset, vyres, yoffset;
	u32 bpp, mem_size, pitch;

	fb_dbg(info, "%s+\n", __func__);

	fb_dbg(info, "info->var:\n");
	fb_dbg(info, "  xres: %i, yres: %i, xres_v: %i, yres_v: %i\n",
			var->xres, var->yres, var->xres_virtual, var->yres_virtual);
	fb_dbg(info, "	xoff: %i, yoff: %i, bpp: %i, graysc: %i\n", var->xoffset,
			var->yoffset, var->bits_per_pixel, var->grayscale);
	fb_dbg(info, "	activate: %i, nonstd: %i, vmode: %i\n", var->activate,
			var->nonstd, var->vmode);
	fb_dbg(info, "	pixclock: %i, hsynclen:%i, vsynclen:%i\n", var->pixclock,
			var->hsync_len, var->vsync_len);
	fb_dbg(info, "	left: %i, right: %i, up:%i, lower:%i\n",
			var->left_margin, var->right_margin, var->upper_margin,
			var->lower_margin);

	bpp = var->bits_per_pixel;
	switch (bpp) {
	case 1 ... 8:
		bpp = 8;
		break;
	case 9 ... 16:
		bpp = 16;
		break;
	case 25 ... 32:
		bpp = 32;
		break;
	default:
		return -EINVAL;
	}

	xres = var->xres;
	vxres = var->xres_virtual;
	if (vxres < xres) {
		fb_err(info, "xres_virtual(%d) is smaller than xres(%d)\n", vxres, xres);
		return -EINVAL;
	}

	xoffset = var->xoffset;
	if (xres + xoffset > vxres) {
		fb_err(info,
				"xres_offset(%d) is greater than xres_virtual(%d) - xres(%d)\n",
				xoffset, vxres, xres);
		return -EINVAL;
	}

	left = var->left_margin;
	right = var->right_margin;
	hslen = var->hsync_len;

	yres = var->yres;
	vyres = var->yres_virtual;
	if (vyres < yres) {
		fb_err(info, "yres_virtual(%d) is smaller than yres(%d)\n", vyres, yres);
		return -EINVAL;
	}

	yoffset = var->yoffset;
	if (yres + yoffset > vyres) {
		fb_err(info,
				"yres_offset(%d) is greater than yres_virtual(%d) - yres(%d)\n",
				yoffset, vyres, yres);
		return -EINVAL;
	}

	lower = var->lower_margin;
	vslen = var->vsync_len;
	upper = var->upper_margin;

	pitch = FB_LINE_SIZE(vxres, bpp);
	mem_size = pitch * vyres;
	if (mem_size > info->fix.smem_len) {
		fb_err(info, "not enough video memory (%d KB requested, %d KB available)\n",
				mem_size >> 10, info->fix.smem_len >> 10);
		return -ENOMEM;
	}

	xtotal = xres + right + hslen + left;
	ytotal = yres + lower + vslen + upper;

	/* fb_dbg for decode variables. */
	fb_dbg(info, "checked variables:\n");
	fb_dbg(info, "  xres: %i, yres: %i, xres_v: %i, yres_v: %i\n",
			xres, yres, vxres, vyres);
	fb_dbg(info, "	xoff: %i, yoff: %i, bpp: %i, pitch: %i, mem_size: %i\n",
			xoffset, yoffset, bpp, pitch, mem_size);
	fb_dbg(info, "	left: %i, right: %i, up:%i, lower:%i\n",
			left, right, upper, lower);
	fb_dbg(info, "	xtotal: %i, ytotal: %i\n", xtotal, ytotal);

	par->mem_offset = yoffset * pitch + ALIGN(xoffset * bpp, 8) / 8;
	switch (bpp) {
	case 1:
		par->fmt = SCL_GOP_FMT_FONT;
		break;
	case 8:
		par->fmt = SCL_GOP_FMT_256LUT;
		break;
	case 16: /* trrrrrgg gggbbbbb */
		par->fmt = (var->green.length == 4) ? SCL_GOP_FMT_ARGB4444 : SCL_GOP_FMT_ARGB1555;
		break;
	case 32:
		par->fmt = SCL_GOP_FMT_ARGB8888;
		break;
	}
	fb_dbg(info, "gop fmt(%d) offset(%d)\n", par->fmt, par->mem_offset);

	return 0;
}

static int cvifb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	u32 mem_size, pitch;
	u32 max_vyres;

	fb_dbg(info, "%s+\n", __func__);

	switch (var->bits_per_pixel) {
	case 8: // 256 LUT, pseudo color
		var->red.offset = var->green.offset = var->blue.offset = 0;
		var->red.length = var->green.length = var->blue.length = 8;
		break;
	case 16:
		if (var->green.length == 4) {
			// ARGB4444
			var->transp.offset = 12;
			var->red.offset = 8;
			var->green.offset = 4;
			var->blue.offset = 0;
			var->transp.length = var->red.length = var->green.length = var->blue.length = 4;
		} else {
			// ABGR1555
			var->transp.offset = 15;
			var->blue.offset = 10;
			var->green.offset = 5;
			var->red.offset = 0;
			var->transp.length = 1;
			var->red.length	= var->green.length = var->blue.length = 5;
		}
		break;
	case 32: // ABGR8888
		var->transp.offset = 24;
		var->blue.offset = 16;
		var->green.offset = 8;
		var->red.offset = 0;
		var->transp.length = 8;
		var->red.length = var->green.length = var->blue.length = 8;
		break;
	default:
		return -EINVAL;
	}

	/* Use xres/yres to set xres/yres_virtual. */
	var->xres_virtual = VXRES_SIZE(var->xres, var->bits_per_pixel);

	var->yres_virtual = double_buffer ? (var->yres * 2) : var->yres;

	pitch = FB_LINE_SIZE(var->xres_virtual, var->bits_per_pixel);
	if ((info->var.xres != var->xres) || (info->var.yres != var->yres)
			|| (info->var.xres_virtual != var->xres_virtual)
			|| (info->var.yres_virtual != var->yres_virtual))
		info->fix.smem_len = pitch * var->yres * (1 + double_buffer);

	/* maximize virtual vertical size for fast scrolling */
	max_vyres = info->fix.smem_len / pitch;
	if (var->yres_virtual > max_vyres)
		var->yres_virtual = max_vyres;

	if (var->xres + var->xoffset > var->xres_virtual) {
		fb_err(info, "xres(%d) + xoffset(%d) > xres_virtual(%d)\n",
				var->xres, var->xoffset, var->xres_virtual);
		return -EINVAL;
	}

	if (var->yres + var->yoffset > var->yres_virtual) {
		fb_err(info, "yres(%d) + yoffset(%d) > yres_virtual(%d)\n",
				var->yres, var->yoffset, var->yres_virtual);
		return -EINVAL;
	}

	mem_size = pitch * var->yres_virtual;
	if (mem_size > info->fix.smem_len) {
		fb_err(info, "not enough video memory (%d KB requested, %d KB available)\n",
				mem_size >> 10, info->fix.smem_len >> 10);
		return -ENOMEM;
	}

	if (info->monspecs.hfmax && info->monspecs.vfmax &&
			info->monspecs.dclkmax && fb_validate_mode(var, info) < 0)
		return -EINVAL;

	/* Interlaced mode not supported */
	if (var->vmode & FB_VMODE_INTERLACED)
		return -EINVAL;

	return 0;
}

static int cvifb_set_par(struct fb_info *info)
{
	struct cvifb_par *par = info->par;
	u32 len, pitch;
	int rc;

	fb_dbg(info, "%s+\n", __func__);

	rc = _cvifb_decode_var(&info->var, par, info);
	if (rc)
		return rc;

	pitch = FB_LINE_SIZE(info->var.xres_virtual, info->var.bits_per_pixel);
	len = pitch * info->var.yres * (1 + double_buffer);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(info->fix.smem_start, info->fix.smem_len, DMA_TO_DEVICE);
#else
	__dma_map_area(info->screen_base, info->fix.smem_len, DMA_TO_DEVICE);
#endif

	smp_mb();	/*memory barrier*/

	info->fix.line_length =
		FB_LINE_SIZE(info->var.xres_virtual, info->var.bits_per_pixel);
	if (info->var.bits_per_pixel == 1)
		info->fix.visual = FB_VISUAL_MONO01;
	else if (info->var.bits_per_pixel == 8)
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		info->fix.visual = FB_VISUAL_TRUECOLOR;

	_fb_activate_var(info);
	return 0;
}

static int _fb_dosetcolreg(u16 regno, u16 red, u16 green, u16 blue, u16 transp)
{
	u16 data;

	// ARGB4444 only
	data = ((transp >> 12) << 12) | ((red >> 12) << 8) | ((green >> 12) << 4) | blue >> 12;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	return sclr_gop_update_256LUT(SCL_GOP_DISP, 1, regno, data);
#else
	return sclr_gop_update_LUT(SCL_GOP_DISP, regno, data);
#endif
}

static int cvifb_setcolreg(u32 regno, u32 red, u32 green,
		u32 blue, u32 transp, struct fb_info *info)
{
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	u8 layer = 1;
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP, layer);
#else
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
#endif
	u32 r, g, b;

	fb_dbg(info, "%s+\n", __func__);

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (info->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green + 7471 * blue) >> 16;

	switch (info->fix.visual) {
	case FB_VISUAL_PSEUDOCOLOR:
		if (regno >= info->cmap.len)
			return -EINVAL;

		/* After OSD window enable, LUT is not updatable. */
		if (cfg->gop_ctrl.b.ow0_en == true)
			return 0;

		_fb_dosetcolreg(regno, red, green, blue, transp);
		break;
	case FB_VISUAL_TRUECOLOR:
		if (regno >= 16)
			return -EINVAL;
		r = (red >> (16 - info->var.red.length)) << info->var.red.offset;
		b = (blue >> (16 - info->var.blue.length)) << info->var.blue.offset;
		g = (green >> (16 - info->var.green.length)) << info->var.green.offset;
		((u32 *)info->pseudo_palette)[regno] = r | g | b;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int cvifb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	u8 layer = 1;
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP, layer);
#else
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
#endif
	u16 *red, *green, *blue, *transp;
	u16 trans = 0xffff;
	int i, index;

	fb_dbg(info, "%s+\n", __func__);

	/* After OSD window enable, LUT is not updatable. */
	if (cfg->gop_ctrl.b.ow0_en == true)
		return 0;

	red    = cmap->red;
	green  = cmap->green;
	blue   = cmap->blue;
	transp = cmap->transp;
	index  = cmap->start;
	for (i = 0; i < cmap->len; ++i) {
		if (transp)
			trans = *transp++;

		_fb_dosetcolreg(index++, *red++, *green++, *blue++, trans);
	}
	return 0;
}

static int cvifb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct cvifb_par *par = info->par;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	u8 layer = 1;
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP, layer);
#else
	struct sclr_gop_cfg *cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
#endif
	u8 ow_number;

	fb_dbg(info, "%s+\n", __func__);

	par->mem_offset = var->yoffset * info->fix.line_length +
		ALIGN(var->xoffset * var->bits_per_pixel, 8) / 8;

	dev_dbg(info->device,
			"pan_display: xoffset: %i yoffset: %i offset: %i\n",
			var->xoffset, var->yoffset, par->mem_offset);

	cfg->ow_cfg[0].addr = par->mem_base + par->mem_offset;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(info->fix.smem_start, info->fix.smem_len, DMA_TO_DEVICE);
#else
	__dma_map_area(info->screen_base, info->fix.smem_len, DMA_TO_DEVICE);
#endif

	smp_mb();	/*memory barrier*/
	ow_number = 0;
#if defined( __SOC_MARS__) || defined( __SOC_PHOBOS__)
	sclr_gop_ow_set_cfg(SCL_GOP_DISP, layer, ow_number, &cfg->ow_cfg[0], true);
#else
	sclr_gop_ow_set_cfg(SCL_GOP_DISP, ow_number, &cfg->ow_cfg[0], true);
#endif

	return 0;
}

static int cvifb_ioctl(struct fb_info *info, u32 cmd, unsigned long arg)
{
	return 0;
}

#ifdef CONFIG_COMPAT
static int cvifb_compat_ioctl(struct fb_info *info, u32 cmd, unsigned long arg)
{
	return 0;
}
#endif

/*
 *  Frame buffer operations
 */
static struct fb_ops cvifb_ops = {
	.owner            = THIS_MODULE,
	.fb_open          = cvifb_open,
	.fb_release       = cvifb_release,
	.fb_check_var     = cvifb_check_var,
	/* set the video mode according to info->var */
	.fb_set_par       = cvifb_set_par,
	/* set color register */
	.fb_setcolreg     = cvifb_setcolreg,
	/* set color registers in batch */
	.fb_setcmap       = cvifb_setcmap,
	/* pan display */
	.fb_pan_display   = cvifb_pan_display,
	.fb_ioctl         = cvifb_ioctl,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl  = cvifb_compat_ioctl,
#endif
	/* Draws a rectangle */
	.fb_fillrect    = cfb_fillrect,
	/* Copy data from area to another */
	.fb_copyarea    = cfb_copyarea,
	/* Draws an image to the display */
	.fb_imageblit   = cfb_imageblit,
};

static int _get_reserved_mem(struct platform_device *pdev,
	uint64_t *addr, uint64_t *size)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;

	if (!pdev) {
		dev_err(&pdev->dev, "[FB] null pointer\n");
		return -EINVAL;
	}

	target = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!target) {
		dev_err(&pdev->dev, "[FB] No %s specified\n", "memory-region");
		return -EINVAL;
	}

	prmem = of_reserved_mem_lookup(target);
	of_node_put(target);

	if (!prmem) {
		dev_err(&pdev->dev, "[FB]: cannot acquire memory-region\n");
		return -EINVAL;
	}

	*addr = prmem->base;
	*size = prmem->size;

	return 0;
}


static int _init_resources(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	struct cvifb_par *par = info->par;
	int rc = 0;
	struct resource *res = NULL;
	uint64_t resv_addr, resv_size;

	if (_get_reserved_mem(pdev, &resv_addr, &resv_size)) {
		dev_err(&pdev->dev, "get reserved memmory failed!\n");
		return -ENODEV;
	}
	par->mem_base = resv_addr;
	par->mem_len = resv_size;
	fb_info(info, "reserved mem: start: 0x%llx, size: 0x%llx.\n",
		resv_addr, resv_size);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	par->reg_base = res->start;
	par->reg_len = res->end - res->start;
	fb_info(info, "res-reg: start: 0x%llx, end: 0x%llx.\n",
			res->start, res->end);

	return rc;
}

int cvifb_probe(struct platform_device *pdev)
{
	int ret;
	struct fb_info *info = NULL;
	struct cvifb_par *par;
	u32 len, pitch;

	double_buffer = option & BIT(0);

	info = framebuffer_alloc(sizeof(struct cvifb_par), &pdev->dev);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	ret = _init_resources(pdev);
	if (ret) {
		dev_err(info->device, "dts parsing ng.\n");
		goto err_dts;
	}

	par = info->par;

	info->fix = cvifb_fix;
	info->fix.mmio_start = par->reg_base;
	info->fix.mmio_len = par->reg_len;
	info->fix.smem_start = par->mem_base;
	info->fix.smem_len = par->mem_len;
	info->flags = FBINFO_DEFAULT | FBINFO_HWACCEL_YPAN;

	info->var.activate = FB_ACTIVATE_NOW;
	info->var.bits_per_pixel = 8;
	info->fbops = &cvifb_ops;
	info->pseudo_palette = par->pseudo_palette;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	info->screen_base = devm_ioremap(&pdev->dev,
			info->fix.smem_start, info->fix.smem_len);
#else
	info->screen_base = devm_ioremap_nocache(&pdev->dev,
			info->fix.smem_start, info->fix.smem_len);
#endif
	info->screen_size = info->fix.smem_len;

	if (mode_option) {
		ret = fb_find_mode(&info->var, info, mode_option,
				info->monspecs.modedb,
				info->monspecs.modedb_len, NULL,
				info->var.bits_per_pixel);
		if (!ret || ret == 4) {
			dev_err(info->device, "mode %s not found\n", mode_option);
			ret = -EINVAL;
		}
	} else {
		struct sclr_disp_timing timing;

		sclr_disp_get_hw_timing(&timing);
		info->var.xres = timing.hfde_end - timing.hfde_start + 1;
		info->var.yres = timing.vfde_end - timing.vfde_start + 1;
		switch (opt_bpp) {
		case 16: case 32:
			info->var.bits_per_pixel = opt_bpp;
			break;
		default:
			dev_err(info->device, "bpp %d not support, fallback to 32\n", opt_bpp);
			info->var.bits_per_pixel = 32;
			break;
		}
		info->var.xres_virtual = VXRES_SIZE(info->var.xres, info->var.bits_per_pixel);
		info->var.yres_virtual =
			double_buffer ? (info->var.yres * 2) : info->var.yres;
		info->var.xoffset = 0;
		info->var.yoffset = 0;
		info->var.activate |= FB_ACTIVATE_TEST;
		info->var.pixclock = timing.htotal * timing.vtotal * 60;
		info->var.left_margin = timing.hfde_start - timing.hsync_end;
		info->var.right_margin = timing.htotal - timing.hfde_end;
		info->var.upper_margin = timing.vfde_start - timing.vsync_end;
		info->var.lower_margin = timing.vtotal - timing.vfde_end;
		info->var.hsync_len = timing.hsync_end - timing.hsync_start;
		info->var.vsync_len = timing.vsync_end - timing.vsync_start;
		info->var.sync &= ~(FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT);
		info->var.vmode = FB_VMODE_NONINTERLACED;
		info->var.activate &= ~FB_ACTIVATE_TEST;
		_cvifb_decode_var(&info->var, par, info);
	}
	fb_destroy_modedb(info->monspecs.modedb);
	info->monspecs.modedb = NULL;

	if (ret == -EINVAL)
		goto err_find_mode;

	if (scale & BIT(0)) {
		info->var.xres >>= 1;
		info->var.xres_virtual >>= 1;
	}
	if (scale & BIT(1)) {
		info->var.yres >>= 1;
		info->var.yres_virtual >>= 1;
	}

	pitch = FB_LINE_SIZE(info->var.xres_virtual, info->var.bits_per_pixel);
	info->fix.line_length = pitch;
	len = pitch * info->var.yres * (1 + double_buffer);

	// clear the framebuffer.
	memset_io(info->screen_base, 0x00, info->screen_size);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)) && defined(__riscv)
	arch_sync_dma_for_device(info->fix.smem_start, info->fix.smem_len, DMA_TO_DEVICE);
#else
	__dma_map_area(info->screen_base, info->fix.smem_len, DMA_TO_DEVICE);
#endif

	smp_mb();	/*memory barrier*/

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret) {
		dev_err(info->device, "cannot allocate colormap\n");
		goto err_alloc_cmap;
	}

	if (!mode_option)
		if (info->fbops->fb_check_var)
			info->fbops->fb_check_var(&info->var, info);
	fb_info(info, "init per current timing (%dx%d)\n",
			info->var.xres, info->var.yres);

	ret = register_framebuffer(info);
	if (ret) {
		dev_err(info->device, "error registering framebuffer\n");
		goto err_reg_framebuffer;
	}

	atomic_set(&par->ref_count, 0);

#ifdef CONFIG_ARCH_CV182X
	// axi_realtime_fab priority
	vip_axi_realtime_fab_priority();
#endif

	fb_info(info, "%s frame buffer device\n", info->fix.id);
	fb_info(info, "scale(%#x) double_buffer(%d)\n", scale, double_buffer);
	return 0;

err_reg_framebuffer:
	fb_dealloc_cmap(&info->cmap);
err_find_mode:
err_alloc_cmap:
err_dts:
	framebuffer_release(info);
	return ret;
}

static int cvifb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);

	fb_dbg(info, "%s+\n", __func__);

	if (info) {
		devm_iounmap(&pdev->dev, info->screen_base);
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}

#ifdef CONFIG_ARCH_CV182X
	_ddr_ctrl_patch(false);
#endif

	return 0;
}

static const struct of_device_id cvi_fb_dt_match[] = {
	{.compatible = "cvitek,fb"},
	{}
};

static struct platform_driver cvifb_driver = {
	.probe		= cvifb_probe,
	.remove		= cvifb_remove,
	.driver     = {
		.name		= "cvifb",
		.owner		= THIS_MODULE,
		.of_match_table = cvi_fb_dt_match,
	}
};

module_param_named(vxres, def_vxres, long, 0664);
module_param_named(vyres, def_vyres, long, 0664);
module_param(mode_option, charp, 0444);

/* scale: to control osd scale up option
 * - bit[0]: if true, h-scale x 2
 * - bit[1]: if true, v-scale x 2
 */
module_param(scale, int, 0444);
module_param(rdma_window, int, 0444);

/* option: to control fb options
 * - bit[0]: if true, double buffer
 * - bit[1]: if true, fb on vpss not vo
 */
module_param(option, int, 0444);

/* opt_bpp:
 * - 16: ABGR 1555
 * - 32: ABGR 8888
 */
module_param(opt_bpp, int, 0444);

MODULE_PARM_DESC(mode_option, "Default video mode (320x240-32@60', etc)");
MODULE_PARM_DESC(scale, "scale up of the fb canvas");

module_platform_driver(cvifb_driver);

MODULE_DESCRIPTION("Cvitek framebuffer Driver");
MODULE_AUTHOR("Jammy Huang");
MODULE_LICENSE("GPL");
