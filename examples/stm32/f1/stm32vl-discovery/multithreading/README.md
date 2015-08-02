# README

This example shows the simplest way to do cooperative multithreading and context
switches on an ARM Cortex-M core without involving an entire RTOS.

The code runs on the STM32F100RBT6B on an STM32 Value line discovery evaluation
board.

A coopeative (non-preemptive) multithreading scheduler like this can be pretty
useful when interleaving several processes that require some state, such as
reading and parsing data. Now that state can be implicitly stored in control
flow and local variables instead of having to be serialized into a context
structure.

The example shows two tasks that run continuously, yielding to each other. Each
task controls pulsewidth modulation of an LED and the visible result is that the
green and blue LEDs on the discovery board pulse at different rates.

Preemptive multithreading could be implemented by adding some bookkeeping in the
scheduler and taking action in the systick timer interrupt. A 1kHz systick timer
is shown in the example, but it is not actually used for anything.
