#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/poll.h>
#include <syslog.h>

#if 0
#define LOGV(args...) printf(args)
#else
#define LOGV(args...)
#endif

int i2c_read(int file, unsigned short addr, unsigned short reg, unsigned short reg_w, unsigned char *r_val)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
	int ret;
	unsigned char temp[2];

	messages[0].addr = addr;
	messages[0].flags = 0;
	messages[0].len = reg_w;

	switch (reg_w) {
	case 1:
		temp[0] = reg;
		break;
	case 2:
		temp[0] = reg >> 8;
		temp[1] = reg;
		break;
	default:
		printf("No support of this register width\n");
		return -1;
	}

	messages[0].buf = temp;
	/* The data will get returned in this structure */
	messages[1].addr = addr;
	/* | I2C_M_NOSTART */
	messages[1].flags = I2C_M_RD;
	messages[1].len = 1;
	messages[1].buf = r_val;

	/* Send the request to the kernel and get the result back */
	packets.msgs = messages;
	packets.nmsgs = 2;
	ret = ioctl(file, I2C_RDWR, &packets);

	if (ret < 0) {
		perror("Unable to send data");
		return ret;
	}

	//printf("get val=%x\n", *r_val);

	return 0;
}

int main(int argc, char const *argv[])
{
	int ret, i2c_file;
	char chrdev_name[12];
	unsigned short dev_addr, reg_addr;
	unsigned short reg_w = 0;
	unsigned short data_w = 0;
	unsigned short end_reg = 0, step = 0;
	unsigned int i;
	unsigned short val = 0;

	if ((argc >= 4) && (argc <= 8)) {
		dev_addr = strtol(argv[2], NULL, 0);
		reg_addr = strtol(argv[3], NULL, 0);
	} else {
		printf("usage: i2c_read <i2c_num> <device_addr> <reg_addr> <end_reg_addr>\n");
		printf("       <reg_width> <data_width> <reg_step>\n");
		printf("\n");
		printf("Note: i2c_num, device_addr, reg_addr are necessary\n");
		return -1;
	}

	switch (argc) {
	case 8:
		step = strtol(argv[7], NULL, 0);
	case 7:
		data_w = strtol(argv[6], NULL, 0);
	case 6:
		reg_w = strtol(argv[5], NULL, 0);
		end_reg = strtol(argv[4], NULL, 0);
		break;
	case 5:
		end_reg = strtol(argv[4], NULL, 0);
		break;
	}

	if (reg_w == 0)
		reg_w = 2; /* set default register width to 2 bytes */
	if (end_reg == 0)
		end_reg = reg_addr;
	if (data_w == 0)
		data_w = 1; /* default data width is 1 byte */
	if (step == 0)
		step = 1; /* default step is 1 */

	if ((end_reg - reg_addr) < 0) {
		printf("end_reg_addr is smaller than start register addr\n");
		return -1;
	}

	ret = sprintf(chrdev_name, "/dev/i2c-%s", argv[1]);
	if (ret < 0)
		return -ENOMEM;

	i2c_file = open(chrdev_name, O_RDWR);

	if (i2c_file < 0) {
		printf("open I2C device %s failed err=%d\n", argv[1], errno);
		return -ENODEV;
	}
	printf("\n -- dump start (offset = %d)--\n\n", step);
	printf("       %02x %02x %02x %02x %02x %02x %02x %02x",
			0, step * 1, step * 2, step * 3, step * 4, step * 5, step * 6, step * 7);
	printf(" %02x %02x %02x %02x %02x %02x %02x %02x\n",
			step * 8, step * 9, step * 10, step * 11, step * 12, step * 13, step * 14, step * 15);
	for (i = 0; i <= (end_reg - reg_addr); i += step) {
		ret = i2c_read(i2c_file, dev_addr, (reg_addr+i), reg_w, (unsigned char *)&val);
		if (i % (16 * step) == 0)
			printf("\n0x%02x: ", (reg_addr + i));
		printf(" %02x", val);
	}

	printf("\n\n -- dump finished --\n\n");
	close(i2c_file);
	return 0;
}
