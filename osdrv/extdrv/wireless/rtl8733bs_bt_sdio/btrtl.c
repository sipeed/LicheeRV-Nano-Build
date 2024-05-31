/*
 *  Bluetooth support for Realtek devices
 *
 *  Copyright (C) 2015 Endless Mobile, Inc.
 *  Copyright (C) 2018 Realtek Semiconductor Corp
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

#include <linux/module.h>
#include <linux/firmware.h>
#include <asm/unaligned.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/file.h>
#include <linux/ctype.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "btrtl.h"

#define VERSION "0.1"

#define BDADDR_STRING_LEN	17
#define BDADDR_FILE		"/opt/bdaddr"

#define RTL_EPATCH_SIGNATURE	"Realtech"
#define RTL_ROM_LMP_8821DS	0x8822
#define RTL_ROM_LMP_8723FS	0x8723

#define IC_MATCH_FL_LMPSUBV	(1 << 0)
#define IC_MATCH_FL_HCIREV	(1 << 1)
#define IC_INFO(lmps, hcir) \
	.match_flags = IC_MATCH_FL_LMPSUBV | IC_MATCH_FL_HCIREV, \
	.lmp_subver = (lmps), \
	.hci_rev = (hcir)

struct id_table {
	__u16 match_flags;
	__u16 lmp_subver;
	__u16 hci_rev;
	bool config_needed;
	char *fw_name;
	char *cfg_name;
};

static const struct id_table ic_id_table[] = {
	{ IC_INFO(RTL_ROM_LMP_8821DS, 0xc),
	  .config_needed = false,
	  .fw_name  = "rtl_bt/rtl8821ds_fw",
	  .cfg_name = "rtl_bt/rtl8821ds_config" },
	{ IC_INFO(RTL_ROM_LMP_8723FS, 0xf),
	  .config_needed = false,
	  .fw_name  = "rtl_bt/rtl8723fs_fw",
	  .cfg_name = "rtl_bt/rtl8723fs_config" },
};

struct rtl_config_item {
	u16 offset;
	u8 len;
	u8 data[0];
} __attribute__((packed));

struct rtl_config_hdr {
	u8 sign[4];
	u16 len;
	struct rtl_config_item item[0];
} __attribute__((packed));

struct cfg_list_item {
	struct list_head list;
	struct rtl_config_item *item;
} __attribute__((packed));

static char *bdaddr = "ff:ff:ff:ff:ff:ff";
module_param(bdaddr, charp, 0644);

static int rtl_read_rom_version(struct hci_dev *hdev, u8 *version)
{
	struct rtl_rom_version_evt *rom_version;
	struct sk_buff *skb;

	/* Read RTL ROM version command */
	skb = __hci_cmd_sync(hdev, 0xfc6d, 0, NULL, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s: Read ROM version failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}

	if (skb->len != sizeof(*rom_version)) {
		BT_ERR("%s: RTL version event length mismatch", hdev->name);
		kfree_skb(skb);
		return -EIO;
	}

	rom_version = (struct rtl_rom_version_evt *)skb->data;
	bt_dev_info(hdev, "rom_version status=%x version=%x",
		    rom_version->status, rom_version->version);

	*version = rom_version->version;

	kfree_skb(skb);
	return 0;
}

