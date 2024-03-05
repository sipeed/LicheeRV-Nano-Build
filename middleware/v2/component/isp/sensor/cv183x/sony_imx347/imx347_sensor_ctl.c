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
#include "imx347_cmos_ex.h"

static void imx347_wdr_4M30_2to1_init(VI_PIPE ViPipe);
static void imx347_linear_4M60_init(VI_PIPE ViPipe);

const CVI_U8 imx347_i2c_addr = 0x1A;//0x34;
//const CVI_U8 imx347_i2c_addr = 0x56;//0x34;

static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int imx347_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunImx347_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, imx347_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int imx347_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int imx347_read_register(VI_PIPE ViPipe, int addr)
{
	/* TODO:*/
	(void) ViPipe;
	(void) addr;

	return CVI_SUCCESS;
}


int imx347_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

#if (IMX347_ADDR_LEN == 2)
	buf[idx] = (addr >> 8) & 0xff;
	idx++;
#endif
	buf[idx] = addr & 0xff;
	idx++;

#if (IMX347_DATA_LEN == 2)
	buf[idx] = (data >> 8) & 0xff;
	idx++;
#endif
	buf[idx] = data & 0xff;
	idx++;

	ret = write(g_fd[ViPipe], buf, IMX347_ADDR_LEN + IMX347_DATA_LEN);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static inline void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void imx347_standby(VI_PIPE ViPipe)
{
	imx347_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx347_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
}

void imx347_restart(VI_PIPE ViPipe)
{
	imx347_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx347_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	//imx347_write_register(ViPipe, 0x304b, 0x0a);
}

