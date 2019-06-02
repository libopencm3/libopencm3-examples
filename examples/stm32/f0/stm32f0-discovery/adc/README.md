# README

It's intended for the ST STM32F0DISCOVERY eval board. Measures voltage on the 
ADC\_IN1 input, and prints it to the serial port.

## Board connections

| Port  | Function    | Description                       |
| ----- | ----------- | --------------------------------- |
| `PA1` | `(ADC_IN1)` | Analog input                      |
| `PA9` | `(USART1)`  | TTL serial output `(115200,8,N,1)` |
