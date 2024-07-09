/*
 *
 *  Generic Bluetooth SDIO driver
 *
 *  Copyright (C) 2007  Cambridge Silicon Radio Ltd.
 *  Copyright (C) 2007  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2018  Realtek Semiconductor Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/timer.h>

#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "btrtl.h"

#define VERSION "0.12.4489ddc.20210423-153940"

#define BTSDIO_DMA_ALIGN		8

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
#define hci_skb_pkt_type(skb)	bt_cb((skb))->pkt_type
#endif

#define BTCOEX
#define CONFIG_COEX
/* #define CONFIG_TEST */
#define RTL8821DS_LPS

#ifdef BTCOEX
#include "rtk_coex.h"
#endif

#ifdef CONFIG_COMBO_MULTISDIO_EXPORT_FROM_RTW
extern int rtw_sdio_multi_state;
#else
int rtw_sdio_multi_state;
EXPORT_SYMBOL(rtw_sdio_multi_state);
#endif

#ifdef CONFIG_TEST
static int delay_time = 10;
static int loop_time = 100; /* default per 100ms */

#endif /* CONFIG_TEST */

static const struct sdio_device_id btsdio_table[] = {
	/* Generic Bluetooth Type-A SDIO device */
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_BT_A) },

	/* Generic Bluetooth Type-B SDIO device */
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_BT_B) },

	/* Generic Bluetooth AMP controller */
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_BT_AMP) },

	{ }	/* Terminating entry */
};

char *pkt_type_str[] = {"command", "acl", "sco", "event"};
MODULE_DEVICE_TABLE(sdio, btsdio_table);

struct btsdio_data {
	struct hci_dev   *hdev;
	struct sdio_func *func;

	struct work_struct work;

	struct sk_buff_head txq;
	int int_count;

	struct task_struct *thread;
	wait_queue_head_t wq;

	spinlock_t lock;

#ifdef CONFIG_TEST
	struct delayed_work timer_loop;
	struct delayed_work timer_delay;
	struct workqueue_struct *test_workqueue;
#endif

#ifdef RTL8821DS_LPS
	unsigned long last_busy;
#endif
};

#define REG_RDAT     0x00	/* Receiver Data */
#define REG_TDAT     0x00	/* Transmitter Data */
#define REG_PC_RRT   0x10	/* Read Packet Control */
#define REG_PC_WRT   0x11	/* Write Packet Control */
#define REG_RTC_STAT 0x12	/* Retry Control Status */
#define REG_RTC_SET  0x12	/* Retry Control Set */
#define REG_INTRD    0x13	/* Interrupt Indication */
#define REG_CL_INTRD 0x13	/* Interrupt Clear */
#define REG_EN_INTRD 0x14	/* Interrupt Enable */
#define REG_MD_STAT  0x20	/* Bluetooth Mode Status */
#define REG_MD_SET   0x20	/* Bluetooth Mode Set */
#define REG_FIFO_STATUS 0x41
#define REG_WAKEUP	0x40	/* Wake-up register */

#ifdef CONFIG_TEST

static void loop_work_func(struct work_struct *work)
{
	struct btsdio_data *data;

	data = container_of(work, struct btsdio_data, timer_delay.work);
	queue_delayed_work(data->test_workqueue, &data->timer_loop,
			   msecs_to_jiffies(loop_time));
	queue_delayed_work(data->test_workqueue, &data->timer_delay,
			   msecs_to_jiffies(delay_time));

	rtw_sdio_multi_state = 1;

	/* BT_INFO("multi_state = %d: BT claiming bus.", rtw_sdio_multi_state);
	 */
}

static void delay_work_func(struct work_struct *work)
{
	struct btsdio_data *data;
	struct sdio_func *func;
	int err;

	data = container_of(work, struct btsdio_data, timer_delay.work);
	func = data->func;

	rtw_sdio_multi_state = 0;

	sdio_claim_host(func);
	sdio_writeb(func, 0x02, 0x40, &err);
	sdio_release_host(func);

	if (err)
		BT_ERR("Write sdio reg in delay timer, error %d", err);

	/* BT_INFO("multi_state = %d: BT releasing bus.", rtw_sdio_multi_state);
	 */
}

