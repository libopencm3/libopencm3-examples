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
 *
 *
 *
 * This code is based on a fork off a graphics example for libopencm3 made
 * by Charles McManis in 2014.
 * The basic drawing functions (circle, line, round-rectangle) are based on
 * the simple graphics library written by the folks at AdaFruit.
 * [Adafruit_GFX.cpp](/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp)
 *
 * Graphics lib, possible history
 * --Probably Knuth, Ada and/or Plato
 * --WIKIPEDIA
 * --ADAFRUIT      (?, 2013)
 * --Chuck McManis (2013, 2014)
 * --Tilen Majerle (2014)
 * --Oliver Meier  (2014+)
 */


#include "gfx_locm3.h"

gfx_state_t __gfx_state = {0};

/*
 * Make sure the surface is aligned for 32bit!
 */
void
gfx_init(uint16_t *surface, int32_t width, int32_t height)
{
	__gfx_state.is_offscreen_rendering = 0;

	*(uint16_t *)&__gfx_state.width_orig  = width;
	*(uint16_t *)&__gfx_state.height_orig = height;
	*(uint32_t *)&__gfx_state.pixel_count = (uint32_t)width*(uint32_t)height;
	__gfx_state.width        = width;
	__gfx_state.height       = height;
	__gfx_state.visible_area = (visible_area_t){0, 0, width, height};
	__gfx_state.rotation     = 0;
	__gfx_state.cursor_y     = __gfx_state.cursor_x = __gfx_state.cursor_x_orig
							 = 0;
	__gfx_state.fontscale    = 1;
	__gfx_state.textcolor    = 0;
	__gfx_state.wrap         = true;
	__gfx_state.surface      = surface;
	__gfx_state.font         = NULL;
}

static gfx_state_t __gfx_state_bkp = {0};

void gfx_set_surface(uint16_t *surface)
{
	if (__gfx_state.is_offscreen_rendering) {
		__gfx_state_bkp.surface = surface;
	} else {
		__gfx_state.surface = surface;
	}
}
uint16_t *
gfx_get_surface()
{
	if (__gfx_state.is_offscreen_rendering) {
		return __gfx_state_bkp.surface;
	} else {
		return __gfx_state.surface;
	}
}

void gfx_offscreen_rendering_begin(
		uint16_t *surface,
		int32_t width,
		int32_t height
) {
	if (!__gfx_state.is_offscreen_rendering) {
		/*gfx_state_bkp = __gfx_state; ???*/
		memcpy(&__gfx_state_bkp, &__gfx_state, sizeof(gfx_state_t));
	}
	gfx_init(surface, width, height);
	__gfx_state.is_offscreen_rendering = 1;
}
void gfx_offscreen_rendering_end()
{
	if (__gfx_state.is_offscreen_rendering) {
		memcpy(&__gfx_state, &__gfx_state_bkp, sizeof(gfx_state_t));
	}
}

void
gfx_set_clipping_area_max()
{
	__gfx_state.visible_area =
			(visible_area_t){
				0, 0, __gfx_state.width, __gfx_state.height
			};
}
void
gfx_set_clipping_area(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	if (x1 < 0) {
		x1 = 0;
	}
	if (y1 < 0) {
		y1 = 0;
	}
	if (x2 < x1) {
		x2 = x1;
	}
	if (y2 < y1) {
		y2 = y1;
	}
	__gfx_state.visible_area = (visible_area_t){x1, y1, x2, y2};
}
visible_area_t
gfx_get_surface_visible_area() {
	return __gfx_state.visible_area;
}


/*
 * just write a RGB565 color to each pixel in the current buffer/surface
 * this function assumes 32bit aligned buffer!
 */
void gfx_fill_screen(uint16_t color)
{
	uint32_t color2 = (color<<16)|color;

	uint32_t i; uint32_t *pixel_addr;
	pixel_addr = (uint32_t *)__gfx_state.surface;
	for (i = 0; i < __gfx_state.pixel_count/2; i++) {
		*pixel_addr++ = color2;
	}
	if (__gfx_state.pixel_count & 1) {
		*(__gfx_state.surface+(__gfx_state.pixel_count-1)) = color;
	}

	/*
	uint32_t i; uint16_t *pixel_addr;
	pixel_addr = __gfx_state.surface;
	for (i = 0; i < __gfx_state.pixel_count&1; i++) {
		*pixel_addr++ = color;
	}
	*/
}



