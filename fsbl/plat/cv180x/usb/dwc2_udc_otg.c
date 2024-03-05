/*
 * drivers/usb/gadget/dwc2_udc_otg.c
 * Designware DWC2 on-chip full/high speed USB OTG 2.0 device controllers
 *
 * Copyright (C) 2008 for Samsung Electronics
 *
 * BSP Support for Samsung's UDC driver
 * available at:
 * git://git.kernel.org/pub/scm/linux/kernel/git/kki_ap/linux-2.6-samsung.git
 *
 * State machine bugfixes:
 * Marek Szyprowski <m.szyprowski@samsung.com>
 *
 * Ported to u-boot:
 * Marek Szyprowski <m.szyprowski@samsung.com>
 * Lukasz Majewski <l.majewski@samsumg.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <list.h>
#include <delay_timer.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"
#include <dwc2_ch9.h>
#include <dwc2_drv_if.h>

#include <byteorder.h>
#include <dwc2_stdtypes.h>
#include <dwc2_errno.h>
#include "dwc2_udc_otg_regs.h"
#include "dwc2_udc_otg_priv.h"
#include <dwc2_udc.h>
#include <dps.h>

/***********************************************************/
#define DRIVER_VERSION "15 March 2009"

static const char ep0name[] = "ep0-control";
static const char ep1name[] = "ep1in-bulk";
static const char ep2name[] = "ep2out-bulk";
static const char ep3name[] = "ep3in-int";

struct dwc2_udc	*the_controller;

static const char driver_name[] = "dwc2-udc";

/* Max packet size*/
/* Local declarations. */
static int dwc2_ep_enable(struct usb_ep *ep,
			 const CH9_UsbEndpointDescriptor *);
static int dwc2_ep_disable(struct usb_ep *ep);
static struct usb_request *dwc2_alloc_request(struct usb_ep *ep);
static void dwc2_free_request(struct usb_ep *ep, struct usb_request *);

static int dwc2_dequeue(struct usb_ep *ep, struct usb_request *);
static int dwc2_fifo_status(struct usb_ep *ep);
static void dwc2_fifo_flush(struct usb_ep *ep);
static void stop_activity(struct dwc2_udc *dev,
			  struct usb_gadget_driver *driver);
static int udc_enable(struct dwc2_udc *dev);
static void dwc2_usbd_init(struct dwc2_udc *dev);
// void udc_reinit(struct dwc2_udc *dev);
static int _dwc2_ep_disable(struct dwc2_ep *ep);

#undef DWC2_LOG
#undef DWC2_DBG

#if defined(DWC2_LOG)

#define DWC2_LOG_ENTRY_NUM	1024

struct dwc2_log_s {
	uint32_t time;
	uint32_t tag;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
	uint32_t param4;
};

static unsigned int log_idx;
static struct dwc2_log_s dwc2_log[DWC2_LOG_ENTRY_NUM];

void dwc2_log_write(uint32_t tag, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4)
{
	//if (log_idx == DWC2_LOG_ENTRY_NUM)
	//	return;

	dwc2_log[log_idx].tag = tag;
	dwc2_log[log_idx].param1 = param1;
	dwc2_log[log_idx].param2 = param2;
	dwc2_log[log_idx].param3 = param3;
	dwc2_log[log_idx].param4 = param4;
	dwc2_log[log_idx].time = get_timer();

	log_idx++;
	log_idx = log_idx % DWC2_LOG_ENTRY_NUM;
}

void set_trigger_cnt(int cnt)
{
	static int test_reset;
	uint32_t *test_ptr = (uint32_t *)0x83000000;

	if (test_reset == cnt)
		*test_ptr = 0xAAA;
	test_reset++;
}

#else

void dwc2_log_write(uint32_t tag, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4)
{
}

void set_trigger_cnt(int cnt)
{
}

#endif

static struct usb_ep_ops dwc2_ep_ops = {
	.enable = dwc2_ep_enable,
	.disable = dwc2_ep_disable,

	.alloc_request = dwc2_alloc_request,
	.free_request = dwc2_free_request,

	.queue = dwc2_queue,
	.dequeue = dwc2_dequeue,

	.set_halt = dwc2_udc_set_halt,
	.fifo_status = dwc2_fifo_status,
	.fifo_flush = dwc2_fifo_flush,
};

#define create_proc_files() do {} while (0)
#define remove_proc_files() do {} while (0)

/***********************************************************/

const char *dwc2_get_ep0_name(void)
{
	return ep0name;
}

struct dwc2_usbotg_reg *reg;

bool dfu_usb_get_reset(void)
{
	return !!(DWC2_UncachedRead32(&reg->gintsts) & INT_RESET);
}

void otg_phy_init(struct dwc2_udc *dev)
{
}
void otg_phy_off(struct dwc2_udc *dev)
{
}

