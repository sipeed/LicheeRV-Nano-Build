#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
	int fd;
	if (argc < 2) {
		fprintf(stderr, "usage: %s /dev/ttyX\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	ioctl(fd, TIOCCONS);
	close(fd);
	exit(EXIT_SUCCESS);
}