static int rtlbt_parse_firmware(struct hci_dev *hdev, u16 lmp_subver,
				const struct firmware *fw,
				unsigned char **_buf)
{
	const u8 extension_sig[] = { 0x51, 0x04, 0xfd, 0x77 };
	struct rtl_epatch_header *epatch_info;
	unsigned char *buf;
	int i, ret, len;
	size_t min_size;
	u8 opcode, length, data, rom_version = 0;
	int project_id = -1;
	const unsigned char *fwptr, *chip_id_base;
	const unsigned char *patch_length_base, *patch_offset_base;
	u32 patch_offset = 0;
	u16 patch_length, num_patches;
	static const struct {
		__u16 lmp_subver;
		__u8 id;
	} project_id_to_lmp_subver[] = {
		{ RTL_ROM_LMP_8821DS, 11 },	/* 8821DS */
		{ RTL_ROM_LMP_8821DS, 13 },	/* 8821DS */
		{ RTL_ROM_LMP_8723FS, 19 },	/* 8723FS */
	};

	ret = rtl_read_rom_version(hdev, &rom_version);
	if (ret)
		return ret;

	min_size = sizeof(struct rtl_epatch_header) + sizeof(extension_sig) + 3;
	if (fw->size < min_size)
		return -EINVAL;

	fwptr = fw->data + fw->size - sizeof(extension_sig);
	if (memcmp(fwptr, extension_sig, sizeof(extension_sig)) != 0) {
		BT_ERR("%s: extension section signature mismatch", hdev->name);
		return -EINVAL;
	}

	/* Loop from the end of the firmware parsing instructions, until
	 * we find an instruction that identifies the "project ID" for the
	 * hardware supported by this firwmare file.
	 * Once we have that, we double-check that that project_id is suitable
	 * for the hardware we are working with.
	 */
	while (fwptr >= fw->data + (sizeof(struct rtl_epatch_header) + 3)) {
		opcode = *--fwptr;
		length = *--fwptr;
		data = *--fwptr;

		BT_DBG("check op=%x len=%x data=%x", opcode, length, data);

		if (opcode == 0xff) /* EOF */
			break;

		if (length == 0) {
			BT_ERR("%s: found instruction with length 0",
			       hdev->name);
			return -EINVAL;
		}

		if (opcode == 0 && length == 1) {
			project_id = data;
			break;
		}

		fwptr -= length;
	}

	if (project_id < 0) {
		BT_ERR("%s: failed to find version instruction", hdev->name);
		return -EINVAL;
	}

	/* Find project_id in table */
	for (i = 0; i < ARRAY_SIZE(project_id_to_lmp_subver); i++) {
		if (project_id == project_id_to_lmp_subver[i].id)
			break;
	}

	if (i >= ARRAY_SIZE(project_id_to_lmp_subver)) {
		BT_ERR("%s: unknown project id %d", hdev->name, project_id);
		return -EINVAL;
	}

	if (lmp_subver != project_id_to_lmp_subver[i].lmp_subver) {
		BT_ERR("%s: firmware is for %x but this is a %x", hdev->name,
		       project_id_to_lmp_subver[i].lmp_subver, lmp_subver);
		return -EINVAL;
	}

	epatch_info = (struct rtl_epatch_header *)fw->data;
	if (memcmp(epatch_info->signature, RTL_EPATCH_SIGNATURE, 8) != 0) {
		BT_ERR("%s: bad EPATCH signature", hdev->name);
		return -EINVAL;
	}

	num_patches = le16_to_cpu(epatch_info->num_patches);
	BT_DBG("fw_version=%x, num_patches=%d",
	       le32_to_cpu(epatch_info->fw_version), num_patches);

	/* After the rtl_epatch_header there is a funky patch metadata section.
	 * Assuming 2 patches, the layout is:
	 * ChipID1 ChipID2 PatchLength1 PatchLength2 PatchOffset1 PatchOffset2
	 *
	 * Find the right patch for this chip.
	 */
	min_size += 8 * num_patches;
	if (fw->size < min_size)
		return -EINVAL;

	chip_id_base = fw->data + sizeof(struct rtl_epatch_header);
	patch_length_base = chip_id_base + (sizeof(u16) * num_patches);
	patch_offset_base = patch_length_base + (sizeof(u16) * num_patches);
	for (i = 0; i < num_patches; i++) {
		u16 chip_id = get_unaligned_le16(chip_id_base +
						 (i * sizeof(u16)));
		if (chip_id == rom_version + 1) {
			patch_length = get_unaligned_le16(patch_length_base +
							  (i * sizeof(u16)));
			patch_offset = get_unaligned_le32(patch_offset_base +
							  (i * sizeof(u32)));
			break;
		}
	}

	if (!patch_offset) {
		BT_ERR("%s: didn't find patch for chip id %d",
		       hdev->name, rom_version);
		return -EINVAL;
	}

	BT_DBG("length=%x offset=%x index %d", patch_length, patch_offset, i);
	min_size = patch_offset + patch_length;
	if (fw->size < min_size)
		return -EINVAL;

	/* Copy the firmware into a new buffer and write the version at
	 * the end.
	 */
	len = patch_length;
	buf = kmemdup(fw->data + patch_offset, patch_length, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf + patch_length - 4, &epatch_info->fw_version, 4);

	*_buf = buf;
	return len;
}

