/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2020 Victor Lamoine <victor.lamoine@gmail.com>
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
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>

void exti0_isr(void)
{
  exti_reset_request(EXTI0);
}

int main(void)
{
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
  rcc_periph_clock_enable(RCC_GPIOC);
  gpio_set_mode(GPIOC,
                GPIO_MODE_OUTPUT_2_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO13); // LED

  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_set_mode(GPIOA,
                GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN,
                GPIO0); // Button connected to 3V3
  gpio_clear(GPIOA, GPIO0); // Pull down resistor

  // Configure wake up
  rcc_periph_clock_enable(RCC_AFIO);  // EXTI
  exti_select_source(EXTI0, GPIOA); // PA0
  EXTI_IMR = 0x1; // Configure interrupt mask register for PA0
  exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
  exti_enable_request(EXTI0);
  nvic_enable_irq(NVIC_EXTI0_IRQ); // PC0 interrupt

  while (1)
  {
    // Toggle LED
    for (uint8_t i = 0; i < 30; ++i)
    {
      gpio_toggle(GPIOC, GPIO13);
      for (uint32_t j = 0; j < 2000000; ++j)
        __asm volatile("nop");
    }
    gpio_set(GPIOC, GPIO13); // LED off

    // Configure + go to sleep
    SCB_SCR |= SCB_SCR_SLEEPDEEP; // Set deepsleep bit
    pwr_set_stop_mode(); // Stop mode
    PWR_CR |= PWR_CR_LPDS; // Voltage regulator off
    exti_reset_request(EXTI0);
    __asm volatile("wfi"); // Sleep and wait for interrupt (rising edge) on PA0

    // HSI clock is used after stop mode
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP; // Clear deepsleep bit

    // Keep LED on for some time
    gpio_clear(GPIOC, GPIO13); // LED on
    for (uint32_t i = 0; i < 15; ++i)
    {
      for (uint32_t j = 0; j < 2000000; ++j)
        __asm volatile("nop");
    }
  }

  return 0;
}

