#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include "math.h"
#include <inttypes.h>

#include <fcntl.h>		/* low-level i/o */
#include "cvi_buffer.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"
#include "cvi_sns_ctrl.h"
#include "cvi_ive.h"
#include "sample_comm.h"
#include "sophgo_middleware.h"


#define MMF_VI_MAX_CHN 			2		// manually limit the max channel number of vi
#define MMF_VO_VIDEO_MAX_CHN 	1		// manually limit the max channel number of vo
#define MMF_VO_OSD_MAX_CHN 		1
#define MMF_VO_VIDEO_LAYER    	0
#define MMF_VO_OSD_LAYER    	1
#define MMF_VO_USE_NV21_ONLY	0
#define MMF_RGN_MAX_NUM			16

#define MMF_VB_VI_ID			0
#define MMF_VB_VO_ID			1
#define MMF_VB_USER_ID			2
#define MMF_VB_ENC_H265_ID		3
#define MMF_VB_ENC_JPEG_ID		4

#if VPSS_MAX_PHY_CHN_NUM < MMF_VI_MAX_CHN
#error "VPSS_MAX_PHY_CHN_NUM < MMF_VI_MAX_CHN"
#endif
typedef struct {
	int mmf_used_cnt;
	int vo_rotate;	// 90, 180, 270
	bool vi_is_inited;
	bool vi_chn_is_inited[MMF_VI_MAX_CHN];
	bool vo_video_chn_is_inited[MMF_VO_VIDEO_MAX_CHN];
	bool vo_osd_chn_is_inited[MMF_VO_OSD_MAX_CHN];
	SIZE_S vi_size;
	SIZE_S vo_vpss_in_size[MMF_VO_VIDEO_MAX_CHN];
	SIZE_S vo_vpss_out_size[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_out_fps[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_out_depth[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_in_format[MMF_VO_VIDEO_MAX_CHN];
	int vo_vpss_fit[MMF_VO_VIDEO_MAX_CHN];
	VIDEO_FRAME_INFO_S vi_frame[MMF_VI_MAX_CHN];
	VIDEO_FRAME_INFO_S *vo_video_pre_frame[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_width[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_height[MMF_VO_VIDEO_MAX_CHN];
	int vo_video_pre_frame_format[MMF_VO_VIDEO_MAX_CHN];
	SAMPLE_VO_CONFIG_S vo_video_cfg[MMF_VO_VIDEO_MAX_CHN];
	VB_CONFIG_S vb_conf;

	int ive_is_init;
	IVE_HANDLE ive_handle;
	IVE_IMAGE_S ive_rgb2yuv_rgb_img;
	IVE_IMAGE_S ive_rgb2yuv_yuv_img;
	int ive_rgb2yuv_w;
	int ive_rgb2yuv_h;

	bool rgn_is_init[MMF_RGN_MAX_NUM];
	bool rgn_is_bind[MMF_RGN_MAX_NUM];
	RGN_TYPE_E rgn_type[MMF_RGN_MAX_NUM];
	int rgn_id[MMF_RGN_MAX_NUM];
	MOD_ID_E rgn_mod_id[MMF_RGN_MAX_NUM];
	CVI_S32 rgn_dev_id[MMF_RGN_MAX_NUM];
	CVI_S32 rgn_chn_id[MMF_RGN_MAX_NUM];
	uint8_t* rgn_canvas_data[MMF_RGN_MAX_NUM];
	int rgn_canvas_w[MMF_RGN_MAX_NUM];
	int rgn_canvas_h[MMF_RGN_MAX_NUM];
	int rgn_canvas_format[MMF_RGN_MAX_NUM];

	int enc_jpg_is_init;
	VENC_STREAM_S enc_jpeg_frame;
	int enc_jpg_frame_w;
	int enc_jpg_frame_h;
	int enc_jpg_frame_fmt;
	int enc_jpg_running;
	VIDEO_FRAME_INFO_S *enc_jpg_frame;

	int vb_of_vi_is_config : 1;
	int vb_of_vo_is_config : 1;
	int vb_of_private_is_config : 1;
	int vb_size_of_vi;
	int vb_count_of_vi;
	int vb_size_of_vo;
	int vb_count_of_vo;
	int vb_size_of_private;
	int vb_count_of_private;

	int enc_h265_is_init : 1;
	int enc_h265_running : 1;
	VIDEO_FRAME_INFO_S *enc_h265_video_frame;
	VENC_STREAM_S enc_h265_stream;
	int enc_h265_frame_w;
	int enc_h265_frame_h;
	int enc_h265_frame_fmt;

	SAMPLE_SNS_TYPE_E sensor_type;
} priv_t;

typedef struct {
	int enc_h265_enable : 1;
	int enc_jpg_enable : 1;
	bool vi_hmirror[MMF_VI_MAX_CHN];
	bool vi_vflip[MMF_VI_MAX_CHN];
	bool vo_video_hmirror[MMF_VO_VIDEO_MAX_CHN];
	bool vo_video_vflip[MMF_VO_VIDEO_MAX_CHN];
	bool vo_osd_hmirror[MMF_VO_OSD_MAX_CHN];
	bool vo_osd_vflip[MMF_VO_OSD_MAX_CHN];
} g_priv_t;

static priv_t priv;
static g_priv_t g_priv;

#define DISP_W	640
#define DISP_H	480
static void priv_param_init(void)
{
	priv.vo_rotate = 90;
	priv.vb_conf.u32MaxPoolCnt = 1;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkSize = ALIGN(DISP_W, DEFAULT_ALIGN) * ALIGN(DISP_H, DEFAULT_ALIGN) * 3;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkCnt = 8;
	priv.vb_conf.astCommPool[MMF_VB_VO_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	priv.vb_conf.u32MaxPoolCnt ++;

	priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
	priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkCnt = 2;
	priv.vb_conf.astCommPool[MMF_VB_USER_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	priv.vb_conf.u32MaxPoolCnt ++;

	g_priv.enc_h265_enable = 1;
	if (g_priv.enc_h265_enable) {
		priv.vb_conf.astCommPool[MMF_VB_ENC_H265_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
		priv.vb_conf.astCommPool[MMF_VB_ENC_H265_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_ENC_H265_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	g_priv.enc_jpg_enable = 1;
	if (g_priv.enc_jpg_enable) {
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].u32BlkSize = ALIGN(2560, DEFAULT_ALIGN) * ALIGN(1440, DEFAULT_ALIGN) * 3 / 2;
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].u32BlkCnt = 1;
		priv.vb_conf.astCommPool[MMF_VB_ENC_JPEG_ID].enRemapMode = VB_REMAP_MODE_CACHED;
		priv.vb_conf.u32MaxPoolCnt ++;
	}

	if (priv.vb_of_vi_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].u32BlkSize = priv.vb_size_of_vi;
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].u32BlkCnt = priv.vb_count_of_vi;
		priv.vb_conf.astCommPool[MMF_VB_VI_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}

	if (priv.vb_of_vo_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkSize = priv.vb_size_of_vo;
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].u32BlkCnt = priv.vb_count_of_vo;
		priv.vb_conf.astCommPool[MMF_VB_VO_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}

	if (priv.vb_of_private_is_config) {
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkSize = priv.vb_size_of_private;
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].u32BlkCnt = priv.vb_count_of_private;
		priv.vb_conf.astCommPool[MMF_VB_USER_ID].enRemapMode = VB_REMAP_MODE_CACHED;
	}
}

static SAMPLE_VI_CONFIG_S g_stViConfig;
static SAMPLE_INI_CFG_S g_stIniCfg;

void mmf_dump_grpattr(VPSS_GRP_ATTR_S *GrpAttr) {
	printf("GrpAttr->u32MaxW: %d, GrpAttr->u32MaxH: %d\r\n", GrpAttr->u32MaxW, GrpAttr->u32MaxH);
	printf("GrpAttr->enPixelFormat:%d \r\n", GrpAttr->enPixelFormat);
	printf("GrpAttr->stFrameRate.s32SrcFrameRate:%d \r\n", GrpAttr->stFrameRate.s32SrcFrameRate);
	printf("GrpAttr->stFrameRate.s32DstFrameRate:%d \r\n", GrpAttr->stFrameRate.s32DstFrameRate);
	printf("GrpAttr->u8VpssDev:%d\n", GrpAttr->u8VpssDev);
}

void mmf_dump_venc_chn_status(VENC_CHN_STATUS_S *status) {
	if (status == NULL) {
		printf("status is null\n");
		return;
	}

	printf("u32LeftPics:\t\t%d\n", status->u32LeftPics);
	printf("u32LeftStreamBytes:\t\t%d\n", status->u32LeftStreamBytes);
	printf("u32LeftStreamFrames:\t\t%d\n", status->u32LeftStreamFrames);
	printf("u32CurPacks:\t\t%d\n", status->u32CurPacks);
	printf("u32LeftRecvPics:\t\t%d\n", status->u32LeftRecvPics);
	printf("u32LeftEncPics:\t\t%d\n", status->u32LeftEncPics);
	printf("bJpegSnapEnd:\t\t%d\n", status->bJpegSnapEnd);
	printf("stVencStrmInfo:\t\tnone\n"); // status->stVencStrmInfo
}

void mmf_dump_chnattr(VPSS_CHN_ATTR_S *ChnAttr) {
	printf("ChnAttr->u32Width:%d \r\n", ChnAttr->u32Width);
	printf("ChnAttr->u32Height:%d \r\n", ChnAttr->u32Height);
	printf("ChnAttr->enVideoFormat:%d \r\n", ChnAttr->enVideoFormat);
	printf("ChnAttr->enPixelFormat:%d \r\n", ChnAttr->enPixelFormat);
	printf("ChnAttr->stFrameRate.s32SrcFrameRate:%d \r\n", ChnAttr->stFrameRate.s32SrcFrameRate);
	printf("ChnAttr->stFrameRate.s32DstFrameRate:%d \r\n", ChnAttr->stFrameRate.s32DstFrameRate);
	printf("ChnAttr->bMirror:%d \r\n", ChnAttr->bMirror);
	printf("ChnAttr->bFlip:%d \r\n", ChnAttr->bFlip);
	printf("ChnAttr->u32Depth:%d \r\n", ChnAttr->u32Depth);
	printf("ChnAttr->stAspectRatio.bEnable:%d \r\n", ChnAttr->stAspectRatio.enMode);
	printf("ChnAttr->stAspectRatio.u32BgColor:%d \r\n", ChnAttr->stAspectRatio.u32BgColor);
	printf("ChnAttr->stAspectRatio.bEnableBgColor:%d \r\n", ChnAttr->stAspectRatio.bEnableBgColor);
	printf("ChnAttr->stAspectRatio.stVideoRect.s32X:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.s32X);
	printf("ChnAttr->stAspectRatio.stVideoRect.s32Y:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.s32Y);
	printf("ChnAttr->stAspectRatio.stVideoRect.u32Width:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.u32Width);
	printf("ChnAttr->stAspectRatio.stVideoRect.u32Height:%d \r\n", ChnAttr->stAspectRatio.stVideoRect.u32Height);
	printf("ChnAttr->stNormalize.bEnable:%d \r\n", ChnAttr->stNormalize.bEnable);
	for (int i = 0; i < 3; i ++)
		printf("ChnAttr->stLumaScale.factor[%d]:%f \r\n", i, ChnAttr->stNormalize.factor[i]);
	for (int i = 0; i < 3; i ++)
		printf("ChnAttr->stLumaScale.mean[%d]:%f \r\n", i, ChnAttr->stNormalize.mean[i]);
	printf("ChnAttr->stNormalize.rounding:%d \r\n", ChnAttr->stNormalize.rounding);
}

void mmf_dump_frame(VIDEO_FRAME_INFO_S *frame) {
	if (frame == NULL) {
		printf("frame is null\n");
		return;
	}

	VIDEO_FRAME_S *vframe = &frame->stVFrame;
	printf("u32Width:\t\t%d\n", vframe->u32Width);
	printf("u32Height:\t\t%d\n", vframe->u32Height);
	printf("u32Stride[0]:\t\t%d\n", vframe->u32Stride[0]);
	printf("u32Stride[1]:\t\t%d\n", vframe->u32Stride[1]);
	printf("u32Stride[2]:\t\t%d\n", vframe->u32Stride[2]);
	printf("u32Length[0]:\t\t%d\n", vframe->u32Length[0]);
	printf("u32Length[1]:\t\t%d\n", vframe->u32Length[1]);
	printf("u32Length[2]:\t\t%d\n", vframe->u32Length[2]);
	printf("u64PhyAddr[0]:\t\t%#lx\n", vframe->u64PhyAddr[0]);
	printf("u64PhyAddr[1]:\t\t%#lx\n", vframe->u64PhyAddr[1]);
	printf("u64PhyAddr[2]:\t\t%#lx\n", vframe->u64PhyAddr[2]);
	printf("pu8VirAddr[0]:\t\t%p\n", vframe->pu8VirAddr[0]);
	printf("pu8VirAddr[1]:\t\t%p\n", vframe->pu8VirAddr[1]);
	printf("pu8VirAddr[2]:\t\t%p\n", vframe->pu8VirAddr[2]);

	printf("enPixelFormat:\t\t%d\n", vframe->enPixelFormat);
	printf("enBayerFormat:\t\t%d\n", vframe->enBayerFormat);
	printf("enVideoFormat:\t\t%d\n", vframe->enVideoFormat);
	printf("enCompressMode:\t\t%d\n", vframe->enCompressMode);
	printf("enDynamicRange:\t\t%d\n", vframe->enDynamicRange);
	printf("enColorGamut:\t\t%d\n", vframe->enColorGamut);

	printf("s16OffsetTop:\t\t%d\n", vframe->s16OffsetTop);
	printf("s16OffsetBottom:\t\t%d\n", vframe->s16OffsetBottom);
	printf("s16OffsetLeft:\t\t%d\n", vframe->s16OffsetLeft);
	printf("s16OffsetRight:\t\t%d\n", vframe->s16OffsetRight);
}

static int _free_leak_memory_of_ion(void)
{
	#define MAX_LINE_LENGTH 256
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char alloc_buf_size_str[20], phy_addr_str[20], buffer_name[20];
    int alloc_buf_size;
	uint64_t phy_addr;

    fp = fopen("/sys/kernel/debug/ion/cvi_carveout_heap_dump/summary", "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        if (sscanf(line, "%*d %s %s %*d %s", alloc_buf_size_str, phy_addr_str, buffer_name) == 3) {
			if (strcmp(buffer_name, "VI_DMA_BUF")
				&& strcmp(buffer_name, "ISP_SHARED_BUFFER_0"))
				continue;
			struct sys_ion_data2 ion_data = {
				.cached = 1,
				.dmabuf_fd = (uint32_t)-1,
			};

            alloc_buf_size = atoi(alloc_buf_size_str);
            phy_addr = (unsigned int)strtol(phy_addr_str, NULL, 16);

			ion_data.size = alloc_buf_size;
			ion_data.addr_p = phy_addr;
			memset(ion_data.name, 0, sizeof(ion_data.name));
			strcpy((char *)ion_data.name, buffer_name);

            printf("alloc_buf_size(%s): %d, phy_addr(%s): %#lx, buffer_name: %s\n",
						alloc_buf_size_str, alloc_buf_size, phy_addr_str, phy_addr, buffer_name);

			printf("ion_data.size:%d, ion_data.addr_p:%#x, ion_data.name:%s\r\n", ion_data.size, (int)ion_data.addr_p, ion_data.name);

			extern int ionFree2(struct sys_ion_data2 *para);
			int res = ionFree2(&ion_data);
			if (res) {
				printf("ionFree2 failed! res:%#x\r\n", res);
				mmf_deinit();
				return -1;
			}
        }
    }

    fclose(fp);

	return 0;
}


static VIDEO_FRAME_INFO_S *_mmf_alloc_frame(int id, SIZE_S stSize, PIXEL_FORMAT_E enPixelFormat)
{
	VIDEO_FRAME_INFO_S *pstVideoFrame;
	VIDEO_FRAME_S *pstVFrame;
	VB_BLK blk;
	VB_CAL_CONFIG_S stVbCfg;

	pstVideoFrame = (VIDEO_FRAME_INFO_S *)calloc(sizeof(*pstVideoFrame), 1);
	if (pstVideoFrame == NULL) {
		SAMPLE_PRT("Failed to allocate VIDEO_FRAME_INFO_S\n");
		return NULL;
	}

	memset(&stVbCfg, 0, sizeof(stVbCfg));
	VENC_GetPicBufferConfig(stSize.u32Width,
				stSize.u32Height,
				enPixelFormat,
				DATA_BITWIDTH_8,
				COMPRESS_MODE_NONE,
				&stVbCfg);

	pstVFrame = &pstVideoFrame->stVFrame;

	pstVFrame->enCompressMode = COMPRESS_MODE_NONE;
	pstVFrame->enPixelFormat = enPixelFormat;
	pstVFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
	pstVFrame->enColorGamut = COLOR_GAMUT_BT709;
	pstVFrame->u32Width = stSize.u32Width;
	pstVFrame->u32Height = stSize.u32Height;
	pstVFrame->u32TimeRef = 0;
	pstVFrame->u64PTS = 0;
	pstVFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

	blk = CVI_VB_GetBlock(id, stVbCfg.u32VBSize);
	if (blk == VB_INVALID_HANDLE) {
		SAMPLE_PRT("Can't acquire vb block. id: %d size:%d\n", id, stVbCfg.u32VBSize);
		free(pstVideoFrame);
		return NULL;
	}

	pstVideoFrame->u32PoolId = CVI_VB_Handle2PoolId(blk);
	pstVFrame->u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);
	pstVFrame->u32Stride[0] = stVbCfg.u32MainStride;
	pstVFrame->u32Length[0] = stVbCfg.u32MainYSize;
	pstVFrame->pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_MmapCache(pstVFrame->u64PhyAddr[0], stVbCfg.u32VBSize);

	if (stVbCfg.plane_num > 1) {
		pstVFrame->u64PhyAddr[1] = ALIGN(pstVFrame->u64PhyAddr[0] + stVbCfg.u32MainYSize, stVbCfg.u16AddrAlign);
		pstVFrame->u32Stride[1] = stVbCfg.u32CStride;
		pstVFrame->u32Length[1] = stVbCfg.u32MainCSize;
		pstVFrame->pu8VirAddr[1] = (CVI_U8 *)pstVFrame->pu8VirAddr[0] + pstVFrame->u32Length[0];
	}

	if (stVbCfg.plane_num > 2) {
		pstVFrame->u64PhyAddr[2] = ALIGN(pstVFrame->u64PhyAddr[1] + stVbCfg.u32MainCSize, stVbCfg.u16AddrAlign);
		pstVFrame->u32Stride[2] = stVbCfg.u32CStride;
		pstVFrame->u32Length[2] = stVbCfg.u32MainCSize;
		pstVFrame->pu8VirAddr[2] = (CVI_U8 *)pstVFrame->pu8VirAddr[1] + pstVFrame->u32Length[1];
	}

	// CVI_VENC_TRACE("phy addr(%#llx, %#llx, %#llx), Size %x\n", (long long)pstVFrame->u64PhyAddr[0]
	// 	, (long long)pstVFrame->u64PhyAddr[1], (long long)pstVFrame->u64PhyAddr[2], stVbCfg.u32VBSize);
	// CVI_VENC_TRACE("vir addr(%p, %p, %p), Size %x\n", pstVFrame->pu8VirAddr[0]
	// 	, pstVFrame->pu8VirAddr[1], pstVFrame->pu8VirAddr[2], stVbCfg.u32MainSize);

	return pstVideoFrame;
}

static CVI_S32 _mmf_free_frame(VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;
	VB_BLK blk;

	if (pstVFrame->pu8VirAddr[0])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[0], pstVFrame->u32Length[0]);
	if (pstVFrame->pu8VirAddr[1])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[1], pstVFrame->u32Length[1]);
	if (pstVFrame->pu8VirAddr[2])
		CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[2], pstVFrame->u32Length[2]);

	blk = CVI_VB_PhysAddr2Handle(pstVFrame->u64PhyAddr[0]);
	if (blk != VB_INVALID_HANDLE) {
		CVI_VB_ReleaseBlock(blk);
	}

	free(pstVideoFrame);

	return CVI_SUCCESS;
}

static int cvi_ive_init(void)
{
	CVI_S32 s32Ret;
	if (priv.ive_is_init)
		return 0;

	priv.ive_rgb2yuv_w = 640;
	priv.ive_rgb2yuv_h = 480;
	priv.ive_handle = CVI_IVE_CreateHandle();
	if (priv.ive_handle == NULL) {
		printf("CVI_IVE_CreateHandle failed!\n");
		return -1;
	}

	s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, IVE_IMAGE_TYPE_U8C3_PACKAGE, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
	if (s32Ret != CVI_SUCCESS) {
		printf("Create src image failed!\n");
		CVI_IVE_DestroyHandle(priv.ive_handle);
		return -1;
	}

	s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img, IVE_IMAGE_TYPE_YUV420SP, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
	if (s32Ret != CVI_SUCCESS) {
		printf("Create src image failed!\n");
		CVI_IVE_DestroyHandle(priv.ive_handle);
		return -1;
	}

	priv.ive_is_init = 1;
	return 0;
}

static int cvi_ive_deinit(void)
{
	if (priv.ive_is_init == 0)
		return 0;

	CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img);
	CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img);
	CVI_IVE_DestroyHandle(priv.ive_handle);

	priv.ive_is_init = 0;
	return 0;
}

static int cvi_rgb2nv21(uint8_t *src, int input_w, int input_h)
{
	CVI_S32 s32Ret;

	int width = ALIGN(input_w, DEFAULT_ALIGN);
	int height = input_h;

	if (width != priv.ive_rgb2yuv_w || height != priv.ive_rgb2yuv_h) {
		CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img);
		CVI_SYS_FreeI(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img);
		priv.ive_rgb2yuv_w = width;
		priv.ive_rgb2yuv_h = height;
		printf("reinit rgb2nv21 buffer, buff w:%d h:%d\n", priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, IVE_IMAGE_TYPE_U8C3_PACKAGE, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		if (s32Ret != CVI_SUCCESS) {
			printf("Create src image failed!\n");
			return -1;
		}

		s32Ret = CVI_IVE_CreateImage_Cached(priv.ive_handle, &priv.ive_rgb2yuv_yuv_img, IVE_IMAGE_TYPE_YUV420SP, priv.ive_rgb2yuv_w, priv.ive_rgb2yuv_h);
		if (s32Ret != CVI_SUCCESS) {
			printf("Create src image failed!\n");
			return -1;
		}
	}

	if (width != input_w) {
		for (int h = 0; h < height; h++) {
			memcpy((uint8_t *)priv.ive_rgb2yuv_rgb_img.u64VirAddr[0] + width * h * 3, (uint8_t *)src + input_w * h * 3, input_w * 3);
		}
	} else {
		memcpy((uint8_t *)priv.ive_rgb2yuv_rgb_img.u64VirAddr[0], (uint8_t *)src, width * height * 3);
	}

	IVE_CSC_CTRL_S stCtrl;
	stCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_RGB2YUV;
	s32Ret = CVI_IVE_CSC(priv.ive_handle, &priv.ive_rgb2yuv_rgb_img, &priv.ive_rgb2yuv_yuv_img, &stCtrl, 1);
	if (s32Ret != CVI_SUCCESS) {
		printf("Run HW IVE CSC YUV2RGB failed!\n");
		return -1;
	}
	return 0;
}

static int _try_release_sys(void)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	SAMPLE_INI_CFG_S	   	stIniCfg;
	SAMPLE_VI_CONFIG_S 		stViConfig;
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		SAMPLE_PRT("Parse complete\n");
		return s32Ret;
	}

	priv.sensor_type = stIniCfg.enSnsType[0];

	s32Ret = CVI_VI_SetDevNum(stIniCfg.devNum);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("VI_SetDevNum failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VI_IniToViCfg failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_DestroyIsp(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VI_DestroyIsp failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_DestroyVi(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VI_DestroyVi failed with %#x\n", s32Ret);
		return s32Ret;
	}

	SAMPLE_COMM_SYS_Exit();
	return s32Ret;
}

