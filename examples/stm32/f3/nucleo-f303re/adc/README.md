# README

This makes an LED bar-graph of an analog input.  It's intended for the
ST Nucleo-F303RE devboard, with the LEDs attached to the Arduino
header pins, going from D13 to D6.  The analog input is sampled from
the Arduino header analog input A0.  It also prints the output on the
serial port, which is available as an ACM device on many OSes
(/dev/ttyACM on Linux, etc).  If you solder SB62 and SB63, you can get
the serial output on the Arduino's usual TX/RX pins as well.


