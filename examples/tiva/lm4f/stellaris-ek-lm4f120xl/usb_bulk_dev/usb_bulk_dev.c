/*
 * This file is part of the libopencm3 project.
 *
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

/**
 * \addtogroup Examples
 *
 * Establishes a basic USB devices with interrupt-driven and polled IN and OUT
 * bulk endpoints.
 */
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/lm4f/usb.h>

#include<stdio.h>

int _write(int file, char *ptr, int len);
void uart_setup(void);

/* =============================================================================
 * = Clock control definitions
 * ---------------------------------------------------------------------------*/

/* This is how the RGB LED is connected on the stellaris launchpad */
#define RGB_PORT	GPIOF
enum {
	LED_R	= GPIO1,
	LED_G	= GPIO3,
	LED_B	= GPIO2,
};

/* This is how the user switches are connected to GPIOF */
enum {
	USR_SW1	= GPIO4,
	USR_SW2	= GPIO0,
};

/* The divisors we loop through when the user presses SW2 */
enum {
	PLL_DIV_80MHZ 	= 5,
	PLL_DIV_57MHZ 	= 7,
	PLL_DIV_40MHZ 	= 10,
	PLL_DIV_30MHZ 	= 13,
	PLL_DIV_20MHZ 	= 20,
	PLL_DIV_16MHZ 	= 25,
};

static const uint8_t plldiv[] = {
	PLL_DIV_80MHZ,
	PLL_DIV_57MHZ,
	PLL_DIV_40MHZ,
	PLL_DIV_30MHZ,
	PLL_DIV_20MHZ,
	PLL_DIV_16MHZ,
	0
};

/* The PLL divisor we are currently on */
static size_t ipll = 0;
/* Are we bypassing the PLL, or not? */
static bool bypass = false;

/* =============================================================================
 * = USB descriptors
 * ---------------------------------------------------------------------------*/

static const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0110,
	.bDeviceClass = 0xff,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0xc03e,
	.idProduct = 0xb007,
	.bcdDevice = 0x0110,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor bulk_endp[] = {{
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
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x03,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x84,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x05,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x86,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct usb_interface_descriptor bulk_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 6,
	.bInterfaceClass = 0xff,
	.bInterfaceSubClass = 0xff,
	.bInterfaceProtocol = 0xff,
	.iInterface = 0,

	.endpoint = bulk_endp,

	.extra = NULL,
	.extralen = 0,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = bulk_iface,
}};

static const struct usb_config_descriptor config_descr = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,
	.interface = ifaces,
};

static usbd_device *bulk_dev;
static uint8_t usbd_control_buffer[128];
static uint8_t config_set = 0;

static const char *usb_strings[] = {
	"libopencm3",
	"usb_dev_bulk",
	"none",
	"DEMO",
};

/* =============================================================================
 * = USB Module
 * ---------------------------------------------------------------------------*/

/*
 * Mux the USB pins to their analog function
 */
static void usb_setup(void)
{
	/* USB pins are connected to port D */
	periph_clock_enable(RCC_GPIOD);
	/* Mux USB pins to their analog function */
	gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4 | GPIO5);
}

/*
 * Enable USB interrupts
 *
 * We don't enable the USB peripheral clock here, but we need it on in order to
 * acces USB registers. Hence, this must be called after usbd_init().
 */
static void usb_ints_setup(void)
{
	uint8_t usbints;
	/* Gimme some interrupts */
	usbints = USB_INT_RESET | USB_INT_DISCON | USB_INT_RESUME |
	    USB_INT_SUSPEND | USB_INT_SOF;
	usb_enable_interrupts(usbints, 0xff, 0xff);
	nvic_enable_irq(NVIC_USB0_IRQ);
}

/*
 * Callback for the interrupt-driven OUT endpoint
 *
 * This gets called whenever a new OUT packet has arrived.
 */
static void bulk_rx_cb(usbd_device * usbd_dev, uint8_t ep)
{
	char buf[64] __attribute__ ((aligned(4)));

	(void)ep;

	/* Read the packet to clear the FIFO and make room for a new packet */
	usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);
}

/*
 * Callback for the interrupt-driven IN endpoint
 *
 * This gets called whenever an IN packet has been successfully transmitted.
 */
static void bulk_tx_cb(usbd_device * usbd_dev, uint8_t ep)
{
	char buf[64] __attribute__ ((aligned(4)));

	(void)ep;

	/* Keep sending packets */
	usbd_ep_write_packet(usbd_dev, 0x82, buf, 64);
}

/*
 * Initialize the USB configuration
 *
 * Called after the host issues a SetConfiguration request.
 */
static void set_config(usbd_device * usbd_dev, uint16_t wValue)
{
	uint8_t data[64] __attribute__ ((aligned(4)));

	(void)wValue;
	printf("Configuring endpoints.\n\r");
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, bulk_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, bulk_tx_cb);
	usbd_ep_setup(usbd_dev, 0x03, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x84, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x05, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x86, USB_ENDPOINT_ATTR_BULK, 64, NULL);

	/* The main loop will not touch the EPs until this is set */
	config_set = 1;

	/*
	 * "Bootstrap" the callback-based endpoint
	 * Data will stay in the FIFO until the host reads it. Once it's sent
	 * our callback kicks in and writes another packet in the FIFO.
	 */
	usbd_ep_write_packet(bulk_dev, 0x82, data, 64);
	printf("Done.\n\r");
}

/* =============================================================================
 * = Clock control module
 * ---------------------------------------------------------------------------*/

/*
 * Setup the buttons and interrupts
 */
