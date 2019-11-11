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
#include <stdint.h>
#include <utils.h>
#include "filesystem/fat.h"

void file_allocation_table::init_from_bpb(void* buf, size_t size) {
    uint8_t* b_arr = reinterpret_cast<uint8_t*>(buf);
    unsigned short bytes_per_sector = read_u16_LE(buf, 0x0B);
    unsigned char sec_per_clus = b_arr[0xD];
    unsigned short reserved_sector_count = read_u16_LE(buf, 0x0E);
    unsigned char num_fats = b_arr[0x10];
    unsigned int sectors_per_fat = read_u32_LE(buf, 0x24);
    unsigned int root_first_cluster = read_u32_LE(buf, 0x2C);
    fat_begin_lba = reserved_sector_count;
    cluster_begin_lba = reserved_sector_count + (num_fats*sectors_per_fat);
    sectors_per_cluster = sec_per_clus;
    root_dir_first_cluster = root_first_cluster;
}

void file_allocation_table::init() {
    m_partition->read(0, m_buffer, 512);
    init_from_bpb(m_buffer, 512);
}

unsigned int file_allocation_table::get_next_cluster(unsigned int current_cluster) {
    // Four bytes for each entry
    unsigned int entries_per_sector = m_partition->get_sector_size() / 4;
    unsigned int nFATSector = current_cluster / entries_per_sector;
    unsigned int sector_offset = current_cluster % entries_per_sector;
    // Add the offset to first sector in FAT
    unsigned int nSector = fat_begin_lba + nFATSector;
    m_partition->read(nSector * m_partition->get_sector_size(), m_buffer, m_partition->get_sector_size());
    return read_u32_LE(m_buffer, sector_offset * 4);
}

void* file_allocation_table::fetch_sector(unsigned int cluster, unsigned int sector) {
    unsigned int nSector = sector + (cluster - 2) * sectors_per_cluster + cluster_begin_lba;
    m_partition->read(nSector * m_partition->get_sector_size(), m_buffer, m_partition->get_sector_size());
    return m_buffer;
}

file_allocation_table::file_allocation_table(partition* mPartition) : m_partition(mPartition) {}

