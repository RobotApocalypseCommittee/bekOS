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

#include "ipc/message.h"
#include <bek/optional.h>

using namespace sc::interlink;

uSize calculate_size_for_message(uSize data_size, uSize fd_count, uSize memory_count) {
    uSize size = sizeof(MessageHeader);
    size += sizeof(MessageHeader::PayloadItem) * (fd_count + memory_count + 1);
    size += data_size;
    return size;
}

uSize calculate_data_offset(u32 n_payload_items) {
    return sizeof(MessageHeader) + sizeof(MessageHeader::PayloadItem) * n_payload_items;
}

ipc::MessageBuffer::MessageBuffer(u32 message_id, bek::span<u8> data, bek::span<long> fds,
                                  bek::span<bek::pair<void*, uSize>> memory_regions)
    : m_buffer(calculate_size_for_message(data.size(), fds.size(), memory_regions.size())) {
    u32 n_payload_items = static_cast<u32>(fds.size() + memory_regions.size() + 1);
    uSize data_offset = calculate_data_offset(n_payload_items);

    header() = MessageHeader{
        .total_size = m_buffer.size(),
        .payload_item_count = n_payload_items,
        .message_id = message_id,
    };

    auto* current_item = reinterpret_cast<MessageHeader::PayloadItem*>(m_buffer.data() + sizeof(MessageHeader));
    *current_item++ = MessageHeader::PayloadItem{.kind = MessageHeader::PayloadItem::DATA,
                                                 .data = {.offset = data_offset, .len = data.size()}};

    for (auto fd : fds) {
        *current_item++ = MessageHeader::PayloadItem{.kind = MessageHeader::PayloadItem::FD, .fd = fd};
    }

    for (auto [ptr, size] : memory_regions) {
        *current_item++ = MessageHeader::PayloadItem{
            .kind = MessageHeader::PayloadItem::MEMORY,
            .memory = {.ptr = reinterpret_cast<uPtr>(ptr), .size = size, .can_read = true, .can_write = true}};
    }

    // Finally
    bek::memcopy(m_buffer.data() + data_offset, data.begin(), data.size());
}
bool ipc::MessageBuffer::verify() const {
    if (m_buffer.size() < sizeof(MessageHeader)) return false;
    if (header().total_size >= m_buffer.size()) return false;
    if (calculate_data_offset(header().payload_item_count) > header().total_size) return false;
    return true;
}

bek::span<const MessageHeader::PayloadItem> ipc::MessageBuffer::payload_items() const {
    auto* first_item = reinterpret_cast<const MessageHeader::PayloadItem*>(m_buffer.data() + sizeof(MessageHeader));
    return bek::span{first_item, header().payload_item_count};
}

void ipc::MessageBuffer::ensure_capacity(uSize capacity) {
    if (m_buffer.size() < capacity) {
        m_buffer.expand(capacity - m_buffer.size());
    }
}
core::expected<ipc::Message> ipc::Message::from_buffer(const MessageBuffer& buffer) {
    if (!buffer.verify()) return EINVAL;
    bek::optional<MessageHeader::PayloadItem> data;
    bek::vector<long> fds;
    bek::vector<bek::pair<void*, uSize>> memory_regions;
    for (auto& item : buffer.payload_items()) {
        switch (item.kind) {
            case MessageHeader::PayloadItem::DATA:
                if (data.is_valid()) {
                    return EINVAL;
                } else {
                    data = item;
                }
                break;
            case MessageHeader::PayloadItem::FD:
                fds.push_back(item.fd);
                break;
            case MessageHeader::PayloadItem::MEMORY:
                memory_regions.push_back({reinterpret_cast<void*>(item.memory.ptr), item.memory.size});
                break;
            default:
                return EINVAL;
        }
    }
    bek::vector<u8> data_buf;
    if (data) {
        data_buf.expand(data->data.len);
        bek::memcopy(data_buf.data(), buffer.data_at(data->data.offset, data->data.len).begin(), data->data.len);
    }
    return Message(buffer.header().message_id, bek::move(data_buf), bek::move(fds), bek::move(memory_regions));
}
ipc::MessageBuffer ipc::Message::to_buffer() const {
    return MessageBuffer{m_message_id, bek::span{m_buffer}, bek::span{m_fds}, bek::span{m_memory_regions}};
}
void ipc::Message::encode_bytes(bek::span<u8> bytes) {
    m_buffer.expand(bytes.size());
    bek::memcopy(m_buffer.end(), bytes.begin(), bytes.size());
}
void ipc::Message::encode_fd(long fd) { m_fds.push_back(fd); }
void ipc::Message::encode_memory_region(void* ptr, uSize size) { m_memory_regions.push_back(bek::pair{ptr, size}); }
void ipc::Message::start_decoding() {
    m_cur_buffer_position = 0;
    m_cur_fd_i = 0;
    m_cur_region_i = 0;
}
core::expected<bek::span<u8>> ipc::Message::decode_bytes(uSize length) {
    if (length + m_cur_buffer_position < m_buffer.size()) {
        u8* ptr = m_buffer.data() + m_cur_buffer_position;
        m_cur_buffer_position += length;
        return bek::span{ptr, length};
    } else {
        return EOVERFLOW;
    }
}
core::expected<long> ipc::Message::decode_fd() {
    if (m_cur_fd_i < m_fds.size()) {
        return m_fds[m_cur_fd_i++];
    } else {
        return EOVERFLOW;
    }
}
core::expected<bek::pair<void*, uSize>> ipc::Message::decode_memory_region() {
    if (m_cur_region_i < m_memory_regions.size()) {
        return m_memory_regions[m_cur_region_i++];
    } else {
        return EOVERFLOW;
    }
}