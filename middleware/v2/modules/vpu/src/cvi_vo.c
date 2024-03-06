#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <math.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cvi_sys.h"
#include <linux/cvi_comm_vo.h>
#include "cvi_buffer.h"

#include "cvi_base.h"
#include "cvi_vo.h"
#include "cvi_gdc.h"
#include "cvi_vb.h"
#include "gdc_mesh.h"
#include "vo_ioctl.h"
#include <linux/vo_uapi.h>
#include <linux/cvi_vo_ctx.h>

#define CHECK_VO_DEV_VALID(VoDev) do {									\
		if ((VoDev >= VO_MAX_DEV_NUM) || (VoDev < 0)) {						\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) invalid.\n", VoDev);			\
			return CVI_ERR_VO_INVALID_DEVID;						\
		}											\
	} while (0)

#define CHECK_VO_LAYER_VALID(VoLayer) do {								\
		if ((VoLayer >= VO_MAX_LAYER_NUM) || (VoLayer < 0)) {					\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) invalid.\n", VoLayer);			\
			return CVI_ERR_VO_INVALID_LAYERID;						\
		}											\
	} while (0)

#define CHECK_VO_CHN_VALID(VoLayer, VoChn) do {							\
		if ((VoLayer >= VO_MAX_LAYER_NUM) || (VoLayer < 0)) {					\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) invalid.\n", VoLayer);			\
			return CVI_ERR_VO_INVALID_LAYERID;						\
		}											\
		if ((VoChn >= VO_MAX_CHN_NUM) || (VoChn < 0)) {						\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoChn(%d) invalid.\n", VoChn);			\
			return CVI_ERR_VO_INVALID_CHNID;						\
		}											\
	} while (0)

#define CHECK_VO_LAYER_ENABLE(VoLayer) do {								\
		if (!gVoCtx->is_layer_enable[VoLayer]) {							\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) isn't enabled yet.\n", VoLayer);		\
			return CVI_ERR_VO_VIDEO_NOT_ENABLED;						\
		}											\
	} while (0)

#define CHECK_VO_LAYER_DISABLE(VoLayer) do {								\
		if (gVoCtx->is_layer_enable[VoLayer]) {							\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) already enabled.\n", VoLayer);		\
			return CVI_ERR_VO_VIDEO_NOT_DISABLED;						\
		}											\
	} while (0)

#define CHECK_VO_CHN_ENABLE(VoLayer, VoChn) do {							\
		if (!gVoCtx->is_chn_enable[VoLayer][VoChn]) {						\
			CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) isn't enabled yet.\n", VoLayer, VoChn);\
			return CVI_ERR_VO_CHN_NOT_ENABLED;						\
		}											\
	} while (0)

