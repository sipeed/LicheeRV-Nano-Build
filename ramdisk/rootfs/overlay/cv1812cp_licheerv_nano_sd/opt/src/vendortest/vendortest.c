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

// vendor test
// use tcl/tk write this program may be a good choise
// but xorg is not ready for this system

struct fb_fix_screeninfo fb_finfo;
struct fb_var_screeninfo fb_vinfo;

uint8_t *fb = NULL;

#define BLUE  (0xFF << 0)
#define GREEN (0xFF << 8)
#define RED   (0xFF << 16)
#define WHITE (BLUE | GREEN | RED)
#define BLACK (0x00)
#define YELLO (REG | GREEN)
#define AFULL (0xFF << 24)

static inline void draw_pixel(int x, int y, uint32_t data) {
  uint8_t *p;
  p = fb;
  p += y * fb_finfo.line_length + x * 4; // 32bit framebuffer
  uint32_t *p32;
  p32 = (uint32_t *)p;
  *p32 = data;
}

static inline void draw_xline(int xmin, int xmax, int y,
			      uint32_t data) {
  for (; xmin <= xmax; xmin++) {
    draw_pixel(xmin, y, data);
  }
}

static inline void draw_yline(int ymin, int ymax, int x,
			      uint32_t data) {
  for (; ymin <= ymax; ymin++) {
    draw_pixel(x, ymin, data);
  }
}


static inline void draw_rect(int xmin, int ymin,
			     int xmax, int ymax,
			     uint32_t data) {
  draw_xline( xmin, xmax, ymin, data);
  draw_xline( xmin, xmax, ymax, data);
  draw_yline( ymin, ymax, xmin, data);
  draw_yline( ymin, ymax, xmax, data);
}

static inline void draw_solid(int xmin, int ymin,
			      int xmax, int ymax,
			      uint32_t data) {
  for (; ymin <= ymax; ymin++) {
    draw_xline(xmin, xmax, ymin, data);
  }
}

#include "uni_vga.h"


// fallbback, no font
#ifndef FONT_H
#define FONT_H 0
#endif

#ifndef FONT_W
#define FONT_W 0
#endif

#ifndef FONT_CODE_POINTS
#define FONT_CODE_POINTS 0
#endif

static inline void draw_char(int x, int y,
			     uint32_t fg, uint32_t bg,
			     int c) {
  int i;
  int found = 0;
  // todo, replace by binary search
  for (i = 0; i < FONT_CODE_POINTS; i++) {
    if (c == font_default_code_points[i]) {
      found = 1;
      break;
    }
  }
  if (!found) {
    return;
  }
  int xf, yf;
  for (yf = 0; yf < FONT_H; yf++) {
    for (xf = 0; xf < FONT_W; xf++) {
      if (font_default_data[i][yf] & (1 << (FONT_W - xf))) {
	draw_pixel(x + xf, y + yf, fg);
      } else {
	draw_pixel(x + xf, y + yf, bg);
      }
    }
  }
}