/***********************************************************/

//#include "dwc2_udc_otg_xfer_dma.c"

/***********************************************************/
/*
 *	udc_disable - disable USB device controller
 */
static void udc_disable(struct dwc2_udc *dev)
{
	dwc2dbg_cond(DEBUG_SETUP != 0, "%s: %p\n", __func__, dev);

	dwc2_set_address(dev, 0);

	dev->ep0state = WAIT_FOR_SETUP;
	dev->gadget.speed = CH9_USB_SPEED_UNKNOWN;
	dev->usb_address = 0;

	otg_phy_off(dev);
}

/*
 *	udc_reinit - initialize software state
 */
void udc_reinit(struct dwc2_udc *dev)
{
	unsigned int i;

	dwc2dbg_cond(DEBUG_SETUP != 0, "%s: %p\n", __func__, dev);

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0state = WAIT_FOR_SETUP;

	/* basic endpoint records init */
	for (i = 0; i < DWC2_MAX_ENDPOINTS; i++) {
		struct dwc2_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->desc = 0;
		ep->stopped = 0;
		INIT_LIST_HEAD(&ep->queue);
		ep->pio_irqs = 0;
	}

	/* the rest was statically initialized, and is read-only */
}

#define BYTES2MAXP(x)	(x / 8)
#define MAXP2BYTES(x)	(x * 8)

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static int udc_enable(struct dwc2_udc *dev)
{
	dwc2dbg_cond(DEBUG_SETUP != 0, "%s: %p\n", __func__, dev);

	otg_phy_init(dev);
	dwc2_usbd_init(dev);
	dwc2_reconfig_usbd(dev, 0);

	dwc2dbg_cond(DEBUG_SETUP != 0,
		   "DWC2 USB 2.0 OTG Controller Core Initialized : 0x%x\n",
		    DWC2_UncachedRead32(&reg->gintmsk));

	dev->gadget.speed = CH9_USB_SPEED_UNKNOWN;

	return 0;
}

/*
 * Register entry point for the peripheral controller driver.
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct dwc2_udc *dev = the_controller;
	int retval = 0;

	dwc2dbg_cond(DEBUG_SETUP != 0, "%s: %s\n", __func__, "no name");

	if (!driver
	    || (driver->speed != CH9_USB_SPEED_FULL
		&& driver->speed != CH9_USB_SPEED_HIGH)
	    || !driver->bind || !driver->disconnect || !driver->setup || !driver->req_mem_alloc
	    || !driver->req_mem_free)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;

	/* first hook up the driver ... */
	dev->driver = driver;

	if (retval) { /* TODO */
		tf_printf("target device_add failed, error %d\n", retval);
		return retval;
	}

	retval = driver->bind(&dev->gadget);
	if (retval) {
		dwc2dbg_cond(DEBUG_SETUP != 0,
			   "%s: bind to driver --> error %d\n",
			    dev->gadget.name, retval);
		dev->driver = 0;
		return retval;
	}

#if defined(USB_IRQ_MODE)
	enable_irq(USB_IRQS_0);
#endif
	dwc2dbg_cond(DEBUG_SETUP != 0,
		   "Registered gadget driver %s\n", dev->gadget.name);
	udc_enable(dev);

	return 0;
}

/*
 * Unregister entry point for the peripheral controller driver.
 */
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct dwc2_udc *dev = the_controller;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	dev->driver = 0;
	stop_activity(dev, driver);

	driver->unbind(&dev->gadget);

#if defined(USB_IRQ_MODE)
	disable_irq(USB_IRQS_0);
#endif

	udc_disable(dev);
	return 0;
}

/*
 *	dwc2_done - retire a request; caller blocked irqs
 */
void dwc2_done(struct dwc2_ep *ep, struct dwc2_request *req, int status)
{
	unsigned int stopped = ep->stopped;

	dwc2dbg("%s: %s %p, req = %p, stopped = %d\n",
	      __func__, ep->ep.name, ep, &req->req, stopped);

	list_del_init(&req->queue);

	if (req->req.status == -EINPROGRESS)
		req->req.status = status;
	else
		status = req->req.status;

	if (status && status != -ESHUTDOWN) {
		dwc2dbg("complete %s req %p stat %d len %u/%u\n",
		      ep->ep.name, &req->req, status,
		      req->req.actual, req->req.length);
	}

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;

#ifdef DWC2_DBG
	tf_printf("calling complete callback\n");
	{
		int i, len = req->req.length;

		tf_printf("pkt[%d] = ", req->req.length);
		if (len > 64)
			len = 64;
		for (i = 0; i < len; i++) {
			tf_printf("%x", ((uint8_t *)req->req.buf)[i]);
			if ((i & 7) == 7)
				tf_printf(" ");
		}
		tf_printf("\n");
	}
#endif
	req->req.complete(&ep->ep, &req->req);

	dwc2dbg("callback completed\n");

	ep->stopped = stopped;
}

