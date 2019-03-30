//
// Created by Joseph on 27/03/2019.
//

#ifndef BEKOS_MEMORY_LOCATIONS_H
#define BEKOS_MEMORY_LOCATIONS_H

// The start of virtual addresses in KERNEL
// All (explicit) memory accesses should be relative to this
#define VA_START 0xffff000000000000

// We have 1GB memory on Pi
#define ADDRESSABLE_MEMORY 0x40000000



#endif //BEKOS_MEMORY_LOCATIONS_H