static int rtl_download_firmware(struct hci_dev *hdev,
				 const unsigned char *data, int fw_len)
{
	struct rtl_download_cmd *dl_cmd;
	int frag_num = fw_len / RTL_FRAG_LEN + 1;
	int frag_len = RTL_FRAG_LEN;
	int ret = 0;
	int i, j;

	dl_cmd = kmalloc(sizeof(struct rtl_download_cmd), GFP_KERNEL);
	if (!dl_cmd)
		return -ENOMEM;

	for (i = 0; i < frag_num; i++) {
		struct sk_buff *skb;

		BT_DBG("download fw (%d/%d)", i, frag_num);

		if (i > 0x7f)
			j = (i & 0x7f) + 1;
		else
			j = i;

		dl_cmd->index = j;
		if (i == (frag_num - 1)) {
			dl_cmd->index |= 0x80; /* data end */
			frag_len = fw_len % RTL_FRAG_LEN;
		}
		memcpy(dl_cmd->data, data, frag_len);

		/* Send download command */
		skb = __hci_cmd_sync(hdev, 0xfc20, frag_len + 1, dl_cmd,
				     HCI_INIT_TIMEOUT);
		if (IS_ERR(skb)) {
			BT_ERR("%s: download fw command failed (%ld)",
			       hdev->name, PTR_ERR(skb));
			ret = PTR_ERR(skb);
			goto out;
		}

		if (skb->len != sizeof(struct rtl_download_response)) {
			BT_ERR("%s: download fw event length mismatch",
			       hdev->name);
			kfree_skb(skb);
			ret = -EIO;
			goto out;
		}

		kfree_skb(skb);
		data += RTL_FRAG_LEN;
	}

out:
	kfree(dl_cmd);
	return ret;
}

int bachk(const char *str)
{
	if (!str)
		return -1;

	if (strlen(str) != 17)
		return -1;

	while (*str) {
		if (!isxdigit(*str++))
			return -1;

		if (!isxdigit(*str++))
			return -1;

		if (*str == 0)
			break;

		if (*str++ != ':')
			return -1;
	}

	return 0;
}

