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
# see also https://stackoverflow.com/questions/27650807/avoid-quotes-when-using-cmake-to-pass-a-process-output-as-compile-options-for-sp
execute_process (COMMAND ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

MESSAGE ("CXX_DEFAULT_INCLUDE_DIRECTORIES     : '${CXX_DEFAULT_INCLUDE_DIRECTORIES}'")

################################################################################
# UTILS FUNCTIONS  
################################################################################

# https://stackoverflow.com/questions/15282444/using-cmake-how-can-i-concat-files-and-install-them
function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

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

################################################################################
# LIST OF TASKS TO BE COMPILED FOR DPU
################################################################################

set (TASKS_LIST 
	"Bank1" "Bank2" "Bank3" "Bank4" "Bank5"
	"Parrot1" "Parrot2"  "Parrot4" "Parrot5" "SumGeneric" "CountInstances" "GetPuid" "DoSomething"
	"Parrot3"
	"RangeSplit" "Array1" "HelloWorld" "Checksum3" "Checksum4" "Checksum5" "IterableSplit"
	"Compare1" "Compare2" "ReturnArray" "IntegerType" 
	"Sum"
	"Vector1" "Vector2" "Vector3" "VectorCheck" 
	"VectorAsInput2" "VectorAsInput3" "VectorAsInputCustom" 
	"VectorChecksum"
	"VectorAsOutputUint8" "VectorAsOutputUint16" "VectorAsOutputUint32"  
	"VectorAsInput"  
	"SketchJaccardTopK"
    "SketchJaccardOptim" 
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
	"Global1"  "Global2"  "Global3"  "Global4"  "Global5"
)

################################################################################
# STACK SIZE DEFINITION FOR EACH TASK
################################################################################

# We try to infer the stack size to be used for each task.
# To do this, we need to know the tasks that use 'global' parameters, ie. parameters
# whose type is tagged by the C++ template template class 'global'.
# Then we can compute the size of the WRAM that will be common to all tasklets and
# by deduction we can compute the stack size for each tasklet

# PASS 1: we generate a file that will be the amalgation of the 'GetSizeGlobalParameters.cpp.in' file for each task  
foreach (FULLTASKNAME ${TASKS_LIST})

    # we retrieve information from the full task name
    process_taskname (${FULLTASKNAME} TASKNAME TASKTEMPLATE)

    # we generate the DPU code file from the generic code
    configure_file (
  	    ${PROJECT_SOURCE_DIR}/src/bpl/utils/GetSizeGlobalParameters.cpp.in  
 	    ${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.getsizeglobal.cpp   
 	    @ONLY
    )
endforeach()

# We concatenate all the generated files into a single one.
# -> we will have only one compilation which should greatly speed up the cmake configuration step.
file (GLOB  GlobalSizeFiles "${CMAKE_CURRENT_BINARY_DIR}/*.getsizeglobal.cpp")
file (WRITE amalgation_getsizeglobal.cpp.in "")
foreach(GlobalSizeFile ${GlobalSizeFiles})
  cat(${GlobalSizeFile} amalgation_getsizeglobal.cpp.in)
endforeach()
configure_file(amalgation_getsizeglobal.cpp.in  amalgation_getsizeglobal.cpp COPYONLY)

 # We infer the stack size from compilation of the underlying task.
EXECUTE_PROCESS (
    COMMAND ${CMAKE_CXX_COMPILER} -ferror-limit=0 -std=gnu++2a ${DPU_TARGET_RTE} -D__x86_64__  -I${CMAKE_SOURCE_DIR}/src -I${CMAKE_CURRENT_SOURCE_DIR}/.. ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST}  ${CMAKE_CURRENT_BINARY_DIR}/amalgation_getsizeglobal.cpp
    # We get the useful information from the compiler error message
    # See https://stackoverflow.com/questions/78580764/retrieving-from-gcc-clang-a-value-known-at-compilation-time/78580978#78580978 
    ERROR_VARIABLE  GLOBAL_WRAM_SIZE
)

# Finally, we build a "map" that associates the task name with the byte size of the WRAM dedicated to 'global' parameters.
foreach (X1 ${GLOBAL_WRAM_SIZE})
    STRING (REGEX MATCHALL "_[a-zA-Z0-9_]*_::error<[0-9]*>"  X2 ${X1})
    IF (NOT X2 STREQUAL "")
        # X2 example:  _VectorChecksum_::error<0>
        STRING (REPLACE "::" ";" X3 ${X2})
        LIST (GET  X3 0 X3class)
        LIST (GET  X3 1 X3value)
	    STRING (REGEX MATCH "[0-9]+"  X4value ${X3value})
	    STRING (REPLACE "_" "" X4class ${X3class})
	    # We add an entry into the "map-like" structure  (see https://stackoverflow.com/questions/66637169/map-in-cmake-script)
	    SET (MAP_GLOBALSIZE_${X4class} ${X4value})
        #MESSAGE ("${X4class} -> ${MAP_GLOBALSIZE_${X4class}}")
    ENDIF()
endforeach()

################################################################################
# DPU BINARIES GENERATION 
################################################################################
foreach (FULLTASKNAME ${TASKS_LIST})

    # we retrieve information from the full task name
    process_taskname (${FULLTASKNAME} TASKNAME TASKTEMPLATE)

    # we generate the DPU code file from the generic code
    configure_file (
        ${PROJECT_SOURCE_DIR}/src/bpl/utils/GetSizeGlobalParameters.cpp.in  
        ${CMAKE_CURRENT_BINARY_DIR}/${TASKNAME}.getsizeglobal.cpp   
        @ONLY
    )

    # We compute the default stack size by using the 'global size' map we created before.
    MATH (EXPR STACK_SIZE_DEFAULT "(( ( (65536*90)/100 - ${MAP_GLOBALSIZE_${TASKNAME}}) / ${NR_TASKLETS})/8)*8")
    MESSAGE ("  STACK_SIZE_DEFAULT: ${STACK_SIZE_DEFAULT} for ${FULLTASKNAME}")

    ################################################################################
    # LOOP OVER EACH TASK TYPE
    ################################################################################
    foreach (TASKTYPE A)
	
        SET (localtarget ${TASKNAME}.${TASKTYPE}.dpu)

        #MESSAGE ("-- DPU BINARY: ${localtarget}    NR_DPUS: ${NR_DPUS}   NR_TASKLETS: ${NR_TASKLETS}   STACK_SIZE_DEFAULT: ${STACK_SIZE_DEFAULT} ")

        # we generate the DPU code file from the generic code
        configure_file             (${PROJECT_SOURCE_DIR}/src/bpl/arch/dpu/ArchUpmem.dpu.${TASKTYPE}.cpp.in   ${localtarget}.cpp   @ONLY)

        # we define the source to be compiled (ie. the generated cpp file)
        add_executable             (${localtarget} ${CMAKE_CURRENT_BINARY_DIR}/${localtarget}.cpp)
        target_include_directories (${localtarget} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${DPU_INCLUDE_DIRS} /opt/upmem/share/upmem/include/syslib)
        target_compile_options     (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -O3 -DNDEBUG -fno-exceptions -fno-rtti  -D__x86_64__)
        target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT} -Wl,-S )
    
        # The following can be used for usage with dpu_profiling
        #target_link_options        (${localtarget} PUBLIC ${DPU_TARGET_RTE} -DNR_TASKLETS=${NR_TASKLETS} -DSTACK_SIZE_DEFAULT=${STACK_SIZE_DEFAULT}  -pg)

        # We need to force inclusion of default include directories for the DPU target
        # Note: must use 'target_compile_options' since 'target_include_directories' doesn't work in this context.
        target_compile_options     (${localtarget} PUBLIC ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST} )

    endforeach()  # TASKTYPE

endforeach()  # FULLTASKNAME
