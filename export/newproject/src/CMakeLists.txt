################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

################################################################################

set (CMAKE_CXX_STANDARD      20)
set (CMAKE_CXX_FLAGS         "-Wall -Wextra -Wno-unused-parameter -Wno-macro-redefined -O3 -DNDEBUG")

################################################################################

include_directories     (${PROJECT_SOURCE_DIR}/src ${DPU_INCLUDE_DIRS})

################################################################################

if (NOT DEFINED NR_DPUS)
    set (NR_DPUS 1)
endif()

if (NOT DEFINED NR_TASKLETS)
    set (NR_TASKLETS 16)
endif()

if (NOT DEFINED STACK_SIZE_DEFAULT)
    MATH(EXPR STACK_SIZE_DEFAULT "(( (65536*91/100) / ${NR_TASKLETS})/8)*8")
endif()

################################################################################

# We define the list of allowed (log2) sizes for input parameters broadcast.
set (BROADCAST_SIZE_MBYTES  "1,2,4,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60")
set (BROADCAST_SIZE_LIST  ${BROADCAST_SIZE_MBYTES})
STRING(REPLACE "," ";" BROADCAST_SIZE_LIST ${BROADCAST_SIZE_LIST} )

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

