################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

find_package(PkgConfig)
pkg_check_modules(DPU REQUIRED dpu)

################################################################################

set (CMAKE_CXX_STANDARD      20)
set (CMAKE_CXX_FLAGS         "-Wall -Wextra -Wno-unused-parameter -O3 -DNDEBUG -g -fdiagnostics-color=always")

# Using this flag will add a checksum while serializing data from host to DPUs.
#set (WITH_SERIALIZATION_HASH_CHECK 1)
 
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
    "VectorChecksum" "VectorChecksumOnce"
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
    "Syracuse" "SyracuseReduce" "SyracuseVector"
)

# We define the location of the BPL. This variable can change according to the project (unit tests, exported project...)
set (BPL_SOURCE_DIR  ${PROJECT_SOURCE_DIR}/src/bpl)
    
################################################################################
ADD_SUBDIRECTORY (host)
ADD_SUBDIRECTORY (tasks)

# Finally, we dump a message to tell how to configure some env var
MESSAGE ("")
MESSAGE ("-------------------------------------------------------------------------------------------")
MESSAGE ("PLEASE RUN THE FOLLOWING COMMAND:")
MESSAGE ("")
MESSAGE ("    export set DPU_BINARIES_DIR=${CMAKE_CURRENT_BINARY_DIR}")
MESSAGE ("")
MESSAGE ("-------------------------------------------------------------------------------------------")
MESSAGE ("")

