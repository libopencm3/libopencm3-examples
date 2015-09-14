/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Marco Russi <marco@marcorussi.net>
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


/* ---------------- Inclusions ----------------- */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>




/* ---------------- Local Defines ----------------- */

/* LIS3DSH registers addresses */
#define ADD_REG_WHO_AM_I				0x0F
#define ADD_REG_CTRL_4					0x20
#define ADD_REG_OUT_X_L					0x28
#define ADD_REG_OUT_X_H					0x29
#define ADD_REG_OUT_Y_L					0x2A
#define ADD_REG_OUT_Y_H					0x2B
#define ADD_REG_OUT_Z_L					0x2C
#define ADD_REG_OUT_Z_H					0x2D

/* WHO AM I register default value */
#define UC_WHO_AM_I_DEFAULT_VALUE		0x3F

/* ADD_REG_CTRL_4 register configuration value:
 * X,Y,Z axis enabled and 400Hz of output data rate */
#define UC_ADD_REG_CTRL_4_CFG_VALUE		0x77

/* Sensitivity for 2G range [mg/digit] */
#define SENS_2G_RANGE_MG_PER_DIGIT		((float)0.06)

/* LED threshold value in mg */
#define LED_TH_MG				(1000)	/* 1000mg (1G) */




/* ---------------- Local Macros ----------------- */

/* set read single command. Attention: command must be 0x3F at most */
#define SET_READ_SINGLE_CMD(x)			(x | 0x80)
/* set read multiple command. Attention: command must be 0x3F at most */
#define SET_READ_MULTI_CMD(x)			(x | 0xC0)
/* set write single command. Attention: command must be 0x3F at most */
#define SET_WRITE_SINGLE_CMD(x)			(x & (~(0xC0)))
/* set write multiple command. Attention: command must be 0x3F at most */
#define SET_WRITE_MULTI_CMD(x)			(x & (~(0x80))	\
						 x |= 0x40)

/* Macros for turning LEDs ON */
#define LED_GREEN_ON()				(gpio_set(GPIOD, GPIO12))
#define LED_ORANGE_ON()				(gpio_set(GPIOD, GPIO13))
#define LED_RED_ON()				(gpio_set(GPIOD, GPIO14))
#define LED_BLUE_ON()				(gpio_set(GPIOD, GPIO15))

/* Macros for turning LEDs OFF */
#define LED_GREEN_OFF()				(gpio_clear(GPIOD, GPIO12))
#define LED_ORANGE_OFF()			(gpio_clear(GPIOD, GPIO13))
#define LED_RED_OFF()				(gpio_clear(GPIOD, GPIO14))
#define LED_BLUE_OFF()				(gpio_clear(GPIOD, GPIO15))




/* ------------- Local functions prototypes --------------- */

static void gpio_setup(void);
static void spi_setup(void);
static void lis3dsh_init(void);
static void lis3dsh_write_reg(int, int);
static int lis3dsh_read_reg(int);
static inline int two_compl_to_int16(int);




/* ------------ Local functions implementation ----------- */

/* Function to setup all used GPIOs */
static void gpio_setup(void)
{
	/* Enable GPIOD clock. */
	rcc_periph_clock_enable(RCC_GPIOD);

	/* Set GPIO12/13/14/15 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			GPIO12 | GPIO13 | GPIO14 | GPIO15);

	/* LIS3DSH pins map:
	PA5 - SPI1_SCK
	PA6 - SPI1_MISO
	PA7 - SPI1_MOSI
	PE3 - CS_SPI
	*/

	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);
	/* set SPI pins as CLK, MOSI, MISO */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
			GPIO5 | GPIO6 | GPIO7);
	/* Push Pull, Speed 100 MHz */
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ,
				GPIO5 | GPIO6 | GPIO7);
	/* Alternate Function: SPI1 */
	gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);

	/* Enable GPIOE clock. */
	rcc_periph_clock_enable(RCC_GPIOE);
	/* set CS as OUTPUT */
	gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
	/* Push Pull, Speed 100 MHz */
	gpio_set_output_options(GPIOE, GPIO_OTYPE_PP,
				GPIO_OSPEED_100MHZ, GPIO3);
	/* set CS high */
	gpio_set(GPIOE, GPIO3);
}


/* Function to setup the SPI1 */
static void spi_setup(void)
{
	/* Enable SPI1 clock. */
	rcc_periph_clock_enable(RCC_SPI1);

	/* reset SPI1 */
	spi_reset(SPI1);
	/* init SPI1 master */
	spi_init_master(SPI1,
					SPI_CR1_BAUDRATE_FPCLK_DIV_64,
					SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
					SPI_CR1_CPHA_CLK_TRANSITION_1,
					SPI_CR1_DFF_8BIT,
					SPI_CR1_MSBFIRST);
	/* enable SPI1 first */
	spi_enable(SPI1);
}


