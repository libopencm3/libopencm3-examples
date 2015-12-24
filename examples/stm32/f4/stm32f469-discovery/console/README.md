Console Example
---------------

These examples include some helper functions, one set are the
clock setup for the board and the console function with has
some basic support for sending strings to and from the debug
part on the STM32F469-Discovery board. 

The main function simply initializes the console for 115200 
baud and sets up a very simple interactive loop. Reset on 
control-C is enabled so when the example is running, if you 
hold the control key and C the board will reset and restart. 
