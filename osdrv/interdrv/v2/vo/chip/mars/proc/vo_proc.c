#include <proc/vo_proc.h>
#include <linux/version.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_vo_ctx.h>

#define VO_PRC_NAME	"cvitek/vo"

static void *vo_shared_mem;
/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void _intfType_to_String(uint32_t intfType, char *str, int len)
{
	switch (intfType) {
	case VO_INTF_CVBS:
		strncpy(str, "CVBS", len);
		break;
	case VO_INTF_YPBPR:
		strncpy(str, "YPBPR", len);
		break;
	case VO_INTF_VGA:
		strncpy(str, "VGA", len);
		break;
	case VO_INTF_BT656:
		strncpy(str, "BT656", len);
		break;
	case VO_INTF_BT1120:
		strncpy(str, "BT1120", len);
		break;
	case VO_INTF_LCD:
		strncpy(str, "LCD", len);
		break;
	case VO_INTF_LCD_18BIT:
		strncpy(str, "LCD_18BIT", len);
		break;
	case VO_INTF_LCD_24BIT:
		strncpy(str, "LCD_24BIT", len);
		break;
	case VO_INTF_LCD_30BIT:
		strncpy(str, "LCD_30BIT", len);
		break;
	case VO_INTF_MIPI:
		strncpy(str, "MIPI", len);
		break;
	case VO_INTF_MIPI_SLAVE:
		strncpy(str, "MIPI_SLAVE", len);
		break;
	case VO_INTF_HDMI:
		strncpy(str, "HDMI", len);
		break;
	case VO_INTF_I80:
		strncpy(str, "I80", len);
		break;
	case VO_INTF_HW_MCU:
		strncpy(str, "HW_MCU", len);
		break;
	default:
		strncpy(str, "Unknown Type", len);
		break;
	}
}

static void _intfSync_to_String(enum _VO_INTF_SYNC_E intfSync, char *str, int len)
{
	switch (intfSync) {
	case VO_OUTPUT_PAL:
		strncpy(str, "PAL", len);
		break;
	case VO_OUTPUT_NTSC:
		strncpy(str, "NTSC", len);
		break;
	case VO_OUTPUT_1080P24:
		strncpy(str, "1080P@24", len);
		break;
	case VO_OUTPUT_1080P25:
		strncpy(str, "1080P@25", len);
		break;
	case VO_OUTPUT_1080P30:
		strncpy(str, "1080P@30", len);
		break;
	case VO_OUTPUT_720P50:
		strncpy(str, "720P@50", len);
		break;
	case VO_OUTPUT_720P60:
		strncpy(str, "720P@60", len);
		break;
	case VO_OUTPUT_1080P50:
		strncpy(str, "1080P@50", len);
		break;
	case VO_OUTPUT_1080P60:
		strncpy(str, "1080P@60", len);
		break;
	case VO_OUTPUT_576P50:
		strncpy(str, "576P@50", len);
		break;
	case VO_OUTPUT_480P60:
		strncpy(str, "480P@60", len);
		break;
	case VO_OUTPUT_800x600_60:
		strncpy(str, "800x600@60", len);
		break;
	case VO_OUTPUT_1024x768_60:
		strncpy(str, "1024x768@60", len);
		break;
	case VO_OUTPUT_1280x1024_60:
		strncpy(str, "1280x1024@60", len);
		break;
	case VO_OUTPUT_1366x768_60:
		strncpy(str, "1366x768@60", len);
		break;
	case VO_OUTPUT_1440x900_60:
		strncpy(str, "1440x900@60", len);
		break;
	case VO_OUTPUT_1280x800_60:
		strncpy(str, "1280x800@60", len);
		break;
	case VO_OUTPUT_1600x1200_60:
		strncpy(str, "1600x1200@60", len);
		break;
	case VO_OUTPUT_1680x1050_60:
		strncpy(str, "1680x1050@60", len);
		break;
	case VO_OUTPUT_1920x1200_60:
		strncpy(str, "1920x1200@60", len);
		break;
	case VO_OUTPUT_640x480_60:
		strncpy(str, "640x480@60", len);
		break;
	case VO_OUTPUT_720x1280_60:
		strncpy(str, "720x1280@60", len);
		break;
	case VO_OUTPUT_1080x1920_60:
		strncpy(str, "1080x1920@60", len);
		break;
	case VO_OUTPUT_480x800_60:
		strncpy(str, "480x800@60", len);
		break;
	case VO_OUTPUT_USER:
		strncpy(str, "User timing", len);
		break;
	default:
		strncpy(str, "Unknown Timing", len);
		break;
	}
}