#endif /* CONFIG_TEST */

static int wait_for_txfifo_ready(struct sdio_func *func)
{
	u8 result;
	int err;

	for (;;) {
		result = sdio_readb(func, REG_FIFO_STATUS, &err);
		if (err) {
			BT_ERR("Read error %d while waiting for tx fifo ready",
			       err);
			return err;
		}

		if ((result & 0x01) || (result & 0x02))
			break;
	}

	return 0;

}

static int wait_for_rxfifo_ready(struct sdio_func *func)
{
	u8 result;
	int err;

	for (;;) {
		result = sdio_readb(func, REG_FIFO_STATUS, &err);
		if (err) {
			BT_ERR("Read error %d while waiting for rx fifo ready",
			       err);
			return err;
		}

		if ((result & 0x04) || (result & 0x08))
			break;
	}

	return 0;
}

#ifdef RTL8821DS_LPS
static int wait_for_io_ready(struct sdio_func *func)
{
	u8 result;
	int err = 0;

	for (;;) {
		/* read function 0 cccr 0x03 reg that is io ready register */
		result = sdio_f0_readb(func, 0x03, &err);
		if (err) {
			BT_ERR("wait for io ready error %d", err);
			return err;
		}

		if (result & (1 << 2))
			break;
	}

	return 0;
}
#endif

static int btsdio_tx_packet(struct btsdio_data *data, struct sk_buff *skb)
{
	int err;
	int pkt_type;
#ifdef CONFIG_COEX
	struct sdio_func *func = data->func;
#endif
	unsigned int len;
	struct sk_buff *tmpskb;

#ifdef RTL8821DS_LPS
	long elapsed;
	u8 reg_val;
#endif

	pkt_type = hci_skb_pkt_type(skb);
	BT_DBG("%s", data->hdev->name);
	/* BT_INFO("tx packet: pkt_type %s, pkt_len: %d",
	 * 	pkt_type_str[pkt_type - 1], skb->len);
	 */

#ifdef BTCOEX
	switch (pkt_type) {
	case HCI_COMMAND_PKT:
		rtk_btcoex_parse_cmd(skb->data, skb->len);
		break;
	case HCI_ACLDATA_PKT:
		rtk_btcoex_parse_l2cap_data_tx(skb->data, skb->len);
		break;
	}
#endif

	/* Prepend Type-A header */
	skb_push(skb, 4);
	if (1) {
		skb->data[0] = skb->len & 0xff;
		skb->data[1] = (skb->len >> 8) & 0xff;
		skb->data[2] = (skb->len >> 16) & 0xff;
	} else {
		skb->data[2] = skb->len & 0xff;
		skb->data[1] = (skb->len >> 8) & 0xff;
		skb->data[0] = (skb->len >> 16) & 0xff;
	}

	skb->data[3] = hci_skb_pkt_type(skb);

	if ((unsigned long)skb->data & (BTSDIO_DMA_ALIGN - 1)) {
		tmpskb = bt_skb_alloc(skb->len, GFP_ATOMIC);
		if (!tmpskb) {
			BT_ERR("Could not alloc tmp skb for btrtksdio tx");
			return -ENOMEM;
		}
		memcpy(skb_put(tmpskb, skb->len), skb->data, skb->len);
		if ((unsigned long)tmpskb->data & (BTSDIO_DMA_ALIGN - 1))
			BT_ERR("Packet address not aligned, %p, %p",
			       skb->data, tmpskb->data);
		kfree_skb(skb);
		skb = tmpskb;
	}

	/* Realtek btsdio tx timing
	 * 1) repeatedly check fifo status until at least one tx fifo ready
	 * 2) write sdio header(4 bytes) and write len <= 128 ? len : 128
	 *    bytes data
	 * 3) if not done, repeat the following until done:
	 *    3.1) check fifo status until at least one tx fifo ready
	 *    3.2) write at most 128 bytes data;
	 */

	sdio_claim_host(data->func);

#ifdef RTL8821DS_LPS
	/* Check if the controller is in LPS */
	elapsed = jiffies - data->last_busy;
	if (elapsed >= 0 && elapsed < msecs_to_jiffies(5000))
		goto exit_lps;

	/* if jiffies has wrapped around (elapsed < 0) or elapsed is equal to
	 * or bigger than 5s, assume 8821DS has been in LPS and wake it up.
	 */
	BT_INFO("Tx: wake up controller that may be in LPS");
	reg_val = 0x01;
	sdio_writeb(func, reg_val, REG_WAKEUP, &err);
	if (err) {
		BT_ERR("Write REG_WAKEUP error %d", err);
		goto exit;
	}

	/* Wait for the Bluetooth IO Ready */
	err = wait_for_io_ready(data->func);
	if (err)
		goto exit;

exit_lps:
	data->last_busy = jiffies;
#endif

#ifdef CONFIG_COEX
	rtw_sdio_multi_state = 1;
	/* BT_INFO("multi_state = %d: BT claiming bus.", rtw_sdio_multi_state);
	 */
#endif

	err = wait_for_txfifo_ready(data->func);
	if (err)
		goto exit;

	err = sdio_writesb(data->func, REG_TDAT, skb->data, 4);
	if (err) {
		/* TODO: retry for header */
		BT_ERR("Write BTSDIO packet header error %d", err);
		goto exit;
	}

	/* get rid of header on success */
	skb_pull(skb, 4);
	data->hdev->stat.byte_tx += 4;

	/* NOTE: FIFO is already ready to read the first part of payload */
	do {
		if (skb->len <= 128)
			len = skb->len;
		else
			len = 128;
		err = sdio_writesb(data->func, REG_TDAT, skb->data, len);
		if (err) {
			BT_ERR("Couldn't write %u byptes to card error %d",
			       len, err);
			goto exit;
		}
		data->hdev->stat.byte_tx += len;
		skb_pull(skb, len);
		err = wait_for_txfifo_ready(data->func);
		if (err)
			break;
	} while (skb->len);
exit:

#ifdef CONFIG_COEX
	rtw_sdio_multi_state = 0;

	sdio_writeb(func, 0x02, 0x40, &err);
	if (err)
		BT_ERR("rtlsdio coex tx error %d", err);
	/* BT_INFO("multi_state = %d: BT releasing bus.", rtw_sdio_multi_state);
	 */
#endif

	sdio_release_host(data->func);
	kfree_skb(skb);

	return 0;
}

