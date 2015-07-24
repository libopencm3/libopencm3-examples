/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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

static void gpio_setup(void)
{
	/* Enable GPIOC clock. */
	/* Manually: */
	/* RCC_APB2ENR |= RCC_APB2ENR_IOPCEN; */
	/* Using API functions: */
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Set GPIO9 (in GPIO port C) to 'output push-pull'. */
	/* Manually: */
	/* GPIOC_CRH = (GPIO_CNF_OUTPUT_PUSHPULL << (((9 - 8) * 4) + 2)); */
	/* GPIOC_CRH |= (GPIO_MODE_OUTPUT_2_MHZ << ((9 - 8) * 4)); */
	/* Using API functions: */
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);
}

int main(void)
{
	int i;

	gpio_setup();

	/* Blink LED1 (PC9) on the board. */
	while (1) {
		/* Manually: */
		/* GPIOC_BSRR = GPIO9; */		/* LED off */
		/* for (i = 0; i < 800000; i++) */	/* Wait a bit. */
		/*	__asm__("nop"); */
		/* GPIOC_BRR = GPIO9; */		/* LED on */
		/* for (i = 0; i < 800000; i++) */	/* Wait a bit. */
		/*	__asm__("nop"); */

		/* Using API functions gpio_set()/gpio_clear(): */
		/* gpio_set(GPIOC, GPIO9); */		/* LED off */
		/* for (i = 0; i < 800000; i++) */	/* Wait a bit. */
		/*	__asm__("nop"); */
		/* gpio_clear(GPIOC, GPIO9); */		/* LED on */
		/* for (i = 0; i < 800000; i++) */	/* Wait a bit. */
		/*	__asm__("nop"); */

		/* Using API function gpio_toggle(): */
		gpio_toggle(GPIOC, GPIO9);	/* LED on/off */
		for (i = 0; i < 800000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
