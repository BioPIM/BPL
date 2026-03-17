#!/bin/python3

import sys
import os
import glob

# Ex of use:
# ./AddBannerAsHeader.py ../src,../export/helloworld/src,../snippets,../test       ./banner4cpp.txt
# ./AddBannerAsHeader.py ../src  ./banner4cmake.txt  "CMakeLists.txt" "##"

####################################################################################################
#
####################################################################################################
def addHeader (filename, banner, prefix="////"):

    result = [];
    
    # we read the file content
    content = [];
    with open(filename,"r") as f:  content = f.read().splitlines();
        
    # we look for a potential existing header (conventionnaly starting with something like ##)
    state = 0;
    
    # EXAMPLE A  (with prefix=='##') :
    #     ( 1)  ##################################################    status <- 1
    #     ( 2)  ## MY HEADER                                          status <- 2
    #     ( 2)  ## 2023                                               status <- 2
    #     ( 3)  ##################################################    status <- 3
    #     ( 4)                                                        status <- 3 
    #     ( 5)  MESSAGE ("hello")                                     status <- 3

    # EXAMPLE B  (with prefix=='##') :
    #     ( 1)  ##################################################    status <- 1
    #     ( 2)  # a comment                                           status <- 4
    #     ( 2)  # BUT NOT A HEADER                                    status <- 4
    #     ( 3)  ##################################################    status <- 4
    #     ( 4)                                                        status <- 4 
    #     ( 5)  MESSAGE ("hello")                                     status <- 4

    foundPreviousHeader = False;
        
    for idx,line in enumerate(content):
        
        if state==0:
            if len(line)==0:  
                continue;
            elif line.startswith(prefix*2):
                state = 1;
                continue;
            else:
                state = 4;
                
        if state==1:
            if line.startswith(prefix):
                state = 2;
                continue;
            else:
                state = 4;
            
        if state==2:  
            if line.startswith(prefix*2):
                state = 3;
                continue;
            elif line.startswith(prefix):
                continue;
            else:
                state = 4;
            
        if state==3:
            result.append(line);

    tail = result if state==3 else content;
    
    # if the banner has an ending empty line, we remove those of the tail in order to have only one empty line
    if len(banner[-1])==0:
        while len(tail)>0 and len(tail[0])==0:
            tail = tail[1:]

    return banner + tail;

####################################################################################################
#
####################################################################################################

defaultbanner = """
////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// data  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////
"""

if len(sys.argv)<2:
    print ("You must provide:")
    print ("   1: directory holding source files");
    print ("   2: file holding the banner to be added to the header of the source files (if not defined, use the 'defaultbanner' variable)");
    print ("   3: regular expression for scanning file (ex: *.[ch]**) ");
    sys.exit(1);
    
sourceDir = sys.argv[1];

# we get the banner (potentially from a provided file)
banner = defaultbanner;
if len(sys.argv)>=3:
    with open(sys.argv[2],"r") as f:  banner = f.read();
banner = banner.splitlines();

matchexp = "*.[ch]*";
if len(sys.argv)>=4:
    matchexp = sys.argv[3]

prefix = "////";
if len(sys.argv)>=5:
    prefix = sys.argv[4]

allfiles = [];
for src in sourceDir.split(","):
    allfiles = allfiles + [x for x in glob.glob (src + "/**/" + matchexp, recursive=True) if not os.path.isdir(x)]

for filename in allfiles:
    
    result = addHeader (filename, banner, prefix);

    # we save the modified file
    if len(result)>0:
        with open(filename,"w") as f:  f.write("\n".join(result));

    print ("{:s}  file: {:60s}    #lines: {:7d}  {:s}".format ("-"*40, filename, len(result), "-"*40))
    
