add_executable(kernel arch/a64/boot.S
        library/kstring.cpp
        library/string.cpp
        library/format.cpp
        library/utility.cpp
        library/assertions.cpp
        interrupts/int_ctrl.S
        interrupts/int_handle.S
        interrupts/int_handle_utils.cpp
        peripherals/device_tree.cpp
        peripherals/arm_gic.cpp
        peripherals/peripherals.cpp
        peripherals/gentimer.cpp
        peripherals/mailbox.cpp
        peripherals/property_tags.cpp
        peripherals/framebuffer.cpp
        peripherals/uart.cpp
        peripherals/pcie.cpp
        peripherals/interrupt_controller.cpp
        mm/page_allocator.cpp
        mm/memory_manager.cpp
        mm/kmalloc.cpp
        usb/xhci_ring.cpp
        usb/descriptors.cpp
        usb/usb.cpp
        usb/hid.cpp
        usb/xhci.cpp
        utils.cpp
        arch/a64/early_boot.cpp
        arch/a64/translation_tables.cpp
        peripherals/qemu.cpp
        peripherals/virtio.cpp
        peripherals/virtio/block.cpp
        mm/dma_utils.cpp
        filesystem/block_device.cpp
        filesystem/partition.cpp
)

if (BEKOS_SYSTEM_NAME STREQUAL "rpi3")
    target_sources(kernel PRIVATE kernel.cpp boot/boot.cpp)
elseif (BEKOS_SYSTEM_NAME STREQUAL "virt")
    target_sources(kernel PRIVATE arch/a64/boot.cpp arch/a64/impl.cpp arch/a64/impl.S)
endif ()

target_include_directories(kernel PRIVATE ../include)

target_compile_options(kernel PRIVATE -ffreestanding -fbuiltin
        -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti
        -nostdlib -nostartfiles -Waddress-of-packed-member
        -Wall -Wextra -march=armv8-a+nofp -fmacro-prefix-map=${PROJECT_SOURCE_DIR}=.)
target_link_options(kernel PRIVATE -nodefaultlibs -nostdlib -nostartfiles -T${CMAKE_CURRENT_SOURCE_DIR}/arch/a64/kernel-link.ld)
target_link_libraries(kernel gcc)

set_target_properties(kernel PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/arch/a64/kernel-link.ld"
        SUFFIX ".elf"
        POSITION_INDEPENDENT_CODE FALSE)

add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${objcopy_location}" "${CMAKE_CURRENT_BINARY_DIR}/kernel.elf" "-O" "binary" "${CMAKE_BINARY_DIR}/kernel8.img"
        VERBATIM)

