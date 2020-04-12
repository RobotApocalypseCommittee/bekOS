# Sets toolchain location - pls fill in with your own
set(toolchain_location /home/???/opt/gcc-aarch64)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_C_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)
set(CMAKE_CXX_COMPILER ${toolchain_location}/bin/aarch64-elf-g++)
set(CMAKE_ASM_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)

set(objcopy_location ${toolchain_location}/bin/aarch64-elf-objcopy)

set(freestanding_include_directories)
# Includes the compiler-provided headers
list(APPEND freestanding_include_directories
        ${toolchain_location}/lib/gcc/aarch64-elf/8.2.1/include)
list(APPEND freestanding_include_directories
        ${toolchain_location}/lib/gcc/aarch64-elf/8.2.1/include-fixed)

#set(CMAKE_CXX_LINK_EXECUTABLE "${toolchain_location}/bin/aarch64-elf-ld -nostdlib -nostartfiles ")

# Sets cmake search locations if we ever decide to use "FIND_*" functions
# This will only search the host system for programs, and only the target sysroot for libs, packages, and headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)