/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2011 Piotr Esden-Tempski <piotr@esden.net>
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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/usb/dwc/otg_fs.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include "adxl345.h"

/* Define this to include the DFU APP interface. */
#define INCLUDE_DFU_INTERFACE

#ifdef INCLUDE_DFU_INTERFACE
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/dfu.h>
#endif

static usbd_device *usbd_dev;

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
	0x05, 0x01, /* USAGE_PAGE (Generic Desktop)         */
	0x09, 0x02, /* USAGE (Mouse)                        */
	0xa1, 0x01, /* COLLECTION (Application)             */
	0x09, 0x01, /*   USAGE (Pointer)                    */
	0xa1, 0x00, /*   COLLECTION (Physical)              */
	0x05, 0x09, /*     USAGE_PAGE (Button)              */
	0x19, 0x01, /*     USAGE_MINIMUM (Button 1)         */
	0x29, 0x03, /*     USAGE_MAXIMUM (Button 3)         */
	0x15, 0x00, /*     LOGICAL_MINIMUM (0)              */
	0x25, 0x01, /*     LOGICAL_MAXIMUM (1)              */
	0x95, 0x03, /*     REPORT_COUNT (3)                 */
	0x75, 0x01, /*     REPORT_SIZE (1)                  */
	0x81, 0x02, /*     INPUT (Data,Var,Abs)             */
	0x95, 0x01, /*     REPORT_COUNT (1)                 */
	0x75, 0x05, /*     REPORT_SIZE (5)                  */
	0x81, 0x01, /*     INPUT (Cnst,Ary,Abs)             */
	0x05, 0x01, /*     USAGE_PAGE (Generic Desktop)     */
	0x09, 0x30, /*     USAGE (X)                        */
	0x09, 0x31, /*     USAGE (Y)                        */
	0x09, 0x38, /*     USAGE (Wheel)                    */
	0x15, 0x81, /*     LOGICAL_MINIMUM (-127)           */
	0x25, 0x7f, /*     LOGICAL_MAXIMUM (127)            */
	0x75, 0x08, /*     REPORT_SIZE (8)                  */
	0x95, 0x03, /*     REPORT_COUNT (3)                 */
	0x81, 0x06, /*     INPUT (Data,Var,Rel)             */
	0xc0,       /*   END_COLLECTION                     */
	0x09, 0x3c, /*   USAGE (Motion Wakeup)              */
	0x05, 0xff, /*   USAGE_PAGE (Vendor Defined Page 1) */
	0x09, 0x01, /*   USAGE (Vendor Usage 1)             */
	0x15, 0x00, /*   LOGICAL_MINIMUM (0)                */
	0x25, 0x01, /*   LOGICAL_MAXIMUM (1)                */
	0x75, 0x01, /*   REPORT_SIZE (1)                    */
	0x95, 0x02, /*   REPORT_COUNT (2)                   */
	0xb1, 0x22, /*   FEATURE (Data,Var,Abs,NPrf)        */
	0x75, 0x06, /*   REPORT_SIZE (6)                    */
	0x95, 0x01, /*   REPORT_COUNT (1)                   */
	0xb1, 0x01, /*   FEATURE (Cnst,Ary,Abs)             */
	0xc0        /* END_COLLECTION                       */
};

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0100,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 4,
	.bInterval = 0x02,
};

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = 1, /* boot */
	.bInterfaceProtocol = 2, /* mouse */
	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

#ifdef INCLUDE_DFU_INTERFACE
const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
	.wDetachTimeout = 255,
	.wTransferSize = 1024,
	.bcdDFUVersion = 0x011A,
};

const struct usb_interface_descriptor dfu_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFE,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 1,
	.iInterface = 0,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};
#endif

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
#ifdef INCLUDE_DFU_INTERFACE
}, {
	.num_altsetting = 1,
	.altsetting = &dfu_iface,
#endif
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
#ifdef INCLUDE_DFU_INTERFACE
	.bNumInterfaces = 2,
#else
	.bNumInterfaces = 1,
#endif
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	"HID Demo",
	"DEMO",
};

