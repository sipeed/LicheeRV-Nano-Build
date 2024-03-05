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
#include "f53_cmos_ex.h"

#define F53_CHIP_ID_HI_ADDR		0x0A
#define F53_CHIP_ID_LO_ADDR		0x0B
#define F53_CHIP_ID			0x0842
static void f53_linear_1080p30_init(VI_PIPE ViPipe);

CVI_U8 f53_i2c_addr = 0x40;        /* I2C Address of F53 */
const CVI_U32 f53_addr_byte = 1;
const CVI_U32 f53_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int f53_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunF53_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, f53_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int f53_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int f53_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (f53_data_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, f53_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, f53_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (f53_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}


int f53_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (f53_addr_byte == 1) {
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (f53_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, f53_addr_byte + f53_data_byte);
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

void f53_standby(VI_PIPE ViPipe)
{
	f53_write_register(ViPipe, 0x12, 0x40);
}

void f53_restart(VI_PIPE ViPipe)
{
	f53_write_register(ViPipe, 0x12, 0x40);
	delay_ms(20);
	f53_write_register(ViPipe, 0x12, 0x00);
}

void f53_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastF53[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		f53_write_register(ViPipe,
				g_pastF53[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastF53[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void f53_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = f53_read_register(ViPipe, 0x12) & ~0x30;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		val |= 0x20;
		break;
	case ISP_SNS_FLIP:
		val |= 0x10;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val |= 0x30;
		break;
	default:
		return;
	}

	f53_write_register(ViPipe, 0x12, val);
}

int f53_probe(VI_PIPE ViPipe)
{
	int nVal;
	CVI_U16 chip_id;

	usleep(4*1000);
	if (f53_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;
	nVal = f53_read_register(ViPipe, F53_CHIP_ID_HI_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id = (nVal & 0xFF) << 8;
	nVal = f53_read_register(ViPipe, F53_CHIP_ID_LO_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id |= (nVal & 0xFF);
	if (chip_id != F53_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void f53_init(VI_PIPE ViPipe)
{
	CVI_BOOL          bInit;
	bInit       = g_pastF53[ViPipe]->bInit;
	f53_i2c_init(ViPipe);

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		f53_linear_1080p30_init(ViPipe);
		g_pastF53[ViPipe]->bInit = CVI_TRUE;
	}
	else {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor init finshed!!\n");
	}
}

void f53_exit(VI_PIPE ViPipe)
{
	f53_i2c_exit(ViPipe);
}

/* 1080P30 and 1080P25 */
static void f53_linear_1080p30_init(VI_PIPE ViPipe)
{
	delay_ms(80);
	f53_write_register(ViPipe, 0x12, 0x40);
	f53_write_register(ViPipe, 0x48, 0x8A);
	f53_write_register(ViPipe, 0x48, 0x0A);
	f53_write_register(ViPipe, 0x0E, 0x19);
	f53_write_register(ViPipe, 0x0F, 0x04);
	f53_write_register(ViPipe, 0x10, 0x20);
	f53_write_register(ViPipe, 0x11, 0x80);
	f53_write_register(ViPipe, 0x46, 0x09);
	f53_write_register(ViPipe, 0x47, 0x66);
	f53_write_register(ViPipe, 0x0D, 0xF2);
	f53_write_register(ViPipe, 0x57, 0x6A);
	f53_write_register(ViPipe, 0x58, 0x22);
	f53_write_register(ViPipe, 0x5F, 0x41);
	f53_write_register(ViPipe, 0x60, 0x24);
	f53_write_register(ViPipe, 0xA5, 0xC0);
	f53_write_register(ViPipe, 0x20, 0x00);
	f53_write_register(ViPipe, 0x21, 0x05);
	f53_write_register(ViPipe, 0x22, 0x65);
	f53_write_register(ViPipe, 0x23, 0x04);
	f53_write_register(ViPipe, 0x24, 0xC4);
	f53_write_register(ViPipe, 0x25, 0x40);
	f53_write_register(ViPipe, 0x26, 0x43);
	f53_write_register(ViPipe, 0x27, 0xC6);
	f53_write_register(ViPipe, 0x28, 0x11);
	f53_write_register(ViPipe, 0x29, 0x04);
	f53_write_register(ViPipe, 0x2A, 0xBB);
	f53_write_register(ViPipe, 0x2B, 0x14);
	f53_write_register(ViPipe, 0x2C, 0x00);
	f53_write_register(ViPipe, 0x2D, 0x00);
	f53_write_register(ViPipe, 0x2E, 0x16);
	f53_write_register(ViPipe, 0x2F, 0x04);
	f53_write_register(ViPipe, 0x41, 0xC9);
	f53_write_register(ViPipe, 0x42, 0x33);
	f53_write_register(ViPipe, 0x47, 0x46);
	f53_write_register(ViPipe, 0x76, 0x6A);
	f53_write_register(ViPipe, 0x77, 0x09);
	f53_write_register(ViPipe, 0x80, 0x01);
	f53_write_register(ViPipe, 0xAF, 0x22);
	f53_write_register(ViPipe, 0xAB, 0x00);
	f53_write_register(ViPipe, 0x1D, 0x00);
	f53_write_register(ViPipe, 0x1E, 0x04);
	f53_write_register(ViPipe, 0x6C, 0x40);
	f53_write_register(ViPipe, 0x9E, 0xF8);
	f53_write_register(ViPipe, 0x6E, 0x2C);
	f53_write_register(ViPipe, 0x70, 0x6C);
	f53_write_register(ViPipe, 0x71, 0x6D);
	f53_write_register(ViPipe, 0x72, 0x6A);
	f53_write_register(ViPipe, 0x73, 0x56);
	f53_write_register(ViPipe, 0x74, 0x02);
	f53_write_register(ViPipe, 0x78, 0x9D);
	f53_write_register(ViPipe, 0x89, 0x01);
	f53_write_register(ViPipe, 0x6B, 0x20);
	f53_write_register(ViPipe, 0x86, 0x40);
	f53_write_register(ViPipe, 0x31, 0x10);
	f53_write_register(ViPipe, 0x32, 0x18);
	f53_write_register(ViPipe, 0x33, 0xE8);
	f53_write_register(ViPipe, 0x34, 0x5E);
	f53_write_register(ViPipe, 0x35, 0x5E);
	f53_write_register(ViPipe, 0x3A, 0xAF);
	f53_write_register(ViPipe, 0x3B, 0x00);
	f53_write_register(ViPipe, 0x3C, 0xFF);
	f53_write_register(ViPipe, 0x3D, 0xFF);
	f53_write_register(ViPipe, 0x3E, 0xFF);
	f53_write_register(ViPipe, 0x3F, 0xBB);
	f53_write_register(ViPipe, 0x40, 0xFF);
	f53_write_register(ViPipe, 0x56, 0x92);
	f53_write_register(ViPipe, 0x59, 0xAF);
	f53_write_register(ViPipe, 0x5A, 0x47);
	f53_write_register(ViPipe, 0x61, 0x18);
	f53_write_register(ViPipe, 0x6F, 0x04);
	f53_write_register(ViPipe, 0x85, 0x5F);
	f53_write_register(ViPipe, 0x8A, 0x44);
	f53_write_register(ViPipe, 0x91, 0x13);
	f53_write_register(ViPipe, 0x94, 0xA0);
	f53_write_register(ViPipe, 0x9B, 0x83);
	f53_write_register(ViPipe, 0x9C, 0xE1);
	f53_write_register(ViPipe, 0xA4, 0x80);
	f53_write_register(ViPipe, 0xA6, 0x22);
	f53_write_register(ViPipe, 0xA9, 0x1C);
	f53_write_register(ViPipe, 0x5B, 0xE7);
	f53_write_register(ViPipe, 0x5C, 0x28);
	f53_write_register(ViPipe, 0x5D, 0x67);
	f53_write_register(ViPipe, 0x5E, 0x11);
	f53_write_register(ViPipe, 0x62, 0x21);
	f53_write_register(ViPipe, 0x63, 0x0F);
	f53_write_register(ViPipe, 0x64, 0xD0);
	f53_write_register(ViPipe, 0x65, 0x02);
	f53_write_register(ViPipe, 0x67, 0x49);
	f53_write_register(ViPipe, 0x66, 0x00);
	f53_write_register(ViPipe, 0x68, 0x00);
	f53_write_register(ViPipe, 0x69, 0x72);
	f53_write_register(ViPipe, 0x6A, 0x12);
	f53_write_register(ViPipe, 0x7A, 0x00);
	f53_write_register(ViPipe, 0x82, 0x20);
	f53_write_register(ViPipe, 0x8D, 0x47);
	f53_write_register(ViPipe, 0x8F, 0x90);
	f53_write_register(ViPipe, 0x45, 0x01);
	f53_write_register(ViPipe, 0x97, 0x20);
	f53_write_register(ViPipe, 0x13, 0x81);
	f53_write_register(ViPipe, 0x96, 0x84);
	f53_write_register(ViPipe, 0x4A, 0x01);
	f53_write_register(ViPipe, 0xB1, 0x00);
	f53_write_register(ViPipe, 0xA1, 0x0F);
	f53_write_register(ViPipe, 0xBE, 0x00);
	f53_write_register(ViPipe, 0x7E, 0x48);
	f53_write_register(ViPipe, 0xB5, 0xC0);
	f53_write_register(ViPipe, 0x50, 0x02);
	f53_write_register(ViPipe, 0x49, 0x10);
	f53_write_register(ViPipe, 0x7F, 0x57);
	f53_write_register(ViPipe, 0x90, 0x00);
	f53_write_register(ViPipe, 0x7B, 0x4A);
	f53_write_register(ViPipe, 0x7C, 0x07);
	f53_write_register(ViPipe, 0x8C, 0xFF);
	f53_write_register(ViPipe, 0x8E, 0x00);
	f53_write_register(ViPipe, 0x8B, 0x01);
	f53_write_register(ViPipe, 0x0C, 0x00);
	f53_write_register(ViPipe, 0xBC, 0x11);
	f53_write_register(ViPipe, 0x19, 0x20);
	f53_write_register(ViPipe, 0x1B, 0x4F);
	f53_write_register(ViPipe, 0x12, 0x00);
	f53_write_register(ViPipe, 0x00, 0x10);

	f53_default_reg_init(ViPipe);
	delay_ms(80);

	printf("ViPipe:%d,===F53 1080P 30fps 10bit LINE Init OK!===\n", ViPipe);
}
