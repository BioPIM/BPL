################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2023
## author: edrezen
################################################################################

################################################################################
# VARIABLE CONFIGURATION
################################################################################

# Define the name of the Host and DPU binaries
SET (HOST_BINARY benchmark_host_vector)
SET (DPU_BINARY  benchmark_dpu_vector)


# NOTE: we use the clang version provided by UPMEM. Indeed, this version is able
# to generate target binaries for the UPMEM architecture. Actually, we also can
# use it to compile also the host binary.
set (CMAKE_CXX_FLAGS    "-Wall -Wextra -Wno-unused-parameter -O3")
set (CMAKE_CXX_STANDARD  17)

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

set (NR_DPUS               1)
set (NR_TASKLETS          16)
set (STACK_SIZE_DEFAULT 2800)

# We set the target for UPMEM architecture
set (DPU_TARGET_RTE "--target=dpu-upmem-dpurte")

execute_process (COMMAND ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

################################################################################
# HOST executable
################################################################################
add_executable             (${HOST_BINARY} host.cpp)
target_link_libraries      (${HOST_BINARY}  PRIVATE stdc++fs dpu pthread)
target_link_directories    (${HOST_BINARY}  PUBLIC ${DPU_LIBDIR} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT})
target_compile_options     (${HOST_BINARY}  PUBLIC -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} -g)
target_include_directories (${HOST_BINARY}  PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${DPU_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

################################################################################
# DPU executable(s)
################################################################################

foreach (NMAX RANGE 4 22) 

    SET (localtarget ${DPU_BINARY}.${NMAX})

    configure_file             (dpu.cpp.in   ${localtarget}.cpp   @ONLY)

	add_executable             (${localtarget} ${localtarget}.cpp)
	target_include_directories (${localtarget} PUBLIC ${DPU_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_SOURCE_DIR}/src)
	target_compile_options     (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -O3 -fno-exceptions -fno-rtti  -D__x86_64__)
	target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} -Wl,-S )
	target_compile_options     (${localtarget} PUBLIC ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST} )

endforeach()


