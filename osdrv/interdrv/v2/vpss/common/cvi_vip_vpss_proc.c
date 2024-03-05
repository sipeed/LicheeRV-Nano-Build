#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/cvi_vip.h>

#include "vpss_debug.h"
#include "cvi_vip_vpss_proc.h"
#include "vpss_common.h"
#include "scaler.h"
#include "vpss_core.h"

#define VPSS_SHARE_MEM_SIZE     (0x8000)
#define VPSS_PROC_NAME          "cvitek/vpss"

// for proc info
static int proc_vpss_mode;
static const char * const str_src[] = {"ISP", "DWA", "MEM"};
static const char * const str_sclr_flip[] = {"No", "HFLIP", "VFLIP", "HVFLIP"};
static const char * const str_sclr_odma_mode[] = {"CSC", "QUANT", "HSV", "DISABLE"};
static const char * const str_sclr_fmt[] = {"YUV420", "YUV422", "RGB_PLANAR", "RGB_PACKED", "BGR_PACKED", "Y_ONLY"};
static const char * const str_sclr_csc[] = {"Disable", "2RGB_601_Limit", "2RGB_601_Full", "2RGB_709_Limit"
	, "2RGB_709_Full", "2YUV_601_Limit", "2YUV_601_Full", "2YUV_709_Limit", "2YUV_709_Full"};

/*************************************************************************
 *	VPSS proc functions
 *************************************************************************/
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

