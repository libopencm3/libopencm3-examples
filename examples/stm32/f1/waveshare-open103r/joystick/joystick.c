/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Joshua Harlan Lifton <joshua.harlan.lifton@gmail.com>
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

/* Joystick definitions */
#define JOY_PORT   GPIOA
#define JOY_STATE  GPIOA_IDR
#define JOY_LEFT   GPIO0
#define JOY_UP     GPIO1
#define JOY_DOWN   GPIO2
#define JOY_RIGHT  GPIO3
#define JOY_CENTER GPIO4
#define JOY_ALL (JOY_LEFT | JOY_UP | JOY_DOWN | JOY_RIGHT | JOY_CENTER)
#define JOY_DEBOUNCE_TICKS 20000

/* LED array definitions */
#define LED_PORT   GPIOC
#define LED1       GPIO9
#define LED2       GPIO10
#define LED3       GPIO11
#define LED4       GPIO12
#define LED_ALL    (LED1 | LED2 | LED3 | LED4)
#define BLINK_TICK 80000

uint16_t joy_state;
uint16_t led_state;
bool led_blinking;

/* Set STM32 to 24 MHz. */
static void clock_setup(void)
{
  rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_24MHZ]);
}

static void led_setup(void)
{
  /* Enable GPIOC clock. */
  rcc_periph_clock_enable(RCC_GPIOC);
  /* Set LEDs to output push-pull. */
  gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, LED_ALL);
  /* Turn off all LEDs. */
  gpio_clear(LED_PORT, LED_ALL);
}

static void joystick_setup(void)
{
  /* Enable GPIOA clock. */
  rcc_periph_clock_enable(RCC_GPIOA);
  /* Set joystick pins to input. */
  gpio_set_mode(JOY_PORT, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_PULL_UPDOWN,
		JOY_ALL);
  /* Enable all joystick pin pull-up resistors. */
  gpio_set(JOY_PORT, JOY_ALL);
}

int main(void)
{
  int i = 0;
  int debounce = 0;
  clock_setup();
  led_setup();
  joystick_setup();

  joy_state = 0;
  led_state = LED1;
  led_blinking = false;

  while (1) {

    /* Set LED state according to joystick state. */
    joy_state = (~JOY_STATE) & JOY_ALL;
    if (joy_state != 0 && debounce == 0) {
      debounce = 1;
      gpio_clear(LED_PORT, LED_ALL);
      if (joy_state & JOY_UP) {
        if (led_state == LED_ALL || led_state == 0) {
	  led_state = LED4;
	} else {
	  led_state >>= 1;
	  if (led_state < LED1) {
	    led_state = LED4;
	  }
	}
      } else if (joy_state & JOY_DOWN) {
	if (led_state == LED_ALL || led_state == 0) {
	  led_state = LED1;
	} else {
	  led_state <<= 1;
	  if (led_state > LED4 || led_state == 0) {
	    led_state = LED1;
	  }
	}
      } else if (joy_state & JOY_LEFT) {
	led_state = LED_ALL;
      } else if (joy_state & JOY_RIGHT) {
	led_state = 0;
      } else if (joy_state & JOY_CENTER) {
	led_blinking = !led_blinking;
      }
    }

    /* Debounce joystick press. */
    else if (debounce > 0 && debounce < JOY_DEBOUNCE_TICKS) {
      debounce++;
    } else if (joy_state == 0 && debounce == JOY_DEBOUNCE_TICKS) {
      debounce++;
    }

    /* Debounce joystick release. */
    else if (debounce > JOY_DEBOUNCE_TICKS && debounce < 2*JOY_DEBOUNCE_TICKS) {
      debounce++;
    } else if (joy_state == 0 && debounce == 2*JOY_DEBOUNCE_TICKS) {
      debounce = 0;
    }

    /* Update LEDs on the fly rather than wait for blink to complete. */
    if (i++ < BLINK_TICK || !led_blinking) {
      gpio_set(LED_PORT, led_state);
    } else {
      gpio_clear(LED_PORT, led_state);
    }
    if (i > 2*BLINK_TICK) {
      i = 0;
    }

  }
  return 0;
}
