/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2023 Bekos Contributors
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

#ifndef BEKOS_AREAS_H
#define BEKOS_AREAS_H

#include <library/types.h>

namespace mem {
/// Represents a mapping of a contiguous physical space to contiguous virtual space, intended for
/// memory-mapped peripherals.
class DeviceArea {
public:
    DeviceArea(uPtr mPhysicalPtr, uPtr mVirtualPtr, uSize mSize)
        : m_physical_ptr(mPhysicalPtr), m_virtual_ptr(mVirtualPtr), m_size(mSize) {}

    [[nodiscard]] DeviceArea subdivide(uSize offset, uSize size) const {
        return {m_physical_ptr + offset, m_virtual_ptr + offset, size};
    }

    /// Reads object from memory starting at offset. Uses volatile. Caller responsible for proper
    /// alignment. \tparam T Type of object to read. Should be integral (or at least POD) type.
    /// \param offset Offset in bytes.
    /// \return Object read from memory.
    template <typename T>
    [[nodiscard]] T read(uSize offset) const {
        return *reinterpret_cast<const volatile T*>(m_virtual_ptr + offset);
    }

    /// Writes provided object to memory starting at offset. Uses volatile. Caller responsible for
    /// proper alignment. \tparam T Type of object to write. Should be integral (or at least POD)
    /// type. \param offset Offset in bytes. \param obj Object to write.
    template <typename T>
    void write(uSize offset, T obj) {
        *reinterpret_cast<volatile T*>(m_virtual_ptr + offset) = obj;
    }

    [[nodiscard]] uSize size() const { return m_size; }
    void* ptr() const { return reinterpret_cast<void*>(m_virtual_ptr); }

private:
    uPtr m_physical_ptr;
    uPtr m_virtual_ptr;
    uSize m_size;

    // TODO: Optionally, own a virtual memory object?
};

class PCIeDeviceArea {
public:
    PCIeDeviceArea(DeviceArea area) : m_area{area} {}

    [[nodiscard]] PCIeDeviceArea subdivide(uSize offset, uSize size) const {
        return {m_area.subdivide(offset, size)};
    }

    /// Reads object from memory starting at offset. Uses volatile. Caller responsible for proper
    /// alignment. \tparam T Type of object to read. Should be integral (or at least POD) type.
    /// \param offset Offset in bytes.
    /// \return Object read from memory.
    template <typename T>
    [[nodiscard]] T read(uSize offset) const;

    /// Writes provided object to memory starting at offset. Uses volatile. Caller responsible for
    /// proper alignment. \tparam T Type of object to write. Should be integral (or at least POD)
    /// type. \param offset Offset in bytes. \param obj Object to write.
    template <typename T>
    void write(uSize offset, T obj);

    void* ptr() const { return m_area.ptr(); }

    [[nodiscard]] uSize size() const { return m_area.size(); }

private:
    DeviceArea m_area;

    // TODO: Optionally, own a virtual memory object?
};

template <>
[[nodiscard]] inline u8 PCIeDeviceArea::read<u8>(uSize offset) const {
    auto x = m_area.read<u32>(offset & ~0b11u);
    return (x >> (8 * (offset & 0b11))) & 0xFF;
}

template <>
[[nodiscard]] inline u16 PCIeDeviceArea::read<u16>(uSize offset) const {
    auto x = m_area.read<u32>(offset & ~0b1u);
    return (x >> (16 * (offset & 0b11))) & 0xFFFF;
}

template <>
[[nodiscard]] inline u32 PCIeDeviceArea::read<u32>(uSize offset) const {
    return m_area.read<u32>(offset);
}

template <>
[[nodiscard]] inline u64 PCIeDeviceArea::read<u64>(uSize offset) const {
    return m_area.read<u32>(offset) | static_cast<u64>(m_area.read<u32>(offset + 4)) << 32;
}

template <>
inline void PCIeDeviceArea::write<u32>(uSize offset, u32 obj) {
    m_area.write<u32>(offset, obj);
}

template <>
inline void PCIeDeviceArea::write<u64>(uSize offset, u64 obj) {
    m_area.write<u32>(offset, obj);
    m_area.write<u32>(offset + 4, obj >> 32);
}
}  // namespace mem

#endif  // BEKOS_AREAS_H
