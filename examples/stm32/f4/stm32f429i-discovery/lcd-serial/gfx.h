/*
 * A simple port of the AdaFruit minimal graphics code to my
 * demo code.
 */
#ifndef _GFX_H
#define _GFX_H
#include <stdint.h>

#define swap(a, b) { int16_t t = a; a = b; b = t; }

void gfx_drawPixel(int x, int y, uint16_t color);
void gfx_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		  uint16_t color);
void gfx_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void gfx_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void gfx_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void gfx_fillScreen(uint16_t color);

void gfx_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
			  uint8_t cornername, uint16_t color);
void gfx_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void gfx_init(void (*draw)(int, int, uint16_t), int, int);

void gfx_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
			  uint8_t cornername, int16_t delta, uint16_t color);
void gfx_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, uint16_t color);
void gfx_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      int16_t x2, int16_t y2, uint16_t color);
void gfx_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
		       int16_t radius, uint16_t color);
void gfx_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
		       int16_t radius, uint16_t color);
void gfx_drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap,
		    int16_t w, int16_t h, uint16_t color);
void gfx_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
		  uint16_t bg, uint8_t size);
void gfx_setCursor(int16_t x, int16_t y);
void gfx_setTextColor(uint16_t c, uint16_t bg);
void gfx_setTextSize(uint8_t s);
void gfx_setTextWrap(uint8_t w);
void gfx_setRotation(uint8_t r);
void gfx_puts(char *);
void gfx_write(uint8_t);

uint16_t gfx_height(void);
uint16_t gfx_width(void);

uint8_t gfx_getRotation(void);

#define GFX_WIDTH   320
#define GFX_HEIGHT  240

struct gfx_state {
	int16_t _width, _height, cursor_x, cursor_y;
	uint16_t textcolor, textbgcolor;
	uint8_t textsize, rotation;
	uint8_t wrap;
	void (*drawpixel)(int, int, uint16_t);
};

extern struct gfx_state __gfx_state;

#define GFX_COLOR_WHITE          0xFFFF
#define GFX_COLOR_BLACK          0x0000
#define GFX_COLOR_GREY           0xF7DE
#define GFX_COLOR_BLUE           0x001F
#define GFX_COLOR_BLUE2          0x051F
#define GFX_COLOR_RED            0xF800
#define GFX_COLOR_MAGENTA        0xF81F
#define GFX_COLOR_GREEN          0x07E0
#define GFX_COLOR_CYAN           0x7FFF
#define GFX_COLOR_YELLOW         0xFFE0

#endif /* _ADAFRUIT_GFX_H */
