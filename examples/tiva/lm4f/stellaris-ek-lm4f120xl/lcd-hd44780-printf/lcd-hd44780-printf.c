/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

#include "hd44780-lcd-printf.h"

#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/uart.h>
#include <stdio.h>


static struct hd44780_lcd *my_vfd = &stellaris_vfd;

static const uint8_t spkr[] = {0x00, 0x01, 0x03, 0x07, 0x07, 0x03, 0x01, 0x00};
static const uint8_t wave[] = {0x00, 0x04, 0x12, 0x0a, 0x0a, 0x12, 0x04, 0x00};

int main(void)
{
	size_t rx_chars, tx_chars, old_rx_num, old_tx_num;
	const char *greeting = "libopencm3 ";

	gpio_enable_ahb_aperture();
	uart_setup();
	uart_irq_setup();

	lcd_init(my_vfd);
	lcd_clear_display(my_vfd);

	uart_printf("Hello from libopencm3-examples\n");

	/* Write data using lcd_put_string(). */
	lcd_put_string(my_vfd, 0, 0, greeting);

	/* Demonstrate putting custom characters on the display. */
	lcd_fill_cgram_character(my_vfd, 0, spkr);
	lcd_fill_cgram_character(my_vfd, 1, wave);
	lcd_write_ddram(my_vfd, strlen(greeting) + 0, 0);
	lcd_write_ddram(my_vfd, strlen(greeting) + 1, 1);

	old_rx_num = old_tx_num = 0xffffffff;
	while (1) {
		/* Demonstrate length-limited lcd_nprintf. */
		rx_chars = uart_get_num_rx_bytes();
		if (old_rx_num != rx_chars) {
			lcd_nprintf(my_vfd, 1, 0, 10, "RX: %3u", rx_chars);
			old_rx_num = rx_chars;
		}

		/* Demonstrate regular lcd_printf. */
		tx_chars = uart_get_num_tx_bytes();
		if (old_tx_num != tx_chars) {
			lcd_printf(my_vfd, 1, 10, "TX: %3u", tx_chars);
			old_tx_num = tx_chars;
		}

		if (rx_chars >= 999 || tx_chars >= 999)
			uart_reset_rxtx_counters();
	}

	/* I always love these comments: "Never reached." */
	return 0;
}
