// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 bitmain
 */

/**********************************************************************
 *
 * usb_tty.c
 * ACM class application.
 *
 ***********************************************************************/
#include <stdlib.h>
#include <malloc.h>
//#include <memalign.h>
#include <crc.h>
#include <watchdog.h>
#include "include/debug.h"
#include "include/cvi_usb.h"
#include <mmio.h>
#include <asm/io.h>
#include "include/platform_def.h"
#include "include/usb_tty.h"
#include "include/system_common.h"
#include "include/cvi_ch9.h"
#include "include/cvi_drv_if.h"
#include "include/cvi_udc_otg_regs.h"
#include "include/cvi_udc_otg_priv.h"
#include "include/cvi_udc.h"
#include "include/cvi_errno.h"
#include "include/cvi_private.h"
#include "cvi_reboot.h"
#include "cvi_efuse.h"
#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include "cvi_update.h"
#include "include/dps.h"

#ifdef BUILD_ATF
extern u16 cvi_usb_vid;
#else
u16 cvi_usb_vid = 0x3346;
#endif

u16 cvi_usb_pid;

static void bulk_out_cmpl_main(struct usb_ep *ep, struct usb_request *req);
static void sram_out_req_s2_d(u64 addr, u32 size);

#define GLOBAL_MEM_START_ADDR 0x80000000
#define USB_PHY_DETECTION

/* cvi USB driver object */
static struct cvi_drv_obj drv_obj = {
	.plat = {
		.regs_otg = USB_BASE, // address where USB core is mapped
		.usb_gusbcfg    = 0x40081408,
		.rx_fifo_sz     = 512,
		.np_tx_fifo_sz  = 512,
		.tx_fifo_sz     = 512,
	},
};

/* variable declare */
static u8 *bulk_buf, *cmd_buf, *ep0_buff;
static struct usb_ep *ep_in, *ep_out, *ep_in_notify;
static struct usb_request *bulk_in_req, *bulk_out_req, *ep0_req, *int_in_req;
static u8 config_value;
static u8 config_break;
static u8 acm_config_value;
static u8 current_speed = CH9_USB_SPEED_UNKNOWN;
static u8 mem_alloc_cnt;
static u32 transfer_size;
static u8 flag_enter_dl;
struct f_acm *acm;

static u8 *bulk_buf_alloc;
static u8 *cmd_buf_alloc;
static u8 *cb0_buf;
static u8 *cb1_buf;
static u8 *cb2_buf;
static u8 *ep0_buff_alloc;
static u8 *rsp_buf;
static u8 *acm_buf;
static u8 *setup_buf;
static u8 *handler;

/* string will be filled then in initializing section */
static char vendor_desc[sizeof(USB_MANUFACTURER_STRING) * 2 + 2];
static char product_desc[sizeof(USB_PRODUCT_STRING) * 2 + 2];
static char serial_desc[sizeof(USB_SERIAL_NUMBER_STRING) * 2 + 2];

typedef void func(void);

static void init_param(void)
{
	bulk_buf = NULL;
	cmd_buf = NULL;
	ep0_buff = NULL;
	ep_in = NULL;
	ep_out = NULL;
	ep_in_notify = NULL;
	bulk_in_req = NULL;
	bulk_out_req = NULL;
	ep0_req = NULL;
	int_in_req = NULL;
	config_value = 0;
	config_break = 0;
	acm_config_value = 0;
	current_speed = CH9_USB_SPEED_UNKNOWN;
	mem_alloc_cnt = 0;
	transfer_size = 0;
	acm = NULL;
}

struct usb_msg_header {
	u8 token;
	u8 len_hi;
	u8 len_low;
	u8 addr4;
	u8 addr3;
	u8 addr2;
	u8 addr1;
	u8 addr0;
} __packed;

struct usb_msg {
	struct usb_msg_header header;
} __packed;

struct usb_msg_s2d {
	struct usb_msg_header header;
	size_t size;
} __packed;

struct usb_msg_d2s {
	struct usb_msg_header header;
	size_t size;
} __packed;

struct usb_rsp {
	u8 no_use0;
	u8 no_use1;
	u8 crc16_hi;
	u8 crc16_low;
	u8 no_use3;
	u8 no_use4;
	u8 token;
	u8 ack_index;
	u8 reserved[RSP_SIZE - 8];
} __packed;

struct sram_info {
	u64 sram_dest;
	u32 total_len;
	u8 reserved[4];
} packed;

static struct sram_info sram_info;

static char *_allow_cmds[] = { "setenv", "saveenv", "efusew", "efuser" };

#if USB_RW_EFUSE // Mark_to_do
enum CVI_EFUSE_LOCK_WRITE_E {
	CVI_EFUSE_LOCK_WRITE_HASH0_PUBLIC = CVI_EFUSE_OTHERS + 1,
	CVI_EFUSE_LOCK_WRITE_LOADER_EK,
	CVI_EFUSE_LOCK_WRITE_DEVICE_EK,
	CVI_EFUSE_LOCK_WRITE_LAST
};

static char *_allow_areas[] = { "USER",	     "DEVICE_ID", "HASH0_PUBLIC",
				"LOADER_EK", "DEVICE_EK", "AREA_LAST" };
static char *_allow_wl_areas[] = { "LOCK_HASH0_PUBLIC",
				   "LOCK_LOADER_EK",
				   "LOCK_DEVICE_EK",
				   "LOCK_LAST",
				   "SECUREBOOT",
				   "OTHERS",
				   "LOCK_WRITE_HASH0_PUBLIC",
				   "LOCK_WRITE_LOADER_EK",
				   "LOCK_WRITE_DEVICE_EK" };
#endif // USB_RW_EFUSE

void print_buf_addr(void)
{
	INFO("bulk_buf_alloc: %p\n", bulk_buf_alloc);
	INFO("cmd_buf_alloc: %p\n", cmd_buf_alloc);
	INFO("ep0_buff_alloc: %p\n", ep0_buff_alloc);
	INFO("setup_buf: %p\n", setup_buf);
	INFO("handler: %p\n", handler);
	INFO("cb0_buf: %p\n", cb0_buf);
	INFO("cb1_buf: %p\n", cb1_buf);
	INFO("cb2_buf: %p\n", cb2_buf);
	INFO("rsp_buf: %p\n", rsp_buf);
	INFO("acm_buf: %p\n", acm_buf);
}

void __attribute__((optimize("O0"))) set_config_break(u8 value)
{
	config_break = value;
}

u8 __attribute__((optimize("O0"))) get_config_break(void)
{
	return config_break;
}

void __attribute__((optimize("O0"))) set_acm_config(u8 value)
{
	acm_config_value = value;
}

u8 __attribute__((optimize("O0"))) get_acm_config(void)
{
	return acm_config_value;
}