/*
 *	dwc2_nuke - dequeue ALL requests
 */
void dwc2_nuke(struct dwc2_ep *ep, int status)
{
	struct dwc2_request *req;

	dwc2dbg("%s: %s %p\n", __func__, ep->ep.name, ep);

	/* called with irqs blocked */
	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct dwc2_request, queue);
		dwc2_done(ep, req, status);
	}
}

static void stop_activity(struct dwc2_udc *dev,
			  struct usb_gadget_driver *driver)
{
	int i;

	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == CH9_USB_SPEED_UNKNOWN)
		driver = 0;
	dev->gadget.speed = CH9_USB_SPEED_UNKNOWN;

	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < DWC2_MAX_ENDPOINTS; i++) {
		struct dwc2_ep *ep = &dev->ep[i];

		ep->stopped = 1;
		dwc2_nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	if (driver) {
		driver->disconnect(&dev->gadget);
	}

	/* re-init driver-visible data structures */
	udc_reinit(dev);
}

static void dwc2_hsotg_init_fifo(struct dwc2_udc *dev)
{
	uint32_t rx_fifo_sz, tx_fifo_sz, np_tx_fifo_sz;
	int timeout, i;
	uint32_t val;

	if (dwc2_hsotg_wait_bit_set(&reg->grstctl, AHBIDLE, 10000))
		tf_printf("%s:  HANG! AHB Idle GRSCTL\n", __func__);

	/* setup fifos */
	rx_fifo_sz = RX_FIFO_SIZE;
	np_tx_fifo_sz = NPTX_FIFO_SIZE;
	tx_fifo_sz = PTX_FIFO_SIZE;

	/* Set Rx FIFO Size (in 32-bit words) */
	DWC2_UncachedWrite32(rx_fifo_sz, &reg->grxfsiz);

	/* Set Non Periodic Tx FIFO Size (TXFIFO[0]) */
	DWC2_UncachedWrite32((np_tx_fifo_sz << 16) | rx_fifo_sz,
	       &reg->gnptxfsiz);

	for (i = 1; i < DWC2_MAX_ENDPOINTS; i++)
		DWC2_UncachedWrite32((rx_fifo_sz + np_tx_fifo_sz + tx_fifo_sz*(i-1)) |
			tx_fifo_sz << 16, &reg->dieptxf[i-1]);

	/* Flush all the FIFO's */
	DWC2_UncachedWrite32(TX_FIFO_FLUSH_ALL |
			TX_FIFO_FLUSH |
			RX_FIFO_FLUSH,
			&reg->grstctl);
	timeout = 100;
	while (1) {
		val = DWC2_UncachedRead32(&reg->grstctl);

		if ((val & (TX_FIFO_FLUSH | RX_FIFO_FLUSH)) == 0)
			break;

		if (--timeout == 0) {
			tf_printf("%s: timeout flushing fifos (grstctl = 0x%x)\n",
					__func__, val);
			break;
		}
		udelay(1);
	}
}

static void dwc2_hsotg_txfifo_flush(struct dwc2_udc *dev, unsigned int idx)
{
	int timeout;
	int val;

	if (dwc2_hsotg_wait_bit_set(&reg->grstctl, AHBIDLE, 10000))
		tf_printf("%s:  HANG! AHB Idle GRSCTL\n", __func__);

	DWC2_UncachedWrite32(TX_FIFO_NUMBER(idx) | TX_FIFO_FLUSH, &reg->grstctl);

	/* wait until the fifo is flushed */
	timeout = 100;

	while (1) {
		val = DWC2_UncachedRead32(&reg->grstctl);

		if ((val & (TX_FIFO_FLUSH)) == 0)
			break;

		if (--timeout == 0) {
			tf_printf("%s: timeout flushing fifo (GRSTCTL=%08x)\n",
				__func__, val);
			break;
		}

		udelay(1);
	}

	/* Wait for at least 3 PHY Clocks */
	udelay(1);
}

static void kill_all_requests(struct dwc2_udc *dev, struct dwc2_ep *ep, int result)
{
	uint32_t ep_num = ep_index(ep);
	uint32_t size_max = (ep_num == 0) ? (NPTX_FIFO_SIZE*4) : (PTX_FIFO_SIZE*4);
	uint32_t size;

	dwc2dbg("%s: %p\n", __func__, ep);

	/* make sure it's actually queued on this endpoint */
	dwc2_nuke(ep, result);

	size = (DWC2_UncachedRead32(&reg->in_endp[ep_num].dtxfsts) & 0xFFFF) * 4;
	if (size < size_max)
		dwc2_hsotg_txfifo_flush(dev, ep->fifo_num);

}