int _try_release_vio_all(void)
{
	CVI_S32 s32Ret = CVI_FAILURE;
	s32Ret = mmf_del_vi_channel_all();
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("mmf_del_vi_channel_all failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = mmf_del_vo_channel_all(0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("mmf_del_vo_channel_all failed with %#x\n", s32Ret);
		return s32Ret;
	}
	return s32Ret;
}

static void _mmf_sys_exit(void)
{
	if (g_stViConfig.s32WorkingViNum != 0) {
		SAMPLE_COMM_VI_DestroyIsp(&g_stViConfig);
		SAMPLE_COMM_VI_DestroyVi(&g_stViConfig);
	}
	SAMPLE_COMM_SYS_Exit();
}

static CVI_S32 _mmf_sys_init(SIZE_S stSize)
{
	VB_CONFIG_S	   stVbConf;
	CVI_U32        u32BlkSize, u32BlkRotSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;

	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	memcpy(&stVbConf, &priv.vb_conf, sizeof(VB_CONFIG_S));
	// stVbConf.u32MaxPoolCnt		= 3;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_YUYV,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, PIXEL_FORMAT_YUYV,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkSize = MAX(u32BlkSize, u32BlkRotSize);

	stVbConf.astCommPool[MMF_VB_VI_ID].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[MMF_VB_VI_ID].u32BlkCnt	= 3;
	stVbConf.astCommPool[MMF_VB_VI_ID].enRemapMode	= VB_REMAP_MODE_CACHED;
#if 0
{
	VB_CONFIG_S vb_config;
	s32Ret = CVI_VB_GetConfig(&vb_config);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VB_GetConfig NG\n");
		return CVI_FAILURE;
	}
	SAMPLE_PRT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	SAMPLE_PRT("vb_config.u32MaxPoolCnt :%d\r\n", stVbConf.u32MaxPoolCnt);
	for (int i = 0; i < stVbConf.u32MaxPoolCnt; ++i) {
		SAMPLE_PRT("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						stVbConf.astCommPool[i].u32BlkSize, \
						stVbConf.astCommPool[i].u32BlkCnt, \
						stVbConf.astCommPool[i].enRemapMode);
	}
	SAMPLE_PRT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	SAMPLE_PRT("vb_config.u32MaxPoolCnt :%d\r\n", vb_config.u32MaxPoolCnt);
	for (int i = 0; i < vb_config.u32MaxPoolCnt; ++i) {
		SAMPLE_PRT("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						vb_config.astCommPool[i].u32BlkSize, \
						vb_config.astCommPool[i].u32BlkCnt, \
						vb_config.astCommPool[i].enRemapMode);
	}
}
#endif

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("system init failed with %#x\n", s32Ret);
		goto error;
	}
#if 0
{
	VB_CONFIG_S vb_config;
	s32Ret = CVI_VB_GetConfig(&vb_config);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_VB_GetConfig NG\n");
		return CVI_FAILURE;
	}
	SAMPLE_PRT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>[%s[%d]\r\n", __func__, __LINE__);
	for (int i = 0; i < vb_config.u32MaxPoolCnt; ++i) {
		SAMPLE_PRT("vb_config.u32MaxPoolCnt :%d\r\n", vb_config.u32MaxPoolCnt);
		SAMPLE_PRT("common pool[%d] BlkSize(%d) BlkCnt(%d) Remap(%d)\n", \
						i, \
						vb_config.astCommPool[i].u32BlkSize, \
						vb_config.astCommPool[i].u32BlkCnt, \
						vb_config.astCommPool[i].enRemapMode);
	}
}
#endif
	return s32Ret;
error:
	_mmf_sys_exit();
	return s32Ret;
}

static CVI_S32 _mmf_vpss_deinit(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	CVI_BOOL           abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	CVI_S32 s32Ret = CVI_SUCCESS;

	/*start vpss*/
	abChnEnable[VpssChn] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
	}

	return s32Ret;
}

static CVI_S32 _mmf_vpss_deinit_new(VPSS_GRP VpssGrp)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_VPSS_StopGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("Vpss Stop Grp %d failed! Please check param\n", VpssGrp);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_DestroyGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("Vpss Destroy Grp %d failed! Please check\n", VpssGrp);
		return CVI_FAILURE;
	}

	return s32Ret;
}