static int acm_mem_init(void)
{
	bulk_buf_alloc = memalign(CONFIG_SYS_CACHELINE_SIZE, ALIGN_CACHE_SIZE(BUF_SIZE));
	memset(bulk_buf_alloc, 0, ALIGN_CACHE_SIZE(BUF_SIZE));
	cmd_buf_alloc = memalign(CONFIG_SYS_CACHELINE_SIZE, ALIGN_CACHE_SIZE(BUF_SIZE));
	memset(cmd_buf_alloc, 0, ALIGN_CACHE_SIZE(BUF_SIZE));
	ep0_buff_alloc = memalign(CONFIG_SYS_CACHELINE_SIZE, ALIGN_CACHE_SIZE(EP0_SIZE));
	memset(ep0_buff_alloc, 0, ALIGN_CACHE_SIZE(EP0_SIZE));
	setup_buf = memalign(CONFIG_SYS_CACHELINE_SIZE, ALIGN_CACHE_SIZE(STP_SIZE));
	memset(setup_buf, 0, ALIGN_CACHE_SIZE(STP_SIZE));
	handler = memalign(CONFIG_SYS_CACHELINE_SIZE, ALIGN_CACHE_SIZE(HANDLER_SIZE));
	memset(handler, 0, ALIGN_CACHE_SIZE(HANDLER_SIZE));
	cb0_buf = malloc(CB_SIZE);
	memset(cb0_buf, 0, CB_SIZE);
	cb1_buf = malloc(CB_SIZE);
	memset(cb1_buf, 0, CB_SIZE);
	cb2_buf = malloc(CB_SIZE);
	memset(cb2_buf, 0, CB_SIZE);
	rsp_buf = malloc(RSP_SIZE);
	memset(rsp_buf, 0, RSP_SIZE);
	acm_buf = malloc(ACM_SIZE);
	memset(acm_buf, 0, ACM_SIZE);

	set_config_break(0);
	set_acm_config(0);
	current_speed = CH9_USB_SPEED_UNKNOWN;
	mem_alloc_cnt = 0;
	transfer_size = 0;
	flag_enter_dl = 0;
	return 0;
}

static void acm_mem_release(void)
{
	free(bulk_buf_alloc);
	free(cmd_buf_alloc);
	free(ep0_buff_alloc);
	free(setup_buf);
	free(handler);
	free(cb0_buf);
	free(cb1_buf);
	free(cb2_buf);
	free(rsp_buf);
	free(acm_buf);
}

/* interrupt handler */
void acm_isr(void)
{
	cviusb_gadget_handle_interrupts(0);
}

static int get_desc_acm(ch9_usb_speed speed, u8 *acm_desc)
{
	int i = 0;
	void *desc;
	int sum = 0;
	void *(*tab)[];

	switch (speed) {
	case CH9_USB_SPEED_FULL:
		tab = &descriptors_fs;
		break;
	case CH9_USB_SPEED_HIGH:
		tab = &descriptors_hs;
		break;

	default:
		return -1;
	}

	desc = (*tab)[i];

	while (desc) {
		int length = *(u8 *)desc;

		VERBOSE("acm get length %d\n", length);
		memcpy(&acm_desc[sum], desc, length);
		sum += length;
		desc = (*tab)[++i];
	}
	/* VERBOSE("acm get sum:%d\n", sum); */
	return sum;
}

static void clear_req(struct usb_request *req)
{
	memset(req, 0, sizeof(*req));
}

static void reset(struct usb_gadget *gadget)
{
	/* struct CUSBD_Dev *dev; */

	INFO("Application: %s\n", __func__);
}

static void disconnect(struct usb_gadget *gadget)
{
	set_acm_config(0);
	mem_alloc_cnt = 1;
	config_value = 0;
	NOTICE("Application: %s\n", __func__);
}

static void resume(struct usb_gadget *gadget)
{
	VERBOSE("Application: %s\n", __func__);
}

static void req_complete(struct usb_ep *ep, struct usb_request *req)
{
	VERBOSE("Request on endpoint completed\n");
	if (req->status == -EIO)
		NOTICE("IO Abort !!!!!\n");
}

static void suspend(struct usb_gadget *gadget)
{
	VERBOSE("Application: %s %c\n", __func, ' ');
}

static void *request_mem_alloc(struct usb_gadget *gadget, u32 require_size)
{
	void *ptr;
	/* VERBOSE("request_mem_alloc: size %d\n", require_size); */
	if (mem_alloc_cnt == 0)
		ptr = cb0_buf;
	else if (mem_alloc_cnt == 1)
		ptr = cb1_buf;
	else
		ptr = cb2_buf;
	VERBOSE("%s: ptr %p, size %d, mem_alloc_cnt %d\n", __func__, ptr,
		require_size, mem_alloc_cnt);
	mem_alloc_cnt++;
	return ptr;
}

static void request_mem_free(struct usb_gadget *gadget, void *usb_request)
{
}

static void reset_out_req(void)
{
	VERBOSE("ep_out->ops->queue\n");
	bulk_out_req->length = transfer_size;
	bulk_out_req->buf = cmd_buf;
	bulk_out_req->dma = (uintptr_t)cmd_buf;
	bulk_out_req->complete = bulk_out_cmpl_main;
	cvi_cache_flush(bulk_out_req->dma, bulk_out_req->length);
	ep_out->ops->queue(ep_out, bulk_out_req);
}

static void bulk_reset_out_req(struct usb_ep *ep, struct usb_request *req)
{
	/* INFO("bulkReset%sReq complete\n", (ep->address == BULK_EP_IN)?"In":"Out"); */
	reset_out_req();
}

static void send_in_req(u32 length, u8 token, cvi_req_complete complete,
			u8 *p_rsp, u8 rsp_len)
{
	u16 crc;
	static u8 ack_idx;
	struct usb_rsp *rsp = (struct usb_rsp *)rsp_buf;

	memset(rsp_buf, 0, RSP_SIZE);
	if (p_rsp && rsp_len > 0 && rsp_len <= (RSP_SIZE - 8))
		memcpy(rsp_buf + 8, p_rsp, rsp_len);
	crc = crc16_ccitt(0, cmd_buf, length);
	VERBOSE("CRC: %x\n", crc);
	rsp->crc16_hi = (crc >> 8) & 0xFF;
	rsp->crc16_low = crc & 0xFF;
	rsp->ack_index = ack_idx;
	rsp->token = token;
	ack_idx++;
	clear_req(bulk_in_req);
	if (rsp_len > 8)
		bulk_in_req->length = RSP_SIZE;
	else
		bulk_in_req->length = 16;
	bulk_in_req->buf = rsp_buf;
	bulk_in_req->dma = (uintptr_t)rsp_buf;
	bulk_in_req->complete = complete;
	VERBOSE("ep_in->ops->queue\n");

	cvi_cache_flush(bulk_in_req->dma, bulk_in_req->length);
	ep_in->ops->queue(ep_in, bulk_in_req);
}

