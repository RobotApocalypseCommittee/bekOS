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

#ifndef BEKOS_PCIE_H
#define BEKOS_PCIE_H

#include "device_tree.h"
#include "interrupt_controller.h"
#include "mm/dma_utils.h"

namespace pcie {

struct FunctionAddress {
    u8 bus;
    u8 device;
    u8 function;

    friend bool operator==(FunctionAddress, FunctionAddress) = default;
};

struct ClassCode {
    u8 class_code;
    u8 subclass;
    u8 prog_if;
    friend bool operator==(ClassCode, ClassCode) = default;
};

enum class CfgHeaderType { General = 0x0, PciBridge = 0x1, CardbusBridge = 0x2 };

enum class AddressSpaceKind { Configuration, IO, Mem32Bit, Mem64Bit };

struct BaseAddressRegister {
    u64 address;
    u64 size;
    AddressSpaceKind kind;
    bool prefetchable;
};

struct MSIXInfo {
    uPtr capability;
    uPtr table;
    uPtr pending_bit_array;
    uSize table_length;
};

enum class InterruptType {
    Pin,
    MSI,
    MSI_X,
};

struct Address {
    u64 address;
    u32 flags;

    constexpr bool relocatable() const { return (flags & (1u << 31)) == 0; }

    constexpr bool cacheable() const { return (flags & (1u << 30)) != 0; }

    constexpr bool aliased() const { return (flags & (1u << 29)) != 0; }

    constexpr AddressSpaceKind space_code() const {
        switch ((flags >> 24) & 0b11) {
            case 0:
                return AddressSpaceKind::Configuration;
            case 1:
                return AddressSpaceKind::IO;
            case 2:
                return AddressSpaceKind::Mem32Bit;
            case 3:
                return AddressSpaceKind::Mem64Bit;
        }
        __unreachable();
    }
};

void bek_basic_format(bek::OutputStream& out, const Address& addr);



class Controller;

class Function {
public:
    enum InterruptPin { IntA = 1, IntB = 2, IntC = 3, IntD = 4 };

    static bek::own_ptr<Function> try_create_function(const mem::PCIeDeviceArea& controller_base,
                                                      u8 bus, u8 device, u8 function,
                                                      Controller& controller);
    explicit Function(mem::PCIeDeviceArea configuration_space, FunctionAddress address,
                      Controller& controller);
    FunctionAddress address() const { return m_address; }
    ClassCode class_code() const { return m_class_code; }
    bool multifunction() const {
        // Check bit 7 of header type register.
        return configuration_space.read<u32>(0xC) & (1u << (7 + 16));
    }

    mem::dma_pool& dma_pool() const;

    BaseAddressRegister bar(u8 n);
    void write_bar(u8 n, AddressSpaceKind, u64 address);
    mem::PCIeDeviceArea initialise_bar(u8 bar_n);
    void enable_memory_accesses();

    bool supports_msix() const;
    bool supports_power_management() const;
    bool power_reset() const;

    InterruptType interrupt_type() const;
    InterruptPin interrupt_pin() const;
    void set_pin_interrupt_handler(InterruptHandler&& handler);
    void enable_pin_interrupts();
    void disable_pin_interrupts();
    bool is_pin_interrupted() const;

private:
    mem::PCIeDeviceArea configuration_space;
    FunctionAddress m_address;
    bek::optional<MSIXInfo> msix_info;
    bek::optional<uSize> power_capability_offset;
    ClassCode m_class_code{};
    CfgHeaderType m_header_type;
    Controller* host;
};

using ProbeFn = bool(Function&);

struct IntMapping {
    FunctionAddress function_address;
    Function::InterruptPin pin_number;
    bek::buffer controller_data;
};

class Controller final : public Device {
    using DeviceType                 = Controller;
    static constexpr Kind DeviceKind = Device::Kind::PCIeHost;

public:
    static dev_tree::DevStatus probe_pcie_host(dev_tree::Node& node, dev_tree::device_tree& tree);

    /// Runs probe function for each function on the PCIe Bus.
    void probe(ProbeFn* probe_function);

    /// Registers and enables an interrupt handler for function. handler must allow for no work to
    /// be done (i.e. the interrupt having been intended for another device.
    void register_pin_interrupt_handler(const Function& function, InterruptHandler&& handler);

    mem::MappedDmaPool& dma_pool() { return m_dma_pool; }

    mem::PCIeDeviceArea initialise_bar(Function& function, u8 bar_n);

private:
    struct IntHandler {
        const Function* function;
        InterruptHandler handler;
    };

    struct AllocatedRange {
        Address pcie_address;
        mem::PhysicalPtr global_address;
        u64 size;
        void* mapped_ptr;
        uSize usage;
    };

    friend void bek_basic_format(bek::OutputStream&, const Controller::AllocatedRange&);

    Controller(mem::PCIeDeviceArea configuration_space, bek::vector<IntMapping> interrupt_mappings,
               InterruptController& parent_controller, bek::vector<dev_tree::range_t> dma_mappings,
               bek::vector<AllocatedRange> address_spaces);

    void interrupt_handler(u32 int_id);
    Kind kind() const override;

    mem::PCIeDeviceArea m_cfg_space;
    bek::hashtable<u32, bek::vector<IntHandler>> m_handlers{};
    bek::vector<bek::own_ptr<Function>> m_functions;
    bek::vector<IntMapping> m_interrupt_mappings;
    mem::MappedDmaPool m_dma_pool;
    InterruptController* m_parent_controller;
    bek::vector<AllocatedRange> m_address_spaces;
};

}  // namespace pcie

#endif  // BEKOS_PCIE_H
