//
// Created by Joseph on 16/06/2019.
//

#include "peripherals/peripherals.h"

// Memory-Mapped I/O output
void mmio_write(uint64_t reg, uint32_t data)
{
    // Cast to uintptr_t to silence compiler warning of casting to a pointer
    *(volatile uint32_t*)(uintptr_t)reg = data;
}

// Memory-Mapped I/O input
uint32_t mmio_read(uint64_t reg)
{
    // Cast to uintptr_t to silence compiler warning of casting to a pointer
    return *(volatile uint32_t*)(uintptr_t)reg;
}