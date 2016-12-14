/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Ken Sarkies <ksarkies@internode.on.net>
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
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>

/* Timer 2 count period, 16 microseconds for a 72MHz APB2 clock */
#define PERIOD 1152

/* Globals */
uint8_t waveform[256];

/*--------------------------------------------------------------------*/
static void clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

/*--------------------------------------------------------------------*/
static void gpio_setup(void)
{
	/* Port A and C are on AHB1 */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);
	/* Set the digital test output on PC1 */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO1);
	/* Set PA4 for DAC channel 1 to analogue, ignoring drive mode. */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);
}

/*--------------------------------------------------------------------*/
static void timer_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);
	/* Timer global mode: - No divider, Alignment edge, Direction up */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_continuous_mode(TIM2);
	timer_set_period(TIM2, PERIOD);
	timer_disable_oc_output(TIM2, TIM_OC2 | TIM_OC3 | TIM_OC4);
	timer_enable_oc_output(TIM2, TIM_OC1);
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_TOGGLE);
	timer_set_oc_value(TIM2, TIM_OC1, 500);
	timer_disable_preload(TIM2);
	/* Set the timer trigger output (for the DAC) to the channel 1 output
	   compare */
	timer_set_master_mode(TIM2, TIM_CR2_MMS_COMPARE_OC1REF);
	timer_enable_counter(TIM2);
}

/*--------------------------------------------------------------------*/
static void dma_setup(void)
{
	/* DAC channel 1 uses DMA controller 1 Stream 5 Channel 7. */
	/* Enable DMA1 clock and IRQ */
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
	dma_stream_reset(DMA1, DMA_STREAM5);
	dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_LOW);
	dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
	dma_enable_circular_mode(DMA1, DMA_STREAM5);
	dma_set_transfer_mode(DMA1, DMA_STREAM5,
				DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	/* The register to target is the DAC1 8-bit right justified data
	   register */
	dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) &DAC_DHR8R1);
	/* The array v[] is filled with the waveform data to be output */
	dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) waveform);
	dma_set_number_of_data(DMA1, DMA_STREAM5, 256);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
	dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM5);
}

/*--------------------------------------------------------------------*/
static void dac_setup(void)
{
	/* Enable the DAC clock on APB1 */
	rcc_periph_clock_enable(RCC_DAC);
	/* Setup the DAC channel 1, with timer 2 as trigger source.
	 * Assume the DAC has woken up by the time the first transfer occurs */
	dac_trigger_enable(CHANNEL_1);
	dac_set_trigger_source(DAC_CR_TSEL1_T2);
	dac_dma_enable(CHANNEL_1);
	dac_enable(CHANNEL_1);
}

/*--------------------------------------------------------------------*/
/* The ISR simply provides a test output for a CRO trigger */

void dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		/* Toggle PC1 just to keep aware of activity and frequency. */
		gpio_toggle(GPIOC, GPIO1);
	}
}

/*--------------------------------------------------------------------*/
int main(void)
{
	/* Fill the array with funky waveform data */
	/* This is for dual channel 8-bit right aligned */
	uint16_t i, x;
	for (i = 0; i < 256; i++) {
		if (i < 10) {
			x = 10;
		} else if (i < 121) {
			x = 10 + ((i*i) >> 7);
		} else if (i < 170) {
			x = i/2;
		} else if (i < 246) {
			x = i + (80 - i/2);
		} else {
			x = 10;
		}
		waveform[i] = x;
	}
	clock_setup();
	gpio_setup();
	timer_setup();
	dma_setup();
	dac_setup();

	while (1);

	return 0;
}
