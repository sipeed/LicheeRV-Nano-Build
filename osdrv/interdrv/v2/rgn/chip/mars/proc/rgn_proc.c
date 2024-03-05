#include "rgn_proc.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define RGN_PROC_NAME "cvitek/rgn"

static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
extern struct cvi_rgn_ctx rgn_prc_ctx;
/*************************************************************************
 *	Region proc functions
 *************************************************************************/
static void _pixFmt_to_String(enum _PIXEL_FORMAT_E PixFmt, char *str, int len)
{
	switch (PixFmt) {
	case PIXEL_FORMAT_8BIT_MODE:
		strncpy(str, "256LUT", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static int rgn_proc_show(struct seq_file *m, void *v)
{
	struct cvi_rgn_ctx *prgnCtx = &rgn_prc_ctx;
	int i;
	char c[32];

	if (!prgnCtx) {
		seq_puts(m, "rgn_prc_ctx = NULL\n");
		return -1;
	}

	seq_printf(m, "\nModule: [RGN], Build Time[%s]\n", UTS_VERSION);
	// Region status of overlay
	seq_puts(m, "\n------REGION STATUS OF OVERLAY--------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s%7s%12s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum",
		"Cmpr", "MaxNeedIon");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == OVERLAY_RGN && prgnCtx[i].bCreated) {
			memset(c, 0, sizeof(c));
			_pixFmt_to_String(prgnCtx[i].stRegion.unAttr.stOverlay.enPixelFormat, c, sizeof(c));

			seq_printf(m, "%7s%3d%10d%10s%20s%10d%10d%10x%20llx%20lx%10d%10d%7s%12d\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				(prgnCtx[i].bUsed) ? "Y" : "N",
				c,
				prgnCtx[i].stRegion.unAttr.stOverlay.stSize.u32Width,
				prgnCtx[i].stRegion.unAttr.stOverlay.stSize.u32Height,
				prgnCtx[i].stRegion.unAttr.stOverlay.u32BgColor,
				prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].u64PhyAddr,
				(uintptr_t)prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].pu8VirtAddr,
				prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].u32Stride,
				prgnCtx[i].stRegion.unAttr.stOverlay.u32CanvasNum,
				prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].bCompressed ? "Y" : "N",
				prgnCtx[i].u32MaxNeedIon);
		}
	}

	// Region chn status of overlay
	seq_puts(m, "\n------REGION CHN STATUS OF OVERLAY----------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == OVERLAY_RGN && prgnCtx[i].bCreated && prgnCtx[i].bUsed) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s%10d%10d%10d\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				MOD_STRING[prgnCtx[i].stChn.enModId],
				prgnCtx[i].stChn.s32DevId,
				prgnCtx[i].stChn.s32ChnId,
				(prgnCtx[i].stChnAttr.bShow) ? "Y" : "N",
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X,
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y,
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayChn.u32Layer);
		}
	}

	// Region status of cover
	seq_puts(m, "\n------REGION STATUS OF COVER----------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == COVER_RGN && prgnCtx[i].bCreated) {
			seq_printf(m, "%7s%3d%10d%10s\n", "#", prgnCtx[i].Handle, prgnCtx[i].stRegion.enType,
				(prgnCtx[i].bUsed) ? "Y" : "N");
		}
	}

	// Region chn status of rect cover
	seq_puts(m, "\n------REGION CHN STATUS OF RECT COVER-------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "Layer", "CoorType");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == COVER_RGN && prgnCtx[i].bCreated && prgnCtx[i].bUsed
			&& prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.enCoverType == AREA_RECT) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10d%10s\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				MOD_STRING[prgnCtx[i].stChn.enModId],
				prgnCtx[i].stChn.s32DevId,
				prgnCtx[i].stChn.s32ChnId,
				(prgnCtx[i].stChnAttr.bShow) ? "Y" : "N",
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.stRect.s32X,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.stRect.s32Y,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.stRect.u32Width,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.stRect.u32Height,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.u32Color,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.u32Layer,
				(prgnCtx[i].stChnAttr.unChnAttr.stCoverChn.enCoordinate == RGN_ABS_COOR) ?
					"ABS" : "RATIO");
		}
	}

	// Region status of coverex
	seq_puts(m, "\n------REGION STATUS OF COVEREX--------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "Hdl", "Type", "Used");
	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == COVEREX_RGN && prgnCtx[i].bCreated) {
			seq_printf(m, "%7s%3d%10d%10s\n", "#", prgnCtx[i].Handle, prgnCtx[i].stRegion.enType,
				(prgnCtx[i].bUsed) ? "Y" : "N");
		}
	}

	// Region chn status of rect coverex
	seq_puts(m, "\n------REGION CHN STATUS OF RECT COVEREX-----------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n%10s%10s%10s%10s%10s%10s\n\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow",
		"X", "Y", "W", "H", "Color", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == COVEREX_RGN && prgnCtx[i].bCreated && prgnCtx[i].bUsed
			&& prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.enCoverType == AREA_RECT) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s\n%10d%10d%10d%10d%10X%10d\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				MOD_STRING[prgnCtx[i].stChn.enModId],
				prgnCtx[i].stChn.s32DevId,
				prgnCtx[i].stChn.s32ChnId,
				(prgnCtx[i].stChnAttr.bShow) ? "Y" : "N",
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.stRect.s32X,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.stRect.s32Y,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.stRect.u32Width,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.stRect.u32Height,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.u32Color,
				prgnCtx[i].stChnAttr.unChnAttr.stCoverExChn.u32Layer);
		}
	}

	// Region status of overlayex
	seq_puts(m, "\n------REGION STATUS OF OVERLAYEX------------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%10s%20s%20s%10s%10s\n",
		"Hdl", "Type", "Used", "PiFmt", "W", "H", "BgColor", "Phy", "Virt", "Stride", "CnvsNum");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == OVERLAYEX_RGN && prgnCtx[i].bCreated) {
			memset(c, 0, sizeof(c));
			_pixFmt_to_String(prgnCtx[i].stRegion.unAttr.stOverlayEx.enPixelFormat, c, sizeof(c));

			seq_printf(m, "%7s%3d%10d%10s%20s%10d%10d%10x%20llx%20lx%10d%10d\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				(prgnCtx[i].bUsed) ? "Y" : "N",
				c,
				prgnCtx[i].stRegion.unAttr.stOverlayEx.stSize.u32Width,
				prgnCtx[i].stRegion.unAttr.stOverlayEx.stSize.u32Height,
				prgnCtx[i].stRegion.unAttr.stOverlayEx.u32BgColor,
				prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].u64PhyAddr,
				(uintptr_t)prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].pu8VirtAddr,
				prgnCtx[i].stCanvasInfo[prgnCtx[i].canvas_idx].u32Stride,
				prgnCtx[i].stRegion.unAttr.stOverlayEx.u32CanvasNum);
		}
	}

	// Region chn status of overlayex
	seq_puts(m, "\n------REGION CHN STATUS OF OVERLAYEX--------------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"Hdl", "Type", "Mod", "Dev", "Chn", "bShow", "X", "Y", "Layer");

	for (i = 0; i < RGN_MAX_NUM; ++i) {
		if (prgnCtx[i].stRegion.enType == OVERLAYEX_RGN && prgnCtx[i].bCreated && prgnCtx[i].bUsed) {
			seq_printf(m, "%7s%3d%10d%10s%10d%10d%10s%10d%10d%10d\n",
				"#",
				prgnCtx[i].Handle,
				prgnCtx[i].stRegion.enType,
				MOD_STRING[prgnCtx[i].stChn.enModId],
				prgnCtx[i].stChn.s32DevId,
				prgnCtx[i].stChn.s32ChnId,
				(prgnCtx[i].stChnAttr.bShow) ? "Y" : "N",
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X,
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y,
				prgnCtx[i].stChnAttr.unChnAttr.stOverlayExChn.u32Layer);
		}
	}

	return 0;
}

static int rgn_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rgn_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops rgn_proc_fops = {
	.proc_open = rgn_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations rgn_proc_fops = {
	.owner = THIS_MODULE,
	.open = rgn_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int rgn_proc_init(void)
{
	int ret = CVI_SUCCESS;

	/* create the /proc file */
	if (proc_create_data(RGN_PROC_NAME, 0644, NULL, &rgn_proc_fops, NULL) == NULL) {
		pr_err("rgn proc creation failed\n");
		ret = -EINVAL;
	}

	return ret;
}

int rgn_proc_remove(void)
{
	remove_proc_entry(RGN_PROC_NAME, NULL);

	return CVI_SUCCESS;
}
