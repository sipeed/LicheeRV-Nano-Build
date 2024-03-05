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
#include "imx385_cmos_ex.h"

static void imx385_wdr_1080p30_2to1_init(VI_PIPE ViPipe);
static void imx385_linear_1080p30_init(VI_PIPE ViPipe);

const CVI_U8 imx385_i2c_addr = 0x1A;
const CVI_U32 imx385_addr_byte = 2;
const CVI_U32 imx385_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int imx385_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunImx385_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, imx385_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int imx385_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int imx385_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (imx385_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, imx385_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, imx385_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (imx385_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}


int imx385_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (imx385_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (imx385_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, imx385_addr_byte + imx385_data_byte);
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

void imx385_standby(VI_PIPE ViPipe)
{
	imx385_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx385_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
}

void imx385_restart(VI_PIPE ViPipe)
{
	imx385_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx385_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	imx385_write_register(ViPipe, 0x304b, 0x0a);
}

void imx385_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx385[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		imx385_write_register(ViPipe,
				g_pastImx385[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx385[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void imx385_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = imx385_read_register(ViPipe, 0x3007) & ~0x3;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		val |= 0x2;
		break;
	case ISP_SNS_FLIP:
		val |= 0x1;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val |= 0x3;
		break;
	default:
		return;
	}

	imx385_write_register(ViPipe, 0x3007, val);
}

void imx385_init(VI_PIPE ViPipe)
{
	WDR_MODE_E        enWDRMode;
	CVI_U8            u8ImgMode;

	enWDRMode   = g_pastImx385[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastImx385[ViPipe]->u8ImgMode;

	imx385_i2c_init(ViPipe);

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		if (u8ImgMode == IMX385_MODE_1080P30_WDR) {
			imx385_wdr_1080p30_2to1_init(ViPipe);
		}
	} else {
		imx385_linear_1080p30_init(ViPipe);
	}
	g_pastImx385[ViPipe]->bInit = CVI_TRUE;
}

void imx385_exit(VI_PIPE ViPipe)
{
	imx385_i2c_exit(ViPipe);
}

/* 1080P30 and 1080P25 */
static void imx385_linear_1080p30_init(VI_PIPE ViPipe)
{
#if 0
	/* 60 fps */
	imx385_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	delay_ms(4);
	imx385_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx385_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	imx385_write_register(ViPipe, 0x3005, 0x01); /* ADBIT 12bit*/
	imx385_write_register(ViPipe, 0x3007, 0x00); /* VREVERS*/
	imx385_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	imx385_write_register(ViPipe, 0x3012, 0x2C);
	imx385_write_register(ViPipe, 0x3013, 0x01);
	imx385_write_register(ViPipe, 0x3049, 0x0A);/* XVSOUTSEL[0] */
	imx385_write_register(ViPipe, 0x3054, 0x66);/* SCDEN */
	imx385_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	imx385_write_register(ViPipe, 0x305D, 0x00); /* INCKSEL2*/
	imx385_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	imx385_write_register(ViPipe, 0x305F, 0x00); /* INCKSEL4*/
	imx385_write_register(ViPipe, 0x310B, 0x07); /* BLKLEVEL*/
	imx385_write_register(ViPipe, 0x3110, 0x12);
	imx385_write_register(ViPipe, 0x31ED, 0x38);
	imx385_write_register(ViPipe, 0x3338, 0xD4);/* WINPV*/
	imx385_write_register(ViPipe, 0x3339, 0x40);/* WINPV*/
	imx385_write_register(ViPipe, 0x333A, 0x10);/* WINWV*/
	imx385_write_register(ViPipe, 0x333B, 0x00);/* WINWV*/
	imx385_write_register(ViPipe, 0x333C, 0xD4);/* WINPH*/
	imx385_write_register(ViPipe, 0x333D, 0x40);/* WINPH*/
	imx385_write_register(ViPipe, 0x333E, 0x10);/* WINWH*/
	imx385_write_register(ViPipe, 0x333F, 0x00);/* WINWH*/
	imx385_write_register(ViPipe, 0x3380, 0x20);/* INCK_FREQ1*/
	imx385_write_register(ViPipe, 0x3381, 0x25);/* INCK_FREQ1*/
	imx385_write_register(ViPipe, 0x3384, 0x60);/* hs_zero*/
	imx385_write_register(ViPipe, 0x3385, 0x27);/* hs_trail*/
	imx385_write_register(ViPipe, 0x338D, 0xB4);/* INCK_FREQ2*/
	imx385_write_register(ViPipe, 0x338E, 0x01);/* INCK_FREQ2*/

	imx385_default_reg_init(ViPipe);

	imx385_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx385_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	delay_ms(10);
#endif

	imx385_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	delay_ms(4);
	imx385_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx385_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	imx385_write_register(ViPipe, 0x3005, 0x01); /* ADBIT 12bit*/
	imx385_write_register(ViPipe, 0x3007, 0x00); /* VREVERS*/
	imx385_write_register(ViPipe, 0x3009, 0x02);
	imx385_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	imx385_write_register(ViPipe, 0x300C, 0x00);
	imx385_write_register(ViPipe, 0x3012, 0x2C);
	imx385_write_register(ViPipe, 0x3013, 0x01);
	imx385_write_register(ViPipe, 0x3016, 0x09);
	imx385_write_register(ViPipe, 0x301B, 0x30);/*hmax*/
	imx385_write_register(ViPipe, 0x301C, 0x11);
	imx385_write_register(ViPipe, 0x3020, 0x00);
	imx385_write_register(ViPipe, 0x3023, 0x00);
	imx385_write_register(ViPipe, 0x3024, 0x00);
	imx385_write_register(ViPipe, 0x302c, 0x00);
	imx385_write_register(ViPipe, 0x3043, 0x01);
	imx385_write_register(ViPipe, 0x3049, 0x0A);/* XVSOUTSEL[0] */
	imx385_write_register(ViPipe, 0x3054, 0x66);/* SCDEN */
	imx385_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	imx385_write_register(ViPipe, 0x305D, 0x00); /* INCKSEL2*/
	imx385_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	imx385_write_register(ViPipe, 0x305F, 0x00); /* INCKSEL4*/
	imx385_write_register(ViPipe, 0x3109, 0x00);
	imx385_write_register(ViPipe, 0x310B, 0x07);
	imx385_write_register(ViPipe, 0x3110, 0x12);
	imx385_write_register(ViPipe, 0x31ED, 0x38);
	imx385_write_register(ViPipe, 0x3338, 0xD4);/* WINPV*/
	imx385_write_register(ViPipe, 0x3339, 0x40);/* WINPV*/
	imx385_write_register(ViPipe, 0x333A, 0x10);/* WINWV*/
	imx385_write_register(ViPipe, 0x333B, 0x00);/* WINWV*/
	imx385_write_register(ViPipe, 0x333C, 0xD4);/* WINPH*/
	imx385_write_register(ViPipe, 0x333D, 0x40);/* WINPH*/
	imx385_write_register(ViPipe, 0x333E, 0x10);/* WINWH*/
	imx385_write_register(ViPipe, 0x333F, 0x00);/* WINWH*/
	imx385_write_register(ViPipe, 0x3344, 0x10);/* recptione*/
	imx385_write_register(ViPipe, 0x3346, 0x03);
	imx385_write_register(ViPipe, 0x3353, 0x0E);
	imx385_write_register(ViPipe, 0x3354, 0x01);
	imx385_write_register(ViPipe, 0x3357, 0x49);
	imx385_write_register(ViPipe, 0x3358, 0x04);
	imx385_write_register(ViPipe, 0x336B, 0x2F);/* THSEXIT*/
	imx385_write_register(ViPipe, 0x337D, 0x0C);
	imx385_write_register(ViPipe, 0x337E, 0x0C);
	imx385_write_register(ViPipe, 0x3380, 0x20);/* INCK_FREQ1*/
	imx385_write_register(ViPipe, 0x3381, 0x25);/* INCK_FREQ1*/
	imx385_write_register(ViPipe, 0x3382, 0x5F);/* clk_post*/
	imx385_write_register(ViPipe, 0x3383, 0x17);/* hs_prepare*/
	imx385_write_register(ViPipe, 0x3384, 0x70);/* hs_zero*/
	imx385_write_register(ViPipe, 0x3385, 0x70);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3386, 0x17);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3387, 0x0F);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3388, 0x4F);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3389, 0x27);/* hs_trail*/
	imx385_write_register(ViPipe, 0x338D, 0xB4);/* INCK_FREQ2*/
	imx385_write_register(ViPipe, 0x338E, 0x01);/* INCK_FREQ2*/

	imx385_default_reg_init(ViPipe);

	imx385_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx385_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	delay_ms(80);
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe:%d,===IMX385 1080P 30fps 12bit LINE Init OK!===\n", ViPipe);
}

