################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

set(TMP_DIR  ${CMAKE_BINARY_DIR}/export)

foreach (NAME  "helloworld" "checksum") 
	add_custom_target (export_${NAME} ALL 
	    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMP_DIR}/${NAME}
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/export/${NAME}    ${TMP_DIR}/${NAME}
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src               ${TMP_DIR}/${NAME}/include
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/scripts           ${TMP_DIR}/${NAME}/scripts
	    COMMAND ${CMAKE_COMMAND} -E copy           ${CMAKE_SOURCE_DIR}/test/unit/tasks/CMakeLists.txt ${TMP_DIR}/${NAME}/src/tasks/CMakeLists.txt
	    WORKING_DIRECTORY "${TMP_DIR}"
	    COMMAND ${CMAKE_COMMAND} -E tar "cfvz" ${CMAKE_BINARY_DIR}/${NAME}.tar.gz  ${NAME} 
	)
endforeach()

################################################################################
# CREATION OF A NEW BPL PROJECT 
################################################################################
IF (DEFINED NEWPROJECT_NAME AND NOT NEWPROJECT_NAME STREQUAL "")

    IF (NOT DEFINED NEWPROJECT_BINARY OR NEWPROJECT_BINARY STREQUAL "")
       SET (NEWPROJECT_BINARY  ${NEWPROJECT_NAME})
    ENDIF()
        
    MESSAGE ("")
    MESSAGE ("Adding target 'create_bpl_project_${NEWPROJECT_NAME}' for building new BPL project '${NEWPROJECT_NAME}'")
    MESSAGE ("")

    set(TMP_PRJ  ${TMP_DIR}/${NEWPROJECT_NAME})

    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/src/main/main.cpp.in         ${TMP_PRJ}/src/main/main.cpp                @ONLY)
    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/src/main/CMakeLists.txt      ${TMP_PRJ}/src/main/CMakeLists.txt          @ONLY)
    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/src/tasks/newproject.hpp.in  ${TMP_PRJ}/src/tasks/${NEWPROJECT_NAME}.hpp @ONLY)
    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/src/CMakeLists.txt           ${TMP_PRJ}/src/CMakeLists.txt               @ONLY)
    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/CMakeLists.txt.in            ${TMP_PRJ}/CMakeLists.txt                   @ONLY)
    configure_file (${CMAKE_SOURCE_DIR}/export/newproject/README.md.in                 ${TMP_PRJ}/README.md                        @ONLY)

	add_custom_target (create_bpl_project_${NEWPROJECT_NAME} ALL 
	    DEPENDS 
	    	${TMP_PRJ}/src/main/main.cpp
	    	${TMP_PRJ}/src/main/CMakeLists.txt
	    	${TMP_PRJ}/src/tasks/${NEWPROJECT_NAME}.hpp
            ${TMP_PRJ}/src/CMakeLists.txt	    	
	    	${TMP_PRJ}/CMakeLists.txt
	    	${TMP_PRJ}/README.md
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src               ${TMP_DIR}/${NEWPROJECT_NAME}/include
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/scripts           ${TMP_DIR}/${NEWPROJECT_NAME}/scripts
        COMMAND ${CMAKE_COMMAND} -E copy           ${CMAKE_SOURCE_DIR}/test/unit/tasks/CMakeLists.txt ${TMP_PRJ}/src/tasks/CMakeLists.txt
	    WORKING_DIRECTORY "${TMP_DIR}"
	    COMMAND ${CMAKE_COMMAND} -E tar "cfvz" ${CMAKE_BINARY_DIR}/${NEWPROJECT_NAME}.tar.gz  ${NEWPROJECT_NAME} 
	)

ENDIF()