// fit = 0, width to new width, height to new height, may be stretch
// fit = 1, keep aspect ratio, fill blank area with black color
// fit = other, keep aspect ratio, crop image to fit new size
static CVI_S32 _mmf_vpss_init(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SIZE_S stSizeIn, SIZE_S stSizeOut, PIXEL_FORMAT_E formatIn, PIXEL_FORMAT_E formatOut,
int fps, int depth, bool mirror, bool flip, int fit)
{
	VPSS_GRP_ATTR_S    stVpssGrpAttr;
	VPSS_CROP_INFO_S   stGrpCropInfo;
	CVI_BOOL           abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
	VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];
	CVI_S32 s32Ret = CVI_SUCCESS;

	memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = formatIn;
	stVpssGrpAttr.u32MaxW                        = stSizeIn.u32Width;
	stVpssGrpAttr.u32MaxH                        = stSizeIn.u32Height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	CVI_FLOAT corp_scale_w = (CVI_FLOAT)stSizeIn.u32Width / stSizeOut.u32Width;
	CVI_FLOAT corp_scale_h = (CVI_FLOAT)stSizeIn.u32Height / stSizeOut.u32Height;
	CVI_U32 crop_w = -1, crop_h = -1;
	if (fit == 0) {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.s32X       = 0;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.s32Y       = 0;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.u32Width   = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].stAspectRatio.stVideoRect.u32Height  = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		stGrpCropInfo.bEnable = false;
	} else if (fit == 1) {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		stGrpCropInfo.bEnable = false;
	} else {
		memset(astVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S) * VPSS_MAX_PHY_CHN_NUM);
		astVpssChnAttr[VpssChn].u32Width                    = stSizeOut.u32Width;
		astVpssChnAttr[VpssChn].u32Height                   = stSizeOut.u32Height;
		astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
		astVpssChnAttr[VpssChn].enPixelFormat               = formatOut;
		astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = fps;
		astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = fps;
		astVpssChnAttr[VpssChn].u32Depth                    = depth;
		astVpssChnAttr[VpssChn].bMirror                     = mirror;
		astVpssChnAttr[VpssChn].bFlip                       = flip;
		astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
		astVpssChnAttr[VpssChn].stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

		crop_w = corp_scale_w < corp_scale_h ? stSizeOut.u32Width * corp_scale_w: stSizeOut.u32Width * corp_scale_h;
		crop_h = corp_scale_w < corp_scale_h ? stSizeOut.u32Height * corp_scale_w: stSizeOut.u32Height * corp_scale_h;
		if (corp_scale_h < 0 || corp_scale_w < 0) {
			SAMPLE_PRT("crop scale error. corp_scale_w: %f, corp_scale_h: %f\n", corp_scale_w, corp_scale_h);
			goto error;
		}

		stGrpCropInfo.bEnable = true;
		stGrpCropInfo.stCropRect.s32X = (stSizeIn.u32Width - crop_w) / 2;
		stGrpCropInfo.stCropRect.s32Y = (stSizeIn.u32Height - crop_h) / 2;
		stGrpCropInfo.stCropRect.u32Width = crop_w;
		stGrpCropInfo.stCropRect.u32Height = crop_h;
	}

	/*start vpss*/
	abChnEnable[0] = CVI_TRUE;
	s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("init vpss group failed. s32Ret: 0x%x ! retry!!!\n", s32Ret);
		s32Ret = SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("stop vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		}
		s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("retry to init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
			return s32Ret;
		} else {
			SAMPLE_PRT("retry to init vpss group ok!\n");
		}
	}

	if (crop_w != 0 && crop_h != 0) {
		s32Ret = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stGrpCropInfo);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("set vpss group crop failed. s32Ret: 0x%x !\n", s32Ret);
			goto error;
		}
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		goto error;
	}

	return s32Ret;
error:
	_mmf_vpss_deinit(VpssGrp, VpssChn);
	return s32Ret;
}

static CVI_S32 _mmf_init(void)
{
	MMF_VERSION_S stVersion;
	SAMPLE_INI_CFG_S	   stIniCfg;
	SAMPLE_VI_CONFIG_S stViConfig;

	PIC_SIZE_E enPicSize;
	SIZE_S stSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	LOG_LEVEL_CONF_S log_conf;

	CVI_SYS_GetVersion(&stVersion);
	SAMPLE_PRT("maix multi-media version:%s\n", stVersion.version);

	log_conf.enModId = CVI_ID_LOG;
	log_conf.s32Level = CVI_DBG_DEBUG;
	CVI_LOG_SetLevelConf(&log_conf);

	// Get config from ini if found.
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		SAMPLE_PRT("Parse complete\n");
	}

	//Set sensor number
	CVI_VI_SetDevNum(stIniCfg.devNum);

	/************************************************
	 * step1:  Config VI
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	memcpy(&g_stViConfig, &stViConfig, sizeof(SAMPLE_VI_CONFIG_S));
	memcpy(&g_stIniCfg, &stIniCfg, sizeof(SAMPLE_INI_CFG_S));

	/************************************************
	 * step2:  Get input size
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType[0], &enPicSize);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
		return s32Ret;
	}

	/************************************************
	 * step3:  Init modules
	 ************************************************/
	if (0 != _free_leak_memory_of_ion()) {
		SAMPLE_PRT("free leak memory error\n");
	}

	s32Ret = _mmf_sys_init(stSize);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("sys init failed. s32Ret: 0x%x !\n", s32Ret);
		goto _need_exit_sys_and_deinit_vi;
	}

	s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("vi init failed. s32Ret: 0x%x !\n", s32Ret);
		SAMPLE_PRT("Please try to check if the camera is working.\n");
		goto _need_exit_sys_and_deinit_vi;
	}

	priv.vi_size.u32Width = stSize.u32Width;
	priv.vi_size.u32Height = stSize.u32Height;

	return s32Ret;

_need_exit_sys_and_deinit_vi:
	_mmf_sys_exit();

	return s32Ret;
}

static void _mmf_deinit(void)
{
	_mmf_sys_exit();
}

static int _vi_get_unused_ch() {
	for (int i = 0; i < MMF_VI_MAX_CHN; i++) {
		if (priv.vi_chn_is_inited[i] == false) {
			return i;
		}
	}
	return -1;
}

int mmf_init(void)
{
    if (priv.mmf_used_cnt) {
		priv.mmf_used_cnt ++;
        printf("maix multi-media already inited(cnt:%d)\n", priv.mmf_used_cnt);
        return 0;
    }

	priv_param_init();

	if (_try_release_sys() != CVI_SUCCESS) {
		printf("try release sys failed\n");
		return -1;
	} else {
		printf("try release sys ok\n");
	}

    if (_mmf_init() != CVI_SUCCESS) {
        printf("maix multi-media init failed\n");
        return -1;
    } else {
		printf("maix multi-media init ok\n");
	}

#if MMF_VO_USE_NV21_ONLY
	if (cvi_ive_init() != CVI_SUCCESS) {
		printf("cvi_ive_init failed\n");
		return -1;
	} else {
		printf("cvi_ive_init ok\n");
	}
#else
	UNUSED(cvi_ive_init);
#endif
	priv.mmf_used_cnt = 1;

	if (_try_release_vio_all() != CVI_SUCCESS) {
		printf("try release vio failed\n");
		return -1;
	} else {
		printf("try release vio ok\n");
	}
    return 0;
}

bool mmf_is_init(void)
{
    return priv.mmf_used_cnt > 0 ? true : false;
}

int mmf_deinit(void) {
    if (!priv.mmf_used_cnt) {
        return 0;
    }

	priv.mmf_used_cnt --;

	if (priv.mmf_used_cnt) {
		return 0;
	} else {
		printf("maix multi-media driver destroyed.\n");
#if MMF_VO_USE_NV21_ONLY
		cvi_ive_deinit();
#else
		UNUSED(cvi_ive_deinit);
#endif

		mmf_del_vi_channel_all();
		mmf_del_vo_channel_all(0);
		mmf_enc_jpg_deinit(0);
		mmf_enc_h265_deinit(0);
		mmf_vi_deinit();
		mmf_del_region_channel_all();
		_mmf_deinit();
	}
    return 0;
}

int mmf_get_vi_unused_channel(void) {
	return _vi_get_unused_ch();
}

static CVI_S32 _mmf_vpss_chn_init(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, int width, int height, PIXEL_FORMAT_E format, int fps, int depth, bool mirror, bool flip, int fit)
{
#if 1
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CROP_INFO_S   stChnCropInfo;
	VPSS_CHN_ATTR_S chn_attr = {0};
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = CVI_VPSS_GetGrpAttr(VpssGrp, &stGrpAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_GetGrpAttr failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}
	CVI_FLOAT corp_scale_w = (CVI_FLOAT)stGrpAttr.u32MaxW / width;
	CVI_FLOAT corp_scale_h = (CVI_FLOAT)stGrpAttr.u32MaxH / height;
	CVI_U32 crop_w = -1, crop_h = -1;
	if (fit == 0) {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
		chn_attr.stAspectRatio.stVideoRect.s32X       = 0;
		chn_attr.stAspectRatio.stVideoRect.s32Y       = 0;
		chn_attr.stAspectRatio.stVideoRect.u32Width   = width;
		chn_attr.stAspectRatio.stVideoRect.u32Height  = height;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		stChnCropInfo.bEnable = false;
	} else if (fit == 1) {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		stChnCropInfo.bEnable = false;
	} else {
		chn_attr.u32Width                    = width;
		chn_attr.u32Height                   = height;
		chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
		chn_attr.enPixelFormat               = format;
		chn_attr.stFrameRate.s32SrcFrameRate = fps;
		chn_attr.stFrameRate.s32DstFrameRate = fps;
		chn_attr.u32Depth                    = depth;
		chn_attr.bMirror                     = mirror;
		chn_attr.bFlip                       = flip;
		chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_AUTO;
		chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
		chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
		chn_attr.stNormalize.bEnable         = CVI_FALSE;

		crop_w = corp_scale_w < corp_scale_h ? width * corp_scale_w: width * corp_scale_h;
		crop_h = corp_scale_w < corp_scale_h ? height * corp_scale_w: height * corp_scale_h;
		if (corp_scale_h < 0 || corp_scale_w < 0) {
			SAMPLE_PRT("crop scale error. corp_scale_w: %f, corp_scale_h: %f\n", corp_scale_w, corp_scale_h);
			return -1;
		}

		stChnCropInfo.bEnable = true;
		stChnCropInfo.stCropRect.s32X = (stGrpAttr.u32MaxW - crop_w) / 2;
		stChnCropInfo.stCropRect.s32Y = (stGrpAttr.u32MaxH - crop_h) / 2;
		stChnCropInfo.stCropRect.u32Width = crop_w;
		stChnCropInfo.stCropRect.u32Height = crop_h;
	}

	if (crop_w != 0 && crop_h != 0) {
		s32Ret = CVI_VPSS_SetChnCrop(VpssGrp, VpssChn, &stChnCropInfo);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("set vpss group crop failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}
	}

	s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	return s32Ret;
#else
	CVI_S32 s32Ret;
	VPSS_CHN_ATTR_S chn_attr = {0};
	chn_attr.u32Width                    = width;
	chn_attr.u32Height                   = height;
	chn_attr.enVideoFormat               = VIDEO_FORMAT_LINEAR;
	chn_attr.enPixelFormat               = format;
	chn_attr.stFrameRate.s32SrcFrameRate = fps;
	chn_attr.stFrameRate.s32DstFrameRate = fps;
	chn_attr.u32Depth                    = depth;
	chn_attr.bMirror                     = mirror;
	chn_attr.bFlip                       = flip;
	chn_attr.stAspectRatio.enMode        = ASPECT_RATIO_MANUAL;
	chn_attr.stAspectRatio.stVideoRect.s32X       = 0;
	chn_attr.stAspectRatio.stVideoRect.s32Y       = 0;
	chn_attr.stAspectRatio.stVideoRect.u32Width   = width;
	chn_attr.stAspectRatio.stVideoRect.u32Height  = height;
	chn_attr.stAspectRatio.bEnableBgColor = CVI_TRUE;
	chn_attr.stAspectRatio.u32BgColor    = COLOR_RGB_BLACK;
	chn_attr.stNormalize.bEnable         = CVI_FALSE;

	s32Ret = CVI_VPSS_SetChnAttr(VpssGrp, VpssChn, &chn_attr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_EnableChn(VpssGrp, VpssChn);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
#endif
}

static CVI_S32 _mmf_vpss_chn_deinit(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	return CVI_VPSS_DisableChn(VpssGrp, VpssChn);
}

static CVI_S32 _mmf_vpss_init_new(VPSS_GRP VpssGrp, CVI_U32 width, CVI_U32 height, PIXEL_FORMAT_E format)
{
	VPSS_GRP_ATTR_S    stVpssGrpAttr;
	CVI_S32 s32Ret = CVI_SUCCESS;
	VPSS_CHN_ATTR_S astVpssChnAttr;

	memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
	stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
	stVpssGrpAttr.enPixelFormat                  = format;
	stVpssGrpAttr.u32MaxW                        = width;
	stVpssGrpAttr.u32MaxH                        = height;
	stVpssGrpAttr.u8VpssDev                      = 0;

	astVpssChnAttr.stFrameRate.s32SrcFrameRate = 60;
	astVpssChnAttr.stFrameRate.s32DstFrameRate = 60;


	s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_CreateGrp(grp:%d) failed with %#x, retry!\n", VpssGrp, s32Ret);
		CVI_VPSS_DestroyGrp(VpssGrp);

		s32Ret = CVI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
			return CVI_FAILURE;
		}
	}

	CVI_VPSS_SetChnAttr(VpssGrp, VPSS_CHN0, &astVpssChnAttr);

	s32Ret = CVI_VPSS_ResetGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_ResetGrp(grp:%d) failed with %#x!%d\n", VpssGrp, s32Ret, CVI_ERR_VPSS_ILLEGAL_PARAM);
		return CVI_FAILURE;
	}

	s32Ret = CVI_VPSS_StartGrp(VpssGrp);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

