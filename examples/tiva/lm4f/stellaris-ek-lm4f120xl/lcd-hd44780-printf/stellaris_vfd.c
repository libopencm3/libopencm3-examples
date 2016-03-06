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

#include "hd44780-gpio.h"

#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/gpio.h>

enum {
	DISP_FNC	= GPIO4,
	DISP_RS		= GPIO5,
	DISP_RW		= GPIO6,
	DISP_E		= GPIO7,
};

static void data_pins_setup(void)
{
	/*
	 * Configure GPIOB
	 * This is the data port for the LCD
	 */
	periph_clock_enable(RCC_GPIOB);
	const uint32_t opins = 0xff;

	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, opins);
	gpio_set_output_config(GPIOB, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, opins);
}

static void handshake_pins_setup(void)
{
	/*
	 * Configure GPIOC
	 * This is the handshake signals (4-7)
	 */
	periph_clock_enable(RCC_GPIOC);
	const uint32_t opins = DISP_FNC | DISP_RS | DISP_RW | DISP_E;

	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, opins);
	gpio_set_output_config(GPIOC, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, opins);
}

static void hardware_init(struct hd44780_lcd *ctx)
{
	(void) ctx;

	handshake_pins_setup();
	data_pins_setup();
}

static void set_data_lines(struct hd44780_lcd *ctx, uint8_t data)
{
	(void) ctx;

	gpio_write(GPIOB, 0xff, data);
}

static void set_handshake_lines(struct hd44780_lcd *ctx,
				enum lcd_ctl_lines lines)
{
	(void) ctx;

	const uint8_t signals_mask = DISP_FNC | DISP_RS | DISP_RW | DISP_E;
	uint8_t signals = 0;

	signals |= (lines & LINE_E) ? DISP_E : 0;
	signals |= (lines & LINE_RS) ? DISP_RS : 0;
	signals |= (lines & LINE_WR) ? DISP_RW : 0;

	gpio_write(GPIOC, signals_mask, signals);
}

static void change_data_lines_mode_in(struct hd44780_lcd *ctx)
{
	(void) ctx;

	const uint32_t opins = 0xff;
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, opins);
}

static void change_data_lines_mode_out(struct hd44780_lcd *ctx)
{
	(void) ctx;

	const uint32_t opins = 0xff;
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, opins);
}

static void lazydelay(unsigned int nano_sec)
{
	/* Not even in the ballpark, but we don't have to be precise. */
	int i = nano_sec / 13;

	while (--i)
		__asm__ volatile ("nop");
}

static void ndelay(struct hd44780_lcd *ctx, unsigned int nano_sec)
{
	(void) ctx;

	lazydelay(nano_sec);
}

struct hd44780_lcd stellaris_vfd = {
	.hardware_init = hardware_init,
	.set_data_lines = set_data_lines,
	.set_handshake_lines = set_handshake_lines,
	.change_data_lines_mode_in = change_data_lines_mode_in,
	.change_data_lines_mode_out = change_data_lines_mode_out,
	.ndelay = ndelay,
	.num_lines = 2,
	.line_len = 20,
	.mem_line_stride = 0x40,
};
