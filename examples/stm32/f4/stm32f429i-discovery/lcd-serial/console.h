/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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

#ifndef __CONSOLE_H
#define __CONSOLE_H

/*
 * Some definitions of our console "functions" attached to the
 * USART.
 *
 * These define sort of the minimum "library" of functions which
 * we can use on a serial port. If you wish to use a different
 * USART there are several things to change:
 *	- CONSOLE_UART change this
 *	- Change the peripheral enable clock
 *	- add usartx_isr for interrupts
 *	- nvic_enable_interrupt(your choice of USART/UART)
 *	- GPIO pins for transmit/receive
 *		(may be on different alternate functions as well)
 */


#define CONSOLE_UART	USART1

/*
 * Our simple console definitions
 */

void console_putc(char c);
char console_getc(int wait);
void console_puts(char *s);
int console_gets(char *s, int len);
void console_setup(int baudrate);

/* this is for fun, if you type ^C to this example it will reset */
#define RESET_ON_CTRLC

#endif
