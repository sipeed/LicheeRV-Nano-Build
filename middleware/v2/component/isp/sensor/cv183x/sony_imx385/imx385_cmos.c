#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include "cvi_type.h"
#include "cvi_debug.h"
#include "cvi_comm_sns.h"
#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"

#include "imx385_cmos_ex.h"
#include "imx385_cmos_param.h"
#include <linux/cvi_vip_snsr.h>

#define DIV_0_TO_1(a)   ((0 == (a)) ? 1 : (a))
#define DIV_0_TO_1_FLOAT(a) ((((a) < 1E-10) && ((a) > -1E-10)) ? 1 : (a))
#define IMX385_ID 385
#define SENSOR_IMX385_WIDTH 1920
#define SENSOR_IMX385_HEIGHT 1080
/****************************************************************************
 * global variables                                                         *
 ****************************************************************************/

ISP_SNS_STATE_S *g_pastImx385[VI_MAX_PIPE_NUM] = {CVI_NULL};

#define IMX385_SENSOR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastImx385[dev])
#define IMX385_SENSOR_SET_CTX(dev, pstCtx)   (g_pastImx385[dev] = pstCtx)
#define IMX385_SENSOR_RESET_CTX(dev)         (g_pastImx385[dev] = CVI_NULL)

ISP_SNS_COMMBUS_U g_aunImx385_BusInfo[VI_MAX_PIPE_NUM] = {
	[0] = { .s8I2cDev = 0},
	[1 ... VI_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

CVI_U16 g_au16Imx385_GainMode[VI_MAX_PIPE_NUM] = {0};

IMX385_STATE_S g_astImx385_State[VI_MAX_PIPE_NUM] = {{0} };
ISP_SNS_MIRRORFLIP_TYPE_E g_aeImx385_MirrorFip[VI_MAX_PIPE_NUM] = {0};

/****************************************************************************
 * local variables and functions                                            *
 ****************************************************************************/
static ISP_FSWDR_MODE_E genFSWDRMode[VI_MAX_PIPE_NUM] = {
	[0 ... VI_MAX_PIPE_NUM - 1] = ISP_FSWDR_NORMAL_MODE
};

static CVI_U32 gu32MaxTimeGetCnt[VI_MAX_PIPE_NUM] = {0};
static CVI_U32 g_au32InitExposure[VI_MAX_PIPE_NUM]  = {0};
static CVI_U32 g_au32LinesPer500ms[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16InitWBGain[VI_MAX_PIPE_NUM][3] = {{0} };
static CVI_U16 g_au16SampleRgain[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16SampleBgain[VI_MAX_PIPE_NUM] = {0};
static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg);
/*****Imx385 Lines Range*****/
#define IMX385_FULL_LINES_MAX  (0x1FFFF)
#define IMX385_FULL_LINES_MAX_2TO1_WDR  (0x1FFFF)    // considering the YOUT_SIZE and bad frame
#define IMX385_VMAX_1080P30_LINEAR	1125

/*****Imx385 Register Address*****/
#define IMX385_HOLD_ADDR		0x3001
#define IMX385_SHS1_ADDR		0x3020
#define IMX385_SHS2_ADDR		0x3023
#define IMX385_GAIN_ADDR		0x3014
#define IMX385_GAIN1_ADDR		0x30F2
#define IMX385_HCG_ADDR			0x3009
#define IMX385_VMAX_ADDR		0x3018
#define IMX385_YOUT_ADDR		0x3357
#define IMX385_RHS1_ADDR		0x302C
#define IMX385_TABLE_END		0xffff

#define IMX385_RES_IS_1080P(w, h)      ((w) <= 1920 && (h) <= 1080)

static CVI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstAeSnsDft);
	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
#if 0
	memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));
#endif
	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FlickerFreq = 50 * 256;
	pstAeSnsDft->u32FullLinesMax = IMX385_FULL_LINES_MAX;
	pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * 30);

	pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
	pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
	pstAeSnsDft->stIntTimeAccu.f32Offset = 0;

	pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
	pstAeSnsDft->stAgainAccu.f32Accuracy = 1;

	pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
	pstAeSnsDft->stDgainAccu.f32Accuracy = 1;

	pstAeSnsDft->u32ISPDgainShift = 8;
	pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
	pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

	if (g_au32LinesPer500ms[ViPipe] == 0)
		pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * 30 / 2;
	else
		pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
	pstAeSnsDft->u32SnsStableFrame = 8;
#if 0
	pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_0;
	pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_32_0;

	pstAeSnsDft->bAERouteExValid = CVI_FALSE;
	pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
	pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;
