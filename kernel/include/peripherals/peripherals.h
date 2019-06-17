//
// Created by Joseph on 27/03/2019.
//

#ifndef BEKOS_PERIPHERALS_H
#define BEKOS_PERIPHERALS_H

#include <stdint.h>

// Useful info
#include "memory_locations.h"

void mmio_write(uint64_t reg, uint32_t data);
uint32_t mmio_read(uint64_t reg);

#endif //BEKOS_PERIPHERALS_H
