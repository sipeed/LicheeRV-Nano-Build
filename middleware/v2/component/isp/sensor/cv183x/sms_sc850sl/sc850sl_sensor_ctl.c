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
#include "sc850sl_cmos_ex.h"

#define SC850SL_CHIP_ID_HI_ADDR		0x3107
#define SC850SL_CHIP_ID_LO_ADDR		0x3108
#define SC850SL_CHIP_ID			0x9d1e

static void sc850sl_linear_2160P30_init(VI_PIPE ViPipe);
static void sc850sl_wdr_2160P30_2to1_init(VI_PIPE ViPipe);

CVI_U8 sc850sl_i2c_addr = 0x30;		/* I2C Address of SC850SL */
const CVI_U32 sc850sl_addr_byte = 2;
const CVI_U32 sc850sl_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int sc850sl_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunSC850SL_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile), "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, sc850sl_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int sc850sl_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int sc850sl_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (sc850sl_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, sc850sl_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, sc850sl_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (sc850sl_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int sc850sl_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (sc850sl_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (sc850sl_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, sc850sl_addr_byte + sc850sl_data_byte);
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

void sc850sl_standby(VI_PIPE ViPipe)
{
	sc850sl_write_register(ViPipe, 0x0100, 0x00);
}

void sc850sl_restart(VI_PIPE ViPipe)
{
	sc850sl_write_register(ViPipe, 0x0100, 0x00);
	delay_ms(20);
	sc850sl_write_register(ViPipe, 0x0100, 0x01);
}

void sc850sl_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastSC850SL[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sc850sl_write_register(ViPipe,
				g_pastSC850SL[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastSC850SL[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void sc850sl_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = 0;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		val |= 0x6;
		break;
	case ISP_SNS_FLIP:
		val |= 0x60;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val |= 0x66;
		break;
	default:
		return;
	}

	sc850sl_write_register(ViPipe, 0x3221, val);
}

int sc850sl_probe(VI_PIPE ViPipe)
{
	int nVal;
	CVI_U16 chip_id;

	delay_ms(4);
	if (sc850sl_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal = sc850sl_read_register(ViPipe, SC850SL_CHIP_ID_HI_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id = (nVal & 0xFF) << 8;
	nVal = sc850sl_read_register(ViPipe, SC850SL_CHIP_ID_LO_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id |= (nVal & 0xFF);

	if (chip_id != SC850SL_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}


void sc850sl_init(VI_PIPE ViPipe)
{
	WDR_MODE_E enWDRMode;
	CVI_BOOL bInit;
	CVI_U8 u8ImgMode;

	bInit		= g_pastSC850SL[ViPipe]->bInit;
	enWDRMode	= g_pastSC850SL[ViPipe]->enWDRMode;
	u8ImgMode	= g_pastSC850SL[ViPipe]->u8ImgMode;

	sc850sl_i2c_init(ViPipe);

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == SC850SL_MODE_2160P30_WDR) {
				/* SC850SL_MODE_2160P30_WDR */
				sc850sl_wdr_2160P30_2to1_init(ViPipe);
			} else {
			}
		} else {
			sc850sl_linear_2160P30_init(ViPipe);
		}
	}
	/* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
	else {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == SC850SL_MODE_2160P30_WDR) {
				/* SC850SL_MODE_2160P30_WDR */
				sc850sl_wdr_2160P30_2to1_init(ViPipe);
			} else {
			}
		} else {
			sc850sl_linear_2160P30_init(ViPipe);
		}
	}
	g_pastSC850SL[ViPipe]->bInit = CVI_TRUE;
}

void sc850sl_exit(VI_PIPE ViPipe)
{
	sc850sl_i2c_exit(ViPipe);
}

