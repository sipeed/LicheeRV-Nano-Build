#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "system_common.h"
#include "timer.h"
#elif defined(ENV_EMU)
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "emu/command.h"
#else
#include <linux/types.h>
#include <linux/delay.h>
#endif  // ENV_CVITEST

#include "ldc_reg.h"
#include "ldc_cfg.h"
#include "ldc.h"
#include "cmdq.h"
#include "reg.h"
#include "reg_ldc.h"
#include "dwa_debug.h"

/****************************************************************************
 * Global parameters
 ***************************************************************************/
static uintptr_t reg_base;

/****************************************************************************
 * Initial info
 ***************************************************************************/

/****************************************************************************
 * Interfaces
 ****************************************************************************/

void ldc_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
}

void ldc_get_base_addr(void **base)
{
	*base = (void *)reg_base;
}

/**
 * ldc_init - setup ldc, mainly interpolation settings.
 *
 * @param cfg: settings for this ldc's operation
 */
void ldc_init(void)
{
}

/**
 * ldc_reset - do reset. This can be activated only if dma stop to avoid hang
 *	       fabric.
 *
 */
void ldc_reset(void)
{
#if 0
	union vip_sys_reset mask;

	mask.raw = 0;
	mask.b.ldc = 1;
	vip_toggle_reset(mask);
#endif
}

/**
 * ldc_intr_ctrl - ldc's interrupt on(1)/off(0)
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_ctrl(u8 intr_mask)
{
	_reg_write(reg_base + REG_LDC_IRQEN, intr_mask);
}

/**
 * ldc_intr_clr - clear ldc's interrupt
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @param intr_mask: On/Off ctrl of the interrupt.
 */
void ldc_intr_clr(u8 intr_mask)
{
	_reg_write(reg_base + REG_LDC_IRQCLR, intr_mask);
}

/**
 * ldc_intr_status - ldc's interrupt status
 *                 bit0: frame_done, bit1: mesh_id axi read err,
 *                 bit2: mesh_table axi read err
 *
 * @return: The interrupt's status. 1 if active.
 */
u8 ldc_intr_status(void)
{
	return _reg_read(reg_base + REG_LDC_IRQSTAT);
}

/**
 * ldc_check_param - check if config param is valid.
 *
 * @param cfg: settings for this ldc's operation
 * @return: true for valid.
 */
bool ldc_check_param(const struct ldc_cfg *cfg)
{
	if ((cfg->src_width > LDC_MAX_WIDTH) || (cfg->src_height > LDC_MAX_HEIGHT)) {
		return false;
	}
	if ((cfg->src_width & (LDC_SIZE_ALIGN - 1)) ||
	    (cfg->src_height & (LDC_SIZE_ALIGN - 1))) {
		return false;
	}
	if ((cfg->src_xstart > cfg->src_xend) ||
	    (cfg->src_xend - cfg->src_xstart < 32) ||
	    (cfg->src_xstart > cfg->src_width - 1) ||
	    (cfg->src_xend > cfg->src_width - 1)) {
		return false;
	}
	if (cfg->map_base & (LDC_ADDR_ALIGN - 1)) {
		return false;
	}
	if ((cfg->src_y_base & (LDC_ADDR_ALIGN - 1)) ||
	    (cfg->src_c_base & (LDC_ADDR_ALIGN - 1))) {
		return false;
	}
	if ((cfg->dst_y_base & (LDC_ADDR_ALIGN - 1)) ||
	    (cfg->dst_c_base & (LDC_ADDR_ALIGN - 1))) {
		return false;
	}
	return true;
}

/**
 * ldc_engine - start a ldc operation, wait frame_done intr after this.
 *              If output target is scaler, scaler's config should be done
 *              before this.
 *
 * @param cfg: settings for this ldc's operation
 */
void ldc_engine(const struct ldc_cfg *cfg)
{
	u8 ras_mode = (cfg->dst_mode == LDC_DST_FLAT) ? 0 : 1;

	_reg_write(reg_base + REG_LDC_DATA_FORMAT, cfg->pixel_fmt);
	_reg_write(reg_base + REG_LDC_RAS_MODE, ras_mode);
	_reg_write(reg_base + REG_LDC_RAS_XSIZE, cfg->ras_width);
	_reg_write(reg_base + REG_LDC_RAS_YSIZE, cfg->ras_height);

	_reg_write(reg_base + REG_LDC_MAP_BASE, cfg->map_base >> LDC_BASE_ADDR_SHIFT);
	_reg_write(reg_base + REG_LDC_MAP_BYPASS, cfg->map_bypass);

	_reg_write(reg_base + REG_LDC_SRC_BASE_Y, cfg->src_y_base >> LDC_BASE_ADDR_SHIFT);
	_reg_write(reg_base + REG_LDC_SRC_BASE_C, cfg->src_c_base >> LDC_BASE_ADDR_SHIFT);
	_reg_write(reg_base + REG_LDC_SRC_XSIZE, cfg->src_width);
	_reg_write(reg_base + REG_LDC_SRC_YSIZE, cfg->src_height);
	_reg_write(reg_base + REG_LDC_SRC_XSTART, cfg->src_xstart);
	_reg_write(reg_base + REG_LDC_SRC_XEND, cfg->src_xend);
	_reg_write(reg_base + REG_LDC_SRC_BG, cfg->bgcolor);

	_reg_write(reg_base + REG_LDC_DST_BASE_Y, cfg->dst_y_base >> LDC_BASE_ADDR_SHIFT);
	_reg_write(reg_base + REG_LDC_DST_BASE_C, cfg->dst_c_base >> LDC_BASE_ADDR_SHIFT);
	_reg_write(reg_base + REG_LDC_DST_MODE, cfg->dst_mode);

	_reg_write(reg_base + REG_LDC_INT_SEL, 0); // 0: ldc intr, 1: cmd intr

	// start ldc
	_reg_write(reg_base + REG_LDC_START, 1);

	// There should be a frame_down intr afterwards.
#ifdef ENV_EMU
	for (int i = 0 ; i < 0x400; i += 4) {
		if (i == 0x280)
			i = 0x300;
		else if (i == 0x380)
			break;
		printf("0x%x: 0x%x\n", i,
		       _reg_read(reg_base + i + REG_ldc_BASE));
	}
#endif
}

