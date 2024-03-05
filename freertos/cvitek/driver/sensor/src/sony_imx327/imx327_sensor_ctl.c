#include <stdio.h>

#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"

#include "sensor.h"
#include "cvi_i2c.h"
#include "cif_uapi.h"

#include "imx327_cmos_ex.h"

static void imx327_wdr_1080p30_2to1_init(VI_PIPE ViPipe);
static void imx327_linear_1080p30_init(VI_PIPE ViPipe);
static void imx327_linear_1080p60_init(VI_PIPE ViPipe);
static void imx327_linear_720p30_init(VI_PIPE ViPipe);
static void imx327_wdr_720p30_2to1_init(VI_PIPE ViPipe);

const CVI_U8 imx327_i2c_addr = IMX327_SLAVE_ID;

void imx327_standby(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	sensor_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
}

void imx327_restart(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3000, 0x00); /* standby */
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);
}

void imx327_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx327[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sensor_write_register(ViPipe,
				g_pastImx327[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx327[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void imx327_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = sensor_read_register(ViPipe, 0x3007) & ~0x3;

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

	sensor_write_register(ViPipe, 0x3007, val);
}

int imx327_probe(VI_PIPE ViPipe)
{
	int nVal;

	usleep(100);
	if (sensor_i2c_init(ViPipe, g_aunImx327_BusInfo[ViPipe].s8I2cDev, I2C_400KHZ,
		IMX327_SLAVE_ID, IMX327_ADDR_LEN, IMX327_DATA_LEN) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal = sensor_read_register(ViPipe, IMX327_CHIP_ID_ADDR);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((nVal & IMX327_CHIP_ID_MASK) != IMX327_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void imx327_init(VI_PIPE ViPipe)
{
	WDR_MODE_E        enWDRMode;
	CVI_U8            u8ImgMode;

	CVI_TRACE_SNS(CVI_DBG_INFO, "imx327_init ViPipe=%d, bus_id=%d\n",
		ViPipe, g_aunImx327_BusInfo[ViPipe].s8I2cDev);

	enWDRMode   = g_pastImx327[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastImx327[ViPipe]->u8ImgMode;

#if (RUN_TYPE == CVIRTOS)
	// release sensor reset pin
	cif_ioctl(ViPipe, CVI_MIPI_UNRESET_SENSOR, 0);
	//mdelay(1);
	udelay(200);
#endif

	sensor_i2c_init(ViPipe, g_aunImx327_BusInfo[ViPipe].s8I2cDev, I2C_400KHZ,
		IMX327_SLAVE_ID, IMX327_ADDR_LEN, IMX327_DATA_LEN);

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		if (u8ImgMode == IMX327_MODE_1080P30_HDR) {
			imx327_wdr_1080p30_2to1_init(ViPipe);
		} else if (u8ImgMode == IMX327_MODE_720P30_HDR) {
			imx327_wdr_720p30_2to1_init(ViPipe);
		}
	} else {
		if (u8ImgMode == IMX327_MODE_1080P60)
			imx327_linear_1080p60_init(ViPipe);
		else if (u8ImgMode == IMX327_MODE_720P30)
			imx327_linear_720p30_init(ViPipe);
		else
			imx327_linear_1080p30_init(ViPipe);
	}
	g_pastImx327[ViPipe]->bInit = CVI_TRUE;
}

/* 1080P30 and 1080P25 */
static void imx327_linear_1080p30_init(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	udelay(1);
	sensor_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	sensor_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	sensor_write_register(ViPipe, 0x3005, 0x01); /* ADBIT 10bit*/
	sensor_write_register(ViPipe, 0x3007, 0x00); /* VREVERS, ...*/
	sensor_write_register(ViPipe, 0x3009, 0x02); /**/
	sensor_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	sensor_write_register(ViPipe, 0x3010, 0x21);
	sensor_write_register(ViPipe, 0x3011, 0x02);
	sensor_write_register(ViPipe, 0x3014, 0x16); /* GAIN, 0x2A=>12.6dB TBD*/
	sensor_write_register(ViPipe, 0x3018, 0x65); /* VMAX[7:0]*/
	sensor_write_register(ViPipe, 0x3019, 0x04); /* VMAX[15:8]*/
	sensor_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:16]:=:0x301A[1:0]*/
	sensor_write_register(ViPipe, 0x301C, 0x30); /* HMAX[7:0], TBD*/
	sensor_write_register(ViPipe, 0x301D, 0x11); /* HMAX[15:8]*/
	sensor_write_register(ViPipe, 0x3020, 0x8C); /* SHS[7:0], TBD*/
	sensor_write_register(ViPipe, 0x3021, 0x01); /* SHS[15:8]*/
	sensor_write_register(ViPipe, 0x3022, 0x00); /* SHS[19:16]*/
	sensor_write_register(ViPipe, 0x3046, 0x01);
	sensor_write_register(ViPipe, 0x304B, 0x0A);
	sensor_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	sensor_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2*/
	sensor_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	sensor_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4*/
	sensor_write_register(ViPipe, 0x309E, 0x4A);
	sensor_write_register(ViPipe, 0x309F, 0x4A);
	sensor_write_register(ViPipe, 0x30D2, 0x19);
	sensor_write_register(ViPipe, 0x30D7, 0x03);
	sensor_write_register(ViPipe, 0x3129, 0x00);
	sensor_write_register(ViPipe, 0x313B, 0x61);
	sensor_write_register(ViPipe, 0x315E, 0x1A);
	sensor_write_register(ViPipe, 0x3164, 0x1A);
	sensor_write_register(ViPipe, 0x317C, 0x00);
	sensor_write_register(ViPipe, 0x31EC, 0x0E);
	sensor_write_register(ViPipe, 0x3405, 0x20); /* Repetition*/
	sensor_write_register(ViPipe, 0x3407, 0x03); /* physical_lane_nl*/
	sensor_write_register(ViPipe, 0x3414, 0x0A); /* opb_size_v*/
	sensor_write_register(ViPipe, 0x3418, 0x49); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3419, 0x04); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3441, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3442, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3443, 0x03); /* csi_lane_mode*/
	sensor_write_register(ViPipe, 0x3444, 0x20); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3445, 0x25); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3446, 0x47); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3447, 0x00); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3448, 0x80); /* thszero*/
	sensor_write_register(ViPipe, 0x3449, 0x00); /* thszero*/
	sensor_write_register(ViPipe, 0x344A, 0x17); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344B, 0x00); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344C, 0x0F); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344D, 0x00); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344E, 0x80); /* thstrail*/
	sensor_write_register(ViPipe, 0x344F, 0x00); /* thstrail*/
	sensor_write_register(ViPipe, 0x3450, 0x47); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3451, 0x00); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3452, 0x0F); /* tclkprepare*/
	sensor_write_register(ViPipe, 0x3453, 0x00); /* tckkprepare*/
	sensor_write_register(ViPipe, 0x3454, 0x0F); /* tlpx*/
	sensor_write_register(ViPipe, 0x3455, 0x00); /* tlpx*/
	sensor_write_register(ViPipe, 0x3472, 0x9C); /* x_out_size*/
	sensor_write_register(ViPipe, 0x3473, 0x07); /* x_out_size*/
	sensor_write_register(ViPipe, 0x3480, 0x49); /* incksel7*/

	imx327_default_reg_init(ViPipe);

	sensor_write_register(ViPipe, 0x3000, 0x00); /* standby */
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);

	printf("ViPipe:%d,===IMX327 1080P 30fps 12bit LINE Init OK!===\n", ViPipe);
}