/* change the rotation and flip width and height accordingly */
void gfx_set_rotation(gfx_rotation_t rotation)
{
	if ((rotation == GFX_ROTATION_0_DEGREES)
	 || (rotation == GFX_ROTATION_180_DEGREES)
	) {
		__gfx_state.width  = __gfx_state.width_orig;
		__gfx_state.height = __gfx_state.height_orig;
	} else {
		__gfx_state.width  = __gfx_state.height_orig;
		__gfx_state.height = __gfx_state.width_orig;
	}
	__gfx_state.rotation = rotation;
	gfx_set_clipping_area_max();
}
gfx_rotation_t gfx_get_rotation(void)
{
	return __gfx_state.rotation;
}
/* Return the size of the display (per current rotation) */
uint16_t gfx_width(void)
{
	return __gfx_state.width;
}

uint16_t gfx_height(void)
{
	return __gfx_state.height;
}


/* GFX_GLOBAL_DISPLAY_SCALE can be  used as a zoom function
 * while debugging code..
 *
 * NOTE: This breaks offscreen rendering!!
 */
#define GFX_GLOBAL_DISPLAY_SCALE 1

static inline uint16_t *gfx_get_pixel_address(int16_t x, int16_t y)
{
	uint16_t *pixel_addr;
	pixel_addr = __gfx_state.surface;
	switch (__gfx_state.rotation) {
	case GFX_ROTATION_0_DEGREES:
		pixel_addr +=                           (x + y*__gfx_state.width_orig);
		break;
	case GFX_ROTATION_270_DEGREES:
		pixel_addr +=                           (x*__gfx_state.width_orig + __gfx_state.width_orig - y)         - GFX_GLOBAL_DISPLAY_SCALE;
		break;
	case GFX_ROTATION_180_DEGREES:
		pixel_addr += __gfx_state.pixel_count - (x + (y + (GFX_GLOBAL_DISPLAY_SCALE-1)) * __gfx_state.width_orig) - GFX_GLOBAL_DISPLAY_SCALE;
		break;
	case GFX_ROTATION_90_DEGREES:
		pixel_addr += __gfx_state.pixel_count - ((x + (GFX_GLOBAL_DISPLAY_SCALE-1))     * __gfx_state.width_orig + __gfx_state.width_orig-y);
		break;
	}
	return pixel_addr;
}


#if GFX_GLOBAL_DISPLAY_SCALE > 1
#define GFX_GLOBAL_DISPLAY_SCALE_OFFSET_X -10
#define GFX_GLOBAL_DISPLAY_SCALE_OFFSET_Y -40

/*
 * draw a single pixel
 * changes buffer addressing (surface) according to orientation
 */
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
	x += GFX_GLOBAL_DISPLAY_SCALE_OFFSET_X;
	y += GFX_GLOBAL_DISPLAY_SCALE_OFFSET_Y;
	x *= GFX_GLOBAL_DISPLAY_SCALE;
	y *= GFX_GLOBAL_DISPLAY_SCALE;

	if (
		x <  __gfx_state.visible_area.x1
	 || x >= __gfx_state.visible_area.x2 - GFX_GLOBAL_DISPLAY_SCALE+1
	 || y <  __gfx_state.visible_area.y1
	 || y >= __gfx_state.visible_area.y2 - GFX_GLOBAL_DISPLAY_SCALE+1
	) {
		return;
	}

	uint16_t *pa = gfx_get_pixel_address(x, y);
	for (uint32_t sa = 0; sa < GFX_GLOBAL_DISPLAY_SCALE; sa++) {
		for (uint32_t sb = 0; sb < GFX_GLOBAL_DISPLAY_SCALE; sb++) {
			*(pa+sb) = color;
		}
		pa += __gfx_state.width_orig;
	}

	return;
}

/*
 * get a single pixel
 */
int32_t gfx_get_pixel(int16_t x, int16_t y)
{
	x += GFX_GLOBAL_DISPLAY_SCALE_OFFSET_X;
	y += GFX_GLOBAL_DISPLAY_SCALE_OFFSET_Y;
	x *= GFX_GLOBAL_DISPLAY_SCALE;
	y *= GFX_GLOBAL_DISPLAY_SCALE;

	if (
		x <  __gfx_state.visible_area.x1
	 || x >= __gfx_state.visible_area.x2 - GFX_GLOBAL_DISPLAY_SCALE+1
	 || y <  __gfx_state.visible_area.y1
	 || y >= __gfx_state.visible_area.y2 - GFX_GLOBAL_DISPLAY_SCALE+1
	) {
		return -1;
	}

	return *gfx_get_pixel_address(x, y);
}

#else

