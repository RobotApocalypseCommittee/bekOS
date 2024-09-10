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

#ifndef BEKOS_ENTITY_H
#define BEKOS_ENTITY_H

#include "api/syscalls.h"
#include "bek/types.h"
#include "library/intrusive_shared_ptr.h"
#include "library/transactional_buffer.h"

/// An Entity is owned by a process, and may be a file, directory, device, stream etc
/// (anything labelled with a file descriptor id in linux?)
class EntityHandle : public bek::RefCounted<EntityHandle> {
public:
    enum SupportedOperations : u8 {
        None = 0,
        Read = 1u,
        Write = 1u << 1,
        Seek = 1u << 2,
        Message = 1u << 3,
        Configure = 1u << 4,
    };

    enum class Kind { File, Serial, Device, Null, Pipe };

    [[nodiscard]] virtual Kind kind() const = 0;

    /// Reads into buffer from offset. If offset == -1, this means a position was not / could not be supplied.
    virtual expected<uSize> read(u64 offset, TransactionalBuffer& buffer) {
        (void)offset, (void)buffer;
        return ENOTSUP;
    };

    /// Writes from buffer from offset. If offset == -1, this means a position was not / could not be supplied.
    virtual expected<uSize> write(u64 offset, TransactionalBuffer& buffer) {
        (void)offset, (void)buffer;
        return ENOTSUP;
    };
    virtual expected<uSize> seek(sc::SeekLocation position, i64 offset) {
        (void)offset, (void)position;
        return ENOTSUP;
    }
    virtual expected<long> message(u64 id, TransactionalBuffer& buffer) const {
        (void)id, (void)buffer;
        return ENOTSUP;
    }
    virtual expected<long> configure(u64 config_item, TransactionalBuffer& buffer) const {
        (void)config_item, (void)buffer;
        return ENOTSUP;
    }
    virtual SupportedOperations get_supported_operations() const = 0;
    virtual ~EntityHandle() = default;

    template <typename T>
        requires bek::same_as<T, typename T::EntityType>
    static T* as(EntityHandle& handle) {
        if (handle.kind() == T::EntityKind) {
            return static_cast<T*>(&handle);
        } else {
            return nullptr;
        }
    }
};

template <>
constexpr inline bool bek_is_bitwise_enum<EntityHandle::SupportedOperations> = true;

class NullHandle : public EntityHandle {
public:
    Kind kind() const override { return EntityHandle::Kind::Null; }
    SupportedOperations get_supported_operations() const override {
        return SupportedOperations::Read | SupportedOperations::Write;
    }
    expected<uSize> read(u64, TransactionalBuffer&) override { return 0ul; }
    expected<uSize> write(u64, TransactionalBuffer& buffer) override { return buffer.size(); }
};

#endif  // BEKOS_ENTITY_H
