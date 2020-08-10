/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2020  Bekos Inc Ltd
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef BEKOS_FAT_H
#define BEKOS_FAT_H

#include <stddef.h>
#include <stdint.h>
#include <hardtypes.h>
#include <library/optional.h>
#include "device.h"
#include "partition.h"

struct FATInfo {
    u16 sector_size; // In bytes
    u32 sectors_per_cluster;
    u32 fat_begin_sector;
    u32 fat_sectors;
    u32 clusters_begin_sector;
    u32 root_begin_cluster;
};

struct HardFATEntry {
    char fatname[11];
    uint8_t attrib;
    uint64_t pad1;
    uint16_t cluster_high;
    uint32_t pad2;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((__packed__));

struct FATEntry {
    char name[13];
    enum FATENTRYType {
        FREE,
        FILE,
        DIRECTORY,
        END,
        UNKNOWN
    } type;
    uint8_t attrib;
    unsigned int size;
    unsigned int start_cluster;
    u32 source_cluster;
    u32 source_cluster_entry;

    bool is_hidden();

    bool is_read_only();

    bool is_directory();

};

bool fname_to_fat(const char* fname, char* fatname);
bool fatname_to_fname(char* fatname, char* fname);

class FileAllocationTable {
public:

    FileAllocationTable(FATInfo info, BlockDevice& device);

    unsigned int getNextCluster(unsigned int current_cluster);

    unsigned int allocateNextCluster(unsigned int current_cluster);

    bool readData(void* buf, unsigned int start_cluster, size_t offset, size_t size);

    bool writeData(void* buf, unsigned int start_cluster, size_t offset, size_t size);

    bool extendFile(unsigned int start_cluster, size_t size);
private:

    bool doDataInterchange(u8 *buf, unsigned int start_cluster, size_t offset, size_t size, bool write);

    FATInfo m_info;
    BlockDevice& m_device;
    unsigned int m_free_cluster_hint{0};
};

#endif //BEKOS_FAT_H
