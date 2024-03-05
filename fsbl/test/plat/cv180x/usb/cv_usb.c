// SPDX-License-Identifier: BSD-3-Clause
/**********************************************************************
 * main.c
 *
 * USB Core Driver
 * main component function
 ***********************************************************************/

#include <stdint.h>
#include <debug.h>
#include "platform.h"
#include "cv_usb.h"
#include "mmio.h"
#include "platform.h"
#include "delay_timer.h"

uint16_t cv_usb_vid = ID_VENDOR; // Cvitek

int usb_polling(void *buf, uint32_t offset, uint32_t size)
{
	int r;

	cv_usb_vid = get_sw_info()->usb_vid;
	INFO("USBVID/%x.\n", cv_usb_vid);

	r = AcmApp(buf, offset, size);

	return r;
}
