/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Jonas Norling <jonas.norling@gmail.com>
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
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencmsis/core_cm3.h>

#include "scheduler.h"

/* Stack space for the tasks. Stacks on ARM should be 8-byte aligned. */
static uint8_t __attribute__((aligned(8))) stack1[256];
static uint8_t __attribute__((aligned(8))) stack2[256];

static void green_led_task(void);
static void blue_led_task(void);

const struct task_data tasks[SCHEDULER_NUM_TASKS] = {
		{ green_led_task, stack1, sizeof(stack1) },
		{ blue_led_task, stack2, sizeof(stack2) },
};

/* Current uptime in milliseconds */
volatile uint32_t uptime;


static float quicksine(float x)
{
	if (x < 0) {
		return 1.27323954f * x + .405284735f * x * x;
	} else {
		return 1.27323954f * x - 0.405284735f * x * x;
	}
}

/* Fade an LED in and out in a sinusoidal fashion. */
static void blinkloop(float speed, uint16_t ledpin)
{
	while (true) {
		float step;
		unsigned i;
		for (step = -3.14; step < 3.14; step += speed) {
			unsigned duty = 128 + 128 * quicksine(step);

			gpio_clear(GPIOC, ledpin); /* LED off */
			for (i = 0; i < 256; i++) {
				if (i == duty) {
					gpio_set(GPIOC, ledpin); /* LED on */
				}

				/* Let the other tasks run */
				scheduler_yield();
			}
		}
	}
}

static void green_led_task(void)
{
	blinkloop(0.005, GPIO9);
}

static void blue_led_task(void)
{
	blinkloop(0.017, GPIO8);
}

/* ISR for the systick interrupt. Triggered every 1ms. We could invoke the
 * scheduler here to implement preemptive multithreading. */
void sys_tick_handler(void)
{
	uptime++;
}

static void config_clocks(void)
{
	rcc_clock_setup_in_hse_8mhz_out_24mhz();

	/* Set systick timer to strike every 1ms (1kHz),
	 * enable its interrupt and start it. */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	systick_set_reload(rcc_apb2_frequency / 8 / 1000 - 1);
	systick_interrupt_enable();
	systick_counter_enable();
}

static void config_gpio(void)
{
	/* Enable GPIOC8 and GPIOC9 which are connected to the LEDs on the
	 * Discovery board */
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
			GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);

	gpio_set(GPIOC, GPIO8); /* Turn on blue */
	gpio_set(GPIOC, GPIO9); /* Turn on green */
}

int main(void)
{
	config_clocks();
	config_gpio();
	scheduler_init();

	/* Start running the tasks. This call will never return. */
	scheduler_yield();

	return 0;
}
