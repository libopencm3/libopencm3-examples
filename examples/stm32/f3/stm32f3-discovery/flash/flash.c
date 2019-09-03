/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2019 Roel Gerrits <roel@roelgerrits.nl>
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
#include <libopencm3/stm32/flash.h>

static void gpio_setup(void)
{
	/* Enable GPIOE clock. */
	rcc_periph_clock_enable(RCC_GPIOE);

	/* Set GPIO12 (in GPIO port E) to 'output push-pull'. */
	gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
		GPIO8 | GPIO9 | GPIO10 | GPIO11 |
		GPIO12 | GPIO13 | GPIO14 | GPIO15);
}


/*
 * This variable is made available by the linker script
 * it's address is the address where the my_flash section starts
 */
extern uint8_t _my_flash_start;


int main(void)
{
	uint16_t counter;

	gpio_setup();

    /* Read value from flash, for reading we can just read from
	 * the address as with any other variable. Writing requires a
	 * little more effort as we will see later on.
	 */
	counter = _my_flash_start;

    /* Write the counter's binary value to the LED's on port E */
	gpio_set(GPIOE, (counter & 0xff) << 8);

	/* In order to write to flash we have to unlock it first */
	flash_unlock();

	/* Flash memory can only be written if it's been erased first
	 * so let's do that now */
	uint32_t my_flash_addr = (uint32_t) &_my_flash_start;
	flash_erase_page(my_flash_addr);

    /* Increment counter and write it to flash */
	counter++;
	flash_program_half_word(my_flash_addr, counter);


	while (1) {
		/* do absolutely nothing */
	}

	return 0;
}

