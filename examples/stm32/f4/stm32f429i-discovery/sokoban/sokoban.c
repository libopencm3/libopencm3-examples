/**************************************************************************
 * Game of Sokoban for kit STM32F429 Discovery using gcc and libopencm3
 * Main module
 * 
 * Copyright (C) 2018 Marcos Augusto Stemmer <mastemmer@gmail.com>
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
****************************************************************************/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <stdio.h>
#include "charset.c"
#include "bmp.c"

#define CPUCLK 168000000

#include "uart.h"
#include "sdram.h"
#include "ili9341.h"
#include "i2c3_touch.h"

//#define DEBUG

struct txtinfo stxt;
extern uint16_t *pbigguy_img;

extern const char *levels[];
	
char walls[12][10];
char back_up[8][192];
int ini_v, fim_v;
int ih, jh;
/******* Global variables used in Sys Tick **********/
volatile uint32_t milisec;
volatile uint32_t xdebounce;

/* Local Function prototypes */
void clock_setup(void);
void lcd_puts(char *txt);
void draw_bmp(int x0, int y0, uint16_t *pbits);
void draw_X(int x, int y);
int touch_irq(void);
void cell(int i, int j);
void sback_up(void);
void draw_level(char *pn);
int newloc(int i, int j);
int find_path(int icam, int jcam);
int move(int di, int dj);
void waitfordebounce(int ndb);
int select_level(void);

/* System ticks once in a milisecond */
void sys_tick_handler(void)
{
	milisec++;
	if(xdebounce) xdebounce--;
}

/* msleep wait in miliseconds */
void msleep(uint32_t t)
{
	t += milisec;
	while(t > milisec);
}

/* Set up clock and sys_tick */
void clock_setup(void)
{
	/* Set CPU clock to 168MHz */
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
	/* Enable clock in GPIOG ande GPIOA */
	RCC_AHB1ENR |= RCC_AHB1ENR_IOPGEN | RCC_AHB1ENR_IOPAEN;
	/* Enable ADC1 */
	rcc_periph_clock_enable(RCC_ADC1);
	GPIOG_MODER |= (1 << 26);	/* GPIOG_13 as output */
	/* Set sys_tick to 1000 interrupts per second */
	systick_set_reload(CPUCLK/1000-1);
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	systick_counter_enable();
	systick_interrupt_enable();
}

void lcd_puts(char *txt)
{
	while(*txt) lcd_putchar(*txt++);
}

/* Draw a bitmp. Left top corner in x0, y0 */
void draw_bmp(int x0, int y0, uint16_t *pbits)
{
	int bmp_hight, bmp_width;
	uint16_t *pb, *pfb;
	int x, y;
	bmp_width = pbits[0];
	bmp_hight = pbits[1];
	pb = (uint16_t *)pbits + 2;
	pfb = (uint16_t *)0xd0000000 + x0 + y0*LCD_WIDTH;
	for(y=0; y < bmp_hight && y < LCD_HIGHT - y0; y++) {
		for(x = 0; x < bmp_width && x < LCD_WIDTH; x++) {
			if(*pb != 0xffff) pfb[x] = *pb;
			pb++;
		}
		pfb += LCD_WIDTH;
	}
	lcd_show_rect(x0,y0,x0+bmp_width-1, y0+bmp_hight-1);
}

void draw_X(int x, int y)
{
	stxt.f_size = 2;
	lcd_rectangle(24*x, 24*y,24*x+24, 24*y+24, 0xffff);
	stxt.f_fgcolor = lcd_cor565(0xfe, 0x6f, 0);
	lcd_line(24*x+4, 24*y+4,24*x+20, 24*y+20);
	lcd_line(24*x+20, 24*y+4,24*x+4, 24*y+20);
	stxt.f_size = 1;
	lcd_show_rect(24*x, 24*y,24*x+24, 24*y+24);
}

int touch_irq(void)
{
	int x, y, z;
	if(touch_get_size() < 0 ) return 0;
	touch_read_raw_xy( &x, &y, &z);
	x++; y++; z++;
	return 1;
}

void cell(int i, int j)
{
	switch(walls[i][j]) {
		case 'W':
			draw_bmp(24*j, 24*i, (uint16_t *)wall_img);
			break;
		case 'B':
			draw_bmp(24*j, 24*i, (uint16_t *)box_img);
			break;
		case 'b':
			draw_bmp(24*j, 24*i, (uint16_t *)box2_img);
			break;
		case 'm':
			draw_X(j, i);
		case 'M':
			draw_bmp(24*j, 24*i, (uint16_t *)guy_img);
			jh = j; ih = i;
			break;
		case 'X':
			draw_X(j, i);
			break;
		case ' ':
			lcd_rectangle(24*j, 24*i,24*j+24, 24*i+24, 0xffff);
			lcd_show_rect(24*j, 24*i,24*j+24, 24*i+24);
			break;
		}
}

