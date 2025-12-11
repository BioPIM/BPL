################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2023
## author: edrezen
################################################################################

################################################################################
# VARIABLE CONFIGURATION
################################################################################

# Define the name of the Host binary.
SET (HOST_BINARY benchmark)

# Define the list of DPU binaries that can be launched by the Host.
SET (DPU_BINARIES "Benchmark1" "Benchmark2" "Syracuse")


# NOTE: we use the clang version provided by UPMEM. Indeed, this version is able
# to generate target binaries for the UPMEM architecture. Actually, we also can
# use it to compile also the host binary.
set (CMAKE_CXX_FLAGS    "-Wall -Wextra -Wno-unused-parameter -O3")
set (CMAKE_CXX_STANDARD  17)

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

if (NOT DEFINED NR_DPUS)
    set (NR_DPUS 1)
endif()

if (NOT DEFINED NR_TASKLETS)
    set (NR_TASKLETS 16)
endif()

# We compute a default stack size (if not already defined)
if (NOT DEFINED STACK_SIZE_DEFAULT)
    MATH(EXPR STACK_SIZE_DEFAULT "(( (65536*70/100) / ${NR_TASKLETS})/8)*8")
endif()

# We set the target for UPMEM architecture
set (DPU_TARGET_RTE "--target=dpu-upmem-dpurte")

# We set the list of default include directories. We need to force adding them to the -I directories for the DPU compilation.
# see also https://stackoverflow.com/questions/27650807/avoid-quotes-when-using-cmake-to-pass-a-process-output-as-compile-options-for-sp
execute_process (COMMAND ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

################################################################################
# HOST executable
################################################################################
file                       (GLOB_RECURSE  HostFiles  ./*)
add_executable             (${HOST_BINARY} ${HostFiles})
target_link_libraries      (${HOST_BINARY}  PRIVATE stdc++fs dpu pthread)
target_link_directories    (${HOST_BINARY}  PUBLIC ${DPU_LIBDIR} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT})
target_compile_options     (${HOST_BINARY}  PUBLIC -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} -g)
target_include_directories (${HOST_BINARY}  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${DPU_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

################################################################################
# DPU executable(s)
################################################################################

# We define the list of allowed (log2) sizes for input parameters broadcast.
set (BROADCAST_SIZE_LOG2  "10,14,18,22,24")
set (BROADCAST_SIZE_LIST  ${BROADCAST_SIZE_LOG2})
STRING(REPLACE "," ";" BROADCAST_SIZE_LIST ${BROADCAST_SIZE_LIST} )

set (DPU_BINARIES_LIST  ${DPU_BINARIES})

# We create the config file.
configure_file (${CMAKE_SOURCE_DIR}/src/bpl/arch/ArchUpmemConfig.hpp.in  config.hpp   @ONLY)

foreach (TASKNAME  ${DPU_BINARIES_LIST})
  foreach (TASKSIZE ${BROADCAST_SIZE_LIST}) 
    foreach (TASKTYPE A)

      SET (localtarget ${TASKNAME}.${TASKTYPE}.${TASKSIZE}.dpu)

      # we generate the DPU code file from the generic code
      configure_file             (${CMAKE_SOURCE_DIR}/src/bpl/arch/dpu/ArchUpmem.dpu.${TASKTYPE}.cpp.in   ${localtarget}.cpp   @ONLY)

      # we define the source to be compiled (ie. the generated cpp file)
      add_executable             (${localtarget} ${CMAKE_CURRENT_BINARY_DIR}/${localtarget}.cpp)
      target_include_directories (${localtarget} PUBLIC ${DPU_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/thirdparty/cpp/include)
      target_compile_options     (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -O2 -fno-exceptions -fno-rtti  -D__x86_64__)
      target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} -Wl,-S )

      # We need to force inclusion of default include directories for the DPU target
      # Note: must use 'target_compile_options' since 'target_include_directories' doesn't work in this context.
      target_compile_options     (${localtarget} PUBLIC ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST} )

    endforeach()
  endforeach()
endforeach()
