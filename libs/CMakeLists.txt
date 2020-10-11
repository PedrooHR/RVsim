cmake_minimum_required(VERSION 3.10)

include_directories("../include/")

set(SRC_FILES
    registers.cpp
    memory.cpp
    isa.cpp
    processor.cpp
    utils.cpp
)

set_source_files_properties(${SRC_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(utils STATIC utils.cpp)
add_library(registers STATIC registers.cpp)
add_library(memory STATIC memory.cpp)
add_library(isa STATIC isa.cpp)
add_library(processor STATIC processor.cpp)

install(TARGETS utils ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}")
install(TARGETS registers ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}")
install(TARGETS memory ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}")
install(TARGETS isa ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}")
install(TARGETS processor ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}")

target_link_libraries(processor
    utils 
    memory 
    isa 
    registers)
