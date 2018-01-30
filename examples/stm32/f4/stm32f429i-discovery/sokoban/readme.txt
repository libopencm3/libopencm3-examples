/**************************************************************************
 * Game of Sokoban for kit STM32F429 Discovery using gcc and libopencm3
 * (c) Marcos Augusto Stemmer
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
****************************************************************************/
This program is made for the ST-Microelectronics STM32F29 Discovery development kit
using the library libopencm3.
Besides being fun to play, this program include some usefull support modules such as
USART with interrupts, mprintf (a lighter version of prinf), ili9341.c: LCD setup and
basic graphics library. The module i3c3_touch.c uses i2c3 to read STMP811 touch-screen.
The module sdram.c, written by Chuck McManis <cmcmanis@mcmanis.com>, was taken unmodified.
The Makefile was made to support compilation both in posix (Linux, Unix, Mac) 
and in Windows command line.

The Makefile lack any method to automatically detect the location of the library libopencm3.
So, to compile, you need to check OPENCM3_DIR in the Makefile.

===============================
Requisites to compile and test
==============================

STM32F429i Discovery developement kit.

C compiler toolset for Cortex M3
arm-none-eabi-gcc

Native PC gcc - C compiler. Windows users may install
MinGW (http://sourceforge.net/mingw)

Flash programming tool:  st-link.

Function library: libopencm3.

In Linux, the library libopencm3 is expected to be in directory
../../libopencm3
In Windows it is expected to be in
c:\arm-none-eabi\lib\libopencm3

To compile an program flash:
make flash


==========
To play:
==========
Touch where you want the guy to move.
If there is a box in the same line of movement it will be pushed.

Regrettable pushes may be undone by pressing the blue button.
