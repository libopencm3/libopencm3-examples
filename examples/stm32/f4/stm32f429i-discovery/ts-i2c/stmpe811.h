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

#ifndef __STMPE811_H
#define __STMPE811_H

/**
 * @brief  Orientation enum
 * @note   You may need experimenting to get proper orientation to match your LCD
 */
typedef enum {
	stmpe811_portrait_1,
	stmpe811_portrait_2,
	stmpe811_landscape_1,
	stmpe811_landscape_2,
} stmpe811_orientation_t;

/**
 * @brief  Touch state enum
 */
typedef enum {
	stmpe811_state_pressed,
	stmpe811_state_released,
	stmpe811_state_ok,
	stmpe811_state_error
} stmpe811_state_t;

/**
 * @brief  Main structure
 */
typedef struct {
	uint16_t x;
	uint16_t y;
	stmpe811_state_t pressed;
	stmpe811_state_t last_pressed;
	stmpe811_orientation_t orientation;
} stmpe811_t;


stmpe811_state_t stmpe811_init(void);
uint16_t stmpe811_read_x(uint16_t x);
uint16_t stmpe811_read_y(uint16_t y);
stmpe811_state_t stmpe811_read_touch(stmpe811_t *stmpe811_data);


#endif
