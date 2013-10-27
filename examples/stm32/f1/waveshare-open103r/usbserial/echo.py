#! /usr/bin/python

import sys
import serial

def echo(text):
    s = serial.Serial()
    s.port = "/dev/ttyACM0"
    s.open()
    print s.read(s.write(text))

if __name__ == "__main__":
    if len(sys.argv) > 1:
        echo(" ".join(sys.argv[1:]))
    else:
        print "Usage: sudo echo.py hello world"
