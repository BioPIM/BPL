################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

cmake_minimum_required(VERSION 3.13)

################################################################################
# Set some basic project attributes
################################################################################
project (org_pim_bpl_checksum VERSION 0.1 DESCRIPTION "Checksum")

################################################################################
# CONFIGURATION
################################################################################

# Define the name of the Host binary.
SET (HOST_BINARY helloworld)

# Define the list of DPU binaries that can be launched by the Host.
SET (TASKS_LIST "HelloWorld" "DoSomething")

# Now we can read the generic part.
ADD_SUBDIRECTORY(src)
