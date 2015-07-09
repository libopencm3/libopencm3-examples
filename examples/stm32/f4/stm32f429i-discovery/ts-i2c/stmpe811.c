/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 brabo <brabo.sil@gmail.com>
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

#include <stdint.h>
#include <stdio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "stmpe811.h"
#include "clock.h"

/* Private defines */
/* I2C address */
#define STMPE811_ADDRESS			0x41

/* STMPE811 Chip ID on reset */
#define STMPE811_CHIP_ID_VALUE			0x0811	/* Chip ID */

/* Registers */
#define STMPE811_CHIP_ID	0x00	/* STMPE811 Device identification */
#define STMPE811_ID_VER		0x02	/* STMPE811 Revision number */
#define STMPE811_SYS_CTRL1	0x03	/* Reset control */
#define STMPE811_SYS_CTRL2	0x04	/* Clock control */
#define STMPE811_SPI_CFG	0x08	/* SPI interface configuration */
#define STMPE811_INT_CTRL	0x09	/* Interrupt control register */
#define STMPE811_INT_EN		0x0A	/* Interrupt enable register */
#define STMPE811_INT_STA	0x0B	/* Interrupt status register */
#define STMPE811_GPIO_EN	0x0C	/* GPIO interrupt enable register */
#define STMPE811_GPIO_INT_STA	0x0D	/* GPIO interrupt status register */
#define STMPE811_ADC_INT_EN	0x0E	/* ADC interrupt enable register */
#define STMPE811_ADC_INT_STA	0x0F	/* ADC interface status register */
#define STMPE811_GPIO_SET_PIN	0x10	/* GPIO set pin register */
#define STMPE811_GPIO_CLR_PIN	0x11	/* GPIO clear pin register */
#define STMPE811_MP_STA		0x12	/* GPIO monitor pin state register */
#define STMPE811_GPIO_DIR	0x13	/* GPIO direction register */
#define STMPE811_GPIO_ED	0x14	/* GPIO edge detect register */
#define STMPE811_GPIO_RE	0x15	/* GPIO rising edge register */
#define STMPE811_GPIO_FE	0x16	/* GPIO falling edge register */
#define STMPE811_GPIO_AF	0x17	/* Alternate function register */
#define STMPE811_ADC_CTRL1	0x20	/* ADC control */
#define STMPE811_ADC_CTRL2	0x21	/* ADC control */
#define STMPE811_ADC_CAPT	0x22	/* To initiate ADC data acquisition */
#define STMPE811_ADC_DATA_CHO	0x30	/* ADC channel 0 */
#define STMPE811_ADC_DATA_CH1	0x32	/* ADC channel 1 */
#define STMPE811_ADC_DATA_CH2	0x34	/* ADC channel 2 */
#define STMPE811_ADC_DATA_CH3	0x36	/* ADC channel 3 */
#define STMPE811_ADC_DATA_CH4	0x38	/* ADC channel 4 */
#define STMPE811_ADC_DATA_CH5	0x3A	/* ADC channel 5 */
#define STMPE811_ADC_DATA_CH6	0x3C	/* ADC channel 6 */
#define STMPE811_ADC_DATA_CH7	0x3E	/* ADC channel 7 */
#define STMPE811_TSC_CTRL	0x40	/* 4-wire tsc setup */
#define STMPE811_TSC_CFG	0x41	/* Tsc configuration */
#define STMPE811_WDW_TR_X	0x42	/* Window setup for top right X */
#define STMPE811_WDW_TR_Y	0x44	/* Window setup for top right Y */
#define STMPE811_WDW_BL_X	0x46	/* Window setup for bottom left X */
#define STMPE811_WDW_BL_Y	0x48	/* Window setup for bottom left Y */
#define STMPE811_FIFO_TH	0x4A	/* FIFO level to generate interrupt */
#define STMPE811_FIFO_STA	0x4B	/* Current status of FIFO */
#define STMPE811_FIFO_SIZE	0x4C	/* Current filled level of FIFO */
#define STMPE811_TSC_DATA_X	0x4D	/* Data port for tsc data access */
#define STMPE811_TSC_DATA_Y	0x4F	/* Data port for tsc data access */
#define STMPE811_TSC_DATA_Z	0x51	/* Data port for tsc data access */
#define STMPE811_TSC_DATA_XYZ	0x52	/* Data port for tsc data access */
#define STMPE811_TSC_FRACTION_Z	0x56	/* Touchscreen controller FRACTION_Z */
#define STMPE811_TSC_DATA	0x57	/* Data port for tsc data access */
#define STMPE811_TSC_I_DRIVE	0x58	/* Touchscreen controller drivel */
#define STMPE811_TSC_SHIELD	0x59	/* Touchscreen controller shield */
#define STMPE811_TEMP_CTRL	0x60	/* Temperature sensor setup */
#define STMPE811_TEMP_DATA	0x61	/* Temperature data access port */
#define STMPE811_TEMP_TH	0x62	/* Threshold for temp controlled int */


