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
#include <kstring.h>
#include "filesystem/fat.h"

bool is_valid_fat_char(char c) {
    return ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') ||
    (strchr(" !#$%&'()-@^_`{}~", c) != nullptr) || (128 <= c && c <= 228) ||
    (230 <= c && c <= 255);
}
bool fname_to_fat(char* fname, char* fatname) {

}

struct Hard_FAT_Entry {
    char fatname[11];
    uint8_t attrib;
    uint64_t pad1;
    uint16_t cluster_high;
    uint32_t pad2;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((__packed__));


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
            end_pos = i+1;
        }
    }
    strncpy(fname, fatname, 8);
    fname[end_pos] = '\0';
    if (has_extension && fatname[8] != '\0' && fatname[8] != ' ') {
        strcat(fname, ".");
        for (int i = 8; i < 11; i++) {
            if (fatname[i] == '\0') {
                end_pos = i;
                break;
            } else if (fatname[i] != ' ') {
                end_pos = i+1;
            }
        }
        fatname[end_pos] = '\0';
        strncat(fname, &fatname[8], 3);
    }
    return true;
}

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

unsigned int file_allocation_table::get_cluster_sectors() {
    return sectors_per_cluster;
}

FATDirectory file_allocation_table::getRootDirectory() {
    return {static_cast<unsigned int>(root_dir_first_cluster), this};
}

void FATDirectory::seekBeginning() {
    currentCluster = rootCluster;
    currentEntry = 0;
}

bool FATDirectory::getNextEntry(FAT_Entry* nextEntry) {
    if (currentCluster >= 0xFFFFFFF8) {
        return false;
    }
    bool validEntryFound = false;
    while (!validEntryFound) {
        unsigned nSector = (currentEntry % table->get_cluster_sectors()) / FAT_ENTRIES_PER_SECTOR;
        auto* listing_sector = reinterpret_cast<Hard_FAT_Entry*>(table->fetch_sector(currentCluster, nSector));
        unsigned nOffset = currentEntry % FAT_ENTRIES_PER_SECTOR;
        auto entry = listing_sector[nOffset];
        if (entry.fatname[0] == '\0') {
            // End of directory
            return false;
        } else if ((entry.attrib & 0b1111) == 0b1111) {
            // Long file name -> ignore (for now)
            // TODO: Implement
        } else if (entry.fatname[0] == 0xE5) {
            // Unused -> skip to next
        } else {
            // Is valid file
            fatname_to_fname(entry.fatname, nextEntry->name);
            nextEntry->attrib = entry.attrib;
            nextEntry->size = entry.size;
            nextEntry->start_cluster = (entry.cluster_high << 16) | entry.cluster_low;
            validEntryFound = true;
        }
        // Increment for next
        currentEntry++;
        if (currentEntry % (table->get_cluster_sectors() * FAT_ENTRIES_PER_SECTOR) == 0) {
            currentCluster = table->get_next_cluster(currentCluster);
        }
    }
    return true;
}

FATDirectory::FATDirectory(unsigned int rootCluster, file_allocation_table* table) : rootCluster(rootCluster),
                                                                                     table(table),
                                                                                     currentCluster(rootCluster),
                                                                                     currentEntry(0) {}

bool FAT_Entry::is_hidden() {
    return attrib & (1 << 1);
}

bool FAT_Entry::is_read_only() {
    return attrib & (1 << 0);
}

bool FAT_Entry::is_directory() {
    return attrib & (1 << 4);
}


