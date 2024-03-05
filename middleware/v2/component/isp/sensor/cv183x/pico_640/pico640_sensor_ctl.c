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
#include "cvi_sns_ctrl.h"
#include "pico640_cmos_ex.h"

static void pico640_linear_480p30_init(VI_PIPE ViPipe);

const CVI_U8 pico640_i2c_addr = 0x13;        /* I2C Address of PICO640 */
const CVI_U32 pico640_addr_byte = 2;
const CVI_U32 pico640_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

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

static int PICO640_GPIO_Export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

static int PICO640_GPIO_SetDirection(unsigned int gpio, unsigned int out_flag)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR"/gpio%d/direction", gpio);
	if (access(buf, 0) == -1)
		PICO640_GPIO_Export(gpio);

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

static int PICO640_GPIO_SetValue(unsigned int gpio, unsigned int value)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR"/gpio%d/value", gpio);
	if (access(buf, 0) == -1)
		PICO640_GPIO_Export(gpio);

	PICO640_GPIO_SetDirection(gpio, 1); //output

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

int pico640_2256_init(void)
{
	char spi_dev[] = "/dev/spidev32766.0";
	int fd;
	int status, speed = 500000;
	CVI_U32 mode = 0;
	uint8_t bits = 16;
	uint16_t delay = 0;
	size_t len = 0;

	uint8_t tx[4];
	uint8_t rx[4];

	struct spi_ioc_transfer tr;

	// DAC
	speed = 500000;
	mode = 1;
	bits = 16;
	delay = 0;
	len = 4;

	fd = open(spi_dev, O_RDWR);
	if (fd < 0) {
		printf("open %s error\n", spi_dev);
		return -1;
	}

	/*
	 * spi mode
	 */
	status = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (status < 0) {
		printf("SPI IOC WR ERROR\n");
	}
	/*
	 * max speed hz
	 */
	status = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (status < 0) {
		printf("SPI SPEED ERROR\n");
		return -1;
	}
	/*
	 * bits pre word
	 */
	status = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	memset(&tr, 0, sizeof(tr));
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));

	tx[3] = 0x50;
	tx[2] = 0x00;
	tx[1] = 0x03;
	tx[0] = 0x37;

	PICO640_GPIO_SetValue(CVI_GPIOB_20, 0);

	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	PICO640_GPIO_SetValue(CVI_GPIOB_20, 1);

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));

	tx[3] = 0x30;
	tx[2] = 0x00;
	tx[1] = 0x03;
	tx[0] = 0x49;

	PICO640_GPIO_SetValue(CVI_GPIOB_20, 0);

	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	PICO640_GPIO_SetValue(CVI_GPIOB_20, 1);

	close(fd);

	// ADC
	speed = 500000;
	mode = 0;
	bits = 16;
	delay = 0;
	len = 2;

	fd = open(spi_dev, O_RDWR);
	if (fd < 0) {
		printf("open %s error\n", spi_dev);
		return -1;
	}

	/*
	 * spi mode
	 */
	status = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (status < 0) {
		printf("SPI IOC WR ERROR\n");
	}
	/*
	 * max speed hz
	 */
	status = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (status < 0) {
		printf("SPI SPEED ERROR\n");
		return -1;
	}
	/*
	 * bits pre word
	 */
	status = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	memset(&tr, 0, sizeof(tr));
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = len;
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));
	/* sw reset */

	/* addr*/
	tx[1] = 0x00;
	/* data*/
	tx[0] = 0x80;

	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	memset(tx, 0, sizeof(tx));
	memset(rx, 0, sizeof(rx));
	/* sw reset */

	/* addr*/
	tx[1] = 0x03;
	/* data*/
	tx[0] = 0x30;

	status = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (status < 0) {
		printf("SPI IOC ERROR\n");
		return -1;
	}

	close(fd);
	return 0;
}

int pico640_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunPICO640_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, pico640_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int pico640_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int pico640_read_register(VI_PIPE ViPipe, int addr)
{
	/* TODO:*/
	(void) ViPipe;
	(void) addr;

	return CVI_SUCCESS;
}