#endif
	switch (pstSnsState->enWDRMode) {
	default:
	case WDR_MODE_NONE:   /*linear mode*/
		pstAeSnsDft->f32Fps = g_astImx385_mode[IMX385_MODE_1080P30].f32MaxFps;
		pstAeSnsDft->f32MinFps = g_astImx385_mode[IMX385_MODE_1080P30].f32MinFps;
		pstAeSnsDft->au8HistThresh[0] = 0xd;
		pstAeSnsDft->au8HistThresh[1] = 0x28;
		pstAeSnsDft->au8HistThresh[2] = 0x60;
		pstAeSnsDft->au8HistThresh[3] = 0x80;

		pstAeSnsDft->u32MaxAgain = 32381;
		pstAeSnsDft->u32MinAgain = 1024;
		pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
		pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

		pstAeSnsDft->u32MaxDgain = 128914;
		pstAeSnsDft->u32MinDgain = 1024;
		pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
		pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

		pstAeSnsDft->u8AeCompensation = 40;
		pstAeSnsDft->u32InitAESpeed = 64;
		pstAeSnsDft->u32InitAETolerance = 5;
		pstAeSnsDft->u32AEResponseFrame = 4;
		pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;
		pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 76151;

		pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
		pstAeSnsDft->u32MinIntTime = 2;
		pstAeSnsDft->u32MaxIntTimeTarget = 65535;
		pstAeSnsDft->u32MinIntTimeTarget = 1;
		break;

	case WDR_MODE_2To1_LINE:
		pstAeSnsDft->f32Fps = g_astImx385_mode[IMX385_MODE_1080P30_WDR].f32MaxFps;
		pstAeSnsDft->f32MinFps = g_astImx385_mode[IMX385_MODE_1080P30_WDR].f32MinFps;
		pstAeSnsDft->au8HistThresh[0] = 0xC;
		pstAeSnsDft->au8HistThresh[1] = 0x18;
		pstAeSnsDft->au8HistThresh[2] = 0x60;
		pstAeSnsDft->au8HistThresh[3] = 0x80;

		pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
		pstAeSnsDft->u32MinIntTime = 2;

		pstAeSnsDft->u32MaxIntTimeTarget = 65535;
		pstAeSnsDft->u32MinIntTimeTarget = pstAeSnsDft->u32MinIntTime;

		pstAeSnsDft->u32MaxAgain = 32381;
		pstAeSnsDft->u32MinAgain = 1024;
		pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
		pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

		pstAeSnsDft->u32MaxDgain = 128914;
		pstAeSnsDft->u32MinDgain = 1024;
		pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
		pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;
		pstAeSnsDft->u32MaxISPDgainTarget = 16 << pstAeSnsDft->u32ISPDgainShift;

		pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 52000;
		pstAeSnsDft->u32InitAESpeed = 64;
		pstAeSnsDft->u32InitAETolerance = 5;
		pstAeSnsDft->u32AEResponseFrame = 4;
		if (genFSWDRMode[ViPipe] == ISP_FSWDR_LONG_FRAME_MODE) {
			pstAeSnsDft->u8AeCompensation = 64;
			pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;
		} else {
			pstAeSnsDft->u8AeCompensation = 40;
			pstAeSnsDft->enAeExpMode = AE_EXP_LOWLIGHT_PRIOR;
			/* [TODO] */
#if 0
			pstAeSnsDft->u16ManRatioEnable = CVI_TRUE;
			pstAeSnsDft->au32Ratio[0] = 0x400;
			pstAeSnsDft->au32Ratio[1] = 0x40;
			pstAeSnsDft->au32Ratio[2] = 0x40;
#endif
		}
		break;
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_fps_set(VI_PIPE ViPipe, CVI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	CVI_U32 u32VMAX = IMX385_VMAX_1080P30_LINEAR;
	CVI_FLOAT f32MaxFps = 0;
	CVI_FLOAT f32MinFps = 0;
	CVI_U32 u32Vts = 0;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;

	CMOS_CHECK_POINTER(pstAeSnsDft);
	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u32Vts = g_astImx385_mode[pstSnsState->u8ImgMode].u32VtsDef;
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;
	f32MaxFps = g_astImx385_mode[pstSnsState->u8ImgMode].f32MaxFps;
	f32MinFps = g_astImx385_mode[pstSnsState->u8ImgMode].f32MinFps;

	switch (pstSnsState->u8ImgMode) {
	case IMX385_MODE_1080P30_WDR:
		if ((f32Fps <= f32MaxFps) && (f32Fps >= f32MinFps)) {
			u32VMAX = u32Vts * f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport Fps: %f\n", f32Fps);
			return CVI_FAILURE;
		}
		u32VMAX = (u32VMAX > IMX385_FULL_LINES_MAX_2TO1_WDR) ? IMX385_FULL_LINES_MAX_2TO1_WDR : u32VMAX;
		break;

	case IMX385_MODE_1080P30:
		if ((f32Fps <= f32MaxFps) && (f32Fps >= f32MinFps)) {
			u32VMAX = u32Vts * f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport Fps: %f\n", f32Fps);
			return CVI_FAILURE;
		}
		u32VMAX = (u32VMAX > IMX385_FULL_LINES_MAX) ? IMX385_FULL_LINES_MAX : u32VMAX;
		break;
	default:
		CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport sensor mode: %d\n", pstSnsState->u8ImgMode);
		return CVI_FAILURE;
	}

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		pstSnsRegsInfo->astI2cData[LINEAR_VMAX_0].u32Data = (u32VMAX & 0xFF);
		pstSnsRegsInfo->astI2cData[LINEAR_VMAX_1].u32Data = ((u32VMAX & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[LINEAR_VMAX_2].u32Data = ((u32VMAX & 0x10000) >> 16);
	} else {
		pstSnsRegsInfo->astI2cData[DOL2_VMAX_0].u32Data = (u32VMAX & 0xFF);
		pstSnsRegsInfo->astI2cData[DOL2_VMAX_1].u32Data = ((u32VMAX & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[DOL2_VMAX_2].u32Data = ((u32VMAX & 0x10000) >> 16);
	}

	if (WDR_MODE_2To1_LINE == pstSnsState->enWDRMode) {
		pstSnsState->u32FLStd = u32VMAX * 2;
		g_astImx385_State[ViPipe].u32RHS1_MAX = (u32VMAX - g_astImx385_State[ViPipe].u32BRL) * 2 - 51;
	} else {
		pstSnsState->u32FLStd = u32VMAX;
	}

	pstAeSnsDft->f32Fps = f32Fps;
	pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 2;
	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
	pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * DIV_0_TO_1_FLOAT(f32Fps));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_inttime_update(VI_PIPE ViPipe, CVI_U32 *u32IntTime)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	CVI_U32 u32Value = 0;
	CVI_U32 u32RHS1 = 0;
	CVI_U32 u32RHS1_MAX = 0;
	CVI_U32 u32SHS1 = 3;
	CVI_U32 u32SHS2 = 0;
	CVI_U32 u32YOUTSIZE;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(u32IntTime);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	if (WDR_MODE_2To1_LINE == pstSnsState->enWDRMode) {
		CVI_U32 u32ShortIntTime = u32IntTime[0];
		CVI_U32 u32LongIntTime = u32IntTime[1];

		if (pstSnsState->au32FL[1] < u32LongIntTime - 1) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "FL %d is smaller than  = %d\n",
					pstSnsState->au32FL[1], u32LongIntTime - 1);
			return CVI_FAILURE;
		}
		u32SHS2 = pstSnsState->au32FL[1] - u32LongIntTime - 1;

		u32RHS1 = u32ShortIntTime + u32SHS1 + 1;
		u32RHS1 = u32RHS1 | 0x1;

		u32RHS1_MAX = g_astImx385_State[ViPipe].u32RHS1_MAX;
		u32RHS1 = (u32RHS1 <= u32RHS1_MAX) ? u32RHS1 : u32RHS1_MAX;
		u32RHS1 = (u32RHS1 >= 5) ? u32RHS1 : 5;
		g_astImx385_State[ViPipe].u32RHS1 = u32RHS1;

		u32SHS2 = (u32SHS2 <= (pstSnsState->au32FL[1] - 2)) ? u32SHS2 : (pstSnsState->au32FL[1] - 2);
		u32SHS2 = (u32SHS2 >= (u32RHS1 + 3)) ? u32SHS2 : (u32RHS1 + 3);

		/* short exposure */
		pstSnsState->au32WDRIntTime[0] = u32RHS1 - (u32SHS1 + 1);
		/* long exposure */
		pstSnsState->au32WDRIntTime[1] = pstSnsState->au32FL[1] - (u32SHS2 + 1);
		/* Return the actual exposure lines*/
		u32IntTime[0] = pstSnsState->au32WDRIntTime[0];
		u32IntTime[1] = pstSnsState->au32WDRIntTime[1];

		u32YOUTSIZE = (1113 + (u32RHS1 - 1) / 2) * 2;

		CVI_TRACE_SNS(CVI_DBG_DEBUG, "u32ShortIntTime = %d u32LongIntTime:%d FL[1]:%d, FL[0]:%d\n",
			u32ShortIntTime, u32LongIntTime, pstSnsState->au32FL[1], pstSnsState->au32FL[0]);
		CVI_TRACE_SNS(CVI_DBG_DEBUG, "ViPipe = %d RHS1 = %d u32RHS1_MAX:%d u32YOUTSIZE = %d\n",
			ViPipe, u32RHS1, u32RHS1_MAX, u32YOUTSIZE);

		pstSnsRegsInfo->astI2cData[DOL2_SHS1_0].u32Data = (u32SHS1 & 0xFF);
		pstSnsRegsInfo->astI2cData[DOL2_SHS1_1].u32Data = ((u32SHS1 & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[DOL2_SHS1_2].u32Data = ((u32SHS1 & 0xF0000) >> 16);

		pstSnsRegsInfo->astI2cData[DOL2_SHS2_0].u32Data = (u32SHS2 & 0xFF);
		pstSnsRegsInfo->astI2cData[DOL2_SHS2_1].u32Data = ((u32SHS2 & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[DOL2_SHS2_2].u32Data = ((u32SHS2 & 0xF0000) >> 16);

		pstSnsRegsInfo->astI2cData[DOL2_RHS1_0].u32Data = (u32RHS1 & 0xFF);
		pstSnsRegsInfo->astI2cData[DOL2_RHS1_1].u32Data = ((u32RHS1 & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[DOL2_RHS1_2].u32Data = ((u32RHS1 & 0xF0000) >> 16);

		pstSnsRegsInfo->astI2cData[DOL2_YOUT_SIZE_0].u32Data = (u32YOUTSIZE & 0xFF);
		pstSnsRegsInfo->astI2cData[DOL2_YOUT_SIZE_1].u32Data = ((u32YOUTSIZE & 0x1F00) >> 8);
		/* update isp */
		cmos_get_wdr_size(ViPipe, &pstSnsState->astSyncInfo[0].ispCfg);
	} else {
		u32Value = pstSnsState->au32FL[0] - *u32IntTime - 1;

		if (u32Value > pstSnsState->au32FL[0] - 3)
			u32Value = pstSnsState->au32FL[0] - 3;
		u32Value = (u32Value >= g_astImx385_mode[pstSnsState->u8ImgMode].stExp[0].u32Min) ?
			u32Value : g_astImx385_mode[pstSnsState->u8ImgMode].stExp[0].u32Min;
		*u32IntTime = pstSnsState->au32FL[0] - u32Value - 1;
		pstSnsRegsInfo->astI2cData[LINEAR_SHS1_0].u32Data = (u32Value & 0xFF);
		pstSnsRegsInfo->astI2cData[LINEAR_SHS1_1].u32Data = ((u32Value & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[LINEAR_SHS1_2].u32Data = ((u32Value & 0x10000) >> 16);
	}

	return CVI_SUCCESS;

}

static CVI_U32 gain_table[691] = {
	1024, 1035, 1047, 1059, 1072, 1084, 1097, 1109, 1122, 1135,
	1148, 1162, 1175, 1189, 1203, 1217, 1231, 1245, 1259, 1274,
	1289, 1304, 1319, 1334, 1349, 1365, 1381, 1397, 1413, 1429,
	1446, 1463, 1480, 1497, 1514, 1532, 1549, 1567, 1585, 1604,
	1622, 1641, 1660, 1679, 1699, 1719, 1739, 1759, 1779, 1800,
	1820, 1842, 1863, 1884, 1906, 1928, 1951, 1973, 1996, 2019,
	2043, 2066, 2090, 2114, 2139, 2164, 2189, 2214, 2240, 2266,
	2292, 2318, 2345, 2373, 2400, 2428, 2456, 2484, 2513, 2542,
	2572, 2601, 2632, 2662, 2693, 2724, 2756, 2788, 2820, 2852,
	2886, 2919, 2953, 2987, 3022, 3057, 3092, 3128, 3164, 3201,
	3238, 3275, 3313, 3351, 3390, 3430, 3469, 3509, 3550, 3591,
	3633, 3675, 3717, 3760, 3804, 3848, 3893, 3938, 3983, 4029,
	4076, 4123, 4171, 4219, 4268, 4318, 4368, 4418, 4469, 4521,
	4574, 4627, 4680, 4734, 4789, 4845, 4901, 4957, 5015, 5073,
	5132, 5191, 5251, 5312, 5374, 5436, 5499, 5562, 5627, 5692,
	5758, 5825, 5892, 5960, 6029, 6099, 6170, 6241, 6313, 6387,
	6461, 6535, 6611, 6688, 6765, 6843, 6923, 7003, 7084, 7166,
	7249, 7333, 7418, 7504, 7591, 7678, 7767, 7857, 7948, 8040,
	8133, 8228, 8323, 8419, 8517, 8615, 8715, 8816, 8918, 9021,
	9126, 9232, 9338, 9447, 9556, 9667, 9779, 9892, 10006, 10122,
	10240, 10358, 10478, 10599, 10722, 10846, 10972, 11099, 11227, 11357,
	11489, 11622, 11757, 11893, 12030, 12170, 12311, 12453, 12597, 12743,
	12891, 13040, 13191, 13344, 13498, 13655, 13813, 13973, 14135, 14298,
	14464, 14631, 14801, 14972, 15146, 15321, 15498, 15678, 15859, 16043,
	16229, 16417, 16607, 16799, 16994, 17190, 17390, 17591, 17795, 18001,
	18209, 18420, 18633, 18849, 19067, 19288, 19511, 19737, 19966, 20197,
	20431, 20668, 20907, 21149, 21394, 21642, 21892, 22146, 22402, 22662,
	22924, 23189, 23458, 23730, 24004, 24282, 24564, 24848, 25136, 25427,
	25721, 26019, 26320, 26625, 26933, 27245, 27561, 27880, 28203, 28529,
	28860, 29194, 29532, 29874, 30220, 30570, 30924, 31282, 31644, 32011,
	32381, 32756, 33135, 33519, 33907, 34300, 34697, 35099, 35505, 35916,
	36332, 36753, 37179, 37609, 38045, 38485, 38931, 39382, 39838, 40299,
	40766, 41238, 41715, 42198, 42687, 43181, 43681, 44187, 44699, 45216,
	45740, 46270, 46805, 47347, 47896, 48450, 49011, 49579, 50153, 50734,
	51321, 51915, 52517, 53125, 53740, 54362, 54992, 55628, 56272, 56924,
	57583, 58250, 58925, 59607, 60297, 60995, 61702, 62416, 63139, 63870,
	64610, 65358, 66114, 66880, 67655, 68438, 69230, 70032, 70843, 71663,
	72493, 73333, 74182, 75041, 75910, 76789, 77678, 78577, 79487, 80408,
	81339, 82281, 83233, 84197, 85172, 86158, 87156, 88165, 89186, 90219,
	91264, 92320, 93389, 94471, 95565, 96671, 97791, 98923, 100069, 101227,
	102400, 103585, 104785, 105998, 107225, 108467, 109723, 110994, 112279, 113579,
	114894, 116225, 117570, 118932, 120309, 121702, 123111, 124537, 125979, 127438,
	128913, 130406, 131916, 133444, 134989, 136552, 138133, 139733, 141351, 142988,
	144643, 146318, 148013, 149726, 151460, 153214, 154988, 156783, 158598, 160435,
	162293, 164172, 166073, 167996, 169941, 171909, 173900, 175913, 177950, 180011,
	182095, 184204, 186337, 188495, 190677, 192885, 195119, 197378, 199664, 201976,
	204314, 206680, 209073, 211494, 213943, 216421, 218927, 221462, 224026, 226620,
	229245, 231899, 234584, 237301, 240049, 242828, 245640, 248484, 251362, 254272,
	257217, 260195, 263208, 266256, 269339, 272458, 275613, 278804, 282033, 285298,
	288602, 291944, 295324, 298744, 302203, 305703, 309243, 312823, 316446, 320110,
	323817, 327566, 331359, 335196, 339078, 343004, 346976, 350994, 355058, 359169,
	363328, 367536, 371791, 376097, 380452, 384857, 389313, 393821, 398382, 402995,
	407661, 412382, 417157, 421987, 426874, 431817, 436817, 441875, 446992, 452168,
	457403, 462700, 468058, 473478, 478960, 484506, 490117, 495792, 501533, 507340,
	513215, 519158, 525170, 531251, 537402, 543625, 549920, 556288, 562729, 569245,
	575837, 582505, 589250, 596073, 602975, 609958, 617021, 624165, 631393, 638704,
	646100, 653581, 661149, 668805, 676550, 684384, 692308, 700325, 708434, 716638,
	724936, 733330, 741822, 750412, 759101, 767891, 776783, 785778, 794877, 804081,
	813392, 822810, 832338, 841976, 851726, 861588, 871565, 881657, 891866, 902194,
	912640, 923208, 933899, 944713, 955652, 966718, 977912, 989236, 1000690, 1012278,
	1024000, 1035857, 1047852, 1059985, 1072259, 1084675, 1097235, 1109941, 1122793, 1135795,
	1148946, 1162251, 1175709, 1189323, 1203095, 1217026, 1231118, 1245374, 1259795, 1274382,
	1289139, 1304067, 1319167, 1334442, 1349894, 1365525, 1381337, 1397333, 1413513, 1429881,
	1446438, 1463187, 1480130, 1497269, 1514606, 1532145, 1549886, 1567833, 1585988, 1604353,
	1622930, 1641723, 1660733, 1679963, 1699416, 1719095, 1739001, 1759138, 1779508, 1800113,
	1820958, 1842043, 1863373, 1884950, 1906777, 1928856, 1951191, 1973785, 1996640, 2019760,
	2043148, 2066807, 2090739, 2114949, 2139439, 2164212, 2189273, 2214623, 2240267, 2266208,
	2292450, 2318995, 2345848, 2373012, 2400490, 2428286, 2456404, 2484848, 2513621, 2542728,
	2572171, 2601956, 2632085, 2662563, 2693394, 2724582, 2756131, 2788046, 2820330, 2852988,
	2886024,
};

static CVI_S32 cmos_again_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32AgainLin, CVI_U32 *pu32AgainDb)
{
	int i;

	(void) ViPipe;

	CMOS_CHECK_POINTER(pu32AgainLin);
	CMOS_CHECK_POINTER(pu32AgainDb);

	if (*pu32AgainLin >= gain_table[690]) {
		*pu32AgainLin = gain_table[690];
		*pu32AgainDb = 690;
		return CVI_SUCCESS;
	}

	for (i = 1; i < 691; i++) {
		if (*pu32AgainLin < gain_table[i]) {
			*pu32AgainLin = gain_table[i - 1];
			*pu32AgainDb = i - 1;
			break;
		}
	}
	return CVI_SUCCESS;
}

static CVI_S32 cmos_dgain_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32DgainLin, CVI_U32 *pu32DgainDb)
{
	int i;

	(void) ViPipe;

	CMOS_CHECK_POINTER(pu32DgainLin);
	CMOS_CHECK_POINTER(pu32DgainDb);
	if (*pu32DgainLin >= gain_table[420]) {
		*pu32DgainLin = gain_table[420];
		*pu32DgainDb = 420;
		return CVI_SUCCESS;
	}

	for (i = 1; i < 421; i++) {
		if (*pu32DgainLin < gain_table[i]) {
			*pu32DgainLin = gain_table[i - 1];
			*pu32DgainDb = i - 1;
			break;
		}
	}
	return CVI_SUCCESS;
}

static CVI_S32 cmos_gains_update(VI_PIPE ViPipe, CVI_U32 *pu32Again, CVI_U32 *pu32Dgain)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	CVI_U32 u32HCG = g_astImx385_State[ViPipe].u8Hcg;
	CVI_U32 u16Mode = g_au16Imx385_GainMode[ViPipe];
	CVI_U32 u32Tmp;
	CVI_U32 u32Again;
	CVI_U32 u32Dgain;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pu32Again);
	CMOS_CHECK_POINTER(pu32Dgain);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		/* linear mode */
		u32Again = pu32Again[0];
		u32Dgain = pu32Dgain[0];

		if (u32Again >= 0x38) {
			// hcg bit[4]
			u32HCG = u32HCG | 0x10;
			u32Again = u32Again - 0x38;
		}

		u32Tmp = u32Again + u32Dgain;
		pstSnsRegsInfo->astI2cData[LINEAR_GAIN_0].u32Data = (u32Tmp & 0xFF);
		pstSnsRegsInfo->astI2cData[LINEAR_GAIN_1].u32Data = ((u32Tmp & 0x300) >> 8);
		pstSnsRegsInfo->astI2cData[LINEAR_HCG].u32Data = (u32HCG & 0xFF);
	} else {
		/* DOL mode */
		if (u16Mode == SNS_GAIN_MODE_WDR_2F) {
			/* don't support gain conversion in this mode. */
			u32Again = pu32Again[1];
			u32Dgain = pu32Dgain[1];

			u32Tmp = u32Again + u32Dgain;
			if (u32Tmp > 0xFF) {
				u32Tmp = 0xFF;
			}
			if (u32HCG > 0xFF) {
				u32HCG = 0xFF;
			}
			pstSnsRegsInfo->astI2cData[DOL2_GAIN_0].u32Data = (u32Tmp & 0xFF);
			pstSnsRegsInfo->astI2cData[DOL2_GAIN_1].u32Data = ((u32Tmp & 0x300) >> 8);
			pstSnsRegsInfo->astI2cData[DOL2_HCG].u32Data = (u32HCG & 0xFF);

			u32Again = pu32Again[0];
			u32Dgain = pu32Dgain[0];

			u32Tmp = u32Again + u32Dgain;
			if (u32Tmp > 0xFF) {
				u32Tmp = 0xFF;
			}
			pstSnsRegsInfo->astI2cData[DOL2_GAIN1_0].u32Data = (u32Tmp & 0xFF);
			pstSnsRegsInfo->astI2cData[DOL2_GAIN1_1].u32Data = ((u32Tmp & 0x300) >> 8);
		} else if (u16Mode == SNS_GAIN_MODE_SHARE) {
			u32Again = pu32Again[0];
			u32Dgain = pu32Dgain[0];

			if (u32Again >= 0x38) {
				// hcg bit[4]
				u32HCG = u32HCG | 0x10;
				u32Again = u32Again - 0x38;
			}

			u32Tmp = u32Again + u32Dgain;
			if (u32Tmp > 0xFF) {
				u32Tmp = 0xFF;
			}
			if (u32HCG > 0xFF) {
				u32HCG = 0xFF;
			}
			pstSnsRegsInfo->astI2cData[DOL2_GAIN_0].u32Data = (u32Tmp & 0xFF);
			pstSnsRegsInfo->astI2cData[DOL2_GAIN_1].u32Data = ((u32Tmp & 0x300) >> 8);
			pstSnsRegsInfo->astI2cData[DOL2_HCG].u32Data = (u32HCG & 0xFF);
		}
	}

	return CVI_SUCCESS;
}

/* limitation for line base WDR
 * SHS1 range:[3, RHS1 - 2]
 * RHS1 range:[2n+5, FSC - BRL*2 -51] and n=0,1,2,3...
 * SHS2 range:[RHS1 +3, FSC-2]
 * Tsexp = RHS1 - (SHS1 +1) <= RHS1 - 4
 * Tlexp = FSC -  (SHS2 +1) <= FSC - ((RHS1+3)+1)
 * Tsexp + Tlexp <= FSC - 8
 * thus:
 * expect Tsexp range: [5 - 4, (FSC - 8) / (ratio + 1)]
 */
static CVI_S32 cmos_get_inttime_max(VI_PIPE ViPipe, CVI_U16 u16ManRatioEnable, CVI_U32 *au32Ratio,
		CVI_U32 *au32IntTimeMax, CVI_U32 *au32IntTimeMin, CVI_U32 *pu32LFMaxIntTime)
{
	CVI_U32 u32IntTimeMaxTmp = 0, u32IntTimeMaxTmp0 = 0;
	CVI_U32 u32RatioTmp = 0x40;
	CVI_U32 u32ShortTimeMinLimit = 0;

	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(au32Ratio);
	CMOS_CHECK_POINTER(au32IntTimeMax);
	CMOS_CHECK_POINTER(au32IntTimeMin);
	CMOS_CHECK_POINTER(pu32LFMaxIntTime);
	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u32ShortTimeMinLimit = (WDR_MODE_2To1_LINE == pstSnsState->enWDRMode) ? 2 : 2;

	if (WDR_MODE_2To1_LINE == pstSnsState->enWDRMode) {
		if (genFSWDRMode[ViPipe] == ISP_FSWDR_LONG_FRAME_MODE) {
			u32IntTimeMaxTmp = pstSnsState->au32FL[0] - 10;
			au32IntTimeMax[0] = u32IntTimeMaxTmp;
			au32IntTimeMin[0] = u32ShortTimeMinLimit;
			return CVI_SUCCESS;
		}
		u32IntTimeMaxTmp0 = ((pstSnsState->au32FL[1] - 8 - pstSnsState->au32WDRIntTime[0]) * 0x40) /
					DIV_0_TO_1(au32Ratio[0]); //use old lexp to get new sexp
		u32IntTimeMaxTmp  = ((pstSnsState->au32FL[0] - 8) * 0x40) / DIV_0_TO_1(au32Ratio[0] + 0x40);

		u32IntTimeMaxTmp = (u32IntTimeMaxTmp > u32IntTimeMaxTmp0) ? u32IntTimeMaxTmp0 : u32IntTimeMaxTmp;
		u32IntTimeMaxTmp  = (u32IntTimeMaxTmp > (g_astImx385_State[ViPipe].u32RHS1_MAX - 4)) ?
						(g_astImx385_State[ViPipe].u32RHS1_MAX - 4) : u32IntTimeMaxTmp;
		u32IntTimeMaxTmp  = (!u32IntTimeMaxTmp) ? 1 : u32IntTimeMaxTmp;
	}

	if (u32IntTimeMaxTmp >= u32ShortTimeMinLimit) {
		if (pstSnsState->enWDRMode == WDR_MODE_2To1_LINE) {
			au32IntTimeMax[0] = u32IntTimeMaxTmp;
			au32IntTimeMax[1] = au32IntTimeMax[0] * au32Ratio[0] >> 6;
			au32IntTimeMax[2] = au32IntTimeMax[1] * au32Ratio[1] >> 6;
			au32IntTimeMax[3] = au32IntTimeMax[2] * au32Ratio[2] >> 6;
			au32IntTimeMin[0] = u32ShortTimeMinLimit;
			au32IntTimeMin[1] = au32IntTimeMin[0] * au32Ratio[0] >> 6;
			au32IntTimeMin[2] = au32IntTimeMin[1] * au32Ratio[1] >> 6;
			au32IntTimeMin[3] = au32IntTimeMin[2] * au32Ratio[2] >> 6;
			CVI_TRACE_SNS(CVI_DBG_DEBUG, "sexp[%d, %d], lexp[%d, %d], ratio:%d\n",
				au32IntTimeMin[0], au32IntTimeMax[0],
				au32IntTimeMin[1], au32IntTimeMax[1], au32Ratio[0]);
		} else {
		}
	} else {
		if (u16ManRatioEnable) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Manaul ExpRatio out of range!\n");
			return CVI_FAILURE;
		}
		u32IntTimeMaxTmp = u32ShortTimeMinLimit;

		if (pstSnsState->enWDRMode == WDR_MODE_2To1_LINE) {
			u32RatioTmp = 0xFFF;
			au32IntTimeMax[0] = u32IntTimeMaxTmp;
			au32IntTimeMax[1] = au32IntTimeMax[0] * u32RatioTmp >> 6;
		} else {
		}
		au32IntTimeMin[0] = au32IntTimeMax[0];
		au32IntTimeMin[1] = au32IntTimeMax[1];
		au32IntTimeMin[2] = au32IntTimeMax[2];
		au32IntTimeMin[3] = au32IntTimeMax[3];
	}

	return CVI_SUCCESS;
}

/* Only used in LINE_WDR mode */
static CVI_S32 cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
	CMOS_CHECK_POINTER(pstAeFSWDRAttr);

	genFSWDRMode[ViPipe] = pstAeFSWDRAttr->enFSWDRMode;
	gu32MaxTimeGetCnt[ViPipe] = 0;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
	CMOS_CHECK_POINTER(pstExpFuncs);

	memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

	pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
	pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
	//pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
	pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
	pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
	pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
	pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;
	pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
	pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
	(void) ViPipe;

	CMOS_CHECK_POINTER(pstAwbSnsDft);

	memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));

	pstAwbSnsDft->u16InitGgain = 1024;
	pstAwbSnsDft->u8AWBRunInterval = 1;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
	CMOS_CHECK_POINTER(pstExpFuncs);

	memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

	pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
	(void) ViPipe;

	memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

	memcpy(pstDef->stNoiseCalibration.CalibrationCoef,
		&g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));
	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_blc_default(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlc)
{
	(void) ViPipe;

	CMOS_CHECK_POINTER(pstBlc);

	memset(pstBlc, 0, sizeof(ISP_CMOS_BLACK_LEVEL_S));

	memcpy(pstBlc,
		&g_stIspBlcCalibratio, sizeof(ISP_CMOS_BLACK_LEVEL_S));
	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg)
{
	const IMX385_MODE_S *pstMode = CVI_NULL;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	pstMode = &g_astImx385_mode[pstSnsState->u8ImgMode];

	if (pstSnsState->enWDRMode != WDR_MODE_NONE) {
		CVI_U32 u32VBP1;
		CVI_U32 u32VBP1_MAX;
		/* output height = BRL + VBP1
		 * VBP1 = (RHS1 - 1)/2
		 * long frame start_y = op_size_v + margin_top
		 * short frame start_y = op_size_v + margin_top + VBP1
		 */
		u32VBP1 = (g_astImx385_State[ViPipe].u32RHS1  - 1) >> 1;
		u32VBP1_MAX = (g_astImx385_State[ViPipe].u32RHS1_MAX - 1) >> 1;
		pstIspCfg->frm_num = 2;
		memcpy(&pstIspCfg->img_size[0], &pstMode->astImg[0], sizeof(ISP_WDR_SIZE_S));
		memcpy(&pstIspCfg->img_size[1], &pstMode->astImg[1], sizeof(ISP_WDR_SIZE_S));

		pstIspCfg->img_size[1].stSnsSize.u32Height = 1113 + u32VBP1;
		pstIspCfg->img_size[1].stWndRect.s32Y = pstMode->u32OpbSize + pstMode->u32MarginVtop;

		pstIspCfg->img_size[1].stMaxSize.u32Height = 1113 + u32VBP1_MAX;
		pstIspCfg->img_size[0].stSnsSize.u32Height = pstIspCfg->img_size[1].stSnsSize.u32Height;
		pstIspCfg->img_size[0].stWndRect.s32Y = pstMode->u32OpbSize + pstMode->u32MarginVtop + u32VBP1;

		pstIspCfg->img_size[0].stMaxSize.u32Height = pstIspCfg->img_size[1].stMaxSize.u32Height;

		CVI_TRACE_SNS(CVI_DBG_DEBUG, "sns_h:%d, wdr_max.h:%d, VBP1:%d\n",
			pstIspCfg->img_size[1].stSnsSize.u32Height,
			pstIspCfg->img_size[1].stMaxSize.u32Height, u32VBP1);
	} else {
		pstIspCfg->frm_num = 1;
		memcpy(&pstIspCfg->img_size[0], &pstMode->astImg[0], sizeof(ISP_WDR_SIZE_S));
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, CVI_U8 u8Mode)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstSnsState->bSyncInit = CVI_FALSE;

	switch (u8Mode) {
	case WDR_MODE_NONE:
		if (pstSnsState->u8ImgMode == IMX385_MODE_1080P30_WDR)
			pstSnsState->u8ImgMode = IMX385_MODE_1080P30;
		pstSnsState->enWDRMode = WDR_MODE_NONE;
		pstSnsState->u32FLStd = g_astImx385_mode[pstSnsState->u8ImgMode].u32VtsDef;
		g_astImx385_State[ViPipe].u8Hcg = 0x2;//30 fps
		syslog(LOG_INFO, "WDR_MODE_NONE\n");
		break;

	case WDR_MODE_2To1_LINE:
		if (pstSnsState->u8ImgMode == IMX385_MODE_1080P30)
			pstSnsState->u8ImgMode = IMX385_MODE_1080P30_WDR;
		pstSnsState->enWDRMode = WDR_MODE_2To1_LINE;
		g_astImx385_State[ViPipe].u8Hcg    = 0x1;// 30 fps after combine
		if (pstSnsState->u8ImgMode == IMX385_MODE_1080P30_WDR) {
			pstSnsState->u32FLStd = g_astImx385_mode[pstSnsState->u8ImgMode].u32VtsDef * 2;
			g_astImx385_State[ViPipe].u32BRL  = g_astImx385_mode[pstSnsState->u8ImgMode].u32BRL;
			syslog(LOG_INFO, "WDR_MODE_2To1_LINE 1080p\n");
		}
		break;
	default:
		CVI_TRACE_SNS(CVI_DBG_ERR, "Unknown mode!\n");
		return CVI_FAILURE;
	}

	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstSnsState->au32FL[1] = pstSnsState->au32FL[0];
	memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));

	return CVI_SUCCESS;
}

