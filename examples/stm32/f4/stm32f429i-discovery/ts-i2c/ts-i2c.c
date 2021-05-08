/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *               2015 brabo <brabo.sil@gmail.com>
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
#include <math.h>
#include "clock.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"
#include "stmpe811.h"

/*
 * This is our example, the heavy lifting is actually in stmpe811.c but
 * this drives that code.
 */
int main(void)
{
	stmpe811_t stmpe811_data;

	clock_setup();

	sdram_init();

	lcd_spi_init();

	gfx_init(lcd_draw_pixel, 240, 320);

	stmpe811_init();

	stmpe811_data.orientation = stmpe811_portrait_2;

	while (1) {
		if (stmpe811_read_touch(&stmpe811_data) == stmpe811_state_pressed) {
			gfx_fill_screen(LCD_GREEN);
			lcd_show_frame();
		} else {
			gfx_fill_screen(LCD_RED);
			lcd_show_frame();
		}

	}


}