static void imx327_wdr_1080p30_2to1_init(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	udelay(1);
	sensor_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	sensor_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	sensor_write_register(ViPipe, 0x3005, 0x01); /* ADBIT*/
	sensor_write_register(ViPipe, 0x3007, 0x00); /* VREVERS, ...*/
	sensor_write_register(ViPipe, 0x3009, 0x01); /**/
	sensor_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	sensor_write_register(ViPipe, 0x300C, 0x11); /* WDMODE [0] 0:Normal, 1:DOL, WDSEL [5:4] 1:DOL 2 frames*/
	sensor_write_register(ViPipe, 0x3011, 0x02);
	sensor_write_register(ViPipe, 0x3014, 0x16); /* GAIN, 0x2A=>12.6dB TBD*/
	sensor_write_register(ViPipe, 0x3018, 0x65); /* VMAX[7:0]*/
	sensor_write_register(ViPipe, 0x3019, 0x04); /* VMAX[15:8]*/
	sensor_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:16]:=:0x301A[1:0]*/
	sensor_write_register(ViPipe, 0x301C, 0x98); /* HMAX[7:0], TBD*/
	sensor_write_register(ViPipe, 0x301D, 0x08); /* HMAX[15:8]*/
	sensor_write_register(ViPipe, 0x3020, 0x02); /* SHS[7:0], TBD*/
	sensor_write_register(ViPipe, 0x3021, 0x00); /* SHS[15:8]*/
	sensor_write_register(ViPipe, 0x3022, 0x00); /* SHS[19:16]*/
	sensor_write_register(ViPipe, 0x3024, 0xC9); /* SHS2[7:0], TBD*/
	sensor_write_register(ViPipe, 0x3025, 0x07); /* SHS2[15:8]*/
	sensor_write_register(ViPipe, 0x3026, 0x00); /* SHS2[19:16]*/
	sensor_write_register(ViPipe, 0x3030, 0x0B); /* RHS1[7:0], TBD*/
	sensor_write_register(ViPipe, 0x3031, 0x00); /* RHS1[15:8]*/
	sensor_write_register(ViPipe, 0x3032, 0x00); /* RHS1[19:16]*/
	sensor_write_register(ViPipe, 0x3045, 0x05); /* DOLSCDEN [0] 1: pattern1 0: pattern2*/
	sensor_write_register(ViPipe, 0x3046, 0x01);
	sensor_write_register(ViPipe, 0x304B, 0x0A);
	sensor_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	sensor_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2*/
	sensor_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	sensor_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4*/
	sensor_write_register(ViPipe, 0x309E, 0x4A);
	sensor_write_register(ViPipe, 0x309F, 0x4A);
	sensor_write_register(ViPipe, 0x30D2, 0x19);
	sensor_write_register(ViPipe, 0x30D7, 0x03);
	sensor_write_register(ViPipe, 0x3106, 0x11); /*DOLHBFIXEN[7] 0: pattern1 1: pattern2*/
	sensor_write_register(ViPipe, 0x3129, 0x00);
	sensor_write_register(ViPipe, 0x313B, 0x61);
	sensor_write_register(ViPipe, 0x315E, 0x1A);
	sensor_write_register(ViPipe, 0x3164, 0x1A);
	sensor_write_register(ViPipe, 0x317C, 0x00);
	sensor_write_register(ViPipe, 0x31EC, 0x0E);
	sensor_write_register(ViPipe, 0x3405, 0x10); /* Repetition*/
	sensor_write_register(ViPipe, 0x3407, 0x03); /* physical_lane_nl*/
	sensor_write_register(ViPipe, 0x3414, 0x0A); /* opb_size_v*/
	sensor_write_register(ViPipe, 0x3415, 0x00); /* NULL0_SIZE_V, set to 00h when DOL*/
	sensor_write_register(ViPipe, 0x3418, 0xB4); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3419, 0x08); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3441, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3442, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3443, 0x03); /* csi_lane_mode*/
	sensor_write_register(ViPipe, 0x3444, 0x20); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3445, 0x25); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3446, 0x57); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3447, 0x00); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3448, 0x80); /* thszero*/
	sensor_write_register(ViPipe, 0x3449, 0x00); /* thszero*/
	sensor_write_register(ViPipe, 0x344A, 0x1F); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344B, 0x00); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344C, 0x1F); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344D, 0x00); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344E, 0x80); /* thstrail*/
	sensor_write_register(ViPipe, 0x344F, 0x00); /* thstrail*/
	sensor_write_register(ViPipe, 0x3450, 0x77); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3451, 0x00); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3452, 0x1F); /* tclkprepare*/
	sensor_write_register(ViPipe, 0x3453, 0x00); /* tckkprepare*/
	sensor_write_register(ViPipe, 0x3454, 0x17); /* tlpx*/
	sensor_write_register(ViPipe, 0x3455, 0x00); /* tlpx*/
	sensor_write_register(ViPipe, 0x3472, 0xA0); /* x_out_size*/
	sensor_write_register(ViPipe, 0x3473, 0x07); /* x_out_size*/
	sensor_write_register(ViPipe, 0x347B, 0x23); /**/
	sensor_write_register(ViPipe, 0x3480, 0x49); /* incksel7*/

	imx327_default_reg_init(ViPipe);

	if (g_au16Imx327_GainMode[ViPipe] == SNS_GAIN_MODE_SHARE) {
		sensor_write_register(ViPipe, 0x30F0, 0xF0);
		sensor_write_register(ViPipe, 0x3010, 0x21);
	} else {
		sensor_write_register(ViPipe, 0x30F0, 0x64);
		sensor_write_register(ViPipe, 0x3010, 0x61);
	}

	sensor_write_register(ViPipe, 0x3000, 0x00); /* standby */
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);

	printf("===Imx327 sensor 1080P30fps 12bit 2to1 WDR(60fps->30fps) init success!=====\n");
}

