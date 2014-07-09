/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2014 Stefan Agner <stefan@agner.ch>
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

#include <stdio.h>
#include <errno.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/vf6xx/ccm.h>
#include <libopencm3/vf6xx/uart.h>

int _write(int file, char *ptr, int len);

int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			uart_send_blocking(UART2, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

static void uart_init(void)
{
	ccm_clock_gate_enable(CG9_UART2);
	uart_enable(UART2);
	uart_set_baudrate(UART2, 115200);
}

int main(void)
{
	int counter = 0; 
	float fcounter = 0.0;
	double dcounter = 0.0;

	ccm_calculate_clocks();
	uart_init();

	/*
	 * Write Hello World an integer, float and double all over
	 * again while incrementing the numbers.
	 */
	while (1) {
		printf("Hello World! %i %f %f\r\n", counter, fcounter,
		       dcounter);
		counter++;
		fcounter += 0.01;
		dcounter += 0.01;
	}

	return 0;
}

