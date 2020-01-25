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
#include "hdevice.h"
#include "partition.h"


#define FAT_SECTOR_SIZE 512
#define FAT_DIR_ENTRY_SIZE 32
#define FAT_DIR_ENTRIES_IN_SECTOR (FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE)

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
    FileAllocationTable(partition* mPartition);

    void init();

    unsigned int getNextCluster(unsigned int current_cluster);

    void* fetchSector(unsigned int cluster, unsigned int sector);
    bool writeSector(unsigned int cluster, unsigned int sector, void* buf);

    unsigned int getClusterSectors();

    bool readData(void* buf, unsigned int start_cluster, size_t offset, size_t size);

    bool writeData(void* buf, unsigned int start_cluster, size_t offset, size_t size);

    u32 getRootCluster();
private:

    void init_from_bpb(void* buf, size_t size);

    bool doDataInterchange(void* buf, unsigned int start_cluster, size_t offset, size_t size, bool write);

    unsigned long fat_begin_lba;
    unsigned long cluster_begin_lba;
    unsigned long sectors_per_cluster;
    unsigned long root_dir_first_cluster;
    uint8_t m_buffer[512];
    hdevice* m_partition;
};

#endif //BEKOS_FAT_H
