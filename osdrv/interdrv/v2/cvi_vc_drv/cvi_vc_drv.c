#include <linux/cvi_base_ctx.h>
#include <linux/slab.h>
#include <base_cb.h>
#include <base_ctx.h>

#include "cvi_vc_drv.h"
#include "cvi_vc_drv_proc.h"
#include "vcodec_cb.h"
#include "vdi_osal.h"

extern int VPU_IsBusy(unsigned int coreIdx);

static const struct of_device_id cvi_vc_drv_match_table[] = {
	{ .compatible = "cvitek,cvi_vc_drv" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_vc_drv_match_table);

extern wait_queue_head_t tWaitQueue[];

uint32_t MaxVencChnNum = VENC_MAX_CHN_NUM;
module_param(MaxVencChnNum, uint, 0644);
#ifdef ENABLE_DEC
uint32_t MaxVdecChnNum = VDEC_MAX_CHN_NUM;
module_param(MaxVdecChnNum, uint, 0644);
#endif
bool cviRcEn = 1;
module_param(cviRcEn, bool, 0644);

static struct semaphore vencSemArry[VENC_MAX_CHN_NUM];
#ifdef ENABLE_DEC
static struct semaphore vdecSemArry[VDEC_MAX_CHN_NUM];
#endif
struct cvi_vc_drv_device *pCviVcDrvDevice;

struct clk_ctrl_info {
	int core_idx;
	int enable;
};
#ifdef VC_DRIVER_TEST
extern int h265_dec_test(u_long arg);
extern int h264_dec_test(u_long arg);
extern int cvi_venc_test(u_long arg);
extern int cvi_jpeg_test(u_long arg);
#endif
static int cvi_vc_drv_open(struct inode *inode, struct file *filp);
static long cvi_vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg);
#ifdef ENABLE_DEC
static long cvi_vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg);
#endif
static int cvi_vc_drv_release(struct inode *inode, struct file *filp);
static unsigned int cvi_vc_drv_poll(struct file *filp,
				    struct poll_table_struct *wait);

extern unsigned long vpu_get_interrupt_reason(int coreIdx);
extern unsigned long jpu_get_interrupt_flag(int chnIdx);

const struct file_operations cvi_vc_drv_venc_fops = {
	.owner = THIS_MODULE,
	.open = cvi_vc_drv_open,
	.unlocked_ioctl = cvi_vc_drv_venc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cvi_vc_drv_venc_ioctl,
#endif
	.release = cvi_vc_drv_release,
	.poll = cvi_vc_drv_poll,
};
#ifdef ENABLE_DEC
const struct file_operations cvi_vc_drv_vdec_fops = {
	.owner = THIS_MODULE,
	.open = cvi_vc_drv_open,
	.unlocked_ioctl = cvi_vc_drv_vdec_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cvi_vc_drv_vdec_ioctl,
#endif
	.release = cvi_vc_drv_release,
};
#endif
static int cvi_vc_drv_open(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);

	pr_info("open channel no. %u\n", minor);

	return 0;
}

