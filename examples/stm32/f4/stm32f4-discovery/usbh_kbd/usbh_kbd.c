/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Kuldeep Singh Dhaka <kuldeepdhaka9@gmail.com>
 * Copyright (C) 2015 Amir Hammad <amir.hammad@hotmail.com>
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

#include <libopencm3/usbh/usbh.h>
#include <libopencm3/usbh/helper/ctrlreq.h>
#include <libopencm3/usbh/class/hid.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/otg_fs.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

/**
 * Generate clock for different part from 8Mhz clock
 * AHB = 168Mhz
 * APB1 = 42Mhz
 * APB2 = 84Mhz
 */
static void clock_setup(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOD);

	rcc_periph_clock_enable(RCC_USART6);
	rcc_periph_clock_enable(RCC_OTGFS);
	rcc_periph_clock_enable(RCC_TIM6);
}

static void gpio_setup(void)
{
	/* Set GPIO12-15 (in GPIO port D) to 'output push-pull'. */
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);

	/* Set */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
	gpio_clear(GPIOC, GPIO0);

	/* OTG_FS */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	/* USART TX */
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
	gpio_set_af(GPIOC, GPIO_AF8, GPIO6);
}

/**
 * Initalize the Output (TX) mode only (log)
 */
static void usart_init(void)
{
	usart_set_baudrate(USART6, 921600);
	usart_set_databits(USART6, 8);
	usart_set_flow_control(USART6, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART6, USART_MODE_TX);
	usart_set_parity(USART6, USART_PARITY_NONE);
	usart_set_stopbits(USART6, USART_STOPBITS_1);

	usart_enable(USART6);
}

/**
 * Setup the timer in 10Khz mode
 * This timer will be used to keep track of last poll time difference
 */
static void tim_setup(void)
{
	uint32_t tick_freq = 10000;

	uint32_t timer_clock = rcc_apb1_frequency;
	if (rcc_ahb_frequency != rcc_apb1_frequency) {
		timer_clock *= 2;
	}

	uint32_t clock_div = timer_clock / tick_freq;

	timer_reset(TIM6);
	timer_set_prescaler(TIM6, clock_div - 1);
	timer_set_period(TIM6, 0xFFFF);
	timer_enable_counter(TIM6);
}

/**
 * Get the counter value of Timer
 * @return Counter value
 */
static uint16_t tim_get_counter(void)
{
	return timer_get_counter(TIM6);
}

/**
 * Output string @a arg
 */
static void usart_puts(const char *arg)
{
	while (*arg != '\0') {
		usart_wait_send_ready(USART6);
		usart_send(USART6, *arg++);
	}
}

static void usart_vprintf(const char *fmt, va_list va)
{
	char db[128];
	if (vsnprintf(db, sizeof(db), fmt, va) > 0) {
		usart_puts(db);
	}
}

static void usart_printf(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	usart_vprintf(fmt, va);
	va_end(va);
}

#if defined(USBH_DEBUG)
# define LOG_MARKER usart_puts("### ");
#else
# define LOG_MARKER
#endif

#define NEW_LINE "\n"

#define LOG_LN(str)		\
	LOG_MARKER			\
	usart_puts(str);	\
	usart_puts(NEW_LINE)

