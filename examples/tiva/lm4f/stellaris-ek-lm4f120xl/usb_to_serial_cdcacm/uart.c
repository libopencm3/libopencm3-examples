/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2012-2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \addtogroup Examples
 *
 */
/*
 * Pins handled by hw peripheral:
 * Tx  <-> PB1
 * Rx  <-> PB0
 * Input pins handled manually via interrupts:
 * DCD <-> PA2
 * DSR <-> PA3
 * RI  <-> PA4
 * CTS <-> PA5 (UNUSED)
 * Output pins handled via commands from the host:
 * DTR <-> PA6
 * RTS <-> PA7
 */

#include "usb_to_serial_cdcacm.h"

#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/uart.h>
#include <libopencm3/lm4f/nvic.h>

static void uart_ctl_line_setup(void)
{
	uint32_t inpins, outpins;

	inpins = PIN_DCD | PIN_DSR | PIN_RI | PIN_CTS;
	outpins = PIN_DTR | PIN_RTS;

	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, outpins);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, inpins);
}

void uart_init(void)
{
	/* Enable GPIOA and GPIOB in run mode. */
	periph_clock_enable(RCC_GPIOB);
	periph_clock_enable(RCC_GPIOA);
	uart_ctl_line_setup();
	/* Mux PB0 and PB1 to AF1 (UART1 in this case) */
	gpio_set_af(GPIOB, 1, GPIO0 | GPIO1);

	/* Enable the UART clock */
	periph_clock_enable(RCC_UART1);
	/* Disable the UART while we mess with its setings */
	uart_disable(UART1);
	/* Configure the UART clock source */
	uart_clock_from_piosc(UART1);
	/* We don't make any other settings here. */
	uart_enable(UART1);

	/* Ping us when something comes in */
	uart_enable_rx_interrupt(UART1);
	nvic_enable_irq(NVIC_UART1_IRQ);
}

uint8_t uart_get_ctl_line_state(void)
{
	return gpio_read(GPIOA, PIN_RI | PIN_DSR | PIN_DCD);
}

void uart_set_ctl_line_state(uint8_t dtr, uint8_t rts)
{
	uint8_t val = 0;

	val |= dtr ? PIN_DTR : 0;
	val |= rts ? PIN_RTS : 0;

	gpio_write(GPIOA, PIN_DTR | PIN_RTS, val);
}

void uart1_isr(void)
{
	uint8_t rx;
	rx = uart_recv(UART1);
	glue_data_received_cb(&rx, 1);
	uart_clear_interrupt_flag(UART1, UART_INT_RX);
}
