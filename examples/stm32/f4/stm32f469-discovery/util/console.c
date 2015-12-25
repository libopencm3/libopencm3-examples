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
 * Console utility library - uses the serial port made available
 * by the debug port on the evaluation board as a default console.
 *
 * This version is interrupt driven.
 */

#include <stdint.h>
#include <ctype.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/cortex.h>
#include "../util/console.h"

/*
 * Some definitions of our console "functions" attached to the
 * USART.
 *
 * These define sort of the minimum "library" of functions which
 * we can use on a serial port.
 */

#define CONSOLE_UART	USART3

/* This is a ring buffer to holding characters as they are typed
 * it maintains both the place to put the next character received
 * from the UART, and the place where the last character was
 * read by the program.
 */

#define RECV_BUF_SIZE	128		/* Arbitrary buffer size */
char recv_buf[RECV_BUF_SIZE];
volatile int recv_ndx_nxt;		/* Next place to store */
volatile int recv_ndx_cur;		/* Next place to read */

/* For interrupt handling we add a new function which is called
 * when recieve interrupts happen. The name (usart3_isr) is created
 * by the irq.json file in libopencm3 calling this interrupt for
 * USART3 'usart3', adding the suffix '_isr', and then weakly binding
 * it to the 'do nothing' interrupt function in vec.c.
 *
 * By defining it in this file the linker will override that weak
 * binding and instead bind it here, but you have to get the name
 * right or it won't work. And you'll wonder where your interrupts
 * are going.
 */
void usart3_isr(void)
{
	uint32_t	reg;
	int			i;

	do {
		reg = USART_SR(CONSOLE_UART);
		if (reg & USART_SR_RXNE) {
			recv_buf[recv_ndx_nxt] = USART_DR(CONSOLE_UART);
#ifdef RESET_ON_CTRLC
			/*
			 * This bit of code will jump to the ResetHandler if you
			 * hit ^C
			 */
			if (recv_buf[recv_ndx_nxt] == '\003') {
				scb_reset_system();
				return; /* never actually reached */
			}
#endif
			/* Check for "overrun" */
			i = (recv_ndx_nxt + 1) % RECV_BUF_SIZE;
			if (i != recv_ndx_cur) {
				recv_ndx_nxt = i;
			}
		}
	/* can read back-to-back interrupts */
	} while ((reg & USART_SR_RXNE) != 0);
}

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
	char		c = 0;

	while ((wait != 0) && (recv_ndx_cur == recv_ndx_nxt));
	if (recv_ndx_cur != recv_ndx_nxt) {
		c = recv_buf[recv_ndx_cur];
		recv_ndx_cur = (recv_ndx_cur + 1) % RECV_BUF_SIZE;
	}
	return c;
}

/*
 * void console_puts(char *s)
 *
 * Send a string to the console, one character at a time, return
 * after the last character, as indicated by a NUL character, is
 * reached.
 *
 * Translate '\n' in the string (newline) to \n\r (newline + 
 * carraige return)
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
 * Wait for a string to be entered on the console, with
 * support for editing characters (delete letter, word,
 * entire line). It returns when the length is reached
 * or a carrige return is entered. <CR> is changed to newline
 * before the buffer is returned.
 */
int console_gets(char *s, int len)
{
	char *t = s;
	char c;

	*t = '\000';
	/* read until a <CR> is received */
	while (((c = console_getc(1)) != '\r') && ((t - s) < len) ) {
		if ((c == '\010') || (c == '\127')) {
			if (t > s) {
				/* send ^H ^H to erase previous character */
				console_puts("\010 \010");
				t--;
			}
		} else if (c == 0x17) {	// ^W erase a word
			while ((t > s) &&  (!(isspace((int) (*t))))) {
				t--;
				console_puts("\010 \010");
			}
		} else if (c == 0x15) { // ^U erase the line
			while (t > s) {
				t--;
				console_puts("\010 \010");
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
	if ((t < s) < len) {
		*t++ = '\n';
		*t = 0;
	}
	return t - s;
}

/*
 * Set up the GPIO subsystem with an "Alternate Function"
 * on some of the pins, in this case connected to a
 * USART.
 */
void console_setup(int baud)
{

	/* MUST enable the GPIO clock in ADDITION to the USART clock */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* This example uses PB10 and PB11 for Tx and Rx respectively
	 */
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO10);

	/* Actual Alternate function number (in this case 7) is part
	 * depenedent, check the data sheet for the right number to
	 * use.
	 */
	gpio_set_af(GPIOB, GPIO_AF7, GPIO11 | GPIO10);


	/* This then enables the clock to the USART3 peripheral.
	 */
	rcc_periph_clock_enable(RCC_USART3);

	/* Set up USART/UART parameters using the libopencm3 helper functions */
	usart_set_baudrate(CONSOLE_UART, baud);
	usart_set_databits(CONSOLE_UART, 8);
	usart_set_stopbits(CONSOLE_UART, USART_STOPBITS_1);
	usart_set_mode(CONSOLE_UART, USART_MODE_TX_RX);
	usart_set_parity(CONSOLE_UART, USART_PARITY_NONE);
	usart_set_flow_control(CONSOLE_UART, USART_FLOWCONTROL_NONE);
	usart_enable(CONSOLE_UART);

	/* Enable interrupts from the USART */
	nvic_enable_irq(NVIC_USART3_IRQ);

	/* Specifically enable recieve interrupts */
	usart_enable_rx_interrupt(CONSOLE_UART);
}

/*
 * Set a different baud rate for the console.
 */
void console_baud(int baud_rate)
{
	usart_set_baudrate(CONSOLE_UART, baud_rate);
}