/* 1080P60 and 1080P50 */
static void imx327_linear_1080p60_init(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	udelay(1);
	sensor_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	sensor_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
	sensor_write_register(ViPipe, 0x3005, 0x01); /* ADBIT 10bit*/
	sensor_write_register(ViPipe, 0x3007, 0x00); /* VREVERS, ...*/
	sensor_write_register(ViPipe, 0x3009, 0x01); /**/
	sensor_write_register(ViPipe, 0x300A, 0xF0); /* BLKLEVEL*/
	sensor_write_register(ViPipe, 0x3010, 0x21);
	sensor_write_register(ViPipe, 0x3011, 0x00);
	sensor_write_register(ViPipe, 0x3014, 0x16); /* GAIN, 0x2A=>12.6dB TBD*/
	sensor_write_register(ViPipe, 0x3018, 0x65); /* VMAX[7:0]*/
	sensor_write_register(ViPipe, 0x3019, 0x04); /* VMAX[15:8]*/
	sensor_write_register(ViPipe, 0x301A, 0x00); /* VMAX[17:16]:=:0x301A[1:0]*/
	sensor_write_register(ViPipe, 0x301C, 0x98); /* HMAX[7:0], TBD*/
	sensor_write_register(ViPipe, 0x301D, 0x08); /* HMAX[15:8]*/
	sensor_write_register(ViPipe, 0x3020, 0x8C); /* SHS[7:0], TBD*/
	sensor_write_register(ViPipe, 0x3021, 0x01); /* SHS[15:8]*/
	sensor_write_register(ViPipe, 0x3022, 0x00); /* SHS[19:16]*/
	sensor_write_register(ViPipe, 0x3046, 0x01);
	sensor_write_register(ViPipe, 0x304B, 0x0A);
	sensor_write_register(ViPipe, 0x305C, 0x18); /* INCKSEL1*/
	sensor_write_register(ViPipe, 0x305D, 0x03); /* INCKSEL2*/
	sensor_write_register(ViPipe, 0x305E, 0x20); /* INCKSEL3*/
	sensor_write_register(ViPipe, 0x305F, 0x01); /* INCKSEL4*/
	sensor_write_register(ViPipe, 0x309E, 0x4A);
	sensor_write_register(ViPipe, 0x309F, 0x4A);
	sensor_write_register(ViPipe, 0x30D2, 0x19);
	sensor_write_register(ViPipe, 0x30D7, 0x03);
	sensor_write_register(ViPipe, 0x3129, 0x00);
	sensor_write_register(ViPipe, 0x313B, 0x61);
	sensor_write_register(ViPipe, 0x315E, 0x1A);
	sensor_write_register(ViPipe, 0x3164, 0x1A);
	sensor_write_register(ViPipe, 0x317C, 0x00);
	sensor_write_register(ViPipe, 0x31EC, 0x0E);
	sensor_write_register(ViPipe, 0x3405, 0x10); /* Repetition*/
	sensor_write_register(ViPipe, 0x3407, 0x03); /* physical_lane_nl*/
	sensor_write_register(ViPipe, 0x3414, 0x0A); /* opb_size_v*/
	sensor_write_register(ViPipe, 0x3418, 0x49); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3419, 0x04); /* y_out_size*/
	sensor_write_register(ViPipe, 0x3441, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3442, 0x0C); /* csi_dt_fmt*/
	sensor_write_register(ViPipe, 0x3443, 0x03); /* csi_lane_mode*/
	sensor_write_register(ViPipe, 0x3444, 0x20); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3445, 0x25); /* extck_freq*/
	sensor_write_register(ViPipe, 0x3446, 0x57); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3447, 0x00); /* tclkpost*/
	sensor_write_register(ViPipe, 0x3448, 0x80); /* thszero*/
	sensor_write_register(ViPipe, 0x3449, 0x00); /* thszero*/
	sensor_write_register(ViPipe, 0x344A, 0x1F); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344B, 0x00); /* thsprepare*/
	sensor_write_register(ViPipe, 0x344C, 0x1F); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344D, 0x00); /* tclktrail*/
	sensor_write_register(ViPipe, 0x344E, 0x80); /* thstrail*/
	sensor_write_register(ViPipe, 0x344F, 0x00); /* thstrail*/
	sensor_write_register(ViPipe, 0x3450, 0x77); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3451, 0x00); /* tclkzero*/
	sensor_write_register(ViPipe, 0x3452, 0x1F); /* tclkprepare*/
	sensor_write_register(ViPipe, 0x3453, 0x00); /* tckkprepare*/
	sensor_write_register(ViPipe, 0x3454, 0x17); /* tlpx*/
	sensor_write_register(ViPipe, 0x3455, 0x00); /* tlpx*/
	sensor_write_register(ViPipe, 0x3472, 0x9C); /* x_out_size*/
	sensor_write_register(ViPipe, 0x3473, 0x07); /* x_out_size*/
	sensor_write_register(ViPipe, 0x3480, 0x49); /* incksel7*/

	imx327_default_reg_init(ViPipe);

	sensor_write_register(ViPipe, 0x3000, 0x00); /* standby */
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);

	printf("ViPipe:%d,===IMX327 1080P 60fps 12bit LINE Init OK!===\n", ViPipe);
}

