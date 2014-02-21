/* 
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis (cmcmanis@mcmanis.com)
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

#define GREEN_LED	GPIO13
#define RED_LED		GPIO13

int main(void)
{
	int i;

	/* Use parameters to set the STM32 clock to 168 MHz. */
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

	/* Enable GPIOG clock. (this enables the pins to work) */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set the "mode" of the GPIO pin to output, no pullups or pulldowns */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GREEN_LED );

	/* Turn on the GREEN led */
	gpio_set(GPIOG, GREEN_LED);

	/* Blink the GREEN LED on the board. */
	while (1) {
		/* Toggle LEDs. */
		gpio_toggle(GPIOG, GREEN_LED);
		for (i = 0; i < 10000000; i++) /* Wait a bit. */
			__asm__("nop");
	}

	return 0;
}
