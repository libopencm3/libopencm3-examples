/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2012 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/uart.h>
#include <libopencm3/lm4f/nvic.h>

static void uart_setup(void)
{
	/* Enable GPIOA in run mode. */
	periph_clock_enable(RCC_GPIOA);
	/* Mux PA0 and PA1 to UART0 (alternate function 1) */
	gpio_set_af(GPIOA, 1, GPIO0 | GPIO1);

	/* Enable the UART clock */
	periph_clock_enable(RCC_UART0);
	/* We need a brief delay before we can access UART config registers */
	__asm__("nop");
	/* Disable the UART while we mess with its setings */
	uart_disable(UART0);
	/* Configure the UART clock source as precision internal oscillator */
	uart_clock_from_piosc(UART0);
	/* Set communication parameters */
	uart_set_baudrate(UART0, 921600);
	uart_set_databits(UART0, 8);
	uart_set_parity(UART0, UART_PARITY_NONE);
	uart_set_stopbits(UART0, 1);
	/* Now that we're done messing with the settings, enable the UART */
	uart_enable(UART0);
}

static void uart_irq_setup(void)
{
	/* Gimme and RX interrupt */
	uart_enable_rx_interrupt(UART0);
	/* Make sure the interrupt is routed through the NVIC */
	nvic_enable_irq(NVIC_UART0_IRQ);
}

/*
 * uart0_isr is declared as a weak function. When we override it here, the
 * libopencm3 build system takes care that it becomes our UART0 ISR.
 */
void uart0_isr(void)
{
	uint8_t rx;
	uint32_t irq_clear = 0;

	if (uart_is_interrupt_source(UART0, UART_INT_RX)) {
		rx = uart_recv(UART0);
		uart_send(UART0, rx);
		irq_clear |= UART_INT_RX;
	}

	uart_clear_interrupt_flag(UART0, irq_clear);
}

int main(void)
{
	gpio_enable_ahb_aperture();
	uart_setup();
	uart_irq_setup();

	/*
	 * Not worrying about the UART here.
	 */
	while (1) {
		/* Do some other task */
	}

	return 0;
}
