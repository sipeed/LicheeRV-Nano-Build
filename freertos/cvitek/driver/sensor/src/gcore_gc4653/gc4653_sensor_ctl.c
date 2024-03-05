#include <stdio.h>

#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"

#include "sensor.h"
#include "cvi_i2c.h"
#include "cif_uapi.h"

#include "gc4653_cmos_ex.h"

#define GC4653_CHIP_ID_ADDR_H	0x03f0
#define GC4653_CHIP_ID_ADDR_L	0x03f1
#define GC4653_CHIP_ID		0x4653

static void gc4653_linear_1440p30_init(VI_PIPE ViPipe);

CVI_U8 gc4653_i2c_addr = 0x10;
const CVI_U32 gc4653_addr_byte = 2;
const CVI_U32 gc4653_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

void gc4653_standby(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x0100, 0x00);
	sensor_write_register(ViPipe, 0x031c, 0xc7);
	sensor_write_register(ViPipe, 0x0317, 0x01);

	printf("gc4653_standby\n");
}

void gc4653_restart(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x0317, 0x00);
	sensor_write_register(ViPipe, 0x031c, 0xc6);
	sensor_write_register(ViPipe, 0x0100, 0x09);

	printf("gc4653_restart\n");
}

void gc4653_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastGc4653[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sensor_write_register(ViPipe,
				g_pastGc4653[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastGc4653[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

int gc4653_probe(VI_PIPE ViPipe)
{
	int nVal;
	int nVal2;

	usleep(50);
	if (sensor_i2c_init(ViPipe, g_aunGc4653_BusInfo[ViPipe].s8I2cDev, I2C_400KHZ,
		gc4653_i2c_addr, gc4653_addr_byte, gc4653_data_byte) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = sensor_read_register(ViPipe, GC4653_CHIP_ID_ADDR_H);
	nVal2 = sensor_read_register(ViPipe, GC4653_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != GC4653_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void gc4653_init(VI_PIPE ViPipe)
{
#if (RUN_TYPE == CVIRTOS)
	// release sensor reset pin
	cif_ioctl(ViPipe, CVI_MIPI_UNRESET_SENSOR, 0);
	// mdelay(20);
	udelay(200);
#endif
	if (sensor_i2c_init(ViPipe, g_aunGc4653_BusInfo[ViPipe].s8I2cDev, I2C_400KHZ,
		gc4653_i2c_addr, gc4653_addr_byte, gc4653_data_byte) != CVI_SUCCESS)
		return;

	gc4653_linear_1440p30_init(ViPipe);

	g_pastGc4653[ViPipe]->bInit = CVI_TRUE;
}

static void gc4653_linear_1440p30_init(VI_PIPE ViPipe)
{
	// delay_ms(10);
	/****system****/
	sensor_write_register(ViPipe, 0x03fe, 0xf0);
	sensor_write_register(ViPipe, 0x03fe, 0x00);
	sensor_write_register(ViPipe, 0x0317, 0x00);
	sensor_write_register(ViPipe, 0x0320, 0x77);
	sensor_write_register(ViPipe, 0x0324, 0xc8);
	sensor_write_register(ViPipe, 0x0325, 0x06);
	sensor_write_register(ViPipe, 0x0326, 0x60);
	sensor_write_register(ViPipe, 0x0327, 0x03);
	sensor_write_register(ViPipe, 0x0334, 0x40);
	sensor_write_register(ViPipe, 0x0336, 0x60);
	sensor_write_register(ViPipe, 0x0337, 0x82);
	sensor_write_register(ViPipe, 0x0315, 0x25);
	sensor_write_register(ViPipe, 0x031c, 0xc6);
	/****************************************/
	/*frame structure*/
	/****************************************/
	sensor_write_register(ViPipe, 0x0287, 0x18);
	sensor_write_register(ViPipe, 0x0084, 0x00);
	sensor_write_register(ViPipe, 0x0087, 0x50);
	sensor_write_register(ViPipe, 0x029d, 0x08);
	sensor_write_register(ViPipe, 0x0290, 0x00);
	/**********AHD 30 other need change ******************/
	sensor_write_register(ViPipe, 0x0340, 0x05);
	sensor_write_register(ViPipe, 0x0341, 0xdc);
	sensor_write_register(ViPipe, 0x0345, 0x06);
	sensor_write_register(ViPipe, 0x034b, 0xb0);
	sensor_write_register(ViPipe, 0x0352, 0x08);
	sensor_write_register(ViPipe, 0x0354, 0x08);
	/****************************************/
	/*ANALOG CIRCUIT*/
	/****************************************/
	sensor_write_register(ViPipe, 0x02d1, 0xe0);
	sensor_write_register(ViPipe, 0x0223, 0xf2);
	sensor_write_register(ViPipe, 0x0238, 0xa4);
	sensor_write_register(ViPipe, 0x02ce, 0x7f);
	sensor_write_register(ViPipe, 0x0232, 0xc4);
	sensor_write_register(ViPipe, 0x02d3, 0x05);
	sensor_write_register(ViPipe, 0x0243, 0x06);
	sensor_write_register(ViPipe, 0x02ee, 0x30);
	sensor_write_register(ViPipe, 0x026f, 0x70);
	sensor_write_register(ViPipe, 0x0257, 0x09);
	sensor_write_register(ViPipe, 0x0211, 0x02);
	sensor_write_register(ViPipe, 0x0219, 0x09);
	sensor_write_register(ViPipe, 0x023f, 0x2d);
	sensor_write_register(ViPipe, 0x0518, 0x00);
	sensor_write_register(ViPipe, 0x0519, 0x01);
	sensor_write_register(ViPipe, 0x0515, 0x08);
	sensor_write_register(ViPipe, 0x02d9, 0x3f);
	sensor_write_register(ViPipe, 0x02da, 0x02);
	sensor_write_register(ViPipe, 0x02db, 0xe8);
	sensor_write_register(ViPipe, 0x02e6, 0x20);
	sensor_write_register(ViPipe, 0x021b, 0x10);
	sensor_write_register(ViPipe, 0x0252, 0x22);
	sensor_write_register(ViPipe, 0x024e, 0x22);
	sensor_write_register(ViPipe, 0x02c4, 0x01);
	sensor_write_register(ViPipe, 0x021d, 0x17);
	sensor_write_register(ViPipe, 0x024a, 0x01);
	sensor_write_register(ViPipe, 0x02ca, 0x02);
	sensor_write_register(ViPipe, 0x0262, 0x10);
	sensor_write_register(ViPipe, 0x029a, 0x20);
	sensor_write_register(ViPipe, 0x021c, 0x0e);
	sensor_write_register(ViPipe, 0x0298, 0x03);
	sensor_write_register(ViPipe, 0x029c, 0x00);
	sensor_write_register(ViPipe, 0x027e, 0x14);
	sensor_write_register(ViPipe, 0x02c2, 0x10);
	sensor_write_register(ViPipe, 0x0540, 0x20);
	sensor_write_register(ViPipe, 0x0546, 0x01);
	sensor_write_register(ViPipe, 0x0548, 0x01);
	sensor_write_register(ViPipe, 0x0544, 0x01);
	sensor_write_register(ViPipe, 0x0242, 0x1b);
	sensor_write_register(ViPipe, 0x02c0, 0x1b);
	sensor_write_register(ViPipe, 0x02c3, 0x20);
	sensor_write_register(ViPipe, 0x02e4, 0x10);
	sensor_write_register(ViPipe, 0x022e, 0x00);
	sensor_write_register(ViPipe, 0x027b, 0x3f);
	sensor_write_register(ViPipe, 0x0269, 0x0f);
	sensor_write_register(ViPipe, 0x02d2, 0x40);
	sensor_write_register(ViPipe, 0x027c, 0x08);
	sensor_write_register(ViPipe, 0x023a, 0x2e);
	sensor_write_register(ViPipe, 0x0245, 0xce);
	sensor_write_register(ViPipe, 0x0530, 0x20);
	sensor_write_register(ViPipe, 0x0531, 0x02);
	sensor_write_register(ViPipe, 0x0228, 0x50);
	sensor_write_register(ViPipe, 0x02ab, 0x00);
	sensor_write_register(ViPipe, 0x0250, 0x00);
	sensor_write_register(ViPipe, 0x0221, 0x50);
	sensor_write_register(ViPipe, 0x02ac, 0x00);
	sensor_write_register(ViPipe, 0x02a5, 0x02);
	sensor_write_register(ViPipe, 0x0260, 0x0b);
	sensor_write_register(ViPipe, 0x0216, 0x04);
	sensor_write_register(ViPipe, 0x0299, 0x1C);
	sensor_write_register(ViPipe, 0x02bb, 0x0d);
	sensor_write_register(ViPipe, 0x02a3, 0x02);
	sensor_write_register(ViPipe, 0x02a4, 0x02);
	sensor_write_register(ViPipe, 0x021e, 0x02);
	sensor_write_register(ViPipe, 0x024f, 0x08);
	sensor_write_register(ViPipe, 0x028c, 0x08);
	sensor_write_register(ViPipe, 0x0532, 0x3f);
	sensor_write_register(ViPipe, 0x0533, 0x02);
	sensor_write_register(ViPipe, 0x0277, 0xc0);
	sensor_write_register(ViPipe, 0x0276, 0xc0);
	sensor_write_register(ViPipe, 0x0239, 0xc0);
	/*exp*/
	sensor_write_register(ViPipe, 0x0202, 0x05);
	sensor_write_register(ViPipe, 0x0203, 0x46);
	/*gain*/
	sensor_write_register(ViPipe, 0x0205, 0xc0);
	sensor_write_register(ViPipe, 0x02b0, 0x68);
	/*dpc*/
	sensor_write_register(ViPipe, 0x0002, 0xa9);
	sensor_write_register(ViPipe, 0x0004, 0x01);
	/*dark_sun*/
	sensor_write_register(ViPipe, 0x021a, 0x98);
	sensor_write_register(ViPipe, 0x0266, 0xa0);
	sensor_write_register(ViPipe, 0x0020, 0x01);
	sensor_write_register(ViPipe, 0x0021, 0x03);
	sensor_write_register(ViPipe, 0x0022, 0x00);
	sensor_write_register(ViPipe, 0x0023, 0x04);
	/****************************************/
	/*mipi*/
	/****************************************/
	/***********   AHD 30 ******************/
	/*30fps*/
	sensor_write_register(ViPipe, 0x0342, 0x06);
	sensor_write_register(ViPipe, 0x0343, 0x40);
	/*30fps*/
	sensor_write_register(ViPipe, 0x03fe, 0x10);
	sensor_write_register(ViPipe, 0x03fe, 0x00);
	sensor_write_register(ViPipe, 0x0106, 0x78);
	sensor_write_register(ViPipe, 0x0108, 0x0c);
	sensor_write_register(ViPipe, 0x0114, 0x01);
	sensor_write_register(ViPipe, 0x0115, 0x12);
	sensor_write_register(ViPipe, 0x0180, 0x46);
	sensor_write_register(ViPipe, 0x0181, 0x30);
	sensor_write_register(ViPipe, 0x0182, 0x05);
	sensor_write_register(ViPipe, 0x0185, 0x01);
	sensor_write_register(ViPipe, 0x03fe, 0x10);
	sensor_write_register(ViPipe, 0x03fe, 0x00);
	sensor_write_register(ViPipe, 0x0100, 0x09);
	// 0x008e = 0x00, which means disabling bayer transformation when flip/mirroring
	//sensor_write_register(ViPipe, 0x008e, 0x00);
	//fix FPN
	sensor_write_register(ViPipe, 0x0277, 0x38);
	sensor_write_register(ViPipe, 0x0276, 0xc0);
	sensor_write_register(ViPipe, 0x000f, 0x10);
	sensor_write_register(ViPipe, 0x0059, 0x00);//close dither
	//otp
	sensor_write_register(ViPipe, 0x0080, 0x02);
	sensor_write_register(ViPipe, 0x0097, 0x0a);
	sensor_write_register(ViPipe, 0x0098, 0x10);
	sensor_write_register(ViPipe, 0x0099, 0x05);
	sensor_write_register(ViPipe, 0x009a, 0xb0);
	sensor_write_register(ViPipe, 0x0317, 0x08);
	sensor_write_register(ViPipe, 0x0a67, 0x80);
	sensor_write_register(ViPipe, 0x0a70, 0x03);
	sensor_write_register(ViPipe, 0x0a82, 0x00);
	sensor_write_register(ViPipe, 0x0a83, 0x10);
	sensor_write_register(ViPipe, 0x0a80, 0x2b);
	sensor_write_register(ViPipe, 0x05be, 0x00);
	sensor_write_register(ViPipe, 0x05a9, 0x01);
	sensor_write_register(ViPipe, 0x0313, 0x80);
	sensor_write_register(ViPipe, 0x05be, 0x01);
	sensor_write_register(ViPipe, 0x0317, 0x00);
	sensor_write_register(ViPipe, 0x0a67, 0x00);

	// gc4653_default_reg_init(ViPipe);
	// delay_ms(10);

	printf("ViPipe:%d,===GC4653 1440P 30fps 10bit LINEAR Init OK!===\n", ViPipe);
}