static int rtl_parse_config(u8 **buff, int len)
{
	u8 sign[4] = { 0x55, 0xab, 0x23, 0x87 };
	struct rtl_config_hdr *cfg_hdr = (void *)*buff;
	u16 data_len;
	struct rtl_config_item *item;
	u8 *pbuf;
	u16 i;
	struct list_head list_configs;
	struct cfg_list_item *n;
	u8 bdaddr_cfg = 0;
	u8 bdaddr_buf[6];
	struct list_head *pos, *next;

	if (!*buff)
		return len;

	if (strcasecmp(bdaddr, "ff:ff:ff:ff:ff:ff")) {
		if (!bachk(bdaddr)) {
			char *str = bdaddr;
			int j;

			BT_INFO("local public address %s", bdaddr);
			bdaddr_cfg = 1;
			for (j = 5; j >= 0; j--, str += 3)
				bdaddr_buf[j] = simple_strtoul(str, NULL, 16);
		} else
			BT_WARN("Invalid address %s", bdaddr);
	}

	if (memcmp(cfg_hdr->sign, sign, 4)) {
		BT_ERR("signature [%02x %02x %02x %02x] error",
		       cfg_hdr->sign[0], cfg_hdr->sign[1],
		       cfg_hdr->sign[2], cfg_hdr->sign[3]);
		return 0;
	}
	data_len = le16_to_cpu(cfg_hdr->len);

	if (data_len != len - sizeof(*cfg_hdr)) {
		BT_ERR("data len %u is not matched to %u",
		       data_len, (u16)(len - sizeof(*cfg_hdr)));
		return 0;
	}

	INIT_LIST_HEAD(&list_configs);
	i = 0;
	pbuf = *buff + sizeof(*cfg_hdr);
	item = (struct rtl_config_item *)pbuf;
	while (i < data_len) {
		u16 ofs;

		ofs = le16_to_cpu(item->offset);
		n = kzalloc(sizeof(*n), GFP_KERNEL);
		if (n) {
			n->item = item;
			list_add_tail(&n->list, &list_configs);
		} else {
			BT_ERR("Couldn't alloc item for %04x", ofs);
		}
		i += (item->len + sizeof(*item));
		item = (struct rtl_config_item *)(pbuf + i);
	}

	list_for_each_safe(pos, next, &list_configs) {
		n = list_entry(pos, struct cfg_list_item, list);
		if (le16_to_cpu(n->item->offset) == 0x0030 && bdaddr_cfg) {
			memcpy(n->item->data, bdaddr_buf, 6);
			bdaddr_cfg = 0;
		}
	}

	/* Add bdaddr */
	if (bdaddr_cfg) {
		u8 *b;

		b = kzalloc(len + 9, GFP_KERNEL);
		if (!b) {
			BT_ERR("Couldn't alloc config buffer");
			return len;
		}

		memcpy(b, *buff, len);

		data_len += 9;
		b[sizeof(cfg_hdr->sign)] = (data_len & 0xff);
		b[sizeof(cfg_hdr->sign) + 1] = ((data_len >> 8) & 0xff);

		b[len] = 0x30;
		b[len + 1] = 0x00;
		b[len + 2] = 6;
		memcpy(b + len + 3, bdaddr_buf, 6);

		len += 9;
		kfree(*buff);
		*buff = b;
	}

	/* free list */
	list_for_each_safe(pos, next, &list_configs) {
		n = list_entry(pos, struct cfg_list_item, list);
		list_del(pos);
		kfree(n);
	}

	return len;
}

static int rtl_load_config(struct hci_dev *hdev, const char *name, u8 **buff)
{
	const struct firmware *fw;
	int ret;

	bt_dev_info(hdev, "rtl: loading %s", name);
	ret = request_firmware(&fw, name, &hdev->dev);
	if (ret < 0)
		return ret;
	ret = fw->size;
	*buff = kmemdup(fw->data, ret, GFP_KERNEL);
	if (!*buff)
		ret = -ENOMEM;

	release_firmware(fw);

	/* Parse config */
	ret = rtl_parse_config(buff, ret);

	return ret;
}

static int send_hci_reset_command(struct hci_dev *hdev)
{
	struct sk_buff *skb;
	int ret = 0;

	skb = __hci_cmd_sync(hdev, HCI_OP_RESET, 0, NULL,
			     HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s: send HCI reset command failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		ret = PTR_ERR(skb);
	}

	kfree_skb(skb);

	return ret;
}

