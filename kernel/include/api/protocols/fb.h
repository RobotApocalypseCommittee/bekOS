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

#ifndef BEKOS_API_FB_H
#define BEKOS_API_FB_H

#include "bek/types.h"

namespace protocol::fb {

enum MessageKind {
    GetDisplayInfo,
    SetDisplayInfo,
    MapFramebuffer,
    UnmapFramebuffer,
    FlushRect,
    MessageKindEnd_,
};

enum ColourFormatColour : u8 { R, G, B, A, X };

#define FB_COLOUR_FORMAT_SUBELEMENT(COLOUR, WIDTH) \
    static_cast<u32>(ColourFormatColour::COLOUR | (static_cast<u8>(WIDTH) << 3))

#define FB_COLOUR_FORMAT_ELEMENT(C1, W1, C2, W2, C3, W3, C4, W4)                          \
    ((FB_COLOUR_FORMAT_SUBELEMENT(C1, W1)) | (FB_COLOUR_FORMAT_SUBELEMENT(C2, W2) << 8) | \
     (FB_COLOUR_FORMAT_SUBELEMENT(C3, W3) << 16) | (FB_COLOUR_FORMAT_SUBELEMENT(C4, W4) << 24))

#define FB_COLOUR_FORMAT_ENTRY3(C1, W1, C2, W2, C3, W3) \
    C1##W1##C2##W2##C3##W3 = FB_COLOUR_FORMAT_ELEMENT(C1, W1, C2, W2, C3, W3, X, 0)

#define FB_COLOUR_FORMAT_ENTRY4(C1, W1, C2, W2, C3, W3, C4, W4) \
    C1##W1##C2##W2##C3##W3##C4##W4 = FB_COLOUR_FORMAT_ELEMENT(C1, W1, C2, W2, C3, W3, C4, W4)

enum ColourFormat {
    FB_COLOUR_FORMAT_ENTRY3(R, 8, G, 8, B, 8),
    FB_COLOUR_FORMAT_ENTRY4(R, 8, G, 8, B, 8, A, 8),
    FB_COLOUR_FORMAT_ENTRY4(R, 8, G, 8, B, 8, X, 8),
    FB_COLOUR_FORMAT_ENTRY4(A, 8, R, 8, G, 8, B, 8),
    FB_COLOUR_FORMAT_ENTRY4(X, 8, R, 8, G, 8, B, 8),
    FB_COLOUR_FORMAT_ENTRY4(B, 8, G, 8, R, 8, A, 8),
    FB_COLOUR_FORMAT_ENTRY4(B, 8, G, 8, R, 8, X, 8),
    FB_COLOUR_FORMAT_ENTRY4(A, 8, B, 8, G, 8, R, 8),
    FB_COLOUR_FORMAT_ENTRY4(X, 8, B, 8, G, 8, R, 8),
};

constexpr u64 colour_format_bit_width(ColourFormat f) {
    u64 v = 0;
    for (int i = 0; i < 4; i++) {
        v += (f >> (8 * i + 3)) & 0b11111;
    }
    return v;
}

static_assert(colour_format_bit_width(R8G8B8) == 24);
static_assert(colour_format_bit_width(R8G8B8A8) == 32);

struct DisplayInfo {
    u16 height;
    u16 width;
    ColourFormat colour_format;
    bool is_double_buffered;
    bool supports_flush;
};

struct Rect {
    u16 x;
    u16 y;
    u16 height;
    u16 width;
};

struct DisplayInfoMessage {
    MessageKind kind;
    DisplayInfo info;
};

struct MapMessage {
    MessageKind kind;
    uPtr buffer;
    uSize size;
    u16 pixel_width;
    u16 pixel_height;
    u16 row_stride;
};

struct FlushRectMessage {
    MessageKind kind;
    Rect rect;
};

}  // namespace protocol::fb

#endif  // BEKOS_API_FB_H
