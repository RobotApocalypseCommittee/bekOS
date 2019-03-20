cmake_minimum_required(VERSION 3.12)

project(Bekos ASM C CXX)
set(CMAKE_CXX_STANDARD 14)

# Cross compiler things
set(CMAKE_SYSTEM_NAME Generic)

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# May be wanted later
#set(CMAKE_SYSROOT .)
#set(CMAKE_STAGING_PREFIX .)


set(freestanding_include_directories)
# Sets toolchain locations
if(LINUX)
    set(toolchain_location /home/joebe/opt/gcc-aarch64)
    set(CMAKE_C_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)
    set(CMAKE_CXX_COMPILER ${toolchain_location}/bin/aarch64-elf-g++)
    set(CMAKE_ASM_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)

    set(objcopy_location ${toolchain_location}/bin/aarch64-elf-objcopy)

    # Includes the compiler-provided headers
    list(APPEND freestanding_include_directories
            ${toolchain_location}/lib/gcc/aarch64-elf/8.2.1/include)

    #set(CMAKE_CXX_LINK_EXECUTABLE "${toolchain_location}/bin/aarch64-elf-ld -nostdlib -nostartfiles ")
endif()

# Sets cmake search locations if we ever decide to use "FIND_*" functions
# This will only search the host system for programs, and only the target sysroot for libs, packages, and headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


add_subdirectory(kernel/src)


