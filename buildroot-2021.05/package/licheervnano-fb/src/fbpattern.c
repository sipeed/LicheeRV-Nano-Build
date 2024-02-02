#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "fbabgr1555.h"

int main(void) {
  int fbfd = -1;
  char *fbdev = "/dev/fb0";
  char *s;
  s = getenv("FRAMEBUFFER");
  if (s != NULL) {
    fbdev = s;
  }
  fbfd = open(fbdev, O_RDWR);
  if (fbfd < 0) {
    perror("can't open framebuffer device");
    exit(EXIT_FAILURE);
  }

  fbabgr1555 fb;
  
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb.fix_info) < 0) {
    perror("can't get framebuffer fix info");
    exit(EXIT_FAILURE);
  }
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb.var_info) < 0) {
    perror("can't get framebuffer var info");
    exit(EXIT_FAILURE);
  }
  
  fb.mem = mmap(NULL, fb.fix_info.smem_len, PROT_READ | PROT_WRITE,
		MAP_SHARED, fbfd, 0);
  if (fb.mem == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  int x, y;

  int patsize;
  patsize = fb.var_info.xres >> 3;

  x = patsize + (patsize >> 1);
  y = patsize + (patsize >> 1);

  fbabgr1555_draw_solid(&fb,
			x - patsize,
			y - patsize,
			x,
			y,
			ABGR1555_RED | ABGR1555_ALPHA);
  fbabgr1555_draw_str(&fb,
		      x - patsize,
		      y - patsize,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " RED ");

  fbabgr1555_draw_solid(&fb,
			x,
			y - patsize,
			x + patsize,
			y,
			ABGR1555_GREEN | ABGR1555_ALPHA);
  fbabgr1555_draw_str(&fb,
		      x,
		      y - patsize,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " GREEN ");


  fbabgr1555_draw_solid(&fb,
			x - patsize,
			y,
			x,
			y + patsize,
			ABGR1555_BLUE | ABGR1555_ALPHA);
  fbabgr1555_draw_str(&fb,
		      x - patsize,
		      y,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " BLUE ");

  fbabgr1555_draw_solid(&fb,
			x,
			y,
			x + patsize,
			y + patsize,
			ABGR1555_WHITE | ABGR1555_ALPHA);
  fbabgr1555_draw_str(&fb,
		      x,
		      y,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " WHITE ");

  x = fb.var_info.xres - x;
  y = fb.var_info.yres - y;

  int xmin, ymin, xmax, ymax;
  uint16_t color;

  xmin = x - patsize;
  ymin = y - patsize;
  xmax = x;
  ymax = y;

  for (; xmin < xmax; xmin++) {
    if (xmin & 1) {
      color = ABGR1555_BLACK | ABGR1555_ALPHA;
    } else {
      color = ABGR1555_WHITE | ABGR1555_ALPHA;
    }
    fbabgr1555_draw_yline(&fb, ymin, ymax, xmin, color);
  }
  fbabgr1555_draw_str(&fb,
		      x - patsize,
		      y - patsize,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " VLINE ");

  xmin = x;
  ymin = y - patsize;
  xmax = x + patsize;
  ymax = y;

  for (; ymin < ymax; ymin++) {
    if (ymin & 1) {
      color = ABGR1555_BLACK | ABGR1555_ALPHA;
    } else {
      color = ABGR1555_WHITE | ABGR1555_ALPHA;
    }
    fbabgr1555_draw_xline(&fb, xmin, xmax, ymin, color);
  }
  fbabgr1555_draw_str(&fb,
		      x,
		      y - patsize,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " HLINE ");


  xmin = x - patsize;
  ymin = y;
  xmax = x;
  ymax = y + patsize;

  for (; ymin < ymax; ymin++) {
    for (xmin = x - patsize; xmin < xmax; xmin++) {
      if ((xmin + ymin) & 1) {
	color = ABGR1555_BLACK | ABGR1555_ALPHA;
      } else {
	color = ABGR1555_WHITE | ABGR1555_ALPHA;
      }
      fbabgr1555_draw_pixel(&fb, xmin, ymin, color);
    }
  }

  fbabgr1555_draw_str(&fb,
		      x - patsize,
		      y,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " CHESS ");

  fbabgr1555_draw_solid(&fb,
			x,
			y,
			x + patsize,
			y + patsize,
			ABGR1555_BLACK | ABGR1555_ALPHA);
  fbabgr1555_draw_str(&fb,
		      x,
		      y,
		      ABGR1555_BLACK | ABGR1555_ALPHA,
		      ABGR1555_WHITE | ABGR1555_ALPHA,
		      " BLACK ");

  exit(EXIT_SUCCESS);
}