/*
 * draw a single pixel
 * changes buffer addressing (surface) according to orientation
 */
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color)
{
	if (
		x <  __gfx_state.visible_area.x1
	 || x >= __gfx_state.visible_area.x2
	 || y <  __gfx_state.visible_area.y1
	 || y >= __gfx_state.visible_area.y2
	) {
		return;
	}

	*gfx_get_pixel_address(x, y) = color;

	return;
}

/*
 * get a single pixel
 */
int32_t gfx_get_pixel(int16_t x, int16_t y)
{
	if (
		x <  __gfx_state.visible_area.x1
	 || x >= __gfx_state.visible_area.x2
	 || y <  __gfx_state.visible_area.y1
	 || y >= __gfx_state.visible_area.y2
	) {
		return -1;
	}

	return *gfx_get_pixel_address(x, y);
}
#endif

typedef struct {
	int16_t ud_dir;
	int16_t x0, x1;
	int16_t y; /* actually y-ud_dir.. */
} fill_segment_t;
typedef struct {
	size_t size;
	fill_segment_t *data;
	size_t count;
	fill_segment_queue_statistics_t stats;
} fill_segment_queue_t;

//#define SHOW_FILLING
#ifdef SHOW_FILLING
#include "lcd_ili9341.h"
#include <assert.h>
#include "clock.h"
#endif


static inline void gfx_flood_fill4_add_if_found(
		fill_segment_queue_t *queue,
		int16_t ud_dir,
		int16_t xa0, int16_t xa1,
		int16_t y,
		uint16_t old_color
) {
	y += ud_dir;
	while ((xa0 <= xa1)) {
		if (gfx_get_pixel(xa0, y) == old_color) {
			if (queue->count < queue->size) {
				queue->data[queue->count++] =
						(fill_segment_t) {
							.ud_dir = ud_dir,
							.x0     = xa0,
							.x1     = xa1,
							.y      = y-ud_dir
						};
			} else {
				queue->stats.overflows++;
			}
			return;
		}
		xa0++;
	}
}

