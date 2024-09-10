# bekOS

A basic Unix-adjacent Operating System for 64-bit ARM processors written in C++. _Not the OS you wanted, but the one you
deserve._

## Features

- [x] Multitasking process management
- [x] DeviceTree parsing on boot and dynamic probing of devices.
- [x] USB xHCI host driver and peripherals
- [x] Support for QEMU virt device
- [ ] [WIP] Userspace Windowing Manager
- [ ] [WIP] Support for Raspberry Pi 4/5
- [ ] Symmetric Multiprocessing

## Project Layout

```
.
├─ bek      # Alternative C++ standard library for kernel and userspace
├─ docs     # Unsorted documentation/description
├─ kernel   # Kernel Source
├─ tools    # Scripts for running/testing
└─ userspace
   ├─ libcore   # Core (C++) library (with init + syscalls)
   └─ programs  # Executables for running in bekOS
```

## Running

1. Install freestanding toolchain (`aarch64-none-elf-gcc`) and QEMU.
2. Adapt `toolchain-arm.cmake.template` for toolchain, and use as `-DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake`.
3. Configure and build with CMake. Target `kernel_with_userspace` is recommended.
4. Run kernel with `tools/run-qemu.sh virt <EXECUTABLE>`, passing the `.img` file produced in build directory.
