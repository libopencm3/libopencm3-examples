/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2013 Alexandru Gagniuc <mr.nuke.me@gmail.com>
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

#include "usb_to_serial_cdcacm.h"

#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/lm4f/usb.h>

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x2000,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0xc03e,
	.idProduct = 0xb007,
	.bcdDevice = 0x2000,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 1,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__ ((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = (1 << 1),
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	}
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"libopencm3",
	"usb_to_serial_cdcacm",
	"none",
	"DEMO",
};

usbd_device *acm_dev;
uint8_t usbd_control_buffer[128];
extern usbd_driver lm4f_usb_driver;

static enum usbd_request_return_codes
cdcacm_control_request(usbd_device * usbd_dev,
		struct usb_setup_data *req, uint8_t ** buf, uint16_t * len,
		usbd_control_complete_callback *complete)
{
	uint8_t dtr, rts;

	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:{
			/*
			 * This Linux cdc_acm driver requires this to be implemented
			 * even though it's optional in the CDC spec, and we don't
			 * advertise it in the ACM functional descriptor.
			 */

			dtr = (req->wValue & (1 << 0)) ? 1 : 0;
			rts = (req->wValue & (1 << 1)) ? 1 : 0;

			glue_set_line_state_cb(dtr, rts);

			return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_SET_LINE_CODING:{
			struct usb_cdc_line_coding *coding;

			if (*len < sizeof(struct usb_cdc_line_coding))
				return USBD_REQ_NOTSUPP;

			coding = (struct usb_cdc_line_coding *)*buf;
			return glue_set_line_coding_cb(coding->dwDTERate,
						       coding->bDataBits,
						       coding->bParityType,
						       coding->bCharFormat);
		}
	}
	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device * usbd_dev, uint8_t ep)
{
	uint8_t buf[64];

	(void)ep;

	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);
	glue_send_data_cb(buf, len);
}

void cdcacm_send_data(uint8_t * buf, uint16_t len)
{
	usbd_ep_write_packet(acm_dev, 0x82, buf, len);
}

static void cdcacm_set_config(usbd_device * usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64,
		      cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(usbd_dev,
				       USB_REQ_TYPE_CLASS |
				       USB_REQ_TYPE_INTERFACE,
				       USB_REQ_TYPE_TYPE |
				       USB_REQ_TYPE_RECIPIENT,
				       cdcacm_control_request);
}

void cdcacm_line_state_changed_cb(uint8_t linemask)
{
	const int size = sizeof(struct usb_cdc_notification) + 2;
	uint8_t buf[size];

	struct usb_cdc_notification *notify = (void *)buf;
	notify->bmRequestType = 0xa1;
	notify->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
	notify->wValue = 0;
	notify->wIndex = 1;
	notify->wLength = 2;
	uint16_t *data = (void *)&buf[sizeof(struct usb_cdc_notification)];
	*data = linemask;

	while (usbd_ep_write_packet(acm_dev, 0x83, buf, size) == size) ;
}

static void usb_pins_setup(void)
{
	/* USB pins are connected to port D */
	periph_clock_enable(RCC_GPIOD);
	/* Mux USB pins to their analog function */
	gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4 | GPIO5);
}

static void usb_ints_setup(void)
{
	uint8_t usbints;
	/* Gimme some interrupts */
	usbints = USB_INT_RESET | USB_INT_DISCON | USB_INT_RESUME | USB_INT_SUSPEND;	//| USB_IM_SOF;
	usb_enable_interrupts(usbints, 0xff, 0xff);
	nvic_enable_irq(NVIC_USB0_IRQ);
}

void cdcacm_init(void)
{
	usbd_device *usbd_dev;

	usb_pins_setup();

	usbd_dev = usbd_init(&lm4f_usb_driver, &dev, &config, usb_strings, 4,
			     usbd_control_buffer, sizeof(usbd_control_buffer));
	acm_dev = usbd_dev;
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	usb_ints_setup();
}

void usb0_isr(void)
{
	usbd_poll(acm_dev);
}