/* warning! this function might be very slow and fail:) */
fill_segment_queue_statistics_t
gfx_flood_fill4(
		int16_t x, int16_t y,
		uint16_t old_color, uint16_t new_color,
		uint8_t fill_segment_buf[], size_t fill_segment_buf_size
) {
	if (old_color == new_color) {
		return (fill_segment_queue_statistics_t){0};
	}

	if (gfx_get_pixel(x, y) != old_color) {
		return (fill_segment_queue_statistics_t){0};
	}

	fill_segment_queue_t queue = {
		.size = fill_segment_buf_size/sizeof(fill_segment_t),
		.data = (fill_segment_t *)fill_segment_buf,
		.count = 0,
		.stats = (fill_segment_queue_statistics_t) {
				.count_max = 0,
				.count_total = 0,
				.overflows = 0,
		}
	};


	int16_t sx, sy;

	int16_t ud_dir_init = 1;
	int16_t ud_dir = 1;

	int16_t x0, x1, x0l, x1l, x0ll, x1ll;

	bool pixel_drawn = false;

	sx = x;
	sy = y;

	/* process first line */
	x--; /* x is always increased first! */
	while (gfx_get_pixel(++x, y) == old_color) {
		gfx_draw_pixel(x, y, new_color);
		pixel_drawn = true;
	}
	x1l = x1ll = x-1;
	x  = sx;
	while (gfx_get_pixel(--x, y) == old_color) {
		gfx_draw_pixel(x, y, new_color);
		pixel_drawn = true;
	}
	x0l = x0ll = x+1;

#ifdef SHOW_FILLING
	ltdc_set_fbuffer_address(LTDC_LAYER_2, (uint32_t)__gfx_state.surface);
#endif

	while (pixel_drawn) {
		pixel_drawn = false;

#ifdef SHOW_FILLING
		if (!LTDC_SRCR_IS_RELOADING()) {
			ltdc_reload(LTDC_SRCR_RELOAD_VBR);
		}
#endif
		y += ud_dir;

		/* find x start point (must be between x0 and x1 of the last iteration) */
		sx  = x0l;
		while ((sx < x1l) && (gfx_get_pixel(sx, y) != old_color)) {
			sx++;
		}

		bool sx_was_oc = gfx_get_pixel(sx, y) == old_color;

		/** middle to right */
		x  = sx-1;
		/* fill additional adjacent old-colored pixels */
		while (gfx_get_pixel(++x, y) == old_color) {
			gfx_draw_pixel(x, y, new_color);
#ifdef SHOW_FILLING
		msleep_loop(1);
#endif
			pixel_drawn = true;
		}
		x1 = x-1;
		/* check if x1 is bigger compared to the last line */
		if (x1 > x1l+1) {
			if (queue.count < queue.size) {
				queue.data[queue.count++] =
						(fill_segment_t) {
							.ud_dir = -ud_dir,
							.x0     = x1l,
							.x1     = x1,
							.y      = y
						};
			} else {
				queue.stats.overflows++;
			}
		} else {
			/* fill all lastline-adjacent old-colored pixels */
			bool adjacent_pixel_drawn = false;
			int16_t xa0=0, xa1;
			while ((x < x1l) || adjacent_pixel_drawn) {
				if (gfx_get_pixel(++x, y) == old_color) {
					gfx_draw_pixel(x, y, new_color);
#ifdef SHOW_FILLING
		msleep_loop(1);
#endif
					if (!adjacent_pixel_drawn) {
						adjacent_pixel_drawn = true;
						xa0 = x;
					}
				} else
				if (adjacent_pixel_drawn) {
					adjacent_pixel_drawn = false;
					xa1 = x-1;

					gfx_flood_fill4_add_if_found(&queue, ud_dir, xa0, xa1, y, old_color);
					if (xa1 > x1l+1) {
						gfx_flood_fill4_add_if_found(&queue, -ud_dir, x1l+1, xa1, y, old_color);
					}
				}
			}
		}
		x1l = x1;


		/** middle-1 to left */
		x  = sx;
		if ((sx > x0l) || sx_was_oc) {
			/* fill additional adjacent old-colored pixels */
			while (gfx_get_pixel(--x, y) == old_color) {
				gfx_draw_pixel(x, y, new_color);
#ifdef SHOW_FILLING
		msleep_loop(1);
#endif
				pixel_drawn = true;
			}
			x0 = x+1;
		} else {
			x0 = sx;
			x--;
		}
		/* check if x0 is smaller compared to the last line */
		if (x0 < x0l-1) {
			if (queue.count < queue.size) {
				queue.data[queue.count++] =
						(fill_segment_t) {
							.ud_dir = -ud_dir,
							.x0     = x0,
							.x1     = x0l,
							.y      = y
						};
			} else {
				queue.stats.overflows++;
			}
		} else {
			/* fill all lastline-adjacent old-colored pixels */
			bool adjacent_pixel_drawn = false;
			int16_t xa0, xa1 = 0;
			while ((x > x0l) || adjacent_pixel_drawn) {
				if (gfx_get_pixel(--x, y) == old_color) {
					gfx_draw_pixel(x, y, new_color);
#ifdef SHOW_FILLING
		msleep_loop(1);
#endif
					if (!adjacent_pixel_drawn) {
						adjacent_pixel_drawn = true;
						xa1 = x;
					}
				} else
				if (adjacent_pixel_drawn) {
					adjacent_pixel_drawn = false;
					xa0 = x+1;

					gfx_flood_fill4_add_if_found(&queue, ud_dir, xa0, xa1, y, old_color);
					if (xa0 < x0l-1) {
						gfx_flood_fill4_add_if_found(&queue, -ud_dir, xa0, x0l-1, y, old_color);
					}
				}
			}
		}
		x0l = x0;

		/** no pixel draw, check if we're already filling upwards */
		if (!pixel_drawn) {
			if (ud_dir == ud_dir_init) {
				pixel_drawn = true;
				ud_dir = -1;
				y = sy;
				x0l = x0ll;
				x1l = x1ll;
			} else {
				ud_dir_init = 2; /* filling saved segments */

				queue.stats.count_total++;
				if (queue.stats.count_max < queue.count) {
					queue.stats.count_max = queue.count;
				}

				if (queue.count) {
					queue.count--;
					ud_dir = queue.data[queue.count].ud_dir;
					x0l = queue.data[queue.count].x0;
					x1l = queue.data[queue.count].x1;
					/* sx  = (x0l+x1l)/2; */
					y   = queue.data[queue.count].y;
					pixel_drawn = true;

#ifdef SHOW_FILLING
					assert(x0l <= x1l);
#endif
				}
			}
		}
	}

#ifdef SHOW_FILLING
	msleep_loop(5000);
#endif

	return queue.stats;
}