static void imx385_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
	imx385_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	delay_ms(10);
	imx385_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx385_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	imx385_write_register(ViPipe, 0x3005, 0x01); /* ADBIT*/
	imx385_write_register(ViPipe, 0x3007, 0x10); /* WINMODE*/
	imx385_write_register(ViPipe, 0x3009, 0x01);
	imx385_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	imx385_write_register(ViPipe, 0x300C, 0x11); /* WDSEL*/
	imx385_write_register(ViPipe, 0x3012, 0x2C);
	imx385_write_register(ViPipe, 0x3013, 0x01);
	imx385_write_register(ViPipe, 0x3016, 0x09);
	imx385_write_register(ViPipe, 0x3020, 0x03); /* SHS[7:0]*/
	imx385_write_register(ViPipe, 0x3021, 0x00); /* SHS[15:8]*/
	imx385_write_register(ViPipe, 0x3022, 0x00); /* SHS[19:16]*/
	imx385_write_register(ViPipe, 0x3023, 0xE9); /* SHS2[7:0], TBD*/
	imx385_write_register(ViPipe, 0x3024, 0x03); /* SHS2[7:0], TBD*/
	imx385_write_register(ViPipe, 0x302C, 0x2B);
	imx385_write_register(ViPipe, 0x3043, 0x05);
	imx385_write_register(ViPipe, 0x3049, 0x0A);
	imx385_write_register(ViPipe, 0x3054, 0x66); /* SCDEN*/
	imx385_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	imx385_write_register(ViPipe, 0x305D, 0x00); /* INCKSEL2*/
	imx385_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	imx385_write_register(ViPipe, 0x305F, 0x00); /* INCKSEL4*/
	imx385_write_register(ViPipe, 0x3109, 0x01);
	imx385_write_register(ViPipe, 0x310B, 0x07);
	imx385_write_register(ViPipe, 0x3110, 0x12);
	imx385_write_register(ViPipe, 0x31ED, 0x38);
	imx385_write_register(ViPipe, 0x3338, 0xD4);/* WINPV*/
	imx385_write_register(ViPipe, 0x3339, 0x40);/* WINPV*/
	imx385_write_register(ViPipe, 0x333A, 0x10);/* WINWV*/
	imx385_write_register(ViPipe, 0x333B, 0x00);/* WINWV*/
	imx385_write_register(ViPipe, 0x333C, 0xD4);/* WINPH*/
	imx385_write_register(ViPipe, 0x333D, 0x40);/* WINPH*/
	imx385_write_register(ViPipe, 0x333E, 0x10);/* WINWH*/
	imx385_write_register(ViPipe, 0x333F, 0x00);/* WINWH*/
	imx385_write_register(ViPipe, 0x3344, 0x00);
	imx385_write_register(ViPipe, 0x3346, 0x03);/* PHY_LANE_NUM*/
	imx385_write_register(ViPipe, 0x3353, 0x0E);
	imx385_write_register(ViPipe, 0x3354, 0x00);
	imx385_write_register(ViPipe, 0x3357, 0xDC); /*PIC SIZE*/
	imx385_write_register(ViPipe, 0x3358, 0x08);
	imx385_write_register(ViPipe, 0x336B, 0x3F);
	imx385_write_register(ViPipe, 0x337D, 0x0C);
	imx385_write_register(ViPipe, 0x337E, 0x0C);
	imx385_write_register(ViPipe, 0x3380, 0x20); /*INCK_FREQ1*/
	imx385_write_register(ViPipe, 0x3381, 0x25);
	imx385_write_register(ViPipe, 0x3382, 0x67);/* clk_post*/
	imx385_write_register(ViPipe, 0x3383, 0x1F);/* hs_prepare*/
	imx385_write_register(ViPipe, 0x3384, 0x60);/* hs_zero*/
	imx385_write_register(ViPipe, 0x3385, 0x27);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3386, 0x1f);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3387, 0x17);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3388, 0x77);/* hs_trail*/
	imx385_write_register(ViPipe, 0x3389, 0x27);/* hs_trail*/
	imx385_write_register(ViPipe, 0x338D, 0xB4); /*INCK_FREQ2*/
	imx385_write_register(ViPipe, 0x338E, 0x01);

	imx385_default_reg_init(ViPipe);

	imx385_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx385_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	delay_ms(80);
	CVI_TRACE_SNS(CVI_DBG_INFO, "===Imx385 sensor 1080P30fps 12bit 2to1 WDR(60fps->30fps) init success!=====\n");
}