static CVI_U32 sensor_cmp_wdr_size(ISP_SNS_ISP_INFO_S *pstWdr1, ISP_SNS_ISP_INFO_S *pstWdr2)
{
	CVI_U32 i;

	if (pstWdr1->frm_num != pstWdr2->frm_num)
		goto _mismatch;
	for (i = 0; i < 2; i++) {
		if (pstWdr1->img_size[i].stSnsSize.u32Width != pstWdr2->img_size[i].stSnsSize.u32Width)
			goto _mismatch;
		if (pstWdr1->img_size[i].stSnsSize.u32Height != pstWdr2->img_size[i].stSnsSize.u32Height)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.s32X != pstWdr2->img_size[i].stWndRect.s32X)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.s32Y != pstWdr2->img_size[i].stWndRect.s32Y)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.u32Width != pstWdr2->img_size[i].stWndRect.u32Width)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.u32Height != pstWdr2->img_size[i].stWndRect.u32Height)
			goto _mismatch;
	}

	return 0;
_mismatch:
	return 1;
}

static CVI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *pstSnsSyncInfo)
{
	CVI_U32 i;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *pstCfg0 = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *pstCfg1 = CVI_NULL;
	ISP_I2C_DATA_S *pstI2c_data = CVI_NULL;

	CMOS_CHECK_POINTER(pstSnsSyncInfo);
	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	pstSnsRegsInfo = &pstSnsSyncInfo->snsCfg;
	pstCfg0 = &pstSnsState->astSyncInfo[0];
	pstCfg1 = &pstSnsState->astSyncInfo[1];
	pstI2c_data = pstCfg0->snsCfg.astI2cData;

	if ((pstSnsState->bSyncInit == CVI_FALSE) || (pstSnsRegsInfo->bConfig == CVI_FALSE)) {
		pstCfg0->snsCfg.enSnsType = SNS_I2C_TYPE;
		pstCfg0->snsCfg.unComBus.s8I2cDev = g_aunImx385_BusInfo[ViPipe].s8I2cDev;
		pstCfg0->snsCfg.u8Cfg2ValidDelayMax = 2;
		pstCfg0->snsCfg.use_snsr_sram = CVI_TRUE;
		pstCfg0->snsCfg.u32RegNum = (WDR_MODE_2To1_LINE == pstSnsState->enWDRMode) ?
					DOL2_REGS_NUM : LINEAR_REGS_NUM;

		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			pstI2c_data[i].bUpdate = CVI_TRUE;
			pstI2c_data[i].u8DevAddr = imx385_i2c_addr;
			pstI2c_data[i].u32AddrByteNum = imx385_addr_byte;
			pstI2c_data[i].u32DataByteNum = imx385_data_byte;
		}

		switch (pstSnsState->enWDRMode) {
		case WDR_MODE_2To1_LINE:
			pstI2c_data[DOL2_HOLD].u32RegAddr = IMX385_HOLD_ADDR;
			pstI2c_data[DOL2_HOLD].u32Data = 1;
			pstI2c_data[DOL2_SHS1_0].u32RegAddr = IMX385_SHS1_ADDR;
			pstI2c_data[DOL2_SHS1_1].u32RegAddr = IMX385_SHS1_ADDR + 1;
			pstI2c_data[DOL2_SHS1_2].u32RegAddr = IMX385_SHS1_ADDR + 2;

			pstI2c_data[DOL2_GAIN_0].u32RegAddr = IMX385_GAIN_ADDR;
			pstI2c_data[DOL2_GAIN_1].u32RegAddr = IMX385_GAIN_ADDR + 1;
			pstI2c_data[DOL2_HCG].u32RegAddr = IMX385_HCG_ADDR;
			pstI2c_data[DOL2_GAIN1_0].u32RegAddr = IMX385_GAIN1_ADDR;
			pstI2c_data[DOL2_GAIN1_1].u32RegAddr = IMX385_GAIN1_ADDR + 1;

			pstI2c_data[DOL2_RHS1_0].u32RegAddr = IMX385_RHS1_ADDR;
			pstI2c_data[DOL2_RHS1_1].u32RegAddr = IMX385_RHS1_ADDR + 1;
			pstI2c_data[DOL2_RHS1_2].u32RegAddr = IMX385_RHS1_ADDR + 2;

			pstI2c_data[DOL2_SHS2_0].u32RegAddr = IMX385_SHS2_ADDR;
			pstI2c_data[DOL2_SHS2_1].u32RegAddr = IMX385_SHS2_ADDR + 1;
			pstI2c_data[DOL2_SHS2_2].u32RegAddr = IMX385_SHS2_ADDR + 2;

			pstI2c_data[DOL2_VMAX_0].u32RegAddr = IMX385_VMAX_ADDR;
			pstI2c_data[DOL2_VMAX_1].u32RegAddr = IMX385_VMAX_ADDR + 1;
			pstI2c_data[DOL2_VMAX_2].u32RegAddr = IMX385_VMAX_ADDR + 2;

			pstI2c_data[DOL2_YOUT_SIZE_0].u32RegAddr = IMX385_YOUT_ADDR;
			pstI2c_data[DOL2_YOUT_SIZE_1].u32RegAddr = IMX385_YOUT_ADDR + 1;
			pstI2c_data[DOL2_REL].u32RegAddr = IMX385_HOLD_ADDR;
			pstI2c_data[DOL2_REL].u32Data = 0;
			break;
		default:
			pstI2c_data[LINEAR_HOLD].u32RegAddr = IMX385_HOLD_ADDR;
			pstI2c_data[LINEAR_HOLD].u32Data = 1;
			pstI2c_data[LINEAR_SHS1_0].u32RegAddr = IMX385_SHS1_ADDR;
			pstI2c_data[LINEAR_SHS1_1].u32RegAddr = IMX385_SHS1_ADDR + 1;
			pstI2c_data[LINEAR_SHS1_2].u32RegAddr = IMX385_SHS1_ADDR + 2;

			pstI2c_data[LINEAR_GAIN_0].u32RegAddr = IMX385_GAIN_ADDR;
			pstI2c_data[LINEAR_GAIN_1].u32RegAddr = IMX385_GAIN_ADDR + 1;
			pstI2c_data[LINEAR_HCG].u32RegAddr = IMX385_HCG_ADDR;

			pstI2c_data[LINEAR_VMAX_0].u32RegAddr = IMX385_VMAX_ADDR;
			pstI2c_data[LINEAR_VMAX_1].u32RegAddr = IMX385_VMAX_ADDR + 1;
			pstI2c_data[LINEAR_VMAX_2].u32RegAddr = IMX385_VMAX_ADDR + 2;
			pstI2c_data[LINEAR_REL].u32RegAddr = IMX385_HOLD_ADDR;
			pstI2c_data[LINEAR_REL].u32Data = 0;
			break;
		}
		pstSnsState->bSyncInit = CVI_TRUE;
		pstCfg0->snsCfg.need_update = CVI_TRUE;
		/* recalcualte WDR size */
		cmos_get_wdr_size(ViPipe, &pstCfg0->ispCfg);
		pstCfg0->ispCfg.need_update = CVI_TRUE;
	} else {
		pstCfg0->snsCfg.need_update = CVI_FALSE;
		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			if (pstCfg0->snsCfg.astI2cData[i].u32Data == pstCfg1->snsCfg.astI2cData[i].u32Data) {
				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_FALSE;
			} else {
				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_TRUE;
				pstCfg0->snsCfg.need_update = CVI_TRUE;
			}
		}
		if (pstCfg0->snsCfg.need_update == CVI_TRUE) {
			if (pstSnsState->enWDRMode == WDR_MODE_2To1_LINE) {
				pstI2c_data[DOL2_HOLD].u32Data = 1;
				pstI2c_data[DOL2_HOLD].bUpdate = CVI_TRUE;
				pstI2c_data[DOL2_REL].u32Data = 0;
				pstI2c_data[DOL2_REL].bUpdate = CVI_TRUE;
			} else {
				pstI2c_data[LINEAR_HOLD].u32Data = 1;
				pstI2c_data[LINEAR_HOLD].bUpdate = CVI_TRUE;
				pstI2c_data[LINEAR_REL].u32Data = 0;
				pstI2c_data[LINEAR_REL].bUpdate = CVI_TRUE;
			}
		}
		/* check update isp crop or not */
		pstCfg0->ispCfg.need_update = (sensor_cmp_wdr_size(&pstCfg0->ispCfg, &pstCfg1->ispCfg) ?
				CVI_TRUE : CVI_FALSE);
	}

	pstSnsRegsInfo->bConfig = CVI_FALSE;
	memcpy(pstSnsSyncInfo, &pstSnsState->astSyncInfo[0], sizeof(ISP_SNS_SYNC_INFO_S));
	memcpy(&pstSnsState->astSyncInfo[1], &pstSnsState->astSyncInfo[0], sizeof(ISP_SNS_SYNC_INFO_S));
	pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

	return CVI_SUCCESS;
}

