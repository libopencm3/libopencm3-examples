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




#include "gfx_locm3.h"

gfx_state_t __gfx_state;

void
gfx_init(uint16_t *surface, int32_t width, int32_t height) {
	*(uint16_t*)&__gfx_state.width_orig  = width;
	*(uint16_t*)&__gfx_state.height_orig = height;
	*(uint32_t*)&__gfx_state.pixel_count = (uint32_t)width*(uint32_t)height;
	__gfx_state.width        = width;
	__gfx_state.height       = height;
	__gfx_state.visible_area = (visible_area_t){0,0,width,height};
	__gfx_state.rotation     = 0;
	__gfx_state.cursor_y     = __gfx_state.cursor_x    = __gfx_state.cursor_x_orig = 0;
	__gfx_state.fontscale    = 1;
	__gfx_state.textcolor    = 0;
	__gfx_state.wrap         = true;
	__gfx_state.surface      = surface;
	__gfx_state.font         = NULL;
}
void gfx_set_surface(uint16_t *surface) {
	__gfx_state.surface = surface;
}
uint16_t *
gfx_get_surface() {
	return __gfx_state.surface;
}

void
gfx_set_surface_visible_area_max() {
	__gfx_state.visible_area = (visible_area_t){0,0,__gfx_state.width,__gfx_state.height};
}
void
gfx_set_surface_visible_area(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
	if (x1<0)  x1=0;
	if (y1<0)  y1=0;
	if (x2<x1) x2=x1;
	if (y2<y1) y2=y1;
	__gfx_state.visible_area = (visible_area_t){x1,y1,x2,y2};
}
visible_area_t
gfx_get_surface_visible_area() {
	return __gfx_state.visible_area;
}


/*
 * just write a RGB565 color to each pixel in the current buffer/surface
 */
void gfx_fill_screen(uint16_t color) {
	uint32_t i; uint16_t *pixel_addr;
	pixel_addr = __gfx_state.surface;
	for (i = 0; i < __gfx_state.pixel_count; i++) {
		*pixel_addr++ = color;
	}
}



/* change the rotation and flip width and height accordingly */
void gfx_rotate(gfx_rotation_t rotation) {
	if (rotation == GFX_ROTATION_0_DEGREES || rotation == GFX_ROTATION_180_DEGREES) {
		__gfx_state.width  = __gfx_state.width_orig;
		__gfx_state.height = __gfx_state.height_orig;
	} else {
		__gfx_state.width  = __gfx_state.height_orig;
		__gfx_state.height = __gfx_state.width_orig;
	}
	__gfx_state.rotation = rotation;
	gfx_set_surface_visible_area_max();
}
uint8_t gfx_get_rotation(void) {
  return __gfx_state.rotation;
}
// Return the size of the display (per current rotation)
uint16_t gfx_width(void) {
  return __gfx_state.width;
}

uint16_t gfx_height(void) {
  return __gfx_state.height;
}


/*
 * draw a single pixel
 * changes buffer addressing (surface) according to orientation
 */
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color) {
	uint16_t *pixel_addr;
	if (x < __gfx_state.visible_area.x1 || x >= __gfx_state.visible_area.x2) {
		return;
	}
	if (y < __gfx_state.visible_area.y1 || y >= __gfx_state.visible_area.y2) {
		return;
	}

	pixel_addr = __gfx_state.surface;
	switch (__gfx_state.rotation) {
		case GFX_ROTATION_0_DEGREES :
			pixel_addr +=                           (x + y*__gfx_state.width_orig);
			break;
		case GFX_ROTATION_180_DEGREES :
			pixel_addr += __gfx_state.pixel_count - (x + y*__gfx_state.width_orig);
			break;
		case GFX_ROTATION_90_DEGREES :
			pixel_addr += __gfx_state.pixel_count - (x*__gfx_state.width_orig + (__gfx_state.width_orig-y));
			break;
		case GFX_ROTATION_270_DEGREES :
			pixel_addr +=                           (x*__gfx_state.width_orig + (__gfx_state.width_orig-y));
			break;
	}
	*pixel_addr = color;

	return;
}