static int btsdio_rx_packet(struct btsdio_data *data)
{
	u8 hdr[4] __aligned(4);
	struct sk_buff *skb;
	int err;
	unsigned int len;
	unsigned int hci_len;
#ifdef CONFIG_COEX
	struct sdio_func *func = data->func;
#endif

	BT_DBG("%s", data->hdev->name);
	sdio_claim_host(data->func);
	err = sdio_readsb(data->func, hdr, REG_RDAT, 4);
	if (err < 0) {
		BT_ERR("Read BTSDIO packet header error %d", err);
		sdio_release_host(data->func);
		return err;
	}

#ifdef RTL8821DS_LPS
	/* If we can read packet from controller, the controller is awake */
	data->last_busy = jiffies;
#endif

#ifdef CONFIG_COEX
	rtw_sdio_multi_state = 1;
	/* BT_INFO("multi_state = %d: BT claiming bus.", rtw_sdio_multi_state);
	 */
#endif

	if (1)
		len = hdr[0] | (hdr[1] << 8) | (hdr[2] << 16);
	else
		len = hdr[2] | (hdr[1] << 8) | (hdr[0] << 16);

	if (len < 4 || len > 65543)
		return -EILSEQ;

	hci_len = len - 4;
	skb = bt_skb_alloc(hci_len, GFP_KERNEL);
	if (!skb) {
		BT_ERR("Couldn't alloc skb for rx");
		/* Out of memory. Prepare a read retry and just
		 * return with the expectation that the next time
		 * we're called we'll have more memory.
		 */
		return -ENOMEM;
	}

	/* Realtek btsdio rx timing:
	 * if hci_len <= 256, read it. Otherwise read 256, then repeat following
	 * repeatedly check for rx fifo confition until at least one is empty,
	 * then read 128.
	 */

	/* NOTE: FIFO is already ready to write the first part of payload */
	len = 256;
	do {
		if (hci_len < len)
			len = hci_len;

		/* For the first time, read 256-byte data */
		err = sdio_readsb(data->func, skb_put(skb, len), REG_RDAT, len);
		if (err) {
			BT_ERR("Read payload error %d", err);
			goto exit;
		}

		hci_len -= len;
		data->hdev->stat.byte_rx += len;

		if (!hci_len)
			break;

		len = 128;
		err = wait_for_rxfifo_ready(data->func);
		if (err)
			break;
	} while (hci_len);

exit:
#ifdef CONFIG_COEX
	rtw_sdio_multi_state = 0;

	sdio_writeb(func, 0x02, 0x40, &err);
	if (err)
		BT_ERR("rtlsdio coex rx error %d", err);
	/* BT_INFO("multi_state = %d: BT releasing bus.", rtw_sdio_multi_state);
	 */
#endif
	sdio_release_host(data->func);
	/* BT_INFO("rx packet: pkt_type %s, pkt_len: %d",
	 * 	pkt_type_str[hdr[3] - 1], len - 4);
	 */

	hci_skb_pkt_type(skb) = hdr[3];
#ifdef BTCOEX
	switch (hci_skb_pkt_type(skb)) {
	case HCI_EVENT_PKT:
		rtk_btcoex_parse_event(skb->data, skb->len);
		break;
	case HCI_ACLDATA_PKT:
		rtk_btcoex_parse_l2cap_data_rx(skb->data, skb->len);
		break;
	}
#endif

	err = hci_recv_frame(data->hdev, skb);
	if (err < 0)
		return err;

	return 0;
}

