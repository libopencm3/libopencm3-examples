/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Joost Rijneveld <joost@joostrijneveld.nl>
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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>

const int datasize = 32;

char buffer[32];

static void clock_setup(void)
{
    rcc_clock_setup_pll(&clock_config[CLOCK_VRANGE1_HSI_PLL_32MHZ]);
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_GPIOAEN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART2EN);
    rcc_periph_clock_enable(RCC_DMA1);
}

static void gpio_setup(void)
{
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
}

static void usart_setup(void)
{
    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

    usart_enable(USART2);
}

static void dma_setup(void)
{
    dma_channel_reset(DMA1, DMA_CHANNEL6);
    
    nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);
    
    // USART2_DR (not USART2_BASE) is where the data will be received
    dma_set_peripheral_address(DMA1, DMA_CHANNEL6, (uint32_t) &USART2_DR);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL6);

    // should be 8 bit for USART2 as well as for the STM32L1
    dma_set_peripheral_size(DMA1, DMA_CHANNEL6, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL6, DMA_CCR_MSIZE_8BIT);

    dma_set_priority(DMA1, DMA_CHANNEL6, DMA_CCR_PL_VERY_HIGH);

    // should be disabled for USART2, but varies for other peripherals
    dma_disable_peripheral_increment_mode(DMA1, DMA_CHANNEL6);
    // should be enabled, otherwise buffer[0] is overwritten
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL6);

    dma_set_memory_address(DMA1, DMA_CHANNEL6, (uint32_t) &buffer);
    dma_set_number_of_data(DMA1, DMA_CHANNEL6, datasize);
    
    dma_disable_transfer_error_interrupt(DMA1, DMA_CHANNEL6);
    dma_disable_half_transfer_interrupt(DMA1, DMA_CHANNEL6);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL6);

    usart_enable_rx_dma(USART2);
    dma_enable_channel(DMA1, DMA_CHANNEL6);
}

void dma1_channel6_isr(void) {
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL6, DMA_TCIF);
    dma_disable_channel(DMA1, DMA_CHANNEL6);
    usart_disable_rx_dma(USART2);

    int i;
    for (i = 0; i < datasize; i++) {
        usart_send_blocking(USART2, buffer[i]);
    }
    usart_send_blocking(USART2, '\r');
    usart_send_blocking(USART2, '\n');
}

int main(void)
{
    clock_setup();
    gpio_setup();
    usart_setup();
    dma_setup();

    while (1);

    return 0;
}
