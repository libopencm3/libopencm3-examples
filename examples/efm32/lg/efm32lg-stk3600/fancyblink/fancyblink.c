/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
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

#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/gpio.h>

/* Set EFM32 to 48 MHz. */
static void clock_setup(void)
{
	cmu_clock_setup_in_hfxo_out_48mhz();

	/* Enable GPIO clocks. */
	cmu_periph_clock_enable(CMU_GPIO);
}

static void gpio_setup(void)
{
	/* Set GPIO{2,3} (in GPIO port E) to 'output push-pull'. */
	gpio_mode_setup(GPIOE, GPIO_MODE_PUSH_PULL, GPIO2 | GPIO3);

	/* Preconfigure the LEDs. */
	gpio_set(GPIOE, GPIO2);	/* Switch off LED. */
	gpio_clear(GPIOE, GPIO3);	/* Switch on LED. */
}

int main(void)
{
	int i;

	clock_setup();
	gpio_setup();

	/* Blink the LEDs (PE2 and PE3) on the board. */
	while (1) {
		gpio_toggle(GPIOE, GPIO2);	/* LED on/off */
		gpio_toggle(GPIOE, GPIO3);	/* LED on/off */
		for (i = 0; i < 800000; i++)	/* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