static void btsdio_interrupt(struct sdio_func *func)
{
	struct btsdio_data *data = sdio_get_drvdata(func);
	u8 intrd;
	ulong flags;
	int err;

	BT_DBG("%s", data->hdev->name);

	/* BT_INFO("Got Data  Interrupt"); */
	intrd = sdio_readb(func, REG_INTRD, &err);
	if (!err && (intrd & 0x01)) {
		sdio_writeb(func, 0x01, REG_CL_INTRD, &err);
		if (err) {
			BT_ERR("Clear REG_CL_INTRD error %d", err);
			return;
		}
		spin_lock_irqsave(&data->lock, flags);
		data->int_count++;
		spin_unlock_irqrestore(&data->lock, flags);
		wake_up_interruptible(&data->wq);
	} else {
		BT_ERR("Unknown interrupt, err %d, intrd 0x%02x", err, intrd);
	}
}

static int btsdio_open(struct hci_dev *hdev)
{
	struct btsdio_data *data = hci_get_drvdata(hdev);
	int err;
#ifdef RTL8821DS_LPS
	u8 reg_val;
#endif

	BT_DBG("%s", hdev->name);

	sdio_claim_host(data->func);

#ifdef RTL8821DS_LPS
	reg_val = 0x01;
	sdio_writeb(data->func, reg_val, REG_WAKEUP, &err);
	if (err) {
		BT_ERR("Write REG_WAKEUP error while open, %d", err);
		goto release;
	}

	data->last_busy = jiffies;
#endif

	err = sdio_enable_func(data->func);
	if (err < 0)
		goto release;

	err = sdio_claim_irq(data->func, btsdio_interrupt);
	if (err < 0) {
		sdio_disable_func(data->func);
		goto release;
	}

	if (data->func->class == SDIO_CLASS_BT_B) {
		sdio_writeb(data->func, 0x00, REG_MD_SET, &err);
		if (err) {
			BT_ERR("Clear REG_MD_SET error %d", err);
			goto release;
		}
	}

	sdio_writeb(data->func, 0x01, REG_EN_INTRD, &err);
	if (err) {
		BT_ERR("Set REG_EN_INTRD error %d", err);
		goto release;
	}

#ifdef BTCOEX
	rtk_btcoex_open(hdev);
#endif

release:
	sdio_release_host(data->func);

	return err;
}

