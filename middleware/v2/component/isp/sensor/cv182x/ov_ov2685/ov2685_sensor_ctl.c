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
// #include <linux/vi_snsr.h>
// #include <linux/cvi_comm_video.h>
#endif
#include "cvi_sns_ctrl.h"
#include "ov2685_cmos_ex.h"

#define OV2685_CHIP_ID_ADDR_H	0x300A
#define OV2685_CHIP_ID_ADDR_L	0x300B
#define OV2685_CHIP_ID		0x2685

static void ov2685_linear_720p30_init(VI_PIPE ViPipe);
static void ov2685_linear_1600x1200_30_init(VI_PIPE ViPipe);

CVI_U8 ov2685_i2c_addr = 0x29;
const CVI_U32 ov2685_addr_byte = 2;
const CVI_U32 ov2685_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int ov2685_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunOv2685_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, ov2685_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int ov2685_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int ov2685_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (ov2685_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, ov2685_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, ov2685_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (ov2685_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int ov2685_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (ov2685_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (ov2685_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, ov2685_addr_byte + ov2685_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	ret = read(g_fd[ViPipe], buf, ov2685_addr_byte + ov2685_data_byte);
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void ov2685_standby(VI_PIPE ViPipe)
{
	ov2685_write_register(ViPipe, 0x0100, 0x00);
	ov2685_write_register(ViPipe, 0x031c, 0xc7);
	ov2685_write_register(ViPipe, 0x0317, 0x01);

	printf("ov2685_standby\n");
}

void ov2685_restart(VI_PIPE ViPipe)
{
	ov2685_write_register(ViPipe, 0x0317, 0x00);
	ov2685_write_register(ViPipe, 0x031c, 0xc6);
	ov2685_write_register(ViPipe, 0x0100, 0x09);

	printf("ov2685_restart\n");
}

void ov2685_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastOv2685[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		ov2685_write_register(ViPipe,
				g_pastOv2685[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastOv2685[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

int ov2685_probe(VI_PIPE ViPipe)
{
	int nVal;
	int nVal2;

	usleep(50);
	if (ov2685_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = ov2685_read_register(ViPipe, OV2685_CHIP_ID_ADDR_H);
	nVal2 = ov2685_read_register(ViPipe, OV2685_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != OV2685_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void ov2685_init(VI_PIPE ViPipe)
{
    CVI_U8            u8ImgMode;

	ov2685_i2c_init(ViPipe);

	u8ImgMode   = g_pastOv2685[ViPipe]->u8ImgMode;

    ov2685_linear_1600x1200_30_init(ViPipe);

	g_pastOv2685[ViPipe]->bInit = CVI_TRUE;
}

void ov2685_exit(VI_PIPE ViPipe)
{
	ov2685_i2c_exit(ViPipe);
}

static void ov2685_linear_1600x1200_30_init(VI_PIPE ViPipe)
{
	ov2685_write_register(ViPipe, 0x0103, 0x0001);
	ov2685_write_register(ViPipe, 0x3002, 0x0000);
	ov2685_write_register(ViPipe, 0x3016, 0x001c);
	ov2685_write_register(ViPipe, 0x3018, 0x0084);
	ov2685_write_register(ViPipe, 0x301d, 0x00f0);
	ov2685_write_register(ViPipe, 0x3020, 0x0000);
	ov2685_write_register(ViPipe, 0x3082, 0x0037);
	ov2685_write_register(ViPipe, 0x3083, 0x0003);
	ov2685_write_register(ViPipe, 0x3084, 0x0009);
	ov2685_write_register(ViPipe, 0x3085, 0x0004);
	ov2685_write_register(ViPipe, 0x3086, 0x0000);
	ov2685_write_register(ViPipe, 0x3087, 0x0001);
	ov2685_write_register(ViPipe, 0x3501, 0x004e);
	ov2685_write_register(ViPipe, 0x3502, 0x00e0);
	ov2685_write_register(ViPipe, 0x3503, 0x0000);
	ov2685_write_register(ViPipe, 0x350b, 0x0036);
	ov2685_write_register(ViPipe, 0x3600, 0x00b4);
	ov2685_write_register(ViPipe, 0x3603, 0x0035);
	ov2685_write_register(ViPipe, 0x3604, 0x0024);
	ov2685_write_register(ViPipe, 0x3605, 0x0000);
	ov2685_write_register(ViPipe, 0x3620, 0x0024);
	ov2685_write_register(ViPipe, 0x3621, 0x0034);
	ov2685_write_register(ViPipe, 0x3622, 0x0003);
	ov2685_write_register(ViPipe, 0x3628, 0x0010);
	ov2685_write_register(ViPipe, 0x3705, 0x003c);
	ov2685_write_register(ViPipe, 0x370a, 0x0021);
	ov2685_write_register(ViPipe, 0x370c, 0x0050);
	ov2685_write_register(ViPipe, 0x370d, 0x00c0);
	ov2685_write_register(ViPipe, 0x3717, 0x0058);
	ov2685_write_register(ViPipe, 0x3718, 0x0080);
	ov2685_write_register(ViPipe, 0x3720, 0x0000);
	ov2685_write_register(ViPipe, 0x3721, 0x0009);
	ov2685_write_register(ViPipe, 0x3722, 0x0006);
	ov2685_write_register(ViPipe, 0x3723, 0x0059);
	ov2685_write_register(ViPipe, 0x3738, 0x0099);
	ov2685_write_register(ViPipe, 0x3781, 0x0080);
	ov2685_write_register(ViPipe, 0x3784, 0x000c);
	ov2685_write_register(ViPipe, 0x3789, 0x0060);
	ov2685_write_register(ViPipe, 0x3800, 0x0000);
	ov2685_write_register(ViPipe, 0x3801, 0x0000);
	ov2685_write_register(ViPipe, 0x3802, 0x0000);
	ov2685_write_register(ViPipe, 0x3803, 0x0000);
	ov2685_write_register(ViPipe, 0x3804, 0x0006);
	ov2685_write_register(ViPipe, 0x3805, 0x004e);
	ov2685_write_register(ViPipe, 0x3806, 0x0004);
	ov2685_write_register(ViPipe, 0x3807, 0x00be);
	ov2685_write_register(ViPipe, 0x3808, 0x0006);
	ov2685_write_register(ViPipe, 0x3809, 0x0040);
	ov2685_write_register(ViPipe, 0x380a, 0x0004);
	ov2685_write_register(ViPipe, 0x380b, 0x00b0);
	ov2685_write_register(ViPipe, 0x380c, 0x0006);
	ov2685_write_register(ViPipe, 0x380d, 0x00a4);
	ov2685_write_register(ViPipe, 0x380e, 0x0005);
	ov2685_write_register(ViPipe, 0x380f, 0x000e);
	ov2685_write_register(ViPipe, 0x3810, 0x0000);
	ov2685_write_register(ViPipe, 0x3811, 0x0008);
	ov2685_write_register(ViPipe, 0x3812, 0x0000);
	ov2685_write_register(ViPipe, 0x3813, 0x0008);
	ov2685_write_register(ViPipe, 0x3814, 0x0011);
	ov2685_write_register(ViPipe, 0x3815, 0x0011);
	ov2685_write_register(ViPipe, 0x3819, 0x0004);
	ov2685_write_register(ViPipe, 0x3820, 0x00c0);
	ov2685_write_register(ViPipe, 0x3821, 0x0000);
	ov2685_write_register(ViPipe, 0x3a06, 0x0001);
	ov2685_write_register(ViPipe, 0x3a07, 0x0084);
	ov2685_write_register(ViPipe, 0x3a08, 0x0001);
	ov2685_write_register(ViPipe, 0x3a09, 0x0043);
	ov2685_write_register(ViPipe, 0x3a0a, 0x0024);
	ov2685_write_register(ViPipe, 0x3a0b, 0x0060);
	ov2685_write_register(ViPipe, 0x3a0c, 0x0028);
	ov2685_write_register(ViPipe, 0x3a0d, 0x0060);
	ov2685_write_register(ViPipe, 0x3a0e, 0x0004);
	ov2685_write_register(ViPipe, 0x3a0f, 0x008c);
	ov2685_write_register(ViPipe, 0x3a10, 0x0005);
	ov2685_write_register(ViPipe, 0x3a11, 0x000c);
	ov2685_write_register(ViPipe, 0x4000, 0x0081);
	ov2685_write_register(ViPipe, 0x4001, 0x0040);
	ov2685_write_register(ViPipe, 0x4008, 0x0002);
	ov2685_write_register(ViPipe, 0x4009, 0x0009);
	ov2685_write_register(ViPipe, 0x4300, 0x0000);
	ov2685_write_register(ViPipe, 0x430e, 0x0000);
	ov2685_write_register(ViPipe, 0x4602, 0x0002);
	ov2685_write_register(ViPipe, 0x481b, 0x0040);
	ov2685_write_register(ViPipe, 0x481f, 0x0040);
	ov2685_write_register(ViPipe, 0x4837, 0x0030);
	ov2685_write_register(ViPipe, 0x5000, 0x001f);
	ov2685_write_register(ViPipe, 0x5001, 0x0005);
	ov2685_write_register(ViPipe, 0x5002, 0x0030);
	ov2685_write_register(ViPipe, 0x5003, 0x0004);
	ov2685_write_register(ViPipe, 0x5004, 0x0000);
	ov2685_write_register(ViPipe, 0x5005, 0x000c);
	ov2685_write_register(ViPipe, 0x0100, 0x0001);

    ov2685_write_register(ViPipe, 0x0000, 0x00);  // end flag

	ov2685_default_reg_init(ViPipe);
	delay_ms(10);

	printf("ViPipe:%d,===OV2685 1600X1200 30fps 10bit LINEAR Init OK!===\n", ViPipe);
}


static void ov2685_linear_720p30_init(VI_PIPE ViPipe)
{
	ov2685_write_register(ViPipe, 0x0103, 0x0001);
	ov2685_write_register(ViPipe, 0x3002, 0x0000);
	ov2685_write_register(ViPipe, 0x3016, 0x001c);
	ov2685_write_register(ViPipe, 0x3018, 0x0084);
	ov2685_write_register(ViPipe, 0x301d, 0x00f0);
	ov2685_write_register(ViPipe, 0x3020, 0x0000);
	ov2685_write_register(ViPipe, 0x3082, 0x0037);
	ov2685_write_register(ViPipe, 0x3083, 0x0003);
	ov2685_write_register(ViPipe, 0x3084, 0x0009);
	ov2685_write_register(ViPipe, 0x3085, 0x0004);
	ov2685_write_register(ViPipe, 0x3086, 0x0000);
	ov2685_write_register(ViPipe, 0x3087, 0x0001);
	ov2685_write_register(ViPipe, 0x3501, 0x004e);
	ov2685_write_register(ViPipe, 0x3502, 0x00e0);
	ov2685_write_register(ViPipe, 0x3503, 0x0000);
	ov2685_write_register(ViPipe, 0x350b, 0x0036);
	ov2685_write_register(ViPipe, 0x3600, 0x00b4);
	ov2685_write_register(ViPipe, 0x3603, 0x0035);
	ov2685_write_register(ViPipe, 0x3604, 0x0024);
	ov2685_write_register(ViPipe, 0x3605, 0x0000);
	ov2685_write_register(ViPipe, 0x3620, 0x0024);
	ov2685_write_register(ViPipe, 0x3621, 0x0034);
	ov2685_write_register(ViPipe, 0x3622, 0x0003);
	ov2685_write_register(ViPipe, 0x3628, 0x0010);
	ov2685_write_register(ViPipe, 0x3705, 0x003c);
	ov2685_write_register(ViPipe, 0x370a, 0x0021);
	ov2685_write_register(ViPipe, 0x370c, 0x0050);
	ov2685_write_register(ViPipe, 0x370d, 0x00c0);
	ov2685_write_register(ViPipe, 0x3717, 0x0058);
	ov2685_write_register(ViPipe, 0x3718, 0x0080);
	ov2685_write_register(ViPipe, 0x3720, 0x0000);
	ov2685_write_register(ViPipe, 0x3721, 0x0009);
	ov2685_write_register(ViPipe, 0x3722, 0x0006);
	ov2685_write_register(ViPipe, 0x3723, 0x0059);
	ov2685_write_register(ViPipe, 0x3738, 0x0099);
	ov2685_write_register(ViPipe, 0x3781, 0x0080);
	ov2685_write_register(ViPipe, 0x3784, 0x000c);
	ov2685_write_register(ViPipe, 0x3789, 0x0060);
	ov2685_write_register(ViPipe, 0x3800, 0x0000);
	ov2685_write_register(ViPipe, 0x3801, 0x00a0);
	ov2685_write_register(ViPipe, 0x3802, 0x0000);
	ov2685_write_register(ViPipe, 0x3803, 0x00f0);
	ov2685_write_register(ViPipe, 0x3804, 0x0005);
	ov2685_write_register(ViPipe, 0x3805, 0x00b4);
	ov2685_write_register(ViPipe, 0x3806, 0x0003);
	ov2685_write_register(ViPipe, 0x3807, 0x00d4);
	ov2685_write_register(ViPipe, 0x3808, 0x0005);
	ov2685_write_register(ViPipe, 0x3809, 0x0000);
	ov2685_write_register(ViPipe, 0x380a, 0x0002);
	ov2685_write_register(ViPipe, 0x380b, 0x00d0);
	ov2685_write_register(ViPipe, 0x380c, 0x0006);
	ov2685_write_register(ViPipe, 0x380d, 0x00a4);
	ov2685_write_register(ViPipe, 0x380e, 0x0005);
	ov2685_write_register(ViPipe, 0x380f, 0x000e);
	ov2685_write_register(ViPipe, 0x3810, 0x0000);
	ov2685_write_register(ViPipe, 0x3811, 0x0008);
	ov2685_write_register(ViPipe, 0x3812, 0x0000);
	ov2685_write_register(ViPipe, 0x3813, 0x0008);
	ov2685_write_register(ViPipe, 0x3814, 0x0011);
	ov2685_write_register(ViPipe, 0x3815, 0x0011);
	ov2685_write_register(ViPipe, 0x3819, 0x0004);
	ov2685_write_register(ViPipe, 0x3820, 0x00c0);
	ov2685_write_register(ViPipe, 0x3821, 0x0000);
	ov2685_write_register(ViPipe, 0x3a06, 0x0001);
	ov2685_write_register(ViPipe, 0x3a07, 0x0084);
	ov2685_write_register(ViPipe, 0x3a08, 0x0001);
	ov2685_write_register(ViPipe, 0x3a09, 0x0043);
	ov2685_write_register(ViPipe, 0x3a0a, 0x0024);
	ov2685_write_register(ViPipe, 0x3a0b, 0x0060);
	ov2685_write_register(ViPipe, 0x3a0c, 0x0028);
	ov2685_write_register(ViPipe, 0x3a0d, 0x0060);
	ov2685_write_register(ViPipe, 0x3a0e, 0x0004);
	ov2685_write_register(ViPipe, 0x3a0f, 0x008c);
	ov2685_write_register(ViPipe, 0x3a10, 0x0005);
	ov2685_write_register(ViPipe, 0x3a11, 0x000c);
	ov2685_write_register(ViPipe, 0x4000, 0x0081);
	ov2685_write_register(ViPipe, 0x4001, 0x0040);
	ov2685_write_register(ViPipe, 0x4008, 0x0002);
	ov2685_write_register(ViPipe, 0x4009, 0x0009);
	ov2685_write_register(ViPipe, 0x4300, 0x0000);
	ov2685_write_register(ViPipe, 0x430e, 0x0000);
	ov2685_write_register(ViPipe, 0x4602, 0x0002);
	ov2685_write_register(ViPipe, 0x481b, 0x0040);
	ov2685_write_register(ViPipe, 0x481f, 0x0040);
	ov2685_write_register(ViPipe, 0x4837, 0x0030);
	ov2685_write_register(ViPipe, 0x5000, 0x001f);
	ov2685_write_register(ViPipe, 0x5001, 0x0005);
	ov2685_write_register(ViPipe, 0x5002, 0x0030);
	ov2685_write_register(ViPipe, 0x5003, 0x0004);
	ov2685_write_register(ViPipe, 0x5004, 0x0000);
	ov2685_write_register(ViPipe, 0x5005, 0x000c);
	ov2685_write_register(ViPipe, 0x0100, 0x0001);

    ov2685_write_register(ViPipe, 0x0000, 0x00);  // end flag

	ov2685_default_reg_init(ViPipe);
	delay_ms(10);

	printf("ViPipe:%d,===OV2685 1280x720 30fps 10bit LINEAR Init OK!===\n", ViPipe);
}
