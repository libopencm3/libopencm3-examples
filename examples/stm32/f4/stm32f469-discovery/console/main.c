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
#include "../util/clock.h"
#include "../util/console.h"

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

	console_setup(115200);

	/* At this point our console is ready to go so we can create our
	 * simple application to run on it.
	 */
	console_puts("\nUART Demonstration Application\n");
	while (1) {
		console_puts("Enter a string: ");
		len = console_gets(buf, 128);
		if (len > 1) {
			buf[len-1] = 0; /* kill trailing newline */
			console_puts("\nYou entered : '");
			console_puts(buf);
			console_puts("'\n");
		} else {
			console_puts("\nNo string entered\n");
		}
	}
}
