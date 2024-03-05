#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/cvi_vip_snsr.h>
#include <linux/spi/spidev.h>
#include "cvi_sns_ctrl.h"
#include "cvi_comm_video.h"
#include "pr2100_cmos_ex.h"
#include <pthread.h>
#include <signal.h>

static void pr2100_set_1080p(VI_PIPE ViPipe);
static void pr2100_set_1080p_2ch(VI_PIPE ViPipe);
static void pr2100_set_1080p_4ch(VI_PIPE ViPipe);

const CVI_U8 pr2100_master_i2c_addr = 0x5C;        /* I2C slave address of PR2100 master chip*/
const CVI_U8 pr2100_slave_i2c_addr = 0x5F;         /* I2C slave address of PR2100 slave chip*/
const CVI_U32 pr2100_addr_byte = 1;
const CVI_U32 pr2100_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};
static VI_PIPE slave_pipe = (VI_MAX_PIPE_NUM - 1);

#define PR2100_TEST_PATTERN 0

#if (PR2100_TEST_PATTERN)
//1920x1080 offset  0x0   0x1FA400  0x278D00
// 0 : White        eb       80       80
// 1 : Yellow       d2       10       92
// 2 : Cyan         aa       a6       10
// 3 : Green        91       36       22
// 4 : Magenta      6a       ca       de
// 5 : Red          51       5a       f0
// 6 : Blue
// 7 : Black
// 8 : Color Bar
// 9 : Ramp
// 10 : Inverse Color Bar
// 11 : Combination Pattern
#define output_pattern_ch0 (0x8 | 0x80)	// color bar
#define output_pattern_ch1 (0x8 | 0xA0)	// reverse color bar
#define output_pattern_ch2 (0x8 | 0x80)	// color bar
#define output_pattern_ch3 (0x8 | 0xA0)	// reverse color bar
#else
#define output_pattern_ch0 (0x00)
#define output_pattern_ch1 (0x00)
#define output_pattern_ch2 (0x00)
#define output_pattern_ch3 (0x00)
#endif

/*gpio*/
enum CVI_GPIO_NUM_E {
CVI_GPIOD_00 = 404,
CVI_GPIOD_01,   CVI_GPIOD_02,   CVI_GPIOD_03,   CVI_GPIOD_04,   CVI_GPIOD_05,
CVI_GPIOD_06,   CVI_GPIOD_07,   CVI_GPIOD_08,   CVI_GPIOD_09,   CVI_GPIOD_10,
CVI_GPIOD_11,
CVI_GPIOC_00 = 416,
CVI_GPIOC_01,   CVI_GPIOC_02,   CVI_GPIOC_03,   CVI_GPIOC_04,   CVI_GPIOC_05,
CVI_GPIOC_06,   CVI_GPIOC_07,   CVI_GPIOC_08,   CVI_GPIOC_09,   CVI_GPIOC_10,
CVI_GPIOC_11,   CVI_GPIOC_12,   CVI_GPIOC_13,   CVI_GPIOC_14,   CVI_GPIOC_15,
CVI_GPIOC_16,   CVI_GPIOC_17,   CVI_GPIOC_18,   CVI_GPIOC_19,   CVI_GPIOC_20,
CVI_GPIOC_21,   CVI_GPIOC_22,   CVI_GPIOC_23,   CVI_GPIOC_24,   CVI_GPIOC_25,
CVI_GPIOC_26,   CVI_GPIOC_27,   CVI_GPIOC_28,   CVI_GPIOC_29,   CVI_GPIOC_30,
CVI_GPIOC_31,
CVI_GPIOB_00 = 448,
CVI_GPIOB_01,   CVI_GPIOB_02,   CVI_GPIOB_03,   CVI_GPIOB_04,   CVI_GPIOB_05,
CVI_GPIOB_06,   CVI_GPIOB_07,   CVI_GPIOB_08,   CVI_GPIOB_09,   CVI_GPIOB_10,
CVI_GPIOB_11,   CVI_GPIOB_12,   CVI_GPIOB_13,   CVI_GPIOB_14,   CVI_GPIOB_15,
CVI_GPIOB_16,   CVI_GPIOB_17,   CVI_GPIOB_18,   CVI_GPIOB_19,   CVI_GPIOB_20,
CVI_GPIOB_21,   CVI_GPIOB_22,   CVI_GPIOB_23,   CVI_GPIOB_24,   CVI_GPIOB_25,
CVI_GPIOB_26,   CVI_GPIOB_27,   CVI_GPIOB_28,   CVI_GPIOB_29,   CVI_GPIOB_30,
CVI_GPIOB_31,
CVI_GPIOA_00 = 480,
CVI_GPIOA_01,   CVI_GPIOA_02,   CVI_GPIOA_03,   CVI_GPIOA_04,   CVI_GPIOA_05,
CVI_GPIOA_06,   CVI_GPIOA_07,   CVI_GPIOA_08,   CVI_GPIOA_09,   CVI_GPIOA_10,
CVI_GPIOA_11,   CVI_GPIOA_12,   CVI_GPIOA_13,   CVI_GPIOA_14,   CVI_GPIOA_15,
CVI_GPIOA_16,   CVI_GPIOA_17,   CVI_GPIOA_18,   CVI_GPIOA_19,   CVI_GPIOA_20,
CVI_GPIOA_21,   CVI_GPIOA_22,   CVI_GPIOA_23,   CVI_GPIOA_24,   CVI_GPIOA_25,
CVI_GPIOA_26,   CVI_GPIOA_27,   CVI_GPIOA_28,   CVI_GPIOA_29,   CVI_GPIOA_30,
CVI_GPIOA_31,
};

#define CVI_GPIO_MIN CVI_GPIOD_00
#define CVI_GPIO_MAX CVI_GPIOA_31

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

static int PR2100_GPIO_Export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR"/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

static int PR2100_GPIO_SetDirection(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR"/gpio%d/direction", gpio);
	if (access(buf, 0) == -1)
		PR2100_GPIO_Export(gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
	//printf("mark %d , %s\n",out_flag, buf);
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);

	close(fd);
	return 0;
}

static int PR2100_GPIO_SetValue(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR"/gpio%d/value", gpio);
	if (access(buf, 0) == -1)
		PR2100_GPIO_Export(gpio);

	PR2100_GPIO_SetDirection(gpio, 1); //output

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}

	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);

	close(fd);
	return 0;
}

int pr2100_sys_init(VI_PIPE ViPipe)
{
	(void) ViPipe;

	//CAM_PEN
	if (PR2100_GPIO_SetValue(CVI_GPIOA_06, 1) != 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "set power down gpio error!\n");
		return CVI_FAILURE;
	}
#if 0
	//SENSOR0_RSTN
	if (PR2100_GPIO_SetValue(CVI_GPIOD_07, 1) != 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "set reset gpio error!\n");
		return CVI_FAILURE;
	}

	//BACK_DET
	if (PR2100_GPIO_SetValue(CVI_GPIOD_01, 1) != 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "set back detect gpio error!\n");
		return CVI_FAILURE;
	}
#endif
	return CVI_SUCCESS;
}

