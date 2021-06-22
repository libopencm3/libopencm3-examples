/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Modified by Fernando Cortes <fermando.corcam@gmail.com>
 * modified by Guillermo Rivera <memogrg@gmail.com>
 * Copyright (C) 2016 Josh Myer <josh@joshisanerd.com>
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
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>


/* This assumes you've put LEDs on the Arduino header, D13~D6.
 *
 * Also, put an analog input into the Arduino A0 pin.
 */

/* Arduino pin mapping */
#define D13 GPIOA, GPIO5
#define D12 GPIOA, GPIO6
#define D11 GPIOA, GPIO7
#define D10 GPIOB, GPIO6
#define  D9 GPIOC, GPIO7
#define  D8 GPIOA, GPIO9
#define  D7 GPIOA, GPIO8
#define  D6 GPIOB, GPIO10

/* Other arduino pins that may matter:
 *  D1 - PA2 (Serial TX, nucleo -> PC)
 *  D0 - PA3 (Serial RX, PC -> nucleo)
 *  A0 - PA0
 */

/* Final port masks for GPIO:
 * PA: 5-9
 * PB: 6,10
 * PC: 7
 */

#define MASKA GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9
#define MASKB GPIO6 | GPIO10
#define MASKC GPIO7


static void adc_setup(void)
{
	uint8_t channel_array[16];
	uint32_t i;

	rcc_periph_clock_enable(RCC_ADC12);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
	adc_off(ADC1);
	adc_set_clk_prescale(ADC_CCR_CKMODE_DIV2);
	adc_set_single_conversion_mode(ADC1);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);

	/* We want to read the temperature sensor, so we have to enable it. */
	adc_enable_temperature_sensor();

	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR1_SMP_61DOT5CYC);


	/* Set up an array of channels to sample from, all ADC1/A0 */
	for (i = 0; i < 16; i++) {
		channel_array[i] = 1;
	}
	channel_array[0] = 1; /* ADC1_IN1 (PA0) */

	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_set_resolution(ADC1, ADC_CFGR_RES_12_BIT);
	adc_power_on(ADC1);

	/* Wait for ADC starting up. */
	for (i = 0; i < 800000; i++) {
		__asm__("nop");
	}

}

static void usart_setup(void)
{
	/* Enable clocks for GPIO port A (for GPIO_USART2_TX) and USART2. */
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Setup GPIO pin GPIO_USART2_TX/GPIO9 on GPIO port A for transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

	/* Setup UART parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKA);

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKB);

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, MASKC);
}

static void my_usart_print_int(uint32_t usart, int16_t value)
{
	int8_t i;
	int8_t nr_digits = 0;
	char buffer[25];

	if (value < 0) {
		usart_send_blocking(usart, '-');
		value = value * -1;
	}

	if (value == 0) {
		usart_send_blocking(usart, '0');
	}

	while (value > 0) {
		buffer[nr_digits++] = "0123456789"[value % 10];
		value /= 10;
	}

	for (i = nr_digits-1; i >= 0; i--) {
		usart_send_blocking(usart, buffer[i]);
	}

	usart_send_blocking(usart, '\r');
	usart_send_blocking(usart, '\n');
}

static void my_usart_print_str(uint32_t usart, char *s)
{
	while (*s) {
		usart_send_blocking(usart, *s);
		s++;
	}
}

static void clock_setup(void)
{
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_64MHZ]);
}

static void blit_fill(uint8_t reading)
{
	int i;
	int max_bit = 0;

	for (i = 0; i < 8; i++) {
		if (reading > 1<<i) {
			max_bit = i;
		}
	}

	/* Turn all LEDs off */
	gpio_clear(GPIOA, MASKA);
	gpio_clear(GPIOB, MASKB);
	gpio_clear(GPIOC, MASKC);

	/* Turn on all LEDs below our level */
	switch (max_bit) {
	case 7: gpio_set(D13);
	case 6: gpio_set(D12);
	case 5: gpio_set(D11);
	case 4: gpio_set(D10);
	case 3: gpio_set( D9);
	case 2: gpio_set( D8);
	case 1: gpio_set( D7);
	case 0: gpio_set( D6);
	}

}

int main(void)
{
	volatile uint16_t temp;

	clock_setup();
	gpio_setup();
	adc_setup();
	usart_setup();

	my_usart_print_str(USART2, "Starting up...\r\n");

	while (1) {
		adc_start_conversion_regular(ADC1);
		while (!(adc_eoc(ADC1)));
		temp = adc_read_regular(ADC1);
		blit_fill(temp >> 4);
		my_usart_print_int(USART2, temp);
	}

	return 0;
}

