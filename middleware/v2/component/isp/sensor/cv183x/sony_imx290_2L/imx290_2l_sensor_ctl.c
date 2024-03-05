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
#include "imx290_2l_cmos_ex.h"

static void imx290_2l_wdr_720p15_2to1_init(VI_PIPE ViPipe);
static void imx290_2l_linear_720p30_init(VI_PIPE ViPipe);

const CVI_U8 imx290_2l_i2c_addr = 0x1A;        /* I2C Address of IMX290 */
const CVI_U32 imx290_2l_addr_byte = 2;
const CVI_U32 imx290_2l_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int imx290_2l_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunImx290_2l_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, imx290_2l_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int imx290_2l_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int imx290_2l_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (imx290_2l_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, imx290_2l_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, imx290_2l_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (imx290_2l_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}


int imx290_2l_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (imx290_2l_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	} else {
		/*buf[idx] = addr & 0xff;*/
		/*idx++;*/
	}

	if (imx290_2l_data_byte == 2) {
		/*buf[idx] = (data >> 8) & 0xff;*/
		/*idx++;*/
		/*buf[idx] = data & 0xff;*/
		/*idx++;*/
	} else {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, imx290_2l_addr_byte + imx290_2l_data_byte);
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

void imx290_2l_standby(VI_PIPE ViPipe)
{
	imx290_2l_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx290_2l_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
}

void imx290_2l_restart(VI_PIPE ViPipe)
{
	imx290_2l_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx290_2l_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	imx290_2l_write_register(ViPipe, 0x304b, 0x0a);
}

void imx290_2l_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx290_2l[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		imx290_2l_write_register(ViPipe,
				g_pastImx290_2l[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx290_2l[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

#define IMX290_CHIP_ID_ADDR	0x31dc
#define IMX290_CHIP_ID		0
#define IMX290_CHIP_ID_MASK	0x7

void imx290_2l_init(VI_PIPE ViPipe)
{
	WDR_MODE_E       enWDRMode;
	CVI_BOOL          bInit;
	CVI_U8            u8ImgMode;

	bInit       = g_pastImx290_2l[ViPipe]->bInit;
	enWDRMode   = g_pastImx290_2l[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastImx290_2l[ViPipe]->u8ImgMode;

	imx290_2l_i2c_init(ViPipe);

	if ((imx290_2l_read_register(ViPipe, IMX290_CHIP_ID_ADDR) & IMX290_CHIP_ID_MASK) != IMX290_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return;
	}

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == IMX290_2L_MODE_720P15_WDR) {
				/* IMX290_2L_MODE_720P15_WDR */
				imx290_2l_wdr_720p15_2to1_init(ViPipe);
			} else {
			}
		} else {
			imx290_2l_linear_720p30_init(ViPipe);
		}
	}
	/* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
	else {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == IMX290_2L_MODE_720P15_WDR) {
				/* IMX290_2L_MODE_720P15_WDR */
				imx290_2l_wdr_720p15_2to1_init(ViPipe);
			} else {
			}
		} else {
			imx290_2l_linear_720p30_init(ViPipe);
		}
	}
	g_pastImx290_2l[ViPipe]->bInit = CVI_TRUE;
}

void imx290_2l_exit(VI_PIPE ViPipe)
{
	imx290_2l_i2c_exit(ViPipe);
}

