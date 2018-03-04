/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include "ring.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define BULK_EP_SIZE 64
#define RINGBUFFER_SIZE (BULK_EP_SIZE * 2)

static struct ring uart_tx_ring;
static uint8_t uart_tx_data[RINGBUFFER_SIZE];

static struct ring uart_rx_ring;
static uint8_t uart_rx_data[RINGBUFFER_SIZE];

/******************************************************************************
 * USB descriptors and data structures
 *****************************************************************************/

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * According to CDC spec its optional, but its absence causes a NULL pointer
 * dereference in Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = BULK_EP_SIZE,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = BULK_EP_SIZE,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
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
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
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
	.extralen = sizeof(cdcacm_functional_descriptors),
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
	"Black Sphere Technologies",
	"CDC-ACM Demo",
	"DEMO",
};

/******************************************************************************
 * USART1 - receiving data from UART
 *****************************************************************************/

void usart1_isr(void)
{
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) &&
		((USART_SR(USART1) & USART_SR_RXNE) != 0)) {

		/* Retrieve the data from the peripheral. */
		ring_write_ch(&uart_rx_ring, usart_recv(USART1));
	}
}

/******************************************************************************
 * USB Bulk transfers (data) - receiving data from USB
 *****************************************************************************/

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	(void)usbd_dev;

	if ((uart_tx_ring.ring_size - uart_tx_ring.data_size) > BULK_EP_SIZE) {
		uint8_t buf[BULK_EP_SIZE];
		int16_t len = usbd_ep_read_packet(usbd_dev, 0x01, buf, BULK_EP_SIZE);

		if (len)
			ring_write(&uart_tx_ring, buf, len);
	}
}

/******************************************************************************
 * USB Interrupt transfers (communication)
 *****************************************************************************/

static uint16_t cdcacm_send_notification(usbd_device *usbd_dev, uint16_t serial_state)
{
	/*
	 * serial_state Field		Description
	 *
	 * 15..7	--		Reserved
	 * 6		bOverRun	Received data has been discarded
					due to an overrun in the device.
	 * 5		bParity		A parity error has occurred.
	 * 4		bFraming	A framing error has occurred.
	 * 3		bRingSignal	State of ring indicator (RI).
	 * 2		bBreak		Break state.
	 * 1		bTxCarrier	State of data set ready (DSR).
	 * 0		bRxCarrier	State of carrier detect (CD).
	 */
	uint16_t status;
	char local_buf[10];
	struct usb_cdc_notification *notif = (void *)local_buf;

	notif->bmRequestType = 0xA1;
	notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
	notif->wValue = 0;
	notif->wIndex = 0;
	notif->wLength = 2;
	local_buf[8] = (uint8_t)(serial_state & 0xff);
	local_buf[9] = (uint8_t)(serial_state >> 8);
	status = usbd_ep_write_packet(usbd_dev, 0x83, local_buf, 10);

	/* status contains count of successfully written bytes to endpoint */
	return status;
}

/******************************************************************************
 * USB Control transfers
 *****************************************************************************/

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static int cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */

		/*
		 * wValue Bit	Description
		 *
		 * 15..2	Reserved (Set to zero.)
		 * 1		RTS
		 * 0		DTR
		 */

		/* We echo signals back to host as notification. */
		uint16_t status;
		status = cdcacm_send_notification(usbd_dev, req->wValue & 3);

		return status ? 1 : 0;
	}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding)) {
			return 0;
		} else {
			struct usb_cdc_line_coding *line_coding = (void *)usbd_control_buffer;

			/* Setup UART parameters. */
			/* Available baud rates: 1200..230400 */
			usart_set_baudrate(USART1, line_coding->dwDTERate);
			usart_set_databits(USART1, line_coding->bDataBits);
			switch(line_coding->bCharFormat) {
			case USB_CDC_1_STOP_BITS:
				usart_set_stopbits(USART1, USART_STOPBITS_1);
				break;
			case USB_CDC_1_5_STOP_BITS:
				usart_set_stopbits(USART1, USART_STOPBITS_1_5);
				break;
			case USB_CDC_2_STOP_BITS:
				usart_set_stopbits(USART1, USART_STOPBITS_2);
				break;
			}
			switch(line_coding->bParityType) {
			case USB_CDC_ODD_PARITY:
				usart_set_parity(USART1, USART_PARITY_ODD);
				break;
			case USB_CDC_EVEN_PARITY:
				usart_set_parity(USART1, USART_PARITY_EVEN);
				break;
			default:
				usart_set_parity(USART1, USART_PARITY_NONE);
			}

			/* Enable USART1 Receive interrupt. */
			USART_CR1(USART1) |= USART_CR1_RXNEIE;
			return 1;
		}
	}
	return 0;
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, BULK_EP_SIZE, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, BULK_EP_SIZE, NULL);
	/* usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, BULK_EP_SIZE, cdcacm_data_tx_cb); */
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);
}

/* ========================================================================= */

int main(void)
{
	int i;

	uint8_t local_buf[BULK_EP_SIZE];
	uint16_t len;

	usbd_device *usbd_dev;

	rcc_clock_setup_in_hsi_out_48mhz();

	/*
	 * RINGBUFFERS
	 */
	/* Initialize ring buffers. */
	ring_init(&uart_rx_ring, uart_rx_data, RINGBUFFER_SIZE);
	ring_init(&uart_tx_ring, uart_tx_data, RINGBUFFER_SIZE);

	/*
	 * LED
	 */
	/* Enable clocks for GPIO port C. */
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set(GPIOC, GPIO13);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	/*
	 * UART1
	 */
	/* Enable clocks for GPIO port A (for GPIO_USART1_TX/RX) and USART1. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);
	/* Enable the USART1 interrupt. */
	nvic_enable_irq(NVIC_USART1_IRQ);
	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	/* Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);
	/* Setup default UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);
	/* Finally enable the USART. */
	usart_enable(USART1);

	/*
	 * USB
	 */
	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver,
		&dev, &config, usb_strings,
		3,
		usbd_control_buffer,
		sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	gpio_clear(GPIOC, GPIO13);

	for (i = 0; i < 0x800000; i++)
		__asm__("nop");

	gpio_set(GPIOC, GPIO13);

	/*
	 * Main loop
	 */
	while (1) {
		usbd_poll(usbd_dev);

		/* Transmitting data to UART */
		len = MIN(uart_tx_ring.data_size, BULK_EP_SIZE);
		if (len > 0) {
			gpio_clear(GPIOC, GPIO13);

			ring_read(&uart_tx_ring, local_buf, len);

			for (i = 0; i < len; i++)
				usart_send_blocking(USART1, local_buf[i]);

			gpio_set(GPIOC, GPIO13);
		}

		/* Transmitting data to USB */
		len = MIN(uart_rx_ring.data_size, BULK_EP_SIZE);
		if (len > 0) {
			gpio_clear(GPIOC, GPIO13);

			ring_read(&uart_rx_ring, local_buf, len);

			while (usbd_ep_write_packet(usbd_dev, 0x82, local_buf, len) == 0);

			gpio_set(GPIOC, GPIO13);
		}
	}
}
