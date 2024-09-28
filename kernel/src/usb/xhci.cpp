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

#include "usb/xhci.h"

#include "library/debug.h"
#include "usb/descriptors.h"
#include "usb/xhci_context.h"
#include "usb/xhci_registers.h"

using namespace xhci;
using DBG = DebugScope<"xHCI", DebugLevel::WARN>;

enum class DeviceSpeed { Low, High, Full, Super };

struct ExtendedCapabilities {
    enum CapabilityID {
        Reserved,
        USBLegacySupport,
        SupportedProtocol,

    };
    struct Capability {
        const ExtendedCapabilities* caps;
        u32 byte_offset;
        CapabilityID id;

        Capability& operator++() {
            u32 top        = caps->area.read<u32>(byte_offset);
            u32 new_offset = ((top >> 8) & 0xFF) * 4 + byte_offset;
            if (new_offset == byte_offset) {
                byte_offset = -1;
            } else {
                byte_offset = new_offset;
                id          = static_cast<CapabilityID>(caps->area.read<u32>(new_offset) & 0xFF);
            }
            return *this;
        }

        bool operator==(const Capability& b) const {
            return (caps == b.caps) && (byte_offset == b.byte_offset);
        }

        const Capability& operator*() const { return *this; }
    };

    [[nodiscard]] Capability end() const {
        return {this, static_cast<u32>(-1), static_cast<CapabilityID>(0)};
    };
    [[nodiscard]] Capability begin() const {
        return {this, 0, static_cast<CapabilityID>(area.read<u32>(0) & 0xFF)};
    };

    mem::PCIeDeviceArea area;
};

struct xhci::SupportedProtocol {
    u8 major;
    u8 minor;
    char name[5];
    u8 compatible_port_offset;
    u8 compatible_port_count;
    u16 protocol_defined_data;
    u8 slot_type;
    struct SpecifiedSpeed {
        u16 mantissa;
        u8 value;
        u8 exponent;
        u8 type;
        bool full_duplex;
        bool superspeed_plus;
    };
    bek::vector<SpecifiedSpeed> speeds;
    static SupportedProtocol from_capability(const ExtendedCapabilities::Capability cap) {
        ASSERT(cap.id == ExtendedCapabilities::SupportedProtocol);
        u32 cap_header   = cap.caps->area.read<u32>(cap.byte_offset);
        u32 cap_name_int = cap.caps->area.read<u32>(cap.byte_offset + 4);
        u32 cap_data     = cap.caps->area.read<u32>(cap.byte_offset + 8);
        u8 slot_type     = cap.caps->area.read<u32>(cap.byte_offset + 12) & 0xF;
        u8 speed_count   = (cap_data >> 28) & 0xF;
        SupportedProtocol protocol{
            static_cast<u8>(cap_header >> 24 & 0xFF),
            static_cast<u8>(cap_header >> 16 & 0xFF),
            {static_cast<char>(cap_name_int & 0xFF), static_cast<char>((cap_name_int >> 8) & 0xFF),
             static_cast<char>((cap_name_int >> 16) & 0xFF),
             static_cast<char>((cap_name_int >> 24) & 0xFF), '\0'},
            static_cast<u8>(cap_data & 0xFF),
            static_cast<u8>((cap_data >> 8) & 0xFF),
            static_cast<u16>((cap_data >> 16) & 0xFFF),
            slot_type,
            bek::vector<SpecifiedSpeed>{speed_count}};

        for (int i = 0; i < speed_count; i++) {
            u32 psi            = cap.caps->area.read<u32>(cap.byte_offset + 16 + (i * 4));
            protocol.speeds[i] = {
                static_cast<u16>((psi >> 16) & 0xFFFF), static_cast<u8>(psi & 0xF),
                static_cast<u8>((psi >> 4) & 0b11),     static_cast<u8>((psi >> 6) & 0b11),
                static_cast<bool>((psi >> 8) & 1),      static_cast<bool>((psi >> 14) & 1)};
        }
        return protocol;
    }
};

constexpr uSize endpoint_index(u8 endpoint_number, usb::TransferType ttype, usb::Direction dir) {
    // Control 0 -> 0
    VERIFY(endpoint_number > 0 || ttype == usb::TransferType::Control);
    if (ttype == usb::TransferType::Control) {
        return 2 * endpoint_number;
    } else {
        return 2 * endpoint_number - (dir == usb::Direction::Out ? 1 : 0);
    }
}

