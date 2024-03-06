/***************************************************************************
 * Copyright (C) CVITEK, Inc - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Jammy Huang <jammy.huang@wisecore.com.tw>, Nov. 2019
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <devmem.h>
#include <linux/cvi_common.h>

int i2c_file = -1;
int i2c_slave_addr;

int i2cInit(uint8_t i2c_bus, uint8_t slave_addr)
{
#define FILENAME_SIZE 12
	char filename[FILENAME_SIZE];
	char writeSize = 0;

	writeSize = snprintf(filename, FILENAME_SIZE, "/dev/i2c-%d", i2c_bus);
	if (writeSize > FILENAME_SIZE) {
		perror("Please enlarge the space of filename");
	}

	i2c_file = open(filename, O_RDWR);
	if (i2c_file < 0) {
		/* ERROR HANDLING: you can check errno to see what went wrong */
		perror("Failed to open the i2c bus");
		return -1;
	}

	i2c_slave_addr = slave_addr;
	return 0;
}

void i2cExit(void)
{
	close(i2c_file);
}

int i2cRead(uint16_t addr, unsigned int addr_nbytes, uint8_t *buffer,
	    unsigned int data_nbytes)
{
	if (addr_nbytes > 2) {
		printf("i2c addr length at most 2.\n");
		return -1;
	}

	uint8_t tx[2];

	if (addr_nbytes == 2) {
		tx[0] = addr >> 8;
		tx[1] = addr & 0xff;
	} else {
		tx[0] = addr & 0xff;
	}

	struct i2c_msg messages[] = {
		{
			.addr = i2c_slave_addr,
			.flags = 0,
			.buf = tx,
			.len = addr_nbytes,
		},
		{
			.addr = i2c_slave_addr,
			.flags = I2C_M_RD | I2C_M_NOSTART,
			.buf = buffer,
			.len = data_nbytes,
		},
	};

	struct i2c_rdwr_ioctl_data payload = {
		.msgs = messages,
		.nmsgs = ARRAY_SIZE(messages),
	};

	if (ioctl(i2c_file, I2C_RDWR, &payload) < 0) {
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to read from the i2c bus.\n");
		return -1;
	}
	return 0;
}

int i2cWrite(uint16_t addr, unsigned int addr_nbytes, uint8_t *buffer,
	     unsigned int data_nbytes)
{
	if (addr_nbytes > 2) {
		printf("i2c addr length at most 2.\n");
		return -1;
	}

	uint16_t len = addr_nbytes + data_nbytes;
	uint8_t *tx = malloc(len);

	if (addr_nbytes == 2) {
		tx[0] = addr >> 8;
		tx[1] = addr & 0xff;
		memcpy(&tx[2], buffer, data_nbytes);
	} else {
		tx[0] = addr & 0xff;
		memcpy(&tx[1], buffer, data_nbytes);
	}

	struct i2c_msg messages[] = {
		{
			.addr = i2c_slave_addr,
			.flags = 0,
			.buf = tx,
			.len = len,
		},
	};

	struct i2c_rdwr_ioctl_data payload = {
		.msgs = messages,
		.nmsgs = ARRAY_SIZE(messages),
	};

	if (ioctl(i2c_file, I2C_RDWR, &payload) < 0) {
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to write to the i2c bus.\n");
		free(tx);
		return -1;
	}

	free(tx);
	return 0;
}

// TODO: refine
#define ISP_BASE_ADDR 0x0A000000
#define ISP_REG_RANGE 0x80000

static void *reg_base;
static int devm_fd = -1;

int regInit(void)
{
	if (devm_fd != -1)
		return -EMFILE;

	devm_fd = devm_open();
	if (devm_fd < 0)
		return -1;
	reg_base = devm_map(devm_fd, ISP_BASE_ADDR, ISP_REG_RANGE);
	return 0;
}

int regExit(void)
{
	if (reg_base != 0)
		devm_unmap(reg_base, ISP_REG_RANGE);
	if (devm_fd)
		devm_close(devm_fd);
	devm_fd = -1;
	return 0;
}

uint32_t regRead(uint32_t addr)
{
	return *(uint32_t *)(reg_base + addr - ISP_BASE_ADDR);
}

void regWrite(uint32_t addr, uint32_t data)
{
	*(uint32_t *)(reg_base + addr - ISP_BASE_ADDR) = data;
}

void regReadBurst(uint32_t addr, uint32_t *buffer, unsigned int ndws)
{
	for (unsigned int i = 0; i < ndws; ++i)
		buffer[i] = regRead(addr + (i << 2));
}

void regWriteBurst(uint32_t addr, uint32_t *buffer, unsigned int ndws)
{
	for (unsigned int i = 0; i < ndws; ++i)
		regWrite(addr + (i << 2), buffer[i]);
}

void regWriteMask(uint32_t addr, uint32_t data, uint32_t mask)
{
	uint32_t value;

	value = regRead(addr) & ~mask;
	value |= (data & mask);
	regWrite(addr, value);
}
