/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Oliver Meier <h2obrain@gmail.com>
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
 *
 */


#ifndef __I2C_H__
#define __I2C_H__

#include "clock.h"
#include <libopencm3/stm32/i2c.h>
#include <libopencmsis/core_cm3.h>



/**
 * I2C extensions
 */

#define I2C_FILTR(i2c_base)		MMIO32(i2c_base + 0x24)
#define I2C_FILTER_ANOFF_DISABLE_ANALOG_FILTER (0b10000)


void i2c_setup_master(uint32_t i2c, uint32_t clock_speed);

#define i2c_is_busy(i2c) ((I2C_SR2(i2c)&I2C_SR2_BUSY) == I2C_SR2_BUSY)

/* TODO FIX I2C CLOCK SETUP!!! */
#define STMPE811_I2C_CLOCK_SPEED 10000
#define I2C_FLAG_WAIT_TIMEOUT    ((uint32_t)(168000000UL*100/STMPE811_I2C_CLOCK_SPEED))
#define I2C_MAX_RETRIES          10000

typedef enum {
	I2C_FLAG_WAIT_NO_ERROR = 0,
	I2C_FLAG_WAIT_UNKNOWN_ERROR
} i2c_flag_wait_error;

void
i2c_trx_retry(uint32_t i2c, uint8_t address,
		const uint8_t *write_data,  uint32_t write_len,
		      uint8_t  read_data[], uint32_t read_len
);

i2c_flag_wait_error
i2c_trx(uint32_t i2c, uint8_t address,
		const uint8_t *write_data,  uint32_t write_len,
		      uint8_t  read_data[], uint32_t read_len
);


#endif /*__I2C_H__ */