static int btsdio_close(struct hci_dev *hdev)
{
	int err;
	struct btsdio_data *data = hci_get_drvdata(hdev);

	BT_DBG("%s", hdev->name);

	sdio_claim_host(data->func);

	sdio_writeb(data->func, 0x00, REG_EN_INTRD, &err);
	if (err)
		BT_ERR("Clear REG_EN_INTRD error %d", err);

	sdio_release_irq(data->func);

	sdio_disable_func(data->func);

	sdio_release_host(data->func);

#ifdef BTCOEX
	rtk_btcoex_close();
#endif

	return 0;
}

static int btsdio_flush(struct hci_dev *hdev)
{
	struct btsdio_data *data = hci_get_drvdata(hdev);

	BT_DBG("%s", hdev->name);

	skb_queue_purge(&data->txq);

	return 0;
}

static int btsdio_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{

	struct btsdio_data *data = hci_get_drvdata(hdev);

	/* BT_INFO("btsdio_send_frame"); */
	BT_DBG("%s", hdev->name);

	switch (hci_skb_pkt_type(skb)) {
	case HCI_COMMAND_PKT:
		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		hdev->stat.acl_tx++;
		break;

	case HCI_SCODATA_PKT:
		hdev->stat.sco_tx++;
		break;

	default:
		return -EILSEQ;
	}

	skb_queue_tail(&data->txq, skb);
	/* schedule_work(&data->work); */
	wake_up_interruptible(&data->wq);
	return 0;
}

static int main_thread(void *dat)
{
	struct btsdio_data *data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
	wait_queue_entry_t wait;
#else
	wait_queue_t wait;
#endif
	ulong flags;
	int err;
	struct sk_buff *skb;

	data = dat;

	init_waitqueue_entry(&wait, current);

	for (;;) {
		add_wait_queue(&data->wq, &wait);
		set_current_state(TASK_INTERRUPTIBLE);

		if (kthread_should_stop()) {
			BT_INFO("break from main thread");
			break;
		}
		if (data->int_count == 0 && skb_queue_empty(&data->txq)) {
			/* BT_INFO("main thread sleeping"); */
			schedule();
		}
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&data->wq, &wait);

		/* BT_INFO("main thread woke up"); */

		spin_lock_irqsave(&data->lock, flags);
		if (data->int_count != 0) {
			data->int_count = 0;
			spin_unlock_irqrestore(&data->lock, flags);
			btsdio_rx_packet(data);
		} else
			spin_unlock_irqrestore(&data->lock, flags);

		skb = skb_dequeue(&data->txq);
		if (skb) {
			err = btsdio_tx_packet(data, skb);
			if (err < 0) {
				BT_ERR("BTSDIO Tx packet error %d", err);
				data->hdev->stat.err_tx++;
				skb_queue_head(&data->txq, skb);
				break;
			}
		}
	}

	return 0;
}

static int btsdio_probe(struct sdio_func *func,
				const struct sdio_device_id *id)
{
	printk("rtk bt sdio driver probe\n\r");
	struct btsdio_data *data;
	struct hci_dev *hdev;
	struct sdio_func_tuple *tuple = func->tuples;
	int err;
	u8 reg_val;

	BT_INFO("func %p id %p class 0x%04x", func, id, func->class);

	while (tuple) {
		BT_DBG("code 0x%x size %d", tuple->code, tuple->size);
		tuple = tuple->next;
	}

	data = devm_kzalloc(&func->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->func = func;

	data->int_count = 0;

	/* INIT_WORK(&data->work, btsdio_work); */

	skb_queue_head_init(&data->txq);

	init_waitqueue_head(&data->wq);

	spin_lock_init(&data->lock);

	if (func->vendor == 0x024c &&
	    (func->device == 0xb73a || func->device == 0x885a)) {
		sdio_claim_host(func);
		/* read and update */
		reg_val = sdio_readb(func, 0x71, &err);
		if (err) {
			BT_ERR("Read 0x71 register failure, %d", err);
		} else {
			reg_val |= (1 << 2);
			sdio_writeb(func, reg_val, 0x71, &err);
			if (err)
				BT_ERR("Enable ECO function error, %d", err);
		}
		sdio_release_host(func);
	}

#ifdef RTL8821DS_LPS
	reg_val = 0x01;
	sdio_claim_host(func);
	sdio_writeb(func, reg_val, REG_WAKEUP, &err);
	sdio_release_host(func);
	if (err) {
		BT_ERR("Write REG_WAKEUP error while probe, %d", err);
		return err;
	}

	data->last_busy = jiffies;
#endif

	data->thread = kthread_run(main_thread, data, "btrtk");

	hdev = hci_alloc_dev();
	if (!hdev) {
		BT_ERR("Couldn't alloc hdev");
		err = -ENOMEM;
		goto err_alloc_dev;
	}

	hdev->bus = HCI_SDIO;
	hci_set_drvdata(hdev, data);

	if (id->class == SDIO_CLASS_BT_AMP)
		hdev->dev_type = HCI_AMP;
	else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
		hdev->dev_type = HCI_BREDR;
#else
		hdev->dev_type = HCI_PRIMARY;
#endif

	data->hdev = hdev;

	SET_HCIDEV_DEV(hdev, &func->dev);

	hdev->open     = btsdio_open;
	hdev->setup    = btrtl_setup_8821ds;
	hdev->close    = btsdio_close;
	hdev->flush    = btsdio_flush;
	hdev->send     = btsdio_send_frame;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 1)
	set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);
	BT_DBG("set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);");
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	set_bit(HCI_QUIRK_SIMULTANEOUS_DISCOVERY, &hdev->quirks);
#endif

	err = hci_register_dev(hdev);
	if (err < 0) {
		BT_ERR("Couldn't register hdev");
		goto err_hci_reg;
	}

