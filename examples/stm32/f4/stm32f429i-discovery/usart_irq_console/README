USART IRQ
---------

This is a slightly fancier console interface using interrupts. Basically
this example sets up the USART for serial input and output as before
except that the receive side is interrupt driven. That means you program
won't miss a character if it happens to be taking its time printing something
at the time.

I've demonstrated this by setting it up so that if you type ^C to the
program it causes an interrupt to occur that resets the program back
to the start. This is done in a slightly tricky way to accomodate the
Cortex M architecture. When you are interrupted in the Cortex M, the 
proccessor goes into "Handler" mode, saves information on the stack,
and takes the interrupt. Part of this involves saving a special value
on the stack in the LR register. The useful thing is that it means you
can write interrupt service routines as regular C code, the not so useful
thing is that if you don't return from that stack, the processor gets
confused about what state it should be in. So to avoid confusing the
processor, the interrupt routine changes where the code will return, then
does the return. This pops the processor out of Handler mode and back into
Thread mode, at which point the C code can do a longjump.

In the future, the C library may be able to note that the processor needs
to change state for you, and save the special gymnastics here, but in the
mean time this works and will continue to work in the future.

