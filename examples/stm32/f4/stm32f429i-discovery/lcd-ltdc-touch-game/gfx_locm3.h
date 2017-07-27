/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Oliver Meier <h2obrain@gmail.com>
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


#ifndef _GFX_LOCM3_H_
#define _GFX_LOCM3_H_


#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fonts/utf8.h"
#include "fonts/fonts.h"

#define swap(a, b) { int16_t t = a; a = b; b = t; }

/*
 * Python
 * def get_rgb565(x) :
 *  return ((x&0xf80000)>>8) | ((x&0xfc00)>>5) | ((x&0xf8)>>3)
 *
 * print print "0x%X"%get_rgb565(some_rgb_value)
 */
#define GFX_COLOR_WHITE          0xFFFF
#define GFX_COLOR_BLACK          0x0000
#define GFX_COLOR_DARKGREY       0x4228
#define GFX_COLOR_GREY           0xF7DE
#define GFX_COLOR_GREY2          0x7BEF /*1111 0111 1101 1110*/
#define GFX_COLOR_BLUE           0x001F
#define GFX_COLOR_BLUE2          0x051F
#define GFX_COLOR_RED            0xF800
#define GFX_COLOR_MAGENTA        0xF81F
#define GFX_COLOR_GREEN          0x07E0
#define GFX_COLOR_GREEN2         0xB723
#define GFX_COLOR_CYAN           0x7FFF
#define GFX_COLOR_YELLOW         0xFFE0
#define GFX_COLOR_ORANGE         0xFBE4
#define GFX_COLOR_BROWN          0xBBCA

typedef enum {
	GFX_ROTATION_0_DEGREES,
	GFX_ROTATION_90_DEGREES,
	GFX_ROTATION_180_DEGREES,
	GFX_ROTATION_270_DEGREES
} gfx_rotation_t;

typedef enum {
	GFX_ALIGNMENT_TOP    = 1<<0,
	GFX_ALIGNMENT_BOTTOM = 1<<1,
	GFX_ALIGNMENT_LEFT   = 1<<2,
	GFX_ALIGNMENT_RIGHT  = 1<<3,
	GFX_ALIGNMENT_CENTER = 1<<4,
} gfx_alignment_t;

typedef struct {
	int16_t x1, y1, x2, y2;
} visible_area_t;

typedef struct {
	uint32_t is_offscreen_rendering;
	const int16_t width_orig, height_orig;
	const uint32_t pixel_count;
	int16_t width, height;
	visible_area_t visible_area;
	int16_t cursor_x, cursor_y, cursor_x_orig;
	uint16_t textcolor;
	uint8_t fontscale;
	gfx_rotation_t rotation;
	bool wrap;
	const font_t *font;
	uint16_t *surface; /* current pixel buffer */
} gfx_state_t;

extern gfx_state_t __gfx_state;


typedef struct {
	size_t count_max;
	size_t count_total;
	size_t overflows;
} fill_segment_queue_statistics_t;


void gfx_init(uint16_t *surface, int32_t w, int32_t h);
/**
 * set pixel buffer address to render graphics on..
 * @param surface pixel buffer address
 */
void gfx_set_surface(uint16_t *surface);
uint16_t *
gfx_get_surface(void);

void gfx_offscreen_rendering_begin(
		uint16_t *surface,
		int32_t width, int32_t height
	);
void gfx_offscreen_rendering_end(void);

void
gfx_set_clipping_area_max(void);
void
gfx_set_clipping_area(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
visible_area_t
gfx_get_surface_visible_area(void);


void gfx_fill_screen(uint16_t color);

void gfx_rotate(gfx_rotation_t rotation);
uint16_t gfx_height(void);
uint16_t gfx_width(void);
gfx_rotation_t gfx_get_rotation(void);

void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color);
int32_t gfx_get_pixel(int16_t x, int16_t y);
fill_segment_queue_statistics_t
gfx_flood_fill4(
		int16_t x, int16_t y,
		uint16_t old_color, uint16_t new_color,
		uint8_t fill_segment_buf[], size_t fill_segment_buf_size
	);

void gfx_draw_line(
		int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		uint16_t color
	);
void gfx_draw_hline(int16_t x, int16_t y, int16_t length, uint16_t color);
void gfx_draw_vline(int16_t x, int16_t y, int16_t length, uint16_t color);
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_draw_raw_rbg565_buffer(
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const uint16_t *img
	);

void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_draw_circle_helper(
		int16_t x0, int16_t y0,
		int16_t r, uint8_t cornername,
		uint16_t color
	);
void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_fill_circle_helper(
		int16_t x0, int16_t y0,
		int16_t r, uint8_t cornername, int16_t delta,
		uint16_t color
	);

void gfx_draw_round_rect(
		int16_t x0, int16_t y0, int16_t w, int16_t h,
		int16_t radius,
		uint16_t color
	);
void gfx_fill_round_rect(
		int16_t x0, int16_t y0, int16_t w, int16_t h,
		int16_t radius,
		uint16_t color
	);

void gfx_draw_triangle(
		int16_t x0, int16_t y0,
		int16_t x1, int16_t y1,
		int16_t x2, int16_t y2,
		uint16_t color
	);
void gfx_fill_triangle(
		int16_t x0, int16_t y0,
		int16_t x1, int16_t y1,
		int16_t x2, int16_t y2,
		uint16_t color
	);


void gfx_draw_char(
		int16_t x, int16_t y,
		uint32_t c,
		uint16_t color,
		uint8_t size
	);
void gfx_set_cursor(int16_t x, int16_t y);
void gfx_set_text_color(uint16_t col);
void gfx_set_font_scale(uint8_t s);
uint8_t gfx_get_font_scale(void);
void gfx_set_text_wrap(bool w);
void gfx_set_font(const font_t *font);
void gfx_puts(const char *);
void gfx_puts2(
		int16_t x, int16_t y,
		const char *s,
		const font_t *font,
		uint16_t col
	);
void gfx_puts3(
		int16_t x, int16_t y,
		const char *s,
		const gfx_alignment_t alignment
	);
void gfx_write(const uint32_t);


uint16_t
gfx_get_char_width(void);
uint16_t
gfx_get_line_height(void);
uint16_t
gfx_get_string_width(const char *);
uint16_t
gfx_get_string_height(const char *s);

uint8_t
gfx_get_text_size(void);
uint16_t
gfx_get_text_color(void);
const font_t *
gfx_get_font(void);
uint8_t
gfx_get_text_wrap(void);


#endif