int pico640_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (pico640_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (pico640_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, pico640_addr_byte + pico640_data_byte);
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


void pico640_init(VI_PIPE ViPipe)
{
	pico640_2256_init();

	pico640_i2c_init(ViPipe);
	pico640_linear_480p30_init(ViPipe);
	g_pastPICO640[ViPipe]->bInit = CVI_TRUE;
}

void pico640_exit(VI_PIPE ViPipe)
{
	pico640_i2c_exit(ViPipe);
}

#define PICO640_NUMBER_OF_LINE 480
#define PICO640_NUMBER_OF_COLUMN 640
#define PICO640_PCLK	19100000// MC/2

static void PICO640_SetFrequence(VI_PIPE ViPipe, CVI_U32 fps)
{
	CVI_U32 dwInterFrame, dwInterLine, dwActiveFrame;

	dwActiveFrame = (PICO640_NUMBER_OF_COLUMN+23)*PICO640_NUMBER_OF_LINE;
	dwInterFrame = 1;

	if (fps != 0)
		dwInterLine = ((PICO640_PCLK/fps) >= (dwActiveFrame+dwInterFrame*PICO640_NUMBER_OF_COLUMN))
				? (((PICO640_PCLK/fps)-(dwActiveFrame+dwInterFrame*PICO640_NUMBER_OF_COLUMN))
				/PICO640_NUMBER_OF_LINE)
				: 0;
	else
		dwInterLine = ((PICO640_PCLK/60) >= (dwActiveFrame+dwInterFrame*PICO640_NUMBER_OF_COLUMN))
				? (((PICO640_PCLK/60)-(dwActiveFrame+dwInterFrame*PICO640_NUMBER_OF_COLUMN))
				/PICO640_NUMBER_OF_LINE)
				: 0;

	if (dwInterLine > 0xFF) {
		dwInterLine = 0xFF;

		if (fps != 0)
			dwInterFrame = ((PICO640_PCLK/fps) >= (dwActiveFrame+dwInterLine*PICO640_NUMBER_OF_LINE))
					? (((PICO640_PCLK/fps)-(dwActiveFrame+dwInterLine*PICO640_NUMBER_OF_LINE))
					/PICO640_NUMBER_OF_COLUMN)
					: 0;
		else
			dwInterFrame = ((PICO640_PCLK/60) >= (dwActiveFrame+dwInterLine*PICO640_NUMBER_OF_LINE))
					? (((PICO640_PCLK/60)-(dwActiveFrame+dwInterLine*PICO640_NUMBER_OF_LINE))
					/PICO640_NUMBER_OF_COLUMN)
					: 0;

		dwInterFrame = (dwInterFrame > 0xFF)
						? 0xFF
						: (dwInterFrame <= 1)
						? 1
						: dwInterFrame;
	}

	//ptInfo->dwHsync = (PICO640_NUMBER_OF_COLUMN+23) + dwInterLine;
	//ptInfo->dwVsync = PICO640_NUMBER_OF_COLUMN + dwInterFrame;

	pico640_write_register(ViPipe, 0x0056, (dwInterFrame&0xFF));
	pico640_write_register(ViPipe, 0x0062, (dwInterLine&0xFF));
}

/* 480P30 and 480P25 */
static void pico640_linear_480p30_init(VI_PIPE ViPipe)
{
	pico640_write_register(ViPipe, 0x0040, 0x83);
	pico640_write_register(ViPipe, 0x0041, 0x40);
	pico640_write_register(ViPipe, 0x0042, 0x40);
	pico640_write_register(ViPipe, 0x004F, 0x01);
	pico640_write_register(ViPipe, 0x0050, 0x6F);
	pico640_write_register(ViPipe, 0x0064, 0x00);
	pico640_write_register(ViPipe, 0x005B, 0x1B);
	pico640_write_register(ViPipe, 0x005C, 0x03);
	pico640_write_register(ViPipe, 0x005C, 0x07);

	/* set size */
	pico640_write_register(ViPipe, 0x0043, 0x00);
	pico640_write_register(ViPipe, 0x0044, 0x00);
	pico640_write_register(ViPipe, 0x0045, 0x01);
	pico640_write_register(ViPipe, 0x0046, 0xDF);

	pico640_write_register(ViPipe, 0x0047, 0x00);
	pico640_write_register(ViPipe, 0x0048, 0x00);
	pico640_write_register(ViPipe, 0x0049, 0x02);
	pico640_write_register(ViPipe, 0x004A, 0x7F);

	/* set frequency */
	PICO640_SetFrequence(ViPipe, 30);
	delay_ms(100);

	pico640_write_register(ViPipe, 0x005C, 0x07);

	printf("ViPipe:%d,===PICO640 Init OK!===\n", ViPipe);
}

