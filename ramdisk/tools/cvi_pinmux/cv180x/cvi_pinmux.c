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

struct pinlist cv180x_pin[] = {
	{ "SD0_CLK", 0x0 },
	{ "SD0_CMD", 0x4 },
	{ "SD0_D0", 0x8 },
	{ "SD0_D1", 0xc },
	{ "SD0_D2", 0x10 },
	{ "SD0_D3", 0x14 },
	{ "SD0_CD", 0x18 },
	{ "SD0_PWR_EN", 0x1c },
	{ "SPK_EN", 0x20 },
	{ "UART0_TX", 0x24 },
	{ "UART0_RX", 0x28 },
	{ "SPINOR_HOLD_X", 0x2c },
	{ "SPINOR_SCK", 0x30 },
	{ "SPINOR_MOSI", 0x34 },
	{ "SPINOR_WP_X", 0x38 },
	{ "SPINOR_MISO", 0x3c },
	{ "SPINOR_CS_X", 0x40 },
	{ "JTAG_CPU_TMS", 0x44 },
	{ "JTAG_CPU_TCK", 0x48 },
	{ "IIC0_SCL", 0x4c },
	{ "IIC0_SDA", 0x50 },
	{ "AUX0", 0x54 },
	{ "GPIO_ZQ", 0x58 },
	{ "PWR_VBAT_DET", 0x5c },
	{ "PWR_RSTN", 0x60 },
	{ "PWR_SEQ1", 0x64 },
	{ "PWR_SEQ2", 0x68 },
	{ "PWR_WAKEUP0", 0x6c },
	{ "PWR_BUTTON1", 0x70 },
	{ "XTAL_XIN", 0x74 },
	{ "PWR_GPIO0", 0x78 },
	{ "PWR_GPIO1", 0x7c },
	{ "PWR_GPIO2", 0x80 },
	{ "SD1_GPIO1", 0x84 },
	{ "SD1_GPIO0", 0x88 },
	{ "SD1_D3", 0x8c },
	{ "SD1_D2", 0x90 },
	{ "SD1_D1", 0x94 },
	{ "SD1_D0", 0x98 },
	{ "SD1_CMD", 0x9c },
	{ "SD1_CLK", 0xa0 },
	{ "PWM0_BUCK", 0xa4 },
	{ "ADC1", 0xa8 },
	{ "USB_VBUS_DET", 0xac },
	{ "MUX_SPI1_MISO", 0xb0 },
	{ "MUX_SPI1_MOSI", 0xb4 },
	{ "MUX_SPI1_CS", 0xb8 },
	{ "MUX_SPI1_SCK", 0xbc },
	{ "PAD_ETH_TXP", 0xc0 },
	{ "PAD_ETH_TXM", 0xc4 },
	{ "PAD_ETH_RXP", 0xc8 },
	{ "PAD_ETH_RXM", 0xcc },
	{ "GPIO_RTX", 0xd0 },
	{ "PAD_MIPIRX4N", 0xd4 },
	{ "PAD_MIPIRX4P", 0xd8 },
	{ "PAD_MIPIRX3N", 0xdc },
	{ "PAD_MIPIRX3P", 0xe0 },
	{ "PAD_MIPIRX2N", 0xe4 },
	{ "PAD_MIPIRX2P", 0xe8 },
	{ "PAD_MIPIRX1N", 0xec },
	{ "PAD_MIPIRX1P", 0xf0 },
	{ "PAD_MIPIRX0N", 0xf4 },
	{ "PAD_MIPIRX0P", 0xf8 },
	{ "PAD_MIPI_TXM2", 0xfc },
	{ "PAD_MIPI_TXP2", 0x100 },
	{ "PAD_MIPI_TXM1", 0x104 },
	{ "PAD_MIPI_TXP1", 0x108 },
	{ "PAD_MIPI_TXM0", 0x10c },
	{ "PAD_MIPI_TXP0", 0x110 },
	{ "PAD_AUD_AINL_MIC", 0x120 },
	{ "PAD_AUD_AOUTR", 0x12c },
	{ "MUX_UART0_IP_SEL", 0x1d4 },
	{ "MUX_UART1_IP_SEL", 0x1d8 },
	{ "MUX_UART2_IP_SEL", 0x1dc },
	{ "MUX_UART3_IP_SEL", 0x1e0 },
	{ "MUX_UART4_IP_SEL", 0x1e4 },
};

uint32_t convert_func_to_value(char *pin, char *func)
{
	uint32_t i = 0;
	uint32_t max_fun_num = NELEMS(cv180x_pin_func);
	char v;

	for (i = 0; i < max_fun_num; i++) {
		if (strcmp(cv180x_pin_func[i].func, func) == 0) {
			if (strncmp(cv180x_pin_func[i].name, pin, strlen(pin)) == 0) {
				v = cv180x_pin_func[i].name[strlen(cv180x_pin_func[i].name) - 1];
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
	uint32_t max_fun_num = NELEMS(cv180x_pin_func);
	char pinname[128];

	sprintf(pinname, "%s%d", name, value);

	printf("%s function:\n", name);
	for (i = 0; i < max_fun_num; i++) {
		if (strncmp(pinname, cv180x_pin_func[i].name, strlen(name)) == 0) {
			if (strcmp(pinname, cv180x_pin_func[i].name) == 0)
				printf("[v] %s\n", cv180x_pin_func[i].func);
			else
				printf("[ ] %s\n", cv180x_pin_func[i].func);
			// break;
		}
	}
	printf("\n");
}

void print_usage(void)
{
	printf("cvi_pinmux for cv180x\n");
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
			for (i = 0; i < NELEMS(cv180x_pin); i++) {
				if (strcmp(optarg, cv180x_pin[i].name) == 0)
					break;
			}
			if (i != NELEMS(cv180x_pin)) {
				value = devmem_readl(PINMUX_BASE + cv180x_pin[i].offset);
				// printf("value %d\n", value);
				print_fun(optarg, value);

				printf("register: 0x%x\n", PINMUX_BASE + cv180x_pin[i].offset);
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

			for (i = 0; i < NELEMS(cv180x_pin); i++) {
				if (strcmp(pin, cv180x_pin[i].name) == 0)
					break;
			}

			if (i != NELEMS(cv180x_pin)) {
				f_val = convert_func_to_value(pin, func);
				if (f_val == INVALID_PIN)
					return 1;
				devmem_writel(PINMUX_BASE + cv180x_pin[i].offset, f_val);

				printf("register: %x\n", PINMUX_BASE + cv180x_pin[i].offset);
				printf("value: %d\n", f_val);
				// printf("value %d\n", value);
			} else {
				printf("\nInvalid option: %s\n", optarg);
			}
			break;

		case 'p':
			printf("Pinlist:\n");
			for (i = 0; i < NELEMS(cv180x_pin); i++)
				printf("%s\n", cv180x_pin[i].name);
			break;

		case 'l':
			for (i = 0; i < NELEMS(cv180x_pin); i++) {
				value = devmem_readl(PINMUX_BASE + cv180x_pin[i].offset);
				// printf("value %d\n", value);
				print_fun(cv180x_pin[i].name, value);
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