static int setup_rtl8821ds(struct hci_dev *hdev, u16 hci_rev,
				u16 lmp_subver)
{
	unsigned char *fw_data = NULL;
	const struct firmware *fw;
	int ret;
	int cfg_sz;
	u8 *cfg_buff = NULL;
	u8 *tbuff;
	char *cfg_name = NULL;
	char *fw_name = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(ic_id_table); i++) {
		if ((ic_id_table[i].match_flags & IC_MATCH_FL_LMPSUBV) &&
		    (ic_id_table[i].lmp_subver != lmp_subver))
			continue;
		if ((ic_id_table[i].match_flags & IC_MATCH_FL_HCIREV) &&
		    (ic_id_table[i].hci_rev != hci_rev))
			continue;

		break;
	}

	if (i >= ARRAY_SIZE(ic_id_table)) {
		BT_ERR("%s: unknown IC info, lmp subver %04x, hci rev %04x",
		       hdev->name, lmp_subver, hci_rev);
		return -EINVAL;
	}

	cfg_name = ic_id_table[i].cfg_name;

	if (cfg_name) {
		cfg_sz = rtl_load_config(hdev, cfg_name, &cfg_buff);
		if (cfg_sz < 0) {
			cfg_sz = 0;
			if (ic_id_table[i].config_needed)
				BT_ERR("Necessary config file %s not found\n",
				       cfg_name);
		}
	} else
		cfg_sz = 0;

	fw_name = ic_id_table[i].fw_name;
	bt_dev_info(hdev, "rtl: loading %s", fw_name);
	ret = request_firmware(&fw, fw_name, &hdev->dev);
	if (ret < 0) {
		BT_ERR("%s: Failed to load %s", hdev->name, fw_name);
		goto err_req_fw;
	}

	ret = rtlbt_parse_firmware(hdev, lmp_subver, fw, &fw_data);
	if (ret < 0)
		goto out;

	if (cfg_sz) {
		tbuff = kzalloc(ret + cfg_sz, GFP_KERNEL);
		if (!tbuff) {
			ret = -ENOMEM;
			goto out;
		}

		memcpy(tbuff, fw_data, ret);
		kfree(fw_data);

		memcpy(tbuff + ret, cfg_buff, cfg_sz);
		ret += cfg_sz;

		fw_data = tbuff;
	}

	bt_dev_info(hdev, "cfg_sz %d, total size %d", cfg_sz, ret);

	ret = rtl_download_firmware(hdev, fw_data, ret);
	if (ret < 0)
		goto out;

	ret = send_hci_reset_command(hdev);

out:
	release_firmware(fw);
	kfree(fw_data);
err_req_fw:
	if (cfg_sz)
		kfree(cfg_buff);
	return ret;
}

static struct sk_buff *btrtl_read_local_version(struct hci_dev *hdev)
{
	struct sk_buff *skb;

	skb = __hci_cmd_sync(hdev, HCI_OP_READ_LOCAL_VERSION, 0, NULL,
			     HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s: HCI_OP_READ_LOCAL_VERSION failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return skb;
	}

	if (skb->len != sizeof(struct hci_rp_read_local_version)) {
		BT_ERR("%s: HCI_OP_READ_LOCAL_VERSION event length mismatch",
		       hdev->name);
		kfree_skb(skb);
		return ERR_PTR(-EIO);
	}

	return skb;
}

int btrtl_setup_8821ds(struct hci_dev *hdev)
{
	struct sk_buff *skb;
	struct hci_rp_read_local_version *resp;
	u16 hci_rev, lmp_subver;

	skb = btrtl_read_local_version(hdev);
	if (IS_ERR(skb))
		return PTR_ERR(skb);

	resp = (struct hci_rp_read_local_version *)skb->data;
	bt_dev_info(hdev, "rtl: examining hci_ver=%02x hci_rev=%04x "
		    "lmp_ver=%02x lmp_subver=%04x",
		    resp->hci_ver, resp->hci_rev,
		    resp->lmp_ver, resp->lmp_subver);

	hci_rev = le16_to_cpu(resp->hci_rev);
	lmp_subver = le16_to_cpu(resp->lmp_subver);
	kfree_skb(skb);

	switch (lmp_subver) {
	case RTL_ROM_LMP_8821DS:
	case RTL_ROM_LMP_8723FS:
		return setup_rtl8821ds(hdev, hci_rev, lmp_subver);
	default:
		bt_dev_info(hdev, "rtl: assuming no firmware upload needed");
		return 0;
	}
}

MODULE_DESCRIPTION("Bluetooth support for Realtek devices ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