/* Bresenham's algorithm - thx wikpedia */
void gfx_draw_line(int16_t x0, int16_t y0,
			    int16_t x1, int16_t y1,
			    uint16_t fg) {
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap_i16(x0, y0);
		swap_i16(x1, y1);
	}

	if (x0 > x1) {
		swap_i16(x0, x1);
		swap_i16(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1) {
		ystep = 1;
	} else {
		ystep = -1;
	}

	for (; x0 <= x1; x0++) {
		if (steep) {
			gfx_draw_pixel(y0, x0, fg);
		} else {
			gfx_draw_pixel(x0, y0, fg);
		}
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}
void gfx_draw_hline(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	if (length < 0) {
		length = -length;
		x     -=  length;
	}
	while (length--) {
		gfx_draw_pixel(x++, y, color);
	}
}
void gfx_draw_vline(int16_t x, int16_t y, int16_t length, uint16_t color)
{
	if (length < 0) {
		length = -length;
		y     -=  length;
	}
	while (length--) {
		gfx_draw_pixel(x, y++, color);
	}
}

void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	int16_t x_s, y_e;
	if (w < 0) {
		w = -w;
		x -= w;
	}
	if (h < 0) {
		h = -h;
		x -= h;
	}
	x_s = x;
	y_e = y+h-1;
	while (w-- != 0) {
		gfx_draw_pixel(x, y  , color);
		gfx_draw_pixel(x, y_e, color);
		x++;
	}
	x--;
	while (h-- != 0) {
		gfx_draw_pixel(x_s, y, color);
		gfx_draw_pixel(x  , y, color);
		y++;
	}
}

void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
			    uint16_t fg) {

	int16_t i;
	for (i = x; i < x+w; i++) {
		gfx_draw_vline(i, y, h, fg);
	}
}




/* Draw RGB565 data */
void gfx_draw_raw_rbg565_buffer(
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const uint16_t *img
) {
	int16_t x_cp, w_cp;
	while (h--) {
		x_cp = x;
		w_cp = w;
		while (w_cp--) {
			gfx_draw_pixel(x_cp, y, *img++);
			x_cp++;
		}
		y++;
	}
}

/* Draw RGB565 data, do not draw ignored_color */
void gfx_draw_raw_rbg565_buffer_ignore_color(
		int16_t x, int16_t y, uint16_t w, uint16_t h,
		const uint16_t *img,
		uint16_t ignored_color
) {
	int16_t x_cp, w_cp;
	while (h--) {
		x_cp = x;
		w_cp = w;
		while (w_cp--) {
			if (*img != ignored_color) {
				gfx_draw_pixel(x_cp, y, *img);
			}
			img++;

			x_cp++;
		}
		y++;
	}
}

/* Draw a circle outline */
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t fg)
{
	int16_t f = 1 - r;
	int16_t dd_f_x = 1;
	int16_t dd_f_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	gfx_draw_pixel(x0  , y0+r, fg);
	gfx_draw_pixel(x0  , y0-r, fg);
	gfx_draw_pixel(x0+r, y0  , fg);
	gfx_draw_pixel(x0-r, y0  , fg);

	while (x < y) {
		if (f >= 0) {
			y--;
			dd_f_y += 2;
			f += dd_f_y;
		}
		x++;
		dd_f_x += 2;
		f += dd_f_x;

		gfx_draw_pixel(x0 + x, y0 + y, fg);
		gfx_draw_pixel(x0 - x, y0 + y, fg);
		gfx_draw_pixel(x0 + x, y0 - y, fg);
		gfx_draw_pixel(x0 - x, y0 - y, fg);
		gfx_draw_pixel(x0 + y, y0 + x, fg);
		gfx_draw_pixel(x0 - y, y0 + x, fg);
		gfx_draw_pixel(x0 + y, y0 - x, fg);
		gfx_draw_pixel(x0 - y, y0 - x, fg);
	}
}

void gfx_draw_circle_helper(
		int16_t x0, int16_t y0,
		int16_t r, uint8_t cornername,
		uint16_t fg
) {
	int16_t f     = 1 - r;
	int16_t dd_f_x = 1;
	int16_t dd_f_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			dd_f_y += 2;
			f     += dd_f_y;
		}
		x++;
		dd_f_x += 2;
		f     += dd_f_x;
		if (cornername & 0x4) {
			gfx_draw_pixel(x0 + x, y0 + y, fg);
			gfx_draw_pixel(x0 + y, y0 + x, fg);
		}
		if (cornername & 0x2) {
			gfx_draw_pixel(x0 + x, y0 - y, fg);
			gfx_draw_pixel(x0 + y, y0 - x, fg);
		}
		if (cornername & 0x8) {
			gfx_draw_pixel(x0 - y, y0 + x, fg);
			gfx_draw_pixel(x0 - x, y0 + y, fg);
		}
		if (cornername & 0x1) {
			gfx_draw_pixel(x0 - y, y0 - x, fg);
			gfx_draw_pixel(x0 - x, y0 - y, fg);
		}
	}
}

