# README

This example program echoes data sent in on USART2 on the
ST STM32F4DISCOVERY eval board. Uses interrupts for that purpose.

The sending is done in a nonblocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA2` | `(USART2_TX)` | TTL serial output `(38400,8,N,1)` |
| `PA3` | `(USART2_RX)` | TTL serial input `(38400,8,N,1)`  |