static CVI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
	CVI_U8 u8SensorImageMode = 0;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstSensorImageMode);
	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u8SensorImageMode = pstSnsState->u8ImgMode;
	pstSnsState->bSyncInit = CVI_FALSE;

	if (pstSensorImageMode->f32Fps <= 30) {
		if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
			if (IMX385_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
				u8SensorImageMode = IMX385_MODE_1080P30;
			} else {
				CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
				       pstSensorImageMode->u16Width,
				       pstSensorImageMode->u16Height,
				       pstSensorImageMode->f32Fps,
				       pstSnsState->enWDRMode);
				return CVI_FAILURE;
			}
		} else if (pstSnsState->enWDRMode == WDR_MODE_2To1_LINE) {
			if (IMX385_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
				u8SensorImageMode = IMX385_MODE_1080P30_WDR;
				g_astImx385_State[ViPipe].u32BRL  = g_astImx385_mode[pstSnsState->u8ImgMode].u32BRL;
			} else {
				CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
				       pstSensorImageMode->u16Width,
				       pstSensorImageMode->u16Height,
				       pstSensorImageMode->f32Fps,
				       pstSnsState->enWDRMode);
				return CVI_FAILURE;
			}
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
			       pstSensorImageMode->u16Width,
			       pstSensorImageMode->u16Height,
			       pstSensorImageMode->f32Fps,
			       pstSnsState->enWDRMode);
			return CVI_FAILURE;
		}
	} else {
	}

	if ((pstSnsState->bInit == CVI_TRUE) && (u8SensorImageMode == pstSnsState->u8ImgMode)) {
		return CVI_FAILURE;
	}

	pstSnsState->u8ImgMode = u8SensorImageMode;

	return CVI_SUCCESS;
}

