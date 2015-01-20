# README

This example program demonstrates the floating point coprocessor usage on
the ST STM32F4DISCOVERY eval board.

A mandelbrot fractal is calculated and sent as "ascii-art" image through
the USART2.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA2` | `(USART2_TX)` | TTL serial output `(38400,8,N,1)` |