// Bresenham's algorithm - thx wikpedia
void gfx_draw_line(int16_t x0, int16_t y0,
			    int16_t x1, int16_t y1,
			    uint16_t fg) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
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

  for (; x0<=x1; x0++) {
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
void gfx_draw_hline(int16_t x, int16_t y, int16_t length, uint16_t color) {
	if (length<0) {
		length = -length;
		x     -=  length;
	}
	while (length-- != 0) {
		gfx_draw_pixel(x++,y,color);
	}
}
void gfx_draw_vline(int16_t x, int16_t y, int16_t length, uint16_t color) {
	if (length<0) {
		length = -length;
		y     -=  length;
	}
	while (length-- != 0) {
		gfx_draw_pixel(x,y++,color);
	}
}

void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	int16_t x_s,y_e;
	if (w<0) {
		w = -w;
		x-=  w;
	}
	if (h<0) {
		h = -h;
		x-=  h;
	}
	x_s = x;
	y_e = y+h;
	while (w-- != 0) {
		gfx_draw_pixel(x,y  ,color);
		gfx_draw_pixel(x,y_e,color);
		x++;
	}
	x--;
	while (h-- != 0) {
		gfx_draw_pixel(x_s,y,color);
		gfx_draw_pixel(x  ,y,color);
		y++;
	}
}

void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	int16_t x_cp,w_cp;
	while (h--) {
		x_cp=x;
		w_cp=w;
		while (w_cp--) {
			gfx_draw_pixel(x_cp,y,color);
			x_cp++;
		}
		y++;
	}
}




// Draw RGB565 data
void gfx_draw_raw_rbg565_buffer(int16_t x, int16_t y, uint16_t w, uint16_t h, const uint16_t *img) {
	int16_t x_cp,w_cp;
	while (h--) {
		x_cp=x;
		w_cp=w;
		while (w_cp--) {
			gfx_draw_pixel(x_cp,y,*img++);
			x_cp++;
		}
		y++;
	}
}


// Draw a circle outline
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t fg) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  gfx_draw_pixel(x0  , y0+r, fg);
  gfx_draw_pixel(x0  , y0-r, fg);
  gfx_draw_pixel(x0+r, y0  , fg);
  gfx_draw_pixel(x0-r, y0  , fg);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
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

void gfx_draw_circle_helper( int16_t x0, int16_t y0,
               int16_t r, uint8_t cornername, uint16_t fg) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
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

void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r,
			      uint16_t fg) {
  gfx_draw_vline(x0, y0-r, 2*r+1, fg);
  gfx_fill_circle_helper(x0, y0, r, 3, 0, fg);
}

// Used to do circles and roundrects
void gfx_fill_circle_helper(int16_t x0, int16_t y0, int16_t r,
    uint8_t cornername, int16_t delta, uint16_t fg) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

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


// Draw a rounded rectangle
void gfx_draw_round_rect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t fg) {
  // smarter version
  gfx_draw_hline(x+r  , y    , w-2*r, fg); // Top
  gfx_draw_hline(x+r  , y+h-1, w-2*r, fg); // Bottom
  gfx_draw_vline(x    , y+r  , h-2*r, fg); // Left
  gfx_draw_vline(x+w-1, y+r  , h-2*r, fg); // Right
  // draw four corners
  gfx_draw_circle_helper(x+r    , y+r    , r, 1, fg);
  gfx_draw_circle_helper(x+w-r-1, y+r    , r, 2, fg);
  gfx_draw_circle_helper(x+w-r-1, y+h-r-1, r, 4, fg);
  gfx_draw_circle_helper(x+r    , y+h-r-1, r, 8, fg);
}

// Fill a rounded rectangle
void gfx_fill_round_rect(int16_t x, int16_t y, int16_t w,
				 int16_t h, int16_t r, uint16_t fg) {
  // smarter version
  gfx_fill_rect(x+r, y, w-2*r, h, fg);

  // draw four corners
  gfx_fill_circle_helper(x+w-r-1, y+r, r, 1, h-2*r-1, fg);
  gfx_fill_circle_helper(x+r    , y+r, r, 2, h-2*r-1, fg);
}


// Draw a triangle
void gfx_draw_triangle(int16_t x0, int16_t y0,
				int16_t x1, int16_t y1,
				int16_t x2, int16_t y2, uint16_t fg) {
  gfx_draw_line(x0, y0, x1, y1, fg);
  gfx_draw_line(x1, y1, x2, y2, fg);
  gfx_draw_line(x2, y2, x0, y0, fg);
}

// Fill a triangle
void gfx_fill_triangle ( int16_t x0, int16_t y0,
				  int16_t x1, int16_t y1,
				  int16_t x2, int16_t y2, uint16_t fg) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
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

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    gfx_draw_hline(a, y, b-a+1, fg);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    gfx_draw_hline(a, y, b-a+1, fg);
  }
}




/**
 * Text functions
 */

