# README

This example program sends repeating sequence of characters "0123456789" on 
USART2 on the ST STM32F4DISCOVERY eval board.

The sending is done in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA2` | `(USART2_TX)` | TTL serial output `(38400,8,N,1)` |
