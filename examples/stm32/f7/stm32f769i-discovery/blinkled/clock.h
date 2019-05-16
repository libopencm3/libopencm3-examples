/*
 * This include file describes the functions exported by clock.c and the
 * CLOCK_SETUP
 * 
 * This file is part of the libopencm3 project.
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

#ifndef __CLOCK_H
#define __CLOCK_H

#include <stdint.h>

#include <libopencm3/stm32/rcc.h>

#define CLOCK_SETUP rcc_3v3[RCC_CLOCK_3V3_216MHZ]

/*
 * Definitions for functions being abstracted out
 */
void msleep(uint32_t);
uint64_t mtime(void);
void clock_setup(void);

#endif /* generic header protector */

