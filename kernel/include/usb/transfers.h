/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2021  Bekos Inc Ltd
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

#ifndef BEKOS_USB_TRANSFERS_H
#define BEKOS_USB_TRANSFERS_H

#include "library/function.h"
#include "library/types.h"
#include "library/buffer.h"

namespace usb {

using CompletionCallback = bek::function<void(void*)>;
class Device;
class Endpoint;

enum class TransferType {
    CONTROL,
    INTERRUPT,
    ISOCHRONOUS,
    BULK
};

enum class TransferDirection {
    IN,
    OUT
};

struct ControlTransferSetup {
    u8 request_type;
    u8 request;
    u16 value;
    u16 index;
    u16 data_length;
};

struct PeriodicTransferInfo {
    /// Period, in frames (TODO: micro-frames?)
    /// -1 for do not repeat
    int period;
};

struct TransferRequest {
    TransferType type;
    TransferDirection direction;
    Device* device;
    Endpoint* endpoint;
    CompletionCallback callback;
    bek::buffer buffer; 
    union {
        ControlTransferSetup control_header;
        PeriodicTransferInfo periodic_info;
    };
};

}

#endif // BEKOS_USB_TRANSFERS_H