struct Controller::Port {
    SupportedProtocol* protocol;
    u8 id;
    PortOperationalRegisters registers;
    bek::own_ptr<Device> attached_device;
    [[nodiscard]] DeviceSpeed get_speed() const {
        if (protocol->speeds.size() == 0) {
            // Means we get ports from registers.
            // xHCI Spec §7.2.2.1.1
            switch (registers.get_port_speed()) {
                case 1:
                    return DeviceSpeed::Full;
                case 2:
                    return DeviceSpeed::Low;
                case 3:
                    return DeviceSpeed::High;
                case 4:
                case 5:
                case 6:
                case 7:
                    return DeviceSpeed::Super;
                default:
                    PANIC("Unrecognised Speed");
            }
        } else {
            // TODO
            PANIC("TODO");
        }
    }
};

struct xhci::Device : usb::Device {
    enum class State {
        WaitingForSlot,
    };

    void on_transfer_event(EventTRB trb) {
        VERIFY(trb.endpoint_id >= 1 && trb.endpoint_id <= 31);
        VERIFY(m_transfer_rings[trb.endpoint_id - 1]);
        m_transfer_rings[trb.endpoint_id - 1]->process_completion(trb);
    }

    static void create(Controller& controller, u8 root_port_id) {
        auto& port = controller.m_ports[root_port_id - 1];
        VERIFY(!port.attached_device);
        port.attached_device = bek::make_own<Device>(controller, 0, State::WaitingForSlot,
                                                     root_port_id, port.get_speed());

        xhci::TRB cmd{0, 0, 0, 0};
        cmd.trb_type(xhci::TRBType::EnableSlot);
        controller.m_command_ring.push_command(
            cmd, [p = port.attached_device.get()](xhci::EventTRB trb) { p->on_slot_enable(trb); });
        controller.doorbell_registers.ring_doorbell(0, 0, 0);
    }
    Device(Controller& controller, u8 slot_id, State initial_state, u8 root_hub_port,
           DeviceSpeed speed)
        : m_controller(controller),
          m_slot_id(slot_id),
          m_ctx_array(controller.function->dma_pool(), device_context_size,
                      controller.capability_registers.context_64bit()),
          m_state(initial_state),
          m_root_hub_port(root_hub_port),
          m_speed(speed) {}

