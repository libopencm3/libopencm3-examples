/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Onno Kortmann <onno@gmx.net>
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

#include <libopencm3/stm32/f0/rcc.h>
#include <libopencm3/stm32/f0/gpio.h>
#include <libopencm3/cm3/systick.h>
 

void sys_tick_handler(void);

int main(void)
{
    /* Set system clock to max of 48MHz, generated from internal HSI oscillator,
       multiplied by PLL. */
    rcc_clock_setup_in_hsi_out_48mhz();

    /* Enable clocks to the GPIO subsystems */
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);

    /* Select pin functions. PC8/PC9 are the two LEDs on the STM32F0DISCOVERY
       board. */
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO8|GPIO9);
 
    gpio_mode_setup(GPIOA, GPIO_MODE_AF,
                    GPIO_PUPD_NONE, GPIO8);
    
    /* Enable system clock output on pin PA8 (so it can be checked with a scope) */
    rcc_set_mco(RCC_CFGR_MCO_SYSCLK);
 
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);

    /* clear counter so it starts right away */
    STK_CVR=0;
 
    /* At 48MHZ clock -> gives systick every 1/6th of a second. Faster for PLL
       multiplied system clocks. */
    systick_set_reload(8000000);
    systick_counter_enable();
    systick_interrupt_enable();
 
    while(1);
}
 
/* Called every systick reload */
void sys_tick_handler(void)
{
    static char c;
    if (c)
        gpio_clear(GPIOC, GPIO8);
    else
        gpio_set(GPIOC, GPIO8);
    c=!c;
}
