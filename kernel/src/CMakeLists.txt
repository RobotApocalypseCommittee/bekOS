add_executable(kernel boot/boot.S boot/boot.cpp peripherals/peripherals.cpp peripherals/gentimer.cpp peripherals/mailbox.cpp
        peripherals/property_tags.cpp peripherals/framebuffer.cpp peripherals/uart.cpp
        library/liballoc.cpp library/stubs.cpp library/assert.cpp library/kstring.cpp
        interrupts/int_ctrl.S interrupts/int_handle.S interrupts/int_handle_utils.cpp
        printf.cpp utils.cpp memory_manager.cpp ../include/mm/hw_ptr.h mm/page_table.cpp kernel.cpp ../include/usb/usb.h ../include/usb/device.h ../include/usb/host_controller.h)

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

