#include <cpu.h>
#include <mmio.h>
#include <debug.h>
#include <assert.h>
#include <errno.h>
#include <bl_common.h>
#include <platform.h>
#include <delay_timer.h>
#include <console.h>
#include <string.h>

#include <cv180x_pinlist_swconfig.h>
#include <cv180x_reg_fmux_gpio.h>
// #include <cv180x_reg_ioblk_G7.h>
// #include <cv180x_reg_ioblk_G10.h>

#include <security/security.h>

// #include <cv_spinor.h>
// #include <cv_spi_nand.h>
// #include <spi_nand.h>
#include <cv_usb.h>
// #include <cv_sd.h>
#include <cv_usb.h>
// #include <sd.h>
// #include <ff.h>

#define PINMUX_MASK(PIN_NAME) FMUX_GPIO_FUNCSEL_##PIN_NAME##_MASK
#define PINMUX_OFFSET(PIN_NAME) FMUX_GPIO_FUNCSEL_##PIN_NAME##_OFFSET
#define PINMUX_VALUE(PIN_NAME, FUNC_NAME) PIN_NAME##__##FUNC_NAME
#define PINMUX_CONFIG(PIN_NAME, FUNC_NAME)                                                                             \
	mmio_clrsetbits_32(PINMUX_BASE + FMUX_GPIO_FUNCSEL_##PIN_NAME,                                                 \
			   PINMUX_MASK(PIN_NAME) << PINMUX_OFFSET(PIN_NAME), PINMUX_VALUE(PIN_NAME, FUNC_NAME))


// Field of EFUSE_CUSTOMER
#define EC_FASTBOOT_SHIFT 0x0
#define EC_FASTBOOT_MASK 0x7

#define EC_GPIO_VALUE_SHIFT 0x4
#define EC_GPIO_VALUE_MASK 0x1

#define EC_FMUX_VALUE_SHIFT 0x5
#define EC_FMUX_VALUE_MASK 0x7

#define EC_FMUX_OFFSET_SHIFT 0x8
#define EC_FMUX_OFFSET_MASK 0xFF

#define EC_GPIO_OFFSET_SHIFT 0x10
#define EC_GPIO_OFFSET_MASK 0x1F

#define EC_GPIO_PORT_SHIFT 0x15
#define EC_GPIO_PORT_MASK 0x7

uint8_t gpio_in_value(uint32_t value)
{
	uint32_t fmux_value;
	uint32_t fmux_offset;
	uint32_t gpio_offset;
	uint32_t gpio_port;
	uint32_t gpio_reg_addr;
	uint32_t gpio_value;
	uint32_t select_value;

	fmux_value = GET_FIELD(value, EC_FMUX_VALUE_MASK, EC_FMUX_VALUE_SHIFT);
	fmux_offset = GET_FIELD(value, EC_FMUX_OFFSET_MASK, EC_FMUX_OFFSET_SHIFT);
	gpio_offset = GET_FIELD(value, EC_GPIO_OFFSET_MASK, EC_GPIO_OFFSET_SHIFT);
	gpio_port = GET_FIELD(value, EC_GPIO_PORT_MASK, EC_GPIO_PORT_SHIFT);
	gpio_value = GET_FIELD(value, EC_GPIO_VALUE_MASK, EC_GPIO_VALUE_SHIFT);

	mmio_write_32(PINMUX_BASE + fmux_offset * 4, fmux_value);
	udelay(10);

	if (gpio_port <= 3)
		gpio_reg_addr = GPIO_BASE + gpio_port * 0x1000 + 0x50;
	else
		gpio_reg_addr = RTC_GPIO_BASE + 0x50;

	select_value = (mmio_read_32(gpio_reg_addr) >> gpio_offset) & 0x1;

	return gpio_value ? !select_value : select_value;
}

#define TOP_BASE 0x03000000
#define EFUSE_BASE (TOP_BASE + 0x00050000)
#define EFUSE_SHADOW_REG (EFUSE_BASE + 0x100)
#define EFUSE_CUSTOMER (EFUSE_SHADOW_REG + 0x04)
uint8_t usb_id_det(void)
{
	uint32_t value;
	uint32_t fastboot;

	value = mmio_read_32(EFUSE_CUSTOMER);
	fastboot = GET_FIELD(value, EC_FASTBOOT_MASK, EC_FASTBOOT_SHIFT);
	if ((fastboot & BIT(2)) == BIT(2))
		return gpio_in_value(value);
	else
		return ((mmio_read_32(REG_TOP_CONF_INFO) & BIT_TOP_USB_ID) >> SHIFT_TOP_USB_ID);
}

static int is_download_gpio_set(void)
{
	return !usb_id_det();
}

int is_usb_dl_enabled(void)
{
	if (get_sw_info()->usd_dl == DOWNLOAD_DISABLE)
		return 0;

	if (get_sw_info()->usd_dl == DOWNLOAD_BUTTON)
		return is_download_gpio_set();

	return 1;
}
