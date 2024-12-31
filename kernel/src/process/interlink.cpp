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

#include "process/interlink.h"

#include <api/interlink.h>
#include <library/debug.h>
#include <process/process.h>

#include "library/hashtable.h"

using namespace interlink;
using namespace sc::interlink;

using DBG = DebugScope<"Interlink", DebugLevel::INFO>;

inline constexpr uSize INTERLINK_DEFAULT_RINGBUFFER_SIZE = 1024u;

struct GlobalInterlinkData {
    bek::hashtable<bek::string, bek::shared_ptr<Server>> servers;
};

GlobalInterlinkData* g_interlink_data = nullptr;

void interlink::initialize() { g_interlink_data = new GlobalInterlinkData; }

expected<bek::shared_ptr<Server>> interlink::lookup(const bek::string& address) {
    auto find_res = g_interlink_data->servers.find(address);
    if (find_res) {
        return *find_res;
    } else {
        return ENOENT;
    }
}

expected<bek::shared_ptr<Server>> interlink::advertise(bek::string address) {
    auto ins_res = g_interlink_data->servers.insert({address, bek::adopt_shared(new Server(address))});
    if (ins_res.second) {
        return ins_res.first->second;
    } else {
        return EADDRINUSE;
    }
}
ServerHandle::~ServerHandle() { m_server->detach_handle(*this); }
void Server::detach_handle(ServerHandle& handle) {
    VERIFY(&handle == m_server_handle);
    m_server_handle = nullptr;
    // We de-advertise.
    g_interlink_data->servers.extract(m_address);
}

Connection::QueuedMessage::~QueuedMessage() { VERIFY(kind == DATA); }
expected<uSize> Connection::client_receive(TransactionalBuffer& buffer, bool blocking) {
    return read_from_queue(buffer, m_client_ringbuffer, m_client_queue, blocking);
}
expected<uSize> Connection::send_to_client(TransactionalBuffer& buffer, bool blocking) {
    return write_to_queue(buffer, m_client_ringbuffer, m_client_queue, blocking);
}
expected<uSize> Connection::server_read(TransactionalBuffer& buffer, bool blocking) {
    return read_from_queue(buffer, m_server_ringbuffer, m_server_queue, blocking);
}
expected<uSize> Connection::send_to_server(TransactionalBuffer& buffer, bool blocking) {
    return write_to_queue(buffer, m_server_ringbuffer, m_server_queue, blocking);
}