static CVI_VOID sensor_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);
	if (pstSnsState->bInit == CVI_TRUE && g_aeImx385_MirrorFip[ViPipe] != eSnsMirrorFlip) {
		imx385_mirror_flip(ViPipe, eSnsMirrorFlip);
		g_aeImx385_MirrorFip[ViPipe] = eSnsMirrorFlip;
	}
}

static CVI_VOID sensor_global_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsState->bInit = CVI_FALSE;
	pstSnsState->bSyncInit = CVI_FALSE;
	pstSnsState->u8ImgMode = IMX385_MODE_1080P30;
	pstSnsState->enWDRMode = WDR_MODE_NONE;
	pstSnsState->u32FLStd  = IMX385_VMAX_1080P30_LINEAR;
	pstSnsState->au32FL[0] = IMX385_VMAX_1080P30_LINEAR;
	pstSnsState->au32FL[1] = IMX385_VMAX_1080P30_LINEAR;

	memset(&pstSnsState->astSyncInfo[0], 0, sizeof(ISP_SNS_SYNC_INFO_S));
	memset(&pstSnsState->astSyncInfo[1], 0, sizeof(ISP_SNS_SYNC_INFO_S));
}

static CVI_S32 sensor_rx_attr(VI_PIPE ViPipe, SNS_COMBO_DEV_ATTR_S *pstRxAttr)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pstRxAttr);

	memcpy(pstRxAttr, &imx385_rx_attr, sizeof(*pstRxAttr));

	pstRxAttr->img_size.width = g_astImx385_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Width;
	pstRxAttr->img_size.height = g_astImx385_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Height;
	if (pstSnsState->enWDRMode != WDR_MODE_NONE) {
		pstRxAttr->wdr_manu.manual_en = 1;
		pstRxAttr->wdr_manu.l2s_distance = 0;
		pstRxAttr->wdr_manu.lsef_length = 0x1FFF;
		pstRxAttr->wdr_manu.discard_padding_lines = 1;
		pstRxAttr->wdr_manu.update = 1;
	} else {
		pstRxAttr->mipi_attr.wdr_mode = CVI_MIPI_WDR_MODE_NONE;
	}

	return CVI_SUCCESS;

}