#ifdef BTCOEX
	rtk_btcoex_probe(hdev);
#endif

	sdio_set_drvdata(func, data);

#ifdef CONFIG_TEST
	BT_INFO("BT interrupt every %d mills", loop_time);

	data->test_workqueue = create_workqueue("rtlsdiotest");
	if (!data->test_workqueue) {
		BT_ERR("Couldn't test create workqueue");
		err = -ENOMEM;
		goto err_create_wq;
	}

	INIT_DELAYED_WORK(&data->timer_loop, loop_work_func);
	INIT_DELAYED_WORK(&data->timer_delay, delay_work_func);

	queue_delayed_work(data->test_workqueue, &data->timer_loop,
			   msecs_to_jiffies(loop_time));
#endif

	return 0;

#ifdef CONFIG_TEST
err_create_wq:
	sdio_set_drvdata(func, NULL);
	hci_unregister_dev(hdev);
#endif
err_hci_reg:
	hci_free_dev(hdev);
err_alloc_dev:
	kthread_stop(data->thread);
	return err;
}

static void btsdio_remove(struct sdio_func *func)
{
	struct btsdio_data *data = sdio_get_drvdata(func);
	struct hci_dev *hdev;

	BT_DBG("func %p", func);

	if (!data)
		return;

	hdev = data->hdev;

#ifdef CONFIG_TEST
	cancel_delayed_work_sync(&data->timer_loop);
	cancel_delayed_work_sync(&data->timer_delay);
	flush_workqueue(data->test_workqueue);
	destroy_workqueue(data->test_workqueue);
#endif

	sdio_claim_host(func);
	sdio_release_irq(func);
	sdio_disable_func(func);
	sdio_release_host(func);

	kthread_stop(data->thread);

	sdio_set_drvdata(func, NULL);

	hci_unregister_dev(hdev);

	hci_free_dev(hdev);
}

static struct sdio_driver btsdio_driver = {
	.name		= "btstdsdio",
	.probe		= btsdio_probe,
	.remove		= btsdio_remove,
	.id_table	= btsdio_table,
};

static int __init btsdio_init(void)
{
	BT_INFO("Realtek Bluetooth SDIO driver ver %s", VERSION);
#ifdef BTCOEX
	rtk_btcoex_init();
#endif
	return sdio_register_driver(&btsdio_driver);
}

static void __exit btsdio_exit(void)
{
	sdio_unregister_driver(&btsdio_driver);
#ifdef BTCOEX
	rtk_btcoex_exit();
#endif
}

module_init(btsdio_init);
module_exit(btsdio_exit);

#ifdef CONFIG_TEST
module_param(delay_time, int, 0644);
module_param(loop_time, int, 0644);
#endif

MODULE_DESCRIPTION("Realtek Bluetooth SDIO driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
