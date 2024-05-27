################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

################################################################################

set (DPU_TARGET_RTE "--target=dpu-upmem-dpurte")

set (CMAKE_C_COMPILER   "clang")
set (CMAKE_CXX_COMPILER "clang++")

################################################################################

# We set the list of default include directories.
execute_process (COMMAND ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

################################################################################
# DPU
################################################################################

set (DPU_BINARIES_LIST  ${DPU_BINARIES})

foreach (TASKNAME  ${DPU_BINARIES_LIST})

 foreach (TASKSIZE ${BROADCAST_SIZE_LIST}) 

  foreach (TASKTYPE A)

    SET (localtarget ${TASKNAME}.${TASKTYPE}.${TASKSIZE}.dpu)

    # we generate the DPU code file from the generic code
    configure_file             (${PROJECT_SOURCE_DIR}/include/bpl/arch/dpu/ArchUpmem.dpu.${TASKTYPE}.cpp.in   ${localtarget}.cpp   @ONLY)

    # we define the source to be compiled (ie. the generated cpp file)
    add_executable             (${localtarget} ${CMAKE_CURRENT_BINARY_DIR}/${localtarget}.cpp)
    target_include_directories (${localtarget} PUBLIC ${DPU_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/tasks ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/thirdparty/cpp/include)
    target_compile_options     (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -O3 -DNDEBUG -fno-exceptions -fno-rtti  -D__x86_64__)
    target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT})
    target_link_options        (${localtarget} PUBLIC -Wl,-S )
    
    # The following can be used for usage with dpu_profiling
    #target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -pg)

	# We need to force inclusion of default include directories for the DPU target
	# Note: must use 'target_compile_options' since 'target_include_directories' doesn't work in this context.
    target_compile_options     (${localtarget} PUBLIC ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST} )

  endforeach()

 endforeach()

endforeach()