/* Function to initialise the LIS3DSH */
static void lis3dsh_init(void)
{
	int int_reg_value;

	/* init SPI1 */
	spi_setup();

	/* get WHO AM I value */
	int_reg_value = lis3dsh_read_reg(ADD_REG_WHO_AM_I);

	/* if WHO AM I value is the expected one */
	if (int_reg_value == UC_WHO_AM_I_DEFAULT_VALUE) {
		/* set output data rate to 400 Hz and enable X,Y,Z axis */
		lis3dsh_write_reg(ADD_REG_CTRL_4, UC_ADD_REG_CTRL_4_CFG_VALUE);
		/* verify written value */
		int_reg_value = lis3dsh_read_reg(ADD_REG_CTRL_4);
		/* if written value is different */
		if (int_reg_value != UC_ADD_REG_CTRL_4_CFG_VALUE) {
			/* ERROR: stay here... */
			while (1);
		}
	} else {
		/* ERROR: stay here... */
		while (1);
	}
}


/* Function to write a register to LIS3DSH through SPI  */
static void lis3dsh_write_reg(int reg, int data)
{
	/* set CS low */
	gpio_clear(GPIOE, GPIO3);
	/* discard returned value */
	spi_xfer(SPI1, SET_WRITE_SINGLE_CMD(reg));
	spi_xfer(SPI1, data);
	/* set CS high */
	gpio_set(GPIOE, GPIO3);
}


/* Function to read a register from LIS3DSH through SPI */
static int lis3dsh_read_reg(int reg)
{
	int reg_value;
	/* set CS low */
	gpio_clear(GPIOE, GPIO3);
	reg_value = spi_xfer(SPI1, SET_READ_SINGLE_CMD(reg));
	reg_value = spi_xfer(SPI1, 0xFF);
	/* set CS high */
	gpio_set(GPIOE, GPIO3);

	return reg_value;
}


/* Transform a two's complement value to 16-bit int value */
static inline int two_compl_to_int16(int two_compl_value)
{
	int int16_value = 0;

	/* conversion */
	if (two_compl_value > 32768) {
		int16_value = -(((~two_compl_value) & 0xFFFF) + 1);
	} else {
		int16_value = two_compl_value;
	}

	return int16_value;
}


/* Main function */
int main(void)
{
	int value_mg_x, value_mg_y, value_mg_z;

	/* setup all GPIOs */
	gpio_setup();

	/* initialise LIS3DSH */
	lis3dsh_init();

	/* infinite loop */
	while (1) {
		/* get X, Y, Z values */
		value_mg_x = ((lis3dsh_read_reg(ADD_REG_OUT_X_H) << 8) |
				lis3dsh_read_reg(ADD_REG_OUT_X_L));
		value_mg_y = ((lis3dsh_read_reg(ADD_REG_OUT_Y_H) << 8) |
				lis3dsh_read_reg(ADD_REG_OUT_Y_L));
		value_mg_z = ((lis3dsh_read_reg(ADD_REG_OUT_Z_H) << 8) |
				lis3dsh_read_reg(ADD_REG_OUT_Z_L));

		/* transform X value from two's complement to 16-bit int */
		value_mg_x = two_compl_to_int16(value_mg_x);
		/* convert X absolute value to mg value */
		value_mg_x = value_mg_x * SENS_2G_RANGE_MG_PER_DIGIT;

		/* transform Y value from two's complement to 16-bit int */
		value_mg_y = two_compl_to_int16(value_mg_y);
		/* convert Y absolute value to mg value */
		value_mg_y = value_mg_y * SENS_2G_RANGE_MG_PER_DIGIT;

		/* transform Z value from two's complement to 16-bit int */
		value_mg_z = two_compl_to_int16(value_mg_z);
		/* convert Z absolute value to mg value */
		value_mg_z = value_mg_z * SENS_2G_RANGE_MG_PER_DIGIT;

		/* set X related LEDs according to specified threshold */
		if (value_mg_x >= LED_TH_MG) {
			LED_BLUE_OFF();
			LED_ORANGE_OFF();
			LED_GREEN_OFF();
			LED_RED_ON();
		} else if (value_mg_x <= -LED_TH_MG) {
			LED_BLUE_OFF();
			LED_ORANGE_OFF();
			LED_RED_OFF();
			LED_GREEN_ON();
		}

		/* set Y related LEDs according to specified threshold */
		if (value_mg_y >= LED_TH_MG) {
			LED_BLUE_OFF();
			LED_RED_OFF();
			LED_GREEN_OFF();
			LED_ORANGE_ON();
		} else if (value_mg_y <= -LED_TH_MG) {
			LED_RED_OFF();
			LED_GREEN_OFF();
			LED_ORANGE_OFF();
			LED_BLUE_ON();
		}

		/* set Z related LEDs according to specified threshold */
		if (value_mg_z >= LED_TH_MG) {
			LED_BLUE_ON();
			LED_ORANGE_ON();
			LED_RED_ON();
			LED_GREEN_ON();
		} else if (value_mg_z <= -LED_TH_MG) {
			LED_BLUE_OFF();
			LED_ORANGE_OFF();
			LED_RED_OFF();
			LED_GREEN_OFF();
		}
	}

	return 0;
}


/* End of file */