int pr2100_i2c_init(VI_PIPE ViPipe, CVI_U8 i2c_addr)
{
	int ret;
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;

	u8DevNum = g_aunPr2100_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
	CVI_TRACE_SNS(CVI_DBG_INFO, "open %s\n", acDevFile);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int pr2100_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int pr2100_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return 0;

	if (pr2100_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, pr2100_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, pr2100_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (pr2100_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	CVI_TRACE_SNS(CVI_DBG_INFO, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int pr2100_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (pr2100_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	if (pr2100_data_byte == 2)
		buf[idx++] = (data >> 8) & 0xff;

	// add data byte 0
	buf[idx++] = data & 0xff;

	ret = write(g_fd[ViPipe], buf, pr2100_addr_byte + pr2100_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	CVI_TRACE_SNS(CVI_DBG_INFO, "i2c w 0x%x 0x%x\n", addr, data);

#if 0 // read back checing
	ret = pr2100_read_register(ViPipe, addr);
	if (ret != data)
		CVI_TRACE_SNS(CVI_DBG_INFO, "i2c readback-check fail, 0x%x != 0x%x\n", ret, data);
#endif
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void pr2100_init(VI_PIPE ViPipe)
{
	CVI_U8 u8ImgMode;

	u8ImgMode = g_pastPr2100[ViPipe]->u8ImgMode;

	if (pr2100_sys_init(ViPipe) != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "PR2100 sys init fail\n");
		return;
	}

	delay_ms(20);

	if (pr2100_i2c_init(ViPipe, pr2100_master_i2c_addr) != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "PR2100 master i2c init fail\n");
		return;
	}

	// check sensor chip id
	pr2100_write_register(ViPipe, 0xff, 0x00);
	if (((pr2100_read_register(ViPipe, 0xfc) << 8) |
	     (pr2100_read_register(ViPipe, 0xfd))) != 0x2100) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "PR2100 master chip id check fail\n");
		return;
	}

	CVI_TRACE_SNS(CVI_DBG_INFO, "Loading Pixelplus PR2100 sensor\n");

	switch (u8ImgMode) {
	case PR2100_MODE_1080P: {
		pr2100_set_1080p(ViPipe);
		break;
	}
	case PR2100_MODE_1080P_2CH: {
		pr2100_set_1080p_2ch(ViPipe);
		break;
	}
	case PR2100_MODE_1080P_4CH: {
		g_aunPr2100_BusInfo[slave_pipe].s8I2cDev = g_aunPr2100_BusInfo[ViPipe].s8I2cDev;
		if (pr2100_i2c_init(slave_pipe, pr2100_slave_i2c_addr) != CVI_SUCCESS) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "PR2100 slave i2c init fail\n");
			break;
		}
		pr2100_write_register(slave_pipe, 0xff, 0x00);
		if (((pr2100_read_register(slave_pipe, 0xfc) << 8) |
		     (pr2100_read_register(slave_pipe, 0xfd))) != 0x2100) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "PR2100 slave chip id check fail\n");
			break;
		}
		pr2100_set_1080p_4ch(ViPipe);
		break;
	}
	default:
		break;
	}

	// wait for signal to stabilize
	delay_ms(800);
	pr2100_write_register(ViPipe, 0xff, 0x00);//page0
}

void pr2100_exit(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "Exit Pixelplus PR2100 Sensor\n");

	CVI_U8 u8ImgMode;

	u8ImgMode = g_pastPr2100[ViPipe]->u8ImgMode;

	pr2100_i2c_exit(ViPipe);

	if (u8ImgMode == PR2100_MODE_1080P_4CH) {
		pr2100_i2c_exit(slave_pipe);
	}
}

