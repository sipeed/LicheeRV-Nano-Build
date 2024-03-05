#include <linux/types.h>
#include <linux/delay.h>

#include <vip_common.h>
#include "reg.h"


static uintptr_t reg_base;
static struct spinlock lock;

void vip_set_base_addr(void *base)
{
	reg_base = (uintptr_t)base;
	spin_lock_init(&lock);
}
EXPORT_SYMBOL_GPL(vip_set_base_addr);

union vip_sys_clk vip_get_clk_lp(void)
{
	union vip_sys_clk clk;

	clk.raw = _reg_read(reg_base + VIP_SYS_VIP_CLK_LP);
	return clk;
}
EXPORT_SYMBOL_GPL(vip_get_clk_lp);

void vip_set_clk_lp(union vip_sys_clk clk)
{
	_reg_write(reg_base + VIP_SYS_VIP_CLK_LP, clk.raw);
}
EXPORT_SYMBOL_GPL(vip_set_clk_lp);

union vip_sys_clk_ctrl0 vip_get_clk_ctrl0(void)
{
	union vip_sys_clk_ctrl0 cfg;

	cfg.raw = _reg_read(reg_base + VIP_SYS_VIP_CLK_CTRL0);
	return cfg;
}
EXPORT_SYMBOL_GPL(vip_get_clk_ctrl0);

void vip_set_clk_ctrl0(union vip_sys_clk_ctrl0 cfg)
{
	_reg_write(reg_base + VIP_SYS_VIP_CLK_CTRL0, cfg.raw);
}
EXPORT_SYMBOL_GPL(vip_set_clk_ctrl0);

union vip_sys_reset vip_get_reset(void)
{
	union vip_sys_reset reset;

	reset.raw = _reg_read(reg_base + VIP_SYS_VIP_RESETS);
	return reset;
}
EXPORT_SYMBOL_GPL(vip_get_reset);

void vip_set_reset(union vip_sys_reset reset)
{
	_reg_write(reg_base + VIP_SYS_VIP_RESETS, reset.raw);
}
EXPORT_SYMBOL_GPL(vip_set_reset);

/**
 * vip_toggle_reset - enable/disable reset specified in mask. Lock protected.
 *
 * @param mask: resets want to be toggled.
 */
void vip_toggle_reset(union vip_sys_reset mask)
{
	union vip_sys_reset value;
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	value = vip_get_reset();
	value.raw |= mask.raw;
	vip_set_reset(value);

	udelay(20);
	value.raw &= ~mask.raw;
	vip_set_reset(value);
	spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL_GPL(vip_toggle_reset);

union vip_sys_intr vip_get_intr_status(void)
{
	union vip_sys_intr intr;

	intr.raw = _reg_read(reg_base + VIP_SYS_VIP_INT);
	return intr;
}
EXPORT_SYMBOL_GPL(vip_get_intr_status);

unsigned int vip_sys_reg_read(uintptr_t addr)
{
	return _reg_read(reg_base + addr);
}
EXPORT_SYMBOL_GPL(vip_sys_reg_read);

void vip_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	_reg_write_mask(reg_base + addr, mask, data);
}
EXPORT_SYMBOL_GPL(vip_sys_reg_write_mask);

/**
 * vip_sys_set_offline - control vip axi channel attribute, realtime/offline.
 *
 * @param bus: axi bus to control.
 * @param offline: true: offline; false: realtime.
 */
void vip_sys_set_offline(enum vip_sys_axi_bus bus, bool offline)
{
	u32 mask = BIT(bus);
	u32 value = (offline) ? mask : ~mask;

	_reg_write_mask(reg_base + VIP_SYS_VIP_AXI_SW, mask, value);
}
EXPORT_SYMBOL_GPL(vip_sys_set_offline);