/* Buffer used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)dev;

	if((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return USBD_REQ_NOTSUPP;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}

#ifdef INCLUDE_DFU_INTERFACE
static void dfu_detach_complete(usbd_device *dev, struct usb_setup_data *req)
{
	(void)req;
	(void)dev;

	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, 0, GPIO15);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
	gpio_set(GPIOA, GPIO10);
	scb_reset_core();
}

static enum usbd_request_return_codes dfu_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *dev, struct usb_setup_data *req))
{
	(void)buf;
	(void)len;
	(void)dev;

	if ((req->bmRequestType != 0x21) || (req->bRequest != DFU_DETACH))
		return USBD_REQ_NOTSUPP; /* Only accept class request. */

	*complete = dfu_detach_complete;

	return USBD_REQ_HANDLED;
}
#endif

static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);
#ifdef INCLUDE_DFU_INTERFACE
	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				dfu_control_request);
#endif

	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(99999);
	systick_interrupt_enable();
	systick_counter_enable();
}

static uint8_t spi_readwrite(uint32_t spi, uint8_t data)
{
	while (SPI_SR(spi) & SPI_SR_BSY)
		;
	SPI_DR(spi) = data;
	while (!(SPI_SR(spi) & SPI_SR_RXNE))
		;
	return SPI_DR(spi);
}

static uint8_t accel_read(uint8_t addr)
{
	uint8_t ret;
	gpio_clear(GPIOB, GPIO12);
	spi_readwrite(SPI2, addr | 0x80);
	ret = spi_readwrite(SPI2, 0);
	gpio_set(GPIOB, GPIO12);
	return ret;
}

static void accel_write(uint8_t addr, uint8_t data)
{
	gpio_clear(GPIOB, GPIO12);
	spi_readwrite(SPI2, addr);
	spi_readwrite(SPI2, data);
	gpio_set(GPIOB, GPIO12);
}

static void accel_get(int16_t *x, int16_t *y, int16_t *z)
{
	if (x)
		*x = accel_read(ADXL345_DATAX0) |
			(accel_read(ADXL345_DATAX1) << 8);
	if (y)
		*y = accel_read(ADXL345_DATAY0) |
			(accel_read(ADXL345_DATAY1) << 8);
	if (z)
		*z = accel_read(ADXL345_DATAZ0) |
			(accel_read(ADXL345_DATAZ1) << 8);
}

int main(void)
{
	int i;

	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE12_72MHZ]);

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_SPI2);
	rcc_periph_clock_enable(RCC_OTGFS);

	/* Configure SPI2: PB13(SCK), PB14(MISO), PB15(MOSI). */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
		      GPIO_SPI2_SCK | GPIO_SPI2_MOSI);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
		      GPIO_SPI2_MISO);
	/* Enable CS pin on PB12. */
	gpio_set(GPIOB, GPIO12);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_10_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);

	/* Force to SPI mode. This should be default after reset! */
	SPI2_I2SCFGR = 0;
	spi_init_master(SPI2,
			SPI_CR1_BAUDRATE_FPCLK_DIV_256,
			SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
			SPI_CR1_CPHA_CLK_TRANSITION_2,
			SPI_CR1_DFF_8BIT,
			SPI_CR1_MSBFIRST);
	/* Ignore the stupid NSS pin. */
	spi_enable_software_slave_management(SPI2);
	spi_set_nss_high(SPI2);
	spi_enable(SPI2);

	(void)accel_read(ADXL345_DEVID);
	accel_write(ADXL345_POWER_CTL, ADXL345_POWER_CTL_MEASURE);
	accel_write(ADXL345_DATA_FORMAT, ADXL345_DATA_FORMAT_LALIGN);

	/* USB_DETECT as input. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO8);

	/* Green LED off, as output. */
	gpio_set(GPIOC, GPIO2);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);

	usbd_dev = usbd_init(&stm32f107_usb_driver, &dev_descr, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, hid_set_config);

	/* Delay some seconds to show that pull-up switch works. */
	for (i = 0; i < 0x800000; i++)
		__asm__("nop");

	/* Wait for USB Vbus. */
	while (gpio_get(GPIOA, GPIO8) == 0)
		__asm__("nop");

	/* Green LED on, connect USB. */
	gpio_clear(GPIOC, GPIO2);
	// OTG_FS_GCCFG &= ~OTG_FS_GCCFG_VBUSBSEN;

	while (1)
		usbd_poll(usbd_dev);
}

void sys_tick_handler(void)
{
	int16_t x, y;
	uint8_t buf[4] = {0, 0, 0, 0};

	accel_get(&x, &y, NULL);
	buf[1] = x >> 9;
	buf[2] = y >> 9;

	usbd_ep_write_packet(usbd_dev, 0x81, buf, 4);
}