static void bulk_cmpl_empty(struct usb_ep *ep, struct usb_request *req)
{
	VERBOSE("%s\n", __func__);
}

static void reset_out_req_s2d(u64 addr, size_t size, cvi_req_complete complete)
{
	/* INFO("ep_out->ops->queue S2D, addr:0x%lx, size:0x%lx\n", addr, size); */
	bulk_out_req->length = size;
	bulk_out_req->buf = (u8 *)addr;
	bulk_out_req->dma = (uintptr_t)addr;
	bulk_out_req->complete = complete;

	cvi_cache_flush(bulk_out_req->dma, bulk_out_req->length);
	ep_out->ops->queue(ep_out, bulk_out_req);
}

static void sram_compl(struct usb_ep *ep, struct usb_request *req)
{
	u32 left = sram_info.total_len -= req->length;
	u64 target = sram_info.sram_dest + req->length;
	/* INFO("sram copy data to 0x%lx, len = 0x%x\n", sram_info.sram_dest, req->length); */
	memcpy((void *)sram_info.sram_dest, (void *)req->buf, req->length);

	if (left == 0U)
		reset_out_req();
	else
		sram_out_req_s2_d(target, left);
}

static void sram_out_req_s2_d(u64 addr, u32 size)
{
	sram_info.total_len = size;
	sram_info.sram_dest = addr;

	bulk_out_req->length = (sram_info.total_len > BUF_SIZE) ?
				     BUF_SIZE :
					   sram_info.total_len;
	bulk_out_req->buf = bulk_buf;
	bulk_out_req->dma = (uintptr_t)bulk_buf;
	bulk_out_req->complete = sram_compl;

	cvi_cache_flush(bulk_out_req->dma, bulk_out_req->length);
	ep_out->ops->queue(ep_out, bulk_out_req);
}

static void send_in_req_d2s(u64 addr, size_t size, cvi_req_complete complete)
{
	/* INFO("ep_in->ops->queue D2S\n"); */
	clear_req(bulk_in_req);
	bulk_in_req->length = size;
	bulk_in_req->buf = (u8 *)addr;
	bulk_in_req->dma = (uintptr_t)addr;
	bulk_in_req->complete = complete;

	cvi_cache_flush(bulk_in_req->dma, bulk_in_req->length);
	ep_in->ops->queue(ep_in, bulk_in_req);
}

#if USB_RW_EFUSE // Mark_to_do
static int ahex2int(char a, char b)
{
	a = (a <= '9') ? a - '0' : (a & 0x7) + 9;
	b = (b <= '9') ? b - '0' : (b & 0x7) + 9;

	return (a << 4) + b;
}

static void efusew_cmd(u32 length, u8 *ack_result, u8 ack_len)
{
	u8 read_buf[128];
	u32 i;

	if (strncmp((void *)((uintptr_t)cmd_buf + (uintptr_t)HEADER_SIZE),
		    "efusew ", strlen("efusew ")) == 0) {
		char cmd[128] = { '\0' };
		u8 area_idx = 0;
		u8 area_size = 0;
		u8 data_offset = 0;
		u8 efuse_locked = 0;
#ifdef CONFIG_HW_WATCHDOG
		hw_watchdog_disable();
#endif
		strlcpy(cmd,
			(void *)((uintptr_t)cmd_buf + (uintptr_t)HEADER_SIZE),
			min((u32)(length - HEADER_SIZE),
			    (u32)sizeof(cmd)));
		NOTICE("run command: %s\n", cmd);
		run_command(cmd, 0);

		memset(ack_result, ACK_FAIL, ack_len);

		// Check param for lock efuse area
		for (area_idx = 0; area_idx < ARRAY_SIZE(_allow_wl_areas);
		     area_idx++) {
			if (strncmp((void *)((uintptr_t)cmd_buf +
					     (uintptr_t)HEADER_SIZE +
					     strlen("efusew ")),
				    _allow_wl_areas[area_idx],
				    strlen(_allow_wl_areas[area_idx])) == 0) {
				area_idx += CVI_EFUSE_AREA_LAST + 1;

				NOTICE("area_idx = %d\n", area_idx);
				if (area_idx >=
				    CVI_EFUSE_LOCK_WRITE_HASH0_PUBLIC) {
					efuse_locked = cvi_efuse_is_write_locked(area_idx);
					NOTICE("cvi_efuse_is_write_locked %d\n",
					       efuse_locked);
				} else {
					efuse_locked =
						cvi_efuse_is_locked(area_idx);
					NOTICE("cvi_efuse_is_locked %d\n",
					       efuse_locked);
				}

				if (efuse_locked > 0)
					ack_result[0] = ACK_PASS;
				break;
			}
		}

		// Read efuse data from corresponding area
		for (area_idx = 0; area_idx < ARRAY_SIZE(_allow_areas);
		     area_idx++) {
			if (strncmp((void *)((uintptr_t)cmd_buf +
					     (uintptr_t)HEADER_SIZE +
					     strlen("efusew ")),
				    _allow_areas[area_idx],
				    strlen(_allow_areas[area_idx])) == 0) {
				NOTICE("area_idx = %d\n", area_idx);
				switch (area_idx) {
				case CVI_EFUSE_AREA_USER:
					area_size = 40;
					break;
				case CVI_EFUSE_AREA_DEVICE_ID:
					area_size = 8;
					break;
				case CVI_EFUSE_AREA_HASH0_PUBLIC:
					area_size = 32;
					break;
				case CVI_EFUSE_AREA_LOADER_EK:
					area_size = 16;
					break;
				case CVI_EFUSE_AREA_DEVICE_EK:
					area_size = 16;
					break;
				default:
					NOTICE("efuse area %d\n", area_idx);
					break;
				}

				memset(read_buf, 0, sizeof(read_buf));
				memset(ack_result, ACK_FAIL, ack_len);
				ack_result[0] = ACK_PASS;

				cvi_efuse_read(area_idx, read_buf, area_size);
				for (i = 0; i < area_size; i++)
					INFO("efuse_data[%d] = %x\n", i,
					     read_buf[i]);
				for (i = 0; i < length; i++)
					INFO("cmd_buf[%d] = %x\n", i, cmd_buf[i]);

				// Verify results
				data_offset = HEADER_SIZE + strlen("efusew ") +
					      strlen(_allow_areas[area_idx]) +
					      1;
				INFO("data_offset %x\n", data_offset);
				for (i = 0; i < (length - data_offset) / 2;
				     i++) {
					u8 data;

					data = ahex2int(cmd_buf[data_offset + i * 2],
							cmd_buf[data_offset + i * 2 + 1]);
					INFO("data = %x; read_buf = %x\n", data,
					     read_buf[i]);
					if (data != read_buf[i]) {
						NOTICE("ERROR err_byte_idx = %x\n",
						       i);
						NOTICE("ERROR write_byte = %x\n",
						       data);
						NOTICE("ERROR efuse_byte = %x\n",
						       read_buf[i]);
						ack_result[0] = ACK_FAIL;
						ack_result[1] = i;
						ack_result[2] = data;
						ack_result[3] = read_buf[i];
						break;
					}
				}
				break;
			}
		}
	}
}

