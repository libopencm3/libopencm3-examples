# README

This example program sends some characters on USART2 on
[Lisa/M 2.0 board](see http://paparazzi.enac.fr/wiki/LisaM for details).

The terminal settings for the receiving device/PC are 115200 8n1.

The sending is done in a blocking way in the code, see the usart\_irq example
for a more elaborate USART example.

