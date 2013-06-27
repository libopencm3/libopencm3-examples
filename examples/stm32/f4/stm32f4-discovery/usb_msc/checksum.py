#!/usr/bin/env python
#
# Utility to calculate filename checksum used in LFN entries
#

import sys

print 'Enter 8.3 Filename (11 uppercase chars):'
sfn = sys.stdin.readline().strip()
print 'Raw filename:', sfn
chk = 0
for c in sfn:
	chk = ((((chk & 1) << 7) | ((chk & 0xFE) >> 1)) + ord(c)) & 0xFF
print 'Checksum (dec): %d' % chk
print 'Checksum (hex): 0x%02x' % chk