void dwc2_disconnect(struct dwc2_udc *dev)
{
	int i;

	if (!dev->connected)
		return;

	dev->connected = 0;

	for (i = 1; i < DWC2_MAX_ENDPOINTS; i++) {
		struct dwc2_ep *ep = &dev->ep[i];

		if (ep->ep.name) {
			kill_all_requests(dev, ep, -ESHUTDOWN);
		}
	}

	/* HACK to let gadget detect disconnected state */
	if (dev->driver->disconnect) {
		dev->driver->disconnect(&dev->gadget);
	}
}

void dwc2_reconfig_usbd(struct dwc2_udc *dev, int is_usb_reset)
{
	/* 2. Soft-reset OTG Core and then unreset again. */
	unsigned int val;
	uint32_t dflt_gusbcfg;
	struct dwc2_plat_otg_data *pdata = (struct dwc2_plat_otg_data *)dev->pdata;
	struct dwc2_ep *ep = &dev->ep[0];

	dwc2dbg("Reseting OTG controller\n");

	kill_all_requests(dev, ep, -ECONNRESET);
	udc_reinit(dev);
	if (!is_usb_reset) {
		uint32_t greset;
		int count = 0;
		uint32_t snpsid = DWC2_UncachedRead32(&reg->gsnpsid) & DWC2_CORE_REV_MASK;

		/* check snpsid */
		if (snpsid < (DWC2_CORE_REV_4_20a & DWC2_CORE_REV_MASK)) {
			/* Core Soft Reset */
			DWC2_UncachedWrite32(CORE_SOFT_RESET, &reg->grstctl);
			do {
				udelay(1);
				greset = DWC2_UncachedRead32(&reg->grstctl);
				if (++count > 50) {
					tf_printf("%s() HANG! Soft Reset GRSTCTL=%0x\n", __func__, greset);
					return;
				}
			} while (greset & CORE_SOFT_RESET);
		} else {
			/* Core Soft Reset */
			DWC2_UncachedWrite32(CORE_SOFT_RESET, &reg->grstctl);
			do {
				udelay(1);
				greset = DWC2_UncachedRead32(&reg->grstctl);
				if (++count > 50) {
					tf_printf("%s() HANG! Soft 4.2 Reset GRSTCTL=%0x\n",
						 __func__, greset);
					return;
				}
			} while (!(greset & CSFTRST_DONE));
			greset = DWC2_UncachedRead32(&reg->grstctl);
			greset &= ~CORE_SOFT_RESET;
			greset |= CSFTRST_DONE;
			DWC2_UncachedWrite32(greset, &reg->grstctl);
		}

		/* Wait for AHB master IDLE state */
		count = 0;
		do {
			udelay(1);
			greset = DWC2_UncachedRead32(&reg->grstctl);
			if (++count > 50) {
				tf_printf("%s() HANG! AHB Idle GRSTCTL=%0x\n",
					 __func__, greset);
				return;
			}
		} while (!(greset & AHB_MASTER_IDLE));
	} else {
		int i;

		for (i = 1; i < DWC2_MAX_ENDPOINTS; i++) {
			struct dwc2_ep *ep = &dev->ep[i];

			if (ep->ep.name)
				_dwc2_ep_disable(ep);
		}

	}

	dflt_gusbcfg =
		1<<30		/* ForceDevMode.*/
		|1<<19		/* 1'b1: PHY does not power down internal clock.*/
		|0<<15		/* PHY Low Power Clock sel*/
		|0<<14		/* Non-Periodic TxFIFO Rewind Enable*/
		|0x5<<10	/* Turnaround time*/
		|0<<9 | 0<<8	/* [0:HNP disable,1:HNP enable][ 0:SRP disable*/
				/* 1:SRP enable] H1= 1,1*/
		|0<<7		/* Ulpi DDR sel*/
		|0<<6		/* 0: high speed utmi+, 1: full speed serial*/
		|0<<4		/* 0: utmi+, 1:ulpi*/
		|1<<3		/* phy i/f  0:8bit, 1:16bit*/
		|0x7<<0;	/* HS/FS Timeout**/

	if (pdata->usb_gusbcfg)
		dflt_gusbcfg = pdata->usb_gusbcfg;

	DWC2_UncachedWrite32(dflt_gusbcfg, &reg->gusbcfg);

	dwc2_hsotg_init_fifo(dev);

	if (!is_usb_reset) {
		/* Put the OTG device core in the disconnected state.*/
		val = DWC2_UncachedRead32(&reg->dctl);
		val |= SOFT_DISCONNECT;
		DWC2_UncachedWrite32(val, &reg->dctl);
	}

	/* Configure OTG Core to initial settings of device mode.*/
	/* [1: full speed(30Mhz) 0:high speed]*/
	DWC2_UncachedWrite32(EP_MISS_CNT(1) | DEV_SPEED_HIGH_SPEED_20, &reg->dcfg);

	/* Clear any pending OTG interrupts */
	DWC2_UncachedWrite32(0xffffffff, &reg->gotgint);

	/* Clear any pending interrupts */
	DWC2_UncachedWrite32(0xffffffff, &reg->gintsts);

	/* Unmask the core interrupts*/
	DWC2_UncachedWrite32(GINTMSK_INIT, &reg->gintmsk);

	/* Initialize ahbcfg.*/
	DWC2_UncachedWrite32(GAHBCFG_INIT, &reg->gahbcfg);

	/* Unmask device IN EP common interrupts*/
	DWC2_UncachedWrite32(DIEPMSK_INIT, &reg->diepmsk);

	/* Unmask device OUT EP common interrupts*/
	DWC2_UncachedWrite32(DOEPMSK_INIT, &reg->doepmsk);

	/* Unmask EPO interrupts*/
	DWC2_UncachedWrite32(((1 << EP0_CON) << DAINT_OUT_BIT)
	       | (1 << EP0_CON), &reg->daintmsk);

	if (!is_usb_reset) {
		val = DWC2_UncachedRead32(&reg->dctl);
		val |= PWRONPRGDONE;
		DWC2_UncachedWrite32(val, &reg->dctl);
		udelay(10);  /* see openiboot */
		val = DWC2_UncachedRead32(&reg->dctl);
		val &= ~PWRONPRGDONE;
		DWC2_UncachedWrite32(val, &reg->dctl);
	}

	/* prepare the setup */
	dwc2_udc_pre_setup(dev);
	/* enable, but don't activate EP0in */
	DWC2_UncachedWrite32(DEPCTL_USBACTEP, &reg->in_endp[0].diepctl);

	/* clear global NAKs */
	val = CGOUTNAK | CGNPINNAK;
	if (!is_usb_reset)
		val |= SOFT_DISCONNECT;
	val |= DWC2_UncachedRead32(&reg->dctl);
	DWC2_UncachedWrite32(val, &reg->dctl);

	/* must be at-least 3ms to allow bus to see disconnect */
	mdelay(3);
}
#define USB20_PHY_WRAP (TOP_BASE + 0x00006000)

