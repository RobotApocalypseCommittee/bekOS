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

#ifndef BEKOS_USER_BUFFER_H
#define BEKOS_USER_BUFFER_H

#include "bek/str.h"
#include "bek/types.h"
#include "kernel_error.h"
#include "transactional_buffer.h"

class UserBuffer : public TransactionalBuffer {
public:
    UserBuffer(uPtr user_ptr, uSize size);
    uSize size() const override { return m_size; }
    expected<uSize> write_from(const void* buffer, uSize length, uSize offset) override;
    expected<uSize> read_to(void* buffer, uSize length, uSize offset) override;
    void clear();

private:
    uPtr m_ptr;
    uSize m_size;
};

/// Reads a string from userspace buffer of length len.
/// \param str
/// \param len Length of string *including null-terminator*.
/// \return bek::string, or error if string too long, lacks terminator, or mem copy failed.
expected<bek::string> read_string_from_user(uPtr str, uSize len);

template <typename T>
expected<T> read_object_from_user(uPtr ptr) {
    UserBuffer buf{ptr, sizeof(T)};
    return buf.read_object<T>();
}

#endif  // BEKOS_USER_BUFFER_H
