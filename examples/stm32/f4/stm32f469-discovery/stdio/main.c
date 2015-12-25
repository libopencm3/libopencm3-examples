/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "../util/clock.h"

int main(void)
{
	double flash_rate = 3.14159;
	char buf[128];
	uint32_t done_time;

	/* things printed to standard error stand out */
	fprintf(stderr, "\nSTM32F469-Discovery");
	printf(" Retarget demo.\n");

	/* This is the LED on the board */
	rcc_periph_clock_enable(RCC_GPIOK);
	gpio_mode_setup(GPIOK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);

	/* You can create a running dialog with the console */
	printf("We're going to start blinking the LED, between 0 and 300 times per\n");
	printf("second.\n");
	while (1) {
		printf("Please enter a flash rate: ");
		fflush(stdout);
		fgets(buf, 128, stdin);
		buf[strlen(buf) - 1] = '\0';
		flash_rate = atof(buf);
		if (flash_rate == 0) {
			printf("Was expecting a number greater than 0 and less than 300.\n");
			printf("but got '%s' instead\n", buf);
		} else if (flash_rate > 300.0) {
			printf("The number should be less than 300.\n");
		} else {
			break;
		}
	}
	/* MS per flash */
	done_time = (int) (500 / flash_rate);
	printf("The closest we can come will be %f flashes per second\n", 500.0 / done_time);
	printf("With %d MS between states\n", (int) done_time);
	printf("Press ^C to restart this demo.\n");
	while (1) {
		msleep(done_time);
		gpio_toggle(GPIOK, GPIO3);
	}
}