/**
 * ldc_is_busy - check if ldc's operation is finished.
 *              ldc_start can only be toggled if only dma done(frame_done intr),
 *              ow dma won't finished.
 */
bool ldc_is_busy(void)
{
	return false;
}

/**
 * ldc_engine_cmdq - start a ldc operation, wait frame_done intr after this.
 *                   If output target is scaler, scaler's config should be done
 *                   before this.
 *
 * @param cfg: settings for cmdq
 * @param cnt: the number of settings
 * @param cmdq_addr: memory-address to put cmdq
 */
void ldc_engine_cmdq(const struct ldc_cfg *cfg, u64 cmdq_addr)
{
	u8 ras_mode = (cfg->dst_mode == LDC_DST_FLAT) ? 0 : 1;
	u8 cmd_idx = 0;
	union cmdq_set *cmd_start = (union cmdq_set *)(uintptr_t)cmdq_addr;

	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_DATA_FORMAT, cfg->pixel_fmt);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_RAS_MODE, ras_mode);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_RAS_XSIZE, cfg->ras_width);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_RAS_YSIZE, cfg->ras_height);

	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_MAP_BASE, cfg->map_base >> LDC_BASE_ADDR_SHIFT);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_MAP_BYPASS, cfg->map_bypass);

	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_BASE_Y, cfg->src_y_base >> LDC_BASE_ADDR_SHIFT);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_BASE_C, cfg->src_c_base >> LDC_BASE_ADDR_SHIFT);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_XSIZE, cfg->src_width);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_YSIZE, cfg->src_height);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_XSTART, cfg->src_xstart);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_XEND, cfg->src_xend);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_SRC_BG, cfg->bgcolor);

	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_DST_BASE_Y, cfg->dst_y_base >> LDC_BASE_ADDR_SHIFT);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_DST_BASE_C, cfg->dst_c_base >> LDC_BASE_ADDR_SHIFT);
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_DST_MODE, cfg->dst_mode);

	// Enable cmdq interrupt
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_INT_SEL, 1); // 0: ldc intr, 1: cmd intr
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_IRQEN, 1);

	// start ldc
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, reg_base + REG_LDC_START, 1);

#if 0
	s// wait end-inter(1)
	u32 flag_num = 1;   // 0: sc_str_flag, sc_stp_flag
	cmdQ_set_wait(&cmd_start[cmd_idx++], /*is_timer*/false, flag_num, 0);

	// clear intr
	cmdQ_set_package(&cmd_start[cmd_idx++].reg, REG_LDC_IRQCLR, 1);
#endif

	cmd_start[cmd_idx-1].reg.intr_end = 1;
	cmd_start[cmd_idx-1].reg.intr_last = 1;

	cmdQ_intr_ctrl(REG_LDC_CMDQ_BASE, 0x02);
	cmdQ_engine(REG_LDC_CMDQ_BASE, (uintptr_t)cmdq_addr,
		    REG_LDC_TOP_BASE >> 22, true, false, cmd_idx);
}

void ldc_get_sb_default(struct ldc_sb_cfg *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
}

void ldc_set_sb(struct ldc_sb_cfg *cfg)
{
	_reg_write(reg_base + REG_SB_MODE,    cfg->sb_mode);
	_reg_write(reg_base + REG_SB_NB,      cfg->sb_nb);
	_reg_write(reg_base + REG_SB_SIZE,    cfg->sb_size);
	_reg_write(reg_base + REG_SB_SW_CLR,  1);
	_reg_write(reg_base + REG_SB_SW_WPTR, cfg->sb_sw_wptr);
	_reg_write(reg_base + REG_SB_SET_STR, cfg->sb_set_str);

	_reg_write(reg_base + REG_LDC_DIR, cfg->ldc_dir);
}

void ldc_set_default_sb(void)
{
	struct ldc_sb_cfg ldc_sb_cfg;

	ldc_get_sb_default(&ldc_sb_cfg);
	ldc_set_sb(&ldc_sb_cfg);
}
