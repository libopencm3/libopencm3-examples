/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Jack Ziesing <jziesing@gmail.com>
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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>

#include <libopencmsis/core_cm3.h>

#ifndef ARRAY_LEN
#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))
#endif

uint16_t frequency_sequence[18] = {
	1000,
	500,
	1000,
	500,
	1000,
	500,
	2000,
	500,
	2000,
	500,
	2000,
	500,
	1000,
	500,
	1000,
	500,
	1000,
	5000,
};

uint16_t frequency_sel = 0;

uint16_t compare_time;
uint16_t new_time;
uint16_t frequency;
int debug = 0;

static void clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
}

static void gpio_setup(void)
{
	/* Enable GPIO clock for leds. */
	rcc_periph_clock_enable(RCC_GPIOD);

	/* Set GPIO12 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
		      GPIO_PUPD_NONE, GPIO12 | GPIO13);

	gpio_set(GPIOD, GPIO12);
	gpio_clear(GPIOD, GPIO13);
}

static void tim_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);

	/* Enable TIM2 interrupt. */
	nvic_enable_irq(NVIC_TIM2_IRQ);

	/* Reset TIM2 peripheral to defaults. */
	rcc_periph_reset_pulse(RST_TIM2);

	/* Timer global mode:
	 * - No divider
	 * - Alignment edge
	 * - Direction up
	 * (These are actually default values after reset above, so this call
	 * is strictly unnecessary, but demos the api for alternative settings)
	 */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        /*
         * Please take note that the clock source for STM32F4 timers
         * might not be the raw APB1/APB2 clocks.  In various conditions they
         * are doubled.  See the Reference Manual for full details!
         * In our case, TIM2 on APB1 is running at double frequency, so this
         * sets the prescaler to have the timer run at 10kHz
         */
	timer_set_prescaler(TIM2, ((rcc_apb1_frequency * 2) / 10000));

	/* Disable preload. */
	timer_disable_preload(TIM2);
	timer_continuous_mode(TIM2);

	/* count full range, as we'll update compare value continuously */
	timer_set_period(TIM2, 65535);

	/* Set the initual output compare value for OC1. */
	timer_set_oc_value(TIM2, TIM_OC1, 1000);

	/* Counter enable. */
	timer_enable_counter(TIM2);

	/* Enable Channel 1 compare interrupt to recalculate compare values */
	timer_enable_irq(TIM2, TIM_DIER_CC1IE);
}

void tim2_isr(void)
{
	if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {

		/* Clear compare interrupt flag. */
		timer_clear_flag(TIM2, TIM_SR_CC1IF);

		/*
		 * Get current timer value to calculate next
		 * compare register value.
		 */
		compare_time = timer_get_counter(TIM2);

		/* Calculate and set the next compare value. */
		frequency = frequency_sequence[frequency_sel++];
		new_time = compare_time + frequency;

		timer_set_oc_value(TIM2, TIM_OC1, new_time);
		if (frequency_sel == ARRAY_LEN(frequency_sequence)) {
			frequency_sel = 0;
		}

		/* Toggle LED to indicate compare event. */
		gpio_toggle(GPIOD, GPIO12);
		gpio_toggle(GPIOD, GPIO13);
	}
}

int main(void)
{
	clock_setup();
	gpio_setup();
	tim_setup();

	/* Loop calling Wait For Interrupt. In older pre cortex ARM this is
	 * just equivalent to nop. On cortex it puts the cpu to sleep until
	 * one of the three occurs:
	 *
	 * a non-masked interrupt occurs and is taken
	 * an interrupt masked by PRIMASK becomes pending
	 * a Debug Entry request
	 */
	while (1)
		__WFI(); /* Wait For Interrupt. */

	return 0;
}
