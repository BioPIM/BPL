################################################################################
## BPL, the Process In Memory library for bioinformatics 
## date  : 2026
## author: edrezen
################################################################################

find_package(LATEX COMPONENTS PDFLATEX)

if(LATEX_FOUND)
    ADD_CUSTOM_TARGET (
    	userguide  
    	COMMAND TEXINPUTS=${CMAKE_CURRENT_SOURCE_DIR}/userguide: ${PDFLATEX_COMPILER} -shell-escape  ${CMAKE_CURRENT_SOURCE_DIR}/userguide/BplUserGuide.tex
    )
endif()