static void efuser_cmd(u32 length, u8 *read_buf, u8 buf_len)
{
	u8 i;

	if (strncmp((void *)((uintptr_t)cmd_buf + (uintptr_t)HEADER_SIZE),
		    "efuser ", strlen("efuser ")) == 0) {
		char cmd[128] = { '\0' };
		u8 area_idx = 0;
		u8 area_size = 0;
#ifdef CONFIG_HW_WATCHDOG
		hw_watchdog_disable();
#endif
		strlcpy(cmd,
			(void *)((uintptr_t)cmd_buf + (uintptr_t)HEADER_SIZE),
			min((u32)(length - HEADER_SIZE),
			    (u32)sizeof(cmd)));
		NOTICE("run command: %s\n", cmd);
		run_command(cmd, 0);

		// Read efuse data from corresponding area
		for (area_idx = 0; area_idx < ARRAY_SIZE(_allow_areas);
		     area_idx++) {
			if (strncmp((void *)((uintptr_t)cmd_buf +
					     (uintptr_t)HEADER_SIZE +
					     strlen("efuser ")),
				    _allow_areas[area_idx],
				    strlen(_allow_areas[area_idx])) == 0) {
				NOTICE("area_idx = %d\n", area_idx);
				switch (area_idx) {
				case CVI_EFUSE_AREA_USER:
					area_size = 40;
					break;
				case CVI_EFUSE_AREA_DEVICE_ID:
					area_size = 8;
					break;
				case CVI_EFUSE_AREA_HASH0_PUBLIC:
					area_size = 32;
					break;
				case CVI_EFUSE_AREA_LOADER_EK:
					area_size = 16;
					break;
				case CVI_EFUSE_AREA_DEVICE_EK:
					area_size = 16;
					break;
				default:
					NOTICE("efuse area %d\n", area_idx);
					break;
				}

				memset(read_buf, 0, buf_len);
				cvi_efuse_read(area_idx, read_buf, area_size);
				for (i = 0; i < area_size; i++)
					INFO("efuse_data[%d] = %x\n", i,
					     read_buf[i]);
				break;
			}
		}

		for (area_idx = 0; area_idx < ARRAY_SIZE(_allow_wl_areas);
		     area_idx++) {
			if (strncmp((void *)((uintptr_t)cmd_buf +
					     (uintptr_t)HEADER_SIZE +
					     strlen("efuser ")),
				    _allow_wl_areas[area_idx],
				    strlen(_allow_wl_areas[area_idx])) == 0) {
				area_idx += CVI_EFUSE_AREA_LAST + 1;
				NOTICE("area_idx = %d\n", area_idx);

				memset(read_buf, 0, buf_len);
				if (area_idx >=
				    CVI_EFUSE_LOCK_WRITE_HASH0_PUBLIC) {
					read_buf[0] = cvi_efuse_is_write_locked(area_idx);
					NOTICE("cvi_efuse_is_write_locked %d\n",
					       read_buf[0]);
				} else {
					read_buf[0] =
						cvi_efuse_is_locked(area_idx);
					NOTICE("cvi_efuse_is_locked %d\n",
					       read_buf[0]);
				}
				break;
			}
		}
	}
}
#endif // USB_RW_EFUSE

