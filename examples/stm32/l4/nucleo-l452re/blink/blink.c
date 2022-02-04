/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (c) 2015 Chuck McManis <cmcmanis@mcmanis.com>
 * Copyright (c) 2022 J.W. Bruce <jwbruce@ieee.org>
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

/* the STM Nucleo-64 PCB connects the user LED */
/* LD2 to GPIO pin PA5 through the SB42 solder jumper */
/* See the Nucleo-64 schematic and/or users manual*/
#define     LD2_PORT        GPIOA
#define     LD2_PIN         GPIO5

static void gpio_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIO driving LD2 (PA5) to 'output push-pull'. */
	gpio_mode_setup(LD2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LD2_PIN);
}

int main(void)
{
	int i;

	gpio_setup();

	/* Blink the LD2 LED (PA5) on the Nucleo board. */
	while (1) {

		/* Using API function gpio_toggle(): */
		gpio_toggle(LD2_PORT, LD2_PIN);	/* LED on/off */
        
        /* Wait a bit. */
		for (i = 0; i < 200000; i++) {	
			__asm__("nop");
		}
	}

	return 0;
}
