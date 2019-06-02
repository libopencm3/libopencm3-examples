/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Copyright (C) 2012 Daniel Serpell <daniel.serpell@gmail.com>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Chuck McManis <cmcmanis@mcmanis.com>
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

#include <stdio.h>
#include <ctype.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include "clock.h"

void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	/* set up the SysTick function (1mS interrupts) */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
	STK_CVR = 0;
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_counter_enable();
	systick_interrupt_enable();
}

/* simple millisecond counter */
static volatile uint32_t system_millis;
static volatile uint32_t delay_timer;

/*
 * Simple systick handler
 *
 * Increments a 32 bit value once per millesecond
 * which rolls over every 49 days.
 */
void sys_tick_handler(void)
{
	system_millis++;
	if (delay_timer > 0) {
		delay_timer--;
	}
}

/*
 * Simple spin loop waiting for time to pass
 *
 * A couple of things to note:
 * First,  you can't just compare to
 * system_millis because doing so will mean
 * you delay forever if you happen to hit a
 * time where it is rolling over.
 * Second, accuracy is "at best" 1mS as you
 * may call this "just before" the systick hits
 * with a value of '1' and it would return
 * nearly immediately. So if you need really
 * precise delays, use one of the timers.
 */
void
msleep(uint32_t delay)
{
	delay_timer = delay;
	while (delay_timer);
}

uint32_t
mtime(void)
{
	return system_millis;
}
