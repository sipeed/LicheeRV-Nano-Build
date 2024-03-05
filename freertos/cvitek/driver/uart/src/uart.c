//#include <stdint.h>
#include "hal_uart_dw.h"
#include "hal_pinmux.h"
#include "cvi_spinlock.h"
#include <stdint.h>
#include <stdbool.h>
#include <types.h>
#include "malloc.h"
#include "dump_uart.h"
#include "fast_image.h"

struct dump_uart_s *dump_uart;
int uart_putc_enable = 1;
extern struct transfer_config_t transfer_config;

void uart_init(void)
{
	int baudrate = 115200;
	int uart_clock = 25 * 1000 * 1000;

	/* set uart to pinmux_uart1 */
	//pinmux_config(PINMUX_UART0);

	hal_uart_init(UART0, baudrate, uart_clock);
}

uint8_t uart_putc(uint8_t ch)
{
	if (ch == '\n') {
		hal_uart_putc('\r');
	}
	hal_uart_putc(ch);
	return ch;
}

void uart_puts(char *str)
{
	if (!str)
		return;

	while (*str) {
		uart_putc(*str++);
	}
}

int uart_getc(void)
{
	return (int)hal_uart_getc();
}

int uart_tstc(void)
{
	return hal_uart_tstc();
}

DEFINE_CVI_SPINLOCK(printf_lock, SPIN_UART);

struct dump_uart_s *dump_uart_init(void)
{
	char * ptr;
	static int init_enable = 0;
	int dump_size;

	if (!init_enable) {
		if (transfer_config.dump_print_size_idx >= DUMP_PRINT_SZ_IDX_LIMIT ||
			transfer_config.dump_print_size_idx < DUMP_PRINT_SZ_IDX_4K)
			dump_size = 1 << DUMP_PRINT_SZ_IDX_4K;
		else
			dump_size = 1 << transfer_config.dump_print_size_idx;

	    ptr = (char *) malloc(dump_size + 0x40);
		if (ptr == 0)
			return 0;

		dump_uart = ((((unsigned long)ptr) + 0x3F) & ~0x3F);
	    dump_uart->dump_uart_pos = 0;
		dump_uart->dump_uart_overflow = 0;
		dump_uart->dump_uart_max_size = dump_size - sizeof(struct dump_uart_s);
		dump_uart->dump_uart_ptr = ((unsigned int) dump_uart) + sizeof(struct dump_uart_s);
		init_enable = 1;
	} else
		printf("transfer_config.dump_print_enable = %d, enable =%d\n", transfer_config.dump_print_enable, init_enable);

	if (transfer_config.dump_print_enable) {
		printf("dump_print_enable & log will not print\n");
		dump_uart_enable();
	}
	return dump_uart;
}

void dump_uart_enable(void)
{
	dump_uart->dump_uart_enable = 1;
	uart_putc_enable = 0;
}

void dump_uart_disable(void)
{
	dump_uart->dump_uart_enable = 0;
	uart_putc_enable = 1;
}

struct dump_uart_s *dump_uart_msg(void)
{
	int flags;
	int dump_size;

	if (transfer_config.dump_print_size_idx >= DUMP_PRINT_SZ_IDX_LIMIT ||
		transfer_config.dump_print_size_idx < DUMP_PRINT_SZ_IDX_4K)
		dump_size = 1 << DUMP_PRINT_SZ_IDX_4K;
	else
		dump_size = 1 << transfer_config.dump_print_size_idx;


	drv_spin_lock_irqsave(&printf_lock, flags);
	flush_dcache_range(dump_uart, dump_size);
	drv_spin_unlock_irqrestore(&printf_lock, flags);
	return dump_uart;
}

int uart_put_buff(char *buf)
{
	int flags;
	int count = 0;
	char *ptr;

	drv_spin_lock_irqsave(&printf_lock, flags);

	if (uart_putc_enable) {
		uart_puts("RT: ");

		while (buf[count]) {
			if (uart_putc(buf[count]) != '\n') {
				count++;
			} else {
				break;
	        }
		}
	} else {

		ptr = (char *)dump_uart->dump_uart_ptr;
		ptr[dump_uart->dump_uart_pos++] = 'R';
		ptr[dump_uart->dump_uart_pos++] = 'T';
		ptr[dump_uart->dump_uart_pos++] = 'D';
		ptr[dump_uart->dump_uart_pos++] = ':';
		ptr[dump_uart->dump_uart_pos++] = ' ';

		while (buf[count]) {
			if (buf[count] == '\n') {
				ptr[dump_uart->dump_uart_pos++] = '\r';
			}
			ptr[dump_uart->dump_uart_pos++] = buf[count];
			count++;
			if (dump_uart->dump_uart_pos >= dump_uart->dump_uart_max_size) {
				dump_uart->dump_uart_overflow = 1;
				dump_uart->dump_uart_pos = 0;
			}
		}
	}
	drv_spin_unlock_irqrestore(&printf_lock, flags);
	return count;
}

