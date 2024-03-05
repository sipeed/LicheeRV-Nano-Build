#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cvi_vip_snsr.h>

#include "cvi_sns_ctrl.h"
#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"
#include "os02d10_slave_cmos_ex.h"

static void os02d10_slave_linear_1080p30_init(VI_PIPE ViPipe);

CVI_U8 os02d10_slave_i2c_addr = 0x3c;        /* I2C Address of OS02D10_SLAVE_SLAVE */
const CVI_U32 os02d10_slave_addr_byte = 1;
const CVI_U32 os02d10_slave_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int os02d10_slave_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunOs02d10_Slave_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, os02d10_slave_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int os02d10_slave_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int os02d10_slave_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (os02d10_slave_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, os02d10_slave_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, os02d10_slave_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (os02d10_slave_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int os02d10_slave_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (os02d10_slave_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
	}
	buf[idx] = addr & 0xff;
	idx++;


	if (os02d10_slave_data_byte == 2) {
		buf[idx] = (data >> 8) & 0xff;
		idx++;
	}
	buf[idx] = data & 0xff;
	idx++;

	ret = write(g_fd[ViPipe], buf, os02d10_slave_addr_byte + os02d10_slave_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error! %#x\n", addr);
		return CVI_FAILURE;
	}

	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void os02d10_slave_switch_page(VI_PIPE ViPipe, CVI_U32 page)
{
	os02d10_slave_write_register(ViPipe, 0xFD, page);
}

void os02d10_slave_standby(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

void os02d10_slave_restart(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

void os02d10_slave_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;
	CVI_U32 start = 0;
	CVI_U32 end = g_pastOs02d10_Slave[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum;

	for (i = start; i < end; i++) {
		os02d10_slave_write_register(ViPipe,
				g_pastOs02d10_Slave[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastOs02d10_Slave[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

#define OS02D10_SALVE_CHIP_ID_ADDR_1		0x02
#define OS02D10_SALVE_CHIP_ID_1				0x23
#define OS02D10_SALVE_CHIP_ID_ADDR_2		0x03
#define OS02D10_SALVE_CHIP_ID_2				0x09

int os02d10_slave_probe(VI_PIPE ViPipe)
{
	int nVal, nVal2;

	if (os02d10_slave_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	usleep(1500);
	os02d10_slave_switch_page(ViPipe, 0);
	nVal  = os02d10_slave_read_register(ViPipe, OS02D10_SALVE_CHIP_ID_ADDR_1);
	nVal2 = os02d10_slave_read_register(ViPipe, OS02D10_SALVE_CHIP_ID_ADDR_2);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if (nVal != OS02D10_SALVE_CHIP_ID_1 || nVal2 != OS02D10_SALVE_CHIP_ID_2) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID(%#x, %#x) Mismatch! Use the wrong sensor??\n", nVal, nVal2);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void os02d10_slave_init(VI_PIPE ViPipe)
{
	os02d10_slave_i2c_init(ViPipe);

	os02d10_slave_linear_1080p30_init(ViPipe);

	g_pastOs02d10_Slave[ViPipe]->bInit = CVI_TRUE;
}

void os02d10_slave_exit(VI_PIPE ViPipe)
{
	os02d10_slave_i2c_exit(ViPipe);
}

/* 1080P30 */
static void os02d10_slave_linear_1080p30_init(VI_PIPE ViPipe)
{
	os02d10_slave_write_register(ViPipe, 0xfd, 0x00);
	os02d10_slave_write_register(ViPipe, 0x36, 0x01);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x00);
	os02d10_slave_write_register(ViPipe, 0x36, 0x00);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x00);
	os02d10_slave_write_register(ViPipe, 0x20, 0x00);
	delay_ms(5);

	os02d10_slave_write_register(ViPipe, 0xfd, 0x00);
	os02d10_slave_write_register(ViPipe, 0x41, 0x06);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x01);
	os02d10_slave_write_register(ViPipe, 0x03, 0x01);
	os02d10_slave_write_register(ViPipe, 0x04, 0x69);
	os02d10_slave_write_register(ViPipe, 0x05, 0x00);
	os02d10_slave_write_register(ViPipe, 0x06, 0x00); // for frame sync
	os02d10_slave_write_register(ViPipe, 0x09, 0x00); // for frame sync
	os02d10_slave_write_register(ViPipe, 0x0a, 0x00); // for frame sync
	os02d10_slave_write_register(ViPipe, 0x01, 0x01); // for frame sync
	os02d10_slave_write_register(ViPipe, 0x0a, 0xa0);
	os02d10_slave_write_register(ViPipe, 0x24, 0x10);
	os02d10_slave_write_register(ViPipe, 0x01, 0x01);
	os02d10_slave_write_register(ViPipe, 0x12, 0x01);
	os02d10_slave_write_register(ViPipe, 0x11, 0x4f);
	os02d10_slave_write_register(ViPipe, 0x19, 0xa0);
	os02d10_slave_write_register(ViPipe, 0xd5, 0x44);
	os02d10_slave_write_register(ViPipe, 0xd6, 0x01);
	os02d10_slave_write_register(ViPipe, 0xd7, 0x19);
	os02d10_slave_write_register(ViPipe, 0x16, 0x38);
	os02d10_slave_write_register(ViPipe, 0x1d, 0x66);
	os02d10_slave_write_register(ViPipe, 0x1f, 0x23);
	os02d10_slave_write_register(ViPipe, 0x21, 0x05);
	os02d10_slave_write_register(ViPipe, 0x25, 0x0f);
	os02d10_slave_write_register(ViPipe, 0x27, 0x46);
	os02d10_slave_write_register(ViPipe, 0x2a, 0x03);
	os02d10_slave_write_register(ViPipe, 0x2b, 0xa2);
	os02d10_slave_write_register(ViPipe, 0x2c, 0x01);
	os02d10_slave_write_register(ViPipe, 0x20, 0x08);
	os02d10_slave_write_register(ViPipe, 0x38, 0x10);
	os02d10_slave_write_register(ViPipe, 0x45, 0xce);
	os02d10_slave_write_register(ViPipe, 0x51, 0x2a);
	os02d10_slave_write_register(ViPipe, 0x52, 0x2a);
	os02d10_slave_write_register(ViPipe, 0x55, 0x15);
	os02d10_slave_write_register(ViPipe, 0x57, 0x20);
	os02d10_slave_write_register(ViPipe, 0x5d, 0x00);
	os02d10_slave_write_register(ViPipe, 0x5e, 0x18);
	os02d10_slave_write_register(ViPipe, 0x66, 0x58);
	os02d10_slave_write_register(ViPipe, 0x68, 0x50);
	os02d10_slave_write_register(ViPipe, 0x71, 0xf0);
	os02d10_slave_write_register(ViPipe, 0x72, 0x25);
	os02d10_slave_write_register(ViPipe, 0x73, 0x30);
	os02d10_slave_write_register(ViPipe, 0x74, 0x25);
	os02d10_slave_write_register(ViPipe, 0x77, 0x02);
	os02d10_slave_write_register(ViPipe, 0x79, 0x00);
	os02d10_slave_write_register(ViPipe, 0x7a, 0x00);
	os02d10_slave_write_register(ViPipe, 0x7b, 0x15);
	os02d10_slave_write_register(ViPipe, 0x8a, 0x11);
	os02d10_slave_write_register(ViPipe, 0x8b, 0x11);
	os02d10_slave_write_register(ViPipe, 0xa1, 0x02);
	os02d10_slave_write_register(ViPipe, 0xb1, 0x01);
	os02d10_slave_write_register(ViPipe, 0xc4, 0x7a);
	os02d10_slave_write_register(ViPipe, 0xc5, 0x7a);
	os02d10_slave_write_register(ViPipe, 0xc6, 0x7a);
	os02d10_slave_write_register(ViPipe, 0xc7, 0x7a);
	os02d10_slave_write_register(ViPipe, 0xf0, 0x40);
	os02d10_slave_write_register(ViPipe, 0xf1, 0x40);
	os02d10_slave_write_register(ViPipe, 0xf2, 0x40);
	os02d10_slave_write_register(ViPipe, 0xf3, 0x40);
	os02d10_slave_write_register(ViPipe, 0xf4, 0x07);
	os02d10_slave_write_register(ViPipe, 0xf6, 0x37);
	os02d10_slave_write_register(ViPipe, 0xf7, 0xf7);
	os02d10_slave_write_register(ViPipe, 0xfc, 0x37);
	os02d10_slave_write_register(ViPipe, 0xfe, 0xf7);
	os02d10_slave_write_register(ViPipe, 0x48, 0xf6);
	os02d10_slave_write_register(ViPipe, 0xfa, 0x10);
	os02d10_slave_write_register(ViPipe, 0xfb, 0x58);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x02);
	os02d10_slave_write_register(ViPipe, 0x34, 0xff);
	os02d10_slave_write_register(ViPipe, 0xa0, 0x00);
	os02d10_slave_write_register(ViPipe, 0xa1, 0x04);
	os02d10_slave_write_register(ViPipe, 0xa2, 0x04);
	os02d10_slave_write_register(ViPipe, 0xa3, 0x40);
	os02d10_slave_write_register(ViPipe, 0xa4, 0x00);
	os02d10_slave_write_register(ViPipe, 0xa5, 0x04);
	os02d10_slave_write_register(ViPipe, 0xa6, 0x07);
	os02d10_slave_write_register(ViPipe, 0xa7, 0x88);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x01);
	os02d10_slave_write_register(ViPipe, 0x8e, 0x07);
	os02d10_slave_write_register(ViPipe, 0x8f, 0x88);
	os02d10_slave_write_register(ViPipe, 0x90, 0x04);
	os02d10_slave_write_register(ViPipe, 0x91, 0x40);

	os02d10_slave_write_register(ViPipe, 0x92, 0x08);//uncontinue mode
	//os02d10_slave_write_register(ViPipe, 0x98, 0xff);//hs trail

	os02d10_slave_write_register(ViPipe, 0x01, 0x01);
	os02d10_slave_write_register(ViPipe, 0xfd, 0x01);
	os02d10_slave_write_register(ViPipe, 0xb1, 0x03);

	os02d10_slave_default_reg_init(ViPipe);

	if (g_au16Os02d10_Slave_UseHwSync[ViPipe]) {
		os02d10_slave_write_register(ViPipe, 0xfd, 0x00);
		os02d10_slave_write_register(ViPipe, 0x1b, 0xff);
		os02d10_slave_write_register(ViPipe, 0x40, 0x01); // enable input of dataout[0]
		os02d10_slave_write_register(ViPipe, 0xfd, 0x01);
		os02d10_slave_write_register(ViPipe, 0x0b, 0x02); // enter into slave mode
		os02d10_slave_write_register(ViPipe, 0x01, 0x01);
	}

	delay_ms(50);

	printf("ViPipe:%d,===OS02D10_SLAVE_SLAVE 1080P 30fps 10bit LINE Init OK!===\n", ViPipe);
}


