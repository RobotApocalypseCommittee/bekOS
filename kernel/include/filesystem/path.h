/*
 * bekOS is a basic OS for the Raspberry Pi
 * Copyright (C) 2024 Bekos Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BEKOS_PATH_H
#define BEKOS_PATH_H

#include "bek/span.h"
namespace fs {

class path {
public:
    static expected<path> parse_path(const bek::string& path_string) {
        // First, we do basic treatment of path.
        auto* cursor = path_string.data();
        auto* end = path_string.data() + path_string.size();

        bek::optional<bek::str_view> disk_specifier{};
        if (cursor != end && *cursor == '/') {
            // Absolute
            cursor++;
            if (cursor != end && *cursor == '(') {
                // Disk Specifier: /(blah)/
                cursor++;
                auto* disk_start = cursor;
                while (cursor != end && *cursor != ')') {
                    cursor++;
                }
                disk_specifier = bek::str_view{disk_start, static_cast<uSize>(cursor - disk_start)};
                // Parse )/
                if (*cursor != ')') {
                    // Unclosed
                    return EINVAL;
                }
                cursor++;
                if (*cursor != '/') {
                    return EINVAL;
                }
                cursor++;
            }
        }

        // Now, just general path parsing.
        bek::vector<bek::str_view> segments;
        auto* segment_start = cursor;
        while (cursor != end) {
            if (*cursor == '/') {
                // End of segment
                if (cursor > (segment_start)) {
                    segments.push_back(bek::str_view{segment_start, static_cast<uSize>(cursor - segment_start)});
                }
                segment_start = cursor + 1;
            }
            cursor++;
        }
        if (cursor > (segment_start)) {
            segments.push_back(bek::str_view{segment_start, static_cast<uSize>(cursor - segment_start)});
        }
        return path{bek::move(path_string), bek::move(segments), bek::move(disk_specifier)};
    }

    bool is_absolute() const { return m_path_string.data()[0] == '/'; }
    bek::optional<bek::str_view> disk_specifier() const { return m_disk_specifier; }
    bek::span<bek::str_view> segments() const { return bek::span<bek::str_view>{m_segments}; }
    bek::str_view view() const { return m_path_string.view(); }

private:
    path(const bek::string& path_string, bek::vector<bek::str_view> segments,
         bek::optional<bek::str_view> disk_specifier)
        : m_path_string(path_string), m_segments(bek::move(segments)), m_disk_specifier(bek::move(disk_specifier)) {}

    const bek::string& m_path_string;
    bek::vector<bek::str_view> m_segments;
    bek::optional<bek::str_view> m_disk_specifier;
};

}  // namespace fs

#endif  // BEKOS_PATH_H