#define LOGF_LN(fmt, ...)				\
	LOG_MARKER							\
	usart_printf(fmt, ##__VA_ARGS__);	\
	usart_puts(NEW_LINE)

#if defined(USBH_DEBUG)
void usbh_log_puts(const char *str)
{
	usart_puts(str);
}

void usbh_log_printf(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	usart_vprintf(fmt, va);
	va_end(va);
}
#endif

uint8_t buf[128];

static char usbkbd_to_char(uint8_t modifier, uint8_t data)
{
	static bool invert_case = false;

	if (data == 0x39) {
		invert_case = !invert_case;
		return 0;
	}

	uint8_t shift_mask = modifier & 0x22;
	bool one_shift_pressed = (shift_mask == 0x2) || (shift_mask == 0x20);

	if (data >= 0x4 && data <= 0x1D) {
		bool invert_case_now = invert_case;
		if (one_shift_pressed) {
			/* left or right shift pressed */
			invert_case_now = !invert_case_now;
		}

		return (invert_case_now ? 'A' : 'a') + (data - 0x04);
	}

	if (data >= 0x1E && data <= 0x27) {
		const char map[10][2] = {
			{'1', '!'},
			{'2', '@'},
			{'3', '#'},
			{'4', '$'},
			{'5', '%'},
			{'6', '^'},
			{'7', '&'},
			{'8', '*'},
			{'9', '('},
			{'0', ')'},
		};
		return map[data - 0x1E][one_shift_pressed ? 1 : 0];
	}

	if (data >= 0x28 && data <= 0x2C) {
		switch (data) {
		case 0x28: return '\n';
		case 0x29: return 0x1B; /* ESC: Escape */
		case 0x2A: return 0x08; /* BS: Backspace */
		case 0x2B: return '\t';
		case 0x2C: return ' ';
		default: return 0;
		}
	}

	if (data >= 0x2D && data <= 0x38) {
		const char map[12][2] = {
			{'-', '_'},
			{'=', '+'},
			{'[', '{'},
			{']', '}'},
			{'\\', '|'},
			{0, 0},
			{';', ':'},
			{'\'', '"'},
			{'`', '~'},
			{',', '<'},
			{'.', '>'},
			{'/', '?'},
		};
		return map[data - 0x2D][one_shift_pressed ? 1 : 0];
	}

	/* TODO: support all character */
	return 0;
}

static void print_converted_data(const uint8_t *data, unsigned len)
{
	const char *modifiers[] = {
		"CtrlL",
		"ShiftL",
		"AltL",
		"WinL",
		"CtrlR",
		"ShiftR",
		"AltR",
		"WinR"
	};

	LOG_MARKER
	usart_puts("KEYBOARD: ");
	uint8_t modif = data[0];
	const char *prepend = "";
	for (unsigned i = 0; i < 7; i++) {
		if (modif & (1 << i)) {
			usart_printf("%s%s", prepend, modifiers[i]);
			prepend = " + ";
		}
	}

	/* data[1] is reserved for vendor */

	for (unsigned i = 2; i < len; i++) {
		char ch = usbkbd_to_char(modif, data[i]);
		if (ch != 0) {
			usart_printf("%s%c", prepend, ch);
			prepend = " + ";
		}
	}

	usart_puts(NEW_LINE);
}

static void print_raw_data(const uint8_t *data, unsigned len)
{
	LOG_MARKER
	usart_puts("readed:");
	for (unsigned i = 0; i < len; i++) {
		usart_printf(" 0x%"PRIx8, data[i]);
	}

	usart_puts(NEW_LINE);
}

static void process_keyboard_data(const uint8_t *data, unsigned len)
{
	print_raw_data(data, len);
	print_converted_data(data, len);
}

uint8_t hid_iface_num, hid_iface_altset, hid_report_id = 0;
uint8_t  hid_ep_number;
uint16_t hid_ep_size, hid_ep_interval;

#define CLASS_HID 0x03
#define SUBCLASS_HID_BOOT 0x01
#define PROTOCOL_HID_KEYBOARD 0x01

static void readed_endpoint(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	switch (status) {
	case USBH_SUCCESS:
		process_keyboard_data(transfer->data, transfer->transferred);
	case USBH_ERR_TIMEOUT:
	case USBH_ERR_IO:
		/* resubmit! */
		LOG_LN("resubmitting transfer");
		usbh_transfer_submit(transfer);
	break;
	default:
		LOGF_LN("unknown error occured, %i", status);
	break;
	}
}

static void read_data_from_keyboard(usbh_device *dev)
{
	LOGF_LN("trying to read %"PRIu16" bytes for first time from 0x%"PRIx8,
		hid_ep_size, hid_ep_number);
	usbh_transfer ep_transfer = {
		.device = dev,
		.ep_type = USBH_EP_INTERRUPT,
		.ep_addr = hid_ep_number,
		.ep_size = hid_ep_size,
		.interval = hid_ep_interval,
		.data = buf,
		.length = hid_ep_size,
		.flags = USBH_FLAG_NONE,
		.timeout = 250,
		.callback = readed_endpoint,
	};

	usbh_transfer_submit(&ep_transfer);
}

static void after_set_idle(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set_idle (0) [maybe not supported]");
	} else {
		LOG_LN("set_idle (0) success");
	}

	read_data_from_keyboard(transfer->device);
}

