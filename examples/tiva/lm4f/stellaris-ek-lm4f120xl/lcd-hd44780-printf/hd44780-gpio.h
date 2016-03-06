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

#ifndef _HD44780_GPIO_H_
#define _HD44780_GPIO_H_

#include <stdint.h>
#include <string.h>

enum lcd_ctl_lines {
	LINE_NONE	= 0,
	LINE_E		= 1 << 0,
	LINE_RS		= 1 << 1,
	LINE_WR		= 1 << 2,
};

enum hd44780_commands {
	HD44780_CMD_CLEAR_DISPLAY	= 1 << 0,
	HD44780_CMD_RETURN_HOME		= 1 << 1,
	HD44780_CMD_ENTRY_MODE_SET	= 1 << 2,
	HD44780_CMD_DISPLAY_ON_OFF	= 1 << 3,
	HD44780_CMD_CURSOR_SHIFT	= 1 << 4,
	HD44780_CMD_FUNCTION_SET	= 1 << 5,
	HD44780_CMD_SET_CGRAM_ADDR	= 1 << 6,
	HD44780_CMD_SET_DDRAM_ADDR	= 1 << 7,
};

enum hd44780_function {
	HD44780_FUNCTION_8BIT		= 1 << 4,
	HD44780_FUNCTION_2LINES		= 1 << 3,
	HD44780_FUNCTION_FONT_5x8	= 1 << 2,
};

enum hd44780_disp_onoff {
	HD44780_DISP_ONOFF_DISPLAY	= 1 << 2,
	HD44780_DISP_ONOFF_CURSOR	= 1 << 1,
	HD44780_DISP_ONOFF_BLINKER	= 1 << 0,
};

enum hd44780_entry_mode {
	HD44780_MODE_INCREMENT		= 1 << 1,
	HD44780_MODE_SHIFT		= 1 << 0,
};

#define HD44780_CMD_MASK(cmd)	((cmd) - 1)

struct hd44780_lcd {
	void (*hardware_init)(struct hd44780_lcd *);
	void (*set_data_lines)(struct hd44780_lcd *, uint8_t data);
	void (*set_handshake_lines)(struct hd44780_lcd *, enum lcd_ctl_lines);
	void (*change_data_lines_mode_in)(struct hd44780_lcd *);
	void (*change_data_lines_mode_out)(struct hd44780_lcd *);
	void (*ndelay)(struct hd44780_lcd *, unsigned int nano_sec);
	void *priv;
	unsigned int num_lines;
	unsigned int line_len;
	unsigned int mem_line_stride;
};

void lcd_init(struct hd44780_lcd *ctx);
void lcd_clear_display(struct hd44780_lcd *ctx);
void lcd_write_ddram(struct hd44780_lcd *ctx, uint8_t addr, uint8_t val);
void lcd_write_cgram(struct hd44780_lcd *ctx, uint8_t addr, uint8_t val);
void lcd_send_command(struct hd44780_lcd *ctx, uint8_t cmd);
void lcd_send_data(struct hd44780_lcd *ctx, uint8_t data);
void lcd_put_string(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
		    const char *str);
void lcd_printf(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
		const char *format, ...);
void lcd_nprintf(struct hd44780_lcd *ctx, uint8_t line, uint8_t start_char,
		 uint8_t num_chars, const char *format, ...);
void lcd_fill_cgram_character(struct hd44780_lcd *ctx, uint8_t char_idx,
			      const uint8_t bitmap[8]);

#endif /* _HD44780_GPIO_H_ */
