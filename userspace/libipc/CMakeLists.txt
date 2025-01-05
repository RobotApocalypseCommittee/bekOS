add_library(bekos_libipc src/message.cpp
        src/connection.cpp)

target_include_directories(bekos_libipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(bekos_libipc PUBLIC -static -nostdlib PRIVATE -Wall -Wextra)
target_link_libraries(bekos_libipc bek bekos_libcore)