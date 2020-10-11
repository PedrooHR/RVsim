cmake_minimum_required(VERSION 3.10)

include_directories("../include/")
set(SRC_FILES
    main.cpp
)

set_source_files_properties(${SRC_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(rvsim main.cpp)

target_link_libraries(rvsim 
    utils
    memory
    isa
    registers
    processor)
