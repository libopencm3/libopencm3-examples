/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Karl Palsson <karlp@tweak.net.au>
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

#include <libopencm3/sam/memorymap.h>
#include <libopencm3/sam/d/port.h>

// led is on PA09

static void gpio_setup(void)
{
	PORT_DIR(PORTA) = (1<<9);
	PORT_PINCFG(PORTA, 9) = 0;
}

int main(void)
{
	int i, j;

	gpio_setup();

	while (1) {
		for (j = 0; j < 10; j++) {
			PORT_OUTTGL(PORTA) = 1<<9;
			for (i = 0; i < (5000 + (j) * 10000); i++) {	/* Wait a bit. */
				__asm__("nop");
			}
		}
	}

	return 0;
}
