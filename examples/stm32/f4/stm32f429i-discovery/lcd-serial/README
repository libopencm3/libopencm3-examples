README lcd-serial
-----------------

This example sets up the LCD display on the DISCO board
and shows some sample screens. It uses some graphics code
that was inspired by AdaFruit graphics library. Not that
a lot of it is used but once the display was "on" it was
something to use to put something other than straight
lines on it :-). 

It is a bit more complex because there are a lot of things
going on at the same time. First the display is connected
to the CPU via an SPI port. Second, the display is, like
most LCD displays, a fairly complex controller chip in itself
so it generally has a fairly complex initialization sequence.
And finally, once initialized, drawing something other than
straight lines involves a bit of bit fiddling.

Once it is set up the initialization routine writes a pattern
of lines in the RAM used to hold a frame and puts it on the
LCD.

Pressing a key will clear the screen and fill it with a box
that has a simple text message in a box and 3 circles along
the bottom.

Pressing any key again, will bring up a display that says
"PLANETS!" and animates three planets orbiting a star (not
to scale :-) to give you a feel for the "speed" of animation
when you're dumping the entire screen through the SPI port
each time to update the display. The next example uses
the TFT interface of the chip to load the data into the 
display.
