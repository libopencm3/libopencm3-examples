# README

Demonstration on how to put the STM32 in stop mode and wake it from an interrupt on PA0 (button press).

This programs can be easily modified to use other low-power modes: sleep, stop and stanby.

## Board connections
A push button between 3.3V to PA0


## Synopsis
- LED blinks a few times
- STM32 goes to stop mode (LED is off)
- Rising edge on PA0 wakes STM32 (press the button)
- LED stays on for a while
- Go to beginning (start again)

