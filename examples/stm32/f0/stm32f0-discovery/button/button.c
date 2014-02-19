/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
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

/* Set STM32 to 48 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();
}

static void gpio_setup(void)
{
	/* Enable GPIOD clock. */
	rcc_periph_clock_enable(RCC_GPIOC);


	/* Set GPIO12 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO8 | GPIO9);
}

static void button_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);


	/* Set GPIO0 (in GPIO port A) to 'input open-drain'. */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

int main(void)
{
	int i;

	clock_setup();
	button_setup();
	gpio_setup();

	/* Blink the LED (PD12) on the board. */
	while (1) {
		gpio_toggle(GPIOC, GPIO8);

		/* Upon button press, blink more slowly. */
		if (gpio_get(GPIOA, GPIO0)) {
			for (i = 0; i < 300000; i++) {	/* Wait a bit. */
				__asm__("nop");
			}
		}

		for (i = 0; i < 300000; i++) {		/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
