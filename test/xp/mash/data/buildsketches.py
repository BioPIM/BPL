#!/usr/bin/python3

# Build a file holding sketches (one line per sketch).
# For simplicity, the integer values of a sketch are limited to 16 values
# and encoded as ascii characters beginning from 'a'

import sys
import random

if len(sys.argv)<3:
	print ("parameters are:")
	print ("1) output filepath")
	print ("2) number of sketches")
	print ("3) size of a sketch")
	sys.exit(1)
	
outputfile =sys.argv[1]
nbsketches = int(sys.argv[2]);	
sketchsize = int(sys.argv[3]);	

with open(outputfile,"wb") as f:
	for i in range(0,nbsketches):
		s0 = [random.choice(range(0,8)) for _ in range(sketchsize)]
		s1 = [x.to_bytes(4, byteorder = 'little') for x in s0]
		for b in s1:
			f.write (b)	
