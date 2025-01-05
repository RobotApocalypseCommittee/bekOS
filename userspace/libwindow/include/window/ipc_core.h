// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#ifndef BEKOS_LIBWINDOW_IPC_CORE_H
#define BEKOS_LIBWINDOW_IPC_CORE_H

#include <ipc/message.h>

#include "gfx.h"
#include "ipc/message.h"
#include "window/core.h"

template <>
struct ipc::Serializer<window::Vec>: ByteWiseSerializer<window::Vec> {};

template <>
struct ipc::Serializer<window::Rect>: ByteWiseSerializer<window::Rect> {};

template <>
struct ipc::Serializer<window::OwningBitmap> {
  void encode(const window::OwningBitmap& o, Message& msg) {
    msg.encode(o.width());
    msg.encode(o.height());
    msg.encode(o.stride());
    msg.encode_memory_region(o.buffer(), o.buffer_size());
  }
  core::expected<window::OwningBitmap> decode(ipc::Message& msg) {
    auto [buffer, buffer_size] = EXPECTED_TRY(msg.decode_memory_region());
    auto width = EXPECTED_TRY(msg.decode<u32>());
    auto height = EXPECTED_TRY(msg.decode<u32>());
    auto stride = EXPECTED_TRY(msg.decode<u32>());
    return window::OwningBitmap::create_from_ipc(buffer, buffer_size, width, height, stride);
  }
};

#endif //BEKOS_LIBWINDOW_IPC_CORE_H
