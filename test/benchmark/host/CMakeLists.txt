################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2025
## author: edrezen
################################################################################

################################################################################
#  BENCHMARK TESTS 
################################################################################

# we rely on the Catch unit test framework  (https://github.com/catchorg/Catch2/)
find_package(Catch2 3 REQUIRED)

find_package(fmt)

################################################################################

# We create the config file.
configure_file (${PROJECT_SOURCE_DIR}/src/bpl/arch/ArchUpmemConfig.hpp.in  config.hpp   @ONLY)

################################################################################
# BPL tests (HOST)
################################################################################

set (CMAKE_CXX_STANDARD 20)

SET (targetname bpl-benchmark)
file                       (GLOB_RECURSE  BplTestFiles  *)
add_executable             (${targetname}.host   ${BplTestFiles})
target_link_libraries      (${targetname}.host PUBLIC Catch2::Catch2WithMain stdc++fs dpu pthread fmt::fmt)
target_link_directories    (${targetname}.host PUBLIC ${DPU_LIBDIR} )
target_compile_options     (${targetname}.host PUBLIC -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} )
target_include_directories (${targetname}.host PUBLIC ${PROJECT_SOURCE_DIR}/test/unit ${PROJECT_SOURCE_DIR}/test/benchmark/host ${CMAKE_CURRENT_BINARY_DIR})

if (DEFINED WITH_SERIALIZATION_HASH_CHECK)
	target_compile_options (${targetname}.host PUBLIC -DWITH_SERIALIZATION_HASH_CHECK )
endif()