static void bulk_out_cmpl_main(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_gadget *gadget = drv_obj.gadget;
	u64 dest_addr = 0x0;
	u32 i = 0;
	u16 crc = 0;
	struct usb_msg *msg = (struct usb_msg *)req->buf;
	struct usb_msg_s2d *msg_s2d = (struct usb_msg_s2d *)req->buf;
	struct usb_msg_d2s *msg_d2s = (struct usb_msg_d2s *)req->buf;
	u32 length =
		((u32)msg->header.len_hi << 8) | msg->header.len_low;
	func *jump_fun;
#if USB_RW_EFUSE // Mark_to_do
	u8 read_buf[128];
	u8 ack_result[16];
	u32 sn_hi, sn_lo;
#endif // USB_RW_EFUSE
#ifdef CONFIG_NAND_SUPPORT
	char cmd[255] = { '\0' };
	static char prev_extra[EXTRA_FLAG_SIZE + 1] = { '\0' };
#endif

	if (req->status == -ESHUTDOWN)
		return;

	dest_addr = ((u64)(msg->header.addr4) << 32) |
		    ((u64)(msg->header.addr3) << 24) |
		    ((u64)(msg->header.addr2) << 16) |
		    ((u64)(msg->header.addr1) << 8) |
		    ((u64)(msg->header.addr0));

	if (length == 0 && dest_addr == 0) {
		VERBOSE("buffer zero\n");
		reset_out_req();
		return;
	}
	/* dest_addr += GLOBAL_MEM_START_ADDR; */
	switch (msg->header.token) {
	case CVI_USB_INFO:
		/* INFO("CVI_USB_INFO\n"); */
		send_in_req(length, CVI_USB_INFO, bulk_reset_out_req, NULL, 0);
		return;
	case CVI_USB_S2D:
		/* INFO("CVI_USB_S2D, addr = 0x%lx, len = 0x%lx\n",dest_addr, msg_s2d->size); */
		send_in_req(length, CVI_USB_S2D, bulk_cmpl_empty, NULL, 0);

		if (dest_addr >= GLOBAL_MEM_START_ADDR) {
			reset_out_req_s2d(dest_addr, msg_s2d->size, bulk_reset_out_req);

#ifdef CONFIG_NAND_SUPPORT
			// Erase partition first
			if (!strncmp((char *)((uintptr_t)HEADER_ADDR), "CIMG", 4)) {
				strlcpy(prev_extra,
					(char *)((uintptr_t)HEADER_ADDR + 20),
					EXTRA_FLAG_SIZE);
				snprintf(cmd, 255, "nand erase.part -y %s", prev_extra);
				pr_debug("%s\n", cmd);
				run_command(cmd, 0);
			}
#endif
		} else {
			sram_out_req_s2_d(dest_addr, msg_s2d->size);
		}
		return;
	case CVI_USB_D2S:
		{
			/* INFO("CVI_USB_D2S\n"); */

			if (dest_addr) {
				send_in_req_d2s(dest_addr, msg_d2s->size, bulk_reset_out_req);
			} else {
				unsigned char sendbuf[8];

				u64 image_addr = CVIMMAP_ION_ADDR;

				for (int i = 0; i < sizeof(sendbuf); i++) {
					sendbuf[i] = (image_addr & 0xff);
					image_addr >>= 8;
				}

				send_in_req_d2s((u64)sendbuf, sizeof(sendbuf), bulk_reset_out_req);
			}

			return;
		}
	case CVI_USB_NONE:
		// INFO("CVI_USB_NONE, addr = 0x%llx, len = 0x%x\n", dest_addr, length);
		memcpy((void *)dest_addr, cmd_buf + HEADER_SIZE,
		       length - HEADER_SIZE);
#ifdef CONFIG_HW_WATCHDOG
		WATCHDOG_RESET();
#endif
		send_in_req(length, CVI_USB_NONE, bulk_reset_out_req, NULL, 0);
		return;
	case CVI_USB_JUMP:
		jump_fun = (func *)dest_addr;
		NOTICE("CVI_USB_JUMP to %llx\n", dest_addr);
		if (gadget && gadget->ops && gadget->ops->pullup)
			gadget->ops->pullup(gadget, 0);
		NOTICE("stop USB port\n");
		jump_fun();
		NOTICE("CVI_USB_JUMP back\n");
		reset_out_req();
		break;
	case CVI_USB_PROGRAM:
		/* INFO("CVI_USB_PROGRAM\n"); */
		_prg_image((void *)UPDATE_ADDR, 0x40, NULL);
		send_in_req(length, CVI_USB_PROGRAM, bulk_reset_out_req, NULL, 0);
		NOTICE("CVI_USB_PROGRAM done\n");
		return;
	case CVI_USB_RESET_ARM:
		NOTICE("CVI_USB_RESET_ARM\n");
		break;
	case CVI_USB_BREAK:
		INFO("CVI_USB_BREAK\n");
		set_config_break(1);
		break;
	case CVI_USB_KEEP_DL:
		NOTICE("CVI_USB_KEEP_DL\n");
		crc = crc16_ccitt(0, cmd_buf, length);
		if (crc == 0xB353) {
			flag_enter_dl = 1;
			NOTICE("flag_enter_dl %d\n", flag_enter_dl);
		} else {
			flag_enter_dl = 0;
			NOTICE("MAGIC NUM NOT MATCH\n");
			NOTICE("flag_enter_dl %d\n", flag_enter_dl);
		}
		break;
	case CVI_USB_PRG_CMD:
		NOTICE("CVI_USB_PRG_CMD\n");
		for (i = 0; i < ARRAY_SIZE(_allow_cmds); i++) {
			if (strncmp((void *)((uintptr_t)cmd_buf +
						  (uintptr_t)HEADER_SIZE),
					 _allow_cmds[i],
					 strlen(_allow_cmds[i])) == 0) {
				char cmd[255] = { '\0' };

				strncpy(cmd,
					(void *)((uintptr_t)cmd_buf +
						 (uintptr_t)HEADER_SIZE),
					min(length - HEADER_SIZE,
					    (u32)254));
				NOTICE("run command: %s\n", cmd);
				run_command(cmd, 0);
				break;
			}
		}
		send_in_req(length, CVI_USB_PRG_CMD, bulk_reset_out_req, NULL, 0);
		break;

#if USB_RW_EFUSE // Mark_to_do
	case CVI_USB_EFUSEW:
		NOTICE("CVI_USB_EFUSEW_CMD\n");
		efusew_cmd(length, ack_result, sizeof(ack_result));
		send_in_req(length, CVI_USB_EFUSEW, bulk_reset_out_req, ack_result,
			    sizeof(ack_result));
		break;

	case CVI_USB_EFUSER:
		NOTICE("CVI_USB_EFUSER\n");
		efuser_cmd(length, read_buf, sizeof(read_buf));
		send_in_req(length, CVI_USB_EFUSER, bulk_reset_out_req, read_buf,
			    40);
		break;

	case CVI_USB_READ_SN:
		NOTICE("CVI_USB_READ_SN\n");
#ifdef CONFIG_HW_WATCHDOG
		hw_watchdog_disable();
#endif
		sn_hi = readl(GP_REG4);
		sn_lo = readl(GP_REG5);
		ack_result[0] = (sn_hi >> 24) & 0xFF;
		ack_result[1] = (sn_hi >> 16) & 0xFF;
		ack_result[2] = (sn_hi >> 8) & 0xFF;
		ack_result[3] = (sn_hi >> 0) & 0xFF;
		ack_result[4] = (sn_lo >> 24) & 0xFF;
		ack_result[5] = (sn_lo >> 16) & 0xFF;
		ack_result[6] = (sn_lo >> 8) & 0xFF;
		ack_result[7] = (sn_lo >> 0) & 0xFF;
		NOTICE("0x%x%x\n", sn_hi, sn_lo);
		send_in_req(length, CVI_USB_READ_SN, bulk_reset_out_req, ack_result,
			    sizeof(ack_result));
		break;
#endif // USB_RW_EFUSE
	case CVI_USB_REBOOT:
		NOTICE("CVI_USB_REBOOT\n");
		// software_root_reset(); // Mark_to_do
		mmio_setbits_32(TOP_BASE + 0x8, 0x4); // wdt reset enable
		mmio_write_32(WATCHDOG_BASE + 0x4,
			      0x20); // Set timeout range reigster
		mmio_write_32(WATCHDOG_BASE + 0xc,
			      0x76); // Counter restart register
		mmio_write_32(WATCHDOG_BASE,
			      0x13); // Response mode and enable WDT
		break;

	default:
		VERBOSE("token not defined:[%d]\n", msg->header.token);
		reset_out_req();
		break;
	}
}

/* ACM control ... data handling is delegated to tty library code.
 * The main task of this function is to activate and deactivate
 * that code based on device state; track parameters like line
 * speed, handshake state, and so on; and issue notifications.
 */

static void acm_complete_set_line_coding(struct usb_ep *ep,
					 struct usb_request *req)
{
	struct usb_cdc_line_coding *value = req->buf;

	acm->port_line_coding = *value;
	VERBOSE("acm data transfer complete\n");
}

static void print_ep0_buf(u32 length)
{
	int i;

	for (i = 0; i < length; i++)
		VERBOSE("%02X ", ep0_buff[i]);
	VERBOSE(" %c\n", ' ');
}

static int bind(struct usb_gadget *gadget)
{
	NOTICE("%s()\n", __func__);
	if (drv_obj.gadget) {
		NOTICE("gadget already %s\n", __func__);
		return 0;
	}
	drv_obj.gadget = gadget;

	return 0;
}

static void unbind(struct usb_gadget *gadget)
{
	NOTICE("%s()\n", __func__);
	drv_obj.gadget = NULL;
}

