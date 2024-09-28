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

#include "peripherals/framebuffer.h"

#include "bek/assertions.h"
#include "bek/utility.h"
#include "library/debug.h"
#include "process/process.h"

using DBG = DebugScope<"FB", DebugLevel::WARN>;
using namespace protocol::fb;

Device::Kind FramebufferDevice::kind() const { return Device::Kind::Framebuffer; }

bek::optional<DeviceProtocol> FramebufferDevice::userspace_protocol() const {
    return DeviceProtocol::FramebufferProvider;
}
expected<long> FramebufferDevice::on_userspace_message(u64, TransactionalBuffer& message) {
    auto msg_id = EXPECTED_TRY(message.read_object<bek::underlying_type<MessageKind>>());
    if (msg_id >= MessageKind::MessageKindEnd_) {
        return EINVAL;
    }
    auto msg_kind = static_cast<MessageKind>(msg_id);

    switch (msg_kind) {
        case GetDisplayInfo: {
            auto msg = EXPECTED_TRY(message.read_object<DisplayInfoMessage>());
            msg.info = info();
            return message.write_object(msg).map_value([](auto v) { return (long)v; });
        }
        case MapFramebuffer: {
            auto msg = EXPECTED_TRY(message.read_object<MapMessage>());
            auto framebuffer = front_buffer();
            auto virt_region = EXPECTED_TRY(
                ProcessManager::the().current_process().with_space_manager([&framebuffer](SpaceManager& manager) {
                    return manager.place_region(bek::nullopt, MemoryOperation::Read | MemoryOperation::Write,
                                                bek::string{"framebuffer"}, framebuffer.hard_region);
                }));
            auto fb_info = info();
            msg.buffer = virt_region.start.get();
            msg.size = virt_region.size;
            msg.pixel_height = fb_info.height;
            msg.pixel_width = fb_info.width;
            msg.row_stride = framebuffer.byte_stride;
            return message.write_object(msg).map_value([](auto v) { return (long)v; });
        }
        case UnmapFramebuffer: {
            auto framebuffer = front_buffer();
            return ProcessManager::the().current_process().with_space_manager([&framebuffer](SpaceManager& manager) {
                return manager.deallocate_userspace_region(framebuffer.hard_region);
            });
        }
        case SetDisplayInfo: {
            auto msg = EXPECTED_TRY(message.read_object<DisplayInfoMessage>());
            msg.info = EXPECTED_TRY(try_set_info(msg.info));
            return message.write_object(msg).map_value([](auto v) { return (long)v; });
        }
        case FlushRect: {
            auto msg = EXPECTED_TRY(message.read_object<FlushRectMessage>());
            if (!info().supports_flush) return ENOTSUP;
            return flush_rect(msg.rect);
        }
        case MessageKindEnd_:
            ASSERT_UNREACHABLE();
    }
    ASSERT_UNREACHABLE();
}
bek::str_view FramebufferDevice::preferred_name_prefix() const { return "generic.framebuffer"_sv; }
