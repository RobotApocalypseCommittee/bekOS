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

#include "arch/a64/boot_info.h"
#include "arch/a64/translation_tables.h"
#include "library/types.h"
#include "peripherals/uart.h"

extern "C" {
extern u8 __kernel_start, __kernel_end, __kernel_ro_end, __kernel_rw_start,
    __initial_pgtables_start, __initial_pgtables_end, __devtree_start;
}

extern "C" {
extern uPtr g_current_embedded_table_phys;
}

#define SIZE_2M (2ull << 20)

#define PT_NORMAL_RE (ReadOnly | AF | (MAIR_NORMAL_NC_INDEX << 2))
#define PT_NORMAL_RW (PrivilegedExecuteNever | AF | (MAIR_NORMAL_NC_INDEX << 2))
#define PT_NORMAL_RO (ReadOnly | PrivilegedExecuteNever | AF | (MAIR_NORMAL_NC_INDEX << 2))
#define PT_DEVICE (AF | (MAIR_DEVICE_nGnRnE_INDEX << 2))

uSize devtree_mapping_size(uPtr device_tree_address) {
    const u8* dev_tree_content = reinterpret_cast<const u8*>(device_tree_address);
    // Read big-endian from byte-offset 0x4
    u32 device_tree_size = (u32)dev_tree_content[4] << 24 | (u32)dev_tree_content[5] << 16 |
                           (u32)dev_tree_content[6] << 8 | dev_tree_content[7];
    // Round down start address
    uPtr rounded_start = device_tree_address & -SIZE_2M;
    // Round up end address
    uPtr rounded_end = (device_tree_address + device_tree_size + SIZE_2M - 1) & -SIZE_2M;
    return rounded_end - rounded_start;
}

[[gnu::always_inline]] uPtr kptr_virt(uPtr ptr, uPtr k_phys) {
    if (ptr >= KERNEL_VBASE) {
        return ptr;
    } else {
        return ptr - k_phys + KERNEL_VBASE;
    }
}

[[gnu::always_inline]] uPtr kptr_phys(uPtr ptr, uPtr k_phys) {
    if (ptr >= KERNEL_VBASE) {
        return ptr - KERNEL_VBASE + k_phys;
    } else {
        return ptr;
    }
}

extern "C" i64 setup_early_tables(uPtr load_address, uPtr device_tree_address) {
    uPtr qemu_pl011_address      = 0x900'0000;
    uPtr virt_qemu_pl011_address = VA_START + 0x900'0000;

    uPtr virt_kernel_start = kptr_virt((uPtr)&__kernel_start, load_address);

    uPtr virt_exec_start = virt_kernel_start;
    uSize exec_size      = &__kernel_ro_end - &__kernel_start;
    uPtr phys_exec_start = kptr_phys(virt_exec_start, load_address);

    uPtr virt_data_start = kptr_virt(reinterpret_cast<uPtr>(&__kernel_rw_start), load_address);
    uSize data_size      = &__kernel_end - &__kernel_rw_start;
    uPtr phys_data_start = kptr_phys(virt_data_start, load_address);

    uPtr pgtables_start =
        kptr_phys(reinterpret_cast<uPtr>(&__initial_pgtables_start), load_address);
    uPtr pgtables_end = kptr_phys(reinterpret_cast<uPtr>(&__initial_pgtables_end), load_address);

    uPtr virt_devtree_start = kptr_virt(reinterpret_cast<uPtr>(&__devtree_start), load_address);
    uPtr phys_devtree_start = device_tree_address & -SIZE_2M;
    // We need to add the offset if there was one.
    uPtr device_tree_virtual_pointer =
        reinterpret_cast<uPtr>(&__devtree_start) + (device_tree_address & (SIZE_2M - 1));
    uSize devtree_size = devtree_mapping_size(device_tree_address);

    uPtr pgtable_current = pgtables_start;

    if (!crude_map_region(virt_exec_start, phys_exec_start, exec_size, PT_NORMAL_RE, pgtables_start,
                          pgtable_current, pgtables_end)) {
        return -1;
    }

    if (!crude_map_region(virt_data_start, phys_data_start, data_size, PT_NORMAL_RW, pgtables_start,
                          pgtable_current, pgtables_end)) {
        return -2;
    }

    if (!crude_map_region(virt_devtree_start, phys_devtree_start, devtree_size, PT_NORMAL_RO,
                          pgtables_start, pgtable_current, pgtables_end)) {
        return -3;
    }

    if (!crude_map_region(phys_exec_start, phys_exec_start, exec_size, PT_NORMAL_RE, pgtables_start,
                          pgtable_current, pgtables_end)) {
        return -4;
    }

    // TODO: Do not do.
    if (!crude_map_region(virt_qemu_pl011_address, qemu_pl011_address, SIZE_2M, PT_DEVICE,
                          pgtables_start, pgtable_current, pgtables_end)) {
        return -5;
    }

    *reinterpret_cast<uPtr*>(kptr_phys((uPtr)&g_current_embedded_table_phys, load_address)) =
        pgtable_current;

    // Get AArch64 Memory Model Feature Register 0
    u64 a64_mem_feature_reg;
    asm volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(a64_mem_feature_reg));
    if ((a64_mem_feature_reg >> 28) & 0xF) {
        // Does not (straighforwardly) support 4KB pages. Value of 1 may be ok.
        return -6;
    }

    u64 physical_addr_size_tag = a64_mem_feature_reg & 0xF;
    u64 tcr_value              = TCR_VALUE(physical_addr_size_tag);

    asm volatile("msr mair_el1, %0" : : "r"(MAIR_VALUE));
    // Insert an ISB in case the TTBR registers care about addr space settings.
    asm volatile("msr tcr_el1, %0; isb" : : "r"(tcr_value));

    // We enable the CnP bit (this may be wrong - TODO)
    asm volatile("msr ttbr0_el1, %0" : : "r"(pgtables_start | 1));
    asm volatile("msr ttbr1_el1, %0" : : "r"(pgtables_start | 1));

    asm volatile("dsb ish; isb");
    return 0;
}
