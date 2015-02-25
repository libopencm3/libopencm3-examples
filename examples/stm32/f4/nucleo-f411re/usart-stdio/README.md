# README

This program demonstrates using the standard I/O and library
functions built into newlib on the Nucleo F411RE eval board.

Normally the functions would seem to be extravagant for an
embedded application but the F4 series has a lot of flash
(512K in the case of the STM32F411RE) and so you can use this
to make it easier to work with.

This example asks you to enter a number, 1 or greater, and
it uses that number as the delay constant when blinking the LED
on the Nucleo board.  It blinks the LED 1000 times so if you
choose a large number it will take a while to finish.

100000 is reasonably fast, 500000 is quite slow.

There is also a simple buffered input routine in the code so
you can edit the number as you are entering it. The ^H or DEL
keys will delete a character, ^U will erase the line, and
^W will delete the last word (defined by space characters).
