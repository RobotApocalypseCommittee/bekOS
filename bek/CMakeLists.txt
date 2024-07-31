add_library(bek src/string.cpp src/utility.cpp
        src/testing_ground.cpp
        include/bek/time.h
        src/time.cpp
        src/c_string.cpp)

target_include_directories(bek PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(bek PRIVATE cxx_std_20)

target_compile_options(bek PUBLIC -static -nostdlib -nostdinc++ PRIVATE -Wall -Wextra)