static CVI_S32 sensor_patch_rx_attr(RX_INIT_ATTR_S *pstRxInitAttr)
{
	SNS_COMBO_DEV_ATTR_S *pstRxAttr = &imx385_rx_attr;
	int i;

	CMOS_CHECK_POINTER(pstRxInitAttr);

	if (pstRxInitAttr->MipiDev >= VI_MAX_DEV_NUM)
		return CVI_SUCCESS;

	if (pstRxInitAttr->stMclkAttr.bMclkEn)
		pstRxAttr->mclk.cam = pstRxInitAttr->stMclkAttr.u8Mclk;

	pstRxAttr->devno = pstRxInitAttr->MipiDev;

	if (pstRxAttr->input_mode == INPUT_MODE_MIPI) {
		struct mipi_dev_attr_s *attr = &pstRxAttr->mipi_attr;

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			attr->lane_id[i] = pstRxInitAttr->as16LaneId[i];
			attr->pn_swap[i] = pstRxInitAttr->as8PNSwap[i];
		}
	} else {
		struct lvds_dev_attr_s *attr = &pstRxAttr->lvds_attr;

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			attr->lane_id[i] = pstRxInitAttr->as16LaneId[i];
			attr->pn_swap[i] = pstRxInitAttr->as8PNSwap[i];
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
	CMOS_CHECK_POINTER(pstSensorExpFunc);

	memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

	pstSensorExpFunc->pfn_cmos_sensor_init = imx385_init;
	pstSensorExpFunc->pfn_cmos_sensor_exit = imx385_exit;
	pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
	pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
	pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

	pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
	pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_blc_default;
	pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

	return CVI_SUCCESS;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static CVI_S32 imx385_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
	g_aunImx385_BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

	if (pastSnsStateCtx == CVI_NULL) {
		pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
		if (pastSnsStateCtx == CVI_NULL) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
			return -ENOMEM;
		}
	}

	memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

	IMX385_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

	return CVI_SUCCESS;
}

