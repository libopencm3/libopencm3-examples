# README

This example demonstrates the following:
 * Using the USB controller
 * Setting up a simple usb to serial converter (CDCACM)

File list:
 * `usb_cdcacm.c` - implementation of the CDCACM subclass
 * `uart.c` - implementation of UART peripheral
 * `usb_to_serial_cdcacm.c` - glue logic between UART and CDCACM device
 * `usb_to_serial_cdcacm.h` - common definitions


Implements a USB-to-serial adapter, compliant to the CDCACM subclass. UART1 is
used for the TX/RX lines. Although UART1 also has lines for flow control, they
are not used. Instead, the control lines are implemented in software via GPIOA.

The following pinout is used:
	Tx  <-  PB1
	Rx   -> PB0
	DCD  -> PA2
	DSR  -> PA3
	RI   -> PA4
	CTS  -> PA5 (UNUSED)
	DTR <-  PA6
	RTS <-  PA7

Note that the CTS pin is unused. The CDCACM specification does not define a way
to control this pin, nor does it define a way to switch between flow control
mechanisms.

The glue logic in `usb_to_serial_cdcacm.c` receives requests from both the UART
and CDCACM interface, and forward them to their destination, while also
controlling the LEDs

The green LED is lit as long as either DTR or RTS are high.
The red LED is lit while the UART is sending data.
The blue LED is lit while data is read from the UART.
The red and blue LEDs will only be lit for very short periods of time, thus they
may be difficult to notice.

## Windows Quirks
On openening the CDCACM port Windows send a `SET_LINE_CODING` request with the
desired baud rate but without valid databits. To run this example CDDACM device
under Windows you have to return always 1 when a `SET_LINE_CODING` request is
received. The following code should work:

File: `usb_cdcacm.c`
Function: `cdcacm_control_request()`

	case USB_CDC_REQ_SET_LINE_CODING:{
			struct usb_cdc_line_coding *coding;
	
			if (*len < sizeof(struct usb_cdc_line_coding))
			{
				return 0;
			}
	
			coding = (struct usb_cdc_line_coding *)*buf;
			glue_set_line_coding_cb(coding->dwDTERate,
						coding->bDataBits,
						coding->bParityType,
						coding->bCharFormat);
			return 1;

