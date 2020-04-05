# README

This example program echoes data sent in on USART3 on the
ST STM32F469IDISCOVERY eval board. Uses interrupts for that purpose.

The sending is done in a nonblocking way.

USART3 is used as it is already connected internally to the ST/Link port
and shows up as a serial port on the host computer connected to the board.

## Board connections

Connection to the ST/Link USB port, appropriate USB serial drivers from ST Micro.

## Notes

You can connect the USART to the stlink by closing the SB11 and SB15 jumpers.
