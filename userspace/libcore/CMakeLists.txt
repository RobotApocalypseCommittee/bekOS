# Use only TARGET_OBJECTS of these.
add_library(crti src/${BEKOS_ARCHITECTURE}/crti.S)
add_library(crtn src/${BEKOS_ARCHITECTURE}/crtn.S)


# libcore contains the core/runtime components of a typical libc, and ergonomic C++ interfaces for bekOS syscalls.
# libc is implemented in terms of libcore primitives.
add_library(bekos_libcore STATIC
        src/${BEKOS_ARCHITECTURE}/crt0.S
        src/crt0.cpp
        src/${BEKOS_ARCHITECTURE}/syscall.S
        src/syscall.cpp
        src/allocation.cpp
        src/dir.cpp
        src/file.cpp
        src/assertions.cpp
        src/io.cpp
        src/process.cpp
        src/device.cpp)

target_include_directories(bekos_libcore
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/kernel/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(bekos_libcore
        PUBLIC -static -nostdlib -nostdinc -nostdinc++ -fno-exceptions -fno-rtti
        PRIVATE -Wall -Wextra)

target_link_options(bekos_libcore
        PUBLIC -nostdlib
        INTERFACE -nostartfiles)

target_link_libraries(bekos_libcore
        PUBLIC bek $<TARGET_OBJECTS:crti> $<TARGET_OBJECTS:crtn>)