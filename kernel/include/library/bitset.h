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

#ifndef BEKOS_BITSET_H
#define BEKOS_BITSET_H

#include "bek/assertions.h"
#include "bek/buffer.h"
#include "bek/types.h"
#include "optional.h"

namespace bek {

class bitset_view {
public:
    bitset_view(char* data, uSize length) : m_data{data}, m_length{length} {}
    explicit bitset_view(bek::mut_buffer data) : m_data{data.data()}, m_length{data.size() * 8} {}

    [[nodiscard]] bool get(uSize index) const {
        ASSERT(index < m_length);
        return (m_data[index / 8] & (1u << (index % 8))) != 0;
    }

    void set(uSize index, bool val) {
        ASSERT(index < m_length);
        if (val) {
            m_data[index / 8] |= (1u << (index % 8));
        } else {
            m_data[index / 8] &= ~(1u << (index % 8));
        }
    }

    void set_range(uSize start, uSize count, bool val) {
        ASSERT(start + count < m_length);
        uSize end = start + count;
        // Align Start
        while (start % 8 && start < end) {
            set(start++, val);
        }

        // Align End
        while (end & 8 && start < end) {
            set(--end, val);
        }

        // Now aligned, so do full
        while (start < end) {
            m_data[start / 8] = (val) ? 0xFF : 0x00;
            start += 8;
        }
    }

    uSize get_region_size(uSize start_index, bool val, uSize max_size = 0xFFFFFFFFFFFFFFFF) const {
        uSize region_max_size = bek::min(max_size, m_length - start_index);
        uSize end             = start_index + region_max_size;
        uSize aligned_end     = (end / 8) * 8;
        uSize start           = start_index;
        bool cease            = false;
        // Align Start
        while (start % 8 && start < end) {
            if (get(start) != val) {
                cease = true;
                break;
            }
            start++;
        }
        if (cease) return start - start_index;

        while (start < aligned_end) {
            if (m_data[start / 8] != (val ? 0xFF : 0x00)) {
                break;
            }
            start += 8;
        }

        // Either, we've ceased prematurely (byte not uniform) or ceased to do unaligned checking at
        // end.
        while (start < end) {
            if (get(start) != val) {
                break;
            }
            start++;
        }
        return start - start_index;
    }

    struct fit_result_t {
        uSize index;
        uSize size;

        constexpr static fit_result_t invalid() { return {0, 0}; }

        [[nodiscard]] constexpr bool is_invalid() const { return (index == 0) && (size == 0); }
    };

    [[nodiscard]] fit_result_t find_first_fit(uSize length, bool val, uSize hint = 0,
                                              uSize alignment = 1, uSize alignment_offset = 0,
                                              uSize max_size = 0xFFFFFFFFFFFFFFFF) const {
        // Align-up hint.
        VERIFY(alignment_offset < alignment);
        uSize start_index = align_up(hint + alignment_offset, alignment) - alignment_offset;
        // Search in units of alignment
        while (start_index < m_length) {
            if (get(start_index) == val) {
                uSize region_size = get_region_size(start_index, val, max_size);
                if (region_size >= length) {
                    return {start_index, region_size};
                }
                start_index += region_size;
            } else {
                uSize bad_region_size = get_region_size(start_index, !val);
                bad_region_size       = bek::align_up(bad_region_size, alignment);
                start_index += bad_region_size;
            }
        }
        return fit_result_t::invalid();
    }

private:
    char* m_data;
    /// In bits.
    uSize m_length;
};

}  // namespace bek

#endif  // BEKOS_BITSET_H