void gfx_fill_circle(
		int16_t x0, int16_t y0,
		int16_t r,
		uint16_t fg
) {
	gfx_draw_vline(x0, y0-r, 2*r+1, fg);
	gfx_fill_circle_helper(x0, y0, r, 3, 0, fg);
}

/* Used to do circles and roundrects */
void gfx_fill_circle_helper(
		int16_t x0, int16_t y0,
		int16_t r, uint8_t cornername, int16_t delta,
		uint16_t fg
) {
	int16_t f     = 1 - r;
	int16_t dd_f_x = 1;
	int16_t dd_f_y = -2 * r;
	int16_t x     = 0;
	int16_t y     = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			dd_f_y += 2;
			f     += dd_f_y;
		}
		x++;
		dd_f_x += 2;
		f     += dd_f_x;

		if (cornername & 0x1) {
			gfx_draw_vline(x0+x, y0-y, 2*y+1+delta, fg);
			gfx_draw_vline(x0+y, y0-x, 2*x+1+delta, fg);
		}
		if (cornername & 0x2) {
			gfx_draw_vline(x0-x, y0-y, 2*y+1+delta, fg);
			gfx_draw_vline(x0-y, y0-x, 2*x+1+delta, fg);
		}
	}
}


/* Draw a rounded rectangle */
void gfx_draw_round_rect(
		int16_t x, int16_t y, int16_t w, int16_t h,
		int16_t r,
		uint16_t fg
) {
	/* smarter version */
	gfx_draw_hline(x+r  , y    , w-2*r, fg); /* Top */
	gfx_draw_hline(x+r  , y+h-1, w-2*r, fg); /* Bottom */
	gfx_draw_vline(x    , y+r  , h-2*r, fg); /* Left */
	gfx_draw_vline(x+w-1, y+r  , h-2*r, fg); /* Right */
	/* draw four corners */
	gfx_draw_circle_helper(x+r    , y+r    , r, 1, fg);
	gfx_draw_circle_helper(x+w-r-1, y+r    , r, 2, fg);
	gfx_draw_circle_helper(x+w-r-1, y+h-r-1, r, 4, fg);
	gfx_draw_circle_helper(x+r    , y+h-r-1, r, 8, fg);
}

/* Fill a rounded rectangle */
void gfx_fill_round_rect(
		int16_t x, int16_t y, int16_t w, int16_t h,
		int16_t r,
		uint16_t fg
) {
	/* smarter version */
	gfx_fill_rect(x+r, y, w-2*r, h, fg);

	/* draw four corners */
	gfx_fill_circle_helper(x+w-r-1, y+r, r, 1, h-2*r-1, fg);
	gfx_fill_circle_helper(x+r    , y+r, r, 2, h-2*r-1, fg);
}


/* Draw a triangle */
void gfx_draw_triangle(
		int16_t x0, int16_t y0,
		int16_t x1, int16_t y1,
		int16_t x2, int16_t y2,
		uint16_t fg
) {
	gfx_draw_line(x0, y0, x1, y1, fg);
	gfx_draw_line(x1, y1, x2, y2, fg);
	gfx_draw_line(x2, y2, x0, y0, fg);
}

/* Fill a triangle */
void gfx_fill_triangle(
		int16_t x0, int16_t y0,
		int16_t x1, int16_t y1,
		int16_t x2, int16_t y2,
		uint16_t fg
) {
	int16_t a, b, y, last;

	/* Sort coordinates by Y order (y2 >= y1 >= y0) */
	if (y0 > y1) {
		swap_i16(y0, y1); swap_i16(x0, x1);
	}
	if (y1 > y2) {
		swap_i16(y2, y1); swap_i16(x2, x1);
	}
	if (y0 > y1) {
		swap_i16(y0, y1); swap_i16(x0, x1);
	}

	/* Handle awkward all-on-same-line case as its own thing */
	if (y0 == y2) {
		a = b = x0;
		if (x1 < a) {
			a = x1;
		} else
		if (x1 > b) {
			b = x1;
		}
		if (x2 < a) {
			a = x2;
		} else
		if (x2 > b) {
			b = x2;
		}
		gfx_draw_hline(a, y0, b-a+1, fg);
		return;
	}

	int16_t
	dx01 = x1 - x0,
	dy01 = y1 - y0,
	dx02 = x2 - x0,
	dy02 = y2 - y0,
	dx12 = x2 - x1,
	dy12 = y2 - y1,
	sa   = 0,
	sb   = 0;

	/* For upper part of triangle, find scanline crossings for segments
	 * 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	 * is included here (and second loop will be skipped, avoiding a /0
	 * error there), otherwise scanline y1 is skipped here and handled
	 * in the second loop...which also avoids a /0 error here if y0=y1
	 * (flat-topped triangle).
	 */
	if (y1 == y2) {
		last = y1;   /* Include y1 scanline */
	} else {
		last = y1-1; /* Skip it */
	}

	for (y = y0; y <= last; y++) {
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		/* longhand:
		a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b) {
			swap_i16(a, b);
		}
		gfx_draw_hline(a, y, b-a+1, fg);
	}

	/* For lower part of triangle, find scanline crossings for segments
	 * 0-2 and 1-2.  This loop is skipped if y1=y2.
	 */
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		/* longhand:
		a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
		b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
		*/
		if (a > b) {
			swap_i16(a, b);
		}
		gfx_draw_hline(a, y, b-a+1, fg);
	}
}




