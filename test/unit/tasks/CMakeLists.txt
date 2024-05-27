################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2024
## author: edrezen
################################################################################

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
execute_process (COMMAND ${CMAKE_SOURCE_DIR}/scripts/FindDefaultIncludes.py  OUTPUT_VARIABLE CXX_DEFAULT_INCLUDE_DIRECTORIES)
separate_arguments (CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST UNIX_COMMAND "${CXX_DEFAULT_INCLUDE_DIRECTORIES}")

MESSAGE ("CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST: ${CXX_DEFAULT_INCLUDE_DIRECTORIES_LIST}")

################################################################################
# BPL tests (DPU)
################################################################################

foreach (FULLTASKNAME
	"Bank1" "Bank2" "Bank3" "Bank4" "Bank5"
	"Parrot1" "Parrot2" "Parrot3" "Parrot4" "Parrot5" "SumGeneric" "CountInstances" "GetPuid" "DoSomething"
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
	"Span1"
	"VectorSwap"
	"Once1"
	"Global1"
)

 foreach (TASKSIZE ${BROADCAST_SIZE_LIST}) 

  foreach (TASKTYPE A)

	# we try to recover potential type information from the full taskname (for instance "TemplateTask<std::string>") 
	string(REGEX REPLACE "<|>" " " TASKNAME_LIST ${FULLTASKNAME})
	separate_arguments (TASKNAME_LIST)
	list (LENGTH TASKNAME_LIST length)
	list (GET    TASKNAME_LIST 0 TASKNAME)
	
	if (length EQUAL 3)
		list (GET TASKNAME_LIST 1 TASKTEMPLATE)
		set (TASKTEMPLATE ",${TASKTEMPLATE}")
	else()
		set (TASKTEMPLATE "")  # no template defined
	endif()
	
    SET (localtarget ${TASKNAME}.${TASKTYPE}.${TASKSIZE}.dpu)

    #MESSAGE ("-- DPU BINARY: ${localtarget}    NR_DPUS: ${NR_DPUS}   NR_TASKLETS: ${NR_TASKLETS}   STACK_SIZE_DEFAULT: ${STACK_SIZE_DEFAULT} ")
	#MESSAGE ("  ${FULLTASKNAME} => ${length}  and ${TASKNAME}  and  ${TASKTEMPLATE}")

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

  endforeach()

 endforeach()

endforeach()
