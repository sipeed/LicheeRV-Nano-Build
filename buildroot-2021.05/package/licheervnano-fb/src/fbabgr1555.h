#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define ABGR1555_BLUE  (0b11111 << 0)
#define ABGR1555_GREEN (0b11111 << 5)
#define ABGR1555_RED   (0b11111 << 10)
#define ABGR1555_WHITE (ABGR1555_BLUE | ABGR1555_GREEN | ABGR1555_RED)
#define ABGR1555_BLACK (0x00)
#define ABGR1555_YELLO (ABGR1555_RED | ABGR1555_GREEN)
#define ABGR1555_ALPHA (0b1 << 15)

struct fbabgr1555 {
  uint8_t *mem;
  struct fb_fix_screeninfo fix_info;
  struct fb_var_screeninfo var_info;
};
typedef struct fbabgr1555 fbabgr1555;

static inline void fbabgr1555_draw_pixel(fbabgr1555 *fb, int x, int y,
                                         uint16_t data) {
  if ((((uint16_t)x) >= fb->var_info.xres) ||
      (((uint16_t)y) >= fb->var_info.yres)) {
    return;
  }
  uint8_t *p;
  p = fb->mem;
  p += y * fb->fix_info.line_length + x * 2; // 16bit framebuffer
  uint16_t *p16;
  p16 = (uint16_t *)p;
  *p16 = data;
}

static inline void fbabgr1555_draw_xline(fbabgr1555 *fb, int xmin, int xmax,
                                         int y, uint16_t data) {
  for (; xmin <= xmax; xmin++) {
    fbabgr1555_draw_pixel(fb, xmin, y, data);
  }
}

static inline void fbabgr1555_draw_yline(fbabgr1555 *fb, int ymin, int ymax,
                                         int x, uint16_t data) {
  for (; ymin <= ymax; ymin++) {
    fbabgr1555_draw_pixel(fb, x, ymin, data);
  }
}

static inline void fbabgr1555_draw_rect(fbabgr1555 *fb, int xmin, int ymin,
                                        int xmax, int ymax, uint16_t data) {
  fbabgr1555_draw_xline(fb, xmin, xmax, ymin, data);
  fbabgr1555_draw_xline(fb, xmin, xmax, ymax, data);
  fbabgr1555_draw_yline(fb, ymin, ymax, xmin, data);
  fbabgr1555_draw_yline(fb, ymin, ymax, xmax, data);
}

static inline void fbabgr1555_draw_solid(fbabgr1555 *fb, int xmin, int ymin,
                                         int xmax, int ymax, uint16_t data) {
  for (; ymin <= ymax; ymin++) {
    fbabgr1555_draw_xline(fb, xmin, xmax, ymin, data);
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

static inline void fbabgr1555_draw_char(fbabgr1555 *fb, int x, int y,
                                        uint16_t fg, uint16_t bg, int c) {
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
        fbabgr1555_draw_pixel(fb, x + xf, y + yf, fg);
      } else {
        fbabgr1555_draw_pixel(fb, x + xf, y + yf, bg);
      }
    }
  }
}

static inline void fbabgr1555_draw_str(fbabgr1555 *fb, int x, int y,
                                       uint16_t fg, uint16_t bg, char *s) {
  int i = 0;
  while (s[i] != '\0') {
    fbabgr1555_draw_char(fb, x, y, fg, bg, s[i]);
    x += FONT_W;
    i++;
  }
}
