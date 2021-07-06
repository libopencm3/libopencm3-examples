# README

It's intended for naked STM32F030F4 MCU and I2C flash memory Atmel 24C02. The I2C bus is connected to flash memory Atmel 24C02 (but many other memories will do, consult datasheet) - [http://ww1.microchip.com/downloads/en/devicedoc/doc0180.pdf]
The application programs bytes 0x01 and 0x02 with values 0x42 and 0x77 respectively, reads the values back and prints them to the serial port.

## Board connections

| Port   | Function   | Description                  |
| ------ | ---------- | ---------------------------- |
| `PA2`  | `(USART1)` | Serial output (115200,8,N,1) |
| `PA9`  | `(SCL)`    | I2C connection               |
| `PA10` | `(SDA)`    | I2C connection               |

The I2C bus requires pullup resistors (e.g. 4k7), this application uses dirty workaround switching on internal pullups.