# README

This example program echoes data sent in on USART1 on the
ST STM32F429IDISCOVERY eval board. Uses interrupts for that purpose.

The sending is done in a nonblocking way.

## Board connections

| Port   | Function      | Description                       |
| ------ | ------------- | --------------------------------- |
| `PA9`  | `(USART1_TX)` | TTL serial output `(115200,8,N,1)` |
| `PA10` | `(USART1_RX)` | TTL serial input `(115200,8,N,1)`  |

## Notes

You can connect the USART to the stlink by closing the SB11 and SB15 jumpers.
