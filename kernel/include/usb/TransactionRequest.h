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

#ifndef BEKOS_TRANSACTIONREQUEST_H
#define BEKOS_TRANSACTIONREQUEST_H

#include <library/function.h>

#include "Device.h"

struct ControlSetupData {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} __attribute((packed));
static_assert(sizeof(ControlSetupData) == 8, "ControlSetupData packing incorrect.");

enum class TransferType { CONTROL, ISOCHRONOUS, INTERRUPT, BULK };
enum class TransferDirection { IN, OUT };

struct TransferRequest {
    Device* device;
    u8 endpoint;
    TransferType type;
    TransferDirection direction;
    u32 transfer_size;
    void* buffer;
    /// Only used for control transfers - including makes things easier
    ControlSetupData control_data;
    bek::function<void(void)> completion_callback;
};

#endif  // BEKOS_TRANSACTIONREQUEST_H
