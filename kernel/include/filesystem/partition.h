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

#ifndef BEKOS_PARTITION_H
#define BEKOS_PARTITION_H

#include "block_device.h"
#include "library/format_core.h"

namespace blk {

enum class PartitionFsKind {
    None,
    /// Meaning it must be identified by contents.
    Undetermined,
    Unrecognised,
    Fat,
};

struct PartitionInfo {
    uSize sector_index;
    uSize size_sectors;
    PartitionFsKind kind;
};
void bek_basic_format(bek::OutputStream&, const PartitionInfo&);

class PartitionProxyDevice final : public BlockDevice {
public:
    PartitionProxyDevice(BlockDevice& device, u32 index, u32 global_id, uSize sec_off,
                         uSize sec_count);
    uSize logical_block_size() const override;
    bool is_read_only() const override;
    uSize capacity() const override;
    TransferResult schedule_read(uSize sector_index, bek::mut_buffer buffer,
                                 TransferCallback cb) override;
    TransferResult schedule_write(uSize sector_index, bek::buffer buffer,
                                  TransferCallback cb) override;

private:
    BlockDevice& m_block_device;
    uSize m_sec_off;
    uSize m_sec_count;
};

bool probe_block_device(BlockDevice& device, bek::function<void(bek::vector<PartitionInfo>)> cb);

}  // namespace blk

#endif //BEKOS_PARTITION_H
