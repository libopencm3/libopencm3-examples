/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Daniel Serpell <daniel.serpell@gmail.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <stdio.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/systick.h>

/* provided by sdram.c and lcd.c */
extern void sdram_init(void);
extern void lcd_init(void);
extern void lcd_draw_pixel(int, int, uint16_t);
extern void lcd_show_frame(void);

/* utility functions */
void uart_putc(char c);
int _write (int fd, char *ptr, int len);
void sys_tick_handler(void);
void msleep(uint32_t);
uint32_t mtime(void);

void mandel(float, float, float);

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO13 on GPIO port G for LED. */
	rcc_periph_clock_enable(RCC_GPIOG);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	/* Setup GPIO A pins for the USART1 function */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_enable(USART1);
}

/* Maximum number of iterations for the escape-time calculation */
#define maxIter 32
uint16_t lcd_colors[] = {
	0x0,
	0x1f00,
	0x00f8,
	0xe007,
	0xff07,
	0x1ff8,
	0xe0ff,
	0xffff,
	0xc339,
	0x1f00 >> 1,
	0x00f8 >> 1,
	0xe007 >> 1,
	0xff07 >> 1,
	0x1ff8 >> 1,
	0xe0ff >> 1,
	0xffff >> 1,
	0xc339 >> 1,
	0x1f00 << 1,
	0x00f8 << 1,
	0x6007 << 1,
	0x6f07 << 1,
	0x1ff8 << 1,
	0x60ff << 1,
	0x6fff << 1,
	0x4339 << 1,
	0x1f00 ^ 0x6ac9,
	0x00f8 ^ 0x6ac9,
	0xe007 ^ 0x6ac9,
	0xff07 ^ 0x6ac9,
	0x1ff8 ^ 0x6ac9,
	0xe0ff ^ 0x6ac9,
	0xffff ^ 0x6ac9,
	0xc339 ^ 0x6ac9,
	0,
	0,
	0,
	0,
	0
};


static int iterate(float, float);
/* Main mandelbrot calculation */
static int iterate(float px, float py)
{
	int it = 0;
	float x = 0, y = 0;
	while (it < maxIter) {
		float nx = x*x;
		float ny = y*y;
		if ((nx + ny) > 4) {
			return it;
		}
		/* Zn+1 = Zn^2 + P */
		y = 2*x*y + py;
		x = nx - ny + px;
		it++;
	}
	return 0;
}

void mandel(float cX, float cY, float scale)
{
	int x, y;
	int change = 0;
	for (x = -120; x < 120; x++) {
		for (y = -160; y < 160; y++) {
			int i = iterate(cX + x*scale, cY + y*scale);
			if (i >= maxIter) {
				i = maxIter;
			} else {
				change++;
			}
			lcd_draw_pixel(x+120, y+160, lcd_colors[i]);
		}
	}
}

int main(void)
{
	int gen = 0;
	float scale = 0.25f, centerX = -0.5f, centerY = 0.0f;


	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);

	/* set up the SysTick function (1mS interrupts) */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	STK_CVR = 0;
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();

	/* USART and GPIO setup */
	gpio_setup();
	/* Enable the SDRAM attached to the board */
	sdram_init();
	/* Enable the LCD attached to the board */
	lcd_init();

	printf("System initialized.\n");

	while (1) {
		/* Blink the LED (PG13) on the board with each fractal drawn. */
		gpio_toggle(GPIOG, GPIO13);		/* LED on/off */
		mandel(centerX, centerY, scale);	/* draw mandelbrot */
		lcd_show_frame();			/* show it */
		/* Change scale and center */
		centerX += 0.175f * scale;
		centerY += 0.522f * scale;
		scale	*= 0.875f;
		gen++;
		if (gen > 99) {
			scale = 0.25f;
			centerX = -0.5f;
			centerY = 0.0f;
			gen = 0;
		}
		// printf("Generation: %d\n", generation);
		// printf("Cx, Cy = %9.2f, %9.2f, scale = %9.2f\n", centerX, centerY, scale);
	}

	return 0;
}

/* simple millisecond counter */
static volatile uint32_t system_millis;
static volatile uint32_t delay_timer;


/*
 * Simple systick handler
 *
 * Increments a 32 bit value once per millesecond
 * which rolls over every 49 days.
 */
void
sys_tick_handler(void) {
	system_millis++;
	if (delay_timer > 0) {
		delay_timer--;
	}
}

/*
 * Simple spin loop waiting for time to pass
 * 
 * A couple of things to note:
 * First,  you can't just compare to 
 * system_millis because doing so will mean 
 * you delay forever if you happen to hit a 
 * time where it is rolling over.
 * Second, accuracy is "at best" 1mS as you
 * may call this "just before" the systick hits
 * with a value of '1' and it would return 
 * nearly immediately. So if you need really
 * precise delays, use one of the timers.
 */
void
msleep(uint32_t delay) {
	delay_timer = delay;
	while (delay_timer) ;
}

uint32_t
mtime(void) {
	return system_millis;
}

/*
 * uart_putc
 *
 * This pushes a character into the transmit buffer for
 * the channel and turns on TX interrupts (which will fire
 * because initially the register will be empty.) If the
 * ISR sends out the last character it turns off the transmit
 * interrupt flag, so that it won't keep firing on an empty
 * transmit buffer.
 */
void
uart_putc(char c) {

	while ((USART_SR(USART1) & USART_SR_TXE) == 0) ;
	USART_DR(USART1) = c;
}

/* 
 * Called by libc stdio functions
 */
int 
_write (int fd, char *ptr, int len) {
	int i = 0;

	/* 
	 * Write "len" of char from "ptr" to file id "fd"
	 * Return number of char written.
	 */
	if (fd > 2) {
		return -1;  // STDOUT, STDIN, STDERR
	}
	while (*ptr && (i < len)) {
		uart_putc(*ptr);
		if (*ptr == '\n') {
			uart_putc('\r');
		}
		i++;
		ptr++;
	}
  return i;
}

