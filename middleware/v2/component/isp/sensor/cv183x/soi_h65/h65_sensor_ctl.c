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
#include "h65_cmos_ex.h"

static void h65_wdr_720p30_2to1_init(VI_PIPE ViPipe);
static void h65_linear_720p30_init(VI_PIPE ViPipe);

CVI_U8 h65_i2c_addr = 0x30;        /* I2C Address of H65 */
const CVI_U32 h65_addr_byte = 1;
const CVI_U32 h65_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int h65_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunH65_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}
	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, h65_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}
	return CVI_SUCCESS;
}

int h65_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int h65_read_register(VI_PIPE ViPipe, int addr)
{
	/* TODO:*/
	(void) ViPipe;
	(void) addr;

	return CVI_SUCCESS;
}

int h65_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (h65_addr_byte == 1) {
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (h65_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, h65_addr_byte + h65_data_byte);
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

void h65_standby(VI_PIPE ViPipe)
{
	h65_write_register(ViPipe, 0x12, 0x40);
}

void h65_restart(VI_PIPE ViPipe)
{
	h65_write_register(ViPipe, 0x12, 0x40);
	delay_ms(20);
	h65_write_register(ViPipe, 0x12, 0x00);
}

void h65_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastH65[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		h65_write_register(ViPipe,
				g_pastH65[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastH65[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void h65_init(VI_PIPE ViPipe)
{
	WDR_MODE_E       enWDRMode;
	CVI_BOOL          bInit;
	CVI_U8            u8ImgMode;

	bInit       = g_pastH65[ViPipe]->bInit;
	enWDRMode   = g_pastH65[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastH65[ViPipe]->u8ImgMode;

	h65_i2c_init(ViPipe);

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == H65_MODE_720P30_WDR) {
				/* H65_MODE_720P30_WDR */
				h65_wdr_720p30_2to1_init(ViPipe);
			}
		} else {
			h65_linear_720p30_init(ViPipe);
		}
	} else { /* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == H65_MODE_720P30_WDR) {
				/* H65_MODE_720P30_WDR */
				h65_wdr_720p30_2to1_init(ViPipe);
			}
		} else {
			h65_linear_720p30_init(ViPipe);
		}
	}
	g_pastH65[ViPipe]->bInit = CVI_TRUE;
}

void h65_exit(VI_PIPE ViPipe)
{
	h65_i2c_exit(ViPipe);
}

/* 720P30 and 1080P25 */
static void h65_linear_720p30_init(VI_PIPE ViPipe)
{
	h65_write_register(ViPipe, 0x12, 0x40);
	h65_write_register(ViPipe, 0x0E, 0x11);
	h65_write_register(ViPipe, 0x0F, 0x04);
	h65_write_register(ViPipe, 0x10, 0x20);
	h65_write_register(ViPipe, 0x11, 0x80);
	h65_write_register(ViPipe, 0x5F, 0x01);
	h65_write_register(ViPipe, 0x60, 0x0f);
	h65_write_register(ViPipe, 0x19, 0x64);
	h65_write_register(ViPipe, 0x48, 0x25);
	h65_write_register(ViPipe, 0x20, 0xD0);
	h65_write_register(ViPipe, 0x21, 0x02);
	h65_write_register(ViPipe, 0x22, 0xE8);
	h65_write_register(ViPipe, 0x23, 0x03);
	h65_write_register(ViPipe, 0x24, 0x80);
	h65_write_register(ViPipe, 0x25, 0xd0);
	h65_write_register(ViPipe, 0x26, 0x22);
	h65_write_register(ViPipe, 0x27, 0x5C);
	h65_write_register(ViPipe, 0x28, 0x1a);
	h65_write_register(ViPipe, 0x29, 0x01);
	h65_write_register(ViPipe, 0x2A, 0x48);
	h65_write_register(ViPipe, 0x2B, 0x25);
	h65_write_register(ViPipe, 0x2C, 0x00);
	h65_write_register(ViPipe, 0x2D, 0x1f);
	h65_write_register(ViPipe, 0x2E, 0xF9);
	h65_write_register(ViPipe, 0x2F, 0x40);
	h65_write_register(ViPipe, 0x41, 0x90);
	h65_write_register(ViPipe, 0x42, 0x12);
	h65_write_register(ViPipe, 0x39, 0x90);
	h65_write_register(ViPipe, 0x1D, 0x00);
	h65_write_register(ViPipe, 0x1E, 0x00);
	h65_write_register(ViPipe, 0x6C, 0x40);
	h65_write_register(ViPipe, 0x70, 0x89);
	h65_write_register(ViPipe, 0x71, 0x8A);
	h65_write_register(ViPipe, 0x72, 0x68);
	h65_write_register(ViPipe, 0x73, 0x73);
	h65_write_register(ViPipe, 0x74, 0x52);
	h65_write_register(ViPipe, 0x75, 0x2B);
	h65_write_register(ViPipe, 0x76, 0x40);
	h65_write_register(ViPipe, 0x77, 0x06);
	h65_write_register(ViPipe, 0x78, 0x0E);
	h65_write_register(ViPipe, 0x6E, 0x2C);
	h65_write_register(ViPipe, 0x1F, 0x10);
	h65_write_register(ViPipe, 0x31, 0x0C);
	h65_write_register(ViPipe, 0x32, 0x20);
	h65_write_register(ViPipe, 0x33, 0x0C);
	h65_write_register(ViPipe, 0x34, 0x4F);
	h65_write_register(ViPipe, 0x36, 0x06);
	h65_write_register(ViPipe, 0x38, 0x39);
	h65_write_register(ViPipe, 0x3A, 0x08);
	h65_write_register(ViPipe, 0x3B, 0x50);
	h65_write_register(ViPipe, 0x3C, 0xA0);
	h65_write_register(ViPipe, 0x3D, 0x00);
	h65_write_register(ViPipe, 0x3E, 0x01);
	h65_write_register(ViPipe, 0x3F, 0x00);
	h65_write_register(ViPipe, 0x40, 0x00);
	h65_write_register(ViPipe, 0x0D, 0x50);
	h65_write_register(ViPipe, 0x5A, 0x43);
	h65_write_register(ViPipe, 0x5B, 0xB3);
	h65_write_register(ViPipe, 0x5C, 0x0C);
	h65_write_register(ViPipe, 0x5D, 0x7E);
	h65_write_register(ViPipe, 0x5E, 0x24);
	h65_write_register(ViPipe, 0x62, 0x40);
	h65_write_register(ViPipe, 0x67, 0x48);
	h65_write_register(ViPipe, 0x6A, 0x11);
	h65_write_register(ViPipe, 0x68, 0x00);
	h65_write_register(ViPipe, 0x8F, 0x9F);
	h65_write_register(ViPipe, 0x0C, 0x00);
	h65_write_register(ViPipe, 0x59, 0x97);
	h65_write_register(ViPipe, 0x4A, 0x05);
	h65_write_register(ViPipe, 0x50, 0x03);
	h65_write_register(ViPipe, 0x47, 0x62);
	h65_write_register(ViPipe, 0x7E, 0xCD);
	h65_write_register(ViPipe, 0x8D, 0x87);
	h65_write_register(ViPipe, 0x49, 0x10);
	h65_write_register(ViPipe, 0x7F, 0x52);
	h65_write_register(ViPipe, 0x8E, 0x00);
	h65_write_register(ViPipe, 0x8C, 0xFF);
	h65_write_register(ViPipe, 0x8B, 0x01);
	h65_write_register(ViPipe, 0x57, 0x02);
	h65_write_register(ViPipe, 0x94, 0x00);
	h65_write_register(ViPipe, 0x95, 0x00);
	h65_write_register(ViPipe, 0x63, 0x80);
	h65_write_register(ViPipe, 0x7B, 0x46);
	h65_write_register(ViPipe, 0x7C, 0x2D);
	h65_write_register(ViPipe, 0x90, 0x00);
	h65_write_register(ViPipe, 0x79, 0x00);
	h65_write_register(ViPipe, 0x13, 0x81);
	h65_write_register(ViPipe, 0x12, 0x00);
	h65_write_register(ViPipe, 0x45, 0x89);
	h65_write_register(ViPipe, 0x93, 0x68);
	delay_ms(500);
	h65_write_register(ViPipe, 0x45, 0x19);
	h65_write_register(ViPipe, 0x1F, 0x11);

	h65_default_reg_init(ViPipe);

	printf("ViPipe:%d,===H65 720P 30fps 10bit LINE Init OK!===\n", ViPipe);
}

static void h65_wdr_720p30_2to1_init(VI_PIPE ViPipe)
{
	h65_write_register(ViPipe, 0x12, 0x40);
	h65_write_register(ViPipe, 0x0E, 0x11);
	h65_write_register(ViPipe, 0x0F, 0x14);
	h65_write_register(ViPipe, 0x10, 0x24);
	h65_write_register(ViPipe, 0x11, 0x80);
	h65_write_register(ViPipe, 0x0D, 0xA0);
	h65_write_register(ViPipe, 0x5F, 0x41);
	h65_write_register(ViPipe, 0x60, 0x1E);
	h65_write_register(ViPipe, 0x58, 0x12);
	h65_write_register(ViPipe, 0x57, 0x60);
	h65_write_register(ViPipe, 0x9D, 0x00);
	h65_write_register(ViPipe, 0x20, 0x00);
	h65_write_register(ViPipe, 0x21, 0x05);
	h65_write_register(ViPipe, 0x22, 0x65);
	h65_write_register(ViPipe, 0x23, 0x04);
	h65_write_register(ViPipe, 0x24, 0xC0);
	h65_write_register(ViPipe, 0x25, 0x38);
	h65_write_register(ViPipe, 0x26, 0x43);
	h65_write_register(ViPipe, 0x27, 0x47);
	h65_write_register(ViPipe, 0x28, 0x19);
	h65_write_register(ViPipe, 0x29, 0x04);
	h65_write_register(ViPipe, 0x2C, 0x00);
	h65_write_register(ViPipe, 0x2D, 0x00);
	h65_write_register(ViPipe, 0x2E, 0x18);
	h65_write_register(ViPipe, 0x2F, 0x44);
	h65_write_register(ViPipe, 0x41, 0xC8);
	h65_write_register(ViPipe, 0x42, 0x13);
	h65_write_register(ViPipe, 0x46, 0x00);
	h65_write_register(ViPipe, 0x76, 0x60);
	h65_write_register(ViPipe, 0x77, 0x09);
	h65_write_register(ViPipe, 0x1D, 0x00);
	h65_write_register(ViPipe, 0x1E, 0x04);
	h65_write_register(ViPipe, 0x6C, 0x40);
	h65_write_register(ViPipe, 0x68, 0x00);
	h65_write_register(ViPipe, 0x6E, 0x2C);
	h65_write_register(ViPipe, 0x70, 0x6C);
	h65_write_register(ViPipe, 0x71, 0x6D);
	h65_write_register(ViPipe, 0x72, 0x6A);
	h65_write_register(ViPipe, 0x73, 0x36);
	h65_write_register(ViPipe, 0x74, 0x02);
	h65_write_register(ViPipe, 0x78, 0x9E);
	h65_write_register(ViPipe, 0x89, 0x01);
	h65_write_register(ViPipe, 0x2A, 0x38);
	h65_write_register(ViPipe, 0x2B, 0x24);
	h65_write_register(ViPipe, 0x31, 0x08);
	h65_write_register(ViPipe, 0x32, 0x4F);
	h65_write_register(ViPipe, 0x33, 0x20);
	h65_write_register(ViPipe, 0x34, 0x5E);
	h65_write_register(ViPipe, 0x35, 0x5E);
	h65_write_register(ViPipe, 0x3A, 0xA0);
	h65_write_register(ViPipe, 0x59, 0x87);
	h65_write_register(ViPipe, 0x5A, 0x04);
	h65_write_register(ViPipe, 0x8A, 0x04);
	h65_write_register(ViPipe, 0x91, 0x13);
	h65_write_register(ViPipe, 0x5B, 0xA0);
	h65_write_register(ViPipe, 0x5C, 0xF0);
	h65_write_register(ViPipe, 0x5D, 0xF4);
	h65_write_register(ViPipe, 0x5E, 0x1F);
	h65_write_register(ViPipe, 0x62, 0x04);
	h65_write_register(ViPipe, 0x63, 0x0F);
	h65_write_register(ViPipe, 0x64, 0xC0);
	h65_write_register(ViPipe, 0x66, 0x04);
	h65_write_register(ViPipe, 0x67, 0x73);
	h65_write_register(ViPipe, 0x69, 0x7C);
	h65_write_register(ViPipe, 0x7A, 0xC0);
	h65_write_register(ViPipe, 0x4A, 0x05);
	h65_write_register(ViPipe, 0x7E, 0xCD);
	h65_write_register(ViPipe, 0x49, 0x10);
	h65_write_register(ViPipe, 0x50, 0x03);
	h65_write_register(ViPipe, 0x7B, 0x4A);
	h65_write_register(ViPipe, 0x7C, 0x0C);
	h65_write_register(ViPipe, 0x7F, 0x57);
	h65_write_register(ViPipe, 0x8F, 0x80);
	h65_write_register(ViPipe, 0x90, 0x00);
	h65_write_register(ViPipe, 0x8E, 0x00);
	h65_write_register(ViPipe, 0x8C, 0xFF);
	h65_write_register(ViPipe, 0x8D, 0xC7);
	h65_write_register(ViPipe, 0x8B, 0x01);
	h65_write_register(ViPipe, 0x0C, 0x00);
	h65_write_register(ViPipe, 0x6A, 0x4D);
	h65_write_register(ViPipe, 0x65, 0x07);
	h65_write_register(ViPipe, 0x80, 0x02);
	h65_write_register(ViPipe, 0x81, 0xC0);
	h65_write_register(ViPipe, 0x19, 0x20);
	h65_write_register(ViPipe, 0x12, 0x00);
	h65_write_register(ViPipe, 0x48, 0x8A);
	h65_write_register(ViPipe, 0x48, 0x0A);

	h65_default_reg_init(ViPipe);

	if (g_au16H65_GainMode[ViPipe] == SNS_GAIN_MODE_ONLY_LEF) {
		h65_write_register(ViPipe, 0x46, 0x01);
	} else {
		h65_write_register(ViPipe, 0x46, 0x05);
	}

	if (g_au16H65_L2SMode[ViPipe] == SNS_L2S_MODE_AUTO) {
		h65_write_register(ViPipe, 0x8F, 0x80);
	} else {
		h65_write_register(ViPipe, 0x8F, 0x81);
	}
	delay_ms(33);
	printf("===H65 sensor 720P30fps 10bit 2to1 WDR(60fps->30fps) init success!=====\n");
}
