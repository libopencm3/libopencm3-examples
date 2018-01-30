/**************************************************************************
 * Program STM32F429 Discovery using gcc and libopencm3
 * ili9341b.c  Graphics library for LCD ILI9341 using SPI interface
 * (c) Marcos Augusto Stemmer
 * Link:
 * https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf
 **********************************************************************/

#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "sdram.h"	/* Use sdram setup (modue sdram.c) */
#include "ili9341.h"	/* Names of LCD ili9341 registers */

/* Use SPI5 to comunicate with LCD */
#define LCD_SPI SPI5
#define SPI5_LCD_CS_0	(GPIOC_BSRR=(1<<18))
#define SPI5_LCD_CS_1	(GPIOC_BSRR=(1<<2))
#define LCD_DATA_CMD_0	(GPIOD_BSRR=(1<<29))
#define LCD_DATA_CMD_1	(GPIOD_BSRR=(1<<13))

/* Local Function prototypes */
void spi5_lcd_init(void);
void ili_cmd(int cmd);
void ili_data8(int data);
void ili_data16(int data);
void ili_mdata(const uint8_t *pd, int nd);
void ili_9341_init(void);
void plot4points(int cx, int cy, int x, int y);
int novoponto(int x, int y);
int mataponto(int k);
int i2c3_set_reg(uint8_t reg);
int i2c3_read_array(uint8_t reg, uint8_t *data, int nb);

/* Context of text: Cursor location, color and size of text */
struct txtinfo *ptxinfo;
/***************************************************************************
 * Configura as portas e a interface SPI5 usada para comunicar com o display
 * PF7	SPI5_SCK		SPI5 clock
 * PF9	SPI5_MOSI		SPI5 data output
 * PC1	SPI5_MEMS_L3GD20	Chip Select of Acelerometer MEMS_L3GD20
 * PC2	SPI5_LCD_CS		Chip Select of display LCD-TFT
 * PD13	LCD_DATA_CMD		Data/Command of LCD-TFT
 * *************************************************************************/
void spi5_lcd_init(void)
{
/* Seup ports GPIOC, GPIOD e GPIOF used to send data to display */
	rcc_periph_clock_enable(RCC_GPIOC | RCC_GPIOD | RCC_GPIOF);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
/* GPIOF poins 9=LCD_MOSI; 7=LCD_SCK as AF5: SPI interface of LCD */
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO9);
/* Setup SPI5 */
	rcc_periph_clock_enable(RCC_SPI5);
	spi_init_master(LCD_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_4,
				SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
				SPI_CR1_CPHA_CLK_TRANSITION_1,
				SPI_CR1_DFF_8BIT,
				SPI_CR1_MSBFIRST);
	spi_enable_ss_output(LCD_SPI);
	spi_enable(LCD_SPI);
}

/* Send a command to ili9341 */
void ili_cmd(int cmd)
{
	SPI5_LCD_CS_0;
	(void) spi_xfer(LCD_SPI, cmd);
	SPI5_LCD_CS_1;
}

/* 1 data byte */
void ili_data8(int data)
{
	LCD_DATA_CMD_1;
	SPI5_LCD_CS_0;
	(void) spi_xfer(LCD_SPI, data);
	SPI5_LCD_CS_1;
	LCD_DATA_CMD_0;
}

/* 16 bits (2 bytes) */
void ili_data16(int data)
{
	LCD_DATA_CMD_1;
	SPI5_LCD_CS_0;
	(void) spi_xfer(LCD_SPI,(data >> 8) & 0xff);
	(void) spi_xfer(LCD_SPI,(data & 0xff));
	SPI5_LCD_CS_1;
	LCD_DATA_CMD_0;
}

/* Send many bytes
 * pd = pointer to data
 * nd = number of bytes */
void ili_mdata(const uint8_t *pd, int nd)
{
	LCD_DATA_CMD_1;
	SPI5_LCD_CS_0;
	while(nd--) { (void) spi_xfer(LCD_SPI, *pd++); }
	SPI5_LCD_CS_1;
	LCD_DATA_CMD_0;
}

