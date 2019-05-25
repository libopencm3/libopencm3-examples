/*
 * This file is part of the libopencm3 project.
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

#include <stdbool.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include "clock.h"
#include "sdram.h"

/**
 * Function definitions
 */
static void pin_setup(void);
static void ram_test(void);

/***********************************************************************
 * Setup functions
 */
#define LED_LD1 GPIOJ,GPIO13
#define LED_LD2 GPIOJ,GPIO5
#define LED_LD3 GPIOA,GPIO12
#define BUTTON_BLUE GPIOA,GPIO0
#define BUTTON_BLUE_PRESSED() gpio_get(BUTTON_BLUE)
void pin_setup()
{
	/* pin-clocks */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOJ);
	/* pins */
	/* outputs (LEDs 1-3) */
	gpio_mode_setup(GPIOJ, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5 | GPIO13);
	gpio_set_output_options(GPIOJ, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO5 | GPIO13);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO12);
	/* inputs (blue button) */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
	/* setup blue/user button */
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_BOTH);
	exti_enable_request(EXTI0);
	nvic_enable_irq(NVIC_EXTI0_IRQ);

	/* setup sdram clocks */
	rcc_periph_clock_enable(RCC_FMC);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOH);
	rcc_periph_clock_enable(RCC_GPIOI);
	
	/* setup sdram pins */
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
	gpio_set_af(GPIOD, GPIO_AF12,
		GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);
	
	gpio_mode_setup(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0  | GPIO1  | GPIO7  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_output_options(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO0  | GPIO1  | GPIO7  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_af(GPIOE, GPIO_AF12,
		GPIO0  | GPIO1  | GPIO7  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_af(GPIOF, GPIO_AF12,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	
	gpio_mode_setup(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0  | GPIO1  | GPIO2  | GPIO4  | GPIO5  | GPIO8  | GPIO15);
	gpio_set_output_options(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO0  | GPIO1  | GPIO2  | GPIO4  | GPIO5  | GPIO8  | GPIO15);
	gpio_set_af(GPIOG, GPIO_AF12,
		GPIO0  | GPIO1  | GPIO2  | GPIO4  | GPIO5  | GPIO8  | GPIO15);

	gpio_mode_setup(GPIOH, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO2  | GPIO3  | GPIO5  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_output_options(GPIOH, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO2  | GPIO3  | GPIO5  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	gpio_set_af(GPIOH, GPIO_AF12,
		GPIO2  | GPIO3  | GPIO5  | GPIO8  | GPIO9  | GPIO10 |
		GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);
	
	gpio_mode_setup(GPIOI, GPIO_MODE_AF, GPIO_PUPD_NONE,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO6  | GPIO7  | GPIO9  | GPIO10);
	gpio_set_output_options(GPIOI, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO6  | GPIO7  | GPIO9  | GPIO10);
	gpio_set_af(GPIOI, GPIO_AF12,
		GPIO0  | GPIO1  | GPIO2  | GPIO3  | GPIO4  | GPIO5  |
		GPIO6  | GPIO7  | GPIO9  | GPIO10);
}

/***********************************************************************
 * Interrupts
 */

/* Blue button interrupt */
void exti0_isr()
{
	exti_reset_request(EXTI0);
	/* Re-/start ram_test */
	ram_test();
}

/***********************************************************************
 * Functions
 */

/**
 * Very simple ram checker
 */
void ram_test()
{
	/* Set error and success */
	gpio_set(LED_LD1);
	gpio_set(LED_LD2);

	/* Iterate over the whole ram area */
	uint32_t error_count=0;
#define SDRAM_SIZE  (0x1000000/sizeof(uint32_t))
	uint32_t *sdram;

	/* Write */
	sdram = (uint32_t *)SDRAM1_BASE_ADDRESS;
	for (uint32_t i = 0; i<SDRAM_SIZE; i++) {
		/* Write value to ram */
		*sdram++=i;
		/* Blink error LED */
		if (i&0x40000) {
			gpio_set(LED_LD1);
		} else {
			gpio_clear(LED_LD1);
		}
	}
	gpio_set(LED_LD1);
	
	/* Read */
	sdram = (uint32_t *)SDRAM1_BASE_ADDRESS;
	for (uint32_t i = 0; i<SDRAM_SIZE; i++) {
		/* Read and compare value */
		if (*sdram++!=i) error_count++;
		/* Blink success LED */
		if (i&0x40000) {
			gpio_set(LED_LD2);
		} else {
			gpio_clear(LED_LD2);
		}
	}
	gpio_set(LED_LD2);

	/* Clear error or success */
	if (error_count>0) {
		gpio_clear(LED_LD2);
	} else {
		gpio_clear(LED_LD1);
	}
}

/***********************************************************************
 * Main loop
 */
int main(void)
{
	/* Init timers. */
	clock_setup();
	/* Setup pins */
	pin_setup();
	/* Setup sdram */
	sdram_init();
	/* Run initial ram test */
	ram_test();
	/* Wait forever */
	while (1) {}
}