static long cvi_vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	unsigned int minor = iminor(file_inode(filp));
	CVI_S32 s32Ret = CVI_FAILURE;

	if (down_interruptible(&vencSemArry[minor])) {
		return s32Ret;
	}

	switch (cmd) {
	case CVI_VC_VENC_CREATE_CHN: {
		VENC_CHN_ATTR_S stChnAttr;

		if (copy_from_user(&stChnAttr, (VENC_CHN_ATTR_S *)arg,
				   sizeof(VENC_CHN_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_CreateChn(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_CreateChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_DESTROY_CHN: {
		s32Ret = CVI_VENC_DestroyChn(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_DestroyChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_RESET_CHN: {
		s32Ret = CVI_VENC_ResetChn(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_ResetChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_START_RECV_FRAME: {
		VENC_RECV_PIC_PARAM_S stRecvParam;

		if (copy_from_user(&stRecvParam, (VENC_RECV_PIC_PARAM_S *)arg,
				   sizeof(VENC_RECV_PIC_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_StartRecvFrame(minor, &stRecvParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_StartRecvFrame with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_STOP_RECV_FRAME: {
		s32Ret = CVI_VENC_StopRecvFrame(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_StopRecvFrame with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_QUERY_STATUS: {
		VENC_CHN_STATUS_S stStatus;

		if (copy_from_user(&stStatus, (VENC_CHN_STATUS_S *)arg,
				   sizeof(VENC_CHN_STATUS_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_QueryStatus(minor, &stStatus);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_QueryStatus with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_CHN_STATUS_S *)arg, &stStatus,
				 sizeof(VENC_CHN_STATUS_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_CHN_ATTR: {
		VENC_CHN_ATTR_S stChnAttr;

		if (copy_from_user(&stChnAttr, (VENC_CHN_ATTR_S *)arg,
				   sizeof(VENC_CHN_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetChnAttr(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetChnAttr with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_CHN_ATTR: {
		VENC_CHN_ATTR_S stChnAttr;

		s32Ret = CVI_VENC_GetChnAttr(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetChnAttr with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_CHN_ATTR_S *)arg, &stChnAttr,
				 sizeof(VENC_CHN_ATTR_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_GET_STREAM: {
		VENC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		VENC_STREAM_S stStream;
		VENC_PACK_S *pUserPack; // keep user space pointer on packs
#endif

		if (copy_from_user(&stStreamEx, (VENC_STREAM_EX_S *)arg,
				   sizeof(VENC_STREAM_EX_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_from_user(&stStream, stStreamEx.pstStream,
				   sizeof(VENC_STREAM_S)) != 0) {
			break;
		}

		// stStream.pstPack will be replaced by kernel space packs
		// in CVI_VENC_GetStream
		pUserPack = stStream.pstPack;
		stStream.pstPack = NULL;
		s32Ret = CVI_VENC_GetStream(minor, &stStream,
					    stStreamEx.s32MilliSec);
#else
		s32Ret = CVI_VENC_GetStream(minor, stStreamEx.pstStream,
					    stStreamEx.s32MilliSec);
#endif

		if (s32Ret != CVI_SUCCESS) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
			if (stStream.pstPack) {
				vfree(stStream.pstPack);
				stStream.pstPack = NULL;
			}
#endif
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		// copy kernel space packs to user space
		if (stStream.pstPack) {
			if (copy_to_user(pUserPack, stStream.pstPack,
					 sizeof(VENC_PACK_S) *
					 stStream.u32PackCount) != 0) {

				if (stStream.pstPack) {
					vfree(stStream.pstPack);
					stStream.pstPack = NULL;
				}

				s32Ret = CVI_FAILURE;
				break;
			}

			if (stStream.pstPack) {
				vfree(stStream.pstPack);
				stStream.pstPack = NULL;
			}
		}

		// restore user space pointer
		stStream.pstPack = pUserPack;
		if (copy_to_user(stStreamEx.pstStream, &stStream,
				 sizeof(VENC_STREAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
			break;
		}
#endif

		if (copy_to_user((VENC_STREAM_EX_S *)arg, &stStreamEx,
				 sizeof(VENC_STREAM_EX_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_RELEASE_STREAM: {
		VENC_STREAM_S stStream;

		if (copy_from_user(&stStream, (VENC_STREAM_S *)arg,
				   sizeof(VENC_STREAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_ReleaseStream(minor, &stStream);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_ReleaseStream with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_INSERT_USERDATA: {
		VENC_USER_DATA_S stUserData;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		__u8 *pUserData = NULL;
#endif

		if (copy_from_user(&stUserData, (VENC_USER_DATA_S *)arg,
				   sizeof(VENC_USER_DATA_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		pUserData = vmalloc(stUserData.u32Len);
		if (pUserData == NULL) {
			s32Ret = CVI_ERR_VENC_NOMEM;
			break;
		}

		if (copy_from_user(pUserData, stUserData.pu8Data,
				   stUserData.u32Len) != 0) {
			vfree(pUserData);
			break;
		}

		stUserData.pu8Data = pUserData;
#endif

		s32Ret = CVI_VENC_InsertUserData(minor, stUserData.pu8Data,
						 stUserData.u32Len);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_InsertUserData with %d\n", s32Ret);
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (pUserData)
			vfree(pUserData);
#endif
	} break;
	case CVI_VC_VENC_SEND_FRAME: {
		VIDEO_FRAME_INFO_EX_S stFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		VIDEO_FRAME_INFO_S stFrame;
#endif

		if (copy_from_user(&stFrameEx, (VIDEO_FRAME_INFO_EX_S *)arg,
				   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_from_user(&stFrame, stFrameEx.pstFrame,
				   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
			break;
		}
		stFrameEx.pstFrame = &stFrame;
#endif

		s32Ret = CVI_VENC_SendFrame(minor, stFrameEx.pstFrame,
					    stFrameEx.s32MilliSec);
	} break;
	case CVI_VC_VENC_SEND_FRAMEEX: {
		USER_FRAME_INFO_EX_S stUserFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		USER_FRAME_INFO_S stUserFrameInfo;
		CVI_S32 w, h;
		__u8 *pu8QpMap = NULL;
#endif

		if (copy_from_user(&stUserFrameEx, (USER_FRAME_INFO_EX_S *)arg,
				   sizeof(USER_FRAME_INFO_EX_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_from_user(&stUserFrameInfo, stUserFrameEx.pstUserFrame,
				   sizeof(USER_FRAME_INFO_S)) != 0) {
			break;
		}
		stUserFrameEx.pstUserFrame = &stUserFrameInfo;

		w = (((stUserFrameInfo.stUserFrame.stVFrame.u32Width + 63) & ~63) >> 6);
		h = (((stUserFrameInfo.stUserFrame.stVFrame.u32Height + 63) & ~63) >> 6);
		pu8QpMap = vmalloc(w * h);
		if (pu8QpMap == NULL) {
			s32Ret = CVI_ERR_VENC_NOMEM;
			break;
		}

		if (copy_from_user(pu8QpMap, (__u8 *)stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr,
				   w * h) != 0) {
			vfree(pu8QpMap);
			break;
		}
		stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr = (__u64)pu8QpMap;
#endif

		s32Ret = CVI_VENC_SendFrameEx(minor, stUserFrameEx.pstUserFrame,
					      stUserFrameEx.s32MilliSec);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (pu8QpMap)
			vfree(pu8QpMap);
#endif
	} break;
	case CVI_VC_VENC_REQUEST_IDR: {
		CVI_BOOL bInstant;

		if (copy_from_user(&bInstant, (CVI_BOOL *)arg,
				   sizeof(CVI_BOOL)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_RequestIDR(minor, bInstant);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_RequestIDR with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_SET_ROI_ATTR: {
		VENC_ROI_ATTR_S stRoiAttr;

		if (copy_from_user(&stRoiAttr, (VENC_ROI_ATTR_S *)arg,
				   sizeof(VENC_ROI_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetRoiAttr(minor, &stRoiAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetRoiAttr with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_ROI_ATTR: {
		VENC_ROI_ATTR_S stRoiAttr;

		if (copy_from_user(&stRoiAttr, (VENC_ROI_ATTR_S *)arg,
				   sizeof(VENC_ROI_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_GetRoiAttr(minor, stRoiAttr.u32Index,
					     &stRoiAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetRoiAttr with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_ROI_ATTR_S *)arg, &stRoiAttr,
				 sizeof(VENC_ROI_ATTR_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_TRANS: {
		VENC_H264_TRANS_S stH264Trans;

		if (copy_from_user(&stH264Trans, (VENC_H264_TRANS_S *)arg,
				   sizeof(VENC_H264_TRANS_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264Trans(minor, &stH264Trans);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH264Trans with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H264_TRANS: {
		VENC_H264_TRANS_S stH264Trans;

		s32Ret = CVI_VENC_GetH264Trans(minor, &stH264Trans);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264Trans with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_TRANS_S *)arg, &stH264Trans,
				 sizeof(VENC_H264_TRANS_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_ENTROPY: {
		VENC_H264_ENTROPY_S stH264EntropyEnc;

		if (copy_from_user(&stH264EntropyEnc,
				   (VENC_H264_ENTROPY_S *)arg,
				   sizeof(VENC_H264_ENTROPY_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264Entropy(minor, &stH264EntropyEnc);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH264Entropy with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H264_ENTROPY: {
		VENC_H264_ENTROPY_S stH264EntropyEnc;

		s32Ret = CVI_VENC_GetH264Entropy(minor, &stH264EntropyEnc);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264Entropy with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_ENTROPY_S *)arg, &stH264EntropyEnc,
				 sizeof(VENC_H264_ENTROPY_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_VUI: {
		VENC_H264_VUI_S stH264Vui;

		if (copy_from_user(&stH264Vui, (VENC_H264_VUI_S *)arg,
				   sizeof(VENC_H264_VUI_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264Vui(minor, &stH264Vui);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH264Vui with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H264_VUI: {
		VENC_H264_VUI_S stH264Vui;

		s32Ret = CVI_VENC_GetH264Vui(minor, &stH264Vui);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264Vui with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_VUI_S *)arg, &stH264Vui,
				 sizeof(VENC_H264_VUI_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H265_VUI: {
		VENC_H265_VUI_S stH265Vui;

		if (copy_from_user(&stH265Vui, (VENC_H265_VUI_S *)arg,
				   sizeof(VENC_H265_VUI_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH265Vui(minor, &stH265Vui);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH265Vui with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H265_VUI: {
		VENC_H265_VUI_S stH265Vui;

		s32Ret = CVI_VENC_GetH265Vui(minor, &stH265Vui);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH265Vui with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H265_VUI_S *)arg, &stH265Vui,
				 sizeof(VENC_H265_VUI_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_JPEG_PARAM: {
		VENC_JPEG_PARAM_S stJpegParam;

		if (copy_from_user(&stJpegParam, (VENC_JPEG_PARAM_S *)arg,
				   sizeof(VENC_JPEG_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetJpegParam(minor, &stJpegParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetJpegParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_JPEG_PARAM: {
		VENC_JPEG_PARAM_S stJpegParam;

		s32Ret = CVI_VENC_GetJpegParam(minor, &stJpegParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetJpegParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_JPEG_PARAM_S *)arg, &stJpegParam,
				 sizeof(VENC_JPEG_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_GET_RC_PARAM: {
		VENC_RC_PARAM_S stRcParam;

		s32Ret = CVI_VENC_GetRcParam(minor, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetRcParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_RC_PARAM_S *)arg, &stRcParam,
				 sizeof(VENC_RC_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_RC_PARAM: {
		VENC_RC_PARAM_S stRcParam;

		if (copy_from_user(&stRcParam, (VENC_RC_PARAM_S *)arg,
				   sizeof(VENC_RC_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetRcParam(minor, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetRcParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_SET_REF_PARAM: {
		VENC_REF_PARAM_S stRefParam;

		if (copy_from_user(&stRefParam, (VENC_REF_PARAM_S *)arg,
				   sizeof(VENC_REF_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetRefParam(minor, &stRefParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetRefParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_REF_PARAM: {
		VENC_REF_PARAM_S stRefParam;

		s32Ret = CVI_VENC_GetRefParam(minor, &stRefParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetRefParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_REF_PARAM_S *)arg, &stRefParam,
				 sizeof(VENC_REF_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H265_TRANS: {
		VENC_H265_TRANS_S stH265Trans;

		if (copy_from_user(&stH265Trans, (VENC_H265_TRANS_S *)arg,
				   sizeof(VENC_H265_TRANS_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH265Trans(minor, &stH265Trans);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH265Trans with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H265_TRANS: {
		VENC_H265_TRANS_S stH265Trans;

		s32Ret = CVI_VENC_GetH265Trans(minor, &stH265Trans);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH265Trans with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H265_TRANS_S *)arg, &stH265Trans,
				 sizeof(VENC_H265_TRANS_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_FRAMELOST_STRATEGY: {
		VENC_FRAMELOST_S stFrmLostParam;

		if (copy_from_user(&stFrmLostParam, (VENC_FRAMELOST_S *)arg,
				   sizeof(VENC_FRAMELOST_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetFrameLostStrategy(minor, &stFrmLostParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetFrameLostStrategy with %d\n",
			       s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_FRAMELOST_STRATEGY: {
		VENC_FRAMELOST_S stFrmLostParam;

		s32Ret = CVI_VENC_GetFrameLostStrategy(minor, &stFrmLostParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetFrameLostStrategy with %d\n",
			       s32Ret);
			break;
		}

		if (copy_to_user((VENC_FRAMELOST_S *)arg, &stFrmLostParam,
				 sizeof(VENC_FRAMELOST_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_SUPERFRAME_STRATEGY: {
		VENC_SUPERFRAME_CFG_S stSuperFrmParam;

		if (copy_from_user(&stSuperFrmParam,
				   (VENC_SUPERFRAME_CFG_S *)arg,
				   sizeof(VENC_SUPERFRAME_CFG_S)) != 0) {
			break;
		}

		s32Ret =
			CVI_VENC_SetSuperFrameStrategy(minor, &stSuperFrmParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetSuperFrameStrategy with %d\n",
			       s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_SUPERFRAME_STRATEGY: {
		VENC_SUPERFRAME_CFG_S stSuperFrmParam;

		s32Ret =
			CVI_VENC_GetSuperFrameStrategy(minor, &stSuperFrmParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetSuperFrameStrategy with %d\n",
			       s32Ret);
			break;
		}

		if (copy_to_user((VENC_SUPERFRAME_CFG_S *)arg, &stSuperFrmParam,
				 sizeof(VENC_SUPERFRAME_CFG_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_CHN_PARAM: {
		VENC_CHN_PARAM_S stChnParam;

		if (copy_from_user(&stChnParam, (VENC_CHN_PARAM_S *)arg,
				   sizeof(VENC_CHN_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetChnParam(minor, &stChnParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetChnParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_CHN_PARAM: {
		VENC_CHN_PARAM_S stChnParam;

		s32Ret = CVI_VENC_GetChnParam(minor, &stChnParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetChnParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_CHN_PARAM_S *)arg, &stChnParam,
				 sizeof(VENC_CHN_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_MOD_PARAM: {
		VENC_PARAM_MOD_S stModParam;

		if (copy_from_user(&stModParam, (VENC_PARAM_MOD_S *)arg,
				   sizeof(VENC_PARAM_MOD_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetModParam(&stModParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetModParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_MOD_PARAM: {
		VENC_PARAM_MOD_S stModParam;

		if (copy_from_user(&stModParam, (VENC_PARAM_MOD_S *)arg,
				   sizeof(VENC_PARAM_MOD_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_GetModParam(&stModParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetModParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_PARAM_MOD_S *)arg, &stModParam,
				 sizeof(VENC_PARAM_MOD_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_ATTACH_VBPOOL: {
		VENC_CHN_POOL_S stPool;

		if (copy_from_user(&stPool, (VENC_CHN_POOL_S *)arg,
				   sizeof(VENC_CHN_POOL_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_AttachVbPool(minor, &stPool);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_AttachVbPool with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_DETACH_VBPOOL: {
		s32Ret = CVI_VENC_DetachVbPool(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_DetachVbPool with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_SET_CUPREDICTION: {
		VENC_CU_PREDICTION_S stCuPrediction;

		if (copy_from_user(&stCuPrediction, (VENC_CU_PREDICTION_S *)arg,
				   sizeof(VENC_CU_PREDICTION_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetCuPrediction(minor, &stCuPrediction);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetCuPrediction with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_CUPREDICTION: {
		VENC_CU_PREDICTION_S stCuPrediction;

		s32Ret = CVI_VENC_GetCuPrediction(minor, &stCuPrediction);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetCuPrediction with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_CU_PREDICTION_S *)arg, &stCuPrediction,
				 sizeof(VENC_CU_PREDICTION_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_CALC_FRAME_PARAM: {
		VENC_FRAME_PARAM_S stFrameParam;

		if (copy_from_user(&stFrameParam, (VENC_FRAME_PARAM_S *)arg,
				   sizeof(VENC_FRAME_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_CalcFrameParam(minor, &stFrameParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_CalcFrameParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_SET_FRAME_PARAM: {
		VENC_FRAME_PARAM_S stFrameParam;

		if (copy_from_user(&stFrameParam, (VENC_FRAME_PARAM_S *)arg,
				   sizeof(VENC_FRAME_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetFrameParam(minor, &stFrameParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetFrameParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_FRAME_PARAM: {
		VENC_FRAME_PARAM_S stFrameParam;

		s32Ret = CVI_VENC_GetFrameParam(minor, &stFrameParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetFrameParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_FRAME_PARAM_S *)arg, &stFrameParam,
				 sizeof(VENC_FRAME_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_SLICE_SPLIT: {
		VENC_H264_SLICE_SPLIT_S stH264Split;

		if (copy_from_user(&stH264Split, (VENC_H264_SLICE_SPLIT_S *)arg,
				sizeof(VENC_H264_SLICE_SPLIT_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264SliceSplit(minor, &stH264Split);
	} break;
	case CVI_VC_VENC_GET_H264_SLICE_SPLIT: {
		VENC_H264_SLICE_SPLIT_S stH264Split;

		s32Ret = CVI_VENC_GetH264SliceSplit(minor, &stH264Split);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264SliceSplit with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_SLICE_SPLIT_S *)arg, &stH264Split,
				 sizeof(VENC_H264_SLICE_SPLIT_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H265_SLICE_SPLIT: {
		VENC_H265_SLICE_SPLIT_S stH265Split;

		if (copy_from_user(&stH265Split, (VENC_H265_SLICE_SPLIT_S *)arg,
				sizeof(VENC_H265_SLICE_SPLIT_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH265SliceSplit(minor, &stH265Split);
	} break;
	case CVI_VC_VENC_GET_H265_SLICE_SPLIT: {
		VENC_H265_SLICE_SPLIT_S stH265Split;

		s32Ret = CVI_VENC_GetH265SliceSplit(minor, &stH265Split);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH265SliceSplit with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H265_SLICE_SPLIT_S *)arg, &stH265Split,
				 sizeof(VENC_H265_SLICE_SPLIT_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_Dblk: {
		VENC_H264_DBLK_S stH264dblk;

		if (copy_from_user(&stH264dblk, (VENC_H264_DBLK_S *)arg,
				   sizeof(VENC_H264_DBLK_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264Dblk(minor, &stH264dblk);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH264Dblk with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H264_Dblk: {
		VENC_H264_DBLK_S stH264dblk;

		s32Ret = CVI_VENC_GetH264Dblk(minor, &stH264dblk);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264Dblk with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_DBLK_S *)arg, &stH264dblk,
				 sizeof(VENC_H264_DBLK_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H265_Dblk: {
		VENC_H265_DBLK_S stH265dblk;

		if (copy_from_user(&stH265dblk, (VENC_H265_DBLK_S *)arg,
				   sizeof(VENC_H265_DBLK_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH265Dblk(minor, &stH265dblk);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH265Dblk with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VENC_GET_H265_Dblk: {
		VENC_H265_DBLK_S stH265dblk;

		s32Ret = CVI_VENC_GetH265Dblk(minor, &stH265dblk);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH265Dblk with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H265_DBLK_S *)arg, &stH265dblk,
				 sizeof(VENC_H265_DBLK_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_SET_H264_INTRA_PRED: {
		VENC_H264_INTRA_PRED_S stH264IntraPred;

		if (copy_from_user(&stH264IntraPred, (VENC_H264_INTRA_PRED_S *)arg,
				sizeof(VENC_H264_INTRA_PRED_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetH264IntraPred(minor, &stH264IntraPred);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SetH264IntraPred with %d\n", s32Ret);
			break;
		}
	} break;
	case CVI_VC_VENC_GET_H264_INTRA_PRED: {
		VENC_H264_INTRA_PRED_S stH264IntraPred;

		s32Ret = CVI_VENC_GetH264IntraPred(minor, &stH264IntraPred);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GetH264IntraPred with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_H264_INTRA_PRED_S *)arg, &stH264IntraPred,
				 sizeof(VENC_H264_INTRA_PRED_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VENC_ENABLE_SVC: {
		CVI_BOOL enable_svc = CVI_FALSE;

		if (copy_from_user(&enable_svc, (CVI_BOOL *)arg,
		   sizeof(CVI_BOOL)) != 0) {
			break;
		}
		s32Ret = CVI_VENC_EnableSVC(minor, enable_svc);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_EnableSVC with %d\n", s32Ret);
			break;
		}
	} break;
	case CVI_VC_VENC_SET_SVC_PARAM: {
		VENC_SVC_PARAM_S stSvcParam;

		if (copy_from_user(&stSvcParam, (VENC_SVC_PARAM_S *)arg,
		   sizeof(VENC_SVC_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VENC_SetSvcParam(minor, &stSvcParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_SET_SvcParam with %d\n", s32Ret);
			break;
		}
	} break;
	case CVI_VC_VENC_GET_SVC_PARAM: {
		VENC_SVC_PARAM_S stSvcParam;

		s32Ret = CVI_VENC_GetSvcParam(minor, &stSvcParam);

		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VENC_GET_SvcParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VENC_SVC_PARAM_S *)arg, &stSvcParam,
		   sizeof(VENC_SVC_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
#ifdef VC_DRIVER_TEST
	case CVI_VC_ENCODE_MAIN_TEST: {
		s32Ret = cvi_venc_test(arg);
	} break;
	case CVI_VC_ENC_DEC_JPEG_TEST: {
		s32Ret = cvi_jpeg_test(arg);
	} break;
#endif
	default: {
		pr_err("venc un-handle cmd id: %x\n", cmd);
	} break;
	}

	up(&vencSemArry[minor]);
	return s32Ret;
}
#ifdef ENABLE_DEC
static long cvi_vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	unsigned int minor = iminor(file_inode(filp));
	CVI_S32 s32Ret = CVI_FAILURE;

	if(MaxVdecChnNum < 0)
		return s32Ret;
	if (down_interruptible(&vdecSemArry[minor])) {
		return s32Ret;
	}

	switch (cmd) {
	case CVI_VC_VDEC_CREATE_CHN: {
		VDEC_CHN_ATTR_S stChnAttr;

		if (copy_from_user(&stChnAttr, (VDEC_CHN_ATTR_S *)arg,
				   sizeof(VDEC_CHN_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_CreateChn(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_CreateChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_DESTROY_CHN: {
		s32Ret = CVI_VDEC_DestroyChn(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_DestroyChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_GET_CHN_ATTR: {
		VDEC_CHN_ATTR_S stChnAttr;

		s32Ret = CVI_VDEC_GetChnAttr(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_GetChnAttr with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VDEC_CHN_ATTR_S *)arg, &stChnAttr,
				 sizeof(VDEC_CHN_ATTR_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VDEC_SET_CHN_ATTR: {
		VDEC_CHN_ATTR_S stChnAttr;

		if (copy_from_user(&stChnAttr, (VDEC_CHN_ATTR_S *)arg,
				   sizeof(VDEC_CHN_ATTR_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_SetChnAttr(minor, &stChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_SetChnAttr with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_START_RECV_STREAM: {
		s32Ret = CVI_VDEC_StartRecvStream(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_StartRecvStream with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_STOP_RECV_STREAM: {
		s32Ret = CVI_VDEC_StopRecvStream(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_StopRecvStream with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_QUERY_STATUS: {
		VDEC_CHN_STATUS_S stStatus;

		if (copy_from_user(&stStatus, (VDEC_CHN_STATUS_S *)arg,
				   sizeof(VDEC_CHN_STATUS_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_QueryStatus(minor, &stStatus);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_QueryStatus with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VDEC_CHN_STATUS_S *)arg, &stStatus,
				 sizeof(VDEC_CHN_STATUS_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VDEC_RESET_CHN: {
		s32Ret = CVI_VDEC_ResetChn(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_ResetChn with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_SET_CHN_PARAM: {
		VDEC_CHN_PARAM_S stChnParam;

		if (copy_from_user(&stChnParam, (VDEC_CHN_PARAM_S *)arg,
				   sizeof(VDEC_CHN_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_SetChnParam(minor, &stChnParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_SetChnParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_GET_CHN_PARAM: {
		VDEC_CHN_PARAM_S stChnParam;

		s32Ret = CVI_VDEC_GetChnParam(minor, &stChnParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_GetChnParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VDEC_CHN_PARAM_S *)arg, &stChnParam,
				 sizeof(VDEC_CHN_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VDEC_SEND_STREAM: {
		VDEC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		VDEC_STREAM_S stStream;
		__u8 *pStreamData = NULL;
#endif

		if (copy_from_user(&stStreamEx, (VDEC_STREAM_EX_S *)arg,
				   sizeof(VDEC_STREAM_EX_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_from_user(&stStream, stStreamEx.pstStream,
				   sizeof(VDEC_STREAM_S)) != 0) {
			break;
		}
		stStreamEx.pstStream = &stStream;

		if (stStream.u32Len) {
			pStreamData = osal_ion_alloc(stStream.u32Len);
			if (pStreamData == NULL) {
				s32Ret = CVI_ERR_VENC_NOMEM;
				break;
			}

			if (copy_from_user(pStreamData, stStream.pu8Addr,
					   stStream.u32Len) != 0) {
				osal_ion_free(pStreamData);
				break;
			}
		}
		stStream.pu8Addr = pStreamData;
#endif

		s32Ret = CVI_VDEC_SendStream(minor, stStreamEx.pstStream,
					     stStreamEx.s32MilliSec);
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (pStreamData)
			osal_ion_free(pStreamData);
#endif
	} break;
	case CVI_VC_VDEC_GET_FRAME: {
		VIDEO_FRAME_INFO_EX_S stFrameInfoEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		VIDEO_FRAME_INFO_S stFrameInfo;
		VIDEO_FRAME_INFO_S *pUserFrameInfo; // keep user space pointer on frame info
#endif

		if (copy_from_user(&stFrameInfoEx, (VIDEO_FRAME_INFO_EX_S *)arg,
				   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_from_user(&stFrameInfo, stFrameInfoEx.pstFrame,
				   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
			break;
		}
		pUserFrameInfo = stFrameInfoEx.pstFrame;
		stFrameInfoEx.pstFrame = &stFrameInfo; // replace user space pointer
#endif

		s32Ret = CVI_VDEC_GetFrame(minor, stFrameInfoEx.pstFrame,
					   stFrameInfoEx.s32MilliSec);
		if (s32Ret != CVI_SUCCESS) {
			break;
		}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE) && defined(__riscv)
		if (copy_to_user(pUserFrameInfo, stFrameInfoEx.pstFrame,
				   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
			break;
		}
		stFrameInfoEx.pstFrame = pUserFrameInfo; // restore user space pointer
#endif

		if (copy_to_user((VIDEO_FRAME_INFO_EX_S *)arg, &stFrameInfoEx,
				 sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
	case CVI_VC_VDEC_RELEASE_FRAME: {
		VIDEO_FRAME_INFO_S stFrameInfo;

		if (copy_from_user(&stFrameInfo, (VIDEO_FRAME_INFO_S *)arg,
				   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_ReleaseFrame(minor, &stFrameInfo);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_ReleaseFrame with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_ATTACH_VBPOOL: {
		VDEC_CHN_POOL_S stPool;

		if (copy_from_user(&stPool, (VDEC_CHN_POOL_S *)arg,
				   sizeof(VDEC_CHN_POOL_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_AttachVbPool(minor, &stPool);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_AttachVbPool with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_DETACH_VBPOOL: {
		s32Ret = CVI_VDEC_DetachVbPool(minor);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_DetachVbPool with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_SET_MOD_PARAM: {
		VDEC_MOD_PARAM_S stModParam;

		if (copy_from_user(&stModParam, (VDEC_MOD_PARAM_S *)arg,
				   sizeof(VDEC_MOD_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_SetModParam(&stModParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_SetModParam with %d\n", s32Ret);
		}
	} break;
	case CVI_VC_VDEC_GET_MOD_PARAM: {
		VDEC_MOD_PARAM_S stModParam;

		if (copy_from_user(&stModParam, (VDEC_MOD_PARAM_S *)arg,
				   sizeof(VDEC_MOD_PARAM_S)) != 0) {
			break;
		}

		s32Ret = CVI_VDEC_GetModParam(&stModParam);
		if (s32Ret != CVI_SUCCESS) {
			pr_err("CVI_VDEC_GetModParam with %d\n", s32Ret);
			break;
		}

		if (copy_to_user((VDEC_MOD_PARAM_S *)arg, &stModParam,
				 sizeof(VDEC_MOD_PARAM_S)) != 0) {
			s32Ret = CVI_FAILURE;
		}
	} break;
#ifdef VC_DRIVER_TEST
	case CVI_VC_DECODE_H265_TEST: {
		h265_dec_test(arg);
	} break;
	case CVI_VC_DECODE_H264_TEST: {
		h264_dec_test(arg);
	} break;
#endif
	default: {
		pr_err("vdec un-handle cmd id: %x\n", cmd);
	} break;
	}

	up(&vdecSemArry[minor]);
	return s32Ret;
}
#endif
extern CVI_S32 cviGetLeftStreamFrames(CVI_S32 VeChn);
static unsigned int cvi_vc_drv_poll(struct file *filp,
				    struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	unsigned int minor = iminor(file_inode(filp));

	VENC_CHN_ATTR_S stChnAttr = { 0 };

	poll_wait(filp, &tWaitQueue[minor], wait);

	CVI_VENC_GetChnAttr(minor, &stChnAttr);

	if ((stChnAttr.stVencAttr.enType == PT_JPEG ||
	     stChnAttr.stVencAttr.enType == PT_MJPEG)
		 && (cviGetLeftStreamFrames(minor) > 0)) {
		mask |= POLLIN | POLLRDNORM;
	}
	if (((stChnAttr.stVencAttr.enType == PT_H265 ||
	     stChnAttr.stVencAttr.enType == PT_H264))
	     && (cviGetLeftStreamFrames(minor) > 0)) {
		mask |= POLLIN | POLLRDNORM;
	}

	return mask;
}

static int cvi_vc_drv_release(struct inode *inode, struct file *filp)
{
	unsigned int minor = iminor(inode);

	pr_info("close channel no. %u\n", minor);
	return 0;
}

static int cvi_vc_drv_register_cdev(struct cvi_vc_drv_device *vdev)
{
	int err = 0;
	int i = 0;

	vdev->cvi_vc_class = class_create(THIS_MODULE, CVI_VC_DRV_CLASS_NAME);
	if (IS_ERR(vdev->cvi_vc_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(vdev->cvi_vc_class);
	}

	if(MaxVencChnNum > 0) {
		/* get the major number of the character device */
		if ((alloc_chrdev_region(&vdev->venc_cdev_id, 0, MaxVencChnNum,
					CVI_VC_DRV_ENCODER_DEV_NAME)) < 0) {
			err = -EBUSY;
			pr_err("could not allocate major number\n");
			return err;
		}
		vdev->s_venc_major = MAJOR(vdev->venc_cdev_id);
		vdev->p_venc_cdev = vzalloc(sizeof(struct cdev) * MaxVencChnNum);

		for (i = 0; i < MaxVencChnNum; i++) {
			char devName[256];
			dev_t subDevice;

			subDevice = MKDEV(vdev->s_venc_major, i);

			/* initialize the device structure and register the device with the kernel */
			cdev_init(&vdev->p_venc_cdev[i], &cvi_vc_drv_venc_fops);
			vdev->p_venc_cdev[i].owner = THIS_MODULE;

			if ((cdev_add(&vdev->p_venc_cdev[i], subDevice, 1)) < 0) {
				err = -EBUSY;
				pr_err("could not allocate chrdev\n");
				return err;
			}

			sprintf(devName, "%s%d", CVI_VC_DRV_ENCODER_DEV_NAME, i);
			device_create(vdev->cvi_vc_class, NULL, subDevice, NULL, "%s",
					devName);
			sema_init(&vencSemArry[i], 1);
		}
	} else {
		printk("Venc Chn is zero\n");
	}
#ifdef ENABLE_DEC
	if(MaxVdecChnNum > 0) {
		/* get the major number of the character device */
		if ((alloc_chrdev_region(&vdev->vdec_cdev_id, 0, MaxVdecChnNum,
					CVI_VC_DRV_DECODER_DEV_NAME)) < 0) {
			err = -EBUSY;
			pr_err("could not allocate major number\n");
			return err;
		}
		vdev->s_vdec_major = MAJOR(vdev->vdec_cdev_id);
		vdev->p_vdec_cdev = vzalloc(sizeof(struct cdev) * MaxVdecChnNum);

		for (i = 0; i < MaxVdecChnNum; i++) {
			char devName[256];
			dev_t subDevice;

			subDevice = MKDEV(vdev->s_vdec_major, i);

			/* initialize the device structure and register the device with the kernel */
			cdev_init(&vdev->p_vdec_cdev[i], &cvi_vc_drv_vdec_fops);
			vdev->p_vdec_cdev[i].owner = THIS_MODULE;

			if ((cdev_add(&vdev->p_vdec_cdev[i], subDevice, 1)) < 0) {
				err = -EBUSY;
				pr_err("could not allocate chrdev\n");
				return err;
			}

			sprintf(devName, "%s%d", CVI_VC_DRV_DECODER_DEV_NAME, i);
			device_create(vdev->cvi_vc_class, NULL, subDevice, NULL, "%s",
					devName);
			sema_init(&vdecSemArry[i], 1);
		}
	} else {
		printk("Vdec Chn is zero\n");
	}
#endif
	return err;
}

static int cvi_vc_drv_get_reg_resource(struct cvi_vc_drv_device *vdev,
				       struct platform_device *pdev)
{
	struct resource *res = NULL;
	cvi_vc_drv_buffer_t *pReg;

	// vc_ctrl
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_ctrl");
	if (res) {
		pReg = &vdev->ctrl_register;
		pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		pReg->virt_addr =
			(__u8 *)ioremap(res->start, res->end - res->start);
#else
		pReg->virt_addr = (__u8 *)ioremap_nocache(
			res->start, res->end - res->start);
#endif
		pReg->size = res->end - res->start;
	} else {
		pr_err("can not get vc_ctrl reg!\n");
		return -ENXIO;
	}

	// vc_sbm
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vc_sbm");
	if (res) {
		pReg = &vdev->sbm_register;
		pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		pReg->virt_addr =
			(__u8 *)ioremap(res->start, res->end - res->start);
#else
		pReg->virt_addr = (__u8 *)ioremap_nocache(
			res->start, res->end - res->start);
#endif
		pReg->size = res->end - res->start;
	} else {
		pr_err("can not get vc_sbm reg!\n");
		return -ENXIO;
	}

	// vc_addr_remap
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "vc_addr_remap");
	if (res) {
		pReg = &vdev->remap_register;
		pReg->phys_addr = res->start;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		pReg->virt_addr =
			(__u8 *)ioremap(res->start, res->end - res->start);
#else
		pReg->virt_addr = (__u8 *)ioremap_nocache(
			res->start, res->end - res->start);
#endif
		pReg->size = res->end - res->start;
	} else {
		pr_err("can not get vc_addr_remap reg!\n");
		return -ENXIO;
	}
	return 0;
}

int vcodec_drv_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	int ret = CVI_FAILURE;

	switch (cmd) {
	case VCODEC_CB_SEND_FRM:
	{
		struct venc_send_frm_info *pstFrmInfo = (struct venc_send_frm_info *)arg;

		ret = cvi_VENC_CB_SendFrame(pstFrmInfo->vpss_grp,
									pstFrmInfo->vpss_chn, pstFrmInfo->vpss_chn1,
									&pstFrmInfo->stInFrmBuf,
									&pstFrmInfo->stInFrmBuf1,
									pstFrmInfo->isOnline,
									pstFrmInfo->sb_nb,
									20000);
		break;
	}

	case VCODEC_CB_SKIP_FRM:
	{
		struct venc_send_frm_info *pstFrmInfo = (struct venc_send_frm_info *)arg;

		ret = cvi_VENC_CB_SkipFrame(pstFrmInfo->vpss_grp, pstFrmInfo->vpss_chn,
							  pstFrmInfo->stInFrmBuf.size.u32Height, 100);
		break;
	}

	case VCODEC_CB_SNAP_JPG_FRM:
	{
		struct venc_snap_frm_info *pstSkipInfo = (struct venc_snap_frm_info *)arg;

		ret = cvi_VENC_CB_SnapJpgFrame(pstSkipInfo->vpss_grp,
										pstSkipInfo->vpss_chn,
										pstSkipInfo->skip_frm_cnt);
		break;
	}

	case VCODEC_CB_OVERFLOW_CHECK:
	{
		struct cvi_vc_info *vc_info = (struct cvi_vc_info *)arg;

		unsigned int reg_00 = cvi_vc_drv_read_vc_reg(REG_SBM, 0x00);
		unsigned int reg_08 = cvi_vc_drv_read_vc_reg(REG_SBM, 0x08);
		unsigned int reg_88 = cvi_vc_drv_read_vc_reg(REG_SBM, 0x88);
		unsigned int reg_90 = cvi_vc_drv_read_vc_reg(REG_SBM, 0x90);
		unsigned int reg_94 = cvi_vc_drv_read_vc_reg(REG_SBM, 0x94);

		vc_info->enable = true;
		vc_info->reg_00 = reg_00;
		vc_info->reg_08 = reg_08;
		vc_info->reg_88 = reg_88;
		vc_info->reg_90 = reg_90;
		vc_info->reg_94 = reg_94;

		pr_info("vc check reg_00=0x%x, reg_08=0x%x, reg_88=0x%x, reg_90=0x%x, reg_94=0x%x\n",
			reg_00, reg_08, reg_88, reg_90, reg_94);

		ret = CVI_SUCCESS;
		break;
	}

	case VCODEC_CB_SWITCH_CHN:
	{
		struct venc_switch_chn *switch_chn = (struct venc_switch_chn *)arg;

		ret = cvi_VENC_CB_SwitchChn(switch_chn->vpss_grp, switch_chn->vpss_chn);

		break;
	}

	default:
		break;
	}

	return ret;
}

static int vcodec_drv_register_cb(struct cvi_vc_drv_device *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VCODEC;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vcodec_drv_cb;

	return base_reg_module_cb(&reg_cb);
}

static int cvi_vc_drv_probe(struct platform_device *pdev)
{
	int err = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cvi_vc_drv_device *vdev;

	vdev = devm_kzalloc(&pdev->dev, sizeof(*vdev), GFP_KERNEL);
	if (!vdev) {
		return -ENOMEM;
	}

	memset(vdev, 0, sizeof(*vdev));

	vdev->dev = dev;

	match = of_match_device(cvi_vc_drv_match_table, &pdev->dev);
	if (!match) {
		pr_err("of_match_device fail\n");
		return -EINVAL;
	}

	vdev->pdata = match->data;

	err = cvi_vc_drv_get_reg_resource(vdev, pdev);
	if (err) {
		pr_err("cvi_vc_drv_get_reg_resourc fail!!\n");
		goto ERROR_PROBE_DEVICE;
	}

	err = cvi_vc_drv_register_cdev(vdev);
	if (err < 0) {
		pr_err("cvi_vc_drv_register_cdev fail\n");
		goto ERROR_PROBE_DEVICE;
	}

	platform_set_drvdata(pdev, vdev);

	pCviVcDrvDevice = vdev;

	venc_proc_init(dev);
	h265e_proc_init(dev);
	h264e_proc_init(dev);
	codecinst_proc_init(dev);
	jpege_proc_init(dev);
	rc_proc_init(dev);
#ifdef ENABLE_DEC
	if(MaxVdecChnNum > 0)
		vdec_proc_init(dev);
#endif
	/* vcodec register cb */
	err = vcodec_drv_register_cb(vdev);
	if (err < 0) {
		pr_err("Failed to register vcodec cb, err %d\n", err);
		goto ERROR_PROBE_DEVICE;
	}

	return 0;

ERROR_PROBE_DEVICE:

	if (vdev->s_venc_major)
		unregister_chrdev_region(vdev->s_venc_major, MaxVencChnNum);
#ifdef ENABLE_DEC
	if (vdev->s_vdec_major)
		unregister_chrdev_region(vdev->s_vdec_major, MaxVdecChnNum);
#endif
	//platform_set_drvdata(pdev, &vcodec_dev);

	return err;
}

static int cvi_vc_drv_remove(struct platform_device *pdev)
{
	struct cvi_vc_drv_device *vdev = platform_get_drvdata(pdev);

	if (vdev->s_venc_major > 0) {
		int i = 0;

		for (i = 0; i < MaxVencChnNum; i++) {
			dev_t subDevice;

			subDevice = MKDEV(vdev->s_venc_major, i);
			cdev_del(&vdev->p_venc_cdev[i]);
			device_destroy(vdev->cvi_vc_class, subDevice);
		}
		vfree(vdev->p_venc_cdev);
		unregister_chrdev_region(vdev->s_venc_major, MaxVencChnNum);
		vdev->s_venc_major = 0;
	}
#ifdef ENABLE_DEC
	if (vdev->s_vdec_major > 0) {
		int i = 0;

		for (i = 0; i < MaxVdecChnNum; i++) {
			dev_t subDevice;

			subDevice = MKDEV(vdev->s_vdec_major, i);
			cdev_del(&vdev->p_vdec_cdev[i]);
			device_destroy(vdev->cvi_vc_class, subDevice);
		}
		vfree(vdev->p_vdec_cdev);
		unregister_chrdev_region(vdev->s_vdec_major, MaxVdecChnNum);
		vdev->s_vdec_major = 0;
	}
#endif
	class_destroy(vdev->cvi_vc_class);
	return 0;
}

unsigned int cvi_vc_drv_read_vc_reg(REG_TYPE eRegType, unsigned long addr)
{
	unsigned long *reg_addr = NULL;

	if (!pCviVcDrvDevice)
		return 0;

	// TODO: enable CCF?

	switch (eRegType) {
	case REG_CTRL:
		reg_addr = (unsigned long *)(addr +
					     (unsigned long)pCviVcDrvDevice
						     ->ctrl_register.virt_addr);
		break;
	case REG_SBM:
		reg_addr = (unsigned long *)(addr +
					     (unsigned long)pCviVcDrvDevice
						     ->sbm_register.virt_addr);
		break;
	case REG_REMAP:
		reg_addr =
			(unsigned long *)(addr +
					  (unsigned long)pCviVcDrvDevice
						  ->remap_register.virt_addr);
		break;
	default:
		break;
	}

	if (!reg_addr)
		return 0;

	pr_debug("read, %p, 0x%X\n", reg_addr, readl(reg_addr));

	return readl(reg_addr);
}

void cvi_vc_drv_write_vc_reg(REG_TYPE eRegType, unsigned long addr,
			     unsigned int data)
{
	unsigned long *reg_addr = NULL;

	if (!pCviVcDrvDevice)
		return;

	// TODO: enable CCF?

	switch (eRegType) {
	case REG_CTRL:
		reg_addr = (unsigned long *)(addr +
					     (unsigned long)pCviVcDrvDevice
						     ->ctrl_register.virt_addr);
		break;
	case REG_SBM:
		reg_addr = (unsigned long *)(addr +
					     (unsigned long)pCviVcDrvDevice
						     ->sbm_register.virt_addr);
		break;
	case REG_REMAP:
		reg_addr =
			(unsigned long *)(addr +
					  (unsigned long)pCviVcDrvDevice
						  ->remap_register.virt_addr);
		break;
	default:
		break;
	}

	if (!reg_addr)
		return;

	pr_debug("write, %p = 0x%X\n", reg_addr, data);

	writel(data, reg_addr);
}

static struct platform_driver cvi_vc_drv_platform_driver = {
	.driver = {
		.name = CVI_VC_DRV_PLATFORM_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cvi_vc_drv_match_table,
	},
	.probe = cvi_vc_drv_probe,
	.remove = cvi_vc_drv_remove,
};

static int __init cvi_vc_drv_init(void)
{
	int res = 0;

	res = platform_driver_register(&cvi_vc_drv_platform_driver);

	pr_info("cvi_vc_drv_init result = 0x%x\n", res);

	return res;
}

static void __exit cvi_vc_drv_exit(void)
{
	venc_proc_deinit();
	h265e_proc_deinit();
	h264e_proc_deinit();
	codecinst_proc_deinit();
	jpege_proc_deinit();
	rc_proc_deinit();
#ifdef ENABLE_DEC
	if(MaxVdecChnNum > 0)
		vdec_proc_deinit();
#endif
	platform_driver_unregister(&cvi_vc_drv_platform_driver);
}

MODULE_AUTHOR("CVITEKVCODEC Inc.");
MODULE_DESCRIPTION("CVITEK VC linux driver");
MODULE_LICENSE("GPL");

module_init(cvi_vc_drv_init);
module_exit(cvi_vc_drv_exit);
