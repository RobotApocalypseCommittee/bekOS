

add_executable(kernel boot.S mm.S kernel.cpp
        peripherals/peripherals.cpp peripherals/gentimer.cpp peripherals/uart.cpp peripherals/mailbox.cpp peripherals/property_tags.cpp peripherals/emmc.cpp peripherals/system_timer.cpp peripherals/interrupt_controller.cpp
        library/liballoc.cpp library/stubs.cpp
        filesystem/mbr.cpp filesystem/fat.cpp filesystem/partition.cpp filesystem/filesystem.cpp filesystem/fatfs.cpp filesystem/entrycache.cpp
        printf.c utils.cpp page_mapping.cpp memory_manager.cpp library/kstring.c library/assert.c
        interrupts/int_handle_utils.cpp interrupts/int_handle.S interrupts/int_ctrl.S
        process/process.cpp process/process_entry.S process/elf.cpp process/syscalls.cpp filesystem/sectorcache.cpp library/utility.cpp)
target_include_directories(kernel PRIVATE ${freestanding_include_directories} ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_compile_options(kernel PRIVATE -ffreestanding -nostdlib -nodefaultlibs -nostartfiles -Wall -Wextra -fno-exceptions -fno-rtti)
set_target_properties(kernel PROPERTIES
        LINK_FLAGS "-static -nostdlib -nostartfiles -T ${CMAKE_SOURCE_DIR}/kernel-link.ld -pie -lgcc"
        LINK_DEPENDS "${CMAKE_SOURCE_DIR}/kernel-link.ld"
        SUFFIX ".elf")

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)