/**
 * Text functions
 */

/* Write utf8 text */
void gfx_write(const uint32_t c)
{
	if (!__gfx_state.font) {
		return;
	}
	if (c == '\n') {
		__gfx_state.cursor_y += gfx_get_line_height();
		__gfx_state.cursor_x  = __gfx_state.cursor_x_orig;
	} else if (c == '\r') {
		__gfx_state.cursor_x  = __gfx_state.cursor_x_orig;
	} else {
		if (__gfx_state.wrap
		 && (
			__gfx_state.cursor_x
		  > (
				__gfx_state.visible_area.x2
			  - gfx_get_char_width()
		))) {
			__gfx_state.cursor_x = __gfx_state.cursor_x_orig;
			__gfx_state.cursor_y += gfx_get_line_height();
		}
		gfx_draw_char(
				__gfx_state.cursor_x, __gfx_state.cursor_y,
				c,
				__gfx_state.textcolor, __gfx_state.fontscale
			);
		__gfx_state.cursor_x += gfx_get_char_width();
	}
}

void gfx_puts(const char *s) {
	while (*s) {
		int32_t value;
		s = utf8_read_value(s, &value);
		gfx_write(value);
	}
}
void gfx_puts4(const char *s, uint32_t max_len) {
	while (*s && max_len--) {
		int32_t value;
		s = utf8_read_value(s, &value);
		gfx_write(value);
	}
}
void gfx_puts2(
		int16_t x, int16_t y,
		const char *s,
		const font_t *font,
		uint16_t col
) {
	gfx_set_cursor(x, y);
	gfx_set_font(font);
	gfx_set_text_color(col);
	gfx_puts(s);
}
/* this is not utf8 right now.. */
void gfx_puts3(
		int16_t x, int16_t y,
		const char *s,
		const gfx_alignment_t alignment
) {
	const char *s_end;
	const char *next_nl; //, *last_nl;

	switch (alignment) {
	case GFX_ALIGNMENT_TOP:
	case GFX_ALIGNMENT_BOTTOM:
	case GFX_ALIGNMENT_LEFT:
		gfx_set_cursor(x, y);
		gfx_puts(s);
		return;

	case GFX_ALIGNMENT_RIGHT:
		s_end = utf8_find_character_in_string(0, s, s+1024);
		next_nl = utf8_find_character_in_string('\n', s, s_end);
		if (next_nl == s_end) {
			gfx_set_cursor(
					x
					- utf8_find_pointer_diff(s, s_end)
					* gfx_get_char_width(),
					y
				);
			gfx_puts(s);
		} else {
			uint32_t line_count = 0;
			while (s < s_end) {
				next_nl = utf8_find_character_in_string('\n', s, s_end);
				gfx_set_cursor(
						x
						- utf8_find_pointer_diff(s, next_nl)
						* gfx_get_char_width(),
						y
						+ line_count
						* gfx_get_line_height()
					);
				do {
					int32_t value;
					s = utf8_read_value(s, &value);
					gfx_write(value);
				} while (s != next_nl);
				s++; line_count++;
			}
		}
		break;

	case GFX_ALIGNMENT_CENTER:
		/* TODO correct rounding on /2 */
		s_end = utf8_find_character_in_string(0, s, s+1024);
		next_nl = utf8_find_character_in_string('\n', s, s_end);
		if (0) { //next_nl == s_end) {
			gfx_set_cursor(
					x-(utf8_find_pointer_diff(s, s_end)
					* gfx_get_char_width())/2,
				y
			);
			gfx_puts(s);
		} else {
//			/* find longest line */
//			uint32_t line_length, longest_line;
//			longest_line = utf8_find_pointer_diff(s, next_nl);
//			last_nl = next_nl+1;
//			while (last_nl < s_end) {
//				next_nl = utf8_find_character_in_string('\n', last_nl, s_end);
//				line_length = utf8_find_pointer_diff(last_nl, next_nl);
//				if (longest_line < line_length) {
//					longest_line = line_length;
//				}
//				last_nl = next_nl+1;
//			}

			/* print lines */
			uint32_t line_count = 0;
			while (s < s_end) {
				next_nl = utf8_find_character_in_string('\n', s, s_end);
				gfx_set_cursor(
						x
						-(utf8_find_pointer_diff(s, next_nl)
						* gfx_get_char_width())/2,
						//- (longest_line*gfx_get_char_width())
//						- ((longest_line
//							- utf8_find_pointer_diff(s, next_nl)
//						) * gfx_get_char_width())/2,
						y
						+ line_count*gfx_get_line_height());
				do {
					int32_t value;
					s = utf8_read_value(s, &value);
					gfx_write(value);
				} while (s != next_nl);
				s++; line_count++;
			}
		}
		break;
	}
}