static void pr2100_set_1080p(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe=%d\n", ViPipe);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xc0, 0x21);
	pr2100_write_register(ViPipe, 0xc1, 0x21);
	pr2100_write_register(ViPipe, 0xc2, 0x21);
	pr2100_write_register(ViPipe, 0xc3, 0x21);
	pr2100_write_register(ViPipe, 0xc4, 0x21);
	pr2100_write_register(ViPipe, 0xc5, 0x21);
	pr2100_write_register(ViPipe, 0xc6, 0x21);
	pr2100_write_register(ViPipe, 0xc7, 0x21);
	pr2100_write_register(ViPipe, 0xc8, 0x21);
	pr2100_write_register(ViPipe, 0xc9, 0x01);
	pr2100_write_register(ViPipe, 0xca, 0x01);
	pr2100_write_register(ViPipe, 0xcb, 0x01);
	pr2100_write_register(ViPipe, 0xd0, 0x06);
	pr2100_write_register(ViPipe, 0xd1, 0x23);
	pr2100_write_register(ViPipe, 0xd2, 0x21);
	pr2100_write_register(ViPipe, 0xd3, 0x44);
	pr2100_write_register(ViPipe, 0xd4, 0x06);
	pr2100_write_register(ViPipe, 0xd5, 0x23);
	pr2100_write_register(ViPipe, 0xd6, 0x21);
	pr2100_write_register(ViPipe, 0xd7, 0x44);
	pr2100_write_register(ViPipe, 0xd8, 0x06);
	pr2100_write_register(ViPipe, 0xd9, 0x22);
	pr2100_write_register(ViPipe, 0xda, 0x2c);
	pr2100_write_register(ViPipe, 0x11, 0x0f);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0x13, 0x00);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0x15, 0x44);
	pr2100_write_register(ViPipe, 0x16, 0x0d);
	pr2100_write_register(ViPipe, 0x31, 0x0f);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x00);
	pr2100_write_register(ViPipe, 0x34, 0x21);
	pr2100_write_register(ViPipe, 0x35, 0x44);
	pr2100_write_register(ViPipe, 0x36, 0x0d);
	pr2100_write_register(ViPipe, 0xf3, 0x06);
	pr2100_write_register(ViPipe, 0xf4, 0x66);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xeb, 0x01);
	pr2100_write_register(ViPipe, 0xf0, 0x03);
	pr2100_write_register(ViPipe, 0xf1, 0xff);
	pr2100_write_register(ViPipe, 0xea, 0x00);
	// pr2100_write_register(ViPipe, 0xe3, 0x04);//Cb-Y-Cr-Y
	pr2100_write_register(ViPipe, 0xe3, 0xc4);//Y-Cb-Y-Cr
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe8, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe4, 0x20);
	pr2100_write_register(ViPipe, 0xe5, 0x64);
	pr2100_write_register(ViPipe, 0xe6, 0x20);
	pr2100_write_register(ViPipe, 0xe7, 0x64);
	pr2100_write_register(ViPipe, 0xe2, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x10);
	pr2100_write_register(ViPipe, 0x05, 0x04);
	pr2100_write_register(ViPipe, 0x06, 0x00);
	pr2100_write_register(ViPipe, 0x07, 0x00);
	pr2100_write_register(ViPipe, 0x08, 0xc9);
	pr2100_write_register(ViPipe, 0x36, 0x0f);
	pr2100_write_register(ViPipe, 0x37, 0x00);
	pr2100_write_register(ViPipe, 0x38, 0x0f);
	pr2100_write_register(ViPipe, 0x39, 0x00);
	pr2100_write_register(ViPipe, 0x3a, 0x0f);
	pr2100_write_register(ViPipe, 0x3b, 0x00);
	pr2100_write_register(ViPipe, 0x3c, 0x0f);
	pr2100_write_register(ViPipe, 0x3d, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x1e);
	pr2100_write_register(ViPipe, 0x47, 0x5e);
	pr2100_write_register(ViPipe, 0x48, 0x9e);
	pr2100_write_register(ViPipe, 0x49, 0xde);
	pr2100_write_register(ViPipe, 0x1c, 0x09);
	pr2100_write_register(ViPipe, 0x1d, 0x08);
	pr2100_write_register(ViPipe, 0x1e, 0x09);
	pr2100_write_register(ViPipe, 0x1f, 0x11);
	pr2100_write_register(ViPipe, 0x20, 0x0c);
	pr2100_write_register(ViPipe, 0x21, 0x28);
	pr2100_write_register(ViPipe, 0x22, 0x0b);
	pr2100_write_register(ViPipe, 0x23, 0x01);
	pr2100_write_register(ViPipe, 0x24, 0x12);
	pr2100_write_register(ViPipe, 0x25, 0x82);
	pr2100_write_register(ViPipe, 0x26, 0x11);
	pr2100_write_register(ViPipe, 0x27, 0x11);
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x05);//Page5
	pr2100_write_register(ViPipe, 0x09, 0x00);
	pr2100_write_register(ViPipe, 0x0a, 0x03);
	pr2100_write_register(ViPipe, 0x0e, 0x80);
	pr2100_write_register(ViPipe, 0x0f, 0x10);
	pr2100_write_register(ViPipe, 0x11, 0x80);
	pr2100_write_register(ViPipe, 0x12, 0x6e);
	pr2100_write_register(ViPipe, 0x13, 0x00);
	pr2100_write_register(ViPipe, 0x14, 0x6e);
	pr2100_write_register(ViPipe, 0x15, 0x00);
	pr2100_write_register(ViPipe, 0x16, 0x00);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x00);
	pr2100_write_register(ViPipe, 0x19, 0x00);
	pr2100_write_register(ViPipe, 0x1a, 0x00);
	pr2100_write_register(ViPipe, 0x1b, 0x00);
	pr2100_write_register(ViPipe, 0x1c, 0x00);
	pr2100_write_register(ViPipe, 0x1d, 0x00);
	pr2100_write_register(ViPipe, 0x1e, 0x00);
	pr2100_write_register(ViPipe, 0x20, 0x88);
	pr2100_write_register(ViPipe, 0x21, 0x07);
	pr2100_write_register(ViPipe, 0x22, 0x80);
	pr2100_write_register(ViPipe, 0x23, 0x04);
	pr2100_write_register(ViPipe, 0x24, 0x38);
	pr2100_write_register(ViPipe, 0x25, 0x0f);
	pr2100_write_register(ViPipe, 0x26, 0x00);
	pr2100_write_register(ViPipe, 0x27, 0x0f);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0x0b);
	pr2100_write_register(ViPipe, 0x2a, 0x40);
	pr2100_write_register(ViPipe, 0x30, 0x18);
	pr2100_write_register(ViPipe, 0x31, 0x07);
	pr2100_write_register(ViPipe, 0x32, 0x80);
	pr2100_write_register(ViPipe, 0x33, 0x04);
	pr2100_write_register(ViPipe, 0x34, 0x38);
	pr2100_write_register(ViPipe, 0x35, 0x0f);
	pr2100_write_register(ViPipe, 0x36, 0x00);
	pr2100_write_register(ViPipe, 0x37, 0x0f);
	pr2100_write_register(ViPipe, 0x38, 0x00);
	pr2100_write_register(ViPipe, 0x39, 0x07);
	pr2100_write_register(ViPipe, 0x3a, 0x80);
	pr2100_write_register(ViPipe, 0x40, 0x28);
	pr2100_write_register(ViPipe, 0x41, 0x07);
	pr2100_write_register(ViPipe, 0x42, 0x80);
	pr2100_write_register(ViPipe, 0x43, 0x04);
	pr2100_write_register(ViPipe, 0x44, 0x38);
	pr2100_write_register(ViPipe, 0x45, 0x0f);
	pr2100_write_register(ViPipe, 0x46, 0x00);
	pr2100_write_register(ViPipe, 0x47, 0x0f);
	pr2100_write_register(ViPipe, 0x48, 0x00);
	pr2100_write_register(ViPipe, 0x49, 0x03);
	pr2100_write_register(ViPipe, 0x4a, 0xc0);
	pr2100_write_register(ViPipe, 0x50, 0x38);
	pr2100_write_register(ViPipe, 0x51, 0x07);
	pr2100_write_register(ViPipe, 0x52, 0x80);
	pr2100_write_register(ViPipe, 0x53, 0x04);
	pr2100_write_register(ViPipe, 0x54, 0x38);
	pr2100_write_register(ViPipe, 0x55, 0x0f);
	pr2100_write_register(ViPipe, 0x56, 0x00);
	pr2100_write_register(ViPipe, 0x57, 0x0f);
	pr2100_write_register(ViPipe, 0x58, 0x00);
	pr2100_write_register(ViPipe, 0x59, 0x00);
	pr2100_write_register(ViPipe, 0x5a, 0x00);
	pr2100_write_register(ViPipe, 0x60, 0x05);
	pr2100_write_register(ViPipe, 0x61, 0x28);
	pr2100_write_register(ViPipe, 0x62, 0x05);
	pr2100_write_register(ViPipe, 0x63, 0x28);
	pr2100_write_register(ViPipe, 0x64, 0x05);
	pr2100_write_register(ViPipe, 0x65, 0x28);
	pr2100_write_register(ViPipe, 0x66, 0x05);
	pr2100_write_register(ViPipe, 0x67, 0x28);
	pr2100_write_register(ViPipe, 0x68, 0xff);
	pr2100_write_register(ViPipe, 0x69, 0xff);
	pr2100_write_register(ViPipe, 0x6a, 0xff);
	pr2100_write_register(ViPipe, 0x6b, 0xff);
	pr2100_write_register(ViPipe, 0x6c, 0xff);
	pr2100_write_register(ViPipe, 0x6d, 0xff);
	pr2100_write_register(ViPipe, 0x6e, 0xff);
	pr2100_write_register(ViPipe, 0x6f, 0xff);
	pr2100_write_register(ViPipe, 0x10, 0xb3);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x80, 0x80);
	pr2100_write_register(ViPipe, 0x81, 0x0e);
	pr2100_write_register(ViPipe, 0x82, 0x0d);
	pr2100_write_register(ViPipe, 0x84, 0xf0);
	pr2100_write_register(ViPipe, 0x8a, 0x00);
	pr2100_write_register(ViPipe, 0x90, 0x00);
	pr2100_write_register(ViPipe, 0x91, 0x00);
	pr2100_write_register(ViPipe, 0x94, 0xff);
	pr2100_write_register(ViPipe, 0x95, 0xff);
	pr2100_write_register(ViPipe, 0xa0, 0x33);
	pr2100_write_register(ViPipe, 0xb0, 0x33);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch0);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x02);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xce, 0x20);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xd2, 0x00);
	pr2100_write_register(ViPipe, 0xd3, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x10, 0x83);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0xe0, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
}

