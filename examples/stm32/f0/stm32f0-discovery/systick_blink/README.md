------------------------------------------------------------------------------
README
------------------------------------------------------------------------------

This example is the same as fancy_blink except that it uses the systick timer
to generate time accurate delays. The blue LED flashes four times per second.

There is internal reference clock available on MCO output pin. This can be used
to debug the PLL clock setup by scope.

Board connections:
------------------

  PA9	(MCO)	Internal reference clock
