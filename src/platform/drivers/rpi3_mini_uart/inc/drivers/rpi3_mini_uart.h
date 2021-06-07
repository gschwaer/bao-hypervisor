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

#ifndef RPI3_MINI_UART_H
#define RPI3_MINI_UART_H

#include <bao.h>
#include <plat/platform.h>

typedef struct {
	uint32_t aux_irq;        // 5000
	uint32_t aux_enb;        // 5004
	uint8_t reserved[0x38];  // 5008
	uint32_t aux_mu_io;      // 5040
	uint32_t aux_mu_ier;     // 5044
	uint32_t aux_mu_iir;     // 5048
	uint32_t aux_mu_lcr;     // 504C
	uint32_t aux_mu_mcr;     // 5050
	uint32_t aux_mu_lsr;     // 5054
	uint32_t aux_mu_msr;     // 5058
	uint32_t aux_mu_scratch; // 505C
	uint32_t aux_mu_cntl;    // 5060
	uint32_t aux_mu_stat;    // 5064
	uint32_t aux_mu_baud;    // 5068
} volatile uart_rpi3_mini_t;

typedef uart_rpi3_mini_t bao_uart_t;

void uart_enable(volatile uart_rpi3_mini_t *uart);
void uart_init(volatile uart_rpi3_mini_t *uart);
void uart_puts(volatile uart_rpi3_mini_t *uart, const char* str);

#endif /* RPI3_MINI_UART_H */