Connection::~Connection() { m_server->detach_connection(*this); }
expected<uSize> Connection::write_to_queue(TransactionalBuffer& buffer, ring_buffer& binary_queue,
                                           bek::vector<Connection::QueuedMessage>& message_queue, bool blocking) {
    // We need the ringbuffer to contain enough space for our message.
    // Blocking currently makes no difference.

    // First, read and verify the buffer.
    auto header = EXPECTED_TRY(buffer.read_object<MessageHeader>());
    if (header.total_size > buffer.size()) return EINVAL;
    if (header.payload_item_count * sizeof(MessageHeader::PayloadItem) > buffer.size()) return EINVAL;
    uSize total_data_size = 0;

    DBG::infoln("Writing message of {} bytes ({} payload items)"_sv, header.total_size, header.payload_item_count);

    for (uSize i = 0; i < header.payload_item_count; i++) {
        auto payload_item = EXPECTED_TRY(buffer.read_object<MessageHeader::PayloadItem>(
            sizeof(MessageHeader) + i * sizeof(MessageHeader::PayloadItem)));
        if (payload_item.kind == MessageHeader::PayloadItem::DATA) {
            total_data_size += payload_item.data.len;
            if (payload_item.data.offset >= buffer.size() ||
                payload_item.data.len + payload_item.data.offset > buffer.size()) {
                return EINVAL;
            }
        }
    }

    if (binary_queue.capacity() < total_data_size) return EOVERFLOW;

    if (binary_queue.free_bytes() < total_data_size && !blocking) return EAGAIN;

    for (uSize i = 0; i < header.payload_item_count; i++) {
        auto payload_item = EXPECTED_TRY(buffer.read_object<MessageHeader::PayloadItem>(
            sizeof(MessageHeader) + i * sizeof(MessageHeader::PayloadItem)));
        bool is_final = i + 1 == header.payload_item_count;
        if (payload_item.kind == MessageHeader::PayloadItem::DATA) {
            TransactionalBufferSubset to_write{buffer, payload_item.data.offset, payload_item.data.len};
            auto res = binary_queue.write_to(to_write, false);
            while (res.has_error() && res.error() == EAGAIN && blocking) {
                res = binary_queue.write_to(to_write, false);
            }
            message_queue.push_back(QueuedMessage(is_final, header.message_id, EXPECTED_TRY(res)));
        } else if (payload_item.kind == MessageHeader::PayloadItem::FD) {
            // FIXME: VERIFY THIS FIRST.
            auto entity = EXPECTED_TRY(ProcessManager::the().current_process().get_open_entity(payload_item.fd));
            message_queue.push_back(QueuedMessage(is_final, header.message_id, bek::move(entity)));
        } else if (payload_item.kind == MessageHeader::PayloadItem::MEMORY) {
            MemoryOperation allowed_ops =
                (payload_item.memory.can_read ? MemoryOperation::Read : MemoryOperation::None) |
                (payload_item.memory.can_write ? MemoryOperation::Write : MemoryOperation::None);
            mem::UserRegion region = {payload_item.memory.ptr, payload_item.memory.size};
            auto shareable_memory = EXPECTED_TRY(ProcessManager::the().current_process().with_space_manager(
                [region, &allowed_ops](SpaceManager& manager) -> expected<bek::shared_ptr<mem::BackingRegion>> {
                    if (!manager.check_region(region.start.ptr, region.size, allowed_ops)) return EPERM;
                    return manager.get_shareable_region(region);
                }));
            message_queue.push_back(
                QueuedMessage(is_final, header.message_id, bek::move(shareable_memory), allowed_ops));
        }
    }
    return total_data_size;
}
expected<uSize> Connection::read_from_queue(TransactionalBuffer& buffer, ring_buffer& binary_queue,
                                            bek::vector<Connection::QueuedMessage>& message_queue, bool blocking) {
    // First, check we have enough size.
    u32 payload_items = 0;
    uSize total_data_size = 0;
    u32 message_id = 0;
    if (message_queue.size() == 0 && !blocking) {
        return EAGAIN;
    }
    while (message_queue.size() == 0) {
        asm volatile ("nop");
    }
    for (auto& payload_item : message_queue) {
        payload_items++;
        if (payload_item.kind == QueuedMessage::DATA) {
            total_data_size += payload_item.data_size;
        }
        message_id = payload_item.message_id;
        if (payload_item.is_final) break;
    }
    uSize estimated_required_size = sizeof(sc::interlink::MessageHeader) +
                                    payload_items * sizeof(sc::interlink::MessageHeader::PayloadItem) + total_data_size;
    DBG::infoln("Receiving message of size {} ({} payload items)"_sv, estimated_required_size, payload_items);
    if (buffer.size() < estimated_required_size) {
        // FIXME: Correct error code.
        return EOVERFLOW;
    }

    EXPECTED_TRY(buffer.write_object(MessageHeader{
        .total_size = estimated_required_size,
        .payload_item_count = payload_items,
        .message_id = message_id,
    }));

    uSize current_data_offset =
        sizeof(sc::interlink::MessageHeader) + payload_items * sizeof(sc::interlink::MessageHeader::PayloadItem);

    for (uSize i = 0; i < payload_items; i++) {
        uSize payload_item_offset = sizeof(MessageHeader) + i * sizeof(MessageHeader::PayloadItem);
        auto item = message_queue.pop(0);
        if (item.kind == QueuedMessage::DATA) {
            TransactionalBufferSubset target{buffer, current_data_offset, item.data_size};
            binary_queue.read_to(target, false);

            EXPECTED_TRY(buffer.write_object(
                MessageHeader::PayloadItem{.kind = MessageHeader::PayloadItem::DATA,
                                           .data = {.offset = current_data_offset, .len = item.data_size}},
                payload_item_offset));
            current_data_offset += item.data_size;
        } else if (item.kind == QueuedMessage::ENTITY) {
            // FIXME: Group?
            auto res =
                ProcessManager::the().current_process().allocate_entity_handle_slot(bek::move(item.entity_handle), 0);
            EXPECTED_TRY(buffer.write_object(
                MessageHeader::PayloadItem{.kind = MessageHeader::PayloadItem::FD, .fd = res}, payload_item_offset));
        } else {
            auto new_region =
                EXPECTED_TRY(ProcessManager::the().current_process().with_space_manager([&item](SpaceManager& manager) {
                    return manager.place_region(bek::nullopt, item.memory_slice.permissions, bek::string{"interlinked"},
                                                bek::move(item.memory_slice.region));
                }));
            EXPECTED_TRY(buffer.write_object(
                MessageHeader::PayloadItem{.kind = MessageHeader::PayloadItem::MEMORY,
                                           .memory =
                                               {
                                                   .ptr = new_region.start.ptr,
                                                   .size = new_region.size,
                                                   .can_read = (item.memory_slice.permissions &
                                                                MemoryOperation::Read) != MemoryOperation::None,
                                                   .can_write = (item.memory_slice.permissions &
                                                                 MemoryOperation::Write) != MemoryOperation::None,
                                               }},
                payload_item_offset));
        }
    }

    return current_data_offset;
}
void Server::detach_connection(Connection& connection) {
    for (auto& c : m_pending_connections) {
        if (c.get() == &connection) {
            m_pending_connections.extract(c);
            return;
        }
    }
}
Server::Server(bek::string address) : m_address(bek::move(address)) {}
EntityHandle::Kind ServerHandle::kind() const { return Kind::InterlinkServer; }
EntityHandle::SupportedOperations ServerHandle::get_supported_operations() const { return None; }
expected<bek::shared_ptr<Connection>> Server::connect() {
    auto connection = bek::adopt_shared(new Connection(this, INTERLINK_DEFAULT_RINGBUFFER_SIZE));
    m_pending_connections.push_back(connection);
    return connection;
}

expected<bek::shared_ptr<Connection>> Server::accept() {
    if (m_pending_connections.size()) {
        return m_pending_connections.pop();
    } else {
        return EAGAIN;
    }
}
bek::shared_ptr<ServerHandle> Server::take_handle() {
    if (m_server_handle) {
        return {m_server_handle};
    } else {
        m_server_handle = new ServerHandle{bek::shared_ptr{this}};
        return bek::adopt_shared(m_server_handle);
    }
}

expected<uSize> ConnectionHandle::receive(TransactionalBuffer& buffer, bool blocking) {
    if (m_side == CLIENT) return m_connection->client_receive(buffer, blocking);
    return m_connection->server_read(buffer, blocking);
}
expected<uSize> ConnectionHandle::send(TransactionalBuffer& buffer, bool blocking) {
    if (m_side == CLIENT) return m_connection->send_to_server(buffer, blocking);
    return m_connection->send_to_client(buffer, blocking);
}
