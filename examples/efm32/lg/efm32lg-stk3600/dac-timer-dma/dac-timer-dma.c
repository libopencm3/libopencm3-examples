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
#include <libopencm3/efm32/dac.h>
#include <libopencm3/efm32/timer.h>

#define SAMPLES_COUNT 1000
#define SAMPLES_VALUE_INC 4
static uint16_t samples[SAMPLES_COUNT] __attribute__((aligned(16)));

/* we need only 1 channel (0th channel) */
static uint8_t dma_desc_mem[DMA_DESC_CH_SIZE * 1] __attribute__((aligned(256)));

static void clock_setup(void)
{
	cmu_clock_setup_in_hfxo_out_48mhz();

	cmu_periph_clock_enable(CMU_GPIO);
	cmu_periph_clock_enable(CMU_DAC0);
	cmu_periph_clock_enable(CMU_DMA);
	cmu_periph_clock_enable(CMU_TIMER0);
}

static void led_setup(void)
{
	gpio_clear(GPIOE, GPIO2);
	gpio_mode_setup(GPIOE, GPIO_MODE_PUSH_PULL, GPIO2);
}

static void dac_setup(void)
{
	/* clock = 48Mhz / (2 ^ 6) = 48Mhz / 64    (<= 1Mhz)
	 * reference: VDD */
	DAC0_CTRL = DAC_CTRL_REFSEL(DAC_CTRL_REFSEL_VDD) | DAC_CTRL_PRESC(6) | DAC_CTRL_OUTMODE(DAC_CTRL_OUTMODE_PIN);
	DAC0_CH0CTRL = DAC_CH0CTRL_EN;
}

static void dma_setup(void)
{
	uint32_t dma_desc = (uint32_t) dma_desc_mem;

	/* DMA global */
	DMA_CONFIG = DMA_CONFIG_EN;
	DMA_CTRLBASE = dma_desc;

	/* channel descriptor */
	uint16_t n_minus_1 = SAMPLES_COUNT - 1;
	DMA_DESC_CHx_CFG(dma_desc, DMA_CH0) =
		DMA_DESC_CH_CFG_DEST_INC_NOINC | DMA_DESC_CH_CFG_DEST_SIZE_HALFWORD |
		DMA_DESC_CH_CFG_SRC_INC_HALFWORD | DMA_DESC_CH_CFG_SRC_SIZE_HALFWORD |
		DMA_DESC_CH_CFG_R_POWER(DMA_R_POWER_1) | DMA_DESC_CH_CFG_CYCLE_CTRL_BASIC |
		DMA_DESC_CH_CFG_N_MINUS_1(n_minus_1);
	DMA_DESC_CHx_DEST_DATA_END_PTR(dma_desc, DMA_CH0) =
		(uint32_t) &DAC0_CH0DATA;
	DMA_DESC_CHx_SRC_DATA_END_PTR(dma_desc, DMA_CH0) =
		((uint32_t) samples) + (n_minus_1 << 1);

	/* Channel 0 */
	DMA_CH0_CTRL = DMA_CH_CTRL_SOURCESEL(DMA_CH_CTRL_SOURCESEL_TIMER0) | DMA_CH_CTRL_SIGSEL(DMA_CH_CTRL_SIGSEL_TIMER0UFOF);
	DMA_CHREQMASKC = DMA_CHREQMASKC_CH0SREQMASKC;
	DMA_RDS = DMA_RDS_RDSCH0;
	DMA_LOOP0 = DMA_LOOP_EN | DMA_LOOP_WIDTH(n_minus_1);
	DMA_CHENS = DMA_CHENS_CH0SENS;
}

/* 250Khz */
static void timer_setup(void)
{
	TIMER0_CTRL = TIMER_CTRL_DMACLRACT;
	TIMER0_TOP = (48 * 2 * 2) - 1;
}

static void waveform_setup(void)
{
	unsigned i;

	/* triangular waveform */
	for (i = 0; i < SAMPLES_COUNT; i++) {
		samples[i] = i * SAMPLES_VALUE_INC;
	}
}

int main(void)
{
	clock_setup();
	waveform_setup();
	led_setup();
	dac_setup();
	dma_setup();
	timer_setup();

	/* start timer */
	TIMER0_CMD = TIMER_CMD_START;

	/* turn "LED0" on */
	gpio_set(GPIOE, GPIO2);

	while(1);
}