static const uint8_t pos_gamma_args[] = { 
	0x0F, 0x29, 0x24, 0x0C, 0x0E, 
	0x09, 0x4E, 0x78, 0x3C, 0x09, 
	0x13, 0x05, 0x17, 0x11, 0x00} ;
	
static const uint8_t neg_gamma_args[] = { 
	0x00, 0x16, 0x1B, 0x04, 0x11, 
	0x07, 0x31, 0x33, 0x42, 0x05, 
	0x0C, 0x0A, 0x28, 0x2F, 0x0F };

/* Initial setup of LCD_TFT display */
void ili_9341_init(void)
{
	LCD_DATA_CMD_0;
	ili_cmd(LCD_FRMCTR1); ili_data16(0x001b);/*0xB1 Frame Rate Control (In Normal Mode) */
	ili_cmd(LCD_POWER1); ili_data8(0x10);	/*0xC0 Power Control 1 */
	ili_cmd(LCD_POWER2); ili_data8(0x10);	/*0xC1 Power Control 2 */
	ili_cmd(LCD_VCOM1); ili_data16(0x4515);	/*0xC5 VCOM Control 1 */
	ili_cmd(LCD_VCOM2); ili_data8(0x90);	/*0xC7 VCOM Control 7 */
	ili_cmd(LCD_MAC); 	/*0x36 Memory Access Control register*/
	ili_data8(0xc8);	/*com 0x08 fica de cabeca para baixo */
	ili_cmd(LCD_RGB_INTERFACE); ili_data8(0xc2);	/*0xB0 RGB Interface Signal	*/
	ili_cmd(LCD_PIXEL_FORMAT); ili_data8(0x55);	/*0x3A Pixel Format: 16 bit/pix */
	ili_cmd(LCD_DFC);		/*0xB6 Display Function */
	ili_data16(0x0aa7);ili_data16(0x2704);
	msleep(50);
	ili_cmd(LCD_GAMMA); ili_data8(1);	/*0x26 Gamma register */
	ili_cmd(LCD_PGAMMA);	/*0xE0 Positive Gamma Correction */
	ili_mdata(pos_gamma_args, 15);
	ili_cmd(LCD_NGAMMA); 	/*0xE1 Negative Gamma Correction */
	ili_mdata(neg_gamma_args, 15);
	ili_cmd(LCD_SLEEP_OUT);		/*0x11*/
	msleep(200);
	ili_cmd(LCD_DISPLAY_ON);	/*0x29*/
	msleep(20);
}

/* Transfer memory FRAME_BUFFER to display */
void lcd_show_frame(void)
{
	ili_cmd(LCD_COLUMN_ADDR);	/* 0x2a Column Address*/
	ili_data16(0);			/* x1 */
	ili_data16(LCD_WIDTH-1);	/* x2 */
	ili_cmd(LCD_PAGE_ADDR);		/* 0x2b Row Address */
	ili_data16(0);			/* y1 */
	ili_data16(LCD_HIGHT-1);	/* y2 */
	ili_cmd(LCD_GRAM);		/* 0x2C Write RAM */
	ili_mdata((const uint8_t *)FRAME_BUFFER, 2*LCD_WIDTH*LCD_HIGHT);
}

/******************************************************************
 * Copy a rectangular area of FRAME_BUFFER to LCD
 * ****************************************************************/
void lcd_show_rect(int x1, int y1, int x2, int y2)
{
	int nw, y;
	uint8_t *pm;
	if(x1 > x2) { y = x1; x1 = x2; x2 = y; }
	if(y1 > y2) { y = y1; y1 = y2; y2 = y; }
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
	if(y2 >= LCD_HIGHT) y2 = LCD_HIGHT - 1;
	ili_cmd(LCD_COLUMN_ADDR);	/* 0x2a Column Address*/
	ili_data16(x1);
	ili_data16(x2);
	ili_cmd(LCD_PAGE_ADDR);		/* 0x2b Row Address */
	ili_data16(y1);
	ili_data16(y2);
	ili_cmd(LCD_GRAM);		/* 0x2C Write RAM */
	pm = (uint8_t *)FRAME_BUFFER + 2*y1 * LCD_WIDTH + 2*x1;
	nw = 2*(x2 - x1 + 1);
	for(y = y1; y <= y2; y++) {
		ili_mdata(pm, nw);
		pm += 2*LCD_WIDTH;
	}
}

