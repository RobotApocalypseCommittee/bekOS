/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
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

#include "peripherals/arm_gic.h"

#include "interrupts/int_ctrl.h"
#include "mm/memory_manager.h"

namespace dist {
constexpr uSize CTLR         = 0x000;
constexpr u32 CTLR_DISABLE   = 0;
constexpr u32 CTLR_ENABLE    = 1;
constexpr u32 CTLR_ENABLE_G0 = 1;
constexpr u32 CTLR_ENABLE_G1 = 2;

constexpr uSize TYPER = 0x004;
constexpr u32 TYPER_LINES_NUMBER(u32 typer) {
    return bek::min(32 * ((typer & 0b11111) + 1), 1020u);
}
constexpr u32 TYPER_CPU_NUMBER(u32 typer) { return ((typer >> 5) & 0b111) + 1; }

constexpr uSize IIDR       = 0x008;
constexpr uSize IGROUPRn   = 0x080;
constexpr uSize ISENABLERn = 0x100;
constexpr uSize ICENABLERn = 0x180;
constexpr uSize ISPENDRn   = 0x200;
constexpr uSize ICPENDRn   = 0x280;
constexpr uSize ISACTIVERn = 0x300;
constexpr uSize ICACTIVERn = 0x380;

/// Not bitmap
constexpr uSize IPRIORITYRn = 0x400;

// Only use top four bits, because those are the ones guaranteed to be supported.
constexpr u8 IPRIORITY_DEFAULT = 0x80;

/// Minumum prioity value (as PMR, will allow all through).
constexpr u8 IPRIORITY_LOWEST = 0xF0;

constexpr uSize ITARGETSRn = 0x800;
constexpr u8 ITARGETS_CPU0 = 1;

constexpr uSize ICFGRn     = 0xC00;
constexpr uSize NSACRn     = 0xE00;
constexpr uSize SGIR       = 0xF00;
constexpr uSize CPENDSGIRn = 0xF10;
constexpr uSize SPENDSGIRn = 0xF20;
}  // namespace dist

namespace cpu {
constexpr uSize CTLR       = 0x00;
constexpr u32 CTLR_DISABLE = 0;
constexpr u32 CTLR_ENABLE  = 1;

constexpr uSize PMR       = 0x04;
constexpr uSize BPR       = 0x08;
constexpr uSize IAR       = 0x0C;
constexpr u32 IAR_ID_MASK = 0x3FF;
constexpr uSize EOIR      = 0x10;
constexpr uSize RPR       = 0x14;
constexpr uSize HPPIR     = 0x18;
constexpr uSize ABPR      = 0x1C;
constexpr uSize AIAR      = 0x20;
constexpr uSize AEOIR     = 0x24;
constexpr uSize AHPPIR    = 0x28;

constexpr uSize APRn   = 0xD0;
constexpr uSize NSAPRn = 0xE0;

constexpr uSize IIDR = 0xFC;
constexpr uSize DIR  = 0x1000;
}  // namespace cpu

ArmGIC::ArmGIC(mem::DeviceArea distributor_area, mem::DeviceArea cpu_base, u8 num_cpus, u32 max_ids)
    : m_handlers(max_ids),
      m_num_cpus{num_cpus},
      m_num_ids{max_ids},
      m_distributor_base{distributor_area},
      m_cpu_base{cpu_base} {}

