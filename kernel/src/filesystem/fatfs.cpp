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


#include "filesystem/fatfs.h"
#include <library/assert.h>
#include "filesystem/entrycache.h"


vector<FilesystemEntryRef> FATFilesystemDirectory::enumerate() {
    FATEntry entry;
    u32 cluster_n = m_root_cluster;
    u32 entry_n = 0;
    vector<FilesystemEntryRef> result;
    while (getNextEntry(&entry, cluster_n, entry_n)) {
        FATFilesystemEntry* new_entry;
        if (entry.type == FATEntry::FILE) {
            new_entry = new FATFilesystemFile(entry);
        } else if (entry.type == FATEntry::DIRECTORY) {
            new_entry = new FATFilesystemDirectory(entry);
        } else {
            continue;
        }


        new_entry->setFilesystem(filesystem, table);
        new_entry->setParent(FilesystemEntryRef(this));
        auto old_ref = filesystem->entryCache->search(FilesystemEntryRef(this), new_entry->m_name);
        if (old_ref.empty()) {
            auto new_ref = AcquirableRef<FATFilesystemEntry>(new_entry);
            filesystem->entryCache->insert(new_ref);
            result.push_back(new_ref);
        } else {
            // Already loaded
            result.push_back(old_ref);
        }
    }
    return result;
}

FilesystemEntryRef FATFilesystemDirectory::lookup(const char* name) {
    auto res = filesystem->entryCache->search(FilesystemEntryRef(this), name);
    if (res.empty()) {
        FATEntry entry;
        u32 cluster_n = m_root_cluster;
        u32 entry_n = 0;
        while (getNextEntry(&entry, cluster_n, entry_n)) {
            if (entry.type == FATEntry::FILE || entry.type == FATEntry::DIRECTORY) {
                if (strcmp(entry.name, name) == 0) {
                    FATFilesystemEntry* new_entry;
                    if (entry.type == FATEntry::FILE) {
                        new_entry = new FATFilesystemFile(entry);
                    } else if (entry.type == FATEntry::DIRECTORY) {
                        new_entry = new FATFilesystemDirectory(entry);
                    } else {
                        continue;
                    }
                    new_entry->setParent(FilesystemEntryRef(this));
                    new_entry->setFilesystem(filesystem, table);
                    auto new_ref = FilesystemEntryRef(new_entry);
                    filesystem->entryCache->insert(new_ref);
                    return new_ref;
                }
            }
        }
        // Found nothing
        return FilesystemEntryRef();
    }
    return res;
}

bool FATFilesystemDirectory::getNextEntry(FATEntry* nextEntry, u32 &cluster_n, u32 &entry_n) {
    const unsigned entries_per_cluster = table->getClusterSectors() * FAT_DIR_ENTRIES_IN_SECTOR;
    if (cluster_n >= 0xFFFFFFF8 || cluster_n == 1 || cluster_n == 0) {
        // Noo valid cluster
        return false;
    }
    unsigned nSector = (entry_n % entries_per_cluster) / FAT_DIR_ENTRIES_IN_SECTOR;
    auto* listing_sector = reinterpret_cast<HardFATEntry*>(table->fetchSector(cluster_n, nSector));
    unsigned nOffset = entry_n % FAT_DIR_ENTRIES_IN_SECTOR;
    auto entry = listing_sector[nOffset];
    // Now check entry
    if (entry.fatname[0] == '\0') {
        nextEntry->type = FATEntry::END;
        return false;
    } else if ((entry.attrib & 0b1111) == 0b1111) {
        // Long File name -> ignore for now
        nextEntry->type = FATEntry::UNKNOWN;
    } else if (entry.fatname[0] == 0xE5) {
        // Free
        nextEntry->type = FATEntry::FREE;
    } else {
        // Valid file
        fatname_to_fname(entry.fatname, nextEntry->name);
        nextEntry->attrib = entry.attrib;
        nextEntry->size = entry.size;
        nextEntry->start_cluster = (entry.cluster_high << 16) | entry.cluster_low;
        nextEntry->type = entry.attrib & (1 << 4) ? FATEntry::DIRECTORY : FATEntry::FILE;
        nextEntry->source_cluster = cluster_n;
        nextEntry->source_cluster_entry = entry_n % entries_per_cluster;
    }
    // Now increment indexes
    entry_n++;
    if (entry_n % (entries_per_cluster) == 0) {
        cluster_n = table->getNextCluster(cluster_n);
    }
    return true;
}

void FATFilesystemEntry::setFilesystem(FATFilesystem* pFilesystem, FileAllocationTable* pTable) {
    filesystem = pFilesystem;
    table = pTable;
}

FATFilesystemEntry::FATFilesystemEntry(const FATEntry& entry): m_root_cluster(entry.start_cluster),
m_source_cluster(entry.source_cluster),
m_source_index(entry.source_cluster_entry)
{
    size = entry.size;
    m_name = new char[strlen(entry.name) + 1];
    strcpy(m_name, entry.name);
    // Others to follow
}

vector<FilesystemEntryRef> FATFilesystemFile::enumerate() {
    assert(0);
    return vector<FilesystemEntryRef>();
}

FilesystemEntryRef FATFilesystemFile::lookup(const char* name) {
    assert(0);
    return FilesystemEntryRef();
}

FilesystemEntryRef FATFilesystem::getInfo(char* path) {
    // TODO: DO
    return FilesystemEntryRef();
}

FilesystemEntryRef FATFilesystem::getRootInfo() {
    if (root_directory.empty()) {
        FATEntry root_entry;
        strcpy(root_entry.name, "root");
        root_entry.start_cluster = fat.getRootCluster();
        root_entry.source_cluster_entry = root_entry.source_cluster = 0;

        root_directory = AcquirableRef<FATFilesystemDirectory>(new FATFilesystemDirectory(root_entry));
        root_directory->setFilesystem(this, &fat);
        entryCache->insert(root_directory);
    }
    return root_directory;
}

File* FATFilesystem::open(FilesystemEntry* entry) {
    // TDOD: DO
    return nullptr;
}

FATFilesystem::FATFilesystem(partition* partition, EntryHashtable* entryCache): Filesystem(entryCache), fat(partition) {
    fat.init();
}
