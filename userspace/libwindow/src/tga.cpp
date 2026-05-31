// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025-2026 Bekos Contributors
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

#include <core/file.h>

#include "window/gfx.h"

namespace window {

namespace {
constexpr uSize TGA_HEADER_SIZE = 18;
constexpr u8 TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOUR = 2;
constexpr u8 TGA_DESCRIPTOR_TOP_LEFT_BIT = 1u << 5;
}  // namespace

core::expected<OwningBitmap> load_tga(bek::str_view path) {
    auto buf = EXPECTED_TRY(core::read_file(path));
    if (buf.size() < TGA_HEADER_SIZE) {
        return EINVAL;
    }

    const u8* h = buf.data();
    u8 id_length = h[0];
    u8 image_type = h[2];
    u16 width = static_cast<u16>(h[12]) | (static_cast<u16>(h[13]) << 8);
    u16 height = static_cast<u16>(h[14]) | (static_cast<u16>(h[15]) << 8);
    u8 depth = h[16];
    u8 descriptor = h[17];

    if (image_type != TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOUR) {
        return EINVAL;
    }
    if (depth != 24 && depth != 32) {
        return EINVAL;
    }
    if (width == 0 || height == 0) {
        return EINVAL;
    }

    uSize bytes_per_pixel = depth / 8;
    uSize pixel_data_offset = TGA_HEADER_SIZE + id_length;
    uSize pixel_data_size = static_cast<uSize>(width) * height * bytes_per_pixel;
    if (pixel_data_offset + pixel_data_size > buf.size()) {
        return EINVAL;
    }

    auto bitmap = EXPECTED_TRY(OwningBitmap::create(width, height));

    bool top_left_origin = (descriptor & TGA_DESCRIPTOR_TOP_LEFT_BIT) != 0;
    const u8* src = buf.data() + pixel_data_offset;

    for (u32 src_row = 0; src_row < height; src_row++) {
        u32 dst_row = top_left_origin ? src_row : (height - 1 - src_row);
        const u8* row_src = src + static_cast<uSize>(src_row) * width * bytes_per_pixel;
        u32* row_dst = bitmap.pixel_at(0, dst_row);
        if (depth == 24) {
            for (u32 x = 0; x < width; x++) {
                u8 b = row_src[x * 3 + 0];
                u8 g = row_src[x * 3 + 1];
                u8 r = row_src[x * 3 + 2];
                row_dst[x] = (0xFFu << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | b;
            }
        } else {
            for (u32 x = 0; x < width; x++) {
                u8 b = row_src[x * 4 + 0];
                u8 g = row_src[x * 4 + 1];
                u8 r = row_src[x * 4 + 2];
                u8 a = row_src[x * 4 + 3];
                row_dst[x] = (static_cast<u32>(a) << 24) | (static_cast<u32>(r) << 16) |
                             (static_cast<u32>(g) << 8) | b;
            }
        }
    }

    return bitmap;
}

}  // namespace window