/**
 * This function gets the index of the char for the font data
 * converts it's bit array address to a 32bit and mask
 * and draws a point of size to it's location
 *
 * @param x    x position
 * @param y    y position
 * @param c    utf8 char id
 * @param col  RGB565 color
 * @param size Size in multiples of 1
 */
void gfx_draw_char(
		int16_t x, int16_t y,
		uint32_t c,
		uint16_t col,
		uint8_t size
) {
	uint32_t i, j, bm;
	const char_t   *cp;
	const uint32_t *cp_data_p;

	if (!__gfx_state.font) {
		return;
	}
	/* get the data-index for this char code from the lookup table */
	cp = font_get_char_index(c, __gfx_state.font);
	if (cp == NULL) {
		return;
	}

	cp_data_p = cp->data;
	bm = 1; /* bit_mask */
	for (j = cp->bbox.y1; j < cp->bbox.y2; j++) {
		for (i = cp->bbox.x1; i < cp->bbox.x2; i++) {
			if (*cp_data_p & bm) {
				/* default size */
				if (size == 1) {
					gfx_draw_pixel(x+i, y+j, col);
				} else {
				/* big size */
					gfx_fill_rect(x+i*size, y+j*size, size, size, col);
				}
			}
			/* overflow */
			if (bm == 0x80000000) {
				bm = 1;
				cp_data_p++;
			} else {
				bm <<= 1;
			}
		}
	}
}

void gfx_set_cursor(int16_t x, int16_t y)
{
	__gfx_state.cursor_x_orig = x;
	__gfx_state.cursor_x = x;
	__gfx_state.cursor_y = y;
}

void gfx_set_font_scale(uint8_t s)
{
	__gfx_state.fontscale = (s > 0) ? s : 1;
}
uint8_t gfx_get_font_scale()
{
	return __gfx_state.fontscale;
}

void gfx_set_text_color(uint16_t col)
{
	__gfx_state.textcolor   = col;
}
void gfx_set_font(const font_t *font)
{
	__gfx_state.font = font;
}

void gfx_set_text_wrap(bool w)
{
	__gfx_state.wrap = w;
}

uint16_t
gfx_get_char_width()
{
	if (!__gfx_state.font) {
		return 0;
	} else {
		return __gfx_state.font->charwidth*__gfx_state.fontscale;
	}
}
uint16_t
gfx_get_line_height()
{
	if (!__gfx_state.font) {
		return 0;
	}
	return __gfx_state.font->lineheight*__gfx_state.fontscale;
}
uint16_t
gfx_get_string_width(const char *s)
{
	uint16_t cnt, cnt_max;
	cnt = cnt_max = 0;
	while (1) {
		if ((*s == 0) || (*s == '\n') || (*s == '\r')) {
			if (cnt_max < cnt) {
				cnt_max = cnt;
			}

			if (*s == 0) {
				break;
			}
			cnt = 0;
		} else {
			cnt++;
		}
		s++;
	}
	return cnt_max * gfx_get_char_width();
}
uint16_t
gfx_get_string_height(const char *s)
{
	uint16_t cnt;
	cnt = 1;
	while (*s) {
		if (*s == '\n') {
			cnt++;
		}
		s++;
	}
	return cnt * gfx_get_line_height();
}
uint8_t
gfx_get_text_size()
{
	return __gfx_state.fontscale;
}
uint16_t
gfx_get_text_color()
{
	return __gfx_state.textcolor;
}
const font_t *
gfx_get_font()
{
	return __gfx_state.font;
}
uint8_t
gfx_get_text_wrap()
{
	return __gfx_state.wrap;
}

