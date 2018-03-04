/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2016 Benjamin Levine <benjamin@jesco.karoo.co.uk>
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

uint16_t exti_line_state;

/* Set STM32 to 48 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_clock_config[RCC_CLOCK_VRANGE1_HSI16_PLL_48MHZ]);
}

static void gpio_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIO5 (in GPIO port A) to 'output push-pull'. */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
}

static void button_setup(void)
{
	/* Enable GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Set GPIO13 (in GPIO port C) to 'input open-drain'. */
	gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO13);
}

int main(void)
{
	int i;

	clock_setup();
	button_setup();
	gpio_setup();

	/* Blink the LED (PA5) on the board. */
	while (1) {
		gpio_toggle(GPIOA, GPIO5);

		/* Upon button press, blink more quickly. */
		exti_line_state = GPIOC_IDR;
		if ((exti_line_state & (1 << 13)) != 0) {
			for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
				__asm__("nop");
			}
		}

		for (i = 0; i < 500000; i++) {		/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
