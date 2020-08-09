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

#include <library/utility.h>
#include "filesystem/device.h"


bool BlockDevice::readBytes(unsigned long start, void *buffer, unsigned long count) {
    auto first_offset = start % block_size();
    auto first_block = start / block_size();
    auto last_block = start + count / block_size();
    auto out = static_cast<u8*>(buffer);
    for (auto current_block = first_block; current_block <= last_block; current_block++) {
        auto offset = (current_block == first_block) ? first_offset : 0;
        auto bytes_to_copy = bek::min(block_size() - offset, count);
        if (!readBlock(current_block, out, offset, bytes_to_copy)) return false;
        count -= bytes_to_copy;
        out += bytes_to_copy;
    }
    return true;
}

bool BlockDevice::writeBytes(unsigned long start, const void *buffer, unsigned long count) {
    auto first_offset = start % block_size();
    auto first_block = start / block_size();
    auto last_block = start + count / block_size();
    auto out = static_cast<const u8*>(buffer);
    for (auto current_block = first_block; current_block <= last_block; current_block++) {
        auto offset = (current_block == first_block) ? first_offset : 0;
        auto bytes_to_copy = bek::min(block_size() - offset, count);
        if (!writeBlock(current_block, out, offset, bytes_to_copy)) return false;
        count -= bytes_to_copy;
        out += bytes_to_copy;
    }
    return true;
}
