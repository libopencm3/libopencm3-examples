# README

This example program sends a message "Pass: n" with increasing number n
from 0 to 200 on USART2 serial line of ST NUCLEO-G031K8 eval board.

The sending is done using newlib library in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA2` | `(USART2_TX)` | TTL serial output `(115200,8,N,1)` |
