/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Damjan Marion <damjan.marion@gmail.com>
 * Copyright (C) 2011 Mark Panajotovic <marko@electrontube.org>
 * Copyright (C) 2016 Josh Myer <josh@joshisanerd.com>
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

/* This assumes you've put LEDs on the Arduino header, D13~D6. */


/* Arduino header pin mapping: */
#define D13 GPIOA, GPIO5
#define D12 GPIOA, GPIO6
#define D11 GPIOA, GPIO7
#define D10 GPIOB, GPIO6
#define  D9 GPIOC, GPIO7
#define  D8 GPIOA, GPIO9
#define  D7 GPIOA, GPIO8
#define  D6 GPIOB, GPIO10


/* Final port usage:
 * PA: 5-9
 * PB: 6,10
 * PC: 7
 */

#define MASKA GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9
#define MASKB GPIO6 | GPIO10
#define MASKC GPIO7

/* Set STM32 to 64 MHz. */
static void clock_setup(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKA);

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKB);

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKC);
}

int main(void)
{
	int i;

	clock_setup();
	gpio_setup();

	/* Set two LEDs for wigwag effect when toggling. */
	gpio_set(D13);
	gpio_set(D6);

	/* Blink the LEDs (PD8, PD9, PD10 and PD11) on the board. */
	while (1) {
		/* Toggle LEDs. */
		gpio_toggle(GPIOA, MASKA);
		gpio_toggle(GPIOB, MASKB);
		gpio_toggle(GPIOC, MASKC);
		/* Wait a bit. */
		for (i = 0; i < 1000000; i++) {
			__asm__("nop");
		}
	}

	return 0;
}