int mmf_vi_init(void)
{
	if (priv.vi_is_inited) {
		return 0;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	s32Ret = _mmf_vpss_init_new(0, priv.vi_size.u32Width, priv.vi_size.u32Height, PIXEL_FORMAT_UYVY); 
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
	}

	priv.vi_is_inited = true;

	return s32Ret;
}

int mmf_vi_deinit(void)
{
	if (!priv.vi_is_inited) {
		return 0;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	s32Ret = _mmf_vpss_deinit_new(0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("_mmf_vpss_deinit_new failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.vi_is_inited = false;

	return s32Ret;
}

int mmf_add_vi_channel(int ch, int width, int height, int format) {
	if (!priv.mmf_used_cnt || !priv.vi_is_inited) {
		printf("%s: maix multi-media or vi not inited\n", __func__);
		return -1;
	}

	if (width <= 0 || height <= 0) {
		printf("invalid width or height\n");
		return -1;
	}

	if (format != PIXEL_FORMAT_NV21
		&& format != PIXEL_FORMAT_RGB_888) {
		printf("invalid format\n");
		return -1;
	}

	if ((format == PIXEL_FORMAT_RGB_888 && width * height * 3 > 640 * 480 * 3)
		|| (format == PIXEL_FORMAT_RGB_888 && width * height * 3 / 2 > 2560 * 1440 * 3 / 2)) {
		printf("image size is too large, for NV21, maximum resolution 2560x1440, for RGB888, maximum resolution 640x480!\n");
		return -1;
	}

	if (mmf_vi_chn_is_open(ch)) {
		printf("vi ch %d already open\n", ch);
		return -1;
	}

#if 0
	CVI_S32 s32Ret = CVI_SUCCESS;
	SIZE_S stSizeIn, stSizeOut;
	int fps = 30;
	int depth = 2;
	PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
	PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)format;
	stSizeIn.u32Width   = priv.vi_size.u32Width;
	stSizeIn.u32Height  = priv.vi_size.u32Height;
	stSizeOut.u32Width  = ALIGN(width, DEFAULT_ALIGN);
	stSizeOut.u32Height = height;
	bool mirror = !g_priv.vi_hmirror[ch];
	bool flip = !g_priv.vi_vflip[ch];
	s32Ret = _mmf_vpss_init(0, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, 2);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("vpss init failed. s32Ret: 0x%x. try again..\r\n", s32Ret);
		s32Ret = _mmf_vpss_deinit(0, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}

		s32Ret = _mmf_vpss_init(0, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, 2);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
			return -1;
		}
	}

	priv.vi_chn_is_inited[ch] = true;
	return 0;
_need_deinit_vpss:
	_mmf_vpss_deinit(0, ch);
	return -1;
#else
	CVI_S32 s32Ret = CVI_SUCCESS;
	int fps = 30;
	int depth = 2;
	int width_out = ALIGN(width, DEFAULT_ALIGN);
	int height_out = height;
	PIXEL_FORMAT_E format_out = (PIXEL_FORMAT_E)format;
	bool mirror = !g_priv.vi_hmirror[ch];
	bool flip = !g_priv.vi_vflip[ch];
	s32Ret = _mmf_vpss_chn_deinit(0, ch);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = _mmf_vpss_chn_init(0, ch, width_out, height_out, format_out, fps, depth, mirror, flip, 2);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, ch, 0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		goto _need_deinit_vpss_chn;
	}

	priv.vi_chn_is_inited[ch] = true;

	return 0;
_need_deinit_vpss_chn:
	_mmf_vpss_chn_deinit(0, ch);
	return -1;
#endif
}

int mmf_del_vi_channel(int ch) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return -1;
	}

	if (priv.vi_chn_is_inited[ch] == false) {
		printf("vi ch %d not open\n", ch);
		return -1;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;
	s32Ret = SAMPLE_COMM_VI_UnBind_VPSS(0, ch, 0);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		// return -1; // continue to deinit vpss
	}

	if (0 != _mmf_vpss_chn_deinit(0, ch)) {
		SAMPLE_PRT("_mmf_vpss_chn_deinit failed. s32Ret: 0x%x !\n", s32Ret);
	}

	priv.vi_chn_is_inited[ch] = false;
	return s32Ret;
}

int mmf_del_vi_channel_all() {
	for (int i = 0; i < MMF_VI_MAX_CHN; i++) {
		if (priv.vi_chn_is_inited[i] == true) {
			mmf_del_vi_channel(i);
		}
	}
	return 0;
}

bool mmf_vi_chn_is_open(int ch) {
	if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
		printf("invalid ch %d\n", ch);
		return false;
	}

	return priv.vi_chn_is_inited[ch];
}

int mmf_reset_vi_channel(int ch, int width, int height, int format)
{
	mmf_del_vi_channel(ch);
	return mmf_add_vi_channel(ch, width, height, format);
}

int mmf_vi_aligned_width(int ch) {
	UNUSED(ch);
	return DEFAULT_ALIGN;
}

int mmf_vi_frame_pop(int ch, void **data, int *len, int *width, int *height, int *format) {
	if (!priv.vi_chn_is_inited[ch]) {
        printf("vi ch %d not open\n", ch);
        return -1;
    }
    if (ch < 0 || ch >= MMF_VI_MAX_CHN) {
        printf("invalid ch %d\n", ch);
        return -1;
    }
    if (data == NULL || len == NULL || width == NULL || height == NULL || format == NULL) {
        printf("invalid param\n");
        return -1;
    }

	int ret = -1;
	VIDEO_FRAME_INFO_S *frame = &priv.vi_frame[ch];
	if (CVI_VPSS_GetChnFrame(0, ch, frame, 3000) == 0) {
        int image_size = frame->stVFrame.u32Length[0]
                        + frame->stVFrame.u32Length[1]
				        + frame->stVFrame.u32Length[2];
        CVI_VOID *vir_addr;
        vir_addr = CVI_SYS_MmapCache(frame->stVFrame.u64PhyAddr[0], image_size);
        CVI_SYS_IonInvalidateCache(frame->stVFrame.u64PhyAddr[0], vir_addr, image_size);

		frame->stVFrame.pu8VirAddr[0] = (CVI_U8 *)vir_addr;		// save virtual address for munmap
		// printf("width: %d, height: %d, total_buf_length: %d, phy:%#lx  vir:%p\n",
		// 	   frame->stVFrame.u32Width,
		// 	   frame->stVFrame.u32Height, image_size,
        //        frame->stVFrame.u64PhyAddr[0], vir_addr);

		*data = vir_addr;
        *len = image_size;
        *width = frame->stVFrame.u32Width;
        *height = frame->stVFrame.u32Height;
        *format = frame->stVFrame.enPixelFormat;
		return 0;
    }
	return ret;
}

void mmf_vi_frame_free(int ch) {
	VIDEO_FRAME_INFO_S *frame = &priv.vi_frame[ch];
	int image_size = frame->stVFrame.u32Length[0]
                        + frame->stVFrame.u32Length[1]
				        + frame->stVFrame.u32Length[2];
	CVI_SYS_Munmap(frame->stVFrame.pu8VirAddr[0], image_size);
	if (CVI_VPSS_ReleaseChnFrame(0, ch, frame) != 0)
			SAMPLE_PRT("CVI_VI_ReleaseChnFrame NG\n");
}

// manage vo channels
int mmf_get_vo_unused_channel(int layer) {
	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		for (int i = 0; i < MMF_VO_VIDEO_MAX_CHN; i++) {
			if (priv.vo_video_chn_is_inited[i] == false) {
				return i;
			}
		}
		break;
	case MMF_VO_OSD_LAYER:
		return mmf_get_region_unused_channel();
	default:
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return -1;
}

// fit = 0, width to new width, height to new height, may be stretch
// fit = 1, keep aspect ratio, fill blank area with black color
// fit = 2, keep aspect ratio, crop image to fit new size
int mmf_add_vo_channel(int layer, int ch, int width, int height, int format, int fit) {
	bool mirror, flip;
	if (layer == MMF_VO_VIDEO_LAYER) {
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}

		SAMPLE_VO_CONFIG_S stVoConfig;
		RECT_S stDefDispRect  = {0, 0, (CVI_U32)width, (CVI_U32)height};
		SIZE_S stDefImageSize = {(CVI_U32)width, (CVI_U32)height};
		CVI_S32 s32Ret = CVI_SUCCESS;
		CVI_U32 panel_init = false;
		VO_PUB_ATTR_S stVoPubAttr;

	#if !MMF_VO_USE_NV21_ONLY
		if (priv.vo_rotate == 90 || priv.vo_rotate == 270) {
			stDefDispRect.u32Width = (CVI_U32)height;
			stDefDispRect.u32Height = (CVI_U32)width;
			stDefImageSize.u32Width = (CVI_U32)height;
			stDefImageSize.u32Height = (CVI_U32)width;
		}
	#else
		if (format == PIXEL_FORMAT_NV21 && (priv.vo_rotate == 90 || priv.vo_rotate == 270)) {
			stDefDispRect.u32Width = (CVI_U32)height;
			stDefDispRect.u32Height = (CVI_U32)width;
			stDefImageSize.u32Width = (CVI_U32)height;
			stDefImageSize.u32Height = (CVI_U32)width;
		}
	#endif
		SIZE_S stSizeIn, stSizeOut;
		int fps = 30;
		int depth = 0;
		PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)format;
		PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;

		CVI_VO_Get_Panel_Status(0, ch, &panel_init);
		if (panel_init) {
			CVI_VO_GetPubAttr(0, &stVoPubAttr);
			SAMPLE_PRT("Panel w=%d, h=%d.\n",\
					stVoPubAttr.stSyncInfo.u16Hact, stVoPubAttr.stSyncInfo.u16Vact);
			stDefDispRect.u32Width = stVoPubAttr.stSyncInfo.u16Hact;
			stDefDispRect.u32Height = stVoPubAttr.stSyncInfo.u16Vact;
			stDefImageSize.u32Width = stVoPubAttr.stSyncInfo.u16Hact;
			stDefImageSize.u32Height = stVoPubAttr.stSyncInfo.u16Vact;
		}
		s32Ret = SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("SAMPLE_COMM_VO_GetDefConfig failed with %#x\n", s32Ret);
			return -1;
		}

		stVoConfig.VoDev	 = 0;
		stVoConfig.stVoPubAttr.enIntfType  = VO_INTF_MIPI;
		stVoConfig.stVoPubAttr.enIntfSync  = VO_OUTPUT_720x1280_60;
		stVoConfig.stDispRect	 = stDefDispRect;
		stVoConfig.stImageSize	 = stDefImageSize;
		stVoConfig.enPixFormat	 = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
		stVoConfig.enVoMode	 = VO_MODE_1MUX;
		s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %#x\n", s32Ret);
			return -1;
		}

		memcpy(&priv.vo_video_cfg[ch], &stVoConfig, sizeof(SAMPLE_VO_CONFIG_S));

		switch (priv.vo_rotate) {
			case 0:break;
			case 90:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_90);
				break;
			case 180:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_180);
				break;
			case 270:
				CVI_VO_SetChnRotation(layer, ch, ROTATION_270);
				break;
			default:
				break;
		}

		stSizeIn.u32Width   = width;
		stSizeIn.u32Height  = height;
		stSizeOut.u32Width  = width;
		stSizeOut.u32Height = height;
		priv.vo_vpss_in_format[ch] = format;
		priv.vo_vpss_in_size[ch].u32Width = stSizeIn.u32Width;
		priv.vo_vpss_in_size[ch].u32Height = stSizeIn.u32Height;
		priv.vo_vpss_out_size[ch].u32Width = stSizeOut.u32Width;
		priv.vo_vpss_out_size[ch].u32Height = stSizeOut.u32Height;
		priv.vo_vpss_fit[ch] = fit;
		mirror = !g_priv.vo_video_hmirror[ch];
		flip = !g_priv.vo_video_vflip[ch];
#if 1
		s32Ret = _mmf_vpss_deinit_new(1);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_stop_vo;
		}

		s32Ret = _mmf_vpss_init_new(1, stSizeIn.u32Width, stSizeIn.u32Height, formatIn);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_stop_vo;
		}

		s32Ret = _mmf_vpss_chn_deinit(1, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		s32Ret = _mmf_vpss_chn_init(1, ch, stSizeOut.u32Width, stSizeOut.u32Height, formatOut, fps, depth, mirror, flip, fit);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VO(1, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_deinit_vpss_chn;
		}

		// if (priv.vo_video_pre_frame[ch]) {
		// 	_mmf_free_frame(priv.vo_video_pre_frame[ch]);
		// 	priv.vo_video_pre_frame[ch] = NULL;
		// 	priv.vo_video_pre_frame_width[ch] = -1;
		// 	priv.vo_video_pre_frame_height[ch] = -1;
		// 	priv.vo_video_pre_frame_format[ch] = -1;
		// }

		priv.vo_video_pre_frame_width[ch] = width;
		priv.vo_video_pre_frame_height[ch] = height;
		priv.vo_video_pre_frame_format[ch] = format;
		// priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format);
		// if (!priv.vo_video_pre_frame[ch]) {
		// 	printf("Alloc frame failed!\r\n");
		// 	goto error_and_unbind;
		// }

		priv.vo_video_chn_is_inited[ch] = true;

		return s32Ret;
// error_and_unbind:
// 		s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(1, ch, layer, ch);
// 		if (s32Ret != CVI_SUCCESS) {
// 			SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
// 		}
error_and_deinit_vpss:
		s32Ret = _mmf_vpss_deinit_new(1);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_deinit_vpss_chn:
		s32Ret = _mmf_vpss_chn_deinit(1, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_stop_vo:
		s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		}
		return -1;
#else
		s32Ret = _mmf_vpss_init(1, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vpss init failed. s32Ret: 0x%x. try again..\r\n", s32Ret);
			s32Ret = _mmf_vpss_deinit(1, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
				goto error_and_stop_vo;
			}

			s32Ret = _mmf_vpss_init(1, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
				goto error_and_stop_vo;
			}
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VO(1, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
			goto error_and_deinit_vpss;
		}

		if (priv.vo_video_pre_frame[ch]) {
			_mmf_free_frame(priv.vo_video_pre_frame[ch]);
			priv.vo_video_pre_frame[ch] = NULL;
			priv.vo_video_pre_frame_width[ch] = -1;
			priv.vo_video_pre_frame_height[ch] = -1;
			priv.vo_video_pre_frame_format[ch] = -1;
		}

		priv.vo_video_pre_frame_width[ch] = width;
		priv.vo_video_pre_frame_height[ch] = height;
		priv.vo_video_pre_frame_format[ch] = format;
		priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format);
		if (!priv.vo_video_pre_frame[ch]) {
			printf("Alloc frame failed!\r\n");
			goto error_and_unbind;
		}

		priv.vo_video_chn_is_inited[ch] = true;

		return s32Ret;
error_and_stop_vo:
		s32Ret = SAMPLE_COMM_VO_StopVO(&stVoConfig);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_unbind:
		s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(1, ch, layer, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
		}
error_and_deinit_vpss:
		s32Ret = _mmf_vpss_deinit(1, ch);
		if (s32Ret != CVI_SUCCESS) {
			SAMPLE_PRT("vpss deinit failed. s32Ret: 0x%x !\n", s32Ret);
		}
		return -1;
#endif
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (fit != 0) {
			printf("region support fit = 0 only!\r\n");
			return false;
		}

		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}

		if (priv.vo_osd_chn_is_inited[ch]) {
			printf("vo osd ch %d already open\n", ch);
			return -1;
		}

		if (format != PIXEL_FORMAT_ARGB_8888) {
			printf("only support ARGB format.\n");
			return -1;
		}

		if (0 != mmf_add_region_channel(ch, OVERLAY_RGN, CVI_ID_VPSS, 1, ch, 0, 0, width, height, format)) {
			printf("mmf_add_region_channel failed!\r\n");
			return -1;
		}

		priv.vo_osd_chn_is_inited[ch] = true;

		return 0;
	} else {
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return -1;
}