    bool schedule_transfer(usb::TransferRequest request) override {
        auto endpoint_idx =
            endpoint_index(request.endpoint_number, request.type, request.direction);
        auto& ctx = m_ctx_array.get_as<EndpointContext>(ContextArray::ep_dci(endpoint_idx));
        if (ctx.get_endpoint_type() != ep_type_from(request.type, request.direction)) return false;
        if (ctx.get_endpoint_state() != 1) return false;
        VERIFY(m_transfer_rings[endpoint_idx]);
        switch (request.type) {
            case usb::TransferType::Control: {
                if (!request.control_setup) return false;
                mem::dma_buffer view =
                    request.buffer ? request.buffer->view() : mem::dma_buffer::null_buffer();
                if (request.buffer) view = request.buffer->view();
                m_transfer_rings[endpoint_idx]->push_control_transfer(
                    *request.control_setup, view,
                    ProducerRing::Callback([cb = bek::move(request.callback),
                                            buf =
                                                bek::move(request.buffer)](EventTRB event) mutable {
                        usb::TransferRequest::Result res =
                            (event.completion_code == 1) ? usb::TransferRequest::Result::Success
                                                         : usb::TransferRequest::Result::Failure;
                        cb(bek::move(buf), res);
                    }));
                m_controller.doorbell_registers.ring_doorbell(m_slot_id, endpoint_idx + 1, 0);
                return true;
            }
            case usb::TransferType::Interrupt: {
                auto view =
                    request.buffer ? request.buffer->view() : mem::dma_buffer::null_buffer();
                DBG::dbgln("Making interrupt transfer on EP (idx) {}, length {}"_sv, endpoint_idx,
                           view.size());
                m_transfer_rings[endpoint_idx]->push_command(
                    TRB::create(TRBType::Normal, view.dma_ptr().get(), view.size(), (1u << 5)),
                    ProducerRing::Callback([cb = bek::move(request.callback),
                                            buf =
                                                bek::move(request.buffer)](EventTRB event) mutable {
                        usb::TransferRequest::Result res =
                            (event.completion_code == 1) ? usb::TransferRequest::Result::Success
                                                         : usb::TransferRequest::Result::Failure;
                        cb(bek::move(buf), res);
                    }));
                m_controller.doorbell_registers.ring_doorbell(m_slot_id, endpoint_idx + 1, 0);
                return true;
            }
            default:
                // TODO: Other transfer types.
                PANIC("TODO: Other transfers.");
        }
    }
    void enable_configuration(u8 configuration_n, const bek::vector<usb::Endpoint>& endpoints,
                              bek::function<void(bool)> cb) override {
        u8 max_ctx_entries = 0;
        for (auto& ep : endpoints) {
            max_ctx_entries = bek::max(max_ctx_entries, (u8)ContextArray::ep_ici(endpoint_index(
                                                            ep.number, ep.ttype, ep.direction)));
        }

        // First, we have to issue the command to setup endpoints.
        bool large_contexts  = m_controller.capability_registers.context_64bit();
        auto p_input_context = bek::make_own<ContextArray>(m_controller.function->dma_pool(),
                                                           input_context_size, large_contexts);
        auto& input_context  = *p_input_context;
        auto& input_ctrl     = input_context.get_as<InputControlContext>(ContextArray::control_ici);
        input_ctrl.set_add_flag(0, true);
        // n = 1 added by function called later.

        auto& input_slot_ctx = input_context.get_as<SlotContext>(ContextArray::slot_ici);
        input_slot_ctx       = m_ctx_array.get_as<SlotContext>(ContextArray::slot_dci);
        input_slot_ctx.set_context_entries(max_ctx_entries);

        for (auto& ep : endpoints) {
            setup_endpoint_data(input_context, ep);
        }

        input_context.sync_after_write();

        m_controller.m_command_ring.push_command(
            xhci::command::configure_endpoint(input_context.dma_ptr(), m_slot_id),
            xhci::ProducerRing::Callback([inp_ctx = bek::move(p_input_context), this,
                                          cb      = bek::move(cb),
                                          configuration_n](xhci::EventTRB trb) mutable {
                if (trb.completion_code == 1) {
                    DBG::dbgln("Successfully enabled endpoints."_sv);
                    schedule_transfer(
                        {usb::TransferType::Control,
                         // Doesn't matter
                         usb::Direction::Out,
                         0,
                         usb::TransferRequest::Callback(
                             [cb = bek::move(cb)](bek::optional<mem::own_dma_buffer>,
                                                  usb::TransferRequest::Result r) mutable {
                                 cb(r == usb::TransferRequest::Result::Success);
                             }),
                         {},
                         usb::SetupPacket{
                             usb::SetupPacket::make_req_type(usb::Direction::Out,
                                                             usb::ControlTransferType::Standard,
                                                             usb::ControlTransferTarget::Device),
                             0x09, configuration_n, 0, 0}});
                } else {
                    DBG::dbgln("Could not enable endpoints, code {}"_sv, trb.completion_code);
                    cb(false);
                }
            }));
        m_controller.doorbell_registers.ring_doorbell(0, 0, 0);
    }
    mem::own_dma_buffer allocate_buffer(uSize size) override {
        return m_controller.function->dma_pool().allocate(size, 1);
    }

private:
    void on_slot_enable(xhci::EventTRB trb) {
        VERIFY(trb.completion_code == 1);
        m_slot_id = trb.slot_id;
        DBG::dbgln("Device received slot {}."_sv, m_slot_id);

        VERIFY(m_controller.m_devices.size() == static_cast<uSize>(m_slot_id) - 1);
        m_controller.m_devices.push_back(this);

        bool large_contexts  = m_controller.capability_registers.context_64bit();
        auto p_input_context = bek::make_own<ContextArray>(m_controller.function->dma_pool(),
                                                           input_context_size, large_contexts);
        auto& input_context  = *p_input_context;
        auto& input_ctrl     = input_context.get_as<InputControlContext>(ContextArray::control_ici);
        input_ctrl.set_add_flag(0, true);
        // n = 1 added by function called later.

        auto& input_slot_ctx = input_context.get_as<SlotContext>(ContextArray::slot_ici);
        input_slot_ctx.set_root_hub_port(m_root_hub_port);
        // TODO: set route
        input_slot_ctx.set_route_string(0);
        // TODO: Does this mean I only need to allocate 3 units of memory above?
        input_slot_ctx.set_context_entries(1);

        u16 max_packet_size = 8;
        switch (m_speed) {
            case DeviceSpeed::Low:
                max_packet_size = 8;
                break;
            case DeviceSpeed::High:
            case DeviceSpeed::Full:
                max_packet_size = 64;
                break;
            case DeviceSpeed::Super:
                max_packet_size = 512;
                break;
        }
        DBG::dbgln("Max packet size: {}"_sv, max_packet_size);

        setup_endpoint_data(input_context, {0, usb::Direction::In, usb::TransferType::Control,
                                            max_packet_size, 0, false});

        input_context.sync_after_write();
        m_controller.m_dbcaa[m_slot_id] = m_ctx_array.dma_ptr();
        m_controller.m_dbcaa.sync_after_write(m_slot_id);

        m_controller.m_command_ring.push_command(
            xhci::TRB::create_address_dev_cmd(input_context.dma_ptr(), m_slot_id),
            xhci::ProducerRing::Callback(
                [inp_ctx = bek::move(p_input_context), this](xhci::EventTRB trb) {
                    on_address_request(trb);
                }));
        m_controller.doorbell_registers.ring_doorbell(0, 0, 0);
    }

