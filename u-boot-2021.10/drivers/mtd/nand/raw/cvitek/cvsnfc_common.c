// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 bitmain
 */

#include <common.h>
#include <nand.h>
#include <linux/mtd/nand.h>
#include <cvsnfc_common.h>

/*****************************************************************************/
struct nand_flash_dev *(*nand_get_spl_flash_type)(
	struct mtd_info *mtd,
	struct nand_chip *chip,
	struct nand_flash_dev_ex *flash_dev_ex) = NULL;

int (*nand_oob_resize)(struct mtd_info *mtd, struct nand_chip *chip,
		       struct nand_flash_dev_ex *flash_dev_ex) = NULL;

const char *type2str(struct match_type_str *table, int length, int type,
		     const char *def)
{
	while (length-- > 0) {
		if (table->type == type)
			return table->str;
		table++;
	}
	return def;
}

/*****************************************************************************/
#if defined(CONFIG_NAND_FLASH_CVSNFC)

/*****************************************************************************/
static struct match_type_str page2name[] = {
	{ NAND_PAGE_512B, "512" },
	{ NAND_PAGE_2K,   "2K" },
	{ NAND_PAGE_4K,   "4K" },
	{ NAND_PAGE_8K,   "8K" },
	{ NAND_PAGE_16K,  "16K" },
	{ NAND_PAGE_32K,  "32K" },
};

const char *nand_page_name(int type)
{
	return type2str(page2name, ARRAY_SIZE(page2name), type, "unknown");
}

/*****************************************************************************/
static struct match_reg_type page2size[] = {
	{ _512B, NAND_PAGE_512B },
	{ _2K, NAND_PAGE_2K },
	{ _4K, NAND_PAGE_4K },
	{ _8K, NAND_PAGE_8K },
	{ _16K, NAND_PAGE_16K },
	{ _32K, NAND_PAGE_32K },
};

int reg2type(struct match_reg_type *table, int length, int reg, int def)
{
	while (length-- > 0) {
		if (table->reg == reg)
			return table->type;
		table++;
	}
	return def;
}

int type2reg(struct match_reg_type *table, int length, int type, int def)
{
	while (length-- > 0) {
		if (table->type == type)
			return table->reg;
		table++;
	}
	return def;
}

int nandpage_size2type(int size)
{
	return reg2type(page2size, ARRAY_SIZE(page2size), size, NAND_PAGE_2K);
}

int nandpage_type2size(int size)
{
	return type2reg(page2size, ARRAY_SIZE(page2size), size, NAND_PAGE_2K);
}
#endif
