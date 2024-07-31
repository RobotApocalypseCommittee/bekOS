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

#ifndef BEKOS_XHCI_H
#define BEKOS_XHCI_H

#include "bek/types.h"
#include "mm/dma_utils.h"
#include "peripherals/pcie.h"
#include "xhci_registers.h"
#include "xhci_ring.h"

namespace xhci {

struct Interrupter;
struct SupportedProtocol;

struct Device;

class Controller {
public:
    Controller(mem::PCIeDeviceArea deviceArea, CapabilityRegisters t_capability_registers,
               pcie::Function* function);
    bool initialise();

private:
    struct Port;
    void handle_interrupt();

    pcie::Function* function;
    mem::PCIeDeviceArea xhci_space;
    CapabilityRegisters capability_registers;
    OperationalRegisters operational_registers;
    RuntimeRegisters runtime_registers;
    DoorbellRegisters doorbell_registers;
    bek::own_ptr<Interrupter> m_primary_interrupter;

    mem::dma_array<u64> m_dbcaa;
    mem::dma_array<u64> m_scratchpad_ptr_array;
    xhci::ProducerRing m_command_ring;
    bek::vector<SupportedProtocol> m_protocols;
    bek::vector<Port> m_ports;
    bek::vector<Device*> m_devices;

    u8 max_device_slots;
    friend struct xhci::Device;
};

}  // namespace xhci
bool probe_xhci(pcie::Function& function);

#endif  // BEKOS_XHCI_H
