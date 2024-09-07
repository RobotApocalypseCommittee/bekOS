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

#ifndef BEKOS_USB_H
#define BEKOS_USB_H

#include <library/optional.h>

#include "bek/buffer.h"
#include "bek/format_core.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "library/function.h"
#include "mm/dma_utils.h"

namespace usb {

enum class TransferType {
    Control     = 0,
    Isochronous = 1,
    Bulk        = 2,
    Interrupt   = 3,
};

void bek_basic_format(bek::OutputStream& out, const TransferType&);

enum class Direction { In,
    Out
};

void bek_basic_format(bek::OutputStream& out, const Direction&);

enum class ControlTransferType : u8 { Standard, Class, Vendor };

enum class ControlTransferTarget : u8 { Device, Interface, Endpoint, Other };

struct SetupPacket {
    u8 request_type;
    u8 request;
    u16 value;
    u16 index;
    u16 data_length;
    constexpr static u8 make_req_type(Direction dir, ControlTransferType ttype,
                                      ControlTransferTarget target) {
        u8 v = (dir == Direction::In ? (1u << 7) : 0);
        v |= (static_cast<u8>(ttype) & 0b11) << 5;
        v |= (static_cast<u8>(target) & 0b11);
        return v;
    }
    /// Returns the direction of the *data* stage.
    [[nodiscard]] constexpr Direction get_direction() const {
        return (request_type & (1u << 7)) ? Direction::In : Direction::Out;
    }
};

struct Endpoint {
    u8 number;
    Direction direction;
    TransferType ttype;
    u16 max_packet_size;
    u8 b_interval;
    /// Only for interrupt
    bool is_notification;

    // TODO: Isochronous Flags.
};

struct Interface {
    u8 interface_class;
    u8 interface_subclass;
    u8 interface_protocol;
    u8 interface_number;
    u8 interface_alternative;
    bek::vector<Endpoint> endpoints;
};

struct TransferRequest {
    enum class Result { Success, BadRequest, BadEndpoint, Failure };
    using Callback = bek::function<void(bek::optional<mem::own_dma_buffer>, Result), false, true>;
    /// Technically, this can be deduced from direction and endpoint n, but good to have a sanity
    /// check.
    TransferType type;
    Direction direction;
    u8 endpoint_number;
    Callback callback;
    bek::optional<mem::own_dma_buffer> buffer;
    bek::optional<SetupPacket> control_setup;
};

class Device {
public:
    virtual bool schedule_transfer(TransferRequest request) = 0;

    virtual void enable_configuration(u8 configuration_n,
                                      const bek::vector<usb::Endpoint>& endpoints,
                                      bek::function<void(bool)> cb) = 0;

    virtual mem::own_dma_buffer allocate_buffer(uSize size) = 0;

    virtual ~Device() = default;
};

class Registrar {
public:
    static Registrar& the();

    /// Registers a device with the Registrar.
    /// The Device must be addressed, and ready to receive descriptor requests to EP0.
    void register_device(Device&);

private:
    bek::vector<Device*> m_devices;
};

class Functionality {
public:
    virtual ~Functionality() = default;
};

}

#endif  // BEKOS_USB_H
