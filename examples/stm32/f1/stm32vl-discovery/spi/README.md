# README

This example program demonstrates simple SPI transceive on stm32vl discovery board.

The terminal settings for the receiving device/PC are 9600 8n1.

The example expects a loopback connection between the MISO and MOSI pins on
SPI1.

A counter increments and the spi sends this byte out, after which it should
come back in to the rx buffer, and the result is reported on the uart.