/* Write utf8 text */
void gfx_write(const uint32_t c) {
	if (!__gfx_state.font) return;
	if (c == '\n') {
		__gfx_state.cursor_y += __gfx_state.fontscale*__gfx_state.font->lineheight;
		__gfx_state.cursor_x  = __gfx_state.cursor_x_orig;
	} else if (c == '\r') {
		__gfx_state.cursor_x  = __gfx_state.cursor_x_orig;
	} else {
		if (__gfx_state.wrap && (__gfx_state.cursor_x > (__gfx_state.visible_area.x2 - __gfx_state.fontscale*__gfx_state.font->charwidth))) {
			__gfx_state.cursor_y += __gfx_state.fontscale*__gfx_state.font->lineheight;
			__gfx_state.cursor_x = __gfx_state.cursor_x_orig; //__gfx_state.visible_area.x1;
		}
		gfx_draw_char(__gfx_state.cursor_x, __gfx_state.cursor_y, 
					 c,
					 __gfx_state.textcolor, __gfx_state.fontscale);
		__gfx_state.cursor_x += __gfx_state.fontscale*__gfx_state.font->charwidth;
	}
}

void gfx_puts(const char *s) {
	while (*s) {
		int32_t value;
		s = utf8_read_value(s,&value);
		gfx_write(value);
	}
}
void gfx_puts2(int16_t x, int16_t y, const char *s, const font_t *font, uint16_t col) {
	gfx_set_cursor(x,y);
	gfx_set_font(font);
	gfx_set_text_color(col);
	//gfx_setTextSize(1);
	gfx_puts(s);
}
/* this is not utf8 right now.. */
void gfx_puts3(int16_t x, int16_t y, const char *s, const gfx_alignment_t alignment) {
	if (!__gfx_state.font) return;
	if (!(alignment&GFX_ALIGNMENT_RIGHT)) {
		gfx_set_cursor(x,y);
		gfx_puts(s);
		return;
	}

	const char *s_end = utf8_find_character_in_string(0,s,s+1024);
	const char *next_nl,*last_nl;

	last_nl = utf8_find_character_in_string('\n', s,s_end);
	if (!last_nl) {
		gfx_set_cursor(x-utf8_find_pointer_diff(s,s_end)*__gfx_state.font->charwidth,y);
		gfx_puts(s);
		return;
	}

	uint32_t line_count = 0;
	while (s<s_end) {
		next_nl=utf8_find_character_in_string('\n',s,s_end);
		gfx_set_cursor(x-utf8_find_pointer_diff(s,next_nl)*__gfx_state.font->charwidth,y+line_count*__gfx_state.font->lineheight);
		do {
			int32_t value;
			s = utf8_read_value(s,&value);
			gfx_write(value);
		} while (s!=next_nl);
		s++;line_count++;
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
void gfx_draw_char(int16_t x, int16_t y, uint32_t c,
			    uint16_t col, uint8_t size) {
	uint32_t i, j, bm;
	const char_t   *cp;
	const uint32_t *cp_data_p;

	if (!__gfx_state.font) return;
	/* get the data-index for this char code from the lookup table */
	cp = font_get_char_index(c,__gfx_state.font);
	if (cp == NULL) return;

	cp_data_p = cp->data;
	bm = 1; // bit_mask
	for (j=cp->bbox.y1; j<cp->bbox.y2; j++) {
		for (i=cp->bbox.x1; i<cp->bbox.x2; i++) {
			if (*cp_data_p & bm) {
				// default size
				if (size == 1) gfx_draw_pixel(x+i, y+j, col);
				// big size
				else           gfx_fill_rect (x+i*size, y+j*size, size, size, col);
			}
			bm <<= 1;
			// overflow
			if (!bm) {
				bm = 1;
				cp_data_p++;
			}
		}
	}
}

void gfx_set_cursor(int16_t x, int16_t y) {
	__gfx_state.cursor_x_orig = x;
	__gfx_state.cursor_x = x;
	__gfx_state.cursor_y = y;
}

void gfx_set_font_scale(uint8_t s) {
	__gfx_state.fontscale = (s > 0) ? s : 1;
}

void gfx_set_text_color(uint16_t col) {
	__gfx_state.textcolor   = col;
}
void gfx_set_font(const font_t *font) {
	__gfx_state.font = font;
}

void gfx_set_text_wrap(uint8_t w) {
	__gfx_state.wrap = w;
}

uint16_t
gfx_get_char_width() {
	if (!__gfx_state.font) return 0;
	return __gfx_state.font->charwidth*__gfx_state.fontscale;
}
uint16_t
gfx_get_line_height() {
	if (!__gfx_state.font) return 0;
	return __gfx_state.font->lineheight*__gfx_state.fontscale;
}
uint16_t
gfx_get_string_width(char *s) {
	return strlen(s) * gfx_get_char_width();
}

uint8_t
gfx_get_font_scale() {
	return __gfx_state.fontscale;
}
uint16_t
gfx_get_text_color() {
	return __gfx_state.textcolor;
}
const font_t *
gfx_get_font() {
	return __gfx_state.font;
}
uint8_t
gfx_get_text_wrap() {
	return __gfx_state.wrap;
}

