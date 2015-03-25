/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Karl Palsson <karlp@tweak.net.au>
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

#define PORT_LED GPIOE
#define PIN_LED GPIO2

static void gpio_setup(void)
{
	/* Enable GPIOB clock. */
	/* Using API functions: */
	cmu_periph_clock_enable(CMU_GPIO);

	/* Set GPIO6 (in GPIO port A) to 'output push-pull'. */
	/* Using API functions: */
	gpio_mode_setup(PORT_LED, GPIO_MODE_PUSH_PULL, PIN_LED);
}

int main(void)
{
	unsigned i;

	gpio_setup();

	/* Toggle the LED pin */
	while (1) {
		/* LED on/off */
		gpio_toggle(PORT_LED, PIN_LED);

		/* Wait a bit. */
		for (i = 0; i < 1000000; i++) {
			__asm__("nop");
		}
	}

	return 0;
}
