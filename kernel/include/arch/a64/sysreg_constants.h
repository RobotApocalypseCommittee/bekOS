/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_SYSREG_CONSTANTS_H
#define BEKOS_SYSREG_CONSTANTS_H

// --- MAIR ---

#define MAIR_DEVICE_nGnRnE_INDEX 0x0
#define MAIR_NORMAL_NC_INDEX 0x1

#define MAIR_DEVICE_nGnRnE_FLAGS \
    0x00ul  // Device non-Gathering, non-Reorderable, no-Early-Write-Acknowledgement
#define MAIR_NORMAL_NC_FLAGS 0x44ul  // Outer non-Cacheable, Inner non-Cacheable
#define MAIR_VALUE                                                  \
    ((MAIR_DEVICE_nGnRnE_FLAGS << (8 * MAIR_DEVICE_nGnRnE_INDEX)) | \
     (MAIR_NORMAL_NC_FLAGS << (8 * MAIR_NORMAL_NC_INDEX)))

// --- TCR ---

// TOSZ = 16 -> 4 levels, 48 bits.
#define TCR_T0SZ (16ul << 0)

// EPD0 - disabled lower-half translation
#define TCR_TTBR0_DISABLE (1ul << 7)

// I/O GRN0 - lower-half walk cacheability - outer & inner write-back cacheable
#define TCR_I_O_GRN0 (0x11ul << 8)

// SH0 - lower-half walk shareability
#define TCR_SHO_INNER (0b11ul << 12)

// TG0 - lower-half granule size (0 = 4K, 1 = 64K, 2 = 16K)
#define TCR_TG0_4K (0ul << 14)

// T1SZ = 16 -> 4 levels, 48 bits.
#define TCR_T1SZ (16ul << 16)

// A1 - upper- or lower- half sets ASID
#define TCR_A1_LOWER (0ul << 22)

// EPD1 - disabled upper-half translation
#define TCR_TTBR1_DISABLE (1ul << 23)

// I/O GRN1 - upper-half walk cacheability - outer & inner write-back cacheable
#define TCR_I_O_GRN1 (0x11ul << 24)

// SH1 - upper-half walk shareability
#define TCR_SH1_INNER (0b11ul << 28)

// TG1 - upper-half granule size (2 = 4K, 3 = 64K, 1 = 16K)
#define TCR_TG1_4K (2ul << 30)

// IPS - intermediate phys bits - should be filled in by user
#define TCR_IPS_SHIFT 32

// ASID size - 1 for enable
#define TCR_ASID_ENABLE (1ul << 36)

// TBI (Top Byte Ignore) (1 for using as tag etc.)
#define TCR_TBI0 (1ul << 37)
#define TCR_TBI1 (1ul << 38)

#define TCR_VALUE(ips_val)                                                               \
    ((ips_val << TCR_IPS_SHIFT) | TCR_T0SZ | TCR_I_O_GRN0 | TCR_SHO_INNER | TCR_TG0_4K | \
     TCR_T1SZ | TCR_I_O_GRN1 | TCR_SH1_INNER | TCR_TG1_4K)

// Reference = https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers

// --- SCTLR ---

// Reserved bits
#define SCTLR_RESERVED ((3ul << 28) | (3ul << 22) | (1ul << 20) | (1ul << 11))
// Sets endianness
#define SCTLR_EE_BIG_ENDIAN (1ul << 25)
#define SCTLR_EOE_BIG_ENDIAN (1ul << 24)

// Disables caches
#define SCTLR_I_CACHE_ENABLED (1ul << 12)
#define SCTLR_D_CACHE_ENABLED (1ul << 2)

// Alignment Checks
#define SCTLR_SP_EL0_CHECK (1ul << 4)
#define SCTLR_SP_EL1_CHECK (1ul << 3)
#define SCTLR_DATA_CHECK (1ul << 1)

// MMU
#define SCTLR_MMU_ENABLED (1ul << 0)

#define SCTLR_VALUE_MMU_DISABLED (SCTLR_RESERVED)
#define SCTLR_VALUE_MMU_ENABLED (SCTLR_RESERVED | SCTLR_MMU_ENABLED)

// HCR_EL2, Hypervisor Configuration Register (EL2)
// Aarch64 used in EL1
#define HCR_RW (1 << 31)
#define HCR_SWIO (1 << 1)  // Hardwired on Pi3
#define HCR_VALUE (HCR_RW | HCR_SWIO)

// SCR_EL3, Secure Configuration Register (EL3)

// Reserved bits
#define SCR_RESERVED (3 << 4)
// Aarch64 for lower levels
#define SCR_RW (1 << 10)
// Non-secure running
#define SCR_NS (1 << 0)
#define SCR_VALUE (SCR_RESERVED | SCR_RW | SCR_NS)

// SPSR_EL3, Saved Program Status Register (EL3)
// Masks the D, I, A and F interrupts
#define SPSR_MASK_ALL (7 << 6)
// Uses EL2 stack pointer, returning to EL2
#define SPSR_EL2h (9 << 0)
#define SPSR_EL1h (5 << 0)
#define SPSR_EL3_VALUE (SPSR_MASK_ALL | SPSR_EL2h)
#define SPSR_EL2_VALUE (SPSR_MASK_ALL | SPSR_EL1h)

#endif  // BEKOS_SYSREG_CONSTANTS_H
