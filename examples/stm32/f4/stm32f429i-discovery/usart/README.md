# README

This example program sends repeating sequence of characters "0123456789" on 
USART1 on the ST STM32F429IDISCOVERY eval board.

The sending is done in a blocking way.

## Board connections

| Port  | Function      | Description                       |
| ----- | ------------- | --------------------------------- |
| `PA9` | `(USART1_TX)` | TTL serial output `(115200,8,N,1)` |

## Notes

You can connect the UART to the stlink by closing the SB11 and SB15 solder
jumpers.
