# README

This example program sends repeating sequence of characters "0123456789" on
USART2 serial line of ST NUCLEO-G031K8 eval board.

The sending is done in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA2` | `(USART2_TX)`	| TTL serial output `(115200,8,N,1)` |
