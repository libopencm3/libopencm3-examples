# README

This example program sends some characters on USART2 on the
ST STM32L DISCOVERY eval board. (USART2 TX on PA2 @ 115200 8n1)

It can _ALSO_ use semihosting to use regular stdio via the attached debugger.
This expects you to be using gcc-arm-embedded from
https://launchpad.net/gcc-arm-embedded

Semihosting is a neat feature, but remember that your application will
NOT WORK standalone if you have semihosting turned on!

    $ make ENABLE_SEMIHOSTING=0 will rebuild this image _without_ semihosting

Semihosting is supported in "recent"[1] OpenOCD versions, however, you need
to enable semihosting first!  If you have not enabled semihosting, you
will receive a message like this:

    (gdb) run
    The program being debugged has been started already.
    Start it from the beginning? (y or n) y
    
    Starting program:
    /home/karlp/src/libopencm3-examples/examples/stm32/l1/stm32l-discovery/usart-semihosting/usart-semihosting.elf 
    
    Program received signal SIGTRAP, Trace/breakpoint trap.
    0x08000456 in initialise_monitor_handles ()
    (gdb)
    
    # Here we enable semi-hosting
    
    (gdb) mon arm semihosting enable
    semihosting is enabled
    (gdb) continue
    ...

You should now see the semihosting output in the window running OpenOCD.

## Size Notes

Semihosting is basically free

### Without
    $ arm-none-eabi-size usart-semihosting.elf 
       text	   data	    bss	    dec	    hex	filename
      29056	   2212	     60	  31328	   7a60	usart-semihosting.elf

### With
    $ arm-none-eabi-size usart-semihosting.elf 
       text	   data	    bss	    dec	    hex	filename
      29832	   2212	    232	  32276	   7e14	usart-semihosting.elf

The large size here is because of printf being included regardless, see
nano.specs if you care about this, this data here is to show that semihosting
doesn't cost much at all.


[1] At least since OpenOcd version 0.8.0-dev-00011-g70a2ffa (2013-05-14-19:41)
possibly earlier.
