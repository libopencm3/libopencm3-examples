/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Karl Palsson <karlp@tweak.net.au>
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

#define LED_GREEN_PIN GPIO4
#define LED_GREEN_PORT GPIOB
#define LED_RED_PIN GPIO5
#define LED_RED_PORT GPIOA

static void gpio_setup(void)
{
	/* Enable GPIOB clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	/* set pins to output mode, push pull */
	gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
	gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN_PIN);
}

int main(void)
{
	int i;

	gpio_setup();

	while (1) {
		/* toggle each led in turn */
		gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
		for (i = 0; i < 100000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		gpio_toggle(LED_RED_PORT, LED_RED_PIN);
		for (i = 0; i < 100000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
