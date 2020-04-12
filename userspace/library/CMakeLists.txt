add_library(bekoslibc src/syscalls.S)

target_include_directories(bekoslibc
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(bekoslibc PUBLIC -static -nostdlib PRIVATE -Wall -Wextra)
target_link_libraries(bekoslibc PUBLIC -static -nostdlib -nostartfiles)