static void imx327_linear_720p30_init(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3003, 0x01); /* SW RESET */
	udelay(1);
	sensor_write_register(ViPipe, 0x3000, 0x01); // STANDBY
	sensor_write_register(ViPipe, 0x3001, 0x00); // HOLD
	sensor_write_register(ViPipe, 0x3002, 0x00); // XMSTA
	sensor_write_register(ViPipe, 0x3005, 0x00); // ADBIT 10bit
	// sensor_write_register(ViPipe, 0x3007, 0x10); // VREVERS, ...
	// sensor_write_register(ViPipe, 0x3007, 0x11); // V VREVERS, ...
	sensor_write_register(ViPipe, 0x3009, 0x02); //
	sensor_write_register(ViPipe, 0x300A, 0x3C); // BLKLEVEL
	//sensor_write_register(ViPipe, 0x300C, 0x00); // WDMODE [0] 0:Normal, 1:DOL, WDSEL [5:4] 1:DOL 2 frames
	sensor_write_register(ViPipe, 0x3011, 0x02);
	// sensor_write_register(ViPipe, 0x3014, 0x16); // GAIN, 0x2A=>12.6dB TBD
	sensor_write_register(ViPipe, 0x3014, 0x00); // GAIN, 0x0=>0 dB TBD
	//sensor_write_register(ViPipe, 0x3016, 0x08); // [TODO] sony's secrect register?
	sensor_write_register(ViPipe, 0x3018, 0xDC); // VMAX[7:0] TBD=750
	sensor_write_register(ViPipe, 0x3019, 0x05); // VMAX[15:8]
	sensor_write_register(ViPipe, 0x301A, 0x00); // VMAX[17:16]:=:0x301A[1:0]
	// sensor_write_register(ViPipe, 0x301C, 0xC8); // HMAX[7:0], TBD=6600
	// sensor_write_register(ViPipe, 0x301D, 0x19); // HMAX[15:8]
	sensor_write_register(ViPipe, 0x301C, 0x58); // HMAX[7:0], TBD=19800
	sensor_write_register(ViPipe, 0x301D, 0x4D); // HMAX[15:8]
	// sensor_write_register(ViPipe, 0x3020, 0x8C); // SHS[7:0], TBD
	// sensor_write_register(ViPipe, 0x3021, 0x01); // SHS[15:8]
	// sensor_write_register(ViPipe, 0x3022, 0x00); // SHS[19:16]
	sensor_write_register(ViPipe, 0x3020, 0xA0); // SHS[7:0], TBD
	sensor_write_register(ViPipe, 0x3021, 0x05); // SHS[15:8]
	sensor_write_register(ViPipe, 0x3022, 0x00); // SHS[19:16]
	//sensor_write_register(ViPipe, 0x3024, 0x00); // SHS2[7:0], TBD
	//sensor_write_register(ViPipe, 0x3025, 0x00); // SHS2[15:8]
	//sensor_write_register(ViPipe, 0x3026, 0x00); // SHS2[19:16]
	//sensor_write_register(ViPipe, 0x3030, 0x00); // RHS1[7:0], TBD
	//sensor_write_register(ViPipe, 0x3031, 0x00); // RHS1[15:8]
	//sensor_write_register(ViPipe, 0x3032, 0x00); // RHS1[19:16]
	//sensor_write_register(ViPipe, 0x3045, 0x00); // DOLSCDEN [0] 1: pattern1 0: pattern2
	// DOLSYDINFOEN[1] 1: embed the id code into 4th of sync
	// code. 0: disable
	// HINFOEN [2] 1: insert id code after 4th sync code 0: disable
	sensor_write_register(ViPipe, 0x3046, 0x00);
	sensor_write_register(ViPipe, 0x304B, 0x0A);
	sensor_write_register(ViPipe, 0x305C, 0x20); // INCKSEL1
	sensor_write_register(ViPipe, 0x305D, 0x00); // INCKSEL2
	sensor_write_register(ViPipe, 0x305E, 0x20); // INCKSEL3
	sensor_write_register(ViPipe, 0x305F, 0x01); // INCKSEL4
	sensor_write_register(ViPipe, 0x309E, 0x4A);
	sensor_write_register(ViPipe, 0x309F, 0x4A);
	sensor_write_register(ViPipe, 0x30D2, 0x19);
	sensor_write_register(ViPipe, 0x30D7, 0x03);
	//sensor_write_register(ViPipe, 0x3106, 0x00); //DOLHBFIXEN[7] 0: pattern1 1: pattern2
	sensor_write_register(ViPipe, 0x3129, 0x1D);
	sensor_write_register(ViPipe, 0x313B, 0x61);
	sensor_write_register(ViPipe, 0x315E, 0x1A);
	sensor_write_register(ViPipe, 0x3164, 0x1A);
	sensor_write_register(ViPipe, 0x317C, 0x12);
	sensor_write_register(ViPipe, 0x31EC, 0x37);
	sensor_write_register(ViPipe, 0x3405, 0x10); // Repetition
	sensor_write_register(ViPipe, 0x3407, 0x01); // physical_lane_nl
	sensor_write_register(ViPipe, 0x3414, 0x04); // opb_size_v
	sensor_write_register(ViPipe, 0x3418, 0xD9); // y_out_size
	sensor_write_register(ViPipe, 0x3419, 0x02); // y_out_size
	sensor_write_register(ViPipe, 0x3441, 0x0A); // csi_dt_fmt
	sensor_write_register(ViPipe, 0x3442, 0x0A); // csi_dt_fmt
	sensor_write_register(ViPipe, 0x3443, 0x01); // csi_lane_mode
	sensor_write_register(ViPipe, 0x3444, 0x20); // extck_freq
	sensor_write_register(ViPipe, 0x3445, 0x25); // extck_freq
	sensor_write_register(ViPipe, 0x3446, 0x4F); // tclkpost
	sensor_write_register(ViPipe, 0x3447, 0x00); // tclkpost
	sensor_write_register(ViPipe, 0x3448, 0x80); // thszero
	sensor_write_register(ViPipe, 0x3449, 0x00); // thszero
	sensor_write_register(ViPipe, 0x344A, 0x17); // thsprepare
	sensor_write_register(ViPipe, 0x344B, 0x00); // thsprepare
	sensor_write_register(ViPipe, 0x344C, 0x17); // tclktrail
	sensor_write_register(ViPipe, 0x344D, 0x00); // tclktrail
	sensor_write_register(ViPipe, 0x344E, 0x80); // thstrail
	sensor_write_register(ViPipe, 0x344F, 0x00); // thstrail
	sensor_write_register(ViPipe, 0x3450, 0x57); // tclkzero
	sensor_write_register(ViPipe, 0x3451, 0x00); // tclkzero
	sensor_write_register(ViPipe, 0x3452, 0x17); // tclkprepare
	sensor_write_register(ViPipe, 0x3453, 0x00); // tckkprepare
	sensor_write_register(ViPipe, 0x3454, 0x17); // tlpx
	sensor_write_register(ViPipe, 0x3455, 0x00); // tlpx
	sensor_write_register(ViPipe, 0x3472, 0x1C); // x_out_size
	sensor_write_register(ViPipe, 0x3473, 0x05); // x_out_size
	sensor_write_register(ViPipe, 0x3480, 0x49); // incksel7

	// imx327_default_reg_init(ViPipe);

	sensor_write_register(ViPipe, 0x3000, 0x00); /* standby */
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);

	printf("ViPipe:%d,===IMX327 720P 30fps 10bit LINE Init for FPGA OK!===\n", ViPipe);
}

