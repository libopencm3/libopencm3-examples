# README

Minimal getting started project for a bare-bones stm32f030 board with 20-pin TSOP package.

## Board connections

| Pin | Port          | Function              | Description                   |
| --- | ------------- | --------------------- | ----------------------------- |
|  1  | `(BOOT0)`     | Boot mode selection   | Pull low                      |
|  4  | `(NRST)`      | Reset                 | Pull high                     |
|  5  | `(VDDA)`      | Analog power          | Connect to V+                 |
| 11  | `(PA5)`       | GPIO                  | Drives LED output             |
| 15  | `(VSS)`       | Ground pin            | Connect to ground             |
| 16  | `(VDD)`       | Logic power           | Connect to V+                 |
| 19  | `(SYS_SWDIO)` | Programming interface | Connect to ST-Link programmer |
| 20  | `(SYS_SWCLK)` | Programming interface | Connect to ST-Link programmer |