    void on_address_request(xhci::EventTRB trb) {
        VERIFY(trb.completion_code == 1);
        // TODO: No Hardcode
        auto& slot_ctx = m_ctx_array.get_as<SlotContext>(0);
        auto address   = slot_ctx.get_device_address();
        DBG::dbgln("Device successfully got address {}."_sv, address);

        // FIXME: Do the dodgy 8-byte boogie.
        VERIFY(m_speed != DeviceSpeed::Full);
        usb::Registrar::the().register_device(*this);
    }

    void setup_endpoint_data(ContextArray& input_ctx, const usb::Endpoint& ep) {
        // Control 0 -> 0, Out n -> 2n - 1, In n -> 2n
        uSize ep_idx = endpoint_index(ep.number, ep.ttype, ep.direction);
        input_ctx.get_as<InputControlContext>(0).set_add_flag(ContextArray::ep_dci(ep_idx), true);

        m_transfer_rings[ep_idx] =
            bek::make_own<xhci::ProducerRing>(m_controller.function->dma_pool());
        auto ring_dma_ptr = m_transfer_rings[ep_idx]->dma_ptr().get();
        auto& ep_ctx      = input_ctx.get_as<EndpointContext>(ContextArray::ep_ici(ep_idx));
        // TODO: ? HS bulk/interrupt have a max burst size for USB 2.
        ep_ctx.set_max_packet_size(ep.max_packet_size);
        if (ep.ttype == usb::TransferType::Interrupt &&
            (m_speed == DeviceSpeed::Full || m_speed == DeviceSpeed::Low)) {
            // Interval simple value in milliseconds.
            VERIFY(ep.b_interval >= 1 && ep.b_interval <= 255);
            auto n_units = 8u * ep.b_interval;
            ep_ctx.set_interval(bek::floor_log_2(n_units));
        } else if (m_speed == DeviceSpeed::Full && ep.ttype == usb::TransferType::Isochronous) {
            VERIFY(ep.b_interval >= 1 && ep.b_interval <= 16);
            ep_ctx.set_interval(ep.b_interval + 2);
        } else if (m_speed != DeviceSpeed::Low && (ep.ttype == usb::TransferType::Interrupt ||
                                                   ep.ttype == usb::TransferType::Isochronous)) {
            VERIFY(ep.b_interval >= 1 && ep.b_interval <= 16);
            ep_ctx.set_interval(ep.b_interval - 1);
        }

        u64 us_interval = 125ul << (ep_ctx.get_interval());
        if (us_interval > 1000) {
            DBG::dbgln("Interval: {}ms."_sv, us_interval / 1000);
        } else {
            DBG::dbgln("Interval: {}us."_sv, us_interval);
        }
        // Default
        ep_ctx.set_error_count(ep.ttype == usb::TransferType::Isochronous ? 0 : 3);
        ep_ctx.set_endpoint_type(ep_type_from(ep.ttype, ep.direction));
        ep_ctx.set_dequeue_ptr(ring_dma_ptr, true);
        // FIXME: Only valid for control.
        ep_ctx.set_avg_trb_length(8);
    }

    bek::array<bek::own_ptr<xhci::ProducerRing>, 31> m_transfer_rings;
    Controller& m_controller;
    u8 m_slot_id;
    ContextArray m_ctx_array;
    State m_state;
    u8 m_root_hub_port;
    DeviceSpeed m_speed;
};