#define I2C_CR2_FREQ_MASK	0x3ff

#define I2C_CCR_CCRMASK	0xfff

#define I2C_TRISE_MASK	0x3f

/**
 * @brief  Default I2C used, on STM32F429-Discovery board
 */
#define STMPE811_I2C					I2C3

/**
 * @brief  Default I2C clock for STMPE811
 */
#ifndef STMPE811_I2C_CLOCK
#define STMPE811_I2C_CLOCK				100000
#endif

static void stmpe811_i2c_init(void);
static uint8_t i2c_start(uint32_t i2c, uint8_t address, uint8_t mode);
static uint8_t i2c_write(uint32_t i2c, uint8_t address, uint8_t reg,
	uint8_t data);
static uint32_t i2c_read(uint32_t i2c, uint8_t address, uint8_t reg);
static void stmpe811_write(uint8_t reg, uint8_t data);
static uint32_t stmpe811_read(uint8_t reg);
static void stmpe811_reset(void);
static void stmpe811_disable_ts(void);
static void stmpe811_disable_gpio(void);
static void stmpe811_enable_fifo_of(void);
static void stmpe811_enable_fifo_th(void);
static void stmpe811_enable_fifo_touch_det(void);
static void stmpe811_set_adc_sample(uint8_t sample);
static void stmpe811_set_adc_resolution(uint8_t res);
static void stmpe811_set_adc_freq(uint8_t freq);
static void stmpe811_set_gpio_af(uint8_t af);
static void stmpe811_set_tsc(uint8_t tsc);
static void stmpe811_set_fifo_th(uint8_t th);
static void stmpe811_reset_fifo(void);
static void stmpe811_set_tsc_fraction_z(uint8_t z);
static void stmpe811_set_tsc_i_drive(uint8_t limit);
static void stmpe811_enable_tsc(void);
static void stmpe811_set_int_sta(uint8_t status);
static void stmpe811_enable_interrupts(void);


static void stmpe811_i2c_init()
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_I2C3);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ,
		GPIO8);
	gpio_set_af(GPIOA, GPIO_AF4, GPIO8);

	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ,
		GPIO9);
	gpio_set_af(GPIOC, GPIO_AF4, GPIO9);

	i2c_peripheral_disable(I2C3); /* disable i2c during setup */
	i2c_reset(I2C3);

	i2c_set_fast_mode(I2C3);
	i2c_set_clock_frequency(I2C3, I2C_CR2_FREQ_42MHZ);
	i2c_set_ccr(I2C3, 35);
	i2c_set_trise(I2C3, 43);

	i2c_peripheral_enable(I2C3); /* finally enable i2c */

	i2c_set_own_7bit_slave_address(I2C3, 0x00);
}

static uint8_t i2c_start(uint32_t i2c, uint8_t address, uint8_t mode)
{
	i2c_send_start(i2c);

	/* Wait for master mode selected */
	while (!((I2C_SR1(i2c) & I2C_SR1_SB)
		& (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

	i2c_send_7bit_address(i2c, address, mode);

	/* Waiting for address is transferred. */
	while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));

	/* Cleaning ADDR condition sequence. */
	uint32_t reg32 = I2C_SR2(i2c);
	(void) reg32; /* unused */

	return 0;
}

static uint8_t i2c_write(uint32_t i2c, uint8_t address, uint8_t reg,
	uint8_t data)
{
	i2c_start(i2c, address, I2C_WRITE);

	i2c_send_data(i2c, reg);

	while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));
	i2c_send_data(i2c, data);

	while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));

	i2c_send_stop(i2c);

	return 0;
}