VO_SYNC_INFO_S stSyncInfo[VO_OUTPUT_BUTT] = {
	[VO_OUTPUT_800x600_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 600, .u16Vbb = 24, .u16Vfb = 1
		, .u16Hact = 800, .u16Hbb = 88, .u16Hfb = 40
		, .u16Vpw = 4, .u16Hpw = 128, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P24] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 24
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 638
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P25] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 25
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P30] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 30
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 440
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 720, .u16Vbb = 20, .u16Vfb = 5
		, .u16Hact = 1280, .u16Hbb = 220, .u16Hfb = 110
		, .u16Vpw = 5, .u16Hpw = 40, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 528
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_1080P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1080, .u16Vbb = 36, .u16Vfb = 4
		, .u16Hact = 1920, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 5, .u16Hpw = 44, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_576P50] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 50
		, .u16Vact = 576, .u16Vbb = 39, .u16Vfb = 5
		, .u16Hact = 720, .u16Hbb = 68, .u16Hfb = 12
		, .u16Vpw = 5, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_480P60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 480, .u16Vbb = 30, .u16Vfb = 9
		, .u16Hact = 720, .u16Hbb = 60, .u16Hfb = 16
		, .u16Vpw = 6, .u16Hpw = 62, .bIdv = 0, .bIhs = 0, .bIvs = 0},
	[VO_OUTPUT_720x1280_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1280, .u16Vbb = 4, .u16Vfb = 6
		, .u16Hact = 720, .u16Hbb = 36, .u16Hfb = 128
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_1080x1920_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 1920, .u16Vbb = 36, .u16Vfb = 6
		, .u16Hact = 1080, .u16Hbb = 148, .u16Hfb = 88
		, .u16Vpw = 16, .u16Hpw = 64, .bIdv = 0, .bIhs = 0, .bIvs = 1},
	[VO_OUTPUT_480x800_60] = {.bSynm = 1, .bIop = 1, .u16FrameRate = 60
		, .u16Vact = 800, .u16Vbb = 20, .u16Vfb = 20
		, .u16Hact = 480, .u16Hbb = 50, .u16Hfb = 50
		, .u16Vpw = 10, .u16Hpw = 10, .bIdv = 0, .bIhs = 0, .bIvs = 1},
};
static pthread_once_t once = PTHREAD_ONCE_INIT;
//static pthread_once_t vo_once = PTHREAD_ONCE_INIT;

struct vo_pm_s {
	VO_PM_OPS_S	stOps;
	CVI_VOID	*pvData;
};
static struct vo_pm_s apstVoPm[VO_MAX_DEV_NUM] = { 0 };

enum i80_op_type {
	I80_OP_GO = 0,
	I80_OP_TIMER,
	I80_OP_DONE,
	I80_OP_MAX,
};

enum i80_ctrl_type {
	I80_CTRL_CMD = 0,
	I80_CTRL_DATA,
	I80_CTRL_EOL = I80_CTRL_DATA,
	I80_CTRL_EOF,
	I80_CTRL_END = I80_CTRL_EOF,
	I80_CTRL_MAX
};

static CVI_U8 i80_ctrl[I80_CTRL_MAX] = { 0x31, 0x75, 0xff };

struct cvi_vo_ctx *gVoCtx;

static CVI_S32 _check_vo_exist(CVI_S32 *pfd)
{
	*pfd = get_vo_fd();
	if (*pfd == -1) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Maybe no VO dev?\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

static CVI_S32 _vo_proc_mmap(void)
{
	CVI_S32 fd = -1;

	fd = get_vo_fd();

	if (gVoCtx != NULL) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "vo proc has already done mmap\n");
		return CVI_SUCCESS;
	}

	gVoCtx = (struct cvi_vo_ctx *)mmap(NULL, VO_SHARE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (gVoCtx == MAP_FAILED) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo proc mmap fail!\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void vo_init(void)
{
	if (_vo_proc_mmap() != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "_vo_proc_mmap failed");
	}
}

