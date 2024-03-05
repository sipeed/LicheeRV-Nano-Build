// SPDX-License-Identifier: BSD-3-Clause

#include <stddef.h>
#include <utils_def.h>
#include <mmio.h>
#include <ddr_sys.h>
#include "regconfig.h"

//regpatch_ddr3_x16_bga.c for 1866

struct regpatch ddr3_1866_patch_regs[] = {
	// BYTE0 RX DQ deskew
	{0x08000b00, 0xFFFFFFFF, 0x05030703},
	{0x08000b04, 0xFFFFFFFF, 0x03030403},
	// BYTE0  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b08, 0xFFFFFFFF, 0x002e3b03},

	// BYTE1 RX DQ deskew
	{0x08000b30, 0xFFFFFFFF, 0x07030204},
	{0x08000b34, 0xFFFFFFFF, 0x03030403},
	// BYTE1  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b38, 0xFFFFFFFF, 0x002e3d04},
};

uint32_t ddr3_1866_patch_regs_count = ARRAY_SIZE(ddr3_1866_patch_regs);

//regpatch_ddr2_1333_x16_qfn.c

struct regpatch ddr2_1333_patch_regs[] = {
	// DQ driving // BYTE0
	{0x08000a38, 0xFFFFFFFF, 0x00000808},

	// DQ driving // BYTE1
	{0x08000a78, 0xFFFFFFFF, 0x00000808},

	// BYTE0 RX DQ deskew
	{0x08000b00, 0xFFFFFFFF, 0x03010504},
	{0x08000b04, 0xFFFFFFFF, 0x02030203},
	// BYTE0  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b08, 0xFFFFFFFF, 0x002a4204},

	// BYTE1 RX DQ deskew
	{0x08000b30, 0xFFFFFFFF, 0x08030803},
	{0x08000b34, 0xFFFFFFFF, 0x02070205},
	// BYTE1  DQ8 deskew [6:0] neg DQS  [15:8]  ;  pos DQS  [23:16]
	{0x08000b38, 0xFFFFFFFF, 0x002b4003},
};

uint32_t ddr2_1333_patch_regs_count = ARRAY_SIZE(ddr2_1333_patch_regs);