static inline void draw_str(int x, int y,
			    uint32_t fg, uint32_t bg,
			    char *s) {
  int i = 0;
  while(s[i] != '\0') {
    draw_char(x, y, fg, bg, s[i]);
    x += FONT_W;
    i++;
  }
}
static inline void draw_progress(int xmin, int ymin,
				 int xmax, int ymax,
				 uint32_t fg, uint32_t bg,
				 int per) {
  ymin -= 2;
  ymax -= 2;
  draw_solid(xmin, ymin, xmax, ymax, bg);
  xmin += 1;
  ymin += 1;
  xmax -= 1;
  ymax -= 1;
  float xshow;
  xshow = xmax - xmin;
  xshow = xshow * ((float)per / 100.0);
  draw_solid(xmin, ymin, xmin + xshow, ymax, fg);
}

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
  
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_finfo) < 0) {
    perror("can't get framebuffer finfo");
    exit(EXIT_FAILURE);
  }
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_vinfo) < 0) {
    perror("can't get framebuffer vinfo");
    exit(EXIT_FAILURE);
  }

  fb = mmap(NULL, fb_finfo.smem_len, PROT_READ | PROT_WRITE,
	    MAP_SHARED, fbfd, 0);
  if (fb == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  memset(fb, 0, fb_finfo.smem_len);

  int touchfd = -1;
  touchfd = open("/dev/input/event1", O_RDONLY);

  // fb_vinfo.xres // screen x max
  // fb_vinfo.yres // screen y max

  int xmin, ymin;
  int xmax, ymax;
  int ystep, xstep;
  int xmin_save;
  int ymin_save;
  int xmax_save;
  int ymax_save;
  int xstep_save, ystep_save;

  int ret;
  // div 16
  ystep = fb_vinfo.yres >> 3;
  xstep = fb_vinfo.xres >> 3;

  uint32_t bg = WHITE | AFULL;
  uint32_t fg = BLACK | AFULL;
  
  // draw RED GREEN BLUE WHITE BLACK block
  xmin = 0;
  ymin = 0;
  xmax = xmin + xstep;
  ymax = ymin + ystep;

  draw_solid(xmin, ymin, xmax, ymax, BLACK | AFULL);
  draw_str(xmin, ymin, fg, bg, "BLACK");
  xmin += xstep;
  xmax += xstep;
  draw_solid(xmin, ymin, xmax, ymax, WHITE | AFULL);
  draw_str(xmin, ymin, fg, bg, "WHITE");
  xmin += xstep;
  xmax += xstep;
  draw_solid(xmin, ymin, xmax, ymax, RED | AFULL);
  draw_str(xmin, ymin, fg, bg, "RED");
  xmin += xstep;
  xmax += xstep;
  draw_solid(xmin, ymin, xmax, ymax, GREEN | AFULL);
  draw_str(xmin, ymin, fg, bg, "GREEN");
  xmin += xstep;
  xmax += xstep;
  draw_solid(xmin, ymin, xmax, ymax, BLUE | AFULL);
  draw_str(xmin, ymin, fg, bg, "BLUE");
  xmin += xstep;
  xmax += xstep;

  // draw v line
  xmin_save = xmin;
  for (;xmin < xmax;xmin++) {
    if (xmin & 1) {
      draw_yline(ymin, ymax, xmin, fg);
    } else {
      draw_yline(ymin, ymax, xmin, bg);
    }
  }
  xmin = xmin_save;
  draw_str(xmin, ymin, fg, bg, "VLINE");

  xmin += xstep;
  xmax += xstep;

  // draw h line
  ymin_save = ymin;
  for (;ymin < ymax;ymin++) {
    if (ymin & 1) {
      draw_xline(xmin, xmax, ymin, bg);
    } else {
      draw_xline(xmin, xmax, ymin, fg);
    }
  }
  ymin = ymin_save;
  draw_str(xmin, ymin, fg, bg, "HLINE");

  xmin += xstep;
  xmax += xstep;

  // draw chess board
  xmin_save = xmin;
  ymin_save = ymin;
  for (;ymin < ymax;ymin++) {
    for (xmin = xmin_save;xmin < xmax;xmin++) {
      if ((xmin + ymin) & 1) {
	draw_pixel(xmin, ymin, fg);
      } else {
	draw_pixel(xmin, ymin, bg);
      }
    }
  }
  xmin = xmin_save;
  ymin = ymin_save;
  draw_str(xmin, ymin, fg, bg, "CHESS");

  // gap
  xmin += 3;
  xmax += 3;
  ymin += 3;
  ymax += 3;
  
  xmin_save = xmin;
  xmax_save = xmax;
  ymin_save = ymin;
  ymax_save = ymax;
  xstep_save = xstep;
  ystep_save = ystep;


  // -1 is not data
  int eth_rx_time = -1;
  int wifi_rx_time = -1;

  int touch_xmove = -1;
  int touch_ymove = -1;
  int touch_btn = -1;
  
  int font_fg;
  int font_bg;

  FILE *logfp;
 
  logfp = popen("tail -f /tmp/ramdisk/vendor_test.log", "r");
  if (logfp == NULL) {
    perror("can't open log");
    exit(EXIT_FAILURE);
  }

  int buttonfd;
  buttonfd = open("/dev/input/event0", O_RDONLY);

  struct {
    struct timeval time;
    unsigned short type;
    unsigned short code;
    unsigned int val;
  } inputevdata;

  if (touchfd >= 0) {
    switch(fork()) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      while(1) {
	if (read(touchfd, &inputevdata, sizeof(inputevdata))
	    == sizeof(inputevdata)) {
	  switch(inputevdata.code) {
	  case 53:
	    touch_xmove = inputevdata.val;
	    break;
	  case 54:
	    touch_ymove = inputevdata.val;
	    break;
	  case 330:
	    touch_btn = inputevdata.val;
	    break;
	  }
	}
	fg = WHITE | AFULL;
	if (touch_btn == 0) {
	  touch_xmove = 0;
	  touch_ymove = 0;
	}

	int touch_xmax;
	int touch_xmin;
	int touch_ymax;
	int touch_ymin;

#define CURSIZE 10

	if (touch_btn > 0) {
	  if (touch_xmove > (((int)fb_vinfo.xres) - 1)) {
	    touch_xmove = (((int)fb_vinfo.xres) -1);
	  }
	  if (touch_ymove > (((int)fb_vinfo.yres) - 1)) {
	    touch_ymove = (fb_vinfo.yres - 1);
	  }
	  if (touch_xmove < 0) {
	    touch_xmove = 0;
	  }
	  if (touch_ymove < 0) {
	    touch_ymove = 0;
	  }
	  touch_xmax = touch_xmove + CURSIZE;
	  if (touch_xmax > (((int)fb_vinfo.xres) - 1)) {
	    touch_xmax = fb_vinfo.xres - 1;
	  }
	  touch_xmin = touch_xmove - CURSIZE;
	  if (touch_xmin < 0) {
	    touch_xmin = 0;
	  }
	  touch_ymax = touch_ymove + CURSIZE;
	  if (touch_ymax > (((int)fb_vinfo.yres) - 1)) {
	    touch_ymax = fb_vinfo.yres - 1;
	  }
	  touch_ymin = touch_ymove - CURSIZE;
	  if (touch_ymin < 0) {
	    touch_ymin = 0;
	  }
	  draw_rect(touch_xmin, touch_ymin,
		    touch_xmax, touch_ymax,
		    fg);
	  draw_xline(touch_xmin, touch_xmax,
		     touch_ymove, fg);
	  draw_yline(touch_ymin, touch_ymax,
		     touch_xmove, fg);
	}
      }
    }
  }

  if (buttonfd >= 0) {
    switch(fork()) {
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
    case 0:
      while(1) {
	if (read(buttonfd, &inputevdata, sizeof(inputevdata))
	    == sizeof(inputevdata)) {
	  switch(inputevdata.code) {
	  case 431:
	    if (inputevdata.val == 1) {
	      draw_solid(fb_vinfo.xres - 40, fb_vinfo.yres - 40,
			fb_vinfo.xres - 1, fb_vinfo.yres - 1,
			GREEN | AFULL);
	    } else {
	      draw_solid(fb_vinfo.xres - 40, fb_vinfo.yres - 40,
			fb_vinfo.xres - 1, fb_vinfo.yres - 1,
			0);
	    }
	    break;
	  }
	}
      }
    }
  }

  

  time_t start_time;
  start_time = time(NULL);

  char buf[128];
  buf[0] = '\0';
  while(1) {
    if (strstr(buf, "wifi rx 10MiB test (s): ")
	!= NULL) {
      strtok(buf, ":");
      s = strtok(NULL, "\n\r");
      if (s != NULL) {
	wifi_rx_time = atoi(s);
      } else {
	;
      }
    } else if (strstr(buf, "eth rx 10MiB test (s): ")
	       != NULL) {
      strtok(buf, ":");
      s = strtok(NULL, "\n\r");
      if (s != NULL) {
	eth_rx_time = atoi(s);
      } else {
	;
      }
    }
    fg = BLACK | AFULL;
    bg = WHITE | AFULL;
    font_fg = WHITE | AFULL;
    font_bg = 0x00; // nothing

    xmin = xmin_save;
    ymin = ymin_save;
    xmax = xmax_save;
    ymax = ymax_save;
    xstep = xstep_save;
    ystep = ystep_save;

    // next
    xstep = fb_vinfo.xres >> 1;
    xmin = 2;
    xmax = xmin + xstep;
    ymin += ystep;
    ystep = FONT_H + 2;
    ymax += ystep;


    snprintf(buf, 127, "ETH RX 10MiB TEST (S): %d   ",
	     eth_rx_time);
    if (eth_rx_time > 3) {
      font_fg = RED | AFULL;
    } else if (eth_rx_time == -1) {
      font_fg = WHITE | AFULL;
    } else {
      font_fg = GREEN | AFULL;
    }
    draw_str(xmin, ymin, font_fg, font_bg, buf);
    ymin += ystep;
    ymax += ystep;

    snprintf(buf, 127, "WIFI RX 10MiB TEST (S): %d   ",
	     wifi_rx_time);
    if (wifi_rx_time > 15) {
      font_fg = RED | AFULL;
    } else if (wifi_rx_time == -1) {
      font_fg = WHITE | AFULL;
    } else {
      font_fg = GREEN | AFULL;
    }
    draw_str(xmin, ymin, font_fg, font_bg, buf);
    ymin += ystep;
    ymax += ystep;

    // show tested time
    snprintf(buf, 127, "TIME (S): %ld   ",
	     time(NULL) - start_time);
    font_fg = WHITE | AFULL;
    draw_str(xmin, ymin, font_fg, font_bg, buf);
    ymin += ystep;
    ymax += ystep;

    // wait draw
    ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);

    // next result
    fgets(buf, 127, logfp);
    buf[127] = '\0';
  }
}