ArmGIC ArmGIC::create(mem::DeviceArea distributor_base, mem::DeviceArea cpu_base) {
    auto typer    = distributor_base.read<u32>(dist::TYPER);
    auto num_cpus = dist::TYPER_CPU_NUMBER(typer);
    auto num_ids  = dist::TYPER_LINES_NUMBER(typer);

    auto ret_val = ArmGIC(distributor_base, cpu_base, num_cpus, num_ids);
    ret_val.initialise();

    return ret_val;
}
bool ArmGIC::initialise() {
    m_distributor_base.write<u32>(dist::CTLR, dist::CTLR_DISABLE);

    // For every id (registers of 32-bits)
    for (u32 i = 0; i < bek::ceil_div(m_num_ids, 32u); i++) {
        // Set all to group 0.
        m_distributor_base.write<u32>(dist::IGROUPRn + i * 4, 0u);
        // Clear all enable, pending, and active bits.
        m_distributor_base.write<u32>(dist::ICENABLERn + i * 4, ~0u);
        m_distributor_base.write<u32>(dist::ICPENDRn + i * 4, ~0u);
        m_distributor_base.write<u32>(dist::ICACTIVERn + i * 4, ~0u);
    }

    // Now loop through priorities - byte per id
    for (u32 i = 0; i < bek::ceil_div(m_num_ids, 4u); i++) {
        m_distributor_base.write<u32>(dist::IPRIORITYRn + i * 4,
                                      (dist::IPRIORITY_DEFAULT << 24) |
                                          (dist::IPRIORITY_DEFAULT << 16) |
                                          (dist::IPRIORITY_DEFAULT << 8) | dist::IPRIORITY_DEFAULT);

        // If not cpu-specific, set target cpu to 0.
        if (i * 4 >= 32) {
            m_distributor_base.write<u32>(dist::ITARGETSRn + i * 4,
                                          (dist::ITARGETS_CPU0 << 24) |
                                              (dist::ITARGETS_CPU0 << 16) |
                                              (dist::ITARGETS_CPU0 << 8) | dist::ITARGETS_CPU0);
        }
    }

    // For every id (register of 16 2-bit elements)
    for (u32 i = 0; i < bek::ceil_div(m_num_ids, 16u); i++) {
        // Set all to level-sensitive (can be altered when registering)
        m_distributor_base.write<u32>(dist::ICFGRn + i * 4, 0);
    }

    // Enable Distributor
    m_distributor_base.write<u32>(dist::CTLR, dist::CTLR_ENABLE);

    // Set up this CPU
    m_cpu_base.write<u32>(cpu::PMR, dist::IPRIORITY_LOWEST);
    m_cpu_base.write<u32>(cpu::CTLR, cpu::CTLR_ENABLE);
    return true;
}
ArmGIC::Handle ArmGIC::register_interrupt(bek::buffer selection_data) {
    // We set the necessary params, but do not enable the interrupt.
    // Three u32 - is_ppi, int id, flags
    ASSERT(selection_data.size() == 3 * 4);

    bool is_spi      = selection_data.get_at_be<u32>(0) == 0;
    u32 interrupt_id = selection_data.get_at_be<u32>(4);
    if (is_spi) {
        // Device tree maps SPIs from 0.
        interrupt_id += 32;
    } else {
        // Device tree maps PPIs from 0
        interrupt_id += 16;
    }
    ASSERT(interrupt_id <= m_num_ids);

    u32 flags = selection_data.get_at_be<u32>(8);

    uSize config_register_offset = dist::ICFGRn + (interrupt_id / 16 * 4);
    auto bit_index               = 2 * (interrupt_id % 16) + 1;

    auto config_value = m_distributor_base.read<u32>(config_register_offset);
    if (flags & 0b11) {
        // Edge triggered
        config_value |= (1u << bit_index);
    } else {
        // Level Triggered
        config_value &= ~(1u << bit_index);
    }
    m_distributor_base.write<u32>(config_register_offset, config_value);

    return {this, interrupt_id};
}
void ArmGIC::register_handler(u32 interrupt_id, InterruptHandler&& handler) {
    ASSERT(interrupt_id < m_num_ids);
    m_handlers[interrupt_id] = bek::move(handler);
}
void ArmGIC::enable_interrupt(u32 interrupt_id) {
    m_distributor_base.write<u32>(dist::ISENABLERn + 4 * (interrupt_id / 32),
                                  1u << (interrupt_id % 32));
}
void ArmGIC::disable_interrupt(u32 interrupt_id) {
    m_distributor_base.write<u32>(dist::ICENABLERn + 4 * (interrupt_id / 32),
                                  1u << (interrupt_id % 32));
}
void ArmGIC::handle_interrupt() {
    u32 iar          = m_cpu_base.read<u32>(cpu::IAR);
    u32 interrupt_id = iar & cpu::IAR_ID_MASK;
    if (interrupt_id < m_num_ids && m_handlers[interrupt_id]) {
        VERIFY(m_handlers[interrupt_id].is_valid());
        enable_interrupts();
        (*m_handlers[interrupt_id])();
        disable_interrupts();
        m_cpu_base.write<u32>(cpu::EOIR, iar);
    } else {
        ASSERT(interrupt_id >= 1020);
    }
}

extern InterruptController* global_intc;
dev_tree::DevStatus ArmGIC::probe_devtree(dev_tree::Node& node, dev_tree::device_tree&, dev_tree::probe_ctx& ctx) {
    if (!(node.compatible.size() && (node.compatible[0] == "arm,gic-400"_sv ||
                                     node.compatible[0] == "arm,cortex-a15-gic"_sv)))
        return dev_tree::DevStatus::Unrecognised;
    auto reg = dev_tree::get_regions_from_reg(node);
    VERIFY(reg.size() == 2);
    mem::DeviceArea distributor_area = mem::MemoryManager::the().map_for_io(reg[0]);
    mem::DeviceArea cpu_area         = mem::MemoryManager::the().map_for_io(reg[1]);
    node.attached_device = bek::make_own<ArmGIC>(ArmGIC::create(distributor_area, cpu_area));
    return dev_tree::DevStatus::Success;
}
