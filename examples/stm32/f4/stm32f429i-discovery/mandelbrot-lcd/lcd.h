/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014-2015 Chuck McManis <cmcmanis@mcmanis.com>
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

#ifndef __LCD_SPI_H
#define LCD_SPI_H

/*
 * prototypes for the LCD example
 *
 * This is a very basic API, initialize, a function which will show the
 * frame, and a function which will draw a pixel in the framebuffer.
 */

void lcd_init(void);
void lcd_show_frame(void);
void lcd_draw_pixel(int x, int y, uint16_t color);

/* Color definitions */
#define	LCD_BLACK   0x0000
#define	LCD_BLUE    0x1F00
#define	LCD_RED     0x00F8
#define	LCD_GREEN   0xE007
#define LCD_CYAN    0xFF07
#define LCD_MAGENTA 0x1FF8
#define LCD_YELLOW  0xE0FF
#define LCD_WHITE   0xFFFF
#define LCD_GREY    0xc339

#define LCD_WIDTH   240
#define LCD_HEIGHT  320

#endif
