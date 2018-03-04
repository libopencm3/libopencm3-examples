/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2018 Baranovskiy Konstantin <baranovskiykonstantin@gmail.com>
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

#ifndef RING_H
#define RING_H

#include <libopencm3/stm32/gpio.h>

struct ring {
	uint8_t *data;
	uint32_t data_size;
	uint32_t ring_size;
	uint32_t begin;
	uint32_t end;
};

extern void ring_init(struct ring *ring, uint8_t *buf, int32_t ring_size);

extern int32_t ring_write_ch(struct ring *ring, uint8_t ch);

extern int32_t ring_write(struct ring *ring, uint8_t *data, int32_t size);

extern int32_t ring_read_ch(struct ring *ring, uint8_t *ch);

extern int32_t ring_read(struct ring *ring, uint8_t *data, int32_t size);

#endif