static CVI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	IMX385_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
	SENSOR_FREE(pastSnsStateCtx);
	IMX385_SENSOR_RESET_CTX(ViPipe);
}

static CVI_S32 sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	CVI_S32 s32Ret;
	ISP_SENSOR_REGISTER_S stIspRegister;
	AE_SENSOR_REGISTER_S  stAeRegister;
	AWB_SENSOR_REGISTER_S stAwbRegister;
	ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

	CMOS_CHECK_POINTER(pstAeLib);
	CMOS_CHECK_POINTER(pstAwbLib);

	s32Ret = sensor_ctx_init(ViPipe);

	if (s32Ret != CVI_SUCCESS)
		return CVI_FAILURE;

	stSnsAttrInfo.eSensorId = IMX385_ID;

	s32Ret  = cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
	s32Ret |= CVI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function failed!\n");
		return s32Ret;
	}

	s32Ret  = cmos_init_ae_exp_function(&stAeRegister.stAeExp);
	s32Ret |= CVI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function to ae lib failed!\n");
		return s32Ret;
	}

	s32Ret  = cmos_init_awb_exp_function(&stAwbRegister.stAwbExp);
	s32Ret |= CVI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function to awb lib failed!\n");
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	CVI_S32 s32Ret;

	CMOS_CHECK_POINTER(pstAeLib);
	CMOS_CHECK_POINTER(pstAwbLib);

	s32Ret = CVI_ISP_SensorUnRegCallBack(ViPipe, IMX385_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX385_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function to ae lib failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX385_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function to awb lib failed!\n");
		return s32Ret;
	}

	sensor_ctx_exit(ViPipe);

	return CVI_SUCCESS;
}