static uint32_t i2c_read(uint32_t i2c, uint8_t address, uint8_t reg)
{
	while ((I2C_SR2(i2c) & I2C_SR2_BUSY));

	i2c_start(i2c, address, I2C_WRITE);
	i2c_send_data(i2c, reg);

	while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));

	i2c_start(i2c, address, I2C_READ);

	i2c_send_stop(i2c);

	while (!(I2C_SR1(i2c) & I2C_SR1_RxNE));

	uint32_t result = i2c_get_data(i2c);

	I2C_SR1(i2c) &= ~I2C_SR1_AF;

	return result;
}

static void stmpe811_write(uint8_t reg, uint8_t data)
{
	i2c_write(I2C3, STMPE811_ADDRESS, reg, data);
}

static uint32_t stmpe811_read(uint8_t reg)
{
	uint32_t data;
	data = i2c_read(I2C3, STMPE811_ADDRESS, reg);
	return data;
}

static void stmpe811_reset(void)
{
	uint8_t data;

	data = i2c_read(I2C3, STMPE811_ADDRESS, STMPE811_SYS_CTRL1);

	data |= 0x02;

	i2c_write(I2C3, STMPE811_ADDRESS, STMPE811_SYS_CTRL1, data);

	data &= ~(0x02);

	i2c_write(I2C3, STMPE811_ADDRESS, STMPE811_SYS_CTRL1, data);
	msleep(2);
}

static void stmpe811_disable_ts(void)
{
	stmpe811_write(STMPE811_SYS_CTRL2, 0x08);
}

static void stmpe811_disable_gpio(void)
{
	stmpe811_write(STMPE811_SYS_CTRL2, 0x04);
}

static void stmpe811_enable_fifo_of(void)
{
	uint8_t mode;
	mode = stmpe811_read(STMPE811_INT_EN);
	mode |= 0x04;
	stmpe811_write(STMPE811_INT_EN, mode);
}

static void stmpe811_enable_fifo_th(void)
{
	uint8_t mode;
	mode = stmpe811_read(STMPE811_INT_EN);
	mode |= 0x02;
	stmpe811_write(STMPE811_INT_EN, mode);
}

static void stmpe811_enable_fifo_touch_det(void)
{
	uint8_t mode;
	mode = stmpe811_read(STMPE811_INT_EN);
	mode |= 0x01;
	stmpe811_write(STMPE811_INT_EN, mode);
}

static void stmpe811_set_adc_sample(uint8_t sample)
{
	stmpe811_write(STMPE811_ADC_CTRL1, sample);
}

static void stmpe811_set_adc_resolution(uint8_t res)
{
	stmpe811_write(STMPE811_ADC_CTRL1, res);
}

static void stmpe811_set_adc_freq(uint8_t freq)
{
	stmpe811_write(STMPE811_ADC_CTRL2, freq);
}

static void stmpe811_set_gpio_af(uint8_t af)
{
	stmpe811_write(STMPE811_GPIO_AF, af);
}

static void stmpe811_set_tsc(uint8_t tsc)
{
	stmpe811_write(STMPE811_TSC_CFG, tsc);
}

static void stmpe811_set_fifo_th(uint8_t th)
{
	stmpe811_write(STMPE811_FIFO_TH, th);
}

static void stmpe811_reset_fifo(void)
{
	uint8_t sta;
	sta = stmpe811_read(STMPE811_FIFO_STA);
	sta |= 0x01;
	stmpe811_write(STMPE811_FIFO_STA, sta);
	sta &= ~(0x01);
	stmpe811_write(STMPE811_FIFO_STA, sta);
}

static void stmpe811_set_tsc_fraction_z(uint8_t z)
{
	stmpe811_write(STMPE811_TSC_FRACTION_Z, z);
}

static void stmpe811_set_tsc_i_drive(uint8_t limit)
{
	stmpe811_write(STMPE811_TSC_I_DRIVE, limit);
}

