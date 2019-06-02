# README

This example program sends some characters on USART1 (PA9) on the
ST STM32VLDISCOVERY eval board.

The terminal settings for the receiving device/PC are 115200 8n1.

The sending is done in a blocking way in the code, see the usart\_irq example
for a more elaborate USART example.

