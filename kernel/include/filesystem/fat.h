// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2024-2025 Bekos Contributors
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
#ifndef BEKOS_FAT_H
#define BEKOS_FAT_H

#include "bek/str.h"
#include "bek/vector.h"
#include "block_device.h"
#include "library/block_cache.h"
#include <bek/intrusive_shared_ptr.h>
#include "library/kernel_error.h"
#include "library/lru_cache.h"
#include "library/transactional_buffer.h"
#include "partition.h"

enum class FatType { FAT12, FAT16, FAT32, ExFAT };

struct FATInfo {
    FatType fat_type;
    u16 sector_size;  // In bytes
    u32 sectors_per_cluster;
    u32 fat_begin_sector;
    u32 fat_sectors;
    union RootAttrs {
        /// Valid only for FAT 32
        u32 root_dir_cluster;
        /// Valid only for FAT 16
        struct {
            u16 start_sector;
            u16 entry_count;
        } root_dir_attrs_16;

    } root_info;
    /// Where Cluster 2 Begins.
    u32 data_begin_sector;
};

struct FATEntryLocation {
    u32 directory_start_cluster;
    u32 index_in_directory;
};

struct RawFATLFNEntry;
struct RawFATItemEntry;

struct BasicFATEntry {
    bek::string name;
    u64 creation_timestamp;
    u64 accessed_timestamp;
    u64 modified_timestamp;
    u32 size;
    u32 data_cluster;
    u8 raw_attributes;
    bool is_directory() const { return raw_attributes & 0x10; }
    bool is_read_only() const { return raw_attributes & 0x01; }
    bool is_hidden() const { return raw_attributes & 0x02; }
};

struct LocatedFATEntry {
    BasicFATEntry entry;
    FATEntryLocation location;
};

class FATEntryIterator {
private:
    LocatedFATEntry m_current;
};

enum class FATEntryKind { Root, RootMember, Normal };

class FileAllocationTable {
public:
    FileAllocationTable(FATInfo info, blk::BlockDevice& device);

    unsigned int getNextCluster(unsigned int current_cluster);

    unsigned int allocateNextCluster(unsigned int current_cluster);

    bool doDataInterchange(TransactionalBuffer& buffer, unsigned int start_cluster, uSize offset, uSize size,
                           bool write);

    bool extendFile(unsigned int start_cluster, uSize size);

    bek::vector<LocatedFATEntry> get_entries(u32 start_cluster);
    bek::vector<LocatedFATEntry> get_root_entries();

    expected<BasicFATEntry> get_entry(FATEntryLocation location);
    expected<bool> delete_entry(FATEntryLocation location);
    expected<FATEntryLocation> create_entry(BasicFATEntry entry, u32 directory_start_cluster);
    expected<FATEntryLocation> update_entry(FATEntryLocation location, const BasicFATEntry& entry,
                                            bool update_name = false);

    uSize cluster_size() const { return m_cluster_sectors * m_info.sector_size; }

private:
    bek::shared_ptr<BlockCacheItem> fetch_fat_sector(u32 sector_n);
    bek::shared_ptr<BlockCacheItem> fetch_cluster(u32 cluster, bool needs_content);

    void purge_cluster(u32 cluster_n, bek::shared_ptr<BlockCacheItem> c);
    void purge_fat_sector(uSize fat_sector, bek::shared_ptr<BlockCacheItem> sector);

    // bek::spinlock m_lock;
    LRUCache<u32, BlockCacheItem> m_cluster_cache;
    LRUCache<u32, BlockCacheItem> m_fat_cache;
    FATInfo m_info;
    blk::BlockDevice& m_device;
    uSize m_cluster_sectors;
    unsigned int m_free_cluster_hint{0};
};

#endif  // BEKOS_FAT_H
