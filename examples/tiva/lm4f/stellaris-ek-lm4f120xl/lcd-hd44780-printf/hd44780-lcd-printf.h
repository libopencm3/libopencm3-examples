/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

#ifndef _HD44780_LCD_PRINTF_H_
#define _HD44780_LCD_PRINTF_H_

#include "hd44780-gpio.h"

#include <string.h>

/*
 * stellaris_vfd is the hardware-specific driver. It serves as the context used
 * by the generic hd44780-gpio driver.
 */
extern struct hd44780_lcd stellaris_vfd;

void uart_setup(void);
void uart_irq_setup(void);
void uart_printf(const char *format, ...);
size_t uart_get_num_rx_bytes(void);
size_t uart_get_num_tx_bytes(void);
void uart_reset_rxtx_counters(void);

#endif /* _HD44780_LCD_PRINTF_H_ */
