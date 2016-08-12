# This file is part of the libopencm3 project.
# 
# Copyright (C) 2016 Oliver Meier <h2obrain@gmail.com>
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

from PIL import Image
from sys import argv
from os.path import isfile, splitext
from math import floor

if (not(len(argv)==2 or len(argv)==3)) or not isfile(argv[1]) :
	print "usage Image...py filename.png width_in_px"
	exit(0)

infile = argv[1]

basename = splitext(infile)[0]
outfile = basename+"_RGB565.h"
if isfile(outfile) :
	print outfile , "already exists"
	exit(0)


# die here if not an image
try :
	img = Image.open(infile)
except IOError, ex :
	print "Opening image failed"
	print "\tPIL(IOError) :",ex.message
	exit(0)

if len(argv)==3 :
	try    :
		width = int(argv[2])
		if img.size[0]<>width :
			new_size = (width,int(img.size[1]*width/img.size[0]))
			print "resizing image from",img.size,"to",new_size
			img = img.resize(new_size,Image.ANTIALIAS)
			img.show(title="Resized image")
	except :
		print "Invalid width"
		print "usage Image...py filename.png width_in_px"
		exit(0)

print "image size is",img.size
width = img.size[0]
data = img.tobytes(encoder_name='raw')

pixels_per_line = 16
of   = open(outfile,'w')
of.write("#include <stdint.h>\n")
of.write("#define %s_WIDTH %d\n"  %(basename.upper(), width))
of.write("#define %s_HEIGHT %d\n" %(basename.upper(), len(data)/4/width))
of.write("const uint16_t %s[] = {"  %(basename))
i = 0
while i<len(data) :
	if i==0                          : of.write( "\n\t")
	elif not (i%(pixels_per_line*4)) : of.write(",\n\t")
	else                             : of.write(", ")
	r = int(ord(data[i])>>3); i+=1
	g = int(ord(data[i])>>2); i+=1
	b = int(ord(data[i])>>3); i+=1
	i+=1 # skip alpha
	i16 = (r<<11) | (g<<5) | b
	of.write("0x%04x" %(i16))
of.write("\n};")
of.close()
