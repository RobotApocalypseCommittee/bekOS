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

enum class ClusterType {
    NextPointer,
    EndOfChain,
    Free,
    Corrupt,
    Reserved
};

const unsigned int CLUSTER_EOC = 0x0FFFFFFF;
const unsigned int METADATA_MASK = 0xF0000000;


ClusterType get_cluster_type(unsigned int cluster_n) {
    cluster_n = cluster_n & 0x0FFFFFFFu;
    if (cluster_n <= 0xFFFFFEF && cluster_n > 1) {
        return ClusterType::NextPointer;
    } else if (cluster_n == 0) {
        return ClusterType::Free;
    } else if (cluster_n >= 0xFFFFFF8) {
        return ClusterType::EndOfChain;
    } else if (cluster_n == 0xFFFFFF7) {
        return ClusterType::Corrupt;
    } else {
        return ClusterType::Reserved;
    }
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


FileAllocationTable::FileAllocationTable(FATInfo info, BlockDevice &device): m_info(info), m_device(device) {
    // TODO: Is this even necessary?
    assert(info.sector_size == device.block_size());
}

unsigned int FileAllocationTable::getNextCluster(unsigned int current_cluster) {
    // Four bytes for each entry
    unsigned int entries_per_sector = m_device.block_size() / 4;
    u8 buffer[4];
    // Read int
    m_device.readBytes(m_info.fat_begin_sector * m_info.sector_size + current_cluster * 4, buffer, 4);
    return bek::readLE<u32>(&buffer[0]);
}

unsigned int FileAllocationTable::allocateNextCluster(unsigned int current_cluster) {
    // TODO: This is stupid, and involves lots of extra copying - do something less stupid
    // Which will require an extra fiddling on CachedDevice
    auto buf = bek::vector<u8>(m_info.sector_size);

    unsigned int first_checked_entry = bek::min(2u, m_free_cluster_hint);
    // ints of length 4 bytes
    unsigned int entries_per_sector = m_info.sector_size / 4;
    unsigned int first_checked_sector = first_checked_entry / entries_per_sector;
    unsigned int first_offset = first_checked_entry % entries_per_sector;

    for (unsigned int i = first_checked_sector; i < m_info.fat_sectors; i++) {
        m_device.readBlock(i + m_info.fat_begin_sector, buf.data(), 0, m_info.sector_size);
        unsigned int j = (i == first_checked_sector) ? first_offset : 0;
        for (; j < m_device.block_size(); j += 4) {
            auto cluster_status = bek::readLE<u32>(&buf.data()[j]);
            if (get_cluster_type(cluster_status) != ClusterType::Free) continue;
            auto new_cluster_num = i * entries_per_sector + (j / 4);

            u8 s_buffer[4]; // LE
            bek::writeLE<u32>((cluster_status & METADATA_MASK) | CLUSTER_EOC, s_buffer);
            m_device.writeBytes((m_info.fat_begin_sector + i) * m_device.block_size() + j, s_buffer, 4);

            m_free_cluster_hint = new_cluster_num + 1;

            u64 byte_index = m_info.fat_begin_sector * m_info.sector_size + current_cluster * 4;
            m_device.readBytes(byte_index, s_buffer, 4);
            auto curr_cluster_status = bek::readLE<u32>(s_buffer);
            bek::writeLE<u32>((curr_cluster_status & METADATA_MASK) | new_cluster_num, s_buffer);
            m_device.writeBytes(byte_index, s_buffer, 4);
            return new_cluster_num;
        }
    }
    // Error
    return 0;
}

bool FileAllocationTable::readData(void* buf, unsigned int start_cluster, size_t offset, size_t size) {
    return doDataInterchange(buf, start_cluster, offset, size, false);
}

bool FileAllocationTable::writeData(void* buf, unsigned int start_cluster, size_t offset, size_t size) {
    return doDataInterchange(buf, start_cluster, offset, size, true);
}

bool
FileAllocationTable::doDataInterchange(u8 *buf, unsigned int start_cluster, size_t offset, size_t size, bool write) {
    // Find start cluster
    const unsigned int cluster_size = m_info.sectors_per_cluster * m_info.sector_size;
    unsigned int cluster_n = offset / cluster_size;
    unsigned int first_byte_offset = offset % cluster_size;
    unsigned int current_cluster = start_cluster;

    for (unsigned int c = 0; c < cluster_n; c++) {
        current_cluster = getNextCluster(current_cluster);
        if (get_cluster_type(current_cluster) != ClusterType::NextPointer) {
            // TODO: Extend?
            return false;
        }
    }

    size_t completed_size = 0;
    while (completed_size < size) {
        // Read to end of sector
        unsigned int byte_offset = (completed_size == 0) ? first_byte_offset : 0;
        unsigned int len_to_copy = bek::min<unsigned long>(size - completed_size, cluster_size - byte_offset);
        if (write) {
            m_device.writeBytes(m_info.clusters_begin_sector * m_info.sector_size + current_cluster * cluster_size + byte_offset, buf, len_to_copy);
        } else {
            m_device.readBytes(m_info.clusters_begin_sector * m_info.sector_size + current_cluster * cluster_size + byte_offset, buf, len_to_copy);
        }
        completed_size += len_to_copy;
        buf += len_to_copy;


        // Next cluster
        current_cluster = getNextCluster(current_cluster);
        if (get_cluster_type(current_cluster) != ClusterType::NextPointer) {
            // TODO: Extend?
            return false;
        }
    }
    return true;
}

bool FileAllocationTable::extendFile(unsigned int start_cluster, size_t size) {
    size_t bytes_per_cluster = m_info.sector_size * m_info.sectors_per_cluster;
    size_t current_size = bytes_per_cluster;
    unsigned int current_cluster = start_cluster;
    unsigned int last_cluster;
    while (current_size < size) {
        last_cluster = current_cluster;
        current_cluster = getNextCluster(current_cluster);
        if (get_cluster_type(current_cluster) != ClusterType::NextPointer) {
            current_cluster = allocateNextCluster(last_cluster);
            if (current_cluster == 0) return false;
        }
        current_size += bytes_per_cluster;
    }
    return true;
}


bool FATEntry::is_hidden() {
    return attrib & (1u << 1);
}

bool FATEntry::is_read_only() {
    return attrib & (1u << 0);
}

bool FATEntry::is_directory() {
    return attrib & (1u << 4);
}