int mmf_del_vo_channel(int layer, int ch) {
	if (layer == MMF_VO_VIDEO_LAYER) {
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return CVI_FALSE;
		}

		if (priv.vo_video_chn_is_inited[ch] == false) {
			return CVI_SUCCESS;
		}

		if (CVI_SUCCESS != SAMPLE_COMM_VPSS_UnBind_VO(1, ch, layer, ch)) {
			SAMPLE_PRT("vi unbind vpss failed.!\n");
		}
#if 0
		if (0 != _mmf_vpss_deinit(1, ch)) {
			SAMPLE_PRT("vpss deinit failed.!\n");
		}
#else
		if (0 != _mmf_vpss_chn_deinit(1, ch)) {
			SAMPLE_PRT("_mmf_vpss_chn_deinit failed!\n");
		}

		if (0 != _mmf_vpss_deinit_new(1)) {
			SAMPLE_PRT("_mmf_vpss_deinit_new failed!\n");
		}
#endif
		if (CVI_SUCCESS != SAMPLE_COMM_VO_StopVO(&priv.vo_video_cfg[ch])) {
			SAMPLE_PRT("SAMPLE_COMM_VO_StopVO failed with %#x\n", CVI_FAILURE);
			return CVI_FAILURE;
		}

		// if (priv.vo_video_pre_frame[ch]) {
		// 	_mmf_free_frame(priv.vo_video_pre_frame[ch]);
		// 	priv.vo_video_pre_frame_width[ch] = -1;
		// 	priv.vo_video_pre_frame_height[ch] = -1;
		// 	priv.vo_video_pre_frame_format[ch] = -1;
		// 	priv.vo_video_pre_frame[ch] = NULL;
		// }

		priv.vo_video_chn_is_inited[ch] = false;
		return CVI_SUCCESS;
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return CVI_FALSE;
		}

		if (priv.vo_osd_chn_is_inited[ch] == false) {
			return CVI_SUCCESS;
		}

		if (0 != mmf_del_region_channel(ch)) {
			printf("mmf_del_region_channel failed!\r\n");
		}

		priv.vo_osd_chn_is_inited[ch] = false;
		return CVI_SUCCESS;
	} else {
		printf("invalid layer %d\n", layer);
		return CVI_FAILURE;
	}

	return CVI_FAILURE;
}

int mmf_del_vo_channel_all(int layer) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		for (int i = 0; i < MMF_VO_VIDEO_MAX_CHN; i++) {
			if (priv.vo_video_chn_is_inited[i] == true) {
				s32Ret = mmf_del_vo_channel(layer, i);
				if (s32Ret != CVI_SUCCESS) {
					SAMPLE_PRT("mmf_del_vo_channel failed with %#x\n", s32Ret);
					// return s32Ret; // continue to del other chn
				}
			}
		}
		break;
	case MMF_VO_OSD_LAYER:
		for (int i = 0; i < MMF_VO_OSD_MAX_CHN; i++) {
			if (priv.vo_osd_chn_is_inited[i] == true) {
				s32Ret = mmf_del_vo_channel(layer, i);
				if (s32Ret != CVI_SUCCESS) {
					SAMPLE_PRT("mmf_del_vo_channel failed with %#x\n", s32Ret);
					// return s32Ret; // continue to del other chn
				}
			}
		}
		break;
	default:
		printf("invalid layer %d\n", layer);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

bool mmf_vo_channel_is_open(int layer, int ch) {

	switch (layer) {
	case MMF_VO_VIDEO_LAYER:
		if (ch < 0 || ch >= MMF_VO_VIDEO_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}
		return priv.vo_video_chn_is_inited[ch];
	case MMF_VO_OSD_LAYER:
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return false;
		}
		return priv.vo_osd_chn_is_inited[ch];
	default:
		printf("invalid layer %d\n", layer);
		return false;
	}

	return false;
}

// flush vo
int mmf_vo_frame_push(int layer, int ch, void *data, int len, int width, int height, int format, int fit) {
	CVI_S32 s32Ret = CVI_SUCCESS;
	UNUSED(len);
	UNUSED(layer);
	UNUSED(cvi_rgb2nv21);

	if (layer == MMF_VO_VIDEO_LAYER) {
		if (fit != priv.vo_vpss_fit[ch]
		|| width != (int)priv.vo_vpss_in_size[ch].u32Width
		|| height != (int)priv.vo_vpss_in_size[ch].u32Height
		|| format != (int)priv.vo_vpss_in_format[ch]) {
#if 1
			priv.vo_vpss_in_format[ch] = format;
			priv.vo_vpss_in_size[ch].u32Width = width;
			priv.vo_vpss_in_size[ch].u32Height = height;
			priv.vo_vpss_fit[ch] = fit;
			int width_out = priv.vo_vpss_out_size[ch].u32Width;
			int height_out = priv.vo_vpss_out_size[ch].u32Height;
			int fps_out = priv.vo_vpss_out_fps[ch];
			int depth_out = priv.vo_vpss_out_depth[ch];
			int mirror_out = !g_priv.vo_video_hmirror[ch];
			int flip_out = !g_priv.vo_video_vflip[ch];
			s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(1, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_deinit_new(1);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("_mmf_vpss_deinit_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_init_new(1, width, height, (PIXEL_FORMAT_E)format);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("_mmf_vpss_init_new failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_deinit(1, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("_mmf_vpss_chn_deinit failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = _mmf_vpss_chn_init(1, ch, width_out, height_out, PIXEL_FORMAT_NV21, fps_out, depth_out, mirror_out, flip_out, fit);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("_mmf_vpss_chn_init failed with %#x!\n", s32Ret);
				return -1;
			}

			s32Ret = SAMPLE_COMM_VPSS_Bind_VO(1, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}
#else
			s32Ret = SAMPLE_COMM_VPSS_UnBind_VO(1, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vi unbind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			SIZE_S stSizeIn, stSizeOut;
			int fps = 30;
			int depth = 0;
			PIXEL_FORMAT_E formatIn = (PIXEL_FORMAT_E)format;
			PIXEL_FORMAT_E formatOut = (PIXEL_FORMAT_E)PIXEL_FORMAT_NV21;
			stSizeIn.u32Width   = width;
			stSizeIn.u32Height  = height;
			stSizeOut.u32Width  = priv.vo_vpss_out_size[ch].u32Width;
			stSizeOut.u32Height = priv.vo_vpss_out_size[ch].u32Height;
			priv.vo_vpss_in_size[ch].u32Width = stSizeIn.u32Width;
			priv.vo_vpss_in_size[ch].u32Height = stSizeIn.u32Height;
			priv.vo_vpss_fit[ch] = fit;
			_mmf_vpss_deinit(1, ch);
			bool mirror = !g_priv.vo_video_hmirror[ch];
			bool flip = !g_priv.vo_video_vflip[ch];
			int fit = priv.vo_vpss_fit[ch];
			s32Ret = _mmf_vpss_init(1, ch, stSizeIn, stSizeOut, formatIn, formatOut, fps, depth, mirror, flip, fit);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vpss init failed. s32Ret: 0x%x !\n", s32Ret);
				return -1;
			}

			// SAMPLE_PRT("vpss vo reinit.\r\n");
			// SAMPLE_PRT("stSizeIn.u32Width: %d, stSizeIn.u32Height: %d, stSizeOut.u32Width: %d, stSizeOut.u32Height: %d formatOut:%d\n",
			// 						stSizeIn.u32Width, stSizeIn.u32Height, stSizeOut.u32Width, stSizeOut.u32Height, formatOut);
			s32Ret = SAMPLE_COMM_VPSS_Bind_VO(1, ch, layer, ch);
			if (s32Ret != CVI_SUCCESS) {
				SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
				_mmf_vpss_deinit(1, ch);
				return -1;
			}
#endif
		}

#if 0
		if (!priv.vo_video_pre_frame[ch]
			|| priv.vo_video_pre_frame_width[ch] != width
			|| priv.vo_video_pre_frame_height[ch] != height
			|| priv.vo_video_pre_frame_format[ch] != format) {
			if (priv.vo_video_pre_frame[ch]) {
				_mmf_free_frame(priv.vo_video_pre_frame[ch]);
				priv.vo_video_pre_frame[ch] = NULL;
			}
			priv.vo_video_pre_frame[ch] = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_USER_ID, (SIZE_S){(CVI_U32)width, (CVI_U32)height}, (PIXEL_FORMAT_E)format);
			if (!priv.vo_video_pre_frame[ch]) {
				printf("Alloc frame failed!\r\n");
				return -1;
			}
			priv.vo_video_pre_frame_width[ch] = width;
			priv.vo_video_pre_frame_height[ch] = height;
			priv.vo_video_pre_frame_format[ch] = format;
		}

		VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *)priv.vo_video_pre_frame[ch];
		switch (format) {
		case PIXEL_FORMAT_RGB_888:
			// if (fit == 2) {	// crop image and keep aspect ratio
			// 	CVI_FLOAT corp_scale_w = (CVI_FLOAT)priv.vo_vpss_in_size[ch].u32Width / priv.vo_vpss_out_size[ch].u32Width;
			// 	CVI_FLOAT corp_scale_h = (CVI_FLOAT)priv.vo_vpss_in_size[ch].u32Height / priv.vo_vpss_out_size[ch].u32Height;
			// 	CVI_U32 crop_w = corp_scale_w < corp_scale_h ? priv.vo_vpss_out_size[ch].u32Width * corp_scale_w: priv.vo_vpss_out_size[ch].u32Width * corp_scale_h;
			// 	CVI_U32 crop_h = corp_scale_w < corp_scale_h ? priv.vo_vpss_out_size[ch].u32Height * corp_scale_w: priv.vo_vpss_out_size[ch].u32Height * corp_scale_h;
			// 	if (corp_scale_h < 0 || corp_scale_w < 0) {
			// 		SAMPLE_PRT("crop scale error. corp_scale_w: %d, corp_scale_h: %d\n", corp_scale_w, corp_scale_h);
			// 		return -1;
			// 	}
			// } else
			{
				if (frame->stVFrame.u32Stride[0] != (CVI_U32)width * 3) {
					for (int h = 0; h < height; h++) {
						memcpy((uint8_t *)frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Stride[0] * h, ((uint8_t *)data) + width * h * 3, width * 3);
					}
				} else {
					memcpy(frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3);
				}
				CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0],
									frame->stVFrame.pu8VirAddr[0],
									width * height * 3);
			}
		break;
		case PIXEL_FORMAT_NV21:
			if (frame->stVFrame.u32Stride[0] != (CVI_U32)width) {
				for (int h = 0; h < height * 3 / 2; h ++) {
					memcpy((uint8_t *)frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + width * h, width);
				}
			} else {
				memcpy(frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3 / 2);
			}

			CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0],
								frame->stVFrame.pu8VirAddr[0],
								width * height * 3 / 2);
		break;
		default:
			printf("format not support\n");
			return CVI_FAILURE;
		}

		s32Ret = CVI_VPSS_SendFrame(1, frame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VO_SendFrame failed, ret:%#x\n", s32Ret);
			return s32Ret;
		}