struct xhci::Interrupter {
    InterrupterRegisters registers;
    xhci::EventRing event_ring;
    explicit Interrupter(InterrupterRegisters interrupter_registers, mem::dma_pool& pool)
        : registers(interrupter_registers), event_ring{pool} {}

    void setup() {
        registers.ERSTSZ(event_ring.erst_size());
        registers.ERDP(event_ring.current_ring_dma_ptr().get());
        registers.ERSTBA(event_ring.erst_dma_ptr().get());
    }

    void enable() { registers.set_interrupt_enable(); }

    void disable() { registers.clear_interrupt_enable(); }

    [[nodiscard]] bool is_pending() const { return registers.get_interrupt_pending(); }

    /// Necessary if (and only if) using pin-based interrupts.
    void clear_pending() { registers.clear_interrupt_pending(); }

    template <typename F>
    void process_interrupt(F&& callback) {
        // Loop through any waiting event TRBs on event ring, and process them.
        // Once finished, can update ring dequeue ptr register, clearing busy flag.
        auto trb = event_ring.process();
        while (trb.is_valid()) {
            callback(*trb);
            trb = event_ring.process();
        }
        registers.update_erdp(event_ring.current_ring_dma_ptr().get(), true);
    }
};

bool Controller::initialise() {
    // Get Version
    auto version_data = capability_registers.HCIVERSION();
    DBG::dbgln("HCI Version: {}.{}.{}"_sv, version_data >> 8, (version_data >> 4) & 0xF,
               version_data & 0xF);
    DBG::dbgln("Max Ports: {}"_sv, capability_registers.max_ports());

    // TODO: Check mapping
    DBG::dbgln("Resetting Controller"_sv);
    // Stop Controller
    operational_registers.clear_run_stop();

    // Wait for Halt
    while (!operational_registers.get_hc_halted()) {
    }

    operational_registers.set_hc_reset();

    while (operational_registers.get_hc_not_ready()) {
    }
    DBG::dbgln("Reset Complete"_sv);

    for (u8 i = 1; i <= capability_registers.max_ports(); i++) {
        m_ports.push_back(Port{nullptr, i, operational_registers.port(i), nullptr});
    }

    // Now, look at capabilities.
    ExtendedCapabilities caps{capability_registers.extended_cap_ptr()};

    for (auto cap : caps) {
        if (cap.id == ExtendedCapabilities::SupportedProtocol) {
            m_protocols.push_back(SupportedProtocol::from_capability(cap));
            auto& protocol     = m_protocols.back();
            uSize port_n_start = protocol.compatible_port_offset;
            uSize port_n_end   = protocol.compatible_port_count + port_n_start;

            for (uSize port_n = port_n_start; port_n < port_n_end; port_n++) {
                if (1 <= port_n && port_n <= m_ports.size()) {
                    m_ports[port_n - 1].protocol = &protocol;
                    DBG::dbgln("Port {}: Protocol {}{:X}.{:X} ({} speeds)"_sv, port_n,
                               protocol.name, protocol.major, protocol.minor,
                               protocol.speeds.size());
                } else {
                    DBG::dbgln("Disaster: protocol specifies a non-existent port!"_sv);
                }
            }
        }
    }

    max_device_slots = capability_registers.max_device_slots();
    operational_registers.max_device_slots_enabled(max_device_slots);

    // Allocate Scratchpad
    auto n_scratchpad   = capability_registers.max_scratchpad_buffers();
    auto xhci_page_size = operational_registers.page_size();

    DBG::dbgln("Page Size: {}, Scratchpad Buffers: {}, Dev Slots: {}, Ports: {}"_sv, xhci_page_size,
               n_scratchpad, max_device_slots, m_ports.size());

    // TODO: Deallocate scratchpad buffers?
    // §6.6 of spec
    for (int i = 0; i < n_scratchpad; i++) {
        auto scratchpad = function->dma_pool().allocate(xhci_page_size, xhci_page_size);
        bek::memset(scratchpad.data(), 0, xhci_page_size);
        // IS THIS THE RIGHT THING?
        m_scratchpad_ptr_array[i] = scratchpad.dma_ptr().get();
        scratchpad.release();
    }
    // Set Up DBCAA
    for (auto& addr : m_dbcaa) {
        addr = 0;
    }
    m_dbcaa[0] = m_scratchpad_ptr_array.dma_ptr().get();
    m_scratchpad_ptr_array.sync_after_write();
    operational_registers.DCBAAP(m_dbcaa.dma_ptr().get());

    // Since ring starts 0-ed, the initial ccs is set to 1.
    operational_registers.set_command_ring_pointer(m_command_ring.dma_ptr().get(), true);

    DBG::dbgln("Setting up Interrupters"_sv);
    // TODO: Figure out MSI-X
    auto max_interrupters = capability_registers.max_interrupters();
    if (function->interrupt_type() == pcie::InterruptType::Pin) {
        // Only initialise the primary interrupt.
        m_primary_interrupter =
            bek::make_own<Interrupter>(runtime_registers.interrupter(0), function->dma_pool());
        m_primary_interrupter->setup();
        m_primary_interrupter->enable();
    } else {
        ASSERT(false);
    }

    // TODO: Register interrupt handler (PCIe something something).
    function->set_pin_interrupt_handler([this] { handle_interrupt(); });
    function->enable_pin_interrupts();

    operational_registers.set_interrupter_enable();
    operational_registers.set_run_stop();

    // Wait for not Halt
    while (operational_registers.get_hc_halted()) {
    }
    DBG::dbgln("Unhalted. Will attempt to reset ports."_sv);

    for (uSize i = 1; i <= m_ports.size(); i++) {
        operational_registers.port(i).set_port_reset();
    }

    return true;
}