void imx347_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx347[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		imx347_write_register(ViPipe,
				g_pastImx347[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx347[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void imx347_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 u8Filp = 0;
	CVI_U8 u8Mirror = 0;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		u8Mirror = 1;
		break;
	case ISP_SNS_FLIP:
		u8Filp = 1;
		break;
	case ISP_SNS_MIRROR_FLIP:
		u8Filp = 1;
		u8Mirror = 1;
		break;
	default:
		return;
	}

	imx347_write_register(ViPipe, 0x304e, u8Mirror);
	imx347_write_register(ViPipe, 0x304f, u8Filp);
	if (u8Filp) {
		imx347_write_register(ViPipe, 0x3078, 0x01);
		imx347_write_register(ViPipe, 0x3079, 0x00);
		imx347_write_register(ViPipe, 0x307A, 0x00);
		imx347_write_register(ViPipe, 0x307B, 0x00);
		imx347_write_register(ViPipe, 0x3080, 0xFF);
		imx347_write_register(ViPipe, 0x3081, 0x00);
		imx347_write_register(ViPipe, 0x3082, 0x00);
		imx347_write_register(ViPipe, 0x3083, 0x00);
		imx347_write_register(ViPipe, 0x30A4, 0x00);
		imx347_write_register(ViPipe, 0x30A5, 0x00);
		imx347_write_register(ViPipe, 0x30A6, 0x00);
		imx347_write_register(ViPipe, 0x30A7, 0x00);
		imx347_write_register(ViPipe, 0x30AD, 0x7E);
		imx347_write_register(ViPipe, 0x30B6, 0xFF);
		imx347_write_register(ViPipe, 0x30B7, 0x01);
		imx347_write_register(ViPipe, 0x30D8, 0x45);
		imx347_write_register(ViPipe, 0x30D9, 0x06);
		imx347_write_register(ViPipe, 0x3112, 0x02);
		imx347_write_register(ViPipe, 0x3113, 0x00);
		imx347_write_register(ViPipe, 0x3116, 0x01);
		imx347_write_register(ViPipe, 0x3117, 0x00);
	} else {
		imx347_write_register(ViPipe, 0x3078, 0x01);
		imx347_write_register(ViPipe, 0x3079, 0x00);
		imx347_write_register(ViPipe, 0x307A, 0x00);
		imx347_write_register(ViPipe, 0x307B, 0x00);
		imx347_write_register(ViPipe, 0x3080, 0x01);
		imx347_write_register(ViPipe, 0x3081, 0x00);
		imx347_write_register(ViPipe, 0x3082, 0x00);
		imx347_write_register(ViPipe, 0x3083, 0x00);
		imx347_write_register(ViPipe, 0x30A4, 0x00);
		imx347_write_register(ViPipe, 0x30A5, 0x00);
		imx347_write_register(ViPipe, 0x30A6, 0x00);
		imx347_write_register(ViPipe, 0x30A7, 0x00);
		imx347_write_register(ViPipe, 0x30AD, 0x02);
		imx347_write_register(ViPipe, 0x30B6, 0x00);
		imx347_write_register(ViPipe, 0x30B7, 0x00);
		imx347_write_register(ViPipe, 0x30D8, 0x44);
		imx347_write_register(ViPipe, 0x30D9, 0x06);
		imx347_write_register(ViPipe, 0x3112, 0x02);
		imx347_write_register(ViPipe, 0x3113, 0x00);
		imx347_write_register(ViPipe, 0x3116, 0x02);
		imx347_write_register(ViPipe, 0x3117, 0x00);
	}
}

void imx347_init(VI_PIPE ViPipe)
{
	WDR_MODE_E        enWDRMode;
	CVI_U8            u8ImgMode;

	enWDRMode   = g_pastImx347[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastImx347[ViPipe]->u8ImgMode;

	imx347_i2c_init(ViPipe);

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		if (u8ImgMode == IMX347_MODE_4M30_WDR)
			imx347_wdr_4M30_2to1_init(ViPipe);
	} else {
		if (u8ImgMode == IMX347_MODE_4M60)
			imx347_linear_4M60_init(ViPipe);
	}
	g_pastImx347[ViPipe]->bInit = CVI_TRUE;
}

void imx347_exit(VI_PIPE ViPipe)
{
	imx347_i2c_exit(ViPipe);
}

static void imx347_linear_4M60_init(VI_PIPE ViPipe)
{
	//Reset
	imx347_write_register(ViPipe, 0x3000, 0x01); // STANDBY
	imx347_write_register(ViPipe, 0x3002, 0x01); // XTMSTA
	imx347_write_register(ViPipe, 0x3004, 0x00); // Reset
	delay_ms(1);

	imx347_write_register(ViPipe, 0x300C, 0x5B);
	imx347_write_register(ViPipe, 0x300D, 0x40);
	// imx347_write_register(ViPipe, 0x3050, 0x00);// AD bit= 0:10bit, 1:12bit
	imx347_write_register(ViPipe, 0x30BE, 0x5E);
	imx347_write_register(ViPipe, 0x3110, 0x02);
	// imx347_write_register(ViPipe, 0x314C, 0xC0);
	imx347_write_register(ViPipe, 0x315A, 0x02);
	imx347_write_register(ViPipe, 0x316A, 0x7E);
	// imx347_write_register(ViPipe, 0x319D, 0x00);
	// imx347_write_register(ViPipe, 0x319E, 0x02);
	imx347_write_register(ViPipe, 0x31A1, 0x00);
	imx347_write_register(ViPipe, 0x3202, 0x02);
	imx347_write_register(ViPipe, 0x3288, 0x22);
	imx347_write_register(ViPipe, 0x328A, 0x02);
	imx347_write_register(ViPipe, 0x328C, 0xA2);
	imx347_write_register(ViPipe, 0x328E, 0x22);

	imx347_write_register(ViPipe, 0x3415, 0x27);
	imx347_write_register(ViPipe, 0x3418, 0x27);
	imx347_write_register(ViPipe, 0x3428, 0xFE);
	imx347_write_register(ViPipe, 0x349E, 0x6A);
	imx347_write_register(ViPipe, 0x34A2, 0x9A);
	imx347_write_register(ViPipe, 0x34A4, 0x8A);
	imx347_write_register(ViPipe, 0x34A6, 0x8E);
	imx347_write_register(ViPipe, 0x34AA, 0xD8);
	imx347_write_register(ViPipe, 0x3648, 0x01);
	imx347_write_register(ViPipe, 0x3678, 0x01);
	imx347_write_register(ViPipe, 0x367C, 0x69);
	imx347_write_register(ViPipe, 0x367E, 0x69);
	imx347_write_register(ViPipe, 0x3680, 0x69);
	imx347_write_register(ViPipe, 0x3682, 0x69);
	imx347_write_register(ViPipe, 0x371D, 0x05);
	imx347_write_register(ViPipe, 0x375D, 0x11);
	imx347_write_register(ViPipe, 0x375E, 0x43);
	imx347_write_register(ViPipe, 0x375F, 0x76);
	imx347_write_register(ViPipe, 0x3760, 0x07);
	imx347_write_register(ViPipe, 0x3768, 0x1B);
	imx347_write_register(ViPipe, 0x3769, 0x1B);
	imx347_write_register(ViPipe, 0x376A, 0x1A);
	imx347_write_register(ViPipe, 0x376B, 0x19);
	imx347_write_register(ViPipe, 0x376C, 0x17);
	imx347_write_register(ViPipe, 0x376D, 0x0F);
	imx347_write_register(ViPipe, 0x376E, 0x0B);
	imx347_write_register(ViPipe, 0x376F, 0x0B);
	imx347_write_register(ViPipe, 0x3770, 0x0B);
	imx347_write_register(ViPipe, 0x3776, 0x89);
	imx347_write_register(ViPipe, 0x3777, 0x00);
	imx347_write_register(ViPipe, 0x3778, 0xCA);
	imx347_write_register(ViPipe, 0x3779, 0x00);
	imx347_write_register(ViPipe, 0x377A, 0x45);
	imx347_write_register(ViPipe, 0x377B, 0x01);
	imx347_write_register(ViPipe, 0x377C, 0x56);
	imx347_write_register(ViPipe, 0x377D, 0x02);
	imx347_write_register(ViPipe, 0x377E, 0xFE);
	imx347_write_register(ViPipe, 0x377F, 0x03);
	imx347_write_register(ViPipe, 0x3780, 0xFE);
	imx347_write_register(ViPipe, 0x3781, 0x05);
	imx347_write_register(ViPipe, 0x3782, 0xFE);
	imx347_write_register(ViPipe, 0x3783, 0x06);
	imx347_write_register(ViPipe, 0x3784, 0x7F);
	imx347_write_register(ViPipe, 0x3788, 0x1F);
	imx347_write_register(ViPipe, 0x378A, 0xCA);
	imx347_write_register(ViPipe, 0x378B, 0x00);
	imx347_write_register(ViPipe, 0x378C, 0x45);
	imx347_write_register(ViPipe, 0x378D, 0x01);
	imx347_write_register(ViPipe, 0x378E, 0x56);
	imx347_write_register(ViPipe, 0x378F, 0x02);
	imx347_write_register(ViPipe, 0x3790, 0xFE);
	imx347_write_register(ViPipe, 0x3791, 0x03);
	imx347_write_register(ViPipe, 0x3792, 0xFE);
	imx347_write_register(ViPipe, 0x3793, 0x05);
	imx347_write_register(ViPipe, 0x3794, 0xFE);
	imx347_write_register(ViPipe, 0x3795, 0x06);
	imx347_write_register(ViPipe, 0x3796, 0x7F);
	imx347_write_register(ViPipe, 0x3798, 0xBF);

	// MIPI Timing
	imx347_write_register(ViPipe, 0x3A18, 0x8F);
	imx347_write_register(ViPipe, 0x3A1A, 0x4F);
	imx347_write_register(ViPipe, 0x3A1C, 0x47);
	imx347_write_register(ViPipe, 0x3A1E, 0x37);
	imx347_write_register(ViPipe, 0x3A1F, 0x01);
	imx347_write_register(ViPipe, 0x3A20, 0x4F);
	imx347_write_register(ViPipe, 0x3A22, 0x87);
	imx347_write_register(ViPipe, 0x3A24, 0x4F);
	imx347_write_register(ViPipe, 0x3A26, 0x7F);
	imx347_write_register(ViPipe, 0x3A28, 0x3F);

	imx347_default_reg_init(ViPipe);

	imx347_write_register(ViPipe, 0x3000, 0x00);	/* standby */
	delay_ms(18);
	imx347_write_register(ViPipe, 0x3002, 0x00);	/* master mode start */
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe:%d,===IMX347 4M 60fps 12bit LINE Init OK!===\n", ViPipe);
}

static void imx347_wdr_4M30_2to1_init(VI_PIPE ViPipe)
{
	//Reset
	imx347_write_register(ViPipe, 0x3000, 0x01); // STANDBY
	imx347_write_register(ViPipe, 0x3002, 0x01); // XTMSTA
	imx347_write_register(ViPipe, 0x3004, 0x00); // Reset
	delay_ms(1);

	imx347_write_register(ViPipe, 0x300C, 0x5B);
	imx347_write_register(ViPipe, 0x300D, 0x40);
	imx347_write_register(ViPipe, 0x3048, 0x01);
	imx347_write_register(ViPipe, 0x3049, 0x01);
	imx347_write_register(ViPipe, 0x304A, 0x04);
	imx347_write_register(ViPipe, 0x304B, 0x04);
	imx347_write_register(ViPipe, 0x304C, 0x13);

	imx347_write_register(ViPipe, 0x3050, 0x01);// AD bit= 0:10bit, 1:12bit
	imx347_write_register(ViPipe, 0x3058, 0x80);
	imx347_write_register(ViPipe, 0x3059, 0x0C);
	imx347_write_register(ViPipe, 0x305C, 0x09);
	imx347_write_register(ViPipe, 0x305D, 0x00);
	imx347_write_register(ViPipe, 0x3068, 0x15);
	imx347_write_register(ViPipe, 0x3069, 0x00);
	imx347_write_register(ViPipe, 0x306C, 0x00);
	imx347_write_register(ViPipe, 0x306D, 0x00);

	imx347_write_register(ViPipe, 0x30BE, 0x5E);
	imx347_write_register(ViPipe, 0x3110, 0x02);
	// imx347_write_register(ViPipe, 0x314C, 0xC0);
	imx347_write_register(ViPipe, 0x315A, 0x02);
	imx347_write_register(ViPipe, 0x316A, 0x7E);
	// imx347_write_register(ViPipe, 0x319D, 0x00);
	// imx347_write_register(ViPipe, 0x319E, 0x02);
	imx347_write_register(ViPipe, 0x31A1, 0x00);

	imx347_write_register(ViPipe, 0x31D7, 0x01);

	imx347_write_register(ViPipe, 0x3200, 0x00); // Set individual frame gain
	imx347_write_register(ViPipe, 0x3202, 0x02);
	imx347_write_register(ViPipe, 0x3288, 0x22);
	imx347_write_register(ViPipe, 0x328A, 0x02);
	imx347_write_register(ViPipe, 0x328C, 0xA2);
	imx347_write_register(ViPipe, 0x328E, 0x22);

	imx347_write_register(ViPipe, 0x3415, 0x27);
	imx347_write_register(ViPipe, 0x3418, 0x27);
	imx347_write_register(ViPipe, 0x3428, 0xFE);
	imx347_write_register(ViPipe, 0x349E, 0x6A);
	imx347_write_register(ViPipe, 0x34A2, 0x9A);
	imx347_write_register(ViPipe, 0x34A4, 0x8A);
	imx347_write_register(ViPipe, 0x34A6, 0x8E);
	imx347_write_register(ViPipe, 0x34AA, 0xD8);
	imx347_write_register(ViPipe, 0x3648, 0x01);
	imx347_write_register(ViPipe, 0x3678, 0x01);
	imx347_write_register(ViPipe, 0x367C, 0x69);
	imx347_write_register(ViPipe, 0x367E, 0x69);
	imx347_write_register(ViPipe, 0x3680, 0x69);
	imx347_write_register(ViPipe, 0x3682, 0x69);
	imx347_write_register(ViPipe, 0x371D, 0x05);
	imx347_write_register(ViPipe, 0x375D, 0x11);
	imx347_write_register(ViPipe, 0x375E, 0x43);
	imx347_write_register(ViPipe, 0x375F, 0x76);
	imx347_write_register(ViPipe, 0x3760, 0x07);
	imx347_write_register(ViPipe, 0x3768, 0x1B);
	imx347_write_register(ViPipe, 0x3769, 0x1B);
	imx347_write_register(ViPipe, 0x376A, 0x1A);
	imx347_write_register(ViPipe, 0x376B, 0x19);
	imx347_write_register(ViPipe, 0x376C, 0x17);
	imx347_write_register(ViPipe, 0x376D, 0x0F);
	imx347_write_register(ViPipe, 0x376E, 0x0B);
	imx347_write_register(ViPipe, 0x376F, 0x0B);
	imx347_write_register(ViPipe, 0x3770, 0x0B);
	imx347_write_register(ViPipe, 0x3776, 0x89);
	imx347_write_register(ViPipe, 0x3777, 0x00);
	imx347_write_register(ViPipe, 0x3778, 0xCA);
	imx347_write_register(ViPipe, 0x3779, 0x00);
	imx347_write_register(ViPipe, 0x377A, 0x45);
	imx347_write_register(ViPipe, 0x377B, 0x01);
	imx347_write_register(ViPipe, 0x377C, 0x56);
	imx347_write_register(ViPipe, 0x377D, 0x02);
	imx347_write_register(ViPipe, 0x377E, 0xFE);
	imx347_write_register(ViPipe, 0x377F, 0x03);
	imx347_write_register(ViPipe, 0x3780, 0xFE);
	imx347_write_register(ViPipe, 0x3781, 0x05);
	imx347_write_register(ViPipe, 0x3782, 0xFE);
	imx347_write_register(ViPipe, 0x3783, 0x06);
	imx347_write_register(ViPipe, 0x3784, 0x7F);
	imx347_write_register(ViPipe, 0x3788, 0x1F);
	imx347_write_register(ViPipe, 0x378A, 0xCA);
	imx347_write_register(ViPipe, 0x378B, 0x00);
	imx347_write_register(ViPipe, 0x378C, 0x45);
	imx347_write_register(ViPipe, 0x378D, 0x01);
	imx347_write_register(ViPipe, 0x378E, 0x56);
	imx347_write_register(ViPipe, 0x378F, 0x02);
	imx347_write_register(ViPipe, 0x3790, 0xFE);
	imx347_write_register(ViPipe, 0x3791, 0x03);
	imx347_write_register(ViPipe, 0x3792, 0xFE);
	imx347_write_register(ViPipe, 0x3793, 0x05);
	imx347_write_register(ViPipe, 0x3794, 0xFE);
	imx347_write_register(ViPipe, 0x3795, 0x06);
	imx347_write_register(ViPipe, 0x3796, 0x7F);
	imx347_write_register(ViPipe, 0x3798, 0xBF);

	// MIPI Timing
	imx347_write_register(ViPipe, 0x3A18, 0x8F);
	imx347_write_register(ViPipe, 0x3A1A, 0x4F);
	imx347_write_register(ViPipe, 0x3A1C, 0x47);
	imx347_write_register(ViPipe, 0x3A1E, 0x37);
	imx347_write_register(ViPipe, 0x3A1F, 0x01);
	imx347_write_register(ViPipe, 0x3A20, 0x4F);
	imx347_write_register(ViPipe, 0x3A22, 0x87);
	imx347_write_register(ViPipe, 0x3A24, 0x4F);
	imx347_write_register(ViPipe, 0x3A26, 0x7F);
	imx347_write_register(ViPipe, 0x3A28, 0x3F);

	imx347_default_reg_init(ViPipe);

	imx347_write_register(ViPipe, 0x3000, 0x00);	/* standby */
	delay_ms(18);
	imx347_write_register(ViPipe, 0x3002, 0x00);	/* master mode start */
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe:%d,===IMX347 4M 30fps 12bit 2to1 WDR Init OK!===\n", ViPipe);
}
