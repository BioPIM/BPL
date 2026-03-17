#!/bin/python3

################################################################################
# Find all default includes
################################################################################

import subprocess
import os

# see https://stackoverflow.com/questions/11946294/dump-include-paths-from-g
cmd = "echo | $CXX  -xc++ -E -std=c++20  -v 1>&2 -";

output = subprocess.getoutput (cmd)

result = []
ok     = False;

for line in output.split("\n"):
    if line.startswith("#include <"):        
      ok = True;
    elif ok:
        if line.startswith("End of search list"):
            ok = False;
        else:
            result.append (line.strip())
            
print ("".join([" -I" + os.path.normpath(x) for x in result]))
