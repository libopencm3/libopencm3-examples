# README

[![Gitter channel](https://badges.gitter.im/libopencm3/discuss.svg)](https://gitter.im/libopencm3/discuss)

This repository contains assorted example projects for libopencm3.

The libopencm3 project aims to create an open-source firmware library for
various ARM Cortex-M microcontrollers.

For more information visit http://libopencm3.org

The examples are meant as starting points for different subsystems on multitude
of platforms.

Feel free to add new examples and send them to us either via the mailinglist or
preferably via a github pull request.

## Usage

You _must_ run "make" in the top level directory first.  This builds the
library and all examples.  If you're simply hacking on a single example after
that, you can type "make clean; make" in any of the individual project
directories later.

For more verbose output, to see compiler command lines, use "make V=1"
For insanity levels of verboseness, use "make V=99"

The makefiles are generally useable for your own projects with
only minimal changes for the libopencm3 install path (See Reuse)

## Make Flash Target
For flashing the 'miniblink' example (after you built libopencm3 and the
examples by typing 'make' at the top-level directory) onto the Olimex
STM32-H103 eval board (ST STM32F1 series microcontroller), you can execute:

    cd examples/stm32/f1/stm32-h103/miniblink
    make flash

The Makefiles of the examples are configured to use a certain OpenOCD
flash programmer, you might need to change some of the variables in the
Makefile if you use a different one.

The make flash target also supports a few other programmers. If you provide the
Black Magic Probe serial port the target will automatically choose to program
via Black Magic Probe. For example on linux you would do the following:

    cd examples/stm32/f1/stm32-h103/miniblink
    make flash BMP_PORT=/dev/ttyACM0

This will also work with discovery boards that got the st-link firmware
replaced with the Black Magic Probe firmware.

In case you did not replace the firmware you can program using the st-flash
program by invoking the stlink-flash target:

    cd examples/stm32/f1/stm32vl-discovery/miniblink
    make miniblink.stlink-flash


If you rather use GDB to connect to the st-util you can provide the STLINK\_PORT
to the flash target.

    cd examples/stm32/f1/stm32vl-discovery/miniblink
    make flash STLINK_PORT=:4242

## Flashing Manually
You can also flash manually. Using a miriad of different tools depending on
your setup. Here are a few examples.

### OpenOCD

    openocd -f interface/jtagkey-tiny.cfg -f target/stm32f1x.cfg
    telnet localhost 4444
    reset halt
    flash write_image erase foobar.hex
    reset

Replace the "jtagkey-tiny.cfg" with whatever JTAG device you are using, and/or
replace "stm32f1x.cfg" with your respective config file. Replace "foobar.hex"
with the file name of the image you want to flash.

### Black Magic Probe

    cd examples/stm32/f1/stm32vl-discovery/miniblink
    arm-none-eabi-gdb miniblink.elf
    target extended_remote /dev/ttyACM0
    monitor swdp_scan
    attach 1
    load
    run

To exit the gdb session type `<Ctrl>-C` and `<Ctrl>-D`. It is useful to add the
following to the .gdbinit to make the flashing and debugging easier:

    set target-async on
    set confirm off
    set mem inaccessible-by-default off
    #set debug remote 1
    tar ext /dev/ttyACM0
    mon version
    mon swdp_scan
    att 1

Having this in your .gdbinit boils down the flashing/debugging process to:

    cd examples/stm32/f1/stm32vl-discovery/miniblink
    arm-none-eabi-gdb miniblink.elf
    load
    run

### ST-Link (st-util)

This example uses the st-util by texane that you can find on [GitHub](https://github.com/texane/stlink).

    cd examples/stm32/f1/stm32vl-discovery/miniblink
    arm-none-eabi-gdb miniblink.elf
    tar extended-remote :4242
    load
    run

## Reuse

If you want to use libopencm3 in your own project, this examples repository
shows the general way.  (If there's interest, we can make a stub template
repository)

1. Create an empty repository

       mkdir mycoolrobot && cd mycoolrobot && git init .

2. Add libopencm3 as a submodule

       git submodule add https://github.com/libopencm3/libopencm3
    

3. Grab a copy of the basic rules
These urls grab the latest from the libopencm3-examples repository

       wget \
         https://raw.githubusercontent.com/libopencm3/libopencm3-examples/master/examples/Makefile.rules \
         -O libopencm3.rules.mk

4. Grab a copy of your target Makefile in this case, for STM32L1

       wget \  
         https://raw.githubusercontent.com/libopencm3/libopencm3-examples/master/examples/stm32/l1/Makefile.include \  
         -O libopencm3.target.mk

5. Edit paths in `libopencm3.target.mk`  
Edit the _last_ line of `libopencm3.target.mk` and change the include to read
include `../libopencm3.rules.mk` (the amount of .. depends on where you put your
project in the next step..

6. beg/borrow/steal an example project
For sanity's sake, use the same target as the makefile you grabbed up above)

       cp -a \
         somewhere/libopencm3-examples/examples/stm32/l1/stm32ldiscovery/miniblink \
         myproject

Add the path to OPENCM3\_DIR, and modify the path to makefile include


    diff -u
    ---
    2014-01-24 21:10:52.687477831 +0000
    +++ Makefile    2014-03-23 12:27:57.696088076 +0000
    @@ -19,7 +19,8 @@
     
     BINARY = miniblink
     
    +OPENCM3_DIR=../libopencm3
     LDSCRIPT = $(OPENCM3_DIR)/lib/stm32/l1/stm32l15xxb.ld
     
    -include ../../Makefile.include
    +include ../libopencm3.target.mk
 
You're done :)

You need to run "make" inside the libopencm3 directory once to build the
library, then you can just run make/make clean in your project directory as
often as you like.