static CVI_S32 _vo_proc_unmap(void)
{
	if (gVoCtx == NULL) {
		CVI_TRACE_VO(CVI_DBG_DEBUG, "VO proc no need to unmap\n");
		return CVI_SUCCESS;
	}

	munmap((void *)gVoCtx, VO_SHARE_MEM_SIZE);
	gVoCtx = NULL;

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Bin related APIs.
 **************************************************************************/
#define VO_BIN_GUARDMAGIC 0x12345678
static VO_BIN_INFO_S vo_bin_info = {
	.gamma_info = {
		.enable = CVI_FALSE,
		.osd_apply = CVI_FALSE,
		.value = {
			0,   3,   7,   11,  15,  19,  23,  27,
			31,  35,  39,  43,  47,  51,  55,  59,
			63,  67,  71,  75,  79,  83,  87,  91,
			95,  99,  103, 107, 111, 115, 119, 123,
			127, 131, 135, 139, 143, 147, 151, 155,
			159, 163, 167, 171, 175, 179, 183, 187,
			191, 195, 199, 203, 207, 211, 215, 219,
			223, 227, 231, 235, 239, 243, 247, 251,
			255
		}
	},
	.guard_magic = VO_BIN_GUARDMAGIC
};

VO_BIN_INFO_S *get_vo_bin_info_addr(void)
{
	return &vo_bin_info;
}

CVI_U32 get_vo_bin_guardmagic_code(void)
{
	return VO_BIN_GUARDMAGIC;
}
#if 0 // skip in FPGA test
static CVI_U32 _getFileSize(FILE *fp, CVI_U64 *size)
{
	CVI_S32 ret = CVI_SUCCESS;

	MOD_CHECK_NULL_PTR(CVI_ID_VO, fp);
	MOD_CHECK_NULL_PTR(CVI_ID_VO, size);

	fseek(fp, 0L, SEEK_END);
	*size = ftell(fp);
	rewind(fp);

	return ret;
}

static CVI_S32 CVI_VO_SetVOParamFromBin(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	FILE *fp = NULL;
	CVI_U8 *buf;
	CVI_CHAR binName[BIN_FILE_LENGTH];
	CVI_U64 file_size;

	ret = CVI_BIN_GetBinName(binName);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "GetBinName failure\n");
	}

	fp = fopen(binName, "rb");
	if (fp == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Cant find bin(%s)\n", binName);
		return CVI_FAILURE;
	}
	CVI_TRACE_VO(CVI_DBG_DEBUG, "Bin exist (%s)\n", binName);

	_getFileSize(fp, &file_size);

	buf = (CVI_U8 *)malloc(file_size);
	if (buf == NULL) {
		CVI_TRACE_VO(CVI_DBG_ERR, "Allocae memory failed!\n");
		fclose(fp);
		return CVI_FAILURE;
	}

	//fread info buffer and calling CVI_BIN
	fread(buf, file_size, 1, fp);
	ret = CVI_BIN_LoadParamFromBin(CVI_BIN_ID_VO, buf);
	free(buf);

	{
		struct vdev *d;
		//set gamma with HW from bin or default value
		d = get_dev_info(VDEV_TYPE_DISP, 0);
		vo_set_gamma_ctrl(d->fd, &vo_bin_info.gamma_info);
	}
	return CVI_SUCCESS;
}
#endif

void vo_layer_init(void)
{
	PROC_AMP_CTRL_S ctrl;

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
	}

	for (CVI_U8 i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i) {
		CVI_VO_GetLayerProcAmpCtrl(0, i, &ctrl);
		gVoCtx->proc_amp[i] = ctrl.default_value;
	}
}

