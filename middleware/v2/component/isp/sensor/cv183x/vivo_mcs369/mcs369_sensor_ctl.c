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
#include "mcs369_cmos_ex.h"

const CVI_U8 mcs369_i2c_addr = 0x13;        /* I2C Address of MCS369 */
const CVI_U32 mcs369_addr_byte = 2;
const CVI_U32 mcs369_data_byte = 1;

int mcs369_read_register(VI_PIPE ViPipe, int addr)
{
	(void) ViPipe;
	(void) addr;

	return CVI_SUCCESS;
}

int mcs369_write_register(VI_PIPE ViPipe, int addr, int data)
{
	(void) ViPipe;
	(void) addr;
	(void) data;

	return CVI_SUCCESS;
}

void mcs369_init(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

void mcs369_exit(VI_PIPE ViPipe)
{
	(void) ViPipe;
}

