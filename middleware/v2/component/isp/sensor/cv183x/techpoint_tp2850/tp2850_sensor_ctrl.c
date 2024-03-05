#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cvi_vip_snsr.h>
#include <linux/spi/spidev.h>
#include "cvi_sns_ctrl.h"
#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"
#include "tp2850_cmos_ex.h"
#include <pthread.h>
#include <signal.h>

const CVI_U8 tp2850_i2c_addr = 0x44;        /* I2C slave address of TP2850, SA0=0:0x44, SA0=1:0x45*/
const CVI_U32 tp2850_addr_byte = 1;
const CVI_U32 tp2850_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};
static pthread_t g_tp2850_thid;

#define TP2850_BLUE_SCREEN 0

int tp2850_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;
	int ret;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;

	u8DevNum = g_aunTP2850_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
	syslog(LOG_DEBUG, "open %s\n", acDevFile);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, tp2850_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int tp2850_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int tp2850_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return 0;

	if (tp2850_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, tp2850_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, tp2850_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (tp2850_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int tp2850_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (tp2850_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	if (tp2850_data_byte == 2)
		buf[idx++] = (data >> 8) & 0xff;

	// add data byte 0
	buf[idx++] = data & 0xff;

	ret = write(g_fd[ViPipe], buf, tp2850_addr_byte + tp2850_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);

#if 0 // read back checing
	ret = tp2850_read_register(ViPipe, addr);
	if (ret != data)
		syslog(LOG_DEBUG, "i2c readback-check fail, 0x%x != 0x%x\n", ret, data);
#endif
	return CVI_SUCCESS;
}

void tp2850_init_setting(VI_PIPE ViPipe, CVI_U8 mode)
{
	unsigned char tmp;

	tp2850_write_register(ViPipe, 0x40, 0x08); //MIPI page
	tp2850_write_register(ViPipe, 0x01, 0xf8);
	tp2850_write_register(ViPipe, 0x02, 0x01);
	tp2850_write_register(ViPipe, 0x08, 0x03);
	tp2850_write_register(ViPipe, 0x20, 0x12);
	tp2850_write_register(ViPipe, 0x39, 0x00);

	tp2850_write_register(ViPipe, 0x40, 0x00); //decoder page
	tp2850_write_register(ViPipe, 0x4c, 0x40);
	tp2850_write_register(ViPipe, 0x4e, 0x00);
	tp2850_write_register(ViPipe, 0x27, 0x2d);
	tp2850_write_register(ViPipe, 0xfd, 0x80);

	if (mode == TP2850_MODE_1080P_30P) {
		tp2850_write_register(ViPipe, 0x02, 0x40);
		tp2850_write_register(ViPipe, 0x07, 0xc0);
		tp2850_write_register(ViPipe, 0x0b, 0xc0);
		tp2850_write_register(ViPipe, 0x0c, 0x03);
		tp2850_write_register(ViPipe, 0x0d, 0x50);

		tp2850_write_register(ViPipe, 0x15, 0x03);
		tp2850_write_register(ViPipe, 0x16, 0xd2);
		tp2850_write_register(ViPipe, 0x17, 0x80);
		tp2850_write_register(ViPipe, 0x18, 0x29);
		tp2850_write_register(ViPipe, 0x19, 0x38);
		tp2850_write_register(ViPipe, 0x1a, 0x47);
		tp2850_write_register(ViPipe, 0x1c, 0x08);  //1920*1080, 30fps
		tp2850_write_register(ViPipe, 0x1d, 0x98);  //

		tp2850_write_register(ViPipe, 0x20, 0x30);
		tp2850_write_register(ViPipe, 0x21, 0x84);
		tp2850_write_register(ViPipe, 0x22, 0x36);
		tp2850_write_register(ViPipe, 0x23, 0x3c);

		tp2850_write_register(ViPipe, 0x2b, 0x60);
		tp2850_write_register(ViPipe, 0x2c, 0x0a);
		tp2850_write_register(ViPipe, 0x2d, 0x30);
		tp2850_write_register(ViPipe, 0x2e, 0x70);

		tp2850_write_register(ViPipe, 0x30, 0x48);
		tp2850_write_register(ViPipe, 0x31, 0xbb);
		tp2850_write_register(ViPipe, 0x32, 0x2e);
		tp2850_write_register(ViPipe, 0x33, 0x90);

		tp2850_write_register(ViPipe, 0x35, 0x05);
		tp2850_write_register(ViPipe, 0x38, 0x00);
		tp2850_write_register(ViPipe, 0x39, 0x1C);

		tp2850_write_register(ViPipe, 0x02, 0x44);

		tp2850_write_register(ViPipe, 0x0d, 0x72);

		tp2850_write_register(ViPipe, 0x15, 0x01);
		tp2850_write_register(ViPipe, 0x16, 0xf0);
		tp2850_write_register(ViPipe, 0x18, 0x2a);

		tp2850_write_register(ViPipe, 0x20, 0x38);
		tp2850_write_register(ViPipe, 0x21, 0x46);

		tp2850_write_register(ViPipe, 0x25, 0xfe);
		tp2850_write_register(ViPipe, 0x26, 0x0d);

		tp2850_write_register(ViPipe, 0x2c, 0x3a);
		tp2850_write_register(ViPipe, 0x2d, 0x54);
		tp2850_write_register(ViPipe, 0x2e, 0x40);

		tp2850_write_register(ViPipe, 0x30, 0xa5);
		tp2850_write_register(ViPipe, 0x31, 0x95);
		tp2850_write_register(ViPipe, 0x32, 0xe0);
		tp2850_write_register(ViPipe, 0x33, 0x60);

		tp2850_write_register(ViPipe, 0x40, 0x08);
		tp2850_write_register(ViPipe, 0x23, 0x02);
		tp2850_write_register(ViPipe, 0x13, 0x04);
		tp2850_write_register(ViPipe, 0x14, 0x46);
		tp2850_write_register(ViPipe, 0x15, 0x09);

		tp2850_write_register(ViPipe, 0x25, 0x08);
		tp2850_write_register(ViPipe, 0x26, 0x04);
		tp2850_write_register(ViPipe, 0x27, 0x0c);

		tp2850_write_register(ViPipe, 0x10, 0x88);
		tp2850_write_register(ViPipe, 0x10, 0x08);
		tp2850_write_register(ViPipe, 0x23, 0x00);
		tp2850_write_register(ViPipe, 0x40, 0x00);
	} else { //TP2850_MODE_1440P_30P
		tp2850_write_register(ViPipe, 0x02, 0x50);
		tp2850_write_register(ViPipe, 0x07, 0xc0);
		tp2850_write_register(ViPipe, 0x0b, 0xc0);
		tp2850_write_register(ViPipe, 0x0c, 0x03);
		tp2850_write_register(ViPipe, 0x0d, 0x50);

		tp2850_write_register(ViPipe, 0x15, 0x23);
		tp2850_write_register(ViPipe, 0x16, 0x1b);
		tp2850_write_register(ViPipe, 0x17, 0x00); // HOUT=0xa00=2560
		tp2850_write_register(ViPipe, 0x18, 0x38);
		tp2850_write_register(ViPipe, 0x19, 0xa0); // VOUT=0x5a0=1440
		tp2850_write_register(ViPipe, 0x1a, 0x5a);
		tp2850_write_register(ViPipe, 0x1c, 0x0c); // HTS=3298
		tp2850_write_register(ViPipe, 0x1d, 0xe2);

		tp2850_write_register(ViPipe, 0x20, 0x50);
		tp2850_write_register(ViPipe, 0x21, 0x84);
		tp2850_write_register(ViPipe, 0x22, 0x36);
		tp2850_write_register(ViPipe, 0x23, 0x3c);

		tp2850_write_register(ViPipe, 0x27, 0xad);

		tp2850_write_register(ViPipe, 0x2b, 0x60);
		tp2850_write_register(ViPipe, 0x2c, 0x0a);
		tp2850_write_register(ViPipe, 0x2d, 0x58);
		tp2850_write_register(ViPipe, 0x2e, 0x70);

		tp2850_write_register(ViPipe, 0x30, 0x74);
		tp2850_write_register(ViPipe, 0x31, 0x58);
		tp2850_write_register(ViPipe, 0x32, 0x9f);
		tp2850_write_register(ViPipe, 0x33, 0x60);

		tp2850_write_register(ViPipe, 0x35, 0x15);
		tp2850_write_register(ViPipe, 0x36, 0xdc);
		tp2850_write_register(ViPipe, 0x38, 0x40);
		tp2850_write_register(ViPipe, 0x39, 0x48);

		tmp = tp2850_read_register(ViPipe, 0x14);
		tmp |= 0x40;
		tp2850_write_register(ViPipe, 0x14, tmp);

		tp2850_write_register(ViPipe, 0x13, 0x00);
		tp2850_write_register(ViPipe, 0x15, 0x23);
		tp2850_write_register(ViPipe, 0x16, 0x16);
		tp2850_write_register(ViPipe, 0x18, 0x32);

		tp2850_write_register(ViPipe, 0x20, 0x80);
		tp2850_write_register(ViPipe, 0x21, 0x86);
		tp2850_write_register(ViPipe, 0x22, 0x36);

		tp2850_write_register(ViPipe, 0x2b, 0x60);
		tp2850_write_register(ViPipe, 0x2d, 0xa0);
		tp2850_write_register(ViPipe, 0x2e, 0x40);

		tp2850_write_register(ViPipe, 0x30, 0x48);
		tp2850_write_register(ViPipe, 0x31, 0x6a);
		tp2850_write_register(ViPipe, 0x32, 0xbe);
		tp2850_write_register(ViPipe, 0x33, 0x80);

		tp2850_write_register(ViPipe, 0x39, 0x40);

		tp2850_write_register(ViPipe, 0x40, 0x08);
		tp2850_write_register(ViPipe, 0x23, 0x02);
		tp2850_write_register(ViPipe, 0x13, 0x04);
		tp2850_write_register(ViPipe, 0x14, 0x05);
		tp2850_write_register(ViPipe, 0x15, 0x04);

		tp2850_write_register(ViPipe, 0x25, 0x10);
		tp2850_write_register(ViPipe, 0x26, 0x06);
		tp2850_write_register(ViPipe, 0x27, 0x16);
		tp2850_write_register(ViPipe, 0x39, 0x01);

		tp2850_write_register(ViPipe, 0x10, 0x88);
		tp2850_write_register(ViPipe, 0x10, 0x08);
		tp2850_write_register(ViPipe, 0x23, 0x00);
		tp2850_write_register(ViPipe, 0x40, 0x00);
	}
}

void tp2850_init(VI_PIPE ViPipe)
{
	tp2850_i2c_init(ViPipe);

	syslog(LOG_DEBUG, "Loading Techpoint TP2850 sensor\n");

	// check sensor chip id
	tp2850_write_register(ViPipe, 0x40, 0x0);
	if (tp2850_read_register(ViPipe, 0xfe) != 0x28 ||
		tp2850_read_register(ViPipe, 0xff) != 0x50) {
		syslog(LOG_DEBUG, "read TP2850 chip id fail\n");
		return;
	}

	if (g_pastTP2850[ViPipe]->u8ImgMode == TP2850_MODE_1080P_30P)
		syslog(LOG_DEBUG, "Techpoint TP2850 1080\n");
	else
		syslog(LOG_DEBUG, "Techpoint TP2850 1440\n");

	tp2850_init_setting(ViPipe, g_pastTP2850[ViPipe]->u8ImgMode);
#if TP2850_BLUE_SCREEN
	tp2850_write_register(ViPipe, 0x40, 0x00);
	tp2850_write_register(ViPipe, 0x2A, 0x34);
#endif
}

void tp2850_exit(VI_PIPE ViPipe)
{
	if (g_tp2850_thid)
		pthread_kill(g_tp2850_thid, SIGQUIT);

	tp2850_i2c_exit(ViPipe);
}
