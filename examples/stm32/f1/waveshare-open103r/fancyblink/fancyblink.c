/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Damjan Marion <damjan.marion@gmail.com>
 * Copyright (C) 2011 Mark Panajotovic <marko@electrontube.org>
 * Copyright (C) 2013 Joshua Harlan Lifton <joshua.harlan.lifton@gmail.com>
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

/* Set STM32 to 24 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_24MHZ]);

	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOC);
}

static void gpio_setup(void)
{
	/* Set GPIO9-12 (in GPIO port C) to 'output push-pull'. */
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL,
		      GPIO9 | GPIO10 | GPIO11 | GPIO12);
}

int main(void)
{
	int i;

	clock_setup();
	gpio_setup();

	/* Set two LEDs for wigwag effect when toggling. */
	gpio_set(GPIOC, GPIO9 | GPIO11);

	/* Blink the LEDs (PC9 - PC12) on the board. */
	while (1) {
		gpio_toggle(GPIOC, GPIO9 | GPIO10 | GPIO11 | GPIO12);
		for (i = 0; i < 2000000; i++)      /* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
