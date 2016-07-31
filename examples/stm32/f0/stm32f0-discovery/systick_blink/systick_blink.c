/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Chuck McManis <cmcmanis@mcmanis.com>
 * Copyright (C) 2013 Onno Kortmann <onno@gmx.net>
 * Copyright (C) 2013 Frantisek Burian <BuFran@seznam.cz> (merge)
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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

/* Called when systick fires */
void sys_tick_handler(void)
{
	gpio_toggle(GPIOC, GPIO8);
}

/*
 * Set up timer to fire every x milliseconds
 * This is a unusual usage of systick, be very careful with the 24bit range
 * of the systick counter!  You can range from 1 to 2796ms with this.
 */
static void systick_setup(int xms)
{
	/* div8 per ST, stays compatible with M3/M4 parts, well done ST */
	systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
	/* clear counter so it starts right away */
	STK_CVR = 0;

	systick_set_reload(rcc_ahb_frequency / 8 / 1000 * xms);
	systick_counter_enable();
	systick_interrupt_enable();
}

/* set STM32 to clock by 48MHz from HSI oscillator */
static void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();

	/* Enable clocks to the GPIO subsystems */
	rcc_periph_clock_enable(RCC_GPIOC);
}

static void gpio_setup(void)
{
	/* Set blue led (PC8) as output */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);
}

static void mco_setup(void)
{
	/* PA8 to AF 0 for MCO */
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO8);
	gpio_set_af(GPIOA, 0, GPIO8);

	/* clock output on pin PA8 (allows checking with scope) */
	rcc_set_mco(RCC_CFGR_MCO_SYSCLK);
}

int main(void)
{
	clock_setup();
	gpio_setup();
	mco_setup();

	/* 125ms ticks =>  250ms period => 4Hz blinks */
	systick_setup(125);

	/* Do nothing in main loop */
	while (1);
}