static int setup(struct usb_gadget *gadget, const ch9_usb_setup *ctrl)
{
	/* get device reference */
	int length = 0;
	u16 status_value[2];
	struct ch9_usb_device_descriptor *dev_desc;
	ch9_usb_endpoint_descriptor *endpoint_ep_in_desc, *endpoint_ep_out_desc,
		*endpoint_ep_in_desc2;
	ch9_usb_setup tmp_ctrl;

	*(status_value + 0) = 0;
	*(status_value + 1) = 0;

	tmp_ctrl.b_request = ctrl->b_request;
	tmp_ctrl.bm_request_type = ctrl->bm_request_type;
	tmp_ctrl.w_index = le16_to_cpu(ctrl->w_index);
	tmp_ctrl.w_length = le16_to_cpu(ctrl->w_length);
	tmp_ctrl.w_value = le16_to_cpu(ctrl->w_value);

	VERBOSE("speed: %d\n", gadget->speed);
	VERBOSE("b_request: %02X\n", tmp_ctrl.b_request);
	VERBOSE("b_request_type: %02X\n", tmp_ctrl.bm_request_type);
	VERBOSE("w_index: %04X\n", tmp_ctrl.w_index);
	VERBOSE("w_value: %04X\n", tmp_ctrl.w_value);
	VERBOSE("w_length: %04X\n", tmp_ctrl.w_length);

	ep0_req->buf = ep0_buff;
	ep0_req->dma = (uintptr_t)ep0_buff;
	ep0_req->complete = req_complete;

	switch (gadget->speed) {
	case CH9_USB_SPEED_FULL:
		endpoint_ep_in_desc = &acm_fs_in_desc;
		endpoint_ep_out_desc = &acm_fs_out_desc;
		endpoint_ep_in_desc2 = &acm_fs_notify_desc;
		dev_desc = &dev_hs_desc;
		break;

	case CH9_USB_SPEED_HIGH:
		endpoint_ep_in_desc = &acm_hs_in_desc;
		endpoint_ep_out_desc = &acm_hs_out_desc;
		endpoint_ep_in_desc2 = &acm_fs_notify_desc;
		dev_desc = &dev_hs_desc;
		break;

	default:
		VERBOSE("Unknown speed: %d\n", gadget->speed);
		return 1;
	}

	switch (ctrl->bm_request_type & CH9_USB_REQ_TYPE_MASK) {
	case CH9_USB_REQ_TYPE_STANDARD:

		switch (tmp_ctrl.b_request) {
		case CH9_USB_REQ_GET_DESCRIPTOR:
			VERBOSE("GET DESCRIPTOR %c\n", ' ');
			if ((tmp_ctrl.bm_request_type & CH9_REQ_RECIPIENT_MASK) ==
			    CH9_USB_REQ_RECIPIENT_INTERFACE) {
				switch (tmp_ctrl.w_value >> 8) {
				default:
					return -1;
				}
			} else if ((tmp_ctrl.bm_request_type &
				    CH9_REQ_RECIPIENT_MASK) ==
				   CH9_USB_REQ_RECIPIENT_DEVICE) {
				switch (tmp_ctrl.w_value >> 8) {
				case CH9_USB_DT_DEVICE:
					length = CH9_USB_DS_DEVICE;
					if (cvi_usb_vid != 0) {
						NOTICE("Patch VID %x\n",
						       cvi_usb_vid);
						dev_desc->id_vendor =
							cpu_to_le16(cvi_usb_vid);
					}
					if (cvi_usb_pid != 0) {
						NOTICE("Patch PID %x\n",
						       cvi_usb_pid);
						dev_desc->id_product =
							cpu_to_le16(cvi_usb_pid);
					}
					memmove(ep0_buff, dev_desc, 18);
					VERBOSE("DevDesc[0] = %d\n",
						dev_desc->b_length);
					print_ep0_buf(length);
					break;

				case CH9_USB_DT_CONFIGURATION: {
					u8 *ptr =
						&ep0_buff[CH9_USB_DS_CONFIGURATION];
					u16 acm_desc_len =
						(u16)get_desc_acm(gadget->speed, ptr);

					length = le16_to_cpu(acm_desc_len +
							     CH9_USB_DS_CONFIGURATION);
					conf_desc.w_total_length =
						cpu_to_le16(length);
					memmove(ep0_buff, &conf_desc,
						CH9_USB_DS_CONFIGURATION);
					print_ep0_buf(length);
					break;
				}

				case CH9_USB_DT_STRING: {
					u8 desc_index =
						(u8)(tmp_ctrl.w_value &
							  0xFF);
					char *str_desc;

					VERBOSE("Sstring_desc %c\n", ' ');
					switch (desc_index) {
					case 0:
						str_desc = (char *)&language_desc;
						length = str_desc[0];
						VERBOSE("language %c\n", ' ');
						break;

					case 1:
						str_desc = (char *)&vendor_desc;
						length = str_desc[0];
						VERBOSE("vendor %c\n", ' ');
						break;

					case 2:
						str_desc = (char *)&product_desc;
						length = str_desc[0];
						VERBOSE("product %c\n", ' ');
						break;

					case 3:
						str_desc = (char *)&serial_desc;
						length = str_desc[0];
						VERBOSE("serial %c\n", ' ');
						break;

					default:
						return -1;
					}
					memmove(ep0_buff, str_desc, length);
					break;
				}

				case CH9_USB_DT_BOS: {
					int offset = 0;

					length =
						le16_to_cpu(bos_desc.w_total_length);

					memmove(ep0_buff, &bos_desc,
						CH9_USB_DS_BOS);
					offset += CH9_USB_DS_BOS;
					memmove(&ep0_buff[offset],
						&capability_ext_desc,
						CH9_USB_DS_DEVICE_CAPABILITY_20);
				}
					print_ep0_buf(length);
					VERBOSE("bos_desc %c\n", ' ');
					break;

				case CH9_USB_DT_DEVICE_QUALIFIER:
					length = CH9_USB_DS_DEVICE_QUALIFIER;
					memmove(ep0_buff, &qualifier_desc,
						length);
					break;

				case CH9_USB_DT_OTHER_SPEED_CONFIGURATION: {
					u8 *ptr =
						&ep0_buff[CH9_USB_DS_CONFIGURATION];
					u16 acm_desc_len =
						(u16)get_desc_acm(gadget->speed, ptr);

					length = le16_to_cpu(acm_desc_len +
							     CH9_USB_DS_CONFIGURATION);
					conf_desc.w_total_length =
						cpu_to_le16(length);
					memmove(ep0_buff, &conf_desc,
						CH9_USB_DS_CONFIGURATION);
					print_ep0_buf(length);
					break;
				}

				default:
					return -1;

				} /* switch */
			} /* if */
			break;

		case CH9_USB_REQ_SET_CONFIGURATION: {
			struct usb_ep *ep;
			struct list_head *list;

			VERBOSE("SET CONFIGURATION(%d)\n",
				le16_to_cpu(tmp_ctrl.w_value));
			if (tmp_ctrl.w_value > 1)
				return -1; /* no such configuration */
			/* unconfigure device */
			if (tmp_ctrl.w_value == 0) {
				config_value = 0;
				for (list = gadget->ep_list.next;
				     list != &gadget->ep_list;
				     list = list->next) {
					ep = (struct usb_ep *)list;
					if (ep->name)
						ep->ops->disable(ep);
				}
				return 0;
			}

			/* device already configured */
			if (config_value == 1 && tmp_ctrl.w_value == 1)
				return 0;

			/* configure device */
			config_value = (u8)tmp_ctrl.w_value;

			for (list = gadget->ep_list.next;
			     list != &gadget->ep_list; list = list->next) {
				ep = (struct usb_ep *)list;
				if (ep->name &&
				    (!strcmp(ep->name, "ep1in-bulk"))) {
					ep->ops->enable(ep, endpoint_ep_in_desc);
					VERBOSE("enable EP IN\n");
					break;
				}
			}
			for (list = gadget->ep_list.next;
			     list != &gadget->ep_list; list = list->next) {
				ep = (struct usb_ep *)list;
				if ((ep->name &&
				     !strcmp(ep->name, "ep2out-bulk"))) {
					ep->ops->enable(ep, endpoint_ep_out_desc);
					VERBOSE("enable EP OUT\n");
					break;
				}
			}
			for (list = gadget->ep_list.next;
			     list != &gadget->ep_list; list = list->next) {
				ep = (struct usb_ep *)list;
				if ((ep->name &&
				     !strcmp(ep->name, "ep3in-int"))) {
					ep->ops->enable(ep, endpoint_ep_in_desc2);
					break;
					VERBOSE("enable EP Notify\n");
				}
			}

			/*Code control  Self powered feature of USB*/
			if (conf_desc.bm_attributes &
			    CH9_USB_CONFIG_SELF_POWERED) {
				if (gadget->ops->set_selfpowered)
					gadget->ops->set_selfpowered(gadget, 1);
			} else {
				if (gadget->ops->set_selfpowered)
					gadget->ops->set_selfpowered(gadget, 0);
			}
		}
			return 0;

		case CH9_USB_REQ_GET_CONFIGURATION:
			length = 1;
			memmove(ep0_buff, &config_value, length);
			/* VERBOSE("CH9_USB_REQ_GET_CONFIGURATION %c\n", ' '); */
			break;

		default:
			return -1;
		}
		break;

	case CH9_USB_REQ_TYPE_CLASS: {
		/* SET_LINE_CODING ... just read and save what the host sends */
		switch (tmp_ctrl.b_request) {
		case USB_CDC_REQ_SET_LINE_CODING:
			length = tmp_ctrl.w_length;
			ep0_req->complete = acm_complete_set_line_coding;
			VERBOSE("USB_CDC_REQ_SET_LINE_CODING %d\n", length);
			set_acm_config(1);
			break;
		case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
			acm->port_handshake_bits = tmp_ctrl.w_value;
			set_acm_config(1);
			VERBOSE("USB_CDC_REQ_SET_CONTROL_LINE_STATE %c\n", ' ');
			break;
		case USB_CDC_REQ_GET_LINE_CODING:
			length = tmp_ctrl.w_length;
			memmove(ep0_buff, &acm->port_line_coding, length);
			/* ep0_req->complete = acm_complete_get_line_coding; */
			VERBOSE("USB_CDC_REQ_GET_LINE_CODING %d\n", length);
			set_acm_config(1);
			break;
		}
		break;
	}
	}

	if (length > 0) {
		ep0_req->length =
			tmp_ctrl.w_length < length ? tmp_ctrl.w_length : length;

		cvi_cache_flush(ep0_req->dma, ep0_req->length);
		gadget->ep0->ops->queue(gadget->ep0, ep0_req);
	}
	return 0;
}