static void stmpe811_enable_tsc(void)
{
	uint8_t mode;
	mode = stmpe811_read(STMPE811_TSC_CTRL);
	mode |= 0x01;
	stmpe811_write(STMPE811_TSC_CTRL, mode);
}

static void stmpe811_set_int_sta(uint8_t status)
{
	stmpe811_write(STMPE811_INT_STA, status);
}

static void stmpe811_enable_interrupts(void)
{
	uint8_t mode;
	mode = stmpe811_read(STMPE811_INT_CTRL);
	mode |= 0x01;
	stmpe811_write(STMPE811_INT_CTRL, mode);
}

stmpe811_state_t stmpe811_init(void)
{
	stmpe811_i2c_init();
	msleep(50);

	stmpe811_reset();

	/* start of the documented initialization sequence */
	stmpe811_disable_ts();
	stmpe811_disable_gpio();

	stmpe811_enable_fifo_of();
	stmpe811_enable_fifo_th();
	stmpe811_enable_fifo_touch_det();

	stmpe811_set_adc_sample(0x40);     /* set to 80 cycles */
	stmpe811_set_adc_resolution(0x08); /* set adc resolution to 12 bit */

	stmpe811_set_adc_freq(0x01);	/* set adc clock speed to 3.25 MHz */

	stmpe811_set_gpio_af(0x00);	/* set GPIO AF to function as ts/adc */

	/*
	 * tsc cfg set to avg control = 4 samples
	 * touch detection delay = 500 microseconds
	 * panel driver settling time = 500 microcseconds
	 */
	stmpe811_set_tsc(0x9A);

	stmpe811_set_fifo_th(0x01);        /* set fifo threshold */
	stmpe811_reset_fifo();             /* reset fifo */

	/* set fractional part to 7, whole part to 1 */
	stmpe811_set_tsc_fraction_z(0x07);

	/* set current limit value to 50 mA */
	stmpe811_set_tsc_i_drive(0x01);

	stmpe811_enable_tsc();             /* enable tsc (touchscreen clock) */

	stmpe811_set_int_sta(0xFF);        /* clear interrupt status */

	stmpe811_enable_interrupts();
	/* end of the documented initialization sequence */

	msleep(2);

	return stmpe811_state_ok;
}

uint16_t stmpe811_read_x(uint16_t x)
{
	uint8_t data[2];
	int16_t val, dx;
	data[1] = stmpe811_read(STMPE811_TSC_DATA_X);
	data[0] = stmpe811_read(STMPE811_TSC_DATA_X + 1);
	val = (data[1] << 8 | (data[0] & 0xFF));

	if (val <= 3000) {
		val = 3900 - val;
	} else {
		val = 3800 - val;
	}

	val /= 15;

	if (val > 239) {
		val = 239;
	} else if (val < 0) {
		val = 0;
	}

	dx = (val > x) ? (val - x) : (x - val);
	if (dx > 4) {
		return val;
	}
	return x;
}

uint16_t stmpe811_read_y(uint16_t y)
{
	uint8_t data[2];
	int16_t val, dy;
	data[1] = stmpe811_read(STMPE811_TSC_DATA_Y);
	data[0] = stmpe811_read(STMPE811_TSC_DATA_Y + 1);
	val = (data[1] << 8 | (data[0] & 0xFF));

	val -= 360;
	val = val / 11;

	if (val <= 0) {
		val = 0;
	} else if (val >= 320) {
		val = 319;
	}

	dy = (val > y) ? (val - y) : (y - val);
	if (dy > 4) {
		return val;
	}
	return y;
}

stmpe811_state_t stmpe811_read_touch(stmpe811_t *stmpe811_data)
{
	uint8_t val;

	stmpe811_data->last_pressed = stmpe811_data->pressed;

	val = stmpe811_read(STMPE811_TSC_CTRL);

	if ((val & 0x80) == 0) {
		/* No touch detected, reset fifo and return released state */
		stmpe811_data->pressed = stmpe811_state_released;

		stmpe811_reset_fifo();

		return stmpe811_state_released;
	}

	/* Touch detected, reset fifo and return pressed state */
	stmpe811_reset_fifo();

	stmpe811_data->pressed = stmpe811_state_pressed;
	return stmpe811_state_pressed;
}