static void after_set_protocol_boot(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set_protocol (boot)");
		return;
	}

	LOG_LN("set_protocol (boot) success");

	LOGF_LN("trying to SET_IDLE(0)");
	usbh_hid_set_idle(transfer->device, 0, hid_report_id, hid_iface_num,
		after_set_idle);
}

static void after_interface_set(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set interface, assuming the interface will work");
	} else {
		LOG_LN("set interface success, interface will work");
	}

	LOG_LN("performing SET_PROTOCOL(boot)");
	usbh_hid_set_protocol(transfer->device, USB_REQ_HID_PROTOCOL_BOOT,
		hid_iface_num, after_set_protocol_boot);
}

static void after_config_set(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to set configuration!");
		return;
	}

	LOGF_LN("trying to interface num=%"PRIu8", altset=%"PRIu8,
		hid_iface_num, hid_iface_altset);
	usbh_ctrlreq_set_interface(transfer->device, hid_iface_num,
			hid_iface_altset, after_interface_set);
}

static void got_conf_desc(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to read configuration descriptor 0!");
		return;
	}

	LOGF_LN("got %"PRIu16" bytes of configuration descriptor",
		transfer->transferred);


	struct usb_config_descriptor *cfg = transfer->data;
	if (!transfer->transferred || cfg->bLength < 4) {
		LOG_LN("descriptor is to small");
		return;
	}

	if (cfg->wTotalLength > transfer->transferred) {
		LOG_LN("descriptor only partially readed, "
			"what kind of keyboard have such a big descriptor?*%!");
		return;
	}

	int len = cfg->wTotalLength - cfg->bLength;
	void *ptr = transfer->data + cfg->bLength;
	struct usb_interface_descriptor *iface_interest = NULL;
	struct usb_endpoint_descriptor *ep_interest = NULL;

	while (len > 0) {
		uint8_t *d = ptr;
		LOGF_LN("trying: descriptor with length = %"PRIu8
			" and type = 0x%"PRIx8, d[0], d[1]);

		switch (d[1]) {
		case USB_DT_INTERFACE: {
			struct usb_interface_descriptor *iface = ptr;
			if (iface->bInterfaceClass == CLASS_HID &&
				iface->bInterfaceSubClass == SUBCLASS_HID_BOOT &&
				iface->bInterfaceProtocol == PROTOCOL_HID_KEYBOARD &&
				iface->bNumEndpoints) {
				iface_interest = iface;
				LOGF_LN("interface %"PRIu8" (alt set: %"PRIu8") "
					"could be of interest",
					iface->bInterfaceNumber, iface->bAlternateSetting);
			} else {
				iface_interest = NULL;
			}

			ep_interest = NULL;
		} break;
		case USB_DT_ENDPOINT: {
			if (iface_interest == NULL) {
				break;
			}

			struct usb_endpoint_descriptor *ep = ptr;
			if ((ep->bmAttributes & USB_ENDPOINT_ATTR_TYPE) ==
						USB_ENDPOINT_ATTR_INTERRUPT &&
					(ep->bEndpointAddress & 0x80)) {
				ep_interest = ep;
				break;
			}
		} break;
		}

		if (iface_interest != NULL && ep_interest != NULL) {
			break;
		}

		ptr += d[0];
		len -= d[0];
	}

	if (iface_interest == NULL || ep_interest == NULL) {
		LOG_LN("no valid keyboard interface");
		return;
	}

	LOG_LN("got a valid keyboard interface");
	hid_iface_num = iface_interest->bInterfaceNumber;
	hid_iface_altset = iface_interest->bAlternateSetting;
	hid_ep_number = ep_interest->bEndpointAddress;
	hid_ep_size = ep_interest->wMaxPacketSize;
	hid_ep_interval = ep_interest->bInterval;

	LOGF_LN("trying to set config: %"PRIu8, cfg->bConfigurationValue);
	usbh_ctrlreq_set_config(transfer->device, cfg->bConfigurationValue,
			after_config_set);
}

