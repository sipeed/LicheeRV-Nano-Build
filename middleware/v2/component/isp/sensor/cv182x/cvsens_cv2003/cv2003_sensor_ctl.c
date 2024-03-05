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
#include "cv2003_cmos_ex.h"

#define CV2003_CHIP_ID_ADDR_H	0x307A
#define CV2003_CHIP_ID_ADDR_L	0x3138
#define CV2003_CHIP_ID			0x0203

static void cv2003_linear_1080P30_init(VI_PIPE ViPipe);

CVI_U8 cv2003_i2c_addr = 0x35;
const CVI_U32 cv2003_addr_byte = 2;
const CVI_U32 cv2003_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int cv2003_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunCV2003_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, cv2003_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int cv2003_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int cv2003_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (cv2003_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, cv2003_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, cv2003_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (cv2003_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int cv2003_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (cv2003_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (cv2003_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, cv2003_addr_byte + cv2003_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	ret = read(g_fd[ViPipe], buf, cv2003_addr_byte + cv2003_data_byte);
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void cv2003_standby(VI_PIPE ViPipe)
{
	cv2003_write_register(ViPipe, 0x3000, 0x1);

	printf("%s\n", __func__);
}

void cv2003_restart(VI_PIPE ViPipe)
{
	cv2003_write_register(ViPipe, 0x3000, 0x01);
	delay_ms(20);
	cv2003_write_register(ViPipe, 0x3000, 0x00);

	printf("%s\n", __func__);
}

void cv2003_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastCV2003[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		cv2003_write_register(ViPipe,
				g_pastCV2003[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastCV2003[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

int cv2003_probe(VI_PIPE ViPipe)
{
	int nVal;
	int nVal2;

	usleep(50);
	if (cv2003_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = cv2003_read_register(ViPipe, CV2003_CHIP_ID_ADDR_H);
	nVal2 = cv2003_read_register(ViPipe, CV2003_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != CV2003_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void cv2003_init(VI_PIPE ViPipe)
{
	CVI_U8            u8ImgMode;

	u8ImgMode   = g_pastCV2003[ViPipe]->u8ImgMode;

	cv2003_i2c_init(ViPipe);

	if (u8ImgMode == CV2003_MODE_1920X1080P30) {
		cv2003_linear_1080P30_init(ViPipe);
	}
	// }

	g_pastCV2003[ViPipe]->bInit = CVI_TRUE;
}

void cv2003_exit(VI_PIPE ViPipe)
{
	cv2003_i2c_exit(ViPipe);
}

static void cv2003_linear_1080P30_init(VI_PIPE ViPipe)
{
	delay_ms(10);

	//30fps
	cv2003_write_register(ViPipe, 0x3300, 0x03);
	cv2003_write_register(ViPipe, 0x3422, 0xBF);
	cv2003_write_register(ViPipe, 0x3401, 0x00);
	cv2003_write_register(ViPipe, 0x3440, 0x01);
	cv2003_write_register(ViPipe, 0x3442, 0x00);
	// cv2003_write_register(ViPipe, 0x3460, 0x03);//drive capability
	cv2003_write_register(ViPipe, 0x3806, 0x00);
	cv2003_write_register(ViPipe, 0x3908, 0x5F);
	cv2003_write_register(ViPipe, 0x3909, 0x00);
	cv2003_write_register(ViPipe, 0x3929, 0x01);
	cv2003_write_register(ViPipe, 0x3158, 0x01);
	cv2003_write_register(ViPipe, 0x3159, 0x01);
	cv2003_write_register(ViPipe, 0x315A, 0x01);
	cv2003_write_register(ViPipe, 0x315B, 0x01);
	cv2003_write_register(ViPipe, 0x35b3, 0x15);
	cv2003_write_register(ViPipe, 0x3148, 0x64);
	cv2003_write_register(ViPipe, 0x3031, 0x00);
	cv2003_write_register(ViPipe, 0x3118, 0x01);
	cv2003_write_register(ViPipe, 0x3119, 0x06);
	cv2003_write_register(ViPipe, 0x3670, 0x00);
	cv2003_write_register(ViPipe, 0x3679, 0x02);
	cv2003_write_register(ViPipe, 0x3330, 0x00);
	cv2003_write_register(ViPipe, 0x320e, 0x02);
	cv2003_write_register(ViPipe, 0x3804, 0x10);
	cv2003_write_register(ViPipe, 0x35a1, 0x06);
	cv2003_write_register(ViPipe, 0x35a8, 0x06);
	cv2003_write_register(ViPipe, 0x35a9, 0x06);
	cv2003_write_register(ViPipe, 0x35aa, 0x06);
	cv2003_write_register(ViPipe, 0x35ab, 0x06);
	cv2003_write_register(ViPipe, 0x35ac, 0x06);
	cv2003_write_register(ViPipe, 0x35ad, 0x06);
	cv2003_write_register(ViPipe, 0x35ae, 0x07);
	cv2003_write_register(ViPipe, 0x35af, 0x07);
	cv2003_write_register(ViPipe, 0x333B, 0x01);
	cv2003_write_register(ViPipe, 0x3338, 0x08);
	cv2003_write_register(ViPipe, 0x3339, 0x00);
	cv2003_write_register(ViPipe, 0x3144, 0x20);
	cv2003_write_register(ViPipe, 0x301c, 0x00);
	cv2003_write_register(ViPipe, 0x3030, 0x01);
	cv2003_write_register(ViPipe, 0x3020, 0xCA);
	cv2003_write_register(ViPipe, 0x3021, 0x08);
	cv2003_write_register(ViPipe, 0x3024, 0x80);
	cv2003_write_register(ViPipe, 0x3025, 0x02);
	cv2003_write_register(ViPipe, 0x3038, 0x04);
	cv2003_write_register(ViPipe, 0x3039, 0x00);
	cv2003_write_register(ViPipe, 0x303A, 0x80);
	cv2003_write_register(ViPipe, 0x303B, 0x07);
	cv2003_write_register(ViPipe, 0x3034, 0x04);
	cv2003_write_register(ViPipe, 0x3035, 0x00);
	cv2003_write_register(ViPipe, 0x3036, 0x38);
	cv2003_write_register(ViPipe, 0x3037, 0x04);
	cv2003_write_register(ViPipe, 0x3908, 0x48);
	cv2003_write_register(ViPipe, 0x390A, 0x02);

	//slave mode
	// cv2003_write_register(ViPipe, 0x3001, 0x01);
	// cv2003_write_register(ViPipe, 0x307A, 0x02);
	// cv2003_write_register(ViPipe, 0x306D, 0x0F);

	cv2003_default_reg_init(ViPipe);
	delay_ms(100);
	cv2003_write_register(ViPipe, 0x3000, 0x00);

	printf("ViPipe:%d,===CV2003 1080P 30fps 12bit LINEAR Init OK!===\n", ViPipe);
}
