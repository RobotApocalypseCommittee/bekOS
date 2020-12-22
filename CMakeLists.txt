cmake_minimum_required(VERSION 3.10)

project(Bekos ASM C CXX)
set(CMAKE_CXX_STANDARD 17)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif ()
endif ()

set(QEMU_EXECUTABLE "" CACHE STRING "Set manually if failed to discover, or on windows.")
option(WINDOWS_QEMU "Bridge: using a windows qemu executable." FALSE)
if ((QEMU_EXECUTABLE STREQUAL "") AND NOT WINDOWS_QEMU)
    find_program(QEMU_EXECUTABLE qemu-system-aarch64)
endif()

add_subdirectory(kernel/src)
add_subdirectory(userspace)

if (NOT EXISTS "${QEMU_EXECUTABLE}")
    message(WARNING "${QEMU_EXECUTABLE}")
    message(WARNING "Could not find qemu executable - will be unable to run.")
elseif(WINDOWS_QEMU)
    add_custom_target(run ${CMAKE_SOURCE_DIR}/tools/run-qemu.sh "${QEMU_EXECUTABLE}" "$<TARGET_FILE:kernel>" VERBATIM)
    add_dependencies(run kernel)
endif()


