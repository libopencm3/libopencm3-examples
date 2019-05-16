/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
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

#ifndef __SDRAM_H
#define __SDRAM_H

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>

#define SDRAM1_BASE_ADDRESS (0xc0000000U)
#define SDRAM2_BASE_ADDRESS (0xd0000000U)

/* Initialize the SDRAM chip on the board */
void sdram_init(void);
void sdram_init_custom(
		enum fmc_sdram_bank sdram_bank,
		uint32_t memory_width,
		uint32_t row_count, uint32_t column_count,
		struct sdram_timing timing,
		uint32_t cas_latency,
		bool read_burst, bool write_burst,
		uint32_t burst_length
);

#ifndef NULL
#define NULL	(void *)(0)
#endif

#endif
