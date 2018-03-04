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

/******************************************************************************
 * Simple RINGBUFFER
 * Based on:
 * https://github.com/open-bldc/open-bldc/tree/master/source/libgovernor
 *****************************************************************************/

#include "ring.h"

void ring_init(struct ring *ring, uint8_t *buf, int32_t ring_size)
{
	ring->data = buf;
	ring->ring_size = ring_size;
	ring->data_size = 0;
	ring->begin = 0;
	ring->end = 0;
}

int32_t ring_write_ch(struct ring *ring, uint8_t ch)
{
	if (ring->data_size < ring->ring_size) {
		ring->data[ring->end++] = ch;
		ring->end %= ring->ring_size;
		ring->data_size++;
		return (uint32_t)ch;
	}

	return -1;
}

int32_t ring_write(struct ring *ring, uint8_t *data, int32_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {
		if (ring_write_ch(ring, data[i]) < 0)
			return -i;
	}

	return i;
}

int32_t ring_read_ch(struct ring *ring, uint8_t *ch)
{
	int32_t ret = -1;

	if (ring->data_size > 0) {
		ret = ring->data[ring->begin++];
		ring->begin %= ring->ring_size;
		ring->data_size--;
		if (ch)
			*ch = ret;
	}

	return ret;
}

int32_t ring_read(struct ring *ring, uint8_t *data, int32_t size)
{
	int32_t i;

	for (i = 0; i < size; i++) {
		if (ring_read_ch(ring, data + i) < 0)
			return i;
	}

	return -i;
}

