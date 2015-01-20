# README

DAC test with DMA and timer 2 trigger

Timer 2 is setup to provide a trigger signal on OC1, with a period of 142 Hz

The DAC is setup on channel 1 to output a sample on the timer trigger.

DMA controller 1, stream 5, channel 7 is used to move data from a
predefined array in circular mode when the DAC requests.

DMA transfer complete occurs when the data array has been passed through.
In the ISR port PC1 is toggled to provide a CRO trigger.

The analogue output appears on PA4 (DAC channel 1).

Tested and working, example capture from an oscilloscope is included

Ken Sarkies 15/01/2014