void sback_up(void)
{
	int i, j;
	char *pv;
	pv = back_up[fim_v];
	for(i=0; i<12; i++) {
		for(j=0; j<10; j++) {
			*pv++ = walls[i][j];
		}
		*pv++ = '\n';
	}
	*pv = '\0';
	fim_v = (fim_v + 1) & 7;
	if(fim_v == ini_v) ini_v = (ini_v + 1) & 7;
}

void draw_level(char *pn)
{
	int i, j, c;
	for(i = 0; i < 12; i++) {
		for(j = 0; j < 10; j++) walls[i][j]=' ';
	}
	for(i = j = 0; *pn; ) {
		c = *pn++;
		if(c == '\n') { j = 0; i++; }
		else walls[i][j++] = c;
	}
	for(i = 0; i < 12; i++) {
		for(j = 0; j < 10; j++) {
			cell(i, j);
		}
	}
}

int imh, jmh;

#define PTMAX 80
char path[80];

char mcam[12][10];

int lpi[PTMAX];
int lpj[PTMAX];

int ncam;
int rank;

/* Check cell [i,j] solving the maze */
int newloc(int i, int j)
{
	if(i < 0 || j < 0 || i > 11 || j > 9) return -1;
	if(mcam[i][j] != 'x') return -1;
	if(ncam > PTMAX-2) return -2;
	mcam[i][j]=rank+'0';
	lpi[ncam]=i;
	lpj[ncam]=j;
	return ++ncam;
}

/* Maze solution. Move man to any cell */
int find_path(int icam, int jcam)
{
	int ix, jx, kx, pkx=0;
	int iu, ju;
	int nc, n=0;
	for(ix=0; ix < 12; ix++) {
		for(jx = 0; jx < 10; jx++) {
			mcam[ix][jx]='x';
			if(walls[ix][jx]=='W' || walls[ix][jx]=='B' || walls[ix][jx]=='b') 
				mcam[ix][jx] = 'z';
		}
	}
	ncam = 1;
	lpi[0]=ih;
	lpj[0]=jh;
	mcam[ih][jh]='0';
	rank = 0;
	do	{
		nc = ncam;
		rank++;
		for(kx=0; kx < nc; kx++) {
			ix = lpi[kx];
			jx = lpj[kx];
			newloc(ix-1,jx);
			newloc(ix+1,jx);
			newloc(ix,jx-1);
			newloc(ix,jx+1);
		}
		for(kx=0,ix=nc; ix < ncam; kx++, ix++) {
			lpi[kx] = lpi[ix];
			lpj[kx] = lpj[ix];
			if(lpi[kx] == icam && lpj[kx] == jcam) { pkx = -93; break; }
		}
		ncam -= nc;
		if(ncam <= 0) { path[0]= '\0'; return -1; }
	} while(pkx != -93);
	path[rank]='\0';
	ix = icam; jx = jcam;
	for(kx = rank; kx--; ) {
		iu = ix-1;
		ju = jx;
		pkx = mcam[iu][ju];
		path[kx]='m';
		if(mcam[ix+1][jx] < pkx) {
			iu = ix+1;
			pkx = mcam[iu][ju];
			path[kx] = 'i';
		}
		if(mcam[ix][jx-1] < pkx) {
			iu = ix;
			ju = jx-1;
			pkx = mcam[iu][ju];
			path[kx] = 'k';
		}
		if(mcam[ix][jx+1] < pkx) {
			iu = ix;
			ju = jx+1;
			pkx = mcam[iu][ju];
			path[kx] = 'j';
		}
		ix = iu; jx = ju;
	}
	ncam = n;
	return n;
}

int move(int di, int dj)
{
	int c, d;
	c = walls[ih+di][jh+dj];
	if(c == 'W') {imh = ih; jmh = jh; return -1; }
	if(c == 'B' || c == 'b') {
		d = walls[ih + 2*di][jh + 2*dj];
		if(d != ' ' && d != 'X') return -1;
		sback_up();
		walls[ih + 2*di][jh + 2*dj] = d == 'X'? 'b' : 'B';
		walls[ih+di][jh+dj]= c == 'b'? 'X': ' ';
		cell(ih + di, jh + dj);
		cell(ih + 2*di, jh + 2*dj);
	} else sback_up();
	walls[ih][jh] = (walls[ih][jh]=='m'? 'X': ' ');
	cell(ih, jh);
	ih += di; jh += dj;
	walls[ih][jh] = (walls[ih][jh]=='X'? 'm': 'M');
	cell(ih, jh);
	msleep(50);
	return 0;
}

