/*
 *   bekOS is a basic OS for the Raspberry Pi
 *
 *   Copyright (C) 2019  Bekos Inc Ltd
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
#include "hdevice.h"
#include "partition.h"

#define FAT_ENTRIES_PER_SECTOR 16
class FAT_Entry {
public:
    char name[13];
    uint8_t attrib;
    unsigned int size;
    unsigned int start_cluster;
    bool is_hidden();
    bool is_read_only();
    bool is_directory();

};

class file_allocation_table;

class FATDirectory {
public:
    FATDirectory(unsigned int rootCluster, file_allocation_table* table);

    void seekBeginning();
    bool getNextEntry(FAT_Entry* nextEntry);

private:
    unsigned currentEntry;
    unsigned currentCluster;

    unsigned rootCluster;
    file_allocation_table* table;
};

class file_allocation_table {
public:
    file_allocation_table(partition* mPartition);

    void init();

    unsigned int get_next_cluster(unsigned int current_cluster);
    void* fetch_sector(unsigned int cluster, unsigned int sector);
    unsigned int get_cluster_sectors();
    FATDirectory getRootDirectory();
private:

    void init_from_bpb(void* buf, size_t size);
    unsigned long fat_begin_lba;
    unsigned long cluster_begin_lba;
    unsigned long sectors_per_cluster;
    unsigned long root_dir_first_cluster;
    uint8_t m_buffer[512];
    hdevice* m_partition;
};
#endif //BEKOS_FAT_H
