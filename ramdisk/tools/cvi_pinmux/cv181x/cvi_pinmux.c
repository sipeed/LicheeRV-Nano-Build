#include "devmem.h"
#include "func.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
#define PINMUX_BASE 0x03001000
#define INVALID_PIN 9999

struct pinlist {
	char name[32];
	uint32_t offset;
} pinlist_st;

struct pinlist cv181x_pin[] = {
	{ "CAM_MCLK0", 0x0 },
	{ "CAM_PD0", 0x4 },
	{ "CAM_RST0", 0x8 },
	{ "CAM_MCLK1", 0xc },
	{ "CAM_PD1", 0x10 },
	{ "IIC3_SCL", 0x14 },
	{ "IIC3_SDA", 0x18 },
	{ "SD0_CLK", 0x1c },
	{ "SD0_CMD", 0x20 },
	{ "SD0_D0", 0x24 },
	{ "SD0_D1", 0x28 },
	{ "SD0_D2", 0x2c },
	{ "SD0_D3", 0x30 },
	{ "SD0_CD", 0x34 },
	{ "SD0_PWR_EN", 0x38 },
	{ "SPK_EN", 0x3c },
	{ "UART0_TX", 0x40 },
	{ "UART0_RX", 0x44 },
	{ "EMMC_RSTN", 0x48 },
	{ "EMMC_DAT2", 0x4c },
	{ "EMMC_CLK", 0x50 },
	{ "EMMC_DAT0", 0x54 },
	{ "EMMC_DAT3", 0x58 },
	{ "EMMC_CMD", 0x5c },
	{ "EMMC_DAT1", 0x60 },
	{ "JTAG_CPU_TMS", 0x64 },
	{ "JTAG_CPU_TCK", 0x68 },
	{ "JTAG_CPU_TRST", 0x6c },
	{ "IIC0_SCL", 0x70 },
	{ "IIC0_SDA", 0x74 },
	{ "AUX0", 0x78 },
	{ "PWR_VBAT_DET", 0x7c },
	{ "PWR_RSTN", 0x80 },
	{ "PWR_SEQ1", 0x84 },
	{ "PWR_SEQ2", 0x88 },
	{ "PWR_SEQ3", 0x8c },
	{ "PWR_WAKEUP0", 0x90 },
	{ "PWR_WAKEUP1", 0x94 },
	{ "PWR_BUTTON1", 0x98 },
	{ "PWR_ON", 0x9c },
	{ "XTAL_XIN", 0xa0 },
	{ "PWR_GPIO0", 0xa4 },
	{ "PWR_GPIO1", 0xa8 },
	{ "PWR_GPIO2", 0xac },
	{ "CLK32K", 0xb0 },
	{ "CLK25M", 0xb4 },
	{ "IIC2_SCL", 0xb8 },
	{ "IIC2_SDA", 0xbc },
	{ "UART2_TX", 0xc0 },
	{ "UART2_RTS", 0xc4 },
	{ "UART2_RX", 0xc8 },
	{ "UART2_CTS", 0xcc },
	{ "SD1_D3", 0xd0 },
	{ "SD1_D2", 0xd4 },
	{ "SD1_D1", 0xd8 },
	{ "SD1_D0", 0xdc },
	{ "SD1_CMD", 0xe0 },
	{ "SD1_CLK", 0xe4 },
	{ "RSTN", 0xe8 },
	{ "PWM0_BUCK", 0xec },
	{ "ADC3", 0xf0 },
	{ "ADC2", 0xf4 },
	{ "ADC1", 0xf8 },
	{ "USB_ID", 0xfc },
	{ "USB_VBUS_EN", 0x100 },
	{ "PKG_TYPE0", 0x104 },
	{ "USB_VBUS_DET", 0x108 },
	{ "PKG_TYPE1", 0x10c },
	{ "PKG_TYPE2", 0x110 },
	{ "MUX_SPI1_MISO", 0x114 },
	{ "MUX_SPI1_MOSI", 0x118 },
	{ "MUX_SPI1_CS", 0x11c },
	{ "MUX_SPI1_SCK", 0x120 },
	{ "PAD_ETH_TXP", 0x124 },
	{ "PAD_ETH_TXM", 0x128 },
	{ "PAD_ETH_RXP", 0x12c },
	{ "PAD_ETH_RXM", 0x130 },
	{ "VIVO_D10", 0x134 },
	{ "VIVO_D9", 0x138 },
	{ "VIVO_D8", 0x13c },
	{ "VIVO_D7", 0x140 },
	{ "VIVO_D6", 0x144 },
	{ "VIVO_D5", 0x148 },
	{ "VIVO_D4", 0x14c },
	{ "VIVO_D3", 0x150 },
	{ "VIVO_D2", 0x154 },
	{ "VIVO_D1", 0x158 },
	{ "VIVO_D0", 0x15c },
	{ "VIVO_CLK", 0x160 },
	{ "PAD_MIPIRX5N", 0x164 },
	{ "PAD_MIPIRX5P", 0x168 },
	{ "PAD_MIPIRX4N", 0x16c },
	{ "PAD_MIPIRX4P", 0x170 },
	{ "PAD_MIPIRX3N", 0x174 },
	{ "PAD_MIPIRX3P", 0x178 },
	{ "PAD_MIPIRX2N", 0x17c },
	{ "PAD_MIPIRX2P", 0x180 },
	{ "PAD_MIPIRX1N", 0x184 },
	{ "PAD_MIPIRX1P", 0x188 },
	{ "PAD_MIPIRX0N", 0x18c },
	{ "PAD_MIPIRX0P", 0x190 },
	{ "PAD_MIPI_TXM4", 0x194 },
	{ "PAD_MIPI_TXP4", 0x198 },
	{ "PAD_MIPI_TXM3", 0x19c },
	{ "PAD_MIPI_TXP3", 0x1a0 },
	{ "PAD_MIPI_TXM2", 0x1a4 },
	{ "PAD_MIPI_TXP2", 0x1a8 },
	{ "PAD_MIPI_TXM1", 0x1ac },
	{ "PAD_MIPI_TXP1", 0x1b0 },
	{ "PAD_MIPI_TXM0", 0x1b4 },
	{ "PAD_MIPI_TXP0", 0x1b8 },
	{ "PAD_AUD_AINL_MIC", 0x1bc },
	{ "PAD_AUD_AINR_MIC", 0x1c0 },
	{ "PAD_AUD_AOUTL", 0x1c4 },
	{ "PAD_AUD_AOUTR", 0x1c8 },
	{ "GPIO_RTX", 0x1cc },
	{ "GPIO_ZQ", 0x1d0 },
	{ "UART0_IP_SEL", 0x1d4 },
	{ "UART1_IP_SEL", 0x1d8 },
	{ "UART2_IP_SEL", 0x1dc },
	{ "UART3_IP_SEL", 0x1e0 },
{ "UART4_IP_SEL", 0x1e4 },
};

