# README

Getting started with I2C project for a bare-bones stm32f030 board with 20-pin TSOP package.  Driving an MCP4725 DAC over I2C.

## Board connections

| Pin | Port          | Function              | Description                   |
| --- | ------------- | --------------------- | ----------------------------- |
|  1  | `(BOOT0)`     | Boot mode selection   | Pull low                      |
|  4  | `(NRST)`      | Reset                 | Pull high                     |
|  5  | `(VDDA)`      | Analog power          | Connect to V+                 |
| 15  | `(VSS)`       | Ground pin            | Connect to ground             |
| 16  | `(VDD)`       | Logic power           | Connect to V+                 |
| 17  | `(SCL)`       | I2C clock             | Connect to DAC SCL            |
| 18  | `(SDA)`       | I2C data              | Connect to DAC SDA            |
| 19  | `(SYS_SWDIO)` | Programming interface | Connect to ST-Link programmer |
| 20  | `(SYS_SWCLK)` | Programming interface | Connect to ST-Link programmer |
