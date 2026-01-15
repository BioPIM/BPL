################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

################################################################################
if (NOT DEFINED NR_DPUS)
    set (NR_DPUS 1)
endif()

if (NOT DEFINED NR_TASKLETS)
    set (NR_TASKLETS 16)
endif()

################################################################################
ADD_SUBDIRECTORY (main)
ADD_SUBDIRECTORY (tasks)

################################################################################

# Finally, we dump a message to tell how to configure some env var
MESSAGE ("")
MESSAGE ("-------------------------------------------------------------------------------------------")
MESSAGE ("PLEASE RUN THE FOLLOWING COMMAND:")
MESSAGE ("")
MESSAGE ("    export set DPU_BINARIES_DIR=${CMAKE_CURRENT_BINARY_DIR}")
MESSAGE ("")
MESSAGE ("-------------------------------------------------------------------------------------------")
MESSAGE ("")

