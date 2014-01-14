/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Karl Palsson <karlp@tweak.net.au>
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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "syscfg.h"
#include "usb_cdcacm.h"

static usbd_device *usbd_dev;

static void usart_setup(void)
{
	/* Enable the USART2 interrupt. */
	nvic_enable_irq(NVIC_USART2_IRQ);

	/* USART2 pins are on port A */
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);

	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Enable USART2 Receive interrupt. */
	usart_enable_rx_interrupt(USART2);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

void otg_fs_isr(void)
{
	usbd_poll(usbd_dev);
}

void usart2_isr(void)
{
	/* Check if we were called because of RXNE. */
	if ((USART_CR1(USART2) & USART_CR1_RXNEIE) &&
		(USART_SR(USART2) & USART_SR_RXNE)) {
		gpio_set(LED_RX_PORT, LED_RX_PIN);
		uint8_t c = usart_recv(USART2);
		glue_data_received_cb(&c, 1);
		gpio_clear(LED_RX_PORT, LED_RX_PIN);
	}
	if ((USART_CR1(USART2) & USART_CR1_TCIE) &&
		(USART_SR(USART2) & USART_SR_TC)) {
		USART_CR1(USART2) &= ~USART_CR1_TCIE;
		USART_SR(USART2) &= ~USART_SR_TC;
		gpio_clear(RS485DE_PORT, RS485DE_PIN);
		gpio_clear(LED_TX_PORT, LED_TX_PIN);
	}
}

int main(void)
{
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);
	/* Leds and rs485 are on port D */
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(LED_RX_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
		LED_RX_PIN | LED_TX_PIN);
	gpio_mode_setup(RS485DE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
		RS485DE_PIN);

	usart_setup();

	usb_cdcacm_init(&usbd_dev);

	while (1) {
		;
	}

}

