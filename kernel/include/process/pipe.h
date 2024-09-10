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

#ifndef BEKOS_PIPE_H
#define BEKOS_PIPE_H

#include "bek/types.h"
#include "bek/vector.h"
#include "entity.h"
#include "library/kernel_error.h"
#include "library/transactional_buffer.h"

class Pipe : public bek::RefCounted<Pipe> {
public:
    Pipe();
    expected<uSize> write(TransactionalBuffer& buffer, bool blocking);
    expected<uSize> read(TransactionalBuffer& buffer, bool blocking);

private:
    bek::vector<u8> m_data;
    uSize m_read_idx;
    uSize m_write_idx;
};

class PipeHandle : public EntityHandle {
public:
    PipeHandle(bek::shared_ptr<Pipe> pipe, bool is_reader, bool is_blocking)
        : m_pipe(bek::move(pipe)), m_is_reader(is_reader), m_is_blocking(is_blocking) {}

    SupportedOperations get_supported_operations() const override { return m_is_reader ? Read : Write; }
    expected<uSize> read(u64 offset, TransactionalBuffer& buffer) override {
        return m_is_reader ? m_pipe->read(buffer, m_is_blocking) : ENOTSUP;
    }
    expected<uSize> write(u64 offset, TransactionalBuffer& buffer) override {
        return !m_is_reader ? m_pipe->write(buffer, m_is_blocking) : ENOTSUP;
    }

    Kind kind() const override { return Kind::Pipe; }
    using EntityKind = PipeHandle;

private:
    bek::shared_ptr<Pipe> m_pipe;
    bool m_is_reader;
    bool m_is_blocking;
};

#endif  // BEKOS_PIPE_H