#else
		VIDEO_FRAME_INFO_S stVideoFrame;
		VB_BLK blk;
		VB_CAL_CONFIG_S stVbCalConfig;
		UNUSED(len);
		COMMON_GetPicBufferConfig(width, height, (PIXEL_FORMAT_E)format, DATA_BITWIDTH_8
			, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

		// if (priv.vo_video_cfg[ch].enPixFormat != (PIXEL_FORMAT_E)format) {
		// 	printf("vo ch %d format not match. input:%d need:%d\n", ch, format, priv.vo_video_cfg[ch].enPixFormat);
		// 	return CVI_FAILURE;
		// }

		memset(&stVideoFrame, 0, sizeof(stVideoFrame));
		stVideoFrame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
		stVideoFrame.stVFrame.enPixelFormat = (PIXEL_FORMAT_E)format;
		stVideoFrame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
		stVideoFrame.stVFrame.enColorGamut = COLOR_GAMUT_BT709;
		stVideoFrame.stVFrame.u32Width = width;
		stVideoFrame.stVFrame.u32Height = height;
		stVideoFrame.stVFrame.u32Stride[0] = stVbCalConfig.u32MainStride;
		if (stVbCalConfig.plane_num == 2) {
			stVideoFrame.stVFrame.u32Stride[1] = stVbCalConfig.u32CStride;
		}
		if (stVbCalConfig.plane_num == 3) {
			stVideoFrame.stVFrame.u32Stride[2] = stVbCalConfig.u32CStride;
		}
		stVideoFrame.stVFrame.u32TimeRef = 0;
		stVideoFrame.stVFrame.u64PTS = 0;
		stVideoFrame.stVFrame.enDynamicRange = DYNAMIC_RANGE_SDR8;

		int retry_cnt = 0;
	_retry:
		blk = CVI_VB_GetBlock(MMF_VB_USER_ID, stVbCalConfig.u32VBSize);
		if (blk == VB_INVALID_HANDLE) {
			if (retry_cnt ++ < 5) {
				usleep(1000);
				goto _retry;
			}
			SAMPLE_PRT("SAMPLE_COMM_VPSS_SendFrame: Can't acquire vb block\n");
			return CVI_FAILURE;
		}
		// printf("u32PoolId:%d, u32Length:%d, u64PhyAddr:%#lx\r\n", CVI_VB_Handle2PoolId(blk), stVbCalConfig.u32VBSize, CVI_VB_Handle2PhysAddr(blk));

		stVideoFrame.u32PoolId = CVI_VB_Handle2PoolId(blk);
		stVideoFrame.stVFrame.u32Length[0] = stVbCalConfig.u32MainYSize;
		stVideoFrame.stVFrame.u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(blk);

		if (stVbCalConfig.plane_num == 2) {
			stVideoFrame.stVFrame.u32Length[1] = stVbCalConfig.u32MainCSize;
			stVideoFrame.stVFrame.u64PhyAddr[1] = stVideoFrame.stVFrame.u64PhyAddr[0]
				+ ALIGN(stVbCalConfig.u32MainYSize, stVbCalConfig.u16AddrAlign);
		}
		if (stVbCalConfig.plane_num == 3) {
			stVideoFrame.stVFrame.u32Length[2] = stVbCalConfig.u32MainCSize;
			stVideoFrame.stVFrame.u64PhyAddr[2] = stVideoFrame.stVFrame.u64PhyAddr[1]
				+ ALIGN(stVbCalConfig.u32MainCSize, stVbCalConfig.u16AddrAlign);
		}

		CVI_U32 total_size = stVideoFrame.stVFrame.u32Length[0] + stVideoFrame.stVFrame.u32Length[1] + stVideoFrame.stVFrame.u32Length[2];
		stVideoFrame.stVFrame.pu8VirAddr[0]
				= (CVI_U8*)CVI_SYS_MmapCache(stVideoFrame.stVFrame.u64PhyAddr[0], total_size);

		if (stVbCalConfig.plane_num == 2) {
			stVideoFrame.stVFrame.pu8VirAddr[1] = stVideoFrame.stVFrame.pu8VirAddr[0] + stVideoFrame.stVFrame.u32Length[0];
		}

		if (stVbCalConfig.plane_num == 3) {
			stVideoFrame.stVFrame.pu8VirAddr[2] = stVideoFrame.stVFrame.pu8VirAddr[1] + stVideoFrame.stVFrame.u32Length[1];
		}

		switch (format) {
		case PIXEL_FORMAT_RGB_888:
			if (stVideoFrame.stVFrame.u32Stride[0] != (CVI_U32)width * 3) {
				for (int h = 0; h < height; h++) {
					memcpy((uint8_t *)stVideoFrame.stVFrame.pu8VirAddr[0] + stVideoFrame.stVFrame.u32Stride[0] * h, ((uint8_t *)data) + width * h * 3, width * 3);
				}
			} else {
				memcpy(stVideoFrame.stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3);
			}
			CVI_SYS_IonFlushCache(stVideoFrame.stVFrame.u64PhyAddr[0],
								stVideoFrame.stVFrame.pu8VirAddr[0],
								width * height * 3);
		break;
		case PIXEL_FORMAT_NV21:
			if (stVideoFrame.stVFrame.u32Stride[0] != (CVI_U32)width) {
				for (int h = 0; h < height * 3 / 2; h ++) {
					memcpy((uint8_t *)stVideoFrame.stVFrame.pu8VirAddr[0] + stVideoFrame.stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + width * h, width);
				}
			} else {
				memcpy(stVideoFrame.stVFrame.pu8VirAddr[0], ((uint8_t *)data), width * height * 3 / 2);
			}

			CVI_SYS_IonFlushCache(stVideoFrame.stVFrame.u64PhyAddr[0],
								stVideoFrame.stVFrame.pu8VirAddr[0],
								width * height * 3 / 2);
		break;
		default:
			printf("format not support\n");
			CVI_VB_ReleaseBlock(blk);
			return CVI_FAILURE;
		}

	#if 0
		VPSS_GRP_ATTR_S GrpAttr;
		CVI_VPSS_GetGrpAttr(1, &GrpAttr);
		mmf_dump_grpattr(&GrpAttr);

		VPSS_CHN_ATTR_S ChnAttr;
		CVI_VPSS_GetChnAttr(1, ch, &ChnAttr);
		mmf_dump_chnattr(&ChnAttr);

		mmf_dump_frame(&stVideoFrame);
	#endif
		// UNUSED(layer);
		// mmf_vo_frame_push
		s32Ret = CVI_VPSS_SendFrame(1, &stVideoFrame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VO_SendFrame failed >< with %#x\n", s32Ret);
			CVI_VB_ReleaseBlock(blk);
			return s32Ret;
		}

		CVI_VB_ReleaseBlock(blk);

		for (int i = 0; i < stVbCalConfig.plane_num; ++i) {
			if (stVideoFrame.stVFrame.u32Length[i] == 0)
				continue;
			CVI_SYS_Munmap(stVideoFrame.stVFrame.pu8VirAddr[i], stVideoFrame.stVFrame.u32Length[i]);
		}
#endif
	} else if (layer == MMF_VO_OSD_LAYER) {
		if (ch < 0 || ch >= MMF_VO_OSD_MAX_CHN) {
			printf("invalid ch %d\n", ch);
			return -1;
		}

		if (priv.vo_osd_chn_is_inited[ch] == false) {
			printf("vo osd ch %d not open\n", ch);
			return -1;
		}

		if (format != PIXEL_FORMAT_ARGB_8888) {
			printf("only support ARGB format.\n");
			return -1;
		}

		if (0 != mmf_region_frame_push(ch, data, len)) {
			printf("mmf_region_flush failed!\r\n");
			return -1;
		}
	} else {
		printf("invalid layer %d\n", layer);
		return -1;
	}

	return CVI_SUCCESS;
}


static CVI_S32 SAMPLE_COMM_REGION_AttachToChn2(CVI_S32 ch, int x, int y, RGN_TYPE_E enType, MMF_CHN_S *pstChn)
{
#define OverlayMinHandle 0
#define OverlayExMinHandle 20
#define CoverMinHandle 40
#define CoverExMinHandle 60
#define MosaicMinHandle 80
#define OdecHandle 100
	CVI_S32 s32Ret = CVI_SUCCESS;
	RGN_CHN_ATTR_S stChnAttr;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("HandleId is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (enType != OVERLAY_RGN) {
		SAMPLE_PRT("enType is illegal %d!\n", enType);
		return CVI_FAILURE;
	}

	if (pstChn == CVI_NULL) {
		SAMPLE_PRT("pstChn is NULL !\n");
		return CVI_FAILURE;
	}
	memset(&stChnAttr, 0, sizeof(stChnAttr));

	/*set the chn config*/
	stChnAttr.bShow = CVI_TRUE;
	switch (enType) {
	case OVERLAY_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = OVERLAY_RGN;
		stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = CVI_FALSE;
		break;
	case OVERLAYEX_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = OVERLAYEX_RGN;
		stChnAttr.unChnAttr.stOverlayExChn.stInvertColor.bInvColEn = CVI_FALSE;
		break;
	case COVER_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = COVER_RGN;
		stChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;

		stChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 100;
		stChnAttr.unChnAttr.stCoverChn.stRect.u32Width = 100;

		stChnAttr.unChnAttr.stCoverChn.u32Color = 0x0000ffff;

		stChnAttr.unChnAttr.stCoverChn.enCoordinate = RGN_ABS_COOR;
		break;
	case COVEREX_RGN:
		stChnAttr.bShow = CVI_TRUE;
		stChnAttr.enType = COVEREX_RGN;
		stChnAttr.unChnAttr.stCoverExChn.enCoverType = AREA_RECT;

		stChnAttr.unChnAttr.stCoverExChn.stRect.u32Height = 100;
		stChnAttr.unChnAttr.stCoverExChn.stRect.u32Width = 100;

		stChnAttr.unChnAttr.stCoverExChn.u32Color = 0x0000ffff;
		break;
	case MOSAIC_RGN:
		stChnAttr.enType = MOSAIC_RGN;
		stChnAttr.unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_8;
		stChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 96; // 8 pixel align
		stChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 96;
		break;
	default:
		return CVI_FAILURE;
	}

	if (enType == OVERLAY_RGN) {
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = x;
		stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = y;
		stChnAttr.unChnAttr.stOverlayChn.u32Layer = ch;
	}
	s32Ret = CVI_RGN_AttachToChn(ch, pstChn, &stChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_REGION_AttachToChn failed!\n");
		return s32Ret;
	}

	return s32Ret;
}

static int _mmf_region_init(int ch, int type, int width, int height, int format)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	RGN_ATTR_S stRegion;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}
	if (type != OVERLAY_RGN) {
		SAMPLE_PRT("enType is illegal %d!\n", type);
		return CVI_FAILURE;
	}

	if (priv.rgn_is_init[ch]) {
		return 0;
	}

	stRegion.enType = (RGN_TYPE_E)type;
	stRegion.unAttr.stOverlay.enPixelFormat = (PIXEL_FORMAT_E)format;
	stRegion.unAttr.stOverlay.stSize.u32Height = height;
	stRegion.unAttr.stOverlay.stSize.u32Width = width;
	stRegion.unAttr.stOverlay.u32BgColor = 0x00000000; // ARGB1555 transparent
	stRegion.unAttr.stOverlay.u32CanvasNum = 1;
	stRegion.unAttr.stOverlay.stCompressInfo.enOSDCompressMode = OSD_COMPRESS_MODE_NONE;
	s32Ret = CVI_RGN_Create(ch, &stRegion);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_Create failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.rgn_type[ch] = (RGN_TYPE_E)type;
	priv.rgn_id[ch] = ch;
	priv.rgn_is_init[ch] = true;

	return s32Ret;
}


static int _mmf_region_bind(int ch, int mod_id, int dev_id, int chn_id, int x, int y)
{
	CVI_S32 s32Ret;

	MMF_CHN_S stChn = {
		.enModId = (MOD_ID_E)mod_id,
		.s32DevId = (CVI_S32)dev_id,
		.s32ChnId = (CVI_S32)chn_id
	};

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (priv.rgn_is_bind[ch]) {
		return 0;
	}

	RGN_TYPE_E type = (RGN_TYPE_E)priv.rgn_type[ch];
	s32Ret = SAMPLE_COMM_REGION_AttachToChn2(ch, x, y, type, &stChn);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("SAMPLE_COMM_REGION_AttachToChn failed!\n");
		return s32Ret;
	}

	priv.rgn_mod_id[ch] = (MOD_ID_E)mod_id;
	priv.rgn_dev_id[ch] = (CVI_S32)dev_id;
	priv.rgn_chn_id[ch] = (CVI_S32)chn_id;
	priv.rgn_is_bind[ch] = true;

	return s32Ret;
}

static int _mmf_region_unbind(int ch)
{
	CVI_S32 s32Ret;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	MMF_CHN_S stChn = {
		.enModId = (MOD_ID_E)priv.rgn_mod_id[ch],
		.s32DevId = (CVI_S32)priv.rgn_dev_id[ch],
		.s32ChnId = (CVI_S32)priv.rgn_chn_id[ch]
	};

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_DetachFromChn(ch, &stChn);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_DetachFromChn %d failed!\n", ch);
	}

	priv.rgn_is_bind[ch] = false;

	return s32Ret;
}

static int _mmf_region_deinit(int ch)
{
	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (priv.rgn_is_bind[ch]) {
		_mmf_region_unbind(ch);
	}

	CVI_S32 s32Ret = CVI_RGN_Destroy(ch);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_Destroy failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	priv.rgn_is_init[ch] = false;

	return s32Ret;
}

int mmf_get_region_unused_channel(void)
{
	for (int i = 0; i < MMF_RGN_MAX_NUM; i++) {
		if (priv.rgn_is_init[i] == false) {
			return i;
		}
	}
	return -1;
}

int mmf_add_region_channel(int ch, int type, int mod_id, int dev_id, int chn_id, int x, int y, int width, int height, int format)
{
	if (0 != _mmf_region_init(ch, type, width, height, format)) {
		return -1;
	}

	if (0 != _mmf_region_bind(ch, mod_id, dev_id, chn_id, x, y)) {
		_mmf_region_deinit(ch);
		return -1;
	}

	return 0;
}

int mmf_del_region_channel(int ch)
{
	if (0 != _mmf_region_deinit(ch)) {
		return -1;
	}

	return 0;
}

int mmf_del_region_channel_all(void)
{
	for (int ch = 0; ch < MMF_RGN_MAX_NUM; ch ++) {
		_mmf_region_deinit(ch);
	}

	return 0;
}

int mmf_region_get_canvas(int ch, void **data, int *width, int *height, int *format)
{
	CVI_S32 s32Ret;
	RGN_CANVAS_INFO_S stCanvasInfo;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_GetCanvasInfo(ch, &stCanvasInfo);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	if (data) *data = stCanvasInfo.pu8VirtAddr;
	if (width) *width = (int)stCanvasInfo.stSize.u32Width;
	if (height) *height = (int)stCanvasInfo.stSize.u32Height;
	if (format) *format = (int)stCanvasInfo.enPixelFormat;

	return s32Ret;
}