int vpss_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char c[32];
	struct cvi_vip_dev *bdev = m->private;
	bool isSingleMode = bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding ==
							CVI_VIP_IMG_2_SC_NONE ? true : false;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_shdw_ctx();

	// Module Param
	seq_printf(m, "\nModule: [VPSS], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------\n");
	seq_printf(m, "%25s%25s\n", "vpss_vb_source", "vpss_split_node_num");
	seq_printf(m, "%18d%25d\n", 0, 1);
	seq_puts(m, "\n-------------------------------VPSS MODE----------------------------------\n");
	seq_printf(m, "%25s%15s%15s\n", "vpss_mode", "dev0", "dev1");
	seq_printf(m, "%25s%15s%15s\n", isSingleMode ? "single" : "dual", isSingleMode ? "N" :
		(bdev->img_vdev[CVI_VIP_IMG_D].is_online_from_isp ? "input_isp" : "input_mem"),
		bdev->img_vdev[CVI_VIP_IMG_V].is_online_from_isp ? "input_isp" : "input_mem");

	// VPSS GRP ATTR
	seq_puts(m, "\n-------------------------------VPSS GRP ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%5s\n", "GrpID", "MaxW", "MaxH", "PixFmt",
				"SrcFRate", "DstFRate", "dev");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			memset(c, 0, sizeof(c));
			_pixFmt_to_String(pVpssCtx[i]->stGrpAttr.enPixelFormat, c, sizeof(c));

			seq_printf(m, "%8s%2d%10d%10d%20s%10d%10d%5d\n",
				"#",
				i,
				pVpssCtx[i]->stGrpAttr.u32MaxW,
				pVpssCtx[i]->stGrpAttr.u32MaxH,
				c,
				pVpssCtx[i]->stGrpAttr.stFrameRate.s32SrcFrameRate,
				pVpssCtx[i]->stGrpAttr.stFrameRate.s32DstFrameRate,
				pVpssCtx[i]->stGrpAttr.u8VpssDev);
		}
	}

	//VPSS CHN ATTR
	seq_puts(m, "\n-------------------------------VPSS CHN ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "PhyChnID", "Enable", "MirrorEn", "FlipEn", "SrcFRate", "DstFRate");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"Depth", "Aspect", "videoX", "videoY", "videoW", "videoH", "BgColor");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				int32_t X, Y;
				uint32_t W, H;

				memset(c, 0, sizeof(c));
				if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_NONE)
					strncpy(c, "NONE", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_AUTO)
					strncpy(c, "AUTO", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode
						== ASPECT_RATIO_MANUAL)
					strncpy(c, "MANUAL", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
					X = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32X;
					Y = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32Y;
					W = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Width;
					H = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Height;
				} else {
					X = Y = 0;
					W = H = 0;
				}

				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d\n%10d%10s%10d%10d%10d%10d%#10x\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].isEnabled) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.bMirror) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.bFlip) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stFrameRate.s32SrcFrameRate,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stFrameRate.s32DstFrameRate,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Depth,
					c,
					X,
					Y,
					W,
					H,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.u32BgColor);
			}
		}
	}

	// VPSS GRP CROP INFO
	seq_puts(m, "\n-------------------------------VPSS GRP CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%10d\n",
				"#",
				i,
				(pVpssCtx[i]->stGrpCropInfo.bEnable) ? "Y" : "N",
				(pVpssCtx[i]->stGrpCropInfo.enCropCoordinate == VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
				pVpssCtx[i]->stGrpCropInfo.stCropRect.s32X,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.s32Y,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.u32Width,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.u32Height);
		}
	}

	// VPSS CHN CROP INFO
	seq_puts(m, "\n-------------------------------VPSS CHN CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].stCropInfo.bEnable) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stCropInfo.enCropCoordinate
						== VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.s32X,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.s32Y,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.u32Width,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.u32Height);
			}
		}
	}

	// VPSS GRP WORK STATUS
	seq_puts(m, "\n-------------------------------VPSS GRP WORK STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%20s%20s%20s%20s\n",
		"GrpID", "RecvCnt", "LostCnt", "StartFailCnt", "bStart",
		"CostTime(us)", "MaxCostTime(us)",
		"HwCostTime(us)", "HwMaxCostTime(us)");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			seq_printf(m, "%8s%2d%10d%10d%20d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				pVpssCtx[i]->stGrpWorkStatus.u32RecvCnt,
				pVpssCtx[i]->stGrpWorkStatus.u32LostCnt,
				pVpssCtx[i]->stGrpWorkStatus.u32StartFailCnt,
				(pVpssCtx[i]->isStarted) ? "Y" : "N",
				pVpssCtx[i]->stGrpWorkStatus.u32CostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32MaxCostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32HwCostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32HwMaxCostTime);
		}
	}

	// VPSS CHN OUTPUT RESOLUTION
	seq_puts(m, "\n-------------------------------VPSS CHN OUTPUT RESOLUTION-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s%10s%10s%10s\n",
		"GrpID", "ChnID", "Enable", "Width", "Height", "Pixfmt", "Videofmt", "SendOK", "FrameRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				memset(c, 0, sizeof(c));
				_pixFmt_to_String(pVpssCtx[i]->stChnCfgs[j].stChnAttr.enPixelFormat, c, sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%20s%10s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].isEnabled) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Width,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Height,
					c,
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.enVideoFormat
						== VIDEO_FORMAT_LINEAR) ? "LINEAR" : "UNKNOWN",
					pVpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32SendOk,
					pVpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32RealFrameRate);
			}
		}
	}

	// VPSS CHN ROTATE INFO
	seq_puts(m, "\n-------------------------------VPSS CHN ROTATE INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "GrpID", "ChnID", "Rotate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				memset(c, 0, sizeof(c));
				if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_0)
					strncpy(c, "0", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_90)
					strncpy(c, "90", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_180)
					strncpy(c, "180", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_270)
					strncpy(c, "270", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s\n", "#", i, "#", j, c);
			}
		}
	}

	// VPSS CHN LDC INFO
	seq_puts(m, "\n-------------------------------VPSS CHN LDC INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n", "GrpID", "ChnID", "Enable", "Aspect", "XRatio", "YRatio");
	seq_printf(m, "%10s%10s%10s%20s\n", "XYRatio", "XOffset", "YOffset", "DistortionRatio");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d\n%10d%10d%10d%20d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].stLDCAttr.bEnable) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.bAspect) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32XRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32YRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32XYRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32CenterXOffset,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32CenterYOffset,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32DistortionRatio);
			}
		}
	}

	//VPSS driver status
	seq_puts(m, "\n------------------------------DRV WORK STATUS------------------------------\n");
	seq_printf(m, "%14s%20s%20s%20s%20s%12s\n", "dev", "UserTrigCnt", "UserTrigFailCnt",
			"IspTrigCnt0", "IspTrigFailCnt0", "IrqCnt0");
	seq_printf(m, "%14s%20s%20s%20s%20s%12s\n", "IspTrigCnt1", "IspTrigFailCnt1", "IrqCnt1",
			"IspTrigCnt2", "IspTrigFailCnt2", "IrqCnt2");
	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		seq_printf(m, "%12s%2d%20d%20d%20d%20d%12d\n%14d%20d%20d%20d%20d%12d\n",
			"#",
			i,
			bdev->img_vdev[i].user_trig_cnt,
			bdev->img_vdev[i].user_trig_fail_cnt,
			bdev->img_vdev[i].isp_trig_cnt[0],
			bdev->img_vdev[i].isp_trig_fail_cnt[0],
			bdev->img_vdev[i].irq_cnt[0],
			bdev->img_vdev[i].isp_trig_cnt[1],
			bdev->img_vdev[i].isp_trig_fail_cnt[1],
			bdev->img_vdev[i].irq_cnt[1],
			bdev->img_vdev[i].isp_trig_cnt[2],
			bdev->img_vdev[i].isp_trig_fail_cnt[2],
			bdev->img_vdev[i].irq_cnt[2]);
	}

	// VPSS Slice buffer status
	seq_puts(m, "\n-------------------------------VPSS CHN BUF WRAP ATTR---------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s\n", "GrpID", "ChnID", "Enable", "BufLine", "WrapBufSize");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < pVpssCtx[i]->chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					pVpssCtx[i]->stChnCfgs[j].stBufWrap.bEnable ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stBufWrap.u32BufLine,
					pVpssCtx[i]->stChnCfgs[j].stBufWrap.u32WrapBufferSize);
			}
		}
	}

	return 0;
}

