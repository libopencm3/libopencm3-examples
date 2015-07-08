/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Daniel Serpell <daniel.serpell@gmail.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static void clock_setup(void)
{
	/* Enable high-speed clock at 120MHz */
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);

	/* Enable GPIOG clock for LED & USARTs. */
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART1. */
	rcc_periph_clock_enable(RCC_USART1);
}

static void usart_setup(void)
{
	/* Setup USART1 parameters. */
	usart_set_baudrate(USART1, 38400);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO13 on GPIO port G for LED. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	/* Setup GPIO pins for USART1 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);

	/* Setup USART1 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9);
}

/* Maximum number of iterations for the escape-time calculation */
#define max_iter 32
/* This array converts the iteration count to a character representation. */
static char color[max_iter+1] = " .:++xxXXX%%%%%%################";

/* Main mandelbrot calculation */
static int iterate(float px, float py)
{
	int it = 0;
	float x = 0, y = 0;
	while (it < max_iter) {
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

static void mandel(float c_x, float c_y, float scale)
{
	int x, y;
	for (x = -60; x < 60; x++) {
		for (y = -50; y < 50; y++) {
			int i = iterate(c_x + x*scale, c_y + y*scale);
			usart_send_blocking(USART1, color[i]);
		}
		usart_send_blocking(USART1, '\r');
		usart_send_blocking(USART1, '\n');
	}
}

int main(void)
{
	float scale = 0.25f, center_x = -0.5f, center_y = 0.0f;

	clock_setup();
	gpio_setup();
	usart_setup();

	while (1) {
		/* Blink the LED (PG13) on the board with each fractal drawn. */
		gpio_toggle(GPIOG, GPIO13);		/* LED on/off */
		mandel(center_x, center_y, scale);	/* draw mandelbrot */

		/* Change scale and center */
		center_x += 0.175f * scale;
		center_y += 0.522f * scale;
		scale	*= 0.875f;

		usart_send_blocking(USART1, '\r');
		usart_send_blocking(USART1, '\n');
	}

	return 0;
}
