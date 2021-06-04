/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018 Roel Jordans <r.jordans@tue.nl>
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
#include <libopencm3/stm32/i2c.h>

/* Wait a bit */
static void delay(int n)
{
	for (int i = 0; i < n; i++) {
		__asm__("nop");
	}
}

/* Setup I2C1 interface */
static void i2c_setup(void)
{
	/* Enable GPIOA and I2C1 clocks. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_I2C1);
	i2c_reset(I2C1);

	/* Set I2C mode with external pullups */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO9 | GPIO10);
	i2c_peripheral_disable(I2C1);

	/* Configure ANFOFF DNF[3:0] in CR1 */
	i2c_enable_analog_filter(I2C1);
	i2c_set_digital_filter(I2C1, 0);

	/* Configure I2C bus speed, uses 8MHz HSI clock internally */
	i2c_set_speed(I2C1, i2c_speed_sm_100k, 8);

	/* Configure No-Stretch CR1 (only relevant in slave mode) */
	i2c_enable_stretching(I2C1);

	/* Addressing mode */
	i2c_set_7bit_addr_mode(I2C1);
	i2c_peripheral_enable(I2C1);
}

/* Set DAC output register value */
static void mcp4725_set_value(uint32_t i2c, uint8_t addr, uint32_t val)
{
	/* Command package, 2 bytes with last 12 bits the value, high bits 0 */
	uint8_t command[] = { (val >> 8) & 0x0f, val & 0xff};

	/* I2C transfer data */
	i2c_transfer7(i2c, addr, command, sizeof(command), NULL, 0);
}

int main(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();

	i2c_setup();

	/* Write sawtooth shape to DAC output */
	int val = 0;
	while (1) {
		/* Set value */
		mcp4725_set_value(I2C1, 0x62, val++);

		/* Wrap-around to keep within 12 bit resolution */
		val &= 0xfff;

		/* Wait a bit */
		delay(4000);
	}

	return 0;
}
