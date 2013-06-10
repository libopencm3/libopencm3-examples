/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Ken Sarkies <ksarkies@internode.on.net>
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

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/adc.h>
#include <libopencm3/dispatch/nvic.h>

u16 value = 500;

void gpio_setup(void);
void adc_setup(void);

/*--------------------------------------------------------------------------*/

int main(void)
{
	u32 i;

	gpio_setup();
	adc_setup();

	while (1) {
        adc_start_conversion_regular(ADC1);
		u32 count = 195*value;
		for (i = 0; i < count; i++) __asm__("nop");
		gpio_toggle(GPIOD, GPIO12);
		for (i = 0; i < count; i++) __asm__("nop");
		gpio_toggle(GPIOD, GPIO13);
	}

	return 1;
}

/*--------------------------------------------------------------------------*/

void gpio_setup(void)
{
/* GPIO LED ports */
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN | RCC_AHB1ENR_IOPAEN);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
		      GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ,
		      GPIO12 | GPIO13 | GPIO14 | GPIO15);
}

/*--------------------------------------------------------------------------*/

void adc_setup(void)
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);
/* Set mode of port PA1 for ADC1 to analogue input mode. */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);
/* Setup the ADC */
	nvic_enable_irq(NVIC_ADC_IRQ);
/* The channel array refers only to channel 1 of ADC1 */
	u8 channel[1] = { ADC_CHANNEL1 };
    adc_set_clk_prescale(ADC_CCR_ADCPRE_BY2);
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_set_sample_time(ADC1, ADC_CHANNEL1, ADC_SMPR1_SMP_1DOT5CYC);
	adc_set_multi_mode(ADC_CCR_MULTI_INDEPENDENT);
    adc_set_regular_sequence(ADC1, 1, channel);
	adc_enable_eoc_interrupt(ADC1);
    adc_power_on(ADC1);
}

/*--------------------------------------------------------------------------*/

void adc_isr(void)
{
        value = adc_read_regular(ADC1);
}

