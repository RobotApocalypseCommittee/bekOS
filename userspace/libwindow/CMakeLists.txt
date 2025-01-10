add_library(bekos_libwindow src/core.cpp
        src/gfx.cpp
        src/ssfn.cpp
        src/ssfn.h
        src/button.cpp
        src/widget.cpp
        include/window/Window.gen.cpp
        src/window.cpp)

target_include_directories(bekos_libwindow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(bekos_libwindow PUBLIC -static -nostdlib PRIVATE -Wall -Wextra)
target_link_libraries(bekos_libwindow bek bekos_libcore bekos_libipc)