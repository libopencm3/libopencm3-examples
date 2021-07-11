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
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "usb_cdcacm.h"
#include "syscfg.h"

void usb_cdcacm_setup_pre_arch(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_OTGFS);
	rcc_periph_clock_enable(RCC_DMA1);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO9 | GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);

}

void usb_cdcacm_setup_post_arch(void)
{
	/* Better enable interrupts */
	nvic_enable_irq(NVIC_OTG_FS_IRQ);
	nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);
}

static void dma_write(uint8_t *data, int size)
{
	/* Reset DMA channel*/
	dma_stream_reset(DMA1, STREAM_USART2_TX);

	dma_set_peripheral_address(DMA1,
		STREAM_USART2_TX, (uint32_t) &USART2_DR);
	dma_set_memory_address(DMA1, STREAM_USART2_TX, (uint32_t) data);
	dma_set_number_of_data(DMA1, STREAM_USART2_TX, size);
	dma_set_transfer_mode(DMA1,
		STREAM_USART2_TX, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_enable_memory_increment_mode(DMA1, STREAM_USART2_TX);
	dma_set_peripheral_size(DMA1, STREAM_USART2_TX, DMA_SxCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, STREAM_USART2_TX, DMA_SxCR_MSIZE_8BIT);
	dma_set_priority(DMA1, STREAM_USART2_TX, DMA_SxCR_PL_HIGH);
	dma_enable_transfer_complete_interrupt(DMA1, STREAM_USART2_TX);
	dma_channel_select(DMA1, STREAM_USART2_TX, DMA_SxCR_CHSEL_4);
	dma_enable_stream(DMA1, STREAM_USART2_TX);

	usart_enable_tx_dma(USART2);
}

void dma1_stream6_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM6, DMA_TCIF)) {
		USART_CR1(USART2) |= USART_CR1_TCIE;
		dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
	}
}

void glue_send_data_cb(uint8_t *buf, uint16_t len)
{
	gpio_set(LED_TX_PORT, LED_TX_PIN);
	gpio_set(RS485DE_PORT, RS485DE_PIN);
	dma_write(buf, len);
}

void glue_set_line_state_cb(uint8_t dtr, uint8_t rts)
{
	(void) dtr;
	(void) rts;
	// LM4f has an implementation of this if you're keen
}

int glue_set_line_coding_cb(uint32_t baud, uint8_t databits,
	enum usb_cdc_line_coding_bParityType cdc_parity,
	enum usb_cdc_line_coding_bCharFormat cdc_stopbits)
{
	int uart_parity;
	int uart_stopbits;

	if (databits < 8 || databits > 9) {
		return 0;
	}

	/* Be careful here, ST counts parity as a data bit */
	switch (cdc_parity) {
	case USB_CDC_NO_PARITY:
		uart_parity = USART_PARITY_NONE;
		break;
	case USB_CDC_ODD_PARITY:
		uart_parity = USART_PARITY_ODD;
		databits++;
		break;
	case USB_CDC_EVEN_PARITY:
		uart_parity = USART_PARITY_EVEN;
		databits++;
		break;
	default:
		return 0;
	}

	switch (cdc_stopbits) {
	case USB_CDC_1_STOP_BITS:
		uart_stopbits = USART_STOPBITS_1;
		break;
	case USB_CDC_2_STOP_BITS:
		uart_stopbits = USART_STOPBITS_2;
		break;
	default:
		return 0;
	}

	/* Disable the UART while we mess with its settings */
	usart_disable(USART2);
	/* Set communication parameters */
	usart_set_baudrate(USART2, baud);
	usart_set_databits(USART2, databits);
	usart_set_parity(USART2, uart_parity);
	usart_set_stopbits(USART2, uart_stopbits);
	/* Back to work. */
	usart_enable(USART2);

	return 1;
}
