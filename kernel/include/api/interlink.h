// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024 Bekos Contributors
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

#ifndef BEKOS_API_INTERLINK_H
#define BEKOS_API_INTERLINK_H

#include "bek/types.h"

namespace sc::interlink {

struct MessageHeader {
    struct PayloadItem {
        enum {
            DATA,
            FD,
            MEMORY,
        } kind;
        union {
            struct {
                u64 offset;
                u64 len;
            } data;
            long fd;
            struct {
                u64 ptr;
                u64 size;
                bool can_read;
                bool can_write;
            } memory;
        };
    };
    uSize total_size;
    u32 payload_item_count;
    u32 message_id;
};

template <uSize __PAYLOAD_N = 0>  // NOLINT(bugprone-reserved-identifier)
struct Message {
    MessageHeader header;
    MessageHeader::PayloadItem items[__PAYLOAD_N];
    u8 data[0];
};

}  // namespace sc::interlink

#endif  // BEKOS_API_INTERLINK_H