Controller::Controller(mem::PCIeDeviceArea deviceArea, CapabilityRegisters t_capability_registers,
                       pcie::Function* function)
    : function{function},
      xhci_space{deviceArea},
      capability_registers{t_capability_registers},
      operational_registers{
          xhci_space.subdivide(capability_registers.CAPLENGTH(), OperationalRegisters::BANK_SIZE)},
      runtime_registers(xhci_space.subdivide(capability_registers.runtime_register_offset(),
                                             RuntimeRegisters::BANK_SIZE)),
      doorbell_registers(
          xhci_space.subdivide(capability_registers.DBOFF(), DoorbellRegisters::BANK_SIZE)),
      m_dbcaa(function->dma_pool(), 256, 64),
      m_scratchpad_ptr_array(function->dma_pool(), capability_registers.max_scratchpad_buffers(),
                             64),
      m_command_ring(function->dma_pool()),
      max_device_slots{capability_registers.max_device_slots()} {
    DBG::dbgln("Max Dev Slots: {}"_sv, max_device_slots);
}
void Controller::handle_interrupt() {
    DBG::dbgln("Got Interrupt!"_sv);
    if (!m_primary_interrupter->is_pending()) {
        DBG::dbgln("Interrupt is not pending?"_sv);
        return;
    }

    // Only necessary for pin-based interrupts.
    m_primary_interrupter->clear_pending();
    m_primary_interrupter->process_interrupt([this](const xhci::EventTRB& trb) {
        DBG::dbgln("Event: {}"_sv, trb.kind);
        if (trb.kind == xhci::TRBType::PortStatusChange) {
            auto port_id   = trb.port_id;
            auto port_regs = operational_registers.port(port_id);
            if (port_regs.get_connect_status_change() && port_regs.get_connect_status()) {
                DBG::dbgln("New Connection to port {}"_sv, port_id);
                port_regs.clear_connect_status_change();
                port_regs.clear_port_reset_change();
                // FIXME: Not guaranteed for USB2 (or 3?)
                VERIFY(port_regs.get_port_enabled());
                VERIFY(port_regs.get_port_link_state() == PortOperationalRegisters::U0);
                Device::create(*this, port_id);
            } else {
                DBG::dbgln("Unknown change to port {}"_sv, port_id);
            }
        } else if (trb.kind == xhci::TRBType::CommandCompletion) {
            m_command_ring.process_completion(trb);
        } else if (trb.kind == TRBType::TransferEvent) {
            VERIFY(trb.slot_id >= 1);
            VERIFY(m_devices.size() >= trb.slot_id);
            m_devices[trb.slot_id - 1]->on_transfer_event(trb);
        }
    });
}
bool probe_xhci(pcie::Function& function) {
    if (function.class_code() != pcie::ClassCode{0xC, 0x03, 0x30}) return false;
    DBG::dbgln("Detected xHCI PCIe Function."_sv);
    // Get Register space

    auto space = function.initialise_bar(0);
    function.enable_memory_accesses();

    // FIXME: Make global somehow
    auto* p_xhci = new Controller(space, {space}, &function);
    p_xhci->initialise();
    return true;
}
