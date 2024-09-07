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

#include "library/user_buffer.h"

#include "arch/a64/memory_constants.h"

constexpr uSize user_string_max_length = 1024;

expected<uSize> UserBuffer::write_from(const void* buffer, uSize length, uSize offset) {
    if (length + offset > m_size) return EINVAL;
    // TODO: Safe Memcpy
    bek::memcopy(reinterpret_cast<void*>(m_ptr + offset), buffer, length);
    return length;
}
expected<uSize> UserBuffer::read_to(void* buffer, uSize length, uSize offset) {
    if (length + offset > m_size) return EINVAL;
    // TODO: Safe Memcpy
    bek::memcopy(buffer, reinterpret_cast<const void*>(m_ptr + offset), length);
    return length;
}
UserBuffer::UserBuffer(uPtr user_ptr, uSize size) : m_ptr{user_ptr}, m_size{size} {
    // TODO: Make arch generic.
    VERIFY(user_ptr < VA_START);
}
void UserBuffer::clear() { bek::memset(reinterpret_cast<void*>(m_ptr), 0, m_size); }
expected<bek::string> read_string_from_user(uPtr str, uSize len) {
    if (len > user_string_max_length) return EINVAL;
    UserBuffer buffer{str, len};
    bek::string string{static_cast<u32>(len), '\0'};
    buffer.read_to(string.mut_data(), len, 0);
    return string;
}
