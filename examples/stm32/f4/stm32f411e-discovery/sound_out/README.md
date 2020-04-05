# README

This example demonstrates using the oboard audio DAC on
STM32F411-Discovery board.
It plays a continuous 1 kHz sine wave.

## Board connections

Sepeakers attached to the 3.5mm audio jack.
Note: volume is turned low, but beware of loud sounds
in case of bugs (i.e. plug the headphones to your ear
only after playback starts!)

## DMA mode

There is a compile-time option (#define USE_DMA) that
demonstrates how to have the DMA engine write audio
data to the DAC.

## LEDs

Both versions of the program blink the orange
led (PD13) at ~0.5Hz. The DMA version toggles the
green led (PD12) on every DMA interrupt, which means
the pattern is visible with oscilloscope only (500Hz).
