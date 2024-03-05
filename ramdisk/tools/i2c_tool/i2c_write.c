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

int i2c_write(int file, unsigned short addr, unsigned short reg, unsigned short val, unsigned short reg_w,
			unsigned short val_w)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
	int ret;
	unsigned char temp[4];

	messages[0].addr = addr;
	messages[0].flags = 0;
	messages[0].len = reg_w + val_w;

		switch (reg_w) {
		case 1:
			temp[0] = reg;
			switch (val_w) {
			case 1:
				temp[1] = val;
				break;
			case 2:
				temp[1] = val >> 8;
				temp[2] = val;
				break;
			default:
				printf("No support of this value width\n");
			}
			break;
		case 2:
			temp[0] = reg >> 8;
			temp[1] = reg;
			switch (val_w) {
			case 1:
				temp[2] = val;
				break;
			case 2:
				temp[2] = val >> 8;
				temp[3] = val;
				break;
			default:
				printf("No support of this value width\n");
			}
			break;
		default:
			printf("No support of this register width\n");
			return -1;
		}

	messages[0].buf = temp;

	/* Send the request to the kernel and get the result back */
	packets.msgs = messages;
	packets.nmsgs = 1;
	ret = ioctl(file, I2C_RDWR, &packets);

	if (ret < 0) {
		perror("Unable to send data");
		return ret;
	}

	return 0;
}

int main(int argc, char const *argv[])
{
	int ret, i2c_file;
	char chrdev_name[12];
	unsigned short dev_addr, reg_addr;
	unsigned short val;
	unsigned short reg_w = 0;
	unsigned short data_w = 0;


	if ((argc >= 5) && (argc <= 7)) {
		dev_addr = strtol(argv[2], NULL, 0);
		reg_addr = strtol(argv[3], NULL, 0);
		val = strtol(argv[4], NULL, 0);
	} else {
		printf("usage: i2c_write <i2c_num> <device_addr> <reg_addr> <value>\n");
		printf("       <reg_width> <data_width>\n");
		printf("\n");
		printf("Note: i2c_num, device_addr, reg_addr, value are necessary\n");
		return -1;
	}

	switch (argc) {
	case 7:
		data_w = strtol(argv[6], NULL, 0);
	case 6:
		reg_w = strtol(argv[5], NULL, 0);
	default:
		if (reg_w == 0)
			reg_w = 2; /* set default register width to 2 bytes */
		if (data_w == 0)
			data_w = 1; /* default data width is 1 byte */
		break;
	}

	ret = sprintf(chrdev_name, "/dev/i2c-%s", argv[1]);
	if (ret < 0)
		return -ENOMEM;

	i2c_file = open(chrdev_name, O_RDWR);

	if (i2c_file < 0) {
		printf("open I2C device %s failed err=%d\n", argv[1], errno);
		return -ENODEV;
	}

	ret = i2c_write(i2c_file, dev_addr, reg_addr, val, reg_w, data_w);

	close(i2c_file);
	return 0;
}
