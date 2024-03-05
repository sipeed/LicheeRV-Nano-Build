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
#include "sc4210_cmos_ex.h"

static void sc4210_wdr_1440p30_2to1_init(VI_PIPE ViPipe);
static void sc4210_linear_1440p30_init(VI_PIPE ViPipe);

CVI_U8 sc4210_i2c_addr = 0x30;        /* I2C Address of SC4210 */
const CVI_U32 sc4210_addr_byte = 2;
const CVI_U32 sc4210_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int sc4210_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunSC4210_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, sc4210_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int sc4210_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int sc4210_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return 0;

	if (sc4210_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, sc4210_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, sc4210_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (sc4210_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;

	return CVI_SUCCESS;
}

int sc4210_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (sc4210_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (sc4210_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, sc4210_addr_byte + sc4210_data_byte);
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

void sc4210_standby(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

void sc4210_restart(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

void sc4210_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastSC4210[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sc4210_write_register(ViPipe,
				g_pastSC4210[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastSC4210[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void sc4210_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 value = 0;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		value = 0;
		break;
	case ISP_SNS_MIRROR:
		value = 0x06;
		break;
	case ISP_SNS_FLIP:
		value = 0x60;
		break;
	case ISP_SNS_MIRROR_FLIP:
		value = 0x66;
		break;
	default:
		return;
	}
	sc4210_write_register(ViPipe, 0x3221, value);
}

void sc4210_init(VI_PIPE ViPipe)
{
	WDR_MODE_E        enWDRMode;
	CVI_U8            u8ImgMode;

	enWDRMode   = g_pastSC4210[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastSC4210[ViPipe]->u8ImgMode;

	sc4210_i2c_init(ViPipe);

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		if (u8ImgMode == SC4210_MODE_1440P30_WDR) {
			/* SC4210_MODE_1440P30_WDR */
			sc4210_wdr_1440p30_2to1_init(ViPipe);
		} else {
		}
	} else {
		sc4210_linear_1440p30_init(ViPipe);
	}
	g_pastSC4210[ViPipe]->bInit = CVI_TRUE;
}

void sc4210_exit(VI_PIPE ViPipe)
{
	sc4210_i2c_exit(ViPipe);
}

/* 1440P30 and 1440P25 */
static void sc4210_linear_1440p30_init(VI_PIPE ViPipe)
{
	printf("vipe=%d SC4210_MIPI_12bit_4lane_364.5Mbps_30fps_2560x1440_BSI_modified20190520", ViPipe);

	delay_ms(50);
	sc4210_write_register(ViPipe, 0x0103, 0x01);//软复位
	sc4210_write_register(ViPipe, 0x0100, 0x00);//软睡眠模式 0:enable;1:disable
	sc4210_write_register(ViPipe, 0x36e9, 0xa7);
	sc4210_write_register(ViPipe, 0x36f9, 0xa0);
	sc4210_write_register(ViPipe, 0x300f, 0xff);
	sc4210_write_register(ViPipe, 0x3001, 0x07);
	sc4210_write_register(ViPipe, 0x3000, 0x00);
	sc4210_write_register(ViPipe, 0x300f, 0x00);
	sc4210_write_register(ViPipe, 0x300a, 0x2c);
	sc4210_write_register(ViPipe, 0x3002, 0xc0);
	sc4210_write_register(ViPipe, 0x3302, 0x18);
	sc4210_write_register(ViPipe, 0x3634, 0x64);
	sc4210_write_register(ViPipe, 0x3624, 0x07);
	sc4210_write_register(ViPipe, 0x3630, 0xc4);
	sc4210_write_register(ViPipe, 0x3631, 0x80);
	sc4210_write_register(ViPipe, 0x363c, 0x06);
	sc4210_write_register(ViPipe, 0x363d, 0x06);
	sc4210_write_register(ViPipe, 0x3038, 0x22);
	sc4210_write_register(ViPipe, 0x3253, 0x08);
	sc4210_write_register(ViPipe, 0x325e, 0x00);
	sc4210_write_register(ViPipe, 0x325f, 0x00);
	sc4210_write_register(ViPipe, 0x3251, 0x08);
	sc4210_write_register(ViPipe, 0x3225, 0x01);
	sc4210_write_register(ViPipe, 0x3227, 0x03);
	sc4210_write_register(ViPipe, 0x391b, 0x80);
	sc4210_write_register(ViPipe, 0x391c, 0x0f);
	sc4210_write_register(ViPipe, 0x3935, 0x80);
	sc4210_write_register(ViPipe, 0x3936, 0x1f);
	sc4210_write_register(ViPipe, 0x3908, 0x11);
	sc4210_write_register(ViPipe, 0x3273, 0x01);
	sc4210_write_register(ViPipe, 0x3241, 0x02);
	sc4210_write_register(ViPipe, 0x3243, 0x03);
	sc4210_write_register(ViPipe, 0x3249, 0x17);
	sc4210_write_register(ViPipe, 0x3229, 0x08);
	sc4210_write_register(ViPipe, 0x3905, 0xd8);
	sc4210_write_register(ViPipe, 0x3018, 0x73);//MIPI lane数量 4lane
	sc4210_write_register(ViPipe, 0x3031, 0x0c);//MIPI bit 0x0c:12bit; 0x0a:10bit; 0x08:8bit;
	sc4210_write_register(ViPipe, 0x4603, 0x00);
	sc4210_write_register(ViPipe, 0x4837, 0x2c);
	sc4210_write_register(ViPipe, 0x3817, 0x20);
	sc4210_write_register(ViPipe, 0x39a0, 0x08);
	sc4210_write_register(ViPipe, 0x39a1, 0x10);
	sc4210_write_register(ViPipe, 0x39a2, 0x20);
	sc4210_write_register(ViPipe, 0x39a3, 0x40);
	sc4210_write_register(ViPipe, 0x39a4, 0x20);
	sc4210_write_register(ViPipe, 0x39a5, 0x10);
	sc4210_write_register(ViPipe, 0x39a6, 0x08);
	sc4210_write_register(ViPipe, 0x39a7, 0x04);
	sc4210_write_register(ViPipe, 0x39a8, 0x18);
	sc4210_write_register(ViPipe, 0x39a9, 0x30);
	sc4210_write_register(ViPipe, 0x39aa, 0x40);
	sc4210_write_register(ViPipe, 0x39ab, 0x60);
	sc4210_write_register(ViPipe, 0x39ac, 0x38);
	sc4210_write_register(ViPipe, 0x39ad, 0x20);
	sc4210_write_register(ViPipe, 0x39ae, 0x10);
	sc4210_write_register(ViPipe, 0x39af, 0x08);
	sc4210_write_register(ViPipe, 0x3980, 0x00);
	sc4210_write_register(ViPipe, 0x3981, 0x50);
	sc4210_write_register(ViPipe, 0x3982, 0x00);
	sc4210_write_register(ViPipe, 0x3983, 0x40);
	sc4210_write_register(ViPipe, 0x3984, 0x00);
	sc4210_write_register(ViPipe, 0x3985, 0x20);
	sc4210_write_register(ViPipe, 0x3986, 0x00);
	sc4210_write_register(ViPipe, 0x3987, 0x10);
	sc4210_write_register(ViPipe, 0x3988, 0x00);
	sc4210_write_register(ViPipe, 0x3989, 0x20);
	sc4210_write_register(ViPipe, 0x398a, 0x00);
	sc4210_write_register(ViPipe, 0x398b, 0x30);
	sc4210_write_register(ViPipe, 0x398c, 0x00);
	sc4210_write_register(ViPipe, 0x398d, 0x50);
	sc4210_write_register(ViPipe, 0x398e, 0x00);
	sc4210_write_register(ViPipe, 0x398f, 0x60);
	sc4210_write_register(ViPipe, 0x3990, 0x00);
	sc4210_write_register(ViPipe, 0x3991, 0x70);
	sc4210_write_register(ViPipe, 0x3992, 0x00);
	sc4210_write_register(ViPipe, 0x3993, 0x36);
	sc4210_write_register(ViPipe, 0x3994, 0x00);
	sc4210_write_register(ViPipe, 0x3995, 0x20);
	sc4210_write_register(ViPipe, 0x3996, 0x00);
	sc4210_write_register(ViPipe, 0x3997, 0x14);
	sc4210_write_register(ViPipe, 0x3998, 0x00);
	sc4210_write_register(ViPipe, 0x3999, 0x20);
	sc4210_write_register(ViPipe, 0x399a, 0x00);
	sc4210_write_register(ViPipe, 0x399b, 0x50);
	sc4210_write_register(ViPipe, 0x399c, 0x00);
	sc4210_write_register(ViPipe, 0x399d, 0x90);
	sc4210_write_register(ViPipe, 0x399e, 0x00);
	sc4210_write_register(ViPipe, 0x399f, 0xf0);
	sc4210_write_register(ViPipe, 0x39b9, 0x00);
	sc4210_write_register(ViPipe, 0x39ba, 0xa0);
	sc4210_write_register(ViPipe, 0x39bb, 0x80);
	sc4210_write_register(ViPipe, 0x39bc, 0x00);
	sc4210_write_register(ViPipe, 0x39bd, 0x44);
	sc4210_write_register(ViPipe, 0x39be, 0x00);
	sc4210_write_register(ViPipe, 0x39bf, 0x00);
	sc4210_write_register(ViPipe, 0x39c0, 0x00);
	sc4210_write_register(ViPipe, 0x3933, 0x24);
	sc4210_write_register(ViPipe, 0x3934, 0xb0);
	sc4210_write_register(ViPipe, 0x3942, 0x04);
	sc4210_write_register(ViPipe, 0x3943, 0xc0);
	sc4210_write_register(ViPipe, 0x3940, 0x68);
	sc4210_write_register(ViPipe, 0x39c5, 0x41);
	sc4210_write_register(ViPipe, 0x36ea, 0x37);
	sc4210_write_register(ViPipe, 0x36eb, 0x16);
	sc4210_write_register(ViPipe, 0x36ec, 0x03);
	sc4210_write_register(ViPipe, 0x36fa, 0x37);
	sc4210_write_register(ViPipe, 0x36fb, 0x14);
	sc4210_write_register(ViPipe, 0x36fc, 0x00);
	sc4210_write_register(ViPipe, 0x320c, 0x05);//行长high 8bit
	sc4210_write_register(ViPipe, 0x320d, 0x46);//行长low 8bit
	sc4210_write_register(ViPipe, 0x4501, 0xb4);
	sc4210_write_register(ViPipe, 0x3304, 0x20);
	sc4210_write_register(ViPipe, 0x331e, 0x19);
	sc4210_write_register(ViPipe, 0x3309, 0x40);
	sc4210_write_register(ViPipe, 0x331f, 0x39);
	sc4210_write_register(ViPipe, 0x3305, 0x00);
	sc4210_write_register(ViPipe, 0x330a, 0x00);
	sc4210_write_register(ViPipe, 0x3320, 0x05);
	sc4210_write_register(ViPipe, 0x337a, 0x08);
	sc4210_write_register(ViPipe, 0x337b, 0x10);
	sc4210_write_register(ViPipe, 0x33a3, 0x0c);
	sc4210_write_register(ViPipe, 0x3308, 0x10);
	sc4210_write_register(ViPipe, 0x3366, 0x92);
	sc4210_write_register(ViPipe, 0x3314, 0x84);
	sc4210_write_register(ViPipe, 0x334c, 0x10);
	sc4210_write_register(ViPipe, 0x3312, 0x02);
	sc4210_write_register(ViPipe, 0x336c, 0xc2);
	sc4210_write_register(ViPipe, 0x337e, 0x40);
	sc4210_write_register(ViPipe, 0x3338, 0x10);
	sc4210_write_register(ViPipe, 0x3301, 0x28);
	sc4210_write_register(ViPipe, 0x330b, 0xe8);
	sc4210_write_register(ViPipe, 0x3622, 0xff);
	sc4210_write_register(ViPipe, 0x3633, 0x22);
	sc4210_write_register(ViPipe, 0x4509, 0x10);
	sc4210_write_register(ViPipe, 0x3231, 0x01);
	sc4210_write_register(ViPipe, 0x3220, 0x10);//HDR mode使能控制
	sc4210_write_register(ViPipe, 0x3e0e, 0x6a);
	sc4210_write_register(ViPipe, 0x3625, 0x02);
	sc4210_write_register(ViPipe, 0x3636, 0x20);
	sc4210_write_register(ViPipe, 0x366e, 0x04);
	sc4210_write_register(ViPipe, 0x360f, 0x05);
	sc4210_write_register(ViPipe, 0x367a, 0x40);
	sc4210_write_register(ViPipe, 0x367b, 0x40);
	sc4210_write_register(ViPipe, 0x3671, 0xff);
	sc4210_write_register(ViPipe, 0x3672, 0x1f);
	sc4210_write_register(ViPipe, 0x3673, 0x1f);
	sc4210_write_register(ViPipe, 0x3670, 0x48);
	sc4210_write_register(ViPipe, 0x369c, 0x40);
	sc4210_write_register(ViPipe, 0x369d, 0x40);
	sc4210_write_register(ViPipe, 0x3690, 0x42);
	sc4210_write_register(ViPipe, 0x3691, 0x44);
	sc4210_write_register(ViPipe, 0x3692, 0x44);
	sc4210_write_register(ViPipe, 0x36a2, 0x40);
	sc4210_write_register(ViPipe, 0x36a3, 0x40);
	sc4210_write_register(ViPipe, 0x3699, 0x80);
	sc4210_write_register(ViPipe, 0x369a, 0x9f);
	sc4210_write_register(ViPipe, 0x369b, 0x9f);
	sc4210_write_register(ViPipe, 0x36d0, 0x20);
	sc4210_write_register(ViPipe, 0x36d1, 0x40);
	sc4210_write_register(ViPipe, 0x36d2, 0x40);
	sc4210_write_register(ViPipe, 0x36cc, 0x2c);
	sc4210_write_register(ViPipe, 0x36cd, 0x30);
	sc4210_write_register(ViPipe, 0x36ce, 0x30);
	sc4210_write_register(ViPipe, 0x5000, 0x0e);
	sc4210_write_register(ViPipe, 0x3e26, 0x40);
	sc4210_write_register(ViPipe, 0x4418, 0x0b);
	sc4210_write_register(ViPipe, 0x3306, 0x74);
	sc4210_write_register(ViPipe, 0x5784, 0x10);
	sc4210_write_register(ViPipe, 0x5785, 0x08);
	sc4210_write_register(ViPipe, 0x5787, 0x06);
	sc4210_write_register(ViPipe, 0x5788, 0x06);
	sc4210_write_register(ViPipe, 0x5789, 0x00);
	sc4210_write_register(ViPipe, 0x578a, 0x06);
	sc4210_write_register(ViPipe, 0x578b, 0x06);
	sc4210_write_register(ViPipe, 0x578c, 0x00);
	sc4210_write_register(ViPipe, 0x5790, 0x10);
	sc4210_write_register(ViPipe, 0x5791, 0x10);
	sc4210_write_register(ViPipe, 0x5792, 0x00);
	sc4210_write_register(ViPipe, 0x5793, 0x10);
	sc4210_write_register(ViPipe, 0x5794, 0x10);
	sc4210_write_register(ViPipe, 0x5795, 0x00);
	sc4210_write_register(ViPipe, 0x57c4, 0x10);
	sc4210_write_register(ViPipe, 0x57c5, 0x08);
	sc4210_write_register(ViPipe, 0x57c7, 0x06);
	sc4210_write_register(ViPipe, 0x57c8, 0x06);
	sc4210_write_register(ViPipe, 0x57c9, 0x00);
	sc4210_write_register(ViPipe, 0x57ca, 0x06);
	sc4210_write_register(ViPipe, 0x57cb, 0x06);
	sc4210_write_register(ViPipe, 0x57cc, 0x00);
	sc4210_write_register(ViPipe, 0x57d0, 0x10);
	sc4210_write_register(ViPipe, 0x57d1, 0x10);
	sc4210_write_register(ViPipe, 0x57d2, 0x00);
	sc4210_write_register(ViPipe, 0x57d3, 0x10);
	sc4210_write_register(ViPipe, 0x57d4, 0x10);
	sc4210_write_register(ViPipe, 0x57d5, 0x00);
	sc4210_write_register(ViPipe, 0x33e0, 0xa0);
	sc4210_write_register(ViPipe, 0x33e1, 0x08);
	sc4210_write_register(ViPipe, 0x33e2, 0x00);
	sc4210_write_register(ViPipe, 0x33e3, 0x10);
	sc4210_write_register(ViPipe, 0x33e4, 0x10);
	sc4210_write_register(ViPipe, 0x33e5, 0x00);
	sc4210_write_register(ViPipe, 0x33e6, 0x10);
	sc4210_write_register(ViPipe, 0x33e7, 0x10);
	sc4210_write_register(ViPipe, 0x33e8, 0x00);
	sc4210_write_register(ViPipe, 0x33e9, 0x10);
	sc4210_write_register(ViPipe, 0x33ea, 0x16);
	sc4210_write_register(ViPipe, 0x33eb, 0x00);
	sc4210_write_register(ViPipe, 0x33ec, 0x10);
	sc4210_write_register(ViPipe, 0x33ed, 0x18);
	sc4210_write_register(ViPipe, 0x33ee, 0xa0);
	sc4210_write_register(ViPipe, 0x33ef, 0x08);
	sc4210_write_register(ViPipe, 0x33f4, 0x00);
	sc4210_write_register(ViPipe, 0x33f5, 0x10);
	sc4210_write_register(ViPipe, 0x33f6, 0x10);
	sc4210_write_register(ViPipe, 0x33f7, 0x00);
	sc4210_write_register(ViPipe, 0x33f8, 0x10);
	sc4210_write_register(ViPipe, 0x33f9, 0x10);
	sc4210_write_register(ViPipe, 0x33fa, 0x00);
	sc4210_write_register(ViPipe, 0x33fb, 0x10);
	sc4210_write_register(ViPipe, 0x33fc, 0x16);
	sc4210_write_register(ViPipe, 0x33fd, 0x00);
	sc4210_write_register(ViPipe, 0x33fe, 0x10);
	sc4210_write_register(ViPipe, 0x33ff, 0x18);
	sc4210_write_register(ViPipe, 0x3638, 0x28);
	sc4210_write_register(ViPipe, 0x36ed, 0x0c);
	sc4210_write_register(ViPipe, 0x36fd, 0x2c);
	sc4210_write_register(ViPipe, 0x363b, 0x03);
	sc4210_write_register(ViPipe, 0x3635, 0x20);
	sc4210_write_register(ViPipe, 0x335d, 0x20);
	sc4210_write_register(ViPipe, 0x330e, 0x18);
	sc4210_write_register(ViPipe, 0x3367, 0x08);
	sc4210_write_register(ViPipe, 0x3368, 0x05);
	sc4210_write_register(ViPipe, 0x3369, 0xdc);
	sc4210_write_register(ViPipe, 0x336a, 0x0b);
	sc4210_write_register(ViPipe, 0x336b, 0xb8);
	sc4210_write_register(ViPipe, 0x550f, 0x20);
	sc4210_write_register(ViPipe, 0x4407, 0xb0);
	sc4210_write_register(ViPipe, 0x3e00, 0x00);
	sc4210_write_register(ViPipe, 0x3e01, 0xbb);
	sc4210_write_register(ViPipe, 0x3e02, 0x40);
	sc4210_write_register(ViPipe, 0x3e03, 0x0b);
	sc4210_write_register(ViPipe, 0x3e06, 0x00);
	sc4210_write_register(ViPipe, 0x3e07, 0x80);
	sc4210_write_register(ViPipe, 0x3e08, 0x03);
	sc4210_write_register(ViPipe, 0x3e09, 0x40);
	sc4210_write_register(ViPipe, 0x391d, 0x01);
	sc4210_write_register(ViPipe, 0x3632, 0x88);
	sc4210_write_register(ViPipe, 0x36e9, 0x27);
	sc4210_write_register(ViPipe, 0x36f9, 0x20);

	sc4210_default_reg_init(ViPipe);

	sc4210_write_register(ViPipe, 0x0100, 0x01);
	printf("ViPipe:%d,===SC4210 1440P 30fps 12bit LINE Init OK!===\n", ViPipe);
}

static void sc4210_wdr_1440p30_2to1_init(VI_PIPE ViPipe)
{
	printf("pipe=%d SC4210_MIPI_10bit_4lane_626.4Mbps_(60fps->30fps)_2560x1440_VC_BSI_20190428", ViPipe);

	delay_ms(50);
	sc4210_write_register(ViPipe, 0x0103, 0x01);
	sc4210_write_register(ViPipe, 0x0100, 0x00);
	sc4210_write_register(ViPipe, 0x36e9, 0xb4);
	sc4210_write_register(ViPipe, 0x36f9, 0xa0);
	sc4210_write_register(ViPipe, 0x300f, 0xff);
	sc4210_write_register(ViPipe, 0x3001, 0x07);
	sc4210_write_register(ViPipe, 0x3000, 0x00);
	sc4210_write_register(ViPipe, 0x300f, 0x00);
	sc4210_write_register(ViPipe, 0x300a, 0x2c);
	sc4210_write_register(ViPipe, 0x3002, 0xc0);
	sc4210_write_register(ViPipe, 0x3302, 0x18);
	sc4210_write_register(ViPipe, 0x3634, 0x64);
	sc4210_write_register(ViPipe, 0x3624, 0x07);
	sc4210_write_register(ViPipe, 0x3630, 0xc4);
	sc4210_write_register(ViPipe, 0x3631, 0x80);
	sc4210_write_register(ViPipe, 0x363c, 0x06);
	sc4210_write_register(ViPipe, 0x363d, 0x06);
	sc4210_write_register(ViPipe, 0x3038, 0x22);
	sc4210_write_register(ViPipe, 0x325e, 0x00);
	sc4210_write_register(ViPipe, 0x325f, 0x00);
	sc4210_write_register(ViPipe, 0x3251, 0x08);
	sc4210_write_register(ViPipe, 0x3225, 0x01);
	sc4210_write_register(ViPipe, 0x3227, 0x03);
	sc4210_write_register(ViPipe, 0x391b, 0x80);
	sc4210_write_register(ViPipe, 0x391c, 0x0f);
	sc4210_write_register(ViPipe, 0x3935, 0x80);
	sc4210_write_register(ViPipe, 0x3936, 0x1f);
	sc4210_write_register(ViPipe, 0x3908, 0x11);
	sc4210_write_register(ViPipe, 0x3273, 0x01);
	sc4210_write_register(ViPipe, 0x3241, 0x02);
	sc4210_write_register(ViPipe, 0x3243, 0x03);
	sc4210_write_register(ViPipe, 0x3249, 0x17);
	sc4210_write_register(ViPipe, 0x3229, 0x08);
	sc4210_write_register(ViPipe, 0x3018, 0x73);
	sc4210_write_register(ViPipe, 0x3031, 0x0a);
	sc4210_write_register(ViPipe, 0x4603, 0x00);
	sc4210_write_register(ViPipe, 0x39a6, 0x08);
	sc4210_write_register(ViPipe, 0x39a7, 0x04);
	sc4210_write_register(ViPipe, 0x39bf, 0x00);
	sc4210_write_register(ViPipe, 0x39c0, 0x00);
	sc4210_write_register(ViPipe, 0x3940, 0x60);
	sc4210_write_register(ViPipe, 0x3943, 0xd0);
	sc4210_write_register(ViPipe, 0x39c5, 0x41);
	sc4210_write_register(ViPipe, 0x3817, 0x21);
	sc4210_write_register(ViPipe, 0x36eb, 0x06);
	sc4210_write_register(ViPipe, 0x36ec, 0x03);
	sc4210_write_register(ViPipe, 0x36fa, 0x37);
	sc4210_write_register(ViPipe, 0x36fb, 0x04);
	sc4210_write_register(ViPipe, 0x36fc, 0x00);
	sc4210_write_register(ViPipe, 0x320c, 0x05);
	sc4210_write_register(ViPipe, 0x320d, 0x46);
	sc4210_write_register(ViPipe, 0x4501, 0xa4);
	sc4210_write_register(ViPipe, 0x3304, 0x20);
	sc4210_write_register(ViPipe, 0x331e, 0x19);
	sc4210_write_register(ViPipe, 0x3309, 0x50);
	sc4210_write_register(ViPipe, 0x331f, 0x49);
	sc4210_write_register(ViPipe, 0x3305, 0x00);
	sc4210_write_register(ViPipe, 0x330a, 0x00);
	sc4210_write_register(ViPipe, 0x3320, 0x05);
	sc4210_write_register(ViPipe, 0x337a, 0x08);
	sc4210_write_register(ViPipe, 0x337b, 0x10);
	sc4210_write_register(ViPipe, 0x33a3, 0x0c);
	sc4210_write_register(ViPipe, 0x3308, 0x10);
	sc4210_write_register(ViPipe, 0x3366, 0x92);
	sc4210_write_register(ViPipe, 0x3314, 0x84);
	sc4210_write_register(ViPipe, 0x334c, 0x10);
	sc4210_write_register(ViPipe, 0x3312, 0x02);
	sc4210_write_register(ViPipe, 0x336c, 0xc2);
	sc4210_write_register(ViPipe, 0x337e, 0x40);
	sc4210_write_register(ViPipe, 0x3338, 0x10);
	sc4210_write_register(ViPipe, 0x3622, 0xff);
	sc4210_write_register(ViPipe, 0x3633, 0x42);
	sc4210_write_register(ViPipe, 0x320e, 0x0b);//VTS 帧长 默认值1500 30帧,升帧增加此值,60帧为3000
	sc4210_write_register(ViPipe, 0x320f, 0xb8);//VTS 帧长 默认值1500 30帧,升帧增加此值,60帧为3000
	sc4210_write_register(ViPipe, 0x3250, 0x3f);
	sc4210_write_register(ViPipe, 0x4816, 0x11);
	sc4210_write_register(ViPipe, 0x3231, 0x01);
	sc4210_write_register(ViPipe, 0x3220, 0x50);
	sc4210_write_register(ViPipe, 0x3e0e, 0x6a);
	sc4210_write_register(ViPipe, 0x3625, 0x02);
	sc4210_write_register(ViPipe, 0x3636, 0x20);
	sc4210_write_register(ViPipe, 0x36ea, 0x23);
	sc4210_write_register(ViPipe, 0x366e, 0x04);
	sc4210_write_register(ViPipe, 0x360f, 0x05);
	sc4210_write_register(ViPipe, 0x367a, 0x40);
	sc4210_write_register(ViPipe, 0x367b, 0x48);
	sc4210_write_register(ViPipe, 0x3671, 0xff);
	sc4210_write_register(ViPipe, 0x3672, 0x9f);
	sc4210_write_register(ViPipe, 0x3673, 0x9f);
	sc4210_write_register(ViPipe, 0x3670, 0x48);
	sc4210_write_register(ViPipe, 0x369c, 0x40);
	sc4210_write_register(ViPipe, 0x369d, 0x48);
	sc4210_write_register(ViPipe, 0x3690, 0x43);
	sc4210_write_register(ViPipe, 0x3691, 0x54);
	sc4210_write_register(ViPipe, 0x3692, 0x66);
	sc4210_write_register(ViPipe, 0x36a2, 0x40);
	sc4210_write_register(ViPipe, 0x36a3, 0x48);
	sc4210_write_register(ViPipe, 0x3699, 0x8c);
	sc4210_write_register(ViPipe, 0x369a, 0x96);
	sc4210_write_register(ViPipe, 0x369b, 0x9f);
	sc4210_write_register(ViPipe, 0x36d0, 0x20);
	sc4210_write_register(ViPipe, 0x36d1, 0x40);
	sc4210_write_register(ViPipe, 0x36d2, 0x40);
	sc4210_write_register(ViPipe, 0x36cc, 0x2c);
	sc4210_write_register(ViPipe, 0x36cd, 0x30);
	sc4210_write_register(ViPipe, 0x36ce, 0x30);
	sc4210_write_register(ViPipe, 0x3364, 0x1e);
	sc4210_write_register(ViPipe, 0x3301, 0x24);
	sc4210_write_register(ViPipe, 0x3393, 0x24);
	sc4210_write_register(ViPipe, 0x3394, 0x24);
	sc4210_write_register(ViPipe, 0x3395, 0x24);
	sc4210_write_register(ViPipe, 0x3390, 0x08);
	sc4210_write_register(ViPipe, 0x3391, 0x08);
	sc4210_write_register(ViPipe, 0x3392, 0x08);
	sc4210_write_register(ViPipe, 0x3399, 0x1c);
	sc4210_write_register(ViPipe, 0x339a, 0x26);
	sc4210_write_register(ViPipe, 0x339b, 0x1d);
	sc4210_write_register(ViPipe, 0x339c, 0x26);
	sc4210_write_register(ViPipe, 0x3396, 0x08);
	sc4210_write_register(ViPipe, 0x3397, 0x38);
	sc4210_write_register(ViPipe, 0x3398, 0x3c);
	sc4210_write_register(ViPipe, 0x5000, 0x0e);
	sc4210_write_register(ViPipe, 0x3e26, 0x40);
	sc4210_write_register(ViPipe, 0x4418, 0x16);
	sc4210_write_register(ViPipe, 0x3638, 0x28);
	sc4210_write_register(ViPipe, 0x3306, 0x78);
	sc4210_write_register(ViPipe, 0x330b, 0xe0);
	sc4210_write_register(ViPipe, 0x5784, 0x10);
	sc4210_write_register(ViPipe, 0x5785, 0x08);
	sc4210_write_register(ViPipe, 0x5787, 0x06);
	sc4210_write_register(ViPipe, 0x5788, 0x06);
	sc4210_write_register(ViPipe, 0x5789, 0x00);
	sc4210_write_register(ViPipe, 0x578a, 0x06);
	sc4210_write_register(ViPipe, 0x578b, 0x06);
	sc4210_write_register(ViPipe, 0x578c, 0x00);
	sc4210_write_register(ViPipe, 0x5790, 0x10);
	sc4210_write_register(ViPipe, 0x5791, 0x10);
	sc4210_write_register(ViPipe, 0x5792, 0x00);
	sc4210_write_register(ViPipe, 0x5793, 0x10);
	sc4210_write_register(ViPipe, 0x5794, 0x10);
	sc4210_write_register(ViPipe, 0x5795, 0x00);
	sc4210_write_register(ViPipe, 0x57c4, 0x10);
	sc4210_write_register(ViPipe, 0x57c5, 0x08);
	sc4210_write_register(ViPipe, 0x57c7, 0x06);
	sc4210_write_register(ViPipe, 0x57c8, 0x06);
	sc4210_write_register(ViPipe, 0x57c9, 0x00);
	sc4210_write_register(ViPipe, 0x57ca, 0x06);
	sc4210_write_register(ViPipe, 0x57cb, 0x06);
	sc4210_write_register(ViPipe, 0x57cc, 0x00);
	sc4210_write_register(ViPipe, 0x57d0, 0x10);
	sc4210_write_register(ViPipe, 0x57d1, 0x10);
	sc4210_write_register(ViPipe, 0x57d2, 0x00);
	sc4210_write_register(ViPipe, 0x57d3, 0x10);
	sc4210_write_register(ViPipe, 0x57d4, 0x10);
	sc4210_write_register(ViPipe, 0x57d5, 0x00);
	sc4210_write_register(ViPipe, 0x33e0, 0xa0);
	sc4210_write_register(ViPipe, 0x33e1, 0x08);
	sc4210_write_register(ViPipe, 0x33e2, 0x00);
	sc4210_write_register(ViPipe, 0x33e3, 0x10);
	sc4210_write_register(ViPipe, 0x33e4, 0x10);
	sc4210_write_register(ViPipe, 0x33e5, 0x00);
	sc4210_write_register(ViPipe, 0x33e6, 0x10);
	sc4210_write_register(ViPipe, 0x33e7, 0x10);
	sc4210_write_register(ViPipe, 0x33e8, 0x00);
	sc4210_write_register(ViPipe, 0x33e9, 0x10);
	sc4210_write_register(ViPipe, 0x33ea, 0x16);
	sc4210_write_register(ViPipe, 0x33eb, 0x00);
	sc4210_write_register(ViPipe, 0x33ec, 0x10);
	sc4210_write_register(ViPipe, 0x33ed, 0x18);
	sc4210_write_register(ViPipe, 0x33ee, 0xa0);
	sc4210_write_register(ViPipe, 0x33ef, 0x08);
	sc4210_write_register(ViPipe, 0x33f4, 0x00);
	sc4210_write_register(ViPipe, 0x33f5, 0x10);
	sc4210_write_register(ViPipe, 0x33f6, 0x10);
	sc4210_write_register(ViPipe, 0x33f7, 0x00);
	sc4210_write_register(ViPipe, 0x33f8, 0x10);
	sc4210_write_register(ViPipe, 0x33f9, 0x10);
	sc4210_write_register(ViPipe, 0x33fa, 0x00);
	sc4210_write_register(ViPipe, 0x33fb, 0x10);
	sc4210_write_register(ViPipe, 0x33fc, 0x16);
	sc4210_write_register(ViPipe, 0x33fd, 0x00);
	sc4210_write_register(ViPipe, 0x33fe, 0x10);
	sc4210_write_register(ViPipe, 0x33ff, 0x18);
	sc4210_write_register(ViPipe, 0x3905, 0x98);
	sc4210_write_register(ViPipe, 0x4509, 0x08);
	sc4210_write_register(ViPipe, 0x36ed, 0x0c);
	sc4210_write_register(ViPipe, 0x36fd, 0x2c);
	sc4210_write_register(ViPipe, 0x3933, 0x1f);
	sc4210_write_register(ViPipe, 0x3934, 0xff);
	sc4210_write_register(ViPipe, 0x3942, 0x04);
	sc4210_write_register(ViPipe, 0x393e, 0x01);
	sc4210_write_register(ViPipe, 0x39bc, 0x00);
	sc4210_write_register(ViPipe, 0x39bd, 0x58);
	sc4210_write_register(ViPipe, 0x39be, 0xc0);
	sc4210_write_register(ViPipe, 0x39a0, 0x14);
	sc4210_write_register(ViPipe, 0x39a1, 0x28);
	sc4210_write_register(ViPipe, 0x39a2, 0x48);
	sc4210_write_register(ViPipe, 0x39a3, 0x70);
	sc4210_write_register(ViPipe, 0x39a4, 0x18);
	sc4210_write_register(ViPipe, 0x39a5, 0x04);
	sc4210_write_register(ViPipe, 0x3980, 0x00);
	sc4210_write_register(ViPipe, 0x3981, 0x30);
	sc4210_write_register(ViPipe, 0x3982, 0x00);
	sc4210_write_register(ViPipe, 0x3983, 0x2c);
	sc4210_write_register(ViPipe, 0x3984, 0x00);
	sc4210_write_register(ViPipe, 0x3985, 0x15);
	sc4210_write_register(ViPipe, 0x3986, 0x00);
	sc4210_write_register(ViPipe, 0x3987, 0x10);
	sc4210_write_register(ViPipe, 0x3988, 0x00);
	sc4210_write_register(ViPipe, 0x3989, 0x30);
	sc4210_write_register(ViPipe, 0x398a, 0x00);
	sc4210_write_register(ViPipe, 0x398b, 0x28);
	sc4210_write_register(ViPipe, 0x398c, 0x00);
	sc4210_write_register(ViPipe, 0x398d, 0x30);
	sc4210_write_register(ViPipe, 0x398e, 0x00);
	sc4210_write_register(ViPipe, 0x398f, 0x70);
	sc4210_write_register(ViPipe, 0x39b9, 0x00);
	sc4210_write_register(ViPipe, 0x39ba, 0x00);
	sc4210_write_register(ViPipe, 0x39bb, 0x00);
	sc4210_write_register(ViPipe, 0x39a8, 0x01);
	sc4210_write_register(ViPipe, 0x39a9, 0x14);
	sc4210_write_register(ViPipe, 0x39ab, 0x50);
	sc4210_write_register(ViPipe, 0x39ad, 0x20);
	sc4210_write_register(ViPipe, 0x39ae, 0x10);
	sc4210_write_register(ViPipe, 0x39af, 0x08);
	sc4210_write_register(ViPipe, 0x3991, 0x00);
	sc4210_write_register(ViPipe, 0x3992, 0x00);
	sc4210_write_register(ViPipe, 0x3993, 0x60);
	sc4210_write_register(ViPipe, 0x3994, 0x00);
	sc4210_write_register(ViPipe, 0x3995, 0x30);
	sc4210_write_register(ViPipe, 0x3996, 0x00);
	sc4210_write_register(ViPipe, 0x3997, 0x10);
	sc4210_write_register(ViPipe, 0x3998, 0x00);
	sc4210_write_register(ViPipe, 0x3999, 0x1c);
	sc4210_write_register(ViPipe, 0x399a, 0x00);
	sc4210_write_register(ViPipe, 0x399b, 0x48);
	sc4210_write_register(ViPipe, 0x399c, 0x00);
	sc4210_write_register(ViPipe, 0x399d, 0x90);
	sc4210_write_register(ViPipe, 0x399e, 0x00);
	sc4210_write_register(ViPipe, 0x399f, 0xc0);
	sc4210_write_register(ViPipe, 0x3990, 0x0a);
	sc4210_write_register(ViPipe, 0x39aa, 0x28);
	sc4210_write_register(ViPipe, 0x39ac, 0x30);
	sc4210_write_register(ViPipe, 0x363b, 0x03);
	sc4210_write_register(ViPipe, 0x3635, 0x20);
	sc4210_write_register(ViPipe, 0x335d, 0x20);
	sc4210_write_register(ViPipe, 0x330e, 0x20);
	sc4210_write_register(ViPipe, 0x3367, 0x08);
	sc4210_write_register(ViPipe, 0x3368, 0x0b);
	sc4210_write_register(ViPipe, 0x3369, 0x04);
	sc4210_write_register(ViPipe, 0x336a, 0x16);
	sc4210_write_register(ViPipe, 0x336b, 0x08);
	sc4210_write_register(ViPipe, 0x3362, 0x72);
	sc4210_write_register(ViPipe, 0x3360, 0x20);
	sc4210_write_register(ViPipe, 0x4819, 0x40);
	sc4210_write_register(ViPipe, 0x4829, 0x01);
	sc4210_write_register(ViPipe, 0x4837, 0x1b);
	sc4210_write_register(ViPipe, 0x550f, 0x20);
	sc4210_write_register(ViPipe, 0x4407, 0xb0);
	sc4210_write_register(ViPipe, 0x3253, 0x10);
	sc4210_write_register(ViPipe, 0x3e00, 0x01);
	sc4210_write_register(ViPipe, 0x3e01, 0x5f);
	sc4210_write_register(ViPipe, 0x3e02, 0xe0);
	sc4210_write_register(ViPipe, 0x3e03, 0x0b);
	sc4210_write_register(ViPipe, 0x3e04, 0x16);
	sc4210_write_register(ViPipe, 0x3e05, 0x00);
	sc4210_write_register(ViPipe, 0x3e23, 0x00);
	sc4210_write_register(ViPipe, 0x3e24, 0xb4);
	sc4210_write_register(ViPipe, 0x3e06, 0x00);
	sc4210_write_register(ViPipe, 0x3e07, 0x80);
	sc4210_write_register(ViPipe, 0x3e08, 0x03);
	sc4210_write_register(ViPipe, 0x3e09, 0x40);
	sc4210_write_register(ViPipe, 0x3e10, 0x00);
	sc4210_write_register(ViPipe, 0x3e11, 0x80);
	sc4210_write_register(ViPipe, 0x3e12, 0x03);
	sc4210_write_register(ViPipe, 0x3e13, 0x40);
	sc4210_write_register(ViPipe, 0x391d, 0x21);
	sc4210_write_register(ViPipe, 0x3632, 0x88);
	sc4210_write_register(ViPipe, 0x36e9, 0x34);
	sc4210_write_register(ViPipe, 0x36f9, 0x20);

	sc4210_default_reg_init(ViPipe);

	sc4210_write_register(ViPipe, 0x0100, 0x01);
	usleep(33*1000);

	printf("===SC4210 sensor 1440P 30fps 10bit 2to1 WDR(60fps->30fps) init success!=====\n");
}
