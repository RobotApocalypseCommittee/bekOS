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

#ifndef BEKOS_INTERLINK_H
#define BEKOS_INTERLINK_H

#include <library/ringbuffer.h>
#include <mm/backing_region.h>
#include <mm/space_manager.h>

#include "bek/vector.h"
#include "entity.h"
#include "library/intrusive_shared_ptr.h"
#include "library/kernel_error.h"

namespace interlink {

class Server;

class ServerHandle: public EntityHandle {
public:
    [[nodiscard]] Kind kind() const override;
    SupportedOperations get_supported_operations() const override;
    ~ServerHandle() override;
    explicit ServerHandle(bek::shared_ptr<Server> server) : m_server(bek::move(server)) {}
    Server& server() const { return *m_server; }
private:
    bek::shared_ptr<Server> m_server;
};

class Connection: public bek::RefCounted<Connection> {
    struct QueuedMessage {
        enum {
            DATA,
            ENTITY,
            MEMORY,
        } kind;
        bool is_final;
        u32 message_id;
        // TODO: DANGER - technically needs an explicit move and destructor.
        // But (I'm fairly sure) it works out, because all deletes are explicit and moves are bitwise copies.
        union {
            uSize data_size{};
            bek::shared_ptr<EntityHandle> entity_handle;
            struct {
                bek::shared_ptr<mem::BackingRegion> region;
                MemoryOperation permissions;
            } memory_slice;
        };
        ~QueuedMessage();
        QueuedMessage(bool is_final, u32 message_id, uSize data_size): kind(DATA), is_final(is_final), message_id(message_id), data_size(data_size) {}
        QueuedMessage(bool is_final, u32 message_id, const bek::shared_ptr<EntityHandle> entity): kind(ENTITY), is_final(is_final), message_id(message_id), entity_handle(bek::move(entity)) {}
        QueuedMessage(bool is_final, u32 message_id, bek::shared_ptr<mem::BackingRegion> region,
                      MemoryOperation permissions)
            : kind(MEMORY), is_final(is_final), message_id(message_id), memory_slice{bek::move(region), permissions} {}
        QueuedMessage(const QueuedMessage& other) = delete;
        QueuedMessage(QueuedMessage&& other) noexcept {
            bek::memcopy(this, &other, sizeof(QueuedMessage));
            bek::memset(&other, 0, sizeof(QueuedMessage));
        }
        QueuedMessage& operator=(const QueuedMessage& other) = delete;
        QueuedMessage& operator=(QueuedMessage&& other) noexcept {
            if (this == &other) return *this;
            bek::memcopy(this, &other, sizeof(QueuedMessage));
            bek::memset(&other, 0, sizeof(QueuedMessage));
            return *this;
        }

    };

public:
    Connection(bek::shared_ptr<Server> server, uSize ringbuffer_size): m_server{bek::move(server)}, m_client_ringbuffer(ringbuffer_size), m_server_ringbuffer(ringbuffer_size) {}
    ALWAYS_INLINE expected<uSize> client_receive(TransactionalBuffer& buffer, bool blocking);
    ALWAYS_INLINE expected<uSize> send_to_client(TransactionalBuffer& buffer, bool blocking);
    ALWAYS_INLINE expected<uSize> server_read(TransactionalBuffer& buffer, bool blocking);
    ALWAYS_INLINE expected<uSize> send_to_server(TransactionalBuffer& buffer, bool blocking);

    virtual ~Connection();
private:
    static expected<uSize> write_to_queue(TransactionalBuffer& buffer, ring_buffer& binary_queue, bek::vector<Connection::QueuedMessage>&, bool blocking);
    static expected<uSize> read_from_queue(TransactionalBuffer& buffer, ring_buffer& binary_queue, bek::vector<Connection::QueuedMessage>&, bool blocking);

    bek::shared_ptr<Server> m_server;
    bek::vector<QueuedMessage> m_client_queue;
    bek::vector<QueuedMessage> m_server_queue;

    /// Buffer of pending data to client (from server).
    ring_buffer m_client_ringbuffer;
    /// Buffer of pending data to server (from client).
    ring_buffer m_server_ringbuffer;
};

class Server final : public bek::RefCounted<Server> {
public:
    explicit Server(bek::string address);
    expected<bek::shared_ptr<Connection>> connect();
    expected<bek::shared_ptr<Connection>> accept();

    bek::shared_ptr<ServerHandle> take_handle();
    void detach_handle(ServerHandle& handle);
    void detach_connection(Connection& connection);
private:
    bek::string m_address;
    ServerHandle* m_server_handle{nullptr};
    bek::vector<bek::shared_ptr<Connection>> m_pending_connections;
};

class ConnectionHandle: public EntityHandle {
public:
    enum Side { CLIENT, SERVER };

private:
    bek::shared_ptr<Connection> m_connection;
    Side m_side;

public:
    ConnectionHandle(bek::shared_ptr<Connection> connection, Side side): m_connection(bek::move(connection)), m_side(side) {};
    [[nodiscard]] Kind kind() const override { return Kind::InterlinkConnection; }
    SupportedOperations get_supported_operations() const override { return None; }
    expected<uSize> receive(TransactionalBuffer& buffer, bool blocking);
    expected<uSize> send(TransactionalBuffer& buffer, bool blocking);
};

void initialize();
expected<bek::shared_ptr<Server>> advertise(bek::string address);
expected<bek::shared_ptr<Server>> lookup(const bek::string& address);

};

#endif  // BEKOS_INTERLINK_H
