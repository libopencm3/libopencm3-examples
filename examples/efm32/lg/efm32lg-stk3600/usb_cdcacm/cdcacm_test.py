#! /usr/bin/env python
#
# This file is part of the libopencm3 project.
#
# Copyright (C) 2011 Piotr Esden-Tempski <piotr@esden.net>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.
#

import serial
import signal
import sys
import time

DEVICE_PATH = '/dev/ttyACM0'
TEST_STRING = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*(){}"

def signal_handler(signal, frame):
	print('You pressed Ctrl+C!')
	ser.close()
	sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

ser = serial.Serial(DEVICE_PATH, 115200, timeout=0.05)

error = 0
tim = time.time()
cycles = 0

while True:
	ser.write(TEST_STRING)
	buf = ser.read(1024)
	cycles += 1
	if buf != TEST_STRING:
		error += 1
	print("received %s errors: %i cycles: %i runtime: %i" %
		(buf, error, cycles, time.time() - tim))
    #time.sleep(0.1)

ser.close();
