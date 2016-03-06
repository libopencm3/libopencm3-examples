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

#include <stdarg.h>
#include <stdio.h>

#define MIN(x, y)	((x) < (y) ? (x) : (y))

static void set_data_lines(struct hd44780_lcd *ctx, uint8_t data)
{
	ctx->set_data_lines(ctx, data);
}

static void set_handshake_lines(struct hd44780_lcd *ctx,
				enum lcd_ctl_lines lines)
{
	ctx->set_handshake_lines(ctx, lines);
}

static void ndelay(struct hd44780_lcd *ctx, unsigned int nano_sec)
{
	ctx->ndelay(ctx, nano_sec);
}

void lcd_init(struct hd44780_lcd *ctx)
{
	uint8_t function_bits;

	ctx->hardware_init(ctx);

	function_bits = HD44780_FUNCTION_8BIT;
	if (ctx->num_lines > 1)
		function_bits |= HD44780_FUNCTION_2LINES;

	lcd_send_command(ctx, HD44780_CMD_FUNCTION_SET | function_bits);
	/* VFD-specific: set brightness after sending FUNCTION_SET command */
	lcd_send_data(ctx, 0x02);
	/* Enable display. Disable cursor and blinker. */
	lcd_send_command(ctx, HD44780_CMD_DISPLAY_ON_OFF |
				HD44780_DISP_ONOFF_DISPLAY);
	lcd_send_command(ctx, HD44780_CMD_ENTRY_MODE_SET |
				HD44780_MODE_INCREMENT);
}

void lcd_clear_display(struct hd44780_lcd *ctx)
{
	lcd_send_command(ctx, HD44780_CMD_CLEAR_DISPLAY);
	ndelay(ctx, 2300000);
}

void lcd_send_command(struct hd44780_lcd *ctx, uint8_t cmd)
{
	set_handshake_lines(ctx, LINE_E);
	ndelay(ctx, 1000);
	set_data_lines(ctx, cmd);
	ndelay(ctx, 1000);
	set_handshake_lines(ctx, LINE_NONE);
	ndelay(ctx, 1000);
}

void lcd_send_data(struct hd44780_lcd *ctx, uint8_t data)
{
	set_handshake_lines(ctx, LINE_E | LINE_RS);
	ndelay(ctx, 1000);
	set_data_lines(ctx, data);
	ndelay(ctx, 1000);
	set_handshake_lines(ctx, LINE_RS);
	ndelay(ctx, 1000);

}

void lcd_write_ddram(struct hd44780_lcd *ctx, uint8_t addr, uint8_t val)
{
	lcd_send_command(ctx, HD44780_CMD_SET_DDRAM_ADDR | addr);
	lcd_send_data(ctx, val);
}

static void set_cgram_addr(struct hd44780_lcd *ctx, uint8_t addr)
{
	addr &= HD44780_CMD_MASK(HD44780_CMD_SET_CGRAM_ADDR);
	lcd_send_command(ctx, HD44780_CMD_SET_CGRAM_ADDR | addr);
}

void lcd_write_cgram(struct hd44780_lcd *ctx, uint8_t addr, uint8_t val)
{
	set_cgram_addr(ctx, addr);
	lcd_send_data(ctx, val);
}

void lcd_put_string(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
		    const char *str)
{
	unsigned int i;
	uint8_t addr;

	if (start_char >= ctx->line_len)
		return;

	addr = ctx->mem_line_stride * line + start_char;

	lcd_send_command(ctx, HD44780_CMD_SET_DDRAM_ADDR | addr);
	for (i = 0; i < (ctx->line_len - start_char); i++) {
		if (str[i] == '\0')
			break;
		lcd_send_data(ctx, str[i]);
	}
}

static void lcd_print_variadic(struct hd44780_lcd *ctx, uint8_t line,
			       uint8_t start_char, uint8_t num_chars,
			       const char *format, va_list args)
{
	char buffer[64];
	int len;

	if (start_char >= ctx->line_len)
		return;

	len = MIN(ctx->line_len - start_char, num_chars);
	len = vsnprintf(buffer, len, format, args);

	lcd_put_string(ctx, line, start_char, buffer);
}

void lcd_printf(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
		const char *format, ...)
{
	va_list args;

	va_start(args, format);
	lcd_print_variadic(ctx, line, start_char, ctx->line_len, format, args);
	va_end(args);
}

void lcd_nprintf(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
			uint8_t num_chars, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	lcd_print_variadic(ctx, line, start_char, num_chars, format, args);
	va_end(args);
}

void lcd_fill_cgram_character(struct hd44780_lcd *ctx, uint8_t char_idx,
			      const uint8_t bitmap[8])
{
	int i;

	if (char_idx > 7)
		return;

	set_cgram_addr(ctx, char_idx * 8);
	for (i = 0; i < 8; i++)
		lcd_send_data(ctx, bitmap[i]);
}