#define ATF_STATE_USB_UTMI_RST_DONE 0xC000300B
static void dwc2_usbd_init(struct dwc2_udc *dev)
{
	unsigned int uTemp;
	uint32_t dflt_gusbcfg;
	struct dwc2_plat_otg_data *pdata = (struct dwc2_plat_otg_data *)dev->pdata;
	uint32_t *usb_phy_reg = (uint32_t *)(USB20_PHY_WRAP + 0x14);
	uint32_t reg_data;

	dwc2dbg("Init OTG controller\n");

	/* Unmask subset of endpoint interrupts */
	DWC2_UncachedWrite32(DOEPMSK_INIT, &reg->doepmsk);
	DWC2_UncachedWrite32(DIEPMSK_INIT, &reg->diepmsk);
	DWC2_UncachedWrite32(0, &reg->daintmsk);

	/* Be in disconnected state until gadget is registered */
	uTemp = DWC2_UncachedRead32(&reg->dctl);
	uTemp |= SOFT_DISCONNECT;
	DWC2_UncachedWrite32(uTemp, &reg->dctl);

	/* setup fifo*/
	dwc2_hsotg_init_fifo(dev);

	dflt_gusbcfg =
		1<<30		/* ForceDevMode.*/
		|1<<19		/* 1'b1: PHY does not power down internal clock.*/
		|0<<15		/* PHY Low Power Clock sel*/
		|0<<14		/* Non-Periodic TxFIFO Rewind Enable*/
		|0x5<<10	/* Turnaround time*/
		|0<<9 | 0<<8	/* [0:HNP disable,1:HNP enable][ 0:SRP disable*/
				/* 1:SRP enable] H1= 1,1*/
		|0<<7		/* Ulpi DDR sel*/
		|0<<6		/* 0: high speed utmi+, 1: full speed serial*/
		|0<<4		/* 0: utmi+, 1:ulpi*/
		|0<<3		/* phy i/f  0:8bit, 1:16bit*/
		|0x7<<0;	/* HS/FS Timeout**/

	if (pdata->usb_gusbcfg)
		dflt_gusbcfg = pdata->usb_gusbcfg;

	reg_data = 0;
	if (get_sw_info()->usb_utmi_rst) {
		reg_data = DWC2_UncachedRead32(usb_phy_reg);
		DWC2_UncachedWrite32(0x18B, usb_phy_reg);
		ATF_STATE = ATF_STATE_USB_UTMI_RST_DONE;
	}

	DWC2_UncachedWrite32(dflt_gusbcfg, &reg->gusbcfg);

	if (get_sw_info()->usb_utmi_rst) {
		DWC2_UncachedWrite32(reg_data, usb_phy_reg);
		udelay(100);
	}

	/* Initialize OTG Link Core.*/
	DWC2_UncachedWrite32(GAHBCFG_INIT, &reg->gahbcfg);
}

