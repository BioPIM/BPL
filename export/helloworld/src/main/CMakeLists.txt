################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

# We create the config file.
configure_file (${CMAKE_SOURCE_DIR}/include/bpl/arch/ArchUpmemConfig.hpp.in  config.hpp   @ONLY)

file                       (GLOB_RECURSE  HostFiles  main.cpp)
add_executable             (${HOST_BINARY} ${HostFiles})
target_link_libraries      (${HOST_BINARY} PRIVATE stdc++fs dpu pthread)
target_link_directories    (${HOST_BINARY} PUBLIC ${DPU_LIBDIR} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT})
target_compile_options     (${HOST_BINARY} PUBLIC -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} )
target_include_directories (${HOST_BINARY}  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include ${DPU_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

