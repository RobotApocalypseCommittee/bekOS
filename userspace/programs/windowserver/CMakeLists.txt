bekos_program(windowserver SOURCES windowserver.cpp)
target_link_libraries(program_windowserver PUBLIC bekos_libcore bekos_libwindow bekos_libipc)