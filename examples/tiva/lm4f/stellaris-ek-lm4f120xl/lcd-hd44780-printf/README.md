# README

This example demonstrates interfacing the Stellaris board to any HD44780 style
LCD display, using an 8-bit data bus. It demonstrates the use of LCDs or VFDs
(Vacuum Fluorescent Displays) driven by a HD44780 controller. It also
demonstrates the use of printf-like functionality to print data to the display.

While this example has been split up in component parts, it is complex, and
contains many parts. If you're only looking for an example on how to use the
UART, see uart_echo_simple, and uart_echo_interrupt examples instead.

## HD44780 diplay control

The HD44780 interface is split into two parts:

### hd44780-gpio.[ch]

These files provide a generic, object-oriented interface to talk to HD44780
display controllers. It abstracts the communication interface into several easy
to use functions, ranging from low-level control of the communication lines, to
high-level control of the data to be displayed. The driver is named
hd44780-gpio because the communication is driven via GPIO (as opposed to using
an external bus).

Hardware-specific details, such as driving the GPIO lines are not implemented
here, but are instead outsourced to hardware-specific drivers. A
hardware-specific driver can be provided by filling up a hd44870_lcd structure.
This structure is also the context for every function in the generic display
code.

The hd44780-gpio aims to be a minimalistic driver, and does not aim to impement
all the bells and whistles of the HD44780 controller, nor is it guaranteed to
work on all display geometries (but it's pretty easy to adapt).

### stellaris_vfd.c

This file implements the hardware-specific part of the display driver. It
provides initialization of the GPIO lines, and controls the communication lines
leading to the display. It also implements other hardware details, such as
delays, and details about the geometry of the display.

## UART control

This example uses UART0 send and receive data. It also echoes back any data
received. The echo happens within the UART interrupt routine. See
uart_echo_interrupt for a simpler example on using UART interrupts.

The uart.c file also keeps tracks of the numbers of received and sent bytes,
and provides a way to report them to the application, as well as reset the
counters.

## Main application

the application prints the "libopencm3" text on the first line of the display,
followed by two special characters. The number of bytes sent and received over
UART0 are displayed on the second line. The display is updated whenever one of
the counters changes.

The main application demonstrates how to use printf-like functionality to send
data over the UART. It then demonstrates how to put simple strings on the
display, and use the CGRAM (character generation RAM) of the HD44780 to create
custom characters.
The program's event loop demonstrates how to partition the display into
subsections for displaying different information, using lcd_nprintf(), and
lcd_printf(). With lcd_nprintf(), it is possible to print text to the LCD and
confine it to a specified number of characters.

## UART

PA0 is the Rx pin, and PA1 is the Tx pin (from the LM4F perspective). These
pins are connected to the CDCACM interface on the debug chip, so no hardware is
necessary to test this example. Just connect the debug USB cable and use a
terminal program to open the ACM port with 921600-8N1.

For example:
    picocom /dev/ttyACM0 -b921600

## Connections
	display data[0-7] 	<-> PB[0-7]
	display FNC		 -> PD4 (unused)
	display RS		 -> PD5
	display RW		 -> PD6
	display E		 -> PD7
	UART0 TX		 -> PA0
	UART0 RX		<-  PA1

These connections were chosen such that writing to the data bus can be done
with a single GPIO write. This is, however, not required, and GPIOs from
different ports may be mixed to create the 8-bit data bus. Such changes should
be reflected in the hardware-specific driver in stellaris_vfd.c .

## Hardware

This example was developed using a Noritake CU20025ECPB-U1J 2x20 vacuum
fluorescent display.