int mmf_region_update_canvas(int ch)
{
	CVI_S32 s32Ret;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_UpdateCanvas(ch);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

int mmf_region_frame_push(int ch, void *data, int len)
{
	CVI_S32 s32Ret;
	RGN_CANVAS_INFO_S stCanvasInfo;

	if (ch < 0 || ch >= MMF_RGN_MAX_NUM) {
		SAMPLE_PRT("Handle ch is illegal %d!\n", ch);
		return CVI_FAILURE;
	}

	if (!priv.rgn_is_init[ch]) {
		return 0;
	}

	if (!priv.rgn_is_bind[ch]) {
		return 0;
	}

	s32Ret = CVI_RGN_GetCanvasInfo(ch, &stCanvasInfo);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_GetCanvasInfo failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	if (stCanvasInfo.enPixelFormat == PIXEL_FORMAT_ARGB_8888) {
		if (!data || (CVI_U32)len != stCanvasInfo.stSize.u32Width * stCanvasInfo.stSize.u32Height * 4) {
			printf("Param is error!\r\n");
			return CVI_FAILURE;
		}
		memcpy(stCanvasInfo.pu8VirtAddr, data, len);
	} else {
		printf("Not support format!\r\n");
		return CVI_FAILURE;
	}

	s32Ret = CVI_RGN_UpdateCanvas(ch);
	if (s32Ret != CVI_SUCCESS) {
		SAMPLE_PRT("CVI_RGN_UpdateCanvas failed with %#x!\n", s32Ret);
		return CVI_FAILURE;
	}
	return s32Ret;
}

int mmf_enc_jpg_init(int ch, int w, int h, int format, int quality)
{
	if (priv.enc_jpg_is_init)
		return 0;

	if (quality <= 50) {
		printf("quality range is (50, 100]\n");
		return -1;
	}

	if ((format == PIXEL_FORMAT_RGB_888 && w * h * 3 > 640 * 480 * 3)
		|| (format == PIXEL_FORMAT_RGB_888 && w * h * 3 / 2 > 2560 * 1440 * 3 / 2)) {
		printf("image size is too large, for NV21, maximum resolution 2560x1440, for RGB888, maximum resolution 640x480!\n");
		return -1;
	}

	CVI_S32 s32Ret = CVI_SUCCESS;

	VENC_CHN_ATTR_S stVencChnAttr;
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	stVencChnAttr.stVencAttr.enType = PT_JPEG;
	stVencChnAttr.stVencAttr.u32MaxPicWidth = w;
	stVencChnAttr.stVencAttr.u32MaxPicHeight = h;
	stVencChnAttr.stVencAttr.u32PicWidth = w;
	stVencChnAttr.stVencAttr.u32PicHeight = h;
	stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_FALSE;
	stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_FALSE;
	stVencChnAttr.stVencAttr.bByFrame = 1;
	stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGFIXQP;

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VENC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	s32Ret = CVI_VENC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_DestoryChn [%d] failed with %#x\n", ch, s32Ret);
	}

	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %#x\n", ch, s32Ret);
		return s32Ret;
	}

	VENC_JPEG_PARAM_S stJpegParam;
	memset(&stJpegParam, 0, sizeof(VENC_JPEG_PARAM_S));
	s32Ret = CVI_VENC_GetJpegParam(ch, &stJpegParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_GetJpegParam failed with %#x\n", s32Ret);
		CVI_VENC_DestroyChn(ch);
		return s32Ret;
	}
	stJpegParam.u32Qfactor = quality;
	s32Ret = CVI_VENC_SetJpegParam(ch, &stJpegParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_SetJpegParam failed with %#x\n", s32Ret);
		CVI_VENC_DestroyChn(ch);
		return s32Ret;
	}

	switch (format) {
	case PIXEL_FORMAT_RGB_888:
		s32Ret = _mmf_vpss_init(2, ch, (SIZE_S){(CVI_U32)w, (CVI_U32)h}, (SIZE_S){(CVI_U32)w, (CVI_U32)h}, PIXEL_FORMAT_RGB_888, PIXEL_FORMAT_YUV_PLANAR_420, 30, 0, CVI_FALSE, CVI_FALSE, 0);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS init failed with %d\n", s32Ret);
			CVI_VENC_StopRecvFrame(ch);
			CVI_VENC_DestroyChn(ch);
			return s32Ret;
		}

		s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(2, ch, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS bind VENC failed with %#x\n", s32Ret);
			_mmf_vpss_deinit(2, ch);
			CVI_VENC_StopRecvFrame(ch);
			CVI_VENC_DestroyChn(ch);
			return s32Ret;
		}
		break;
	case PIXEL_FORMAT_NV21:
		break;
	default:
		printf("unknown format!\n");
		CVI_VENC_StopRecvFrame(ch);
		CVI_VENC_DestroyChn(ch);
		return -1;
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StartRecvPic failed with %#x\n", s32Ret);
		return CVI_FAILURE;
	}

	if (priv.enc_jpg_frame) {
		_mmf_free_frame(priv.enc_jpg_frame);
		priv.enc_jpg_frame = NULL;
	}

	priv.enc_jpg_frame_w = w;
	priv.enc_jpg_frame_h = h;
	priv.enc_jpg_frame_fmt = format;
	priv.enc_jpg_is_init = 1;
	priv.enc_jpg_running = 0;

	return s32Ret;
}

int mmf_enc_jpg_deinit(int ch)
{
	if (!priv.enc_jpg_is_init)
		return 0;

	CVI_S32 s32Ret = CVI_SUCCESS;

	if (!mmf_enc_jpg_pop(ch, NULL, NULL)) {
		mmf_enc_jpg_free(ch);
	}

	switch (priv.enc_jpg_frame_fmt) {
	case PIXEL_FORMAT_RGB_888:
		s32Ret = SAMPLE_COMM_VPSS_UnBind_VENC(2, ch, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS unbind VENC failed with %d\n", s32Ret);
		}

		s32Ret = _mmf_vpss_deinit(2, ch);
		if (s32Ret != CVI_SUCCESS) {
			printf("VPSS deinit failed with %d\n", s32Ret);
		}
		break;
	case PIXEL_FORMAT_NV21:
		break;
	default:
		break;
	}

	s32Ret = CVI_VENC_StopRecvFrame(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StopRecvPic failed with %d\n", s32Ret);
	}

	s32Ret = CVI_VENC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ResetChn vechn[%d] failed with %#x!\n",
				ch, s32Ret);
	}

	s32Ret = CVI_VENC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_DestroyChn [%d] failed with %d\n", ch, s32Ret);
	}

	if (priv.enc_jpg_frame) {
		_mmf_free_frame(priv.enc_jpg_frame);
		priv.enc_jpg_frame = NULL;
	}

	priv.enc_jpg_frame_w = 0;
	priv.enc_jpg_frame_h = 0;
	priv.enc_jpg_frame_fmt = 0;
	priv.enc_jpg_is_init = 0;
	priv.enc_jpg_running = 0;

	return s32Ret;
}

int mmf_enc_jpg_push(int ch, uint8_t *data, int w, int h, int format)
{
	UNUSED(ch);
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.enc_jpg_running) {
		return s32Ret;
	}

	SIZE_S stSize = {(CVI_U32)w, (CVI_U32)h};
	if (priv.enc_jpg_frame == NULL || priv.enc_jpg_frame_w != w || priv.enc_jpg_frame_h != h || priv.enc_jpg_frame_fmt != format) {
		if (!priv.enc_jpg_is_init)
			mmf_enc_jpg_init(ch, w, h, format, 80);

		priv.enc_jpg_frame_w = w;
		priv.enc_jpg_frame_h = h;
		priv.enc_jpg_frame_fmt = format;
		priv.enc_jpg_frame = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_ENC_JPEG_ID, stSize, (PIXEL_FORMAT_E)format);
		if (!priv.enc_jpg_frame) {
			printf("Alloc frame failed!\r\n");
			return -1;
		}
	}

	switch (format) {
		case PIXEL_FORMAT_RGB_888:
		{
			if (priv.enc_jpg_frame->stVFrame.u32Stride[0] != (CVI_U32)w * 3) {
				for (CVI_U32 h = 0; h < priv.enc_jpg_frame->stVFrame.u32Height; h++) {
					memcpy((uint8_t *)priv.enc_jpg_frame->stVFrame.pu8VirAddr[0] + priv.enc_jpg_frame->stVFrame.u32Stride[0] * h, ((uint8_t *)data) + w * h * 3, w * 3);
				}
			} else {
				memcpy(priv.enc_jpg_frame->stVFrame.pu8VirAddr[0], data, w * h * 3);
			}

			s32Ret = CVI_VPSS_SendFrame(2, priv.enc_jpg_frame, 1000);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VPSS_SendFrame failed with %d\n", s32Ret);
				return s32Ret;
			}
		}
		break;
		case PIXEL_FORMAT_NV21:
			if (priv.enc_jpg_frame->stVFrame.u32Stride[0] != (CVI_U32)w) {
				for (CVI_U32 h = 0; h < priv.enc_jpg_frame->stVFrame.u32Height * 3 / 2; h ++) {
					memcpy((uint8_t *)priv.enc_jpg_frame->stVFrame.pu8VirAddr[0] + priv.enc_jpg_frame->stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + w * h, w);
				}
			} else {
				memcpy(priv.enc_jpg_frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), w * h * 3 / 2);
			}

			s32Ret = CVI_VENC_SendFrame(ch, priv.enc_jpg_frame, 1000);
			if (s32Ret != CVI_SUCCESS) {
				printf("CVI_VENC_SendFrame failed with %d\n", s32Ret);
				return s32Ret;
			}
		break;
		default: return -1;
	}

	priv.enc_jpg_running = 1;

	return s32Ret;
}

int mmf_enc_jpg_pop(int ch, uint8_t **data, int *size)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (!priv.enc_jpg_running) {
		return s32Ret;
	}

	priv.enc_jpeg_frame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 1);
	if (!priv.enc_jpeg_frame.pstPack) {
		printf("Malloc failed!\r\n");
		return -1;
	}

	VENC_CHN_STATUS_S stStatus;
	s32Ret = CVI_VENC_QueryStatus(ch, &stStatus);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_QueryStatus failed with %#x\n", s32Ret);
		return s32Ret;
	}

	if (stStatus.u32CurPacks > 0) {
		s32Ret = CVI_VENC_GetStream(ch, &priv.enc_jpeg_frame, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
			return s32Ret;
		}
	} else {
		printf("CVI_VENC_QueryStatus find not pack\r\n");
		return -1;
	}

	if (data)
		*data = priv.enc_jpeg_frame.pstPack[0].pu8Addr;
	if (size)
		*size = priv.enc_jpeg_frame.pstPack[0].u32Len;

	return s32Ret;
}

int mmf_enc_jpg_free(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (!priv.enc_jpg_running) {
		return s32Ret;
	}

	if (priv.enc_jpeg_frame.pstPack)
		free(priv.enc_jpeg_frame.pstPack);
	s32Ret = CVI_VENC_ReleaseStream(ch, &priv.enc_jpeg_frame);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ReleaseStream failed with %#x\n", s32Ret);
		return s32Ret;
	}

	priv.enc_jpg_running = 0;
	return s32Ret;
}