/*************************************************************************
 *	Proc functions
 *************************************************************************/
static void _show_mem(struct seq_file *m, struct sclr_mem *mem)
{
	seq_printf(m, "start_x(%3d)\t\tstart_y(%3d)\t\twidth(%4d)\t\theight(%4d)\n"
		  , mem->start_x, mem->start_y, mem->width, mem->height);
	seq_printf(m, "pitch_y(%3d)\t\tpitch_c(%3d)\n", mem->pitch_y, mem->pitch_c);
}

static void _show_img_status(struct seq_file *m, u8 i)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(i);
	union sclr_img_dbg_status status = sclr_img_get_dbg_status(i, true);

	seq_printf(m, "--------------IMG_IN%d STATUS-----------------\n", i);
	seq_printf(m, "src(%s)\t\tcsc(%15s)\tfmt(%s)\t\tburst(%d)\n", str_src[cfg->src], str_sclr_csc[cfg->csc]
		  , str_sclr_fmt[cfg->fmt], cfg->burst);
	_show_mem(m, &cfg->mem);
	seq_printf(m, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
		  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
		  , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
		  , status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
	seq_printf(m, "ip idle(%d)\t\tip int(%d)\n", status.b.ip_idle, status.b.ip_int);
}

static void _show_sc_status(struct seq_file *m, u8 i)
{
	struct sclr_core_cfg *cfg = sclr_get_cfg(i);
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(i);
	union sclr_odma_dbg_status status = sclr_odma_get_dbg_status(i);

	seq_printf(m, "--------------SC%d STATUS---------------------\n", i);
	seq_printf(m, "sc bypass(%d)\t\tgop bypass(%d)\t\tcir bypass(%d)\t\todma bypass(%d)\n"
		  , cfg->sc_bypass, cfg->gop_bypass, cfg->cir_bypass, cfg->odma_bypass);
	seq_printf(m, "src-size(%4d*%4d)\tcrop offset(%4d*%4d)\tcrop size(%4d*%4d)\toutput size(%4d*%4d)\n"
		  , cfg->sc.src.w, cfg->sc.src.h, cfg->sc.crop.x, cfg->sc.crop.y, cfg->sc.crop.w, cfg->sc.crop.h
		  , cfg->sc.dst.w, cfg->sc.dst.h);
	seq_printf(m, "flip(%s)\t\tfmt(%s)\t\tburst(%d)\n", str_sclr_flip[odma_cfg->flip]
		  , str_sclr_fmt[odma_cfg->fmt], odma_cfg->burst);
	seq_printf(m, "mode(%s)\t\tcsc(%15s)\t\n", str_sclr_odma_mode[odma_cfg->csc_cfg.mode]
		  , str_sclr_csc[odma_cfg->csc_cfg.csc_type]);
	_show_mem(m, &odma_cfg->mem);
	seq_printf(m, "full_yuv(%d%d%d)\t\tempty_yuv(%d%d%d)\t\taxi_active_yuv(%d%d%d)\taxi_active(%d)\n"
		  , status.b.y_buf_full, status.b.u_buf_full, status.b.v_buf_full
		  , status.b.y_buf_empty, status.b.u_buf_empty, status.b.v_buf_empty
		  , status.b.y_axi_active, status.b.u_axi_active, status.b.v_axi_active, status.b.axi_active);
}

static int vpss_proc_show(struct seq_file *m, void *v)
{
	int i;

	// show driver status if vpss_mode == 1
	if (proc_vpss_mode) {
		for (i = 0; i < CVI_VIP_IMG_MAX; ++i)
			_show_img_status(m, i);
		for (i = 0; i < CVI_VIP_SC_MAX; ++i)
			_show_sc_status(m, i);
		return 0;
	}
	return vpss_ctx_proc_show(m, v);
}

static ssize_t vpss_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_vpss_mode))
		proc_vpss_mode = 0;

	return count;
}

static int vpss_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpss_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vpss_proc_fops = {
	.proc_open = vpss_proc_open,
	.proc_read = seq_read,
	.proc_write = vpss_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vpss_proc_fops = {
	.owner = THIS_MODULE,
	.open = vpss_proc_open,
	.read = seq_read,
	.write = vpss_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vpss_proc_init(struct cvi_vip_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(VPSS_PROC_NAME, 0644, NULL,
				 &vpss_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int vpss_proc_remove(struct cvi_vip_dev *dev)
{
	remove_proc_entry(VPSS_PROC_NAME, NULL);
	return 0;
}
