add_executable(kernel
        library/assertions.cpp
        library/block_cache.cpp
        library/user_buffer.cpp
        interrupts/int_handle.S
        interrupts/deferred_calls.cpp
        interrupts/interrupt_handler.cpp
        interrupts/error_handler.cpp
        peripherals/device_tree.cpp
        peripherals/arm/arm_gic.cpp
        peripherals/peripherals.cpp
        peripherals/arm/gentimer.cpp
        peripherals/mailbox.cpp
        peripherals/property_tags.cpp
        peripherals/framebuffer.cpp
        peripherals/uart.cpp
        peripherals/pcie.cpp
        peripherals/interrupt_controller.cpp
        peripherals/virtio.cpp
        peripherals/virtio/block.cpp
        peripherals/virtio/framebuffer.cpp
        peripherals/device.cpp
        peripherals/raspberry/framebuffer.cpp
        mm/page_allocator.cpp
        mm/memory_manager.cpp
        mm/kmalloc.cpp
        mm/dma_utils.cpp
        mm/space_manager.cpp
        mm/backing_region.cpp
        mm/device_backed_region.cpp
        mm/addresses.cpp
        usb/xhci_ring.cpp
        usb/descriptors.cpp
        usb/usb.cpp
        usb/hid.cpp
        usb/xhci.cpp
        utils.cpp
        filesystem/block_device.cpp
        filesystem/partition.cpp
        filesystem/fatfs.cpp
        filesystem/fat.cpp
        filesystem/filesystem.cpp
        filesystem/entry.cpp
        process/process.cpp
        process/entity.cpp
        process/elf.cpp
        process/syscalls.cpp
        process/tty.cpp
        peripherals/timing.cpp
        library/kernel_error.cpp
)

if (BEKOS_ARCHITECTURE STREQUAL "a64")
    target_sources(kernel PRIVATE
            arch/a64/cxxabi.cpp
            arch/a64/saved_registers.cpp
            arch/a64/process.S
            arch/a64/barrier.cpp
            arch/a64/boot.S
            arch/a64/early_boot.cpp
            arch/a64/translation_tables.cpp
            arch/a64/process_entry.cpp
            arch/a64/impl.cpp
            arch/a64/syscontrol.S)
endif ()

if (BEKOS_SYSTEM_NAME STREQUAL "rpi3")
    target_sources(kernel PRIVATE boot/boot.cpp)
elseif (BEKOS_SYSTEM_NAME STREQUAL "virt")
    target_sources(kernel PRIVATE arch/a64/boot.cpp)
endif ()

target_include_directories(kernel PRIVATE ../include)

target_compile_options(kernel PRIVATE -ffreestanding -fbuiltin
        -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti
        -nostdlib -nostartfiles -march=armv8-a+nofp -fmacro-prefix-map=${PROJECT_SOURCE_DIR}=.)
target_link_options(kernel PRIVATE -nodefaultlibs -nostdlib -nostartfiles -T${CMAKE_CURRENT_SOURCE_DIR}/arch/a64/kernel-link.ld)
target_link_libraries(kernel gcc bek)

set_target_properties(kernel PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/arch/a64/kernel-link.ld"
        SUFFIX ".elf"
        POSITION_INDEPENDENT_CODE FALSE)

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)