static int dwc2_ep_enable(struct usb_ep *_ep,
			 const CH9_UsbEndpointDescriptor *desc)
{
	struct dwc2_ep *ep;
	struct dwc2_udc *dev;

	dwc2dbg("%s: %p\n", __func__, _ep);

	ep = container_of(_ep, struct dwc2_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != CH9_USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress
	    || ep_maxpacket(ep) <
	    le16ToCpu(desc->wMaxPacketSize)) {

		dwc2dbg("%s: bad ep or descriptor\n", __func__);
		return -EINVAL;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if (ep->bmAttributes != desc->bmAttributes
	    && ep->bmAttributes != CH9_USB_EP_BULK
	    && desc->bmAttributes != CH9_USB_EP_INTERRUPT) {

		dwc2dbg("%s: %s type mismatch\n", __func__, _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == CH9_USB_EP_BULK &&
	     le16ToCpu(desc->wMaxPacketSize) >
	     ep_maxpacket(ep)) || !desc->wMaxPacketSize) {

		dwc2dbg("%s: bad %s maxpacket\n", __func__, _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == CH9_USB_SPEED_UNKNOWN) {

		dwc2dbg("%s: bogus device state\n", __func__);
		return -ESHUTDOWN;
	}

	ep->stopped = 0;
	ep->desc = ep->ep.desc = desc;
	ep->pio_irqs = 0;
	ep->ep.maxpacket = le16ToCpu(desc->wMaxPacketSize);

	/* Reset halt state */
	dwc2_udc_set_nak(ep);
	dwc2_udc_set_halt(_ep, 0);

	dwc2_udc_ep_activate(ep);

	dwc2dbg("%s: enabled %s, stopped = %d, maxpacket = %d\n",
	      __func__, _ep->name, ep->stopped, ep->ep.maxpacket);

	return 0;
}

void dwc2_hsotg_set_bit(uint32_t *reg, uint32_t val)
{
	DWC2_UncachedWrite32(DWC2_UncachedRead32(reg) | val, reg);
}

void dwc2_hsotg_clear_bit(uint32_t *reg, uint32_t val)
{
	DWC2_UncachedWrite32(DWC2_UncachedRead32(reg) & ~val, reg);
}

int dwc2_hsotg_wait_bit_set(uint32_t *reg, uint32_t bit, uint32_t timeout)
{
	uint32_t i;

	for (i = 0; i < timeout; i++) {
		if (DWC2_UncachedRead32(reg) & bit)
			return 0;
		udelay(1);
	}

	return -ETIMEDOUT;
}

static void dwc2_ep_stop_xfer(struct dwc2_udc *dev, struct dwc2_ep *ep)
{
	uint32_t ep_num = ep_index(ep);
	uint32_t *epctrl_reg, *epint_reg;

	epctrl_reg = ep_is_in(ep) ? &reg->in_endp[ep_num].diepctl :
			&reg->out_endp[ep_num].doepctl;
	epint_reg = ep_is_in(ep) ? &reg->in_endp[ep_num].diepint :
			&reg->out_endp[ep_num].doepint;
	if (ep_is_in(ep)) {
		dwc2_hsotg_set_bit(epctrl_reg, DEPCTL_SNAK);
		/* Wait for Nak effect */
		if (dwc2_hsotg_wait_bit_set(epint_reg, INEPNAKEFF, 100))
			tf_printf("%s: timeout DIEPINT.NAKEFF\n", __func__);
	} else {
		if (!(DWC2_UncachedRead32(&reg->gintsts) & INT_GOUTNakEff)) {
			dwc2_hsotg_set_bit(&reg->dctl, SGOUTNAK);
		}
		/* Wait for global nak to take effect */
		if (dwc2_hsotg_wait_bit_set(&reg->gintsts, INT_GOUTNakEff, 100)) {
			tf_printf("%s: timeout GINTSTS.GOUTNAKEFF\n", __func__);
		}
	}

	/* disable ep */
	dwc2_hsotg_set_bit(epctrl_reg, DEPCTL_SNAK | DEPCTL_EPDIS);
	/* Wait for ep to be disabled */
	if (dwc2_hsotg_wait_bit_set(epint_reg, EPDISBLD, 100)) {
		tf_printf("%s: timeout DOEPCTL.EPDisable\n", __func__);
	}
	/* Clear EPDISBLD interrupt */
	dwc2_hsotg_set_bit(epint_reg, EPDISBLD);
	if (ep_is_in(ep)) {
		dwc2_hsotg_txfifo_flush(dev, ep->fifo_num);
	} else {
		dwc2_hsotg_set_bit(&reg->dctl, CGOUTNAK);
	}
}

static int _dwc2_ep_disable(struct dwc2_ep *ep)
{
	uint32_t ep_num = ep_index(ep);
	uint32_t *epctrl_reg;
	uint32_t ctrl;

	if (ep == &the_controller->ep[0]) {
		tf_printf("%s: call for ep0-out\n", __func__);
		return -EINVAL;
	}

	epctrl_reg = ep_is_in(ep) ? &reg->in_endp[ep_num].diepctl :
			&reg->out_endp[ep_num].doepctl;
	ctrl = DWC2_UncachedRead32(epctrl_reg);
	if (ctrl & DEPCTL_EPENA)
		dwc2_ep_stop_xfer(the_controller, ep);
	ctrl &= ~DEPCTL_EPENA;
	ctrl &= ~DEPCTL_USBACTEP;
	ctrl |= DEPCTL_SNAK;
	DWC2_UncachedWrite32(ctrl, epctrl_reg);
	/* Nuke all pending requests */
	kill_all_requests(the_controller, ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;

	return 0;
}

/*
 * Disable EP
 */
static int dwc2_ep_disable(struct usb_ep *_ep)
{
	struct dwc2_ep *ep;

	dwc2dbg("%s: %p\n", __func__, _ep);

	ep = container_of(_ep, struct dwc2_ep, ep);

	if (!_ep || !ep->desc) {
		dwc2dbg("%s: %s not enabled\n", __func__,
		      _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	_dwc2_ep_disable(ep);

	dwc2dbg("%s: disabled %s\n", __func__, _ep->name);

	return 0;
}

static struct usb_request *dwc2_alloc_request(struct usb_ep *ep)
{
	struct dwc2_udc *dev = the_controller;
	struct usb_gadget_driver *driver = dev->driver;
	struct dwc2_request *req;

	dwc2dbg("%s: %s %p\n", __func__, ep->name, ep);

	req = driver->req_mem_alloc(&dev->gadget, sizeof(*req));
	if (!req)
		return 0;

	memset(req, 0, sizeof(*req));
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void dwc2_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct dwc2_udc *dev = the_controller;
	struct usb_gadget_driver *driver = dev->driver;
	struct dwc2_request *req;

	dwc2dbg("%s: %p\n", __func__, ep);

	req = container_of(_req, struct dwc2_request, req);
	if (!list_empty(&req->queue))
		tf_printf("warning! free unfinished request!\n");
	driver->req_mem_free(&dev->gadget, req);
}

/* dequeue JUST ONE request */
static int dwc2_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct dwc2_ep *ep;
	struct dwc2_request *req;

	dwc2dbg("%s: %p\n", __func__, _ep);

	ep = container_of(_ep, struct dwc2_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		return -EINVAL;
	}

	dwc2_done(ep, req, -ECONNRESET);

	return 0;
}

/*
 * Return bytes in EP FIFO
 */
static int dwc2_fifo_status(struct usb_ep *_ep)
{
	int count = 0;
	struct dwc2_ep *ep;

	ep = container_of(_ep, struct dwc2_ep, ep);
	if (!_ep) {
		dwc2dbg("%s: bad ep\n", __func__);
		return -ENODEV;
	}

	dwc2dbg("%s: %d\n", __func__, ep_index(ep));

	/* LPD can't report unclaimed bytes from IN fifos */
	if (ep_is_in(ep))
		return -EOPNOTSUPP;

	return count;
}

/*
 * Flush EP FIFO
 */
static void dwc2_fifo_flush(struct usb_ep *_ep)
{
	struct dwc2_ep *ep;

	ep = container_of(_ep, struct dwc2_ep, ep);
	if (!_ep || (!ep->desc && ep->ep.name != ep0name)) {
		dwc2dbg("%s: bad ep\n", __func__);
		return;
	}

	dwc2dbg("%s: %d\n", __func__, ep_index(ep));
}

static int pullup(struct usb_gadget *gadget, int is_on)
{
	unsigned int uTemp = DWC2_UncachedRead32(&reg->dctl);

	if (!is_on) {
		uTemp |= SOFT_DISCONNECT;
		DWC2_UncachedWrite32(uTemp, &reg->dctl);
	} else {
		uTemp &= ~SOFT_DISCONNECT;
		DWC2_UncachedWrite32(uTemp, &reg->dctl);
	}

	return 0;
}

static int wakeup(struct usb_gadget *gadget)
{
	dwc2_hsotg_set_bit(&reg->dctl, RMTWKUPSIG);
	mdelay(10);
	dwc2_hsotg_clear_bit(&reg->dctl, RMTWKUPSIG);

	return 0;
}

static const struct usb_gadget_ops dwc2_udc_ops = {
	/* current versions must always be self-powered */
	.pullup = pullup,
	.wakeup = wakeup,
};

uint8_t dwc2_phy_to_log_ep(uint8_t phy_num, uint8_t dir)
{
	return (phy_num) ? ((phy_num << 1) - (!!dir)) : 0;
}

/*
 *	probe - binds to the platform device
 */

int dwc2_udc_probe(struct dwc2_plat_otg_data *pdata)
{
	struct dwc2_udc *dev;
	int retval = 0;

	dwc2dbg("%s: %p\n", __func__, pdata);

	if (pdata->size < sizeof(*dev)) {
		tf_printf("size for handler is too samll (%ld, %d)\n", sizeof(*dev), pdata->size);
		return -1;
	}

	dwc2_log_write(0xbeefbeef, 0, 0, 0, 0);
	dev = (struct dwc2_udc *)pdata->handler;
	memset(dev, 0, sizeof(*dev));

	dev->pdata = (void *)pdata;

	reg = (struct dwc2_usbotg_reg *)pdata->regs_otg;

	/* gadget init */
	dev->usb_address = 0;
	dev->gadget.ops = &dwc2_udc_ops;
	dev->gadget.ep0 = &dev->ep[0].ep;
	dev->gadget.name = driver_name;
	dev->gadget.is_dualspeed = 1;
	dev->gadget.is_otg = 0;
	dev->gadget.is_a_peripheral = 0;
	dev->gadget.b_hnp_enable = 0;
	dev->gadget.a_hnp_support = 0;
	dev->gadget.a_alt_hnp_support = 0;
	dev->gadget.max_speed = CH9_USB_SPEED_HIGH;
	/* eps init */
	dev->ep[0].ep.name = ep0name;
	dev->ep[0].ep.ops = &dwc2_ep_ops;
	dev->ep[0].ep.maxpacket = EP0_FIFO_SIZE;
	dev->ep[0].dev = dev;
	dev->ep[0].bEndpointAddress = 0;
	dev->ep[0].bmAttributes = 0;
	dev->ep[0].ep_type = ep_control;
	dev->ep[1].ep.name = ep1name;
	dev->ep[1].ep.ops = &dwc2_ep_ops;
	dev->ep[1].ep.maxpacket = EP_FIFO_SIZE;
	dev->ep[1].dev = dev;
	dev->ep[1].bEndpointAddress = USB_DIR_IN | 1;
	dev->ep[1].bmAttributes = CH9_USB_EP_BULK;
	dev->ep[1].ep_type = ep_bulk_out;
	dev->ep[1].fifo_num = 1;
	dev->ep[2].ep.name = ep2name;
	dev->ep[2].ep.ops = &dwc2_ep_ops;
	dev->ep[2].ep.maxpacket = EP_FIFO_SIZE;
	dev->ep[2].dev = dev;
	dev->ep[2].bEndpointAddress = USB_DIR_OUT | 1;
	dev->ep[2].bmAttributes = CH9_USB_EP_BULK;
	dev->ep[2].ep_type = ep_bulk_in;
	dev->ep[2].fifo_num = 1;
	dev->ep[3].ep.name = ep3name;
	dev->ep[3].ep.ops = &dwc2_ep_ops;
	dev->ep[3].ep.maxpacket = EP_FIFO_SIZE;
	dev->ep[3].dev = dev;
	dev->ep[3].bEndpointAddress = USB_DIR_IN | 2;
	dev->ep[3].bmAttributes = CH9_USB_EP_INTERRUPT;
	dev->ep[3].ep_type = ep_interrupt;
	dev->ep[3].fifo_num = 2;

	the_controller = dev;

	dev->usb_ctrl = (CH9_UsbSetup *)pdata->ctrl_req;
	if (!dev->usb_ctrl) {
		tf_printf("No memory available for UDC!\n");
		return -ENOMEM;
	}

	dev->usb_ctrl_dma_addr = (dma_addr_t)((uintptr_t)dev->usb_ctrl);
	dev->reg = reg;

	udc_reinit(dev);

	return retval;
}

int usb_gadget_handle_interrupts(int index)
{
	uint32_t intr_status = DWC2_UncachedRead32(&reg->gintsts);
	uint32_t gintmsk = DWC2_UncachedRead32(&reg->gintmsk);

#ifdef DWC2_DBG
{
	static uint32_t print_cnt = 0xFFFF;

	if (print_cnt) {
		print_cnt--;
	} else {
		tf_printf("still alive\n");
		print_cnt = 0xFFFF;
	}
}
#endif
	if (intr_status & gintmsk)
		return dwc2_udc_irq(1, (void *)the_controller);
	return 0;
}
