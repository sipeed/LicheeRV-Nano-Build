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
#include "f352_cmos_ex.h"

#define F352_CHIP_ID_HI_ADDR		0x0A
#define F352_CHIP_ID_LO_ADDR		0x0B
#define F352_CHIP_ID			0x0701
static void f352_wdr_1080p30_2to1_init(VI_PIPE ViPipe);
static void f352_linear_1080p30_init(VI_PIPE ViPipe);

CVI_U8 f352_i2c_addr = 0x40;        /* I2C Address of F352 */
const CVI_U32 f352_addr_byte = 1;
const CVI_U32 f352_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int f352_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunF352_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, f352_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int f352_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int f352_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (f352_data_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, f352_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, f352_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (f352_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}


int f352_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (f352_addr_byte == 1) {
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (f352_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, f352_addr_byte + f352_data_byte);
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

void f352_standby(VI_PIPE ViPipe)
{
	f352_write_register(ViPipe, 0x12, 0x40);
}

void f352_restart(VI_PIPE ViPipe)
{
	f352_write_register(ViPipe, 0x12, 0x40);
	delay_ms(20);
	f352_write_register(ViPipe, 0x12, 0x00);
}

void f352_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastF352[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		f352_write_register(ViPipe,
				g_pastF352[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastF352[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void f352_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = f352_read_register(ViPipe, 0x12) & ~0x30;

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

	f352_write_register(ViPipe, 0x12, val);
}

int f352_probe(VI_PIPE ViPipe)
{
	int nVal;
	CVI_U16 chip_id;

	usleep(4*1000);
	if (f352_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;
	nVal = f352_read_register(ViPipe, F352_CHIP_ID_HI_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id = (nVal & 0xFF) << 8;
	nVal = f352_read_register(ViPipe, F352_CHIP_ID_LO_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id |= (nVal & 0xFF);
	if (chip_id != F352_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void f352_init(VI_PIPE ViPipe)
{
	WDR_MODE_E       enWDRMode;
	CVI_BOOL          bInit;
	CVI_U8            u8ImgMode;

	bInit       = g_pastF352[ViPipe]->bInit;
	enWDRMode   = g_pastF352[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastF352[ViPipe]->u8ImgMode;

	f352_i2c_init(ViPipe);

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == F352_MODE_1080P30_WDR) {
				/* F352_MODE_1080P30_WDR */
				f352_wdr_1080p30_2to1_init(ViPipe);
			} else {
			}
		} else {
			f352_linear_1080p30_init(ViPipe);
		}
	}
	/* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
	else {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == F352_MODE_1080P30_WDR) {
				/* F352_MODE_1080P30_WDR */
				f352_wdr_1080p30_2to1_init(ViPipe);
			} else {
			}
		} else {
			f352_linear_1080p30_init(ViPipe);
		}
	}
	g_pastF352[ViPipe]->bInit = CVI_TRUE;
}

void f352_exit(VI_PIPE ViPipe)
{
	f352_i2c_exit(ViPipe);
}

/* 1080P30 and 1080P25 */
static void f352_linear_1080p30_init(VI_PIPE ViPipe)
{
	delay_ms(20);
		f352_write_register(ViPipe, 0x12, 0x40);
	f352_write_register(ViPipe, 0xAD, 0x01);
	f352_write_register(ViPipe, 0xAD, 0x00);
	f352_write_register(ViPipe, 0x0E, 0x11);
	f352_write_register(ViPipe, 0x0F, 0x04);
	f352_write_register(ViPipe, 0x10, 0x24);
	f352_write_register(ViPipe, 0x11, 0x80);
	f352_write_register(ViPipe, 0x0D, 0x00);
	f352_write_register(ViPipe, 0x64, 0x11);
	f352_write_register(ViPipe, 0x65, 0x98);
	f352_write_register(ViPipe, 0xBE, 0x30);
	f352_write_register(ViPipe, 0xBF, 0xC0);
	f352_write_register(ViPipe, 0xBC, 0xC9);
	f352_write_register(ViPipe, 0x20, 0x00);
	f352_write_register(ViPipe, 0x21, 0x05);
	f352_write_register(ViPipe, 0x22, 0x65);
	f352_write_register(ViPipe, 0x23, 0x04);
	f352_write_register(ViPipe, 0x24, 0xC0);
	f352_write_register(ViPipe, 0x25, 0x38);
	f352_write_register(ViPipe, 0x26, 0x43);
	f352_write_register(ViPipe, 0x27, 0x04);
	f352_write_register(ViPipe, 0x28, 0x0B);
	f352_write_register(ViPipe, 0x29, 0x00);
	f352_write_register(ViPipe, 0x2A, 0xFF);
	f352_write_register(ViPipe, 0x2B, 0x0F);
	f352_write_register(ViPipe, 0x2C, 0x01);
	f352_write_register(ViPipe, 0x2D, 0x04);
	f352_write_register(ViPipe, 0x2E, 0x13);
	f352_write_register(ViPipe, 0x2F, 0x34);
	f352_write_register(ViPipe, 0x30, 0xC4);
	f352_write_register(ViPipe, 0x87, 0xC6);
	f352_write_register(ViPipe, 0x9F, 0xCC);
	f352_write_register(ViPipe, 0x9D, 0xB0);
	f352_write_register(ViPipe, 0xAC, 0x00);
	f352_write_register(ViPipe, 0xA3, 0x00);
	f352_write_register(ViPipe, 0x88, 0x08);
	f352_write_register(ViPipe, 0x1D, 0x00);
	f352_write_register(ViPipe, 0x1E, 0x10);
	f352_write_register(ViPipe, 0x3A, 0x4F);
	f352_write_register(ViPipe, 0x3B, 0x6A);
	f352_write_register(ViPipe, 0x3C, 0x29);
	f352_write_register(ViPipe, 0x3D, 0x35);
	f352_write_register(ViPipe, 0x44, 0x1B);
	f352_write_register(ViPipe, 0x3E, 0x02);
	f352_write_register(ViPipe, 0x3F, 0x13);
	f352_write_register(ViPipe, 0x42, 0x04);
	f352_write_register(ViPipe, 0x43, 0x00);
	f352_write_register(ViPipe, 0x70, 0x00);
	f352_write_register(ViPipe, 0x71, 0x24);
	f352_write_register(ViPipe, 0x7E, 0x1C);
	f352_write_register(ViPipe, 0x33, 0x18);
	f352_write_register(ViPipe, 0xA9, 0x40);
	f352_write_register(ViPipe, 0xB0, 0x52);
	f352_write_register(ViPipe, 0xB1, 0x4B);
	f352_write_register(ViPipe, 0xB2, 0x4B);
	f352_write_register(ViPipe, 0xB3, 0x42);
	f352_write_register(ViPipe, 0xB5, 0x78);
	f352_write_register(ViPipe, 0xB6, 0x5F);
	f352_write_register(ViPipe, 0xB7, 0x65);
	f352_write_register(ViPipe, 0xB8, 0x30);
	f352_write_register(ViPipe, 0xB9, 0x2C);
	f352_write_register(ViPipe, 0xBA, 0xEF);
	f352_write_register(ViPipe, 0xDD, 0x0F);
	f352_write_register(ViPipe, 0xDE, 0xFF);
	f352_write_register(ViPipe, 0xF4, 0x91);
	f352_write_register(ViPipe, 0x35, 0xF0);
	f352_write_register(ViPipe, 0x39, 0xF0);
	f352_write_register(ViPipe, 0x56, 0x38);
	f352_write_register(ViPipe, 0x57, 0x68);
	f352_write_register(ViPipe, 0x58, 0xA1);
	f352_write_register(ViPipe, 0x59, 0x10);
	f352_write_register(ViPipe, 0x5A, 0x6C);
	f352_write_register(ViPipe, 0x5B, 0x3F);
	f352_write_register(ViPipe, 0x5C, 0xF1);
	f352_write_register(ViPipe, 0x5D, 0x28);
	f352_write_register(ViPipe, 0x5E, 0xA9);
	f352_write_register(ViPipe, 0x5F, 0x10);
	f352_write_register(ViPipe, 0x60, 0xB5);
	f352_write_register(ViPipe, 0x63, 0x80);
	f352_write_register(ViPipe, 0x67, 0xA4);
	f352_write_register(ViPipe, 0x68, 0xC2);
	f352_write_register(ViPipe, 0x69, 0x30);
	f352_write_register(ViPipe, 0x6C, 0x17);
	f352_write_register(ViPipe, 0x6D, 0x40);
	f352_write_register(ViPipe, 0xBB, 0x8F);
	f352_write_register(ViPipe, 0xC4, 0x00);
	f352_write_register(ViPipe, 0xCC, 0xC0);
	f352_write_register(ViPipe, 0xCE, 0x2A);
	f352_write_register(ViPipe, 0xE9, 0x60);
	f352_write_register(ViPipe, 0xEA, 0x21);
	f352_write_register(ViPipe, 0xE1, 0xE2);
	f352_write_register(ViPipe, 0x4A, 0x20);
	f352_write_register(ViPipe, 0x80, 0x05);
	f352_write_register(ViPipe, 0x81, 0x04);
	f352_write_register(ViPipe, 0x84, 0x84);
	f352_write_register(ViPipe, 0xA6, 0x0E);
	f352_write_register(ViPipe, 0x49, 0x10);
	f352_write_register(ViPipe, 0xF2, 0x08);
	f352_write_register(ViPipe, 0x85, 0x00);
	f352_write_register(ViPipe, 0xB4, 0x01);
	f352_write_register(ViPipe, 0xD2, 0x80);
	f352_write_register(ViPipe, 0xD0, 0x00);
	f352_write_register(ViPipe, 0xD3, 0x0B);
	f352_write_register(ViPipe, 0x38, 0x0A);
	f352_write_register(ViPipe, 0xC3, 0xF8);
	f352_write_register(ViPipe, 0x7D, 0x01);
	f352_write_register(ViPipe, 0x1F, 0x02);
	f352_write_register(ViPipe, 0x12, 0x00);

	f352_default_reg_init(ViPipe);
	delay_ms(80);

	printf("ViPipe:%d,===F352 1080P 30fps 10bit LINE Init OK!===\n", ViPipe);
}

static void f352_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
	delay_ms(10);
	f352_write_register(ViPipe, 0x12, 0x48);
	f352_write_register(ViPipe, 0xAD, 0x01);
	f352_write_register(ViPipe, 0xAD, 0x00);
	f352_write_register(ViPipe, 0x0E, 0x11);
	f352_write_register(ViPipe, 0x0F, 0x04);
	f352_write_register(ViPipe, 0x10, 0x48);
	f352_write_register(ViPipe, 0x11, 0x80);
	f352_write_register(ViPipe, 0x0D, 0x10);
	f352_write_register(ViPipe, 0x64, 0x11);
	f352_write_register(ViPipe, 0x65, 0x98);
	f352_write_register(ViPipe, 0xBE, 0x30);
	f352_write_register(ViPipe, 0xBF, 0xC0);
	f352_write_register(ViPipe, 0xBC, 0xC9);
	f352_write_register(ViPipe, 0x20, 0x80);
	f352_write_register(ViPipe, 0x21, 0x02);
	f352_write_register(ViPipe, 0x22, 0x65);
	f352_write_register(ViPipe, 0x23, 0x04);
	f352_write_register(ViPipe, 0x24, 0xC4);
	f352_write_register(ViPipe, 0x25, 0x40);
	f352_write_register(ViPipe, 0x26, 0x43);
	f352_write_register(ViPipe, 0x27, 0x00);
	f352_write_register(ViPipe, 0x28, 0x07);
	f352_write_register(ViPipe, 0x29, 0x00);
	f352_write_register(ViPipe, 0x2A, 0x7C);
	f352_write_register(ViPipe, 0x2B, 0x0E);
	f352_write_register(ViPipe, 0x2C, 0x00);
	f352_write_register(ViPipe, 0x2D, 0x04);
	f352_write_register(ViPipe, 0x2E, 0x13);
	f352_write_register(ViPipe, 0x2F, 0x34);
	f352_write_register(ViPipe, 0x30, 0xC5);
	f352_write_register(ViPipe, 0x87, 0xC6);
	f352_write_register(ViPipe, 0x9F, 0xCC);
	f352_write_register(ViPipe, 0x9D, 0xB0);
	f352_write_register(ViPipe, 0xAC, 0x00);
	f352_write_register(ViPipe, 0xA3, 0x00);
	f352_write_register(ViPipe, 0x88, 0x08);
	f352_write_register(ViPipe, 0x1D, 0x00);
	f352_write_register(ViPipe, 0x1E, 0x10);
	f352_write_register(ViPipe, 0x3A, 0xB7);
	f352_write_register(ViPipe, 0x3B, 0xBA);
	f352_write_register(ViPipe, 0x3C, 0x4C);
	f352_write_register(ViPipe, 0x3D, 0x59);
	f352_write_register(ViPipe, 0x44, 0x1B);
	f352_write_register(ViPipe, 0x3E, 0x0A);
	f352_write_register(ViPipe, 0x3F, 0x15);
	f352_write_register(ViPipe, 0x42, 0x06);
	f352_write_register(ViPipe, 0x43, 0x00);
	f352_write_register(ViPipe, 0x70, 0x00);
	f352_write_register(ViPipe, 0x71, 0x24);
	f352_write_register(ViPipe, 0x7E, 0x1C);
	f352_write_register(ViPipe, 0x33, 0x18);
	f352_write_register(ViPipe, 0xA9, 0x40);
	f352_write_register(ViPipe, 0xB0, 0x52);
	f352_write_register(ViPipe, 0xB1, 0x4B);
	f352_write_register(ViPipe, 0xB2, 0x4B);
	f352_write_register(ViPipe, 0xB3, 0x42);
	f352_write_register(ViPipe, 0xB5, 0x78);
	f352_write_register(ViPipe, 0xB6, 0x5F);
	f352_write_register(ViPipe, 0xB7, 0x65);
	f352_write_register(ViPipe, 0xB8, 0x30);
	f352_write_register(ViPipe, 0xB9, 0x2C);
	f352_write_register(ViPipe, 0xBA, 0xEF);
	f352_write_register(ViPipe, 0xDD, 0x0F);
	f352_write_register(ViPipe, 0xDE, 0xFF);
	f352_write_register(ViPipe, 0xF4, 0x91);
	f352_write_register(ViPipe, 0x35, 0xF0);
	f352_write_register(ViPipe, 0x39, 0xF0);
	f352_write_register(ViPipe, 0x56, 0x38);
	f352_write_register(ViPipe, 0x57, 0x68);
	f352_write_register(ViPipe, 0x58, 0xA1);
	f352_write_register(ViPipe, 0x59, 0x10);
	f352_write_register(ViPipe, 0x5A, 0x6C);
	f352_write_register(ViPipe, 0x5B, 0x3F);
	f352_write_register(ViPipe, 0x5C, 0xB1);
	f352_write_register(ViPipe, 0x5D, 0x28);
	f352_write_register(ViPipe, 0x5E, 0xA9);
	f352_write_register(ViPipe, 0x5F, 0x10);
	f352_write_register(ViPipe, 0x60, 0xB5);
	f352_write_register(ViPipe, 0x63, 0x80);
	f352_write_register(ViPipe, 0x67, 0xAD);
	f352_write_register(ViPipe, 0x68, 0xC2);
	f352_write_register(ViPipe, 0x69, 0x30);
	f352_write_register(ViPipe, 0x6C, 0x17);
	f352_write_register(ViPipe, 0x6D, 0x40);
	f352_write_register(ViPipe, 0xBB, 0x8F);
	f352_write_register(ViPipe, 0xC4, 0x00);
	f352_write_register(ViPipe, 0xCC, 0xC0);
	f352_write_register(ViPipe, 0xCE, 0x2A);
	f352_write_register(ViPipe, 0xE9, 0x60);
	f352_write_register(ViPipe, 0xEA, 0x21);
	f352_write_register(ViPipe, 0xE1, 0xE2);
	f352_write_register(ViPipe, 0x4A, 0x20);
	f352_write_register(ViPipe, 0x80, 0x05);
	f352_write_register(ViPipe, 0x81, 0x04);
	f352_write_register(ViPipe, 0x84, 0x84);
	f352_write_register(ViPipe, 0xA6, 0x0E);
	f352_write_register(ViPipe, 0x49, 0x10);
	f352_write_register(ViPipe, 0xF2, 0x08);
	f352_write_register(ViPipe, 0x85, 0x00);
	f352_write_register(ViPipe, 0xB4, 0x01);
	f352_write_register(ViPipe, 0xD2, 0x80);
	f352_write_register(ViPipe, 0xD0, 0x00);
	f352_write_register(ViPipe, 0xD3, 0x0B);
	f352_write_register(ViPipe, 0x38, 0x0A);
	f352_write_register(ViPipe, 0xC3, 0xF8);
	f352_write_register(ViPipe, 0x7D, 0x05);
	f352_write_register(ViPipe, 0x1F, 0x02);
	f352_write_register(ViPipe, 0x1B, 0x17);
	f352_write_register(ViPipe, 0x06, 0x12);
	f352_write_register(ViPipe, 0x7C, 0x00);
	f352_write_register(ViPipe, 0xA4, 0x11);
	f352_write_register(ViPipe, 0x03, 0xFF);
	f352_write_register(ViPipe, 0x04, 0xFF);
	f352_write_register(ViPipe, 0x37, 0x44);
	f352_write_register(ViPipe, 0x12, 0x08);

	f352_default_reg_init(ViPipe);

	delay_ms(33);
	printf("===F352 sensor 1080P30fps 12bit 2to1 WDR(60fps->30fps) init success!=====\n");
}