static void _pixFmt_to_String(enum _PIXEL_FORMAT_E PixFmt, char *str, int len)
{
	switch (PixFmt) {
	case PIXEL_FORMAT_RGB_888:
		strncpy(str, "RGB_888", len);
		break;
	case PIXEL_FORMAT_BGR_888:
		strncpy(str, "BGR_888", len);
		break;
	case PIXEL_FORMAT_RGB_888_PLANAR:
		strncpy(str, "RGB_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_BGR_888_PLANAR:
		strncpy(str, "BGR_888_PLANAR", len);
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
	case PIXEL_FORMAT_RGB_BAYER_8BPP:
		strncpy(str, "RGB_BAYER_8BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_10BPP:
		strncpy(str, "RGB_BAYER_10BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_12BPP:
		strncpy(str, "RGB_BAYER_12BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_14BPP:
		strncpy(str, "RGB_BAYER_14BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_16BPP:
		strncpy(str, "RGB_BAYER_16BPP", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strncpy(str, "YUV_PLANAR_422", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strncpy(str, "YUV_PLANAR_420", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strncpy(str, "YUV_PLANAR_444", len);
		break;
	case PIXEL_FORMAT_YUV_400:
		strncpy(str, "YUV_400", len);
		break;
	case PIXEL_FORMAT_HSV_888:
		strncpy(str, "HSV_888", len);
		break;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		strncpy(str, "HSV_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_NV12:
		strncpy(str, "NV12", len);
		break;
	case PIXEL_FORMAT_NV21:
		strncpy(str, "NV21", len);
		break;
	case PIXEL_FORMAT_NV16:
		strncpy(str, "NV16", len);
		break;
	case PIXEL_FORMAT_NV61:
		strncpy(str, "NV61", len);
		break;
	case PIXEL_FORMAT_YUYV:
		strncpy(str, "YUYV", len);
		break;
	case PIXEL_FORMAT_UYVY:
		strncpy(str, "UYVY", len);
		break;
	case PIXEL_FORMAT_YVYU:
		strncpy(str, "YVYU", len);
		break;
	case PIXEL_FORMAT_VYUY:
		strncpy(str, "VYUY", len);
		break;
	case PIXEL_FORMAT_FP32_C1:
		strncpy(str, "FP32_C1", len);
		break;
	case PIXEL_FORMAT_FP32_C3_PLANAR:
		strncpy(str, "FP32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT32_C1:
		strncpy(str, "INT32_C1", len);
		break;
	case PIXEL_FORMAT_INT32_C3_PLANAR:
		strncpy(str, "INT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT32_C1:
		strncpy(str, "UINT32_C1", len);
		break;
	case PIXEL_FORMAT_UINT32_C3_PLANAR:
		strncpy(str, "UINT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_BF16_C1:
		strncpy(str, "BF16_C1", len);
		break;
	case PIXEL_FORMAT_BF16_C3_PLANAR:
		strncpy(str, "BF16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT16_C1:
		strncpy(str, "INT16_C1", len);
		break;
	case PIXEL_FORMAT_INT16_C3_PLANAR:
		strncpy(str, "INT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT16_C1:
		strncpy(str, "UINT16_C1", len);
		break;
	case PIXEL_FORMAT_UINT16_C3_PLANAR:
		strncpy(str, "UINT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT8_C1:
		strncpy(str, "INT8_C1", len);
		break;
	case PIXEL_FORMAT_INT8_C3_PLANAR:
		strncpy(str, "INT8_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT8_C1:
		strncpy(str, "UINT8_C1", len);
		break;
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		strncpy(str, "UINT8_C3_PLANAR", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static int _vo_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vo_ctx *pvoCtx = NULL;
	int i, j, cnt;
	char c[32], d[32];

	pvoCtx = (struct cvi_vo_ctx *)(vo_shared_mem);
	if (!pvoCtx) {
		seq_puts(m, "vo shm = NULL\n");
		return -1;
	}
#if 0//TODO: UTS_VERSION
	seq_printf(m, "\nModule: [VO], Build Time[%s]\n", UTS_VERSION);
#endif
	// Device Config
	seq_puts(m, "\n-------------------------------DEVICE CONFIG------------------------------\n");
	seq_printf(m, "%10s%10s%20s%20s%10s%10s\n", "DevID", "DevEn", "IntfType", "IntfSync", "BkClr", "DevFrt");
	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_intfType_to_String(pvoCtx->stPubAttr.enIntfType, c, sizeof(c));

		memset(d, 0, sizeof(d));
		_intfSync_to_String(pvoCtx->stPubAttr.enIntfSync, d, sizeof(d));
		seq_printf(m, "%8s%2d%10s%20s%20s%10X%10d\n",
				"#",
				i,
				(pvoCtx->is_dev_enable[i]) ? "Y" : "N",
				c,
				d,
				pvoCtx->stPubAttr.u32BgColor,
				pvoCtx->stPubAttr.stSyncInfo.u16FrameRate);
	}

	// video layer status 1
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%20s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "VideoEn", "PixFmt", "ImgW", "ImgH", "DispX", "DispY", "DispW", "DispH", "DispFrt");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_pixFmt_to_String(pvoCtx->stLayerAttr.enPixFormat, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%20s%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				(pvoCtx->is_layer_enable[i]) ? "Y" : "N",
				c,
				pvoCtx->stLayerAttr.stImageSize.u32Width,
				pvoCtx->stLayerAttr.stImageSize.u32Height,
				pvoCtx->stLayerAttr.stDispRect.s32X,
				pvoCtx->stLayerAttr.stDispRect.s32Y,
				pvoCtx->stLayerAttr.stDispRect.u32Width,
				pvoCtx->stLayerAttr.stDispRect.u32Height,
				pvoCtx->stLayerAttr.u32DispFrmRt);
	}

	// video layer status 2
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 2 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "DevId", "EnChNum", "Luma", "Cont", "Hue", "Satu", "BufLen");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (pvoCtx->is_chn_enable[i][j])
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				0,
				cnt,
				pvoCtx->proc_amp[PROC_AMP_BRIGHTNESS],
				pvoCtx->proc_amp[PROC_AMP_CONTRAST],
				pvoCtx->proc_amp[PROC_AMP_HUE],
				pvoCtx->proc_amp[PROC_AMP_SATURATION],
				pvoCtx->u32DisBufLen);
	}

	// chn basic info
	seq_puts(m, "\n-------------------------------CHN BASIC INFO-----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ChnEn", "Prio", "ChnX", "ChnY", "ChnW", "ChnH", "RotAngle");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			memset(c, 0, sizeof(c));
			if (pvoCtx->enRotation == ROTATION_0)
				strncpy(c, "0", sizeof(c));
			else if (pvoCtx->enRotation == ROTATION_90)
				strncpy(c, "90", sizeof(c));
			else if (pvoCtx->enRotation == ROTATION_180)
				strncpy(c, "180", sizeof(c));
			else if (pvoCtx->enRotation == ROTATION_270)
				strncpy(c, "270", sizeof(c));
			else
				strncpy(c, "Invalid", sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10s\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->is_chn_enable[j]) ? "Y" : "N",
				pvoCtx->stChnAttr.u32Priority,
				pvoCtx->stChnAttr.stRect.s32X,
				pvoCtx->stChnAttr.stRect.s32Y,
				pvoCtx->stChnAttr.stRect.u32Width,
				pvoCtx->stChnAttr.stRect.u32Height,
				c);
		}
	}

	// chn play info
	seq_puts(m, "\n-------------------------------CHN PLAY INFO------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%20s%20s%20s\n",
		"LayerId", "ChnId", "Show", "Pause", "Thrshd", "ChnFrt", "ChnGap(us)", "DispPts", "PreDonePts");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {

			seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%20d%20lld%20lld\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->show) ? "Y" : "N",
				(pvoCtx->pause) ? "Y" : "N",
				pvoCtx->u32DisBufLen,
				pvoCtx->chnStatus[i][j].u32RealFrameRate,
				(pvoCtx->chnStatus[i][j].u32RealFrameRate == 0) ?
					0 : (1000000/pvoCtx->chnStatus[i][j].u32RealFrameRate),
				pvoCtx->u64DisplayPts[i][j],
				pvoCtx->u64PreDonePts[i][j]);
		}
	}

	return 0;
}

static int _vo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vo_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vo_proc_fops = {
	.proc_open = _vo_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vo_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vo_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


int vo_proc_init(struct cvi_vo_dev *_vdev, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VO_PRC_NAME, 0644, NULL, &_vo_proc_fops, _vdev) == NULL) {
		pr_err("vo proc creation failed\n");
		rc = -1;
	}

	vo_shared_mem = shm;
	return rc;
}

int vo_proc_remove(void)
{
	remove_proc_entry(VO_PRC_NAME, NULL);

	return 0;
}