static void button_setup(void)
{
	/*
	 * Configure GPIOF
	 * This port is used to control the RGB LED
	 */
	periph_clock_enable(RCC_GPIOF);

	/*
	 * Now take care of our buttons
	 */
	const uint32_t btnpins = USR_SW1 | USR_SW2;

	/*
	 * PF0 is a locked by default. We need to unlock it before we can
	 * re-purpose it as a GPIO pin.
	 */
	gpio_unlock_commit(GPIOF, USR_SW2);
	/* Configure pins as inputs, with pull-up. */
	gpio_mode_setup(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, btnpins);

	/* Trigger interrupt on rising-edge (when button is depressed) */
	gpio_configure_trigger(GPIOF, GPIO_TRIG_EDGE_RISE, btnpins);
	/* Finally, Enable interrupt */
	gpio_enable_interrupts(GPIOF, btnpins);
	/* Enable the interrupt in the NVIC as well */
	nvic_enable_irq(NVIC_GPIOF_IRQ);
}

/* =============================================================================
 * = A main() function which does not need to do too much
 * ---------------------------------------------------------------------------*/

int main(void)
{
	uint8_t data[65] __attribute__ ((aligned(4)));

	gpio_enable_ahb_aperture();
	rcc_sysclk_config(OSCSRC_MOSC, XTAL_16M, PLL_DIV_80MHZ);

	/* We use the UART for debugging */
	uart_setup();
	/* And the buttons for changing the system clock on-the-fly */
	button_setup();

	/* Mux the GPIO pins to the USB peripheral */
	usb_setup();
	/* Let the stack take care of the rest */
	bulk_dev = usbd_init(&lm4f_usb_driver, &dev_descr, &config_descr,
			     usb_strings, 4,
			     usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(bulk_dev, set_config);
	/* Enable the interrupts. */
	usb_ints_setup();

	/* HALT! Don't touch the EP's until we configure them */
	while (!config_set) ;

	/*
	 * For our polled endpoints, we just read and write continuously. The
	 * driver will only move data in or out of the FIFOs if it is safe to
	 * do so.
	 */
	while (1) {
		usbd_ep_read_packet(bulk_dev, 0x03, data, 64);
		usbd_ep_write_packet(bulk_dev, 0x84, data, 64);
		/*
		 * On endpoints 5 and 6, we deliberately misalign the buffer.
		 * This degrades the endpoint performance.
		 */
		usbd_ep_read_packet(bulk_dev, 0x05, data + 1, 64);
		usbd_ep_write_packet(bulk_dev, 0x86, data + 1, 64);
	}

	/* Never reached */
	return 0;
}

/* =============================================================================
 * = USB interrupt service routine. All the magic happens here
 * ---------------------------------------------------------------------------*/
void usb0_isr(void)
{
	usbd_poll(bulk_dev);
}

/* =============================================================================
 * = GPIO interrupt service routine. Pressing a button gets us here.
 * ---------------------------------------------------------------------------*/

void gpiof_isr(void)
{
	uint8_t serviced_irqs = 0;

	if (gpio_is_interrupt_source(GPIOF, USR_SW1)) {
		/* SW1 was just depressed */
		bypass = !bypass;
		if (bypass) {
			rcc_pll_bypass_enable();
			/*
			 * The divisor is still applied to the raw clock.
			 * Disable the divisor, or we'll divide the raw clock.
			 */
			SYSCTL_RCC &= ~SYSCTL_RCC_USESYSDIV;
			printf("Changing system clock to 16MHz MOSC\n\r");
		} else {
			rcc_change_pll_divisor(plldiv[ipll]);
			printf("Changing system clock to %iMHz\n\r",
			       400 / plldiv[ipll]);
		}
		/* Clear interrupt source */
		serviced_irqs |= USR_SW1;
	}

	if (gpio_is_interrupt_source(GPIOF, USR_SW2)) {
		/* SW2 was just depressed */
		if (!bypass) {
			if (plldiv[++ipll] == 0)
				ipll = 0;
			printf("Changing system clock to %iMHz\n\r",
			       400 / plldiv[ipll]);
			rcc_change_pll_divisor(plldiv[ipll]);
		}
		/* Clear interrupt source */
		serviced_irqs |= USR_SW2;
	}

	gpio_clear_interrupt_flag(GPIOF, serviced_irqs);
}

/* =============================================================================
 * = Debug module
 * ---------------------------------------------------------------------------*/

#include <libopencm3/lm4f/uart.h>
#include <errno.h>

/*
 * Initialize the UART
 */
void uart_setup(void)
{
	/* Enable GPIOA in run mode. */
	periph_clock_enable(RCC_GPIOA);
	/* Configure PA0 and PA1 as alternate function pins */
	gpio_set_af(GPIOA, 1, GPIO0 | GPIO1);

	/* Enable the UART clock */
	periph_clock_enable(RCC_UART0);
	/* Slight delay before we can access the UART registers */
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	/* Disable the UART while we mess with its setings */
	uart_disable(UART0);
	/* Configure the UART clock source */
	uart_clock_from_piosc(UART0);
	/* Set communication parameters */
	uart_set_baudrate(UART0, 921600);
	/* Set 8N1 */
	uart_set_databits(UART0, 8);
	uart_set_parity(UART0, UART_PARITY_NONE);
	uart_set_stopbits(UART0, 1);
	/* Enable FIFOs */
	UART_LCRH(UART0) |= UART_LCRH_FEN;
	/* Now that we're done messing with the settings, enable the UART */
	uart_enable(UART0);
}

/*
 * Write to the debug port
 *
 * This is called whenever printf is used. We write stdio to the UART.
 */
int _write(int file, char *ptr, int len)
{
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			uart_send_blocking(UART0, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}
