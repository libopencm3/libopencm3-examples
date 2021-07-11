/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Karl Palsson <karlp@tweak.net.au>
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
/*
 * This is the header file for a usb_cdcacm implmentation, usb_cdcacm.c is the
 * platform independent portion, and usb_cdcacm-arch.c should be re-implemented
 * for other platforms.
 */

#ifndef USB_CDCACM_H
#define	USB_CDCACM_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

	void usb_cdcacm_init(usbd_device **usb_dev);
	void usb_cdcacm_setup_pre_arch(void);
	void usb_cdcacm_setup_post_arch(void);
	void cdcacm_send_data(uint8_t *buf, uint16_t len);
	void cdcacm_line_state_changed_cb(uint8_t linemask);

	/* Call this if you have data to send to the usb host */
	void glue_data_received_cb(uint8_t *buf, uint16_t len);
	/* These will be called by usb_cdcacm code */
	void glue_send_data_cb(uint8_t *buf, uint16_t len);

	void glue_set_line_state_cb(uint8_t dtr, uint8_t rts);
	int glue_set_line_coding_cb(uint32_t baud, uint8_t databits,
		enum usb_cdc_line_coding_bParityType cdc_parity,
		enum usb_cdc_line_coding_bCharFormat cdc_stopbits);

#ifdef	__cplusplus
}
#endif

#endif	/* USB_CDCACM_H */

