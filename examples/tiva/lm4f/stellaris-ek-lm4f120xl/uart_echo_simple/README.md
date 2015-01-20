# README

This example demonstrates the ease of setting up the UART with libopencm3.
Basic UART echo is achieved by using blocking reads and writes. The UART is set
up as 921600-8N1.
PA0 is the Rx pin, and PA1 is the Tx pin (from the LM4F perspective). These
pins are connected to the CDCACM interface on the debug chip, so no hardware is
necessary to test this example. Just connect the debug USB cable and use a
terminal program to open the ACM port with 921600-8N1.

For example:
    picocom /dev/ttyACM0 -b921600