static void sc850sl_linear_2160P30_init(VI_PIPE ViPipe)
{
	sc850sl_write_register(ViPipe, 0x0103, 0x01);
	sc850sl_write_register(ViPipe, 0x0100, 0x00);
	sc850sl_write_register(ViPipe, 0x36e9, 0x80);
	sc850sl_write_register(ViPipe, 0x36f9, 0x80);
	sc850sl_write_register(ViPipe, 0x36ea, 0x08);
	sc850sl_write_register(ViPipe, 0x36eb, 0x0c);
	sc850sl_write_register(ViPipe, 0x36ec, 0x4a);
	sc850sl_write_register(ViPipe, 0x36ed, 0x24);
	sc850sl_write_register(ViPipe, 0x36fa, 0x16);
	sc850sl_write_register(ViPipe, 0x36fb, 0x33);
	sc850sl_write_register(ViPipe, 0x36fc, 0x10);
	sc850sl_write_register(ViPipe, 0x36fd, 0x17);
	sc850sl_write_register(ViPipe, 0x36e9, 0x00);
	sc850sl_write_register(ViPipe, 0x36f9, 0x54);
	sc850sl_write_register(ViPipe, 0x3018, 0x7a);
	sc850sl_write_register(ViPipe, 0x3019, 0xf0);
	sc850sl_write_register(ViPipe, 0x301e, 0x3c);
	sc850sl_write_register(ViPipe, 0x301f, 0x01);
	sc850sl_write_register(ViPipe, 0x302a, 0x00);
	sc850sl_write_register(ViPipe, 0x3031, 0x0c);
	sc850sl_write_register(ViPipe, 0x3037, 0x00);
	sc850sl_write_register(ViPipe, 0x320c, 0x04);
	sc850sl_write_register(ViPipe, 0x320d, 0x4c);
	sc850sl_write_register(ViPipe, 0x3250, 0x40);
	sc850sl_write_register(ViPipe, 0x3301, 0x3c);
	sc850sl_write_register(ViPipe, 0x3306, 0xa0);
	sc850sl_write_register(ViPipe, 0x330a, 0x01);
	sc850sl_write_register(ViPipe, 0x330b, 0xa0);
	sc850sl_write_register(ViPipe, 0x3333, 0x10);
	sc850sl_write_register(ViPipe, 0x3362, 0x70);
	sc850sl_write_register(ViPipe, 0x3616, 0x0c);
	sc850sl_write_register(ViPipe, 0x3622, 0x04);
	sc850sl_write_register(ViPipe, 0x3629, 0x04);
	sc850sl_write_register(ViPipe, 0x362b, 0x03);
	sc850sl_write_register(ViPipe, 0x362d, 0x00);
	sc850sl_write_register(ViPipe, 0x3630, 0x68);
	sc850sl_write_register(ViPipe, 0x3633, 0x45);
	sc850sl_write_register(ViPipe, 0x3634, 0x22);
	sc850sl_write_register(ViPipe, 0x3635, 0x40);
	sc850sl_write_register(ViPipe, 0x3637, 0x06);
	sc850sl_write_register(ViPipe, 0x363b, 0x02);
	sc850sl_write_register(ViPipe, 0x363c, 0x07);
	sc850sl_write_register(ViPipe, 0x363d, 0x05);
	sc850sl_write_register(ViPipe, 0x363e, 0x8f);
	sc850sl_write_register(ViPipe, 0x364a, 0x86);
	sc850sl_write_register(ViPipe, 0x364c, 0x2a);
	sc850sl_write_register(ViPipe, 0x3654, 0x40);
	sc850sl_write_register(ViPipe, 0x365c, 0x40);
	sc850sl_write_register(ViPipe, 0x36bb, 0x08);
	sc850sl_write_register(ViPipe, 0x36bc, 0x40);
	sc850sl_write_register(ViPipe, 0x36c9, 0x04);
	sc850sl_write_register(ViPipe, 0x36ca, 0x07);
	sc850sl_write_register(ViPipe, 0x36cb, 0x07);
	sc850sl_write_register(ViPipe, 0x36d0, 0x10);
	sc850sl_write_register(ViPipe, 0x3901, 0x04);
	sc850sl_write_register(ViPipe, 0x3904, 0x20);
	sc850sl_write_register(ViPipe, 0x3905, 0x91);
	sc850sl_write_register(ViPipe, 0x3928, 0x04);
	sc850sl_write_register(ViPipe, 0x3946, 0x20);
	sc850sl_write_register(ViPipe, 0x3961, 0x40);
	sc850sl_write_register(ViPipe, 0x3962, 0x40);
	sc850sl_write_register(ViPipe, 0x3963, 0xc8);
	sc850sl_write_register(ViPipe, 0x3964, 0xc8);
	sc850sl_write_register(ViPipe, 0x3965, 0x40);
	sc850sl_write_register(ViPipe, 0x3966, 0x40);
	sc850sl_write_register(ViPipe, 0x3967, 0x00);
	sc850sl_write_register(ViPipe, 0x39cd, 0xc8);
	sc850sl_write_register(ViPipe, 0x39ce, 0xc8);
	sc850sl_write_register(ViPipe, 0x3e01, 0x82);
	sc850sl_write_register(ViPipe, 0x3e02, 0x00);
	sc850sl_write_register(ViPipe, 0x3e04, 0x8c);
	sc850sl_write_register(ViPipe, 0x3e05, 0x60);
	sc850sl_write_register(ViPipe, 0x3e06, 0x00);
	sc850sl_write_register(ViPipe, 0x3e07, 0x80);
	sc850sl_write_register(ViPipe, 0x3e08, 0x03);
	sc850sl_write_register(ViPipe, 0x3e09, 0x40);
	sc850sl_write_register(ViPipe, 0x3e1c, 0x0f);
	sc850sl_write_register(ViPipe, 0x3e51, 0x8c);
	sc850sl_write_register(ViPipe, 0x3e52, 0x60);
	sc850sl_write_register(ViPipe, 0x3e58, 0x03);
	sc850sl_write_register(ViPipe, 0x3e59, 0x40);
	sc850sl_write_register(ViPipe, 0x3e66, 0x00);
	sc850sl_write_register(ViPipe, 0x3e67, 0x80);
	sc850sl_write_register(ViPipe, 0x3e68, 0x00);
	sc850sl_write_register(ViPipe, 0x3e69, 0x80);
	sc850sl_write_register(ViPipe, 0x3e6a, 0x00);
	sc850sl_write_register(ViPipe, 0x3e6b, 0x80);
	sc850sl_write_register(ViPipe, 0x3e71, 0x8c);
	sc850sl_write_register(ViPipe, 0x3e72, 0x60);
	sc850sl_write_register(ViPipe, 0x3e82, 0x03);
	sc850sl_write_register(ViPipe, 0x3e83, 0x40);
	sc850sl_write_register(ViPipe, 0x3e86, 0x03);
	sc850sl_write_register(ViPipe, 0x3e87, 0x40);
	sc850sl_write_register(ViPipe, 0x4424, 0x02);
	sc850sl_write_register(ViPipe, 0x4501, 0xc4);
	sc850sl_write_register(ViPipe, 0x4509, 0x20);
	sc850sl_write_register(ViPipe, 0x4561, 0x12);
	sc850sl_write_register(ViPipe, 0x4800, 0x24);
	sc850sl_write_register(ViPipe, 0x4837, 0x0e);
	sc850sl_write_register(ViPipe, 0x4900, 0x24);
	sc850sl_write_register(ViPipe, 0x4937, 0x0e);
	sc850sl_write_register(ViPipe, 0x5000, 0x0e);
	sc850sl_write_register(ViPipe, 0x500f, 0x35);
	sc850sl_write_register(ViPipe, 0x5020, 0x00);
	sc850sl_write_register(ViPipe, 0x5787, 0x10);
	sc850sl_write_register(ViPipe, 0x5788, 0x06);
	sc850sl_write_register(ViPipe, 0x5789, 0x00);
	sc850sl_write_register(ViPipe, 0x578a, 0x18);
	sc850sl_write_register(ViPipe, 0x578b, 0x0c);
	sc850sl_write_register(ViPipe, 0x578c, 0x00);
	sc850sl_write_register(ViPipe, 0x5790, 0x10);
	sc850sl_write_register(ViPipe, 0x5791, 0x06);
	sc850sl_write_register(ViPipe, 0x5792, 0x01);
	sc850sl_write_register(ViPipe, 0x5793, 0x18);
	sc850sl_write_register(ViPipe, 0x5794, 0x0c);
	sc850sl_write_register(ViPipe, 0x5795, 0x01);
	sc850sl_write_register(ViPipe, 0x5799, 0x06);
	sc850sl_write_register(ViPipe, 0x59e0, 0xfe);
	sc850sl_write_register(ViPipe, 0x59e1, 0x40);
	sc850sl_write_register(ViPipe, 0x59e2, 0x38);
	sc850sl_write_register(ViPipe, 0x59e3, 0x30);
	sc850sl_write_register(ViPipe, 0x59e4, 0x20);
	sc850sl_write_register(ViPipe, 0x59e5, 0x38);
	sc850sl_write_register(ViPipe, 0x59e6, 0x30);
	sc850sl_write_register(ViPipe, 0x59e7, 0x20);
	sc850sl_write_register(ViPipe, 0x59e8, 0x3f);
	sc850sl_write_register(ViPipe, 0x59e9, 0x38);
	sc850sl_write_register(ViPipe, 0x59ea, 0x30);
	sc850sl_write_register(ViPipe, 0x59eb, 0x3f);
	sc850sl_write_register(ViPipe, 0x59ec, 0x38);
	sc850sl_write_register(ViPipe, 0x59ed, 0x30);
	sc850sl_write_register(ViPipe, 0x59ee, 0xfe);
	sc850sl_write_register(ViPipe, 0x59ef, 0x40);
	sc850sl_write_register(ViPipe, 0x59f4, 0x38);
	sc850sl_write_register(ViPipe, 0x59f5, 0x30);
	sc850sl_write_register(ViPipe, 0x59f6, 0x20);
	sc850sl_write_register(ViPipe, 0x59f7, 0x38);
	sc850sl_write_register(ViPipe, 0x59f8, 0x30);
	sc850sl_write_register(ViPipe, 0x59f9, 0x20);
	sc850sl_write_register(ViPipe, 0x59fa, 0x3f);
	sc850sl_write_register(ViPipe, 0x59fb, 0x38);
	sc850sl_write_register(ViPipe, 0x59fc, 0x30);
	sc850sl_write_register(ViPipe, 0x59fd, 0x3f);
	sc850sl_write_register(ViPipe, 0x59fe, 0x38);
	sc850sl_write_register(ViPipe, 0x59ff, 0x30);
	sc850sl_write_register(ViPipe, 0x3648, 0xe0);

	sc850sl_default_reg_init(ViPipe);

	sc850sl_write_register(ViPipe, 0x0100, 0x01);

	printf("ViPipe:%d,===SC850SL 2160P 30fps 12bit LINE Init OK!===\n", ViPipe);
}

