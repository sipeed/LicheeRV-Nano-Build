#include <stdio.h>
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
#include "pico384_cmos_ex.h"

static void pico384_linear_1080p30_init(VI_PIPE ViPipe);

const CVI_U8 pico384_i2c_addr = 0x24;        /* I2C Address of PICO384 */
const CVI_U32 pico384_addr_byte = 2;
const CVI_U32 pico384_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int pico384_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunPICO384_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, pico384_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int pico384_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int pico384_read_register(VI_PIPE ViPipe, int addr)
{
	/* TODO:*/
	(void) ViPipe;
	(void) addr;

	return CVI_SUCCESS;
}

int pico384_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (pico384_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (pico384_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, pico384_addr_byte + pico384_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}


void pico384_init(VI_PIPE ViPipe)
{
	pico384_i2c_init(ViPipe);
	pico384_linear_1080p30_init(ViPipe);
	g_pastPICO384[ViPipe]->bInit = CVI_TRUE;
}

void pico384_exit(VI_PIPE ViPipe)
{
	pico384_i2c_exit(ViPipe);
}

/* 1080P30 and 1080P25 */
static void pico384_linear_1080p30_init(VI_PIPE ViPipe)
{
	pico384_write_register(ViPipe, 0x0041, 0x60);
	pico384_write_register(ViPipe, 0x005B, 0x33);
	pico384_write_register(ViPipe, 0x008B, 0x03);
	pico384_write_register(ViPipe, 0x0040, 0xD3);
	pico384_write_register(ViPipe, 0x004F, 0x01);
	pico384_write_register(ViPipe, 0x0050, 0x6F);
	pico384_write_register(ViPipe, 0x0056, 0x00);
	pico384_write_register(ViPipe, 0x0062, 0x00);
	pico384_write_register(ViPipe, 0x005D, 0x04);
	pico384_write_register(ViPipe, 0x005C, 0x03);
	pico384_write_register(ViPipe, 0x005C, 0x07);

	/* InterBoost */
	pico384_write_register(ViPipe, 0x005C, 0x06);
	pico384_write_register(ViPipe, 0x004B, 0xB1);
	pico384_write_register(ViPipe, 0x004C, 0x01);
	pico384_write_register(ViPipe, 0x004D, 0x46);
	pico384_write_register(ViPipe, 0x005B, 0x30);
	pico384_write_register(ViPipe, 0x005C, 0x07);

	delay_ms(100);

	pico384_write_register(ViPipe, 0x005B, 0x33);

	printf("ViPipe:%d,===PICO384 Init OK!===\n", ViPipe);
}