void waitfordebounce(int ndb)
{
	xdebounce=ndb;
	do	{
		if(touch_irq()) xdebounce = ndb;
	} while(xdebounce);
}

int select_level(void)
{
	int i, j;
	int lcd_x, lcd_y, lcd_z;
	int niv, niva;
	int maxlevels;
	for(maxlevels = 0; maxlevels < 25 && levels[maxlevels]!=NULL; maxlevels++);
	for(i=0; i<11; i++) for(j=0; j<10; j++)
		draw_bmp(24*j, 24*i, (uint16_t *)wall_img);
	draw_bmp(8,8,pbigguy_img);
	stxt.f_size = 1;
	stxt.f_line = 20;
	stxt.f_column = 50;
	stxt.f_fgcolor = 0xffff;
	mprintf(lcd_putchar,"Select level");
	stxt.f_fgcolor = 0;
	for(i = 0; i < 5; i++) {
		for(j = 0; j < 5; j++) {
			lcd_rectangle(42*j+16, 40*i+54, 42*j+54,40*i+90,0xffff);
			stxt.f_size = 2;
			stxt.f_line = 40*i+62;
			stxt.f_column = 42*j+20;
			mprintf(lcd_putchar, "%2d", 5*i+j+1);
		}
	}
	lcd_show_frame();
	waitfordebounce(200);
	niv = 20000;
	lcd_x = lcd_y = lcd_z = 0;
	niv = 987; niva = 0;
	do	{
		if(niv != 987) niva = niv;
		niv = 987;
		if(touch_get_size() > 0) {
			(void)touch_read_xy( &lcd_x, &lcd_y, &lcd_z);
			if(lcd_x > 14 && lcd_x < 220 && lcd_y > 52 && lcd_y < 252) {
				j = (lcd_x - 14)/42;
				i = (lcd_y - 52)/40;
				niv = 5*i + j;
			}
		}
	} while((niva != niv) || (niv > maxlevels));
	waitfordebounce(200);
	mprintf(U1putchar,"Level = %d\r\n", niv +1);
	return niv;
}

