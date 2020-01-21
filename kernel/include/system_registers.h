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

#ifndef BEKOS_SYSTEM_REGISTERS_H
#define BEKOS_SYSTEM_REGISTERS_H

// Reference = https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers

// SCTLR_EL1 register

// Reserved bits
#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
// Sets endianness
#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)
// Disables caches
#define SCTLR_I_CACHE_DISABLED          (0 << 12)
#define SCTLR_D_CACHE_DISABLED          (0 << 2)
// MMU on?
#define SCTLR_MMU_DISABLED              (0 << 0)
#define SCTLR_MMU_ENABLED               (1 << 0)

#define SCTLR_VALUE_MMU_DISABLED    (SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// HCR_EL2, Hypervisor Configuration Register (EL2)
// Aarch64 used in EL1
#define HCR_RW                  (1 << 31)
#define HCR_VALUE           HCR_RW

// SCR_EL3, Secure Configuration Register (EL3)

// Reserved bits
#define SCR_RESERVED                (3 << 4)
// Aarch64 for lower levels
#define SCR_RW              (1 << 10)
// Non-secure running
#define SCR_NS              (1 << 0)
#define SCR_VALUE                   (SCR_RESERVED | SCR_RW | SCR_NS)


// SPSR_EL3, Saved Program Status Register (EL3)
// Masks the D, I, A and F interrupts
#define SPSR_MASK_ALL           (7 << 6)
// Uses EL1 stack pointer, and returns to El1
#define SPSR_EL1h           (5 << 0)
#define SPSR_VALUE          (SPSR_MASK_ALL | SPSR_EL1h)

// TCR register controls the mapping tables
// Value of 16 -> Max mapping
#define TCR_T0SZ			(64 - 48)
// Ditto, shifted for register location in TCR
#define TCR_T1SZ			((64 - 48) << 16)
// Set the granule sizes to 4KB
#define TCR_TG0_4K			(0 << 14)
#define TCR_TG1_4K			(2 << 30)

#define TCR_VALUE			(TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)

#endif //BEKOS_SYSTEM_REGISTERS_H
