#include <stdio.h>
#include <string.h>
#ifdef RUN_IN_SRAM
#include "config.h"
#include "marsrv_common.h"
#include "fw_config.h"
#elif (RUN_TYPE == CVIRTOS)
#include "cvi_type.h"
#endif
// #include "dw_uart.h"
#include "delay.h"
#include "mmio.h"

#include "sensor.h"
#include "cif_uapi.h"
#include "cvi_i2c.h"
#if CVI_I2C_DMA_ENABLE
#include "sysdma.h"
#endif


static SENSOR_INFO SenInfo = {0};
extern const char *snsr_type_name[SAMPLE_SNS_TYPE_BUTT];

/* Helper API to fill the stViConfig according to the pstIniCfg. */
//CVI_S32 SAMPLE_COMM_VI_IniToViCfg(SENSOR_INFO *pstIniCfg, SAMPLE_VI_CONFIG_S *pstViConfig)
CVI_S32 SensorInfoToViCfg(SENSOR_INFO *pSenInfo, SAMPLE_VI_CONFIG_S *pstViConfig)
{
	// DYNAMIC_RANGE_E    enDynamicRange	= DYNAMIC_RANGE_SDR8;
	// PIXEL_FORMAT_E	   enPixFormat		= VI_PIXEL_FORMAT;
	// VIDEO_FORMAT_E	   enVideoFormat	= VIDEO_FORMAT_LINEAR;
	// COMPRESS_MODE_E    enCompressMode	= COMPRESS_MODE_TILE;
	// VI_VPSS_MODE_E	   enMastPipeMode	= VI_OFFLINE_VPSS_OFFLINE;
	CVI_S32 s32WorkSnsId = 0;

	SENSOR_USR_CFG *pSenCfg = pSenInfo->cfg;

	if (!pstViConfig) {
		printf("%s: null ptr\n", __func__);
		return CVI_FAILURE;
	}

	SAMPLE_COMM_VI_GetSensorInfo(pstViConfig);
	for (; s32WorkSnsId < pSenInfo->header->dev_num; s32WorkSnsId++) {
		pstViConfig->s32WorkingViNum					= 1 + s32WorkSnsId;
		pstViConfig->as32WorkingViId[s32WorkSnsId]			= s32WorkSnsId;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.enSnsType	= g_enSnsType[s32WorkSnsId];
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.MipiDev		= pSenCfg->mipi_dev;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.stMclkAttr.bMclkEn = pSenCfg->mclk_en;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.stMclkAttr.u8Mclk = pSenCfg->mclk;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.s32BusId		= pSenCfg->bus_id;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.s32SnsI2cAddr	= pSenCfg->slave_id;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.u8HwSync		= pSenCfg->u8HwSync;
		pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.u8Orien		= pSenCfg->u8Orien;

		memcpy(pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as16LaneId,
			pSenCfg->lane_id, sizeof(pSenCfg->lane_id));
		memcpy(pstViConfig->astViInfo[s32WorkSnsId].stSnsInfo.as8PNSwap,
			pSenCfg->pn_swap, sizeof(pSenCfg->pn_swap));

		pstViConfig->astViInfo[s32WorkSnsId].stDevInfo.ViDev		= s32WorkSnsId;
		pstViConfig->astViInfo[s32WorkSnsId].stDevInfo.enWDRMode	=
		    (g_enSnsType[s32WorkSnsId] >= SAMPLE_SNS_TYPE_LINEAR_BUTT) ? WDR_MODE_2To1_LINE : WDR_MODE_NONE;
		pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
		pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]	= s32WorkSnsId;
		pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]	= -1;
		pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]	= -1;
		pstViConfig->astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]	= -1;

		// pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.ViChn		= s32WorkSnsId;
		// pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enPixFormat	= VI_PIXEL_FORMAT;
		// pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange	= DYNAMIC_RANGE_SDR8;
		// pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat	= VIDEO_FORMAT_LINEAR;
		// pstViConfig->astViInfo[s32WorkSnsId].stChnInfo.enCompressMode	= COMPRESS_MODE_TILE;

		// printf("s32WorkSnsId=%d, enSnsType=%d, bus_id=%d, mclk_en=%d\n",
		// 	s32WorkSnsId, g_enSnsType[s32WorkSnsId], pSenCfg->bus_id, pSenCfg->mclk_en);
	}

	return CVI_SUCCESS;
}

