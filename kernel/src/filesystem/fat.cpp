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
#include <stdint.h>
#include <utils.h>
#include <kstring.h>
#include <library/assert.h>
#include "filesystem/fat.h"

bool is_valid_cluster(unsigned int cluster_n) {
    return !(cluster_n >= 0xFFFFFFF8 || cluster_n == 1 || cluster_n == 0);
}

bool is_valid_fat_char(char c) {
    return ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') ||
           (strchr(" !#$%&'()-@^_`{}~", c) != nullptr) || (128 <= c && c <= 228) ||
           (230 <= c);
}

bool fname_to_fat(const char* fname, char* fatname) {
    memset(fatname, ' ', 11);
    const char* fnamePos = fname;
    char* fatnamePos = fatname;
    unsigned curr_len = 8;
    while (curr_len > 0) {
        if (is_valid_fat_char(*fnamePos)) {
            *fatnamePos++ = *fnamePos++;
            curr_len--;
        } else if (*fnamePos == '.') {
            fatnamePos = &fatname[8];
            fnamePos++;
            curr_len = 3;
        } else if (*fnamePos == '\0') {
            return true;
        } else {
            return false;
        }
    }
    return true;
}


bool fatname_to_fname(char* fatname, char* fname) {
    // Starts with dot
    bool has_extension = fatname[0] != 0x2E;
    // Do main name
    int end_pos = 0;
    for (int i = 0; i < 8; i++) {
        if (fatname[i] == '\0') {
            end_pos = i;
            break;
        } else if (fatname[i] != ' ') {
            end_pos = i + 1;
        }
    }
    strncpy(fname, fatname, 8);
    fname[end_pos] = '\0';
    if (has_extension && fatname[8] != '\0' && fatname[8] != ' ') {
        strcat(fname, ".");
        int extension_length = 0;
        for (int i = 8; i < 11; i++) {
            if (fatname[i] == '\0') {
                break;
            } else if (fatname[i] != ' ') {
                extension_length = i + 1 - 8;
            }
        }
        strncat(fname, &fatname[8], extension_length);
    }
    return true;
}

void FileAllocationTable::init_from_bpb(void* buf, size_t size) {
    (void) size;
    uint8_t* b_arr = reinterpret_cast<uint8_t*>(buf);
    unsigned short bytes_per_sector = read_u16_LE(buf, 0x0B);
    unsigned char sec_per_clus = b_arr[0xD];
    unsigned short reserved_sector_count = read_u16_LE(buf, 0x0E);
    unsigned char num_fats = b_arr[0x10];
    unsigned int sectors_per_fat = read_u32_LE(buf, 0x24);
    unsigned int root_first_cluster = read_u32_LE(buf, 0x2C);
    fat_begin_lba = reserved_sector_count;
    cluster_begin_lba = reserved_sector_count + (num_fats * sectors_per_fat);
    sectors_per_cluster = sec_per_clus;
    root_dir_first_cluster = root_first_cluster;
}

void FileAllocationTable::init() {
    m_partition->read(0, m_buffer, 512);
    init_from_bpb(m_buffer, 512);
}

unsigned int FileAllocationTable::getNextCluster(unsigned int current_cluster) {
    // Four bytes for each entry
    unsigned int entries_per_sector = m_partition->get_sector_size() / 4;
    unsigned int nFATSector = current_cluster / entries_per_sector;
    unsigned int sector_offset = current_cluster % entries_per_sector;
    // Add the offset to first sector in FAT
    unsigned int nSector = fat_begin_lba + nFATSector;
    m_partition->read(nSector * m_partition->get_sector_size(), m_buffer, m_partition->get_sector_size());
    return read_u32_LE(m_buffer, sector_offset * 4);
}

void* FileAllocationTable::fetchSector(unsigned int cluster, unsigned int sector) {
    unsigned int nSector = sector + (cluster - 2) * sectors_per_cluster + cluster_begin_lba;
    m_partition->read(nSector * m_partition->get_sector_size(), m_buffer, m_partition->get_sector_size());
    return m_buffer;
}

FileAllocationTable::FileAllocationTable(partition* mPartition) : m_partition(mPartition) {}

unsigned int FileAllocationTable::getClusterSectors() {
    return sectors_per_cluster;
}

u32 FileAllocationTable::getRootCluster() {
    return root_dir_first_cluster;
}

bool FileAllocationTable::readData(void* buf, unsigned int start_cluster, size_t offset, size_t size) {
    return doDataInterchange(buf, start_cluster, offset, size, false);
}

bool FileAllocationTable::writeData(void* buf, unsigned int start_cluster, size_t offset, size_t size) {
    return doDataInterchange(buf, start_cluster, offset, size, true);
}

bool
FileAllocationTable::doDataInterchange(void* buf, unsigned int start_cluster, size_t offset, size_t size, bool write) {
    // Find start cluster
    unsigned int sectorSize = m_partition->get_sector_size();

    unsigned int cluster_offset = offset / (sectors_per_cluster * sectorSize);
    unsigned int sector_offset = (offset / sectorSize) % sectors_per_cluster;
    unsigned int byte_offset = offset % sectorSize;
    unsigned int current_cluster = start_cluster;

    for (unsigned int c = 0; c < cluster_offset; c++) {
        current_cluster = getNextCluster(current_cluster);
        if (!is_valid_cluster(current_cluster)) {
            return false;
        }
    }

    size_t completed_size = 0;
    while (completed_size < size) {
        // Read to end of sector
        unsigned int len_to_copy = size - completed_size > sectorSize - byte_offset ? sectorSize - byte_offset: size - completed_size;
        if (write) {
            // Buffer -> sector
            // First check whether full or partial sector
            if (len_to_copy == sectorSize) {
                // Full sector - dont worry about other data
                if (!writeSector(current_cluster, sector_offset, static_cast<char*>(buf) + completed_size)) {
                    return false;
                }
            } else {
                // Have to read, copy, and write
                char* sector = static_cast<char*>(fetchSector(current_cluster, sector_offset));
                memcpy(sector + byte_offset, static_cast<char*>(buf) + completed_size, len_to_copy);
                if (!writeSector(current_cluster, sector_offset, sector)) {
                    return false;
                }
            }
        } else {
            // sector -> buffer
            char* sector = static_cast<char*>(fetchSector(current_cluster, sector_offset));
            memcpy(static_cast<char*>(buf) + completed_size, sector + byte_offset, len_to_copy);
        }
        completed_size += len_to_copy;
        // All subsequent reads
        byte_offset = 0;

        sector_offset++;
        if (sector_offset % sectors_per_cluster == 0) {
            current_cluster = getNextCluster(current_cluster);
            if (!is_valid_cluster(current_cluster)) {
                // TODO: Extend files when writing beyond end.
                return false;
            }
            sector_offset = 0;
        }
    }
    return true;
}

bool FileAllocationTable::writeSector(unsigned int cluster, unsigned int sector, void* buf) {
    unsigned int nSector = sector + (cluster - 2) * sectors_per_cluster + cluster_begin_lba;
    m_partition->write(nSector * m_partition->get_sector_size(), buf, m_partition->get_sector_size());
    return true;
}


bool FATEntry::is_hidden() {
    return attrib & (1 << 1);
}

bool FATEntry::is_read_only() {
    return attrib & (1 << 0);
}

bool FATEntry::is_directory() {
    return attrib & (1 << 4);
}


