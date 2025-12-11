################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2025
## author: edrezen
################################################################################

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

################################################################################

set (CMAKE_CXX_STANDARD      20)
set (CMAKE_CXX_FLAGS         "-Wall -Wextra -Wno-unused-parameter -O3 -DNDEBUG -g -fdiagnostics-color=always")

################################################################################
include_directories     (${PROJECT_SOURCE_DIR}/src ${DPU_INCLUDE_DIRS})

################################################################################

if (NOT DEFINED NR_DPUS)
    set (NR_DPUS 1)
endif()

if (NOT DEFINED NR_TASKLETS)
    set (NR_TASKLETS 16)
endif()

if (NOT DEFINED NR_THREADS)
    set (NR_THREADS 24)
endif()

################################################################################
ADD_SUBDIRECTORY (host)

