Standard I/O Example
--------------------

This just shows using the native retargeting capability of
the ARM GCC Embedded compiler. By linking in retarget,
console, and clock. The startup script will call SystemInit
in retarget automatically and set things up before main()
is called, in particular the serial port is set up as the
standard input, output, and error streams for the program
and the clock is setup to be 168Mhz (based on the STM32F469-Discovery's
external (HSE) crystal setup.

Control-C is enabled so when the example is running, if you 
hold the control key and C the board will reset and restart. 

## Notes ##

Often time people do not use the standard libraries (especially printf!)
because they include a lot of code that usually is not needed. However
since the F4 chip on our board has 2 megabytes of flash, there is space
to have it, and once you pay the price for including it, you can use it
as often as you like without adding additional space requirements.