static void got_dev_desc(const usbh_transfer *transfer,
	usbh_transfer_status status, usbh_urb_id urb_id)
{
	(void) urb_id;

	if (status != USBH_SUCCESS) {
		LOG_LN("failed to read device descriptor!");
		return;
	}

	struct usb_device_descriptor *desc = transfer->data;

	if (!desc->bNumConfigurations) {
		LOG_LN("device do not have any configuration"
			"**THROW IT OUT OF THE WINDOW!**");
		return;
	}

	uint8_t class = desc->bDeviceClass;
	uint8_t subclass = desc->bDeviceSubClass;
	uint8_t protocol = desc->bDeviceProtocol;

	LOGF_LN("bDeviceClass: 0x%"PRIx32, class);
	LOGF_LN("bDeviceSubClass: 0x%"PRIx32, subclass);
	LOGF_LN("bDeviceProtocol: 0x%"PRIx32, protocol);

	if (class == CLASS_HID && subclass == SUBCLASS_HID_BOOT &&
			protocol == PROTOCOL_HID_KEYBOARD) {
		LOG_LN("device is a keyboard of our interest");
	} else if (!class || !subclass || !protocol) {
		LOG_LN("have to look into configuration descriptor");
	} else {
		LOG_LN("device is not a keyboard");
		return;
	}

	usbh_device *dev = transfer->device;
	usbh_ctrlreq_read_desc(dev, USB_DT_CONFIGURATION, 0, buf, 128, got_conf_desc);
}

/**
 * when a device is connected, the following sequence is followed
 *  - fetch full device descriptor
 *  [search for valid keyboard device]
 *  - fetch configuration descriptor
 *  [search for valid keyboard interface]
 *  - set configuration
 *  - set interface
 *  - set protocol (boot)
 *  - set idle (0)
 *  - read from endpoint
 */

static void device_disconnected(usbh_device *dev)
{
	(void) dev;

	LOG_LN("keyboard disconnected!");
}

static void got_a_device(usbh_device *dev)
{
	LOG_LN("finally got a device!");
	usbh_device_register_disconnected_callback(dev, device_disconnected);
	usbh_ctrlreq_read_desc(dev, USB_DT_DEVICE, 0, buf, 18, got_dev_desc);
}

int main(void)
{
	clock_setup();
	gpio_setup();

	tim_setup();

	usart_init();
	usart_puts("\n\n\n\n======STARTING=============\n");

	gpio_set(GPIOD,  GPIO13);

	usbh_host *host = usbh_init(USBH_STM32_OTG_FS);
	usbh_register_connected_callback(host, got_a_device);

	gpio_clear(GPIOD,  GPIO13);

	uint16_t last = tim_get_counter();

	while (1) {
		/* Set busy led */
		gpio_set(GPIOD,  GPIO14);

		uint16_t now = tim_get_counter();
		uint16_t diff = (last > now) ? (now + (0xFFFF - last)) : (now - last);
		uint32_t diff_us = diff * 100;

		usbh_poll(host, diff_us);

		last = now;

		/* Clear busy led */
		gpio_clear(GPIOD,  GPIO14);

		/* dummy delay, approx 1ms interval */
		for (volatile uint32_t i = 0; i < 14903; i++) {}
	}

	return 0;
}
