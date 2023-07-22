/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <stdio.h>

#define PORT_LED GPIOC
#define PIN_LED GPIO6

static void clock_setup(void)
{
	/* Enable GPIOC clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART. */
	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(void)
{
	/* Setup USART parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO8/9 on GPIO port C for LEDs. */
	gpio_mode_setup(PORT_LED, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PIN_LED);

	/* Setup GPIO pins for USART transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);

	/* Setup USART TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
}

void uart_out(char* data) {
    while(*data) {
	    usart_send_blocking(USART2, *data++); /* USART2: Send byte. */
    }
}

int main(void)
{
	int i, j = 0, c = 0;
    char buf[256];

	clock_setup();
	gpio_setup();
	usart_setup();
    sprintf(buf, "Hello world\r\n");
    uart_out(buf);

	/* Blink the LED on the board with every transmitted byte. */
	while (1) {
		gpio_toggle(PORT_LED, PIN_LED);	/* LED on/off */
		usart_send_blocking(USART2, c + '0'); /* USART2: Send byte. */
		c = (c == 9) ? 0 : c + 1;	/* Increment c. */
		if ((j++ % 80) == 0) {		/* Newline after line full. */
			usart_send_blocking(USART2, '\r');
			usart_send_blocking(USART2, '\n');
		}
		for (i = 0; i < 100000; i++) {	/* Wait a bit. */
			__asm__("NOP");
		}
	}

	return 0;
}
