#include <proc/vi_proc.h>
#include <linux/version.h>
#include <linux/cvi_vi_ctx.h>

#define VI_PRC_NAME	"cvitek/vi"

static void *vi_shared_mem;
/*************************************************************************
 *	VI proc functions
 *************************************************************************/

static int _vi_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vi_dev *vdev = m->private;
	struct cvi_vi_ctx *pviProcCtx = NULL;
	u8 i = 0, j = 0, chn = 0;
	char o[8], p[8];
	u8 isRGB = 0;

	pviProcCtx = (struct cvi_vi_ctx *)(vi_shared_mem);

	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------------\n");
	seq_puts(m, "\tDetectErrFrame\tDropErrFrame\n");
	seq_printf(m, "\t\t%d\t\t%d\n", pviProcCtx->modParam.s32DetectErrFrame, pviProcCtx->modParam.u32DropErrFrame);

	seq_puts(m, "\n-------------------------------VI MODE------------------------------------------\n");
	seq_puts(m, "\tDevID\tPrerawFE\tPrerawBE\tPostraw\t\tScaler\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			seq_printf(m, "\t%3d\t%7s\t\t%7s\t\t%7s\t\t%7s\n", i,
				vdev->ctx.isp_pipe_cfg[i].is_offline_preraw ? "offline" : "online",
				vdev->ctx.is_offline_be ? "offline" : "online",
				vdev->ctx.is_offline_postraw ?
				(vdev->ctx.is_slice_buf_on ? "slice" : "offline") : "online",
				vdev->ctx.isp_pipe_cfg[i].is_offline_scaler ? "offline" : "online");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR1-------------------------------------\n");
	seq_puts(m, "\tDevID\tDevEn\tBindPipe\tWidth\tHeight\tIntfM\tWkM\tScanM\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			seq_printf(m, "\t%3d\t%3s\t%4s\t\t%4d\t%4d", i,
				(pviProcCtx->isDevEnable[i] ? "Y" : "N"), "Y",
				pviProcCtx->devAttr[i].stSize.u32Width,
				pviProcCtx->devAttr[i].stSize.u32Height);

			memset(o, 0, 8);
			if (pviProcCtx->devAttr[i].enIntfMode == VI_MODE_BT656 ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_BT601 ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_BT1120_STANDARD ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_BT1120_INTERLEAVED)
				memcpy(o, "BT", sizeof(o));
			else if (pviProcCtx->devAttr[i].enIntfMode == VI_MODE_MIPI ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_MIPI_YUV420_NORMAL ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_MIPI_YUV420_LEGACY ||
				pviProcCtx->devAttr[i].enIntfMode == VI_MODE_MIPI_YUV422)
				memcpy(o, "MIPI", sizeof(o));
			else if (pviProcCtx->devAttr[i].enIntfMode == VI_MODE_LVDS)
				memcpy(o, "LVDS", sizeof(o));

			memset(p, 0, 8);
			if (pviProcCtx->devAttr[i].enWorkMode == VI_WORK_MODE_1Multiplex)
				memcpy(p, "1MUX", sizeof(p));
			else if (pviProcCtx->devAttr[i].enWorkMode == VI_WORK_MODE_2Multiplex)
				memcpy(p, "2MUX", sizeof(p));
			else if (pviProcCtx->devAttr[i].enWorkMode == VI_WORK_MODE_3Multiplex)
				memcpy(p, "3MUX", sizeof(p));
			else if (pviProcCtx->devAttr[i].enWorkMode == VI_WORK_MODE_4Multiplex)
				memcpy(p, "4MUX", sizeof(p));
			else
				memcpy(p, "Other", sizeof(p));

			seq_printf(m, "\t%4s\t%4s\t%3s\n", o, p,
				(pviProcCtx->devAttr[i].enScanMode == VI_SCAN_INTERLACED) ? "I" : "P");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR2-------------------------------------\n");
	seq_puts(m, "\tDevID\tAD0\tAD1\tAD2\tAD3\tSeq\tDataType\tWDRMode\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			memset(o, 0, 8);
			if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_VUVU)
				memcpy(o, "VUVU", sizeof(o));
			else if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_UVUV)
				memcpy(o, "UVUV", sizeof(o));
			else if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_UYVY)
				memcpy(o, "UYVY", sizeof(o));
			else if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_VYUY)
				memcpy(o, "VYUY", sizeof(o));
			else if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_YUYV)
				memcpy(o, "YUYV", sizeof(o));
			else if (pviProcCtx->devAttr[i].enDataSeq == VI_DATA_SEQ_YVYU)
				memcpy(o, "YVYU", sizeof(o));

			isRGB = (pviProcCtx->devAttr[i].enInputDataType == VI_DATA_TYPE_RGB);

			seq_printf(m, "\t%3d\t%1d\t%1d\t%1d\t%1d\t%3s\t%4s\t\t%3s\n", i,
				pviProcCtx->devAttr[i].as32AdChnId[0],
				pviProcCtx->devAttr[i].as32AdChnId[1],
				pviProcCtx->devAttr[i].as32AdChnId[2],
				pviProcCtx->devAttr[i].as32AdChnId[3],
				(isRGB) ? "N/A" : o, (isRGB) ? "RGB" : "YUV",
				(vdev->ctx.isp_pipe_cfg[i].is_hdr_on) ? "WDR_2F1" : "None");
		}
	}

	seq_puts(m, "\n-------------------------------VI BIND ATTR-------------------------------------\n");
	seq_puts(m, "\tDevID\tPipeNum\t\tPipeId\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			seq_printf(m, "\t%3d\t%3d\t\t%3d\n", i,
				pviProcCtx->devBindPipeAttr[i].u32Num,
				pviProcCtx->devBindPipeAttr[i].PipeId[pviProcCtx->devBindPipeAttr[i].u32Num]);
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV TIMING ATTR-------------------------------\n");
	seq_puts(m, "\tDevID\tDevTimingEn\tDevFrmRate\tDevWidth\tDevHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			seq_printf(m, "\t%3d\t%5s\t\t%4d\t\t%5d\t\t%5d\n", i,
				(pviProcCtx->stTimingAttr[i].bEnable) ? "Y" : "N",
				pviProcCtx->stTimingAttr[i].s32FrmRate,
				pviProcCtx->devAttr[i].stSize.u32Width,
				pviProcCtx->devAttr[i].stSize.u32Height);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN ATTR1-------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tWidth\tHeight\tMirror\tFlip\tSrcFRate\tDstFRate\tPixFmt\tVideoFmt\tBindPool\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422)
					memcpy(o, "422P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420)
					memcpy(o, "420P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444)
					memcpy(o, "444P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_NV12)
					memcpy(o, "NV12", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_NV21)
					memcpy(o, "NV21", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%4d\t%4d\t%3s\t%2s\t%4d\t\t%4d\t\t%3s\t%6s\t\t%4d\n", i, j,
					pviProcCtx->chnAttr[chn].stSize.u32Width,
					pviProcCtx->chnAttr[chn].stSize.u32Height,
					(pviProcCtx->chnAttr[chn].bMirror) ? "Y" : "N",
					(pviProcCtx->chnAttr[chn].bFlip) ? "Y" : "N",
					pviProcCtx->chnAttr[chn].stFrameRate.s32SrcFrameRate,
					pviProcCtx->chnAttr[chn].stFrameRate.s32DstFrameRate,
					o, "SDR8", pviProcCtx->chnAttr[chn].u32BindVbPool);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN ATT2--------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tCompressMode\tDepth\tAlign\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;
				memset(o, 0, 8);
				if (pviProcCtx->chnAttr[chn].enCompressMode == COMPRESS_MODE_NONE)
					memcpy(o, "None", sizeof(o));
				else
					memcpy(o, "Y", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%4s\t\t%3d\t%3d\n", i, j,
					o, pviProcCtx->chnAttr[chn].u32Depth, 32);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN OUTPUT RESOLUTION-------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tMirror\tFlip\tWidth\tHeight\tPixFmt\tVideoFmt\tCompressMode\tFrameRate\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422)
					memcpy(o, "422P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420)
					memcpy(o, "420P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_444)
					memcpy(o, "444P", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_NV12)
					memcpy(o, "NV12", sizeof(o));
				else if (pviProcCtx->chnAttr[chn].enPixelFormat == PIXEL_FORMAT_NV21)
					memcpy(o, "NV21", sizeof(o));

				memset(p, 0, 8);
				if (pviProcCtx->chnAttr[chn].enCompressMode == COMPRESS_MODE_NONE)
					memcpy(p, "None", sizeof(p));
				else
					memcpy(p, "Y", sizeof(p));

				seq_printf(m, "\t%3d\t%3d\t%3s\t%2s\t%4d\t%4d\t%3s\t%6s\t\t%6s\t\t%5d\n", i, j,
					(pviProcCtx->chnAttr[chn].bMirror) ? "Y" : "N",
					(pviProcCtx->chnAttr[chn].bFlip) ? "Y" : "N",
					pviProcCtx->chnAttr[chn].stSize.u32Width,
					pviProcCtx->chnAttr[chn].stSize.u32Height,
					o, "SDR8", p,
					pviProcCtx->chnAttr[chn].stFrameRate.s32DstFrameRate);
			}
		}
	}


	seq_puts(m, "\n-------------------------------VI CHN ROTATE INFO-------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tRotate\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (pviProcCtx->enRotation[chn] == ROTATION_0)
					memcpy(o, "0", sizeof(o));
				else if (pviProcCtx->enRotation[chn] == ROTATION_90)
					memcpy(o, "90", sizeof(o));
				else if (pviProcCtx->enRotation[chn] == ROTATION_180)
					memcpy(o, "180", sizeof(o));
				else if (pviProcCtx->enRotation[chn] == ROTATION_270)
					memcpy(o, "270", sizeof(o));
				else
					memcpy(o, "Invalid", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%3s\n", i, j, o);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN EARLY INTERRUPT INFO----------------------\n");
	seq_puts(m, "\tDevID\tChnID\tEnable\tLineCnt\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				seq_printf(m, "\t%3d\t%3d\t%3s\t%4d\n", i, j,
					pviProcCtx->enEalyInt[chn].bEnable ? "Y" : "N",
					pviProcCtx->enEalyInt[chn].u32LineCnt);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN CROP INFO---------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tCropEn\tCoorType\tCoorX\tCoorY\tWidth\tHeight\tTrimX\tTrimY\tTrimWid\tTrimHgt\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (pviProcCtx->chnCrop[chn].enCropCoordinate == VI_CROP_RATIO_COOR)
					memcpy(o, "RAT", sizeof(o));
				else
					memcpy(o, "ABS", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%3s\t%5s\t\t%4d\t%4d\t%4d\t%4d\t%4d\t%3d\t%3d\t%4d\n", i, j,
					pviProcCtx->chnCrop[chn].bEnable ? "Y" : "N", o,
					pviProcCtx->chnCrop[chn].stCropRect.s32X,
					pviProcCtx->chnCrop[chn].stCropRect.s32Y,
					pviProcCtx->chnCrop[chn].stCropRect.u32Width,
					pviProcCtx->chnCrop[chn].stCropRect.u32Height,
					pviProcCtx->chnCrop[chn].stCropRect.s32X,
					pviProcCtx->chnCrop[chn].stCropRect.s32Y,
					pviProcCtx->chnCrop[chn].stCropRect.u32Width,
					pviProcCtx->chnCrop[chn].stCropRect.u32Height);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN STATUS------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tEnable\tFrameRate\tIntCnt\tRecvPic\tLostFrame\tVbFail\tWidth\tHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (pviProcCtx->isDevEnable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += pviProcCtx->devAttr[j].chn_num;
			}

			for (j = 0; j < pviProcCtx->devAttr[i].chn_num; j++, chn++) {
				if (chn >= pviProcCtx->total_chn_num)
					break;

				seq_printf(m, "\t%3d\t%3d\t%3s\t%5d\t\t%5d\t%5d\t%5d\t\t%5d\t%4d\t%4d\n", i, j,
					pviProcCtx->chnStatus[chn].bEnable ? "Y" : "N",
					pviProcCtx->chnStatus[chn].u32FrameRate,
					pviProcCtx->chnStatus[chn].u32IntCnt,
					pviProcCtx->chnStatus[chn].u32RecvPic,
					pviProcCtx->chnStatus[chn].u32LostFrame,
					pviProcCtx->chnStatus[chn].u32VbFail,
					pviProcCtx->chnStatus[chn].stSize.u32Width,
					pviProcCtx->chnStatus[chn].stSize.u32Height);
			}
		}
	}

	return 0;
}

static int _vi_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vi_proc_show, PDE_DATA(inode));
}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops _vi_proc_fops = {
	.proc_open = _vi_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vi_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vi_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_proc_init(struct cvi_vi_dev *_vdev, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_PRC_NAME, 0644, NULL, &_vi_proc_fops, _vdev) == NULL) {
		pr_err("vi proc creation failed\n");
		rc = -1;
	}

	vi_shared_mem = shm;
	return rc;
}

int vi_proc_remove(void)
{
	remove_proc_entry(VI_PRC_NAME, NULL);

	return 0;
}