CVI_S32 CVI_VO_Suspend(void)
{
	CVI_S32 s32Ret;
	CVI_S32 fd = -1;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (vo_sdk_suspend(fd) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "vo sdk suspend fail\n");
		return CVI_FAILURE;
	}

	for (VO_DEV VoDev = 0; VoDev < VO_MAX_DEV_NUM; ++VoDev) {
		if (apstVoPm[VoDev].stOps.pfnPanelSuspend) {
			s32Ret = apstVoPm[VoDev].stOps.pfnPanelSuspend(apstVoPm[VoDev].pvData);
			if (s32Ret != CVI_SUCCESS) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Panel[%d] suspend failed with %#x!\n", VoDev, s32Ret);
				return s32Ret;
			}
		}
	}

	CVI_TRACE_VO(CVI_DBG_DEBUG, "-\n");
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_Resume(void)
{
	VO_LAYER VoLayer = 0;
	VO_CHN VoChn = 0;
	CVI_S32 s32Ret;
	CVI_S32 fd = -1;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	for (VO_DEV VoDev = 0; VoDev < VO_MAX_DEV_NUM; ++VoDev) {
		if (!gVoCtx->is_dev_enable[VoDev])
			continue;

		if (apstVoPm[VoDev].stOps.pfnPanelResume) {
			s32Ret = apstVoPm[VoDev].stOps.pfnPanelResume(apstVoPm[VoDev].pvData);
			if (s32Ret != CVI_SUCCESS) {
				CVI_TRACE_VO(CVI_DBG_ERR, "Panel[%d] resume failed with %#x!\n", VoDev, s32Ret);
				return s32Ret;
			}
		}
		gVoCtx->is_dev_enable[VoDev] = CVI_FALSE;
		CVI_VO_SetPubAttr(VoDev, &gVoCtx->stPubAttr);
		gVoCtx->is_dev_enable[VoDev] = CVI_TRUE;
	}

	if (gVoCtx->is_layer_enable[VoLayer]) {
		gVoCtx->is_layer_enable[VoLayer] = CVI_FALSE;
		CVI_VO_SetVideoLayerAttr(VoLayer, &gVoCtx->stLayerAttr);
		gVoCtx->is_layer_enable[VoLayer] = CVI_TRUE;
	}
	if (gVoCtx->is_chn_enable[VoLayer][VoChn]) {
		if (vo_sdk_resume(fd) != CVI_SUCCESS) {
			CVI_TRACE_VO(CVI_DBG_ERR, "vo sdk resume fail\n");
			return CVI_FAILURE;
		}
	}

	CVI_TRACE_VO(CVI_DBG_DEBUG, "-\n");
	return CVI_SUCCESS;
}
/**************************************************************************
 *   Public APIs.
 **************************************************************************/
CVI_S32 CVI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr)
{
	CVI_S32 fd = -1;
	struct vo_pub_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoDev = VoDev;
	CVI_TRACE_VO(CVI_DBG_DEBUG, "VoDev(%d) INTF type(0x%x) Sync(%d)\n", VoDev
		    , pstPubAttr->enIntfType, pstPubAttr->enIntfSync);

	memcpy(&cfg.pstPubAttr, pstPubAttr,sizeof(VO_PUB_ATTR_S));

	if (VoDev == 0 && (!gVoCtx)) {
		//pthread_once(&vo_once, vo_init);
		CVI_TRACE_VO(CVI_DBG_DEBUG, "gVoCtx == null");
		vo_init();
	}

	if (vo_sdk_set_pubattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) Set Pub Attr fail\n", VoDev);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;

}

