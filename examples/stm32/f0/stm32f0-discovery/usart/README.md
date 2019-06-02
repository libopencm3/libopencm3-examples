# README

This example program sends repeating sequence of characters "0123456789" on 
USART1 serial line of ST STM32F0DISCOVERY eval board.

The sending is done in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA9` | `(USART1_TX)`	| TTL serial output `(115200,8,N,1)` |
