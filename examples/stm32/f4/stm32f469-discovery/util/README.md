Utility Functions
-----------------

Not part of the library, these utility functions provide some
helper functions for the examples that simplify things.

**clock.c** - sets up the clock to 168Mhz and and enables the
	SysTick interrupt with 1khz interrupts.

**console.c** - a set of convienience routines for using the debug
	serial port (USART3) which is availble as /dev/ttyACM0 on
	the linux box, as a console port.

**retarget.c** - implements the minimum set of character I/O functions
	and automatically plumbs them so that you can use standard
	functions like printf() in your programs.
