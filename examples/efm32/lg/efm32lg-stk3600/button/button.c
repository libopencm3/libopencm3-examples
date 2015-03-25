/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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

#define BUTTON_PORT GPIOB
#define BUTTON_PIN GPIO9

#define LED_PORT GPIOE
#define LED_PIN GPIO2

static void clock_setup(void)
{
	/* Enable GPIO clock. */
	cmu_periph_clock_enable(CMU_GPIO);
}

static void led_setup(void)
{
	/* Set "LED0" to 'output push-pull'. */
	gpio_mode_setup(LED_PORT, GPIO_MODE_PUSH_PULL, LED_PIN);
}

static void button_setup(void)
{
	/* Set button "PB0" to 'input'. */
	gpio_clear(BUTTON_PORT, BUTTON_PIN);
	gpio_mode_setup(BUTTON_PORT, GPIO_MODE_INPUT, BUTTON_PIN);
}

int main(void)
{
	unsigned i;

	clock_setup();
	led_setup();
	button_setup();

	/* Blink the LED0 on the board. */
	while (1) {
		gpio_toggle(LED_PORT, LED_PIN);

		/* Upon button press, blink more slowly. */
		if (gpio_get(BUTTON_PORT, BUTTON_PIN)) {
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