void start_camera(uint32_t u32SnsId)
{
	uint32_t i;
	SENSOR_USR_CFG *pSenCfg;
	SAMPLE_SNS_TYPE_E *pSnsType;

	static SAMPLE_VI_CONFIG_S stViConfigSys;
	ISP_SENSOR_EXP_FUNC_S stSnsrSensorFunc;
	VI_DEV_ATTR_S stViDevAttr;
	SNS_COMBO_DEV_ATTR_S stDevAttr;

	// PIC_SIZE_E	enSize;
	// SIZE_S		stSize;
#ifdef FPGA_PORTING // load from flash
	SENSOR_USR_CFG imx327_info = {
		.name 		= "SONY_IMX327_MIPI_1M_30FPS_10BIT",
		// .name 		= "SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1",
		.devno 		= 0,
		.mclk_en	= 1,
		.bus_id 	= 3,
		.lane_id 	= {0, 4, 3, -1, -1},
		.pn_swap 	= {0, 0, 0, 0, 0},
	};
	SENSOR_CFG_INI_HEADER sensor_ini_header = {
		.dev_num	= 1,
		.cfg_ofs	= ALIGN(sizeof(SENSOR_USR_CFG), 4),
	};

	pSenCfg = &imx327_info;
	SenInfo.cfg = pSenCfg;
	SenInfo.header = &sensor_ini_header;
	SenInfo.header->dev_num = 1;

	printf("SENSOR_USR_CFG size=%ld\n", sizeof(SENSOR_USR_CFG));
#else	// for ASIC
	SENSOR_CFG_INI_HEADER sensor_ini_header = {
		// .dev_num	= 1,
		.cfg_ofs	= ALIGN(sizeof(SENSOR_USR_CFG), 4),
	};
	SenInfo.header = &sensor_ini_header;

	#if 0 // test IMX327 MIPI
	SENSOR_USR_CFG imx327_info = {
		.name 		= "SONY_IMX327_MIPI_2M_30FPS_12BIT",
		.devno 		= 0,
		.bus_id 	= 3,
		.lane_id	= {2, 3, 1, 4, 0},
		.pn_swap	= {1, 1, 1, 1, 1},
	};
	pSenCfg = &imx327_info;
	SenInfo.header->dev_num = 1;
	#else // test GC4653
	SENSOR_USR_CFG gc4653_info = {
		.name 		= "GCORE_GC4653_MIPI_4M_30FPS_10BIT",
		.devno 		= 0,
		.bus_id 	= 3,
		.slave_id	= 41,
		.lane_id	= {1, 2, 0, -1, -1},
		.pn_swap	= {1, 1, 1, 0, 0},
	};
	pSenCfg = &gc4653_info;
	SenInfo.header->dev_num = 1;
	#endif

	SenInfo.cfg = pSenCfg;

	// printf("SENSOR_USR_CFG size=%ld\n", sizeof(SENSOR_USR_CFG));
	#if 0 // read from NonOS
	const struct cam_pll_s *clk;
	uint32_t pin, value;

	SenInfo.header = (SENSOR_CFG_INI_HEADER *) SEN_CFG_INI_ADDR;
	//memcpy(&SenInfo.header, (char *) SEN_CFG_INI_ADDR, sizeof(SENSOR_CFG_INI_HEADER));
	if (SenInfo.header->cfg_ofs != ALIGN(sizeof(SENSOR_USR_CFG), 4) {
		printf("SENSOR_USR_CFG size mismatch (%d:%d)\n",
			SenInfo.header->cfg_ofs, ALIGN(sizeof(SENSOR_USR_CFG));
		return;
	}

	SenInfo.cfg = (SENSOR_USR_CFG *) (SEN_CFG_ADDR + (u32SnsId * SenInfo.header->cfg_ofs));
	// SIMPLE_AE_CTRL_S *last_result = (SIMPLE_AE_CTRL_S *) SYSDMA_LLP_ADDR;
	#endif
#endif

	SenInfo.init_ok = 0;

	if (pSenCfg->devno > VI_MAX_DEV_NUM) {
		printf("[Error] sensor devno > %d\n", VI_MAX_DEV_NUM);
		return;
	}
	if (pSenCfg->bus_id > CVI_I2C_MAX_NUM) {
		printf("[Error] sensor i2c bus_id > %d\n", CVI_I2C_MAX_NUM);
		return;
	}

	pSnsType = &g_enSnsType[pSenCfg->devno];
	printf("init cam%d %s ", u32SnsId, pSenCfg->name);
	for (i = 0; i < SAMPLE_SNS_TYPE_BUTT; i++) {
		if (strcmp(pSenCfg->name, snsr_type_name[i]) == 0) {
			*pSnsType = i;
			break;
		}
	}
	printf("at %d\n", *pSnsType);

	// update sensor obj
	SenInfo.pstSnsObj = CVI_GetSnsObj(*pSnsType);

	// update sensor interface
	switch (*pSnsType) {
	// case SONY_IMX327_SUBLVDS_2M_30FPS_12BIT:
	// 	SenInfo.vi_mode = INPUT_MODE_SUBLVDS;
	// 	break;
	default:
		SenInfo.vi_mode = INPUT_MODE_MIPI;
		break;
	}

	// update sensor resolution
	// SAMPLE_COMM_VI_GetSizeBySensor(*pSnsType, &enSize);
	// SAMPLE_COMM_SYS_GetPicSize(enSize, &stSize);
	// SenInfo.width = stSize.u32Width;
	// SenInfo.height = stSize.u32Height;

	// update lane num
	switch (*pSnsType) {
	case SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
	case GCORE_GC4653_SLAVE_MIPI_4M_30FPS_10BIT:
		SenInfo.lane_num = DPHY_2_DLANE;
		break;
	default:
		SenInfo.lane_num = DPHY_4_DLANE;
		break;
	}

	// update HDR mode
	if (*pSnsType < SAMPLE_SNS_TYPE_LINEAR_BUTT)
		SenInfo.hdr_mode = HDR_MODE_LINEAR;

	// default sensor reset pin, should re-assign by board name
	#ifdef BOARD_WEVB_007A
	pSenCfg->snsr_reset = 0xA02;	// GPIOA_02
	#elif defined(BOARD_WEVB_006A)
	pSenCfg->snsr_reset = 0xC0D;	// GPIOC_13
	#endif

#ifdef FPGA_PORTING
	// power sequence
	const uint32_t fpga_pwr_reg = 0x0A0880F8;
	mmio_write_32(fpga_pwr_reg, 0x02100000);
	mmio_write_32(fpga_pwr_reg, 0);
	// printf("power-on delay\n");
	udelay(100);
	mmio_write_32(fpga_pwr_reg, 0x08);
	// printf("power-on delay\n");
	udelay(100);
	mmio_write_32(fpga_pwr_reg, 0x18);
	// printf("power-on delay\n");
	udelay(100);
	mmio_write_32(fpga_pwr_reg, 0x38);
	// printf("power-on delay\n");
	udelay(100);
	// clear reset and enable phy rx
	mmio_write_32(fpga_pwr_reg, 0x32100039);
	udelay(100);
#endif

	/************************************************
	 * step1:  Config VI
	 ************************************************/
	SAMPLE_COMM_VI_GetSensorInfo(&stViConfigSys);

	if (SensorInfoToViCfg(&SenInfo, &stViConfigSys) == CVI_FAILURE) {
		printf("SAMPLE_COMM_VI_InfoToViCfg failed\n");
		return;
	}
	/************************************************
	 * step2:  Config Sensor & ISP
	 ************************************************/
	if (SAMPLE_COMM_VI_StartSensor(&stViConfigSys) == CVI_FAILURE) {
		printf("SAMPLE_COMM_VI_StartSensor failed\n");
		return;
	}
	if (SAMPLE_COMM_VI_CreateIsp(&stViConfigSys) == CVI_FAILURE) {
		printf("SAMPLE_COMM_VI_CreateIsp failed\n");
		return;
	}

	/************************************************
	 * step3:  Start ISP driver
	 ************************************************/
	SAMPLE_COMM_VI_GetDevAttrBySns(pSenCfg->devno, *pSnsType, &stViDevAttr);
	CVI_VI_SetDevAttr(pSenCfg->devno, &stViDevAttr);
	/************************************************
	 * step4:  Sensor streaming
	 ************************************************/
	/*
	 * do cif init
	*/
	if (cif_open(&SenInfo) != CVI_SUCCESS) {
		printf("cif init fail\n");
		return;
	}

	// update rx_attr
	stDevAttr.input_mode = SenInfo.vi_mode;
	SenInfo.pstSnsObj->pfnGetRxAttr(pSenCfg->devno, &stDevAttr);

	stDevAttr.devno = pSenCfg->devno;
	// stDevAttr.img_size.width = SenInfo.width;
	// stDevAttr.img_size.height = SenInfo.height;
	SenInfo.width = stDevAttr.img_size.width;
	SenInfo.height = stDevAttr.img_size.height;
	stDevAttr.mclk.cam = (pSenCfg->mclk_en > 0) ? (pSenCfg->mclk_en - 1) : 0;
	if (SenInfo.vi_mode == INPUT_MODE_MIPI) {
		if (pSenCfg->hs_settle) {
			stDevAttr.mipi_attr.dphy.hs_settle = pSenCfg->hs_settle;
			stDevAttr.mipi_attr.dphy.enable = 1;
		}
	}
	for (i = 0; i < SEN_MAX_LANE_NUM; i++) {
		if (SenInfo.vi_mode == INPUT_MODE_MIPI) {
			stDevAttr.mipi_attr.lane_id[i] = pSenCfg->lane_id[i];
			stDevAttr.mipi_attr.pn_swap[i] = pSenCfg->pn_swap[i];
		} else if (SenInfo.vi_mode == INPUT_MODE_SUBLVDS) {
			stDevAttr.lvds_attr.lane_id[i] = pSenCfg->lane_id[i];
		}
	}

	cif_ioctl(pSenCfg->devno, CVI_MIPI_RESET_SENSOR, 0);
	cif_ioctl(pSenCfg->devno, CVI_MIPI_RESET_MIPI, 0);
	cif_ioctl(pSenCfg->devno, CVI_MIPI_SET_DEV_ATTR, (unsigned long) &stDevAttr);
	cif_ioctl(pSenCfg->devno, CVI_MIPI_ENABLE_SENSOR_CLOCK, 0);
	udelay(20);
	// cif_ioctl(pSenCfg->devno, CVI_MIPI_UNRESET_SENSOR, 0);
	// mdelay(20);

	SenInfo.pstSnsObj->pfnExpSensorCb(&stSnsrSensorFunc);
	if (stSnsrSensorFunc.pfn_cmos_sensor_init)
		stSnsrSensorFunc.pfn_cmos_sensor_init(pSenCfg->devno);

	// printf("vip_start_stream success\n");
}
