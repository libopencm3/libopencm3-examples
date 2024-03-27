/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2024 Karl Palsson <karlp@tweak.net.au>
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

#include <errno.h>
#include <unistd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>

#include <libopencm3/stm32/l4/dfsdm.h>

#include <stdio.h>
#include "usart_stdio.h"

#define LED_GREEN_PORT GPIOA
#define LED_GREEN_PIN GPIO5

/* CPU core clock setup */
static void clock_setup(void)
{
    rcc_clock_setup_pll(&rcc_hsi16_configs[RCC_CLOCK_VRANGE1_80MHZ]);
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN_PIN);
}

void dfsdm_filter_setup(void)
{    
    dfsdm_flt_set_regular_ch(0, 0);
    dfsdm_flt_rcont_repeat(0);
    // dfsdm_flt_rcont_once(0);
    dfsdm_flt_reg_end_of_conv_irq_enable(0);

    dfsdm_flt_set_sinc_filter_order(0, 3);

    /* (FOSR = FOSR[9:0] +1) same as with clock divider */
    dfsdm_flt_set_sinc_oversample_ratio(0, 63);
    dfsdm_flt_set_integrator_oversample_ratio(0, 0);

    /* enable rdma */
    dfsdm_flt_rdma_enable(0);
    dfsdm_flt_filter_enable(0);

    printf("DFSDM_FLTxCR1: %lx\r\n", DFSDM_FLTxCR1(0));
    printf("DFSDM_FLTxCR1: %lx\r\n", DFSDM_FLTxCR2(0));
}

void dfsdm_setup(void)
{    
    /* Setup GPIO pins for AF5 for SPI1 signals. */
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO1);
    gpio_set_af(GPIOB, GPIO_AF6, GPIO1);

    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOC, GPIO_AF6, GPIO2);

    rcc_periph_clock_enable(RCC_DFSDM);

    dfsdm_set_ckout_src_sys(0);

    /* the actual divider is (Divider = CKOUTDIV+1), so we need to subtract by one: 80MHz/8 would be 10MHz sampling rate */
    dfsdm_set_ckout_divider(0,7);

    dfsdm_set_datpack_standard(0);
    dfsdm_set_datmpx_external(0);
    dfsdm_set_chinsel_same_channel(0);
    dfsdm_set_ckaben_disable(0);
    dfsdm_set_scden_disable(0);
    dfsdm_set_spi_clock_ckout(0);
    dfsdm_set_sitp_spi_rise_edge(0);

    dfsdm_set_offset(0, 0);
    dfsdm_set_data_shift(0, 0);

    dfsdm_set_chen_enable(0);

    printf("DFSDM_CHyCFGR1: %lx\r\n", DFSDM_CHyCFGR1(0));
    printf("DFSDM_CHyCFGR2: %lx\r\n", DFSDM_CHyCFGR2(0));
}

/*
 * The rate isr will be called is calculated like this:
 * fCKIN/(Fosr*(Iosr-1+Ford)+(Ford+1))
 * Taken from the reference manual STM32L476RG (24.4.13 Data unit block).
 * So e.g. fCKIN = 10MHz, Fosr = 16, Iosr = 0 (so in this formular I think we should use 1), 
 * Ford = 3 (Sinc3) this gives:
 * 10000000/(16*(1-1+3)+(3+1)) = 192307.69230
 */
void dfsdm0_isr(void)
{
    if(dfsdm_flt_get_eof_regular_conv_flag(0))
    {
        int32_t tmp_data = dfsdm_flt_get_regular_ch_conv_data(0);

        /* here you can do something with the converted data */
        (void) tmp_data;

        gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
    }
}

int main(void)
{
	clock_setup();
    gpio_setup();
    usart_setup();

    printf("Hello from stm32476rg nucleo DFSDM IRQ example!\r\n");

    nvic_enable_irq(NVIC_DFSDM0_IRQ);

    dfsdm_setup();
    dfsdm_filter_setup();
    dfsdm_enable(0);

    /* start regular conversion */
    dfsdm_flt_rswstart(0);

	while (1) 
    {
	}

	return 0;
}
