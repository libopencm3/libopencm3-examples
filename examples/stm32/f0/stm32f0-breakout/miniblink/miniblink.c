/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018 Roel Jordans <r.jordans@tue.nl>
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
#include <libopencm3/stm32/flash.h>

#define PORT_LED GPIOA
#define PIN_LED GPIO5

static void delay(int n)
{
	for (int i = 0; i < n; i++) {	/* Wait a bit. */
		__asm__("nop");
	}
}

static void gpio_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIO5 (in GPIO port A) to 'output push-pull'. */
	gpio_mode_setup(PORT_LED, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PIN_LED);
}

int main(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();

	gpio_setup();

	/* Blink the LED (PA5) on the board. */
	while (1) {
		gpio_toggle(PORT_LED, PIN_LED);	/* LED on/off */
		delay(4000000);
	}

	return 0;
}