static void pr2100_set_1080p_2ch(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe=%d\n", ViPipe);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xc0, 0x21);
	pr2100_write_register(ViPipe, 0xc1, 0x21);
	pr2100_write_register(ViPipe, 0xc2, 0x21);
	pr2100_write_register(ViPipe, 0xc3, 0x21);
	pr2100_write_register(ViPipe, 0xc4, 0x21);
	pr2100_write_register(ViPipe, 0xc5, 0x21);
	pr2100_write_register(ViPipe, 0xc6, 0x21);
	pr2100_write_register(ViPipe, 0xc7, 0x21);
	pr2100_write_register(ViPipe, 0xc8, 0x21);
	pr2100_write_register(ViPipe, 0xc9, 0x01);
	pr2100_write_register(ViPipe, 0xca, 0x01);
	pr2100_write_register(ViPipe, 0xcb, 0x01);
	pr2100_write_register(ViPipe, 0xd0, 0x06);
	pr2100_write_register(ViPipe, 0xd1, 0x23);
	pr2100_write_register(ViPipe, 0xd2, 0x21);
	pr2100_write_register(ViPipe, 0xd3, 0x44);
	pr2100_write_register(ViPipe, 0xd4, 0x06);
	pr2100_write_register(ViPipe, 0xd5, 0x23);
	pr2100_write_register(ViPipe, 0xd6, 0x21);
	pr2100_write_register(ViPipe, 0xd7, 0x44);
	pr2100_write_register(ViPipe, 0xd8, 0x06);
	pr2100_write_register(ViPipe, 0xd9, 0x22);
	pr2100_write_register(ViPipe, 0xda, 0x2c);
	pr2100_write_register(ViPipe, 0x11, 0x0f);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0x13, 0x00);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0x15, 0x44);
	pr2100_write_register(ViPipe, 0x16, 0x0d);
	pr2100_write_register(ViPipe, 0x31, 0x0f);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x00);
	pr2100_write_register(ViPipe, 0x34, 0x21);
	pr2100_write_register(ViPipe, 0x35, 0x44);
	pr2100_write_register(ViPipe, 0x36, 0x0d);
	pr2100_write_register(ViPipe, 0xf3, 0x06);
	pr2100_write_register(ViPipe, 0xf4, 0x66);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xeb, 0x01);
	pr2100_write_register(ViPipe, 0xf0, 0x03);
	pr2100_write_register(ViPipe, 0xf1, 0xff);
	pr2100_write_register(ViPipe, 0xea, 0x00);
	// pr2100_write_register(ViPipe, 0xe3, 0x04);//Cb-Y-Cr-Y
	pr2100_write_register(ViPipe, 0xe3, 0xc4);//Y-Cb-Y-Cr
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe8, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe4, 0x20);
	pr2100_write_register(ViPipe, 0xe5, 0x64);
	pr2100_write_register(ViPipe, 0xe6, 0x20);
	pr2100_write_register(ViPipe, 0xe7, 0x64);
	pr2100_write_register(ViPipe, 0xe2, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x10);
	pr2100_write_register(ViPipe, 0x05, 0x04);
	pr2100_write_register(ViPipe, 0x06, 0x00);
	pr2100_write_register(ViPipe, 0x07, 0x00);
	pr2100_write_register(ViPipe, 0x08, 0xc9);
	pr2100_write_register(ViPipe, 0x36, 0x1e);
	pr2100_write_register(ViPipe, 0x37, 0x08);
	pr2100_write_register(ViPipe, 0x38, 0x0f);
	pr2100_write_register(ViPipe, 0x39, 0x00);
	pr2100_write_register(ViPipe, 0x3a, 0x0f);
	pr2100_write_register(ViPipe, 0x3b, 0x00);
	pr2100_write_register(ViPipe, 0x3c, 0x0f);
	pr2100_write_register(ViPipe, 0x3d, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x1e);
	pr2100_write_register(ViPipe, 0x47, 0x5e);
	pr2100_write_register(ViPipe, 0x48, 0x9e);
	pr2100_write_register(ViPipe, 0x49, 0xde);
	pr2100_write_register(ViPipe, 0x1c, 0x09);
	pr2100_write_register(ViPipe, 0x1d, 0x08);
	pr2100_write_register(ViPipe, 0x1e, 0x09);
	pr2100_write_register(ViPipe, 0x1f, 0x11);
	pr2100_write_register(ViPipe, 0x20, 0x0c);
	pr2100_write_register(ViPipe, 0x21, 0x28);
	pr2100_write_register(ViPipe, 0x22, 0x0b);
	pr2100_write_register(ViPipe, 0x23, 0x01);
	pr2100_write_register(ViPipe, 0x24, 0x12);
	pr2100_write_register(ViPipe, 0x25, 0x82);
	pr2100_write_register(ViPipe, 0x26, 0x11);
	pr2100_write_register(ViPipe, 0x27, 0x11);
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x05);//Page5
	pr2100_write_register(ViPipe, 0x09, 0x00);
	pr2100_write_register(ViPipe, 0x0a, 0x03);
	pr2100_write_register(ViPipe, 0x0e, 0x80);
	pr2100_write_register(ViPipe, 0x0f, 0x10);
	pr2100_write_register(ViPipe, 0x11, 0x90);
	pr2100_write_register(ViPipe, 0x12, 0x6e);
	pr2100_write_register(ViPipe, 0x13, 0x80);
	pr2100_write_register(ViPipe, 0x14, 0x6e);
	// pr2100_write_register(ViPipe, 0x15, 0x15);
	pr2100_write_register(ViPipe, 0x15, 0x00);
	pr2100_write_register(ViPipe, 0x16, 0x08);
	pr2100_write_register(ViPipe, 0x17, 0x7a);
	pr2100_write_register(ViPipe, 0x18, 0x09);
	pr2100_write_register(ViPipe, 0x19, 0x56);
	pr2100_write_register(ViPipe, 0x1a, 0x14);
	pr2100_write_register(ViPipe, 0x1b, 0xa0);
	pr2100_write_register(ViPipe, 0x1c, 0x04);
	// pr2100_write_register(ViPipe, 0x1d, 0x65);
	pr2100_write_register(ViPipe, 0x1d, 0x64);
	pr2100_write_register(ViPipe, 0x1e, 0xe5);
	pr2100_write_register(ViPipe, 0x20, 0x88);
	pr2100_write_register(ViPipe, 0x21, 0x07);
	pr2100_write_register(ViPipe, 0x22, 0x80);
	pr2100_write_register(ViPipe, 0x23, 0x04);
	pr2100_write_register(ViPipe, 0x24, 0x38);
	pr2100_write_register(ViPipe, 0x25, 0x0f);
	pr2100_write_register(ViPipe, 0x26, 0x00);
	pr2100_write_register(ViPipe, 0x27, 0x0f);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0x0b);
	pr2100_write_register(ViPipe, 0x2a, 0x40);
	pr2100_write_register(ViPipe, 0x30, 0x98);
	pr2100_write_register(ViPipe, 0x31, 0x07);
	pr2100_write_register(ViPipe, 0x32, 0x80);
	pr2100_write_register(ViPipe, 0x33, 0x04);
	pr2100_write_register(ViPipe, 0x34, 0x38);
	pr2100_write_register(ViPipe, 0x35, 0x0f);
	pr2100_write_register(ViPipe, 0x36, 0x00);
	pr2100_write_register(ViPipe, 0x37, 0x0f);
	pr2100_write_register(ViPipe, 0x38, 0x00);
	pr2100_write_register(ViPipe, 0x39, 0x07);
	pr2100_write_register(ViPipe, 0x3a, 0x80);
	pr2100_write_register(ViPipe, 0x40, 0x28);
	pr2100_write_register(ViPipe, 0x41, 0x07);
	pr2100_write_register(ViPipe, 0x42, 0x80);
	pr2100_write_register(ViPipe, 0x43, 0x04);
	pr2100_write_register(ViPipe, 0x44, 0x38);
	pr2100_write_register(ViPipe, 0x45, 0x0f);
	pr2100_write_register(ViPipe, 0x46, 0x00);
	pr2100_write_register(ViPipe, 0x47, 0x0f);
	pr2100_write_register(ViPipe, 0x48, 0x00);
	pr2100_write_register(ViPipe, 0x49, 0x03);
	pr2100_write_register(ViPipe, 0x4a, 0xc0);
	pr2100_write_register(ViPipe, 0x50, 0x38);
	pr2100_write_register(ViPipe, 0x51, 0x07);
	pr2100_write_register(ViPipe, 0x52, 0x80);
	pr2100_write_register(ViPipe, 0x53, 0x04);
	pr2100_write_register(ViPipe, 0x54, 0x38);
	pr2100_write_register(ViPipe, 0x55, 0x0f);
	pr2100_write_register(ViPipe, 0x56, 0x00);
	pr2100_write_register(ViPipe, 0x57, 0x0f);
	pr2100_write_register(ViPipe, 0x58, 0x00);
	pr2100_write_register(ViPipe, 0x59, 0x00);
	pr2100_write_register(ViPipe, 0x5a, 0x00);
	pr2100_write_register(ViPipe, 0x60, 0x05);
	pr2100_write_register(ViPipe, 0x61, 0x28);
	pr2100_write_register(ViPipe, 0x62, 0x05);
	pr2100_write_register(ViPipe, 0x63, 0x28);
	pr2100_write_register(ViPipe, 0x64, 0x05);
	pr2100_write_register(ViPipe, 0x65, 0x28);
	pr2100_write_register(ViPipe, 0x66, 0x05);
	pr2100_write_register(ViPipe, 0x67, 0x28);
	pr2100_write_register(ViPipe, 0x68, 0xff);
	pr2100_write_register(ViPipe, 0x69, 0xff);
	pr2100_write_register(ViPipe, 0x6a, 0xff);
	pr2100_write_register(ViPipe, 0x6b, 0xff);
	pr2100_write_register(ViPipe, 0x6c, 0xff);
	pr2100_write_register(ViPipe, 0x6d, 0xff);
	pr2100_write_register(ViPipe, 0x6e, 0xff);
	pr2100_write_register(ViPipe, 0x6f, 0xff);
	pr2100_write_register(ViPipe, 0x10, 0xf3);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x80, 0x80);
	pr2100_write_register(ViPipe, 0x81, 0x0e);
	pr2100_write_register(ViPipe, 0x82, 0x0d);
	pr2100_write_register(ViPipe, 0x84, 0xf0);
	pr2100_write_register(ViPipe, 0x8a, 0x00);
	pr2100_write_register(ViPipe, 0x90, 0x00);
	pr2100_write_register(ViPipe, 0x91, 0x00);
	pr2100_write_register(ViPipe, 0x94, 0xff);
	pr2100_write_register(ViPipe, 0x95, 0xff);
	pr2100_write_register(ViPipe, 0xa0, 0x33);
	pr2100_write_register(ViPipe, 0xb0, 0x33);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch0);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x02);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xce, 0x20);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xd2, 0x00);
	pr2100_write_register(ViPipe, 0xd3, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x10, 0x83);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0xe0, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch1);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x02);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xce, 0x20);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xd2, 0x00);
	pr2100_write_register(ViPipe, 0xd3, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x30, 0x83);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0xe1, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
}

