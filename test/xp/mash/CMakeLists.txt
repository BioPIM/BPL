################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

cmake_minimum_required(VERSION 3.13)

################################################################################
# Set some basic project attributes
################################################################################
project (org_pim_bpl_mash VERSION 0.1 DESCRIPTION "mash")

################################################################################
# CONFIGURATION
################################################################################

# Define the name of the Host binary.
SET (HOST_BINARY mash)

# Define the list of DPU binaries that can be launched by the Host.
SET (TASKS_LIST "SketchJaccard")

# We define the location of the BPL. 
set (BPL_SOURCE_DIR  ${PROJECT_SOURCE_DIR}/include/bpl)

# Now we can read the generic part.
ADD_SUBDIRECTORY(src)