uint32_t convert_func_to_value(char *pin, char *func)
{
	uint32_t i = 0;
	uint32_t max_fun_num = NELEMS(cv181x_pin_func);
	char v;

	for (i = 0; i < max_fun_num; i++) {
		if (strcmp(cv181x_pin_func[i].func, func) == 0) {
			if (strncmp(cv181x_pin_func[i].name, pin, strlen(pin)) == 0) {
				v = cv181x_pin_func[i].name[strlen(cv181x_pin_func[i].name) - 1];
				break;
			}
		}
	}

	if (i == max_fun_num) {
		printf("ERROR: invalid pin or func\n");
		return INVALID_PIN;
	}

	return (v - 0x30);
}

void print_fun(char *name, uint32_t value)
{
	uint32_t i = 0;
	uint32_t max_fun_num = NELEMS(cv181x_pin_func);
	char pinname[128];

	sprintf(pinname, "%s%d", name, value);

	printf("%s function:\n", name);
	for (i = 0; i < max_fun_num; i++) {
		if (strncmp(pinname, cv181x_pin_func[i].name, strlen(name)) == 0) {
			if (strcmp(pinname, cv181x_pin_func[i].name) == 0)
				printf("[v] %s\n", cv181x_pin_func[i].func);
			else
				printf("[ ] %s\n", cv181x_pin_func[i].func);
			// break;
		}
	}
	printf("\n");
}

void print_usage(void)
{
	printf("cvi_pinmux for cv181x\n");
	printf("./cvi_pinmux -p          <== List all pins\n");
	printf("./cvi_pinmux -l          <== List all pins and its func\n");
	printf("./cvi_pinmux -r pin      <== Get func from pin\n");
	printf("./cvi_pinmux -w pin/func <== Set func to pin\n");
}

int main(int argc, char *argv[])
{
	int opt = 0;
	uint32_t i = 0;
	uint32_t value;
	char pin[32];
	char func[32];
	uint32_t f_val;

	if (argc == 1) {
		print_usage();
		return 1;
	}

	while ((opt = getopt(argc, argv, "hplr:w:")) != -1) {
		switch (opt) {
		case 'r':
			for (i = 0; i < NELEMS(cv181x_pin); i++) {
				if (strcmp(optarg, cv181x_pin[i].name) == 0)
					break;
			}
			if (i != NELEMS(cv181x_pin)) {
				value = devmem_readl(PINMUX_BASE + cv181x_pin[i].offset);
				// printf("value %d\n", value);
				print_fun(optarg, value);

				printf("register: 0x%x\n", PINMUX_BASE + cv181x_pin[i].offset);
				printf("value: %d\n", value);
			} else {
				printf("\nInvalid option: %s", optarg);
			}
			break;

		case 'w':
			// printf("optarg %s\n", optarg);
			if (sscanf(optarg, "%[^/]/%s", pin, func) != 2)
				print_usage();

			printf("pin %s\n", pin);
			printf("func %s\n", func);

			for (i = 0; i < NELEMS(cv181x_pin); i++) {
				if (strcmp(pin, cv181x_pin[i].name) == 0)
					break;
			}

			if (i != NELEMS(cv181x_pin)) {
				f_val = convert_func_to_value(pin, func);
				if (f_val == INVALID_PIN)
					return 1;
				devmem_writel(PINMUX_BASE + cv181x_pin[i].offset, f_val);

				printf("register: %x\n", PINMUX_BASE + cv181x_pin[i].offset);
				printf("value: %d\n", f_val);
				// printf("value %d\n", value);
			} else {
				printf("\nInvalid option: %s\n", optarg);
			}
			break;

		case 'p':
			printf("Pinlist:\n");
			for (i = 0; i < NELEMS(cv181x_pin); i++)
				printf("%s\n", cv181x_pin[i].name);
			break;

		case 'l':
			for (i = 0; i < NELEMS(cv181x_pin); i++) {
				value = devmem_readl(PINMUX_BASE + cv181x_pin[i].offset);
				// printf("value %d\n", value);
				print_fun(cv181x_pin[i].name, value);
			}
			break;

		case 'h':
			print_usage();
			break;

		case '?':
			print_usage();
			break;

		default:
			print_usage();
			break;
		}
	}

	return 0;
}
