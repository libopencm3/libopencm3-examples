/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 * Modified 2018 by Jiri Bilek <jiri@bilek.info>
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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#define USART_TX	GPIO2
#define I2C_MEMORY_ADDR  0x50

static void systick_setup(int xms);
static void clock_setup(void);
void delay(uint32_t time);
static void usart_setup(void);
static void i2c_setup(void);
static void print(const char *s);

volatile uint32_t millis = 0L;  // millis since start of the program

/*
 * Called when systick fires
 */
void sys_tick_handler(void)
{
	++millis;
}

/*
 * Set up timer to fire every x milliseconds
 */
static void systick_setup(int xms)
{
	systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
	STK_CVR = 0;

	systick_set_reload((rcc_ahb_frequency / 8 / 1000 * xms) - 1);
	systick_counter_enable();
	systick_interrupt_enable();
}

/*
 * Set STM32 to clock by 48 MHz from HSI oscillator
 */
static void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();
}

/*
 *  Delay in milliseconds
 */
void delay(uint32_t time)
{
	uint32_t old_millis = millis;

	while (millis - old_millis < time)
		;
}

/*
 * Set up USART1: TX on USART_TX(PA2 - alternate pin), RX not used
 * 115200 baud, 8N1, no flow control
 */
static void usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX);
	gpio_set_af(GPIOA, GPIO_AF1, USART_TX);

	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	usart_enable(USART1);
}

/*
 * Set up I2C1 on PA9, PA10.
 * Both pins are internally pulled up, not necessary when external pullup is used.
 */
static void i2c_setup(void)
{
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_set_i2c_clock_hsi(I2C1);

	i2c_reset(I2C1);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO9 | GPIO10);
	i2c_peripheral_disable(I2C1);
	//configure ANFOFF DNF[3:0] in CR1
	i2c_enable_analog_filter(I2C1);
	i2c_set_digital_filter(I2C1, 0);
	/* HSI is at 8Mhz */
	i2c_set_speed(I2C1, i2c_speed_sm_100k, 8);
	//configure No-Stretch CR1 (only relevant in slave mode)
	i2c_enable_stretching(I2C1);
	//addressing mode
	i2c_set_7bit_addr_mode(I2C1);

	i2c_peripheral_enable(I2C1);
}

static void print(const char *s)
{
	while (*s)
		usart_send_blocking(USART1, *s++);
}

static void printhex(uint8_t h)
{
	h &= 0xf;
	if (h > 9)
		h += 'A' - '9' - 1;

	usart_send_blocking(USART1, h + '0');
}

static void print2hex(uint8_t h)
{
	printhex(h >> 4);
	printhex(h & 0xf);
}

int main(void)
{
	clock_setup();
	usart_setup();
	i2c_setup();
	systick_setup(1);  // 1 ms tick

	// write 0x42, 0x75 into address 0x01
	uint8_t cmdWrite[] = { 0x01, 0x42, 0x75 };
	i2c_transfer7(I2C1, I2C_MEMORY_ADDR, cmdWrite, sizeof(cmdWrite), NULL, 0);

	delay(5);  // needed for finishing the write (see Datasheet Atmel 24c02)

	// read 2 bytes from address 0x01
	uint8_t cmdRead[] = { 0x01 };
	uint8_t dataOut[2];
	i2c_transfer7(I2C1, I2C_MEMORY_ADDR, cmdRead, sizeof(cmdRead), dataOut, sizeof(dataOut));

	print("Addr 0x01, value = ");
	print2hex(dataOut[0]);
	print(" ");
	print2hex(dataOut[1]);
	print("\n");

	/* No Main loop */
	return 0;
}
