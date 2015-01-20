# README

This example demonstrates the following:
* Setting up polled USB endpoints
* Setting up interrupt driven USB endpoints
* Using the UART as a debug tool

## USB module

Several USB endpoints are being set up:
* EP1 OUT - interrupt driven RX endpoint
* EP2 IN  - interrupt driven TX endpoint
* EP3 OUT - polled RX endpoint
* EP4 IN  - polled TX endpoint
* EP5 OUT - polled RX endpoint with unaligned buffer
* EP6 IN  - polled TX endpoint with unaligned buffer

These endpoints do not transfer any meaningful data. Instead, they try to push
data in and out as fast as possible by writing it to the FIFOs or reading it
from the FIFOs.

The interrupt driven endpoints only read or write a packet during a callback
from the USB driver. Since the USB driver is run entirely from the USB ISR,
these callbacks are essentially interrupt driven.

The polled endpoints try to continuously read and write data. Even though
usbd\_ep\_read/write\_packet is called continuously for these endpoints, the USB
driver will only write a packet to the TX FIFO if it is empty, and only read
a packet from the FIFO if one has arrived.

The endpoints with a misaligned buffer show the performance drop when the buffer
is not aligned to a 4 byte boundary. 32-bit memory accesses to the buffer are
downgraded to 8-bit accesses by the hardware.

## Clock change module

Pressing SW2 toggles the system clock between 80MHz, 57MHz, 40MHz, 30MHz, 20MHz,
and 16MHz by changing the PLL divisor.

Pressing SW1 bypasses the PLL completely, and runs off the raw 16MHz clock
provided by the external crystal oscillator.

Changing the system clock on-the-fly does not affect the USB peripheral. It is
possible to change the system clock while benchmarking the USB endpoint.

The current system clock is printed on the debug interface. This allows testing
the performance of the USB endpoints under different clocks.

## Debug module

printf() support is provided via UART0. The UART0 pins are connected to the
CDCACM interface on the ICDI chip, so no extra hardware is necessary to check
the debug output. Just connect the debug USB cable and use a terminal program to
open the ACM port with 921600-8N1.

For example:
    picocom /dev/ttyACM0 -b921600