CVI_S32 CVI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	CVI_S32 fd = -1;
	struct vo_pub_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoDev = VoDev;

	if (vo_sdk_get_pubattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) Get Pub Attr fail\n", VoDev);
		return CVI_FAILURE;
	}

	memcpy(pstPubAttr, &cfg.pstPubAttr, sizeof(VO_PUB_ATTR_S));

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_I80Init(VO_DEV VoDev, const VO_I80_INSTR_S *pi80Instr, CVI_U8 size)
{
	CVI_S32 fd = -1;
	CVI_U32 sw_cmd;

	MOD_CHECK_NULL_PTR(CVI_ID_VO, pi80Instr);
	CHECK_VO_DEV_VALID(VoDev);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	if (gVoCtx->stPubAttr.enIntfType != VO_INTF_I80) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VO DEV(%d) interface(%d) is not I80.\n",
			     VoDev, gVoCtx->stPubAttr.enIntfType);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	vo_set_i80_sw_mode(fd, CVI_TRUE);
	for (int i = 0; i < size; i++) {
		if (pi80Instr[i].data_type > 1) {
			CVI_TRACE_VO(CVI_DBG_ERR, "VO I80 instr type(%d) invalid.\n", pi80Instr[i].data_type);
			return CVI_ERR_VO_ILLEGAL_PARAM;
		}

		sw_cmd = (i80_ctrl[pi80Instr[i].data_type] << 8) | pi80Instr[i].data;
		vo_send_i80_cmd(fd, sw_cmd);

		if (pi80Instr[i].delay)
			usleep(pi80Instr[i].delay);
	}
	// pull high i80-lane
	vo_send_i80_cmd(fd, 0xffff);
	vo_send_i80_cmd(fd, 0x2ffff);
	vo_set_i80_sw_mode(fd, CVI_FALSE);
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_Enable(VO_DEV VoDev)
{
	CVI_S32 fd = -1;
	struct vo_dev_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoDev = VoDev;

	if (vo_sdk_enable(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) Enable fail\n", VoDev);
		return CVI_FAILURE;
	}

#if 0 // skip in FPGA test
	//set vo parameter if bin has parameters
	CVI_VO_SetVOParamFromBin();
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_Disable(VO_DEV VoDev)
{
	CVI_S32 fd = -1;
	struct vo_dev_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoDev = VoDev;

	if (vo_sdk_disable(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) Disable fail\n", VoDev);
		return CVI_FAILURE;
	}

	if (_vo_proc_unmap() != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "_vo_proc_mmap failed");
		return CVI_ERR_VO_NO_MEM;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SetVideoLayerCSC(VO_LAYER VoLayer, const VO_CSC_S *pstVideoCSC)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, pstVideoCSC);
	CHECK_VO_LAYER_VALID(VoLayer);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetVideoLayerCSC(VO_LAYER VoLayer, VO_CSC_S *pstVideoCSC)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, pstVideoCSC);
	CHECK_VO_LAYER_VALID(VoLayer);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_EnableVideoLayer(VO_LAYER VoLayer)
{
	CVI_S32 fd = -1;
	struct vo_video_layer_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	if (vo_sdk_enable_videolayer(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Enable Video Layer fail\n", VoLayer);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_DisableVideoLayer(VO_LAYER VoLayer)
{
	CVI_S32 fd = -1;
	struct vo_video_layer_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	if (vo_sdk_disable_videolayer(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Disable Video Layer fail\n", VoLayer);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SetVideoLayerAttr(VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	CVI_S32 fd = -1;
	struct vo_video_layer_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	memcpy(&cfg.pstLayerAttr, pstLayerAttr, sizeof(VO_VIDEO_LAYER_ATTR_S));

	if (vo_sdk_set_videolayerattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Video Layer Attr fail\n", VoLayer);
		return CVI_FAILURE;
	}

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	if (IS_FMT_YUV(gVoCtx->stLayerAttr.enPixFormat))
		pthread_once(&once, vo_layer_init);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetVideoLayerAttr(VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	CVI_S32 fd = -1;
	struct vo_video_layer_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;

	if (vo_sdk_get_videolayerattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Get Video Layer Attr fail\n", VoLayer);
		return CVI_FAILURE;
	}
	memcpy(pstLayerAttr, &cfg.pstLayerAttr, sizeof(VO_VIDEO_LAYER_ATTR_S));

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetLayerProcAmpCtrl(VO_LAYER VoLayer, PROC_AMP_E type, PROC_AMP_CTRL_S *ctrl)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, ctrl);
	CHECK_VO_LAYER_VALID(VoLayer);

	if (!IS_FMT_YUV(gVoCtx->stLayerAttr.enPixFormat)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}
	if (type >= PROC_AMP_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) ProcAmp type(%d) invalid.\n", VoLayer, type);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	static PROC_AMP_CTRL_S ctrls[PROC_AMP_MAX] = {
		{ .minimum = 0, .maximum = 255, .step = 1, .default_value = 128 },
		{ .minimum = 0, .maximum = 255, .step = 1, .default_value = 128 },
		{ .minimum = 0, .maximum = 255, .step = 1, .default_value = 128 },
		{ .minimum = 0, .maximum = 359, .step = 1, .default_value = 0 },
	};

	*ctrl = ctrls[type];
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetLayerProcAmp(VO_LAYER VoLayer, PROC_AMP_E type, CVI_S32 *value)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, value);
	CHECK_VO_LAYER_VALID(VoLayer);

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	if (!IS_FMT_YUV(gVoCtx->stLayerAttr.enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Only YUV format support.\n", VoLayer);
		return CVI_ERR_VO_NOT_SUPPORT;
	}
	if (type >= PROC_AMP_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) ProcAmp type(%d) invalid.\n", VoLayer, type);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	*value = gVoCtx->proc_amp[type];
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SetLayerProcAmp(VO_LAYER VoLayer, PROC_AMP_E type, CVI_S32 value)
{
	PROC_AMP_CTRL_S ctrl;
	CVI_S32 fd = -1;

	CHECK_VO_LAYER_VALID(VoLayer);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	if (!IS_FMT_YUV(gVoCtx->stLayerAttr.enPixFormat)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Only YUV format support.\n", VoLayer);
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (type >= PROC_AMP_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) ProcAmp type(%d) invalid.\n", VoLayer, type);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	CVI_VO_GetLayerProcAmpCtrl(VoLayer, type, &ctrl);
	if ((value > ctrl.maximum) || (value < ctrl.minimum)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) new value(%d) out of range(%d ~ %d).\n"
			, VoLayer, value, ctrl.minimum, ctrl.maximum);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}

	gVoCtx->proc_amp[type] = value;

	struct cvi_csc_cfg csc_cfg;
	CVI_S32 b = (gVoCtx->proc_amp[PROC_AMP_BRIGHTNESS] >> 1) - 64;
	float c = (float)gVoCtx->proc_amp[PROC_AMP_CONTRAST] / 128;
	float s = (float)gVoCtx->proc_amp[PROC_AMP_SATURATION] / 128;
	float h = (float)gVoCtx->proc_amp[PROC_AMP_HUE] * 2 * PI / 360;
	float A = cos(h) * c * s;
	float B = sin(h) * c * s;
	float tmp;

	if (b > 0) {
		csc_cfg.sub[0] = 0;
		csc_cfg.add[0] = b * c;
		csc_cfg.add[1] = csc_cfg.add[0];
		csc_cfg.add[2] = csc_cfg.add[0];
	} else {
		csc_cfg.sub[0] = abs(b);
		csc_cfg.add[0] = 0;
		csc_cfg.add[1] = 0;
		csc_cfg.add[2] = 0;
	}
	csc_cfg.sub[1] = 128;
	csc_cfg.sub[2] = 128;

	csc_cfg.coef[0][0] = c * BIT(10);
	tmp = B * -1.402;
	csc_cfg.coef[0][1] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);
	tmp = A * 1.402;
	csc_cfg.coef[0][2] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);
	csc_cfg.coef[1][0] = c * BIT(10);
	tmp = A * -0.344 + B * 0.714;
	csc_cfg.coef[1][1] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);
	tmp = B * -0.344 + A * -0.714;
	csc_cfg.coef[1][2] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);
	csc_cfg.coef[2][0] = c * BIT(10);
	tmp = A * 1.772;
	csc_cfg.coef[2][1] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);
	tmp = B * 1.772;
	csc_cfg.coef[2][2] = (tmp >= 0) ? tmp * BIT(10) : (CVI_U16)((-tmp) * BIT(10)) | BIT(13);

	vo_set_csc(fd, &csc_cfg);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, csc_cfg.coef[0][0], csc_cfg.coef[0][1]
		, csc_cfg.coef[0][2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, csc_cfg.coef[1][0], csc_cfg.coef[1][1]
		, csc_cfg.coef[1][2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, csc_cfg.coef[2][0], csc_cfg.coef[2][1]
		, csc_cfg.coef[2][2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, csc_cfg.sub[0], csc_cfg.sub[1], csc_cfg.sub[2]);
	CVI_TRACE_VO(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, csc_cfg.add[0], csc_cfg.add[1], csc_cfg.add[2]);

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, const VO_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 fd = -1;
	struct vo_chn_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	memcpy(&cfg.pstChnAttr, pstChnAttr ,sizeof(VO_CHN_ATTR_S));

	if (vo_sdk_set_chnattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Chn Attr fail\n", VoLayer);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetChnAttr(VO_LAYER VoLayer, VO_CHN VoChn, VO_CHN_ATTR_S *pstChnAttr)
{
	CVI_S32 fd = -1;
	struct vo_chn_attr_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	if (vo_sdk_get_chnattr(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Chn Attr fail\n", VoLayer);
		return CVI_FAILURE;
	}

	memcpy(pstChnAttr, &cfg.pstChnAttr, sizeof(VO_CHN_ATTR_S));

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetScreenFrame(VO_LAYER VoLayer, VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, pstVideoFrame);
	CHECK_VO_LAYER_VALID(VoLayer);

	UNUSED(s32MilliSec);
	return CVI_ERR_VO_NOT_SUPPORT;
}

CVI_S32 CVI_VO_ReleaseScreenFrame(VO_LAYER VoLayer, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	MOD_CHECK_NULL_PTR(CVI_ID_VO, pstVideoFrame);
	CHECK_VO_LAYER_VALID(VoLayer);
	return CVI_ERR_VO_NOT_SUPPORT;
}

CVI_S32 CVI_VO_SetDisplayBufLen(VO_LAYER VoLayer, CVI_U32 u32BufLen)
{
	CVI_S32 fd = -1;
	struct vo_display_buflen_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.u32BufLen = u32BufLen;

	if (vo_sdk_set_displaybuflen(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Display BufLen (%d)fail\n", VoLayer, u32BufLen);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetDisplayBufLen(VO_LAYER VoLayer, CVI_U32 *pu32BufLen)
{
	CVI_S32 fd = -1;
	struct vo_display_buflen_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;

	if (vo_sdk_get_displaybuflen(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Get Display BufLen (%d)fail\n", VoLayer, cfg.u32BufLen);
		return CVI_FAILURE;
	}

	*pu32BufLen = cfg.u32BufLen;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_EnableChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 fd = -1;
	struct vo_chn_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	if (vo_sdk_enable_chn(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Enable Chn(%d) fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_DisableChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 fd = -1;
	struct vo_chn_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	if (vo_sdk_disable_chn(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Disable Chn(%d) fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SetChnRotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E enRotation)
{
	CVI_S32 fd = -1;
	struct vo_chn_rotation_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;
	cfg.enRotation = enRotation;
	if (enRotation == ROTATION_180) {
		CVI_TRACE_VO(CVI_DBG_ERR, "not support rotation(%d).\n", enRotation);
		return CVI_ERR_VO_NOT_SUPPORT;
	} else if (enRotation >= ROTATION_MAX) {
		CVI_TRACE_VO(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_VO_ILLEGAL_PARAM;
	}
	if (vo_sdk_set_chnrotation(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Chn(%d) Rotation fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetChnRotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation)
{
	CVI_S32 fd = -1;
	struct vo_chn_rotation_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	if (vo_sdk_get_chnrotation(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Set Chn(%d) Rotation fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	*penRotation = cfg.enRotation;
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_SendFrame(VO_LAYER VoLayer, VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstVideoFrame, CVI_S32 s32MilliSec)
{
	CVI_S32 fd = -1;
	struct vo_snd_frm_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;
	cfg.s32MilliSec = s32MilliSec;

	memcpy(&cfg.stVideoFrame, pstVideoFrame, sizeof(VIDEO_FRAME_INFO_S));

	if (vo_sdk_send_frame(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Chn(%d) send frame fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_ClearChnBuf(VO_LAYER VoLayer, VO_CHN VoChn, CVI_BOOL bClrAll)
{
	CVI_S32 fd = -1;
	struct vo_clear_chn_buf_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;
	cfg.bClrAll = bClrAll;

	if (vo_sdk_clearchnbuf(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Chn(%d) clean buf fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_ShowChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 fd = -1;

	CHECK_VO_CHN_VALID(VoLayer, VoChn);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (vo_set_pattern(fd, CVI_VIP_PAT_OFF) != 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Pattern Off failed.\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	gVoCtx->show = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_HideChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CVI_S32 fd = -1;

	CHECK_VO_CHN_VALID(VoLayer, VoChn);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (vo_set_pattern(fd, CVI_VIP_PAT_BLACK) != 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) Pattern Black failed.\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}
	gVoCtx->show = CVI_FALSE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_ShowPattern(VO_DEV VoDev, enum VO_PATTERN_MODE PatternId)
{
	CVI_S32 fd = -1;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (vo_set_pattern(fd, (enum cvi_vip_pattern)PatternId) != 0) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) set Pattern failed.\n", VoDev);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_CloseFd(void)
{
	vo_close();
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_PauseChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CHECK_VO_CHN_VALID(VoLayer, VoChn);

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	gVoCtx->pause = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_ResumeChn(VO_LAYER VoLayer, VO_CHN VoChn)
{
	CHECK_VO_CHN_VALID(VoLayer, VoChn);

	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	gVoCtx->pause = CVI_FALSE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_Get_Panel_Status(VO_LAYER VoLayer, VO_CHN VoChn, CVI_U32 *is_init)
{
	CVI_S32 fd = -1;
	struct vo_panel_status_cfg cfg;

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	cfg.VoLayer = VoLayer;
	cfg.VoChn = VoChn;

	if (vo_sdk_get_panelstatue(fd, &cfg) != CVI_SUCCESS) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) Chn(%d) Get Panel Status fail\n", VoLayer, VoChn);
		return CVI_FAILURE;
	}

	memcpy(&is_init, &cfg.is_init, sizeof(is_init));

	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_RegPmCallBack(VO_DEV VoDev, VO_PM_OPS_S *pstPmOps, void *pvData)
{
	CHECK_VO_DEV_VALID(VoDev);
	MOD_CHECK_NULL_PTR(CVI_ID_VO, pstPmOps);

	apstVoPm[VoDev].stOps = *pstPmOps;
	apstVoPm[VoDev].pvData = pvData;
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_UnRegPmCallBack(VO_DEV VoDev)
{
	CHECK_VO_DEV_VALID(VoDev);

	memset(&apstVoPm[VoDev].stOps, 0, sizeof(apstVoPm[VoDev].stOps));
	apstVoPm[VoDev].pvData = NULL;
	return CVI_SUCCESS;
}

CVI_BOOL CVI_VO_IsEnabled(VO_DEV VoDev)
{
	if (!gVoCtx) {
		CVI_TRACE_VO(CVI_DBG_NOTICE, "VO context not init yet.\n");
		return CVI_FAILURE;
	}

	return gVoCtx->is_dev_enable[VoDev];
}

CVI_S32 CVI_VO_SetGammaInfo(VO_GAMMA_INFO_S *pinfo)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_VO, pinfo);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (!CVI_VO_IsEnabled(0))
		return CVI_ERR_VO_SYS_NOTREADY;

	vo_set_gamma_ctrl(fd, pinfo);

	memcpy(&vo_bin_info.gamma_info, pinfo, sizeof(VO_GAMMA_INFO_S));
	vo_bin_info.guard_magic = VO_BIN_GUARDMAGIC;
	return CVI_SUCCESS;
}

CVI_S32 CVI_VO_GetGammaInfo(VO_GAMMA_INFO_S *pinfo)
{
	CVI_S32 fd = -1;

	MOD_CHECK_NULL_PTR(CVI_ID_VO, pinfo);

	if (_check_vo_exist(&fd)) {
		return CVI_ERR_VO_NOT_SUPPORT;
	}

	if (!CVI_VO_IsEnabled(0))
		return CVI_ERR_VO_SYS_NOTREADY;

	//calling HW
	vo_get_gamma_ctrl(fd, pinfo);

	return CVI_SUCCESS;
}


