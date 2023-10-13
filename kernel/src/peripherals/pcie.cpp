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

#include "peripherals/pcie.h"

#include "library/debug.h"
#include "mm/memory_manager.h"
#include "usb/xhci.h"

using DBG = DebugScope<"PCIe", true>;

namespace pcie {

enum CapabilityID : u8 {
    POWER = 0x1,
    MSI   = 0x5,
    MSIX  = 0x11,
};

struct CapabilityHeader {
    CapabilityID capability_id;
    u8 next_offset;
};

void bek_basic_format(bek::OutputStream& out, const Address& addr) {
    auto code     = addr.space_code();
    auto code_str = (code == AddressSpaceKind::Configuration) ? "cfg"_sv
                    : (code == AddressSpaceKind::IO)          ? "io"_sv
                    : (code == AddressSpaceKind::Mem32Bit)    ? "32bit"_sv
                                                              : "64bit"_sv;
    bek::format_to(out, "{:XL} ("_sv, addr.address);
    if (addr.relocatable()) {
        out.write("r,"_sv);
    } else {
        out.write("nr,"_sv);
    }
    if (addr.cacheable()) {
        out.write("c,"_sv);
    } else {
        out.write("nc,"_sv);
    }
    if (addr.aliased()) {
        out.write("a,"_sv);
    } else {
        out.write("na,"_sv);
    }
    bek::format_to(out, "{})"_sv, code_str);
}

bek::vector<IntMapping> get_interrupt_mapping(dev_tree::Node& node,
                                              dev_tree::Node& int_parent_node) {
    bek::vector<IntMapping> mapping{};
    auto o_mapping_buffer = node.get_property("interrupt-map"_sv);
    if (!o_mapping_buffer) return mapping;
    auto mapping_buffer = *o_mapping_buffer;

    const u32 int_address_cells =
        dev_tree::get_property_u32(int_parent_node, "#address-cells"_sv).value_or(0);
    const u32 int_int_cells = *dev_tree::get_property_u32(int_parent_node, "#interrupt-cells"_sv);

    // PCIe Address (3); INT PIN (1); PHANDLE (1); INT ADDRESS (?); INT CELLS (?)
    ASSERT(mapping_buffer.size() % 4 * (3 + 1 + 1 + int_address_cells + int_int_cells) == 0);

    uSize offset = 0;
    while (offset < mapping_buffer.size()) {
        u32 address_flags = mapping_buffer.get_at_be<u32>(offset);
        // Skip useless parts of address.
        offset += 3 * 4;
        u32 int_pin = mapping_buffer.get_at_be<u32>(offset);
        offset += 4;
        u32 int_controller_phandle = mapping_buffer.get_at_be<u32>(offset);
        ASSERT(int_controller_phandle == int_parent_node.phandle);
        offset += 4;
        // Ignore...?
        offset += int_address_cells * 4;
        bek::buffer int_data = mapping_buffer.subdivide(offset, int_int_cells * 4);
        offset += int_int_cells * 4;
        mapping.push_back(IntMapping{{static_cast<u8>((address_flags >> 16) & 0xFF),
                                      static_cast<u8>((address_flags >> 11) & 0x1F),
                                      static_cast<u8>((address_flags >> 8) & 0x7)},
                                     static_cast<Function::InterruptPin>(int_pin),
                                     int_data});
    }
    return mapping;
}

dev_tree::DevStatus Controller::probe_pcie_host(dev_tree::Node& node, dev_tree::device_tree& tree) {
    if (!(node.compatible.size() && node.compatible[0] == "pci-host-ecam-generic"_sv))
        return dev_tree::DevStatus::Unrecognised;
    DBG::dbgln("Probing Device {}"_sv, node.name);

    ASSERT(*dev_tree::get_property_u32(node, "#address-cells"_sv) == 3);
    auto parent_address_cells =
        dev_tree::get_property_u32(*node.parent, "#address-cells"_sv).value_or(2);
    auto size_cells = dev_tree::get_property_u32(node, "#size-cells"_sv).value_or(1);

    // Look for interrupt parent.
    auto int_parent_phandle = dev_tree::get_inheritable_property_u32(node, "interrupt-parent"_sv);
    if (!int_parent_phandle) {
        DBG::dbgln("Could not get interrupt-parent phandle."_sv);
        return dev_tree::DevStatus::Failure;
    }
    auto [p_int_parent, result_status] = dev_tree::get_node_by_phandle(tree, *int_parent_phandle);
    if (!p_int_parent) {
        return result_status;
    }

    auto* interrupt_parent = Device::as<InterruptController>(p_int_parent->attached_device.get());
    ASSERT(interrupt_parent);

    // Now, to parse interrupt mapping.
    auto mapping = get_interrupt_mapping(node, *p_int_parent);

    // This should be the configuration space.
    // TODO: Address Translation
    auto regs = dev_tree::get_regions_from_reg(node);
    ASSERT(regs.size());
    mem::PCIeDeviceArea config_space{mem::MemoryManager::the().map_for_io(regs[0])};
    DBG::dbgln("Configuration Space: 0X{:X} ({}MiB)"_sv, (uPtr)config_space.ptr(),
               config_space.size() >> 20);

    // Read ranges
    auto buf = *node.get_property("ranges"_sv);

    auto n = buf.size() / (4 * (parent_address_cells + 3 + size_cells));
    ASSERT(n);
    u32 offset = 0;
    bek::vector<Controller::AllocatedRange> ranges;
    while (n--) {
        Address addr{buf.get_at_be<u64>(offset + 4), buf.get_at_be<u32>(offset)};
        offset += 12;
        auto parent_addr = dev_tree::read_from_buffer(buf, offset, parent_address_cells);
        auto size        = dev_tree::read_from_buffer(buf, offset, size_cells);
        auto region      = dev_tree::map_region_to_root(node, {{parent_addr}, size});
        if (!region) {
            DBG::dbgln(
                "Mapping Ranges: Could not get physical region for range {:Xl} (size {:Xl})"_sv,
                parent_addr, size);
            return dev_tree::DevStatus::Failure;
        }
        // Now, we need to map the region as IO space.
        auto mapped = mem::MemoryManager::the().map_for_io(*region);

        ranges.push_back(AllocatedRange{addr, region->start, size, mapped.ptr(), 0});
    }

    DBG::dbgln("Ranges: "_sv);
    for (auto& range : ranges) {
        DBG::dbgln("   {}"_sv, range);
    }

    auto dma_mapping = dev_tree::get_dma_to_phys_ranges(node);

    node.attached_device =
        bek::own_ptr{new Controller(config_space, bek::move(mapping), *interrupt_parent,
                                    bek::move(dma_mapping), bek::move(ranges))};
    // TODO: Make dynamic and global-ly
    auto* p_controller = static_cast<Controller*>(node.attached_device.get());
    p_controller->probe(probe_xhci);

    // Not sure what to do with the ranges tho.
    return dev_tree::DevStatus::Success;
}
Function::Function(mem::PCIeDeviceArea configuration_space, FunctionAddress address,
                   Controller& controller)
    : configuration_space{configuration_space}, m_address(address), msix_info{}, host(&controller) {
    // Byte except bit 7 (multi-function)
    m_header_type = static_cast<CfgHeaderType>((configuration_space.read<u32>(0xC) >> 16) & 0x7F);
    auto class_code_reg = configuration_space.read<u32>(0x8);
    m_class_code        = {static_cast<u8>((class_code_reg >> 24) & 0xFF),
                           static_cast<u8>((class_code_reg >> 16) & 0xFF),
                           static_cast<u8>((class_code_reg >> 8) & 0xFF)};
    if (m_header_type == CfgHeaderType::General) {
        u8 cap_offset = configuration_space.read<u32>(0x34) & 0xFF;
        while (cap_offset != 0) {
            CapabilityHeader cap_header = {
                static_cast<CapabilityID>(configuration_space.read<u32>(cap_offset) & 0xFF),
                static_cast<u8>((configuration_space.read<u32>(cap_offset) >> 8) & 0xFF)};

            if (cap_header.capability_id == POWER) {
                power_capability_offset = cap_offset;
            } else if (cap_header.capability_id == MSIX) {
                u32 table_register = configuration_space.read<u32>(cap_offset + 4);
                u32 pba_register   = configuration_space.read<u32>(cap_offset + 8);

                auto table_bar = bar(table_register & 0b111);
                uPtr table_ptr = table_bar.address + (table_register & ~(0b111));

                auto pba_bar = bar(pba_register & 0b111);
                uPtr pba_ptr = pba_bar.address + (pba_register & ~(0b111));

                u16 table_size = (configuration_space.read<u32>(cap_offset) >> 16) & 0x7FF;
                msix_info      = MSIXInfo{cap_offset, table_ptr, pba_ptr, table_size};
            }
            cap_offset = cap_header.next_offset;
        }
    }
}
bool Function::supports_msix() const { return msix_info.is_valid(); }
bool Function::supports_power_management() const { return power_capability_offset.is_valid(); }
bool Function::power_reset() const { return false; }
InterruptType Function::interrupt_type() const { return InterruptType::Pin; }
Function::InterruptPin Function::interrupt_pin() const {
    return static_cast<InterruptPin>((configuration_space.read<u32>(0x3C) >> 8) & 0b111);
}
void Function::set_pin_interrupt_handler(InterruptHandler&& handler) {
    host->register_pin_interrupt_handler(*this, bek::move(handler));
}
void Function::enable_pin_interrupts() {
    u32 config_status = configuration_space.read<u32>(0x4);
    // Status register is RW1C.
    config_status &= 0xFFFF;
    // Clear int disable
    config_status &= ~(1u << 10);
    configuration_space.write<u32>(0x4, config_status);
}
void Function::disable_pin_interrupts() {
    u32 config_status = configuration_space.read<u32>(0x4);
    // Status register is RW1C.
    config_status &= 0xFFFF;
    // Set int disable
    config_status |= (1u << 10);
    configuration_space.write<u32>(0x4, config_status);
}
bool Function::is_pin_interrupted() const {
    auto status = configuration_space.read<u32>(0x4) >> 16;
    return status & (1u << 3);
}
bek::own_ptr<Function> Function::try_create_function(const mem::PCIeDeviceArea& controller_base,
                                                     u8 bus, u8 device, u8 function,
                                                     Controller& controller) {
    uSize offset = ((static_cast<uSize>(bus) * 256) + (static_cast<uSize>(device) * 8) +
                    static_cast<uSize>(function)) *
                   4096;
    u16 vendor = controller_base.read<u32>(offset) & 0xFFFF;
    if (vendor == 0xFFFF) {
        return nullptr;
    } else {
        return bek::make_own<Function>(controller_base.subdivide(offset, 4096),
                                       FunctionAddress{bus, device, function}, controller);
    }
}

constexpr AddressSpaceKind get_bar_kind(u64 bar_value) {
    if (bar_value & 0b1) {
        return AddressSpaceKind::IO;
    }
    auto memory_kind = (bar_value >> 1) & 0b11;
    switch (memory_kind) {
        case 0:
            return AddressSpaceKind::Mem32Bit;
        case 1:
            ASSERT(false);
        case 2:
            return AddressSpaceKind::Mem64Bit;
        default:
            ASSERT(false);
    }
}

BaseAddressRegister Function::bar(u8 n) {
    auto bar_offset = 0x10 + 4 * n;

    u64 bar_val = configuration_space.read<u32>(bar_offset);
    auto kind   = get_bar_kind(bar_val);

    u64 address_mask = (kind == AddressSpaceKind::IO) ? 0xFFFFFFFFFFFFFFFC : 0xFFFFFFFFFFFFFFF0;
    // Don't bother with checking prefetchable.
    if (kind == AddressSpaceKind::Mem64Bit) {
        VERIFY(n < 5);
        u64 next_bar_val = configuration_space.read<u32>(bar_offset + 4);
        bar_val |= next_bar_val << 32;
    }

    // Now get size.
    u32 original_bar_val = configuration_space.read<u32>(bar_offset);
    configuration_space.write<u32>(bar_offset, 0xFFFFFFFF);
    u32 size_bar_val = configuration_space.read<u32>(bar_offset);
    configuration_space.write<u32>(bar_offset, original_bar_val);
    u32 size = ~(size_bar_val & static_cast<u32>(address_mask)) + 1;

    u64 address = bar_val & address_mask;

    return {address, size, kind, true};
}
void Function::write_bar(u8 n, AddressSpaceKind kind, u64 address) {
    if (kind == AddressSpaceKind::IO || kind == AddressSpaceKind::Mem32Bit) {
        VERIFY((address >> 32) == 0);
    } else {
        configuration_space.write<u32>(0x10 + 4 * n + 4, address >> 32);
    }

    u32 address_mask = (kind == AddressSpaceKind::IO) ? 0xFFFFFFFC : 0xFFFFFFF0;
    configuration_space.write<u32>(0x10 + 4 * n, address & address_mask);
}
void Function::enable_memory_accesses() {
    auto pattern = configuration_space.read<u32>(0x4) & 0xFFFF;
    pattern |= 11u << 1;
    configuration_space.write<u32>(0x4, pattern);
}
mem::dma_pool& Function::dma_pool() const { return host->dma_pool(); }
mem::PCIeDeviceArea Function::initialise_bar(u8 bar_n) {
    return host->initialise_bar(*this, bar_n);
}
void Controller::register_pin_interrupt_handler(const Function& function,
                                                InterruptHandler&& handler) {
    FunctionAddress addr = function.address();
    auto pin_no          = function.interrupt_pin();
    IntMapping* match    = nullptr;
    for (auto& mapping : m_interrupt_mappings) {
        if (mapping.function_address == addr && pin_no == mapping.pin_number) {
            match = &mapping;
            break;
        }
    }
    VERIFY(match);
    auto handle = m_parent_controller->register_interrupt(match->controller_data);

    auto* handlers = m_handlers.find(handle.interrupt_id);
    if (!handlers) {
        m_handlers.insert({handle.interrupt_id, {}});
        handlers = m_handlers.find(handle.interrupt_id);

        // Need to set up handler.
        handle.register_handler([this, id = handle.interrupt_id] { interrupt_handler(id); });
    }
    handlers->push_back(IntHandler{&function, bek::move(handler)});
    m_parent_controller->enable_interrupt(handle.interrupt_id);
}
void Controller::interrupt_handler(u32 int_id) {
    auto& handlers = m_handlers.find_unchecked(int_id);
    for (auto& handler : handlers) {
        handler.handler();
    }
}
Device::Kind Controller::kind() const { return Device::Kind::PCIeHost; }

Controller::Controller(mem::PCIeDeviceArea configuration_space,
                       bek::vector<IntMapping> interrupt_mappings,
                       InterruptController& parent_controller,
                       bek::vector<dev_tree::range_t> dma_mappings,
                       bek::vector<AllocatedRange> address_spaces)
    : m_cfg_space(configuration_space),
      m_interrupt_mappings(bek::move(interrupt_mappings)),
      m_dma_pool(bek::move(dma_mappings)),
      m_parent_controller{&parent_controller},
      m_address_spaces(bek::move(address_spaces)) {
    for (u32 bus = 0; bus < 256; bus++) {
        for (u8 device = 0; device < 32; device++) {
            // Probe Device
            auto function = Function::try_create_function(m_cfg_space, bus, device, 0, *this);
            if (!function) continue;

            bool multi_function = function->multifunction();
            auto info           = function->class_code();
            DBG::dbgln("Detected {} device: class code {:x}, subclass {:x}, interface {:x}"_sv,
                       multi_function ? "multi-function"_sv : "single-function"_sv, info.class_code,
                       info.subclass, info.prog_if);
            m_functions.push_back(bek::move(function));
            if (multi_function) {
                for (u8 func = 1; func < 8; func++) {
                    function = Function::try_create_function(m_cfg_space, bus, device, func, *this);
                    if (function) {
                        info = function->class_code();
                        DBG::dbgln("    {}: Class code {:x}, subclass {:x}, interface {:x}"_sv,
                                   func, info.class_code, info.subclass, info.prog_if);
                        m_functions.push_back(bek::move(function));
                    }
                }
            }
        }
    }
}
void Controller::probe(ProbeFn* probe_function) {
    for (auto& func : m_functions) {
        probe_function(*func);
    }
}
mem::PCIeDeviceArea Controller::initialise_bar(Function& function, u8 bar_n) {
    auto bar = function.bar(bar_n);
    VERIFY(bar.size > 0);
    if (bar.address) {
        // TODO: Already initialised?
        PANIC("TODO");
    }
    if (!bar.address) {
        // Need to allocate space.
        for (auto& range : m_address_spaces) {
            if (range.pcie_address.space_code() == bar.kind) {
                auto offset = bek::align_up(range.usage, bar.size);
                if (offset + bar.size <= range.size) {
                    function.write_bar(bar_n, bar.kind, range.pcie_address.address + offset);
                    range.usage = offset + bar.size;
                    return {mem::DeviceArea{range.global_address.get(),
                                            (uPtr)range.mapped_ptr + offset, bar.size}};
                }
            }
        }
    }
    // FIXME: More graceful failure.
    PANIC("Could not allocate bar.");
}
void bek_basic_format(bek::OutputStream& out, const Controller::AllocatedRange& rng) {
    bek::format_to(out, "{} ({}), at {:Xl}"_sv, rng.pcie_address, rng.size, (uPtr)rng.mapped_ptr);
}

}  // namespace pcie