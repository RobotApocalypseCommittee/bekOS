/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "peripherals/peripherals.h"

extern bool is_upper_half;

uPtr bus_address(uPtr mapped_address) {
    return mapped_address & (~VA_START);
}
uPtr mapped_address(uPtr bus_address) {
    return bus_address | VA_START;
}
uPtr gpu_address(uPtr mapped_address) { return bus_address(mapped_address) + 0xC0000000; }



void mmio_register_device::write_register(uPtr reg, u32 value) {
    volatile u32* ptr = reinterpret_cast<volatile u32*>(peripheral_base + reg);
    *ptr = value;
}
u32 mmio_register_device::read_register(uPtr reg) {
    volatile u32* ptr = reinterpret_cast<volatile u32*>(peripheral_base + reg);
    return *ptr;
}
