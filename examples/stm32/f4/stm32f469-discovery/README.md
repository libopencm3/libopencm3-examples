Examples for the STM32F469-Discovery board
------------------------------------------

This directory contains various examples which use libopencm3 and run on
the [ST Micro STM32F469-Discovery][disco] board. This board has a number
of on board peripherals such as a MicroSD card slot, USB OTG connector,
MEMS microphones, 800 x 450 touch screen graphics, and four user LEDs.

Additionally the board is loaded with the ST/Link V2.1-1 software which is
[mBed][] compatible. That means it shows up as a debug device, a disk device,
and a serial port on the host machine. The examples showing use of the serial
port configure this port so that additional hardware is not needed.

[disco]: http://www.st.com/web/catalog/tools/FM116/CL1620/SC959/SS1532/LN1848/PF262395
[mBed]: http://mbed.org/
