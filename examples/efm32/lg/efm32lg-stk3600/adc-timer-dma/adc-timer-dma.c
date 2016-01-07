/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
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

#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/gpio.h>
#include <libopencm3/efm32/dma.h>
#include <libopencm3/efm32/adc.h>
#include <libopencm3/efm32/prs.h>
#include <libopencm3/efm32/timer.h>

#define SAMPLES_COUNT 1000
static uint16_t samples[SAMPLES_COUNT] __attribute__((aligned(16)));

/* we need only 1 channel (0th channel) */
static uint8_t dma_desc_mem[DMA_DESC_CH_SIZE * 1] __attribute__((aligned(256)));

static void clock_setup(void)
{
	cmu_clock_setup_in_hfxo_out_48mhz();

	cmu_periph_clock_enable(CMU_GPIO);
	cmu_periph_clock_enable(CMU_ADC0);
	cmu_periph_clock_enable(CMU_DMA);
	cmu_periph_clock_enable(CMU_TIMER0);
	cmu_periph_clock_enable(CMU_PRS);
}

static void led_setup(void)
{
	gpio_clear(GPIOE, GPIO2);
	gpio_mode_setup(GPIOE, GPIO_MODE_PUSH_PULL, GPIO2);
}

static void adc_setup(void)
{
	/*
	 * Tconv = (Ta + N) * OVS
	 * (Tconv / 12Mhz) < (1 / 500Khz)   [500Khz is timer (PRS CH0) trigger rate]
	 * Tconv < (120 / 5)
	 * Tconv < 24
	 *
	 * we need atleast 12bit resolution, N = 13
	 * (Ta + 13) * OVS < 24
	 *  for OVS = 1
	 *
	 * Ta < 11
	 * so, Ta = 8
	 * Acquisition time = 8 cycles
	 */

	/*
	 * ADC Warm up = 48Mhz / Timebase = 48Mhz / (95 + 1) = 2us  (and is <= 1us)
	 * prescalar = 48Mhz / (3 + 1) = 12Mhz  (and is <= 13Mhz)
	 * keep adc warm between samples
	 */
	/* TODO: workaround(ADC_E117) */
	ADC0_CTRL = ADC_CTRL_TIMEBASE(95) | ADC_CTRL_PRESC(3) |
			ADC_CTRL_WARMUPMODE(ADC_CTRL_WARMUPMODE_KEEPADCWARM);

	/*
	 * get trigger from PRS Ch0  (connected to timer0, at 500Khz)
	 * Reference: VDD
	 * Acquisition Time: 8 ADC Clocks
	 * Resolution: 12bit
	 * Channels: {0, 1, 2, 3, 4, 5, 6, 7}
	 */
	ADC0_SCANCTRL = ADC_SCANCTRL_PRSSEL(ADC_SCANCTRL_PRSSEL_PRSCH0) | ADC_SCANCTRL_PRSEN |
					ADC_SCANCTRL_AT(ADC_SCANCTRL_AT_8CYCLES) | ADC_SCANCTRL_REF(ADC_SCANCTRL_REF_VDD) |
					ADC_SCANCTRL_INPUTSEL_CH0 | ADC_SCANCTRL_INPUTSEL_CH1 |
					ADC_SCANCTRL_INPUTSEL_CH2 | ADC_SCANCTRL_INPUTSEL_CH3 |
					ADC_SCANCTRL_INPUTSEL_CH4 | ADC_SCANCTRL_INPUTSEL_CH5 |
					ADC_SCANCTRL_INPUTSEL_CH6 | ADC_SCANCTRL_INPUTSEL_CH7 |
					ADC_SCANCTRL_RES(ADC_SCANCTRL_RES_12BIT);
}

static void dma_setup(void)
{
	uint32_t dma_desc = (uint32_t) dma_desc_mem;

	/* DMA global */
	DMA_CONFIG = DMA_CONFIG_EN;
	DMA_CTRLBASE = dma_desc;

	/* channel descriptor */
	uint16_t n_minus_1 = SAMPLES_COUNT - 1;
	DMA_DESC_CHx_CFG(dma_desc, 0) =
		DMA_DESC_CH_CFG_DEST_INC_HALFWORD | DMA_DESC_CH_CFG_DEST_SIZE_HALFWORD |
		DMA_DESC_CH_CFG_SRC_INC_NOINC | DMA_DESC_CH_CFG_SRC_SIZE_HALFWORD |
		DMA_DESC_CH_CFG_R_POWER(DMA_R_POWER_1) | DMA_DESC_CH_CFG_CYCLE_CTRL_BASIC |
		DMA_DESC_CH_CFG_N_MINUS_1(n_minus_1);
	DMA_DESC_CHx_DEST_DATA_END_PTR(dma_desc, 0) = ((uint32_t) samples) + (n_minus_1 << 1);
	DMA_DESC_CHx_SRC_DATA_END_PTR(dma_desc, 0) = (uint32_t) &ADC0_SCANDATA;

	/* Channel 0 */
	DMA_CH0_CTRL = DMA_CH_CTRL_SOURCESEL(DMA_CH_CTRL_SOURCESEL_ADC0) | DMA_CH_CTRL_SIGSEL(DMA_CH_CTRL_SIGSEL_ADC0SCAN);
	DMA_CHREQMASKC = DMA_CHREQMASKC_CH0SREQMASKC;
	DMA_RDS = DMA_RDS_RDSCH0;
	DMA_CHENS = DMA_CHENS_CH0SENS;
}

static void timer_setup(void)
{
	TIMER0_TOP = (48 * 2 * 8) - 1;
}

static void prs_setup(void)
{
	PRS_CH0_CTRL = PRS_CH_CTRL_SOURCESEL(PRS_CH_CTRL_SOURCESEL_TIMER0) | PRS_CH_CTRL_SIGSEL(PRS_CH_CTRL_SIGSEL_TIMER0OF);
}

int main(void)
{
	clock_setup();
	led_setup();
	adc_setup();
	dma_setup();
	timer_setup();
	prs_setup();

	/* start timer */
	TIMER0_CMD = TIMER_CMD_START;

	/* wait till dma not received data */
	while(!(DMA_IF & DMA_IF_CH0DONE));

	/* stop timer */
	TIMER0_CMD = TIMER_CMD_STOP;

	/* turn "LED0" on */
	gpio_set(GPIOE, GPIO2);

	while(1);
	return 0;
}
