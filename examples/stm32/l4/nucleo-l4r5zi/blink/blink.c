/*
 * This file is part of the libopencm3 project.
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
	/* Enable GPIOB and GPIOC clock. */
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

	/* Set GPIO7 and GPIO14 (in GPIO port B) to 'output push-pull'. */
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);

	/* Set GPIO7 (in GPIO port C) to 'output push-pull'. */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO7);
}

int main(void)
{
	int i;

	gpio_setup();

	/* Blink the LEDs on the board. */
	enum {
		GreenLED,
		BlueLED,
		RedLED
	} led = GreenLED;
	while (1) {
		/* Using API function gpio_toggle(): */
		switch(led) {
		case GreenLED:
			gpio_toggle(GPIOC, GPIO7);
			break;
		case BlueLED:
			gpio_toggle(GPIOB, GPIO7);
			break;
		case RedLED:
			gpio_toggle(GPIOB, GPIO14);
			break;
		}
		led = (led + 1) % 3;

		/* Wait a bit. */
		for (i = 0; i < 1000000; i++) {
			__asm__("nop");
		}
	}

	return 0;
}
