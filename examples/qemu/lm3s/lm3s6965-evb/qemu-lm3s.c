/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Daniele Lacamera <root@danielinux.net>
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
#include <libopencm3/cm3/systick.h>
#include <libopencmsis/core_cm3.h>


/* LM3S Specifics, usart and RCC */
#include <libopencm3/lm3s/rcc.h>
#include <libopencm3/lm3s/usart.h>

static volatile uint32_t jiffies = 0u;

static void printbanner(void)
{
    char msg[] = "One second elapsed!\r\n";
    char *c = msg;
    while (*c != 0) {
        usart_send(USART0_BASE, (uint16_t )*c);
        c++;
    }
}

void sys_tick_handler(void)
{
    jiffies++;
}

static inline uint32_t systick_app_config(uint32_t n_ticks)
{
    /* constant from systick_set_reload -- as this returns something that's
     *      * not void, this is the only possible error condition */
    if (n_ticks & ~0x00FFFFFF) {
        return 1;
    }

    systick_set_reload(n_ticks);
    systick_set_clocksource(1);
    systick_interrupt_enable();
    systick_counter_enable();

    return 0;
}

#define HZ 1000

int main(void)
{
    rcc_clock_setup_in_xtal_8mhz_out_50mhz();
    systick_app_config(HZ);

    nvic_set_priority(NVIC_SYSTICK_IRQ, 0);
    jiffies = 0;
    while(1) {
        if (jiffies > HZ) {
            printbanner();
            jiffies = 0;
        }
    }
}
