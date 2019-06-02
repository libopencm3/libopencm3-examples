# README

This example program sends some characters on USART2 on the
ST STM32L DISCOVERY eval board. (USART2 TX on PA2)

The terminal settings for the receiving device/PC are 115200 8n1.

The sending is done in a blocking way in the code, see the usart\_irq example
for a more elaborate USART example.

