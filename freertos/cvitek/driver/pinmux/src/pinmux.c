#include <stdint.h>
#include <string.h>
#include "hal_pinmux.h"

void pinmux_init(void)
{
// #ifdef FPGA_PORTING
#ifdef FAST_IMAGE_ENABLE
	hal_pinmux_config(PINMUX_I2C2);
	hal_pinmux_config(PINMUX_CAM0);
#endif

// #elif defined(BOARD_WEVB_006A)

// #elif defined(BOARD_WEVB_007A)

// #endif
}
