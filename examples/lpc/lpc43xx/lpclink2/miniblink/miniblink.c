/*
* This file is part of the libopencm3 project.
*
* Copyright (C) 2010 Uwe Hermann <uwe@hermann-uwe.de>
* Copyright (C) 2012 Michael Ossmann <mike@ossmann.com>
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

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include "../lpclink2_conf.h"

static void gpio_setup(void)
{
	/* Configure SCU Pin Mux as GPIO */
	scu_pinmux(SCU_PINMUX_LED1, SCU_GPIO_FAST);

	scu_pinmux(SCU_PINMUX_BOOT1, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_BOOT2, SCU_GPIO_FAST);
	scu_pinmux(SCU_PINMUX_BOOT3, SCU_GPIO_FAST);

	/* Configure all GPIO as Input (safe state) */
	GPIO0_DIR = 0;
	GPIO1_DIR = 0;
	GPIO2_DIR = 0;
	GPIO3_DIR = 0;
	GPIO4_DIR = 0;
	GPIO5_DIR = 0;
	GPIO6_DIR = 0;
	GPIO7_DIR = 0;

	/* Configure GPIO as Output */
	GPIO0_DIR |= (PIN_LED1); /* Configure GPIO1[1] (P1_1) as output. */
}

uint32_t boot1, boot2, boot3;

int main(void)
{
	int i;
	gpio_setup();

	/* Blink LED1 on the board and Read BOOT1/2/3 pins. */
	while (1)
	{
		boot1 = BOOT1_STATE;
		boot2 = BOOT2_STATE;
		boot3 = BOOT3_STATE;

		gpio_set(PORT_LED1, (PIN_LED1)); /* LEDs on */
		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
		gpio_clear(PORT_LED1, (PIN_LED1)); /* LED off */
		for (i = 0; i < 2000000; i++)	/* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