static void get_unicode_string(char *target, const char *src)
{
	size_t src_len = strlen(src) * 2;
	int i;

	*target++ = src_len + 2;
	*target++ = CH9_USB_DT_STRING;

	if (src_len > 100)
		src_len = 100;
	for (i = 0; i < src_len; i += 2) {
		*target++ = *src++;
		*target++ = 0;
	}
}

static struct usb_gadget_driver g_driver = {
	.function = "TTY",
	.speed = CH9_USB_SPEED_HIGH,
	.bind = bind,
	.unbind = unbind,
	.setup = setup,
	.reset = reset,
	.disconnect = disconnect,
	.suspend = suspend,
	.resume = resume,
	.req_mem_alloc = request_mem_alloc,
	.req_mem_free = request_mem_free,
};

int acm_app_init(void)
{
	struct usb_ep *ep0 = drv_obj.gadget->ep0;

	/*  set unicode strings */
	get_unicode_string(vendor_desc, USB_MANUFACTURER_STRING);
	get_unicode_string(product_desc, USB_PRODUCT_STRING);
	get_unicode_string(serial_desc, USB_SERIAL_NUMBER_STRING);

	/*  align buffers to modulo8 address */
	ep0_buff = ep0_buff_alloc;
	bulk_buf = bulk_buf_alloc;
	cmd_buf = cmd_buf_alloc;

	memset(ep0_buff_alloc, 0x00, EP0_SIZE);
	memset(bulk_buf_alloc, 0x00, BUF_SIZE);
	memset(cmd_buf_alloc, 0x00, BUF_SIZE);

	/* allocate request for ep0 */
	ep0_req = ep0->ops->alloc_request(ep0);

	/* Change descriptor for maxSpeed == HS only Device*/
	/* For USB2.0 we have to modified w_total_length of BOS descriptor*/
	if (drv_obj.gadget->max_speed < CH9_USB_SPEED_SUPER) {
		bos_desc.w_total_length = cpu_to_le16(CH9_USB_DS_BOS + CH9_USB_DS_DEVICE_CAPABILITY_20);
		bos_desc.b_num_device_caps = 1;
		dev_hs_desc.bcd_usb = cpu_to_le16(BCD_USB_HS_ONLY);
	}

	/* acm init */
	acm = (struct f_acm *)acm_buf;
	acm->port_line_coding.dw_dte_rate = 921600;
	acm->port_line_coding.b_char_format = USB_CDC_1_STOP_BITS;
	acm->port_line_coding.b_parity_type = USB_CDC_NO_PARITY;
	acm->port_line_coding.b_data_bits = 8;
	acm->port_handshake_bits = 0;
	/* VERBOSE("acm size %X\n", sizeof(struct f_acm)); */
	return 0;
}

