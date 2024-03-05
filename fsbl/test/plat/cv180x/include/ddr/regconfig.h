/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __REG_CFG_H__
#define __REG_CFG_H__

struct regconf {
	uint32_t addr;
	uint32_t val;
};

struct regpatch {
	uint32_t addr;
	uint32_t mask;
	uint32_t val;
};

#ifdef DDR2_3
extern struct regpatch ddr3_1866_patch_regs[];
extern uint32_t ddr3_1866_patch_regs_count;
extern struct regpatch ddr2_1333_patch_regs[];
extern uint32_t ddr2_1333_patch_regs_count;
#else
extern struct regpatch ddr_patch_regs[];
extern uint32_t ddr_patch_regs_count;
#endif

#endif /* __REG_CFG_H__ */
