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

#ifndef BEKOS_BLOCK_DEVICE_H
#define BEKOS_BLOCK_DEVICE_H

#include "bek/buffer.h"
#include "bek/own_ptr.h"
#include "bek/str.h"
#include "bek/types.h"
#include "bek/vector.h"
#include "library/function.h"
#include "library/hashtable.h"

namespace blk {

class PartitionProxyDevice;

constexpr inline uSize SECTOR_SIZE = 512;

enum class TransferResult {
    Success,
    BadAlignment,
    OutOfBounds,
    Failure,
};

using TransferCallback = bek::function<void(TransferResult), false, true>;

class BlockDevice {
public:
    virtual uSize logical_block_size() const                   = 0;
    virtual bool is_read_only() const                          = 0;
    virtual uSize capacity() const                             = 0;
    virtual TransferResult schedule_read(uSize byte_offset, bek::mut_buffer buffer, TransferCallback cb)  = 0;
    virtual TransferResult schedule_write(uSize byte_offset, bek::buffer buffer, TransferCallback cb) = 0;
    virtual ~BlockDevice()                                     = default;

    [[nodiscard]] constexpr u32 global_id() const { return m_global_id; }

    [[nodiscard]] bek::str_view name() const { return m_name.view(); }

protected:
    BlockDevice(bek::string name, u32 global_id)
        : m_global_id(global_id), m_name(bek::move(name)) {}

    u32 m_global_id;
    bek::string m_name;
};

TransferResult blocking_read(BlockDevice& dev, uSize byte_offset, bek::mut_buffer buffer);
TransferResult blocking_write(BlockDevice& dev, uSize byte_offset, bek::buffer buffer);

class BlockDeviceRegistry {
public:
    static BlockDeviceRegistry& the();
    void register_raw_device(bek::own_ptr<BlockDevice> device);
    bek::pair<bek::string, u32> allocate_identifiers(bek::str_view prefix);
    bek::vector<BlockDevice*> get_accessible_devices() const;

private:
    bek::vector<bek::own_ptr<BlockDevice>> m_raw_devices;
    bek::vector<bek::own_ptr<PartitionProxyDevice>> m_partitions;
    u32 global_next_id;
    bek::hashtable<bek::string, u32> m_next_ids;
};

}  // namespace blk

#endif //BEKOS_BLOCK_DEVICE_H
