/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2012 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

/**@{*/

#ifndef __STELLARIS_EK_LM4F120XL_USB_TO_SERIAL_CDCACM_H
#define __STELLARIS_EK_LM4F120XL_USB_TO_SERIAL_CDCACM_H

#include <libopencm3/cm3/common.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/usb/cdc.h>

/* =============================================================================
 * Definitions missing from cdc.h
 *
 * TODO: Integrate those definitions into cdc.h
 * ---------------------------------------------------------------------------*/

/* Table 17: Line Coding Structure */
enum usb_cdc_line_coding_bCharFormat {
	USB_CDC_1_STOP_BITS			= 0,
	USB_CDC_1_5_STOP_BITS			= 1,
	USB_CDC_2_STOP_BITS			= 2,
};

enum usb_cdc_line_coding_bParityType {
	USB_CDC_NO_PARITY			= 0,
	USB_CDC_ODD_PARITY			= 1,
	USB_CDC_EVEN_PARITY			= 2,
	USB_CDC_MARK_PARITY			= 3,
	USB_CDC_SPACE_PARITY			= 4,
};

/* =============================================================================
 * UART control
 * ---------------------------------------------------------------------------*/
enum rs232pin {
	PIN_DCD					= GPIO2,
	PIN_DSR					= GPIO3,
	PIN_RI					= GPIO4,
	PIN_CTS					= GPIO5,
	PIN_DTR					= GPIO6,
	PIN_RTS					= GPIO7,
};

void uart_init(void);
u8 uart_get_ctl_line_state(void);
void uart_set_ctl_line_state(u8 dtr, u8 rts);
/* =============================================================================
 * CDCACM control
 * ---------------------------------------------------------------------------*/
enum cdc_serial_state_line {
	CDCACM_DCD				= (1 << 0),
	CDCACM_DSR				= (1 << 1),
	CDCACM_RI				= (1 << 3),
};

void cdcacm_init(void);
void cdcacm_line_state_changed_cb(u8 linemask);
void cdcacm_send_data(u8 *buf, u16 len);
/* =============================================================================
 * CDCACM <-> UART glue
 * ---------------------------------------------------------------------------*/
void glue_data_recieved_cb(u8 *buf, u16 len);
void glue_set_line_state_cb(u8 dtr, u8 rts);
int glue_set_line_coding_cb(u32 baud, u8 databits,
			    enum usb_cdc_line_coding_bParityType cdc_parity,
			    enum usb_cdc_line_coding_bCharFormat cdc_stopbits);
void glue_send_data_cb(u8 *buf, u16 len);

#endif /* __STELLARIS_EK_LM4F120XL_USB_TO_SERIAL_CDCACM_H */

/**@}*/

