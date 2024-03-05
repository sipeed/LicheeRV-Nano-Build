#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include <linux/cvi_comm_video.h>
#include <linux/dwa_uapi.h>

#include "ldc.h"
#include "dwa_debug.h"
#include "dwa_platform.h"
#include "cvi_vip_dwa.h"
#include "cvi_vip_gdc_proc.h"
#include "ldc_test.h"

irqreturn_t dwa_isr(int irq, void *_dev)
{
	u8 intr_status = ldc_intr_status();

	if (unlikely(ldc_test_enabled)) {
		ldc_test_irq_handler(1);
		return IRQ_HANDLED;
	}

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "status(0x%x)\n", intr_status);
	ldc_intr_clr(intr_status);
	dwa_irq_handler(intr_status, _dev);

	return IRQ_HANDLED;
}
