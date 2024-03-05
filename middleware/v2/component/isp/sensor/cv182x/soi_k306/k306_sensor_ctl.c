#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#ifdef ARCH_CV182X
#include <linux/cvi_vip_snsr.h>
#include "cvi_comm_video.h"
#else
#include <linux/vi_snsr.h>
#include <linux/cvi_comm_video.h>
#endif
#include "cvi_sns_ctrl.h"
#include "k306_cmos_ex.h"

#define K306_CHIP_ID_HI_ADDR		0x0A
#define K306_CHIP_ID_LO_ADDR		0x0B
#define K306_CHIP_ID			0x0860

static void k306_linear_1440p25_init(VI_PIPE ViPipe);

CVI_U8 k306_i2c_addr = 0x40;        /* I2C Address of K306 */
const CVI_U32 k306_addr_byte = 1;
const CVI_U32 k306_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int k306_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunK306_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, k306_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int k306_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int k306_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, k306_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	ret = read(g_fd[ViPipe], buf, k306_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (k306_data_byte == 1) {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int k306_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (k306_addr_byte == 1) {
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (k306_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, k306_addr_byte + k306_data_byte);
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

void k306_standby(VI_PIPE ViPipe)
{
	k306_write_register(ViPipe, 0x12, 0x40);
}

void k306_restart(VI_PIPE ViPipe)
{
	k306_write_register(ViPipe, 0x12, 0x40);
	delay_ms(20);
	k306_write_register(ViPipe, 0x12, 0x00);
}

void k306_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastK306[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		k306_write_register(ViPipe,
				g_pastK306[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastK306[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void k306_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val1 = k306_read_register(ViPipe, 0x12);
	CVI_U8 val2 = k306_read_register(ViPipe, 0xAA);
	CVI_U8 val3 = k306_read_register(ViPipe, 0x27);

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		val1 = 0x30;
		val2 = 0x84;
		break;
	case ISP_SNS_MIRROR:
		val1 = 0x10;
		val2 = 0x8B;
		val3 += 1;
		break;
	case ISP_SNS_FLIP:
		val1 = 0x20;
		val2 = 0x84;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val1 = 0x00;
		val2 = 0x8B;
		val3 += 1;
		break;
	default:
		return;
	}

	k306_write_register(ViPipe, 0x12, val1);
	k306_write_register(ViPipe, 0xAA, val2);
	k306_write_register(ViPipe, 0x27, val3);
}

int k306_probe(VI_PIPE ViPipe)
{
	int nVal;
	CVI_U16 chip_id;

	usleep(4*1000);
	if (k306_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal = k306_read_register(ViPipe, K306_CHIP_ID_HI_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id = (nVal & 0xFF) << 8;
	nVal = k306_read_register(ViPipe, K306_CHIP_ID_LO_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id |= (nVal & 0xFF);

	if (chip_id != K306_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void k306_init(VI_PIPE ViPipe)
{
	k306_i2c_init(ViPipe);

	k306_linear_1440p25_init(ViPipe);
	g_pastK306[ViPipe]->bInit = CVI_TRUE;
}

void k306_exit(VI_PIPE ViPipe)
{
	k306_i2c_exit(ViPipe);
}

static void k306_linear_1440p25_init(VI_PIPE ViPipe)
{
	usleep(20*1000);
	k306_write_register(ViPipe, 0x12, 0x60);
	k306_write_register(ViPipe, 0x48, 0x86);
	k306_write_register(ViPipe, 0x48, 0x06);
	k306_write_register(ViPipe, 0x0E, 0x11);
	k306_write_register(ViPipe, 0x0F, 0x0C);
	k306_write_register(ViPipe, 0x10, 0x48);
	k306_write_register(ViPipe, 0x0C, 0x20);
	k306_write_register(ViPipe, 0x0D, 0xA0);
	k306_write_register(ViPipe, 0x57, 0x67);
	k306_write_register(ViPipe, 0x58, 0x1F);
	k306_write_register(ViPipe, 0x5F, 0x41);
	k306_write_register(ViPipe, 0x60, 0x20);
	k306_write_register(ViPipe, 0x20, 0xC0);
	k306_write_register(ViPipe, 0x21, 0x03);
	k306_write_register(ViPipe, 0x22, 0xDC);
	k306_write_register(ViPipe, 0x23, 0x05);
	k306_write_register(ViPipe, 0x24, 0x80);
	k306_write_register(ViPipe, 0x25, 0xA0);
	k306_write_register(ViPipe, 0x26, 0x52);
	k306_write_register(ViPipe, 0x27, 0xBC);
	k306_write_register(ViPipe, 0x28, 0x15);
	k306_write_register(ViPipe, 0x29, 0x03);
	k306_write_register(ViPipe, 0x2A, 0xB6);
	k306_write_register(ViPipe, 0x2B, 0x13);
	k306_write_register(ViPipe, 0x2C, 0x00);
	k306_write_register(ViPipe, 0x2D, 0x00);
	k306_write_register(ViPipe, 0x2E, 0x6E);
	k306_write_register(ViPipe, 0x2F, 0x04);
	k306_write_register(ViPipe, 0x41, 0x04);
	k306_write_register(ViPipe, 0x42, 0x05);
	k306_write_register(ViPipe, 0x47, 0x46);
	k306_write_register(ViPipe, 0x76, 0x80);
	k306_write_register(ViPipe, 0x77, 0x0C);
	k306_write_register(ViPipe, 0x80, 0x04);
	k306_write_register(ViPipe, 0xAF, 0x22);
	k306_write_register(ViPipe, 0x46, 0x08);
	k306_write_register(ViPipe, 0xAA, 0x80);
	k306_write_register(ViPipe, 0x1D, 0x00);
	k306_write_register(ViPipe, 0x1E, 0x04);
	k306_write_register(ViPipe, 0x6C, 0x40);
	k306_write_register(ViPipe, 0x9E, 0xB8);
	k306_write_register(ViPipe, 0x6F, 0x00);
	k306_write_register(ViPipe, 0x6E, 0x2C);
	k306_write_register(ViPipe, 0x70, 0xD9);
	k306_write_register(ViPipe, 0x71, 0xDD);
	k306_write_register(ViPipe, 0x72, 0xCC);
	k306_write_register(ViPipe, 0x73, 0x7A);
	k306_write_register(ViPipe, 0x74, 0x02);
	k306_write_register(ViPipe, 0x78, 0x1B);
	k306_write_register(ViPipe, 0x89, 0x01);
	k306_write_register(ViPipe, 0x6B, 0x20);
	k306_write_register(ViPipe, 0x86, 0x40);
	k306_write_register(ViPipe, 0xB0, 0x42);
	k306_write_register(ViPipe, 0xBF, 0x01);
	k306_write_register(ViPipe, 0x0A, 0xC3);
	k306_write_register(ViPipe, 0xBF, 0x00);
	k306_write_register(ViPipe, 0x7F, 0x46);
	k306_write_register(ViPipe, 0x30, 0x8D);
	k306_write_register(ViPipe, 0x31, 0x08);
	k306_write_register(ViPipe, 0x32, 0x20);
	k306_write_register(ViPipe, 0x33, 0x5C);
	k306_write_register(ViPipe, 0x34, 0x30);
	k306_write_register(ViPipe, 0x35, 0x30);
	k306_write_register(ViPipe, 0x3A, 0xB6);
	k306_write_register(ViPipe, 0x56, 0x92);
	k306_write_register(ViPipe, 0x59, 0x48);
	k306_write_register(ViPipe, 0x5A, 0x01);
	k306_write_register(ViPipe, 0x61, 0x00);
	k306_write_register(ViPipe, 0x64, 0xE0);
	k306_write_register(ViPipe, 0x85, 0x44);
	k306_write_register(ViPipe, 0x8A, 0x00);
	k306_write_register(ViPipe, 0x91, 0x40);
	k306_write_register(ViPipe, 0x94, 0xE0);
	k306_write_register(ViPipe, 0x9B, 0x8F);
	k306_write_register(ViPipe, 0xA6, 0x02);
	k306_write_register(ViPipe, 0xA7, 0xA0);
	k306_write_register(ViPipe, 0xA9, 0x48);
	k306_write_register(ViPipe, 0x45, 0x09);
	k306_write_register(ViPipe, 0x5B, 0xA5);
	k306_write_register(ViPipe, 0x5C, 0x8C);
	k306_write_register(ViPipe, 0x5D, 0x47);
	k306_write_register(ViPipe, 0x5E, 0xC8);
	k306_write_register(ViPipe, 0x65, 0x32);
	k306_write_register(ViPipe, 0x66, 0x80);
	k306_write_register(ViPipe, 0x67, 0x44);
	k306_write_register(ViPipe, 0x68, 0x00);
	k306_write_register(ViPipe, 0x69, 0x74);
	k306_write_register(ViPipe, 0x6A, 0x2B);
	k306_write_register(ViPipe, 0x7A, 0x82);
	k306_write_register(ViPipe, 0x8D, 0x6F);
	k306_write_register(ViPipe, 0x8F, 0x94);
	k306_write_register(ViPipe, 0xA4, 0xC7);
	k306_write_register(ViPipe, 0xA5, 0x0F);
	k306_write_register(ViPipe, 0xB7, 0x21);
	k306_write_register(ViPipe, 0x97, 0x20);
	k306_write_register(ViPipe, 0x13, 0x81);
	k306_write_register(ViPipe, 0x96, 0x84);
	k306_write_register(ViPipe, 0x4A, 0x01);
	k306_write_register(ViPipe, 0xB1, 0x00);
	k306_write_register(ViPipe, 0xA1, 0x0F);
	k306_write_register(ViPipe, 0xB5, 0xC4);
	k306_write_register(ViPipe, 0xA3, 0x40);
	k306_write_register(ViPipe, 0xBF, 0x01);
	k306_write_register(ViPipe, 0x03, 0x01);
	k306_write_register(ViPipe, 0x04, 0x80);
	k306_write_register(ViPipe, 0x05, 0x32);
	k306_write_register(ViPipe, 0xBF, 0x00);
	k306_write_register(ViPipe, 0x50, 0x02);
	k306_write_register(ViPipe, 0x49, 0x10);
	k306_write_register(ViPipe, 0x7E, 0x4C);
	k306_write_register(ViPipe, 0x8C, 0xFF);
	k306_write_register(ViPipe, 0x8E, 0x00);
	k306_write_register(ViPipe, 0x8B, 0x01);
	k306_write_register(ViPipe, 0xBC, 0x11);
	k306_write_register(ViPipe, 0x82, 0x00);
	k306_write_register(ViPipe, 0x19, 0x20);
	k306_write_register(ViPipe, 0x1B, 0x4F);
	k306_write_register(ViPipe, 0x12, 0x20);
	k306_write_register(ViPipe, 0x48, 0x86);
	k306_write_register(ViPipe, 0x48, 0x06);

	k306_default_reg_init(ViPipe);
	usleep(33*1000);

	printf("ViPipe:%d,===K306 1440P 30fps 10bit LINE Init OK!===\n", ViPipe);
}

