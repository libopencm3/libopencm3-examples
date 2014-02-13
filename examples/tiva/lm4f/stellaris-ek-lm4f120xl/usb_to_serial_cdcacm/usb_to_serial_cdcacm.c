/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2011 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2012-2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

/**
 * \addtogroup Examples
 *
 * Flashes the Red, Green and Blue diodes on the board, in order.
 *
 * RED controlled by PF1
 * Green controlled by PF3
 * Blue controlled by PF2
 */

#include "usb_to_serial_cdcacm.h"

#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/uart.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/cm3/scb.h>

#define PLL_DIV_80MHZ		5
/* This is how the RGB LED is connected on the stellaris launchpad */
#define RGB_PORT	GPIOF
enum {
	LED_R	= GPIO1,
	LED_G	= GPIO3,
	LED_B	= GPIO2,
};

/*
 * Clock setup:
 * Take the main crystal oscillator at 16MHz, run it through the PLL, and divide
 * the 400MHz PLL clock to get a system clock of 80MHz.
 */
static void clock_setup(void)
{
	rcc_sysclk_config(OSCSRC_MOSC, XTAL_16M, PLL_DIV_80MHZ);
}

/*
 * GPIO setup:
 * Enable the pins driving the RGB LED as outputs.
 */
static void gpio_setup(void)
{
	/*
	 * Configure GPIOF
	 * This port is used to control the RGB LED
	 */
	periph_clock_enable(RCC_GPIOF);
	const uint32_t opins = (LED_R | LED_G | LED_B);

	gpio_mode_setup(RGB_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, opins);
	gpio_set_output_config(RGB_PORT, GPIO_OTYPE_PP, GPIO_DRIVE_2MA, opins);

}

static void cm4f_enable_fpu(void)
{
	/* Enable FPU */
	SCB_CPACR |= SCB_CPACR_FULL * (SCB_CPACR_CP10 | SCB_CPACR_CP11);
	/* Wait for store to complete */
	__asm__("DSB");
	/* Reset pipeline. Now the FPU is enabled */
	__asm__("ISB");

}

void glue_data_received_cb(uint8_t * buf, uint16_t len)
{
	/* Blue LED indicates data coming in */
	gpio_set(RGB_PORT, LED_B);
	cdcacm_send_data(buf, len);
	gpio_clear(RGB_PORT, LED_B);
}

void glue_set_line_state_cb(uint8_t dtr, uint8_t rts)
{
	/* Green LED indicated one of the control lines are active */
	if (dtr || rts)
		gpio_set(RGB_PORT, LED_G);
	else
		gpio_clear(RGB_PORT, LED_G);

	uart_set_ctl_line_state(dtr, rts);
}

int glue_set_line_coding_cb(uint32_t baud, uint8_t databits,
			    enum usb_cdc_line_coding_bParityType cdc_parity,
			    enum usb_cdc_line_coding_bCharFormat cdc_stopbits)
{
	enum uart_parity parity;
	uint8_t uart_stopbits;

	if (databits < 5 || databits > 8)
		return 0;

	switch (cdc_parity) {
	case USB_CDC_NO_PARITY:
		parity = UART_PARITY_NONE;
		break;
	case USB_CDC_ODD_PARITY:
		parity = UART_PARITY_ODD;
		break;
	case USB_CDC_EVEN_PARITY:
		parity = UART_PARITY_EVEN;
		break;
	default:
		return 0;
	}

	switch (cdc_stopbits) {
	case USB_CDC_1_STOP_BITS:
		uart_stopbits = 1;
		break;
	case USB_CDC_2_STOP_BITS:
		uart_stopbits = 2;
		break;
	default:
		return 0;
	}

	/* Disable the UART while we mess with its settings */
	uart_disable(UART1);
	/* Set communication parameters */
	uart_set_baudrate(UART1, baud);
	uart_set_databits(UART1, databits);
	uart_set_parity(UART1, parity);
	uart_set_stopbits(UART1, uart_stopbits);
	/* Back to work. */
	uart_enable(UART1);

	return 1;
}

void glue_send_data_cb(uint8_t * buf, uint16_t len)
{
	int i;

	/* Red LED indicates data going out */
	gpio_set(RGB_PORT, LED_R);

	for (i = 0; i < len; i++) {
		uart_send_blocking(UART1, buf[i]);
	}

	gpio_clear(RGB_PORT, LED_R);
}

static void mainloop(void)
{
	uint8_t linestate, cdcacmstate;
	static uint8_t oldlinestate = 0;

	/* See if the state of control lines has changed */
	linestate = uart_get_ctl_line_state();
	if (oldlinestate != linestate) {
		/* Inform host of state change */
		cdcacmstate = 0;
		if (linestate & PIN_RI)
			cdcacmstate |= CDCACM_RI;
		if (linestate & PIN_DSR)
			cdcacmstate |= CDCACM_DSR;
		if (linestate & PIN_DCD)
			cdcacmstate |= CDCACM_DCD;

		cdcacm_line_state_changed_cb(cdcacmstate);
	}
	oldlinestate = linestate;
}

int main(void)
{
	gpio_enable_ahb_aperture();
	clock_setup();
	gpio_setup();

	cdcacm_init();
	uart_init();

	cm4f_enable_fpu();

	while (1)
		mainloop();

	return 0;
}
