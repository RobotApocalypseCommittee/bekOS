add_executable(kernel kernel.cpp boot.S mm.S
        peripherals/peripherals.cpp peripherals/gentimer.cpp peripherals/uart.cpp peripherals/mailbox.cpp peripherals/property_tags.cpp peripherals/emmc.cpp peripherals/system_timer.cpp peripherals/interrupt_controller.cpp
        library/liballoc.cpp library/stubs.cpp library/assert.cpp library/utility.cpp library/lock.cpp library/string.cpp library/kstring.cpp
        filesystem/mbr.cpp filesystem/fat.cpp filesystem/partition.cpp filesystem/filesystem.cpp filesystem/fatfs.cpp filesystem/entrycache.cpp filesystem/blockcache.cpp filesystem/cacheddevice.cpp filesystem/device.cpp
        printf.cpp utils.cpp page_mapping.cpp memory_manager.cpp
        interrupts/int_handle_utils.cpp interrupts/int_handle.S interrupts/int_ctrl.S
        process/process.cpp process/process_entry.S process/elf.cpp process/syscalls.cpp peripherals/framebuffer.cpp)

target_include_directories(kernel PRIVATE ../include)

target_compile_options(kernel PRIVATE -ffreestanding -fbuiltin
        -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti
        -nostdlib -nostartfiles
        -Wall -Wextra)
target_link_options(kernel PRIVATE -nodefaultlibs -nostdlib -nostartfiles -static -T${CMAKE_SOURCE_DIR}/kernel-link.ld)
target_link_libraries(kernel gcc)

set_target_properties(kernel PROPERTIES
        LINK_DEPENDS "${CMAKE_SOURCE_DIR}/kernel-link.ld"
        SUFFIX ".elf")

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)