int main(void)
{
	int c;
	int ccx;
	char *pcam;
	int knt, finished;
	int xlevel;
	int lcd_x, lcd_y, lcd_z;
	clock_setup();	/* Setup clock and Sys Tick */
	usart_setup();	/* Setup USART1 */
	sdram_init();	/* Setup 8MB SDRAM at 0xd0000000 */
	stxt.font = dos8x16;
	stxt.f_hight = 16;
	stxt.f_size = 2;
	stxt.f_bgcolor = 0xffff;
	stxt.f_fgcolor = 0;
	stxt.f_line = 4;
	stxt.f_column = 20;
	pbigguy_img = (uint16_t *)bigguy_img;
	U1puts("\r\nSokoban!\r\nEnter to continue without calibrating touch-screen\r\n");
	if(touch_calibra(&stxt)) {
		U1puts("\r\nLCD Error!\r\n");
		while(1);
	}
	U1puts("\r\nBackspace or blue button undo a move (back_up)\r\n'r' restart level\r\n"
		"'+' Increment level\r\n");
	knt = 80;
	xlevel = 0;
	do	{
		lcd_rectangle(0,0,239,319,0xffff);
		draw_level((char *)levels[xlevel]);
		sback_up();
		stxt.f_size = 1;
		stxt.f_line = 298;
		stxt.f_column = 10;
		stxt.f_fgcolor = lcd_cor565(0xff,0,0);
		mprintf(lcd_putchar,"Level %d", xlevel+1);
		lcd_rectangle(123, 293, 233, 313, lcd_cor565(0x60,0x60,0x60));
		lcd_rectangle(120, 290, 230, 310, lcd_cor565(0xB0,0xB0,0xB0));
		stxt.f_fgcolor = 0xffff;
		stxt.f_size = 1;
		stxt.f_line = 294;
		stxt.f_column = 132;
		mprintf(lcd_putchar,"Sel.Level");
		lcd_show_rect(0,289,239,315);
		ini_v = fim_v = 0;
		imh = ih; jmh = jh;
		path[0]='\0';
		finished = 0;
		pcam = path;
		touch_init();
		c = '$';
		do	{
			c = '$';
			if((*pcam=='\0') && (touch_get_size() > 0)) {
				(void)touch_read_xy( &lcd_x, &lcd_y, &lcd_z);
				imh = lcd_y / 24;
				jmh = lcd_x / 24;
				if((imh != ih) && (jmh != jh)) {
					(void)find_path(imh,jmh);
				} else  {
					pcam = path;
					ccx = 0;
					while(imh > ih) { 
						*pcam++ = 'm'; imh--; 
						c = walls[imh][jmh];
						if(c == 'B' || c == 'b') ccx++;
						if(c == 'W') ccx = 8;
					}
					while(imh < ih) { 
						*pcam++ = 'i'; imh++; c = walls[imh][jmh];
						if(c == 'B' || c == 'b') ccx++;
						if(c == 'W') ccx = 8;
					}

					while(jmh < jh) { 
						*pcam++ = 'j'; jmh++; c = walls[imh][jmh];
						if(c == 'B' || c == 'b') ccx++;
						if(c == 'W') ccx = 8;
					}

					while(jmh > jh) { 
						*pcam++ = 'k'; jmh--; c = walls[imh][jmh];
						if(c == 'B' || c == 'b') ccx++;
						if(c == 'W') ccx = 8;
					}
					c = '$';
					if(ccx > 1) {
						find_path(lcd_y / 24, lcd_x / 24);
					} else	*pcam = '\0';
				}
				pcam = path;
				if(!finished)
				if((lcd_x > 160)  && (lcd_y > 288)) {
					xlevel = select_level();
					lcd_show_frame();
					*pcam = '\0';
					c = 'r';
					break;
				}
			} else {
				imh = ih; jmh = jh;
				if(finished) finished--;
			}
			
			
			if(U1available()) {
				c = U1getchar();
				if(c==0x7f) c=8;
			}

			if(GPIOA_IDR & 1) {	/* Check for blue button */
				if(knt == 0) c = '\b';
				knt = 80;
			}
			else	if(knt) knt--;	/* Decrement blue button debounce counter */

			if(c == '\b') {
				if(ini_v != fim_v) {
					fim_v = (fim_v - 1) & 7;
					draw_level(back_up[fim_v]);
					imh = ih; jmh = jh;
					path[0]='\0';
					pcam = path;
					while(touch_irq());
					c = '$';
				}
				else c = 'r';	/* c='r' restart level */
			}
			
			if(*pcam) c = *pcam++;
			
			
			if(c == '$') {	/* '$'= Nothing received  */
				continue;
			}
			if(c == 0x1b)	{
				c = U1getchar();	/* Arrow keys in Linux */
				if(c == 0x5b) {
					c = U1getchar();
					switch(c) {
						case 0x41: c = 'i'; break;
						case 0x43: c = 'k'; break;
						case 0x44: c = 'j'; break;
						case 0x42: c = 'm'; break;
					}
				}
			}
			if(c == 0xe0) {
				c = U1getchar();	/* Arrow keys in MSDOS/Windows */
				switch(c) {
					case 0x48: c = 'i'; break;
					case 0x4b: c = 'j'; break;
					case 0x4d: c = 'k'; break;
					case 0x50: c = 'm'; break;
				}
			}
			if(c >= '0' && c <= '9') {
				xlevel = c - '0';
				c = 'r';
			}
			switch(c) {
			case 'i': move(-1,0); break;
			case 'j': move(0,-1); break;
			case 'k': move(0, 1); break;
			case 'm': move(1, 0); break;
			}
			lcd_z = 0;	/* Check for job done (no 'B') */
			for(lcd_x=0; lcd_x < 10; lcd_x++) {
				for(lcd_y = 0; lcd_y < 12; lcd_y++) {
					if(walls[lcd_y][lcd_x] == 'B') {
						lcd_x = 10; lcd_z = 1;
						break;
					}
				}
			}
			if(c == '+') lcd_z = 0;
			if(lcd_z == 0) {
				for(lcd_z = 12; lcd_z--; ) {
					for(lcd_x = 0; lcd_x < 10; lcd_x++) {
						for(lcd_y = 0; lcd_y < 12; lcd_y++) {
							c = walls[lcd_y][lcd_x];
							if(c == 'b' || c == 'B') {
								walls[lcd_y][lcd_x] = c ^ ('A' ^ 'a');
								cell(lcd_y, lcd_x);
							}
						}
					}
					msleep(125);
				}
				stxt.f_size = 1;
				stxt.f_line = 294;
				stxt.f_column = 10;
				lcd_rectangle(0, 290, 239, 319, 0xffff);
				mprintf(lcd_putchar,"Level %d Complete!", xlevel+1);
				mprintf(U1putchar,"Level %d Complete! (Enter to continue)\r\n", xlevel+1);
				knt = 80;
				do	{
					lcd_show_rect(0,286,319,319);
					if(touch_irq()) knt = 80;
					else knt--;
				} while(knt > 0);
				c = '$';
				do	{
					if(U1available()) {
						c = U1getchar();
					}
					if(touch_irq()) break;
				} while(c != '+' && c!= ' ' && c!='\r' && c!='\n');
				finished = 40;		
				xlevel++;
				if(levels[xlevel]==NULL) xlevel= 0;
				c = 'r';
			}
		} while(c != 'r');
	} while(1);
	return 0;
}
