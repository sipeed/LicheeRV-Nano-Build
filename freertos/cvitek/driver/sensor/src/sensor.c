/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

/*
	common sensor i2c function
*/
uint8_t g_i2c_bus[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int sensor_i2c_init(VI_PIPE ViPipe, uint8_t u8I2cDev, uint16_t speed,
		uint8_t slave_addr, uint8_t alen, uint8_t dlen)
{
	g_i2c_bus[ViPipe] = u8I2cDev;

	return cvi_i2c_master_init(u8I2cDev, slave_addr, speed, alen, dlen);
}

int sensor_i2c_exit(VI_PIPE ViPipe)
{
	if (g_i2c_bus[ViPipe] >= 0) {
		g_i2c_bus[ViPipe] = -1;
		return CVI_SUCCESS;
	}

	return CVI_FAILURE;
}

int inline sensor_read_register(VI_PIPE ViPipe, int addr)
{
	return cvi_i2c_master_read(g_i2c_bus[ViPipe], addr);
}

int inline sensor_write_register(VI_PIPE ViPipe, int addr, int data)
{
	return cvi_i2c_master_write(g_i2c_bus[ViPipe], addr, data);
}

void sensor_prog(VI_PIPE ViPipe, int *rom)
{
	int i = 0;

	while (1) {
		int lookup = rom[i++];
		int addr = (lookup >> 16) & 0xFFFF;
		int data = lookup & 0xFFFF;

		if (addr == 0xFFFE)
			delay_ms(data);
		else if (addr != 0xFFFF)
			sensor_write_register(ViPipe, addr, data);
	}
}

int32_t cmos_set_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *pstSnsSyncInfo)
{
	uint8_t i, count;
	ISP_I2C_DATA_S *pstI2c_data = pstSnsSyncInfo->snsCfg.astI2cData;

	count = pstSnsSyncInfo->snsCfg.u32RegNum;

	// read back check
	for (i = 0; i < count; i++) {
		if (!pstI2c_data[i].bUpdate)
			continue;

		if (pstI2c_data[i].u8DelayFrmNum > 0) {
			pstI2c_data[i].u8DelayFrmNum--;
			continue;
		}

		sensor_write_register(ViPipe, pstI2c_data[i].u32RegAddr, pstI2c_data[i].u32Data);
		pstI2c_data[i].bUpdate = false;
	}

	return CVI_SUCCESS;
}

CVI_VOID *CVI_GetSnsObj(SAMPLE_SNS_TYPE_E enSnsType)
{
	CVI_VOID *pSnsObj;

	switch (enSnsType) {
#if defined(SENSOR_SONY_IMX327)
	case SONY_IMX327_MIPI_1M_30FPS_10BIT:
	case SONY_IMX327_MIPI_1M_30FPS_10BIT_WDR2TO1:
	case SONY_IMX327_MIPI_2M_30FPS_12BIT:
	case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
		return &stSnsImx327_Obj;
#endif
#if defined(SENSOR_GCORE_GC4653)
	case GCORE_GC4653_MIPI_4M_30FPS_10BIT:
	case GCORE_GC4653_SLAVE_MIPI_4M_30FPS_10BIT:
		return &stSnsGc4653_Obj;
#endif
	default:
		pSnsObj = CVI_NULL;
		printf("verify sensor name failed!!\n");
		break;
	}

	return pSnsObj;
}
