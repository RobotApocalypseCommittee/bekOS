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

#include "filesystem/partition.h"

#include "bek/format.h"
#include "filesystem/mbr.h"
#include "library/byte_format.h"

using namespace blk;

bek::string create_partition_string(bek::str_view name, u32 index) {
    return bek::format("{}.{}"_sv, name, index);
}

PartitionProxyDevice::PartitionProxyDevice(BlockDevice& device, u32 index, u32 global_id,
                                           uSize sec_off, uSize sec_count)
    : BlockDevice(create_partition_string(device.name(), index), global_id),
      m_block_device(device),
      m_sec_off(sec_off),
      m_sec_count(sec_count) {}
uSize PartitionProxyDevice::logical_block_size() const {
    return m_block_device.logical_block_size();
}
bool PartitionProxyDevice::is_read_only() const { return m_block_device.is_read_only(); }
uSize PartitionProxyDevice::capacity() const { return m_sec_count; }
TransferResult PartitionProxyDevice::schedule_read(uSize byte_offset, bek::mut_buffer buffer, TransferCallback cb) {
    if ((byte_offset + buffer.size()) > m_sec_count * SECTOR_SIZE) {
        return TransferResult::OutOfBounds;
    }
    return m_block_device.schedule_read(byte_offset + m_sec_off * SECTOR_SIZE, buffer, bek::move(cb));
}
TransferResult PartitionProxyDevice::schedule_write(uSize byte_offset, bek::buffer buffer, TransferCallback cb) {
    if ((byte_offset + buffer.size()) > m_sec_count * SECTOR_SIZE) {
        return TransferResult::OutOfBounds;
    }
    return m_block_device.schedule_write(byte_offset + m_sec_off * SECTOR_SIZE, buffer, bek::move(cb));
}

constexpr inline bek::str_view name_filesystem_kind(PartitionFsKind kind) {
    switch (kind) {
        case PartitionFsKind::None:
            return "none"_sv;
        case PartitionFsKind::Undetermined:
            return "undetermined"_sv;
        case PartitionFsKind::Unrecognised:
            return "unrecognised"_sv;
        case PartitionFsKind::Fat:
            return "FAT"_sv;
    }
    ASSERT_UNREACHABLE();
}
void blk::bek_basic_format(bek::OutputStream& out, const PartitionInfo& info) {
    bek::format_to(out, "{} partition, sector {}, size {} sectors ({})."_sv, name_filesystem_kind(info.kind),
                   info.sector_index, info.size_sectors, bek::ByteSize{info.size_sectors * SECTOR_SIZE});
}

bool blk::probe_block_device(BlockDevice& device,
                             bek::function<void(bek::vector<PartitionInfo>)> cb) {
    // FIXME: Manual Memory Allocation!
    bek::mut_buffer buffer{(char*)kmalloc(SECTOR_SIZE), SECTOR_SIZE};
    return device.schedule_read(
               0, buffer,
               blk::TransferCallback([cb = bek::move(cb), buffer](blk::TransferResult res) mutable {
                   bek::vector<PartitionInfo> partitions{};
                   if (res != TransferResult::Success) {
                       kfree(buffer.data(), SECTOR_SIZE);
                       cb(bek::move(partitions));
                   } else {
                       for (int i = 0; i < 4; i++) {
                           auto& mbr_raw = buffer.get_at<RawMbrPartition>(
                               MBR_OFFSET + i * sizeof(RawMbrPartition));
                           if (mbr_raw.sector_count) {
                               partitions.push_back(
                                   PartitionInfo{mbr_raw.lba_begin, mbr_raw.sector_count,
                                                 kind_from_code(mbr_raw.type_code)});
                           }
                       }
                       kfree(buffer.data(), SECTOR_SIZE);
                       cb(bek::move(partitions));
                   }
               })) == TransferResult::Success;
}
