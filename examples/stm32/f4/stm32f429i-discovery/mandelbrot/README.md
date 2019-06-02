# README

This example program demonstrates the floating point coprocessor usage on
the ST STM32F429IDISCOVERY eval board.

A mandelbrot fractal is calculated and sent as "ascii-art" image through
the USART1.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA9` | `(USART1_TX)` | TTL serial output `(115200,8,N,1)` |
