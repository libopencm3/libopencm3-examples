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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include "clock.h"


/**
 * Function definitions
 */
static void pin_setup(void);
static void update_led_counter(void);

/**
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
}

/**
 * Interrupts
 */

/* Blue button interrupt (EXTI makes not too much sense here :)) */
static bool blue_button_state_changed = false;
void exti0_isr()
{
	exti_reset_request(EXTI0);
	blue_button_state_changed = true;
	update_led_counter();
}

/**
 * Functions
 */
void update_led_counter()
{
	/* little grey code blinker */
	static const uint32_t  gcl[] = {0,1,3,2,6,7,5,4};
	static const uint32_t *gc    = gcl;
	static const uint32_t *gcle  = gcl+sizeof(gcl)/sizeof(gcl[0])-1;
	if (blue_button_state_changed) {
		blue_button_state_changed = false;
		gc = gcl;
	}
	if (BUTTON_BLUE_PRESSED()) {
		gpio_set(LED_LD1);
		gpio_set(LED_LD2);
		gpio_set(LED_LD3);
	} else {
		if (*gc&0b001) gpio_set(LED_LD1);
		else           gpio_clear(LED_LD1);
		if (*gc&0b010) gpio_set(LED_LD2);
		else           gpio_clear(LED_LD2);
		if (*gc&0b100) gpio_set(LED_LD3);
		else           gpio_clear(LED_LD3);
		if (gc==gcle) gc = gcl;
		else          gc++;
	}
}

/**
 * Main loop
 */
int main(void)
{
	/* init timers. */
	clock_setup();
	/* setup pins */
	pin_setup();

	while (1) {
		msleep(1000);
		update_led_counter();
	}
}