static void pr2100_set_1080p_4ch_slave(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe=%d\n", ViPipe);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch2);
	pr2100_write_register(ViPipe, 0xc5, 0x00);
	pr2100_write_register(ViPipe, 0xc6, 0x00);
	pr2100_write_register(ViPipe, 0xc7, 0x00);
	pr2100_write_register(ViPipe, 0xc8, 0x00);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x04);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xcd, 0x40);
	pr2100_write_register(ViPipe, 0xce, 0x00);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x10, 0x83);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0xe0, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch3);
	pr2100_write_register(ViPipe, 0xc5, 0x00);
	pr2100_write_register(ViPipe, 0xc6, 0x00);
	pr2100_write_register(ViPipe, 0xc7, 0x00);
	pr2100_write_register(ViPipe, 0xc8, 0x00);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x04);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xcd, 0x40);
	pr2100_write_register(ViPipe, 0xce, 0x00);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x30, 0x83);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0xe1, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xc0, 0x20);
	pr2100_write_register(ViPipe, 0xc1, 0x20);
	pr2100_write_register(ViPipe, 0xc2, 0x20);
	pr2100_write_register(ViPipe, 0xc3, 0x20);
	pr2100_write_register(ViPipe, 0xc4, 0x20);
	pr2100_write_register(ViPipe, 0xc5, 0x20);
	pr2100_write_register(ViPipe, 0xc6, 0x20);
	pr2100_write_register(ViPipe, 0xc7, 0x20);
	pr2100_write_register(ViPipe, 0xc8, 0x20);
	pr2100_write_register(ViPipe, 0xc9, 0x04);
	pr2100_write_register(ViPipe, 0xca, 0x04);
	pr2100_write_register(ViPipe, 0xcb, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x06);
	pr2100_write_register(ViPipe, 0xd1, 0x23);
	pr2100_write_register(ViPipe, 0xd2, 0x21);
	pr2100_write_register(ViPipe, 0xd3, 0x44);
	pr2100_write_register(ViPipe, 0xd4, 0x06);
	pr2100_write_register(ViPipe, 0xd5, 0x23);
	pr2100_write_register(ViPipe, 0xd6, 0x21);
	pr2100_write_register(ViPipe, 0xd7, 0x44);
	pr2100_write_register(ViPipe, 0xd8, 0x86);
	pr2100_write_register(ViPipe, 0xd9, 0x23);
	pr2100_write_register(ViPipe, 0xda, 0x21);
	pr2100_write_register(ViPipe, 0x11, 0x0f);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0x13, 0x00);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0x15, 0x44);
	pr2100_write_register(ViPipe, 0x16, 0x0d);
	pr2100_write_register(ViPipe, 0x31, 0x0f);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x00);
	pr2100_write_register(ViPipe, 0x34, 0x21);
	pr2100_write_register(ViPipe, 0x35, 0x44);
	pr2100_write_register(ViPipe, 0x36, 0x0d);
	pr2100_write_register(ViPipe, 0xf3, 0x06);
	pr2100_write_register(ViPipe, 0xf4, 0x66);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe2, 0x14);
	pr2100_write_register(ViPipe, 0xe8, 0x08);
	pr2100_write_register(ViPipe, 0xd7, 0x41);
	pr2100_write_register(ViPipe, 0xe9, 0x08);
	pr2100_write_register(ViPipe, 0xd7, 0x61);
	pr2100_write_register(ViPipe, 0x80, 0x80);
	pr2100_write_register(ViPipe, 0x81, 0x0e);
	pr2100_write_register(ViPipe, 0x82, 0x0d);
	pr2100_write_register(ViPipe, 0x84, 0xf0);
	pr2100_write_register(ViPipe, 0x8a, 0x00);
	pr2100_write_register(ViPipe, 0x90, 0x00);
	pr2100_write_register(ViPipe, 0x91, 0x00);
	pr2100_write_register(ViPipe, 0x94, 0xff);
	pr2100_write_register(ViPipe, 0x95, 0xff);
	pr2100_write_register(ViPipe, 0xa0, 0x33);
	pr2100_write_register(ViPipe, 0xb0, 0x33);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x30);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xeb, 0x00);
	pr2100_write_register(ViPipe, 0xf0, 0x02);
	pr2100_write_register(ViPipe, 0xf1, 0x00);
	pr2100_write_register(ViPipe, 0xea, 0x10);
	// pr2100_write_register(ViPipe, 0xe3, 0x04);
	pr2100_write_register(ViPipe, 0xe3, 0xc4);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe8, 0x08);
	pr2100_write_register(ViPipe, 0xe9, 0x08);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe9, 0x08);
	pr2100_write_register(ViPipe, 0xe9, 0x08);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe4, 0x20);
	pr2100_write_register(ViPipe, 0xe5, 0x64);
	pr2100_write_register(ViPipe, 0xe6, 0x02);
	pr2100_write_register(ViPipe, 0xe7, 0x64);
	pr2100_write_register(ViPipe, 0xf3, 0x04);
	pr2100_write_register(ViPipe, 0xf4, 0x44);
}

