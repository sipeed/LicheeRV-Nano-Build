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
#include "os02k10_slave_cmos_ex.h"

static void os02k10_slave_linear_1080p30_init(VI_PIPE ViPipe);

CVI_U8 os02k10_slave_i2c_addr = 0x36;        /* I2C Address of OS02K10_SLAVE */
const CVI_U32 os02k10_slave_addr_byte = 2;
const CVI_U32 os02k10_slave_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int os02k10_slave_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunOs02k10_Slave_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, os02k10_slave_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int os02k10_slave_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int os02k10_slave_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (os02k10_slave_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, os02k10_slave_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, os02k10_slave_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (os02k10_slave_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);

	return data;
}

int os02k10_slave_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (os02k10_slave_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (os02k10_slave_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, os02k10_slave_addr_byte + os02k10_slave_data_byte);
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


void os02k10_slave_standby(VI_PIPE ViPipe)
{
	os02k10_slave_write_register(ViPipe, 0x0100, 0x00); /* STANDBY */
}

void os02k10_slave_restart(VI_PIPE ViPipe)
{
	os02k10_slave_write_register(ViPipe, 0x0100, 0x01); /* standby */
}

void os02k10_slave_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;
	CVI_U32 start = 1;
	CVI_U32 end = g_pastOs02k10_Slave[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum - 3;

	for (i = start; i < end; i++) {
		os02k10_slave_write_register(ViPipe,
				g_pastOs02k10_Slave[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastOs02k10_Slave[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

//bit[3:2] = 2'b11 flip bit[1] = 1'b1 mirror
void os02k10_slave_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = 0;

	val = os02k10_slave_read_register(ViPipe, 0x3820);
	val &= ~(0x7 << 1);

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		val |= 0x1<<1;
		break;
	case ISP_SNS_FLIP:
		val |= 0x3<<2;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val |= 0x1<<1;
		val |= 0x3<<2;
		break;
	default:
		return;
	}

	os02k10_slave_standby(ViPipe);
	os02k10_slave_write_register(ViPipe, 0x3820, val);
	delay_ms(100);
	os02k10_slave_restart(ViPipe);
}

#define OS02K10_CHIP_ID_ADDR_H		0x300A
#define OS02K10_CHIP_ID_ADDR_M		0x300B
#define OS02K10_CHIP_ID_ADDR_L		0x300C
#define OS02K10_CHIP_ID			0x530243

int os02k10_slave_probe(VI_PIPE ViPipe)
{
	int nVal, nVal2, nVal3;

	usleep(1000);
	if (os02k10_slave_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = os02k10_slave_read_register(ViPipe, OS02K10_CHIP_ID_ADDR_H);
	nVal2 = os02k10_slave_read_register(ViPipe, OS02K10_CHIP_ID_ADDR_M);
	nVal3 = os02k10_slave_read_register(ViPipe, OS02K10_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0 || nVal3 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 16) | ((nVal2 & 0xFF) << 8) | (nVal3 & 0xFF)) != OS02K10_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

void os02k10_slave_init(VI_PIPE ViPipe)
{
	os02k10_slave_i2c_init(ViPipe);

	delay_ms(10);

	os02k10_slave_linear_1080p30_init(ViPipe);

	g_pastOs02k10_Slave[ViPipe]->bInit = CVI_TRUE;
}

void os02k10_slave_exit(VI_PIPE ViPipe)
{
	os02k10_slave_i2c_exit(ViPipe);
}

/* 1080P30 */
static void os02k10_slave_linear_1080p30_init(VI_PIPE ViPipe)
{
	os02k10_slave_write_register(ViPipe, 0x302a, 0x00);
	delay_ms(50);

	os02k10_slave_write_register(ViPipe, 0x0103, 0x01);
	os02k10_slave_write_register(ViPipe, 0x0109, 0x01);
	os02k10_slave_write_register(ViPipe, 0x0104, 0x02);

	os02k10_slave_write_register(ViPipe, 0x0102, 0x00);
	os02k10_slave_write_register(ViPipe, 0x0303, 0x04);
	os02k10_slave_write_register(ViPipe, 0x0305, 0x50);
	os02k10_slave_write_register(ViPipe, 0x0306, 0x00);
	os02k10_slave_write_register(ViPipe, 0x0307, 0x00);
	os02k10_slave_write_register(ViPipe, 0x030a, 0x01);
	os02k10_slave_write_register(ViPipe, 0x0317, 0x0a);
	os02k10_slave_write_register(ViPipe, 0x0323, 0x04);
	os02k10_slave_write_register(ViPipe, 0x0324, 0x00);
	os02k10_slave_write_register(ViPipe, 0x0325, 0x90);
	os02k10_slave_write_register(ViPipe, 0x0327, 0x07);
	os02k10_slave_write_register(ViPipe, 0x032c, 0x02);
	os02k10_slave_write_register(ViPipe, 0x032d, 0x02);
	os02k10_slave_write_register(ViPipe, 0x032e, 0x05);
	os02k10_slave_write_register(ViPipe, 0x300f, 0x11);
	os02k10_slave_write_register(ViPipe, 0x3012, 0x21);
	os02k10_slave_write_register(ViPipe, 0x3026, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3027, 0x08);
	os02k10_slave_write_register(ViPipe, 0x302d, 0x24);
	os02k10_slave_write_register(ViPipe, 0x3103, 0x29);
	os02k10_slave_write_register(ViPipe, 0x3106, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3400, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3406, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3408, 0x05);
	os02k10_slave_write_register(ViPipe, 0x3409, 0x22);
	os02k10_slave_write_register(ViPipe, 0x340c, 0x05);
	os02k10_slave_write_register(ViPipe, 0x3425, 0x51);
	os02k10_slave_write_register(ViPipe, 0x3426, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3427, 0x14);
	os02k10_slave_write_register(ViPipe, 0x3428, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3429, 0x10);
	os02k10_slave_write_register(ViPipe, 0x342a, 0x10);
	os02k10_slave_write_register(ViPipe, 0x342b, 0x04);
	os02k10_slave_write_register(ViPipe, 0x3504, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3508, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3509, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3544, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3548, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3549, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3584, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3588, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3589, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3601, 0x70);
	os02k10_slave_write_register(ViPipe, 0x3604, 0xe3);
	os02k10_slave_write_register(ViPipe, 0x3605, 0x7f);
	os02k10_slave_write_register(ViPipe, 0x3606, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3608, 0xa8);
	os02k10_slave_write_register(ViPipe, 0x360a, 0xd0);
	os02k10_slave_write_register(ViPipe, 0x360b, 0x08);
	os02k10_slave_write_register(ViPipe, 0x360e, 0xc8);
	os02k10_slave_write_register(ViPipe, 0x360f, 0x66);
	os02k10_slave_write_register(ViPipe, 0x3610, 0x81);
	os02k10_slave_write_register(ViPipe, 0x3611, 0x89);
	os02k10_slave_write_register(ViPipe, 0x3612, 0x4e);
	os02k10_slave_write_register(ViPipe, 0x3613, 0xbd);
	os02k10_slave_write_register(ViPipe, 0x362a, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x362b, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x362c, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x362d, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x362e, 0x0c);
	os02k10_slave_write_register(ViPipe, 0x362f, 0x1a);
	os02k10_slave_write_register(ViPipe, 0x3630, 0x32);
	os02k10_slave_write_register(ViPipe, 0x3631, 0x64);
	os02k10_slave_write_register(ViPipe, 0x3638, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3643, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3644, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3645, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3646, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3647, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3648, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3649, 0x00);
	os02k10_slave_write_register(ViPipe, 0x364a, 0x04);
	os02k10_slave_write_register(ViPipe, 0x364c, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x364d, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x364e, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x364f, 0x0e);
	os02k10_slave_write_register(ViPipe, 0x3650, 0xff);
	os02k10_slave_write_register(ViPipe, 0x3651, 0xff);
	os02k10_slave_write_register(ViPipe, 0x3661, 0x07);
	os02k10_slave_write_register(ViPipe, 0x3662, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3663, 0x20);
	os02k10_slave_write_register(ViPipe, 0x3665, 0x12);
	os02k10_slave_write_register(ViPipe, 0x3667, 0xd4);
	os02k10_slave_write_register(ViPipe, 0x3668, 0x80);
	os02k10_slave_write_register(ViPipe, 0x366f, 0xc4);
	os02k10_slave_write_register(ViPipe, 0x3670, 0xc7);
	os02k10_slave_write_register(ViPipe, 0x3671, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3673, 0x6a);
	os02k10_slave_write_register(ViPipe, 0x3681, 0x80);
	os02k10_slave_write_register(ViPipe, 0x3700, 0x26);
	os02k10_slave_write_register(ViPipe, 0x3701, 0x1e);
	os02k10_slave_write_register(ViPipe, 0x3702, 0x25);
	os02k10_slave_write_register(ViPipe, 0x3703, 0x28);
	os02k10_slave_write_register(ViPipe, 0x3706, 0x3e);
	os02k10_slave_write_register(ViPipe, 0x3707, 0x0a);
	os02k10_slave_write_register(ViPipe, 0x3708, 0x36);
	os02k10_slave_write_register(ViPipe, 0x3709, 0x55);
	os02k10_slave_write_register(ViPipe, 0x370a, 0x00);
	os02k10_slave_write_register(ViPipe, 0x370b, 0xa3);
	os02k10_slave_write_register(ViPipe, 0x3714, 0x01);
	os02k10_slave_write_register(ViPipe, 0x371b, 0x16);
	os02k10_slave_write_register(ViPipe, 0x371c, 0x00);
	os02k10_slave_write_register(ViPipe, 0x371d, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3756, 0x9b);
	os02k10_slave_write_register(ViPipe, 0x3757, 0x9b);
	os02k10_slave_write_register(ViPipe, 0x3762, 0x1d);
	os02k10_slave_write_register(ViPipe, 0x376c, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3776, 0x05);
	os02k10_slave_write_register(ViPipe, 0x3777, 0x22);
	os02k10_slave_write_register(ViPipe, 0x3779, 0x60);
	os02k10_slave_write_register(ViPipe, 0x377c, 0x48);
	os02k10_slave_write_register(ViPipe, 0x3783, 0x02);
	os02k10_slave_write_register(ViPipe, 0x3784, 0x06);
	os02k10_slave_write_register(ViPipe, 0x3785, 0x0a);
	os02k10_slave_write_register(ViPipe, 0x3790, 0x10);
	os02k10_slave_write_register(ViPipe, 0x3793, 0x04);
	os02k10_slave_write_register(ViPipe, 0x3794, 0x07);
	os02k10_slave_write_register(ViPipe, 0x3796, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3797, 0x02);
	os02k10_slave_write_register(ViPipe, 0x379c, 0x4d);
	os02k10_slave_write_register(ViPipe, 0x37a1, 0x80);
	os02k10_slave_write_register(ViPipe, 0x37bb, 0x88);
	os02k10_slave_write_register(ViPipe, 0x37be, 0x01);
	os02k10_slave_write_register(ViPipe, 0x37bf, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37c0, 0x01);
	os02k10_slave_write_register(ViPipe, 0x37c7, 0x56);
	os02k10_slave_write_register(ViPipe, 0x37ca, 0x21);
	os02k10_slave_write_register(ViPipe, 0x37cc, 0x13);
	os02k10_slave_write_register(ViPipe, 0x37cd, 0x90);
	os02k10_slave_write_register(ViPipe, 0x37cf, 0x02);
	os02k10_slave_write_register(ViPipe, 0x37d1, 0x3e);
	os02k10_slave_write_register(ViPipe, 0x37d2, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37d3, 0xa3);
	os02k10_slave_write_register(ViPipe, 0x37d5, 0x3e);
	os02k10_slave_write_register(ViPipe, 0x37d6, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37d7, 0xa3);
	os02k10_slave_write_register(ViPipe, 0x37d8, 0x01);
	os02k10_slave_write_register(ViPipe, 0x37da, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37db, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37dc, 0x00);
	os02k10_slave_write_register(ViPipe, 0x37dd, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3800, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3801, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3802, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3803, 0x04);
	os02k10_slave_write_register(ViPipe, 0x3804, 0x07);
	os02k10_slave_write_register(ViPipe, 0x3805, 0x8f);
	os02k10_slave_write_register(ViPipe, 0x3806, 0x04);
	os02k10_slave_write_register(ViPipe, 0x3807, 0x43);
	os02k10_slave_write_register(ViPipe, 0x3808, 0x07);
	os02k10_slave_write_register(ViPipe, 0x3809, 0x80);
	os02k10_slave_write_register(ViPipe, 0x380a, 0x04);
	os02k10_slave_write_register(ViPipe, 0x380b, 0x38);
	os02k10_slave_write_register(ViPipe, 0x380c, 0x08);
	os02k10_slave_write_register(ViPipe, 0x380d, 0x70);
	os02k10_slave_write_register(ViPipe, 0x380e, 0x04);
	os02k10_slave_write_register(ViPipe, 0x380f, 0xe2);
	os02k10_slave_write_register(ViPipe, 0x3811, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3813, 0x04);
	os02k10_slave_write_register(ViPipe, 0x3814, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3815, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3816, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3817, 0x01);
	os02k10_slave_write_register(ViPipe, 0x381c, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3820, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3821, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3822, 0x14);
	os02k10_slave_write_register(ViPipe, 0x384c, 0x02);
	os02k10_slave_write_register(ViPipe, 0x384d, 0xd0);
	os02k10_slave_write_register(ViPipe, 0x3858, 0x0d);
	os02k10_slave_write_register(ViPipe, 0x3865, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3866, 0xc0);
	os02k10_slave_write_register(ViPipe, 0x3867, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3868, 0xc0);
	os02k10_slave_write_register(ViPipe, 0x3900, 0x13);
	os02k10_slave_write_register(ViPipe, 0x3940, 0x13);
	os02k10_slave_write_register(ViPipe, 0x3980, 0x13);
	os02k10_slave_write_register(ViPipe, 0x3c01, 0x11);
	os02k10_slave_write_register(ViPipe, 0x3c05, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3c0f, 0x1c);
	os02k10_slave_write_register(ViPipe, 0x3c12, 0x0d);
	os02k10_slave_write_register(ViPipe, 0x3c14, 0x21);
	os02k10_slave_write_register(ViPipe, 0x3c19, 0x01);
	os02k10_slave_write_register(ViPipe, 0x3c21, 0x40);
	os02k10_slave_write_register(ViPipe, 0x3c3b, 0x18);
	os02k10_slave_write_register(ViPipe, 0x3c3d, 0xc9);
	os02k10_slave_write_register(ViPipe, 0x3c55, 0x08);
	os02k10_slave_write_register(ViPipe, 0x3c5d, 0xcf);
	os02k10_slave_write_register(ViPipe, 0x3c5e, 0xcf);
	os02k10_slave_write_register(ViPipe, 0x3ce0, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3ce1, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3ce2, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3ce3, 0x00);
	os02k10_slave_write_register(ViPipe, 0x3d8c, 0x70);
	os02k10_slave_write_register(ViPipe, 0x3d8d, 0x10);
	os02k10_slave_write_register(ViPipe, 0x4001, 0x2f);
	os02k10_slave_write_register(ViPipe, 0x4033, 0x80);
	os02k10_slave_write_register(ViPipe, 0x4008, 0x02);
	os02k10_slave_write_register(ViPipe, 0x4009, 0x11);
	os02k10_slave_write_register(ViPipe, 0x4004, 0x01);
	os02k10_slave_write_register(ViPipe, 0x4005, 0x00);
	os02k10_slave_write_register(ViPipe, 0x400a, 0x04);
	os02k10_slave_write_register(ViPipe, 0x400b, 0xf0);
	os02k10_slave_write_register(ViPipe, 0x400e, 0x40);
	os02k10_slave_write_register(ViPipe, 0x4011, 0xbb);
	os02k10_slave_write_register(ViPipe, 0x410f, 0x01);
	os02k10_slave_write_register(ViPipe, 0x402e, 0x01);
	os02k10_slave_write_register(ViPipe, 0x402f, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4030, 0x01);
	os02k10_slave_write_register(ViPipe, 0x4031, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4032, 0x9f);
	os02k10_slave_write_register(ViPipe, 0x4050, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4051, 0x07);
	os02k10_slave_write_register(ViPipe, 0x4288, 0xcf);
	os02k10_slave_write_register(ViPipe, 0x4289, 0x03);
	os02k10_slave_write_register(ViPipe, 0x428a, 0x46);
	os02k10_slave_write_register(ViPipe, 0x430b, 0xff);
	os02k10_slave_write_register(ViPipe, 0x430c, 0xff);
	os02k10_slave_write_register(ViPipe, 0x430d, 0x00);
	os02k10_slave_write_register(ViPipe, 0x430e, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4314, 0x04);
	os02k10_slave_write_register(ViPipe, 0x4500, 0x18);
	os02k10_slave_write_register(ViPipe, 0x4501, 0x18);
	os02k10_slave_write_register(ViPipe, 0x4504, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4507, 0x02);
	os02k10_slave_write_register(ViPipe, 0x4508, 0x1a);
	os02k10_slave_write_register(ViPipe, 0x4603, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4640, 0x62);
	os02k10_slave_write_register(ViPipe, 0x4646, 0xaa);
	os02k10_slave_write_register(ViPipe, 0x4647, 0x55);
	os02k10_slave_write_register(ViPipe, 0x4648, 0x99);
	os02k10_slave_write_register(ViPipe, 0x4649, 0x66);
	os02k10_slave_write_register(ViPipe, 0x464d, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4654, 0x11);
	os02k10_slave_write_register(ViPipe, 0x4655, 0x22);
	os02k10_slave_write_register(ViPipe, 0x4800, 0x04);
	os02k10_slave_write_register(ViPipe, 0x4810, 0xff);
	os02k10_slave_write_register(ViPipe, 0x4811, 0xff);
	os02k10_slave_write_register(ViPipe, 0x480e, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4813, 0x00);
	os02k10_slave_write_register(ViPipe, 0x4837, 0x0c);
	os02k10_slave_write_register(ViPipe, 0x484b, 0x27);
	os02k10_slave_write_register(ViPipe, 0x4d00, 0x4e);
	os02k10_slave_write_register(ViPipe, 0x4d01, 0x0c);
	os02k10_slave_write_register(ViPipe, 0x4d09, 0x4f);
	os02k10_slave_write_register(ViPipe, 0x5000, 0x1f);
	os02k10_slave_write_register(ViPipe, 0x5080, 0x00);
	os02k10_slave_write_register(ViPipe, 0x50c0, 0x00);
	os02k10_slave_write_register(ViPipe, 0x5100, 0x00);
	os02k10_slave_write_register(ViPipe, 0x5200, 0x00);
	os02k10_slave_write_register(ViPipe, 0x5201, 0x70);
	os02k10_slave_write_register(ViPipe, 0x5202, 0x03);
	os02k10_slave_write_register(ViPipe, 0x5203, 0x7f);
	os02k10_slave_write_register(ViPipe, 0x5780, 0x53);
	os02k10_slave_write_register(ViPipe, 0x5786, 0x01);
	os02k10_slave_write_register(ViPipe, 0x0305, 0x50);
	os02k10_slave_write_register(ViPipe, 0x4837, 0x10);
	os02k10_slave_write_register(ViPipe, 0x380e, 0x06);
	os02k10_slave_write_register(ViPipe, 0x380f, 0x82);
	os02k10_slave_write_register(ViPipe, 0x3501, 0x06);
	os02k10_slave_write_register(ViPipe, 0x3502, 0x7a);

	os02k10_slave_default_reg_init(ViPipe);

	if (g_au16Os02k10_Slave_UseHwSync[ViPipe]) {
		os02k10_slave_write_register(ViPipe, 0x0100, 0x00);
		os02k10_slave_write_register(ViPipe, 0x3660, 0x03);
		os02k10_slave_write_register(ViPipe, 0x3823, 0x50);
		os02k10_slave_write_register(ViPipe, 0x383e, 0x81);
		os02k10_slave_write_register(ViPipe, 0x3881, 0x06);
		os02k10_slave_write_register(ViPipe, 0x3882, 0x00);
		os02k10_slave_write_register(ViPipe, 0x3883, 0x08);
		os02k10_slave_write_register(ViPipe, 0x3835, 0x00);
		os02k10_slave_write_register(ViPipe, 0x3836, 0x08);
		os02k10_slave_write_register(ViPipe, 0x3009, 0x02);
	}

	os02k10_slave_write_register(ViPipe, 0x0100, 0x01);

	delay_ms(100);

	printf("ViPipe:%d,===OS02K10_SLAVE 1520P 30fps 10bit LINE Init OK!===\n", ViPipe);
}