/* initialize display */
void lcd_init(struct txtinfo *txi)
{
	ptxinfo = txi;
	spi5_lcd_init();
	ili_9341_init();
	lcd_rectangle(0,0,LCD_WIDTH-1, LCD_HIGHT-1, txi->f_bgcolor);
	lcd_show_frame();
}

/* Convert RGB color components to 565 16 bit integer */
/* Since interface is big endian, the bytes are swapped */
/* G2G1G0B4B3B2B1B0 R4R3R2R1R0G5G4G3	*/
int lcd_cor565(int r, int g, int b)
{
return (((g & 0x1c) << 11) | ((b & 0xF8) << 5) |
	(r & 0xf8) | ((g & 0xe0) >> 5));
}

/* Use Memory Access Control Reg. to turn image upside down */
void lcd_upsdown(void)
{
	static int pos = 0x08;
	ili_cmd(LCD_MAC); 	/*0x36 Memory Access Control register*/
	ili_data8(pos);	/* pos=8: Upside down. */
	pos ^= 0xc0;
}	
	
/* Draw a rectangle on FRAME_BUFFER */
void lcd_rectangle(int x1, int y1, int x2, int y2, int cor)
{
	uint16_t *p;
	int x, y;
	if(x2 < x1) { x = x1; x1 = x2; x2 = x; }
	if(y2 < y1) { y = y1; y1 = y2; y2 = y; }
	if(x1 >= LCD_WIDTH || y1 >= LCD_HIGHT) return;
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= LCD_WIDTH) x2 = LCD_WIDTH - 1;
	if(y2 >= LCD_HIGHT) y2 = LCD_HIGHT - 1;
	p = FRAME_BUFFER + LCD_WIDTH*y1;
	for(y = y1; y <= y2; y++) {
		for(x = x1; x <= x2; x++){
			p[x] = cor;
			}
		p += LCD_WIDTH;
	}
}

/* ************************************
 * Write a dot on FRAME BUFFER
 * Color of dot:	ptxinfo->f_fgcolor
 * Size of dot:		ptxinfo->f_size 
 **************************************/
void lcd_dot(int x, int y)
{
	uint16_t *p;
	int color;
	if(x > LCD_WIDTH-2 || y > LCD_HIGHT-2 || x < 0 || y < 0) return;
	p = FRAME_BUFFER + x + y * LCD_WIDTH;
	color = ptxinfo->f_fgcolor;
	switch(ptxinfo->f_size) {
		case 1: *p = ptxinfo->f_fgcolor;
			break;
		case 2: *p = p[1]= p[LCD_WIDTH]=p[LCD_WIDTH+1]= color;
			break;
		default:
		case 3: /*p[-LCD_WIDTH]=p[-1]=p[0]=p[1]=p[LCD_WIDTH]=color;
			break;*/
		case 4: 
			*p++ = color; *p++ = color;*p++ = color;*p++ = color;
			if(y)	{
				p -= (LCD_WIDTH+3);
				*p++ = color;*p++ = color;
				p += (2*LCD_WIDTH - 3);
				}
			*p++ = color; *p++ = color;*p++ = color;*p++ = color;
			p += (LCD_WIDTH - 3);
			*p++ = color; *p++ = color;
			break;
		}
}

/* ************************************************
 * Write a character
 * Use struct txinfo 
 **************************************************/
