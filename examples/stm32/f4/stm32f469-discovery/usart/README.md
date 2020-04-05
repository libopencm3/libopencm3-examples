# README

This example program sends repeating sequence of characters "0123456789" on 
USART3 on the ST STM32F469IDISCOVERY eval board.

USART3 (GPIO Port B, pin 10) is connected to the ST/Link circuit and shows up
as a serial port on the host computer when it is connected. To observe the
characters open a program which connects to the serial port (PuTTy or HyperTerm
on Windows, GNU screen on Linux or Mac systems).

The sending is done in a blocking way. A new line is generated every 70 characters.

## Board connections

USB Connection to the ST/Link port