void sc850sl_wdr_2160P30_2to1_init(VI_PIPE ViPipe)
{
	sc850sl_write_register(ViPipe, 0x0103, 0x01);
	sc850sl_write_register(ViPipe, 0x0100, 0x00);
	sc850sl_write_register(ViPipe, 0x36e9, 0x80);
	sc850sl_write_register(ViPipe, 0x36f9, 0x80);
	sc850sl_write_register(ViPipe, 0x36ea, 0x09);
	sc850sl_write_register(ViPipe, 0x36eb, 0x0c);
	sc850sl_write_register(ViPipe, 0x36ec, 0x4a);
	sc850sl_write_register(ViPipe, 0x36ed, 0x34);
	sc850sl_write_register(ViPipe, 0x36fa, 0x09);
	sc850sl_write_register(ViPipe, 0x36fb, 0x31);
	sc850sl_write_register(ViPipe, 0x36fc, 0x10);
	sc850sl_write_register(ViPipe, 0x36fd, 0x17);
	sc850sl_write_register(ViPipe, 0x36e9, 0x24);
	sc850sl_write_register(ViPipe, 0x36f9, 0x24);
	sc850sl_write_register(ViPipe, 0x3018, 0x7a);
	sc850sl_write_register(ViPipe, 0x3019, 0xf0);
	sc850sl_write_register(ViPipe, 0x301a, 0x30);
	sc850sl_write_register(ViPipe, 0x301e, 0x3c);
	sc850sl_write_register(ViPipe, 0x301f, 0x06);
	sc850sl_write_register(ViPipe, 0x302a, 0x00);
	sc850sl_write_register(ViPipe, 0x3031, 0x0a);
	sc850sl_write_register(ViPipe, 0x3032, 0x20);
	sc850sl_write_register(ViPipe, 0x3033, 0x22);
	sc850sl_write_register(ViPipe, 0x3037, 0x00);
	sc850sl_write_register(ViPipe, 0x303e, 0xb4);
	sc850sl_write_register(ViPipe, 0x320c, 0x03);
	sc850sl_write_register(ViPipe, 0x320d, 0x84);
	sc850sl_write_register(ViPipe, 0x320e, 0x11);// vts
	sc850sl_write_register(ViPipe, 0x320f, 0x94);
	sc850sl_write_register(ViPipe, 0x3226, 0x10);
	sc850sl_write_register(ViPipe, 0x3227, 0x03);
	sc850sl_write_register(ViPipe, 0x3250, 0xff);
	sc850sl_write_register(ViPipe, 0x327e, 0x00);
	sc850sl_write_register(ViPipe, 0x3280, 0x00);
	sc850sl_write_register(ViPipe, 0x3281, 0x01);// Row overlap HDR enable
	sc850sl_write_register(ViPipe, 0x3301, 0x28);
	sc850sl_write_register(ViPipe, 0x3304, 0x38);
	sc850sl_write_register(ViPipe, 0x3306, 0xb0);
	sc850sl_write_register(ViPipe, 0x3309, 0x58);
	sc850sl_write_register(ViPipe, 0x330a, 0x01);
	sc850sl_write_register(ViPipe, 0x330b, 0x3a);
	sc850sl_write_register(ViPipe, 0x3314, 0x92);
	sc850sl_write_register(ViPipe, 0x331e, 0x31);
	sc850sl_write_register(ViPipe, 0x331f, 0x51);
	sc850sl_write_register(ViPipe, 0x3333, 0x10);
	sc850sl_write_register(ViPipe, 0x3348, 0x50);
	sc850sl_write_register(ViPipe, 0x335d, 0x60);
	sc850sl_write_register(ViPipe, 0x3362, 0x70);
	sc850sl_write_register(ViPipe, 0x33fe, 0x02);
	sc850sl_write_register(ViPipe, 0x3400, 0x12);
	sc850sl_write_register(ViPipe, 0x3406, 0x04);
	sc850sl_write_register(ViPipe, 0x3410, 0x12);
	sc850sl_write_register(ViPipe, 0x3416, 0x06);
	sc850sl_write_register(ViPipe, 0x3433, 0x01);
	sc850sl_write_register(ViPipe, 0x3440, 0x12);
	sc850sl_write_register(ViPipe, 0x3446, 0x08);
	sc850sl_write_register(ViPipe, 0x3478, 0x01);
	sc850sl_write_register(ViPipe, 0x3479, 0x01);
	sc850sl_write_register(ViPipe, 0x347a, 0x02);
	sc850sl_write_register(ViPipe, 0x347b, 0x01);
	sc850sl_write_register(ViPipe, 0x347c, 0x02);
	sc850sl_write_register(ViPipe, 0x347d, 0x01);
	sc850sl_write_register(ViPipe, 0x3616, 0x0c);
	sc850sl_write_register(ViPipe, 0x3622, 0x04);
	sc850sl_write_register(ViPipe, 0x3629, 0x04);
	sc850sl_write_register(ViPipe, 0x362b, 0x0f);
	sc850sl_write_register(ViPipe, 0x362d, 0x00);
	sc850sl_write_register(ViPipe, 0x3630, 0x68);
	sc850sl_write_register(ViPipe, 0x3633, 0x46);
	sc850sl_write_register(ViPipe, 0x3634, 0x22);
	sc850sl_write_register(ViPipe, 0x3635, 0x40);
	sc850sl_write_register(ViPipe, 0x3637, 0x18);
	sc850sl_write_register(ViPipe, 0x3638, 0x1e);
	sc850sl_write_register(ViPipe, 0x363b, 0x02);
	sc850sl_write_register(ViPipe, 0x363c, 0x04);
	sc850sl_write_register(ViPipe, 0x363d, 0x04);
	sc850sl_write_register(ViPipe, 0x363e, 0x8f);
	sc850sl_write_register(ViPipe, 0x364a, 0x86);
	sc850sl_write_register(ViPipe, 0x364c, 0x3e);
	sc850sl_write_register(ViPipe, 0x3650, 0x3d);
	sc850sl_write_register(ViPipe, 0x3654, 0x40);
	sc850sl_write_register(ViPipe, 0x3656, 0x68);
	sc850sl_write_register(ViPipe, 0x3657, 0x0f);
	sc850sl_write_register(ViPipe, 0x3658, 0x3d);
	sc850sl_write_register(ViPipe, 0x365c, 0x40);
	sc850sl_write_register(ViPipe, 0x365e, 0x68);
	sc850sl_write_register(ViPipe, 0x36bb, 0x08);
	sc850sl_write_register(ViPipe, 0x36bc, 0x40);
	sc850sl_write_register(ViPipe, 0x36c9, 0x04);
	sc850sl_write_register(ViPipe, 0x36ca, 0x07);
	sc850sl_write_register(ViPipe, 0x36cb, 0x07);
	sc850sl_write_register(ViPipe, 0x36d0, 0x10);
	sc850sl_write_register(ViPipe, 0x3901, 0x04);
	sc850sl_write_register(ViPipe, 0x3904, 0x20);
	sc850sl_write_register(ViPipe, 0x3905, 0x91);
	sc850sl_write_register(ViPipe, 0x391e, 0x03);
	sc850sl_write_register(ViPipe, 0x3928, 0x04);
	sc850sl_write_register(ViPipe, 0x3933, 0xa0);
	sc850sl_write_register(ViPipe, 0x3937, 0x20);
	sc850sl_write_register(ViPipe, 0x3946, 0x20);
	sc850sl_write_register(ViPipe, 0x3961, 0x40);
	sc850sl_write_register(ViPipe, 0x3962, 0x40);
	sc850sl_write_register(ViPipe, 0x3963, 0xc8);
	sc850sl_write_register(ViPipe, 0x3964, 0xc8);
	sc850sl_write_register(ViPipe, 0x3965, 0x40);
	sc850sl_write_register(ViPipe, 0x3966, 0x40);
	sc850sl_write_register(ViPipe, 0x3967, 0xe4);
	sc850sl_write_register(ViPipe, 0x39cd, 0xc8);
	sc850sl_write_register(ViPipe, 0x39ce, 0xc8);
	sc850sl_write_register(ViPipe, 0x3e00, 0x01);
	sc850sl_write_register(ViPipe, 0x3e01, 0x00);
	sc850sl_write_register(ViPipe, 0x3e02, 0x00);
	sc850sl_write_register(ViPipe, 0x3e04, 0x10);
	sc850sl_write_register(ViPipe, 0x3e05, 0x00);
	sc850sl_write_register(ViPipe, 0x3e06, 0x00);
	sc850sl_write_register(ViPipe, 0x3e07, 0x80);
	sc850sl_write_register(ViPipe, 0x3e08, 0x03);
	sc850sl_write_register(ViPipe, 0x3e09, 0x40);
	sc850sl_write_register(ViPipe, 0x3e0e, 0x02);
	sc850sl_write_register(ViPipe, 0x3e0f, 0x00);
	sc850sl_write_register(ViPipe, 0x3e1c, 0x0f);
	sc850sl_write_register(ViPipe, 0x3e23, 0x01);// max_sexp
	sc850sl_write_register(ViPipe, 0x3e24, 0x0a);
	sc850sl_write_register(ViPipe, 0x3e51, 0x8c);
	sc850sl_write_register(ViPipe, 0x3e52, 0x60);
	sc850sl_write_register(ViPipe, 0x3e53, 0x00);
	sc850sl_write_register(ViPipe, 0x3e54, 0x00);
	sc850sl_write_register(ViPipe, 0x3e58, 0x03);
	sc850sl_write_register(ViPipe, 0x3e59, 0x40);
	sc850sl_write_register(ViPipe, 0x3e66, 0x00);
	sc850sl_write_register(ViPipe, 0x3e67, 0x80);
	sc850sl_write_register(ViPipe, 0x3e68, 0x00);
	sc850sl_write_register(ViPipe, 0x3e69, 0x80);
	sc850sl_write_register(ViPipe, 0x3e6a, 0x00);
	sc850sl_write_register(ViPipe, 0x3e6b, 0x80);
	sc850sl_write_register(ViPipe, 0x3e71, 0x8c);
	sc850sl_write_register(ViPipe, 0x3e72, 0x60);
	sc850sl_write_register(ViPipe, 0x3e73, 0x00);
	sc850sl_write_register(ViPipe, 0x3e74, 0x00);
	sc850sl_write_register(ViPipe, 0x3e82, 0x03);
	sc850sl_write_register(ViPipe, 0x3e83, 0x40);
	sc850sl_write_register(ViPipe, 0x3e86, 0x03);
	sc850sl_write_register(ViPipe, 0x3e87, 0x40);
	sc850sl_write_register(ViPipe, 0x4424, 0x02);
	sc850sl_write_register(ViPipe, 0x4501, 0xb4);
	sc850sl_write_register(ViPipe, 0x4509, 0x20);
	sc850sl_write_register(ViPipe, 0x4561, 0x12);
	sc850sl_write_register(ViPipe, 0x4800, 0x24);
	sc850sl_write_register(ViPipe, 0x4837, 0x0b);
	sc850sl_write_register(ViPipe, 0x4853, 0xf8);
	sc850sl_write_register(ViPipe, 0x4900, 0x24);
	sc850sl_write_register(ViPipe, 0x4937, 0x0b);
	sc850sl_write_register(ViPipe, 0x4953, 0xf8);
	sc850sl_write_register(ViPipe, 0x5000, 0x0e);
	sc850sl_write_register(ViPipe, 0x500f, 0x35);
	sc850sl_write_register(ViPipe, 0x5020, 0x00);
	sc850sl_write_register(ViPipe, 0x5787, 0x10);
	sc850sl_write_register(ViPipe, 0x5788, 0x06);
	sc850sl_write_register(ViPipe, 0x5789, 0x00);
	sc850sl_write_register(ViPipe, 0x578a, 0x18);
	sc850sl_write_register(ViPipe, 0x578b, 0x0c);
	sc850sl_write_register(ViPipe, 0x578c, 0x00);
	sc850sl_write_register(ViPipe, 0x5790, 0x10);
	sc850sl_write_register(ViPipe, 0x5791, 0x06);
	sc850sl_write_register(ViPipe, 0x5792, 0x01);
	sc850sl_write_register(ViPipe, 0x5793, 0x18);
	sc850sl_write_register(ViPipe, 0x5794, 0x0c);
	sc850sl_write_register(ViPipe, 0x5795, 0x01);
	sc850sl_write_register(ViPipe, 0x5799, 0x06);
	sc850sl_write_register(ViPipe, 0x57a2, 0x60);
	sc850sl_write_register(ViPipe, 0x59e0, 0xfe);
	sc850sl_write_register(ViPipe, 0x59e1, 0x40);
	sc850sl_write_register(ViPipe, 0x59e2, 0x38);
	sc850sl_write_register(ViPipe, 0x59e3, 0x30);
	sc850sl_write_register(ViPipe, 0x59e4, 0x20);
	sc850sl_write_register(ViPipe, 0x59e5, 0x38);
	sc850sl_write_register(ViPipe, 0x59e6, 0x30);
	sc850sl_write_register(ViPipe, 0x59e7, 0x20);
	sc850sl_write_register(ViPipe, 0x59e8, 0x3f);
	sc850sl_write_register(ViPipe, 0x59e9, 0x38);
	sc850sl_write_register(ViPipe, 0x59ea, 0x30);
	sc850sl_write_register(ViPipe, 0x59eb, 0x3f);
	sc850sl_write_register(ViPipe, 0x59ec, 0x38);
	sc850sl_write_register(ViPipe, 0x59ed, 0x30);
	sc850sl_write_register(ViPipe, 0x59ee, 0xfe);
	sc850sl_write_register(ViPipe, 0x59ef, 0x40);
	sc850sl_write_register(ViPipe, 0x59f4, 0x38);
	sc850sl_write_register(ViPipe, 0x59f5, 0x30);
	sc850sl_write_register(ViPipe, 0x59f6, 0x20);
	sc850sl_write_register(ViPipe, 0x59f7, 0x38);
	sc850sl_write_register(ViPipe, 0x59f8, 0x30);
	sc850sl_write_register(ViPipe, 0x59f9, 0x20);
	sc850sl_write_register(ViPipe, 0x59fa, 0x3f);
	sc850sl_write_register(ViPipe, 0x59fb, 0x38);
	sc850sl_write_register(ViPipe, 0x59fc, 0x30);
	sc850sl_write_register(ViPipe, 0x59fd, 0x3f);
	sc850sl_write_register(ViPipe, 0x59fe, 0x38);
	sc850sl_write_register(ViPipe, 0x59ff, 0x30);
	sc850sl_write_register(ViPipe, 0x3648, 0xe0);

	sc850sl_default_reg_init(ViPipe);

	sc850sl_write_register(ViPipe, 0x0100, 0x01);
	printf("ViPipe:%d,===SC850SL 2160P 30fps 10bit WDR Init OK!===\n", ViPipe);
}