/* 720P30 and 720P25 */
static void imx290_2l_linear_720p30_init(VI_PIPE ViPipe)
{
	imx290_2l_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	delay_ms(4);
	imx290_2l_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx290_2l_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */

	imx290_2l_write_register(ViPipe, 0x3005, 0x00); /* ADBIT 10bit */
	imx290_2l_write_register(ViPipe, 0x3007, 0x10); /* VREVERS, */
	imx290_2l_write_register(ViPipe, 0x3009, 0x02);
	imx290_2l_write_register(ViPipe, 0x300A, 0x3C); /* BLKLEVEL */
	imx290_2l_write_register(ViPipe, 0x300C, 0x00);	/* WDMODE [0] ,WDSEL [5:4] */
	imx290_2l_write_register(ViPipe, 0x300F, 0x00);
	imx290_2l_write_register(ViPipe, 0x3010, 0x21);
	imx290_2l_write_register(ViPipe, 0x3012, 0x64);
	imx290_2l_write_register(ViPipe, 0x3014, 0x2A); /* GAIN, 0x2A=>12.6dB TBD */
	imx290_2l_write_register(ViPipe, 0x3016, 0x09);
	imx290_2l_write_register(ViPipe, 0x3018, 0xEE);	/* VMAX[7:0] */
	imx290_2l_write_register(ViPipe, 0x3019, 0x02);	/* VMAX[15:8] */
	imx290_2l_write_register(ViPipe, 0x301A, 0x00);	/* VMAX[17:16]:=:0x301A[1:0] */
	imx290_2l_write_register(ViPipe, 0x301C, 0xC8); /* HMAX[7:0], TBD */
	imx290_2l_write_register(ViPipe, 0x301D, 0x19); /* HMAX[15:8] */
	imx290_2l_write_register(ViPipe, 0x3045, 0x00);	/* DOLSCDEN [0], DOLSYDINFOEN [1], HINFOEN [2]*/
	imx290_2l_write_register(ViPipe, 0x3046, 0xD0); /* Lane number D-2, E-4, F-8 */
	imx290_2l_write_register(ViPipe, 0x304B, 0x0A);
	imx290_2l_write_register(ViPipe, 0x305C, 0x20); /* INCKSEL1 */
	imx290_2l_write_register(ViPipe, 0x305D, 0x00); /* INCKSEL2 */
	imx290_2l_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3 */
	imx290_2l_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4 */
	imx290_2l_write_register(ViPipe, 0x3070, 0x02);
	imx290_2l_write_register(ViPipe, 0x3071, 0x11);
	imx290_2l_write_register(ViPipe, 0x309B, 0x10);
	imx290_2l_write_register(ViPipe, 0x309C, 0x22);
	imx290_2l_write_register(ViPipe, 0x30A2, 0x02);
	imx290_2l_write_register(ViPipe, 0x30A6, 0x20);
	imx290_2l_write_register(ViPipe, 0x30A8, 0x20);
	imx290_2l_write_register(ViPipe, 0x30AA, 0x20);
	imx290_2l_write_register(ViPipe, 0x30AC, 0x20);
	imx290_2l_write_register(ViPipe, 0x30B0, 0x43);
	imx290_2l_write_register(ViPipe, 0x30F0, 0x64); /* F0: normal, 64: each frame gain*/
	imx290_2l_write_register(ViPipe, 0x3106, 0x00);	/* DOLHBFIXEN [7], DOLHBFIXEN [1:0],  DOLHBFIXEN [5:4] */
	imx290_2l_write_register(ViPipe, 0x3119, 0x9E);
	imx290_2l_write_register(ViPipe, 0x311C, 0x1E);
	imx290_2l_write_register(ViPipe, 0x311E, 0x08);
	imx290_2l_write_register(ViPipe, 0x3128, 0x05);
	imx290_2l_write_register(ViPipe, 0x3129, 0x1D);
	imx290_2l_write_register(ViPipe, 0x313D, 0x83);
	imx290_2l_write_register(ViPipe, 0x3150, 0x03);
	imx290_2l_write_register(ViPipe, 0x315E, 0x1A);
	imx290_2l_write_register(ViPipe, 0x3164, 0x1A);
	imx290_2l_write_register(ViPipe, 0x317C, 0x12);
	imx290_2l_write_register(ViPipe, 0x317E, 0x00);
	imx290_2l_write_register(ViPipe, 0x31EC, 0x37);
	imx290_2l_write_register(ViPipe, 0x32B8, 0x50);
	imx290_2l_write_register(ViPipe, 0x32B9, 0x10);
	imx290_2l_write_register(ViPipe, 0x32BA, 0x00);
	imx290_2l_write_register(ViPipe, 0x32BB, 0x04);
	imx290_2l_write_register(ViPipe, 0x32C8, 0x50);
	imx290_2l_write_register(ViPipe, 0x32C9, 0x10);
	imx290_2l_write_register(ViPipe, 0x32CA, 0x00);
	imx290_2l_write_register(ViPipe, 0x32CB, 0x04);
	imx290_2l_write_register(ViPipe, 0x332C, 0xD3);
	imx290_2l_write_register(ViPipe, 0x332D, 0x10);
	imx290_2l_write_register(ViPipe, 0x332E, 0x0D);
	imx290_2l_write_register(ViPipe, 0x3358, 0x06);
	imx290_2l_write_register(ViPipe, 0x3359, 0xE1);
	imx290_2l_write_register(ViPipe, 0x335A, 0x11);
	imx290_2l_write_register(ViPipe, 0x3360, 0x1E);
	imx290_2l_write_register(ViPipe, 0x3361, 0x61);
	imx290_2l_write_register(ViPipe, 0x3362, 0x10);

	imx290_2l_write_register(ViPipe, 0x33B0, 0x50);
	imx290_2l_write_register(ViPipe, 0x33B2, 0x1A);
	imx290_2l_write_register(ViPipe, 0x33B3, 0x04);
	imx290_2l_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7 */

	imx290_2l_default_reg_init(ViPipe);

	imx290_2l_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx290_2l_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	imx290_2l_write_register(ViPipe, 0x304b, 0x0a);

	printf("ViPipe:%d,===IMX290_2L 720P 30fps 10bit LINE Init OK!===\n", ViPipe);
}

