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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

#include <libopencmsis/core_cm3.h>

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

	/* Reset TIM2 peripheral. */
	timer_reset(TIM2);

	/* Timer global mode:
	 * - No divider
	 * - Alignment edge
	 * - Direction up
	 */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	/* Reset prescaler value.
	 * Running the clock at 5kHz.
	 */
	/*
	 * On STM32F4 the timers are not running directly from pure APB1 or
	 * APB2 clock busses.  The APB1 and APB2 clocks used for timers might
	 * be the double of the APB1 and APB2 clocks.  This depends on the
	 * setting in DCKCFGR register. By default the behaviour is the
	 * following: If the Prescaler APBx is greater than 1 the derived timer
	 * APBx clocks will be double of the original APBx frequencies. Only if
	 * the APBx prescaler is set to 1 the derived timer APBx will equal the
	 * original APBx frequencies.
	 *
	 * In our case here the APB1 is devided by 4 system frequency and APB2
	 * divided by 2. This means APB1 timer will be 2 x APB1 and APB2 will
	 * be 2 x APB2. So when we try to calculate the prescaler value we have
	 * to use rcc_apb1_freqency * 2!!! 
	 *
	 * For additional information see reference manual for the stm32f4
	 * familiy of chips. Page 204 and 213
	 */
	timer_set_prescaler(TIM2, ((rcc_apb1_frequency * 2) / 10000));

	/* Disable preload. */
	timer_disable_preload(TIM2);

	/* Continous mode. */
	timer_continuous_mode(TIM2);

	/* Period (36kHz). */
	timer_set_period(TIM2, 65535);

	/* Disable outputs. */
	timer_disable_oc_output(TIM2, TIM_OC1);
	timer_disable_oc_output(TIM2, TIM_OC2);
	timer_disable_oc_output(TIM2, TIM_OC3);
	timer_disable_oc_output(TIM2, TIM_OC4);

	/* -- OC1 configuration -- */

	/* Configure global mode of line 1. */
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_FROZEN);

	/* Set the capture compare value for OC1. */
	timer_set_oc_value(TIM2, TIM_OC1, 1000);

	/* ---- */

	/* ARR reload enable. */
	timer_disable_preload(TIM2);

	/* Counter enable. */
	timer_enable_counter(TIM2);

	/* Enable commutation interrupt. */
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
		if (frequency_sel == 18)
			frequency_sel = 0;

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