void lcd_putchar(int c)
{
	int x, y, k, lc, m;
	y = ptxinfo->f_line;
	switch(c) {
		case '\r': ptxinfo->f_column = 4; break;
		case '\n': ptxinfo->f_line += (ptxinfo->f_hight + 4)*ptxinfo->f_size;
			break;
	}
	if(c < 0x20) return;
	for(k = ptxinfo->f_hight; k--; y += ptxinfo->f_size) {
		x = ptxinfo->f_column;
		lc = ptxinfo->font[ptxinfo->f_hight * (c - 0x1f) - k];
		for(m = 0x80; m; m >>= 1) {
			if(m & lc) lcd_dot(x, y);
			x += ptxinfo->f_size;
			}
		}
	ptxinfo->f_column += 9*ptxinfo->f_size;
}

/*********************************************
Bresenham algorithm to draw a circle
[http://en.wikipedia.org/wiki/Midpoint_circle_algorithm]
*************************************************/
void plot4points(int cx, int cy, int x, int y)
{
	lcd_dot(cx + x, cy + y);
	if (x) lcd_dot(cx - x, cy + y);
	if (y) lcd_dot(cx + x, cy - y);
	lcd_dot(cx - x, cy - y);
}

/* Circle (cx,cy)  */
void lcd_circle(int cx, int cy, int raio)
{
int error = -raio;
int x = raio;
int y = 0;
while (x >= y) {
	plot4points(cx, cy, x, y);
	plot4points(cx, cy, y, x);
	error += y++;
	error += y;
	if (error >= 0) {
		error -= (--x);
		error -= x;
		}
	}
}

/* Draw a straight line */
void lcd_line(int x1, int y1, int x2, int y2)
{
int dx, dy, sx, sy;
int error;
sx = sy =1;
dx = x2-x1;
if(dx < 0) { dx = -dx; sx=-1; }
dy = y2-y1;
if(dy < 0) { dy = -dy; sy=-1; }
if(dx > dy) {
	error = dx/2;
	do	{
		lcd_dot(x1,y1);
		if(x1 == x2) break;
		x1 += sx;
		error -= dy;
		if(error < 0) { y1 += sy; error += dx; }
		} while(1);
	}
else	{
	error = dy/2;
	do	{
		lcd_dot(x1,y1);
		if(y1 == y2) break;
		y1 += sy;
		error -= dx;
		if(error < 0) { x1 += sx; error += dy; }
		} while(1);
	}
}

/*********************************
 * lcd_flood
 * Fill area replacing pixels of initial color with new color
 * ******************************/
#define NPONTOS 20000

/*Gloval variables for lcd_flood */
struct sponto {
	uint16_t x, y;
} *pontos;

int npontos, cori, corf;
uint16_t *ppix;

/* used by lcd_flood */
int novoponto(int x, int y)
{
	int ui;
	if(x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HIGHT) return -1;
	ui = x+y*LCD_WIDTH;
	if(ppix[ui] != cori || npontos > NPONTOS-2) return -1;
	ppix[ui]=corf;
	pontos[npontos].x = x;
	pontos[npontos].y = y;
	npontos++;
	return npontos;
}

/* used by lcd_flood */
int mataponto(int k)
{
	if(k < npontos) {
		pontos[k].x = pontos[npontos].x;
		pontos[k].y = pontos[npontos].y;
	}
	return --npontos;
}

/****************************************************************
 * Flood Fill area
 * *************************************************************/
void lcd_flood(int x, int y, int cor)
{
	int k;
	int ix, iy;
	int ui;
	pontos = (struct sponto *)(FRAME_BUFFER + LCD_WIDTH*LCD_HIGHT);
	corf=cor;
	ppix = (uint16_t *)FRAME_BUFFER;
	npontos = 1;
	pontos[0].x = x;
	pontos[0].y = y;
	ui = x+y*LCD_WIDTH;
	cori = ppix[ui];
	ppix[ui] = cor;
	do	{
		for(k = 0; k < npontos; k++) {
			ix = pontos[k].x; iy = pontos[k].y;
			novoponto(ix-1, iy); novoponto(ix+1, iy);
			novoponto(ix-1, iy-1); novoponto(ix+1, iy+1);
			novoponto(ix+1, iy-1); novoponto(ix-1, iy+1);
			novoponto(ix, iy-1); novoponto(ix, iy+1);
			mataponto(k);
		}
	}while(npontos);
}
