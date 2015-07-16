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
#include <libopencm3/vf6xx/gpio.h>
#include <libopencm3/vf6xx/iomuxc.h>


static const uint32_t gpio_out = IOMUXC_PAD(MUX_MODE_ALT0, SPEED_LOW, \
			DSE_150OHM, PUS_PU_100KOHM, IOMUXC_OBE | IOMUXC_IBE);

static const uint32_t gpio_in = IOMUXC_PAD(MUX_MODE_ALT0, SPEED_LOW, \
			DSE_150OHM, PUS_PU_100KOHM, IOMUXC_IBE);

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
	int old_state = -1;
	int new_state = 0;
	int i = 0;
	float f = 0.0f;


	/* Init Clock Control and UART */
	ccm_calculate_clocks();
	uart_init();

	/* Iris Carrier Board, X16, Pin 15 */
	iomuxc_mux(PTC3, gpio_out);

	/* Iris Carrier Board, X16, Pin 16 */
	iomuxc_mux(PTC2, gpio_in);

	gpio_set(PTC3);
	printf("GPIO %d is %s\r\n", PTC3, gpio_get(PTC3) ? "high" : "low");

	/*
	 * Read input GPIO in a loop and print changes
	 * A jumper between PTC3<=> PTC2 can be used to set PTC2 to high
	 * print integer and floating point counters as well, just for fun.
	 */
	while (1) {
		new_state = gpio_get(PTC2);
		if (old_state != new_state) {
			printf("loop: %d (step: %f) GPIO %d is %s\r\n",
				i, f, PTC2, new_state ? "high" : "low");
			old_state = new_state;
		}
		i++;
		f += 0.01;
	}


	return 0;
}