static void imx290_2l_wdr_720p15_2to1_init(VI_PIPE ViPipe)
{
	imx290_2l_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	delay_ms(4);
	imx290_2l_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx290_2l_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	imx290_2l_write_register(ViPipe, 0x3005, 0x00); /* ADBIT 10bit */
	imx290_2l_write_register(ViPipe, 0x3007, 0x10); /* VREVERS, */
	imx290_2l_write_register(ViPipe, 0x3009, 0x02);
	imx290_2l_write_register(ViPipe, 0x300A, 0x3C); /* BLKLEVEL */
	imx290_2l_write_register(ViPipe, 0x300C, 0x11);	/* WDMODE [0] ,WDSEL [5:4] */
	imx290_2l_write_register(ViPipe, 0x300F, 0x00);
	imx290_2l_write_register(ViPipe, 0x3012, 0x64);
	imx290_2l_write_register(ViPipe, 0x3014, 0x2A); /* GAIN, 0x2A=>12.6dB TBD */
	imx290_2l_write_register(ViPipe, 0x3016, 0x09);
	imx290_2l_write_register(ViPipe, 0x3018, 0xEE);	/* VMAX[7:0] */
	imx290_2l_write_register(ViPipe, 0x3019, 0x02);	/* VMAX[15:8] */
	imx290_2l_write_register(ViPipe, 0x301A, 0x00);	/* VMAX[17:16]:=:0x301A[1:0] */
	imx290_2l_write_register(ViPipe, 0x301C, 0xC8); /* HMAX[7:0], TBD */
	imx290_2l_write_register(ViPipe, 0x301D, 0x19); /* HMAX[15:8] */
	imx290_2l_write_register(ViPipe, 0x3030, 0x09);	/* RHS1[7:0] */
	imx290_2l_write_register(ViPipe, 0x3031, 0x00);	/* RHS1[15:8] */
	imx290_2l_write_register(ViPipe, 0x3032, 0x00);	/* RHS1[19:16] */
	imx290_2l_write_register(ViPipe, 0x3045, 0x03);	/* DOLSCDEN [0], DOLSYDINFOEN [1], HINFOEN [2]*/
	imx290_2l_write_register(ViPipe, 0x3046, 0xD0); /* Lane number D-2, E-4, F-8 */
	imx290_2l_write_register(ViPipe, 0x304B, 0x0A);
	imx290_2l_write_register(ViPipe, 0x305C, 0x20); /* INCKSEL1 */
	imx290_2l_write_register(ViPipe, 0x305D, 0x00); /* INCKSEL2 */
	imx290_2l_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3 */
	imx290_2l_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4 */
	imx290_2l_write_register(ViPipe, 0x3070, 0x02);
	imx290_2l_write_register(ViPipe, 0x3071, 0x11);
	imx290_2l_write_register(ViPipe, 0x309B, 0x10);
	imx290_2l_write_register(ViPipe, 0x309C, 0x22);
	imx290_2l_write_register(ViPipe, 0x30A2, 0x02);
	imx290_2l_write_register(ViPipe, 0x30A6, 0x20);
	imx290_2l_write_register(ViPipe, 0x30A8, 0x20);
	imx290_2l_write_register(ViPipe, 0x30AA, 0x20);
	imx290_2l_write_register(ViPipe, 0x30AC, 0x20);
	imx290_2l_write_register(ViPipe, 0x30B0, 0x43);
	imx290_2l_write_register(ViPipe, 0x30F0, 0x64); /* F0: normal, 64: each frame gain*/
	imx290_2l_write_register(ViPipe, 0x3106, 0x11);	/* DOLHBFIXEN [7], DOLHBFIXEN [1:0], DOLHBFIXEN [5:4] */
	imx290_2l_write_register(ViPipe, 0x3119, 0x9E);
	imx290_2l_write_register(ViPipe, 0x311C, 0x1E);
	imx290_2l_write_register(ViPipe, 0x311E, 0x08);
	imx290_2l_write_register(ViPipe, 0x3128, 0x05);
	imx290_2l_write_register(ViPipe, 0x3129, 0x1D);
	imx290_2l_write_register(ViPipe, 0x313D, 0x83);
	imx290_2l_write_register(ViPipe, 0x3150, 0x03);
	imx290_2l_write_register(ViPipe, 0x315E, 0x1A);
	imx290_2l_write_register(ViPipe, 0x3164, 0x1A);
	imx290_2l_write_register(ViPipe, 0x317C, 0x12);
	imx290_2l_write_register(ViPipe, 0x317E, 0x00);
	imx290_2l_write_register(ViPipe, 0x31EC, 0x37);
	imx290_2l_write_register(ViPipe, 0x32B8, 0x50);
	imx290_2l_write_register(ViPipe, 0x32B9, 0x10);
	imx290_2l_write_register(ViPipe, 0x32BA, 0x00);
	imx290_2l_write_register(ViPipe, 0x32BB, 0x04);
	imx290_2l_write_register(ViPipe, 0x32C8, 0x50);
	imx290_2l_write_register(ViPipe, 0x32C9, 0x10);
	imx290_2l_write_register(ViPipe, 0x32CA, 0x00);
	imx290_2l_write_register(ViPipe, 0x32CB, 0x04);
	imx290_2l_write_register(ViPipe, 0x332C, 0xD3);
	imx290_2l_write_register(ViPipe, 0x332D, 0x10);
	imx290_2l_write_register(ViPipe, 0x332E, 0x0D);
	imx290_2l_write_register(ViPipe, 0x3358, 0x06);
	imx290_2l_write_register(ViPipe, 0x3359, 0xE1);
	imx290_2l_write_register(ViPipe, 0x335A, 0x11);
	imx290_2l_write_register(ViPipe, 0x3360, 0x1E);
	imx290_2l_write_register(ViPipe, 0x3361, 0x61);
	imx290_2l_write_register(ViPipe, 0x3362, 0x10);
	imx290_2l_write_register(ViPipe, 0x33B0, 0x50);
	imx290_2l_write_register(ViPipe, 0x33B2, 0x1A);
	imx290_2l_write_register(ViPipe, 0x33B3, 0x04);
	imx290_2l_write_register(ViPipe, 0x3480, 0x49); /* INCKSEL7 */

	imx290_2l_default_reg_init(ViPipe);

	if (g_au16Imx290_2l_GainMode[ViPipe] == SNS_GAIN_MODE_SHARE) {
		imx290_2l_write_register(ViPipe, 0x30F0, 0xF0);
		imx290_2l_write_register(ViPipe, 0x3010, 0x21);
	} else {
		imx290_2l_write_register(ViPipe, 0x30F0, 0x64);
		imx290_2l_write_register(ViPipe, 0x3010, 0x61);
	}

	imx290_2l_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx290_2l_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	imx290_2l_write_register(ViPipe, 0x304b, 0x0a);

	printf("===Imx290_2l sensor 720P30fps 10bit 2to1 WDR(30fps->15fps) init success!=====\n");
}
