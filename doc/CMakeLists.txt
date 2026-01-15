################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

find_package (Doxygen)

if (DOXYGEN_FOUND)
    CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/doxygen/bpl.doxyfile  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile  @ONLY)
    ADD_CUSTOM_TARGET (doxygen  ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/doxygen.cs)
endif (DOXYGEN_FOUND)

ADD_SUBDIRECTORY(doc)