int mmf_enc_h265_init(int ch, int w, int h)
{
	if (priv.enc_h265_is_init)
		return 0;

	CVI_S32 s32Ret = CVI_SUCCESS;

	VENC_CHN_ATTR_S stVencChnAttr;
	memset(&stVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
	stVencChnAttr.stVencAttr.enType = PT_H265;
	stVencChnAttr.stVencAttr.u32MaxPicWidth = w;
	stVencChnAttr.stVencAttr.u32MaxPicHeight = h;
	stVencChnAttr.stVencAttr.u32BufSize = 1024 * 1024;	// 1024Kb
	stVencChnAttr.stVencAttr.bByFrame = 1;
	stVencChnAttr.stVencAttr.u32PicWidth = w;
	stVencChnAttr.stVencAttr.u32PicHeight = h;
	stVencChnAttr.stVencAttr.bEsBufQueueEn = CVI_TRUE;
	stVencChnAttr.stVencAttr.bIsoSendFrmEn = CVI_TRUE;
	stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 2;
	stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
	stVencChnAttr.stRcAttr.stH265Cbr.u32Gop = 50;
	stVencChnAttr.stRcAttr.stH265Cbr.u32StatTime = 2;
	stVencChnAttr.stRcAttr.stH265Cbr.u32SrcFrameRate = 30;
	stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate = 30;
	stVencChnAttr.stRcAttr.stH265Cbr.u32BitRate = 3000;
	stVencChnAttr.stRcAttr.stH265Cbr.bVariFpsEn = 0;
	s32Ret = CVI_VENC_CreateChn(ch, &stVencChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_CreateChn [%d] failed with %d\n", ch, s32Ret);
		return s32Ret;
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	stRecvParam.s32RecvPicNum = -1;
	s32Ret = CVI_VENC_StartRecvFrame(ch, &stRecvParam);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StartRecvPic failed with %d\n", s32Ret);
		return CVI_FAILURE;
	}

	{
		VENC_H265_TRANS_S h265Trans = {0};
		s32Ret = CVI_VENC_GetH265Trans(ch, &h265Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH265Trans failed with %d\n", s32Ret);
			return s32Ret;
		}
		h265Trans.cb_qp_offset = 0;
		h265Trans.cr_qp_offset = 0;
		s32Ret = CVI_VENC_SetH265Trans(ch, &h265Trans);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH265Trans failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	{
		VENC_H265_VUI_S h265Vui = {0};
		s32Ret = CVI_VENC_GetH265Vui(ch, &h265Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetH265Vui failed with %d\n", s32Ret);
			return s32Ret;
		}

		h265Vui.stVuiAspectRatio.aspect_ratio_info_present_flag = 0;
		h265Vui.stVuiAspectRatio.aspect_ratio_idc = 1;
		h265Vui.stVuiAspectRatio.overscan_info_present_flag = 0;
		h265Vui.stVuiAspectRatio.overscan_appropriate_flag = 0;
		h265Vui.stVuiAspectRatio.sar_width = 1;
		h265Vui.stVuiAspectRatio.sar_height = 1;
		h265Vui.stVuiTimeInfo.timing_info_present_flag = 1;
		h265Vui.stVuiTimeInfo.num_units_in_tick = 1;
		h265Vui.stVuiTimeInfo.time_scale = 30;
		h265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 = 1;
		h265Vui.stVuiVideoSignal.video_signal_type_present_flag = 0;
		h265Vui.stVuiVideoSignal.video_format = 5;
		h265Vui.stVuiVideoSignal.video_full_range_flag = 0;
		h265Vui.stVuiVideoSignal.colour_description_present_flag = 0;
		h265Vui.stVuiVideoSignal.colour_primaries = 2;
		h265Vui.stVuiVideoSignal.transfer_characteristics = 2;
		h265Vui.stVuiVideoSignal.matrix_coefficients = 2;
		h265Vui.stVuiBitstreamRestric.bitstream_restriction_flag = 0;

		// _mmf_dump_venc_h265_vui(&h265Vui);

		s32Ret = CVI_VENC_SetH265Vui(ch, &h265Vui);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetH265Vui failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	// rate control
	{
		VENC_RC_PARAM_S stRcParam;
		s32Ret = CVI_VENC_GetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetRcParam failed with %d\n", s32Ret);
			return s32Ret;
		}
		stRcParam.s32FirstFrameStartQp = 35;
		stRcParam.stParamH265Cbr.u32MinIprop = 1;
		stRcParam.stParamH265Cbr.u32MaxIprop = 10;
		stRcParam.stParamH265Cbr.u32MaxQp = 51;
		stRcParam.stParamH265Cbr.u32MinQp = 20;
		stRcParam.stParamH265Cbr.u32MaxIQp = 51;
		stRcParam.stParamH265Cbr.u32MinIQp = 20;

		// _mmf_dump_venc_rc_param(&stRcParam);

		s32Ret = CVI_VENC_SetRcParam(ch, &stRcParam);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetRcParam failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	// frame lost set
	{
		VENC_FRAMELOST_S stFL;
		s32Ret = CVI_VENC_GetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetFrameLostStrategy failed with %d\n", s32Ret);
			return s32Ret;
		}
		stFL.enFrmLostMode = FRMLOST_PSKIP;

		// _mmf_dump_venc_framelost(&stFL);

		s32Ret = CVI_VENC_SetFrameLostStrategy(ch, &stFL);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_SetFrameLostStrategy failed with %d\n", s32Ret);
			return s32Ret;
		}
	}

	if (priv.enc_h265_video_frame) {
		_mmf_free_frame(priv.enc_h265_video_frame);
		priv.enc_h265_video_frame = NULL;
	}

	priv.enc_h265_frame_w = w;
	priv.enc_h265_frame_h = h;
	priv.enc_h265_frame_fmt = 0;
	priv.enc_h265_running = 0;
	priv.enc_h265_is_init = 1;

	return s32Ret;
}

int mmf_enc_h265_deinit(int ch)
{
	if (!priv.enc_h265_is_init)
		return 0;

	CVI_S32 s32Ret = CVI_SUCCESS;

	mmf_h265_stream_t stream;
	if (!mmf_enc_h265_pop(ch, &stream)) {
		mmf_enc_h265_free(ch);
	}

	s32Ret = CVI_VENC_StopRecvFrame(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_StopRecvPic failed with %d\n", s32Ret);
	}

	s32Ret = CVI_VENC_ResetChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ResetChn vechn[%d] failed with %#x!\n", ch, s32Ret);
	}

	s32Ret = CVI_VENC_DestroyChn(ch);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_DestroyChn [%d] failed with %d\n", ch, s32Ret);
	}

	if (priv.enc_h265_video_frame) {
		_mmf_free_frame(priv.enc_h265_video_frame);
		priv.enc_h265_video_frame = NULL;
	}

	priv.enc_h265_frame_w = 0;
	priv.enc_h265_frame_h = 0;
	priv.enc_h265_frame_fmt = 0;
	priv.enc_h265_running = 0;
	priv.enc_h265_is_init = 0;

	return s32Ret;
}

int mmf_enc_h265_push(int ch, uint8_t *data, int w, int h, int format)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (priv.enc_h265_running) {
		return s32Ret;
	}

	if (priv.enc_h265_video_frame == NULL || priv.enc_h265_frame_w != w || priv.enc_h265_frame_h != h || priv.enc_h265_frame_fmt != format) {
		if (!priv.enc_h265_is_init)
			mmf_enc_h265_init(ch, w, h);
		priv.enc_h265_frame_w = w;
		priv.enc_h265_frame_h = h;
		priv.enc_h265_frame_fmt = format;
		priv.enc_h265_video_frame = (VIDEO_FRAME_INFO_S *)_mmf_alloc_frame(MMF_VB_ENC_H265_ID, (SIZE_S){(CVI_U32)w, (CVI_U32)h}, (PIXEL_FORMAT_E)format);
		if (!priv.enc_h265_video_frame) {
			printf("Alloc frame failed!\r\n");
			return -1;
		}
	}

	switch (format) {
		case PIXEL_FORMAT_NV21:
		{
			if (priv.enc_h265_video_frame->stVFrame.u32Stride[0] != (CVI_U32)w) {
				for (int h0 = 0; h0 < h * 3 / 2; h0 ++) {
					memcpy((uint8_t *)priv.enc_h265_video_frame->stVFrame.pu8VirAddr[0] + priv.enc_h265_video_frame->stVFrame.u32Stride[0] * h,
							((uint8_t *)data) + w * h0, w);
				}
			} else {
				memcpy((uint8_t *)priv.enc_h265_video_frame->stVFrame.pu8VirAddr[0], ((uint8_t *)data), w * h * 3 / 2);
			}
		}
		break;
		default: return -1;
	}

	s32Ret = CVI_VENC_SendFrame(ch, priv.enc_h265_video_frame, 1000);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
		return s32Ret;
	}

	priv.enc_h265_running = 1;

	return s32Ret;
}

int mmf_enc_h265_pop(int ch, mmf_h265_stream_t *stream)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (!priv.enc_h265_running) {
		return s32Ret;
	}

	int fd = CVI_VENC_GetFd(ch);
	if (fd < 0) {
		printf("CVI_VENC_GetFd failed with %d\n", fd);
		return -1;
	}

	fd_set readFds;
	struct timeval timeoutVal;
	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	timeoutVal.tv_sec = 0;
	timeoutVal.tv_usec = 80*1000;
	s32Ret = select(fd + 1, &readFds, NULL, NULL, &timeoutVal);
	if (s32Ret < 0) {
		if (errno == EINTR) {
			printf("VencChn(%d) select failed!\n", ch);
			return -1;
		}
	} else if (s32Ret == 0) {
		printf("VencChn(%d) select timeout!\n", ch);
		return -1;
	}

	priv.enc_h265_stream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * 8);
	if (!priv.enc_h265_stream.pstPack) {
		printf("Malloc failed!\r\n");
		return -1;
	}

	VENC_CHN_STATUS_S stStatus;
	s32Ret = CVI_VENC_QueryStatus(ch, &stStatus);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_QueryStatus failed with %#x\n", s32Ret);
		return s32Ret;
	}

	if (stStatus.u32CurPacks > 0) {
		s32Ret = CVI_VENC_GetStream(ch, &priv.enc_h265_stream, 1000);
		if (s32Ret != CVI_SUCCESS) {
			printf("CVI_VENC_GetStream failed with %#x\n", s32Ret);
			free(priv.enc_h265_stream.pstPack);
			return s32Ret;
		}
	} else {
		printf("CVI_VENC_QueryStatus find not pack\r\n");
		free(priv.enc_h265_stream.pstPack);
		return -1;
	}

	if (stream) {
		stream->count = priv.enc_h265_stream.u32PackCount;
		if (stream->count > 8) {
			printf("pack count is too large! cnt:%d\r\n", stream->count);
			free(priv.enc_h265_stream.pstPack);
			return -1;
		}
		for (int i = 0; i < stream->count; i++) {
			stream->data[i] = priv.enc_h265_stream.pstPack[i].pu8Addr + priv.enc_h265_stream.pstPack[i].u32Offset;
			stream->data_size[i] = priv.enc_h265_stream.pstPack[i].u32Len - priv.enc_h265_stream.pstPack[i].u32Offset;
		}
	}

	return s32Ret;
}

int mmf_enc_h265_free(int ch)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	if (!priv.enc_h265_running) {
		return s32Ret;
	}

	if (priv.enc_h265_stream.pstPack)
		free(priv.enc_h265_stream.pstPack);

	s32Ret = CVI_VENC_ReleaseStream(ch, &priv.enc_h265_stream);
	if (s32Ret != CVI_SUCCESS) {
		printf("CVI_VENC_ReleaseStream failed with %#x\n", s32Ret);
		return s32Ret;
	}

	priv.enc_h265_running = 0;
	return s32Ret;
}

int mmf_invert_format_to_maix(int mmf_format) {
	switch (mmf_format) {
		case PIXEL_FORMAT_RGB_888:
			return 0;
		case PIXEL_FORMAT_BGR_888:
			return 1;
		case PIXEL_FORMAT_ARGB_8888:
			return 3;
		case PIXEL_FORMAT_NV21:
			return 8;
		default:
			return 0xFF;
	}
}

int mmf_invert_format_to_mmf(int maix_format) {
	switch (maix_format) {
		case 0:
			return PIXEL_FORMAT_RGB_888;
		case 1:
			return PIXEL_FORMAT_BGR_888;
		case 3:
			return PIXEL_FORMAT_ARGB_8888;
		case 8:
			return PIXEL_FORMAT_NV21;
		default:
			return -1;
	}
}

int mmf_vb_config_of_vi(uint32_t size, uint32_t count)
{
	priv.vb_size_of_vi = size;
	priv.vb_count_of_vi = count;
	priv.vb_of_vi_is_config = 1;
	return 0;
}

int mmf_vb_config_of_vo(uint32_t size, uint32_t count)
{
	priv.vb_size_of_vo = size;
	priv.vb_count_of_vo = count;
	priv.vb_of_vo_is_config = 1;
	return 0;
}

int mmf_vb_config_of_private(uint32_t size, uint32_t count)
{
	priv.vb_size_of_private = size;
	priv.vb_count_of_private = count;
	priv.vb_of_private_is_config = 1;
	return 0;
}

int mmf_set_exp_mode(int ch, int mode)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;

	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));

	CVI_ISP_GetExposureAttr(ch, &stExpAttr);

	stExpAttr.u8DebugMode = 0;

	if (mode == 0) {
		stExpAttr.bByPass = 0;
		stExpAttr.enOpType = OP_TYPE_AUTO;
		stExpAttr.stManual.enExpTimeOpType = OP_TYPE_AUTO;
		stExpAttr.stManual.enISONumOpType = OP_TYPE_AUTO;
		printf("AE Auto!\n");
	} else if (mode == 1) {
		stExpAttr.bByPass = 0;
		stExpAttr.enOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enISONumOpType = OP_TYPE_MANUAL;
		stExpAttr.stManual.enGainType = AE_TYPE_ISO;
		printf("AE Manual!\n");
	}

	CVI_ISP_SetExposureAttr(ch, &stExpAttr);

	//usleep(100 * 1000);
	return 0;
}

// 0, auto; 1, manual
int mmf_get_exp_mode(int ch)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	return stExpAttr.enOpType;
}

int mmf_get_exptime(int ch, uint32_t *exptime)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);
	if (exptime) {
		*exptime = stExpInfo.u32ExpTime;
	}
	return 0;
}

int mmf_set_exptime(int ch, uint32_t exptime)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ExpTime = exptime;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_iso_num(int ch, uint32_t *iso_num)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);
	if (iso_num) {
		*iso_num = stExpInfo.u32ISO;
	}
	return 0;
}

int mmf_set_iso_num(int ch, uint32_t iso_num)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ISONum = iso_num;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_exptime_and_iso(int ch, uint32_t *exptime, uint32_t *iso_num)
{
	ISP_EXP_INFO_S stExpInfo;
	memset(&stExpInfo, 0, sizeof(ISP_EXP_INFO_S));
	CVI_ISP_QueryExposureInfo(ch, &stExpInfo);

	if (exptime) {
		*exptime = stExpInfo.u32ExpTime;
	}

	if (iso_num) {
		*iso_num = stExpInfo.u32ISO;
	}
	return 0;
}

int mmf_set_exptime_and_iso(int ch, uint32_t exptime, uint32_t iso_num)
{
	ISP_EXPOSURE_ATTR_S stExpAttr;
	memset(&stExpAttr, 0, sizeof(ISP_EXPOSURE_ATTR_S));
	CVI_ISP_GetExposureAttr(ch, &stExpAttr);
	if (stExpAttr.enOpType == OP_TYPE_MANUAL) {
		stExpAttr.stManual.u32ExpTime = exptime;
		stExpAttr.stManual.u32ISONum = iso_num;
		CVI_ISP_SetExposureAttr(ch, &stExpAttr);
		//usleep(100 * 1000);
	} else {
		return -1;
	}

	return 0;
}

int mmf_get_sensor_id(void)
{
	switch (priv.sensor_type) {
	case GCORE_GC4653_MIPI_4M_30FPS_10BIT: return 0x4653;
	default: return -1;
	}

	return -1;
}

void mmf_set_vi_hmirror(int ch, bool en)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	g_priv.vi_hmirror[ch] = en;
}

void mmf_set_vi_vflip(int ch, bool en)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	g_priv.vi_vflip[ch] = en;
}

void mmf_set_vo_video_hmirror(int ch, bool en)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	g_priv.vo_video_hmirror[ch] = en;
}

void mmf_set_vo_video_flip(int ch, bool en)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	g_priv.vo_video_vflip[ch] = en;
}

void mmf_set_constrast(int ch, uint32_t val)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Contrast = val;
	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}

void mmf_set_saturation(int ch, uint32_t val)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;

	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Saturation = val;

	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}


void mmf_set_luma(int ch, uint32_t val)
{
	if (ch > MMF_VI_MAX_CHN) {
		printf("invalid ch, must be [0, %d)\r\n", ch);
		return;
	}

	val = val > 0x64 ? 0x64 : val;

	ISP_CSC_ATTR_S stCscAttr;
	memset(&stCscAttr, 0, sizeof(ISP_CSC_ATTR_S));

	CVI_ISP_GetCSCAttr(ch, &stCscAttr);
	stCscAttr.Enable = true;
	stCscAttr.Luma = val;
	CVI_ISP_SetCSCAttr(ch, &stCscAttr);
}