static void imx327_wdr_720p30_2to1_init(VI_PIPE ViPipe)
{
	sensor_write_register(ViPipe, 0x3003, 0x01); // SW Reset
	delay_ms(3);
	sensor_write_register(ViPipe, 0x3000, 0x01); // STANDBY
	sensor_write_register(ViPipe, 0x3001, 0x00); // HOLD
	sensor_write_register(ViPipe, 0x3002, 0x00); // XMSTA
	sensor_write_register(ViPipe, 0x3005, 0x00); // ADBIT=10bit
	// sensor_write_register(ViPipe, 0x3007, 0x11); // VREVERS, ...
	sensor_write_register(ViPipe, 0x3009, 0x02); //
	sensor_write_register(ViPipe, 0x300A, 0x3C); // BLKLEVEL
	sensor_write_register(ViPipe, 0x300C, 0x11); // WDMODE [0] 0:Normal, 1:DOL, WDSEL [5:4] 1:DOL 2 frames
	sensor_write_register(ViPipe, 0x3011, 0x02);
	sensor_write_register(ViPipe, 0x3014, 0x16); // GAIN, 0x2A=>12.6dB TBD
	//sensor_write_register(ViPipe, 0x3016, 0x08); // [TODO] sony's secrect register?
	sensor_write_register(ViPipe, 0x3018, 0xB0); // VMAX[7:0]
	sensor_write_register(ViPipe, 0x3019, 0x04); // VMAX[15:8]
	sensor_write_register(ViPipe, 0x301A, 0x00); // VMAX[17:16]:=:0x301A[1:0]
	// sensor_write_register(ViPipe, 0x301C, 0xC8); // HMAX[7:0], TBD=6600
	// sensor_write_register(ViPipe, 0x301D, 0x19); // HMAX[15:8]
	sensor_write_register(ViPipe, 0x301C, 0x90); // HMAX[7:0], TBD=13200
	sensor_write_register(ViPipe, 0x301D, 0x33); // HMAX[15:8]
	sensor_write_register(ViPipe, 0x3020, 0x02); // SHS[7:0], TBD
	sensor_write_register(ViPipe, 0x3021, 0x00); // SHS[15:8]
	sensor_write_register(ViPipe, 0x3022, 0x00); // SHS[19:16]
	sensor_write_register(ViPipe, 0x3024, 0x1B); // SHS2[7:0], TBD
	sensor_write_register(ViPipe, 0x3025, 0x05); // SHS2[15:8]
	sensor_write_register(ViPipe, 0x3026, 0x00); // SHS2[19:16]
	sensor_write_register(ViPipe, 0x3030, 0x09); // RHS1[7:0], TBD
	sensor_write_register(ViPipe, 0x3031, 0x00); // RHS1[15:8]
	sensor_write_register(ViPipe, 0x3032, 0x00); // RHS1[19:16]
	sensor_write_register(ViPipe, 0x3045, 0x05); // DOLSCDEN [0] 1: pattern1 0: pattern2
	// DOLSYDINFOEN[1] 1: embed the id code into 4th of sync
	// code. 0: disable
	// HINFOEN [2] 1: insert id code after 4th sync code 0: disable
	sensor_write_register(ViPipe, 0x3046, 0x00);
	sensor_write_register(ViPipe, 0x304B, 0x0A);
	sensor_write_register(ViPipe, 0x305C, 0x20); // INCKSEL1
	sensor_write_register(ViPipe, 0x305D, 0x00); // INCKSEL2
	sensor_write_register(ViPipe, 0x305E, 0x20); // INCKSEL3
	sensor_write_register(ViPipe, 0x305F, 0x01); // INCKSEL4
	sensor_write_register(ViPipe, 0x309E, 0x4A);
	sensor_write_register(ViPipe, 0x309F, 0x4A);
	sensor_write_register(ViPipe, 0x30D2, 0x19);
	sensor_write_register(ViPipe, 0x30D7, 0x03);
	sensor_write_register(ViPipe, 0x3106, 0x11); //DOLHBFIXEN[7] 0: pattern1 1: pattern2
	sensor_write_register(ViPipe, 0x3129, 0x1D);
	sensor_write_register(ViPipe, 0x313B, 0x61);
	sensor_write_register(ViPipe, 0x315E, 0x1A);
	sensor_write_register(ViPipe, 0x3164, 0x1A);
	sensor_write_register(ViPipe, 0x317C, 0x12);
	sensor_write_register(ViPipe, 0x31EC, 0x37);
	sensor_write_register(ViPipe, 0x3405, 0x10); // Repetition
	sensor_write_register(ViPipe, 0x3407, 0x01); // physical_lane_nl
	sensor_write_register(ViPipe, 0x3414, 0x04); // opb_size_v
	sensor_write_register(ViPipe, 0x3415, 0x00); // NULL0_SIZE_V, set to 00h when DOL
	sensor_write_register(ViPipe, 0x3418, 0xC6); // y_out_size
	sensor_write_register(ViPipe, 0x3419, 0x05); // y_out_size
	sensor_write_register(ViPipe, 0x3441, 0x0A); // csi_dt_fmt
	sensor_write_register(ViPipe, 0x3442, 0x0A); // csi_dt_fmt
	sensor_write_register(ViPipe, 0x3443, 0x01); // csi_lane_mode
	sensor_write_register(ViPipe, 0x3444, 0x20); // extck_freq
	sensor_write_register(ViPipe, 0x3445, 0x25); // extck_freq
	sensor_write_register(ViPipe, 0x3446, 0x4F); // tclkpost
	sensor_write_register(ViPipe, 0x3447, 0x00); // tclkpost
	sensor_write_register(ViPipe, 0x3448, 0x80); // thszero
	sensor_write_register(ViPipe, 0x3449, 0x00); // thszero
	sensor_write_register(ViPipe, 0x344A, 0x17); // thsprepare
	sensor_write_register(ViPipe, 0x344B, 0x00); // thsprepare
	sensor_write_register(ViPipe, 0x344C, 0x17); // tclktrail
	sensor_write_register(ViPipe, 0x344D, 0x00); // tclktrail
	sensor_write_register(ViPipe, 0x344E, 0x80); // thstrail
	sensor_write_register(ViPipe, 0x344F, 0x00); // thstrail
	sensor_write_register(ViPipe, 0x3450, 0x57); // tclkzero
	sensor_write_register(ViPipe, 0x3451, 0x00); // tclkzero
	sensor_write_register(ViPipe, 0x3452, 0x17); // tclkprepare
	sensor_write_register(ViPipe, 0x3453, 0x00); // tckkprepare
	sensor_write_register(ViPipe, 0x3454, 0x17); // tlpx
	sensor_write_register(ViPipe, 0x3455, 0x00); // tlpx
	sensor_write_register(ViPipe, 0x3472, 0x20); // x_out_size
	sensor_write_register(ViPipe, 0x3473, 0x05); // x_out_size
	sensor_write_register(ViPipe, 0x347B, 0x23); //
	sensor_write_register(ViPipe, 0x3480, 0x49); // incksel7

	sensor_write_register(ViPipe, 0x3000, 0x00); // STANDBY
	// delay_ms(20); // only needed in slave mode
	sensor_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
	sensor_write_register(ViPipe, 0x304b, 0x0a);

	printf("===Imx327 sensor 720P15fps 10bit 2to1 WDR(30fps->15fps) init success!=====\n");
}
