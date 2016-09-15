/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
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

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/nvic.h>

#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/timer.h>

void rcc_clock_setup_in_hsi_out_48mhz_corrected(void);
void dac_init(void);
void gpio_init(void);

void rcc_clock_setup_in_hsi_out_48mhz_corrected(void)
{
	rcc_osc_on(HSI);
	rcc_wait_for_osc_ready(HSI);
	rcc_set_sysclk_source(HSI);

	// correction (f072 has PREDIV after clock multiplexer (near PLL)
	//Figure 12. Clock tree (STM32F07x devices)  P96 	RM0091
	//applies to rcc_clock_setup_in_hsi_out_*mhz()
	rcc_set_prediv(RCC_CFGR2_PREDIV_DIV2);

	rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
	rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

	flash_set_ws(FLASH_ACR_LATENCY_024_048MHZ);

	// 8MHz * 12 / 2 = 48MHz
	rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_MUL12);

	RCC_CFGR &= ~RCC_CFGR_PLLSRC;

	rcc_osc_on(PLL);
	rcc_wait_for_osc_ready(PLL);
	rcc_set_sysclk_source(PLL);

	rcc_ppre_frequency = 48000000;
	rcc_core_frequency = 48000000;
}

void gpio_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4 | GPIO5);
}

void dac_init(void)
{
	rcc_periph_clock_enable(RCC_DAC);
	
	dac_disable(CHANNEL_1);
	dac_set_waveform_characteristics(DAC_CR_MAMP1_12);
	dac_enable(CHANNEL_1);
	
	dac_load_data_buffer_single(0, RIGHT12, CHANNEL_1);
	dac_software_trigger(CHANNEL_1);
	dac_load_data_buffer_single(1000, RIGHT12, CHANNEL_1);
}

int main(void)
{
	rcc_clock_setup_in_hsi_out_48mhz_corrected();
	
	gpio_init();
	dac_init();
	
	while (1)
	{
		
	}
}
