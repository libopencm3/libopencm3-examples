/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"

/* Convert degrees to radians */
#define d2r(d) ((d) * 6.2831853 / 360.0)

/*
 * This is our example, the heavy lifing is actually in lcd-spi.c but
 * this drives that code.
 */
int main(void)
{
	int p1, p2, p3;

	clock_setup();
	console_setup(115200);
	sdram_init();
	lcd_spi_init();
	console_puts("LCD Initialized\n");
	console_puts("Should have a checker pattern, press any key to proceed\n");
	msleep(2000);
/*	(void) console_getc(1); */
	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_fillScreen(LCD_GREY);
	gfx_fillRoundRect(10, 10, 220, 220, 5, LCD_WHITE);
	gfx_drawRoundRect(10, 10, 220, 220, 5, LCD_RED);
	gfx_fillCircle(20, 250, 10, LCD_RED);
	gfx_fillCircle(120, 250, 10, LCD_GREEN);
	gfx_fillCircle(220, 250, 10, LCD_BLUE);
	gfx_setTextSize(2);
	gfx_setCursor(15, 25);
	gfx_puts("STM32F4-DISCO");
	gfx_setTextSize(1);
	gfx_setCursor(15, 49);
	gfx_puts("Simple example to put some");
	gfx_setCursor(15, 60);
	gfx_puts("stuff on the LCD screen.");
	lcd_show_frame();
	console_puts("Now it has a bit of structured graphics.\n");
	console_puts("Press a key for some simple animation.\n");
	msleep(2000);
/*	(void) console_getc(1); */
	gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
	gfx_setTextSize(3);
	p1 = 0;
	p2 = 45;
	p3 = 90;
	while (1) {
		gfx_fillScreen(LCD_BLACK);
		gfx_setCursor(15, 36);
		gfx_puts("PLANETS!");
		gfx_fillCircle(120, 160, 40, LCD_YELLOW);
		gfx_drawCircle(120, 160, 55, LCD_GREY);
		gfx_drawCircle(120, 160, 75, LCD_GREY);
		gfx_drawCircle(120, 160, 100, LCD_GREY);

		gfx_fillCircle(120 + (sin(d2r(p1)) * 55),
			       160 + (cos(d2r(p1)) * 55), 5, LCD_RED);
		gfx_fillCircle(120 + (sin(d2r(p2)) * 75),
			       160 + (cos(d2r(p2)) * 75), 10, LCD_WHITE);
		gfx_fillCircle(120 + (sin(d2r(p3)) * 100),
			       160 + (cos(d2r(p3)) * 100), 8, LCD_BLUE);
		p1 = (p1 + 3) % 360;
		p2 = (p2 + 2) % 360;
		p3 = (p3 + 1) % 360;
		lcd_show_frame();
	}
}
