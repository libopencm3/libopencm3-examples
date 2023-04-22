# README

Example code that demonstrates the reading and writing of flash 
at runtime.

For this we add a custom section in the linker file that defines
which section of flash we will use to write to.

The linker file will provide the C code with a variable that is used
to get the address of our flash section.

The example will read the variable from flash, write it to the LED's, 
then it will increase the counter and write it back to flash.

This will cause the LED pattern to change at every reset or power cycle.

NOTE: The flash memory in stm32 chips has a limited durability of
about 10.000 erase/write cycles, after which it may become unreliable.
Keep this in mind when designing your application.

