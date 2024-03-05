#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cvi_vip_snsr.h>

#include "cvi_sns_ctrl.h"
#include "cvi_comm_video.h"
#include "cvi_sns_ctrl.h"
#include "sc8238_cmos_ex.h"

static void sc8238_wdr_2160p15_2to1_init(VI_PIPE ViPipe);
static void sc8238_linear_2160p30_init(VI_PIPE ViPipe);

const CVI_U8 sc8238_i2c_addr = 0x30;        /* I2C Address of SC8238 */
const CVI_U32 sc8238_addr_byte = 2;
const CVI_U32 sc8238_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int sc8238_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunSC8238_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, sc8238_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int sc8238_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int sc8238_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return 0;

	if (sc8238_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, sc8238_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, sc8238_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (sc8238_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int sc8238_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

//	if (sc8238_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
//	}

//	if (sc8238_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
//	}

	ret = write(g_fd[ViPipe], buf, sc8238_addr_byte + sc8238_data_byte);
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

void sc8238_standby(VI_PIPE ViPipe)
{
	sc8238_write_register(ViPipe, 0x0100, 0x00);
}

void sc8238_restart(VI_PIPE ViPipe)
{
	sc8238_write_register(ViPipe, 0x0103, 0x01);
	delay_ms(20);
	sc8238_write_register(ViPipe, 0x0103, 0x00);
}

void sc8238_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastSC8238[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sc8238_write_register(ViPipe,
				g_pastSC8238[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastSC8238[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void sc8238_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
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
	sc8238_write_register(ViPipe, 0x3221, value);
}

void sc8238_init(VI_PIPE ViPipe)
{
	WDR_MODE_E       enWDRMode;
	CVI_BOOL          bInit;
	CVI_U8            u8ImgMode;

	bInit       = g_pastSC8238[ViPipe]->bInit;
	enWDRMode   = g_pastSC8238[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastSC8238[ViPipe]->u8ImgMode;

	sc8238_i2c_init(ViPipe);

	/* When sensor first init, config all registers */
	if (bInit == CVI_FALSE) {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == SC8238_MODE_2160P15_WDR) {
				/* SC8238_MODE_2160P15_WDR */
				sc8238_wdr_2160p15_2to1_init(ViPipe);
			} else {
			}
		} else {
			sc8238_linear_2160p30_init(ViPipe);
		}
	}
	/* When sensor switch mode(linear<->WDR or resolution), config different registers(if possible) */
	else {
		if (enWDRMode == WDR_MODE_2To1_LINE) {
			if (u8ImgMode == SC8238_MODE_2160P15_WDR) {
				/* SC8238_MODE_2160P15_WDR */
				sc8238_wdr_2160p15_2to1_init(ViPipe);
			} else {
			}
		} else {
			sc8238_linear_2160p30_init(ViPipe);
		}
	}
	sc8238_mirror_flip(ViPipe, g_aeSC8238_MirrorFip[ViPipe]);
	g_pastSC8238[ViPipe]->bInit = CVI_TRUE;
}

void sc8238_exit(VI_PIPE ViPipe)
{
	sc8238_i2c_exit(ViPipe);
}

/* 2160P30 and 2160P25 */
static void sc8238_linear_2160p30_init(VI_PIPE ViPipe)
{
	/* 4ms delay for Power On Sequence */
	delay_ms(4);
	sc8238_write_register(ViPipe, 0x0103, 0x01);
	sc8238_write_register(ViPipe, 0x0100, 0x00);
	sc8238_write_register(ViPipe, 0x36e9, 0x80);
	sc8238_write_register(ViPipe, 0x36f9, 0x80);
	sc8238_write_register(ViPipe, 0x3018, 0x72);
	sc8238_write_register(ViPipe, 0x3019, 0x00);
	sc8238_write_register(ViPipe, 0x301f, 0x02);
	sc8238_write_register(ViPipe, 0x3031, 0x0a);
	sc8238_write_register(ViPipe, 0x3037, 0x20);
	sc8238_write_register(ViPipe, 0x3038, 0x44);
	sc8238_write_register(ViPipe, 0x3203, 0x08);
	sc8238_write_register(ViPipe, 0x3207, 0x87);
	sc8238_write_register(ViPipe, 0x320c, 0x08);
	sc8238_write_register(ViPipe, 0x320d, 0x34);
	sc8238_write_register(ViPipe, 0x3213, 0x08);
	sc8238_write_register(ViPipe, 0x3241, 0x00);
	sc8238_write_register(ViPipe, 0x3243, 0x03);
	sc8238_write_register(ViPipe, 0x3248, 0x04);
	sc8238_write_register(ViPipe, 0x3271, 0x1c);
	sc8238_write_register(ViPipe, 0x3273, 0x1f);
	sc8238_write_register(ViPipe, 0x3301, 0x1c);
	sc8238_write_register(ViPipe, 0x3306, 0xa8);
	sc8238_write_register(ViPipe, 0x3308, 0x20);
	sc8238_write_register(ViPipe, 0x3309, 0x68);
	sc8238_write_register(ViPipe, 0x330b, 0x48);
	sc8238_write_register(ViPipe, 0x330d, 0x28);
	sc8238_write_register(ViPipe, 0x330e, 0x58);
	sc8238_write_register(ViPipe, 0x3314, 0x94);
	sc8238_write_register(ViPipe, 0x331f, 0x59);
	sc8238_write_register(ViPipe, 0x3332, 0x24);
	sc8238_write_register(ViPipe, 0x334c, 0x10);
	sc8238_write_register(ViPipe, 0x3350, 0x24);
	sc8238_write_register(ViPipe, 0x3358, 0x24);
	sc8238_write_register(ViPipe, 0x335c, 0x24);
	sc8238_write_register(ViPipe, 0x335d, 0x60);
	sc8238_write_register(ViPipe, 0x3364, 0x16);
	sc8238_write_register(ViPipe, 0x3366, 0x92);
	sc8238_write_register(ViPipe, 0x3367, 0x08);
	sc8238_write_register(ViPipe, 0x3368, 0x07);
	sc8238_write_register(ViPipe, 0x3369, 0x00);
	sc8238_write_register(ViPipe, 0x336a, 0x00);
	sc8238_write_register(ViPipe, 0x336b, 0x00);
	sc8238_write_register(ViPipe, 0x336c, 0xc2);
	sc8238_write_register(ViPipe, 0x337f, 0x33);
	sc8238_write_register(ViPipe, 0x3390, 0x08);
	sc8238_write_register(ViPipe, 0x3391, 0x18);
	sc8238_write_register(ViPipe, 0x3392, 0x38);
	sc8238_write_register(ViPipe, 0x3393, 0x1c);
	sc8238_write_register(ViPipe, 0x3394, 0x28);
	sc8238_write_register(ViPipe, 0x3395, 0x60);
	sc8238_write_register(ViPipe, 0x3396, 0x08);
	sc8238_write_register(ViPipe, 0x3397, 0x18);
	sc8238_write_register(ViPipe, 0x3398, 0x38);
	sc8238_write_register(ViPipe, 0x3399, 0x1c);
	sc8238_write_register(ViPipe, 0x339a, 0x1c);
	sc8238_write_register(ViPipe, 0x339b, 0x28);
	sc8238_write_register(ViPipe, 0x339c, 0x60);
	sc8238_write_register(ViPipe, 0x339e, 0x24);
	sc8238_write_register(ViPipe, 0x33aa, 0x24);
	sc8238_write_register(ViPipe, 0x33af, 0x48);
	sc8238_write_register(ViPipe, 0x33e1, 0x08);
	sc8238_write_register(ViPipe, 0x33e2, 0x18);
	sc8238_write_register(ViPipe, 0x33e3, 0x10);
	sc8238_write_register(ViPipe, 0x33e4, 0x0c);
	sc8238_write_register(ViPipe, 0x33e5, 0x10);
	sc8238_write_register(ViPipe, 0x33e6, 0x06);
	sc8238_write_register(ViPipe, 0x33e7, 0x02);
	sc8238_write_register(ViPipe, 0x33e8, 0x18);
	sc8238_write_register(ViPipe, 0x33e9, 0x10);
	sc8238_write_register(ViPipe, 0x33ea, 0x0c);
	sc8238_write_register(ViPipe, 0x33eb, 0x10);
	sc8238_write_register(ViPipe, 0x33ec, 0x04);
	sc8238_write_register(ViPipe, 0x33ed, 0x02);
	sc8238_write_register(ViPipe, 0x33ee, 0xa0);
	sc8238_write_register(ViPipe, 0x33ef, 0x08);
	sc8238_write_register(ViPipe, 0x33f4, 0x18);
	sc8238_write_register(ViPipe, 0x33f5, 0x10);
	sc8238_write_register(ViPipe, 0x33f6, 0x0c);
	sc8238_write_register(ViPipe, 0x33f7, 0x10);
	sc8238_write_register(ViPipe, 0x33f8, 0x06);
	sc8238_write_register(ViPipe, 0x33f9, 0x02);
	sc8238_write_register(ViPipe, 0x33fa, 0x18);
	sc8238_write_register(ViPipe, 0x33fb, 0x10);
	sc8238_write_register(ViPipe, 0x33fc, 0x0c);
	sc8238_write_register(ViPipe, 0x33fd, 0x10);
	sc8238_write_register(ViPipe, 0x33fe, 0x04);
	sc8238_write_register(ViPipe, 0x33ff, 0x02);
	sc8238_write_register(ViPipe, 0x360f, 0x01);
	sc8238_write_register(ViPipe, 0x3622, 0xf7);
	sc8238_write_register(ViPipe, 0x3624, 0x45);
	sc8238_write_register(ViPipe, 0x3628, 0x83);
	sc8238_write_register(ViPipe, 0x3630, 0x80);
	sc8238_write_register(ViPipe, 0x3631, 0x80);
	sc8238_write_register(ViPipe, 0x3632, 0xa8);
	sc8238_write_register(ViPipe, 0x3633, 0x53);
	sc8238_write_register(ViPipe, 0x3635, 0x02);
	sc8238_write_register(ViPipe, 0x3637, 0x52);
	sc8238_write_register(ViPipe, 0x3638, 0x0a);
	sc8238_write_register(ViPipe, 0x363a, 0x88);
	sc8238_write_register(ViPipe, 0x363b, 0x06);
	sc8238_write_register(ViPipe, 0x363d, 0x01);
	sc8238_write_register(ViPipe, 0x363e, 0x00);
	sc8238_write_register(ViPipe, 0x3641, 0x00);
	sc8238_write_register(ViPipe, 0x3670, 0x4a);
	sc8238_write_register(ViPipe, 0x3671, 0xf7);
	sc8238_write_register(ViPipe, 0x3672, 0xf7);
	sc8238_write_register(ViPipe, 0x3673, 0x17);
	sc8238_write_register(ViPipe, 0x3674, 0x80);
	sc8238_write_register(ViPipe, 0x3675, 0x85);
	sc8238_write_register(ViPipe, 0x3676, 0xa5);
	sc8238_write_register(ViPipe, 0x367a, 0x48);
	sc8238_write_register(ViPipe, 0x367b, 0x78);
	sc8238_write_register(ViPipe, 0x367c, 0x48);
	sc8238_write_register(ViPipe, 0x367d, 0x78);
	sc8238_write_register(ViPipe, 0x3690, 0x53);
	sc8238_write_register(ViPipe, 0x3691, 0x63);
	sc8238_write_register(ViPipe, 0x3692, 0x54);
	sc8238_write_register(ViPipe, 0x3699, 0x88);
	sc8238_write_register(ViPipe, 0x369a, 0x9f);
	sc8238_write_register(ViPipe, 0x369b, 0x9f);
	sc8238_write_register(ViPipe, 0x369c, 0x48);
	sc8238_write_register(ViPipe, 0x369d, 0x78);
	sc8238_write_register(ViPipe, 0x36a2, 0x48);
	sc8238_write_register(ViPipe, 0x36a3, 0x78);
	sc8238_write_register(ViPipe, 0x36bb, 0x48);
	sc8238_write_register(ViPipe, 0x36bc, 0x78);
	sc8238_write_register(ViPipe, 0x36c9, 0x05);
	sc8238_write_register(ViPipe, 0x36ca, 0x05);
	sc8238_write_register(ViPipe, 0x36cb, 0x05);
	sc8238_write_register(ViPipe, 0x36cc, 0x00);
	sc8238_write_register(ViPipe, 0x36cd, 0x10);
	sc8238_write_register(ViPipe, 0x36ce, 0x1a);
	sc8238_write_register(ViPipe, 0x36d0, 0x30);
	sc8238_write_register(ViPipe, 0x36d1, 0x48);
	sc8238_write_register(ViPipe, 0x36d2, 0x78);
	sc8238_write_register(ViPipe, 0x36ea, 0x39);
	sc8238_write_register(ViPipe, 0x36eb, 0x06);
	sc8238_write_register(ViPipe, 0x36ec, 0x05);
	sc8238_write_register(ViPipe, 0x36ed, 0x24);
	sc8238_write_register(ViPipe, 0x36fa, 0x39);
	sc8238_write_register(ViPipe, 0x36fb, 0x13);
	sc8238_write_register(ViPipe, 0x36fc, 0x10);
	sc8238_write_register(ViPipe, 0x36fd, 0x14);
	sc8238_write_register(ViPipe, 0x3901, 0x00);
	sc8238_write_register(ViPipe, 0x3902, 0xc5);
	sc8238_write_register(ViPipe, 0x3904, 0x18);
	sc8238_write_register(ViPipe, 0x3905, 0xd8);
	sc8238_write_register(ViPipe, 0x394c, 0x0f);
	sc8238_write_register(ViPipe, 0x394d, 0x20);
	sc8238_write_register(ViPipe, 0x394e, 0x08);
	sc8238_write_register(ViPipe, 0x394f, 0x90);
	sc8238_write_register(ViPipe, 0x3980, 0x71);
	sc8238_write_register(ViPipe, 0x3981, 0x70);
	sc8238_write_register(ViPipe, 0x3982, 0x00);
	sc8238_write_register(ViPipe, 0x3983, 0x00);
	sc8238_write_register(ViPipe, 0x3984, 0x20);
	sc8238_write_register(ViPipe, 0x3987, 0x0b);
	sc8238_write_register(ViPipe, 0x3990, 0x03);
	sc8238_write_register(ViPipe, 0x3991, 0xfd);
	sc8238_write_register(ViPipe, 0x3992, 0x03);
	sc8238_write_register(ViPipe, 0x3993, 0xfc);
	sc8238_write_register(ViPipe, 0x3994, 0x00);
	sc8238_write_register(ViPipe, 0x3995, 0x00);
	sc8238_write_register(ViPipe, 0x3996, 0x00);
	sc8238_write_register(ViPipe, 0x3997, 0x05);
	sc8238_write_register(ViPipe, 0x3998, 0x00);
	sc8238_write_register(ViPipe, 0x3999, 0x09);
	sc8238_write_register(ViPipe, 0x399a, 0x00);
	sc8238_write_register(ViPipe, 0x399b, 0x12);
	sc8238_write_register(ViPipe, 0x399c, 0x00);
	sc8238_write_register(ViPipe, 0x399d, 0x12);
	sc8238_write_register(ViPipe, 0x399e, 0x00);
	sc8238_write_register(ViPipe, 0x399f, 0x18);
	sc8238_write_register(ViPipe, 0x39a0, 0x00);
	sc8238_write_register(ViPipe, 0x39a1, 0x14);
	sc8238_write_register(ViPipe, 0x39a2, 0x03);
	sc8238_write_register(ViPipe, 0x39a3, 0xe3);
	sc8238_write_register(ViPipe, 0x39a4, 0x03);
	sc8238_write_register(ViPipe, 0x39a5, 0xf2);
	sc8238_write_register(ViPipe, 0x39a6, 0x03);
	sc8238_write_register(ViPipe, 0x39a7, 0xf6);
	sc8238_write_register(ViPipe, 0x39a8, 0x03);
	sc8238_write_register(ViPipe, 0x39a9, 0xfa);
	sc8238_write_register(ViPipe, 0x39aa, 0x03);
	sc8238_write_register(ViPipe, 0x39ab, 0xff);
	sc8238_write_register(ViPipe, 0x39ac, 0x00);
	sc8238_write_register(ViPipe, 0x39ad, 0x06);
	sc8238_write_register(ViPipe, 0x39ae, 0x00);
	sc8238_write_register(ViPipe, 0x39af, 0x09);
	sc8238_write_register(ViPipe, 0x39b0, 0x00);
	sc8238_write_register(ViPipe, 0x39b1, 0x12);
	sc8238_write_register(ViPipe, 0x39b2, 0x00);
	sc8238_write_register(ViPipe, 0x39b3, 0x22);
	sc8238_write_register(ViPipe, 0x39b4, 0x0c);
	sc8238_write_register(ViPipe, 0x39b5, 0x1c);
	sc8238_write_register(ViPipe, 0x39b6, 0x38);
	sc8238_write_register(ViPipe, 0x39b7, 0x5b);
	sc8238_write_register(ViPipe, 0x39b8, 0x50);
	sc8238_write_register(ViPipe, 0x39b9, 0x38);
	sc8238_write_register(ViPipe, 0x39ba, 0x20);
	sc8238_write_register(ViPipe, 0x39bb, 0x10);
	sc8238_write_register(ViPipe, 0x39bc, 0x0c);
	sc8238_write_register(ViPipe, 0x39bd, 0x16);
	sc8238_write_register(ViPipe, 0x39be, 0x21);
	sc8238_write_register(ViPipe, 0x39bf, 0x36);
	sc8238_write_register(ViPipe, 0x39c0, 0x3b);
	sc8238_write_register(ViPipe, 0x39c1, 0x2a);
	sc8238_write_register(ViPipe, 0x39c2, 0x16);
	sc8238_write_register(ViPipe, 0x39c3, 0x0c);
	sc8238_write_register(ViPipe, 0x39c5, 0x30);
	sc8238_write_register(ViPipe, 0x39c6, 0x07);
	sc8238_write_register(ViPipe, 0x39c7, 0xf8);
	sc8238_write_register(ViPipe, 0x39c9, 0x07);
	sc8238_write_register(ViPipe, 0x39ca, 0xf8);
	sc8238_write_register(ViPipe, 0x39cc, 0x00);
	sc8238_write_register(ViPipe, 0x39cd, 0x1b);
	sc8238_write_register(ViPipe, 0x39ce, 0x00);
	sc8238_write_register(ViPipe, 0x39cf, 0x00);
	sc8238_write_register(ViPipe, 0x39d0, 0x1b);
	sc8238_write_register(ViPipe, 0x39d1, 0x00);
	sc8238_write_register(ViPipe, 0x39e2, 0x15);
	sc8238_write_register(ViPipe, 0x39e3, 0x87);
	sc8238_write_register(ViPipe, 0x39e4, 0x12);
	sc8238_write_register(ViPipe, 0x39e5, 0xb7);
	sc8238_write_register(ViPipe, 0x39e6, 0x00);
	sc8238_write_register(ViPipe, 0x39e7, 0x8c);
	sc8238_write_register(ViPipe, 0x39e8, 0x01);
	sc8238_write_register(ViPipe, 0x39e9, 0x31);
	sc8238_write_register(ViPipe, 0x39ea, 0x01);
	sc8238_write_register(ViPipe, 0x39eb, 0xd7);
	sc8238_write_register(ViPipe, 0x39ec, 0x08);
	sc8238_write_register(ViPipe, 0x39ed, 0x00);
	sc8238_write_register(ViPipe, 0x3e00, 0x01);
	sc8238_write_register(ViPipe, 0x3e01, 0x18);
	sc8238_write_register(ViPipe, 0x3e02, 0xa0);
	sc8238_write_register(ViPipe, 0x3e08, 0x03);
	sc8238_write_register(ViPipe, 0x3e09, 0x40);
	sc8238_write_register(ViPipe, 0x3e0e, 0x09);
	sc8238_write_register(ViPipe, 0x3e14, 0x31);
	sc8238_write_register(ViPipe, 0x3e16, 0x00);
	sc8238_write_register(ViPipe, 0x3e17, 0xac);
	sc8238_write_register(ViPipe, 0x3e18, 0x00);
	sc8238_write_register(ViPipe, 0x3e19, 0xac);
	sc8238_write_register(ViPipe, 0x3e1b, 0x3a);
	sc8238_write_register(ViPipe, 0x3e1e, 0x76);
	sc8238_write_register(ViPipe, 0x3e25, 0x23);
	sc8238_write_register(ViPipe, 0x3e26, 0x40);
	sc8238_write_register(ViPipe, 0x4501, 0xa4);
	sc8238_write_register(ViPipe, 0x4509, 0x10);
	sc8238_write_register(ViPipe, 0x4837, 0x1c);
	sc8238_write_register(ViPipe, 0x5799, 0x06);
	sc8238_write_register(ViPipe, 0x57aa, 0x2f);
	sc8238_write_register(ViPipe, 0x57ab, 0xff);
	sc8238_write_register(ViPipe, 0x36e9, 0x53);
	sc8238_write_register(ViPipe, 0x36f9, 0x57);

	sc8238_default_reg_init(ViPipe);

	sc8238_write_register(ViPipe, 0x0100, 0x01);

	printf("ViPipe:%d,===SC8238 2160P 30fps 10bit LINE Init OK!===\n", ViPipe);
}

static void sc8238_wdr_2160p15_2to1_init(VI_PIPE ViPipe)
{
	/* 4ms delay for Power On Sequence */
	delay_ms(4);
	sc8238_write_register(ViPipe, 0x0103, 0x01);
	sc8238_write_register(ViPipe, 0x0100, 0x00);
	sc8238_write_register(ViPipe, 0x36e9, 0x80);
	sc8238_write_register(ViPipe, 0x36f9, 0x80);
	sc8238_write_register(ViPipe, 0x3018, 0x72);
	sc8238_write_register(ViPipe, 0x3019, 0x00);
	sc8238_write_register(ViPipe, 0x301f, 0x23);
	sc8238_write_register(ViPipe, 0x3031, 0x0a);
	sc8238_write_register(ViPipe, 0x3037, 0x20);
	sc8238_write_register(ViPipe, 0x3038, 0x44);
	sc8238_write_register(ViPipe, 0x320c, 0x08);
	sc8238_write_register(ViPipe, 0x320d, 0x34);
	sc8238_write_register(ViPipe, 0x320e, 0x11);
	sc8238_write_register(ViPipe, 0x320f, 0x94);
	sc8238_write_register(ViPipe, 0x3220, 0x50);
	sc8238_write_register(ViPipe, 0x3241, 0x00);
	sc8238_write_register(ViPipe, 0x3243, 0x03);
	sc8238_write_register(ViPipe, 0x3248, 0x04);
	sc8238_write_register(ViPipe, 0x3250, 0x3f);
	sc8238_write_register(ViPipe, 0x3271, 0x1c);
	sc8238_write_register(ViPipe, 0x3273, 0x1f);
	sc8238_write_register(ViPipe, 0x3301, 0x1c);
	sc8238_write_register(ViPipe, 0x3306, 0xa8);
	sc8238_write_register(ViPipe, 0x3308, 0x20);
	sc8238_write_register(ViPipe, 0x3309, 0x68);
	sc8238_write_register(ViPipe, 0x330b, 0x48);
	sc8238_write_register(ViPipe, 0x330d, 0x28);
	sc8238_write_register(ViPipe, 0x330e, 0x58);
	sc8238_write_register(ViPipe, 0x3314, 0x98);
	sc8238_write_register(ViPipe, 0x331f, 0x59);
	sc8238_write_register(ViPipe, 0x3332, 0x24);
	sc8238_write_register(ViPipe, 0x334a, 0x18);
	sc8238_write_register(ViPipe, 0x334c, 0x10);
	sc8238_write_register(ViPipe, 0x3350, 0x24);
	sc8238_write_register(ViPipe, 0x3358, 0x24);
	sc8238_write_register(ViPipe, 0x335c, 0x24);
	sc8238_write_register(ViPipe, 0x335d, 0x60);
	sc8238_write_register(ViPipe, 0x3360, 0x40);
	sc8238_write_register(ViPipe, 0x3362, 0x72);
	sc8238_write_register(ViPipe, 0x3364, 0x16);
	sc8238_write_register(ViPipe, 0x3366, 0x92);
	sc8238_write_register(ViPipe, 0x3367, 0x08);
	sc8238_write_register(ViPipe, 0x3368, 0x10);
	sc8238_write_register(ViPipe, 0x3369, 0x00);
	sc8238_write_register(ViPipe, 0x336a, 0x00);
	sc8238_write_register(ViPipe, 0x336b, 0x00);
	sc8238_write_register(ViPipe, 0x336c, 0xc2);
	sc8238_write_register(ViPipe, 0x336f, 0x58);
	sc8238_write_register(ViPipe, 0x337f, 0x33);
	sc8238_write_register(ViPipe, 0x3390, 0x08);
	sc8238_write_register(ViPipe, 0x3391, 0x18);
	sc8238_write_register(ViPipe, 0x3392, 0x38);
	sc8238_write_register(ViPipe, 0x3393, 0x1c);
	sc8238_write_register(ViPipe, 0x3394, 0x28);
	sc8238_write_register(ViPipe, 0x3395, 0x60);
	sc8238_write_register(ViPipe, 0x3396, 0x08);
	sc8238_write_register(ViPipe, 0x3397, 0x18);
	sc8238_write_register(ViPipe, 0x3398, 0x38);
	sc8238_write_register(ViPipe, 0x3399, 0x1c);
	sc8238_write_register(ViPipe, 0x339a, 0x1c);
	sc8238_write_register(ViPipe, 0x339b, 0x28);
	sc8238_write_register(ViPipe, 0x339c, 0x60);
	sc8238_write_register(ViPipe, 0x339e, 0x24);
	sc8238_write_register(ViPipe, 0x33aa, 0x24);
	sc8238_write_register(ViPipe, 0x33af, 0x48);
	sc8238_write_register(ViPipe, 0x33e1, 0x08);
	sc8238_write_register(ViPipe, 0x33e2, 0x18);
	sc8238_write_register(ViPipe, 0x33e3, 0x10);
	sc8238_write_register(ViPipe, 0x33e4, 0x0c);
	sc8238_write_register(ViPipe, 0x33e5, 0x10);
	sc8238_write_register(ViPipe, 0x33e6, 0x06);
	sc8238_write_register(ViPipe, 0x33e7, 0x02);
	sc8238_write_register(ViPipe, 0x33e8, 0x18);
	sc8238_write_register(ViPipe, 0x33e9, 0x10);
	sc8238_write_register(ViPipe, 0x33ea, 0x0c);
	sc8238_write_register(ViPipe, 0x33eb, 0x10);
	sc8238_write_register(ViPipe, 0x33ec, 0x04);
	sc8238_write_register(ViPipe, 0x33ed, 0x02);
	sc8238_write_register(ViPipe, 0x33ee, 0xa0);
	sc8238_write_register(ViPipe, 0x33ef, 0x08);
	sc8238_write_register(ViPipe, 0x33f4, 0x18);
	sc8238_write_register(ViPipe, 0x33f5, 0x10);
	sc8238_write_register(ViPipe, 0x33f6, 0x0c);
	sc8238_write_register(ViPipe, 0x33f7, 0x10);
	sc8238_write_register(ViPipe, 0x33f8, 0x06);
	sc8238_write_register(ViPipe, 0x33f9, 0x02);
	sc8238_write_register(ViPipe, 0x33fa, 0x18);
	sc8238_write_register(ViPipe, 0x33fb, 0x10);
	sc8238_write_register(ViPipe, 0x33fc, 0x0c);
	sc8238_write_register(ViPipe, 0x33fd, 0x10);
	sc8238_write_register(ViPipe, 0x33fe, 0x04);
	sc8238_write_register(ViPipe, 0x33ff, 0x02);
	sc8238_write_register(ViPipe, 0x360f, 0x01);
	sc8238_write_register(ViPipe, 0x3622, 0xf7);
	sc8238_write_register(ViPipe, 0x3624, 0x45);
	sc8238_write_register(ViPipe, 0x3628, 0x83);
	sc8238_write_register(ViPipe, 0x3630, 0x80);
	sc8238_write_register(ViPipe, 0x3631, 0x80);
	sc8238_write_register(ViPipe, 0x3632, 0xa8);
	sc8238_write_register(ViPipe, 0x3633, 0x23);
	sc8238_write_register(ViPipe, 0x3635, 0x02);
	sc8238_write_register(ViPipe, 0x3636, 0x11);
	sc8238_write_register(ViPipe, 0x3637, 0x10);
	sc8238_write_register(ViPipe, 0x3638, 0x0a);
	sc8238_write_register(ViPipe, 0x363a, 0x88);
	sc8238_write_register(ViPipe, 0x363b, 0x06);
	sc8238_write_register(ViPipe, 0x363d, 0x01);
	sc8238_write_register(ViPipe, 0x363e, 0x00);
	sc8238_write_register(ViPipe, 0x3641, 0x00);
	sc8238_write_register(ViPipe, 0x3670, 0x4a);
	sc8238_write_register(ViPipe, 0x3671, 0xf7);
	sc8238_write_register(ViPipe, 0x3672, 0xf7);
	sc8238_write_register(ViPipe, 0x3673, 0x17);
	sc8238_write_register(ViPipe, 0x3674, 0x80);
	sc8238_write_register(ViPipe, 0x3675, 0x85);
	sc8238_write_register(ViPipe, 0x3676, 0xa5);
	sc8238_write_register(ViPipe, 0x367a, 0x48);
	sc8238_write_register(ViPipe, 0x367b, 0x78);
	sc8238_write_register(ViPipe, 0x367c, 0x48);
	sc8238_write_register(ViPipe, 0x367d, 0x78);
	sc8238_write_register(ViPipe, 0x3690, 0x53);
	sc8238_write_register(ViPipe, 0x3691, 0x63);
	sc8238_write_register(ViPipe, 0x3692, 0x54);
	sc8238_write_register(ViPipe, 0x3699, 0x88);
	sc8238_write_register(ViPipe, 0x369a, 0x88);
	sc8238_write_register(ViPipe, 0x369b, 0x88);
	sc8238_write_register(ViPipe, 0x369c, 0x48);
	sc8238_write_register(ViPipe, 0x369d, 0x78);
	sc8238_write_register(ViPipe, 0x36a2, 0x48);
	sc8238_write_register(ViPipe, 0x36a3, 0x78);
	sc8238_write_register(ViPipe, 0x36bb, 0x48);
	sc8238_write_register(ViPipe, 0x36bc, 0x78);
	sc8238_write_register(ViPipe, 0x36c9, 0x05);
	sc8238_write_register(ViPipe, 0x36ca, 0x05);
	sc8238_write_register(ViPipe, 0x36cb, 0x05);
	sc8238_write_register(ViPipe, 0x36cc, 0x00);
	sc8238_write_register(ViPipe, 0x36cd, 0x10);
	sc8238_write_register(ViPipe, 0x36ce, 0x1a);
	sc8238_write_register(ViPipe, 0x36d0, 0x30);
	sc8238_write_register(ViPipe, 0x36d1, 0x48);
	sc8238_write_register(ViPipe, 0x36d2, 0x78);
	sc8238_write_register(ViPipe, 0x36ea, 0x39);
	sc8238_write_register(ViPipe, 0x36eb, 0x06);
	sc8238_write_register(ViPipe, 0x36ec, 0x05);
	sc8238_write_register(ViPipe, 0x36ed, 0x24);
	sc8238_write_register(ViPipe, 0x36fa, 0x39);
	sc8238_write_register(ViPipe, 0x36fb, 0x13);
	sc8238_write_register(ViPipe, 0x36fc, 0x10);
	sc8238_write_register(ViPipe, 0x36fd, 0x14);
	sc8238_write_register(ViPipe, 0x3802, 0x01);
	sc8238_write_register(ViPipe, 0x3901, 0x00);
	sc8238_write_register(ViPipe, 0x3902, 0xc5);
	sc8238_write_register(ViPipe, 0x3904, 0x18);
	sc8238_write_register(ViPipe, 0x3905, 0xd8);
	sc8238_write_register(ViPipe, 0x394c, 0x0f);
	sc8238_write_register(ViPipe, 0x394d, 0x20);
	sc8238_write_register(ViPipe, 0x394e, 0x08);
	sc8238_write_register(ViPipe, 0x394f, 0x90);
	sc8238_write_register(ViPipe, 0x3980, 0x71);
	sc8238_write_register(ViPipe, 0x3981, 0x70);
	sc8238_write_register(ViPipe, 0x3982, 0x00);
	sc8238_write_register(ViPipe, 0x3983, 0x00);
	sc8238_write_register(ViPipe, 0x3984, 0x20);
	sc8238_write_register(ViPipe, 0x3987, 0x0b);
	sc8238_write_register(ViPipe, 0x3990, 0x03);
	sc8238_write_register(ViPipe, 0x3991, 0xfd);
	sc8238_write_register(ViPipe, 0x3992, 0x03);
	sc8238_write_register(ViPipe, 0x3993, 0xfc);
	sc8238_write_register(ViPipe, 0x3994, 0x00);
	sc8238_write_register(ViPipe, 0x3995, 0x00);
	sc8238_write_register(ViPipe, 0x3996, 0x00);
	sc8238_write_register(ViPipe, 0x3997, 0x05);
	sc8238_write_register(ViPipe, 0x3998, 0x00);
	sc8238_write_register(ViPipe, 0x3999, 0x09);
	sc8238_write_register(ViPipe, 0x399a, 0x00);
	sc8238_write_register(ViPipe, 0x399b, 0x12);
	sc8238_write_register(ViPipe, 0x399c, 0x00);
	sc8238_write_register(ViPipe, 0x399d, 0x12);
	sc8238_write_register(ViPipe, 0x399e, 0x00);
	sc8238_write_register(ViPipe, 0x399f, 0x18);
	sc8238_write_register(ViPipe, 0x39a0, 0x00);
	sc8238_write_register(ViPipe, 0x39a1, 0x14);
	sc8238_write_register(ViPipe, 0x39a2, 0x03);
	sc8238_write_register(ViPipe, 0x39a3, 0xe3);
	sc8238_write_register(ViPipe, 0x39a4, 0x03);
	sc8238_write_register(ViPipe, 0x39a5, 0xf2);
	sc8238_write_register(ViPipe, 0x39a6, 0x03);
	sc8238_write_register(ViPipe, 0x39a7, 0xf6);
	sc8238_write_register(ViPipe, 0x39a8, 0x03);
	sc8238_write_register(ViPipe, 0x39a9, 0xfa);
	sc8238_write_register(ViPipe, 0x39aa, 0x03);
	sc8238_write_register(ViPipe, 0x39ab, 0xff);
	sc8238_write_register(ViPipe, 0x39ac, 0x00);
	sc8238_write_register(ViPipe, 0x39ad, 0x06);
	sc8238_write_register(ViPipe, 0x39ae, 0x00);
	sc8238_write_register(ViPipe, 0x39af, 0x09);
	sc8238_write_register(ViPipe, 0x39b0, 0x00);
	sc8238_write_register(ViPipe, 0x39b1, 0x12);
	sc8238_write_register(ViPipe, 0x39b2, 0x00);
	sc8238_write_register(ViPipe, 0x39b3, 0x22);
	sc8238_write_register(ViPipe, 0x39b4, 0x0c);
	sc8238_write_register(ViPipe, 0x39b5, 0x1c);
	sc8238_write_register(ViPipe, 0x39b6, 0x38);
	sc8238_write_register(ViPipe, 0x39b7, 0x5b);
	sc8238_write_register(ViPipe, 0x39b8, 0x50);
	sc8238_write_register(ViPipe, 0x39b9, 0x38);
	sc8238_write_register(ViPipe, 0x39ba, 0x20);
	sc8238_write_register(ViPipe, 0x39bb, 0x10);
	sc8238_write_register(ViPipe, 0x39bc, 0x0c);
	sc8238_write_register(ViPipe, 0x39bd, 0x16);
	sc8238_write_register(ViPipe, 0x39be, 0x21);
	sc8238_write_register(ViPipe, 0x39bf, 0x36);
	sc8238_write_register(ViPipe, 0x39c0, 0x3b);
	sc8238_write_register(ViPipe, 0x39c1, 0x2a);
	sc8238_write_register(ViPipe, 0x39c2, 0x16);
	sc8238_write_register(ViPipe, 0x39c3, 0x0c);
	sc8238_write_register(ViPipe, 0x39c5, 0x30);
	sc8238_write_register(ViPipe, 0x39c6, 0x07);
	sc8238_write_register(ViPipe, 0x39c7, 0xf8);
	sc8238_write_register(ViPipe, 0x39c9, 0x07);
	sc8238_write_register(ViPipe, 0x39ca, 0xf8);
	sc8238_write_register(ViPipe, 0x39cc, 0x00);
	sc8238_write_register(ViPipe, 0x39cd, 0x1b);
	sc8238_write_register(ViPipe, 0x39ce, 0x00);
	sc8238_write_register(ViPipe, 0x39cf, 0x00);
	sc8238_write_register(ViPipe, 0x39d0, 0x1b);
	sc8238_write_register(ViPipe, 0x39d1, 0x00);
	sc8238_write_register(ViPipe, 0x39e2, 0x15);
	sc8238_write_register(ViPipe, 0x39e3, 0x87);
	sc8238_write_register(ViPipe, 0x39e4, 0x12);
	sc8238_write_register(ViPipe, 0x39e5, 0xb7);
	sc8238_write_register(ViPipe, 0x39e6, 0x00);
	sc8238_write_register(ViPipe, 0x39e7, 0x8c);
	sc8238_write_register(ViPipe, 0x39e8, 0x01);
	sc8238_write_register(ViPipe, 0x39e9, 0x31);
	sc8238_write_register(ViPipe, 0x39ea, 0x01);
	sc8238_write_register(ViPipe, 0x39eb, 0xd7);
	sc8238_write_register(ViPipe, 0x39ec, 0x08);
	sc8238_write_register(ViPipe, 0x39ed, 0x00);
	sc8238_write_register(ViPipe, 0x3e00, 0x02);
	sc8238_write_register(ViPipe, 0x3e01, 0x0f);
	sc8238_write_register(ViPipe, 0x3e02, 0xa0);
	sc8238_write_register(ViPipe, 0x3e04, 0x20);
	sc8238_write_register(ViPipe, 0x3e05, 0xc0);
	sc8238_write_register(ViPipe, 0x3e06, 0x00);
	sc8238_write_register(ViPipe, 0x3e07, 0x80);
	sc8238_write_register(ViPipe, 0x3e08, 0x03);
	sc8238_write_register(ViPipe, 0x3e09, 0x40);
	sc8238_write_register(ViPipe, 0x3e0e, 0x09);
	sc8238_write_register(ViPipe, 0x3e10, 0x00);
	sc8238_write_register(ViPipe, 0x3e11, 0x80);
	sc8238_write_register(ViPipe, 0x3e12, 0x03);
	sc8238_write_register(ViPipe, 0x3e13, 0x40);
	sc8238_write_register(ViPipe, 0x3e14, 0x31);
	sc8238_write_register(ViPipe, 0x3e16, 0x00);
	sc8238_write_register(ViPipe, 0x3e17, 0xac);
	sc8238_write_register(ViPipe, 0x3e18, 0x00);
	sc8238_write_register(ViPipe, 0x3e19, 0xac);
	sc8238_write_register(ViPipe, 0x3e1b, 0x3a);
	sc8238_write_register(ViPipe, 0x3e1e, 0x76);
	sc8238_write_register(ViPipe, 0x3e23, 0x01);
	sc8238_write_register(ViPipe, 0x3e24, 0x0e);
	sc8238_write_register(ViPipe, 0x3e25, 0x23);
	sc8238_write_register(ViPipe, 0x3e26, 0x40);
	sc8238_write_register(ViPipe, 0x4501, 0xa4);
	sc8238_write_register(ViPipe, 0x4509, 0x10);
	sc8238_write_register(ViPipe, 0x4816, 0x51);
	sc8238_write_register(ViPipe, 0x4837, 0x1c);
	sc8238_write_register(ViPipe, 0x5799, 0x06);
	sc8238_write_register(ViPipe, 0x57aa, 0x2f);
	sc8238_write_register(ViPipe, 0x57ab, 0xff);
	sc8238_write_register(ViPipe, 0x36e9, 0x53);
	sc8238_write_register(ViPipe, 0x36f9, 0x57);

	sc8238_default_reg_init(ViPipe);

	/* 1 frame delay for image stable */
	usleep(66*1000);

	sc8238_write_register(ViPipe, 0x0100, 0x01);

	printf("===SC8238 sensor 2160P30fps 10bit 2to1 WDR(30fps->15fps) init success!=====\n");
}
