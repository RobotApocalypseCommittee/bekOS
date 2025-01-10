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
#include "filesystem/fat.h"

#include "bek/assertions.h"
#include "bek/format.h"
#include "bek/time.h"
#include "library/debug.h"

using DBG = DebugScope<"FAT", DebugLevel::WARN>;

enum class ClusterType { NextPointer, EndOfChain, Free, Corrupt, Reserved };

enum class EntryType { Normal, EndOfDirectory, Deleted, LongFileName };

const unsigned int CLUSTER_EOC = 0x0FFFFFFF;
const unsigned int METADATA_MASK = 0xF0000000;

struct short_name {
    char s[11];
};

static_assert(sizeof(short_name) == 11);

struct [[gnu::packed]] RawFATItemEntry {
    short_name fatname{};
    u8 attrib{};
    [[maybe_unused]] u16 _reserved1{};
    u16 create_time{};
    u16 create_date{};
    u16 access_date{};

    u16 cluster_high{};
    u16 modify_time{};
    u16 modify_date{};
    u16 cluster_low{};
    u32 size{};

    static RawFATItemEntry from(short_name fatname, const BasicFATEntry& e) {
        auto creation_time = UnixTimestamp(e.creation_timestamp).decompose();
        auto modified_time = UnixTimestamp(e.modified_timestamp).decompose();
        // auto accessed_time = UnixTimestamp(e.accessed_timestamp).decompose();

        return RawFATItemEntry{fatname,
                               e.raw_attributes,
                               0,
                               dos_time_from(creation_time),
                               dos_date_from(creation_time),
                               dos_date_from(modified_time),
                               static_cast<u16>(e.data_cluster >> 16),
                               dos_time_from(modified_time),
                               dos_date_from(modified_time),
                               static_cast<u16>(e.data_cluster),
                               e.size};
    }
};
static_assert(sizeof(RawFATItemEntry) == 32);

struct [[gnu::packed]] RawFATLFNEntry {
    u8 order{};
    u16 first_chars[5]{};
    [[maybe_unused]] u8 attrib{0x0F};
    u8 entry_type{};
    u8 checksum{};
    u16 next_chars[6]{};
    [[maybe_unused]] u16 _reserved0{};
    u16 final_chars[2]{};
};
static_assert(sizeof(RawFATLFNEntry) == 32);

union [[gnu::packed]] RawFATEntry {
    u8 data[32]{};
    RawFATLFNEntry lfn_entry;
    RawFATItemEntry item_entry;

    EntryType type() {
        if (data[0] == 0) {
            return EntryType::EndOfDirectory;
        } else if (data[0] == 0xE5) {
            return EntryType::Deleted;
        } else if ((data[11] & 0x0F) == 0x0F) {
            return EntryType::LongFileName;
        } else {
            return EntryType::Normal;
        }
    }
};

static_assert(sizeof(RawFATEntry) == 32);

