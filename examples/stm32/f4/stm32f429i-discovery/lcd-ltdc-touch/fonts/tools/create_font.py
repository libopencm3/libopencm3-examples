#!/usr/bin/env python2
# -*- coding: utf-8 -*-

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


from sys import argv,stdout
from os.path import isfile, dirname, basename, splitext, join
from PIL import Image, ImageFont, ImageDraw
import codecs

print len(argv),argv

if len(argv)<3 or len(argv)>4 or (len(argv)==4 and not isfile(argv[3])) :
	print "usage: ./Create_Font.py ./font.ttf fontsize [charsetfile-utf8.txt]"
	print " - ./font.ttf can also be ./font.pcf or something like Arial or so
	exit(0)

fontfile = argv[1]
fontsize = None
try :
	fontsize = int(argv[2])
except :
	print "Fontsize is not a number!"
	exit(0)

if len(argv)==4 :
	charsetfile = argv[3]
else :
	charsetfile = join(dirname(argv[0]),"default_charset.txt")
	

#fontfile    = "Tamsyn5x9r.pcf"
#fontfile    = "Tamsyn5x9b.pcf"
#fontfile    = "/usr/share/fonts/liberation/LiberationMono-Regular.ttf"
#fontfile    = "/usr/share/fonts/liberation/LiberationMono-Bold.ttf"
#fontfile    = "/usr/share/fonts/dejavu/DejaVuSansMono.ttf"
#fontfile    = "/usr/share/fonts/dejavu/DejaVuSansMono-Bold.ttf"
#fontfile    = "/usr/share/fonts/gnu-free/FreeMono.ttf"
#fontfile    = "Arial"
#fontsize    = 9
datasize    = 32

charset     = codecs.open(charsetfile,'r','utf-8').read().replace('\n','').replace('\r','')
# sort charset and remove duplicates
charset     = ''.join(sorted(set(charset), key=charset.index))
font        = ImageFont.truetype(fontfile,size=fontsize)

filtered_charset = u""
for c in charset :
	if c<>u' ' :
		m,o  = font.getmask2(c, mode="1")
		if m.getbbox()==None :
			print "Dropping:",c.encode('utf-8')
			continue
	filtered_charset+=c
charset = filtered_charset
			


#render all the text (needed to determine img size)
dummy_img,offset = font.getmask2(charset, mode="1")

ascent,descent  = font.getmetrics()
lineheight      = dummy_img.size[1]+offset[1]
charwidth       = font.getsize("MMM")[0]-font.getsize("MM")[0]
totalwidth      = charwidth*len(charset)
charset_imgsize = (totalwidth,lineheight)



charset_image = Image.new(mode="1", size=charset_imgsize)
d=ImageDraw.Draw(charset_image)
d.rectangle(((0,0),charset_imgsize), fill=0, outline=None)
offx=0


fontname       = splitext(basename(fontfile))[0]+"_"+str(fontsize)
fontname       = fontname.replace("-","_")

print "Creating font", fontname


chars_table_name  = "chars_"+fontname
data_table_name   = "chars_data_"+fontname
chars_table       = "static const char_t "+chars_table_name+"[] = {\n\t"
data_table        = "static const uint"+str(datasize)+"_t "+data_table_name+"[] = {"
data_table_offset = 0; v=0; i=0; fmt = "0x%%0%dx,"%(datasize/8*2)
for c in charset :
	x1 = y1 = x2 = y2 = 0
	bbox = (0,0,0,0)
	if c<>u' ' :
		m,o  = font.getmask2(c, mode="1")
		bbox = m.getbbox()
		x1   = (charwidth - bbox[2])/2
		y1   = (o[1]-offset[1] + bbox[1])
		x2   = x1 + bbox[2]-bbox[0]
		y2   = y1 + bbox[3]-bbox[1]
		
	
	stdout.write((u"%s"%c).encode('utf-8'))
	#print "",x1,x2,y1,y2
	
	# dummy image
	if c<>u' ' :
		d.draw.draw_bitmap((offx+x1,y1),m.crop(bbox),255)
	offx += charwidth
	
	# fill data in chars table
	chars_table+= """{
		.utf8_value = %d,
		.bbox       = { % 2d,% 2d,% 2d,% 2d },
		.data       = &%s[%d]
	}, """	%(
		ord(c),
		x1,y1,x2,y2,
		data_table_name,data_table_offset
	)
    
	# fill data in chars data table
	data_table+= "\n\t/* '%s' */\n\t" %c
	
	i=0; v=0; lc = 0
	for y in range(bbox[1],bbox[3]) :
		for x in range(bbox[0],bbox[2]) :
			v |= (m.getpixel((x,y))==255) << i
			i+=1
			if i==datasize :
				if lc > 0 : data_table += " "
				data_table += fmt %v
				i=0; v=0
				data_table_offset += 1
				lc+=1
				if lc==4 :
					data_table += "\n\t"
					lc=0

	# finish last data_table entry
	if v<>0 :
		if lc > 0 : data_table += " "
		data_table += fmt %v
		data_table_offset += 1
	elif lc==0 :
		data_table = data_table[:-1]

data_table  = data_table[:-1] + "\n};"

chars_table = chars_table[:-2] + "\n};"


charset_image.save(fontname+".pbm")


fnu = fontname.upper()

fontsize_definition   = "%s %d"%(fnu,fontsize)

header_filename       = "%s.h"%fontname
header_file = """
#ifndef _%s_
#define _%s_

#include <stdint.h>
#include "fonts.h"

extern const font_t font_%s;

#endif
""" %(
	fnu,fnu,
	fontname
)

c_filename       = "%s.c"%fontname
c_file = """
#include "%s"

%s

%s

const font_t font_%s = {
	.fontsize       = %d,
	.lineheight     = %d,
	.ascent         = %d,
	.descent        = %d,
	.charwidth      = %d,
	.char_count     = %d,
	.chars          = %s,
	.chars_data     = %s,
};

""" %(
	header_filename,
	data_table,
	chars_table,
	fontname, fontsize, lineheight, ascent, descent, charwidth,
	len(charset),chars_table_name, data_table_name,
)


open(header_filename,'w').write(header_file.encode('utf-8'))
open(c_filename,'w').write(c_file.encode('utf-8'))

print "\ndone."