static CVI_S32 sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
	CMOS_CHECK_POINTER(pstInitAttr);

	g_au32InitExposure[ViPipe] = pstInitAttr->u32Exposure;
	g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
	g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
	g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
	g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
	g_au16SampleRgain[ViPipe] = pstInitAttr->u16SampleRgain;
	g_au16SampleBgain[ViPipe] = pstInitAttr->u16SampleBgain;
	g_au16Imx385_GainMode[ViPipe] = pstInitAttr->enGainMode;

	return CVI_SUCCESS;
}

ISP_SNS_OBJ_S stSnsImx385_Obj = {
	.pfnRegisterCallback    = sensor_register_callback,
	.pfnUnRegisterCallback  = sensor_unregister_callback,
	.pfnStandby             = imx385_standby,
	.pfnRestart             = imx385_restart,
	.pfnMirrorFlip          = sensor_mirror_flip,
	.pfnWriteReg            = imx385_write_register,
	.pfnReadReg             = imx385_read_register,
	.pfnSetBusInfo          = imx385_set_bus_info,
	.pfnSetInit             = sensor_set_init,
	.pfnPatchRxAttr		= sensor_patch_rx_attr,
	.pfnPatchI2cAddr	= CVI_NULL,
	.pfnGetRxAttr		= sensor_rx_attr,
	.pfnExpSensorCb		= cmos_init_sensor_exp_function,
	.pfnExpAeCb		= cmos_init_ae_exp_function,
	.pfnSnsProbe		= CVI_NULL,
};

