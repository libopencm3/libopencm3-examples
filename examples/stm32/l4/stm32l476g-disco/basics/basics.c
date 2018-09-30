/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018 Karl Palsson <karlp@tweak.net.au>
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#define LED_GREEN_PORT GPIOE
#define LED_GREEN_PIN GPIO8
#define LED_RED_PORT GPIOB
#define LED_RED_PIN GPIO2

/* joysticks are on PA0,1,2,3,5 (center, left, right, up, down) */
#define JOY_PORT GPIOA
#define JOYC_EXTI EXTI0
#define JOYC_PIN GPIO0
#define JOYC_NVIC NVIC_EXTI0_IRQ

#define USART_CONSOLE USART2  /* PD5/6 , af7 */

int _write(int file, char *ptr, int len);

struct state_t {
	bool falling;
	int last_hold;
	//int tickcount;
};

static struct state_t state;


static void clock_setup(void)
{
	/* FIXME - this should eventually become a clock struct helper setup */
	rcc_osc_on(RCC_HSI16);
	
	flash_prefetch_enable();
	flash_set_ws(4);
	flash_dcache_enable();
	flash_icache_enable();
	/* 16MHz / 4 = > 4 * 40 = 160MHz VCO => 80MHz main pll  */
	rcc_set_main_pll(RCC_PLLCFGR_PLLSRC_HSI16, 4, 40,
			0, 0, RCC_PLLCFGR_PLLR_DIV2);
	rcc_osc_on(RCC_PLL);
	/* either rcc_wait_for_osc_ready() or do other things */

	/* Enable clocks for the ports we need */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);

	/* Enable clocks for peripherals we need */
	rcc_periph_clock_enable(RCC_USART2);
        rcc_periph_clock_enable(RCC_TIM7);
	rcc_periph_clock_enable(RCC_SYSCFG);

	rcc_set_sysclk_source(RCC_CFGR_SW_PLL); /* careful with the param here! */
	rcc_wait_for_sysclk_status(RCC_PLL);
	/* FIXME - eventually handled internally */
	rcc_ahb_frequency = 80e6;
	rcc_apb1_frequency = 80e6;
	rcc_apb2_frequency = 80e6;
}

static void usart_setup(void)
{
	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5|GPIO6);

	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOD, GPIO_AF7, GPIO5);

	usart_set_baudrate(USART_CONSOLE, 115200);
	usart_set_databits(USART_CONSOLE, 8);
	usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
	usart_set_mode(USART_CONSOLE, USART_MODE_TX);
	usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
	usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART_CONSOLE);
}

/**
 * Use USART_CONSOLE as a console.
 * This is a syscall for newlib
 * @param file
 * @param ptr
 * @param len
 * @return
 */
int _write(int file, char *ptr, int len)
{
	int i;

	if (file == STDOUT_FILENO || file == STDERR_FILENO) {
		for (i = 0; i < len; i++) {
			if (ptr[i] == '\n') {
				usart_send_blocking(USART_CONSOLE, '\r');
			}
			usart_send_blocking(USART_CONSOLE, ptr[i]);
		}
		return i;
	}
	errno = EIO;
	return -1;
}

void exti0_isr(void)
{
        exti_reset_request(JOYC_EXTI);
        if (state.falling) {
                gpio_clear(LED_RED_PORT, LED_RED_PIN);
                state.falling = false;
		state.last_hold = TIM_CNT(TIM7);
                exti_set_trigger(JOYC_EXTI, EXTI_TRIGGER_RISING);
        } else {
		/* pushed */
                gpio_set(LED_RED_PORT, LED_RED_PIN);
                state.falling = true;
                TIM_CNT(TIM7) = 0;
		state.last_hold = 0;
                exti_set_trigger(JOYC_EXTI, EXTI_TRIGGER_FALLING);
        }
}

/*
 * Free running ms timer.
 */
static void setup_tim7(void)
{
        rcc_periph_reset_pulse(RST_TIM7);
        timer_set_prescaler(TIM7, rcc_apb1_frequency / 1e3 - 1);
        timer_set_period(TIM7, 0xffff);
        timer_enable_counter(TIM7);
}


int main(void)
{
	int j = 0;
	clock_setup();
	usart_setup();
	printf("hi guys!\n");

	/* green led for ticking */
	gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_GREEN_PIN);
	/* red led for buttons */
	gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			LED_RED_PIN);

	setup_tim7();
	gpio_mode_setup(JOY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, JOYC_PIN);
        nvic_enable_irq(JOYC_NVIC);
	exti_select_source(JOYC_EXTI, JOY_PORT);
	state.falling = false;
	exti_set_trigger(JOYC_EXTI, EXTI_TRIGGER_RISING);
	exti_enable_request(JOYC_EXTI);
	
	while (1) {
		printf("tick: %d\n", j++);
		if (state.last_hold) {
			printf("button was held for %d ms\n", state.last_hold);
			state.last_hold = 0;
		}
		gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);

		for (int i = 0; i < 4000000; i++) { /* Wait a bit. */
			__asm__("NOP");
		}
	}

	return 0;
}
