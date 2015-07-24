/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Karl Palsson <karlp@tweak.net.au>
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
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>

#define LED_DISCOVERY_USER_PORT	GPIOC
#define LED_DISCOVERY_USER_PIN	GPIO8

#define USART_CONSOLE USART2

int _write(int file, char *ptr, int len);

static void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_24mhz();
	/* Enable clocks for USART2 and DAC*/
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_DAC);

	/* and the ADC and IO ports */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_ADC1);
}

static void usart_setup(void)
{
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);

	usart_set_baudrate(USART_CONSOLE, 115200);
	usart_set_databits(USART_CONSOLE, 8);
	usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
	usart_set_mode(USART_CONSOLE, USART_MODE_TX);
	usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
	usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART_CONSOLE);
}

/*
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

static void adc_setup(void)
{
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);

	/* Make sure the ADC doesn't run during config. */
	adc_off(ADC1);

	/* We configure everything for one single conversion. */
	adc_disable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);

	adc_power_on(ADC1);

	/* Wait for ADC starting up. */
	int i;
	for (i = 0; i < 800000; i++) {	/* Wait a bit. */
		__asm__("nop");
	}

	adc_reset_calibration(ADC1);
	adc_calibration(ADC1);
}

static void dac_setup(void)
{
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5);
	dac_disable(CHANNEL_2);
	dac_disable_waveform_generation(CHANNEL_2);
	dac_enable(CHANNEL_2);
	dac_set_trigger_source(DAC_CR_TSEL2_SW);
}

static uint16_t read_adc_naiive(uint8_t channel)
{
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_direct(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}

int main(void)
{
	int i;
	int j = 0;
	clock_setup();
	usart_setup();
	printf("hi guys!\n");
	adc_setup();
	dac_setup();
	gpio_set_mode(LED_DISCOVERY_USER_PORT, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, LED_DISCOVERY_USER_PIN);

	while (1) {
		uint16_t input_adc0 = read_adc_naiive(0);
		uint16_t target = input_adc0 / 2;
		dac_load_data_buffer_single(target, RIGHT12, CHANNEL_2);
		dac_software_trigger(CHANNEL_2);
		uint16_t input_adc1 = read_adc_naiive(1);
		printf("tick: %d: adc0= %u, target adc1=%d, adc1=%d\n",
			j++, input_adc0, target, input_adc1);
		/* LED on/off */
		gpio_toggle(LED_DISCOVERY_USER_PORT, LED_DISCOVERY_USER_PIN);
		for (i = 0; i < 1000000; i++) {	/* Wait a bit. */
			__asm__("NOP");
		}
	}

	return 0;
}