#ifdef BUILD_ATF
u8 usb_phy_det_connection(void)
{
	u8 phy_det_connected = 0;
	u32 cvi_usb_phy_config = 0;

	cvi_usb_phy_config = plat_cvi_gpio_read(BIT_MASK_GPIO_USB_PHY_DET_OFF);
	if (cvi_usb_phy_config == 0) {
		phy_det_connected = 1;
		NOTICE("by pass USB phy detection\n");
	} else {
		mmio_clrbits_32(GPIO_BASE + 0x4, 1 << 4);
		phy_det_connected =
			(mmio_read_32(GPIO_BASE + 0x50) & (1 << 4)) >> 4;
		INFO("phy_det_connected %d\n", phy_det_connected);
	}

	return phy_det_connected;
}
#endif

#ifndef BUILD_ATF
u32 plat_cvi_gpio_read(u32 mask)
{
	NOTICE("Overwrite fip_src to FIP_SRC_USB\n");
	return FIP_SRC_USB;
}
#endif

#if defined(USB_PHY_DETECTION)
u8 usb_vbus_det(void)
{
	u8 vbus;
	u32 save_pinmux;

	save_pinmux = mmio_read_32(PINMUX_USB_VBUS_DET);
	mmio_write_32(PINMUX_USB_VBUS_DET, 0);
	vbus = (!!(mmio_read_32(REG_TOP_CONF_INFO) & BIT_TOP_CONF_INFO_VBUS));
	mmio_write_32(PINMUX_USB_VBUS_DET, save_pinmux);

	return vbus;
}
#endif

void acm_patch_id(unsigned short vid, unsigned short pid)
{
	cvi_usb_vid = vid;
	cvi_usb_pid = pid;
}

/* ACM entry */
int acm_app(void)
{
	struct usb_gadget *gadget;
	u32 res = 0; /* keeps result of operation on driver */
	struct list_head *list; // used in for_each loop
	struct usb_ep *ep;
	int fip_src = FIP_SRC_MEMMAP;
	u32 ts = 0;
#ifdef BUILD_ATF
	u8 phy_det_connected = 0;

	phy_det_connected = usb_phy_det_connection();

	if (phy_det_connected == 0) {
		NOTICE("USB cable is not connected\n");
		return res;
	}
#endif

#if defined(USB_PHY_DETECTION)
	{
		u32 cnt = 50;

		INFO("waiting for connection ...\n");
		/* debounce */
		while (cnt--) {
			if (!usb_vbus_det())
				cnt = 50;
			mdelay(1);
		}
		INFO("detect vbus ...\n");
	}
#endif
	init_param();
	acm_mem_init();
	print_buf_addr();
	fip_src = plat_cvi_gpio_read(0);
	NOTICE("fip_src %d\n", fip_src);
	if (fip_src == FIP_SRC_USB)
		flag_enter_dl = 1;
	else
		flag_enter_dl = 0;
#ifdef USB_IRQ_MODE
	request_irq(USB_DEV_INTR0, acm_isr, 0, NULL, NULL);
	request_irq(USB_DEV_INTR1, acm_isr, 0, NULL, NULL);
#endif
	drv_obj.plat.handler = (void *)handler;
	drv_obj.plat.size = HANDLER_SIZE;
	drv_obj.plat.ctrl_req = (void *)setup_buf;
	res = cvi_udc_probe(&drv_obj.plat);
	if (res != 0)
		goto error;

	/* bind the gadget object here. */
	if (cviusb_gadget_register_driver(&g_driver) < 0) {
		NOTICE("Gadget Register Fail\n");
		goto error;
	}
	gadget = drv_obj.gadget;
	if (!gadget) {
		NOTICE("Gadget object not existed!\n");
		goto error;
	}

	acm_app_init();

	VERBOSE("Initializing OK! %d\n", __LINE__);

	ts = get_timer(0);
	VERBOSE("ts: %u\n", get_timer(ts));

	gadget->ops->pullup(gadget, 1);
unconfigured:
	while (!get_acm_config()) {
#ifndef USB_IRQ_MODE
		acm_isr();
#endif
		if (get_timer(ts) > 1000 && flag_enter_dl == 0) {
			NOTICE("Enumeration failed\n");
			acm_mem_release();
			return 0;
		}
	}
	NOTICE("USB enumeration done\n");

	mem_alloc_cnt = 1;
	/* find IN endpoint */
	for (list = gadget->ep_list.next; list != &gadget->ep_list;
	     list = list->next) {
		ep = (struct usb_ep *)list;
		if (ep->desc && ep->desc->b_endpoint_address == BULK_EP_IN) {
			bulk_in_req = ep->ops->alloc_request(ep);
			ep_in = ep;
		} else if (ep->desc &&
			   (ep->desc->b_endpoint_address == BULK_EP_OUT)) {
			bulk_out_req = ep->ops->alloc_request(ep);
			ep_out = ep;
		} else if (ep->desc &&
			   (ep->desc->b_endpoint_address == BULK_EP_NOTIFY)) {
			int_in_req = ep->ops->alloc_request(ep);
			ep_in_notify = ep;
		}
	}

	current_speed = gadget->speed;
	switch (current_speed) {
	case CH9_USB_SPEED_FULL:
		transfer_size = 64;
		break;
	case CH9_USB_SPEED_HIGH:
		transfer_size = 512;
		break;
	case CH9_USB_SPEED_SUPER:
		transfer_size = 1024;
		break;
	default:
		VERBOSE("Test error\n");
		acm_mem_release();
		return -1;
	}

	VERBOSE("OUT DATA TRANSFER size :%d\n", transfer_size);
	clear_req(bulk_out_req);
	bulk_out_req->buf = cmd_buf;
	bulk_out_req->dma = (uintptr_t)cmd_buf;
	bulk_out_req->complete = bulk_out_cmpl_main;
	bulk_out_req->length = transfer_size;
	cvi_cache_flush(bulk_out_req->dma, bulk_out_req->length);

	VERBOSE("IN DATA TRANSFER\n");
	clear_req(bulk_in_req);
	bulk_in_req->buf = bulk_buf;
	bulk_in_req->dma = (uintptr_t)bulk_buf;
	bulk_in_req->complete = bulk_reset_out_req;
	bulk_in_req->length = transfer_size;
	cvi_cache_flush(bulk_in_req->dma, bulk_in_req->length);

	ep_out->ops->queue(ep_out, bulk_out_req);
	NOTICE("connection speed: %d\n", gadget->speed);
	ts = get_timer(0);
	VERBOSE("ts: %u\n", get_timer(ts));

	while (1) {
#ifndef USB_IRQ_MODE
		acm_isr();
#endif
		if (!get_acm_config())
			goto unconfigured;
		if (get_config_break())
			break;
		if (flag_enter_dl == 0) {
			if (get_timer(ts) > 1000) {
				NOTICE("wait data timeout\n");
				break;
			}
		}
	}
	NOTICE("Leave transfer loop\n");
	gadget->ops->pullup(gadget, 0);
	cviusb_gadget_unregister_driver(&g_driver);
	acm_mem_release();
	NOTICE("USB stop\n");
	return 0;

error:
	ERROR("Error %u\n", res);
	return res;
}
