cmake_minimum_required(VERSION 3.10)

project(Bekos ASM C CXX)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall -Wextra -Waddress-of-packed-member -Wno-sized-deallocation -Wno-unused-parameter -Werror=return-type)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif ()
endif ()

find_program(QEMU_EXECUTABLE qemu-system-aarch64 REQUIRED)

set(BEKOS_SYSTEM_NAME "virt" CACHE STRING "System name. Supports: virt, rpi3")
set(BEKOS_ARCHITECTURE "a64" CACHE STRING "System architecture. Supports: a64")

add_subdirectory(bek)
add_subdirectory(kernel/src)
add_subdirectory(userspace)

add_custom_target(kernel_with_userspace)
add_dependencies(kernel_with_userspace kernel bekos_userspace)

if (NOT EXISTS "${QEMU_EXECUTABLE}")
    message(WARNING "${QEMU_EXECUTABLE}")
    message(WARNING "Could not find qemu executable - will be unable to run.")
else ()
    add_custom_target(run ${CMAKE_SOURCE_DIR}/tools/run-qemu.sh "${BEKOS_SYSTEM_NAME}" "$<TARGET_FILE:kernel>" "${QEMU_EXECUTABLE}" VERBATIM)
    add_dependencies(run kernel)

    add_custom_target(rundbg ${CMAKE_SOURCE_DIR}/tools/run-qemu.sh "${BEKOS_SYSTEM_NAME}" "$<TARGET_FILE:kernel>" "${QEMU_EXECUTABLE}" "-d" VERBATIM)
    add_dependencies(rundbg kernel)
endif()




