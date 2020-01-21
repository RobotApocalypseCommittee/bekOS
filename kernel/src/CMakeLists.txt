

add_executable(kernel boot.S mm.S kernel.cpp peripherals/peripherals.cpp peripherals/gentimer.cpp printf.c peripherals/uart.cpp peripherals/mailbox.cpp peripherals/property_tags.cpp utils.cpp peripherals/emmc.cpp page_mapping.cpp memory_manager.cpp filesystem/mbr.cpp liballoc/liballoc.cpp liballoc/stubs.cpp filesystem/fat.cpp filesystem/partition.cpp kernel_lib/kstring.c filesystem/filesystem.cpp filesystem/fatfs.cpp library/assert.c filesystem/entrycache.cpp)
target_include_directories(kernel PRIVATE ${freestanding_include_directories} ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(kernel PRIVATE -ffreestanding -nostdlib -nodefaultlibs -nostartfiles -Wall -Wextra -fno-exceptions -fno-rtti)
set_target_properties(kernel PROPERTIES
        LINK_FLAGS "-nostdlib -nostartfiles -T ${CMAKE_SOURCE_DIR}/kernel-link.ld -pie -lgcc"
        LINK_DEPENDS "${CMAKE_SOURCE_DIR}/kernel-link.ld"
        SUFFIX ".elf")

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)
