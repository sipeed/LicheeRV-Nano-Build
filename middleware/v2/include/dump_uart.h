/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: fast_image.h
 * Description:
 */

#ifndef __DUMP_UART_H__
#define __DUMP_UART_H__

#define DUMP_PRINT_DEFAULT_SIZE 0x1000

/* this structure should be modified both freertos & osdrv side */
struct dump_uart_s {
	uint64_t dump_uart_ptr;
	uint32_t  dump_uart_max_size;
	uint32_t  dump_uart_pos;
	uint8_t  dump_uart_enable;
	uint8_t dump_uart_overflow;
} __packed;

#ifndef __linux__
/* used for freertos */
struct dump_uart_s *dump_uart_init(void);
struct dump_uart_s *dump_uart_msg(void);
void dump_uart_enable(void);
void dump_uart_disable(void);
#endif
#endif // end of __DUMP_UART_H__

