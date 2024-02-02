#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
  int fbfd = -1;
  char *fbdev = "/dev/fb0";
  char *s;
  chdir("/");
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  switch(fork()) {
  case -1:
    perror("fork");
    exit(EXIT_FAILURE);
  case 0:
    s = getenv("FRAMEBUFFER");
    if (s != NULL) {
      fbdev = s;
    }
    fbfd = open(fbdev, O_RDWR);
    if (fbfd < 0) {
      perror("can't open framebuffer device");
      exit(EXIT_FAILURE);
    }
    // nothing, just open framebuffer node
    // for buggy cvi_fb.ko driver
    while(1) {
      sleep(100000);
    }
    break;
  default:
    _exit(EXIT_SUCCESS);
    break;
  }
}
