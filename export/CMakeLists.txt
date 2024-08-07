################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2023
## author: edrezen
################################################################################

set(TMP_DIR  ${CMAKE_BINARY_DIR}/export)

foreach (NAME  "helloworld" "checksum") 

	add_custom_target (export_${NAME} ALL 
	    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMP_DIR}/${NAME}
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/export/${NAME}    ${TMP_DIR}/${NAME}
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src               ${TMP_DIR}/${NAME}/include
	    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/scripts           ${TMP_DIR}/${NAME}/scripts
	    WORKING_DIRECTORY "${TMP_DIR}"
	    COMMAND ${CMAKE_COMMAND} -E tar "cfvz" ${CMAKE_BINARY_DIR}/${NAME}.tar.gz  ${NAME} 
	)

endforeach()
