cmake_minimum_required(VERSION 3.13)

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

set(source_files)
list(APPEND source_files
        boot.S
        kernel.cpp)

set(kernel_include_directories)
# Sets toolchain locations
if(LINUX)
    set(toolchain_location /home/Attoa/opt/gcc-arm-8.2-x86_64-aarch64-elf)
    set(CMAKE_C_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)
    set(CMAKE_CXX_COMPILER ${toolchain_location}/bin/aarch64-elf-g++)
    set(CMAKE_ASM_COMPILER ${toolchain_location}/bin/aarch64-elf-gcc)

    set(objcopy_location ${toolchain_location}/bin/aarch64-elf-objcopy)

    # Includes the compiler-provided headers
    list(APPEND kernel_include_directories
            ${toolchain_location}/lib/gcc/aarch64-elf/8.2.1/include)

    #set(CMAKE_CXX_LINK_EXECUTABLE "${toolchain_location}/bin/aarch64-elf-ld -nostdlib -nostartfiles ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib -nostartfiles -T ${CMAKE_SOURCE_DIR}/kernel-link.ld -o kernel.elf")
endif()

# Sets cmake search locations if we ever decide to use "FIND_*" functions
# This will only search the host system for programs, and only the target sysroot for libs, packages, and headers
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Assembly flags to create boot code
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding -nostdinc -nostdlib -nostartfiles")
# C++ Flags to create boot code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -nostdinc -nostdlib -nostartfiles")

add_executable(kernel ${source_files})

set_target_properties(kernel PROPERTIES SUFFIX ".elf")
target_include_directories(kernel PRIVATE ${kernel_include_directories})

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)
