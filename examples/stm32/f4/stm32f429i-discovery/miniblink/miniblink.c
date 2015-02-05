/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
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
	/* Enable GPIOD clock. */
	/* Manually: */
	/* RCC_AHB1ENR |= RCC_AHB1ENR_IOPGEN; */
	/* Using API functions: */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set GPIO13 (in GPIO port G) to 'output push-pull'. */
	/* Manually: */
	/* GPIOG_CRH = (GPIO_CNF_OUTPUT_PUSHPULL << 2); */
	/* GPIOG_CRH |= (GPIO_MODE_OUTPUT_2_MHZ << 2); */
	/* Using API functions: */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
}

int main(void)
{
	int i;

	gpio_setup();

	/* Blink the LED (PG13) on the board. */
	while (1) {
		/* Manually: */
#if 0
		GPIOG_BSRR = GPIO13;		/* LED off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		GPIOG_BRR = GPIO13;		/* LED on */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
#endif

		/* Using API functions gpio_set()/gpio_clear(): */
#if 0
		gpio_set(GPIOG, GPIO13);	/* LED off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
		gpio_clear(GPIOG, GPIO13);	/* LED on */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
#endif

		/* Using API function gpio_toggle(): */
		gpio_toggle(GPIOG, GPIO13);	/* LED on/off */
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("nop");
		}
	}

	return 0;
}
