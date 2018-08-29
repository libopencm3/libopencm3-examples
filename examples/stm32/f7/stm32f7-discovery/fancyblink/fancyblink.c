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

static void clock_setup(void) 
{
	rcc_clock_setup_hse_3v3(&hse_25mhz_3v3[CLOCK_3V3_216MHZ]);
	rcc_periph_clock_enable(RCC_GPIOI);
}

static void gpio_setup(void)
{

	gpio_mode_setup(GPIOI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
}

int main(void)
{
	int i;
	
	clock_setup();
	gpio_setup();

	/* Blink the LED (PI1) on the board. */
	while (1) {

		/* Using API function gpio_toggle(): */
		gpio_toggle(GPIOI, GPIO1);	/* LED on/off */
		for (i = 0; i < 13500000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
