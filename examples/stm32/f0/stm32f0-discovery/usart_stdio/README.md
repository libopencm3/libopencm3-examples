# README

This example program sends a message "Pass: n" with increasing number n
from 0 to 200 on USART1 serial line of ST STM32F0DISCOVERY eval board.

The sending is done using newlib library in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA9` | `(USART1_TX)` | TTL serial output `(115200,8,N,1)` |
