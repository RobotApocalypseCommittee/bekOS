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
#ifndef BEKOS_FRAMEBUFFER_H
#define BEKOS_FRAMEBUFFER_H

#include "api/protocols/fb.h"
#include "bek/types.h"
#include "device.h"
#include "mm/addresses.h"
#include "mm/device_backed_region.h"

struct FramebufferInfo {
    mem::VirtualRegion region;
    u32 byte_stride;
    bek::shared_ptr<DeviceBackedRegion> hard_region;
};

class FramebufferDevice : public Device {
public:
    // Framebuffer Specific
    virtual const FramebufferInfo& front_buffer() const = 0;
    virtual ErrorCode flush_rect(protocol::fb::Rect r) = 0;
    virtual protocol::fb::DisplayInfo info() const = 0;
    virtual expected<protocol::fb::DisplayInfo> try_set_info(const protocol::fb::DisplayInfo& info) = 0;

    // Device
    Kind kind() const override;
    bek::str_view preferred_name_prefix() const override;
    bek::optional<DeviceProtocol> userspace_protocol() const override;
    expected<long> on_userspace_message(u64 id, TransactionalBuffer& message) override;
};

#endif  // BEKOS_FRAMEBUFFER_H
