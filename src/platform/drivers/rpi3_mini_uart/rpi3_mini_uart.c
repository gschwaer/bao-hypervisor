/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Gero Schwaericke <gero.schwaericke@tum.de>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#include <platform.h>
#include <cpu.h>

#include <drivers/rpi3_mini_uart.h>
// 0x3F215000

typedef struct {
	uint32_t gp_fsel0;   // 0000
	uint32_t gp_fsel1;   // 0004
	uint32_t gp_fsel2;   // 0008
	uint32_t gp_fsel3;   // 000C
	uint32_t gp_fsel4;   // 0010
	uint32_t gp_fsel5;   // 0014
	uint32_t reserved1;  // 0018
	uint32_t gp_set0;    // 001C
	uint32_t gp_set1;    // 0020
	uint32_t reserved2;  // 0024
	uint32_t gp_clr0;    // 0028
	uint32_t gp_clr1;    // 002C
	uint32_t reserved3;  // 0030
	uint32_t gp_lev0;    // 0034
	uint32_t gp_lev1;    // 0038
	uint32_t reserved4;  // 003C
	uint32_t gp_eds0;    // 0040
	uint32_t gp_eds1;    // 0044
	uint32_t reserved5;  // 0048
	uint32_t gp_ren0;    // 004C
	uint32_t gp_ren1;    // 0050
	uint32_t reserved6;  // 0054
	uint32_t gp_fen0;    // 0058
	uint32_t gp_fen1;    // 005C
	uint32_t reserved7;  // 0060
	uint32_t gp_hen0;    // 0064
	uint32_t gp_hen1;    // 0068
	uint32_t reserved8;  // 006C
	uint32_t gp_len0;    // 0070
	uint32_t gp_len1;    // 0074
	uint32_t reserved9;  // 0078
	uint32_t gp_aren0;   // 007C
	uint32_t gp_aren1;   // 0080
	uint32_t reserved10; // 0084
	uint32_t gp_afen0;   // 0088
	uint32_t gp_afen1;   // 008C
	uint32_t reserved11; // 0090
	uint32_t gp_pud;     // 0094
	uint32_t gp_pudclk0; // 0098
	uint32_t gp_pudclk1; // 009C
} volatile bao_gpio_t;

volatile bao_gpio_t gpio
    __attribute__((section(".devices"), aligned(PAGE_SIZE)));

/* Based on the uart code of @bztsrc at:
 * https://github.com/bztsrc/raspi3-tutorial/blob/master/03_uart1/uart.c */
void uart_init(volatile uart_rpi3_mini_t *uart)
{
	register uint32_t r;
	
	/* map gpio range */
	mem_map_dev(&cpu.as, (void*)&gpio, platform.console.gpio_base,
		    NUM_PAGES(sizeof(gpio)));

	uart->aux_enb |= 1;	  // enable UART1, AUX mini uart
	uart->aux_mu_cntl = 0;    // disable uart
	uart->aux_mu_lcr = 3;     // 8 bits
	uart->aux_mu_mcr = 0;
	uart->aux_mu_ier = 0;
	uart->aux_mu_iir = 0xc6;  // disable interrupts
	uart->aux_mu_baud = 270;  // 115200 baud
	
	/* map UART1 to GPIO pins */
	r = gpio.gp_fsel1;
	r &= ~((7<<12)|(7<<15));  // gpio14, gpio15
	r |= (2<<12)|(2<<15);     // alt5
	gpio.gp_fsel1 = r;
	gpio.gp_pud = 0;          // enable pins 14 and 15
	r=150; while(r--) { asm volatile("nop"); }
	gpio.gp_pudclk0 = (1<<14)|(1<<15);
	r=150; while(r--) { asm volatile("nop"); }
	gpio.gp_pudclk0 = 0;      // flush GPIO setup
}

void uart_enable(volatile uart_rpi3_mini_t *uart)
{
	uart->aux_mu_cntl = 3;    // enable Tx, Rx
}

void uart_putc(volatile uart_rpi3_mini_t *uart, int8_t c){
	/* wait until we can send */
	do{ asm volatile("nop"); } while (!(uart->aux_mu_lsr & 0x20));
	/* write the character to the buffer */
	uart->aux_mu_io = c;
}

void uart_puts(volatile uart_rpi3_mini_t *uart, char const* str)
{
	while (*str) {
		uart_putc(uart, *str++);
	}
}
