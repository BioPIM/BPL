################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

################################################################################
# COMPILER OPTIONS
################################################################################
set (DPU_TARGET_RTE "--target=dpu-upmem-dpurte")

set (CMAKE_C_COMPILER   "clang")
set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_FLAGS         "-Wall -Wextra -Wno-unused-parameter -Wno-macro-redefined -O3 -DNDEBUG -g")

MESSAGE ("-- DPU:  CMAKE_C_COMPILER  : ${CMAKE_C_COMPILER}")
MESSAGE ("-- DPU:  CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

################################################################################

# We create the config file.
configure_file (${PROJECT_SOURCE_DIR}/src/bpl/arch/ArchUpmemConfig.hpp.in  config.hpp   @ONLY)

# We set the list of default include directories.
if(NOT DEFINED CXX_FOR_DEFAULT_INCLUDE_QUERY OR CXX_FOR_DEFAULT_INCLUDE_QUERY STREQUAL "")
  if(DEFINED ENV{CXX} AND NOT "$ENV{CXX}" STREQUAL "")
    set(_cxx_for_default "$ENV{CXX}")
  else()
    set(_cxx_for_default "${CMAKE_CXX_COMPILER}")
  endif()
  set(CXX_FOR_DEFAULT_INCLUDE_QUERY "${_cxx_for_default}" CACHE STRING "Compiler used to query default include directories")
endif()

execute_process (
  COMMAND ${CMAKE_COMMAND} -E env "CXX=${CXX_FOR_DEFAULT_INCLUDE_QUERY}" ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py
  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

MESSAGE ("CXX_DEFAULT_INCLUDE_DIRECTORIES     : '${CXX_DEFAULT_INCLUDE_DIRECTORIES}'")

################################################################################
# LIST OF TASKS TO BE COMPILED FOR DPU
################################################################################

set (TASKS_LIST 
	"Bank1" "Bank2" "Bank3" "Bank4" "Bank5"
	"Parrot1" "Parrot2"  "Parrot4" "Parrot5" "SumGeneric" "CountInstances" "GetPuid" "DoSomething"
	"Parrot3"
	"RangeSplit" "Array1" "Array2" "HelloWorld" "Checksum3" "Checksum4" "Checksum5" "IterableSplit"
	"Compare2" "ReturnArray" "IntegerType" "Compare1"
	"Sum"
	"Vector1" "Vector2" "Vector3" "VectorCheck" 
	"VectorAsInput2" "VectorAsInput3" "VectorAsInputCustom" 
	"VectorChecksum"
	"VectorAsOutputUint8" "VectorAsOutputUint16" "VectorAsOutputUint32"  
	"VectorAsInput"  
	"SketchJaccardTopK"
  "SketchJaccardOptim"
  "SketchJaccardDistance" "SketchJaccardDistanceOnce" 
  "SketchJaccard" 
	"VectorAsInputSplit"
	"SplitRangeInt"
	"Mutex1" "Mutex2"
	"TemplateTask<double>"
	"VectorInputOutput"
	"VectorInputOutput2"
	"VectorOfVectors"
	"VectorFlush"
	"VectorAdd"
	"Span1"
	"VectorSwap"
	"VectorMove1"
	"VectorMove2"
	"VectorEmplaceBack"
	"Once1"
	"Global0" "Global1"  "Global2"  "Global3"  "Global4"  "Global5" "Global6"
	"Struct1" "Struct2" "Struct3"
	"VectorMany" "VectorMany1" "VectorMany2" "VectorMany3" "VectorCache" "VectorConfig"
	"GlobalOnce1"  "GlobalOnce2"
	"VectorViewRandomAccess1"  "VectorViewRandomAccess2"
	"VectorAddAndIterate"
	"VectorSizeof12"
	"VectorMany4" "VectorMany2b"
	"VectorRandomAccessGlobal1" "VectorRandomAccessGlobal2"
	"GlobalOnce3" "GlobalOnce4"
	"SplitDifferentSizes"
	"VectorEmpty1" "VectorEmpty2"
	"VectorSplitDpu"
	"OncePadding1"
	"ResultsToVector"
	"IterateMultipleVectors"
	"LauncherPool1" 
	"VectorReverseIterator" "VectorReverseIterator2" 
	"VectorReverseIterator3" "VectorReverseIterator3global" "VectorReverseIterator3once" "VectorReverseIterator3glonce" 
	"VectorStd"
	"VectorTwoRunsGlobalRandomAccess"
	"VectorIterator1" "VectorIterator2" "VectorIterator3"
	"VectorSplitSimple" "VectorSplitOverload" "TemplateSpecialization"
	"Min" "Max"
	"VectorReverseInPlace"
	"SortSelection"  
	"SortSelectionVector"  
	"VectorCustomAllocator"
	"SplitMyLong" 
	"VectorCreation"
	"VectorSharedIteratorTrue" "VectorSharedIteratorFalse"
	"VectorSerialize"
	"SyracuseReduce" "SyracuseVector"
)

################################################################################
# UTILS FUNCTIONS  
################################################################################

# Split a full task name if needed.
#   Ex: Mutex1               => Mutex1
#   Ex: TemplateTask<double> => TemplateTask;double
function(process_taskname FULLTASKNAME  TASKNAME TASKTEMPLATE)
  string(REGEX REPLACE "<|>" " " TASKNAME_LIST ${FULLTASKNAME})
  separate_arguments (TASKNAME_LIST)
  list (LENGTH TASKNAME_LIST length)
  list (GET    TASKNAME_LIST 0 TASKNAME)
  if (length EQUAL 3)
 	list (GET TASKNAME_LIST 1 TASKTEMPLATE) 
	set (TASKTEMPLATE ",${TASKTEMPLATE}" )
  else()
	set (TASKTEMPLATE "")  # no template defined
  endif()
  SET(TASKNAME     ${TASKNAME}     PARENT_SCOPE)
  SET(TASKTEMPLATE ${TASKTEMPLATE} PARENT_SCOPE)
endfunction()

# Register a custom command that analyzes a task's DPU binary, applies a
# 10% safety margin (aligned to 8 bytes), and emits a response file providing
# -DSTACK_SIZE_DEFAULT=<value> to clang during the optimized build.
function(register_stack_analysis TASKNAME TASKTYPE)
  set(localtarget_analysis ${TASKNAME}.${TASKTYPE}.dpu.analysis)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.stack_size.rsp
    COMMAND /bin/bash -c "\
      set -euo pipefail; \
      max=\$(dpu_stack_analyzer \"$<TARGET_FILE:${localtarget_analysis}>\" \
              | grep 'Max size:' \
              | sed 's/Max size: //'); \
      if [[ -z \"\$max\" ]]; then \
        echo \"ERROR: Could not read max size\" >&2; exit 1; \
      fi; \
      if [[ ! \$max =~ ^[0-9]+$ ]]; then \
        echo \"ERROR: Invalid max size: \$max\" >&2; exit 1; \
      fi; \
      optimal=\$(echo \"scale=0; ((\$max + ((\$max + 90) / 100)) + 7) / 8 * 8\" | bc); \
      echo ${TASKNAME} \": Calculated optimal stack size: Max=\$max -> Optimal=\$optimal\"; \
      printf -- '-DSTACK_SIZE_DEFAULT=%s\\n' \"$optimal\" > ${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.stack_size.rsp\
    "
    DEPENDS ${localtarget_analysis}
    COMMENT "Analyzing stack usage for ${TASKNAME} and generating stack flag"
    VERBATIM
  )
endfunction()

set(DPU_TASK_TYPES A)

set(DPU_COMMON_INCLUDES
    ${CMAKE_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/..
    ${DPU_INCLUDE_DIRS}
    /opt/upmem/share/upmem/include/syslib
)

set(DPU_COMMON_COMPILE_FLAGS
    ${DPU_TARGET_RTE}
    -DNR_TASKLETS=${NR_TASKLETS}
    -O3 -DNDEBUG
    -fno-exceptions -fno-rtti
    -D__x86_64__
)

set(DPU_LINK_FLAGS
    ${DPU_TARGET_RTE}
    -DNR_TASKLETS=${NR_TASKLETS}
    -Wl,-S
)

function(configure_dpu_target TASKNAME TASKTEMPLATE TASKTYPE OUTNAME STACK_DEFINE)
  configure_file(
    ${PROJECT_SOURCE_DIR}/src/bpl/arch/dpu/ArchUpmem.dpu.${TASKTYPE}.cpp.in
    ${OUTNAME}.cpp
    @ONLY
  )

  add_executable(${OUTNAME} ${CMAKE_CURRENT_BINARY_DIR}/${OUTNAME}.cpp)
  target_include_directories(${OUTNAME} PUBLIC ${DPU_COMMON_INCLUDES})
  target_compile_options(${OUTNAME} PUBLIC ${DPU_COMMON_COMPILE_FLAGS})
  target_compile_options(${OUTNAME} PUBLIC ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST})
  target_link_options(${OUTNAME} PUBLIC ${DPU_LINK_FLAGS})

  if(STACK_DEFINE)
    target_compile_options(${OUTNAME} PUBLIC "@$<SHELL_PATH:${STACK_DEFINE}>")
    target_link_options(${OUTNAME} PUBLIC "@$<SHELL_PATH:${STACK_DEFINE}>")
  else()
    target_compile_options(${OUTNAME} PUBLIC -DSTACK_SIZE_DEFAULT=64)
    target_link_options(${OUTNAME} PUBLIC -DSTACK_SIZE_DEFAULT=64)
  endif()
endfunction()

################################################################################
# DPU BINARIES GENERATION 
################################################################################
foreach(FULLTASKNAME ${TASKS_LIST})
  process_taskname(${FULLTASKNAME} TASKNAME TASKTEMPLATE)

  foreach(TASKTYPE IN LISTS DPU_TASK_TYPES)
    # We create the analysis target to get stack size information.
    set(localtarget_analysis ${TASKNAME}.${TASKTYPE}.dpu.analysis)
    configure_dpu_target("${TASKNAME}" "${TASKTEMPLATE}" "${TASKTYPE}" "${localtarget_analysis}" "")
    register_stack_analysis(${TASKNAME} ${TASKTYPE})

    # We create the actual target that will be used.
    set(stack_rsp ${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.stack_size.rsp)
    set(stack_ready_target ${TASKNAME}_${TASKTYPE}_stack_ready)
    add_custom_target(${stack_ready_target} DEPENDS ${stack_rsp})

    set(localtarget ${TASKNAME}.${TASKTYPE}.dpu)
    configure_dpu_target("${TASKNAME}" "${TASKTEMPLATE}" "${TASKTYPE}" "${localtarget}" "${stack_rsp}")
    add_dependencies(${localtarget} ${localtarget_analysis} ${stack_ready_target})

#    # We clean up the temporary files created to get stack size information.
#    add_custom_command(TARGET ${localtarget}
#      POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E remove -f
#                "${CMAKE_CURRENT_BINARY_DIR}/${localtarget_analysis}"
#                "${CMAKE_CURRENT_BINARY_DIR}/${localtarget_analysis}.cpp"
#                "${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.stack_size.rsp"
#        COMMENT "Removing temporaries after building ${localtarget}"
#    )
  endforeach()
endforeach()
