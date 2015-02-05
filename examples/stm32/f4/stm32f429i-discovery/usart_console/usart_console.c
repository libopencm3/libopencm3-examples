/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
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

/*
 * USART example (alternate console)
 */

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include "clock.h"

/*
 * Some definitions of our console "functions" attached to the
 * USART.
 *
 * These define sort of the minimum "library" of functions which
 * we can use on a serial port.
 */

#define CONSOLE_UART	USART1

void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);

/*
 * console_putc(char c)
 *
 * Send the character 'c' to the USART, wait for the USART
 * transmit buffer to be empty first.
 */
void console_putc(char c)
{
	uint32_t	reg;
	do {
		reg = USART_SR(CONSOLE_UART);
	} while ((reg & USART_SR_TXE) == 0);
	USART_DR(CONSOLE_UART) = (uint16_t) c & 0xff;
}

/*
 * char = console_getc(int wait)
 *
 * Check the console for a character. If the wait flag is
 * non-zero. Continue checking until a character is received
 * otherwise return 0 if called and no character was available.
 */
char console_getc(int wait)
{
	uint32_t	reg;
	do {
		reg = USART_SR(CONSOLE_UART);
	} while ((wait != 0) && ((reg & USART_SR_RXNE) == 0));
	return (reg & USART_SR_RXNE) ? USART_DR(CONSOLE_UART) : '\000';
}

/*
 * void console_puts(char *s)
 *
 * Send a string to the console, one character at a time, return
 * after the last character, as indicated by a NUL character, is
 * reached.
 */
void console_puts(char *s)
{
	while (*s != '\000') {
		console_putc(*s);
		/* Add in a carraige return, after sending line feed */
		if (*s == '\n') {
			console_putc('\r');
		}
		s++;
	}
}

/*
 * int console_gets(char *s, int len)
 *
 * Wait for a string to be entered on the console, limited
 * support for editing characters (back space and delete)
 * end when a <CR> character is received.
 */
int console_gets(char *s, int len)
{
	char *t = s;
	char c;

	*t = '\000';
	/* read until a <CR> is received */
	while ((c = console_getc(1)) != '\r') {
		if ((c == '\010') || (c == '\127')) {
			if (t > s) {
				/* send ^H ^H to erase previous character */
				console_puts("\010 \010");
				t--;
			}
		} else {
			*t = c;
			console_putc(c);
			if ((t - s) < len) {
				t++;
			}
		}
		/* update end of string with NUL */
		*t = '\000';
	}
	return t - s;
}

/*
 * Set up the GPIO subsystem with an "Alternate Function"
 * on some of the pins, in this case connected to a
 * USART.
 */
int main(void)
{
	char buf[128];
	int	len;

	clock_setup(); /* initialize our clock */

	/* MUST enable the GPIO clock in ADDITION to the USART clock */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* This example uses PA9 and PA10 for Tx and Rx respectively
	 * but other pins are available for this role on USART1 (our chosen
	 * USART) as it is connected to the programmer interface through
	 * jumpers.
	 */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);

	/* Actual Alternate function number (in this case 7) is part
	 * depenedent, check the data sheet for the right number to
	 * use.
	 */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	/* This then enables the clock to the USART1 peripheral which is
	 * attached inside the chip to the APB1 bus. Different peripherals
	 * attach to different buses, and even some UARTS are attached to
	 * APB1 and some to APB2, again the data sheet is useful here.
	 * We use the rcc_periph_clock_enable function that knows on which bus
	 * the peripheral is and sets things up accordingly.
	 */
	rcc_periph_clock_enable(RCC_USART1);

	/* Set up USART/UART parameters using the libopencm3 helper functions */
	usart_set_baudrate(CONSOLE_UART, 115200);
	usart_set_databits(CONSOLE_UART, 8);
	usart_set_stopbits(CONSOLE_UART, USART_STOPBITS_1);
	usart_set_mode(CONSOLE_UART, USART_MODE_TX_RX);
	usart_set_parity(CONSOLE_UART, USART_PARITY_NONE);
	usart_set_flow_control(CONSOLE_UART, USART_FLOWCONTROL_NONE);
	usart_enable(CONSOLE_UART);

	/* At this point our console is ready to go so we can create our
	 * simple application to run on it.
	 */
	console_puts("\nUART Demonstration Application\n");
	while (1) {
		console_puts("Enter a string: ");
		len = console_gets(buf, 128);
		if (len) {
			console_puts("\nYou entered : '");
			console_puts(buf);
			console_puts("'\n");
		} else {
			console_puts("\nNo string entered\n");
		}
	}
}