bool is_valid_fat_char(char c) {
    if (('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (128 <= c && c <= 228) || (230 <= c)) return true;
    switch (c) {
        case ' ':
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '-':
        case '`':
        case '{':
        case '}':
        case '~':
            return true;
        default:
            return false;
    }
}

expected<short_name> generate_short_name(bek::str_view full_name, bek::vector<short_name> short_names) {
    VERIFY(full_name.size() != 0);
    short_name r{};
    bek::memset(r.s, ' ', 11);
    // Special File Names
    if (full_name == "."_sv) {
        r.s[0] = '.';
        return r;
    } else if (full_name == ".."_sv) {
        r.s[0] = '.';
        r.s[1] = '.';
        return r;
    }
    while (full_name.size() && (full_name[0] == '.' || full_name[0] == ' ')) {
        full_name = full_name.substr(1, -1);
    }
    int total_chars = 0;
    while (full_name.size() && full_name[0] != '.' && total_chars < 8) {
        char c = full_name[0];
        if (c == ' ') {
            // Simply Ignore.
        } else if ('a' <= c && c <= 'z') {
            // Convert to uppercase
            r.s[total_chars] = c - 32;
            total_chars++;
        } else if (is_valid_fat_char(c)) {
            r.s[total_chars] = c;
            total_chars++;
        } else {
            r.s[total_chars] = '_';
            total_chars++;
        }
        full_name = full_name.substr(1, -1);
    }

    int base_len = total_chars;

    int last_dot_pos = -1;
    for (unsigned i = 0; i < full_name.size(); i++) {
        if (full_name[i] == '.') last_dot_pos = i;
    }
    if (last_dot_pos != -1 && static_cast<unsigned>(last_dot_pos) + 1 < full_name.size()) {
        // There is an extension!
        full_name = full_name.substr(last_dot_pos + 1, -1);
        total_chars = 0;
        while (total_chars < 3 && full_name.size()) {
            char c = full_name[0];
            if (c == ' ') {
                // Simply Ignore.
            } else if ('a' <= c && c <= 'z') {
                // Convert to uppercase
                r.s[total_chars + 8] = c - 32;
                total_chars++;
            } else if (is_valid_fat_char(c)) {
                r.s[total_chars + 8] = c;
                total_chars++;
            } else {
                r.s[total_chars + 8] = '_';
                total_chars++;
            }
            full_name = full_name.substr(1, -1);
        }
    }

    // Numeric Tail Generation
    bool duplicate = false;
    int number = 0;
    for (auto& s : short_names) {
        if (!bek::mem_compare(s.s, r.s, 11)) {
            duplicate = true;
        }
    }
    while (duplicate) {
        number++;
        bek::string n_string = bek::format("~{}"_sv, number);
        u32 insertion_posn = bek::min<u32>(base_len, 8u - n_string.size());
        short_name candidate = r;
        bek::memcopy(&candidate.s[insertion_posn], n_string.data(), n_string.size());

        for (auto& s : short_names) {
            if (!bek::mem_compare(s.s, candidate.s, 11)) {
                continue;
            }
        }

        r = candidate;
        duplicate = false;
    }
    return r;
}

u8 checksum_shortname(const short_name& name) {
    u8 checksum = 0;
    for (char i : name.s) {
        // NOTE: The operation is an unsigned char rotate right
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + i;
    }
    return checksum;
}

bek::string short_name_to_string(short_name name) {
    int main_len = 0;
    while (main_len < 8 && name.s[main_len] != ' ') {
        main_len++;
    }
    int ext_len = 0;
    while (ext_len < 3 && name.s[8 + ext_len] != ' ') {
        ext_len++;
    }
    if (ext_len) {
        return bek::format("{}.{}"_sv, bek::str_view(name.s, main_len), bek::str_view(&name.s[8], ext_len));
    } else {
        return bek::string(bek::str_view(name.s, main_len));
    }
}

struct PackedFATEntry {
    explicit PackedFATEntry(bek::vector<RawFATEntry> entries) : entries{bek::move(entries)} {}
    uSize entry_count() const { return entries.size(); }
    uSize lfn_entries() const { return entry_count() - 1; }
    RawFATItemEntry& entry() { return entries[lfn_entries()].item_entry; }
    RawFATLFNEntry& lfn_entry(uSize i) { return entries[i].lfn_entry; }

    expected<BasicFATEntry> to_basic() {
        bek::string name = extract_name();
        auto& e = entry();
        auto creation = UnixTimestamp::from_decomposed(datetime_from_dos(e.create_date, e.create_time)).seconds();
        auto access = UnixTimestamp::from_decomposed(datetime_from_dos(e.access_date, 0)).seconds();
        auto modified = UnixTimestamp::from_decomposed(datetime_from_dos(e.modify_date, e.modify_time)).seconds();
        auto data_cluster = static_cast<u32>(e.cluster_low) | (static_cast<u32>(e.cluster_high) << 16);
        return BasicFATEntry{bek::move(name), creation, access, modified, e.size, data_cluster, e.attrib};
    }

    bek::vector<RawFATEntry> entries;

private:
    bek::string extract_name() {
        auto x = extract_long_name();
        if (x.has_value()) {
            return x.release_value();
        } else {
            return short_name_to_string(entry().fatname);
        }
    }

    expected<bek::string> extract_long_name() {
        if (!lfn_entries()) return EINVAL;
        auto first = lfn_entry(0);
        if (!(first.order & 0x40)) {
            return EINVAL;
        }
        u8 n = first.order & 0x3F;
        if (n != lfn_entries()) {
            return EINVAL;
        }
        u8 checksum = checksum_shortname(entry().fatname);

        bek::StringStream builder;
        builder.reserve(13 * lfn_entries());
        for (uSize i = 0; i < lfn_entries(); i++) {
            auto e = lfn_entry(i);
            if ((e.order & 0x3F) != (n - i)) {
                return EINVAL;
            }
            if (e.checksum != checksum) {
                return EINVAL;
            }
            u16 first_chars[5]{};
            bek::memcopy(first_chars, e.first_chars, sizeof(first_chars));
            for (auto c : first_chars) {
                if (c != 0) {
                    // TODO: Unicode
                    builder.write(c);
                } else {
                    return builder.build_and_clear();
                }
            }
            u16 next_chars[6]{};
            bek::memcopy(next_chars, e.next_chars, sizeof(next_chars));
            for (auto c : next_chars) {
                if (c != 0) {
                    // TODO: Unicode
                    builder.write(c);
                } else {
                    return builder.build_and_clear();
                }
            }
            u16 final_chars[2]{};
            bek::memcopy(final_chars, e.final_chars, sizeof(final_chars));
            for (auto c : final_chars) {
                if (c != 0) {
                    // TODO: Unicode
                    builder.write(c);
                } else {
                    return builder.build_and_clear();
                }
            }
        }
        // Very Odd - got to end without null terminator...
        return builder.build_and_clear();
    }
};

inline u32 cluster_index(unsigned int cluster) { return cluster & (~METADATA_MASK); }

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

bek::pair<u32, u32> fat_sector_index_and_offset_for(const FATInfo& info, u32 cluster) {
    // FIXME: FAT12
    uSize total_byte_offset = cluster * (info.fat_type == FatType::FAT16 ? 2 : 4);
    u32 sector_i = total_byte_offset / info.sector_size;
    u32 byte_offset = total_byte_offset % info.sector_size;
    return {sector_i, byte_offset};
}

u32 extract_fat_value(const FATInfo& info, u32 byte_offset, const u8* sector) {
    bek::buffer buf{reinterpret_cast<const char*>(sector), info.sector_size};
    if (info.fat_type == FatType::FAT16) {
        u32 x = buf.get_at_le<u16>(byte_offset);
        // Need to convert to fat 32 equivalent.
        if (x < 0xFFF7) {
            return x;
        } else {
            return x | 0x0FFF0000;
        }
    } else {
        return cluster_index(buf.get_at_le<u32>(byte_offset));
    }
}

void set_fat_value(const FATInfo& info, u32 byte_offset, u8* sector, u32 value) {
    bek::mut_buffer buf{reinterpret_cast<char*>(sector), info.sector_size};
    if (info.fat_type == FatType::FAT16) {
        // Need to convert from fat 32 equivalent (truncation works?)
        buf.get_at<u16>(byte_offset) = value;
    } else {
        buf.get_at<u32>(byte_offset) = (buf.get_at<u32>(byte_offset) & METADATA_MASK) | (value & (~METADATA_MASK));
    }
}

u32 get_fat_entry_count(const FATInfo& info) {
    // FIXME: FAT12
    return (info.fat_sectors * info.sector_size) / (info.fat_type == FatType::FAT16 ? 2 : 4);
}

struct FATRawEntryList {
    struct FATEntryIterator {
        u32 chain_index;
        u32 in_cluster_index;
        FATRawEntryList& list;
        RawFATEntry current_entry;

        FATEntryIterator& operator++() {
            VERIFY(chain_index < list.cluster_count);
            in_cluster_index++;
            if (in_cluster_index == list.entries_per_cluster) {
                in_cluster_index = 0;
                chain_index++;
            }
            // Get new value.
            if (chain_index != list.cluster_count) {
                current_entry = list.get_entry(chain_index, in_cluster_index);
            }

            // If end-of-directory, set to end.
            if (current_entry.type() == EntryType::EndOfDirectory) {
                chain_index = list.cluster_count;
                in_cluster_index = 0;
            }

            return *this;
        }

        bek::pair<RawFATEntry&, uSize> operator*() {
            return {current_entry, list.entries_per_cluster * chain_index + in_cluster_index};
        }
        friend bool operator!=(const FATEntryIterator& a, const FATEntryIterator& b) {
            return (a.chain_index != b.chain_index) || a.in_cluster_index != b.in_cluster_index;
        }
    };

    FATEntryIterator begin() {
        u32 chain_index = start_index / entries_per_cluster;
        u32 in_chain_index = start_index % entries_per_cluster;
        return {chain_index, in_chain_index, *this, get_entry(chain_index, in_chain_index)};
    }

    FATEntryIterator end() { return {static_cast<u32>(cluster_chain.size()), 0, *this, get_entry(0, 0)}; }

    static FATRawEntryList create(FileAllocationTable& fat, u32 start_cluster, u32 start_index = 0) {
        bek::vector<u32> chain;
        chain.push_back(start_cluster);
        auto next_cluster = fat.getNextCluster(start_cluster);
        while (get_cluster_type(next_cluster) == ClusterType::NextPointer) {
            chain.push_back(next_cluster);
            next_cluster = fat.getNextCluster(next_cluster);
        }
        VERIFY(start_index < chain.size() * fat.cluster_size() / 32);
        auto chain_size = chain.size();
        return FATRawEntryList{bek::move(chain), fat, static_cast<u32>(chain_size), fat.cluster_size() / 32,
                               start_index};
    }

    bek::vector<u32> cluster_chain;
    FileAllocationTable& fat;
    u32 cluster_count;
    uSize entries_per_cluster;
    u32 start_index = 0;
    RawFATEntry get_entry(u32 chain_index, u32 entry_index) {
        BitwiseObjectBuffer<RawFATEntry> result{{.data = {}}};
        // FIXME: Error Handling
        fat.doDataInterchange(result, cluster_chain[chain_index], 32 * entry_index, 32, false);
        return result.object();
    }
    void set_entry(u32 directory_index, const RawFATEntry& entry) {
        u32 chain_index = directory_index / entries_per_cluster;
        u32 entry_index = directory_index % entries_per_cluster;

        BitwiseObjectBuffer<RawFATEntry> result{entry};
        // FIXME: Error Handling
        fat.doDataInterchange(result, cluster_chain[chain_index], 32 * entry_index, 32, true);
    }
};

bek::vector<short_name> list_short_names(FileAllocationTable& fat, u32 start_cluster) {
    bek::vector<short_name> result;
    for (auto [e, _] : FATRawEntryList::create(fat, start_cluster)) {
        if (e.type() == EntryType::Normal) {
            result.push_back(e.item_entry.fatname);
        }
    }
    return result;
}

inline u64 cluster_byte_addr(FATInfo& info, u32 cluster_i) {
    VERIFY(cluster_i >= 2);
    return (info.data_begin_sector + info.sectors_per_cluster * (cluster_i - 2)) * (u64)info.sector_size;
}

inline u64 fat_sector_byte_addr(FATInfo& info, u64 sector_n) {
    return (info.fat_begin_sector + sector_n) * static_cast<u64>(info.sector_size);
}

constexpr uSize CLUSTER_CACHE_MAX = 10;
constexpr uSize FAT_SECTOR_CACHE_MAX = 10;

FileAllocationTable::FileAllocationTable(FATInfo info, blk::BlockDevice& device)
    : m_cluster_cache{CLUSTER_CACHE_MAX,
                      [this](u32 c, bek::shared_ptr<BlockCacheItem> x) { purge_cluster(c, bek::move(x)); }},
      m_fat_cache(FAT_SECTOR_CACHE_MAX,
                  [this](u32 s, bek::shared_ptr<BlockCacheItem> x) { purge_fat_sector(s, bek::move(x)); }),
      m_info(info),
      m_device(device),
      m_cluster_sectors{m_info.sectors_per_cluster} {}

bek::shared_ptr<BlockCacheItem> FileAllocationTable::fetch_fat_sector(u32 sector_n) {
    VERIFY(sector_n < m_info.fat_sectors);
    auto p_sector = m_fat_cache.find(sector_n);
    while (!p_sector) {
        auto sector = BlockCacheItem::create(m_info.sector_size);
        VERIFY(sector);
        auto res = blk::blocking_read(m_device, (m_info.fat_begin_sector + sector_n) * m_info.sector_size,
                                      bek::mut_buffer{reinterpret_cast<char*>(sector->data()), m_info.sector_size});
        VERIFY(res == blk::TransferResult::Success);

        // If already there, we've been overtaken (should this even be possible?)
        if (m_fat_cache.set(sector_n, sector)) {
            p_sector = bek::move(sector);
        } else {
            p_sector = m_fat_cache.find(sector_n);
        }
    }

    return p_sector;
}

unsigned int FileAllocationTable::getNextCluster(unsigned int current_cluster) {
    // FIXME: FAT12 etc.
    auto [sector_i, offset] = fat_sector_index_and_offset_for(m_info, current_cluster);  // NOLINT
    // Extract from cache if possible.
    auto sector = fetch_fat_sector(sector_i);
    return extract_fat_value(m_info, offset, sector->data());
}

unsigned int FileAllocationTable::allocateNextCluster(unsigned int current_cluster) {
    // bek::locker locker(m_lock);
    unsigned int first_checked_entry = bek::max(2u, m_free_cluster_hint);
    // FIXME: FAT12
    const u32 total_clusters = get_fat_entry_count(m_info);
    u32 current_sector_i = fat_sector_index_and_offset_for(m_info, first_checked_entry).first;
    auto current_sector = fetch_fat_sector(current_sector_i);

    for (u32 candidate_cluster = first_checked_entry; candidate_cluster < total_clusters; candidate_cluster++) {
        auto [sector_i, offset] = fat_sector_index_and_offset_for(m_info, candidate_cluster);  // NOLINT
        if (sector_i != current_sector_i) {
            current_sector_i = sector_i;
            current_sector = fetch_fat_sector(current_sector_i);
        }

        auto next_cluster = extract_fat_value(m_info, offset, current_sector->data());

        if (get_cluster_type(next_cluster) != ClusterType::Free) continue;

        set_fat_value(m_info, offset, current_sector->data(), CLUSTER_EOC);
        current_sector->set_whole_dirty();

        m_free_cluster_hint = candidate_cluster + 1;
        if (m_free_cluster_hint >= total_clusters) {
            m_free_cluster_hint = 2;
        }

        auto [original_sector_i, original_offset] = fat_sector_index_and_offset_for(m_info, current_cluster);  // NOLINT
        auto sector = fetch_fat_sector(original_sector_i);
        set_fat_value(m_info, original_offset, sector->data(), candidate_cluster);
        sector->set_whole_dirty();
        return candidate_cluster;
    }
    // Error
    return 0;
}

bool FileAllocationTable::extendFile(unsigned int start_cluster, uSize size) {
    // bek::locker locker(m_lock);
    uSize bytes_per_cluster = m_info.sector_size * m_info.sectors_per_cluster;
    uSize current_size = bytes_per_cluster;
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

bek::vector<LocatedFATEntry> FileAllocationTable::get_root_entries() {
    if (m_info.fat_type == FatType::FAT16 || m_info.fat_type == FatType::FAT12) {
        // Fat Entries are a set number of entries in rigid space.
        auto root_info = m_info.root_info.root_dir_attrs_16;

        // FIXME: Assumes Little-Endian
        bek::vector<RawFATEntry> entries{root_info.entry_count};
        blk::blocking_read(m_device, root_info.start_sector * m_info.sector_size, entries.buffer());

        bek::vector<LocatedFATEntry> res_entries;
        bek::vector<RawFATEntry> working_entry;
        FATEntryLocation working_location{0, 0};
        u32 i = 0;
        for (auto& e : entries) {
            if (e.type() == EntryType::LongFileName) {
                if (working_entry.size() == 0 && !(e.lfn_entry.order & 0x40)) {
                    // We expect the start of a LFN, but are midway through
                    // Discard as invalid.
                    continue;
                } else {
                    if (!working_entry.size()) {
                        working_location.index_in_directory = i;
                    }
                    working_entry.push_back(e);
                }
            } else if (e.type() == EntryType::Normal) {
                // This finalises an entry.
                if (!working_entry.size()) {
                    working_location.index_in_directory = i;
                }
                working_entry.push_back(e);
                PackedFATEntry entry{bek::move(working_entry)};
                working_entry = {};
                auto basic_entry = entry.to_basic();
                if (basic_entry.has_value()) {
                    DBG::dbgln("Found Entry: {}"_sv, basic_entry.value().name.view());
                    res_entries.push_back(LocatedFATEntry{basic_entry.release_value(), working_location});
                } else {
                    DBG::dbgln("Directory Enumeration Error: bad entry."_sv);
                }
            } else if (e.type() == EntryType::EndOfDirectory) {
                break;
            }
            i++;
        }
        return res_entries;
    } else {
        // FAT 32 simply placed in a special cluster.
        return get_entries(m_info.root_info.root_dir_cluster);
    }
}

bek::vector<LocatedFATEntry> FileAllocationTable::get_entries(u32 start_cluster) {
    bek::vector<LocatedFATEntry> entries;
    bek::vector<RawFATEntry> working_entry;
    FATEntryLocation working_location{start_cluster, 0};
    for (auto [e, i] : FATRawEntryList::create(*this, start_cluster)) {
        if (e.type() == EntryType::LongFileName) {
            if (working_entry.size() == 0 && !(e.lfn_entry.order & 0x40)) {
                // We expect the start of a LFN, but are midway through
                // Discard as invalid.
                continue;
            } else {
                if (!working_entry.size()) {
                    working_location.index_in_directory = i;
                }
                working_entry.push_back(e);
            }
        } else if (e.type() == EntryType::Normal) {
            // This finalises an entry.
            if (!working_entry.size()) {
                working_location.index_in_directory = i;
            }
            working_entry.push_back(e);
            PackedFATEntry entry{bek::move(working_entry)};
            auto basic_entry = entry.to_basic();
            if (basic_entry.has_value()) {
                entries.push_back(LocatedFATEntry{basic_entry.release_value(), working_location});
            } else {
                DBG::dbgln("Directory Enumeration Error: bad entry."_sv);
            }
        }
    }
    return entries;
}

void FileAllocationTable::purge_cluster(u32 cluster_n, bek::shared_ptr<BlockCacheItem> c) {
    // TODO: Queue write operation appropriately.
    if (c->is_dirty()) {
        auto [start, end] = c->dirty_range();
        DBG::dbgln("Writing dirty cluster {} to disk (bytes {} to {})."_sv, cluster_n, start, end);
        bek::buffer to_transfer = {reinterpret_cast<const char*>(c->data()), cluster_size()};
        to_transfer = to_transfer.subdivide(start, end - start);
        blk::blocking_write(m_device, cluster_byte_addr(m_info, cluster_n) + start, to_transfer);
        c->clear_dirty();
    }
    // Destruction will follow...
}

void FileAllocationTable::purge_fat_sector(uSize fat_sector, bek::shared_ptr<BlockCacheItem> sector) {
    // TODO: Queue write operation appropriately.
    if (sector->is_dirty()) {
        DBG::dbgln("Writing dirty FAT sector {} to disk."_sv, fat_sector);
        blk::blocking_write(m_device, fat_sector_byte_addr(m_info, fat_sector),
                            bek::buffer{reinterpret_cast<const char*>(sector->data()), m_info.sector_size});
        sector->clear_dirty();
    }
    // Destruction will follow...
}

expected<BasicFATEntry> FileAllocationTable::get_entry(FATEntryLocation location) {
    bek::vector<RawFATEntry> working_entry;
    // FATEntryLocation working_location{location.directory_start_cluster, location.index_in_directory};
    for (auto [e, i] : FATRawEntryList::create(*this, location.directory_start_cluster, location.index_in_directory)) {
        if (e.type() == EntryType::LongFileName) {
            if (working_entry.size() == 0 && !(e.lfn_entry.order & 0x40)) {
                // We expect the start of a LFN, but are midway through
                return EINVAL;
            } else {
                if (!working_entry.size()) {
                    // working_location.index_in_directory = i;
                }
                working_entry.push_back(e);
            }
        } else if (e.type() == EntryType::Normal) {
            // This finalises an entry.
            if (!working_entry.size()) {
                // working_location.index_in_directory = i;
            }
            working_entry.push_back(e);
            PackedFATEntry entry{bek::move(working_entry)};
            auto basic_entry = entry.to_basic();
            if (basic_entry.has_value()) {
                return basic_entry.release_value();
            } else {
                DBG::dbgln("Directory Enumeration Error: bad entry."_sv);
                return EINVAL;
            }
        }
    }
    DBG::dbgln("Reached end of directory looking for entry at location ({}, {})."_sv, location.directory_start_cluster,
               location.index_in_directory);
    return EINVAL;
}

expected<FATEntryLocation> FileAllocationTable::update_entry(FATEntryLocation location, const BasicFATEntry& entry,
                                                             bool update_name) {
    // FIXME: Implement name updates.
    VERIFY(!update_name);
    auto list = FATRawEntryList::create(*this, location.directory_start_cluster, location.index_in_directory);
    for (auto [e, i] : list) {
        if (e.type() == EntryType::Normal) {
            // We assume this is the one!
            e.item_entry = RawFATItemEntry::from(e.item_entry.fatname, entry);
            list.set_entry(i, e);
            return location;
        }
    }
    DBG::dbgln("Did not find normal entry to update."_sv);
    return EIO;
}
bool FileAllocationTable::doDataInterchange(TransactionalBuffer& buffer, unsigned int start_cluster, uSize offset,
                                            uSize size, bool write) {
    // Find start cluster
    uSize cluster_offset = offset / cluster_size();
    uSize within_cluster_offset = offset % cluster_size();

    // Progress over the cluster offset.
    for (unsigned int c = 0; c < cluster_offset; c++) {
        start_cluster = getNextCluster(start_cluster);
        if (get_cluster_type(start_cluster) != ClusterType::NextPointer) {
            // TODO: Extend?
            return false;
        }
    }

    auto current_cluster = start_cluster;

    uSize completed_size = 0;
    while (completed_size < size) {
        // Transfer (rest of) cluster

        // If first cluster, take into account within cluster offset (otherwise, start at
        // beginning).
        uSize byte_offset = (completed_size == 0) ? within_cluster_offset : 0;

        // Min of rest of cluster and rest of size to transfer
        uSize len_to_copy = bek::min(size - completed_size, cluster_size() - byte_offset);

        // TODO: Error Handling

        // Do we need to read cluster first?
        bool needs_content = (!write) | (byte_offset == 0 && len_to_copy == cluster_size());
        // Promptly destroy cluster.
        {
            auto cluster = fetch_cluster(current_cluster, needs_content);
            if (write) {
                buffer.read_to(cluster->data() + byte_offset, len_to_copy, completed_size);
                cluster->add_dirty_region(byte_offset, byte_offset + len_to_copy);
            } else {
                buffer.write_from(cluster->data() + byte_offset, len_to_copy, completed_size);
            }
        }

        completed_size += len_to_copy;
        if (completed_size == size) return true;

        // Next cluster
        current_cluster = getNextCluster(current_cluster);
        if (get_cluster_type(current_cluster) != ClusterType::NextPointer) {
            // TODO: Extend?
            return false;
        }
    }
    return true;
}
bek::shared_ptr<BlockCacheItem> FileAllocationTable::fetch_cluster(u32 cluster_n, bool needs_content) {
    auto cluster = m_cluster_cache.find(cluster_n);
    while (!cluster) {
        auto loaded_cluster = BlockCacheItem::create(cluster_size());
        VERIFY(loaded_cluster);

        if (needs_content) {
            bek::mut_buffer mut_buffer{reinterpret_cast<char*>(loaded_cluster->data()), cluster_size()};
            auto res = blk::blocking_read(m_device, cluster_byte_addr(m_info, cluster_n), mut_buffer);
            VERIFY(res == blk::TransferResult::Success);
        }

        // If already there, we've been overtaken (should this even be possible?)
        if (m_cluster_cache.set(cluster_n, loaded_cluster)) {
            cluster = bek::move(loaded_cluster);
        } else {
            cluster = m_cluster_cache.find(cluster_n);
        }
    }
    return cluster;
}