static void pr2100_set_1080p_4ch(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "ViPipe=%d\n", ViPipe);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch0);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x02);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xcd, 0x40);
	pr2100_write_register(ViPipe, 0xce, 0x00);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xd2, 0x00);
	pr2100_write_register(ViPipe, 0xd3, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x10, 0x83);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0xe0, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x80, 0x00);
	pr2100_write_register(ViPipe, 0x81, 0x09);
	pr2100_write_register(ViPipe, 0x82, 0x00);
	pr2100_write_register(ViPipe, 0x83, 0x07);
	pr2100_write_register(ViPipe, 0x84, 0x00);
	pr2100_write_register(ViPipe, 0x85, 0x17);
	pr2100_write_register(ViPipe, 0x86, 0x03);
	pr2100_write_register(ViPipe, 0x87, 0xe5);
	pr2100_write_register(ViPipe, 0x88, 0x05);
	pr2100_write_register(ViPipe, 0x89, 0x24);
	pr2100_write_register(ViPipe, 0x8a, 0x05);
	pr2100_write_register(ViPipe, 0x8b, 0x24);
	pr2100_write_register(ViPipe, 0x8c, 0x08);
	pr2100_write_register(ViPipe, 0x8d, 0xe8);
	pr2100_write_register(ViPipe, 0x8e, 0x05);
	pr2100_write_register(ViPipe, 0x8f, 0x47);
	pr2100_write_register(ViPipe, 0x90, 0x02);
	pr2100_write_register(ViPipe, 0x91, 0xb4);
	pr2100_write_register(ViPipe, 0x92, 0x73);
	pr2100_write_register(ViPipe, 0x93, 0xe8);
	pr2100_write_register(ViPipe, 0x94, 0x0f);
	pr2100_write_register(ViPipe, 0x95, 0x5e);
	pr2100_write_register(ViPipe, 0x96, 0x03);
	pr2100_write_register(ViPipe, 0x97, 0xd0);
	pr2100_write_register(ViPipe, 0x98, 0x17);
	pr2100_write_register(ViPipe, 0x99, 0x34);
	pr2100_write_register(ViPipe, 0x9a, 0x13);
	pr2100_write_register(ViPipe, 0x9b, 0x56);
	pr2100_write_register(ViPipe, 0x9c, 0x0b);
	pr2100_write_register(ViPipe, 0x9d, 0x9a);
	pr2100_write_register(ViPipe, 0x9e, 0x09);
	pr2100_write_register(ViPipe, 0x9f, 0xab);
	pr2100_write_register(ViPipe, 0xa0, 0x01);
	pr2100_write_register(ViPipe, 0xa1, 0x74);
	pr2100_write_register(ViPipe, 0xa2, 0x01);
	pr2100_write_register(ViPipe, 0xa3, 0x6b);
	pr2100_write_register(ViPipe, 0xa4, 0x00);
	pr2100_write_register(ViPipe, 0xa5, 0xba);
	pr2100_write_register(ViPipe, 0xa6, 0x00);
	pr2100_write_register(ViPipe, 0xa7, 0xa3);
	pr2100_write_register(ViPipe, 0xa8, 0x01);
	pr2100_write_register(ViPipe, 0xa9, 0x39);
	pr2100_write_register(ViPipe, 0xaa, 0x01);
	pr2100_write_register(ViPipe, 0xab, 0x39);
	pr2100_write_register(ViPipe, 0xac, 0x00);
	pr2100_write_register(ViPipe, 0xad, 0xc1);
	pr2100_write_register(ViPipe, 0xae, 0x00);
	pr2100_write_register(ViPipe, 0xaf, 0xc1);
	pr2100_write_register(ViPipe, 0xb0, 0x05);
	pr2100_write_register(ViPipe, 0xb1, 0xcc);
	pr2100_write_register(ViPipe, 0xb2, 0x09);
	pr2100_write_register(ViPipe, 0xb3, 0x6d);
	pr2100_write_register(ViPipe, 0xb4, 0x00);
	pr2100_write_register(ViPipe, 0xb5, 0x17);
	pr2100_write_register(ViPipe, 0xb6, 0x08);
	pr2100_write_register(ViPipe, 0xb7, 0xe8);
	pr2100_write_register(ViPipe, 0xb8, 0xb0);
	pr2100_write_register(ViPipe, 0xb9, 0xce);
	pr2100_write_register(ViPipe, 0xba, 0x90);
	pr2100_write_register(ViPipe, 0xbb, 0x00);
	pr2100_write_register(ViPipe, 0xbc, 0x00);
	pr2100_write_register(ViPipe, 0xbd, 0x04);
	pr2100_write_register(ViPipe, 0xbe, 0x07);
	pr2100_write_register(ViPipe, 0xbf, 0x80);
	pr2100_write_register(ViPipe, 0xc0, 0x00);
	pr2100_write_register(ViPipe, 0xc1, 0x00);
	pr2100_write_register(ViPipe, 0xc2, 0x44);
	pr2100_write_register(ViPipe, 0xc3, 0x38);
	pr2100_write_register(ViPipe, 0xc4, output_pattern_ch1);
	pr2100_write_register(ViPipe, 0xc9, 0x00);
	pr2100_write_register(ViPipe, 0xca, 0x02);
	pr2100_write_register(ViPipe, 0xcb, 0x07);
	pr2100_write_register(ViPipe, 0xcc, 0x80);
	pr2100_write_register(ViPipe, 0xce, 0x20);
	pr2100_write_register(ViPipe, 0xcd, 0x40);
	pr2100_write_register(ViPipe, 0xce, 0x00);
	pr2100_write_register(ViPipe, 0xcf, 0x04);
	pr2100_write_register(ViPipe, 0xd0, 0x38);
	pr2100_write_register(ViPipe, 0xd1, 0x00);
	pr2100_write_register(ViPipe, 0xd2, 0x00);
	pr2100_write_register(ViPipe, 0xd3, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0x30, 0x83);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0xe1, 0x05);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x10, 0x26);
	pr2100_write_register(ViPipe, 0x11, 0x00);
	pr2100_write_register(ViPipe, 0x12, 0x87);
	pr2100_write_register(ViPipe, 0x13, 0x24);
	pr2100_write_register(ViPipe, 0x14, 0x80);
	pr2100_write_register(ViPipe, 0x15, 0x2a);
	pr2100_write_register(ViPipe, 0x16, 0x38);
	pr2100_write_register(ViPipe, 0x17, 0x00);
	pr2100_write_register(ViPipe, 0x18, 0x80);
	pr2100_write_register(ViPipe, 0x19, 0x48);
	pr2100_write_register(ViPipe, 0x1a, 0x6c);
	pr2100_write_register(ViPipe, 0x1b, 0x05);
	pr2100_write_register(ViPipe, 0x1c, 0x61);
	pr2100_write_register(ViPipe, 0x1d, 0x07);
	pr2100_write_register(ViPipe, 0x1e, 0x7e);
	pr2100_write_register(ViPipe, 0x1f, 0x80);
	pr2100_write_register(ViPipe, 0x20, 0x80);
	pr2100_write_register(ViPipe, 0x21, 0x80);
	pr2100_write_register(ViPipe, 0x22, 0x90);
	pr2100_write_register(ViPipe, 0x23, 0x80);
	pr2100_write_register(ViPipe, 0x24, 0x80);
	pr2100_write_register(ViPipe, 0x25, 0x80);
	pr2100_write_register(ViPipe, 0x26, 0x84);
	pr2100_write_register(ViPipe, 0x27, 0x82);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0xff);
	pr2100_write_register(ViPipe, 0x2a, 0xff);
	pr2100_write_register(ViPipe, 0x2b, 0x00);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x14);
	pr2100_write_register(ViPipe, 0x34, 0x14);
	pr2100_write_register(ViPipe, 0x35, 0x80);
	pr2100_write_register(ViPipe, 0x36, 0x80);
	pr2100_write_register(ViPipe, 0x37, 0xad);
	pr2100_write_register(ViPipe, 0x38, 0x4b);
	pr2100_write_register(ViPipe, 0x39, 0x08);
	pr2100_write_register(ViPipe, 0x3a, 0x21);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3d, 0x23);
	pr2100_write_register(ViPipe, 0x3e, 0x05);
	pr2100_write_register(ViPipe, 0x3f, 0xc8);
	pr2100_write_register(ViPipe, 0x40, 0x05);
	pr2100_write_register(ViPipe, 0x41, 0x55);
	pr2100_write_register(ViPipe, 0x42, 0x01);
	pr2100_write_register(ViPipe, 0x43, 0x38);
	pr2100_write_register(ViPipe, 0x44, 0x6a);
	pr2100_write_register(ViPipe, 0x45, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x14);
	pr2100_write_register(ViPipe, 0x47, 0xb0);
	pr2100_write_register(ViPipe, 0x48, 0xdf);
	pr2100_write_register(ViPipe, 0x49, 0x00);
	pr2100_write_register(ViPipe, 0x4a, 0x7b);
	pr2100_write_register(ViPipe, 0x4b, 0x60);
	pr2100_write_register(ViPipe, 0x4c, 0x00);
	pr2100_write_register(ViPipe, 0x4d, 0x26);
	pr2100_write_register(ViPipe, 0x4e, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x54, 0x0e);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xc0, 0x21);
	pr2100_write_register(ViPipe, 0xc1, 0x21);
	pr2100_write_register(ViPipe, 0xc2, 0x21);
	pr2100_write_register(ViPipe, 0xc3, 0x21);
	pr2100_write_register(ViPipe, 0xc4, 0x21);
	pr2100_write_register(ViPipe, 0xc5, 0x21);
	pr2100_write_register(ViPipe, 0xc6, 0x21);
	pr2100_write_register(ViPipe, 0xc7, 0x21);
	pr2100_write_register(ViPipe, 0xc8, 0x21);
	pr2100_write_register(ViPipe, 0xc9, 0x01);
	pr2100_write_register(ViPipe, 0xca, 0x01);
	pr2100_write_register(ViPipe, 0xcb, 0x01);
	pr2100_write_register(ViPipe, 0xd0, 0x06);
	pr2100_write_register(ViPipe, 0xd1, 0x23);
	pr2100_write_register(ViPipe, 0xd2, 0x21);
	pr2100_write_register(ViPipe, 0xd3, 0x44);
	pr2100_write_register(ViPipe, 0xd4, 0x06);
	pr2100_write_register(ViPipe, 0xd5, 0x23);
	pr2100_write_register(ViPipe, 0xd6, 0x21);
	pr2100_write_register(ViPipe, 0xd7, 0x44);
	pr2100_write_register(ViPipe, 0xd8, 0x06);
	pr2100_write_register(ViPipe, 0xd9, 0x22);
	pr2100_write_register(ViPipe, 0xda, 0x2c);
	pr2100_write_register(ViPipe, 0x11, 0x0f);
	pr2100_write_register(ViPipe, 0x12, 0x00);
	pr2100_write_register(ViPipe, 0x13, 0x00);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0x15, 0x44);
	pr2100_write_register(ViPipe, 0x16, 0x0d);
	pr2100_write_register(ViPipe, 0x31, 0x0f);
	pr2100_write_register(ViPipe, 0x32, 0x00);
	pr2100_write_register(ViPipe, 0x33, 0x00);
	pr2100_write_register(ViPipe, 0x34, 0x21);
	pr2100_write_register(ViPipe, 0x35, 0x44);
	pr2100_write_register(ViPipe, 0x36, 0x0d);
	pr2100_write_register(ViPipe, 0xf3, 0x06);
	pr2100_write_register(ViPipe, 0xf4, 0x66);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x00, 0xe4);
	pr2100_write_register(ViPipe, 0x01, 0x61);
	pr2100_write_register(ViPipe, 0x02, 0x00);
	pr2100_write_register(ViPipe, 0x03, 0x56);
	pr2100_write_register(ViPipe, 0x04, 0x0c);
	pr2100_write_register(ViPipe, 0x05, 0x88);
	pr2100_write_register(ViPipe, 0x06, 0x04);
	pr2100_write_register(ViPipe, 0x07, 0xb2);
	pr2100_write_register(ViPipe, 0x08, 0x44);
	pr2100_write_register(ViPipe, 0x09, 0x34);
	pr2100_write_register(ViPipe, 0x0a, 0x02);
	pr2100_write_register(ViPipe, 0x0b, 0x14);
	pr2100_write_register(ViPipe, 0x0c, 0x04);
	pr2100_write_register(ViPipe, 0x0d, 0x08);
	pr2100_write_register(ViPipe, 0x0e, 0x5e);
	pr2100_write_register(ViPipe, 0x0f, 0x5e);
	pr2100_write_register(ViPipe, 0x2c, 0x00);
	pr2100_write_register(ViPipe, 0x2d, 0x00);
	pr2100_write_register(ViPipe, 0x2e, 0x00);
	pr2100_write_register(ViPipe, 0x2f, 0x00);
	pr2100_write_register(ViPipe, 0x30, 0x00);
	pr2100_write_register(ViPipe, 0x31, 0x00);
	pr2100_write_register(ViPipe, 0x32, 0xc0);
	pr2100_write_register(ViPipe, 0x3b, 0x02);
	pr2100_write_register(ViPipe, 0x3c, 0x01);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x51, 0x28);
	pr2100_write_register(ViPipe, 0x52, 0x40);
	pr2100_write_register(ViPipe, 0x53, 0x0c);
	pr2100_write_register(ViPipe, 0x54, 0x0f);
	pr2100_write_register(ViPipe, 0x55, 0x8d);
	pr2100_write_register(ViPipe, 0x70, 0x06);
	pr2100_write_register(ViPipe, 0x71, 0x08);
	pr2100_write_register(ViPipe, 0x72, 0x0a);
	pr2100_write_register(ViPipe, 0x73, 0x0c);
	pr2100_write_register(ViPipe, 0x74, 0x0e);
	pr2100_write_register(ViPipe, 0x75, 0x10);
	pr2100_write_register(ViPipe, 0x76, 0x12);
	pr2100_write_register(ViPipe, 0x77, 0x14);
	pr2100_write_register(ViPipe, 0x78, 0x06);
	pr2100_write_register(ViPipe, 0x79, 0x08);
	pr2100_write_register(ViPipe, 0x7a, 0x0a);
	pr2100_write_register(ViPipe, 0x7b, 0x0c);
	pr2100_write_register(ViPipe, 0x7c, 0x0e);
	pr2100_write_register(ViPipe, 0x7d, 0x10);
	pr2100_write_register(ViPipe, 0x7e, 0x12);
	pr2100_write_register(ViPipe, 0x7f, 0x14);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe2, 0x00);
	pr2100_write_register(ViPipe, 0x80, 0x80);
	pr2100_write_register(ViPipe, 0x81, 0x0e);
	pr2100_write_register(ViPipe, 0x82, 0x0d);
	pr2100_write_register(ViPipe, 0x84, 0xf0);
	pr2100_write_register(ViPipe, 0x8a, 0x00);
	pr2100_write_register(ViPipe, 0x90, 0x00);
	pr2100_write_register(ViPipe, 0x91, 0x00);
	pr2100_write_register(ViPipe, 0x94, 0xff);
	pr2100_write_register(ViPipe, 0x95, 0xff);
	pr2100_write_register(ViPipe, 0xa0, 0x33);
	pr2100_write_register(ViPipe, 0xb0, 0x33);
	pr2100_write_register(ViPipe, 0x14, 0x21);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xeb, 0x01);
	pr2100_write_register(ViPipe, 0xf0, 0x03);
	pr2100_write_register(ViPipe, 0xf1, 0xff);
	pr2100_write_register(ViPipe, 0xea, 0x00);
	// pr2100_write_register(ViPipe, 0xe3, 0x04);
	pr2100_write_register(ViPipe, 0xe3, 0xc4);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x00);
	pr2100_write_register(ViPipe, 0x50, 0x21);
	pr2100_write_register(ViPipe, 0x4f, 0x20);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe8, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x01);//Page1
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xd1, 0x10);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xe9, 0x00);
	pr2100_write_register(ViPipe, 0xff, 0x02);//Page2
	pr2100_write_register(ViPipe, 0xcd, 0x08);
	pr2100_write_register(ViPipe, 0x4f, 0x2c);
	pr2100_write_register(ViPipe, 0xff, 0x00);//Page0
	pr2100_write_register(ViPipe, 0xe4, 0x20);
	pr2100_write_register(ViPipe, 0xe5, 0x64);
	pr2100_write_register(ViPipe, 0xe6, 0x20);
	pr2100_write_register(ViPipe, 0xe7, 0x64);
	pr2100_write_register(ViPipe, 0xff, 0x06);//Page6
	pr2100_write_register(ViPipe, 0x04, 0x10);
	pr2100_write_register(ViPipe, 0x05, 0x04);
	pr2100_write_register(ViPipe, 0x06, 0x00);
	pr2100_write_register(ViPipe, 0x07, 0x00);
	pr2100_write_register(ViPipe, 0x08, 0xc9);
	pr2100_write_register(ViPipe, 0x36, 0x3c);
	pr2100_write_register(ViPipe, 0x37, 0x10);
	pr2100_write_register(ViPipe, 0x38, 0x0f);
	pr2100_write_register(ViPipe, 0x39, 0x00);
	pr2100_write_register(ViPipe, 0x3a, 0x0f);
	pr2100_write_register(ViPipe, 0x3b, 0x00);
	pr2100_write_register(ViPipe, 0x3c, 0x0f);
	pr2100_write_register(ViPipe, 0x3d, 0x00);
	pr2100_write_register(ViPipe, 0x46, 0x1e);
	pr2100_write_register(ViPipe, 0x47, 0x5e);
	pr2100_write_register(ViPipe, 0x48, 0x9e);
	pr2100_write_register(ViPipe, 0x49, 0xde);
	pr2100_write_register(ViPipe, 0x1c, 0x09);
	pr2100_write_register(ViPipe, 0x1d, 0x08);
	pr2100_write_register(ViPipe, 0x1e, 0x09);
	pr2100_write_register(ViPipe, 0x1f, 0x11);
	pr2100_write_register(ViPipe, 0x20, 0x0c);
	pr2100_write_register(ViPipe, 0x21, 0x28);
	pr2100_write_register(ViPipe, 0x22, 0x0b);
	pr2100_write_register(ViPipe, 0x23, 0x01);
	pr2100_write_register(ViPipe, 0x24, 0x12);
	pr2100_write_register(ViPipe, 0x25, 0x82);
	pr2100_write_register(ViPipe, 0x26, 0x11);
	pr2100_write_register(ViPipe, 0x27, 0x11);
	pr2100_write_register(ViPipe, 0x04, 0x50);
	pr2100_write_register(ViPipe, 0xff, 0x05);//Page5
	pr2100_write_register(ViPipe, 0x09, 0x00);
	pr2100_write_register(ViPipe, 0x0a, 0x03);
	pr2100_write_register(ViPipe, 0x0e, 0x80);
	pr2100_write_register(ViPipe, 0x0f, 0x10);
	pr2100_write_register(ViPipe, 0x11, 0xb0);
	pr2100_write_register(ViPipe, 0x12, 0x6e);
	pr2100_write_register(ViPipe, 0x13, 0x80);
	pr2100_write_register(ViPipe, 0x14, 0x6e);
	// pr2100_write_register(ViPipe, 0x15, 0x15);
	pr2100_write_register(ViPipe, 0x15, 0x00);
	pr2100_write_register(ViPipe, 0x16, 0x08);
	pr2100_write_register(ViPipe, 0x17, 0x7a);
	pr2100_write_register(ViPipe, 0x18, 0x09);
	pr2100_write_register(ViPipe, 0x19, 0x56);
	pr2100_write_register(ViPipe, 0x1a, 0x14);
	pr2100_write_register(ViPipe, 0x1b, 0xa0);
	pr2100_write_register(ViPipe, 0x1c, 0x04);
	// pr2100_write_register(ViPipe, 0x1d, 0x65);
	pr2100_write_register(ViPipe, 0x1d, 0x64);
	pr2100_write_register(ViPipe, 0x1e, 0xed);
	pr2100_write_register(ViPipe, 0x20, 0x88);
	pr2100_write_register(ViPipe, 0x21, 0x07);
	pr2100_write_register(ViPipe, 0x22, 0x80);
	pr2100_write_register(ViPipe, 0x23, 0x04);
	pr2100_write_register(ViPipe, 0x24, 0x38);
	pr2100_write_register(ViPipe, 0x25, 0x0f);
	pr2100_write_register(ViPipe, 0x26, 0x00);
	pr2100_write_register(ViPipe, 0x27, 0x0f);
	pr2100_write_register(ViPipe, 0x28, 0x00);
	pr2100_write_register(ViPipe, 0x29, 0x0b);
	pr2100_write_register(ViPipe, 0x2a, 0x40);
	pr2100_write_register(ViPipe, 0x30, 0x98);
	pr2100_write_register(ViPipe, 0x31, 0x07);
	pr2100_write_register(ViPipe, 0x32, 0x80);
	pr2100_write_register(ViPipe, 0x33, 0x04);
	pr2100_write_register(ViPipe, 0x34, 0x38);
	pr2100_write_register(ViPipe, 0x35, 0x0f);
	pr2100_write_register(ViPipe, 0x36, 0x00);
	pr2100_write_register(ViPipe, 0x37, 0x0f);
	pr2100_write_register(ViPipe, 0x38, 0x00);
	pr2100_write_register(ViPipe, 0x39, 0x07);
	pr2100_write_register(ViPipe, 0x3a, 0x80);
	pr2100_write_register(ViPipe, 0x40, 0xa8);
	pr2100_write_register(ViPipe, 0x41, 0x07);
	pr2100_write_register(ViPipe, 0x42, 0x80);
	pr2100_write_register(ViPipe, 0x43, 0x04);
	pr2100_write_register(ViPipe, 0x44, 0x38);
	pr2100_write_register(ViPipe, 0x45, 0x0f);
	pr2100_write_register(ViPipe, 0x46, 0x00);
	pr2100_write_register(ViPipe, 0x47, 0x0f);
	pr2100_write_register(ViPipe, 0x48, 0x00);
	pr2100_write_register(ViPipe, 0x49, 0x03);
	pr2100_write_register(ViPipe, 0x4a, 0xc0);
	pr2100_write_register(ViPipe, 0x50, 0xb8);
	pr2100_write_register(ViPipe, 0x51, 0x07);
	pr2100_write_register(ViPipe, 0x52, 0x80);
	pr2100_write_register(ViPipe, 0x53, 0x04);
	pr2100_write_register(ViPipe, 0x54, 0x38);
	pr2100_write_register(ViPipe, 0x55, 0x0f);
	pr2100_write_register(ViPipe, 0x56, 0x00);
	pr2100_write_register(ViPipe, 0x57, 0x0f);
	pr2100_write_register(ViPipe, 0x58, 0x00);
	pr2100_write_register(ViPipe, 0x59, 0x00);
	pr2100_write_register(ViPipe, 0x5a, 0x00);
	pr2100_write_register(ViPipe, 0x60, 0x05);
	pr2100_write_register(ViPipe, 0x61, 0x28);
	pr2100_write_register(ViPipe, 0x62, 0x05);
	pr2100_write_register(ViPipe, 0x63, 0x28);
	pr2100_write_register(ViPipe, 0x64, 0x05);
	pr2100_write_register(ViPipe, 0x65, 0x28);
	pr2100_write_register(ViPipe, 0x66, 0x05);
	pr2100_write_register(ViPipe, 0x67, 0x28);
	pr2100_write_register(ViPipe, 0x68, 0xff);
	pr2100_write_register(ViPipe, 0x69, 0xff);
	pr2100_write_register(ViPipe, 0x6a, 0xff);
	pr2100_write_register(ViPipe, 0x6b, 0xff);
	pr2100_write_register(ViPipe, 0x6c, 0xff);
	pr2100_write_register(ViPipe, 0x6d, 0xff);
	pr2100_write_register(ViPipe, 0x6e, 0xff);
	pr2100_write_register(ViPipe, 0x6f, 0xff);
	pr2100_write_register(ViPipe, 0x10, 0xf3);

	pr2100_set_1080p_4ch_slave(slave_pipe);
}
